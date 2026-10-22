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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3OS_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3OS_H_

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
    ಸ್ಥಿರ string cwd() { ಹಿಂತಿರುಗಿ filename_Real_ಮಾರ್ಗ("."); }
    ಸ್ಥಿರ string getenvStr(ಸ್ಥಿರಾಂಕ string& envvar, ಸ್ಥಿರಾಂಕ string& default_ಮೌಲ್ಯ);
    ಸ್ಥಿರ ಶೂನ್ಯ setenvStr(ಸ್ಥಿರಾಂಕ string& envvar, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ, ಸ್ಥಿರಾಂಕ string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    ಸ್ಥಿರ string filenameJoin(std::initializer_list<ಸ್ಥಿರಾಂಕ std::string> paths) VL_ಶುದ್ಧ;
    ಮಾದರಿ <typename... ವಾದಗಳು_2>
    ಸ್ಥಿರ string filenameJoin(ವಾದಗಳು_2... ವಾದಗಳು) VL_ಶುದ್ಧ {
        ಹಿಂತಿರುಗಿ filenameJoin({ವಾದಗಳು...});
    };
    ///< @return file path without repeated separators and ./ prefix
    ಸ್ಥಿರ string filenameCleanup(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ;
    ///< @return extension (.) part of filename
    ಸ್ಥಿರ string filenameExt(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ;
    ///< @return non-directory part of filename
    ಸ್ಥಿರ string filenameNonDir(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ;
    ///< @return basename of filename
    ಸ್ಥಿರ string filenameNonDirExt(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ;
    ///< @return directory part of filename
    ಸ್ಥಿರ string filenameDir(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ;
    ///< @return filename with env vars removed
    ಸ್ಥಿರ string filenameSubstitute(ಸ್ಥಿರಾಂಕ string& filename);
    ///< @return realpath of filename
    ಸ್ಥಿರ string filename_Real_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ;
    // @return absolute path of filename, without resolving symlinks
    ಸ್ಥಿರ string filename_Absolute_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ;
    ///< @return relative path of filename, relative to base
    ಸ್ಥಿರ string filename_Relative_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& ತಳ) VL_ಶುದ್ಧ;
    ///< @return filename is relative
    ಸ್ಥಿರ bool filenameIsRel(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ;
    ///< @return path slashed with '/'
    ಸ್ಥಿರ string filename_Slash_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& filename) VL_ಶುದ್ಧ;

    // METHODS (file utilities)
    ಸ್ಥಿರ string getline(std::istream& is, ಅಕ್ಷರ delim = '\n');

    // METHODS (directory utilities)
    ಸ್ಥಿರ ಶೂನ್ಯ createDir(ಸ್ಥಿರಾಂಕ string& dirname);
    ಸ್ಥಿರ ಶೂನ್ಯ filesystemFlush(ಸ್ಥಿರಾಂಕ string& dirname);
    ಸ್ಥಿರ ಶೂನ್ಯ filesystemFlushBuildDir(ಸ್ಥಿರಾಂಕ string& dirname);
    ಸ್ಥಿರ ಶೂನ್ಯ unlinkRegexp(ಸ್ಥಿರಾಂಕ string& dir, ಸ್ಥಿರಾಂಕ string& regexp);

    // METHODS (memory)
    ಸ್ಥಿರ ಶೂನ್ಯ release_ಸ್ಮರಣೆ();

    // METHODS (random)
    ಸ್ಥಿರ uint64_t rand64(std::array<uint64_t, 2>& stater);
    ಸ್ಥಿರ string trueRandom(ಗಾತ್ರ_t ಗಾತ್ರ_2) VL_MT_ಸುರಕ್ಷಿತ;

    // METHODS (time & performance) (See also VlOs methods)
    ಸ್ಥಿರ ಶೂನ್ಯ u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    ಸ್ಥಿರ uint64_t ಸಮಯ_Usecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ system(ಸ್ಥಿರಾಂಕ string& command);
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
