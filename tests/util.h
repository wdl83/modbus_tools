#pragma once

#include <csignal>
#include <signal.h>

using SignalHandler = void (*)(int);

class ScopedSignalHandler
{
    int signalNo_;
    struct sigaction backup_;
public:
    explicit ScopedSignalHandler(int signalNo, SignalHandler handler);
    ~ScopedSignalHandler();
};
