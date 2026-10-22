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
#پایان_اگر
#اگر_تعریف_نشده __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#پایان_اگر
// clang-format on

#ضمن "config_build.h"
#ضمن "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ضمن "V3Os.h"
#ضمن "V3String.h"

#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
#ضمن "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#پایان_اگر

#ضمن <cerrno>
#ضمن <climits>  // PATH_MAX (especially on FreeBSD)
#ضمن <cstdarg>
#اگر_تعریف_شده _MSC_VER
#ضمن <filesystem>  // C++17
#تعریف_کن مسیر_MAX MAX_مسیر
#وإلا
#ضمن <dirent.h>
#ضمن <utime.h>
#پایان_اگر
#ضمن <fcntl.h>
#ضمن <fstream>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

#اگر_تعریف_شده HAVE_TCMALLOC
#ضمن <gperftools/malloc_extension.h>
#پایان_اگر
#اگر_تعریف_شده HAVE_JEMALLOC
#ضمن <jemalloc/jemalloc.h>
#پایان_اگر

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
#پایان_اگر
// clang-format on

#تعریف_کن VL_ALLOW_VERILATEDOS_C
#ضمن "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ثابت string& envvar, ثابت string& default_مقدار) {
    أرجع VString::escape_String_For_مسیر(VlOs::getenvStr(envvar, default_مقدار));
}

فراغ V3Os::setenvStr(ثابت string& envvar, ثابت string& مقدار, ثابت string& why) {
    إذا (why != "") {
        UINFO(1, "export " << envvar << "='" << مقدار << "'  # " << why);
    } وإلا {
        UINFO(1, "export " << envvar << "='" << مقدار << "'");
    }
#إذا defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), مقدار.c_str());
#وإلاإذا defined(BSD_منبع) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), مقدار.c_str(), درست);
#وإلا
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ثابت string vareq = envvar + "=" + مقدار;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#پایان_اگر
}

//######################################################################
// Generic filename utilities

#إذا defined(_WIN32) || defined(__MINGW32__)
ساكن constexpr حرف V3OS_SLASH = '\\';
#وإلا
ساكن constexpr حرف V3OS_SLASH = '/';
#پایان_اگر

ساكن منطقي isSlash(حرف ch) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    أرجع ch == '/' || ch == '\\';
#وإلا
    أرجع ch == '/';
#پایان_اگر
}

string V3Os::filenameCleanup(ثابت string& filename) VL_خالص {
    string str;
    str.reserve(filename.درازی());
    منطقي آخرین_Is_Slash = خطأ;
    لكل (ثابت حرف ch : filename) {
        ثابت منطقي آخرین_Is_Slash_Old = آخرین_Is_Slash;
        آخرین_Is_Slash = isSlash(ch);
        إذا (آخرین_Is_Slash && آخرین_Is_Slash_Old) استمر;
        str += ch;
    }
    إذا (str.اندازه() > 1 && isSlash(str.پشت())) str.برداشت_پشت();
    طالما (str.اندازه() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    أرجع str;
}

string V3Os::filenameJoin(std::initializer_list<ثابت std::string> paths) VL_خالص {
    string fullpath;
    لكل (ثابت خودکار& قلم_2 : paths) {
        إذا (قلم_2.خالی() || قلم_2 == ".") استمر;
        إذا (!fullpath.خالی()) fullpath += V3OS_SLASH;
        fullpath += قلم_2;
    }
    أرجع fullpath;
}

string V3Os::filenameDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::parent_path
    خودکار it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    إذا (it.پایه() == filename.begin()) أرجع ".";
    أرجع string{filename.begin(), (++it).پایه()};
}

string V3Os::filenameExt(ثابت string& filename) VL_خالص {
    string پایه = filenameNonDir(filename);
    ثابت string::اندازه_type pos = پایه.پیدا_کن('.');
    إذا (pos != string::npos) {
        پایه.erase(0, pos);
        أرجع پایه;
    }
    أرجع "";
}

string V3Os::filenameNonDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::filename
    خودکار it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    أرجع string{it.پایه(), filename.انتها()};
}

string V3Os::filenameNonDirExt(ثابت string& filename) VL_خالص {
    string پایه = filenameNonDir(filename);
    ثابت string::اندازه_type pos = پایه.پیدا_کن('.');
    إذا (pos != string::npos) پایه.erase(pos);
    أرجع پایه;
}

