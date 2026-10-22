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

#ifndef VERILATOR_V3OS_H_
#define VERILATOR_V3OS_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ <array>

// Limited V3 headers here - this is a base class for Vlc etc
#അന്തര്ഭാവ "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
പൊതു:
    // METHODS (environment)
    സ്ഥിതം string cwd() { തിരികെ filenameRealPath("."); }
    സ്ഥിതം string getenvStr(സ്ഥിരം string& envvar, സ്ഥിരം string& default_മൂല്യം);
    സ്ഥിതം ശൂന്യം setenvStr(സ്ഥിരം string& envvar, സ്ഥിരം string& മൂല്യം, സ്ഥിരം string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    സ്ഥിതം string filenameJoin(std::initializer_list<സ്ഥിരം std::string> paths) VL_PURE;
    template <typename... Args>
    സ്ഥിതം string filenameJoin(Args... args) VL_PURE {
        തിരികെ filenameJoin({args...});
    };
    ///< @return file path without repeated separators and ./ prefix
    സ്ഥിതം string filenameCleanup(സ്ഥിരം string& filename) VL_PURE;
    ///< @return extension (.) part of filename
    സ്ഥിതം string filenameExt(സ്ഥിരം string& filename) VL_PURE;
    ///< @return non-directory part of filename
    സ്ഥിതം string filenameNonDir(സ്ഥിരം string& filename) VL_PURE;
    ///< @return basename of filename
    സ്ഥിതം string filenameNonDirExt(സ്ഥിരം string& filename) VL_PURE;
    ///< @return directory part of filename
    സ്ഥിതം string filenameDir(സ്ഥിരം string& filename) VL_PURE;
    ///< @return filename with env vars removed
    സ്ഥിതം string filenameSubstitute(സ്ഥിരം string& filename);
    ///< @return realpath of filename
    സ്ഥിതം string filenameRealPath(സ്ഥിരം string& filename) VL_PURE;
    // @return absolute path of filename, without resolving symlinks
    സ്ഥിതം string filenameAbsolutePath(സ്ഥിരം string& filename) VL_PURE;
    ///< @return relative path of filename, relative to base
    സ്ഥിതം string filenameRelativePath(സ്ഥിരം string& filename, സ്ഥിരം string& base) VL_PURE;
    ///< @return filename is relative
    സ്ഥിതം bool filenameIsRel(സ്ഥിരം string& filename) VL_PURE;
    ///< @return path slashed with '/'
    സ്ഥിതം string filenameSlashPath(സ്ഥിരം string& filename) VL_PURE;

    // METHODS (file utilities)
    സ്ഥിതം string getline(std::istream& is, അക്ഷരം delim = '\n');

    // METHODS (directory utilities)
    സ്ഥിതം ശൂന്യം createDir(സ്ഥിരം string& dirname);
    സ്ഥിതം ശൂന്യം filesystemFlush(സ്ഥിരം string& dirname);
    സ്ഥിതം ശൂന്യം filesystemFlushBuildDir(സ്ഥിരം string& dirname);
    സ്ഥിതം ശൂന്യം unlinkRegexp(സ്ഥിരം string& dir, സ്ഥിരം string& regexp);

    // METHODS (memory)
    സ്ഥിതം ശൂന്യം releaseMemory();

    // METHODS (random)
    സ്ഥിതം uint64_t rand64(std::array<uint64_t, 2>& stater);
    സ്ഥിതം string trueRandom(size_t size) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    സ്ഥിതം ശൂന്യം u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    സ്ഥിതം uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    സ്ഥിതം പൂർണ്ണസംഖ്യ system(സ്ഥിരം string& command);
    സ്ഥിതം ശൂന്യം selfTest();
};

#endif  // Guard
