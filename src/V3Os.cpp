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
#إذا defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#ضمن "config_build.h"
#ضمن "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ضمن "V3Os.h"
#ضمن "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#ضمن "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#ضمن <cerrno>
#ضمن <climits>  // PATH_MAX (especially on FreeBSD)
#ضمن <cstdarg>
#ifdef _MSC_VER
#ضمن <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#وإلا
#ضمن <dirent.h>
#ضمن <utime.h>
#endif
#ضمن <fcntl.h>
#ضمن <fstream>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

#ifdef HAVE_TCMALLOC
#ضمن <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#ضمن <jemalloc/jemalloc.h>
#endif

// clang-format off
#إذا defined(_WIN32) || defined(__MINGW32__)
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
#  وإلا
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#وإلا
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#ضمن "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ثابت string& envvar, ثابت string& default_درج) {
    أرجع VString::escapeStringForPath(VlOs::getenvStr(envvar, default_درج));
}

فراغ V3Os::setenvStr(ثابت string& envvar, ثابت string& درج, ثابت string& why) {
    إذا (why != "") {
        UINFO(1, "export " << envvar << "='" << درج << "'  # " << why);
    } وإلا {
        UINFO(1, "export " << envvar << "='" << درج << "'");
    }
#إذا defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), درج.c_str());
#وإلاإذا defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), درج.c_str(), true);
#وإلا
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ثابت string vareq = envvar + "=" + درج;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#إذا defined(_WIN32) || defined(__MINGW32__)
ساكن constexpr حرف V3OS_SLASH = '\\';
#وإلا
ساكن constexpr حرف V3OS_SLASH = '/';
#endif

ساكن منطقي isSlash(حرف ch) VL_PURE {
#إذا defined(_WIN32) || defined(__MINGW32__)
    أرجع ch == '/' || ch == '\\';
#وإلا
    أرجع ch == '/';
#endif
}

string V3Os::filenameCleanup(ثابت string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    منطقي lastIsSlash = خطأ;
    لكل (ثابت حرف ch : filename) {
        ثابت منطقي lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        إذا (lastIsSlash && lastIsSlashOld) استمر;
        str += ch;
    }
    إذا (str.size() > 1 && isSlash(str.back())) str.چرو_back();
    طالما (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    أرجع str;
}

string V3Os::filenameJoin(std::initializer_list<ثابت std::string> paths) VL_PURE {
    string fullpath;
    لكل (ثابت auto& item : paths) {
        إذا (item.empty() || item == ".") استمر;
        إذا (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    أرجع fullpath;
}

string V3Os::filenameDir(ثابت string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    إذا (it.base() == filename.begin()) أرجع ".";
    أرجع string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(ثابت string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ثابت string::size_type pos = base.find('.');
    إذا (pos != string::npos) {
        base.erase(0, pos);
        أرجع base;
    }
    أرجع "";
}

string V3Os::filenameNonDir(ثابت string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    أرجع string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(ثابت string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ثابت string::size_type pos = base.find('.');
    إذا (pos != string::npos) base.erase(pos);
    أرجع base;
}

string V3Os::filenameSubstitute(ثابت string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    تعداد : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    لكل (string::size_type pos = 0; pos < filename.length(); ++pos) {
        إذا ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            بدل (filename[pos + 1]) {
            حالة '{': brackets = CURLY; اكسر;
            حالة '(': brackets = PAREN; اكسر;
            افتراضي: brackets = NONE; اكسر;
            }
            إذا (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            طالما (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            إذا (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ثابت string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            إذا (!envvar.empty()) envvalue = getenvStr(envvar, "");
            إذا (!envvalue.empty()) {
                result += envvalue;
                إذا (brackets == NONE) {
                    pos = endpos;
                } وإلا {
                    pos = endpos + 1;
                }
            } وإلا {
                result += filename[pos];  // *pos == '$'
            }
        } وإلا {
            result += filename[pos];
        }
    }
    أرجع result;
}

string V3Os::filenameRealPath(ثابت string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    حرف retpath[PATH_MAX];
    إذا (
#إذا defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#وإلا
        realpath(filename.c_str(), retpath)
#endif
    ) {
        أرجع std::string{retpath};
    }
    أرجع filename;
}

string V3Os::filenameAbsolutePath(ثابت string& filename) VL_PURE {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    أرجع filenameRealPath(filename);
#وإلا
    إذا (!filenameIsRel(filename)) أرجع filename;
    أرجع filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(ثابت string& filename, ثابت string& base) VL_PURE {
    ثابت string a = V3Os::filenameRealPath(filename);
    ثابت string b = V3Os::filenameRealPath(base);
    string result;
    إذا (a == b) أرجع ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    طالما (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        لكل (; aWordIt != a.end(); ++aWordIt) {
            إذا (isSlash(*aWordIt)) اكسر;
        }

        auto bWordIt = bIt;  // position of next slash
        لكل (; bWordIt != b.end(); ++bWordIt) {
            إذا (isSlash(*bWordIt)) اكسر;
        }

        ثابت string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        ثابت string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        إذا (aWord != bWord) اكسر;
        aIt = aWordIt;
        bIt = bWordIt;
        إذا (aIt != a.end()) ++aIt;  // Skip slash
        إذا (bIt != b.end()) ++bIt;  // Skip slash
    }

    طالما (bIt != b.end()) {
        لكل (; bIt != b.end(); ++bIt) {
            إذا (isSlash(*bIt)) {
                ++bIt;
                اكسر;
            }
        }
        إذا (!result.empty()) result += "/";
        result += "..";
    }

    ثابت string a_هغو = a.substr(aIt - a.begin());
    إذا (!a_هغو.empty()) {
        إذا (!result.empty()) result += "/";
        result += a_هغو;
    }
    أرجع filenameCleanup(result);
}

منطقي V3Os::filenameIsRel(ثابت string& filename) VL_PURE {
#إذا defined(_MSC_VER)
    أرجع std::filesystem::path(filename).is_relative();
#وإلا
    أرجع (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(ثابت string& filename) VL_PURE {
#إذا defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    أرجع slashedPath;
#وإلا
    أرجع filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, حرف delim) {
    string لي;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    حرف buf[65536];
    is.getline(buf, 65535, delim);
    لي = buf;
#وإلا
    std::getline(is, لي, delim);
#endif
    أرجع لي;
}

//######################################################################
// Directory utilities

فراغ V3Os::createDir(ثابت string& dirname) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#وإلا
    mkdir(dirname.c_str(), 0777);
#endif
}

فراغ V3Os::filesystemFlush(ثابت string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#إذا defined(_MSC_VER) || defined(__MINGW32__)
    إذا (صحيح fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        إذا (fd > 0) ::close(fd);
    }
#وإلا
    // Faster to just try both rather than check if a file is a dir.
    إذا (DIR* ثابت dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } وإلا إذا (ثابت صحيح fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        إذا (fd > 0) ::close(fd);
    }
#endif
}

فراغ V3Os::filesystemFlushBuildDir(ثابت string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#إذا !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ثابت صحيح err = utimes(dirname.c_str(), nullptr);
    // Not an error
    إذا (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

فراغ V3Os::unlinkRegexp(ثابت string& dir, ثابت string& regexp) {
#ifdef _MSC_VER
    حاول {
        لكل (ثابت auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            إذا (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                ثابت string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } امسك (std::filesystem::filesystem_error ثابت& ex) {}
#وإلا
    إذا (DIR* ثابت dirp = opendir(dir.c_str())) {
        طالما (بنية dirent* ثابت direntp = readdir(dirp)) {
            إذا (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ثابت string fullname = dir + "/" + std::string{direntp->d_name};
#إذا defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#وإلا
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

فراغ V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = حجم(narenas);
    إذا (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        أرجع;  // Failed to get number of arenas, give up
    }
    حرف buf[64];
    // Index equal to narenas represents all arenas
    (فراغ)VL_SNPRINTF(buf, حجم(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ثابت uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    أرجع result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    حرف* ثابت data = const_cast<حرف*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#إذا defined(_WIN32) || defined(__MINGW32__)
    ثابت NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    إذا (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلاإذا defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    إذا (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلا
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::نبيو};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    إذا (VL_UNCOVERABLE(!is.اقرأ(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    أرجع result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ساكن constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    ثابت uint64_t us
        = ((static_cast<uint64_t>(ft.dw_سم_Date_Time) << 32) + ft.dw_كس_Date_Time + 5ULL) / 10ULL;
    أرجع us - EPOCH_DIFFERENCE_USECS;
#وإلا
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    إذا (gettimeofday(&tv, nullptr) < 0) أرجع 0;
    أرجع static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

فراغ V3Os::u_sleep(int64_t usec) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#وإلا
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

صحيح V3Os::system(ثابت string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    أرجع -1;
#وإلا
    UINFO(1, "Running system: " << command);
    ثابت صحيح ret = ::system(command.c_str());
    إذا (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        أرجع -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ثابت صحيح exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    أرجع exit_code;
#endif
}

فراغ V3Os::selfTest() {
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
