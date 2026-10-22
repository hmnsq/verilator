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
#अगर defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_VERSION
#  define PSAPI_VERSION 1  // Needed for compatibility with Windows 7
# endif
#पूर्ण_अगर
#अगर_अघोषित __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#पूर्ण_अगर
// clang-format on

#समावेश "config_build.h"
#समावेश "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#समावेश "V3Os.h"
#समावेश "V3String.h"

#अगर_अघोषित V3ERROR_NO_GLOBAL_
#समावेश "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#पूर्ण_अगर

#समावेश <cerrno>
#समावेश <climits>  // PATH_MAX (especially on FreeBSD)
#समावेश <cstdarg>
#अगर_घोषित _MSC_VER
#समावेश <filesystem>  // C++17
#घोषणा PATH_MAX MAX_PATH
#अन्यथा
#समावेश <dirent.h>
#समावेश <utime.h>
#पूर्ण_अगर
#समावेश <fcntl.h>
#समावेश <fstream>
#समावेश <memory>

#समावेश <sys/stat.h>
#समावेश <sys/types.h>

#अगर_घोषित HAVE_TCMALLOC
#समावेश <gperftools/malloc_extension.h>
#पूर्ण_अगर
#अगर_घोषित HAVE_JEMALLOC
#समावेश <jemalloc/jemalloc.h>
#पूर्ण_अगर

// clang-format off
#अगर defined(_WIN32) || defined(__MINGW32__)
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
#पूर्ण_अगर
// clang-format on

#घोषणा VL_ALLOW_VERILATEDOS_C
#समावेश "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(स्थिर string& envvar, स्थिर string& default_मूल्य) {
    वापस VString::escapeStringForPath(VlOs::getenvStr(envvar, default_मूल्य));
}

व्योम V3Os::setenvStr(स्थिर string& envvar, स्थिर string& मूल्य, स्थिर string& why) {
    अगर (why != "") {
        UINFO(1, "export " << envvar << "='" << मूल्य << "'  # " << why);
    } अन्यथा {
        UINFO(1, "export " << envvar << "='" << मूल्य << "'");
    }
#अगर defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), मूल्य.c_str());
#या_अगर defined(_BSD_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), मूल्य.c_str(), true);
#अन्यथा
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    स्थिर string vareq = envvar + "=" + मूल्य;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#पूर्ण_अगर
}

//######################################################################
// Generic filename utilities

#अगर defined(_WIN32) || defined(__MINGW32__)
अटल constexpr अक्षर V3OS_SLASH = '\\';
#अन्यथा
अटल constexpr अक्षर V3OS_SLASH = '/';
#पूर्ण_अगर

अटल bool isSlash(अक्षर ch) VL_PURE {
#अगर defined(_WIN32) || defined(__MINGW32__)
    वापस ch == '/' || ch == '\\';
#अन्यथा
    वापस ch == '/';
#पूर्ण_अगर
}

