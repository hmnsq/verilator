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
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#ಅನ್ತರ್ಭಾವ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#ಅನ್ತರ್ಭಾವ <cerrno>
#ಅನ್ತರ್ಭಾವ <climits>  // PATH_MAX (especially on FreeBSD)
#ಅನ್ತರ್ಭಾವ <cstdarg>
#ifdef _MSC_VER
#ಅನ್ತರ್ಭಾವ <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#ಅನ್ಯಥಾ
#ಅನ್ತರ್ಭಾವ <dirent.h>
#ಅನ್ತರ್ಭಾವ <utime.h>
#endif
#ಅನ್ತರ್ಭಾವ <fcntl.h>
#ಅನ್ತರ್ಭಾವ <fstream>
#ಅನ್ತರ್ಭಾವ <memory>

#ಅನ್ತರ್ಭಾವ <sys/stat.h>
#ಅನ್ತರ್ಭಾವ <sys/types.h>

#ifdef HAVE_TCMALLOC
#ಅನ್ತರ್ಭಾವ <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#ಅನ್ತರ್ಭಾವ <jemalloc/jemalloc.h>
#endif

// clang-format off
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
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
#  ಅನ್ಯಥಾ
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#ಅನ್ಯಥಾ
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#ಅನ್ತರ್ಭಾವ "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ಧ್ರುವ string& envvar, ಧ್ರುವ string& default_ಮೌಲ್ಯ) {
    ಪ್ರತ್ಯಾಗಮ VString::escapeStringForPath(VlOs::getenvStr(envvar, default_ಮೌಲ್ಯ));
}

ಶೂನ್ಯ V3Os::setenvStr(ಧ್ರುವ string& envvar, ಧ್ರುವ string& ಮೌಲ್ಯ, ಧ್ರುವ string& why) {
    ಯದಿ (why != "") {
        UINFO(1, "export " << envvar << "='" << ಮೌಲ್ಯ << "'  # " << why);
    } ಅನ್ಯಥಾ {
        UINFO(1, "export " << envvar << "='" << ಮೌಲ್ಯ << "'");
    }
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), ಮೌಲ್ಯ.c_str());
#ನೋಚೇದ್ಯದಿ defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), ಮೌಲ್ಯ.c_str(), ಸತ್ಯ);
#ಅನ್ಯಥಾ
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ಧ್ರುವ string vareq = envvar + "=" + ಮೌಲ್ಯ;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
ಸ್ಥಿರ constexpr ವರ್ಣ V3OS_SLASH = '\\';
#ಅನ್ಯಥಾ
ಸ್ಥಿರ constexpr ವರ್ಣ V3OS_SLASH = '/';
#endif

ಸ್ಥಿರ ತಾರ್ಕಿಕ isSlash(ವರ್ಣ ch) VL_PURE {
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
    ಪ್ರತ್ಯಾಗಮ ch == '/' || ch == '\\';
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ ch == '/';
#endif
}

string V3Os::filenameCleanup(ಧ್ರುವ string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    ತಾರ್ಕಿಕ lastIsSlash = ಅಸತ್ಯ;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ ch : filename) {
        ಧ್ರುವ ತಾರ್ಕಿಕ lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        ಯದಿ (lastIsSlash && lastIsSlashOld) ಅನುವರ್ತ;
        str += ch;
    }
    ಯದಿ (str.size() > 1 && isSlash(str.back())) str.ತೆಗೆ_back();
    ಯಾವತ್ (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    ಪ್ರತ್ಯಾಗಮ str;
}

