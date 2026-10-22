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
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#അന്തര്ഭാവ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#അന്തര്ഭാവ <cerrno>
#അന്തര്ഭാവ <climits>  // PATH_MAX (especially on FreeBSD)
#അന്തര്ഭാവ <cstdarg>
#ifdef _MSC_VER
#അന്തര്ഭാവ <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#അല്ലെങ്കിൽ
#അന്തര്ഭാവ <dirent.h>
#അന്തര്ഭാവ <utime.h>
#endif
#അന്തര്ഭാവ <fcntl.h>
#അന്തര്ഭാവ <fstream>
#അന്തര്ഭാവ <memory>

#അന്തര്ഭാവ <sys/stat.h>
#അന്തര്ഭാവ <sys/types.h>

#ifdef HAVE_TCMALLOC
#അന്തര്ഭാവ <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#അന്തര്ഭാവ <jemalloc/jemalloc.h>
#endif

// clang-format off
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
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
#  അല്ലെങ്കിൽ
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#അല്ലെങ്കിൽ
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#അന്തര്ഭാവ "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(സ്ഥിരം string& envvar, സ്ഥിരം string& default_മൂല്യം) {
    തിരികെ VString::escapeStringForPath(VlOs::getenvStr(envvar, default_മൂല്യം));
}

ശൂന്യം V3Os::setenvStr(സ്ഥിരം string& envvar, സ്ഥിരം string& മൂല്യം, സ്ഥിരം string& why) {
    എങ്കിൽ (why != "") {
        UINFO(1, "export " << envvar << "='" << മൂല്യം << "'  # " << why);
    } അല്ലെങ്കിൽ {
        UINFO(1, "export " << envvar << "='" << മൂല്യം << "'");
    }
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), മൂല്യം.c_str());
#അല്ലെങ്കിലെങ്കിൽ defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), മൂല്യം.c_str(), സത്യം);
#അല്ലെങ്കിൽ
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    സ്ഥിരം string vareq = envvar + "=" + മൂല്യം;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
സ്ഥിതം constexpr അക്ഷരം V3OS_SLASH = '\\';
#അല്ലെങ്കിൽ
സ്ഥിതം constexpr അക്ഷരം V3OS_SLASH = '/';
#endif

സ്ഥിതം തർക്കം isSlash(അക്ഷരം ch) VL_PURE {
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
    തിരികെ ch == '/' || ch == '\\';
#അല്ലെങ്കിൽ
    തിരികെ ch == '/';
#endif
}

