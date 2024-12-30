#include <cassert>
#include <fcntl.h>
#include <limits.h>

#include "Ensure.h"
#include "PseudoSerial.h"

PseudoPair createPseudoPair(
    SerialPort::BaudRate baudRate, SerialPort::Parity parity,
    SerialPort::DataBits dataBits, SerialPort::StopBits stopBits,
    std::ostream *masterDbgTo, std::ostream *slaveDbgTo,
    const char *multiplexor)
{
    assert(multiplexor);
    const int flags = O_RDWR | O_NONBLOCK;

    // master
    FdGuard mfd{multiplexor, flags};

    ENSURE(0 == ::grantpt(mfd.fd()), CRuntimeError);
    ENSURE(0 == ::unlockpt(mfd.fd()), CRuntimeError);

    // slave device path
    char spath[PATH_MAX];
    ENSURE(0 == ::ptsname_r(mfd.fd(), spath, sizeof(spath)), CRuntimeError);

    // slave
    FdGuard sfd{spath, flags};

    return
    {
        SerialPort{std::move(mfd), baudRate, parity, dataBits, stopBits, masterDbgTo},
        SerialPort{std::move(sfd), baudRate, parity, dataBits, stopBits, slaveDbgTo}
    };
}
