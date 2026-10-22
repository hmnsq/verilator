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
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_సంస్కరణ
#  define PSAPI_సంస్కరణ 1  // Needed for compatibility with Windows 7
# endif
#అయితే_ముగింపు
#నిర్వచితం_కాకపోతే __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#అయితే_ముగింపు
// clang-format on

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3String.h"

#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
#అన్తర్భావ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#అయితే_ముగింపు

#అన్తర్భావ <cerrno>
#అన్తర్భావ <climits>  // PATH_MAX (especially on FreeBSD)
#అన్తర్భావ <cstdarg>
#నిర్వచితమైతే _MSC_VER
#అన్తర్భావ <filesystem>  // C++17
#నిర్వచించు మార్గం_MAX MAX_మార్గం
#లేకపోతే
#అన్తర్భావ <dirent.h>
#అన్తర్భావ <utime.h>
#అయితే_ముగింపు
#అన్తర్భావ <fcntl.h>
#అన్తర్భావ <fstream>
#అన్తర్భావ <memory>

#అన్తర్భావ <sys/stat.h>
#అన్తర్భావ <sys/types.h>

#నిర్వచితమైతే HAVE_TCMALLOC
#అన్తర్భావ <gperftools/malloc_extension.h>
#అయితే_ముగింపు
#నిర్వచితమైతే HAVE_JEMALLOC
#అన్తర్భావ <jemalloc/jemalloc.h>
#అయితే_ముగింపు

// clang-format off
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
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
#  లేకపోతే
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#లేకపోతే
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#అయితే_ముగింపు
// clang-format on

#నిర్వచించు VL_ALLOW_VERILATEDOS_C
#అన్తర్భావ "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(స్థిరరాశి string& envvar, స్థిరరాశి string& default_విలువ) {
    తిరిగి VString::escape_String_For_మార్గం(VlOs::getenvStr(envvar, default_విలువ));
}

శూన్య V3Os::setenvStr(స్థిరరాశి string& envvar, స్థిరరాశి string& విలువ, స్థిరరాశి string& why) {
    ఒకవేళ (why != "") {
        UINFO(1, "export " << envvar << "='" << విలువ << "'  # " << why);
    } లేకపోతే {
        UINFO(1, "export " << envvar << "='" << విలువ << "'");
    }
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), విలువ.c_str());
#లేదావేళ defined(BSD_ఆధారం) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), విలువ.c_str(), నిజం);
#లేకపోతే
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    స్థిరరాశి string vareq = envvar + "=" + విలువ;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#అయితే_ముగింపు
}

//######################################################################
// Generic filename utilities

#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
స్థిర constexpr అక్షరం V3OS_SLASH = '\\';
#లేకపోతే
స్థిర constexpr అక్షరం V3OS_SLASH = '/';
#అయితే_ముగింపు

స్థిర తార్కిక isSlash(అక్షరం ch) VL_శుద్ధ {
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
    తిరిగి ch == '/' || ch == '\\';
#లేకపోతే
    తిరిగి ch == '/';
#అయితే_ముగింపు
}

