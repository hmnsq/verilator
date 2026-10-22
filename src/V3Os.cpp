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
#যদি defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#অন্তর্ভাব "V3Os.h"
#অন্তর্ভাব "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#অন্তর্ভাব "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#অন্তর্ভাব <cerrno>
#অন্তর্ভাব <climits>  // PATH_MAX (especially on FreeBSD)
#অন্তর্ভাব <cstdarg>
#ifdef _MSC_VER
#অন্তর্ভাব <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#অন্যথা
#অন্তর্ভাব <dirent.h>
#অন্তর্ভাব <utime.h>
#endif
#অন্তর্ভাব <fcntl.h>
#অন্তর্ভাব <fstream>
#অন্তর্ভাব <memory>

#অন্তর্ভাব <sys/stat.h>
#অন্তর্ভাব <sys/types.h>

#ifdef HAVE_TCMALLOC
#অন্তর্ভাব <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#অন্তর্ভাব <jemalloc/jemalloc.h>
#endif

// clang-format off
#যদি defined(_WIN32) || defined(__MINGW32__)
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
#  অন্যথা
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#অন্যথা
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#অন্তর্ভাব "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ধ্রুবক string& envvar, ধ্রুবক string& default_মান) {
    ফেরত VString::escapeStringForPath(VlOs::getenvStr(envvar, default_মান));
}

