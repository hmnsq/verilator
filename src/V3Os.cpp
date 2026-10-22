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
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_ਸੰਸਕਰਣ
#  define PSAPI_ਸੰਸਕਰਣ 1  // Needed for compatibility with Windows 7
# endif
#ਜੇ_ਅੰਤ
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#ਜੇ_ਅੰਤ
// clang-format on

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ <cerrno>
#ਅਨ੍ਤਰ੍ਭਾਵ <climits>  // PATH_MAX (especially on FreeBSD)
#ਅਨ੍ਤਰ੍ਭਾਵ <cstdarg>
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ _MSC_VER
#ਅਨ੍ਤਰ੍ਭਾਵ <filesystem>  // C++17
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਰਸਤਾ_MAX MAX_ਰਸਤਾ
#ਨਹੀਂਤਾਂ
#ਅਨ੍ਤਰ੍ਭਾਵ <dirent.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <utime.h>
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ <fcntl.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <fstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>

#ਅਨ੍ਤਰ੍ਭਾਵ <sys/stat.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <sys/types.h>

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_TCMALLOC
#ਅਨ੍ਤਰ੍ਭਾਵ <gperftools/malloc_extension.h>
#ਜੇ_ਅੰਤ
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_JEMALLOC
#ਅਨ੍ਤਰ੍ਭਾਵ <jemalloc/jemalloc.h>
#ਜੇ_ਅੰਤ

// clang-format off
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
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
#  ਨਹੀਂਤਾਂ
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#ਨਹੀਂਤਾਂ
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#ਜੇ_ਅੰਤ
// clang-format on

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_ALLOW_VERILATEDOS_C
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ਅਚਲ string& envvar, ਅਚਲ string& default_ਮੁੱਲ) {
    ਵਾਪਸ VString::escape_String_For_ਰਸਤਾ(VlOs::getenvStr(envvar, default_ਮੁੱਲ));
}

ਖਾਲੀ V3Os::setenvStr(ਅਚਲ string& envvar, ਅਚਲ string& ਮੁੱਲ, ਅਚਲ string& why) {
    ਜੇ (why != "") {
        UINFO(1, "export " << envvar << "='" << ਮੁੱਲ << "'  # " << why);
    } ਨਹੀਂਤਾਂ {
        UINFO(1, "export " << envvar << "='" << ਮੁੱਲ << "'");
    }
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), ਮੁੱਲ.c_str());
#ਜਾਂਜੇ defined(BSD_ਸਰੋਤ) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), ਮੁੱਲ.c_str(), ਸੱਚ);
#ਨਹੀਂਤਾਂ
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ਅਚਲ string vareq = envvar + "=" + ਮੁੱਲ;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#ਜੇ_ਅੰਤ
}

//######################################################################
// Generic filename utilities

#ਜੇ defined(_WIN32) || defined(__MINGW32__)
ਸਥਿਰ constexpr ਅੱਖਰ V3OS_SLASH = '\\';
#ਨਹੀਂਤਾਂ
ਸਥਿਰ constexpr ਅੱਖਰ V3OS_SLASH = '/';
#ਜੇ_ਅੰਤ

ਸਥਿਰ ਤਾਰਕਿਕ isSlash(ਅੱਖਰ ch) VL_ਸ਼ੁੱਧ {
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
    ਵਾਪਸ ch == '/' || ch == '\\';
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ ch == '/';
#ਜੇ_ਅੰਤ
}

