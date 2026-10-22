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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#অন্তর্ভাৱ "V3Os.h"
#অন্তর্ভাৱ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#অন্তর্ভাৱ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#অন্তর্ভাৱ <cerrno>
#অন্তর্ভাৱ <climits>  // PATH_MAX (especially on FreeBSD)
#অন্তর্ভাৱ <cstdarg>
#ifdef _MSC_VER
#অন্তর্ভাৱ <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#অন্যথা
#অন্তর্ভাৱ <dirent.h>
#অন্তর্ভাৱ <utime.h>
#endif
#অন্তর্ভাৱ <fcntl.h>
#অন্তর্ভাৱ <fstream>
#অন্তর্ভাৱ <memory>

#অন্তর্ভাৱ <sys/stat.h>
#অন্তর্ভাৱ <sys/types.h>

#ifdef HAVE_TCMALLOC
#অন্তর্ভাৱ <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#অন্তর্ভাৱ <jemalloc/jemalloc.h>
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
#অন্তর্ভাৱ "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ধ্রুৱ string& envvar, ধ্রুৱ string& default_মান) {
    ঘূৰাই VString::escapeStringForPath(VlOs::getenvStr(envvar, default_মান));
}

শূন্য V3Os::setenvStr(ধ্রুৱ string& envvar, ধ্রুৱ string& মান, ধ্রুৱ string& why) {
    যদি (why != "") {
        UINFO(1, "export " << envvar << "='" << মান << "'  # " << why);
    } অন্যথা {
        UINFO(1, "export " << envvar << "='" << মান << "'");
    }
#যদি defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), মান.c_str());
#নোচেদ্যদি defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), মান.c_str(), সঁচা);
#অন্যথা
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ধ্রুৱ string vareq = envvar + "=" + মান;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#যদি defined(_WIN32) || defined(__MINGW32__)
স্থির constexpr ৱর্ণ V3OS_SLASH = '\\';
#অন্যথা
স্থির constexpr ৱর্ণ V3OS_SLASH = '/';
#endif

স্থির তার্কিক isSlash(ৱর্ণ ch) VL_PURE {
#যদি defined(_WIN32) || defined(__MINGW32__)
    ঘূৰাই ch == '/' || ch == '\\';
#অন্যথা
    ঘূৰাই ch == '/';
#endif
}

string V3Os::filenameCleanup(ধ্রুৱ string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    তার্কিক lastIsSlash = মিছা;
    বাবে (ধ্রুৱ ৱর্ণ ch : filename) {
        ধ্রুৱ তার্কিক lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        যদি (lastIsSlash && lastIsSlashOld) অনুৱর্ত;
        str += ch;
    }
    যদি (str.size() > 1 && isSlash(str.back())) str.বেৰ_কৰো_back();
    যেতিয়ালৈ (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    ঘূৰাই str;
}

string V3Os::filenameJoin(std::initializer_list<ধ্রুৱ std::string> paths) VL_PURE {
    string fullpath;
    বাবে (ধ্রুৱ auto& item : paths) {
        যদি (item.empty() || item == ".") অনুৱর্ত;
        যদি (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    ঘূৰাই fullpath;
}

string V3Os::filenameDir(ধ্রুৱ string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    বাবে (; it != filename.rend(); ++it) {
        যদি (isSlash(*it)) ভঙ্গ;
    }
    যদি (it.base() == filename.begin()) ঘূৰাই ".";
    ঘূৰাই string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(ধ্রুৱ string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ধ্রুৱ string::size_type pos = base.find('.');
    যদি (pos != string::npos) {
        base.erase(0, pos);
        ঘূৰাই base;
    }
    ঘূৰাই "";
}

string V3Os::filenameNonDir(ধ্রুৱ string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    বাবে (; it != filename.rend(); ++it) {
        যদি (isSlash(*it)) ভঙ্গ;
    }
    ঘূৰাই string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(ধ্রুৱ string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ধ্রুৱ string::size_type pos = base.find('.');
    যদি (pos != string::npos) base.erase(pos);
    ঘূৰাই base;
}

string V3Os::filenameSubstitute(ধ্রুৱ string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    গণনা : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    বাবে (string::size_type pos = 0; pos < filename.length(); ++pos) {
        যদি ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            ৱিকল্প (filename[pos + 1]) {
            পক্ষ '{': brackets = CURLY; ভঙ্গ;
            পক্ষ '(': brackets = PAREN; ভঙ্গ;
            স্ৱভাৱ: brackets = NONE; ভঙ্গ;
            }
            যদি (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            যেতিয়ালৈ (((endpos + 1) < filename.length())
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
            ধ্রুৱ string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            যদি (!envvar.empty()) envvalue = getenvStr(envvar, "");
            যদি (!envvalue.empty()) {
                result += envvalue;
                যদি (brackets == NONE) {
                    pos = endpos;
                } অন্যথা {
                    pos = endpos + 1;
                }
            } অন্যথা {
                result += filename[pos];  // *pos == '$'
            }
        } অন্যথা {
            result += filename[pos];
        }
    }
    ঘূৰাই result;
}

string V3Os::filenameRealPath(ধ্রুৱ string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    ৱর্ণ retpath[PATH_MAX];
    যদি (
#যদি defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#অন্যথা
        realpath(filename.c_str(), retpath)
#endif
    ) {
        ঘূৰাই std::string{retpath};
    }
    ঘূৰাই filename;
}

string V3Os::filenameAbsolutePath(ধ্রুৱ string& filename) VL_PURE {
#যদি defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    ঘূৰাই filenameRealPath(filename);
#অন্যথা
    যদি (!filenameIsRel(filename)) ঘূৰাই filename;
    ঘূৰাই filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(ধ্রুৱ string& filename, ধ্রুৱ string& base) VL_PURE {
    ধ্রুৱ string a = V3Os::filenameRealPath(filename);
    ধ্রুৱ string b = V3Os::filenameRealPath(base);
    string result;
    যদি (a == b) ঘূৰাই ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    যেতিয়ালৈ (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        বাবে (; aWordIt != a.end(); ++aWordIt) {
            যদি (isSlash(*aWordIt)) ভঙ্গ;
        }

        auto bWordIt = bIt;  // position of next slash
        বাবে (; bWordIt != b.end(); ++bWordIt) {
            যদি (isSlash(*bWordIt)) ভঙ্গ;
        }

        ধ্রুৱ string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        ধ্রুৱ string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        যদি (aWord != bWord) ভঙ্গ;
        aIt = aWordIt;
        bIt = bWordIt;
        যদি (aIt != a.end()) ++aIt;  // Skip slash
        যদি (bIt != b.end()) ++bIt;  // Skip slash
    }

    যেতিয়ালৈ (bIt != b.end()) {
        বাবে (; bIt != b.end(); ++bIt) {
            যদি (isSlash(*bIt)) {
                ++bIt;
                ভঙ্গ;
            }
        }
        যদি (!result.empty()) result += "/";
        result += "..";
    }

    ধ্রুৱ string a_বাম = a.substr(aIt - a.begin());
    যদি (!a_বাম.empty()) {
        যদি (!result.empty()) result += "/";
        result += a_বাম;
    }
    ঘূৰাই filenameCleanup(result);
}

তার্কিক V3Os::filenameIsRel(ধ্রুৱ string& filename) VL_PURE {
#যদি defined(_MSC_VER)
    ঘূৰাই std::filesystem::path(filename).is_relative();
#অন্যথা
    ঘূৰাই (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(ধ্রুৱ string& filename) VL_PURE {
#যদি defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    ঘূৰাই slashedPath;
#অন্যথা
    ঘূৰাই filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, ৱর্ণ delim) {
    string সাৰি;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    ৱর্ণ buf[65536];
    is.getline(buf, 65535, delim);
    সাৰি = buf;
#অন্যথা
    std::getline(is, সাৰি, delim);
#endif
    ঘূৰাই সাৰি;
}

//######################################################################
// Directory utilities

শূন্য V3Os::createDir(ধ্রুৱ string& dirname) {
#যদি defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#অন্যথা
    mkdir(dirname.c_str(), 0777);
#endif
}

শূন্য V3Os::filesystemFlush(ধ্রুৱ string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#যদি defined(_MSC_VER) || defined(__MINGW32__)
    যদি (পূর্ণাঙ্ক fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        যদি (fd > 0) ::close(fd);
    }
#অন্যথা
    // Faster to just try both rather than check if a file is a dir.
    যদি (DIR* ধ্রুৱ dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } অন্যথা যদি (ধ্রুৱ পূর্ণাঙ্ক fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        যদি (fd > 0) ::close(fd);
    }
#endif
}

শূন্য V3Os::filesystemFlushBuildDir(ধ্রুৱ string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#যদি !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ধ্রুৱ পূর্ণাঙ্ক err = utimes(dirname.c_str(), nullptr);
    // Not an error
    যদি (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

শূন্য V3Os::unlinkRegexp(ধ্রুৱ string& dir, ধ্রুৱ string& regexp) {
#ifdef _MSC_VER
    যত্ন {
        বাবে (ধ্রুৱ auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            যদি (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                ধ্রুৱ string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } গৃহাণ (std::filesystem::filesystem_error ধ্রুৱ& ex) {}
#অন্যথা
    যদি (DIR* ধ্রুৱ dirp = opendir(dir.c_str())) {
        যেতিয়ালৈ (সংরচনা dirent* ধ্রুৱ direntp = readdir(dirp)) {
            যদি (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ধ্রুৱ string fullname = dir + "/" + std::string{direntp->d_name};
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
    size_t sz = পরিমাণ(narenas);
    যদি (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        ঘূৰাই;  // Failed to get number of arenas, give up
    }
    ৱর্ণ buf[64];
    // Index equal to narenas represents all arenas
    (শূন্য)VL_SNPRINTF(buf, পরিমাণ(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ধ্রুৱ uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    ঘূৰাই result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    ৱর্ণ* ধ্রুৱ data = const_cast<ৱর্ণ*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#যদি defined(_WIN32) || defined(__MINGW32__)
    ধ্রুৱ NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    যদি (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#নোচেদ্যদি defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    যদি (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#অন্যথা
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::দ্বিমিক};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    যদি (VL_UNCOVERABLE(!is.পঠ(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    ঘূৰাই result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#যদি defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    স্থির constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    ধ্রুৱ uint64_t us
        = ((static_cast<uint64_t>(ft.dw_উপৰেৰ_Date_Time) << 32) + ft.dw_নিচেৰ_Date_Time + 5ULL) / 10ULL;
    ঘূৰাই us - EPOCH_DIFFERENCE_USECS;
#অন্যথা
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    যদি (gettimeofday(&tv, nullptr) < 0) ঘূৰাই 0;
    ঘূৰাই static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
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

পূর্ণাঙ্ক V3Os::system(ধ্রুৱ string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    ঘূৰাই -1;
#অন্যথা
    UINFO(1, "Running system: " << command);
    ধ্রুৱ পূর্ণাঙ্ক ret = ::system(command.c_str());
    যদি (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        ঘূৰাই -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ধ্রুৱ পূর্ণাঙ্ক exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    ঘূৰাই exit_code;
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
