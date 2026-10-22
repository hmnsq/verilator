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
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
# ifndef PSAPI_ಆವೃತ್ತಿ
#  define PSAPI_ಆವೃತ್ತಿ 1  // Needed for compatibility with Windows 7
# endif
#ಆದರೆ_ಅಂತ್ಯ
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ __MINGW32__
# define MINGW_HAS_SECURE_API 1  // Needed to expose a "secure" POSIX-like API
#ಆದರೆ_ಅಂತ್ಯ
// clang-format on

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
#ಅನ್ತರ್ಭಾವ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ <cerrno>
#ಅನ್ತರ್ಭಾವ <climits>  // PATH_MAX (especially on FreeBSD)
#ಅನ್ತರ್ಭಾವ <cstdarg>
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ _MSC_VER
#ಅನ್ತರ್ಭಾವ <filesystem>  // C++17
#ವ್ಯಾಖ್ಯಾನಿಸು ಮಾರ್ಗ_MAX MAX_ಮಾರ್ಗ
#ಇಲ್ಲದಿದ್ದರೆ
#ಅನ್ತರ್ಭಾವ <dirent.h>
#ಅನ್ತರ್ಭಾವ <utime.h>
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ <fcntl.h>
#ಅನ್ತರ್ಭಾವ <fstream>
#ಅನ್ತರ್ಭಾವ <memory>

#ಅನ್ತರ್ಭಾವ <sys/stat.h>
#ಅನ್ತರ್ಭಾವ <sys/types.h>

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_TCMALLOC
#ಅನ್ತರ್ಭಾವ <gperftools/malloc_extension.h>
#ಆದರೆ_ಅಂತ್ಯ
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_JEMALLOC
#ಅನ್ತರ್ಭಾವ <jemalloc/jemalloc.h>
#ಆದರೆ_ಅಂತ್ಯ

// clang-format off
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
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
#  ಇಲ್ಲದಿದ್ದರೆ
#   define WIFEXITED(w) (((w) & 0377) == 0)
#   define WEXITSTATUS(w) (((w) >> 8) & 0377)
#  endif
# endif
#ಇಲ್ಲದಿದ್ದರೆ
# include <sys/time.h>
# ifndef __wasi__
#  include <sys/wait.h>  // Needed on FreeBSD for WIFEXITED
# endif
# include <unistd.h>  // usleep
#ಆದರೆ_ಅಂತ್ಯ
// clang-format on

#ವ್ಯಾಖ್ಯಾನಿಸು VL_ALLOW_VERILATEDOS_C
#ಅನ್ತರ್ಭಾವ "verilatedos_c.h"

//######################################################################
// Environment

string V3Os::getenvStr(ಸ್ಥಿರಾಂಕ string& envvar, ಸ್ಥಿರಾಂಕ string& default_ಮೌಲ್ಯ) {
    ಹಿಂತಿರುಗಿ VString::escape_String_For_ಮಾರ್ಗ(VlOs::getenvStr(envvar, default_ಮೌಲ್ಯ));
}

ಶೂನ್ಯ V3Os::setenvStr(ಸ್ಥಿರಾಂಕ string& envvar, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ, ಸ್ಥಿರಾಂಕ string& why) {
    ಆದರೆ (why != "") {
        UINFO(1, "export " << envvar << "='" << ಮೌಲ್ಯ << "'  # " << why);
    } ಇಲ್ಲದಿದ್ದರೆ {
        UINFO(1, "export " << envvar << "='" << ಮೌಲ್ಯ << "'");
    }
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
    _putenv_s(envvar.c_str(), ಮೌಲ್ಯ.c_str());
#ಅಥವಾಆದರೆ defined(BSD_ಮೂಲ) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
    setenv(envvar.c_str(), ಮೌಲ್ಯ.c_str(), ನಿಜ);
#ಇಲ್ಲದಿದ್ದರೆ
    // setenv() replaced by putenv() in Solaris environment. Prototype is different
    // putenv() requires NAME=VALUE format
    ಸ್ಥಿರಾಂಕ string vareq = envvar + "=" + ಮೌಲ್ಯ;
    putenv(strdup(vareq.c_str()));  // will leak if setting the same variable again
#ಆದರೆ_ಅಂತ್ಯ
}

