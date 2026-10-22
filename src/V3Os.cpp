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
# ifndef PSAPI_نسخو
#  define PSAPI_نسخو 1  // Needed for compatibility with Windows 7
# endif
#جيڪڏهن_پورو
#جيڪڏهن_وصف_نه_ٿيل __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#جيڪڏهن_پورو
// clang-format on

#ضمن "config_build.h"
#ضمن "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ضمن "V3Os.h"
#ضمن "V3String.h"

#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
#ضمن "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#جيڪڏهن_پورو

#ضمن <cerrno>
#ضمن <climits>  // PATH_MAX (especially on FreeBSD)
#ضمن <cstdarg>
#جيڪڏهن_وصف_ٿيل _MSC_VER
#ضمن <filesystem>  // C++17
#وصف_ڪريو رستو_MAX MAX_رستو
#وإلا
#ضمن <dirent.h>
#ضمن <utime.h>
#جيڪڏهن_پورو
#ضمن <fcntl.h>
#ضمن <fstream>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

#جيڪڏهن_وصف_ٿيل HAVE_TCMALLOC
#ضمن <gperftools/malloc_extension.h>
#جيڪڏهن_پورو
#جيڪڏهن_وصف_ٿيل HAVE_JEMALLOC
#ضمن <jemalloc/jemalloc.h>
#جيڪڏهن_پورو

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
#جيڪڏهن_پورو
// clang-format on

#وصف_ڪريو VL_ALLOW_VERILATEDOS_C
#ضمن "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ثابت string& envvar, ثابت string& default_قدر) {
    أرجع VString::escape_String_For_رستو(VlOs::getenvStr(envvar, default_قدر));
}

فراغ V3Os::setenvStr(ثابت string& envvar, ثابت string& قدر, ثابت string& why) {
    إذا (why != "") {
        UINFO(1, "export " << envvar << "='" << قدر << "'  # " << why);
    } وإلا {
        UINFO(1, "export " << envvar << "='" << قدر << "'");
    }
#إذا defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), قدر.c_str());
#وإلاإذا defined(BSD_ذريعو) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), قدر.c_str(), سچ);
#وإلا
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ثابت string vareq = envvar + "=" + قدر;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#جيڪڏهن_پورو
}

//######################################################################
// Generic filename utilities

#إذا defined(_WIN32) || defined(__MINGW32__)
ساكن constexpr حرف V3OS_SLASH = '\\';
#وإلا
ساكن constexpr حرف V3OS_SLASH = '/';
#جيڪڏهن_پورو

ساكن منطقي isSlash(حرف ch) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    أرجع ch == '/' || ch == '\\';
#وإلا
    أرجع ch == '/';
#جيڪڏهن_پورو
}

string V3Os::filenameCleanup(ثابت string& filename) VL_خالص {
    string str;
    str.reserve(filename.ڊيگهائي());
    منطقي آخري_Is_Slash = خطأ;
    لكل (ثابت حرف ch : filename) {
        ثابت منطقي آخري_Is_Slash_Old = آخري_Is_Slash;
        آخري_Is_Slash = isSlash(ch);
        إذا (آخري_Is_Slash && آخري_Is_Slash_Old) استمر;
        str += ch;
    }
    إذا (str.سائيز() > 1 && isSlash(str.پٺ())) str.ڪڍو_پٺ();
    طالما (str.سائيز() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    أرجع str;
}

string V3Os::filenameJoin(std::initializer_list<ثابت std::string> paths) VL_خالص {
    string fullpath;
    لكل (ثابت خودڪار& شي_2 : paths) {
        إذا (شي_2.خالي_جايون() || شي_2 == ".") استمر;
        إذا (!fullpath.خالي_جايون()) fullpath += V3OS_SLASH;
        fullpath += شي_2;
    }
    أرجع fullpath;
}

string V3Os::filenameDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::parent_path
    خودڪار it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    إذا (it.بنياد() == filename.begin()) أرجع ".";
    أرجع string{filename.begin(), (++it).بنياد()};
}

string V3Os::filenameExt(ثابت string& filename) VL_خالص {
    string بنياد = filenameNonDir(filename);
    ثابت string::سائيز_type pos = بنياد.ڳوليو('.');
    إذا (pos != string::npos) {
        بنياد.erase(0, pos);
        أرجع بنياد;
    }
    أرجع "";
}

string V3Os::filenameNonDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::filename
    خودڪار it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    أرجع string{it.بنياد(), filename.پڇاڙي()};
}

string V3Os::filenameNonDirExt(ثابت string& filename) VL_خالص {
    string بنياد = filenameNonDir(filename);
    ثابت string::سائيز_type pos = بنياد.ڳوليو('.');
    إذا (pos != string::npos) بنياد.erase(pos);
    أرجع بنياد;
}

