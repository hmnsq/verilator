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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ <array>

// Limited V3 headers here - this is a base class for Vlc etc
#ଅନ୍ତର୍ଭାଵ "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
ସର୍ବସାଧାରଣ:
    // METHODS (environment)
    ସ୍ଥିର string cwd() { ଫେରନ୍ତୁ filenameRealPath("."); }
    ସ୍ଥିର string getenvStr(ଅଚଳ string& envvar, ଅଚଳ string& default_ମୂଲ୍ୟ);
    ସ୍ଥିର ଶୂନ୍ୟ setenvStr(ଅଚଳ string& envvar, ଅଚଳ string& ମୂଲ୍ୟ, ଅଚଳ string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    ସ୍ଥିର string filenameJoin(std::initializer_list<ଅଚଳ std::string> paths) VL_PURE;
    template <typename... Args>
    ସ୍ଥିର string filenameJoin(Args... args) VL_PURE {
        ଫେରନ୍ତୁ filenameJoin({args...});
    };
    ///< @return file path without repeated separators and ./ prefix
    ସ୍ଥିର string filenameCleanup(ଅଚଳ string& filename) VL_PURE;
    ///< @return extension (.) part of filename
    ସ୍ଥିର string filenameExt(ଅଚଳ string& filename) VL_PURE;
    ///< @return non-directory part of filename
    ସ୍ଥିର string filenameNonDir(ଅଚଳ string& filename) VL_PURE;
    ///< @return basename of filename
    ସ୍ଥିର string filenameNonDirExt(ଅଚଳ string& filename) VL_PURE;
    ///< @return directory part of filename
    ସ୍ଥିର string filenameDir(ଅଚଳ string& filename) VL_PURE;
    ///< @return filename with env vars removed
    ସ୍ଥିର string filenameSubstitute(ଅଚଳ string& filename);
    ///< @return realpath of filename
    ସ୍ଥିର string filenameRealPath(ଅଚଳ string& filename) VL_PURE;
    // @return absolute path of filename, without resolving symlinks
    ସ୍ଥିର string filenameAbsolutePath(ଅଚଳ string& filename) VL_PURE;
    ///< @return relative path of filename, relative to base
    ସ୍ଥିର string filenameRelativePath(ଅଚଳ string& filename, ଅଚଳ string& base) VL_PURE;
    ///< @return filename is relative
    ସ୍ଥିର bool filenameIsRel(ଅଚଳ string& filename) VL_PURE;
    ///< @return path slashed with '/'
    ସ୍ଥିର string filenameSlashPath(ଅଚଳ string& filename) VL_PURE;

    // METHODS (file utilities)
    ସ୍ଥିର string getline(std::istream& is, ଅକ୍ଷର delim = '\n');

    // METHODS (directory utilities)
    ସ୍ଥିର ଶୂନ୍ୟ createDir(ଅଚଳ string& dirname);
    ସ୍ଥିର ଶୂନ୍ୟ filesystemFlush(ଅଚଳ string& dirname);
    ସ୍ଥିର ଶୂନ୍ୟ filesystemFlushBuildDir(ଅଚଳ string& dirname);
    ସ୍ଥିର ଶୂନ୍ୟ unlinkRegexp(ଅଚଳ string& dir, ଅଚଳ string& regexp);

    // METHODS (memory)
    ସ୍ଥିର ଶୂନ୍ୟ releaseMemory();

    // METHODS (random)
    ସ୍ଥିର uint64_t rand64(std::array<uint64_t, 2>& stater);
    ସ୍ଥିର string trueRandom(size_t size) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    ସ୍ଥିର ଶୂନ୍ୟ u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    ସ୍ଥିର uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା system(ଅଚଳ string& command);
    ସ୍ଥିର ଶୂନ୍ୟ selfTest();
};

#endif  // Guard