//######################################################################
// Generic filename utilities

#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
ಸ್ಥಿರ constexpr ಅಕ್ಷರ V3OS_SLASH = '\\';
#ಇಲ್ಲದಿದ್ದರೆ
ಸ್ಥಿರ constexpr ಅಕ್ಷರ V3OS_SLASH = '/';
#ಆದರೆ_ಅಂತ್ಯ

ಸ್ಥಿರ ತಾರ್ಕಿಕ isSlash(ಅಕ್ಷರ ch) VL_ಶುದ್ಧ {
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
    ಹಿಂತಿರುಗಿ ch == '/' || ch == '\\';
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ ch == '/';
#ಆದರೆ_ಅಂತ್ಯ
}

string V3Os::filenameCleanup(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ {
    string str;
    str.reserve(filename.ಉದ್ದಳತೆ());
    ತಾರ್ಕಿಕ ಕೊನೆಯ_Is_Slash = ಸುಳ್ಳು;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ ch : filename) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಕೊನೆಯ_Is_Slash_Old = ಕೊನೆಯ_Is_Slash;
        ಕೊನೆಯ_Is_Slash = isSlash(ch);
        ಆದರೆ (ಕೊನೆಯ_Is_Slash && ಕೊನೆಯ_Is_Slash_Old) ಮುಂದುವರಿ;
        str += ch;
    }
    ಆದರೆ (str.ಗಾತ್ರ_2() > 1 && isSlash(str.ಹಿಂಭಾಗ())) str.ತೆಗೆ_ಹಿಂಭಾಗ();
    ಆಗ (str.ಗಾತ್ರ_2() > 2 && str[0] == '.' && isSlash(str[1])) str.erase(0, 2);
    ಹಿಂತಿರುಗಿ str;
}

string V3Os::filenameJoin(std::initializer_list<ಸ್ಥಿರಾಂಕ std::string> paths) VL_ಶುದ್ಧ {
    string fullpath;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಅಂಶ_2 : paths) {
        ಆದರೆ (ಅಂಶ_2.ಖಾಲಿಗಳು() || ಅಂಶ_2 == ".") ಮುಂದುವರಿ;
        ಆದರೆ (!fullpath.ಖಾಲಿಗಳು()) fullpath += V3OS_SLASH;
        fullpath += ಅಂಶ_2;
    }
    ಹಿಂತಿರುಗಿ fullpath;
}

