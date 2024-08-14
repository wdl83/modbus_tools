#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <vector>

#include "Ensure.h"
#include "SerialPort.h"

namespace Modbus {
namespace RTU {

struct Addr
{
    uint8_t value;

    static constexpr uint8_t min = 1;
    static constexpr uint8_t max = 255;

    Addr(uint8_t addr): value{addr}
    {}

    friend
    std::ostream &operator<<(std::ostream &os, Addr addr)
    {
        os << int(addr.value);
        return os;
    }
};

/* MODBUS over serial line specification and implementation guide V1.02
 * recommended value:
 * - silent interval (3.5t) : 1750us
 * - inter frame silent interval (1.5t): 750us
 * impl. https://github.com/wdl83/modbus_c/
 * uses timeout = 1.5t + 3.5t to confirm End Of Frame.
 * So interFrameTimeout is set to:
 * 1.5t + 3.5t + processing_margin = 750us + 1750us + 500us = 3000us.
 * */
inline
std::chrono::microseconds interFrameTimeout() { return std::chrono::microseconds{3000}; }

using mSecs = std::chrono::milliseconds;

struct Master
{
    using BaudRate = SerialPort::BaudRate;
    using Parity = SerialPort::Parity;
    using DataBits = SerialPort::DataBits;
    using StopBits = SerialPort::StopBits;
    using DataSeq = std::vector<uint16_t>;
    using ByteSeq = std::vector<uint8_t>;
private:
    class DebugScope
    {
        Master &master_;
    public:
        DebugScope(Master &master): master_{master} {}
        ~DebugScope();
    };

    std::ostringstream debugTo_;
    std::string devName_;
    BaudRate baudRate_;
    Parity parity_;
    DataBits dataBits_;
    StopBits stopBits_;
    std::unique_ptr<SerialPort> dev_;
    std::chrono::steady_clock::time_point timestamp_;

    void initDevice();
    void drainDevice();
    void flushDevice();
    uint8_t *readDevice(uint8_t *begin, const uint8_t *const end, mSecs timeout);
    const uint8_t *writeDevice(const uint8_t *begin, const uint8_t *const end, mSecs timeout);
    void updateTiming();
    void ensureTiming();
public:
    Master(
        std::string devName,
        BaudRate baudRate = BaudRate::BR_19200,
        Parity parity = Parity::Even,
        DataBits dataBits = DataBits::Eight,
        StopBits stopBits = StopBits::One);
    SerialPort &device();
    void wrCoil(Addr slaveAddr, uint16_t memAddr, bool data, mSecs timeout);
    void wrRegister(Addr slaveAddr, uint16_t memAddr, uint16_t data, mSecs timeout);
    void wrRegisters(Addr slaveAddr, uint16_t memAddr, const DataSeq &data, mSecs timeout);
    DataSeq rdCoils(Addr slaveAddr, uint16_t memAddr, uint16_t count, mSecs timeout);
    DataSeq rdRegisters(Addr slaveAddr, uint16_t memAddr, uint8_t count, mSecs timeout);
    void wrBytes(Addr slaveAddr, uint16_t memAddr, const ByteSeq &data, mSecs timeout);
    ByteSeq rdBytes(Addr slaveAddr, uint16_t memAddr, uint8_t count, mSecs timeout);
};

} /* RTU */
} /* Modbus */
