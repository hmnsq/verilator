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
#यदि defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_संस्करणम्
#  define PSAPI_संस्करणम् 1  // Needed for compatibility with Windows 7
# endif
#यदि_अन्तः
#यदि_अपरिभाषितम् __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#यदि_अन्तः
// clang-format on

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3String.h"

#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
#अन्तर्भाव "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#यदि_अन्तः

#अन्तर्भाव <cerrno>
#अन्तर्भाव <climits>  // PATH_MAX (especially on FreeBSD)
#अन्तर्भाव <cstdarg>
#यदि_परिभाषितम् _MSC_VER
#अन्तर्भाव <filesystem>  // C++17
#परिभाषय मार्गः_MAX MAX_मार्गः
#अन्यथा
#अन्तर्भाव <dirent.h>
#अन्तर्भाव <utime.h>
#यदि_अन्तः
#अन्तर्भाव <fcntl.h>
#अन्तर्भाव <fstream>
#अन्तर्भाव <memory>

#अन्तर्भाव <sys/stat.h>
#अन्तर्भाव <sys/types.h>

#यदि_परिभाषितम् HAVE_TCMALLOC
#अन्तर्भाव <gperftools/malloc_extension.h>
#यदि_अन्तः
#यदि_परिभाषितम् HAVE_JEMALLOC
#अन्तर्भाव <jemalloc/jemalloc.h>
#यदि_अन्तः

// clang-format off
#यदि defined(_WIN32) || defined(__MINGW32__)
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
#यदि_अन्तः
// clang-format on

#परिभाषय VL_ALLOW_VERILATEDOS_C
#अन्तर्भाव "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ध्रुव string& envvar, ध्रुव string& default_मानम्) {
    प्रत्यागम VString::escape_String_For_मार्गः(VlOs::getenvStr(envvar, default_मानम्));
}

शून्य V3Os::setenvStr(ध्रुव string& envvar, ध्रुव string& मानम्, ध्रुव string& why) {
    यदि (why != "") {
        UINFO(1, "export " << envvar << "='" << मानम् << "'  # " << why);
    } अन्यथा {
        UINFO(1, "export " << envvar << "='" << मानम् << "'");
    }
#यदि defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), मानम्.c_str());
#नोचेद्यदि defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), मानम्.c_str(), सत्य);
#अन्यथा
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ध्रुव string vareq = envvar + "=" + मानम्;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#यदि_अन्तः
}

//######################################################################
// Generic filename utilities

#यदि defined(_WIN32) || defined(__MINGW32__)
स्थिर constexpr वर्ण V3OS_SLASH = '\\';
#अन्यथा
स्थिर constexpr वर्ण V3OS_SLASH = '/';
#यदि_अन्तः

स्थिर तार्किक isSlash(वर्ण ch) VL_विशुद्धम् {
#यदि defined(_WIN32) || defined(__MINGW32__)
    प्रत्यागम ch == '/' || ch == '\\';
#अन्यथा
    प्रत्यागम ch == '/';
#यदि_अन्तः
}

