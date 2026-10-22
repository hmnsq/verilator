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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <array>

// Limited V3 headers here - this is a base class for Vlc etc
#अन्तर्भाव "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
सार्वजनिक:
    // METHODS (environment)
    स्थिर string cwd() { परत filenameRealPath("."); }
    स्थिर string getenvStr(स्थिरांक string& envvar, स्थिरांक string& default_मूल्य);
    स्थिर शून्य setenvStr(स्थिरांक string& envvar, स्थिरांक string& मूल्य, स्थिरांक string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    स्थिर string filenameJoin(std::initializer_list<स्थिरांक std::string> paths) VL_PURE;
    ढाँचा <typename... Args>
    स्थिर string filenameJoin(Args... args) VL_PURE {
        परत filenameJoin({args...});
    };
    ///< @return file path without repeated separators and ./ prefix
    स्थिर string filenameCleanup(स्थिरांक string& filename) VL_PURE;
    ///< @return extension (.) part of filename
    स्थिर string filenameExt(स्थिरांक string& filename) VL_PURE;
    ///< @return non-directory part of filename
    स्थिर string filenameNonDir(स्थिरांक string& filename) VL_PURE;
    ///< @return basename of filename
    स्थिर string filenameNonDirExt(स्थिरांक string& filename) VL_PURE;
    ///< @return directory part of filename
    स्थिर string filenameDir(स्थिरांक string& filename) VL_PURE;
    ///< @return filename with env vars removed
    स्थिर string filenameSubstitute(स्थिरांक string& filename);
    ///< @return realpath of filename
    स्थिर string filenameRealPath(स्थिरांक string& filename) VL_PURE;
    // @return absolute path of filename, without resolving symlinks
    स्थिर string filenameAbsolutePath(स्थिरांक string& filename) VL_PURE;
    ///< @return relative path of filename, relative to base
    स्थिर string filenameRelativePath(स्थिरांक string& filename, स्थिरांक string& base) VL_PURE;
    ///< @return filename is relative
    स्थिर bool filenameIsRel(स्थिरांक string& filename) VL_PURE;
    ///< @return path slashed with '/'
    स्थिर string filenameSlashPath(स्थिरांक string& filename) VL_PURE;

    // METHODS (file utilities)
    स्थिर string getline(std::istream& is, अक्षर delim = '\n');

    // METHODS (directory utilities)
    स्थिर शून्य createDir(स्थिरांक string& स_नाम);
    स्थिर शून्य filesystemFlush(स्थिरांक string& स_नाम);
    स्थिर शून्य filesystemFlushBuildDir(स्थिरांक string& स_नाम);
    स्थिर शून्य unlinkRegexp(स्थिरांक string& dir, स्थिरांक string& regexp);

    // METHODS (memory)
    स्थिर शून्य releaseMemory();

    // METHODS (random)
    स्थिर uint64_t rand64(std::array<uint64_t, 2>& stater);
    स्थिर string trueRandom(माप_प्रकार size) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    स्थिर शून्य u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    स्थिर uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    स्थिर पूर्णांक प्रणाली(स्थिरांक string& command);
    स्थिर शून्य selfTest();
};

#endif  // Guard
