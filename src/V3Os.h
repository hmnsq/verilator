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

#నిర్వచితం_కాకపోతే VERILATOR_V3OS_H_
#నిర్వచించు VERILATOR_V3OS_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <array>

// Limited V3 headers here - this is a base class for Vlc etc
#అన్తర్భావ "V3Error.h"

//============================================================================
// V3Os: OS static class

class V3Os final {
బహిరంగ:
    // METHODS (environment)
    స్థిర string cwd() { తిరిగి filename_Real_మార్గం("."); }
    స్థిర string getenvStr(స్థిరరాశి string& envvar, స్థిరరాశి string& default_విలువ);
    స్థిర శూన్య setenvStr(స్థిరరాశి string& envvar, స్థిరరాశి string& విలువ, స్థిరరాశి string& why);

    // METHODS (generic filename utilities)
    ///< @return concatenated path
    స్థిర string filenameJoin(std::initializer_list<స్థిరరాశి std::string> paths) VL_శుద్ధ;
    మూస <typename... వాదనలు_2>
    స్థిర string filenameJoin(వాదనలు_2... వాదనలు) VL_శుద్ధ {
        తిరిగి filenameJoin({వాదనలు...});
    };
    ///< @return file path without repeated separators and ./ prefix
    స్థిర string filenameCleanup(స్థిరరాశి string& filename) VL_శుద్ధ;
    ///< @return extension (.) part of filename
    స్థిర string filenameExt(స్థిరరాశి string& filename) VL_శుద్ధ;
    ///< @return non-directory part of filename
    స్థిర string filenameNonDir(స్థిరరాశి string& filename) VL_శుద్ధ;
    ///< @return basename of filename
    స్థిర string filenameNonDirExt(స్థిరరాశి string& filename) VL_శుద్ధ;
    ///< @return directory part of filename
    స్థిర string filenameDir(స్థిరరాశి string& filename) VL_శుద్ధ;
    ///< @return filename with env vars removed
    స్థిర string filenameSubstitute(స్థిరరాశి string& filename);
    ///< @return realpath of filename
    స్థిర string filename_Real_మార్గం(స్థిరరాశి string& filename) VL_శుద్ధ;
    // @return absolute path of filename, without resolving symlinks
    స్థిర string filename_Absolute_మార్గం(స్థిరరాశి string& filename) VL_శుద్ధ;
    ///< @return relative path of filename, relative to base
    స్థిర string filename_Relative_మార్గం(స్థిరరాశి string& filename, స్థిరరాశి string& పునాది) VL_శుద్ధ;
    ///< @return filename is relative
    స్థిర bool filenameIsRel(స్థిరరాశి string& filename) VL_శుద్ధ;
    ///< @return path slashed with '/'
    స్థిర string filename_Slash_మార్గం(స్థిరరాశి string& filename) VL_శుద్ధ;

    // METHODS (file utilities)
    స్థిర string getline(std::istream& is, అక్షరం delim = '\n');

    // METHODS (directory utilities)
    స్థిర శూన్య createDir(స్థిరరాశి string& dirname);
    స్థిర శూన్య filesystemFlush(స్థిరరాశి string& dirname);
    స్థిర శూన్య filesystemFlushBuildDir(స్థిరరాశి string& dirname);
    స్థిర శూన్య unlinkRegexp(స్థిరరాశి string& dir, స్థిరరాశి string& regexp);

    // METHODS (memory)
    స్థిర శూన్య release_జ్ఞాపకం();

    // METHODS (random)
    స్థిర uint64_t rand64(std::array<uint64_t, 2>& stater);
    స్థిర string trueRandom(కొలత_t కొలత) VL_MT_సురక్షిత;

    // METHODS (time & performance) (See also VlOs methods)
    స్థిర శూన్య u_sleep(int64_t usec);  ///< Sleep for a given number of microseconds.
    /// Return wall time since epoch in microseconds, or 0 if not implemented
    స్థిర uint64_t సమయం_Usecs();

    // METHODS (sub command)
    /// Run system command, returns the exit code of the child process.
    స్థిర పూర్ణాంకం system(స్థిరరాశి string& command);
    స్థిర శూన్య selfTest();
};

#అయితే_ముగింపు  // Guard