string V3Os::filenameSubstitute(ثابت string& filename) {
    string نتیجه;
    // cppcheck-has-bug-suppress unusedLabel
    تعداد : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    لكل (string::اندازه_type pos = 0; pos < filename.درازی(); ++pos) {
        إذا ((filename[pos] == '$') && (pos + 1 < filename.درازی())) {
            بدل (filename[pos + 1]) {
            حالة '{': brackets = CURLY; اكسر;
            حالة '(': brackets = PAREN; اكسر;
            افتراضي: brackets = NONE; اكسر;
            }
            إذا (brackets != NONE) pos = pos + 1;
            string::اندازه_type endpos = pos + 1;
            طالما (((endpos + 1) < filename.درازی())
                   && (((brackets == NONE)
                        && (std::آیا_حرف_یا_رقم(filename[endpos + 1]) || filename[endpos + 1] == '_'))
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
            إذا (!envvar.خالی()) envvalue = getenvStr(envvar, "");
            إذا (!envvalue.خالی()) {
                نتیجه += envvalue;
                إذا (brackets == NONE) {
                    pos = endpos;
                } وإلا {
                    pos = endpos + 1;
                }
            } وإلا {
                نتیجه += filename[pos];  // *pos == '$'
            }
        } وإلا {
            نتیجه += filename[pos];
        }
    }
    أرجع نتیجه;
}

string V3Os::filename_Real_مسیر(ثابت string& filename) VL_خالص {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    حرف retpath[مسیر_MAX];
    إذا (
#إذا defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), مسیر_MAX)
#وإلا
        realpath(filename.c_str(), retpath)
#پایان_اگر
    ) {
        أرجع std::string{retpath};
    }
    أرجع filename;
}

string V3Os::filename_Absolute_مسیر(ثابت string& filename) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    أرجع filename_Real_مسیر(filename);
#وإلا
    إذا (!filenameIsRel(filename)) أرجع filename;
    أرجع filenameJoin(filename_Real_مسیر("."), filename);
#پایان_اگر
}

string V3Os::filename_Relative_مسیر(ثابت string& filename, ثابت string& پایه) VL_خالص {
    ثابت string a = V3Os::filename_Real_مسیر(filename);
    ثابت string b = V3Os::filename_Real_مسیر(پایه);
    string نتیجه;
    إذا (a == b) أرجع ".";

    خودکار aIt = a.begin();
    خودکار bIt = b.begin();
    طالما (aIt != a.انتها() && bIt != b.انتها()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        خودکار a_کلمه_It = aIt;  // position of next slash
        لكل (; a_کلمه_It != a.انتها(); ++a_کلمه_It) {
            إذا (isSlash(*a_کلمه_It)) اكسر;
        }

        خودکار b_کلمه_It = bIt;  // position of next slash
        لكل (; b_کلمه_It != b.انتها(); ++b_کلمه_It) {
            إذا (isSlash(*b_کلمه_It)) اكسر;
        }

        ثابت string a_کلمه = a.substr(aIt - a.begin(), a_کلمه_It - aIt);
        ثابت string b_کلمه = b.substr(bIt - b.begin(), b_کلمه_It - bIt);
        إذا (a_کلمه != b_کلمه) اكسر;
        aIt = a_کلمه_It;
        bIt = b_کلمه_It;
        إذا (aIt != a.انتها()) ++aIt;  // Skip slash
        إذا (bIt != b.انتها()) ++bIt;  // Skip slash
    }

    طالما (bIt != b.انتها()) {
        لكل (; bIt != b.انتها(); ++bIt) {
            إذا (isSlash(*bIt)) {
                ++bIt;
                اكسر;
            }
        }
        إذا (!نتیجه.خالی()) نتیجه += "/";
        نتیجه += "..";
    }

    ثابت string a_چپ = a.substr(aIt - a.begin());
    إذا (!a_چپ.خالی()) {
        إذا (!نتیجه.خالی()) نتیجه += "/";
        نتیجه += a_چپ;
    }
    أرجع filenameCleanup(نتیجه);
}

منطقي V3Os::filenameIsRel(ثابت string& filename) VL_خالص {
#إذا defined(_MSC_VER)
    أرجع std::filesystem::مسیر(filename).is_relative();
#وإلا
    أرجع (filename.درازی() > 0 && filename[0] != '/');
#پایان_اگر
}

string V3Os::filename_Slash_مسیر(ثابت string& filename) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    string slashed_مسیر = filename;
    std::replace(slashed_مسیر.begin(), slashed_مسیر.انتها(), '\\', '/');
    أرجع slashed_مسیر;
#وإلا
    أرجع filename;
#پایان_اگر
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, حرف delim) {
    string سطر;
#اگر_تعریف_شده __CYGWIN__  // Work around buggy implementation of getline
    حرف buf[65536];
    is.getline(buf, 65535, delim);
    سطر = buf;
#وإلا
    std::getline(is, سطر, delim);
#پایان_اگر
    أرجع سطر;
}

