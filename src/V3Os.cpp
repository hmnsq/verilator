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
# ifndef PSAPI_نسخہ
#  define PSAPI_نسخہ 1  // Needed for compatibility with Windows 7
# endif
#جے_انت
#جے_بنا_تعریف __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#جے_انت
// clang-format on

#ضمن "config_build.h"
#ضمن "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ضمن "V3Os.h"
#ضمن "V3String.h"

#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
#ضمن "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#جے_انت

#ضمن <cerrno>
#ضمن <climits>  // PATH_MAX (especially on FreeBSD)
#ضمن <cstdarg>
#جے_تعریف_شدہ _MSC_VER
#ضمن <filesystem>  // C++17
#تعریف_کرو رستہ_MAX MAX_رستہ
#وإلا
#ضمن <dirent.h>
#ضمن <utime.h>
#جے_انت
#ضمن <fcntl.h>
#ضمن <fstream>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

#جے_تعریف_شدہ HAVE_TCMALLOC
#ضمن <gperftools/malloc_extension.h>
#جے_انت
#جے_تعریف_شدہ HAVE_JEMALLOC
#ضمن <jemalloc/jemalloc.h>
#جے_انت

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
#جے_انت
// clang-format on

#تعریف_کرو VL_ALLOW_VERILATEDOS_C
#ضمن "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ثابت string& envvar, ثابت string& default_قدر) {
    أرجع VString::escape_String_For_رستہ(VlOs::getenvStr(envvar, default_قدر));
}

فراغ V3Os::setenvStr(ثابت string& envvar, ثابت string& قدر, ثابت string& why) {
    إذا (why != "") {
        UINFO(1, "export " << envvar << "='" << قدر << "'  # " << why);
    } وإلا {
        UINFO(1, "export " << envvar << "='" << قدر << "'");
    }
#إذا defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), قدر.c_str());
#وإلاإذا defined(BSD_سروت) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), قدر.c_str(), سچ);
#وإلا
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ثابت string vareq = envvar + "=" + قدر;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#جے_انت
}

//######################################################################
// Generic filename utilities

#إذا defined(_WIN32) || defined(__MINGW32__)
ساكن constexpr حرف V3OS_SLASH = '\\';
#وإلا
ساكن constexpr حرف V3OS_SLASH = '/';
#جے_انت

ساكن منطقي isSlash(حرف ch) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    أرجع ch == '/' || ch == '\\';
#وإلا
    أرجع ch == '/';
#جے_انت
}

string V3Os::filenameCleanup(ثابت string& filename) VL_خالص {
    string str;
    str.reserve(filename.لمائی());
    منطقي آخری_Is_Slash = خطأ;
    لكل (ثابت حرف ch : filename) {
        ثابت منطقي آخری_Is_Slash_Old = آخری_Is_Slash;
        آخری_Is_Slash = isSlash(ch);
        إذا (آخری_Is_Slash && آخری_Is_Slash_Old) استمر;
        str += ch;
    }
    إذا (str.ناپ() > 1 && isSlash(str.پٹھ())) str.نکالو_پٹھ();
    طالما (str.ناپ() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    أرجع str;
}

string V3Os::filenameJoin(std::initializer_list<ثابت std::string> paths) VL_خالص {
    string fullpath;
    لكل (ثابت خودکار& شے_2 : paths) {
        إذا (شے_2.خالی_تھاواں() || شے_2 == ".") استمر;
        إذا (!fullpath.خالی_تھاواں()) fullpath += V3OS_SLASH;
        fullpath += شے_2;
    }
    أرجع fullpath;
}

string V3Os::filenameDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::parent_path
    خودکار it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    إذا (it.ادھار() == filename.begin()) أرجع ".";
    أرجع string{filename.begin(), (++it).ادھار()};
}

string V3Os::filenameExt(ثابت string& filename) VL_خالص {
    string ادھار = filenameNonDir(filename);
    ثابت string::ناپ_type pos = ادھار.لبھو('.');
    إذا (pos != string::npos) {
        ادھار.erase(0, pos);
        أرجع ادھار;
    }
    أرجع "";
}

string V3Os::filenameNonDir(ثابت string& filename) VL_خالص {
    // std::filesystem::path::filename
    خودکار it = filename.rbegin();
    لكل (; it != filename.rend(); ++it) {
        إذا (isSlash(*it)) اكسر;
    }
    أرجع string{it.ادھار(), filename.سرا()};
}

string V3Os::filenameNonDirExt(ثابت string& filename) VL_خالص {
    string ادھار = filenameNonDir(filename);
    ثابت string::ناپ_type pos = ادھار.لبھو('.');
    إذا (pos != string::npos) ادھار.erase(pos);
    أرجع ادھار;
}

