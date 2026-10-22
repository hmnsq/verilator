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
#எனில் defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#endif
#ifndef __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#endif
// clang-format on

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#அந்தர்பாவ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#அந்தர்பாவ <cerrno>
#அந்தர்பாவ <climits>  // PATH_MAX (especially on FreeBSD)
#அந்தர்பாவ <cstdarg>
#ifdef _MSC_VER
#அந்தர்பாவ <filesystem>  // C++17
#define PATH_MAX MAX_PATH
#இல்லையெனில்
#அந்தர்பாவ <dirent.h>
#அந்தர்பாவ <utime.h>
#endif
#அந்தர்பாவ <fcntl.h>
#அந்தர்பாவ <fstream>
#அந்தர்பாவ <memory>

#அந்தர்பாவ <sys/stat.h>
#அந்தர்பாவ <sys/types.h>

#ifdef HAVE_TCMALLOC
#அந்தர்பாவ <gperftools/malloc_extension.h>
#endif
#ifdef HAVE_JEMALLOC
#அந்தர்பாவ <jemalloc/jemalloc.h>
#endif

// clang-format off
#எனில் defined(_WIN32) || defined(__MINGW32__)
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
#  இல்லையெனில்
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#இல்லையெனில்
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#endif
// clang-format on

#define VL_ALLOW_VERILATEDOS_C
#அந்தர்பாவ "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(மாறிலி string& envvar, மாறிலி string& default_மதிப்பு) {
    திரும்பு VString::escapeStringForPath(VlOs::getenvStr(envvar, default_மதிப்பு));
}

வெறுமை V3Os::setenvStr(மாறிலி string& envvar, மாறிலி string& மதிப்பு, மாறிலி string& why) {
    எனில் (why != "") {
        UINFO(1, "export " << envvar << "='" << மதிப்பு << "'  # " << why);
    } இல்லையெனில் {
        UINFO(1, "export " << envvar << "='" << மதிப்பு << "'");
    }
#எனில் defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), மதிப்பு.c_str());
#அல்லதெனில் defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), மதிப்பு.c_str(), உண்மை);
#இல்லையெனில்
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    மாறிலி string vareq = envvar + "=" + மதிப்பு;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#endif
}

//######################################################################
// Generic filename utilities

#எனில் defined(_WIN32) || defined(__MINGW32__)
நிலையான constexpr எழுத்து V3OS_SLASH = '\\';
#இல்லையெனில்
நிலையான constexpr எழுத்து V3OS_SLASH = '/';
#endif

நிலையான தருக்க isSlash(எழுத்து ch) VL_PURE {
#எனில் defined(_WIN32) || defined(__MINGW32__)
    திரும்பு ch == '/' || ch == '\\';
#இல்லையெனில்
    திரும்பு ch == '/';
#endif
}

