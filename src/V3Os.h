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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ <array>

// Limited V3 headers here - this is a base class for Vlc etc
#অন্তর্ভাৱ "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
সার্ৱজনিক:
    // METHODS (environment)
    স্থির string cwd() { ঘূৰাই filenameRealPath("."); }
    স্থির string getenvStr(ধ্রুৱ string& envvar, ধ্রুৱ string& default_মান);
    স্থির শূন্য setenvStr(ধ্রুৱ string& envvar, ধ্রুৱ string& মান, ধ্রুৱ string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    স্থির string filenameJoin(std::initializer_list<ধ্রুৱ std::string> paths) VL_PURE;
    template <typename... Args>
    স্থির string filenameJoin(Args... args) VL_PURE {
        ঘূৰাই filenameJoin({args...});
    };
    ///< @return file path without repeated separators and ./ prefix
    স্থির string filenameCleanup(ধ্রুৱ string& filename) VL_PURE;
    ///< @return extension (.) part of filename
    স্থির string filenameExt(ধ্রুৱ string& filename) VL_PURE;
    ///< @return non-directory part of filename
    স্থির string filenameNonDir(ধ্রুৱ string& filename) VL_PURE;
    ///< @return basename of filename
    স্থির string filenameNonDirExt(ধ্রুৱ string& filename) VL_PURE;
    ///< @return directory part of filename
    স্থির string filenameDir(ধ্রুৱ string& filename) VL_PURE;
    ///< @return filename with env vars removed
    স্থির string filenameSubstitute(ধ্রুৱ string& filename);
    ///< @return realpath of filename
    স্থির string filenameRealPath(ধ্রুৱ string& filename) VL_PURE;
    // @return absolute path of filename, without resolving symlinks
    স্থির string filenameAbsolutePath(ধ্রুৱ string& filename) VL_PURE;
    ///< @return relative path of filename, relative to base
    স্থির string filenameRelativePath(ধ্রুৱ string& filename, ধ্রুৱ string& base) VL_PURE;
    ///< @return filename is relative
    স্থির bool filenameIsRel(ধ্রুৱ string& filename) VL_PURE;
    ///< @return path slashed with '/'
    স্থির string filenameSlashPath(ধ্রুৱ string& filename) VL_PURE;

    // METHODS (file utilities)
    স্থির string getline(std::istream& is, ৱর্ণ delim = '\n');

    // METHODS (directory utilities)
    স্থির শূন্য createDir(ধ্রুৱ string& dirname);
    স্থির শূন্য filesystemFlush(ধ্রুৱ string& dirname);
    স্থির শূন্য filesystemFlushBuildDir(ধ্রুৱ string& dirname);
    স্থির শূন্য unlinkRegexp(ধ্রুৱ string& dir, ধ্রুৱ string& regexp);

    // METHODS (memory)
    স্থির শূন্য releaseMemory();

    // METHODS (random)
    স্থির uint64_t rand64(std::array<uint64_t, 2>& stater);
    স্থির string trueRandom(size_t size) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    স্থির শূন্য u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    স্থির uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    স্থির পূর্ণাঙ্ক system(ধ্রুৱ string& command);
    স্থির শূন্য selfTest();
};

#endif  // Guard
