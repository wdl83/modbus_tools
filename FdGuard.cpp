#include <fcntl.h>

#include "Ensure.h"
#include "FdGuard.h"
#include "Trace.h"

FdGuard::FdGuard(int fd): fd_{fd}
{
    TRACE(TraceLevel::Trace, fd_);
}

FdGuard::FdGuard(std::string path, int flags):
    path_{std::move(path)}
{
    ENSURE(!path_.empty(), RuntimeError);
    fd_ = ::open(path_.c_str(), flags);
    ENSURE(-1 != fd_, CRuntimeError);
    TRACE(TraceLevel::Trace, fd_);
}
FdGuard::FdGuard(const char *path, int flags):
    FdGuard{std::string{path}, flags}
{}

FdGuard::FdGuard(FdGuard &&other) noexcept:
    fd_{other.fd_},
    path_{std::move(other.path_)}
{
    other.fd_ = -1;
    TRACE(TraceLevel::Trace, fd_);
}

FdGuard::~FdGuard()
{
    if(-1 != fd_)
    {
        TRACE(TraceLevel::Trace, fd_);
        (void)::close(fd_);
    }
}