string V3Os::filenameCleanup(സ്ഥിരം string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    തർക്കം lastIsSlash = അസത്യം;
    വേണ്ടി (സ്ഥിരം അക്ഷരം ch : filename) {
        സ്ഥിരം തർക്കം lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        എങ്കിൽ (lastIsSlash && lastIsSlashOld) തുടരുക;
        str += ch;
    }
    എങ്കിൽ (str.size() > 1 && isSlash(str.back())) str.എടുക്കുക_back();
    ആകുമ്പോൾ (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    തിരികെ str;
}

string V3Os::filenameJoin(std::initializer_list<സ്ഥിരം std::string> paths) VL_PURE {
    string fullpath;
    വേണ്ടി (സ്ഥിരം auto& item : paths) {
        എങ്കിൽ (item.empty() || item == ".") തുടരുക;
        എങ്കിൽ (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    തിരികെ fullpath;
}

string V3Os::filenameDir(സ്ഥിരം string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    വേണ്ടി (; it != filename.rend(); ++it) {
        എങ്കിൽ (isSlash(*it)) നിർത്തുക;
    }
    എങ്കിൽ (it.base() == filename.begin()) തിരികെ ".";
    തിരികെ string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(സ്ഥിരം string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    സ്ഥിരം string::size_type pos = base.find('.');
    എങ്കിൽ (pos != string::npos) {
        base.erase(0, pos);
        തിരികെ base;
    }
    തിരികെ "";
}

string V3Os::filenameNonDir(സ്ഥിരം string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    വേണ്ടി (; it != filename.rend(); ++it) {
        എങ്കിൽ (isSlash(*it)) നിർത്തുക;
    }
    തിരികെ string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(സ്ഥിരം string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    സ്ഥിരം string::size_type pos = base.find('.');
    എങ്കിൽ (pos != string::npos) base.erase(pos);
    തിരികെ base;
}

string V3Os::filenameSubstitute(സ്ഥിരം string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    എണ്ണൽ : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    വേണ്ടി (string::size_type pos = 0; pos < filename.length(); ++pos) {
        എങ്കിൽ ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            തിരഞ്ഞെടുപ്പ് (filename[pos + 1]) {
            അവസ്ഥ '{': brackets = CURLY; നിർത്തുക;
            അവസ്ഥ '(': brackets = PAREN; നിർത്തുക;
            സ്ഥിരസ്ഥിതി: brackets = NONE; നിർത്തുക;
            }
            എങ്കിൽ (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            ആകുമ്പോൾ (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            എങ്കിൽ (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            സ്ഥിരം string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            എങ്കിൽ (!envvar.empty()) envvalue = getenvStr(envvar, "");
            എങ്കിൽ (!envvalue.empty()) {
                result += envvalue;
                എങ്കിൽ (brackets == NONE) {
                    pos = endpos;
                } അല്ലെങ്കിൽ {
                    pos = endpos + 1;
                }
            } അല്ലെങ്കിൽ {
                result += filename[pos];  // *pos == '$'
            }
        } അല്ലെങ്കിൽ {
            result += filename[pos];
        }
    }
    തിരികെ result;
}

string V3Os::filenameRealPath(സ്ഥിരം string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    അക്ഷരം retpath[PATH_MAX];
    എങ്കിൽ (
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#അല്ലെങ്കിൽ
        realpath(filename.c_str(), retpath)
#endif
    ) {
        തിരികെ std::string{retpath};
    }
    തിരികെ filename;
}

string V3Os::filenameAbsolutePath(സ്ഥിരം string& filename) VL_PURE {
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    തിരികെ filenameRealPath(filename);
#അല്ലെങ്കിൽ
    എങ്കിൽ (!filenameIsRel(filename)) തിരികെ filename;
    തിരികെ filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(സ്ഥിരം string& filename, സ്ഥിരം string& base) VL_PURE {
    സ്ഥിരം string a = V3Os::filenameRealPath(filename);
    സ്ഥിരം string b = V3Os::filenameRealPath(base);
    string result;
    എങ്കിൽ (a == b) തിരികെ ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    ആകുമ്പോൾ (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        വേണ്ടി (; aWordIt != a.end(); ++aWordIt) {
            എങ്കിൽ (isSlash(*aWordIt)) നിർത്തുക;
        }

        auto bWordIt = bIt;  // position of next slash
        വേണ്ടി (; bWordIt != b.end(); ++bWordIt) {
            എങ്കിൽ (isSlash(*bWordIt)) നിർത്തുക;
        }

        സ്ഥിരം string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        സ്ഥിരം string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        എങ്കിൽ (aWord != bWord) നിർത്തുക;
        aIt = aWordIt;
        bIt = bWordIt;
        എങ്കിൽ (aIt != a.end()) ++aIt;  // Skip slash
        എങ്കിൽ (bIt != b.end()) ++bIt;  // Skip slash
    }

    ആകുമ്പോൾ (bIt != b.end()) {
        വേണ്ടി (; bIt != b.end(); ++bIt) {
            എങ്കിൽ (isSlash(*bIt)) {
                ++bIt;
                നിർത്തുക;
            }
        }
        എങ്കിൽ (!result.empty()) result += "/";
        result += "..";
    }

    സ്ഥിരം string a_ഇടത് = a.substr(aIt - a.begin());
    എങ്കിൽ (!a_ഇടത്.empty()) {
        എങ്കിൽ (!result.empty()) result += "/";
        result += a_ഇടത്;
    }
    തിരികെ filenameCleanup(result);
}

തർക്കം V3Os::filenameIsRel(സ്ഥിരം string& filename) VL_PURE {
#എങ്കിൽ defined(_MSC_VER)
    തിരികെ std::filesystem::path(filename).is_relative();
#അല്ലെങ്കിൽ
    തിരികെ (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(സ്ഥിരം string& filename) VL_PURE {
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    തിരികെ slashedPath;
#അല്ലെങ്കിൽ
    തിരികെ filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, അക്ഷരം delim) {
    string വരി;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    അക്ഷരം buf[65536];
    is.getline(buf, 65535, delim);
    വരി = buf;
#അല്ലെങ്കിൽ
    std::getline(is, വരി, delim);
#endif
    തിരികെ വരി;
}

//######################################################################
// Directory utilities

ശൂന്യം V3Os::createDir(സ്ഥിരം string& dirname) {
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#അല്ലെങ്കിൽ
    mkdir(dirname.c_str(), 0777);
#endif
}

ശൂന്യം V3Os::filesystemFlush(സ്ഥിരം string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#എങ്കിൽ defined(_MSC_VER) || defined(__MINGW32__)
    എങ്കിൽ (പൂർണ്ണസംഖ്യ fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        എങ്കിൽ (fd > 0) ::close(fd);
    }
#അല്ലെങ്കിൽ
    // Faster to just try both rather than check if a file is a dir.
    എങ്കിൽ (DIR* സ്ഥിരം dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        എങ്കിൽ (fd > 0) ::close(fd);
    }
#endif
}

ശൂന്യം V3Os::filesystemFlushBuildDir(സ്ഥിരം string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#എങ്കിൽ !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    സ്ഥിരം പൂർണ്ണസംഖ്യ err = utimes(dirname.c_str(), nullptr);
    // Not an error
    എങ്കിൽ (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

ശൂന്യം V3Os::unlinkRegexp(സ്ഥിരം string& dir, സ്ഥിരം string& regexp) {
#ifdef _MSC_VER
    ശ്രമിക്കുക {
        വേണ്ടി (സ്ഥിരം auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            എങ്കിൽ (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                സ്ഥിരം string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } പിടിക്കുക (std::filesystem::filesystem_error സ്ഥിരം& ex) {}
#അല്ലെങ്കിൽ
    എങ്കിൽ (DIR* സ്ഥിരം dirp = opendir(dir.c_str())) {
        ആകുമ്പോൾ (ഘടന dirent* സ്ഥിരം direntp = readdir(dirp)) {
            എങ്കിൽ (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                സ്ഥിരം string fullname = dir + "/" + std::string{direntp->d_name};
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#അല്ലെങ്കിൽ
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

ശൂന്യം V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = വലുപ്പം(narenas);
    എങ്കിൽ (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        തിരികെ;  // Failed to get number of arenas, give up
    }
    അക്ഷരം buf[64];
    // Index equal to narenas represents all arenas
    (ശൂന്യം)VL_SNPRINTF(buf, വലുപ്പം(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    സ്ഥിരം uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    തിരികെ result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    അക്ഷരം* സ്ഥിരം data = const_cast<അക്ഷരം*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
    സ്ഥിരം NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    എങ്കിൽ (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#അല്ലെങ്കിലെങ്കിൽ defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    എങ്കിൽ (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#അല്ലെങ്കിൽ
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::ദ്വിമാന};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    എങ്കിൽ (VL_UNCOVERABLE(!is.പഠ(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    തിരികെ result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    സ്ഥിതം constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    സ്ഥിരം uint64_t us
        = ((static_cast<uint64_t>(ft.dw_മുകളിലെ_Date_Time) << 32) + ft.dw_താഴെയുള്ള_Date_Time + 5ULL) / 10ULL;
    തിരികെ us - EPOCH_DIFFERENCE_USECS;
#അല്ലെങ്കിൽ
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    എങ്കിൽ (gettimeofday(&tv, nullptr) < 0) തിരികെ 0;
    തിരികെ static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

ശൂന്യം V3Os::u_sleep(int64_t usec) {
#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#അല്ലെങ്കിൽ
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

പൂർണ്ണസംഖ്യ V3Os::system(സ്ഥിരം string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    തിരികെ -1;
#അല്ലെങ്കിൽ
    UINFO(1, "Running system: " << command);
    സ്ഥിരം പൂർണ്ണസംഖ്യ ret = ::system(command.c_str());
    എങ്കിൽ (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        തിരികെ -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    സ്ഥിരം പൂർണ്ണസംഖ്യ exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    തിരികെ exit_code;
#endif
}

ശൂന്യം V3Os::selfTest() {
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
