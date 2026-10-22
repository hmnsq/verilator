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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <array>

// Limited V3 headers here - this is a base class for Vlc etc
#ಅನ್ತರ್ಭಾವ "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
ಸಾರ್ವಜನಿಕ:
    // METHODS (environment)
    ಸ್ಥಿರ string cwd() { ಪ್ರತ್ಯಾಗಮ filenameRealPath("."); }
    ಸ್ಥಿರ string getenvStr(ಧ್ರುವ string& envvar, ಧ್ರುವ string& default_ಮೌಲ್ಯ);
    ಸ್ಥಿರ ಶೂನ್ಯ setenvStr(ಧ್ರುವ string& envvar, ಧ್ರುವ string& ಮೌಲ್ಯ, ಧ್ರುವ string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    ಸ್ಥಿರ string filenameJoin(std::initializer_list<ಧ್ರುವ std::string> paths) VL_PURE;
    template <typename... Args>
    ಸ್ಥಿರ string filenameJoin(Args... args) VL_PURE {
        ಪ್ರತ್ಯಾಗಮ filenameJoin({args...});
    };
    ///< @return file path without repeated separators and ./ prefix
    ಸ್ಥಿರ string filenameCleanup(ಧ್ರುವ string& filename) VL_PURE;
    ///< @return extension (.) part of filename
    ಸ್ಥಿರ string filenameExt(ಧ್ರುವ string& filename) VL_PURE;
    ///< @return non-directory part of filename
    ಸ್ಥಿರ string filenameNonDir(ಧ್ರುವ string& filename) VL_PURE;
    ///< @return basename of filename
    ಸ್ಥಿರ string filenameNonDirExt(ಧ್ರುವ string& filename) VL_PURE;
    ///< @return directory part of filename
    ಸ್ಥಿರ string filenameDir(ಧ್ರುವ string& filename) VL_PURE;
    ///< @return filename with env vars removed
    ಸ್ಥಿರ string filenameSubstitute(ಧ್ರುವ string& filename);
    ///< @return realpath of filename
    ಸ್ಥಿರ string filenameRealPath(ಧ್ರುವ string& filename) VL_PURE;
    // @return absolute path of filename, without resolving symlinks
    ಸ್ಥಿರ string filenameAbsolutePath(ಧ್ರುವ string& filename) VL_PURE;
    ///< @return relative path of filename, relative to base
    ಸ್ಥಿರ string filenameRelativePath(ಧ್ರುವ string& filename, ಧ್ರುವ string& base) VL_PURE;
    ///< @return filename is relative
    ಸ್ಥಿರ bool filenameIsRel(ಧ್ರುವ string& filename) VL_PURE;
    ///< @return path slashed with '/'
    ಸ್ಥಿರ string filenameSlashPath(ಧ್ರುವ string& filename) VL_PURE;

    // METHODS (file utilities)
    ಸ್ಥಿರ string getline(std::istream& is, ವರ್ಣ delim = '\n');

    // METHODS (directory utilities)
    ಸ್ಥಿರ ಶೂನ್ಯ createDir(ಧ್ರುವ string& dirname);
    ಸ್ಥಿರ ಶೂನ್ಯ filesystemFlush(ಧ್ರುವ string& dirname);
    ಸ್ಥಿರ ಶೂನ್ಯ filesystemFlushBuildDir(ಧ್ರುವ string& dirname);
    ಸ್ಥಿರ ಶೂನ್ಯ unlinkRegexp(ಧ್ರುವ string& dir, ಧ್ರುವ string& regexp);

    // METHODS (memory)
    ಸ್ಥಿರ ಶೂನ್ಯ releaseMemory();

    // METHODS (random)
    ಸ್ಥಿರ uint64_t rand64(std::array<uint64_t, 2>& stater);
    ಸ್ಥಿರ string trueRandom(size_t size) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    ಸ್ಥಿರ ಶೂನ್ಯ u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    ಸ್ಥಿರ uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ system(ಧ್ರುವ string& command);
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
};

#endif  // Guard
