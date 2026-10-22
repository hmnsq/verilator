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
#إذا defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_نسخه
#  define PSAPI_نسخه 1  // Needed for compatibility with Windows 7
# endif
#د_که_پای
#که_نه_تعریف_شوی __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#د_که_پای
// clang-format on

#ضمن "config_build.h"
#ضمن "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ضمن "V3Os.h"
#ضمن "V3String.h"

#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
#ضمن "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#د_که_پای

#ضمن <cerrno>
#ضمن <climits>  // PATH_MAX (especially on FreeBSD)
#ضمن <cstdarg>
#که_تعریف_شوی _MSC_VER
#ضمن <filesystem>  // C++17
#تعریف_کړه لار_MAX MAX_لار
#وإلا
#ضمن <dirent.h>
#ضمن <utime.h>
#د_که_پای
#ضمن <fcntl.h>
#ضمن <fstream>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

#که_تعریف_شوی HAVE_TCMALLOC
#ضمن <gperftools/malloc_extension.h>
#د_که_پای
#که_تعریف_شوی HAVE_JEMALLOC
#ضمن <jemalloc/jemalloc.h>
#د_که_پای

// clang-format off
#إذا defined(_WIN32) || defined(__MINGW32__)
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
#  وإلا
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#وإلا
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#د_که_پای
// clang-format on

#تعریف_کړه VL_ALLOW_VERILATEDOS_C
#ضمن "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ثابت string& envvar, ثابت string& default_ارزښت) {
    أرجع VString::escape_String_For_لار(VlOs::getenvStr(envvar, default_ارزښت));
}

فراغ V3Os::setenvStr(ثابت string& envvar, ثابت string& ارزښت, ثابت string& why) {
    إذا (why != "") {
        UINFO(1, "export " << envvar << "='" << ارزښت << "'  # " << why);
    } وإلا {
        UINFO(1, "export " << envvar << "='" << ارزښت << "'");
    }
#إذا defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), ارزښت.c_str());
#وإلاإذا defined(BSD_سرچینه) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), ارزښت.c_str(), ریښتیا);
#وإلا
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ثابت string vareq = envvar + "=" + ارزښت;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#د_که_پای
}

//######################################################################
// Generic filename utilities

#إذا defined(_WIN32) || defined(__MINGW32__)
ساكن constexpr حرف V3OS_SLASH = '\\';
#وإلا
ساكن constexpr حرف V3OS_SLASH = '/';
#د_که_پای

ساكن منطقي isSlash(حرف ch) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    أرجع ch == '/' || ch == '\\';
#وإلا
    أرجع ch == '/';
#د_که_پای
}

string V3Os::filenameCleanup(ثابت string& filename) VL_خالص {
    string str;
    str.reserve(filename.اوږدتیا());
    منطقي وروستی_Is_Slash = خطأ;
    لكل (ثابت حرف ch : filename) {
        ثابت منطقي وروستی_Is_Slash_Old = وروستی_Is_Slash;
        وروستی_Is_Slash = isSlash(ch);
        إذا (وروستی_Is_Slash && وروستی_Is_Slash_Old) استمر;
        str += ch;
    }
    إذا (str.اندازه() > 1 && isSlash(str.شا())) str.راوباسه_شا();
    طالما (str.اندازه() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    أرجع str;
}

string V3Os::filenameJoin(std::initializer_list<ثابت std::string> paths) VL_خالص {
    string fullpath;
    لكل (ثابت خپلکاری& توکی_2 : paths) {
        إذا (توکی_2.تشه() || توکی_2 == ".") استمر;
        إذا (!fullpath.تشه()) fullpath += V3OS_SLASH;
        fullpath += توکی_2;
    }
    أرجع fullpath;
}

string V3Os::filenameDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::parent_path
    خپلکاری it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    إذا (it.بنسټ() == filename.begin()) أرجع ".";
    أرجع string{filename.begin(), (++it).بنسټ()};
}

string V3Os::filenameExt(ثابت string& filename) VL_خالص {
    string بنسټ = filenameNonDir(filename);
    ثابت string::اندازه_type pos = بنسټ.پیدا_کړه('.');
    إذا (pos != string::npos) {
        بنسټ.erase(0, pos);
        أرجع بنسټ;
    }
    أرجع "";
}

string V3Os::filenameNonDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::filename
    خپلکاری it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    أرجع string{it.بنسټ(), filename.پای_ټکی()};
}

string V3Os::filenameNonDirExt(ثابت string& filename) VL_خالص {
    string بنسټ = filenameNonDir(filename);
    ثابت string::اندازه_type pos = بنسټ.پیدا_کړه('.');
    إذا (pos != string::npos) بنسټ.erase(pos);
    أرجع بنسټ;
}