string V3Os::filenameSubstitute(ثابت string& filename) {
    string نتيجو;
    // cppcheck-has-bug-suppress unusedLabel
    تعداد : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    لكل (string::سائيز_type pos = 0; pos < filename.ڊيگهائي(); ++pos) {
        إذا ((filename[pos] == '$') && (pos + 1 < filename.ڊيگهائي())) {
            بدل (filename[pos + 1]) {
            حالة '{': brackets = CURLY; اكسر;
            حالة '(': brackets = PAREN; اكسر;
            افتراضي: brackets = NONE; اكسر;
            }
            إذا (brackets != NONE) pos = pos + 1;
            string::سائيز_type endpos = pos + 1;
            طالما (((endpos + 1) < filename.ڊيگهائي())
                   && (((brackets == NONE)
                        && (std::اکر_يا_انگ(filename[endpos + 1]) || filename[endpos + 1] == '_'))
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
            إذا (!envvar.خالي_جايون()) envvalue = getenvStr(envvar, "");
            إذا (!envvalue.خالي_جايون()) {
                نتيجو += envvalue;
                إذا (brackets == NONE) {
                    pos = endpos;
                } وإلا {
                    pos = endpos + 1;
                }
            } وإلا {
                نتيجو += filename[pos];  // *pos == '$'
            }
        } وإلا {
            نتيجو += filename[pos];
        }
    }
    أرجع نتيجو;
}

string V3Os::filename_Real_رستو(ثابت string& filename) VL_خالص {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    حرف retpath[رستو_MAX];
    إذا (
#إذا defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), رستو_MAX)
#وإلا
        realpath(filename.c_str(), retpath)
#جيڪڏهن_پورو
    ) {
        أرجع std::string{retpath};
    }
    أرجع filename;
}

string V3Os::filename_Absolute_رستو(ثابت string& filename) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    أرجع filename_Real_رستو(filename);
#وإلا
    إذا (!filenameIsRel(filename)) أرجع filename;
    أرجع filenameJoin(filename_Real_رستو("."), filename);
#جيڪڏهن_پورو
}

string V3Os::filename_Relative_رستو(ثابت string& filename, ثابت string& بنياد) VL_خالص {
    ثابت string a = V3Os::filename_Real_رستو(filename);
    ثابت string b = V3Os::filename_Real_رستو(بنياد);
    string نتيجو;
    إذا (a == b) أرجع ".";

    خودڪار aIt = a.begin();
    خودڪار bIt = b.begin();
    طالما (aIt != a.پڇاڙي() && bIt != b.پڇاڙي()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        خودڪار a_لفظ_It = aIt;  // position of next slash
        لكل (; a_لفظ_It != a.پڇاڙي(); ++a_لفظ_It) {
            إذا (isSlash(*a_لفظ_It)) اكسر;
        }

        خودڪار b_لفظ_It = bIt;  // position of next slash
        لكل (; b_لفظ_It != b.پڇاڙي(); ++b_لفظ_It) {
            إذا (isSlash(*b_لفظ_It)) اكسر;
        }

        ثابت string a_لفظ = a.substr(aIt - a.begin(), a_لفظ_It - aIt);
        ثابت string b_لفظ = b.substr(bIt - b.begin(), b_لفظ_It - bIt);
        إذا (a_لفظ != b_لفظ) اكسر;
        aIt = a_لفظ_It;
        bIt = b_لفظ_It;
        إذا (aIt != a.پڇاڙي()) ++aIt;  // Skip slash
        إذا (bIt != b.پڇاڙي()) ++bIt;  // Skip slash
    }

    طالما (bIt != b.پڇاڙي()) {
        لكل (; bIt != b.پڇاڙي(); ++bIt) {
            إذا (isSlash(*bIt)) {
                ++bIt;
                اكسر;
            }
        }
        إذا (!نتيجو.خالي_جايون()) نتيجو += "/";
        نتيجو += "..";
    }

    ثابت string a_کاٻو = a.substr(aIt - a.begin());
    إذا (!a_کاٻو.خالي_جايون()) {
        إذا (!نتيجو.خالي_جايون()) نتيجو += "/";
        نتيجو += a_کاٻو;
    }
    أرجع filenameCleanup(نتيجو);
}

منطقي V3Os::filenameIsRel(ثابت string& filename) VL_خالص {
#إذا defined(_MSC_VER)
    أرجع std::filesystem::رستو(filename).is_relative();
#وإلا
    أرجع (filename.ڊيگهائي() > 0 && filename[0] != '/');
#جيڪڏهن_پورو
}

string V3Os::filename_Slash_رستو(ثابت string& filename) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    string slashed_رستو = filename;
    std::replace(slashed_رستو.begin(), slashed_رستو.پڇاڙي(), '\\', '/');
    أرجع slashed_رستو;
#وإلا
    أرجع filename;
#جيڪڏهن_پورو
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, حرف delim) {
    string سٽ;
#جيڪڏهن_وصف_ٿيل __CYGWIN__  // Work around buggy implementation of getline
    حرف buf[65536];
    is.getline(buf, 65535, delim);
    سٽ = buf;
#وإلا
    std::getline(is, سٽ, delim);
#جيڪڏهن_پورو
    أرجع سٽ;
}

