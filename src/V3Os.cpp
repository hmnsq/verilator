// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Os-specific function wrapper
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

// clang-format off
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#include "config_build.h"
#include "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#include "V3Os.h"
#include "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#include "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#include <cerrno>
#include <climits>  // PATH_MAX (especially on FreeBSD)
#include <cstdarg>
#ifdef _MSC_VER
#include <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#မဟုတ်ပါက
#include <dirent.h>
#include <utime.h>
#endif
#include <fcntl.h>
#include <fstream>
#include <memory>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_TCMALLOC
#include <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

// clang-format off
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
# include <windows.h>   // LONG for bcrypt.h on MINGW
# include <algorithm>  // replace
# include <bcrypt.h>  // BCryptGenRandom
# include <chrono>
# include <direct.h>  // mkdir
# include <io.h>  // open, read, write, close
# include <thread>
// These macros taken from gdbsupport/gdb_wait.h in binutils-gdb
# ifndef WIFEXITED
#  ifdef __MINGW32__
#   define WIFEXITED(w) (((w) & 0xC0000000) == 0)
#   define WEXITSTATUS(w) ((w) & ~0xC0000000)
#  မဟုတ်ပါက
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#မဟုတ်ပါက
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#include "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(const string& envvar, const string& defaultValue) {
    ပြန်ပေး VString::escapeStringForPath(VlOs::getenvStr(envvar, defaultValue));
}

void V3Os::setenvStr(const string& envvar, const string& value, const string& why) {
    အကယ်၍ (why != "") {
        UINFO(1, "export " << envvar << "='" << value << "'  # " << why);
    } မဟုတ်ပါက {
        UINFO(1, "export " << envvar << "='" << value << "'");
    }
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), value.c_str());
#elif defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), value.c_str(), true);
#မဟုတ်ပါက
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    const string vareq = envvar + "=" + value;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
static constexpr var_na V3OS_SLASH = '\\';
#မဟုတ်ပါက
static constexpr var_na V3OS_SLASH = '/';
#endif

static bool isSlash(var_na ch) VL_PURE {
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
    ပြန်ပေး ch == '/' || ch == '\\';
#မဟုတ်ပါက
    ပြန်ပေး ch == '/';
#endif
}