string V3Os::filenameSubstitute(ثابت string& filename) {
    string نتیجہ;
    // cppcheck-has-bug-suppress unusedLabel
    تعداد : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    لكل (string::ناپ_type pos = 0; pos < filename.لمائی(); ++pos) {
        إذا ((filename[pos] == '$') && (pos + 1 < filename.لمائی())) {
            بدل (filename[pos + 1]) {
            حالة '{': brackets = CURLY; اكسر;
            حالة '(': brackets = PAREN; اكسر;
            افتراضي: brackets = NONE; اكسر;
            }
            إذا (brackets != NONE) pos = pos + 1;
            string::ناپ_type endpos = pos + 1;
            طالما (((endpos + 1) < filename.لمائی())
                   && (((brackets == NONE)
                        && (std::حرف_یا_ہندسہ(filename[endpos + 1]) || filename[endpos + 1] == '_'))
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
            إذا (!envvar.خالی_تھاواں()) envvalue = getenvStr(envvar, "");
            إذا (!envvalue.خالی_تھاواں()) {
                نتیجہ += envvalue;
                إذا (brackets == NONE) {
                    pos = endpos;
                } وإلا {
                    pos = endpos + 1;
                }
            } وإلا {
                نتیجہ += filename[pos];  // *pos == '$'
            }
        } وإلا {
            نتیجہ += filename[pos];
        }
    }
    أرجع نتیجہ;
}

string V3Os::filename_Real_رستہ(ثابت string& filename) VL_خالص {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    حرف retpath[رستہ_MAX];
    إذا (
#إذا defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), رستہ_MAX)
#وإلا
        realpath(filename.c_str(), retpath)
#جے_انت
    ) {
        أرجع std::string{retpath};
    }
    أرجع filename;
}

string V3Os::filename_Absolute_رستہ(ثابت string& filename) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    أرجع filename_Real_رستہ(filename);
#وإلا
    إذا (!filenameIsRel(filename)) أرجع filename;
    أرجع filenameJoin(filename_Real_رستہ("."), filename);
#جے_انت
}

string V3Os::filename_Relative_رستہ(ثابت string& filename, ثابت string& ادھار) VL_خالص {
    ثابت string a = V3Os::filename_Real_رستہ(filename);
    ثابت string b = V3Os::filename_Real_رستہ(ادھار);
    string نتیجہ;
    إذا (a == b) أرجع ".";

    خودکار aIt = a.begin();
    خودکار bIt = b.begin();
    طالما (aIt != a.سرا() && bIt != b.سرا()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        خودکار a_لفظ_It = aIt;  // position of next slash
        لكل (; a_لفظ_It != a.سرا(); ++a_لفظ_It) {
            إذا (isSlash(*a_لفظ_It)) اكسر;
        }

        خودکار b_لفظ_It = bIt;  // position of next slash
        لكل (; b_لفظ_It != b.سرا(); ++b_لفظ_It) {
            إذا (isSlash(*b_لفظ_It)) اكسر;
        }

        ثابت string a_لفظ = a.substr(aIt - a.begin(), a_لفظ_It - aIt);
        ثابت string b_لفظ = b.substr(bIt - b.begin(), b_لفظ_It - bIt);
        إذا (a_لفظ != b_لفظ) اكسر;
        aIt = a_لفظ_It;
        bIt = b_لفظ_It;
        إذا (aIt != a.سرا()) ++aIt;  // Skip slash
        إذا (bIt != b.سرا()) ++bIt;  // Skip slash
    }

    طالما (bIt != b.سرا()) {
        لكل (; bIt != b.سرا(); ++bIt) {
            إذا (isSlash(*bIt)) {
                ++bIt;
                اكسر;
            }
        }
        إذا (!نتیجہ.خالی_تھاواں()) نتیجہ += "/";
        نتیجہ += "..";
    }

    ثابت string a_بایاں = a.substr(aIt - a.begin());
    إذا (!a_بایاں.خالی_تھاواں()) {
        إذا (!نتیجہ.خالی_تھاواں()) نتیجہ += "/";
        نتیجہ += a_بایاں;
    }
    أرجع filenameCleanup(نتیجہ);
}

منطقي V3Os::filenameIsRel(ثابت string& filename) VL_خالص {
#إذا defined(_MSC_VER)
    أرجع std::filesystem::رستہ(filename).is_relative();
#وإلا
    أرجع (filename.لمائی() > 0 && filename[0] != '/');
#جے_انت
}

string V3Os::filename_Slash_رستہ(ثابت string& filename) VL_خالص {
#إذا defined(_WIN32) || defined(__MINGW32__)
    string slashed_رستہ = filename;
    std::replace(slashed_رستہ.begin(), slashed_رستہ.سرا(), '\\', '/');
    أرجع slashed_رستہ;
#وإلا
    أرجع filename;
#جے_انت
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, حرف delim) {
    string سطر;
#جے_تعریف_شدہ __CYGWIN__  // Work around buggy implementation of getline
    حرف buf[65536];
    is.getline(buf, 65535, delim);
    سطر = buf;
#وإلا
    std::getline(is, سطر, delim);
#جے_انت
    أرجع سطر;
}

//######################################################################
// Directory utilities

فراغ V3Os::createDir(ثابت string& dirname) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#وإلا
    mkdir(dirname.c_str(), 0777);
#جے_انت
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
#جے_انت
}