//######################################################################
// Directory utilities

فراغ V3Os::createDir(ثابت string& dirname) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#وإلا
    mkdir(dirname.c_str(), 0777);
#جيڪڏهن_پورو
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
#جيڪڏهن_پورو
}

فراغ V3Os::filesystemFlushBuildDir(ثابت string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#إذا !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ثابت صحيح err = utimes(dirname.c_str(), خالي_اشارو);
    // Not an error
    إذا (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#جيڪڏهن_پورو
    filesystemFlush(dirname);
}

فراغ V3Os::unlinkRegexp(ثابت string& dir, ثابت string& regexp) {
#جيڪڏهن_وصف_ٿيل _MSC_VER
    حاول {
        لكل (ثابت خودڪار& dir_داخلا : std::filesystem::directory_iterator(dir.c_str())) {
            إذا (VString::wildmatch(dir_داخلا.رستو().filename().string(), regexp.c_str())) {
                ثابت string fullname = dir + "/" + dir_داخلا.رستو().filename().string();
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
#جيڪڏهن_پورو
            }
        }
        closedir(dirp);
    }
#جيڪڏهن_پورو
}

//######################################################################
// METHODS (memory)

فراغ V3Os::release_يادگيري() {
#جيڪڏهن_وصف_ٿيل HAVE_TCMALLOC
    MallocExtension::instance()->Release_Free_يادگيري();
#جيڪڏهن_پورو
#جيڪڏهن_وصف_ٿيل HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    بنانشان narenas = 0;
    سائيز_t sz = حجم(narenas);
    إذا (mallctl("arenas.narenas", &narenas, &sz, خالي_اشارو, 0)) {
        أرجع;  // Failed to get number of arenas, give up
    }
    حرف buf[64];
    // Index equal to narenas represents all arenas
    (فراغ)VL_SNPRINTF(buf, حجم(buf), "arena.%u.purge", narenas);
    mallctl(buf, خالي_اشارو, خالي_اشارو, خالي_اشارو, 0);
#جيڪڏهن_پورو
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ثابت uint64_t نتيجو = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    أرجع نتيجو;
}

string V3Os::trueRandom(سائيز_t سائيز) VL_MT_محفوظ {
    string نتيجو(سائيز, '\xFF');
    حرف* ثابت معلومات = const_cast<حرف*>(نتيجو.معلومات());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#إذا defined(_WIN32) || defined(__MINGW32__)
    ثابت NTSTATUS hr = BCryptGenRandom(خالي_اشارو, reinterpret_cast<BYTE*>(معلومات), سائيز,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    إذا (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلاإذا defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    إذا (VL_UNCOVERABLE(getentropy(معلومات, سائيز))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلا
    std::ifstream is{"/dev/urandom", std::ios::اندريون | std::ios::ٻٽو};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    إذا (VL_UNCOVERABLE(!is.اقرأ(معلومات, سائيز))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#جيڪڏهن_پورو
    أرجع نتيجو;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::وقت_Usecs() {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ساكن constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    Get_System_وقت_As_File_وقت(&ft);
    ثابت uint64_t us
        = ((static_cast<uint64_t>(ft.dw_مٿاهون_Date_وقت) << 32) + ft.dw_هيٺيون_Date_وقت + 5ULL) / 10ULL;
    أرجع us - EPOCH_DIFFERENCE_USECS;
#وإلا
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    إذا (gettimeofday(&tv, خالي_اشارو) < 0) أرجع 0;
    أرجع static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#جيڪڏهن_پورو
}

فراغ V3Os::u_sleep(int64_t usec) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#وإلا
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#جيڪڏهن_پورو
}

//######################################################################
// METHODS (sub command)

صحيح V3Os::system(ثابت string& command) {
#جيڪڏهن_وصف_ٿيل __wasi__
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
    ثابت صحيح exit_ڪوڊ = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_ڪوڊ);
    UASSERT(exit_ڪوڊ >= 0, "exit code must not be negative");
    أرجع exit_ڪوڊ;
#جيڪڏهن_پورو
}

فراغ V3Os::selfTest() {
#جيڪڏهن_وصف_ٿيل VL_DEBUG
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
    UASSERT_SELFTEST(filename_Relative_رستو("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_رستو("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_رستو("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_رستو("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_رستو("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_رستو("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_رستو("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filename_Relative_رستو("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_رستو("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_رستو("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_رستو("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_رستو("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_رستو("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_رستو("a/b/x/y", "a/q"), "../b/x/y");
#جيڪڏهن_پورو
}