//######################################################################
// Directory utilities

فراغ V3Os::createDir(ثابت string& dirname) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#وإلا
    mkdir(dirname.c_str(), 0777);
#پایان_اگر
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
#پایان_اگر
}

فراغ V3Os::filesystemFlushBuildDir(ثابت string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#إذا !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ثابت صحيح err = utimes(dirname.c_str(), اشاره_تهی);
    // Not an error
    إذا (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#پایان_اگر
    filesystemFlush(dirname);
}

فراغ V3Os::unlinkRegexp(ثابت string& dir, ثابت string& regexp) {
#اگر_تعریف_شده _MSC_VER
    حاول {
        لكل (ثابت خودکار& dir_مدخل : std::filesystem::directory_iterator(dir.c_str())) {
            إذا (VString::wildmatch(dir_مدخل.مسیر().filename().string(), regexp.c_str())) {
                ثابت string fullname = dir + "/" + dir_مدخل.مسیر().filename().string();
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
#پایان_اگر
            }
        }
        closedir(dirp);
    }
#پایان_اگر
}

//######################################################################
// METHODS (memory)

فراغ V3Os::release_حافظه() {
#اگر_تعریف_شده HAVE_TCMALLOC
    MallocExtension::instance()->Release_Free_حافظه();
#پایان_اگر
#اگر_تعریف_شده HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    بیعلامت narenas = 0;
    اندازه_t sz = حجم(narenas);
    إذا (mallctl("arenas.narenas", &narenas, &sz, اشاره_تهی, 0)) {
        أرجع;  // Failed to get number of arenas, give up
    }
    حرف buf[64];
    // Index equal to narenas represents all arenas
    (فراغ)VL_SNPRINTF(buf, حجم(buf), "arena.%u.purge", narenas);
    mallctl(buf, اشاره_تهی, اشاره_تهی, اشاره_تهی, 0);
#پایان_اگر
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ثابت uint64_t نتیجه = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    أرجع نتیجه;
}

string V3Os::trueRandom(اندازه_t اندازه) VL_MT_امن {
    string نتیجه(اندازه, '\xFF');
    حرف* ثابت داده = const_cast<حرف*>(نتیجه.داده());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#إذا defined(_WIN32) || defined(__MINGW32__)
    ثابت NTSTATUS hr = BCryptGenRandom(اشاره_تهی, reinterpret_cast<BYTE*>(داده), اندازه,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    إذا (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلاإذا defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    إذا (VL_UNCOVERABLE(getentropy(داده, اندازه))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلا
    std::ifstream is{"/dev/urandom", std::ios::درون | std::ios::دودویی};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    إذا (VL_UNCOVERABLE(!is.اقرأ(داده, اندازه))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#پایان_اگر
    أرجع نتیجه;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::زمان_Usecs() {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ساكن constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    Get_System_زمان_As_File_زمان(&ft);
    ثابت uint64_t us
        = ((static_cast<uint64_t>(ft.dw_بالا_Date_زمان) << 32) + ft.dw_کم_Date_زمان + 5ULL) / 10ULL;
    أرجع us - EPOCH_DIFFERENCE_USECS;
#وإلا
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    إذا (gettimeofday(&tv, اشاره_تهی) < 0) أرجع 0;
    أرجع static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#پایان_اگر
}

فراغ V3Os::u_sleep(int64_t usec) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#وإلا
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#پایان_اگر
}

//######################################################################
// METHODS (sub command)

صحيح V3Os::system(ثابت string& command) {
#اگر_تعریف_شده __wasi__
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
    ثابت صحيح exit_کد = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_کد);
    UASSERT(exit_کد >= 0, "exit code must not be negative");
    أرجع exit_کد;
#پایان_اگر
}

فراغ V3Os::selfTest() {
#اگر_تعریف_شده VL_DEBUG
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
    UASSERT_SELFTEST(filename_Relative_مسیر("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_مسیر("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_مسیر("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_مسیر("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_مسیر("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_مسیر("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_مسیر("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filename_Relative_مسیر("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_مسیر("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_مسیر("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_مسیر("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_مسیر("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_مسیر("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_مسیر("a/b/x/y", "a/q"), "../b/x/y");
#پایان_اگر
}
