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
#જો defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#અન્તર્ભાવ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#અન્તર્ભાવ <cerrno>
#અન્તર્ભાવ <climits>  // PATH_MAX (especially on FreeBSD)
#અન્તર્ભાવ <cstdarg>
#ifdef _MSC_VER
#અન્તર્ભાવ <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#નહીંતર
#અન્તર્ભાવ <dirent.h>
#અન્તર્ભાવ <utime.h>
#endif
#અન્તર્ભાવ <fcntl.h>
#અન્તર્ભાવ <fstream>
#અન્તર્ભાવ <memory>

#અન્તર્ભાવ <sys/stat.h>
#અન્તર્ભાવ <sys/types.h>

#ifdef HAVE_TCMALLOC
#અન્તર્ભાવ <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#અન્તર્ભાવ <jemalloc/jemalloc.h>
#endif

// clang-format off
#જો defined(_WIN32) || defined(__MINGW32__)
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
#  નહીંતર
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#નહીંતર
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#અન્તર્ભાવ "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(અચલ string& envvar, અચલ string& default_કિંમત) {
    પાછા VString::escapeStringForPath(VlOs::getenvStr(envvar, default_કિંમત));
}

શૂન્ય V3Os::setenvStr(અચલ string& envvar, અચલ string& કિંમત, અચલ string& why) {
    જો (why != "") {
        UINFO(1, "export " << envvar << "='" << કિંમત << "'  # " << why);
    } નહીંતર {
        UINFO(1, "export " << envvar << "='" << કિંમત << "'");
    }
#જો defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), કિંમત.c_str());
#અથવાજો defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), કિંમત.c_str(), સાચું);
#નહીંતર
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    અચલ string vareq = envvar + "=" + કિંમત;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#જો defined(_WIN32) || defined(__MINGW32__)
સ્થિર constexpr અક્ષર V3OS_SLASH = '\\';
#નહીંતર
સ્થિર constexpr અક્ષર V3OS_SLASH = '/';
#endif

સ્થિર તાર્કિક isSlash(અક્ષર ch) VL_PURE {
#જો defined(_WIN32) || defined(__MINGW32__)
    પાછા ch == '/' || ch == '\\';
#નહીંતર
    પાછા ch == '/';
#endif
}

string V3Os::filenameCleanup(અચલ string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    તાર્કિક lastIsSlash = ખોટું;
    માટે (અચલ અક્ષર ch : filename) {
        અચલ તાર્કિક lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        જો (lastIsSlash && lastIsSlashOld) ચાલુ;
        str += ch;
    }
    જો (str.size() > 1 && isSlash(str.back())) str.કાઢો_back();
    જ્યારે (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    પાછા str;
}

string V3Os::filenameJoin(std::initializer_list<અચલ std::string> paths) VL_PURE {
    string fullpath;
    માટે (અચલ auto& item : paths) {
        જો (item.empty() || item == ".") ચાલુ;
        જો (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    પાછા fullpath;
}

string V3Os::filenameDir(અચલ string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    માટે (; it != filename.rend(); ++it) {
        જો (isSlash(*it)) તોડો;
    }
    જો (it.base() == filename.begin()) પાછા ".";
    પાછા string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(અચલ string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    અચલ string::size_type pos = base.find('.');
    જો (pos != string::npos) {
        base.erase(0, pos);
        પાછા base;
    }
    પાછા "";
}

string V3Os::filenameNonDir(અચલ string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    માટે (; it != filename.rend(); ++it) {
        જો (isSlash(*it)) તોડો;
    }
    પાછા string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(અચલ string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    અચલ string::size_type pos = base.find('.');
    જો (pos != string::npos) base.erase(pos);
    પાછા base;
}

string V3Os::filenameSubstitute(અચલ string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    ગણના : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    માટે (string::size_type pos = 0; pos < filename.length(); ++pos) {
        જો ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            પસંદગી (filename[pos + 1]) {
            સ્થિતિ '{': brackets = CURLY; તોડો;
            સ્થિતિ '(': brackets = PAREN; તોડો;
            મૂળભૂત: brackets = NONE; તોડો;
            }
            જો (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            જ્યારે (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            જો (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            અચલ string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            જો (!envvar.empty()) envvalue = getenvStr(envvar, "");
            જો (!envvalue.empty()) {
                result += envvalue;
                જો (brackets == NONE) {
                    pos = endpos;
                } નહીંતર {
                    pos = endpos + 1;
                }
            } નહીંતર {
                result += filename[pos];  // *pos == '$'
            }
        } નહીંતર {
            result += filename[pos];
        }
    }
    પાછા result;
}

string V3Os::filenameRealPath(અચલ string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    અક્ષર retpath[PATH_MAX];
    જો (
#જો defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#નહીંતર
        realpath(filename.c_str(), retpath)
#endif
    ) {
        પાછા std::string{retpath};
    }
    પાછા filename;
}

string V3Os::filenameAbsolutePath(અચલ string& filename) VL_PURE {
#જો defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    પાછા filenameRealPath(filename);
#નહીંતર
    જો (!filenameIsRel(filename)) પાછા filename;
    પાછા filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(અચલ string& filename, અચલ string& base) VL_PURE {
    અચલ string a = V3Os::filenameRealPath(filename);
    અચલ string b = V3Os::filenameRealPath(base);
    string result;
    જો (a == b) પાછા ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    જ્યારે (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        માટે (; aWordIt != a.end(); ++aWordIt) {
            જો (isSlash(*aWordIt)) તોડો;
        }

        auto bWordIt = bIt;  // position of next slash
        માટે (; bWordIt != b.end(); ++bWordIt) {
            જો (isSlash(*bWordIt)) તોડો;
        }

        અચલ string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        અચલ string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        જો (aWord != bWord) તોડો;
        aIt = aWordIt;
        bIt = bWordIt;
        જો (aIt != a.end()) ++aIt;  // Skip slash
        જો (bIt != b.end()) ++bIt;  // Skip slash
    }

    જ્યારે (bIt != b.end()) {
        માટે (; bIt != b.end(); ++bIt) {
            જો (isSlash(*bIt)) {
                ++bIt;
                તોડો;
            }
        }
        જો (!result.empty()) result += "/";
        result += "..";
    }

    અચલ string a_ડાબું = a.substr(aIt - a.begin());
    જો (!a_ડાબું.empty()) {
        જો (!result.empty()) result += "/";
        result += a_ડાબું;
    }
    પાછા filenameCleanup(result);
}

તાર્કિક V3Os::filenameIsRel(અચલ string& filename) VL_PURE {
#જો defined(_MSC_VER)
    પાછા std::filesystem::path(filename).is_relative();
#નહીંતર
    પાછા (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(અચલ string& filename) VL_PURE {
#જો defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    પાછા slashedPath;
#નહીંતર
    પાછા filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, અક્ષર delim) {
    string લીટી;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    અક્ષર buf[65536];
    is.getline(buf, 65535, delim);
    લીટી = buf;
#નહીંતર
    std::getline(is, લીટી, delim);
#endif
    પાછા લીટી;
}

//######################################################################
// Directory utilities

શૂન્ય V3Os::createDir(અચલ string& dirname) {
#જો defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#નહીંતર
    mkdir(dirname.c_str(), 0777);
#endif
}

શૂન્ય V3Os::filesystemFlush(અચલ string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#જો defined(_MSC_VER) || defined(__MINGW32__)
    જો (પૂર્ણાંક fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        જો (fd > 0) ::close(fd);
    }
#નહીંતર
    // Faster to just try both rather than check if a file is a dir.
    જો (DIR* અચલ dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } નહીંતર જો (અચલ પૂર્ણાંક fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        જો (fd > 0) ::close(fd);
    }
#endif
}

શૂન્ય V3Os::filesystemFlushBuildDir(અચલ string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#જો !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    અચલ પૂર્ણાંક err = utimes(dirname.c_str(), nullptr);
    // Not an error
    જો (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

શૂન્ય V3Os::unlinkRegexp(અચલ string& dir, અચલ string& regexp) {
#ifdef _MSC_VER
    પ્રયાસ {
        માટે (અચલ auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            જો (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                અચલ string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } પકડો (std::filesystem::filesystem_error અચલ& ex) {}
#નહીંતર
    જો (DIR* અચલ dirp = opendir(dir.c_str())) {
        જ્યારે (સંરચના dirent* અચલ direntp = readdir(dirp)) {
            જો (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                અચલ string fullname = dir + "/" + std::string{direntp->d_name};
#જો defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#નહીંતર
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

શૂન્ય V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = કદ(narenas);
    જો (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        પાછા;  // Failed to get number of arenas, give up
    }
    અક્ષર buf[64];
    // Index equal to narenas represents all arenas
    (શૂન્ય)VL_SNPRINTF(buf, કદ(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    અચલ uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    પાછા result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    અક્ષર* અચલ data = const_cast<અક્ષર*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#જો defined(_WIN32) || defined(__MINGW32__)
    અચલ NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    જો (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#અથવાજો defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    જો (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#નહીંતર
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::દ્વિઅંકી};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    જો (VL_UNCOVERABLE(!is.પઠ(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    પાછા result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#જો defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    સ્થિર constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    અચલ uint64_t us
        = ((static_cast<uint64_t>(ft.dw_ઉપલું_Date_Time) << 32) + ft.dw_નીચલું_Date_Time + 5ULL) / 10ULL;
    પાછા us - EPOCH_DIFFERENCE_USECS;
#નહીંતર
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    જો (gettimeofday(&tv, nullptr) < 0) પાછા 0;
    પાછા static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

શૂન્ય V3Os::u_sleep(int64_t usec) {
#જો defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#નહીંતર
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

પૂર્ણાંક V3Os::system(અચલ string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    પાછા -1;
#નહીંતર
    UINFO(1, "Running system: " << command);
    અચલ પૂર્ણાંક ret = ::system(command.c_str());
    જો (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        પાછા -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    અચલ પૂર્ણાંક exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    પાછા exit_code;
#endif
}

શૂન્ય V3Os::selfTest() {
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
