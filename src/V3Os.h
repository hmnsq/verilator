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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ <array>

// Limited V3 headers here - this is a base class for Vlc etc
#અન્તર્ભાવ "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
જાહેર:
    // METHODS (environment)
    સ્થિર string cwd() { પાછા filenameRealPath("."); }
    સ્થિર string getenvStr(અચલ string& envvar, અચલ string& default_કિંમત);
    સ્થિર શૂન્ય setenvStr(અચલ string& envvar, અચલ string& કિંમત, અચલ string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    સ્થિર string filenameJoin(std::initializer_list<અચલ std::string> paths) VL_PURE;
    template <typename... Args>
    સ્થિર string filenameJoin(Args... args) VL_PURE {
        પાછા filenameJoin({args...});
    };
    ///< @return file path without repeated separators and ./ prefix
    સ્થિર string filenameCleanup(અચલ string& filename) VL_PURE;
    ///< @return extension (.) part of filename
    સ્થિર string filenameExt(અચલ string& filename) VL_PURE;
    ///< @return non-directory part of filename
    સ્થિર string filenameNonDir(અચલ string& filename) VL_PURE;
    ///< @return basename of filename
    સ્થિર string filenameNonDirExt(અચલ string& filename) VL_PURE;
    ///< @return directory part of filename
    સ્થિર string filenameDir(અચલ string& filename) VL_PURE;
    ///< @return filename with env vars removed
    સ્થિર string filenameSubstitute(અચલ string& filename);
    ///< @return realpath of filename
    સ્થિર string filenameRealPath(અચલ string& filename) VL_PURE;
    // @return absolute path of filename, without resolving symlinks
    સ્થિર string filenameAbsolutePath(અચલ string& filename) VL_PURE;
    ///< @return relative path of filename, relative to base
    સ્થિર string filenameRelativePath(અચલ string& filename, અચલ string& base) VL_PURE;
    ///< @return filename is relative
    સ્થિર bool filenameIsRel(અચલ string& filename) VL_PURE;
    ///< @return path slashed with '/'
    સ્થિર string filenameSlashPath(અચલ string& filename) VL_PURE;

    // METHODS (file utilities)
    સ્થિર string getline(std::istream& is, અક્ષર delim = '\n');

    // METHODS (directory utilities)
    સ્થિર શૂન્ય createDir(અચલ string& dirname);
    સ્થિર શૂન્ય filesystemFlush(અચલ string& dirname);
    સ્થિર શૂન્ય filesystemFlushBuildDir(અચલ string& dirname);
    સ્થિર શૂન્ય unlinkRegexp(અચલ string& dir, અચલ string& regexp);

    // METHODS (memory)
    સ્થિર શૂન્ય releaseMemory();

    // METHODS (random)
    સ્થિર uint64_t rand64(std::array<uint64_t, 2>& stater);
    સ્થિર string trueRandom(size_t size) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    સ્થિર શૂન્ય u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    સ્થિર uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    સ્થિર પૂર્ણાંક system(અચલ string& command);
    સ્થિર શૂન્ય selfTest();
};

#endif  // Guard
