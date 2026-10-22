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
#اگر defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#شامل "config_build.h"
#شامل "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#شامل "V3Os.h"
#شامل "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#شامل "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#شامل <cerrno>
#شامل <climits>  // PATH_MAX (especially on FreeBSD)
#شامل <cstdarg>
#ifdef _MSC_VER
#شامل <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#وگرنه
#شامل <dirent.h>
#شامل <utime.h>
#endif
#شامل <fcntl.h>
#شامل <fstream>
#شامل <memory>

#شامل <sys/stat.h>
#شامل <sys/types.h>

#ifdef HAVE_TCMALLOC
#شامل <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#شامل <jemalloc/jemalloc.h>
#endif

// clang-format off
#اگر defined(_WIN32) || defined(__MINGW32__)
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
#  وگرنه
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#وگرنه
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#شامل "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ثابت string& envvar, ثابت string& default_مقدار) {
    بازگشت VString::escapeStringForPath(VlOs::getenvStr(envvar, default_مقدار));
}

تهی V3Os::setenvStr(ثابت string& envvar, ثابت string& مقدار, ثابت string& why) {
    اگر (why != "") {
        UINFO(1, "export " << envvar << "='" << مقدار << "'  # " << why);
    } وگرنه {
        UINFO(1, "export " << envvar << "='" << مقدار << "'");
    }
#اگر defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), مقدار.c_str());
#وگرنهاگر defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), مقدار.c_str(), درست);
#وگرنه
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ثابت string vareq = envvar + "=" + مقدار;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#اگر defined(_WIN32) || defined(__MINGW32__)
ایستا constexpr نویسه V3OS_SLASH = '\\';
#وگرنه
ایستا constexpr نویسه V3OS_SLASH = '/';
#endif

ایستا منطقی isSlash(نویسه ch) VL_PURE {
#اگر defined(_WIN32) || defined(__MINGW32__)
    بازگشت ch == '/' || ch == '\\';
#وگرنه
    بازگشت ch == '/';
#endif
}

string V3Os::filenameCleanup(ثابت string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    منطقی lastIsSlash = نادرست;
    برای (ثابت نویسه ch : filename) {
        ثابت منطقی lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        اگر (lastIsSlash && lastIsSlashOld) ادامه;
        str += ch;
    }
    اگر (str.size() > 1 && isSlash(str.back())) str.برداشت_back();
    وقتی (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    بازگشت str;
}

string V3Os::filenameJoin(std::initializer_list<ثابت std::string> paths) VL_PURE {
    string fullpath;
    برای (ثابت auto& item : paths) {
        اگر (item.empty() || item == ".") ادامه;
        اگر (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    بازگشت fullpath;
}

string V3Os::filenameDir(ثابت string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    برای (; it != filename.rend(); ++it) {
        اگر (isSlash(*it)) بشکن;
    }
    اگر (it.base() == filename.begin()) بازگشت ".";
    بازگشت string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(ثابت string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ثابت string::size_type pos = base.find('.');
    اگر (pos != string::npos) {
        base.erase(0, pos);
        بازگشت base;
    }
    بازگشت "";
}

string V3Os::filenameNonDir(ثابت string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    برای (; it != filename.rend(); ++it) {
        اگر (isSlash(*it)) بشکن;
    }
    بازگشت string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(ثابت string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ثابت string::size_type pos = base.find('.');
    اگر (pos != string::npos) base.erase(pos);
    بازگشت base;
}

string V3Os::filenameSubstitute(ثابت string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    شمارش : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    برای (string::size_type pos = 0; pos < filename.length(); ++pos) {
        اگر ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            انتخاب (filename[pos + 1]) {
            حالت '{': brackets = CURLY; بشکن;
            حالت '(': brackets = PAREN; بشکن;
            پیشفرض: brackets = NONE; بشکن;
            }
            اگر (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            وقتی (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            اگر (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ثابت string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            اگر (!envvar.empty()) envvalue = getenvStr(envvar, "");
            اگر (!envvalue.empty()) {
                result += envvalue;
                اگر (brackets == NONE) {
                    pos = endpos;
                } وگرنه {
                    pos = endpos + 1;
                }
            } وگرنه {
                result += filename[pos];  // *pos == '$'
            }
        } وگرنه {
            result += filename[pos];
        }
    }
    بازگشت result;
}

string V3Os::filenameRealPath(ثابت string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    نویسه retpath[PATH_MAX];
    اگر (
#اگر defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#وگرنه
        realpath(filename.c_str(), retpath)
#endif
    ) {
        بازگشت std::string{retpath};
    }
    بازگشت filename;
}

string V3Os::filenameAbsolutePath(ثابت string& filename) VL_PURE {
#اگر defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    بازگشت filenameRealPath(filename);
#وگرنه
    اگر (!filenameIsRel(filename)) بازگشت filename;
    بازگشت filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(ثابت string& filename, ثابت string& base) VL_PURE {
    ثابت string a = V3Os::filenameRealPath(filename);
    ثابت string b = V3Os::filenameRealPath(base);
    string result;
    اگر (a == b) بازگشت ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    وقتی (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        برای (; aWordIt != a.end(); ++aWordIt) {
            اگر (isSlash(*aWordIt)) بشکن;
        }

        auto bWordIt = bIt;  // position of next slash
        برای (; bWordIt != b.end(); ++bWordIt) {
            اگر (isSlash(*bWordIt)) بشکن;
        }

        ثابت string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        ثابت string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        اگر (aWord != bWord) بشکن;
        aIt = aWordIt;
        bIt = bWordIt;
        اگر (aIt != a.end()) ++aIt;  // Skip slash
        اگر (bIt != b.end()) ++bIt;  // Skip slash
    }

    وقتی (bIt != b.end()) {
        برای (; bIt != b.end(); ++bIt) {
            اگر (isSlash(*bIt)) {
                ++bIt;
                بشکن;
            }
        }
        اگر (!result.empty()) result += "/";
        result += "..";
    }

    ثابت string a_چپ = a.substr(aIt - a.begin());
    اگر (!a_چپ.empty()) {
        اگر (!result.empty()) result += "/";
        result += a_چپ;
    }
    بازگشت filenameCleanup(result);
}

منطقی V3Os::filenameIsRel(ثابت string& filename) VL_PURE {
#اگر defined(_MSC_VER)
    بازگشت std::filesystem::path(filename).is_relative();
#وگرنه
    بازگشت (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(ثابت string& filename) VL_PURE {
#اگر defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    بازگشت slashedPath;
#وگرنه
    بازگشت filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, نویسه delim) {
    string سطر;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    نویسه buf[65536];
    is.getline(buf, 65535, delim);
    سطر = buf;
#وگرنه
    std::getline(is, سطر, delim);
#endif
    بازگشت سطر;
}

//######################################################################
// Directory utilities

تهی V3Os::createDir(ثابت string& dirname) {
#اگر defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#وگرنه
    mkdir(dirname.c_str(), 0777);
#endif
}

تهی V3Os::filesystemFlush(ثابت string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#اگر defined(_MSC_VER) || defined(__MINGW32__)
    اگر (صحیح fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        اگر (fd > 0) ::close(fd);
    }
#وگرنه
    // Faster to just try both rather than check if a file is a dir.
    اگر (DIR* ثابت dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } وگرنه اگر (ثابت صحیح fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        اگر (fd > 0) ::close(fd);
    }
#endif
}

تهی V3Os::filesystemFlushBuildDir(ثابت string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#اگر !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ثابت صحیح err = utimes(dirname.c_str(), nullptr);
    // Not an error
    اگر (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

تهی V3Os::unlinkRegexp(ثابت string& dir, ثابت string& regexp) {
#ifdef _MSC_VER
    تلاش {
        برای (ثابت auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            اگر (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                ثابت string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } بگیر (std::filesystem::filesystem_error ثابت& ex) {}
#وگرنه
    اگر (DIR* ثابت dirp = opendir(dir.c_str())) {
        وقتی (ساختار dirent* ثابت direntp = readdir(dirp)) {
            اگر (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ثابت string fullname = dir + "/" + std::string{direntp->d_name};
#اگر defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#وگرنه
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

تهی V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = اندازه(narenas);
    اگر (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        بازگشت;  // Failed to get number of arenas, give up
    }
    نویسه buf[64];
    // Index equal to narenas represents all arenas
    (تهی)VL_SNPRINTF(buf, اندازه(buf), "arena.%u.purge", narenas);
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
    بازگشت result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    نویسه* ثابت data = const_cast<نویسه*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#اگر defined(_WIN32) || defined(__MINGW32__)
    ثابت NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    اگر (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وگرنهاگر defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    اگر (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وگرنه
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::دودویی};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    اگر (VL_UNCOVERABLE(!is.اقرأ(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    بازگشت result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#اگر defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ایستا constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    ثابت uint64_t us
        = ((static_cast<uint64_t>(ft.dw_بالا_Date_Time) << 32) + ft.dw_کم_Date_Time + 5ULL) / 10ULL;
    بازگشت us - EPOCH_DIFFERENCE_USECS;
#وگرنه
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    اگر (gettimeofday(&tv, nullptr) < 0) بازگشت 0;
    بازگشت static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

تهی V3Os::u_sleep(int64_t usec) {
#اگر defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#وگرنه
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

صحیح V3Os::system(ثابت string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    بازگشت -1;
#وگرنه
    UINFO(1, "Running system: " << command);
    ثابت صحیح ret = ::system(command.c_str());
    اگر (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        بازگشت -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ثابت صحیح exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    بازگشت exit_code;
#endif
}

تهی V3Os::selfTest() {
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
