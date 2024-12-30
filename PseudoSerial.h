#pragma once

#include <ostream>

#include "SerialPort.h"

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