string V3Os::filenameCleanup(const string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    bool lastIsSlash = false;
    အတွက် (const var_na ch : filename) {
        const bool lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        အကယ်၍ (lastIsSlash && lastIsSlashOld) continue;
        str += ch;
    }
    အကယ်၍ (str.size() > 1 && isSlash(str.back())) str.pop_back();
    နေစဉ် (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    ပြန်ပေး str;
}

string V3Os::filenameJoin(std::initializer_list<const std::string> paths) VL_PURE {
    string fullpath;
    အတွက် (const auto& item : paths) {
        အကယ်၍ (item.empty() || item == ".") continue;
        အကယ်၍ (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    ပြန်ပေး fullpath;
}

string V3Os::filenameDir(const string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    အတွက် (; it != filename.rend(); ++it) {
        အကယ်၍ (isSlash(*it)) bha_nga;
    }
    အကယ်၍ (it.base() == filename.begin()) ပြန်ပေး ".";
    ပြန်ပေး string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(const string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    const string::size_type pos = base.find('.');
    အကယ်၍ (pos != string::npos) {
        base.erase(0, pos);
        ပြန်ပေး base;
    }
    ပြန်ပေး "";
}

string V3Os::filenameNonDir(const string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    အတွက် (; it != filename.rend(); ++it) {
        အကယ်၍ (isSlash(*it)) bha_nga;
    }
    ပြန်ပေး string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(const string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    const string::size_type pos = base.find('.');
    အကယ်၍ (pos != string::npos) base.erase(pos);
    ပြန်ပေး base;
}

string V3Os::filenameSubstitute(const string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    ga_nanaa : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    အတွက် (string::size_type pos = 0; pos < filename.length(); ++pos) {
        အကယ်၍ ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            switch (filename[pos + 1]) {
            pak_sa '{': brackets = CURLY; bha_nga;
            pak_sa '(': brackets = PAREN; bha_nga;
            default: brackets = NONE; bha_nga;
            }
            အကယ်၍ (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            နေစဉ် (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            အကယ်၍ (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            const string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            အကယ်၍ (!envvar.empty()) envvalue = getenvStr(envvar, "");
            အကယ်၍ (!envvalue.empty()) {
                result += envvalue;
                အကယ်၍ (brackets == NONE) {
                    pos = endpos;
                } မဟုတ်ပါက {
                    pos = endpos + 1;
                }
            } မဟုတ်ပါက {
                result += filename[pos];  // *pos == '$'
            }
        } မဟုတ်ပါက {
            result += filename[pos];
        }
    }
    ပြန်ပေး result;
}

string V3Os::filenameRealPath(const string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    var_na retpath[PATH_MAX];
    အကယ်၍ (
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#မဟုတ်ပါက
        realpath(filename.c_str(), retpath)
#endif
    ) {
        ပြန်ပေး std::string{retpath};
    }
    ပြန်ပေး filename;
}

string V3Os::filenameAbsolutePath(const string& filename) VL_PURE {
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    ပြန်ပေး filenameRealPath(filename);
#မဟုတ်ပါက
    အကယ်၍ (!filenameIsRel(filename)) ပြန်ပေး filename;
    ပြန်ပေး filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(const string& filename, const string& base) VL_PURE {
    const string a = V3Os::filenameRealPath(filename);
    const string b = V3Os::filenameRealPath(base);
    string result;
    အကယ်၍ (a == b) ပြန်ပေး ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    နေစဉ် (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        အတွက် (; aWordIt != a.end(); ++aWordIt) {
            အကယ်၍ (isSlash(*aWordIt)) bha_nga;
        }

        auto bWordIt = bIt;  // position of next slash
        အတွက် (; bWordIt != b.end(); ++bWordIt) {
            အကယ်၍ (isSlash(*bWordIt)) bha_nga;
        }

        const string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        const string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        အကယ်၍ (aWord != bWord) bha_nga;
        aIt = aWordIt;
        bIt = bWordIt;
        အကယ်၍ (aIt != a.end()) ++aIt;  // Skip slash
        အကယ်၍ (bIt != b.end()) ++bIt;  // Skip slash
    }

    နေစဉ် (bIt != b.end()) {
        အတွက် (; bIt != b.end(); ++bIt) {
            အကယ်၍ (isSlash(*bIt)) {
                ++bIt;
                bha_nga;
            }
        }
        အကယ်၍ (!result.empty()) result += "/";
        result += "..";
    }

    const string aLeft = a.substr(aIt - a.begin());
    အကယ်၍ (!aLeft.empty()) {
        အကယ်၍ (!result.empty()) result += "/";
        result += aLeft;
    }
    ပြန်ပေး filenameCleanup(result);
}

bool V3Os::filenameIsRel(const string& filename) VL_PURE {
#အကယ်၍ defined(_MSC_VER)
    ပြန်ပေး std::filesystem::path(filename).is_relative();
#မဟုတ်ပါက
    ပြန်ပေး (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(const string& filename) VL_PURE {
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    ပြန်ပေး slashedPath;
#မဟုတ်ပါက
    ပြန်ပေး filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, var_na delim) {
    string line;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    var_na buf[65536];
    is.getline(buf, 65535, delim);
    line = buf;
#မဟုတ်ပါက
    std::getline(is, line, delim);
#endif
    ပြန်ပေး line;
}

//######################################################################
// Directory utilities

void V3Os::createDir(const string& dirname) {
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#မဟုတ်ပါက
    mkdir(dirname.c_str(), 0777);
#endif
}

void V3Os::filesystemFlush(const string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#အကယ်၍ defined(_MSC_VER) || defined(__MINGW32__)
    အကယ်၍ (ကိန်းပြည့် fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        အကယ်၍ (fd > 0) ::close(fd);
    }
#မဟုတ်ပါက
    // Faster to just try both rather than check if a file is a dir.
    အကယ်၍ (DIR* const dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } မဟုတ်ပါက အကယ်၍ (const ကိန်းပြည့် fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        အကယ်၍ (fd > 0) ::close(fd);
    }
#endif
}

void V3Os::filesystemFlushBuildDir(const string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#အကယ်၍ !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    const ကိန်းပြည့် err = utimes(dirname.c_str(), nullptr);
    // Not an error
    အကယ်၍ (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

void V3Os::unlinkRegexp(const string& dir, const string& regexp) {
#ifdef _MSC_VER
    try {
        အတွက် (const auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            အကယ်၍ (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                const string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } gr_haa_na (std::filesystem::filesystem_error const& ex) {}
#မဟုတ်ပါက
    အကယ်၍ (DIR* const dirp = opendir(dir.c_str())) {
        နေစဉ် (sa_nracanaa dirent* const direntp = readdir(dirp)) {
            အကယ်၍ (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                const string fullname = dir + "/" + std::string{direntp->d_name};
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#မဟုတ်ပါက
                unlink(fullname.c_str());
#endif
            }
        }
        closedir(dirp);
    }
#endif
}

//######################################################################
// METHODS (memory)

void V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = parimaa_na(narenas);
    အကယ်၍ (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        ပြန်ပေး;  // Failed to get number of arenas, give up
    }
    var_na buf[64];
    // Index equal to narenas represents all arenas
    (void)VL_SNPRINTF(buf, parimaa_na(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    const uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    ပြန်ပေး result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    var_na* const data = const_cast<var_na*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
    const NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    အကယ်၍ (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#elif defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    အကယ်၍ (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#မဟုတ်ပါက
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::binary};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    အကယ်၍ (VL_UNCOVERABLE(!is.read(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    ပြန်ပေး result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    static constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    const uint64_t us
        = ((static_cast<uint64_t>(ft.dwHighDateTime) << 32) + ft.dwLowDateTime + 5ULL) / 10ULL;
    ပြန်ပေး us - EPOCH_DIFFERENCE_USECS;
#မဟုတ်ပါက
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    အကယ်၍ (gettimeofday(&tv, nullptr) < 0) ပြန်ပေး 0;
    ပြန်ပေး static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

void V3Os::u_sleep(int64_t usec) {
#အကယ်၍ defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#မဟုတ်ပါက
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

ကိန်းပြည့် V3Os::system(const string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    ပြန်ပေး -1;
#မဟုတ်ပါက
    UINFO(1, "Running system: " << command);
    const ကိန်းပြည့် ret = ::system(command.c_str());
    အကယ်၍ (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        ပြန်ပေး -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    const ကိန်းပြည့် exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    ပြန်ပေး exit_code;
#endif
}

void V3Os::selfTest() {
#ifdef VL_DEBUG
    UASSERT_SELFTEST(filenameCleanup(""), "");
    UASSERT_SELFTEST(filenameCleanup("."), ".");
    UASSERT_SELFTEST(filenameCleanup(".."), "..");
    UASSERT_SELFTEST(filenameCleanup("/"), "/");
    UASSERT_SELFTEST(filenameCleanup("../"), "..");
    UASSERT_SELFTEST(filenameCleanup("//"), "/");
    UASSERT_SELFTEST(filenameCleanup("//."), "/.");
    UASSERT_SELFTEST(filenameCleanup("./"), ".");
    UASSERT_SELFTEST(filenameCleanup("././"), ".");
    UASSERT_SELFTEST(filenameCleanup(".///"), ".");
    UASSERT_SELFTEST(filenameCleanup("a"), "a");
    UASSERT_SELFTEST(filenameCleanup("a/"), "a");
    UASSERT_SELFTEST(filenameCleanup("a/b"), "a/b");
    UASSERT_SELFTEST(filenameCleanup("././//./a/b"), "a/b");
    UASSERT_SELFTEST(filenameCleanup(".//./a///"), "a");
    UASSERT_SELFTEST(filenameCleanup("///a/./b///."), "/a/./b/.");
    UASSERT_SELFTEST(filenameCleanup("aaa/bbb/ccc/"), "aaa/bbb/ccc");
    UASSERT_SELFTEST(filenameCleanup("./aaa/bbb/ccc/"), "aaa/bbb/ccc");
    UASSERT_SELFTEST(filenameCleanup("../aaa/bbb/ccc/"), "../aaa/bbb/ccc");
    UASSERT_SELFTEST(filenameDir("a.a/b.b/f.e"), "a.a/b.b");
    UASSERT_SELFTEST(filenameExt("a.a/b.b/f"), "");
    UASSERT_SELFTEST(filenameExt("a.a/b.b/f.e"), ".e");
    UASSERT_SELFTEST(filenameNonDirExt("a.a/b.b/f.e"), "f");
    UASSERT_SELFTEST(filenameRelativePath("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filenameRelativePath("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filenameRelativePath("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filenameRelativePath("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filenameRelativePath("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filenameRelativePath("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filenameRelativePath("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filenameRelativePath("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filenameRelativePath("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filenameRelativePath("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filenameRelativePath("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filenameRelativePath("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filenameRelativePath("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filenameRelativePath("a/b/x/y", "a/q"), "../b/x/y");
#endif
}
