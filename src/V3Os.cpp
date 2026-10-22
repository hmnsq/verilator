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
#जइ defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#अन्तर्भाव "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#अन्तर्भाव <cerrno>
#अन्तर्भाव <climits>  // PATH_MAX (especially on FreeBSD)
#अन्तर्भाव <cstdarg>
#ifdef _MSC_VER
#अन्तर्भाव <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#अण्णहा
#अन्तर्भाव <dirent.h>
#अन्तर्भाव <utime.h>
#endif
#अन्तर्भाव <fcntl.h>
#अन्तर्भाव <fstream>
#अन्तर्भाव <memory>

#अन्तर्भाव <sys/stat.h>
#अन्तर्भाव <sys/types.h>

#ifdef HAVE_TCMALLOC
#अन्तर्भाव <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#अन्तर्भाव <jemalloc/jemalloc.h>
#endif

// clang-format off
#जइ defined(_WIN32) || defined(__MINGW32__)
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
#  अण्णहा
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#अण्णहा
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#अन्तर्भाव "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ध्रुव string& envvar, ध्रुव string& default_मानम्) {
    प्रत्यागम VString::escapeStringForPath(VlOs::getenvStr(envvar, default_मानम्));
}

शून्य V3Os::setenvStr(ध्रुव string& envvar, ध्रुव string& मानम्, ध्रुव string& why) {
    जइ (why != "") {
        UINFO(1, "export " << envvar << "='" << मानम् << "'  # " << why);
    } अण्णहा {
        UINFO(1, "export " << envvar << "='" << मानम् << "'");
    }
#जइ defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), मानम्.c_str());
#नोचेद्यदि defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), मानम्.c_str(), सच्च);
#अण्णहा
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ध्रुव string vareq = envvar + "=" + मानम्;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#जइ defined(_WIN32) || defined(__MINGW32__)
स्थिर constexpr वर्ण V3OS_SLASH = '\\';
#अण्णहा
स्थिर constexpr वर्ण V3OS_SLASH = '/';
#endif

स्थिर तार्किक isSlash(वर्ण ch) VL_PURE {
#जइ defined(_WIN32) || defined(__MINGW32__)
    प्रत्यागम ch == '/' || ch == '\\';
#अण्णहा
    प्रत्यागम ch == '/';
#endif
}

string V3Os::filenameCleanup(ध्रुव string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    तार्किक lastIsSlash = मिच्छ;
    प्रति (ध्रुव वर्ण ch : filename) {
        ध्रुव तार्किक lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        जइ (lastIsSlash && lastIsSlashOld) अनुवर्त;
        str += ch;
    }
    जइ (str.size() > 1 && isSlash(str.back())) str.निष्कासय_back();
    जाव (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    प्रत्यागम str;
}

string V3Os::filenameJoin(std::initializer_list<ध्रुव std::string> paths) VL_PURE {
    string fullpath;
    प्रति (ध्रुव auto& item : paths) {
        जइ (item.empty() || item == ".") अनुवर्त;
        जइ (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    प्रत्यागम fullpath;
}

string V3Os::filenameDir(ध्रुव string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    प्रति (; it != filename.rend(); ++it) {
        जइ (isSlash(*it)) भङ्ग;
    }
    जइ (it.base() == filename.begin()) प्रत्यागम ".";
    प्रत्यागम string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(ध्रुव string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ध्रुव string::size_type pos = base.find('.');
    जइ (pos != string::npos) {
        base.erase(0, pos);
        प्रत्यागम base;
    }
    प्रत्यागम "";
}

string V3Os::filenameNonDir(ध्रुव string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    प्रति (; it != filename.rend(); ++it) {
        जइ (isSlash(*it)) भङ्ग;
    }
    प्रत्यागम string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(ध्रुव string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    ध्रुव string::size_type pos = base.find('.');
    जइ (pos != string::npos) base.erase(pos);
    प्रत्यागम base;
}

string V3Os::filenameSubstitute(ध्रुव string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    गणना : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    प्रति (string::size_type pos = 0; pos < filename.length(); ++pos) {
        जइ ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            विकल्प (filename[pos + 1]) {
            पक्ष '{': brackets = CURLY; भङ्ग;
            पक्ष '(': brackets = PAREN; भङ्ग;
            स्वभाव: brackets = NONE; भङ्ग;
            }
            जइ (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            जाव (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            जइ (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ध्रुव string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            जइ (!envvar.empty()) envvalue = getenvStr(envvar, "");
            जइ (!envvalue.empty()) {
                result += envvalue;
                जइ (brackets == NONE) {
                    pos = endpos;
                } अण्णहा {
                    pos = endpos + 1;
                }
            } अण्णहा {
                result += filename[pos];  // *pos == '$'
            }
        } अण्णहा {
            result += filename[pos];
        }
    }
    प्रत्यागम result;
}

string V3Os::filenameRealPath(ध्रुव string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    वर्ण retpath[PATH_MAX];
    जइ (
#जइ defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#अण्णहा
        realpath(filename.c_str(), retpath)
#endif
    ) {
        प्रत्यागम std::string{retpath};
    }
    प्रत्यागम filename;
}

string V3Os::filenameAbsolutePath(ध्रुव string& filename) VL_PURE {
#जइ defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    प्रत्यागम filenameRealPath(filename);
#अण्णहा
    जइ (!filenameIsRel(filename)) प्रत्यागम filename;
    प्रत्यागम filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(ध्रुव string& filename, ध्रुव string& base) VL_PURE {
    ध्रुव string a = V3Os::filenameRealPath(filename);
    ध्रुव string b = V3Os::filenameRealPath(base);
    string result;
    जइ (a == b) प्रत्यागम ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    जाव (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        प्रति (; aWordIt != a.end(); ++aWordIt) {
            जइ (isSlash(*aWordIt)) भङ्ग;
        }

        auto bWordIt = bIt;  // position of next slash
        प्रति (; bWordIt != b.end(); ++bWordIt) {
            जइ (isSlash(*bWordIt)) भङ्ग;
        }

        ध्रुव string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        ध्रुव string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        जइ (aWord != bWord) भङ्ग;
        aIt = aWordIt;
        bIt = bWordIt;
        जइ (aIt != a.end()) ++aIt;  // Skip slash
        जइ (bIt != b.end()) ++bIt;  // Skip slash
    }

    जाव (bIt != b.end()) {
        प्रति (; bIt != b.end(); ++bIt) {
            जइ (isSlash(*bIt)) {
                ++bIt;
                भङ्ग;
            }
        }
        जइ (!result.empty()) result += "/";
        result += "..";
    }

    ध्रुव string a_वामः = a.substr(aIt - a.begin());
    जइ (!a_वामः.empty()) {
        जइ (!result.empty()) result += "/";
        result += a_वामः;
    }
    प्रत्यागम filenameCleanup(result);
}

तार्किक V3Os::filenameIsRel(ध्रुव string& filename) VL_PURE {
#जइ defined(_MSC_VER)
    प्रत्यागम std::filesystem::path(filename).is_relative();
#अण्णहा
    प्रत्यागम (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(ध्रुव string& filename) VL_PURE {
#जइ defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    प्रत्यागम slashedPath;
#अण्णहा
    प्रत्यागम filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, वर्ण delim) {
    string पङ्क्तिः;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    वर्ण buf[65536];
    is.getline(buf, 65535, delim);
    पङ्क्तिः = buf;
#अण्णहा
    std::getline(is, पङ्क्तिः, delim);
#endif
    प्रत्यागम पङ्क्तिः;
}

//######################################################################
// Directory utilities

शून्य V3Os::createDir(ध्रुव string& dirname) {
#जइ defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#अण्णहा
    mkdir(dirname.c_str(), 0777);
#endif
}

शून्य V3Os::filesystemFlush(ध्रुव string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#जइ defined(_MSC_VER) || defined(__MINGW32__)
    जइ (पूर्णाङ्क fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        जइ (fd > 0) ::close(fd);
    }
#अण्णहा
    // Faster to just try both rather than check if a file is a dir.
    जइ (DIR* ध्रुव dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } अण्णहा जइ (ध्रुव पूर्णाङ्क fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        जइ (fd > 0) ::close(fd);
    }
#endif
}

शून्य V3Os::filesystemFlushBuildDir(ध्रुव string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#जइ !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ध्रुव पूर्णाङ्क err = utimes(dirname.c_str(), nullptr);
    // Not an error
    जइ (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

शून्य V3Os::unlinkRegexp(ध्रुव string& dir, ध्रुव string& regexp) {
#ifdef _MSC_VER
    यत्न {
        प्रति (ध्रुव auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            जइ (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                ध्रुव string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } गृहाण (std::filesystem::filesystem_error ध्रुव& ex) {}
#अण्णहा
    जइ (DIR* ध्रुव dirp = opendir(dir.c_str())) {
        जाव (संरचना dirent* ध्रुव direntp = readdir(dirp)) {
            जइ (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ध्रुव string fullname = dir + "/" + std::string{direntp->d_name};
#जइ defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#अण्णहा
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

शून्य V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = परिमाण(narenas);
    जइ (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        प्रत्यागम;  // Failed to get number of arenas, give up
    }
    वर्ण buf[64];
    // Index equal to narenas represents all arenas
    (शून्य)VL_SNPRINTF(buf, परिमाण(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ध्रुव uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    प्रत्यागम result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    वर्ण* ध्रुव data = const_cast<वर्ण*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#जइ defined(_WIN32) || defined(__MINGW32__)
    ध्रुव NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    जइ (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#नोचेद्यदि defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    जइ (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#अण्णहा
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::द्विमान};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    जइ (VL_UNCOVERABLE(!is.पठ(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    प्रत्यागम result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#जइ defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    स्थिर constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    ध्रुव uint64_t us
        = ((static_cast<uint64_t>(ft.dw_उच्चः_Date_Time) << 32) + ft.dw_नीचः_Date_Time + 5ULL) / 10ULL;
    प्रत्यागम us - EPOCH_DIFFERENCE_USECS;
#अण्णहा
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    जइ (gettimeofday(&tv, nullptr) < 0) प्रत्यागम 0;
    प्रत्यागम static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

शून्य V3Os::u_sleep(int64_t usec) {
#जइ defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#अण्णहा
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

पूर्णाङ्क V3Os::system(ध्रुव string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    प्रत्यागम -1;
#अण्णहा
    UINFO(1, "Running system: " << command);
    ध्रुव पूर्णाङ्क ret = ::system(command.c_str());
    जइ (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        प्रत्यागम -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ध्रुव पूर्णाङ्क exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    प्रत्यागम exit_code;
#endif
}

शून्य V3Os::selfTest() {
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
