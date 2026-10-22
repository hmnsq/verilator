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
#जर defined(_WIN32) || defined(__MINGW32__)
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
#अन्यथा
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
#जर defined(_WIN32) || defined(__MINGW32__)
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
#  अन्यथा
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#अन्यथा
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

string V3Os::getenvStr(स्थिरांक string& envvar, स्थिरांक string& default_मूल्य) {
    परत VString::escapeStringForPath(VlOs::getenvStr(envvar, default_मूल्य));
}

शून्य V3Os::setenvStr(स्थिरांक string& envvar, स्थिरांक string& मूल्य, स्थिरांक string& why) {
    जर (why != "") {
        UINFO(1, "export " << envvar << "='" << मूल्य << "'  # " << why);
    } अन्यथा {
        UINFO(1, "export " << envvar << "='" << मूल्य << "'");
    }
#जर defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), मूल्य.c_str());
#किंवाजर defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), मूल्य.c_str(), सत्य);
#अन्यथा
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    स्थिरांक string vareq = envvar + "=" + मूल्य;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#जर defined(_WIN32) || defined(__MINGW32__)
स्थिर constexpr अक्षर V3OS_SLASH = '\\';
#अन्यथा
स्थिर constexpr अक्षर V3OS_SLASH = '/';
#endif

स्थिर तार्किक isSlash(अक्षर ch) VL_PURE {
#जर defined(_WIN32) || defined(__MINGW32__)
    परत ch == '/' || ch == '\\';
#अन्यथा
    परत ch == '/';
#endif
}

string V3Os::filenameCleanup(स्थिरांक string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    तार्किक lastIsSlash = असत्य;
    साठी (स्थिरांक अक्षर ch : filename) {
        स्थिरांक तार्किक lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        जर (lastIsSlash && lastIsSlashOld) सुरूठेवा;
        str += ch;
    }
    जर (str.size() > 1 && isSlash(str.back())) str.काढा_back();
    पर्यंत (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    परत str;
}

string V3Os::filenameJoin(std::initializer_list<स्थिरांक std::string> paths) VL_PURE {
    string fullpath;
    साठी (स्थिरांक स्वतः& item : paths) {
        जर (item.empty() || item == ".") सुरूठेवा;
        जर (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    परत fullpath;
}

string V3Os::filenameDir(स्थिरांक string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    स्वतः it = filename.rbegin();
    साठी (; it != filename.rend(); ++it) {
        जर (isSlash(*it)) थांबा;
    }
    जर (it.base() == filename.begin()) परत ".";
    परत string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(स्थिरांक string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    स्थिरांक string::size_type pos = base.find('.');
    जर (pos != string::npos) {
        base.erase(0, pos);
        परत base;
    }
    परत "";
}

string V3Os::filenameNonDir(स्थिरांक string& filename) VL_PURE {
    // std::filesystem::path::filename
    स्वतः it = filename.rbegin();
    साठी (; it != filename.rend(); ++it) {
        जर (isSlash(*it)) थांबा;
    }
    परत string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(स्थिरांक string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    स्थिरांक string::size_type pos = base.find('.');
    जर (pos != string::npos) base.erase(pos);
    परत base;
}

string V3Os::filenameSubstitute(स्थिरांक string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    गणना : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    साठी (string::size_type pos = 0; pos < filename.length(); ++pos) {
        जर ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            निवड (filename[pos + 1]) {
            स्थिती '{': brackets = CURLY; थांबा;
            स्थिती '(': brackets = PAREN; थांबा;
            पूर्वनिर्धारित: brackets = NONE; थांबा;
            }
            जर (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            पर्यंत (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::है_अक्षर_अंक(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            जर (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            स्थिरांक string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            जर (!envvar.empty()) envvalue = getenvStr(envvar, "");
            जर (!envvalue.empty()) {
                result += envvalue;
                जर (brackets == NONE) {
                    pos = endpos;
                } अन्यथा {
                    pos = endpos + 1;
                }
            } अन्यथा {
                result += filename[pos];  // *pos == '$'
            }
        } अन्यथा {
            result += filename[pos];
        }
    }
    परत result;
}

string V3Os::filenameRealPath(स्थिरांक string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    अक्षर retpath[PATH_MAX];
    जर (
#जर defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#अन्यथा
        realpath(filename.c_str(), retpath)
#endif
    ) {
        परत std::string{retpath};
    }
    परत filename;
}

string V3Os::filenameAbsolutePath(स्थिरांक string& filename) VL_PURE {
#जर defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    परत filenameRealPath(filename);
#अन्यथा
    जर (!filenameIsRel(filename)) परत filename;
    परत filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(स्थिरांक string& filename, स्थिरांक string& base) VL_PURE {
    स्थिरांक string a = V3Os::filenameRealPath(filename);
    स्थिरांक string b = V3Os::filenameRealPath(base);
    string result;
    जर (a == b) परत ".";

    स्वतः aIt = a.begin();
    स्वतः bIt = b.begin();
    पर्यंत (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        स्वतः aWordIt = aIt;  // position of next slash
        साठी (; aWordIt != a.end(); ++aWordIt) {
            जर (isSlash(*aWordIt)) थांबा;
        }

        स्वतः bWordIt = bIt;  // position of next slash
        साठी (; bWordIt != b.end(); ++bWordIt) {
            जर (isSlash(*bWordIt)) थांबा;
        }

        स्थिरांक string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        स्थिरांक string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        जर (aWord != bWord) थांबा;
        aIt = aWordIt;
        bIt = bWordIt;
        जर (aIt != a.end()) ++aIt;  // Skip slash
        जर (bIt != b.end()) ++bIt;  // Skip slash
    }

    पर्यंत (bIt != b.end()) {
        साठी (; bIt != b.end(); ++bIt) {
            जर (isSlash(*bIt)) {
                ++bIt;
                थांबा;
            }
        }
        जर (!result.empty()) result += "/";
        result += "..";
    }

    स्थिरांक string a_डावा = a.substr(aIt - a.begin());
    जर (!a_डावा.empty()) {
        जर (!result.empty()) result += "/";
        result += a_डावा;
    }
    परत filenameCleanup(result);
}

तार्किक V3Os::filenameIsRel(स्थिरांक string& filename) VL_PURE {
#जर defined(_MSC_VER)
    परत std::filesystem::path(filename).is_relative();
#अन्यथा
    परत (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(स्थिरांक string& filename) VL_PURE {
#जर defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    परत slashedPath;
#अन्यथा
    परत filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, अक्षर delim) {
    string ओळ;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    अक्षर buf[65536];
    is.getline(buf, 65535, delim);
    ओळ = buf;
#अन्यथा
    std::getline(is, ओळ, delim);
#endif
    परत ओळ;
}

//######################################################################
// Directory utilities

शून्य V3Os::createDir(स्थिरांक string& स_नाम) {
#जर defined(_WIN32) || defined(__MINGW32__)
    _mkdir(स_नाम.c_str());
#अन्यथा
    सूची_गढ़ो(स_नाम.c_str(), 0777);
#endif
}

शून्य V3Os::filesystemFlush(स्थिरांक string& स_नाम) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#जर defined(_MSC_VER) || defined(__MINGW32__)
    जर (पूर्णांक fd = ::खोलो(स_नाम.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        जर (fd > 0) ::बंद(fd);
    }
#अन्यथा
    // Faster to just try both rather than check if a file is a dir.
    जर (सूची* स्थिरांक dirp = सूची_खोलो(स_नाम.c_str())) {  // LCOV_EXCL_BR_LINE
        बंद_सूची(dirp);  // LCOV_EXCL_LINE
    } अन्यथा जर (स्थिरांक पूर्णांक fd = ::खोलो(स_नाम.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        जर (fd > 0) ::बंद(fd);
    }
#endif
}

शून्य V3Os::filesystemFlushBuildDir(स्थिरांक string& स_नाम) {
    // Attempt to force out written directory, for NFS like file systems.
#जर !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    स्थिरांक पूर्णांक err = utimes(स_नाम.c_str(), nullptr);
    // Not an error
    जर (err != 0) UINFO(1, "-Info: File not utimed: " << स_नाम);
#endif
    filesystemFlush(स_नाम);
}

शून्य V3Os::unlinkRegexp(स्थिरांक string& dir, स्थिरांक string& regexp) {
#ifdef _MSC_VER
    प्रयत्न {
        साठी (स्थिरांक स्वतः& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            जर (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                स्थिरांक string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } पकडा (std::filesystem::filesystem_error स्थिरांक& ex) {}
#अन्यथा
    जर (सूची* स्थिरांक dirp = सूची_खोलो(dir.c_str())) {
        पर्यंत (संरचना dirent* स्थिरांक direntp = सूची_पढ़ो(dirp)) {
            जर (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                स्थिरांक string fullname = dir + "/" + std::string{direntp->d_name};
#जर defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#अन्यथा
                unlink(fullname.c_str());
#endif
            }
        }
        बंद_सूची(dirp);
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
    अचिन्हित narenas = 0;
    माप_प्रकार sz = आकार(narenas);
    जर (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        परत;  // Failed to get number of arenas, give up
    }
    अक्षर buf[64];
    // Index equal to narenas represents all arenas
    (शून्य)VL_SNPRINTF(buf, आकार(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    स्थिरांक uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    परत result;
}

string V3Os::trueRandom(माप_प्रकार size) VL_MT_SAFE {
    string result(size, '\xFF');
    अक्षर* स्थिरांक data = const_cast<अक्षर*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#जर defined(_WIN32) || defined(__MINGW32__)
    स्थिरांक NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    जर (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#किंवाजर defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    जर (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#अन्यथा
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::द्विमान};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    जर (VL_UNCOVERABLE(!is.पठ(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    परत result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#जर defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    स्थिर constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    स्थिरांक uint64_t us
        = ((static_cast<uint64_t>(ft.dw_वरचा_Date_Time) << 32) + ft.dw_खालचा_Date_Time + 5ULL) / 10ULL;
    परत us - EPOCH_DIFFERENCE_USECS;
#अन्यथा
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    जर (gettimeofday(&tv, nullptr) < 0) परत 0;
    परत static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

शून्य V3Os::u_sleep(int64_t usec) {
#जर defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#अन्यथा
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

पूर्णांक V3Os::प्रणाली(स्थिरांक string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    परत -1;
#अन्यथा
    UINFO(1, "Running system: " << command);
    स्थिरांक पूर्णांक ret = ::प्रणाली(command.c_str());
    जर (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::म_त्रुटि(त्रुटि_सं));  // LCOV_EXCL_LINE
        परत -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    स्थिरांक पूर्णांक exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    परत exit_code;
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
