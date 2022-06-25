#pragma once

#include "Ensure.h"

namespace Modbus {
namespace RTU {

using CRCError = EXCEPTION(std::runtime_error);
using ReplyError = EXCEPTION(std::runtime_error);
using RequestError = EXCEPTION(std::runtime_error);
using TagFormatError = EXCEPTION(std::runtime_error);
using TagMissingError = EXCEPTION(std::runtime_error);
using TimeoutError = EXCEPTION(std::runtime_error);
using TimingError = EXCEPTION(std::runtime_error);

}
}
