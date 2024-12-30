#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include "Ensure.h"
#include "SerialPort.h"


namespace {

void validateSysCallResult(int r)
{
    const auto valid = -1 != r || (-1 == r && EINTR == errno);
    ENSURE(valid, CRuntimeError);
}

void dump(std::ostream &os, const uint8_t *begin, const uint8_t *const end)
{
    const auto flags = os.flags();
    auto num = 0;

    while(begin != end)
    {
        const int value = *begin;
        ++begin;

        if(0 == value)
        {
            ++num;
            if(begin != end) continue;
        }

        if(0 < num)
        {
            if(1 < num) os << std::dec << num << 'x';
            os << std::hex << std::setw(2) << std::setfill('0') << 0;
            if(begin != end) os << ' ';
            num = 0;
        }

        if(0 != value) os << std::hex << std::setw(2) << std::setfill('0') << value;
        if(begin != end) os << ' ';
    }
    os.flags(flags);
}

void debug(
    std::ostream *dst,
    const char *tag,
    SerialPort::Clock::duration lastOpDiff,
    SerialPort::Clock::duration lastOpDuration,
    const uint8_t *begin, const uint8_t *const end,
    const uint8_t *const curr)
{
    if(!dst) return;

    using namespace std::chrono;

    const auto timeout = curr == begin;

    (*dst) << tag << ' '
        << duration_cast<milliseconds>(lastOpDiff).count() << "ms "
        << duration_cast<microseconds>(lastOpDuration).count() << "us ("
        << curr - begin << ") ";
    dump(*dst, begin, end);
    if(timeout) (*dst) << " timeout";
    (*dst) << '\n';
}

} /* namespace */

SerialPort::SerialPort(
    FdGuard fdGuard,
    BaudRate baudRate, Parity parity, DataBits dataBits, StopBits stopBits,
    std::ostream *debugTo):
        debugTo_{debugTo},
        baudRate_{baudRate},
        parity_{parity},
        dataBits_{dataBits},
        stopBits_{stopBits},
        fdGuard_{std::move(fdGuard)}
{
    Settings settings;
    getSettings(settings);
    settingsBackup_ = settings;
    modifySettings(settings, baudRate_, parity_, dataBits_, stopBits_);
    setSettings(settings);
    /* flush in/out buffers */
    flush();
    lastTimestamp_  = Clock::now();
}

SerialPort::SerialPort(
    std::string device,
    BaudRate baudRate, Parity parity, DataBits dataBits, StopBits stopBits,
    std::ostream *debugTo):
    SerialPort
    {
        FdGuard{std::move(device), O_RDWR | O_NONBLOCK},
        baudRate, parity, dataBits, stopBits,
        debugTo
    }
{}

SerialPort::~SerialPort()
{
    TRACE(
        TraceLevel::Debug,
        "rxCntr ", rxCntr_, ", txCntr ", txCntr_,
        ", rxTotalCntr ", rxTotalCntr_, ", txTotalCntr ", txTotalCntr_);

    if(fdGuard_)
    {
        flush();
        (void)::tcsetattr(fdGuard_.fd(), TCSANOW, &settingsBackup_);
    }
}

void SerialPort::getSettings(Settings &settings, int fd)
{
    ::memset(&settings, 0, sizeof(struct termios));
    ENSURE(-1 != ::tcgetattr(fd, &settings), CRuntimeError);
}

