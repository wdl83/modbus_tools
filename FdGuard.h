#pragma once


class FdGuard
{
    int fd_{-1};
    std::string path_{};
public:
    explicit FdGuard(int fd);
    // use open syscall
    explicit FdGuard(std::string path, int flags);
    explicit FdGuard(const char *path, int flags);
    FdGuard(FdGuard &&) noexcept;
    FdGuard(const FdGuard &) = delete;
    ~FdGuard();
    FdGuard &operator=(const FdGuard &) = delete;
    explicit operator bool() const {return -1 != fd_; }

    int release()
    {
        auto rfd = fd_;
        fd_ = -1;
        return rfd;
    }

    int fd() const {return fd_;}
    const std::string &path() const {return path_;}
};
