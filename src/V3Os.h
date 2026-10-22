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

#اگر_غیر_تعریف_شدہ VERILATOR_V3OS_H_
#تعریف_کرو VERILATOR_V3OS_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <array>

// Limited V3 headers here - this is a base class for Vlc etc
#شامل "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
عوامی:
    // METHODS (environment)
    ساکن string cwd() { واپس filename_Real_راستہ("."); }
    ساکن string getenvStr(ثابت string& envvar, ثابت string& default_قدر);
    ساکن خالی setenvStr(ثابت string& envvar, ثابت string& قدر, ثابت string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    ساکن string filenameJoin(std::initializer_list<ثابت std::string> paths) VL_خالص;
    سانچہ <typename... دلائل_2>
    ساکن string filenameJoin(دلائل_2... دلائل) VL_خالص {
        واپس filenameJoin({دلائل...});
    };
    ///< @return file path without repeated separators and ./ prefix
    ساکن string filenameCleanup(ثابت string& filename) VL_خالص;
    ///< @return extension (.) part of filename
    ساکن string filenameExt(ثابت string& filename) VL_خالص;
    ///< @return non-directory part of filename
    ساکن string filenameNonDir(ثابت string& filename) VL_خالص;
    ///< @return basename of filename
    ساکن string filenameNonDirExt(ثابت string& filename) VL_خالص;
    ///< @return directory part of filename
    ساکن string filenameDir(ثابت string& filename) VL_خالص;
    ///< @return filename with env vars removed
    ساکن string filenameSubstitute(ثابت string& filename);
    ///< @return realpath of filename
    ساکن string filename_Real_راستہ(ثابت string& filename) VL_خالص;
    // @return absolute path of filename, without resolving symlinks
    ساکن string filename_Absolute_راستہ(ثابت string& filename) VL_خالص;
    ///< @return relative path of filename, relative to base
    ساکن string filename_Relative_راستہ(ثابت string& filename, ثابت string& بنیاد) VL_خالص;
    ///< @return filename is relative
    ساکن bool filenameIsRel(ثابت string& filename) VL_خالص;
    ///< @return path slashed with '/'
    ساکن string filename_Slash_راستہ(ثابت string& filename) VL_خالص;

    // METHODS (file utilities)
    ساکن string getline(std::istream& is, حرف delim = '\n');

    // METHODS (directory utilities)
    ساکن خالی createDir(ثابت string& dirname);
    ساکن خالی filesystemFlush(ثابت string& dirname);
    ساکن خالی filesystemFlushBuildDir(ثابت string& dirname);
    ساکن خالی unlinkRegexp(ثابت string& dir, ثابت string& regexp);

    // METHODS (memory)
    ساکن خالی release_حافظہ();

    // METHODS (random)
    ساکن uint64_t rand64(std::array<uint64_t, 2>& stater);
    ساکن string trueRandom(سائز_t سائز) VL_MT_محفوظ;

    // METHODS (time & performance) (See also VlOs methods)
    ساکن خالی u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    ساکن uint64_t وقت_Usecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    ساکن صحیح system(ثابت string& command);
    ساکن خالی selfTest();
};

#اگر_ختم  // Guard
