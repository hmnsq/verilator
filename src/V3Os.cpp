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
#اگر defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_نسخہ
#  define PSAPI_نسخہ 1  // Needed for compatibility with Windows 7
# endif
#اگر_ختم
#اگر_غیر_تعریف_شدہ __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#اگر_ختم
// clang-format on

#شامل "config_build.h"
#شامل "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#شامل "V3Os.h"
#شامل "V3String.h"

#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
#شامل "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#اگر_ختم

#شامل <cerrno>
#شامل <climits>  // PATH_MAX (especially on FreeBSD)
#شامل <cstdarg>
#اگر_تعریف_شدہ _MSC_VER
#شامل <filesystem>  // C++17
#تعریف_کرو راستہ_MAX MAX_راستہ
#ورنہ
#شامل <dirent.h>
#شامل <utime.h>
#اگر_ختم
#شامل <fcntl.h>
#شامل <fstream>
#شامل <memory>

#شامل <sys/stat.h>
#شامل <sys/types.h>

#اگر_تعریف_شدہ HAVE_TCMALLOC
#شامل <gperftools/malloc_extension.h>
#اگر_ختم
#اگر_تعریف_شدہ HAVE_JEMALLOC
#شامل <jemalloc/jemalloc.h>
#اگر_ختم

// clang-format off
#اگر defined(_WIN32) || defined(__MINGW32__)
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
#  ورنہ
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#ورنہ
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#اگر_ختم
// clang-format on

#تعریف_کرو VL_ALLOW_VERILATEDOS_C
#شامل "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ثابت string& envvar, ثابت string& default_قدر) {
    واپس VString::escape_String_For_راستہ(VlOs::getenvStr(envvar, default_قدر));
}

خالی V3Os::setenvStr(ثابت string& envvar, ثابت string& قدر, ثابت string& why) {
    اگر (why != "") {
        UINFO(1, "export " << envvar << "='" << قدر << "'  # " << why);
    } ورنہ {
        UINFO(1, "export " << envvar << "='" << قدر << "'");
    }
#اگر defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), قدر.c_str());
#وگرنہ_اگر defined(BSD_ماخذ) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), قدر.c_str(), سچ);
#ورنہ
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ثابت string vareq = envvar + "=" + قدر;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#اگر_ختم
}

//######################################################################
// Generic filename utilities

#اگر defined(_WIN32) || defined(__MINGW32__)
ساکن constexpr حرف V3OS_SLASH = '\\';
#ورنہ
ساکن constexpr حرف V3OS_SLASH = '/';
#اگر_ختم

ساکن منطقی isSlash(حرف ch) VL_خالص {
#اگر defined(_WIN32) || defined(__MINGW32__)
    واپس ch == '/' || ch == '\\';
#ورنہ
    واپس ch == '/';
#اگر_ختم
}

string V3Os::filenameCleanup(ثابت string& filename) VL_خالص {
    string str;
    str.reserve(filename.لمبائی_2());
    منطقی آخری_Is_Slash = جھوٹ;
    برائے (ثابت حرف ch : filename) {
        ثابت منطقی آخری_Is_Slash_Old = آخری_Is_Slash;
        آخری_Is_Slash = isSlash(ch);
        اگر (آخری_Is_Slash && آخری_Is_Slash_Old) جاری;
        str += ch;
    }
    اگر (str.سائز() > 1 && isSlash(str.عقب())) str.نکالو_عقب();
    جب (str.سائز() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    واپس str;
}

string V3Os::filenameJoin(std::initializer_list<ثابت std::string> paths) VL_خالص {
    string fullpath;
    برائے (ثابت خودکار& شے_3 : paths) {
        اگر (شے_3.خالی_خانے() || شے_3 == ".") جاری;
        اگر (!fullpath.خالی_خانے()) fullpath += V3OS_SLASH;
        fullpath += شے_3;
    }
    واپس fullpath;
}

string V3Os::filenameDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::parent_path
    خودکار it = filename.rbegin();
    برائے (; it != filename.rend(); ++it) {
        اگر (isSlash(*it)) توڑو;
    }
    اگر (it.بنیاد() == filename.begin()) واپس ".";
    واپس string{filename.begin(), (++it).بنیاد()};
}

