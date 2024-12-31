#include <cstdlib>
#include <cstring>

#include "Ensure.h"
#include "Trace.h"
#include "util.h"


ScopedSignalHandler::ScopedSignalHandler(int signalNo, SignalHandler handler):
    signalNo_{signalNo}
{
    ASSERT(handler);
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    action.sa_handler = handler;
    action.sa_flags = 0;
    ENSURE(0 == ::sigaction(signalNo, &action, &backup_), CRuntimeError);
    TRACE(TraceLevel::Debug, '[', strsignal(signalNo), "] #", signalNo);
}

ScopedSignalHandler::~ScopedSignalHandler()
{
    if(0 != ::sigaction(signalNo_, &backup_, nullptr))
    {
        TRACE(TraceLevel::Error, strerror(errno));
        std::abort();
    }
}