string V3Os::filenameCleanup(மாறிலி string& filename) VL_PURE {
    string str;
    str.reserve(filename.length());
    தருக்க lastIsSlash = பொய்;
    க்கு (மாறிலி எழுத்து ch : filename) {
        மாறிலி தருக்க lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        எனில் (lastIsSlash && lastIsSlashOld) தொடர்;
        str += ch;
    }
    எனில் (str.size() > 1 && isSlash(str.back())) str.எடு_back();
    போது (str.size() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    திரும்பு str;
}

string V3Os::filenameJoin(std::initializer_list<மாறிலி std::string> paths) VL_PURE {
    string fullpath;
    க்கு (மாறிலி auto& item : paths) {
        எனில் (item.empty() || item == ".") தொடர்;
        எனில் (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    திரும்பு fullpath;
}

string V3Os::filenameDir(மாறிலி string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    auto it = filename.rbegin();
    க்கு (; it != filename.rend(); ++it) {
        எனில் (isSlash(*it)) நிறுத்து;
    }
    எனில் (it.base() == filename.begin()) திரும்பு ".";
    திரும்பு string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(மாறிலி string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    மாறிலி string::size_type pos = base.find('.');
    எனில் (pos != string::npos) {
        base.erase(0, pos);
        திரும்பு base;
    }
    திரும்பு "";
}

string V3Os::filenameNonDir(மாறிலி string& filename) VL_PURE {
    // std::filesystem::path::filename
    auto it = filename.rbegin();
    க்கு (; it != filename.rend(); ++it) {
        எனில் (isSlash(*it)) நிறுத்து;
    }
    திரும்பு string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(மாறிலி string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    மாறிலி string::size_type pos = base.find('.');
    எனில் (pos != string::npos) base.erase(pos);
    திரும்பு base;
}

string V3Os::filenameSubstitute(மாறிலி string& filename) {
    string result;
    // cppcheck-has-bug-suppress unusedLabel
    எண்ணிக்கை : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    க்கு (string::size_type pos = 0; pos < filename.length(); ++pos) {
        எனில் ((filename[pos] == '$') && (pos + 1 < filename.length())) {
            தேர்வு (filename[pos + 1]) {
            நிலை '{': brackets = CURLY; நிறுத்து;
            நிலை '(': brackets = PAREN; நிறுத்து;
            இயல்பு: brackets = NONE; நிறுத்து;
            }
            எனில் (brackets != NONE) pos = pos + 1;
            string::size_type endpos = pos + 1;
            போது (((endpos + 1) < filename.length())
                   && (((brackets == NONE)
                        && (std::isalnum(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            எனில் (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            மாறிலி string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            எனில் (!envvar.empty()) envvalue = getenvStr(envvar, "");
            எனில் (!envvalue.empty()) {
                result += envvalue;
                எனில் (brackets == NONE) {
                    pos = endpos;
                } இல்லையெனில் {
                    pos = endpos + 1;
                }
            } இல்லையெனில் {
                result += filename[pos];  // *pos == '$'
            }
        } இல்லையெனில் {
            result += filename[pos];
        }
    }
    திரும்பு result;
}

string V3Os::filenameRealPath(மாறிலி string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    எழுத்து retpath[PATH_MAX];
    எனில் (
#எனில் defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#இல்லையெனில்
        realpath(filename.c_str(), retpath)
#endif
    ) {
        திரும்பு std::string{retpath};
    }
    திரும்பு filename;
}

string V3Os::filenameAbsolutePath(மாறிலி string& filename) VL_PURE {
#எனில் defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    திரும்பு filenameRealPath(filename);
#இல்லையெனில்
    எனில் (!filenameIsRel(filename)) திரும்பு filename;
    திரும்பு filenameJoin(filenameRealPath("."), filename);
#endif
}

string V3Os::filenameRelativePath(மாறிலி string& filename, மாறிலி string& base) VL_PURE {
    மாறிலி string a = V3Os::filenameRealPath(filename);
    மாறிலி string b = V3Os::filenameRealPath(base);
    string result;
    எனில் (a == b) திரும்பு ".";

    auto aIt = a.begin();
    auto bIt = b.begin();
    போது (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        auto aWordIt = aIt;  // position of next slash
        க்கு (; aWordIt != a.end(); ++aWordIt) {
            எனில் (isSlash(*aWordIt)) நிறுத்து;
        }

        auto bWordIt = bIt;  // position of next slash
        க்கு (; bWordIt != b.end(); ++bWordIt) {
            எனில் (isSlash(*bWordIt)) நிறுத்து;
        }

        மாறிலி string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        மாறிலி string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        எனில் (aWord != bWord) நிறுத்து;
        aIt = aWordIt;
        bIt = bWordIt;
        எனில் (aIt != a.end()) ++aIt;  // Skip slash
        எனில் (bIt != b.end()) ++bIt;  // Skip slash
    }

    போது (bIt != b.end()) {
        க்கு (; bIt != b.end(); ++bIt) {
            எனில் (isSlash(*bIt)) {
                ++bIt;
                நிறுத்து;
            }
        }
        எனில் (!result.empty()) result += "/";
        result += "..";
    }

    மாறிலி string a_இடது = a.substr(aIt - a.begin());
    எனில் (!a_இடது.empty()) {
        எனில் (!result.empty()) result += "/";
        result += a_இடது;
    }
    திரும்பு filenameCleanup(result);
}

தருக்க V3Os::filenameIsRel(மாறிலி string& filename) VL_PURE {
#எனில் defined(_MSC_VER)
    திரும்பு std::filesystem::path(filename).is_relative();
#இல்லையெனில்
    திரும்பு (filename.length() > 0 && filename[0] != '/');
#endif
}

string V3Os::filenameSlashPath(மாறிலி string& filename) VL_PURE {
#எனில் defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    திரும்பு slashedPath;
#இல்லையெனில்
    திரும்பு filename;
#endif
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, எழுத்து delim) {
    string வரி;
#ifdef __CYGWIN__  // Work around buggy implementation of getline
    எழுத்து buf[65536];
    is.getline(buf, 65535, delim);
    வரி = buf;
#இல்லையெனில்
    std::getline(is, வரி, delim);
#endif
    திரும்பு வரி;
}

//######################################################################
// Directory utilities

வெறுமை V3Os::createDir(மாறிலி string& dirname) {
#எனில் defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#இல்லையெனில்
    mkdir(dirname.c_str(), 0777);
#endif
}

வெறுமை V3Os::filesystemFlush(மாறிலி string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#எனில் defined(_MSC_VER) || defined(__MINGW32__)
    எனில் (முழு fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        எனில் (fd > 0) ::close(fd);
    }
#இல்லையெனில்
    // Faster to just try both rather than check if a file is a dir.
    எனில் (DIR* மாறிலி dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } இல்லையெனில் எனில் (மாறிலி முழு fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        எனில் (fd > 0) ::close(fd);
    }
#endif
}

வெறுமை V3Os::filesystemFlushBuildDir(மாறிலி string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#எனில் !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    மாறிலி முழு err = utimes(dirname.c_str(), nullptr);
    // Not an error
    எனில் (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#endif
    filesystemFlush(dirname);
}

வெறுமை V3Os::unlinkRegexp(மாறிலி string& dir, மாறிலி string& regexp) {
#ifdef _MSC_VER
    முயல் {
        க்கு (மாறிலி auto& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            எனில் (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                மாறிலி string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } பிடி (std::filesystem::filesystem_error மாறிலி& ex) {}
#இல்லையெனில்
    எனில் (DIR* மாறிலி dirp = opendir(dir.c_str())) {
        போது (கட்டமைப்பு dirent* மாறிலி direntp = readdir(dirp)) {
            எனில் (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                மாறிலி string fullname = dir + "/" + std::string{direntp->d_name};
#எனில் defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#இல்லையெனில்
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

வெறுமை V3Os::releaseMemory() {
#ifdef HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
#ifdef HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    unsigned narenas = 0;
    size_t sz = அளவு(narenas);
    எனில் (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        திரும்பு;  // Failed to get number of arenas, give up
    }
    எழுத்து buf[64];
    // Index equal to narenas represents all arenas
    (வெறுமை)VL_SNPRINTF(buf, அளவு(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#endif
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    மாறிலி uint64_t result = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    திரும்பு result;
}

string V3Os::trueRandom(size_t size) VL_MT_SAFE {
    string result(size, '\xFF');
    எழுத்து* மாறிலி data = const_cast<எழுத்து*>(result.data());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#எனில் defined(_WIN32) || defined(__MINGW32__)
    மாறிலி NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(data), size,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    எனில் (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#அல்லதெனில் defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    எனில் (VL_UNCOVERABLE(getentropy(data, size))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#இல்லையெனில்
    std::ifstream is{"/dev/urandom", std::ios::in | std::ios::இருமம்};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    எனில் (VL_UNCOVERABLE(!is.பட(data, size))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#endif
    திரும்பு result;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#எனில் defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    நிலையான constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    மாறிலி uint64_t us
        = ((static_cast<uint64_t>(ft.dw_உயர்வு_Date_Time) << 32) + ft.dw_கீழ்_Date_Time + 5ULL) / 10ULL;
    திரும்பு us - EPOCH_DIFFERENCE_USECS;
#இல்லையெனில்
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    எனில் (gettimeofday(&tv, nullptr) < 0) திரும்பு 0;
    திரும்பு static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}

வெறுமை V3Os::u_sleep(int64_t usec) {
#எனில் defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#இல்லையெனில்
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#endif
}

//######################################################################
// METHODS (sub command)

முழு V3Os::system(மாறிலி string& command) {
#ifdef __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    திரும்பு -1;
#இல்லையெனில்
    UINFO(1, "Running system: " << command);
    மாறிலி முழு ret = ::system(command.c_str());
    எனில் (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        திரும்பு -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    மாறிலி முழு exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    திரும்பு exit_code;
#endif
}

வெறுமை V3Os::selfTest() {
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