string V3Os::filenameJoin(std::initializer_list<ಧ್ರುವ std::string> paths) VL_PURE {
    string fullpath;
    ಪ್ರತಿ (ಧ್ರುವ auto& item : paths) {
        ಯದಿ (item.empty() || item == ".") ಅನುವರ್ತ;
        ಯದಿ (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    ಪ್ರತ್ಯಾಗಮ fullpath;
}

string V3Os::filenameDir(ಧ್ರುವ string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    ಪ್ರತಿ (; it != filename.rend(); ++it) {
        ಯದಿ (isSlash(*it)) ಭಙ್ಗ;
    }
    ಯದಿ (it.base() == filename.begin()) ಪ್ರತ್ಯಾಗಮ ".";
    ಪ್ರತ್ಯಾಗಮ string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(ಧ್ರುವ string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ಧ್ರುವ string::size_type pos = base.find('.');
    ಯದಿ (pos != string::npos) {
        base.erase(0, pos);
        ಪ್ರತ್ಯಾಗಮ base;
    }
    ಪ್ರತ್ಯಾಗಮ "";
}

string V3Os::filenameNonDir(ಧ್ರುವ string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    ಪ್ರತಿ (; it != filename.rend(); ++it) {
        ಯದಿ (isSlash(*it)) ಭಙ್ಗ;
    }
    ಪ್ರತ್ಯಾಗಮ string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(ಧ್ರುವ string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ಧ್ರುವ string::size_type pos = base.find('.');
    ಯದಿ (pos != string::npos) base.erase(pos);
    ಪ್ರತ್ಯಾಗಮ base;
}

string V3Os::filenameSubstitute(ಧ್ರುವ string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    ಗಣನಾ : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    ಪ್ರತಿ (string::size_type pos = 0; pos < filename.length(); ++pos) {
        ಯದಿ ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            ವಿಕಲ್ಪ (filename[pos + 1]) {
            ಪಕ್ಷ '{': brackets = CURLY; ಭಙ್ಗ;
            ಪಕ್ಷ '(': brackets = PAREN; ಭಙ್ಗ;
            ಸ್ವಭಾವ: brackets = NONE; ಭಙ್ಗ;
            }
            ಯದಿ (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            ಯಾವತ್ (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            ಯದಿ (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ಧ್ರುವ string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            ಯದಿ (!envvar.empty()) envvalue = getenvStr(envvar, "");
            ಯದಿ (!envvalue.empty()) {
                result += envvalue;
                ಯದಿ (brackets == NONE) {
                    pos = endpos;
                } ಅನ್ಯಥಾ {
                    pos = endpos + 1;
                }
            } ಅನ್ಯಥಾ {
                result += filename[pos];  // *pos == '$'
            }
        } ಅನ್ಯಥಾ {
            result += filename[pos];
        }
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

string V3Os::filenameRealPath(ಧ್ರುವ string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    ವರ್ಣ retpath[PATH_MAX];
    ಯದಿ (
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#ಅನ್ಯಥಾ
        realpath(filename.c_str(), retpath)
#endif
    ) {
        ಪ್ರತ್ಯಾಗಮ std::string{retpath};
    }
    ಪ್ರತ್ಯಾಗಮ filename;
}

string V3Os::filenameAbsolutePath(ಧ್ರುವ string& filename) VL_PURE {
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    ಪ್ರತ್ಯಾಗಮ filenameRealPath(filename);
#ಅನ್ಯಥಾ
    ಯದಿ (!filenameIsRel(filename)) ಪ್ರತ್ಯಾಗಮ filename;
    ಪ್ರತ್ಯಾಗಮ filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(ಧ್ರುವ string& filename, ಧ್ರುವ string& base) VL_PURE {
    ಧ್ರುವ string a = V3Os::filenameRealPath(filename);
    ಧ್ರುವ string b = V3Os::filenameRealPath(base);
    string result;
    ಯದಿ (a == b) ಪ್ರತ್ಯಾಗಮ ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    ಯಾವತ್ (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        ಪ್ರತಿ (; aWordIt != a.end(); ++aWordIt) {
            ಯದಿ (isSlash(*aWordIt)) ಭಙ್ಗ;
        }

        auto bWordIt = bIt;  // position of next slash
        ಪ್ರತಿ (; bWordIt != b.end(); ++bWordIt) {
            ಯದಿ (isSlash(*bWordIt)) ಭಙ್ಗ;
        }

        ಧ್ರುವ string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        ಧ್ರುವ string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        ಯದಿ (aWord != bWord) ಭಙ್ಗ;
        aIt = aWordIt;
        bIt = bWordIt;
        ಯದಿ (aIt != a.end()) ++aIt;  // Skip slash
        ಯದಿ (bIt != b.end()) ++bIt;  // Skip slash
    }

    ಯಾವತ್ (bIt != b.end()) {
        ಪ್ರತಿ (; bIt != b.end(); ++bIt) {
            ಯದಿ (isSlash(*bIt)) {
                ++bIt;
                ಭಙ್ಗ;
            }
        }
        ಯದಿ (!result.empty()) result += "/";
        result += "..";
    }

    ಧ್ರುವ string a_ಎಡ = a.substr(aIt - a.begin());
    ಯದಿ (!a_ಎಡ.empty()) {
        ಯದಿ (!result.empty()) result += "/";
        result += a_ಎಡ;
    }
    ಪ್ರತ್ಯಾಗಮ filenameCleanup(result);
}

ತಾರ್ಕಿಕ V3Os::filenameIsRel(ಧ್ರುವ string& filename) VL_PURE {
#ಯದಿ defined(_MSC_VER)
    ಪ್ರತ್ಯಾಗಮ std::filesystem::path(filename).is_relative();
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(ಧ್ರುವ string& filename) VL_PURE {
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    ಪ್ರತ್ಯಾಗಮ slashedPath;
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, ವರ್ಣ delim) {
    string ಸಾಲು;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    ವರ್ಣ buf[65536];
    is.getline(buf, 65535, delim);
    ಸಾಲು = buf;
#ಅನ್ಯಥಾ
    std::getline(is, ಸಾಲು, delim);
#endif
    ಪ್ರತ್ಯಾಗಮ ಸಾಲು;
}

//######################################################################
// Directory utilities

ಶೂನ್ಯ V3Os::createDir(ಧ್ರುವ string& dirname) {
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#ಅನ್ಯಥಾ
    mkdir(dirname.c_str(), 0777);
#endif
}

ಶೂನ್ಯ V3Os::filesystemFlush(ಧ್ರುವ string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#ಯದಿ defined(_MSC_VER) || defined(__MINGW32__)
    ಯದಿ (ಪೂರ್ಣಾಙ್ಕ fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ಯದಿ (fd > 0) ::close(fd);
    }
#ಅನ್ಯಥಾ
    // Faster to just try both rather than check if a file is a dir.
    ಯದಿ (DIR* ಧ್ರುವ dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ಯದಿ (fd > 0) ::close(fd);
    }
#endif
}

ಶೂನ್ಯ V3Os::filesystemFlushBuildDir(ಧ್ರುವ string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#ಯದಿ !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ err = utimes(dirname.c_str(), nullptr);
    // Not an error
    ಯದಿ (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

ಶೂನ್ಯ V3Os::unlinkRegexp(ಧ್ರುವ string& dir, ಧ್ರುವ string& regexp) {
#ifdef _MSC_VER
    ಯತ್ನ {
        ಪ್ರತಿ (ಧ್ರುವ auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            ಯದಿ (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                ಧ್ರುವ string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } ಗೃಹಾಣ (std::filesystem::filesystem_error ಧ್ರುವ& ex) {}
#ಅನ್ಯಥಾ
    ಯದಿ (DIR* ಧ್ರುವ dirp = opendir(dir.c_str())) {
        ಯಾವತ್ (ಸಂರಚನಾ dirent* ಧ್ರುವ direntp = readdir(dirp)) {
            ಯದಿ (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ಧ್ರುವ string fullname = dir + "/" + std::string{direntp->d_name};
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#ಅನ್ಯಥಾ
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

ಶೂನ್ಯ V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = ಪರಿಮಾಣ(narenas);
    ಯದಿ (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        ಪ್ರತ್ಯಾಗಮ;  // Failed to get number of arenas, give up
    }
    ವರ್ಣ buf[64];
    // Index equal to narenas represents all arenas
    (ಶೂನ್ಯ)VL_SNPRINTF(buf, ಪರಿಮಾಣ(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ಧ್ರುವ uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    ಪ್ರತ್ಯಾಗಮ result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    ವರ್ಣ* ಧ್ರುವ data = const_cast<ವರ್ಣ*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
    ಧ್ರುವ NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ಯದಿ (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#ನೋಚೇದ್ಯದಿ defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    ಯದಿ (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#ಅನ್ಯಥಾ
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::ದ್ವಿಮಾನ};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    ಯದಿ (VL_UNCOVERABLE(!is.ಪಠ(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    ಪ್ರತ್ಯಾಗಮ result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ಸ್ಥಿರ constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    ಧ್ರುವ uint64_t us
        = ((static_cast<uint64_t>(ft.dw_ಮೇಲಿನ_Date_Time) << 32) + ft.dw_ಕೆಳಗಿನ_Date_Time + 5ULL) / 10ULL;
    ಪ್ರತ್ಯಾಗಮ us - EPOCH_DIFFERENCE_USECS;
#ಅನ್ಯಥಾ
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    ಯದಿ (gettimeofday(&tv, nullptr) < 0) ಪ್ರತ್ಯಾಗಮ 0;
    ಪ್ರತ್ಯಾಗಮ static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

ಶೂನ್ಯ V3Os::u_sleep(int64_t usec) {
#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#ಅನ್ಯಥಾ
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

ಪೂರ್ಣಾಙ್ಕ V3Os::system(ಧ್ರುವ string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    ಪ್ರತ್ಯಾಗಮ -1;
#ಅನ್ಯಥಾ
    UINFO(1, "Running system: " << command);
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ret = ::system(command.c_str());
    ಯದಿ (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        ಪ್ರತ್ಯಾಗಮ -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    ಪ್ರತ್ಯಾಗಮ exit_code;
#endif
}

ಶೂನ್ಯ V3Os::selfTest() {
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
