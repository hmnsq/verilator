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

#अगर_अघोषित VERILATOR_V3OS_H_
#घोषणा VERILATOR_V3OS_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश <array>

// Limited V3 headers here - this is a base class for Vlc etc
#समावेश "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
खुला:
    // METHODS (environment)
    अटल string cwd() { वापस filenameRealPath("."); }
    अटल string getenvStr(स्थिर string& envvar, स्थिर string& default_मूल्य);
    अटल व्योम setenvStr(स्थिर string& envvar, स्थिर string& मूल्य, स्थिर string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    अटल string filenameJoin(std::initializer_list<स्थिर std::string> paths) VL_PURE;
    ढाँचा <typename... Args>
    अटल string filenameJoin(Args... args) VL_PURE {
        वापस filenameJoin({args...});
    };
    ///< @return file path without repeated separators and ./ prefix
    अटल string filenameCleanup(स्थिर string& filename) VL_PURE;
    ///< @return extension (.) part of filename
    अटल string filenameExt(स्थिर string& filename) VL_PURE;
    ///< @return non-directory part of filename
    अटल string filenameNonDir(स्थिर string& filename) VL_PURE;
    ///< @return basename of filename
    अटल string filenameNonDirExt(स्थिर string& filename) VL_PURE;
    ///< @return directory part of filename
    अटल string filenameDir(स्थिर string& filename) VL_PURE;
    ///< @return filename with env vars removed
    अटल string filenameSubstitute(स्थिर string& filename);
    ///< @return realpath of filename
    अटल string filenameRealPath(स्थिर string& filename) VL_PURE;
    // @return absolute path of filename, without resolving symlinks
    अटल string filenameAbsolutePath(स्थिर string& filename) VL_PURE;
    ///< @return relative path of filename, relative to base
    अटल string filenameRelativePath(स्थिर string& filename, स्थिर string& base) VL_PURE;
    ///< @return filename is relative
    अटल bool filenameIsRel(स्थिर string& filename) VL_PURE;
    ///< @return path slashed with '/'
    अटल string filenameSlashPath(स्थिर string& filename) VL_PURE;

    // METHODS (file utilities)
    अटल string getline(std::istream& is, अक्षर delim = '\n');

    // METHODS (directory utilities)
    अटल व्योम createDir(स्थिर string& स_नाम);
    अटल व्योम filesystemFlush(स्थिर string& स_नाम);
    अटल व्योम filesystemFlushBuildDir(स्थिर string& स_नाम);
    अटल व्योम unlinkRegexp(स्थिर string& dir, स्थिर string& regexp);

    // METHODS (memory)
    अटल व्योम releaseMemory();

    // METHODS (random)
    अटल uint64_t rand64(std::array<uint64_t, 2>& stater);
    अटल string trueRandom(माप_प्रकार आकार) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    अटल व्योम u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    अटल uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    अटल पूर्णांक प्रणाली(स्थिर string& command);
    अटल व्योम selfTest();
};

#पूर्ण_अगर  // Guard
