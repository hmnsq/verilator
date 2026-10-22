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

#که_نه_تعریف_شوی VERILATOR_V3OS_H_
#تعریف_کړه VERILATOR_V3OS_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <array>

// Limited V3 headers here - this is a base class for Vlc etc
#ضمن "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
عام:
    // METHODS (environment)
    ساكن string cwd() { أرجع filename_Real_لار("."); }
    ساكن string getenvStr(ثابت string& envvar, ثابت string& default_ارزښت);
    ساكن فراغ setenvStr(ثابت string& envvar, ثابت string& ارزښت, ثابت string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    ساكن string filenameJoin(std::initializer_list<ثابت std::string> paths) VL_خالص;
    کينډۍ <typename... دلیلونه_2>
    ساكن string filenameJoin(دلیلونه_2... دلیلونه) VL_خالص {
        أرجع filenameJoin({دلیلونه...});
    };
    ///< @return file path without repeated separators and ./ prefix
    ساكن string filenameCleanup(ثابت string& filename) VL_خالص;
    ///< @return extension (.) part of filename
    ساكن string filenameExt(ثابت string& filename) VL_خالص;
    ///< @return non-directory part of filename
    ساكن string filenameNonDir(ثابت string& filename) VL_خالص;
    ///< @return basename of filename
    ساكن string filenameNonDirExt(ثابت string& filename) VL_خالص;
    ///< @return directory part of filename
    ساكن string filenameDir(ثابت string& filename) VL_خالص;
    ///< @return filename with env vars removed
    ساكن string filenameSubstitute(ثابت string& filename);
    ///< @return realpath of filename
    ساكن string filename_Real_لار(ثابت string& filename) VL_خالص;
    // @return absolute path of filename, without resolving symlinks
    ساكن string filename_Absolute_لار(ثابت string& filename) VL_خالص;
    ///< @return relative path of filename, relative to base
    ساكن string filename_Relative_لار(ثابت string& filename, ثابت string& بنسټ) VL_خالص;
    ///< @return filename is relative
    ساكن bool filenameIsRel(ثابت string& filename) VL_خالص;
    ///< @return path slashed with '/'
    ساكن string filename_Slash_لار(ثابت string& filename) VL_خالص;

    // METHODS (file utilities)
    ساكن string getline(std::istream& is, حرف delim = '\n');

    // METHODS (directory utilities)
    ساكن فراغ createDir(ثابت string& dirname);
    ساكن فراغ filesystemFlush(ثابت string& dirname);
    ساكن فراغ filesystemFlushBuildDir(ثابت string& dirname);
    ساكن فراغ unlinkRegexp(ثابت string& dir, ثابت string& regexp);

    // METHODS (memory)
    ساكن فراغ release_حافظه();

    // METHODS (random)
    ساكن uint64_t rand64(std::array<uint64_t, 2>& stater);
    ساكن string trueRandom(اندازه_t اندازه) VL_MT_خوندي;

    // METHODS (time & performance) (See also VlOs methods)
    ساكن فراغ u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    ساكن uint64_t وخت_Usecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    ساكن صحيح system(ثابت string& command);
    ساكن فراغ selfTest();
};

#د_که_پای  // Guard