string V3Os::filenameExt(ثابت string& filename) VL_خالص {
    string بنیاد = filenameNonDir(filename);
    ثابت string::سائز_type pos = بنیاد.ڈھونڈو('.');
    اگر (pos != string::npos) {
        بنیاد.erase(0, pos);
        واپس بنیاد;
    }
    واپس "";
}

string V3Os::filenameNonDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::filename
    خودکار it = filename.rbegin();
    برائے (; it != filename.rend(); ++it) {
        اگر (isSlash(*it)) توڑو;
    }
    واپس string{it.بنیاد(), filename.انتہا()};
}

string V3Os::filenameNonDirExt(ثابت string& filename) VL_خالص {
    string بنیاد = filenameNonDir(filename);
    ثابت string::سائز_type pos = بنیاد.ڈھونڈو('.');
    اگر (pos != string::npos) بنیاد.erase(pos);
    واپس بنیاد;
}

string V3Os::filenameSubstitute(ثابت string& filename) {
    string نتیجہ;
    // cppcheck-has-bug-suppress unusedLabel
    شمار : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    برائے (string::سائز_type pos = 0; pos < filename.لمبائی_2(); ++pos) {
        اگر ((filename[pos] == '$') && (pos + 1 < filename.لمبائی_2())) {
            انتخاب (filename[pos + 1]) {
            حالت '{': brackets = CURLY; توڑو;
            حالت '(': brackets = PAREN; توڑو;
            طےشدہ: brackets = NONE; توڑو;
            }
            اگر (brackets != NONE) pos = pos + 1;
            string::سائز_type endpos = pos + 1;
            جب (((endpos + 1) < filename.لمبائی_2())
                   && (((brackets == NONE)
                        && (std::حرف_یا_ہندسہ(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            اگر (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ثابت string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            اگر (!envvar.خالی_خانے()) envvalue = getenvStr(envvar, "");
            اگر (!envvalue.خالی_خانے()) {
                نتیجہ += envvalue;
                اگر (brackets == NONE) {
                    pos = endpos;
                } ورنہ {
                    pos = endpos + 1;
                }
            } ورنہ {
                نتیجہ += filename[pos];  // *pos == '$'
            }
        } ورنہ {
            نتیجہ += filename[pos];
        }
    }
    واپس نتیجہ;
}

string V3Os::filename_Real_راستہ(ثابت string& filename) VL_خالص {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    حرف retpath[راستہ_MAX];
    اگر (
#اگر defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), راستہ_MAX)
#ورنہ
        realpath(filename.c_str(), retpath)
#اگر_ختم
    ) {
        واپس std::string{retpath};
    }
    واپس filename;
}

string V3Os::filename_Absolute_راستہ(ثابت string& filename) VL_خالص {
#اگر defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    واپس filename_Real_راستہ(filename);
#ورنہ
    اگر (!filenameIsRel(filename)) واپس filename;
    واپس filenameJoin(filename_Real_راستہ("."), filename);
#اگر_ختم
}

string V3Os::filename_Relative_راستہ(ثابت string& filename, ثابت string& بنیاد) VL_خالص {
    ثابت string a = V3Os::filename_Real_راستہ(filename);
    ثابت string b = V3Os::filename_Real_راستہ(بنیاد);
    string نتیجہ;
    اگر (a == b) واپس ".";

    خودکار aIt = a.begin();
    خودکار bIt = b.begin();
    جب (aIt != a.انتہا() && bIt != b.انتہا()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        خودکار a_لفظ_It = aIt;  // position of next slash
        برائے (; a_لفظ_It != a.انتہا(); ++a_لفظ_It) {
            اگر (isSlash(*a_لفظ_It)) توڑو;
        }

        خودکار b_لفظ_It = bIt;  // position of next slash
        برائے (; b_لفظ_It != b.انتہا(); ++b_لفظ_It) {
            اگر (isSlash(*b_لفظ_It)) توڑو;
        }

        ثابت string a_لفظ = a.substr(aIt - a.begin(), a_لفظ_It - aIt);
        ثابت string b_لفظ = b.substr(bIt - b.begin(), b_لفظ_It - bIt);
        اگر (a_لفظ != b_لفظ) توڑو;
        aIt = a_لفظ_It;
        bIt = b_لفظ_It;
        اگر (aIt != a.انتہا()) ++aIt;  // Skip slash
        اگر (bIt != b.انتہا()) ++bIt;  // Skip slash
    }

    جب (bIt != b.انتہا()) {
        برائے (; bIt != b.انتہا(); ++bIt) {
            اگر (isSlash(*bIt)) {
                ++bIt;
                توڑو;
            }
        }
        اگر (!نتیجہ.خالی_خانے()) نتیجہ += "/";
        نتیجہ += "..";
    }

    ثابت string a_بایاں = a.substr(aIt - a.begin());
    اگر (!a_بایاں.خالی_خانے()) {
        اگر (!نتیجہ.خالی_خانے()) نتیجہ += "/";
        نتیجہ += a_بایاں;
    }
    واپس filenameCleanup(نتیجہ);
}

منطقی V3Os::filenameIsRel(ثابت string& filename) VL_خالص {
#اگر defined(_MSC_VER)
    واپس std::filesystem::راستہ(filename).is_relative();
#ورنہ
    واپس (filename.لمبائی_2() > 0 && filename[0] != '/');
#اگر_ختم
}

string V3Os::filename_Slash_راستہ(ثابت string& filename) VL_خالص {
#اگر defined(_WIN32) || defined(__MINGW32__)
    string slashed_راستہ = filename;
    std::replace(slashed_راستہ.begin(), slashed_راستہ.انتہا(), '\\', '/');
    واپس slashed_راستہ;
#ورنہ
    واپس filename;
#اگر_ختم
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, حرف delim) {
    string سطر;
#اگر_تعریف_شدہ __CYGWIN__  // Work around buggy implementation of getline
    حرف buf[65536];
    is.getline(buf, 65535, delim);
    سطر = buf;
#ورنہ
    std::getline(is, سطر, delim);
#اگر_ختم
    واپس سطر;
}

//######################################################################
// Directory utilities

خالی V3Os::createDir(ثابت string& dirname) {
#اگر defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#ورنہ
    mkdir(dirname.c_str(), 0777);
#اگر_ختم
}

خالی V3Os::filesystemFlush(ثابت string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#اگر defined(_MSC_VER) || defined(__MINGW32__)
    اگر (صحیح fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        اگر (fd > 0) ::close(fd);
    }
#ورنہ
    // Faster to just try both rather than check if a file is a dir.
    اگر (DIR* ثابت dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } ورنہ اگر (ثابت صحیح fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        اگر (fd > 0) ::close(fd);
    }
#اگر_ختم
}

خالی V3Os::filesystemFlushBuildDir(ثابت string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#اگر !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ثابت صحیح err = utimes(dirname.c_str(), خالی_اشاریہ);
    // Not an error
    اگر (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#اگر_ختم
    filesystemFlush(dirname);
}

خالی V3Os::unlinkRegexp(ثابت string& dir, ثابت string& regexp) {
#اگر_تعریف_شدہ _MSC_VER
    کوشش {
        برائے (ثابت خودکار& dir_اندراج : std::filesystem::directory_iterator(dir.c_str())) {
            اگر (VString::wildmatch(dir_اندراج.راستہ().filename().string(), regexp.c_str())) {
                ثابت string fullname = dir + "/" + dir_اندراج.راستہ().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } پکڑو (std::filesystem::filesystem_error ثابت& ex) {}
#ورنہ
    اگر (DIR* ثابت dirp = opendir(dir.c_str())) {
        جب (ساخت dirent* ثابت direntp = readdir(dirp)) {
            اگر (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ثابت string fullname = dir + "/" + std::string{direntp->d_name};
#اگر defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#ورنہ
                unlink(fullname.c_str());
#اگر_ختم
            }
        }
        closedir(dirp);
    }
#اگر_ختم
}

//######################################################################
// METHODS (memory)

خالی V3Os::release_حافظہ() {
#اگر_تعریف_شدہ HAVE_TCMALLOC
    MallocExtension::instance()->Release_Free_حافظہ();
#اگر_ختم
#اگر_تعریف_شدہ HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    غیرعلامتی narenas = 0;
    سائز_t sz = حجم(narenas);
    اگر (mallctl("arenas.narenas", &narenas, &sz, خالی_اشاریہ, 0)) {
        واپس;  // Failed to get number of arenas, give up
    }
    حرف buf[64];
    // Index equal to narenas represents all arenas
    (خالی)VL_SNPRINTF(buf, حجم(buf), "arena.%u.purge", narenas);
    mallctl(buf, خالی_اشاریہ, خالی_اشاریہ, خالی_اشاریہ, 0);
#اگر_ختم
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ثابت uint64_t نتیجہ = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    واپس نتیجہ;
}

string V3Os::trueRandom(سائز_t سائز) VL_MT_محفوظ {
    string نتیجہ(سائز, '\xFF');
    حرف* ثابت ڈیٹا = const_cast<حرف*>(نتیجہ.ڈیٹا());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#اگر defined(_WIN32) || defined(__MINGW32__)
    ثابت NTSTATUS hr = BCryptGenRandom(خالی_اشاریہ, reinterpret_cast<BYTE*>(ڈیٹا), سائز,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    اگر (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وگرنہ_اگر defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    اگر (VL_UNCOVERABLE(getentropy(ڈیٹا, سائز))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#ورنہ
    std::ifstream is{"/dev/urandom", std::ios::اندر | std::ios::ثنائی};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    اگر (VL_UNCOVERABLE(!is.اقرأ(ڈیٹا, سائز))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#اگر_ختم
    واپس نتیجہ;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::وقت_Usecs() {
#اگر defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ساکن constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    Get_System_وقت_As_File_وقت(&ft);
    ثابت uint64_t us
        = ((static_cast<uint64_t>(ft.dw_بلند_Date_وقت) << 32) + ft.dw_کم_Date_وقت + 5ULL) / 10ULL;
    واپس us - EPOCH_DIFFERENCE_USECS;
#ورنہ
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    اگر (gettimeofday(&tv, خالی_اشاریہ) < 0) واپس 0;
    واپس static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#اگر_ختم
}

خالی V3Os::u_sleep(int64_t usec) {
#اگر defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#ورنہ
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#اگر_ختم
}

//######################################################################
// METHODS (sub command)

صحیح V3Os::system(ثابت string& command) {
#اگر_تعریف_شدہ __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    واپس -1;
#ورنہ
    UINFO(1, "Running system: " << command);
    ثابت صحیح ret = ::system(command.c_str());
    اگر (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        واپس -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ثابت صحیح exit_رمز = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_رمز);
    UASSERT(exit_رمز >= 0, "exit code must not be negative");
    واپس exit_رمز;
#اگر_ختم
}

خالی V3Os::selfTest() {
#اگر_تعریف_شدہ VL_DEBUG
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
    UASSERT_SELFTEST(filename_Relative_راستہ("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_راستہ("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_راستہ("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_راستہ("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_راستہ("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_راستہ("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_راستہ("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filename_Relative_راستہ("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_راستہ("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_راستہ("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_راستہ("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_راستہ("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_راستہ("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_راستہ("a/b/x/y", "a/q"), "../b/x/y");
#اگر_ختم
}
