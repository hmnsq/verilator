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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3OS_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3OS_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <array>

// Limited V3 headers here - this is a base class for Vlc etc
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
ਜਨਤਕ:
    // METHODS (environment)
    ਸਥਿਰ string cwd() { ਵਾਪਸ filename_Real_ਰਸਤਾ("."); }
    ਸਥਿਰ string getenvStr(ਅਚਲ string& envvar, ਅਚਲ string& default_ਮੁੱਲ);
    ਸਥਿਰ ਖਾਲੀ setenvStr(ਅਚਲ string& envvar, ਅਚਲ string& ਮੁੱਲ, ਅਚਲ string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    ਸਥਿਰ string filenameJoin(std::initializer_list<ਅਚਲ std::string> paths) VL_ਸ਼ੁੱਧ;
    ਸਾਂਚਾ <typename... ਦਲੀਲਾਂ_2>
    ਸਥਿਰ string filenameJoin(ਦਲੀਲਾਂ_2... ਦਲੀਲਾਂ) VL_ਸ਼ੁੱਧ {
        ਵਾਪਸ filenameJoin({ਦਲੀਲਾਂ...});
    };
    ///< @return file path without repeated separators and ./ prefix
    ਸਥਿਰ string filenameCleanup(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ;
    ///< @return extension (.) part of filename
    ਸਥਿਰ string filenameExt(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ;
    ///< @return non-directory part of filename
    ਸਥਿਰ string filenameNonDir(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ;
    ///< @return basename of filename
    ਸਥਿਰ string filenameNonDirExt(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ;
    ///< @return directory part of filename
    ਸਥਿਰ string filenameDir(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ;
    ///< @return filename with env vars removed
    ਸਥਿਰ string filenameSubstitute(ਅਚਲ string& filename);
    ///< @return realpath of filename
    ਸਥਿਰ string filename_Real_ਰਸਤਾ(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ;
    // @return absolute path of filename, without resolving symlinks
    ਸਥਿਰ string filename_Absolute_ਰਸਤਾ(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ;
    ///< @return relative path of filename, relative to base
    ਸਥਿਰ string filename_Relative_ਰਸਤਾ(ਅਚਲ string& filename, ਅਚਲ string& ਅਧਾਰ) VL_ਸ਼ੁੱਧ;
    ///< @return filename is relative
    ਸਥਿਰ bool filenameIsRel(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ;
    ///< @return path slashed with '/'
    ਸਥਿਰ string filename_Slash_ਰਸਤਾ(ਅਚਲ string& filename) VL_ਸ਼ੁੱਧ;

    // METHODS (file utilities)
    ਸਥਿਰ string getline(std::istream& is, ਅੱਖਰ delim = '\n');

    // METHODS (directory utilities)
    ਸਥਿਰ ਖਾਲੀ createDir(ਅਚਲ string& dirname);
    ਸਥਿਰ ਖਾਲੀ filesystemFlush(ਅਚਲ string& dirname);
    ਸਥਿਰ ਖਾਲੀ filesystemFlushBuildDir(ਅਚਲ string& dirname);
    ਸਥਿਰ ਖਾਲੀ unlinkRegexp(ਅਚਲ string& dir, ਅਚਲ string& regexp);

    // METHODS (memory)
    ਸਥਿਰ ਖਾਲੀ release_ਸਮ੍ਰਿਤੀ();

    // METHODS (random)
    ਸਥਿਰ uint64_t rand64(std::array<uint64_t, 2>& stater);
    ਸਥਿਰ string trueRandom(ਮਾਪ_t ਮਾਪ) VL_MT_ਸੁਰੱਖਿਅਤ;

    // METHODS (time & performance) (See also VlOs methods)
    ਸਥਿਰ ਖਾਲੀ u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    ਸਥਿਰ uint64_t ਸਮਾਂ_Usecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    ਸਥਿਰ ਪੂਰਨ system(ਅਚਲ string& command);
    ਸਥਿਰ ਖਾਲੀ selfTest();
};

#ਜੇ_ਅੰਤ  // Guard
