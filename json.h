#pragma once

#include <nlohmann/json.hpp>

#include "Master.h"

namespace Modbus {
namespace RTU {
namespace JSON {

using json = nlohmann::json;

json rdCoils(Master &master, Addr slave, mSecs timeout, const json &input);
json rdRegisters(Master &master, Addr slave, mSecs timeout, const json &input);
json wrCoil(Master &master, Addr slave, mSecs timeout, const json &input);
json wrRegister(Master &master, Addr slave, mSecs timeout, const json &input);
json wrRegisters(Master &master, Addr slave, mSecs timeout, const json &input);
json wrBytes(Master &master, Addr slave, mSecs timeout, const json &input);
json rdBytes(Master &master, Addr slave, mSecs timeout, const json &input);
void dispatch(Master &master, const json &input, json &output);

} /* JSON */
} /* RTU */
} /* Modbus */
