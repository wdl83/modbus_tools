#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <cstring>

#include <iomanip>
#include <iostream>

#include "Ensure.h"
#include "SerialPort.h"

namespace {

const auto gDebug = nullptr != ::getenv("DEBUG_SERIAL");

void validateSysCallResult(int r)
{
    const auto valid = -1 != r || (-1 == r && EINTR == errno);
    ENSURE(valid, CRuntimeError);
}

void dump(std::ostream &os, const uint8_t *begin, const uint8_t *const end)
{
    const auto flags = os.flags();

    while(begin != end)
    {
        os
            << '['
            << std::hex << std::setw(2) << std::setfill('0') << int(*begin)
            << ']';
        ++begin;
    }
    os.flags(flags);
}

void debug(
    const char *tag,
    const uint8_t *begin, const uint8_t *const end,
    const uint8_t *const curr)
{
    if(!gDebug) return;

    const auto timeout = curr != end;

    dump(std::cout, begin, end);
    std::cout << " " << tag;
    if(timeout) std::cout << " TIMEOUT, length " << curr - begin;
    std::cout << std::endl;
}

} /* namespace */

namespace Modbus {

SerialPort::SerialPort(
    std::string devName,
    BaudRate baudRate,
    Parity parity,
    DataBits dataBits,
    StopBits stopBits):
        devName_{std::move(devName)},
        baudRate_{baudRate},
        parity_{parity},
        dataBits_{dataBits},
        stopBits_{stopBits}
{
    ENSURE(!devName_.empty(), RuntimeError);

    fd_ = ::open(devName_.c_str(), O_RDWR | O_NONBLOCK);

    ENSURE(-1 != fd_, CRuntimeError);

    struct termios settings;

    ::memset(&settings, 0, sizeof(struct termios));

    // get current settings
    ENSURE(-1 != ::tcgetattr(fd_, &settings), CRuntimeError);
    // backup settings
    settings_ = std::make_unique<struct termios>(settings);
    // BaudRate
    {
        cfsetispeed(&settings, static_cast<::speed_t>(baudRate_));
        cfsetospeed(&settings, static_cast<::speed_t>(baudRate_));
    }
    // Parity
    {
        if(Parity::None == parity_)
        {
            settings.c_cflag &= ~PARENB;
            settings.c_iflag &= ~INPCK;
        }
        else
        {
            settings.c_iflag |= INPCK;
            //settings.c_iflag &= ~IGNPAR;
            settings.c_cflag |= PARENB;

            if(Parity::Odd == parity_) settings.c_cflag |= PARODD;
            else if(Parity::Even == parity_) settings.c_cflag &= ~PARODD;
        }
    }
    // DataBits
    {
        settings.c_cflag &= ~CSIZE;
        if(DataBits::Five == dataBits_) settings.c_cflag |= CS5;
        else if(DataBits::Six == dataBits_) settings.c_cflag |= CS6;
        else if(DataBits::Seven == dataBits_) settings.c_cflag |= CS7;
        else if(DataBits::Eight == dataBits_) settings.c_cflag |= CS8;
    }
    // StopBits
    {
        if(StopBits::One == stopBits_) settings.c_cflag &= ~CSTOPB;
        else if(StopBits::Two == stopBits_) settings.c_cflag |= CSTOPB;
    }
    // Additional settings (similar to cfmakeraw())
    {
        settings.c_cflag &= ~CRTSCTS;
        // enable receiver
        settings.c_cflag |= CREAD;
        // ignore modem control lines
        settings.c_cflag |= CLOCAL;
        // disable canonical mode (line-by-line processing)
        settings.c_lflag &= ~ICANON;
        // disable input char echo
        settings.c_lflag &= ~ECHO;
        // disable special character interpretation
        settings.c_lflag &= ~ISIG;
        // disable SW flow control
        settings.c_iflag &= ~(IXON | IXOFF |IXANY);
        // disable spacial character processing
        settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
        // disable impl. defined output processing
        settings.c_oflag &= ~OPOST;
        // do not convert '\n' to '\r\n'
        settings.c_oflag &= ~ONLCR;
        // dont block read syscall (poll syscall will be used to monitor fd)
        settings.c_cc[VTIME] = 0;
        settings.c_cc[VMIN] = 0;
    }

    ENSURE(-1 != ::tcsetattr(fd_, TCSANOW, &settings), CRuntimeError);
    /* flush in/out buffers */
    ENSURE(-1 != ::tcflush(fd_, TCIOFLUSH), CRuntimeError);
}

SerialPort::~SerialPort()
{
    TRACE(
        TraceLevel::Info,
        "rxCntr ", rxCntr_, ", txCntr ", txCntr_,
        ", rxTotalCntr ", rxTotalCntr_, ", txTotalCntr ", txTotalCntr_);

    if(-1 != fd_ && settings_)
    {
        (void)::tcflush(fd_, TCIOFLUSH);
        (void)::tcsetattr(fd_, TCSANOW, settings_.get());
        (void)::close(fd_);
    }
}

uint8_t *SerialPort::read(uint8_t *begin, const uint8_t *const end, mSecs timeout)
{
    ENSURE(-1 != fd_, RuntimeError);

    struct pollfd events =
    {
        fd_,
        short(POLLIN) /* events */,
        short(0) /* revents */
    };

    using namespace std::chrono;

    const auto timestamp = steady_clock::now();
    mSecs elapsed{0};
    auto curr = begin;

    while(curr != end && timeout >= elapsed)
    {
        {
            auto r = ::poll(&events, 1, (timeout - elapsed).count());

            // ignore poll interrupted by received signal
            validateSysCallResult(r);
            elapsed = duration_cast<mSecs>(steady_clock::now() - timestamp);
            /* fd is not ready for reading */
            if(0 == r) continue;
            if(0 == (events.revents & POLLIN)) continue;
        }

        {
            auto r = ::read(fd_, curr, end - curr);

            validateSysCallResult(r);
            ENSURE(0 != r, RuntimeError);
            std::advance(curr, r);
            rxCntr_ += r;
            rxTotalCntr_ += r;
        }
    }

    debug(__FUNCTION__, begin, end, curr);
    return curr;
}

const uint8_t *SerialPort::write(const uint8_t *begin, const uint8_t *const end, mSecs timeout)
{
    ENSURE(-1 != fd_, RuntimeError);

    struct pollfd events =
    {
        fd_,
        short(POLLOUT) /* events */,
        short(0) /* revents */
    };

    using namespace std::chrono;

    const auto timestamp = steady_clock::now();
    mSecs elapsed{0};
    auto curr = begin;

    while(curr != end && timeout >= elapsed)
    {
        {
            auto r = ::poll(&events, 1, (timeout - elapsed).count());

            // ignore poll interrupted by received signal
            validateSysCallResult(r);
            elapsed = duration_cast<mSecs>(steady_clock::now() - timestamp);
            /* fd is not ready for reading */
            if(0 == r) continue;
            if(0 == (events.revents & POLLOUT)) continue;
        }

        {
            auto r = ::write(fd_, curr, end - curr);

            validateSysCallResult(r);
            ENSURE(0 != r, RuntimeError);
            std::advance(curr, r);
            txCntr_ += r;
            txTotalCntr_ += r;
        }
    }

    debug(__FUNCTION__, begin, end, curr);
    return curr;
}

void SerialPort::drain()
{
    ENSURE(-1 != fd_, RuntimeError);
    ENSURE(-1 != ::tcdrain(fd_), CRuntimeError);
}

void SerialPort::flush()
{
    ENSURE(-1 != fd_, RuntimeError);
    ENSURE(-1 != ::tcflush(fd_, TCIOFLUSH), CRuntimeError);
}

void SerialPort::rxFlush()
{
    ENSURE(-1 != fd_, RuntimeError);
    ENSURE(-1 != ::tcflush(fd_, TCIFLUSH), CRuntimeError);
}

void SerialPort::txFlush()
{
    ENSURE(-1 != fd_, RuntimeError);
    ENSURE(-1 != ::tcflush(fd_, TCOFLUSH), CRuntimeError);
}

SerialPort::BaudRate toBaudRate(const std::string &rate)
{
    using BaudRate = Modbus::SerialPort::BaudRate;

    if("1200" == rate) return BaudRate::BR_1200;
    else if("2400" == rate) return BaudRate::BR_2400;
    else if("4800" == rate) return BaudRate::BR_4800;
    else if("9600" == rate) return BaudRate::BR_9600;
    else if("19200" == rate) return BaudRate::BR_19200;
    else if("38400" == rate) return BaudRate::BR_38400;
    else if("57600" == rate) return BaudRate::BR_57600;
    else if("11520" == rate) return BaudRate::BR_115200;

    TRACE(TraceLevel::Warning, "unsupported rate, ", rate);

    return BaudRate::BR_19200;
}

SerialPort::Parity toParity(const std::string &parity)
{
    using Parity = Modbus::SerialPort::Parity;

    if("N" == parity) return Parity::None;
    else if("O" == parity) return Parity::Odd;
    else if("E" == parity) return Parity::Even;

    TRACE(TraceLevel::Warning, "unsupported parity, ", parity);

    return Parity::Even;
}

} /* Modbus */