فراغ V3Os::filesystemFlushBuildDir(ثابت string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#إذا !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ثابت صحيح err = utimes(dirname.c_str(), خالی_سوچک);
    // Not an error
    إذا (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#جے_انت
    filesystemFlush(dirname);
}

فراغ V3Os::unlinkRegexp(ثابت string& dir, ثابت string& regexp) {
#جے_تعریف_شدہ _MSC_VER
    حاول {
        لكل (ثابت خودکار& dir_اندراج : std::filesystem::directory_iterator(dir.c_str())) {
            إذا (VString::wildmatch(dir_اندراج.رستہ().filename().string(), regexp.c_str())) {
                ثابت string fullname = dir + "/" + dir_اندراج.رستہ().filename().string();
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
#جے_انت
            }
        }
        closedir(dirp);
    }
#جے_انت
}

//######################################################################
// METHODS (memory)

فراغ V3Os::release_حافظہ() {
#جے_تعریف_شدہ HAVE_TCMALLOC
    MallocExtension::instance()->Release_Free_حافظہ();
#جے_انت
#جے_تعریف_شدہ HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    بنانشانی narenas = 0;
    ناپ_t sz = حجم(narenas);
    إذا (mallctl("arenas.narenas", &narenas, &sz, خالی_سوچک, 0)) {
        أرجع;  // Failed to get number of arenas, give up
    }
    حرف buf[64];
    // Index equal to narenas represents all arenas
    (فراغ)VL_SNPRINTF(buf, حجم(buf), "arena.%u.purge", narenas);
    mallctl(buf, خالی_سوچک, خالی_سوچک, خالی_سوچک, 0);
#جے_انت
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ثابت uint64_t نتیجہ = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    أرجع نتیجہ;
}

string V3Os::trueRandom(ناپ_t ناپ) VL_MT_محفوظ {
    string نتیجہ(ناپ, '\xFF');
    حرف* ثابت انکڑے = const_cast<حرف*>(نتیجہ.انکڑے());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#إذا defined(_WIN32) || defined(__MINGW32__)
    ثابت NTSTATUS hr = BCryptGenRandom(خالی_سوچک, reinterpret_cast<BYTE*>(انکڑے), ناپ,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    إذا (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلاإذا defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    إذا (VL_UNCOVERABLE(getentropy(انکڑے, ناپ))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#وإلا
    std::ifstream is{"/dev/urandom", std::ios::وچ | std::ios::ثنائی};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    إذا (VL_UNCOVERABLE(!is.اقرأ(انکڑے, ناپ))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#جے_انت
    أرجع نتیجہ;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::ویلا_Usecs() {
#إذا defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ساكن constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    Get_System_ویلا_As_File_ویلا(&ft);
    ثابت uint64_t us
        = ((static_cast<uint64_t>(ft.dw_بلند_Date_ویلا) << 32) + ft.dw_کم_Date_ویلا + 5ULL) / 10ULL;
    أرجع us - EPOCH_DIFFERENCE_USECS;
#وإلا
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    إذا (gettimeofday(&tv, خالی_سوچک) < 0) أرجع 0;
    أرجع static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#جے_انت
}

فراغ V3Os::u_sleep(int64_t usec) {
#إذا defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#وإلا
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#جے_انت
}

//######################################################################
// METHODS (sub command)

صحيح V3Os::system(ثابت string& command) {
#جے_تعریف_شدہ __wasi__
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
    ثابت صحيح exit_کوڈ = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_کوڈ);
    UASSERT(exit_کوڈ >= 0, "exit code must not be negative");
    أرجع exit_کوڈ;
#جے_انت
}

فراغ V3Os::selfTest() {
#جے_تعریف_شدہ VL_DEBUG
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
    UASSERT_SELFTEST(filename_Relative_رستہ("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_رستہ("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_رستہ("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_رستہ("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_رستہ("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_رستہ("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_رستہ("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filename_Relative_رستہ("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_رستہ("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_رستہ("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_رستہ("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_رستہ("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_رستہ("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_رستہ("a/b/x/y", "a/q"), "../b/x/y");
#جے_انت
}