string V3Os::filenameCleanup(स्थिर string& filename) VL_PURE {
    string str;
    str.reserve(filename.लंबाई());
    bool lastIsSlash = false;
    क्रम (स्थिर अक्षर ch : filename) {
        स्थिर bool lastIsSlashOld = lastIsSlash;
        lastIsSlash = isSlash(ch);
        अगर (lastIsSlash && lastIsSlashOld) जारी;
        str += ch;
    }
    अगर (str.आकार() > 1 && isSlash(str.back())) str.निकालो_back();
    जबतक (str.आकार() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    वापस str;
}

string V3Os::filenameJoin(std::initializer_list<स्थिर std::string> paths) VL_PURE {
    string fullpath;
    क्रम (स्थिर स्वतः& item : paths) {
        अगर (item.empty() || item == ".") जारी;
        अगर (!fullpath.empty()) fullpath += V3OS_SLASH;
        fullpath += item;
    }
    वापस fullpath;
}

string V3Os::filenameDir(स्थिर string& filename) VL_PURE {
    // std::filesystem::path::parent_path
    स्वतः it = filename.rbegin();
    क्रम (; it != filename.rend(); ++it) {
        अगर (isSlash(*it)) अवरोध;
    }
    अगर (it.base() == filename.begin()) वापस ".";
    वापस string{filename.begin(), (++it).base()};
}

string V3Os::filenameExt(स्थिर string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    स्थिर string::आकार_type pos = base.find('.');
    अगर (pos != string::npos) {
        base.erase(0, pos);
        वापस base;
    }
    वापस "";
}

string V3Os::filenameNonDir(स्थिर string& filename) VL_PURE {
    // std::filesystem::path::filename
    स्वतः it = filename.rbegin();
    क्रम (; it != filename.rend(); ++it) {
        अगर (isSlash(*it)) अवरोध;
    }
    वापस string{it.base(), filename.end()};
}

string V3Os::filenameNonDirExt(स्थिर string& filename) VL_PURE {
    string base = filenameNonDir(filename);
    स्थिर string::आकार_type pos = base.find('.');
    अगर (pos != string::npos) base.erase(pos);
    वापस base;
}

string V3Os::filenameSubstitute(स्थिर string& filename) {
    string परिणाम;
    // cppcheck-has-bug-suppress unusedLabel
    क्रमागत : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    क्रम (string::आकार_type pos = 0; pos < filename.लंबाई(); ++pos) {
        अगर ((filename[pos] == '$') && (pos + 1 < filename.लंबाई())) {
            चयन (filename[pos + 1]) {
            हाल '{': brackets = CURLY; अवरोध;
            हाल '(': brackets = PAREN; अवरोध;
            शेष: brackets = NONE; अवरोध;
            }
            अगर (brackets != NONE) pos = pos + 1;
            string::आकार_type endpos = pos + 1;
            जबतक (((endpos + 1) < filename.लंबाई())
                   && (((brackets == NONE)
                        && (std::है_अक्षर_अंक(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            अगर (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            स्थिर string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            अगर (!envvar.empty()) envvalue = getenvStr(envvar, "");
            अगर (!envvalue.empty()) {
                परिणाम += envvalue;
                अगर (brackets == NONE) {
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
    वापस परिणाम;
}

string V3Os::filenameRealPath(स्थिर string& filename) VL_PURE {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    अक्षर retpath[PATH_MAX];
    अगर (
#अगर defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), PATH_MAX)
#अन्यथा
        realpath(filename.c_str(), retpath)
#पूर्ण_अगर
    ) {
        वापस std::string{retpath};
    }
    वापस filename;
}

string V3Os::filenameAbsolutePath(स्थिर string& filename) VL_PURE {
#अगर defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    वापस filenameRealPath(filename);
#अन्यथा
    अगर (!filenameIsRel(filename)) वापस filename;
    वापस filenameJoin(filenameRealPath("."), filename);
#पूर्ण_अगर
}

string V3Os::filenameRelativePath(स्थिर string& filename, स्थिर string& base) VL_PURE {
    स्थिर string a = V3Os::filenameRealPath(filename);
    स्थिर string b = V3Os::filenameRealPath(base);
    string परिणाम;
    अगर (a == b) वापस ".";

    स्वतः aIt = a.begin();
    स्वतः bIt = b.begin();
    जबतक (aIt != a.end() && bIt != b.end()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        स्वतः aWordIt = aIt;  // position of next slash
        क्रम (; aWordIt != a.end(); ++aWordIt) {
            अगर (isSlash(*aWordIt)) अवरोध;
        }

        स्वतः bWordIt = bIt;  // position of next slash
        क्रम (; bWordIt != b.end(); ++bWordIt) {
            अगर (isSlash(*bWordIt)) अवरोध;
        }

        स्थिर string aWord = a.substr(aIt - a.begin(), aWordIt - aIt);
        स्थिर string bWord = b.substr(bIt - b.begin(), bWordIt - bIt);
        अगर (aWord != bWord) अवरोध;
        aIt = aWordIt;
        bIt = bWordIt;
        अगर (aIt != a.end()) ++aIt;  // Skip slash
        अगर (bIt != b.end()) ++bIt;  // Skip slash
    }

    जबतक (bIt != b.end()) {
        क्रम (; bIt != b.end(); ++bIt) {
            अगर (isSlash(*bIt)) {
                ++bIt;
                अवरोध;
            }
        }
        अगर (!परिणाम.empty()) परिणाम += "/";
        परिणाम += "..";
    }

    स्थिर string a_बायाँ = a.substr(aIt - a.begin());
    अगर (!a_बायाँ.empty()) {
        अगर (!परिणाम.empty()) परिणाम += "/";
        परिणाम += a_बायाँ;
    }
    वापस filenameCleanup(परिणाम);
}

bool V3Os::filenameIsRel(स्थिर string& filename) VL_PURE {
#अगर defined(_MSC_VER)
    वापस std::filesystem::path(filename).is_relative();
#अन्यथा
    वापस (filename.लंबाई() > 0 && filename[0] != '/');
#पूर्ण_अगर
}

string V3Os::filenameSlashPath(स्थिर string& filename) VL_PURE {
#अगर defined(_WIN32) || defined(__MINGW32__)
    string slashedPath = filename;
    std::replace(slashedPath.begin(), slashedPath.end(), '\\', '/');
    वापस slashedPath;
#अन्यथा
    वापस filename;
#पूर्ण_अगर
}

//######################################################################
// File utilities

string V3Os::लो_पंक्ति(std::अ_प्रवाह& is, अक्षर delim) {
    string पंक्ति;
#अगर_घोषित __CYGWIN__  // Work around buggy implementation of getline
    अक्षर buf[65536];
    is.लो_पंक्ति(buf, 65535, delim);
    पंक्ति = buf;
#अन्यथा
    std::लो_पंक्ति(is, पंक्ति, delim);
#पूर्ण_अगर
    वापस पंक्ति;
}

//######################################################################
// Directory utilities

व्योम V3Os::createDir(स्थिर string& स_नाम) {
#अगर defined(_WIN32) || defined(__MINGW32__)
    _mkdir(स_नाम.c_str());
#अन्यथा
    सूची_गढ़ो(स_नाम.c_str(), 0777);
#पूर्ण_अगर
}

व्योम V3Os::filesystemFlush(स्थिर string& स_नाम) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#अगर defined(_MSC_VER) || defined(__MINGW32__)
    अगर (पूर्णांक fd = ::खोलो(स_नाम.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        अगर (fd > 0) ::बंद(fd);
    }
#अन्यथा
    // Faster to just try both rather than check if a file is a dir.
    अगर (सूची* स्थिर dirp = सूची_खोलो(स_नाम.c_str())) {  // LCOV_EXCL_BR_LINE
        बंद_सूची(dirp);  // LCOV_EXCL_LINE
    } अन्यथा अगर (स्थिर पूर्णांक fd = ::खोलो(स_नाम.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        अगर (fd > 0) ::बंद(fd);
    }
#पूर्ण_अगर
}

व्योम V3Os::filesystemFlushBuildDir(स्थिर string& स_नाम) {
    // Attempt to force out written directory, for NFS like file systems.
#अगर !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    स्थिर पूर्णांक err = utimes(स_नाम.c_str(), nullptr);
    // Not an error
    अगर (err != 0) UINFO(1, "-Info: File not utimed: " << स_नाम);
#पूर्ण_अगर
    filesystemFlush(स_नाम);
}

व्योम V3Os::unlinkRegexp(स्थिर string& dir, स्थिर string& regexp) {
#अगर_घोषित _MSC_VER
    प्रयास {
        क्रम (स्थिर स्वतः& dirEntry : std::filesystem::directory_iterator(dir.c_str())) {
            अगर (VString::wildmatch(dirEntry.path().filename().string(), regexp.c_str())) {
                स्थिर string fullname = dir + "/" + dirEntry.path().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } पकड़ो (std::filesystem::filesystem_error स्थिर& ex) {}
#अन्यथा
    अगर (सूची* स्थिर dirp = सूची_खोलो(dir.c_str())) {
        जबतक (काष्ठा dirent* स्थिर direntp = सूची_पढ़ो(dirp)) {
            अगर (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                स्थिर string fullname = dir + "/" + std::string{direntp->d_name};
#अगर defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#अन्यथा
                unlink(fullname.c_str());
#पूर्ण_अगर
            }
        }
        बंद_सूची(dirp);
    }
#पूर्ण_अगर
}

//######################################################################
// METHODS (memory)

व्योम V3Os::releaseMemory() {
#अगर_घोषित HAVE_TCMALLOC
    MallocExtension::instance()->ReleaseFreeMemory();
#पूर्ण_अगर
#अगर_घोषित HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    अचिन्हित narenas = 0;
    माप_प्रकार sz = माप(narenas);
    अगर (mallctl("arenas.narenas", &narenas, &sz, nullptr, 0)) {
        वापस;  // Failed to get number of arenas, give up
    }
    अक्षर buf[64];
    // Index equal to narenas represents all arenas
    (व्योम)VL_SNPRINTF(buf, माप(buf), "arena.%u.purge", narenas);
    mallctl(buf, nullptr, nullptr, nullptr, 0);
#पूर्ण_अगर
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    स्थिर uint64_t परिणाम = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    वापस परिणाम;
}

string V3Os::trueRandom(माप_प्रकार आकार) VL_MT_SAFE {
    string परिणाम(आकार, '\xFF');
    अक्षर* स्थिर तथ्य = const_cast<अक्षर*>(परिणाम.तथ्य());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#अगर defined(_WIN32) || defined(__MINGW32__)
    स्थिर NTSTATUS hr = BCryptGenRandom(nullptr, reinterpret_cast<BYTE*>(तथ्य), आकार,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    अगर (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#या_अगर defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    अगर (VL_UNCOVERABLE(getentropy(तथ्य, आकार))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#अन्यथा
    std::अख_प्रवाह is{"/dev/urandom", std::पनप::in | std::पनप::द्विग};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    अगर (VL_UNCOVERABLE(!is.पढ़ो(तथ्य, आकार))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#पूर्ण_अगर
    वापस परिणाम;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::timeUsecs() {
#अगर defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    अटल constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    GetSystemTimeAsFileTime(&ft);
    स्थिर uint64_t us
        = ((static_cast<uint64_t>(ft.dw_ऊपरी_Date_Time) << 32) + ft.dw_निचला_Date_Time + 5ULL) / 10ULL;
    वापस us - EPOCH_DIFFERENCE_USECS;
#अन्यथा
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    अगर (gettimeofday(&tv, nullptr) < 0) वापस 0;
    वापस static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#पूर्ण_अगर
}

व्योम V3Os::u_sleep(int64_t usec) {
#अगर defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#अन्यथा
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#पूर्ण_अगर
}

//######################################################################
// METHODS (sub command)

पूर्णांक V3Os::प्रणाली(स्थिर string& command) {
#अगर_घोषित __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    वापस -1;
#अन्यथा
    UINFO(1, "Running system: " << command);
    स्थिर पूर्णांक ret = ::प्रणाली(command.c_str());
    अगर (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::म_त्रुटि(त्रुटि_सं));  // LCOV_EXCL_LINE
        वापस -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    स्थिर पूर्णांक exit_code = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_code);
    UASSERT(exit_code >= 0, "exit code must not be negative");
    वापस exit_code;
#पूर्ण_अगर
}

व्योम V3Os::selfTest() {
#अगर_घोषित VL_DEBUG
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
#पूर्ण_अगर
}
