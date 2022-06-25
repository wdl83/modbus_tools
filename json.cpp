#include <thread>

#include "Except.h"
#include "json.h"

namespace Modbus {
namespace RTU {
namespace JSON {

const char *const ADDR = "addr";
const char *const COUNT = "count";
const char *const FCODE = "fcode";
const char *const RETRY = "retry";
const char *const SLAVE = "slave";
const char *const TIMEOUT_MS = "timeout_ms";
const char *const VALUE = "value";

constexpr auto FCODE_RD_COILS = 1;
constexpr auto FCODE_RD_HOLDING_REGISTERS = 3;
constexpr auto FCODE_WR_COIL = 5;
constexpr auto FCODE_WR_REGISTER = 6;
constexpr auto FCODE_WR_REGISTERS = 16;
constexpr auto FCODE_RD_BYTES = 65;
constexpr auto FCODE_WR_BYTES = 66;

template <typename T, typename V>
bool inRange(V value)
{
    return
        std::numeric_limits<T>::min() <= value
        && std::numeric_limits<T>::max() >= value;
}

json rdCoils(Master &master, Addr slave, mSecs timeout, const json &input, int retryNum)
{
    ENSURE(input.count(ADDR), TagMissingError);
    ENSURE(input[ADDR].is_number(), TagFormatError);

    const auto addr = input[ADDR].get<int>();

    ENSURE(inRange<uint16_t>(addr), TagFormatError);

    ENSURE(input.count(COUNT), TagMissingError);
    ENSURE(input[COUNT].is_number(), TagFormatError);

    const auto count = input[COUNT].get<int>();

    ENSURE(inRange<uint16_t>(count), TagFormatError);
    /* this is Modbus V1.1b3 protocol requirement */
    ENSURE(2001 > count, TagFormatError);

    Master::DataSeq data;

    do
    {
        const auto warn = [&]()
        {
            TRACE(
                TraceLevel::Warning,
                " failed,"
                " retryNum ", retryNum,
                " addr ", slave,
                " data ", input.dump());
        };
        try { data = master.rdCoils(slave, addr, count, timeout); break; }
        catch(const TimeoutError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const CRCError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const ReplyError &) { --retryNum; warn(); if(!retryNum) throw; }
        std::this_thread::sleep_for(timeout);
    } while(retryNum);

    return json
    {
        {SLAVE, slave.value},
        {ADDR, addr},
        {COUNT, count},
        {VALUE, data}
    };
}

json rdRegisters(Master &master, Addr slave, mSecs timeout, const json &input, int retryNum)
{
    ENSURE(input.count(ADDR), TagMissingError);
    ENSURE(input[ADDR].is_number(), TagFormatError);

    const auto addr = input[ADDR].get<int>();

    ENSURE(inRange<uint16_t>(addr), TagFormatError);

    ENSURE(input.count(COUNT), TagMissingError);
    ENSURE(input[COUNT].is_number(), TagFormatError);

    const auto count = input[COUNT].get<int>();

    ENSURE(inRange<uint8_t>(count), TagFormatError);

    Master::DataSeq data;

    do
    {
        const auto warn = [&]()
        {
            TRACE(
                TraceLevel::Warning,
                " failed,"
                " retryNum ", retryNum,
                " addr ", slave,
                " data ", input.dump());
        };
        try { data = master.rdRegisters(slave, addr, count, timeout); break; }
        catch(const TimeoutError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const CRCError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const ReplyError &) { --retryNum; warn(); if(!retryNum) throw; }
        std::this_thread::sleep_for(timeout);
    } while(retryNum);

    return json
    {
        {SLAVE, slave.value},
        {ADDR, addr},
        {COUNT, count},
        {VALUE, data}
    };
}

json wrCoil(Master &master, Addr slave, mSecs timeout, const json &input, int retryNum)
{
    ENSURE(input.count(ADDR), TagMissingError);
    ENSURE(input[ADDR].is_number(), TagFormatError);

    const auto addr = input[ADDR].get<int>();

    ENSURE(inRange<uint16_t>(addr), TagFormatError);

    ENSURE(input.count(VALUE), TagMissingError);

    ENSURE(input[VALUE].is_boolean(), TagFormatError);

    const auto value = input[VALUE].get<bool>();

    do
    {
        const auto warn = [&]()
        {
            TRACE(
                TraceLevel::Warning,
                " failed,"
                " retryNum ", retryNum,
                " addr ", slave,
                " data ", input.dump());
        };
        try { master.wrCoil(slave, addr, value, timeout); break; }
        catch(const TimeoutError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const CRCError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const ReplyError &) { --retryNum; warn(); if(!retryNum) throw; }
        std::this_thread::sleep_for(timeout);
    } while(retryNum);

    return json
    {
        {SLAVE, slave.value},
        {ADDR, addr}
    };
}

json wrRegister(Master &master, Addr slave, mSecs timeout, const json &input, int retryNum)
{
    ENSURE(input.count(ADDR), TagMissingError);
    ENSURE(input[ADDR].is_number(), TagFormatError);

    const auto addr = input[ADDR].get<int>();

    ENSURE(inRange<uint16_t>(addr), TagFormatError);

    ENSURE(input.count(VALUE), TagMissingError);

    ENSURE(input[VALUE].is_number(), TagFormatError);

    const auto value = input[VALUE].get<int>();

    ENSURE(inRange<uint16_t>(value), TagFormatError);

    do
    {
        const auto warn = [&]()
        {
            TRACE(
                TraceLevel::Warning,
                " failed,"
                " retryNum ", retryNum,
                " addr ", slave,
                " data ", input.dump());
        };
        try { master.wrRegister(slave, addr, value, timeout); break; }
        catch(const TimeoutError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const CRCError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const ReplyError &) { --retryNum; warn(); if(!retryNum) throw; }
        std::this_thread::sleep_for(timeout);
    } while(retryNum);

    return json
    {
        {SLAVE, slave.value},
        {ADDR, addr}
    };
}

json wrRegisters(Master &master, Addr slave, mSecs timeout, const json &input, int retryNum)
{
    ENSURE(input.count(ADDR), TagMissingError);
    ENSURE(input[ADDR].is_number(), TagFormatError);

    const auto addr = input[ADDR].get<int>();

    ENSURE(inRange<uint16_t>(addr), TagFormatError);

    ENSURE(input.count(COUNT), TagMissingError);
    ENSURE(input[COUNT].is_number(), TagFormatError);

    const auto count = input[COUNT].get<int>();

    ENSURE(inRange<uint8_t>(count), TagFormatError);

    ENSURE(input.count(VALUE), TagMissingError);

    ENSURE(input[VALUE].is_array(), TagFormatError);

    const auto value = input[VALUE].get<std::vector<int>>();

    ENSURE(int(value.size()) == count, TagFormatError);

    Master::DataSeq seq(std::begin(value), std::end(value));

    do
    {
        const auto warn = [&]()
        {
            TRACE(
                TraceLevel::Warning,
                " failed,"
                " retryNum ", retryNum,
                " addr ", slave,
                " data ", input.dump());
        };
        try { master.wrRegisters(slave, addr, seq, timeout); break; }
        catch(const TimeoutError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const CRCError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const ReplyError &) { --retryNum; warn(); if(!retryNum) throw; }
        std::this_thread::sleep_for(timeout);
    } while(retryNum);

    return json
    {
        {SLAVE, slave.value},
        {ADDR, addr},
        {COUNT, count}
    };
}

json wrBytes(Master &master, Addr slave, mSecs timeout, const json &input, int retryNum)
{
    ENSURE(input.count(ADDR), TagMissingError);
    ENSURE(input[ADDR].is_number(), TagFormatError);

    const auto addr = input[ADDR].get<int>();

    ENSURE(inRange<uint16_t>(addr), TagFormatError);

    ENSURE(input.count(COUNT), TagMissingError);
    ENSURE(input[COUNT].is_number(), TagFormatError);

    const auto count = input[COUNT].get<int>();

    ENSURE(inRange<uint8_t>(count), TagFormatError);

    ENSURE(input.count(VALUE), TagMissingError);

    ENSURE(input[VALUE].is_array(), TagFormatError);

    const auto value = input[VALUE].get<std::vector<int>>();

    ENSURE(int(value.size()) == count, TagFormatError);

    Master::ByteSeq seq(std::begin(value), std::end(value));

    do
    {
        const auto warn = [&]()
        {
            TRACE(
                TraceLevel::Warning,
                " failed,"
                " retryNum ", retryNum,
                " addr ", slave,
                " data ", input.dump());
        };
        try { master.wrBytes(slave, addr, seq, timeout); break; }
        catch(const TimeoutError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const CRCError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const ReplyError &) { --retryNum; warn(); if(!retryNum) throw; }
        std::this_thread::sleep_for(timeout);
    } while(retryNum);

    return json
    {
        {SLAVE, slave.value},
        {ADDR, addr},
        {COUNT, count}
    };
}

json rdBytes(Master &master, Addr slave, mSecs timeout, const json &input, int retryNum)
{
    ENSURE(input.count(ADDR), TagMissingError);
    ENSURE(input[ADDR].is_number(), TagFormatError);

    const auto addr = input[ADDR].get<int>();

    ENSURE(inRange<uint16_t>(addr), TagFormatError);

    ENSURE(input.count(COUNT), TagMissingError);
    ENSURE(input[COUNT].is_number(), TagFormatError);

    const auto count = input[COUNT].get<int>();

    ENSURE(inRange<uint8_t>(count), TagFormatError);

    Master::ByteSeq data;

    do
    {
        const auto warn = [&]()
        {
            TRACE(
                TraceLevel::Warning,
                " failed,"
                " retryNum ", retryNum,
                " addr ", slave,
                " data ", input.dump());
        };
        try { data = master.rdBytes(slave, addr, count, timeout); break; }
        catch(const TimeoutError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const CRCError &) { --retryNum; warn(); if(!retryNum) throw; }
        catch(const ReplyError &) { --retryNum; warn(); if(!retryNum) throw; }
        std::this_thread::sleep_for(timeout);
    } while(retryNum);

    return json
    {
        {SLAVE, slave.value},
        {ADDR, addr},
        {COUNT, count},
        {VALUE, data}
    };
}

void dispatch(Master &master, const json &input, json &output)
{
    ENSURE(input.count(SLAVE), TagMissingError);
    ENSURE(input[SLAVE].is_number(), TagFormatError);

    const auto slave = input[SLAVE].get<int>();

    ENSURE(inRange<uint8_t>(slave), TagFormatError);

    /* default timeout @ 19200bps (default MODBUS RTU rate)
     * 256 bytes ADU (max size) is transmitted as 2816 bits (11bits / frame)
     * 11 bits == [start_bit | 8_data_bits | parity_bit | stop_bit]
     * 1bit takes 52,08us, 256bytes ~ 146666us ~ 147ms
     * worst case is 256 bytes Request + 256 byte Reply ~ 2x 147ms = 294ms */
    mSecs timeout{500};

    if(input.count(TIMEOUT_MS))
    {
        /* if timeout in milliseconds is provided - use it */
        ENSURE(input[TIMEOUT_MS].is_number(), TagFormatError);

        const int timeout_ms = input[TIMEOUT_MS].get<int>();

        ENSURE(0 < timeout_ms, TagFormatError);

        timeout = mSecs{timeout_ms};
    }

    auto retryNum = 1;

    if(input.count(RETRY))
    {
        ENSURE(input[RETRY].is_number(), TagFormatError);

        const auto retry = input[RETRY].get<int>();

        ENSURE(0 < retry, TagFormatError);

        retryNum = retry;
    }

    ENSURE(input.count(FCODE), TagMissingError);
    ENSURE(input[FCODE].is_number(), TagFormatError);

    const auto fcode = input[FCODE].get<int>();

    switch(fcode)
    {
        case FCODE_RD_COILS:
        {
            output.push_back(rdCoils(master, {uint8_t(slave)}, timeout, input, retryNum));
            break;
        }
        case FCODE_RD_HOLDING_REGISTERS:
        {
            output.push_back(rdRegisters(master, {uint8_t(slave)}, timeout, input, retryNum));
            break;
        }
        case FCODE_WR_COIL:
        {
            output.push_back(wrCoil(master, {uint8_t(slave)}, timeout, input, retryNum));
            break;
        }
        case FCODE_WR_REGISTER:
        {
            output.push_back(wrRegister(master, {uint8_t(slave)}, timeout, input, retryNum));
            break;
        }
        case FCODE_WR_REGISTERS:
        {
            output.push_back(wrRegisters(master, {uint8_t(slave)}, timeout, input, retryNum));
            break;
        }
        case FCODE_WR_BYTES:
        {
            output.push_back(wrBytes(master, {uint8_t(slave)}, timeout, input, retryNum));
            break;
        }
        case FCODE_RD_BYTES:
        {
            output.push_back(rdBytes(master, {uint8_t(slave)}, timeout, input, retryNum));
            break;
        }
        default:
        {
            ENSURE(false && "not supported fcode", RuntimeError);
            break;
        }
    }
}

} /* JSON */
} /* RTU */
} /* Modbus */
