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

#यदि_अपरिभाषितम् VERILATOR_V3OS_H_
#परिभाषय VERILATOR_V3OS_H_

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
    स्थिर string cwd() { प्रत्यागम filename_Real_मार्गः("."); }
    स्थिर string getenvStr(ध्रुव string& envvar, ध्रुव string& default_मानम्);
    स्थिर शून्य setenvStr(ध्रुव string& envvar, ध्रुव string& मानम्, ध्रुव string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    स्थिर string filenameJoin(std::initializer_list<ध्रुव std::string> paths) VL_विशुद्धम्;
    प्रतिरूपम् <typename... तर्काः_2>
    स्थिर string filenameJoin(तर्काः_2... तर्काः) VL_विशुद्धम् {
        प्रत्यागम filenameJoin({तर्काः...});
    };
    ///< @return file path without repeated separators and ./ prefix
    स्थिर string filenameCleanup(ध्रुव string& filename) VL_विशुद्धम्;
    ///< @return extension (.) part of filename
    स्थिर string filenameExt(ध्रुव string& filename) VL_विशुद्धम्;
    ///< @return non-directory part of filename
    स्थिर string filenameNonDir(ध्रुव string& filename) VL_विशुद्धम्;
    ///< @return basename of filename
    स्थिर string filenameNonDirExt(ध्रुव string& filename) VL_विशुद्धम्;
    ///< @return directory part of filename
    स्थिर string filenameDir(ध्रुव string& filename) VL_विशुद्धम्;
    ///< @return filename with env vars removed
    स्थिर string filenameSubstitute(ध्रुव string& filename);
    ///< @return realpath of filename
    स्थिर string filename_Real_मार्गः(ध्रुव string& filename) VL_विशुद्धम्;
    // @return absolute path of filename, without resolving symlinks
    स्थिर string filename_Absolute_मार्गः(ध्रुव string& filename) VL_विशुद्धम्;
    ///< @return relative path of filename, relative to base
    स्थिर string filename_Relative_मार्गः(ध्रुव string& filename, ध्रुव string& आधारः) VL_विशुद्धम्;
    ///< @return filename is relative
    स्थिर bool filenameIsRel(ध्रुव string& filename) VL_विशुद्धम्;
    ///< @return path slashed with '/'
    स्थिर string filename_Slash_मार्गः(ध्रुव string& filename) VL_विशुद्धम्;

    // METHODS (file utilities)
    स्थिर string getline(std::istream& is, वर्ण delim = '\n');

    // METHODS (directory utilities)
    स्थिर शून्य createDir(ध्रुव string& dirname);
    स्थिर शून्य filesystemFlush(ध्रुव string& dirname);
    स्थिर शून्य filesystemFlushBuildDir(ध्रुव string& dirname);
    स्थिर शून्य unlinkRegexp(ध्रुव string& dir, ध्रुव string& regexp);

    // METHODS (memory)
    स्थिर शून्य release_स्मृतिः();

    // METHODS (random)
    स्थिर uint64_t rand64(std::array<uint64_t, 2>& stater);
    स्थिर string trueRandom(आकार_t आकार) VL_MT_SAFE;

    // METHODS (time & performance) (See also VlOs methods)
    स्थिर शून्य u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    स्थिर uint64_t timeUsecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    स्थिर पूर्णाङ्क system(ध्रुव string& command);
    स्थिर शून्य selfTest();
};

#यदि_अन्तः  // Guard