string V3Os::filenameSubstitute(ثابت string& filename) {
    string پایله;
    // cppcheck-has-bug-suppress unusedLabel
    تعداد : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    لكل (string::اندازه_type pos = 0; pos < filename.اوږدتیا(); ++pos) {
        إذا ((filename[pos] == '$') && (pos + 1 < filename.اوږدتیا())) {
            بدل (filename[pos + 1]) {
            حالة '{': brackets = CURLY; اكسر;
            حالة '(': brackets = PAREN; اكسر;
            افتراضي: brackets = NONE; اكسر;
            }
            إذا (brackets != NONE) pos = pos + 1;
            string::اندازه_type endpos = pos + 1;
            طالما (((endpos + 1) < filename.اوږدتیا())
                   && (((brackets == NONE)
                        && (std::که_توری_یا_عدد(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            إذا (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ثابت string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            إذا (!envvar.تشه()) envvalue = getenvStr(envvar, "");
            إذا (!envvalue.تشه()) {
                پایله += envvalue;
                إذا (brackets == NONE) {
                    pos = endpos;
                } وإلا {
                    pos = endpos + 1;
                }
            } وإلا {
                پایله += filename[pos];  // *pos == '$'
            }
        } وإلا {
            پایله += filename[pos];
        }
    }
    أرجع پایله;
}

string V3Os::filename_Real_لار(ثابت string& filename) VL_خالص {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    حرف retpath[لار_MAX];
    إذا (
#إذا defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), لار_MAX)
#وإلا
        realpath(filename.c_str(), retpath)
#د_که_پای
    ) {
        أرجع std::string{retpath};
    }
    أرجع filename;
}

string V3Os::filename_Absolute_لار(ثابت string& filename) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    أرجع filename_Real_لار(filename);
#وإلا
    إذا (!filenameIsRel(filename)) أرجع filename;
    أرجع filenameJoin(filename_Real_لار("."), filename);
#د_که_پای
}

string V3Os::filename_Relative_لار(ثابت string& filename, ثابت string& بنسټ) VL_خالص {
    ثابت string a = V3Os::filename_Real_لار(filename);
    ثابت string b = V3Os::filename_Real_لار(بنسټ);
    string پایله;
    إذا (a == b) أرجع ".";

    خپلکاری aIt = a.begin();
    خپلکاری bIt = b.begin();
    طالما (aIt != a.پای_ټکی() && bIt != b.پای_ټکی()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        خپلکاری a_کلمه_It = aIt;  // position of next slash
        لكل (; a_کلمه_It != a.پای_ټکی(); ++a_کلمه_It) {
            إذا (isSlash(*a_کلمه_It)) اكسر;
        }

        خپلکاری b_کلمه_It = bIt;  // position of next slash
        لكل (; b_کلمه_It != b.پای_ټکی(); ++b_کلمه_It) {
            إذا (isSlash(*b_کلمه_It)) اكسر;
        }

        ثابت string a_کلمه = a.substr(aIt - a.begin(), a_کلمه_It - aIt);
        ثابت string b_کلمه = b.substr(bIt - b.begin(), b_کلمه_It - bIt);
        إذا (a_کلمه != b_کلمه) اكسر;
        aIt = a_کلمه_It;
        bIt = b_کلمه_It;
        إذا (aIt != a.پای_ټکی()) ++aIt;  // Skip slash
        إذا (bIt != b.پای_ټکی()) ++bIt;  // Skip slash
    }

    طالما (bIt != b.پای_ټکی()) {
        لكل (; bIt != b.پای_ټکی(); ++bIt) {
            إذا (isSlash(*bIt)) {
                ++bIt;
                اكسر;
            }
        }
        إذا (!پایله.تشه()) پایله += "/";
        پایله += "..";
    }

    ثابت string a_کیڼ = a.substr(aIt - a.begin());
    إذا (!a_کیڼ.تشه()) {
        إذا (!پایله.تشه()) پایله += "/";
        پایله += a_کیڼ;
    }
    أرجع filenameCleanup(پایله);
}

منطقي V3Os::filenameIsRel(ثابت string& filename) VL_خالص {
#إذا defined(_MSC_VER)
    أرجع std::filesystem::لار(filename).is_relative();
#وإلا
    أرجع (filename.اوږدتیا() > 0 && filename[0] != '/');
#د_که_پای
}

string V3Os::filename_Slash_لار(ثابت string& filename) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    string slashed_لار = filename;
    std::replace(slashed_لار.begin(), slashed_لار.پای_ټکی(), '\\', '/');
    أرجع slashed_لار;
#وإلا
    أرجع filename;
#د_که_پای
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, حرف delim) {
    string کرښه;
#که_تعریف_شوی __CYGWIN__  // Work around buggy implementation of getline
    حرف buf[65536];
    is.getline(buf, 65535, delim);
    کرښه = buf;
#وإلا
    std::getline(is, کرښه, delim);
#د_که_پای
    أرجع کرښه;
}

//######################################################################
// Directory utilities

فراغ V3Os::createDir(ثابت string& dirname) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#وإلا
    mkdir(dirname.c_str(), 0777);
#د_که_پای
}

فراغ V3Os::filesystemFlush(ثابت string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#إذا defined(_MSC_VER) || defined(__MINGW32__)
    إذا (صحيح fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        إذا (fd > 0) ::close(fd);
    }
#وإلا
    // Faster to just try both rather than check if a file is a dir.
    إذا (DIR* ثابت dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } وإلا إذا (ثابت صحيح fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        إذا (fd > 0) ::close(fd);
    }
#د_که_پای
}

فراغ V3Os::filesystemFlushBuildDir(ثابت string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#إذا !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ثابت صحيح err = utimes(dirname.c_str(), تش_اشاره);
    // Not an error
    إذا (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#د_که_پای
    filesystemFlush(dirname);
}

فراغ V3Os::unlinkRegexp(ثابت string& dir, ثابت string& regexp) {
#که_تعریف_شوی _MSC_VER
    حاول {
        لكل (ثابت خپلکاری& dir_ثبت : std::filesystem::directory_iterator(dir.c_str())) {
            إذا (VString::wildmatch(dir_ثبت.لار().filename().string(), regexp.c_str())) {
                ثابت string fullname = dir + "/" + dir_ثبت.لار().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } امسك (std::filesystem::filesystem_error ثابت& ex) {}
#وإلا
    إذا (DIR* ثابت dirp = opendir(dir.c_str())) {
        طالما (بنية dirent* ثابت direntp = readdir(dirp)) {
            إذا (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ثابت string fullname = dir + "/" + std::string{direntp->d_name};
#إذا defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#وإلا
                unlink(fullname.c_str());
#د_که_پای
            }
        }
        closedir(dirp);
    }
#د_که_پای
}

//######################################################################
// METHODS (memory)

فراغ V3Os::release_حافظه() {
#که_تعریف_شوی HAVE_TCMALLOC
    MallocExtension::instance()->Release_Free_حافظه();
#د_که_پای
#که_تعریف_شوی HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    بینښه narenas = 0;
    اندازه_t sz = حجم(narenas);
    إذا (mallctl("arenas.narenas", &narenas, &sz, تش_اشاره, 0)) {
        أرجع;  // Failed to get number of arenas, give up
    }
    حرف buf[64];
    // Index equal to narenas represents all arenas
    (فراغ)VL_SNPRINTF(buf, حجم(buf), "arena.%u.purge", narenas);
    mallctl(buf, تش_اشاره, تش_اشاره, تش_اشاره, 0);
#د_که_پای
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ثابت uint64_t پایله = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    أرجع پایله;
}

string V3Os::trueRandom(اندازه_t اندازه) VL_MT_خوندي {
    string پایله(اندازه, '\xFF');
    حرف* ثابت معلومات = const_cast<حرف*>(پایله.معلومات());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#إذا defined(_WIN32) || defined(__MINGW32__)
    ثابت NTSTATUS hr = BCryptGenRandom(تش_اشاره, reinterpret_cast<BYTE*>(معلومات), اندازه,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    إذا (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلاإذا defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    إذا (VL_UNCOVERABLE(getentropy(معلومات, اندازه))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلا
    std::ifstream is{"/dev/urandom", std::ios::دننه | std::ios::دوګونی};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    إذا (VL_UNCOVERABLE(!is.اقرأ(معلومات, اندازه))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#د_که_پای
    أرجع پایله;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::وخت_Usecs() {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ساكن constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    Get_System_وخت_As_File_وخت(&ft);
    ثابت uint64_t us
        = ((static_cast<uint64_t>(ft.dw_لوړ_Date_وخت) << 32) + ft.dw_ټیټ_Date_وخت + 5ULL) / 10ULL;
    أرجع us - EPOCH_DIFFERENCE_USECS;
#وإلا
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    إذا (gettimeofday(&tv, تش_اشاره) < 0) أرجع 0;
    أرجع static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#د_که_پای
}

فراغ V3Os::u_sleep(int64_t usec) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#وإلا
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#د_که_پای
}

//######################################################################
// METHODS (sub command)

صحيح V3Os::system(ثابت string& command) {
#که_تعریف_شوی __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    أرجع -1;
#وإلا
    UINFO(1, "Running system: " << command);
    ثابت صحيح ret = ::system(command.c_str());
    إذا (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        أرجع -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ثابت صحيح exit_کوډ = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_کوډ);
    UASSERT(exit_کوډ >= 0, "exit code must not be negative");
    أرجع exit_کوډ;
#د_که_پای
}

فراغ V3Os::selfTest() {
#که_تعریف_شوی VL_DEBUG
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
    UASSERT_SELFTEST(filename_Relative_لار("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_لار("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_لار("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_لار("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_لار("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_لار("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_لار("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filename_Relative_لار("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_لار("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_لار("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_لار("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_لار("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_لار("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_لار("a/b/x/y", "a/q"), "../b/x/y");
#د_که_پای
}
