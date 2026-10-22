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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <array>

// Limited V3 headers here - this is a base class for Vlc etc
#شامل "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
عمومی:
    // METHODS (environment)
    ایستا string cwd() { بازگشت filenameRealPath("."); }
    ایستا string getenvStr(ثابت string& envvar, ثابت string& default_مقدار);
    ایستا تهی setenvStr(ثابت string& envvar, ثابت string& مقدار, ثابت string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    ایستا string filenameJoin(std::initializer_list<ثابت std::string> paths) VL_PURE;
    template <typename... Args>
    ایستا string filenameJoin(Args... args) VL_PURE {
        بازگشت filenameJoin({args...});
    };
    ///< @return file path without repeated separators and ./ prefix
    ایستا string filenameCleanup(ثابت string& filename) VL_PURE;
    ///< @return extension (.) part of filename
    ایستا string filenameExt(ثابت string& filename) VL_PURE;
    ///< @return non-directory part of filename
    ایستا string filenameNonDir(ثابت string& filename) VL_PURE;
    ///< @return basename of filename
    ایستا string filenameNonDirExt(ثابت string& filename) VL_PURE;
    ///< @return directory part of filename
    ایستا string filenameDir(ثابت string& filename) VL_PURE;
    ///< @return filename with env vars removed
    ایستا string filenameSubstitute(ثابت string& filename);
    ///< @return realpath of filename
    ایستا string filenameRealPath(ثابت string& filename) VL_PURE;
    // @return absolute path of filename, without resolving symlinks
    ایستا string filenameAbsolutePath(ثابت string& filename) VL_PURE;
    ///< @return relative path of filename, relative to base
    ایستا string filenameRelativePath(ثابت string& filename, ثابت string& base) VL_PURE;
    ///< @return filename is relative
    ایستا bool filenameIsRel(ثابت string& filename) VL_PURE;
    ///< @return path slashed with '/'
    ایستا string filenameSlashPath(ثابت string& filename) VL_PURE;

    // METHODS (file utilities)
    ایستا string getline(std::istream& is, نویسه delim = '\n');

    // METHODS (directory utilities)
    ایستا تهی createDir(ثابت string& dirname);
    ایستا تهی filesystemFlush(ثابت string& dirname);
    ایستا تهی filesystemFlushBuildDir(ثابت string& dirname);
    ایستا تهی unlinkRegexp(ثابت string& dir, ثابت string& regexp);

    // METHODS (memory)
    ایستا تهی releaseMemory();

    // METHODS (random)
    ایستا uint64_t rand64(std::array<uint64_t, 2>& stater);
    ایستا string trueRandom(size_t size) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    ایستا تهی u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    ایستا uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    ایستا صحیح system(ثابت string& command);
    ایستا تهی selfTest();
};

#endif  // Guard