string V3Os::filenameCleanup(ध्रुव string& filename) VL_विशुद्धम् {
    string str;
    str.reserve(filename.दीर्घता_2());
    तार्किक चरमः_Is_Slash = असत्य;
    प्रति (ध्रुव वर्ण ch : filename) {
        ध्रुव तार्किक चरमः_Is_Slash_Old = चरमः_Is_Slash;
        चरमः_Is_Slash = isSlash(ch);
        यदि (चरमः_Is_Slash && चरमः_Is_Slash_Old) अनुवर्त;
        str += ch;
    }
    यदि (str.आकार() > 1 && isSlash(str.पृष्ठभागः())) str.निष्कासय_पृष्ठभागः();
    यावत् (str.आकार() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    प्रत्यागम str;
}

string V3Os::filenameJoin(std::initializer_list<ध्रुव std::string> paths) VL_विशुद्धम् {
    string fullpath;
    प्रति (ध्रुव स्वचालित& item : paths) {
        यदि (item.रिक्तानि() || item == ".") अनुवर्त;
        यदि (!fullpath.रिक्तानि()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    प्रत्यागम fullpath;
}

string V3Os::filenameDir(ध्रुव string& filename) VL_विशुद्धम् {
    // std::filesystem::path::parent_path
    स्वचालित it = filename.rbegin();
    प्रति (; it != filename.rend(); ++it) {
        यदि (isSlash(*it)) भङ्ग;
    }
    यदि (it.आधारः() == filename.begin()) प्रत्यागम ".";
    प्रत्यागम string{filename.begin(), (++it).आधारः()};
}

string V3Os::filenameExt(ध्रुव string& filename) VL_विशुद्धम् {
    string आधारः = filenameNonDir(filename);
    ध्रुव string::आकार_type pos = आधारः.find('.');
    यदि (pos != string::npos) {
        आधारः.erase(0, pos);
        प्रत्यागम आधारः;
    }
    प्रत्यागम "";
}

string V3Os::filenameNonDir(ध्रुव string& filename) VL_विशुद्धम् {
    // std::filesystem::path::filename
    स्वचालित it = filename.rbegin();
    प्रति (; it != filename.rend(); ++it) {
        यदि (isSlash(*it)) भङ्ग;
    }
    प्रत्यागम string{it.आधारः(), filename.अन्तः_2()};
}

string V3Os::filenameNonDirExt(ध्रुव string& filename) VL_विशुद्धम् {
    string आधारः = filenameNonDir(filename);
    ध्रुव string::आकार_type pos = आधारः.find('.');
    यदि (pos != string::npos) आधारः.erase(pos);
    प्रत्यागम आधारः;
}

string V3Os::filenameSubstitute(ध्रुव string& filename) {
    string परिणाम;
    // cppcheck-has-bug-suppress unusedLabel
    गणना : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    प्रति (string::आकार_type pos = 0; pos < filename.दीर्घता_2(); ++pos) {
        यदि ((filename[pos] == '$') && (pos + 1 < filename.दीर्घता_2())) {
            विकल्प (filename[pos + 1]) {
            पक्ष '{': brackets = CURLY; भङ्ग;
            पक्ष '(': brackets = PAREN; भङ्ग;
            स्वभाव: brackets = NONE; भङ्ग;
            }
            यदि (brackets != NONE) pos = pos + 1;
            string::आकार_type endpos = pos + 1;
            यावत् (((endpos + 1) < filename.दीर्घता_2())
                   && (((brackets == NONE)
                        && (std::अक्षरम्_अङ्कः_वा(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            यदि (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ध्रुव string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            यदि (!envvar.रिक्तानि()) envvalue = getenvStr(envvar, "");
            यदि (!envvalue.रिक्तानि()) {
                परिणाम += envvalue;
                यदि (brackets == NONE) {
                    pos = endpos;
                } अन्यथा {
                    pos = endpos + 1;
                }
            } अन्यथा {
                परिणाम += filename[pos];  // *pos == '$'
            }
        } अन्यथा {
            परिणाम += filename[pos];
        }
    }
    प्रत्यागम परिणाम;
}

string V3Os::filename_Real_मार्गः(ध्रुव string& filename) VL_विशुद्धम् {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    वर्ण retpath[मार्गः_MAX];
    यदि (
#यदि defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), मार्गः_MAX)
#अन्यथा
        realpath(filename.c_str(), retpath)
#यदि_अन्तः
    ) {
        प्रत्यागम std::string{retpath};
    }
    प्रत्यागम filename;
}

string V3Os::filename_Absolute_मार्गः(ध्रुव string& filename) VL_विशुद्धम् {
#यदि defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    प्रत्यागम filename_Real_मार्गः(filename);
#अन्यथा
    यदि (!filenameIsRel(filename)) प्रत्यागम filename;
    प्रत्यागम filenameJoin(filename_Real_मार्गः("."), filename);
#यदि_अन्तः
}

string V3Os::filename_Relative_मार्गः(ध्रुव string& filename, ध्रुव string& आधारः) VL_विशुद्धम् {
    ध्रुव string a = V3Os::filename_Real_मार्गः(filename);
    ध्रुव string b = V3Os::filename_Real_मार्गः(आधारः);
    string परिणाम;
    यदि (a == b) प्रत्यागम ".";

    स्वचालित aIt = a.begin();
    स्वचालित bIt = b.begin();
    यावत् (aIt != a.अन्तः_2() && bIt != b.अन्तः_2()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        स्वचालित aWordIt = aIt;  // position of next slash
        प्रति (; aWordIt != a.अन्तः_2(); ++aWordIt) {
            यदि (isSlash(*aWordIt)) भङ्ग;
        }

        स्वचालित bWordIt = bIt;  // position of next slash
        प्रति (; bWordIt != b.अन्तः_2(); ++bWordIt) {
            यदि (isSlash(*bWordIt)) भङ्ग;
        }

        ध्रुव string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        ध्रुव string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        यदि (aWord != bWord) भङ्ग;
        aIt = aWordIt;
        bIt = bWordIt;
        यदि (aIt != a.अन्तः_2()) ++aIt;  // Skip slash
        यदि (bIt != b.अन्तः_2()) ++bIt;  // Skip slash
    }

    यावत् (bIt != b.अन्तः_2()) {
        प्रति (; bIt != b.अन्तः_2(); ++bIt) {
            यदि (isSlash(*bIt)) {
                ++bIt;
                भङ्ग;
            }
        }
        यदि (!परिणाम.रिक्तानि()) परिणाम += "/";
        परिणाम += "..";
    }

    ध्रुव string a_वामः = a.substr(aIt - a.begin());
    यदि (!a_वामः.रिक्तानि()) {
        यदि (!परिणाम.रिक्तानि()) परिणाम += "/";
        परिणाम += a_वामः;
    }
    प्रत्यागम filenameCleanup(परिणाम);
}

तार्किक V3Os::filenameIsRel(ध्रुव string& filename) VL_विशुद्धम् {
#यदि defined(_MSC_VER)
    प्रत्यागम std::filesystem::मार्गः(filename).is_relative();
#अन्यथा
    प्रत्यागम (filename.दीर्घता_2() > 0 && filename[0] != '/');
#यदि_अन्तः
}

string V3Os::filename_Slash_मार्गः(ध्रुव string& filename) VL_विशुद्धम् {
#यदि defined(_WIN32) || defined(__MINGW32__)
    string slashed_मार्गः = filename;
    std::replace(slashed_मार्गः.begin(), slashed_मार्गः.अन्तः_2(), '\\', '/');
    प्रत्यागम slashed_मार्गः;
#अन्यथा
    प्रत्यागम filename;
#यदि_अन्तः
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, वर्ण delim) {
    string पङ्क्तिः;
#यदि_परिभाषितम् __CYGWIN__  // Work around buggy implementation of getline
    वर्ण buf[65536];
    is.getline(buf, 65535, delim);
    पङ्क्तिः = buf;
#अन्यथा
    std::getline(is, पङ्क्तिः, delim);
#यदि_अन्तः
    प्रत्यागम पङ्क्तिः;
}

//######################################################################
// Directory utilities

शून्य V3Os::createDir(ध्रुव string& dirname) {
#यदि defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#अन्यथा
    mkdir(dirname.c_str(), 0777);
#यदि_अन्तः
}

शून्य V3Os::filesystemFlush(ध्रुव string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#यदि defined(_MSC_VER) || defined(__MINGW32__)
    यदि (पूर्णाङ्क fd = ::उद्घाटय(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        यदि (fd > 0) ::close(fd);
    }
#अन्यथा
    // Faster to just try both rather than check if a file is a dir.
    यदि (DIR* ध्रुव dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } अन्यथा यदि (ध्रुव पूर्णाङ्क fd = ::उद्घाटय(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        यदि (fd > 0) ::close(fd);
    }
#यदि_अन्तः
}

शून्य V3Os::filesystemFlushBuildDir(ध्रुव string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#यदि !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ध्रुव पूर्णाङ्क err = utimes(dirname.c_str(), शून्यसूचकः);
    // Not an error
    यदि (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#यदि_अन्तः
    filesystemFlush(dirname);
}

शून्य V3Os::unlinkRegexp(ध्रुव string& dir, ध्रुव string& regexp) {
#यदि_परिभाषितम् _MSC_VER
    यत्न {
        प्रति (ध्रुव स्वचालित& dir_प्रविष्टिः : std::filesystem::directory_iterator(dir.c_str())) {
            यदि (VString::wildmatch(dir_प्रविष्टिः.मार्गः().filename().string(), regexp.c_str())) {
                ध्रुव string fullname = dir + "/" + dir_प्रविष्टिः.मार्गः().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } गृहाण (std::filesystem::filesystem_error ध्रुव& ex) {}
#अन्यथा
    यदि (DIR* ध्रुव dirp = opendir(dir.c_str())) {
        यावत् (संरचना dirent* ध्रुव direntp = readdir(dirp)) {
            यदि (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ध्रुव string fullname = dir + "/" + std::string{direntp->d_name};
#यदि defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#अन्यथा
                unlink(fullname.c_str());
#यदि_अन्तः
            }
        }
        closedir(dirp);
    }
#यदि_अन्तः
}

//######################################################################
// METHODS (memory)

शून्य V3Os::release_स्मृतिः() {
#यदि_परिभाषितम् HAVE_TCMALLOC
    MallocExtension::instance()->Release_Free_स्मृतिः();
#यदि_अन्तः
#यदि_परिभाषितम् HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    अचिह्नित narenas = 0;
    आकार_t sz = परिमाण(narenas);
    यदि (mallctl("arenas.narenas", &narenas, &sz, शून्यसूचकः, 0)) {
        प्रत्यागम;  // Failed to get number of arenas, give up
    }
    वर्ण buf[64];
    // Index equal to narenas represents all arenas
    (शून्य)VL_SNPRINTF(buf, परिमाण(buf), "arena.%u.purge", narenas);
    mallctl(buf, शून्यसूचकः, शून्यसूचकः, शून्यसूचकः, 0);
#यदि_अन्तः
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ध्रुव uint64_t परिणाम = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    प्रत्यागम परिणाम;
}

string V3Os::trueRandom(आकार_t आकार) VL_MT_SAFE {
    string परिणाम(आकार, '\xFF');
    वर्ण* ध्रुव दत्त = const_cast<वर्ण*>(परिणाम.दत्त());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#यदि defined(_WIN32) || defined(__MINGW32__)
    ध्रुव NTSTATUS hr = BCryptGenRandom(शून्यसूचकः, reinterpret_cast<BYTE*>(दत्त), आकार,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    यदि (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#नोचेद्यदि defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    यदि (VL_UNCOVERABLE(getentropy(दत्त, आकार))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#अन्यथा
    std::ifstream is{"/dev/urandom", std::ios::अन्तर् | std::ios::द्विमान};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    यदि (VL_UNCOVERABLE(!is.पठ(दत्त, आकार))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#यदि_अन्तः
    प्रत्यागम परिणाम;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#यदि defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    स्थिर constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    ध्रुव uint64_t us
        = ((static_cast<uint64_t>(ft.dw_उच्चः_Date_Time) << 32) + ft.dw_नीचः_Date_Time + 5ULL) / 10ULL;
    प्रत्यागम us - EPOCH_DIFFERENCE_USECS;
#अन्यथा
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    यदि (gettimeofday(&tv, शून्यसूचकः) < 0) प्रत्यागम 0;
    प्रत्यागम static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#यदि_अन्तः
}

शून्य V3Os::u_sleep(int64_t usec) {
#यदि defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#अन्यथा
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#यदि_अन्तः
}

//######################################################################
// METHODS (sub command)

पूर्णाङ्क V3Os::system(ध्रुव string& command) {
#यदि_परिभाषितम् __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    प्रत्यागम -1;
#अन्यथा
    UINFO(1, "Running system: " << command);
    ध्रुव पूर्णाङ्क ret = ::system(command.c_str());
    यदि (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        प्रत्यागम -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ध्रुव पूर्णाङ्क exit_संहिता = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_संहिता);
    UASSERT(exit_संहिता >= 0, "exit code must not be negative");
    प्रत्यागम exit_संहिता;
#यदि_अन्तः
}

शून्य V3Os::selfTest() {
#यदि_परिभाषितम् VL_DEBUG
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
    UASSERT_SELFTEST(filename_Relative_मार्गः("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_मार्गः("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_मार्गः("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_मार्गः("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_मार्गः("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_मार्गः("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_मार्गः("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filename_Relative_मार्गः("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_मार्गः("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_मार्गः("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_मार्गः("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_मार्गः("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_मार्गः("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_मार्गः("a/b/x/y", "a/q"), "../b/x/y");
#यदि_अन्तः
}
