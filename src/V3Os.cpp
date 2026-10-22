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
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#ଅନ୍ତର୍ଭାଵ <cerrno>
#ଅନ୍ତର୍ଭାଵ <climits>  // PATH_MAX (especially on FreeBSD)
#ଅନ୍ତର୍ଭାଵ <cstdarg>
#ifdef _MSC_VER
#ଅନ୍ତର୍ଭାଵ <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#ଅନ୍ୟଥା
#ଅନ୍ତର୍ଭାଵ <dirent.h>
#ଅନ୍ତର୍ଭାଵ <utime.h>
#endif
#ଅନ୍ତର୍ଭାଵ <fcntl.h>
#ଅନ୍ତର୍ଭାଵ <fstream>
#ଅନ୍ତର୍ଭାଵ <memory>

#ଅନ୍ତର୍ଭାଵ <sys/stat.h>
#ଅନ୍ତର୍ଭାଵ <sys/types.h>

#ifdef HAVE_TCMALLOC
#ଅନ୍ତର୍ଭାଵ <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#ଅନ୍ତର୍ଭାଵ <jemalloc/jemalloc.h>
#endif

// clang-format off
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
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
#  ଅନ୍ୟଥା
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#ଅନ୍ୟଥା
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#ଅନ୍ତର୍ଭାଵ "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ଅଚଳ string& envvar, ଅଚଳ string& default_ମୂଲ୍ୟ) {
    ଫେରନ୍ତୁ VString::escapeStringForPath(VlOs::getenvStr(envvar, default_ମୂଲ୍ୟ));
}

ଶୂନ୍ୟ V3Os::setenvStr(ଅଚଳ string& envvar, ଅଚଳ string& ମୂଲ୍ୟ, ଅଚଳ string& why) {
    ଯଦି (why != "") {
        UINFO(1, "export " << envvar << "='" << ମୂଲ୍ୟ << "'  # " << why);
    } ଅନ୍ୟଥା {
        UINFO(1, "export " << envvar << "='" << ମୂଲ୍ୟ << "'");
    }
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), ମୂଲ୍ୟ.c_str());
#ନଚେତଯଦି defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), ମୂଲ୍ୟ.c_str(), ସତ୍ୟ);
#ଅନ୍ୟଥା
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ଅଚଳ string vareq = envvar + "=" + ମୂଲ୍ୟ;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#ଯଦି defined(_WIN32) || defined(__MINGW32__)
ସ୍ଥିର constexpr ଅକ୍ଷର V3OS_SLASH = '\\';
#ଅନ୍ୟଥା
ସ୍ଥିର constexpr ଅକ୍ଷର V3OS_SLASH = '/';
#endif

ସ୍ଥିର ତାର୍କିକ isSlash(ଅକ୍ଷର ch) VL_PURE {
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
    ଫେରନ୍ତୁ ch == '/' || ch == '\\';
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ ch == '/';
#endif
}

string V3Os::filenameCleanup(ଅଚଳ string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    ତାର୍କିକ lastIsSlash = ମିଥ୍ୟା;
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର ch : filename) {
        ଅଚଳ ତାର୍କିକ lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        ଯଦି (lastIsSlash && lastIsSlashOld) ଜାରି;
        str += ch;
    }
    ଯଦି (str.size() > 1 && isSlash(str.back())) str.କାଢ଼_back();
    ଯେତେବେଳେ (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    ଫେରନ୍ତୁ str;
}

