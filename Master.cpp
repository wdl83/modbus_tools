#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "Master.h"
#include "Except.h"
#include "crc.h"

namespace Modbus {
namespace RTU {
namespace {

using ByteSeq = Master::ByteSeq;
using DataSeq  = Master::DataSeq;

constexpr const uint8_t FCODE_RD_COILS = 1;
constexpr const uint8_t FCODE_RD_HOLDING_REGISTERS = 3;
constexpr const uint8_t FCODE_WR_COIL = 5;
constexpr const uint8_t FCODE_WR_REGISTER = 6;
constexpr const uint8_t FCODE_WR_REGISTERS = 16;
constexpr const uint8_t FCODE_USER1_OFFSET = 65;
constexpr const uint8_t FCODE_RD_BYTES = FCODE_USER1_OFFSET + 0;
constexpr const uint8_t FCODE_WR_BYTES = FCODE_USER1_OFFSET + 1;

const auto gDebug = ::getenv("DEBUG") ? bool(::atoi(getenv("DEBUG"))) : false;

uint8_t lowByte(uint16_t word) { return word & 0xFF; }
uint8_t highByte(uint16_t word) { return word >> 8; }

void dump(std::ostream &os, uint8_t data)
{
    os << std::hex << std::setw(2) << std::setfill('0') << int(data);
}

void dump(std::ostream &os, const uint8_t *begin, const uint8_t *const end)
{
    const auto flags = os.flags();

    while(begin != end)
    {
        dump(os, *begin);
        ++begin;
        if(begin != end) os << ' ';
    }
    os.flags(flags);
}

enum class DataSource
{
    Master, Slave
};

std::string dump(
    DataSource dataSource,
    const char *tag,
    int line,
    const uint8_t *begin, const uint8_t *const end,
    const uint8_t *const curr)
{
    std::ostringstream oss;

    if(DataSource::Master == dataSource) oss << ">";
    else if(DataSource::Slave == dataSource) oss << "<";

    if(curr != end)
    {
        if(begin == curr) oss << " timeout\n";
        else if(
            std::distance(begin, curr) == 4 /* addr + fcode + crc */
            && 0x80 < *std::next(begin))
        {
            oss
                << " exception fcode " << int(*std::next(begin)) << "\n";
        }
        else if(
            std::distance(begin, curr) == 5 /* addr + fcode + ecode + crc */
            && 0x80 < *std::next(begin))
        {
            oss
                << " exception fcode " << int(*std::next(begin))
                << " ecode " << int(*std::next(begin, 2)) << "\n";
        }
        else
        {
            oss
                << " unsupported (partial reply?)"
                << ", length " << curr - begin
                << ", expected " << end - begin << "\n";
        }
    }
    dump(oss, begin, end);
    if(gDebug) oss << ", " << line << ":" << tag;
    oss << '\n';

    auto str = oss.str();

    if(gDebug) std::cout << str;
    return str;
}

ByteSeq &append(ByteSeq &seq, uint16_t word)
{
    seq.push_back(highByte(word));
    seq.push_back(lowByte(word));
    return seq;
}

ByteSeq toByteSeq(const DataSeq &dataSeq)
{
    ByteSeq byteSeq;

    for(auto data : dataSeq) append(byteSeq, data);
    return byteSeq;
}

DataSeq toDataSeq(const ByteSeq &byteSeq, bool zeroPadding = false)
{
    ENSURE(0u == (byteSeq.size() & 1) || zeroPadding, RuntimeError);

    const auto end = std::end(byteSeq);
    DataSeq dataSeq;

    for(auto i = std::begin(byteSeq); i != end;)
    {
        const uint16_t highByte = *i++;
        const uint16_t lowByte = i == end ? UINT8_C(0) : *i++;
        dataSeq.push_back((highByte << 8) | lowByte);
    }
    return dataSeq;
}

void append(ByteSeq &dst, const ByteSeq &src)
{
    dst.insert(std::end(dst), std::begin(src), std::end(src));
}

void append(ByteSeq &dst, const DataSeq &src)
{
    append(dst, toByteSeq(src));
}

ByteSeq &appendCRC(ByteSeq &seq)
{
    auto begin = seq.data();
    auto end = begin + seq.size();
    auto crc = calcCRC(begin, end);

    seq.push_back(crc.lowByte());
    seq.push_back(crc.highByte());
    return seq;
}

void validateCRC(const ByteSeq &seq)
{
    if(seq.empty()) return;

    ENSURE(2u < seq.size(), CRCError);

    const CRC recvValue{*seq.rbegin(), *std::next(seq.rbegin())};

    const auto begin = seq.data();
    auto calcValue = calcCRC(begin, std::next(begin, seq.size() - 2));

    if(gDebug)
    {
        const auto flags = std::cout.flags();
        std::cout << "rCRC ";
        dump(std::cout, recvValue.highByte());
        dump(std::cout, recvValue.lowByte());
        std::cout << " cCRC ";
        dump(std::cout, calcValue.highByte());
        dump(std::cout, calcValue.lowByte());
        std::cout << "\n";
        std::cout.flags(flags);
    }

    ENSURE(recvValue.value == calcValue.value, CRCError);
}

} /* namespace */

void Master::initDevice()
{
    if(dev_) return;

    dev_ =
        std::make_unique<SerialPort>(devName_, baudRate_, parity_, dataBits_, stopBits_);
    ENSURE(dev_, RuntimeError);
    updateTiming();
}

void Master::drainDevice()
{
    initDevice();
    try
    {
        dev_->drain();
    }
    catch(CRuntimeError &except)
    {
        dev_.reset();
        throw;
    }
    catch(RuntimeError &exept)
    {
        dev_.reset();
        throw;
    }
}

uint8_t *Master::readDevice(uint8_t *begin, const uint8_t *const end, mSecs timeout)
{
    initDevice();
    try
    {
        ensureTiming();
        auto *r = dev_->read(begin, end, timeout);
        updateTiming();
        return r;
    }
    catch(CRuntimeError &except)
    {
        dev_.reset();
        throw;
    }
    catch(RuntimeError &exept)
    {
        dev_.reset();
        throw;
    }
}

const uint8_t *Master::writeDevice(const uint8_t *begin, const uint8_t *const end, mSecs timeout)
{
    initDevice();
    try
    {
        ensureTiming();
        const auto *r = dev_->write(begin, end, timeout);
        updateTiming();
        return r;
    }
    catch(CRuntimeError &except)
    {
        dev_.reset();
        throw;
    }
    catch(RuntimeError &exept)
    {
        dev_.reset();
        throw;
    }
}

SerialPort &Master::device()
{
    initDevice();
    return *dev_;
}

void Master::wrCoil(
    Addr slaveAddr,
    uint16_t memAddr,
    bool data,
    mSecs timeout)
{
    ByteSeq req
    {
        slaveAddr.value,
        FCODE_WR_COIL,
        highByte(memAddr),
        lowByte(memAddr),
        data ? static_cast<uint8_t>(0xFF) : static_cast<uint8_t>(0),
        UINT8_C(0)
    };

    appendCRC(req);

    // request
    {
        const auto reqBegin = req.data();
        const auto reqEnd = reqBegin + req.size();
        const auto r = writeDevice(reqBegin, reqEnd, mSecs{0});

        const auto debug = dump(DataSource::Master, __FUNCTION__, __LINE__, reqBegin, reqEnd, r);
        vENSURE(reqEnd == r, RequestError, debug);
    }

    drainDevice();

    constexpr const auto repSize =
        1 /* slave */
        + 1 /* fcode */
        + 2 /* address */
        + 2 /* value */
        + sizeof(CRC);
    ByteSeq rep(repSize, 0);

    // reply
    {
        const auto repBegin = rep.data();
        const auto repEnd = repBegin + rep.size();
        const auto r = readDevice(repBegin, repEnd, timeout);

        const auto debug = dump(DataSource::Slave, __FUNCTION__, __LINE__, repBegin, repEnd, r);
        vENSURE(repBegin != r, TimeoutError, debug);
    }

    validateCRC(rep);
    ENSURE(
        std::equal(
            std::begin(rep), std::next(std::begin(rep), rep.size() - sizeof(CRC)),
            std::begin(req)),
        ReplyError);
}

void Master::wrRegister(
    Addr slaveAddr,
    uint16_t memAddr,
    uint16_t data,
    mSecs timeout)
{
    ByteSeq req
    {
        slaveAddr.value,
        FCODE_WR_REGISTER,
        highByte(memAddr),
        lowByte(memAddr),
        highByte(data),
        lowByte(data)
    };

    const auto reqSize = req.size();

    appendCRC(req);

    // request
    {
        const auto reqBegin = req.data();
        const auto reqEnd = reqBegin + req.size();
        const auto r = writeDevice(reqBegin, reqEnd, mSecs{0});

        const auto debug = dump(DataSource::Master, __FUNCTION__, __LINE__, reqBegin, reqEnd, r);
        vENSURE(reqEnd == r, RequestError, debug);
    }

    drainDevice();

    ByteSeq rep(reqSize + sizeof(CRC), 0);

    // reply
    {
        const auto repBegin = rep.data();
        const auto repEnd = repBegin + rep.size();
        const auto r = readDevice(repBegin, repEnd, timeout);

        const auto debug = dump(DataSource::Slave, __FUNCTION__, __LINE__, repBegin, repEnd, r);
        vENSURE(repBegin != r, TimeoutError, debug);
    }

    validateCRC(rep);
    ENSURE(
        std::equal(
            std::begin(rep), std::next(std::begin(rep), rep.size() - sizeof(CRC)),
            std::begin(req)),
        ReplyError);
}

void Master::wrRegisters(
    Addr slaveAddr,
    uint16_t memAddr,
    const DataSeq &dataSeq,
    mSecs timeout)
{
    if(dataSeq.empty()) return;

    ENSURE(0x7C > dataSeq.size(), RuntimeError);

    ByteSeq req
    {
        slaveAddr.value,
        FCODE_WR_REGISTERS,
        highByte(memAddr), lowByte(memAddr), /* starting address */
        highByte(dataSeq.size()), lowByte(dataSeq.size()), /* quantity of registers */
        uint8_t(dataSeq.size() << 1) /* byte_count */
    };

    append(req, dataSeq);
    appendCRC(req);

    // request
    {
        const auto reqBegin = req.data();
        const auto reqEnd = reqBegin + req.size();
        const auto r = writeDevice(reqBegin, reqEnd, mSecs{0});

        const auto debug = dump(DataSource::Master, __FUNCTION__, __LINE__, reqBegin, reqEnd, r);
        vENSURE(reqEnd == r, RequestError, debug);
    }

    drainDevice();

    ByteSeq rep(
        1 /* addr */
        + 1 /* fcode */
        + 2 /* starting address */
        + 2 /* quantity of registers */
        + sizeof(CRC), 0);

    // reply
    {
        const auto repBegin = rep.data();
        const auto repEnd = repBegin + rep.size();
        const auto r = readDevice(repBegin, repEnd, timeout);

        const auto debug = dump(DataSource::Slave, __FUNCTION__, __LINE__, repBegin, repEnd, r);
        vENSURE(repBegin != r, TimeoutError, debug);
    }

    validateCRC(rep);
    ENSURE(
        std::equal(
            std::begin(rep), std::next(std::begin(rep), rep.size() - sizeof(CRC)),
            std::begin(req)),
        ReplyError);
}

DataSeq Master::rdCoils(
    Addr slaveAddr,
    uint16_t memAddr,
    uint16_t count,
    mSecs timeout)
{
    ENSURE(0 < count, RuntimeError);
    ENSURE(0x7D1 > count, RuntimeError);

    ByteSeq req
    {
        slaveAddr.value,
        FCODE_RD_COILS,
        highByte(memAddr),
        lowByte(memAddr),
        highByte(count),
        lowByte(count)
    };

    appendCRC(req);

    // request
    {
        const auto reqBegin = req.data();
        const auto reqEnd = reqBegin + req.size();
        const auto r = writeDevice(reqBegin, reqEnd, mSecs{0});

        const auto debug = dump(DataSource::Master, __FUNCTION__, __LINE__, reqBegin, reqEnd, r);
        vENSURE(reqEnd == r, RequestError, debug);
    }

    drainDevice();

    constexpr const auto repHeaderSize = 1 /* slave */ + 1 /* fcode */ + 1 /* byte count */;
    const auto payloadSize = (count >> 3) + (count & 0x7 ? 1 : 0);
    const auto repSize = repHeaderSize + payloadSize  /* data[] */ + sizeof(CRC);
    ByteSeq rep(repSize, 0);

    // reply
    {
        const auto repBegin = rep.data();
        const auto repEnd = repBegin + rep.size();
        const auto r = readDevice(repBegin, repEnd, timeout);

        const auto debug = dump(DataSource::Slave, __FUNCTION__, __LINE__, repBegin, repEnd, r);
        vENSURE(repBegin != r, TimeoutError, debug);
    }

    validateCRC(rep);
    ENSURE(rep[0] == slaveAddr.value, ReplyError);
    ENSURE(rep[1] == FCODE_RD_COILS, ReplyError);

    auto dataSeq =
        DataSeq(
            std::next(std::begin(rep), repHeaderSize),
            std::next(std::begin(rep), rep.size() - sizeof(CRC)));

    return dataSeq;
}

DataSeq Master::rdRegisters(
    Addr slaveAddr,
    uint16_t memAddr,
    uint8_t count,
    mSecs timeout)
{
    ENSURE(0 < count, RuntimeError);
    ENSURE(0x7E > count, RuntimeError);

    ByteSeq req
    {
        slaveAddr.value,
        FCODE_RD_HOLDING_REGISTERS,
        highByte(memAddr),
        lowByte(memAddr),
        highByte(count),
        lowByte(count)
    };

    appendCRC(req);

    // request
    {
        const auto reqBegin = req.data();
        const auto reqEnd = reqBegin + req.size();
        const auto r = writeDevice(reqBegin, reqEnd, mSecs{0});

        const auto debug = dump(DataSource::Master, __FUNCTION__, __LINE__, reqBegin, reqEnd, r);
        vENSURE(reqEnd == r, RequestError, debug);
    }

    drainDevice();

    constexpr const auto repHeaderSize = 1 /* slave */ + 1 /* fcode */ + 1 /* byte count */;
    const auto repSize = repHeaderSize + (count << 1)  /* data[] */ + sizeof(CRC);
    ByteSeq rep(repSize, 0);

    // reply
    {
        const auto repBegin = rep.data();
        const auto repEnd = repBegin + rep.size();
        const auto r = readDevice(repBegin, repEnd, timeout);

        const auto debug = dump(DataSource::Slave, __FUNCTION__, __LINE__, repBegin, repEnd, r);
        vENSURE(repBegin != r, TimeoutError, debug);
    }

    validateCRC(rep);
    ENSURE(rep[0] == slaveAddr.value, ReplyError);
    ENSURE(rep[1] == FCODE_RD_HOLDING_REGISTERS, ReplyError);

    auto dataSeq =
        toDataSeq(
            ByteSeq
            {
                std::next(std::begin(rep), repHeaderSize),
                std::next(std::begin(rep), rep.size() - sizeof(CRC))
            });
    return dataSeq;
}

void Master::wrBytes(
    Addr slaveAddr,
    uint16_t memAddr,
    const ByteSeq &byteSeq,
    mSecs timeout)
{
    if(byteSeq.empty()) return;

    ENSURE(250u > byteSeq.size(), RuntimeError);

    ByteSeq req
    {
        slaveAddr.value,
        FCODE_WR_BYTES,
        highByte(memAddr),
        lowByte(memAddr),
        uint8_t(byteSeq.size())
    };

    const auto reqSize = req.size();

    append(req, byteSeq);
    appendCRC(req);

    // request
    {
        const auto reqBegin = req.data();
        const auto reqEnd = reqBegin + req.size();
        const auto r = writeDevice(reqBegin, reqEnd, mSecs{0});

        const auto debug = dump(DataSource::Master, __FUNCTION__, __LINE__, reqBegin, reqEnd, r);
        vENSURE(reqEnd == r, RequestError, debug);
    }

    drainDevice();

    ByteSeq rep(reqSize + sizeof(CRC), 0);

    // reply
    {
        const auto repBegin = rep.data();
        const auto repEnd = repBegin + rep.size();
        const auto r = readDevice(repBegin, repEnd, timeout);

        const auto debug = dump(DataSource::Slave, __FUNCTION__, __LINE__, repBegin, repEnd, r);
        vENSURE(repBegin != r, TimeoutError, debug);
    }

    validateCRC(rep);
    ENSURE(
        std::equal(
            std::begin(rep), std::next(std::begin(rep), rep.size() - sizeof(CRC)),
            std::begin(req)),
        ReplyError);
}

ByteSeq Master::rdBytes(
    Addr slaveAddr,
    uint16_t memAddr,
    uint8_t count,
    mSecs timeout)
{
    ENSURE(0 < count, RuntimeError);
    ENSURE(250 > count, RuntimeError);

    ByteSeq req
    {
        slaveAddr.value,
        FCODE_RD_BYTES,
        highByte(memAddr),
        lowByte(memAddr),
        uint8_t(count)
    };

    const auto reqHeaderSize = req.size();

    appendCRC(req);

    // request
    {
        const auto reqBegin = req.data();
        const auto reqEnd = reqBegin + req.size();
        const auto r = writeDevice(reqBegin, reqEnd, mSecs{0});

        const auto debug = dump(DataSource::Master, __FUNCTION__, __LINE__, reqBegin, reqEnd, r);
        vENSURE(reqEnd == r, RequestError, debug);
    }

    drainDevice();

    const auto repHeaderSize = reqHeaderSize;
    const auto repSize = repHeaderSize + count  /* data[] */ + sizeof(CRC);
    ByteSeq rep(repSize, 0);

    // reply
    {
        const auto repBegin = rep.data();
        const auto repEnd = repBegin + rep.size();
        const auto r = readDevice(repBegin, repEnd, timeout);

        const auto debug = dump(DataSource::Slave, __FUNCTION__, __LINE__, repBegin, repEnd, r);
        vENSURE(repBegin != r, TimeoutError, debug);
    }

    validateCRC(rep);

    ENSURE(
        std::equal(
            std::begin(rep), std::next(std::begin(rep), repHeaderSize),
            std::begin(req)),
        ReplyError);

    ByteSeq dataSeq
    {
        std::next(std::begin(rep), repHeaderSize),
        std::next(std::begin(rep), rep.size() - sizeof(CRC))
    };

    return dataSeq;
}

void Master::updateTiming()
{
    timestamp_ = std::chrono::steady_clock::now();
}

void Master::ensureTiming()
{
    using namespace std::chrono;

    const auto now = steady_clock::now();

    ENSURE(now > timestamp_, RuntimeError);

    const auto diff =
        std::min(
            interFrameTimeout(),
            duration_cast<microseconds>(now - timestamp_));

    if(microseconds{0} < diff)
    {
        TRACE(TraceLevel::Debug, "waiting ", diff.count(), "us");
        std::this_thread::sleep_for(diff);
    }
}

} /* RTU */
} /* Modbus */