string V3Os::filenameDir(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ {
    // std::filesystem::path::parent_path
    ಸ್ವಯಂಚಾಲಿತ it = filename.rbegin();
    ಗಾಗಿ (; it != filename.rend(); ++it) {
        ಆದರೆ (isSlash(*it)) ನಿಲ್ಲಿಸು;
    }
    ಆದರೆ (it.ತಳ() == filename.begin()) ಹಿಂತಿರುಗಿ ".";
    ಹಿಂತಿರುಗಿ string{filename.begin(), (++it).ತಳ()};
}

string V3Os::filenameExt(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ {
    string ತಳ = filenameNonDir(filename);
    ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type pos = ತಳ.ಕಂಡುಹಿಡಿ('.');
    ಆದರೆ (pos != string::npos) {
        ತಳ.erase(0, pos);
        ಹಿಂತಿರುಗಿ ತಳ;
    }
    ಹಿಂತಿರುಗಿ "";
}

string V3Os::filenameNonDir(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ {
    // std::filesystem::path::filename
    ಸ್ವಯಂಚಾಲಿತ it = filename.rbegin();
    ಗಾಗಿ (; it != filename.rend(); ++it) {
        ಆದರೆ (isSlash(*it)) ನಿಲ್ಲಿಸು;
    }
    ಹಿಂತಿರುಗಿ string{it.ತಳ(), filename.ಕೊನೆ()};
}

string V3Os::filenameNonDirExt(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ {
    string ತಳ = filenameNonDir(filename);
    ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type pos = ತಳ.ಕಂಡುಹಿಡಿ('.');
    ಆದರೆ (pos != string::npos) ತಳ.erase(pos);
    ಹಿಂತಿರುಗಿ ತಳ;
}

string V3Os::filenameSubstitute(ಸ್ಥಿರಾಂಕ string& filename) {
    string ಫಲಿತಾಂಶ;
    // cppcheck-has-bug-suppress unusedLabel
    ಎಣಿಕೆ : uint8_t { NONE, PAREN, CURLY } brackets = NONE;
    ಗಾಗಿ (string::ಗಾತ್ರ_type pos = 0; pos < filename.ಉದ್ದಳತೆ(); ++pos) {
        ಆದರೆ ((filename[pos] == '$') && (pos + 1 < filename.ಉದ್ದಳತೆ())) {
            ಆಯ್ಕೆ (filename[pos + 1]) {
            ಸ್ಥಿತಿ '{': brackets = CURLY; ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ '(': brackets = PAREN; ನಿಲ್ಲಿಸು;
            ಪೂರ್ವನಿಯೋಜಿತ: brackets = NONE; ನಿಲ್ಲಿಸು;
            }
            ಆದರೆ (brackets != NONE) pos = pos + 1;
            string::ಗಾತ್ರ_type endpos = pos + 1;
            ಆಗ (((endpos + 1) < filename.ಉದ್ದಳತೆ())
                   && (((brackets == NONE)
                        && (std::ಅಕ್ಷರವೇ_ಅಂಕಿಯೇ(filename[endpos + 1]) || filename[endpos + 1] == '_'))
                       || ((brackets == CURLY) && (filename[endpos + 1] != '}'))
                       || ((brackets == PAREN) && (filename[endpos + 1] != ')'))))
                ++endpos;
            // Catch bracket errors
            ಆದರೆ (((brackets == CURLY) && (filename[endpos + 1] != '}'))
                || ((brackets == PAREN) && (filename[endpos + 1] != ')'))) {
                v3fatal("Unmatched brackets in variable substitution in file: " + filename);
            }
            ಸ್ಥಿರಾಂಕ string envvar = filename.substr(pos + 1, endpos - pos);
            string envvalue;
            ಆದರೆ (!envvar.ಖಾಲಿಗಳು()) envvalue = getenvStr(envvar, "");
            ಆದರೆ (!envvalue.ಖಾಲಿಗಳು()) {
                ಫಲಿತಾಂಶ += envvalue;
                ಆದರೆ (brackets == NONE) {
                    pos = endpos;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    pos = endpos + 1;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಫಲಿತಾಂಶ += filename[pos];  // *pos == '$'
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಫಲಿತಾಂಶ += filename[pos];
        }
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string V3Os::filename_Real_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ {
    // Get rid of all the ../ behavior in the middle of the paths.
    // If there is a ../ that goes down from the 'root' of this path it is preserved.
    ಅಕ್ಷರ retpath[ಮಾರ್ಗ_MAX];
    ಆದರೆ (
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
        _fullpath(retpath, filename.c_str(), ಮಾರ್ಗ_MAX)
#ಇಲ್ಲದಿದ್ದರೆ
        realpath(filename.c_str(), retpath)
#ಆದರೆ_ಅಂತ್ಯ
    ) {
        ಹಿಂತಿರುಗಿ std::string{retpath};
    }
    ಹಿಂತಿರುಗಿ filename;
}

string V3Os::filename_Absolute_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ {
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
    // _fullpath does not resolve symlinks
    ಹಿಂತಿರುಗಿ filename_Real_ಮಾರ್ಗ(filename);
#ಇಲ್ಲದಿದ್ದರೆ
    ಆದರೆ (!filenameIsRel(filename)) ಹಿಂತಿರುಗಿ filename;
    ಹಿಂತಿರುಗಿ filenameJoin(filename_Real_ಮಾರ್ಗ("."), filename);
#ಆದರೆ_ಅಂತ್ಯ
}

string V3Os::filename_Relative_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& ತಳ) VL_ಶುದ್ಧ {
    ಸ್ಥಿರಾಂಕ string a = V3Os::filename_Real_ಮಾರ್ಗ(filename);
    ಸ್ಥಿರಾಂಕ string b = V3Os::filename_Real_ಮಾರ್ಗ(ತಳ);
    string ಫಲಿತಾಂಶ;
    ಆದರೆ (a == b) ಹಿಂತಿರುಗಿ ".";

    ಸ್ವಯಂಚಾಲಿತ aIt = a.begin();
    ಸ್ವಯಂಚಾಲಿತ bIt = b.begin();
    ಆಗ (aIt != a.ಕೊನೆ() && bIt != b.ಕೊನೆ()) {
        // UINFO(9, "fnrp scan " << (aIt - a.begin()) << " " << a.substr(aIt - a.begin()));
        // UINFO(9, "fnrp scan " << (bIt - b.begin()) << " " << b.substr(bIt - b.begin()));
        ಸ್ವಯಂಚಾಲಿತ a_ಶಬ್ದ_It = aIt;  // position of next slash
        ಗಾಗಿ (; a_ಶಬ್ದ_It != a.ಕೊನೆ(); ++a_ಶಬ್ದ_It) {
            ಆದರೆ (isSlash(*a_ಶಬ್ದ_It)) ನಿಲ್ಲಿಸು;
        }

        ಸ್ವಯಂಚಾಲಿತ b_ಶಬ್ದ_It = bIt;  // position of next slash
        ಗಾಗಿ (; b_ಶಬ್ದ_It != b.ಕೊನೆ(); ++b_ಶಬ್ದ_It) {
            ಆದರೆ (isSlash(*b_ಶಬ್ದ_It)) ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿರಾಂಕ string a_ಶಬ್ದ = a.substr(aIt - a.begin(), a_ಶಬ್ದ_It - aIt);
        ಸ್ಥಿರಾಂಕ string b_ಶಬ್ದ = b.substr(bIt - b.begin(), b_ಶಬ್ದ_It - bIt);
        ಆದರೆ (a_ಶಬ್ದ != b_ಶಬ್ದ) ನಿಲ್ಲಿಸು;
        aIt = a_ಶಬ್ದ_It;
        bIt = b_ಶಬ್ದ_It;
        ಆದರೆ (aIt != a.ಕೊನೆ()) ++aIt;  // Skip slash
        ಆದರೆ (bIt != b.ಕೊನೆ()) ++bIt;  // Skip slash
    }

    ಆಗ (bIt != b.ಕೊನೆ()) {
        ಗಾಗಿ (; bIt != b.ಕೊನೆ(); ++bIt) {
            ಆದರೆ (isSlash(*bIt)) {
                ++bIt;
                ನಿಲ್ಲಿಸು;
            }
        }
        ಆದರೆ (!ಫಲಿತಾಂಶ.ಖಾಲಿಗಳು()) ಫಲಿತಾಂಶ += "/";
        ಫಲಿತಾಂಶ += "..";
    }

    ಸ್ಥಿರಾಂಕ string a_ಎಡ = a.substr(aIt - a.begin());
    ಆದರೆ (!a_ಎಡ.ಖಾಲಿಗಳು()) {
        ಆದರೆ (!ಫಲಿತಾಂಶ.ಖಾಲಿಗಳು()) ಫಲಿತಾಂಶ += "/";
        ಫಲಿತಾಂಶ += a_ಎಡ;
    }
    ಹಿಂತಿರುಗಿ filenameCleanup(ಫಲಿತಾಂಶ);
}

ತಾರ್ಕಿಕ V3Os::filenameIsRel(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ {
#ಆದರೆ defined(_MSC_VER)
    ಹಿಂತಿರುಗಿ std::filesystem::ಮಾರ್ಗ(filename).is_relative();
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ (filename.ಉದ್ದಳತೆ() > 0 && filename[0] != '/');
#ಆದರೆ_ಅಂತ್ಯ
}

string V3Os::filename_Slash_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ {
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
    string slashed_ಮಾರ್ಗ = filename;
    std::replace(slashed_ಮಾರ್ಗ.begin(), slashed_ಮಾರ್ಗ.ಕೊನೆ(), '\\', '/');
    ಹಿಂತಿರುಗಿ slashed_ಮಾರ್ಗ;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ filename;
#ಆದರೆ_ಅಂತ್ಯ
}

//######################################################################
// File utilities

string V3Os::getline(std::istream& is, ಅಕ್ಷರ delim) {
    string ಸಾಲು;
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ __CYGWIN__  // Work around buggy implementation of getline
    ಅಕ್ಷರ buf[65536];
    is.getline(buf, 65535, delim);
    ಸಾಲು = buf;
#ಇಲ್ಲದಿದ್ದರೆ
    std::getline(is, ಸಾಲು, delim);
#ಆದರೆ_ಅಂತ್ಯ
    ಹಿಂತಿರುಗಿ ಸಾಲು;
}

//######################################################################
// Directory utilities

ಶೂನ್ಯ V3Os::createDir(ಸ್ಥಿರಾಂಕ string& dirname) {
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
    _mkdir(dirname.c_str());
#ಇಲ್ಲದಿದ್ದರೆ
    mkdir(dirname.c_str(), 0777);
#ಆದರೆ_ಅಂತ್ಯ
}

ಶೂನ್ಯ V3Os::filesystemFlush(ಸ್ಥಿರಾಂಕ string& dirname) {
    // NFS caches stat() calls so to get up-to-date information must
    // do a open or opendir on the filename.
#ಆದರೆ defined(_MSC_VER) || defined(__MINGW32__)
    ಆದರೆ (ಪೂರ್ಣಾಂಕ fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ಆದರೆ (fd > 0) ::close(fd);
    }
#ಇಲ್ಲದಿದ್ದರೆ
    // Faster to just try both rather than check if a file is a dir.
    ಆದರೆ (DIR* ಸ್ಥಿರಾಂಕ dirp = opendir(dirname.c_str())) {  // LCOV_EXCL_BR_LINE
        closedir(dirp);  // LCOV_EXCL_LINE
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ fd = ::open(dirname.c_str(), O_RDONLY)) {  // LCOV_EXCL_BR_LINE
        ಆದರೆ (fd > 0) ::close(fd);
    }
#ಆದರೆ_ಅಂತ್ಯ
}

ಶೂನ್ಯ V3Os::filesystemFlushBuildDir(ಸ್ಥಿರಾಂಕ string& dirname) {
    // Attempt to force out written directory, for NFS like file systems.
#ಆದರೆ !defined(_MSC_VER) && !defined(__MINGW32__)
    // Linux kernel may not reread from NFS unless timestamp modified
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ err = utimes(dirname.c_str(), ಶೂನ್ಯಸೂಚಕ);
    // Not an error
    ಆದರೆ (err != 0) UINFO(1, "-Info: File not utimed: " << dirname);
#ಆದರೆ_ಅಂತ್ಯ
    filesystemFlush(dirname);
}

ಶೂನ್ಯ V3Os::unlinkRegexp(ಸ್ಥಿರಾಂಕ string& dir, ಸ್ಥಿರಾಂಕ string& regexp) {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ _MSC_VER
    ಪ್ರಯತ್ನಿಸು {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& dir_ನಮೂದು : std::filesystem::directory_iterator(dir.c_str())) {
            ಆದರೆ (VString::wildmatch(dir_ನಮೂದು.ಮಾರ್ಗ().filename().string(), regexp.c_str())) {
                ಸ್ಥಿರಾಂಕ string fullname = dir + "/" + dir_ನಮೂದು.ಮಾರ್ಗ().filename().string();
                _unlink(fullname.c_str());
            }
        }
    } ಹಿಡಿ (std::filesystem::filesystem_error ಸ್ಥಿರಾಂಕ& ex) {}
#ಇಲ್ಲದಿದ್ದರೆ
    ಆದರೆ (DIR* ಸ್ಥಿರಾಂಕ dirp = opendir(dir.c_str())) {
        ಆಗ (ರಚನೆ dirent* ಸ್ಥಿರಾಂಕ direntp = readdir(dirp)) {
            ಆದರೆ (VString::wildmatch(direntp->d_name, regexp.c_str())) {
                ಸ್ಥಿರಾಂಕ string fullname = dir + "/" + std::string{direntp->d_name};
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
                _unlink(fullname.c_str());
#ಇಲ್ಲದಿದ್ದರೆ
                unlink(fullname.c_str());
#ಆದರೆ_ಅಂತ್ಯ
            }
        }
        closedir(dirp);
    }
#ಆದರೆ_ಅಂತ್ಯ
}

//######################################################################
// METHODS (memory)

ಶೂನ್ಯ V3Os::release_ಸ್ಮರಣೆ() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_TCMALLOC
    MallocExtension::instance()->Release_Free_ಸ್ಮರಣೆ();
#ಆದರೆ_ಅಂತ್ಯ
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_JEMALLOC
    // Purge all unused dirty pages across all arenas
    ಚಿಹ್ನೆರಹಿತ narenas = 0;
    ಗಾತ್ರ_t sz = ಗಾತ್ರ(narenas);
    ಆದರೆ (mallctl("arenas.narenas", &narenas, &sz, ಶೂನ್ಯಸೂಚಕ, 0)) {
        ಹಿಂತಿರುಗಿ;  // Failed to get number of arenas, give up
    }
    ಅಕ್ಷರ buf[64];
    // Index equal to narenas represents all arenas
    (ಶೂನ್ಯ)VL_SNPRINTF(buf, ಗಾತ್ರ(buf), "arena.%u.purge", narenas);
    mallctl(buf, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, 0);
#ಆದರೆ_ಅಂತ್ಯ
}

//######################################################################
// METHODS (random)

uint64_t V3Os::rand64(std::array<uint64_t, 2>& stater) {
    // Xoroshiro128+ algorithm
    ಸ್ಥಿರಾಂಕ uint64_t ಫಲಿತಾಂಶ = stater[0] + stater[1];
    stater[1] ^= stater[0];
    stater[0] = (((stater[0] << 55) | (stater[0] >> 9)) ^ stater[1] ^ (stater[1] << 14));
    stater[1] = (stater[1] << 36) | (stater[1] >> 28);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string V3Os::trueRandom(ಗಾತ್ರ_t ಗಾತ್ರ_2) VL_MT_ಸುರಕ್ಷಿತ {
    string ಫಲಿತಾಂಶ(ಗಾತ್ರ_2, '\xFF');
    ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ದತ್ತಾಂಶ = const_cast<ಅಕ್ಷರ*>(ಫಲಿತಾಂಶ.ದತ್ತಾಂಶ());
    // Note: std::string.data() returns a non-const Char* from C++17 onwards.
    // For pre-C++17, this cast is OK in practice, even though it's UB.
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
    ಸ್ಥಿರಾಂಕ NTSTATUS hr = BCryptGenRandom(ಶೂನ್ಯಸೂಚಕ, reinterpret_cast<BYTE*>(ದತ್ತಾಂಶ), ಗಾತ್ರ_2,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ಆದರೆ (VL_UNCOVERABLE(!BCRYPT_SUCCESS(hr))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#ಅಥವಾಆದರೆ defined(__wasi__)
    // WASI has no /dev/urandom. getentropy() rejects sizes over 256 bytes,
    // which is more than any caller asks for, and errors rather than truncates.
    ಆದರೆ (VL_UNCOVERABLE(getentropy(ದತ್ತಾಂಶ, ಗಾತ್ರ_2))) {
        v3fatal("Could not acquire random data. Try specifying a key instead.");  // LCOV_EXCL_LINE
    }
#ಇಲ್ಲದಿದ್ದರೆ
    std::ifstream is{"/dev/urandom", std::ios::ಒಳ | std::ios::ದ್ವಿಮಾನ};
    // This read uses the size of the buffer.
    // Flawfinder: ignore
    ಆದರೆ (VL_UNCOVERABLE(!is.ಪಠ(ದತ್ತಾಂಶ, ಗಾತ್ರ_2))) {
        v3fatal("Could not open /dev/urandom, no source of randomness. "  // LCOV_EXCL_LINE
                "Try specifying a key instead.");
    }
#ಆದರೆ_ಅಂತ್ಯ
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

//######################################################################
// METHODS (performance)

uint64_t V3Os::ಸಮಯ_Usecs() {
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
    // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
    ಸ್ಥಿರ constexpr uint64_t EPOCH_DIFFERENCE_USECS = 11644473600000000ULL;

    FILETIME ft;  // contains number of 0.1us intervals since the beginning of 1601 UTC.
    Get_System_ಸಮಯ_As_File_ಸಮಯ(&ft);
    ಸ್ಥಿರಾಂಕ uint64_t us
        = ((static_cast<uint64_t>(ft.dw_ಮೇಲಿನ_Date_ಸಮಯ) << 32) + ft.dw_ಕೆಳಗಿನ_Date_ಸಮಯ + 5ULL) / 10ULL;
    ಹಿಂತಿರುಗಿ us - EPOCH_DIFFERENCE_USECS;
#ಇಲ್ಲದಿದ್ದರೆ
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    timeval tv;
    ಆದರೆ (gettimeofday(&tv, ಶೂನ್ಯಸೂಚಕ) < 0) ಹಿಂತಿರುಗಿ 0;
    ಹಿಂತಿರುಗಿ static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#ಆದರೆ_ಅಂತ್ಯ
}

ಶೂನ್ಯ V3Os::u_sleep(int64_t usec) {
#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
#ಇಲ್ಲದಿದ್ದರೆ
    // cppcheck-suppress obsoleteFunctionsusleep
    // Flawfinder: ignore
    ::usleep(usec);
#ಆದರೆ_ಅಂತ್ಯ
}

//######################################################################
// METHODS (sub command)

ಪೂರ್ಣಾಂಕ V3Os::system(ಸ್ಥಿರಾಂಕ string& command) {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ __wasi__
    // WASI has no process spawning
    v3fatal("Running subcommands is not supported on this platform: " << command);
    ಹಿಂತಿರುಗಿ -1;
#ಇಲ್ಲದಿದ್ದರೆ
    UINFO(1, "Running system: " << command);
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ret = ::system(command.c_str());
    ಆದರೆ (VL_UNCOVERABLE(ret == -1)) {
        v3fatal("Failed to execute command:"  // LCOV_EXCL_LINE
                << command << " " << std::strerror(errno));  // LCOV_EXCL_LINE
        ಹಿಂತಿರುಗಿ -1;  // LCOV_EXCL_LINE
    }
    UASSERT(WIFEXITED(ret), "system(" << command << ") returned unexpected value of " << ret);
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ exit_ಸಂಕೇತ = WEXITSTATUS(ret);
    UINFO(1, command << " returned exit code of " << exit_ಸಂಕೇತ);
    UASSERT(exit_ಸಂಕೇತ >= 0, "exit code must not be negative");
    ಹಿಂತಿರುಗಿ exit_ಸಂಕೇತ;
#ಆದರೆ_ಅಂತ್ಯ
}

ಶೂನ್ಯ V3Os::selfTest() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
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
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("/a/b", "/a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("/a/b", "/a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("/a/b", "/a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("/a/b/x", "/a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("/a/b/x/y", "/"), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("/a/b/x/y", "/a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("/a/b/x/y", "/a/q"), "../b/x/y");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("a/b", "a/b"), ".");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("a/b", "a/b/c"), "..");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("a/b", "a/b/c/d"), "../..");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("a/b/x", "a/b/c/d"), "../../x");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("a/b/x/y", ""), "a/b/x/y");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("a/b/x/y", "a/b"), "x/y");
    UASSERT_SELFTEST(filename_Relative_ಮಾರ್ಗ("a/b/x/y", "a/q"), "../b/x/y");
#ಆದರೆ_ಅಂತ್ಯ
}