void SerialPort::modifySettings(
    Settings &settings,
    BaudRate baudRate, Parity parity, DataBits dataBits, StopBits stopBits)
{
    // BaudRate
    {
        cfsetispeed(&settings, static_cast<::speed_t>(baudRate));
        cfsetospeed(&settings, static_cast<::speed_t>(baudRate));
    }
    // Parity
    {
        if(Parity::None == parity)
        {
            settings.c_cflag &= ~PARENB;
            settings.c_iflag &= ~INPCK;
        }
        else
        {
            settings.c_iflag |= INPCK;
            //settings.c_iflag &= ~IGNPAR;
            settings.c_cflag |= PARENB;

            if(Parity::Odd == parity) settings.c_cflag |= PARODD;
            else if(Parity::Even == parity) settings.c_cflag &= ~PARODD;
        }
    }
    // DataBits
    {
        settings.c_cflag &= ~CSIZE;
        if(DataBits::Five == dataBits) settings.c_cflag |= CS5;
        else if(DataBits::Six == dataBits) settings.c_cflag |= CS6;
        else if(DataBits::Seven == dataBits) settings.c_cflag |= CS7;
        else if(DataBits::Eight == dataBits) settings.c_cflag |= CS8;
    }
    // StopBits
    {
        if(StopBits::One == stopBits) settings.c_cflag &= ~CSTOPB;
        else if(StopBits::Two == stopBits) settings.c_cflag |= CSTOPB;
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
}

void SerialPort::setSettings(int fd, const Settings &settings)
{
    ENSURE(-1 != ::tcsetattr(fd, TCSANOW, &settings), CRuntimeError);
    /* tcsetattr() returns success if any of the requested changes could be
     * successfully carried out - validate that all requested settings are applied */
    Settings current;
    getSettings(current, fd);
    ENSURE(0 == memcmp(&settings, &current, sizeof(Settings)), RuntimeError);
}

uint8_t *SerialPort::read(uint8_t *begin, const uint8_t *const end, mSecs timeout)
{
    ASSERT(fdGuard_);
    ENSURE(mSecs{0} <= timeout, RuntimeError);

    struct pollfd events =
    {
        fdGuard_.fd(),
        short(POLLIN) /* events */,
        short(0) /* revents */
    };

    using namespace std::chrono;

    const auto startTimestamp = Clock::now();
    const auto lastOpDiff = startTimestamp - lastTimestamp_;
    mSecs elapsed{0};
    auto curr = begin;

    while(curr != end && timeout >= elapsed)
    {
        {
            auto r = ::poll(&events, 1, (timeout - elapsed).count());

            // ignore poll interrupted by received signal
            validateSysCallResult(r);
            elapsed = duration_cast<mSecs>(Clock::now() - startTimestamp);
            /* fd is not ready for reading */
            if(0 == r) continue;
            if(0 == (events.revents & POLLIN)) continue;
        }

        {
            auto r = ::read(fdGuard_.fd(), curr, end - curr);

            validateSysCallResult(r);
            ENSURE(0 != r, RuntimeError);
            std::advance(curr, r);
            rxCntr_ += r;
            rxTotalCntr_ += r;
        }
    }

    const auto now = Clock::now();
    lastTimestamp_ = now;
    debug(debugTo_, __FUNCTION__, lastOpDiff, now - startTimestamp, begin, end, curr);
    return curr;
}

const uint8_t *SerialPort::write(const uint8_t *begin, const uint8_t *const end, mSecs timeout)
{
    ASSERT(fdGuard_);
    ENSURE(mSecs{0} <= timeout, RuntimeError);

    struct pollfd events =
    {
        fdGuard_.fd(),
        short(POLLOUT) /* events */,
        short(0) /* revents */
    };

    using namespace std::chrono;

    const auto startTimestamp = Clock::now();
    const auto lastOpDiff = startTimestamp - lastTimestamp_;
    mSecs elapsed{0};
    auto curr = begin;

    while(curr != end && timeout >= elapsed)
    {
        {
            auto r = ::poll(&events, 1, (timeout - elapsed).count());

            // ignore poll interrupted by received signal
            validateSysCallResult(r);
            elapsed = duration_cast<mSecs>(Clock::now() - startTimestamp);
            /* fd is not ready for reading */
            if(0 == r) continue;
            if(0 == (events.revents & POLLOUT)) continue;
        }

        {
            auto r = ::write(fdGuard_.fd(), curr, end - curr);

            validateSysCallResult(r);
            ENSURE(0 != r, RuntimeError);
            std::advance(curr, r);
            txCntr_ += r;
            txTotalCntr_ += r;
        }
    }

    const auto now = Clock::now();
    lastTimestamp_ = now;
    debug(debugTo_, __FUNCTION__, lastOpDiff, now - startTimestamp, begin, end, curr);
    return curr;
}

void SerialPort::drain(int fd)
{
    ENSURE(-1 != fd, RuntimeError);
    ENSURE(-1 != ::tcdrain(fd), CRuntimeError);
}

void SerialPort::flush(int fd)
{
    ENSURE(-1 != fd, RuntimeError);
    ENSURE(-1 != ::tcflush(fd, TCIOFLUSH), CRuntimeError);
}

void SerialPort::rxFlush(int fd)
{
    ENSURE(-1 != fd, RuntimeError);
    ENSURE(-1 != ::tcflush(fd, TCIFLUSH), CRuntimeError);
}

void SerialPort::txFlush(int fd)
{
    ENSURE(-1 != fd, RuntimeError);
    ENSURE(-1 != ::tcflush(fd, TCOFLUSH), CRuntimeError);
}

SerialPort::BaudRate toBaudRate(const std::string &rate)
{
    using BaudRate = SerialPort::BaudRate;

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
    using Parity = SerialPort::Parity;

    if("N" == parity) return Parity::None;
    else if("O" == parity) return Parity::Odd;
    else if("E" == parity) return Parity::Even;

    TRACE(TraceLevel::Warning, "unsupported parity, ", parity);

    return Parity::Even;
}
