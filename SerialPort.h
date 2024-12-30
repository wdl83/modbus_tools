#pragma once

#include <chrono>
#include <ostream>
#include <string>

#include <termios.h>

#include "FdGuard.h"


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

    using uSecs = std::chrono::microseconds;
    using mSecs = std::chrono::milliseconds;
    using Clock = std::chrono::steady_clock;
    using Settings = struct termios;
private:
    std::ostream *debugTo_;
    BaudRate baudRate_;
    Parity parity_;
    DataBits dataBits_;
    StopBits stopBits_;
    FdGuard fdGuard_;
    Settings settingsBackup_;
    Clock::time_point lastTimestamp_;
    uint64_t rxCntr_{0};
    uint64_t txCntr_{0};
    uint64_t rxTotalCntr_{0};
    uint64_t txTotalCntr_{0};
public:
    explicit SerialPort(
        FdGuard,
        BaudRate, Parity, DataBits, StopBits,
        std::ostream *);
    explicit SerialPort(
        std::string device,
        BaudRate, Parity, DataBits, StopBits,
        std::ostream *);

    SerialPort(const SerialPort &) = delete;
    ~SerialPort();

    SerialPort &operator=(const SerialPort &) = delete;

    static void getSettings(Settings &, int fd);
    static void modifySettings(Settings &, BaudRate, Parity, DataBits, StopBits);
    static void setSettings(int fd, const Settings &);

    void getSettings(Settings &settings) const { getSettings(settings, fdGuard_.fd()); }
    void setSettings(const Settings &settings) { setSettings(fdGuard_.fd(), settings); }

    uint8_t *read(uint8_t *begin, const uint8_t *const end, mSecs timeout);
    const uint8_t *write(const uint8_t *begin, const uint8_t *const end, mSecs timeout);

    /* wait until data written is transmitted */
    static void drain(int fd);
    static void flush(int fd);
    static void rxFlush(int fd);
    static void txFlush(int fd);

    void drain() { drain(fdGuard_.fd()); }
    void flush() { flush(fdGuard_.fd()); }
    void rxFlush() { txFlush(fdGuard_.fd()); }
    void txFlush() { txFlush(fdGuard_.fd()); }

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


struct PseudoPair
{
    SerialPort master;
    SerialPort slave;
};

PseudoPair createPseudoPair(
    SerialPort::BaudRate, SerialPort::Parity,
    SerialPort::DataBits, SerialPort::StopBits,
    std::ostream *masterDbgTo = nullptr,
    std::ostream *slaveDbgTo = nullptr,
    const char *multiplexor = "/dev/ptmx");