string V3Os::filenameJoin(std::initializer_list<ଅଚଳ std::string> paths) VL_PURE {
    string fullpath;
    ପାଇଁ (ଅଚଳ auto& item : paths) {
        ଯଦି (item.empty() || item == ".") ଜାରି;
        ଯଦି (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    ଫେରନ୍ତୁ fullpath;
}

string V3Os::filenameDir(ଅଚଳ string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    ପାଇଁ (; it != filename.rend(); ++it) {
        ଯଦି (isSlash(*it)) ଭାଙ୍ଗ;
    }
    ଯଦି (it.base() == filename.begin()) ଫେରନ୍ତୁ ".";
    ଫେରନ୍ତୁ string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(ଅଚଳ string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ଅଚଳ string::size_type pos = base.find('.');
    ଯଦି (pos != string::npos) {
        base.erase(0, pos);
        ଫେରନ୍ତୁ base;
    }
    ଫେରନ୍ତୁ "";
}

string V3Os::filenameNonDir(ଅଚଳ string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    ପାଇଁ (; it != filename.rend(); ++it) {
        ଯଦି (isSlash(*it)) ଭାଙ୍ଗ;
    }
    ଫେରନ୍ତୁ string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(ଅଚଳ string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ଅଚଳ string::size_type pos = base.find('.');
    ଯଦି (pos != string::npos) base.erase(pos);
    ଫେରନ୍ତୁ base;
}

string V3Os::filenameSubstitute(ଅଚଳ string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    ଗଣନା : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    ପାଇଁ (string::size_type pos = 0; pos < filename.length(); ++pos) {
        ଯଦି ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            ବାଛ (filename[pos + 1]) {
            ସ୍ଥିତି '{': brackets = CURLY; ଭାଙ୍ଗ;
            ସ୍ଥିତି '(': brackets = PAREN; ଭାଙ୍ଗ;
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ: brackets = NONE; ଭାଙ୍ଗ;
            }
            ଯଦି (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            ଯେତେବେଳେ (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            ଯଦି (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ଅଚଳ string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            ଯଦି (!envvar.empty()) envvalue = getenvStr(envvar, "");
            ଯଦି (!envvalue.empty()) {
                result += envvalue;
                ଯଦି (brackets == NONE) {
                    pos = endpos;
                } ଅନ୍ୟଥା {
                    pos = endpos + 1;
                }
            } ଅନ୍ୟଥା {
                result += filename[pos];  // *pos == '$'
            }
        } ଅନ୍ୟଥା {
            result += filename[pos];
        }
    }
    ଫେରନ୍ତୁ result;
}

string V3Os::filenameRealPath(ଅଚଳ string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    ଅକ୍ଷର retpath[PATH_MAX];
    ଯଦି (
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#ଅନ୍ୟଥା
        realpath(filename.c_str(), retpath)
#endif
    ) {
        ଫେରନ୍ତୁ std::string{retpath};
    }
    ଫେରନ୍ତୁ filename;
}

string V3Os::filenameAbsolutePath(ଅଚଳ string& filename) VL_PURE {
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    ଫେରନ୍ତୁ filenameRealPath(filename);
#ଅନ୍ୟଥା
    ଯଦି (!filenameIsRel(filename)) ଫେରନ୍ତୁ filename;
    ଫେରନ୍ତୁ filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(ଅଚଳ string& filename, ଅଚଳ string& base) VL_PURE {
    ଅଚଳ string a = V3Os::filenameRealPath(filename);
    ଅଚଳ string b = V3Os::filenameRealPath(base);
    string result;
    ଯଦି (a == b) ଫେରନ୍ତୁ ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    ଯେତେବେଳେ (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        ପାଇଁ (; aWordIt != a.end(); ++aWordIt) {
            ଯଦି (isSlash(*aWordIt)) ଭାଙ୍ଗ;
        }

        auto bWordIt = bIt;  // position of next slash
        ପାଇଁ (; bWordIt != b.end(); ++bWordIt) {
            ଯଦି (isSlash(*bWordIt)) ଭାଙ୍ଗ;
        }

        ଅଚଳ string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        ଅଚଳ string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        ଯଦି (aWord != bWord) ଭାଙ୍ଗ;
        aIt = aWordIt;
        bIt = bWordIt;
        ଯଦି (aIt != a.end()) ++aIt;  // Skip slash
        ଯଦି (bIt != b.end()) ++bIt;  // Skip slash
    }

    ଯେତେବେଳେ (bIt != b.end()) {
        ପାଇଁ (; bIt != b.end(); ++bIt) {
            ଯଦି (isSlash(*bIt)) {
                ++bIt;
                ଭାଙ୍ଗ;
            }
        }
        ଯଦି (!result.empty()) result += "/";
        result += "..";
    }

    ଅଚଳ string a_ବାମ = a.substr(aIt - a.begin());
    ଯଦି (!a_ବାମ.empty()) {
        ଯଦି (!result.empty()) result += "/";
        result += a_ବାମ;
    }
    ଫେରନ୍ତୁ filenameCleanup(result);
}

ତାର୍କିକ V3Os::filenameIsRel(ଅଚଳ string& filename) VL_PURE {
#ଯଦି defined(_MSC_VER)
    ଫେରନ୍ତୁ std::filesystem::path(filename).is_relative();
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(ଅଚଳ string& filename) VL_PURE {
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    ଫେରନ୍ତୁ slashedPath;
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, ଅକ୍ଷର delim) {
    string ଧାଡ଼ି;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    ଅକ୍ଷର buf[65536];
    is.getline(buf, 65535, delim);
    ଧାଡ଼ି = buf;
#ଅନ୍ୟଥା
    std::getline(is, ଧାଡ଼ି, delim);
#endif
    ଫେରନ୍ତୁ ଧାଡ଼ି;
}

//######################################################################
// Directory utilities

ଶୂନ୍ୟ V3Os::createDir(ଅଚଳ string& dirname) {
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#ଅନ୍ୟଥା
    mkdir(dirname.c_str(), 0777);
#endif
}

ଶୂନ୍ୟ V3Os::filesystemFlush(ଅଚଳ string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#ଯଦି defined(_MSC_VER) || defined(__MINGW32__)
    ଯଦି (ପୂର୍ଣ୍ଣସଂଖ୍ୟା fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ଯଦି (fd > 0) ::close(fd);
    }
#ଅନ୍ୟଥା
    // Faster to just try both rather than check if a file is a dir.
    ଯଦି (DIR* ଅଚଳ dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ଯଦି (fd > 0) ::close(fd);
    }
#endif
}

ଶୂନ୍ୟ V3Os::filesystemFlushBuildDir(ଅଚଳ string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#ଯଦି !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା err = utimes(dirname.c_str(), nullptr);
    // Not an error
    ଯଦି (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

ଶୂନ୍ୟ V3Os::unlinkRegexp(ଅଚଳ string& dir, ଅଚଳ string& regexp) {
#ifdef _MSC_VER
    ଚେଷ୍ଟା {
        ପାଇଁ (ଅଚଳ auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            ଯଦି (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                ଅଚଳ string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } ଧର (std::filesystem::filesystem_error ଅଚଳ& ex) {}
#ଅନ୍ୟଥା
    ଯଦି (DIR* ଅଚଳ dirp = opendir(dir.c_str())) {
        ଯେତେବେଳେ (ସଂରଚନା dirent* ଅଚଳ direntp = readdir(dirp)) {
            ଯଦି (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ଅଚଳ string fullname = dir + "/" + std::string{direntp->d_name};
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#ଅନ୍ୟଥା
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

ଶୂନ୍ୟ V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = ଆକାର(narenas);
    ଯଦି (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        ଫେରନ୍ତୁ;  // Failed to get number of arenas, give up
    }
    ଅକ୍ଷର buf[64];
    // Index equal to narenas represents all arenas
    (ଶୂନ୍ୟ)VL_SNPRINTF(buf, ଆକାର(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ଅଚଳ uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    ଫେରନ୍ତୁ result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    ଅକ୍ଷର* ଅଚଳ data = const_cast<ଅକ୍ଷର*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
    ଅଚଳ NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ଯଦି (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#ନଚେତଯଦି defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    ଯଦି (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#ଅନ୍ୟଥା
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::ଦ୍ୱିମିକ};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    ଯଦି (VL_UNCOVERABLE(!is.ପଠ(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    ଫେରନ୍ତୁ result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ସ୍ଥିର constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    ଅଚଳ uint64_t us
        = ((static_cast<uint64_t>(ft.dw_ଉପର_Date_Time) << 32) + ft.dw_ତଳ_Date_Time + 5ULL) / 10ULL;
    ଫେରନ୍ତୁ us - EPOCH_DIFFERENCE_USECS;
#ଅନ୍ୟଥା
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    ଯଦି (gettimeofday(&tv, nullptr) < 0) ଫେରନ୍ତୁ 0;
    ଫେରନ୍ତୁ static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

ଶୂନ୍ୟ V3Os::u_sleep(int64_t usec) {
#ଯଦି defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#ଅନ୍ୟଥା
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3Os::system(ଅଚଳ string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    ଫେରନ୍ତୁ -1;
#ଅନ୍ୟଥା
    UINFO(1, "Running system: " << command);
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ret = ::system(command.c_str());
    ଯଦି (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        ଫେରନ୍ତୁ -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    ଫେରନ୍ତୁ exit_code;
#endif
}

ଶୂନ୍ୟ V3Os::selfTest() {
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