শূন্য V3Os::setenvStr(ধ্রুবক string& envvar, ধ্রুবক string& মান, ধ্রুবক string& why) {
    যদি (why != "") {
        UINFO(1, "export " << envvar << "='" << মান << "'  # " << why);
    } অন্যথা {
        UINFO(1, "export " << envvar << "='" << মান << "'");
    }
#যদি defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), মান.c_str());
#নতুবা_যদি defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), মান.c_str(), সত্য);
#অন্যথা
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ধ্রুবক string vareq = envvar + "=" + মান;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#যদি defined(_WIN32) || defined(__MINGW32__)
স্থির constexpr অক্ষর V3OS_SLASH = '\\';
#অন্যথা
স্থির constexpr অক্ষর V3OS_SLASH = '/';
#endif

স্থির যৌক্তিক isSlash(অক্ষর ch) VL_PURE {
#যদি defined(_WIN32) || defined(__MINGW32__)
    ফেরত ch == '/' || ch == '\\';
#অন্যথা
    ফেরত ch == '/';
#endif
}

string V3Os::filenameCleanup(ধ্রুবক string& filename) VL_PURE {
    string str;
    str.reserve(filename.দৈর্ঘ্য());
    যৌক্তিক lastIsSlash = মিথ্যা;
    জন্য (ধ্রুবক অক্ষর ch : filename) {
        ধ্রুবক যৌক্তিক lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        যদি (lastIsSlash && lastIsSlashOld) চালু;
        str += ch;
    }
    যদি (str.আকার_2() > 1 && isSlash(str.back())) str.বের_করো_back();
    যখন (str.আকার_2() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    ফেরত str;
}

string V3Os::filenameJoin(std::initializer_list<ধ্রুবক std::string> paths) VL_PURE {
    string fullpath;
    জন্য (ধ্রুবক auto& item : paths) {
        যদি (item.empty() || item == ".") চালু;
        যদি (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    ফেরত fullpath;
}

string V3Os::filenameDir(ধ্রুবক string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    জন্য (; it != filename.rend(); ++it) {
        যদি (isSlash(*it)) বিরতি;
    }
    যদি (it.base() == filename.begin()) ফেরত ".";
    ফেরত string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(ধ্রুবক string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ধ্রুবক string::আকার_type pos = base.find('.');
    যদি (pos != string::npos) {
        base.erase(0, pos);
        ফেরত base;
    }
    ফেরত "";
}

string V3Os::filenameNonDir(ধ্রুবক string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    জন্য (; it != filename.rend(); ++it) {
        যদি (isSlash(*it)) বিরতি;
    }
    ফেরত string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(ধ্রুবক string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ধ্রুবক string::আকার_type pos = base.find('.');
    যদি (pos != string::npos) base.erase(pos);
    ফেরত base;
}

string V3Os::filenameSubstitute(ধ্রুবক string& filename) {
    string ফলাফল;
    // cppcheck-has-bug-suppress unusedLabel
    গণনা : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    জন্য (string::আকার_type pos = 0; pos < filename.দৈর্ঘ্য(); ++pos) {
        যদি ((filename[pos] == '$') && (pos + 1 < filename.দৈর্ঘ্য())) {
            নির্বাচন (filename[pos + 1]) {
            ক্ষেত্র '{': brackets = CURLY; বিরতি;
            ক্ষেত্র '(': brackets = PAREN; বিরতি;
            পূর্বনির্ধারিত: brackets = NONE; বিরতি;
            }
            যদি (brackets != NONE) pos = pos + 1;
            string::আকার_type endpos = pos + 1;
            যখন (((endpos + 1) < filename.দৈর্ঘ্য())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            যদি (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ধ্রুবক string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            যদি (!envvar.empty()) envvalue = getenvStr(envvar, "");
            যদি (!envvalue.empty()) {
                ফলাফল += envvalue;
                যদি (brackets == NONE) {
                    pos = endpos;
                } অন্যথা {
                    pos = endpos + 1;
                }
            } অন্যথা {
                ফলাফল += filename[pos];  // *pos == '$'
            }
        } অন্যথা {
            ফলাফল += filename[pos];
        }
    }
    ফেরত ফলাফল;
}

string V3Os::filenameRealPath(ধ্রুবক string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    অক্ষর retpath[PATH_MAX];
    যদি (
#যদি defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#অন্যথা
        realpath(filename.c_str(), retpath)
#endif
    ) {
        ফেরত std::string{retpath};
    }
    ফেরত filename;
}

string V3Os::filenameAbsolutePath(ধ্রুবক string& filename) VL_PURE {
#যদি defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    ফেরত filenameRealPath(filename);
#অন্যথা
    যদি (!filenameIsRel(filename)) ফেরত filename;
    ফেরত filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(ধ্রুবক string& filename, ধ্রুবক string& base) VL_PURE {
    ধ্রুবক string a = V3Os::filenameRealPath(filename);
    ধ্রুবক string b = V3Os::filenameRealPath(base);
    string ফলাফল;
    যদি (a == b) ফেরত ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    যখন (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        জন্য (; aWordIt != a.end(); ++aWordIt) {
            যদি (isSlash(*aWordIt)) বিরতি;
        }

        auto bWordIt = bIt;  // position of next slash
        জন্য (; bWordIt != b.end(); ++bWordIt) {
            যদি (isSlash(*bWordIt)) বিরতি;
        }

        ধ্রুবক string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        ধ্রুবক string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        যদি (aWord != bWord) বিরতি;
        aIt = aWordIt;
        bIt = bWordIt;
        যদি (aIt != a.end()) ++aIt;  // Skip slash
        যদি (bIt != b.end()) ++bIt;  // Skip slash
    }

    যখন (bIt != b.end()) {
        জন্য (; bIt != b.end(); ++bIt) {
            যদি (isSlash(*bIt)) {
                ++bIt;
                বিরতি;
            }
        }
        যদি (!ফলাফল.empty()) ফলাফল += "/";
        ফলাফল += "..";
    }

    ধ্রুবক string a_বাম = a.substr(aIt - a.begin());
    যদি (!a_বাম.empty()) {
        যদি (!ফলাফল.empty()) ফলাফল += "/";
        ফলাফল += a_বাম;
    }
    ফেরত filenameCleanup(ফলাফল);
}

যৌক্তিক V3Os::filenameIsRel(ধ্রুবক string& filename) VL_PURE {
#যদি defined(_MSC_VER)
    ফেরত std::filesystem::path(filename).is_relative();
#অন্যথা
    ফেরত (filename.দৈর্ঘ্য() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(ধ্রুবক string& filename) VL_PURE {
#যদি defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    ফেরত slashedPath;
#অন্যথা
    ফেরত filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, অক্ষর delim) {
    string সারি;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    অক্ষর buf[65536];
    is.getline(buf, 65535, delim);
    সারি = buf;
#অন্যথা
    std::getline(is, সারি, delim);
#endif
    ফেরত সারি;
}

//######################################################################
// Directory utilities

শূন্য V3Os::createDir(ধ্রুবক string& dirname) {
#যদি defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#অন্যথা
    mkdir(dirname.c_str(), 0777);
#endif
}

শূন্য V3Os::filesystemFlush(ধ্রুবক string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#যদি defined(_MSC_VER) || defined(__MINGW32__)
    যদি (পূর্ণসংখ্যা fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        যদি (fd > 0) ::close(fd);
    }
#অন্যথা
    // Faster to just try both rather than check if a file is a dir.
    যদি (DIR* ধ্রুবক dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } অন্যথা যদি (ধ্রুবক পূর্ণসংখ্যা fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        যদি (fd > 0) ::close(fd);
    }
#endif
}

শূন্য V3Os::filesystemFlushBuildDir(ধ্রুবক string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#যদি !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ধ্রুবক পূর্ণসংখ্যা err = utimes(dirname.c_str(), nullptr);
    // Not an error
    যদি (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

শূন্য V3Os::unlinkRegexp(ধ্রুবক string& dir, ধ্রুবক string& regexp) {
#ifdef _MSC_VER
    চেষ্টা {
        জন্য (ধ্রুবক auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            যদি (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                ধ্রুবক string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } ধরো (std::filesystem::filesystem_error ধ্রুবক& ex) {}
#অন্যথা
    যদি (DIR* ধ্রুবক dirp = opendir(dir.c_str())) {
        যখন (কাঠামো dirent* ধ্রুবক direntp = readdir(dirp)) {
            যদি (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ধ্রুবক string fullname = dir + "/" + std::string{direntp->d_name};
#যদি defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#অন্যথা
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

শূন্য V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    আকার_t sz = আকার(narenas);
    যদি (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        ফেরত;  // Failed to get number of arenas, give up
    }
    অক্ষর buf[64];
    // Index equal to narenas represents all arenas
    (শূন্য)VL_SNPRINTF(buf, আকার(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ধ্রুবক uint64_t ফলাফল = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    ফেরত ফলাফল;
}

string V3Os::trueRandom(আকার_t আকার_2) VL_MT_SAFE {
    string ফলাফল(আকার_2, '\xFF');
    অক্ষর* ধ্রুবক তথ্য = const_cast<অক্ষর*>(ফলাফল.তথ্য());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#যদি defined(_WIN32) || defined(__MINGW32__)
    ধ্রুবক NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(তথ্য), আকার_2,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    যদি (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#নতুবা_যদি defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    যদি (VL_UNCOVERABLE(getentropy(তথ্য, আকার_2))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#অন্যথা
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::দ্বিমিক};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    যদি (VL_UNCOVERABLE(!is.পঠ(তথ্য, আকার_2))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    ফেরত ফলাফল;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#যদি defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    স্থির constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    ধ্রুবক uint64_t us
        = ((static_cast<uint64_t>(ft.dw_উপরের_Date_Time) << 32) + ft.dw_নিচের_Date_Time + 5ULL) / 10ULL;
    ফেরত us - EPOCH_DIFFERENCE_USECS;
#অন্যথা
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    যদি (gettimeofday(&tv, nullptr) < 0) ফেরত 0;
    ফেরত static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

শূন্য V3Os::u_sleep(int64_t usec) {
#যদি defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#অন্যথা
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

পূর্ণসংখ্যা V3Os::system(ধ্রুবক string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    ফেরত -1;
#অন্যথা
    UINFO(1, "Running system: " << command);
    ধ্রুবক পূর্ণসংখ্যা ret = ::system(command.c_str());
    যদি (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        ফেরত -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ধ্রুবক পূর্ণসংখ্যা exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    ফেরত exit_code;
#endif
}

শূন্য V3Os::selfTest() {
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