string V3Os::filenameCleanup(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ {
    string str;
    str.reserve(filename.ਲੰਮਾਈ());
    ਤਾਰਕਿਕ ਆਖਰੀ_Is_Slash = ਝੂਠ;
    ਲਈ (ਅਚਲ ਅੱਖਰ ch : filename) {
        ਅਚਲ ਤਾਰਕਿਕ ਆਖਰੀ_Is_Slash_Old = ਆਖਰੀ_Is_Slash;
        ਆਖਰੀ_Is_Slash = isSlash(ch);
        ਜੇ (ਆਖਰੀ_Is_Slash && ਆਖਰੀ_Is_Slash_Old) ਜਾਰੀ;
        str += ch;
    }
    ਜੇ (str.ਮਾਪ() > 1 && isSlash(str.ਪਿੱਠ())) str.ਕੱਢੋ_ਪਿੱਠ();
    ਜਦੋਂ (str.ਮਾਪ() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    ਵਾਪਸ str;
}

string V3Os::filenameJoin(std::initializer_list<ਅਚਲ std::string> paths) VL_ਸ਼ੁੱਧ {
    string fullpath;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਵਸਤੂ_3 : paths) {
        ਜੇ (ਵਸਤੂ_3.ਖਾਲੀ_ਥਾਵਾਂ() || ਵਸਤੂ_3 == ".") ਜਾਰੀ;
        ਜੇ (!fullpath.ਖਾਲੀ_ਥਾਵਾਂ()) fullpath += V3OS_SLASH;
        fullpath += ਵਸਤੂ_3;
    }
    ਵਾਪਸ fullpath;
}

string V3Os::filenameDir(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ {
    // std::filesystem::path::parent_path
    ਸਵੈਚਾਲਿਤ it = filename.rbegin();
    ਲਈ (; it != filename.rend(); ++it) {
        ਜੇ (isSlash(*it)) ਰੋਕੋ;
    }
    ਜੇ (it.ਅਧਾਰ() == filename.begin()) ਵਾਪਸ ".";
    ਵਾਪਸ string{filename.begin(), (++it).ਅਧਾਰ()};
}

string V3Os::filenameExt(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ {
    string ਅਧਾਰ = filenameNonDir(filename);
    ਅਚਲ string::ਮਾਪ_type pos = ਅਧਾਰ.ਲੱਭੋ('.');
    ਜੇ (pos != string::npos) {
        ਅਧਾਰ.erase(0, pos);
        ਵਾਪਸ ਅਧਾਰ;
    }
    ਵਾਪਸ "";
}

string V3Os::filenameNonDir(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ {
    // std::filesystem::path::filename
    ਸਵੈਚਾਲਿਤ it = filename.rbegin();
    ਲਈ (; it != filename.rend(); ++it) {
        ਜੇ (isSlash(*it)) ਰੋਕੋ;
    }
    ਵਾਪਸ string{it.ਅਧਾਰ(), filename.ਸਿਰਾ()};
}

string V3Os::filenameNonDirExt(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ {
    string ਅਧਾਰ = filenameNonDir(filename);
    ਅਚਲ string::ਮਾਪ_type pos = ਅਧਾਰ.ਲੱਭੋ('.');
    ਜੇ (pos != string::npos) ਅਧਾਰ.erase(pos);
    ਵਾਪਸ ਅਧਾਰ;
}

string V3Os::filenameSubstitute(ਅਚਲ string& filename) {
    string ਨਤੀਜਾ;
    // cppcheck-has-bug-suppress unusedLabel
    ਗਿਣਤੀ : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    ਲਈ (string::ਮਾਪ_type pos = 0; pos < filename.ਲੰਮਾਈ(); ++pos) {
        ਜੇ ((filename[pos] == '$') && (pos + 1 < filename.ਲੰਮਾਈ())) {
            ਚੋਣ (filename[pos + 1]) {
            ਹਾਲਤ '{': brackets = CURLY; ਰੋਕੋ;
            ਹਾਲਤ '(': brackets = PAREN; ਰੋਕੋ;
            ਮੂਲ: brackets = NONE; ਰੋਕੋ;
            }
            ਜੇ (brackets != NONE) pos = pos + 1;
            string::ਮਾਪ_type endpos = pos + 1;
            ਜਦੋਂ (((endpos + 1) < filename.ਲੰਮਾਈ())
                   && (((brackets == NONE)
                        && (std::ਅੱਖਰ_ਜਾਂ_ਅੰਕ(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            ਜੇ (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ਅਚਲ string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            ਜੇ (!envvar.ਖਾਲੀ_ਥਾਵਾਂ()) envvalue = getenvStr(envvar, "");
            ਜੇ (!envvalue.ਖਾਲੀ_ਥਾਵਾਂ()) {
                ਨਤੀਜਾ += envvalue;
                ਜੇ (brackets == NONE) {
                    pos = endpos;
                } ਨਹੀਂਤਾਂ {
                    pos = endpos + 1;
                }
            } ਨਹੀਂਤਾਂ {
                ਨਤੀਜਾ += filename[pos];  // *pos == '$'
            }
        } ਨਹੀਂਤਾਂ {
            ਨਤੀਜਾ += filename[pos];
        }
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

string V3Os::filename_Real_ਰਸਤਾ(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    ਅੱਖਰ retpath[ਰਸਤਾ_MAX];
    ਜੇ (
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), ਰਸਤਾ_MAX)
#ਨਹੀਂਤਾਂ
        realpath(filename.c_str(), retpath)
#ਜੇ_ਅੰਤ
    ) {
        ਵਾਪਸ std::string{retpath};
    }
    ਵਾਪਸ filename;
}

string V3Os::filename_Absolute_ਰਸਤਾ(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ {
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    ਵਾਪਸ filename_Real_ਰਸਤਾ(filename);
#ਨਹੀਂਤਾਂ
    ਜੇ (!filenameIsRel(filename)) ਵਾਪਸ filename;
    ਵਾਪਸ filenameJoin(filename_Real_ਰਸਤਾ("."), filename);
#ਜੇ_ਅੰਤ
}

string V3Os::filename_Relative_ਰਸਤਾ(ਅਚਲ string& filename, ਅਚਲ string& ਅਧਾਰ) VL_ਸ਼ੁੱਧ {
    ਅਚਲ string a = V3Os::filename_Real_ਰਸਤਾ(filename);
    ਅਚਲ string b = V3Os::filename_Real_ਰਸਤਾ(ਅਧਾਰ);
    string ਨਤੀਜਾ;
    ਜੇ (a == b) ਵਾਪਸ ".";

    ਸਵੈਚਾਲਿਤ aIt = a.begin();
    ਸਵੈਚਾਲਿਤ bIt = b.begin();
    ਜਦੋਂ (aIt != a.ਸਿਰਾ() && bIt != b.ਸਿਰਾ()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        ਸਵੈਚਾਲਿਤ a_ਸ਼ਬਦ_It = aIt;  // position of next slash
        ਲਈ (; a_ਸ਼ਬਦ_It != a.ਸਿਰਾ(); ++a_ਸ਼ਬਦ_It) {
            ਜੇ (isSlash(*a_ਸ਼ਬਦ_It)) ਰੋਕੋ;
        }

        ਸਵੈਚਾਲਿਤ b_ਸ਼ਬਦ_It = bIt;  // position of next slash
        ਲਈ (; b_ਸ਼ਬਦ_It != b.ਸਿਰਾ(); ++b_ਸ਼ਬਦ_It) {
            ਜੇ (isSlash(*b_ਸ਼ਬਦ_It)) ਰੋਕੋ;
        }

        ਅਚਲ string a_ਸ਼ਬਦ = a.substr(aIt - a.begin(), a_ਸ਼ਬਦ_It - aIt);
        ਅਚਲ string b_ਸ਼ਬਦ = b.substr(bIt - b.begin(), b_ਸ਼ਬਦ_It - bIt);
        ਜੇ (a_ਸ਼ਬਦ != b_ਸ਼ਬਦ) ਰੋਕੋ;
        aIt = a_ਸ਼ਬਦ_It;
        bIt = b_ਸ਼ਬਦ_It;
        ਜੇ (aIt != a.ਸਿਰਾ()) ++aIt;  // Skip slash
        ਜੇ (bIt != b.ਸਿਰਾ()) ++bIt;  // Skip slash
    }

    ਜਦੋਂ (bIt != b.ਸਿਰਾ()) {
        ਲਈ (; bIt != b.ਸਿਰਾ(); ++bIt) {
            ਜੇ (isSlash(*bIt)) {
                ++bIt;
                ਰੋਕੋ;
            }
        }
        ਜੇ (!ਨਤੀਜਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ਨਤੀਜਾ += "/";
        ਨਤੀਜਾ += "..";
    }

    ਅਚਲ string a_ਖੱਬਾ = a.substr(aIt - a.begin());
    ਜੇ (!a_ਖੱਬਾ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਜੇ (!ਨਤੀਜਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ਨਤੀਜਾ += "/";
        ਨਤੀਜਾ += a_ਖੱਬਾ;
    }
    ਵਾਪਸ filenameCleanup(ਨਤੀਜਾ);
}

ਤਾਰਕਿਕ V3Os::filenameIsRel(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ {
#ਜੇ defined(_MSC_VER)
    ਵਾਪਸ std::filesystem::ਰਸਤਾ(filename).is_relative();
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ (filename.ਲੰਮਾਈ() > 0 && filename[0] != '/');
#ਜੇ_ਅੰਤ
}

string V3Os::filename_Slash_ਰਸਤਾ(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ {
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
    string slashed_ਰਸਤਾ = filename;
    std::replace(slashed_ਰਸਤਾ.begin(), slashed_ਰਸਤਾ.ਸਿਰਾ(), '\\', '/');
    ਵਾਪਸ slashed_ਰਸਤਾ;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ filename;
#ਜੇ_ਅੰਤ
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, ਅੱਖਰ delim) {
    string ਸਤਰ;
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ __CYGWIN__  // Work around buggy implementation of getline
    ਅੱਖਰ buf[65536];
    is.getline(buf, 65535, delim);
    ਸਤਰ = buf;
#ਨਹੀਂਤਾਂ
    std::getline(is, ਸਤਰ, delim);
#ਜੇ_ਅੰਤ
    ਵਾਪਸ ਸਤਰ;
}

//######################################################################
// Directory utilities

ਖਾਲੀ V3Os::createDir(ਅਚਲ string& dirname) {
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#ਨਹੀਂਤਾਂ
    mkdir(dirname.c_str(), 0777);
#ਜੇ_ਅੰਤ
}

ਖਾਲੀ V3Os::filesystemFlush(ਅਚਲ string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#ਜੇ defined(_MSC_VER) || defined(__MINGW32__)
    ਜੇ (ਪੂਰਨ fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ਜੇ (fd > 0) ::close(fd);
    }
#ਨਹੀਂਤਾਂ
    // Faster to just try both rather than check if a file is a dir.
    ਜੇ (DIR* ਅਚਲ dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ ਪੂਰਨ fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ਜੇ (fd > 0) ::close(fd);
    }
#ਜੇ_ਅੰਤ
}

ਖਾਲੀ V3Os::filesystemFlushBuildDir(ਅਚਲ string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#ਜੇ !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ਅਚਲ ਪੂਰਨ err = utimes(dirname.c_str(), ਖਾਲੀ_ਸੂਚਕ);
    // Not an error
    ਜੇ (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#ਜੇ_ਅੰਤ
    filesystemFlush(dirname);
}

ਖਾਲੀ V3Os::unlinkRegexp(ਅਚਲ string& dir, ਅਚਲ string& regexp) {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ _MSC_VER
    ਕੋਸ਼ਿਸ਼ {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& dir_ਇੰਦਰਾਜ : std::filesystem::directory_iterator(dir.c_str())) {
            ਜੇ (VString::wildmatch(dir_ਇੰਦਰਾਜ.ਰਸਤਾ().filename().string(), regexp.c_str())) {
                ਅਚਲ string fullname = dir + "/" + dir_ਇੰਦਰਾਜ.ਰਸਤਾ().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } ਫੜੋ (std::filesystem::filesystem_error ਅਚਲ& ex) {}
#ਨਹੀਂਤਾਂ
    ਜੇ (DIR* ਅਚਲ dirp = opendir(dir.c_str())) {
        ਜਦੋਂ (ਬਣਤਰ dirent* ਅਚਲ direntp = readdir(dirp)) {
            ਜੇ (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ਅਚਲ string fullname = dir + "/" + std::string{direntp->d_name};
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#ਨਹੀਂਤਾਂ
                unlink(fullname.c_str());
#ਜੇ_ਅੰਤ
            }
        }
        closedir(dirp);
    }
#ਜੇ_ਅੰਤ
}

//######################################################################
// METHODS (memory)

ਖਾਲੀ V3Os::release_ਸਮ੍ਰਿਤੀ() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_TCMALLOC
    MallocExtension::instance()->Release_Free_ਸਮ੍ਰਿਤੀ();
#ਜੇ_ਅੰਤ
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    ਬਿਨਾਨਿਸ਼ਾਨੀ narenas = 0;
    ਮਾਪ_t sz = ਆਕਾਰ(narenas);
    ਜੇ (mallctl("arenas.narenas", &narenas, &sz, ਖਾਲੀ_ਸੂਚਕ, 0)) {
        ਵਾਪਸ;  // Failed to get number of arenas, give up
    }
    ਅੱਖਰ buf[64];
    // Index equal to narenas represents all arenas
    (ਖਾਲੀ)VL_SNPRINTF(buf, ਆਕਾਰ(buf), "arena.%u.purge", narenas);
    mallctl(buf, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, 0);
#ਜੇ_ਅੰਤ
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ਅਚਲ uint64_t ਨਤੀਜਾ = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    ਵਾਪਸ ਨਤੀਜਾ;
}

string V3Os::trueRandom(ਮਾਪ_t ਮਾਪ) VL_MT_ਸੁਰੱਖਿਅਤ {
    string ਨਤੀਜਾ(ਮਾਪ, '\xFF');
    ਅੱਖਰ* ਅਚਲ ਅੰਕੜੇ = const_cast<ਅੱਖਰ*>(ਨਤੀਜਾ.ਅੰਕੜੇ());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
    ਅਚਲ NTSTATUS hr = BCryptGenRandom(ਖਾਲੀ_ਸੂਚਕ, reinterpret_cast<BYTE*>(ਅੰਕੜੇ), ਮਾਪ,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ਜੇ (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#ਜਾਂਜੇ defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    ਜੇ (VL_UNCOVERABLE(getentropy(ਅੰਕੜੇ, ਮਾਪ))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#ਨਹੀਂਤਾਂ
    std::ifstream is{"/dev/urandom", std::ios::ਵਿੱਚ_2 | std::ios::ਦੁਆਧਾਰੀ};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    ਜੇ (VL_UNCOVERABLE(!is.ਪਠ(ਅੰਕੜੇ, ਮਾਪ))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#ਜੇ_ਅੰਤ
    ਵਾਪਸ ਨਤੀਜਾ;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::ਸਮਾਂ_Usecs() {
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ਸਥਿਰ constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    Get_System_ਸਮਾਂ_As_File_ਸਮਾਂ(&ft);
    ਅਚਲ uint64_t us
        = ((static_cast<uint64_t>(ft.dw_ਉੱਚਾ_Date_ਸਮਾਂ) << 32) + ft.dw_ਨੀਵਾਂ_Date_ਸਮਾਂ + 5ULL) / 10ULL;
    ਵਾਪਸ us - EPOCH_DIFFERENCE_USECS;
#ਨਹੀਂਤਾਂ
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    ਜੇ (gettimeofday(&tv, ਖਾਲੀ_ਸੂਚਕ) < 0) ਵਾਪਸ 0;
    ਵਾਪਸ static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#ਜੇ_ਅੰਤ
}

ਖਾਲੀ V3Os::u_sleep(int64_t usec) {
#ਜੇ defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#ਨਹੀਂਤਾਂ
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#ਜੇ_ਅੰਤ
}

//######################################################################
// METHODS (sub command)

ਪੂਰਨ V3Os::system(ਅਚਲ string& command) {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    ਵਾਪਸ -1;
#ਨਹੀਂਤਾਂ
    UINFO(1, "Running system: " << command);
    ਅਚਲ ਪੂਰਨ ret = ::system(command.c_str());
    ਜੇ (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        ਵਾਪਸ -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ਅਚਲ ਪੂਰਨ exit_ਕੋਡ = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_ਕੋਡ);
    UASSERT(exit_ਕੋਡ >= 0, "exit code must not be negative");
    ਵਾਪਸ exit_ਕੋਡ;
#ਜੇ_ਅੰਤ
}

ਖਾਲੀ V3Os::selfTest() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
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
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_ਰਸਤਾ("a/b/x/y", "a/q"), "../b/x/y");
#ਜੇ_ਅੰਤ
}
