#pragma once

#include <chrono>
#include <memory>
#include <string>

#include <termios.h>

namespace Modbus {

struct SerialPort
{
    enum class BaudRate: speed_t
    {
        BR_1200 = B1200,
        BR_2400 = B2400,
        BR_4800 = B4800,
        BR_9600 = B9600,
        BR_19200 = B19200,
        BR_38400 = B38400,
        BR_57600 = B57600,
        BR_115200 = B115200
    };

    enum class Parity : char {None = 'N', Odd = 'O', Even = 'E'};
    enum class DataBits {Five = 5, Six = 6, Seven = 7, Eight = 8};
    enum class StopBits {One = 1, Two = 2};

    using mSecs = std::chrono::milliseconds;
private:
    std::string devName_;
    BaudRate baudRate_;
    Parity parity_;
    DataBits dataBits_;
    StopBits stopBits_;
    int fd_{-1};
    std::unique_ptr<struct termios> settings_;
    uint64_t rxCntr_{0};
    uint64_t txCntr_{0};
    uint64_t rxTotalCntr_{0};
    uint64_t txTotalCntr_{0};
public:
    SerialPort(std::string devName, BaudRate, Parity, DataBits, StopBits);
    ~SerialPort();
    uint8_t *read(uint8_t *begin, const uint8_t *const end, mSecs timeout);
    const uint8_t *write(const uint8_t *begin, const uint8_t *const end, mSecs timeout);
    /* wait until data written is transmitted */
    void drain();
    void flush();
    void rxFlush();
    void txFlush();

    uint64_t rxCntr() const {return rxCntr_;}
    uint64_t txCntr() const {return txCntr_;}

    void clearCntrs()
    {
        rxCntr_ = 0;
        txCntr_ = 0;
    }

    uint64_t rxTotalCntr() const {return rxTotalCntr_;}
    uint64_t txTotalCntr() const {return txTotalCntr_;}
};

SerialPort::BaudRate toBaudRate(const std::string &);
SerialPort::Parity toParity(const std::string &);

} /* Modbus */
