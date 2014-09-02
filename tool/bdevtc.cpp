/**
 * bdevtc.cpp - control bdevt devices.
 *
 * (C) 2014, Cybozu Labs, Inc.
 * @author HOSHINO Takashi <hoshino@labs.cybozu.co.jp>
 */
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include "logger.h"
#include "ioctl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

using StrVec = std::vector<std::string>;
using Ss = std::stringstream;

const char * const ctlPath = "/dev/bdevt_ctl";

template <typename T>
void unused(T &) {}

class Exp : public std::exception
{
    std::string s_;
public:
    Exp(const std::string &s) : s_(s) {}
    const char *what() const noexcept {
        return s_.c_str();
    }
    template <typename T>
    Exp& operator<<(T&& t) {
        std::stringstream ss;
        ss << ":" << std::forward<T>(t);
        s_ += ss.str();
        return *this;
    }
};

uint64_t parseSize(const std::string &s)
{
    if (s.empty()) throw Exp("bad size") << s;
    char *end;
    uint64_t val = ::strtoul(s.data(), &end, 10);
    switch (*end) {
    case 'g': case 'G': val <<= 30; break;
    case 'm': case 'M': val <<= 20; break;
    case 'k': case 'K': val <<= 10; break;
    }
    return val;
}

class File
{
    int fd_;
    public:
    File() : fd_(-1) {}
    ~File() try {
        close();
    } catch (...) {
    }
    int fd() const {
        if (fd_ < 0) throw std::runtime_error("bad fd");
        return fd_;
    }
    void open(const std::string &path) {
        fd_ = ::open(path.c_str(), O_RDWR);
        if (fd_ < 0) throw Exp("open failed") << path;
    }
    void close() {
        if (fd_ > 0) {
            if (::close(fd_) != 0) throw Exp("close failed") << fd_;
            fd_ = -1;
        }
    }
};

void invokeIoctl(const std::string &path, struct bdevt_ctl &ctl)
{
    File file;
    file.open(path);
    if (::ioctl(file.fd(), BDEVT_IOCTL, &ctl) < 0) {
        throw std::runtime_error("ioctl failed.");
    }
    file.close();
}

void doCreate(const StrVec &params)
{
    if (params.empty()) throw std::runtime_error("specify size.");
    const uint64_t sizeLb = parseSize(params[0]) >> 9;

    struct bdevt_ctl ctl = {
        .command = BDEVT_IOCTL_START_DEV,
        .val_u64 = sizeLb,
    };
    invokeIoctl(ctlPath, ctl);
    std::cout << ctl.val_u32 << std::endl; // minor id.
}

void doDelete(const StrVec &params)
{
    if (params.empty()) throw std::runtime_error("specify device.");
    const std::string &devPath = params[0];

    struct bdevt_ctl ctl = {
        .command = BDEVT_IOCTL_STOP_DEV,
    };
    invokeIoctl(devPath, ctl);
}

#define NYI(func) \
    throw Exp("not yet implemented") << func

void doNumDev(const StrVec &params)
{
    unused(params);
    NYI(__func__);
}

void doGetMajor(const StrVec &params)
{
    unused(params);
    NYI(__func__);
}

void doMakeError(const StrVec &params)
{
    unused(params);
    NYI(__func__);
}

void doRecoverError(const StrVec &params)
{
    unused(params);
    NYI(__func__);
}

void doMakeCrash(const StrVec &params)
{
    unused(params);
    NYI(__func__);
}

void doRecoverCrash(const StrVec &params)
{
    unused(params);
    NYI(__func__);
}

void dispatch(int argc, char *argv[])
{
    struct {
        const char *cmd;
        void (*handler)(const StrVec &);
        const char *helpMsg;
    } tbl[] = {
        {"create", doCreate, "SIZE (with k/m/g)"},
        {"delete", doDelete, "DEV"},
        {"num-dev", doNumDev, ""},
        {"get-major", doGetMajor, ""},
        {"make-error", doMakeError, "QQQ"},
        {"recover-error", doRecoverError, "QQQ"},
        {"make-crash", doMakeCrash, "QQQ"},
        {"recover-crash", doRecoverCrash, "QQQ"},
    };

    if (argc < 2) {
        Ss ss;
        ss << "Usage:\n";
        for (size_t i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i++) {
            ss << "  " << tbl[i].cmd << " " << tbl[i].helpMsg << "\n";
        }
        std::cerr << ss.str();
        ::exit(1);
    }
    const std::string cmd(argv[1]);

    StrVec v;
    for (int i = 2; i < argc; i++) v.push_back(argv[i]);

    for (size_t i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i++) {
        if (cmd == tbl[i].cmd) {
            tbl[i].handler(v);
            return;
        }
    }
    throw Exp("command not found") << cmd;
}

int main(int argc, char *argv[]) try
{
    dispatch(argc, argv);
    return 0;
} catch (std::exception &e) {
    LOGe("error: %s\n", e.what());
    return 1;
} catch (...) {
    LOGe("error: unknown error\n");
    return 1;
}