string V3Os::filenameCleanup(స్థిరరాశి string& filename) VL_శుద్ధ {
    string str;
    str.reserve(filename.నిడివి());
    తార్కిక చివరి_Is_Slash = అబద్ధం;
    కొరకు (స్థిరరాశి అక్షరం ch : filename) {
        స్థిరరాశి తార్కిక చివరి_Is_Slash_Old = చివరి_Is_Slash;
        చివరి_Is_Slash = isSlash(ch);
        ఒకవేళ (చివరి_Is_Slash && చివరి_Is_Slash_Old) కొనసాగు;
        str += ch;
    }
    ఒకవేళ (str.కొలత() > 1 && isSlash(str.వెనుక())) str.తీయి_వెనుక();
    అయితే (str.కొలత() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    తిరిగి str;
}

string V3Os::filenameJoin(std::initializer_list<స్థిరరాశి std::string> paths) VL_శుద్ధ {
    string fullpath;
    కొరకు (స్థిరరాశి స్వయంచాలక& అంశం_2 : paths) {
        ఒకవేళ (అంశం_2.ఖాళీలు() || అంశం_2 == ".") కొనసాగు;
        ఒకవేళ (!fullpath.ఖాళీలు()) fullpath += V3OS_SLASH;
        fullpath += అంశం_2;
    }
    తిరిగి fullpath;
}

string V3Os::filenameDir(స్థిరరాశి string& filename) VL_శుద్ధ {
    // std::filesystem::path::parent_path
    స్వయంచాలక it = filename.rbegin();
    కొరకు (; it != filename.rend(); ++it) {
        ఒకవేళ (isSlash(*it)) ఆపు;
    }
    ఒకవేళ (it.పునాది() == filename.begin()) తిరిగి ".";
    తిరిగి string{filename.begin(), (++it).పునాది()};
}

string V3Os::filenameExt(స్థిరరాశి string& filename) VL_శుద్ధ {
    string పునాది = filenameNonDir(filename);
    స్థిరరాశి string::కొలత_type pos = పునాది.కనుగొను('.');
    ఒకవేళ (pos != string::npos) {
        పునాది.erase(0, pos);
        తిరిగి పునాది;
    }
    తిరిగి "";
}

string V3Os::filenameNonDir(స్థిరరాశి string& filename) VL_శుద్ధ {
    // std::filesystem::path::filename
    స్వయంచాలక it = filename.rbegin();
    కొరకు (; it != filename.rend(); ++it) {
        ఒకవేళ (isSlash(*it)) ఆపు;
    }
    తిరిగి string{it.పునాది(), filename.చివర()};
}

string V3Os::filenameNonDirExt(స్థిరరాశి string& filename) VL_శుద్ధ {
    string పునాది = filenameNonDir(filename);
    స్థిరరాశి string::కొలత_type pos = పునాది.కనుగొను('.');
    ఒకవేళ (pos != string::npos) పునాది.erase(pos);
    తిరిగి పునాది;
}

string V3Os::filenameSubstitute(స్థిరరాశి string& filename) {
    string ఫలితం;
    // cppcheck-has-bug-suppress unusedLabel
    గణన : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    కొరకు (string::కొలత_type pos = 0; pos < filename.నిడివి(); ++pos) {
        ఒకవేళ ((filename[pos] == '$') && (pos + 1 < filename.నిడివి())) {
            ఎంపిక (filename[pos + 1]) {
            స్థితి '{': brackets = CURLY; ఆపు;
            స్థితి '(': brackets = PAREN; ఆపు;
            అప్రమేయ: brackets = NONE; ఆపు;
            }
            ఒకవేళ (brackets != NONE) pos = pos + 1;
            string::కొలత_type endpos = pos + 1;
            అయితే (((endpos + 1) < filename.నిడివి())
                   && (((brackets == NONE)
                        && (std::అక్షరమా_అంకెనా(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            ఒకవేళ (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            స్థిరరాశి string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            ఒకవేళ (!envvar.ఖాళీలు()) envvalue = getenvStr(envvar, "");
            ఒకవేళ (!envvalue.ఖాళీలు()) {
                ఫలితం += envvalue;
                ఒకవేళ (brackets == NONE) {
                    pos = endpos;
                } లేకపోతే {
                    pos = endpos + 1;
                }
            } లేకపోతే {
                ఫలితం += filename[pos];  // *pos == '$'
            }
        } లేకపోతే {
            ఫలితం += filename[pos];
        }
    }
    తిరిగి ఫలితం;
}

string V3Os::filename_Real_మార్గం(స్థిరరాశి string& filename) VL_శుద్ధ {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    అక్షరం retpath[మార్గం_MAX];
    ఒకవేళ (
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), మార్గం_MAX)
#లేకపోతే
        realpath(filename.c_str(), retpath)
#అయితే_ముగింపు
    ) {
        తిరిగి std::string{retpath};
    }
    తిరిగి filename;
}

string V3Os::filename_Absolute_మార్గం(స్థిరరాశి string& filename) VL_శుద్ధ {
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    తిరిగి filename_Real_మార్గం(filename);
#లేకపోతే
    ఒకవేళ (!filenameIsRel(filename)) తిరిగి filename;
    తిరిగి filenameJoin(filename_Real_మార్గం("."), filename);
#అయితే_ముగింపు
}

string V3Os::filename_Relative_మార్గం(స్థిరరాశి string& filename, స్థిరరాశి string& పునాది) VL_శుద్ధ {
    స్థిరరాశి string a = V3Os::filename_Real_మార్గం(filename);
    స్థిరరాశి string b = V3Os::filename_Real_మార్గం(పునాది);
    string ఫలితం;
    ఒకవేళ (a == b) తిరిగి ".";

    స్వయంచాలక aIt = a.begin();
    స్వయంచాలక bIt = b.begin();
    అయితే (aIt != a.చివర() && bIt != b.చివర()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        స్వయంచాలక a_మాట_It = aIt;  // position of next slash
        కొరకు (; a_మాట_It != a.చివర(); ++a_మాట_It) {
            ఒకవేళ (isSlash(*a_మాట_It)) ఆపు;
        }

        స్వయంచాలక b_మాట_It = bIt;  // position of next slash
        కొరకు (; b_మాట_It != b.చివర(); ++b_మాట_It) {
            ఒకవేళ (isSlash(*b_మాట_It)) ఆపు;
        }

        స్థిరరాశి string a_మాట = a.substr(aIt - a.begin(), a_మాట_It - aIt);
        స్థిరరాశి string b_మాట = b.substr(bIt - b.begin(), b_మాట_It - bIt);
        ఒకవేళ (a_మాట != b_మాట) ఆపు;
        aIt = a_మాట_It;
        bIt = b_మాట_It;
        ఒకవేళ (aIt != a.చివర()) ++aIt;  // Skip slash
        ఒకవేళ (bIt != b.చివర()) ++bIt;  // Skip slash
    }

    అయితే (bIt != b.చివర()) {
        కొరకు (; bIt != b.చివర(); ++bIt) {
            ఒకవేళ (isSlash(*bIt)) {
                ++bIt;
                ఆపు;
            }
        }
        ఒకవేళ (!ఫలితం.ఖాళీలు()) ఫలితం += "/";
        ఫలితం += "..";
    }

    స్థిరరాశి string a_ఎడమ = a.substr(aIt - a.begin());
    ఒకవేళ (!a_ఎడమ.ఖాళీలు()) {
        ఒకవేళ (!ఫలితం.ఖాళీలు()) ఫలితం += "/";
        ఫలితం += a_ఎడమ;
    }
    తిరిగి filenameCleanup(ఫలితం);
}

తార్కిక V3Os::filenameIsRel(స్థిరరాశి string& filename) VL_శుద్ధ {
#ఒకవేళ defined(_MSC_VER)
    తిరిగి std::filesystem::మార్గం(filename).is_relative();
#లేకపోతే
    తిరిగి (filename.నిడివి() > 0 && filename[0] != '/');
#అయితే_ముగింపు
}

string V3Os::filename_Slash_మార్గం(స్థిరరాశి string& filename) VL_శుద్ధ {
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
    string slashed_మార్గం = filename;
    std::replace(slashed_మార్గం.begin(), slashed_మార్గం.చివర(), '\\', '/');
    తిరిగి slashed_మార్గం;
#లేకపోతే
    తిరిగి filename;
#అయితే_ముగింపు
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, అక్షరం delim) {
    string పంక్తి;
#నిర్వచితమైతే __CYGWIN__  // Work around buggy implementation of getline
    అక్షరం buf[65536];
    is.getline(buf, 65535, delim);
    పంక్తి = buf;
#లేకపోతే
    std::getline(is, పంక్తి, delim);
#అయితే_ముగింపు
    తిరిగి పంక్తి;
}

//######################################################################
// Directory utilities

శూన్య V3Os::createDir(స్థిరరాశి string& dirname) {
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#లేకపోతే
    mkdir(dirname.c_str(), 0777);
#అయితే_ముగింపు
}

శూన్య V3Os::filesystemFlush(స్థిరరాశి string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#ఒకవేళ defined(_MSC_VER) || defined(__MINGW32__)
    ఒకవేళ (పూర్ణాంకం fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ఒకవేళ (fd > 0) ::close(fd);
    }
#లేకపోతే
    // Faster to just try both rather than check if a file is a dir.
    ఒకవేళ (DIR* స్థిరరాశి dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } లేకపోతే ఒకవేళ (స్థిరరాశి పూర్ణాంకం fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ఒకవేళ (fd > 0) ::close(fd);
    }
#అయితే_ముగింపు
}

శూన్య V3Os::filesystemFlushBuildDir(స్థిరరాశి string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#ఒకవేళ !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    స్థిరరాశి పూర్ణాంకం err = utimes(dirname.c_str(), శూన్యసూచిక);
    // Not an error
    ఒకవేళ (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#అయితే_ముగింపు
    filesystemFlush(dirname);
}

శూన్య V3Os::unlinkRegexp(స్థిరరాశి string& dir, స్థిరరాశి string& regexp) {
#నిర్వచితమైతే _MSC_VER
    ప్రయత్నించు {
        కొరకు (స్థిరరాశి స్వయంచాలక& dir_నమోదు : std::filesystem::directory_iterator(dir.c_str())) {
            ఒకవేళ (VString::wildmatch(dir_నమోదు.మార్గం().filename().string(), regexp.c_str())) {
                స్థిరరాశి string fullname = dir + "/" + dir_నమోదు.మార్గం().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } పట్టుకో (std::filesystem::filesystem_error స్థిరరాశి& ex) {}
#లేకపోతే
    ఒకవేళ (DIR* స్థిరరాశి dirp = opendir(dir.c_str())) {
        అయితే (నిర్మాణం dirent* స్థిరరాశి direntp = readdir(dirp)) {
            ఒకవేళ (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                స్థిరరాశి string fullname = dir + "/" + std::string{direntp->d_name};
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#లేకపోతే
                unlink(fullname.c_str());
#అయితే_ముగింపు
            }
        }
        closedir(dirp);
    }
#అయితే_ముగింపు
}

//######################################################################
// METHODS (memory)

శూన్య V3Os::release_జ్ఞాపకం() {
#నిర్వచితమైతే HAVE_TCMALLOC
    MallocExtension::instance()->Release_Free_జ్ఞాపకం();
#అయితే_ముగింపు
#నిర్వచితమైతే HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    గుర్తులేని narenas = 0;
    కొలత_t sz = పరిమాణం(narenas);
    ఒకవేళ (mallctl("arenas.narenas", &narenas, &sz, శూన్యసూచిక, 0)) {
        తిరిగి;  // Failed to get number of arenas, give up
    }
    అక్షరం buf[64];
    // Index equal to narenas represents all arenas
    (శూన్య)VL_SNPRINTF(buf, పరిమాణం(buf), "arena.%u.purge", narenas);
    mallctl(buf, శూన్యసూచిక, శూన్యసూచిక, శూన్యసూచిక, 0);
#అయితే_ముగింపు
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    స్థిరరాశి uint64_t ఫలితం = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    తిరిగి ఫలితం;
}

string V3Os::trueRandom(కొలత_t కొలత) VL_MT_సురక్షిత {
    string ఫలితం(కొలత, '\xFF');
    అక్షరం* స్థిరరాశి దత్తాంశం = const_cast<అక్షరం*>(ఫలితం.దత్తాంశం());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
    స్థిరరాశి NTSTATUS hr = BCryptGenRandom(శూన్యసూచిక, reinterpret_cast<BYTE*>(దత్తాంశం), కొలత,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ఒకవేళ (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#లేదావేళ defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    ఒకవేళ (VL_UNCOVERABLE(getentropy(దత్తాంశం, కొలత))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#లేకపోతే
    std::ifstream is{"/dev/urandom", std::ios::లోపల | std::ios::ద్విమాన};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    ఒకవేళ (VL_UNCOVERABLE(!is.పఠ(దత్తాంశం, కొలత))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#అయితే_ముగింపు
    తిరిగి ఫలితం;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::సమయం_Usecs() {
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    స్థిర constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    Get_System_సమయం_As_File_సమయం(&ft);
    స్థిరరాశి uint64_t us
        = ((static_cast<uint64_t>(ft.dw_ఎగువ_Date_సమయం) << 32) + ft.dw_దిగువ_Date_సమయం + 5ULL) / 10ULL;
    తిరిగి us - EPOCH_DIFFERENCE_USECS;
#లేకపోతే
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    ఒకవేళ (gettimeofday(&tv, శూన్యసూచిక) < 0) తిరిగి 0;
    తిరిగి static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#అయితే_ముగింపు
}

శూన్య V3Os::u_sleep(int64_t usec) {
#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#లేకపోతే
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#అయితే_ముగింపు
}

//######################################################################
// METHODS (sub command)

పూర్ణాంకం V3Os::system(స్థిరరాశి string& command) {
#నిర్వచితమైతే __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    తిరిగి -1;
#లేకపోతే
    UINFO(1, "Running system: " << command);
    స్థిరరాశి పూర్ణాంకం ret = ::system(command.c_str());
    ఒకవేళ (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        తిరిగి -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    స్థిరరాశి పూర్ణాంకం exit_సంకేతం = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_సంకేతం);
    UASSERT(exit_సంకేతం >= 0, "exit code must not be negative");
    తిరిగి exit_సంకేతం;
#అయితే_ముగింపు
}

శూన్య V3Os::selfTest() {
#నిర్వచితమైతే VL_DEBUG
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
    UASSERT_SELFTEST(filename_Relative_మార్గం("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_మార్గం("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_మార్గం("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_మార్గం("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_మార్గం("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_మార్గం("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_మార్గం("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filename_Relative_మార్గం("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_మార్గం("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_మార్గం("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_మార్గం("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_మార్గం("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_మార్గం("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_మార్గం("a/b/x/y", "a/q"), "../b/x/y");
#అయితే_ముగింపు
}
