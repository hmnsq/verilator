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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ <array>

// Limited V3 headers here - this is a base class for Vlc etc
#அந்தர்பாவ "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
பொது:
    // METHODS (environment)
    நிலையான string cwd() { திரும்பு filenameRealPath("."); }
    நிலையான string getenvStr(மாறிலி string& envvar, மாறிலி string& default_மதிப்பு);
    நிலையான வெறுமை setenvStr(மாறிலி string& envvar, மாறிலி string& மதிப்பு, மாறிலி string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    நிலையான string filenameJoin(std::initializer_list<மாறிலி std::string> paths) VL_PURE;
    template <typename... Args>
    நிலையான string filenameJoin(Args... args) VL_PURE {
        திரும்பு filenameJoin({args...});
    };
    ///< @return file path without repeated separators and ./ prefix
    நிலையான string filenameCleanup(மாறிலி string& filename) VL_PURE;
    ///< @return extension (.) part of filename
    நிலையான string filenameExt(மாறிலி string& filename) VL_PURE;
    ///< @return non-directory part of filename
    நிலையான string filenameNonDir(மாறிலி string& filename) VL_PURE;
    ///< @return basename of filename
    நிலையான string filenameNonDirExt(மாறிலி string& filename) VL_PURE;
    ///< @return directory part of filename
    நிலையான string filenameDir(மாறிலி string& filename) VL_PURE;
    ///< @return filename with env vars removed
    நிலையான string filenameSubstitute(மாறிலி string& filename);
    ///< @return realpath of filename
    நிலையான string filenameRealPath(மாறிலி string& filename) VL_PURE;
    // @return absolute path of filename, without resolving symlinks
    நிலையான string filenameAbsolutePath(மாறிலி string& filename) VL_PURE;
    ///< @return relative path of filename, relative to base
    நிலையான string filenameRelativePath(மாறிலி string& filename, மாறிலி string& base) VL_PURE;
    ///< @return filename is relative
    நிலையான bool filenameIsRel(மாறிலி string& filename) VL_PURE;
    ///< @return path slashed with '/'
    நிலையான string filenameSlashPath(மாறிலி string& filename) VL_PURE;

    // METHODS (file utilities)
    நிலையான string getline(std::istream& is, எழுத்து delim = '\n');

    // METHODS (directory utilities)
    நிலையான வெறுமை createDir(மாறிலி string& dirname);
    நிலையான வெறுமை filesystemFlush(மாறிலி string& dirname);
    நிலையான வெறுமை filesystemFlushBuildDir(மாறிலி string& dirname);
    நிலையான வெறுமை unlinkRegexp(மாறிலி string& dir, மாறிலி string& regexp);

    // METHODS (memory)
    நிலையான வெறுமை releaseMemory();

    // METHODS (random)
    நிலையான uint64_t rand64(std::array<uint64_t, 2>& stater);
    நிலையான string trueRandom(size_t size) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    நிலையான வெறுமை u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    நிலையான uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    நிலையான முழு system(மாறிலி string& command);
    நிலையான வெறுமை selfTest();
};

#endif  // Guard
