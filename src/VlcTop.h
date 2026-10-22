// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: Top global container
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_VLCTOP_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_VLCTOP_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "VlcOptions.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "VlcPoint.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "VlcSource.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_ਸਿਖਰ final {
ਜਨਤਕ:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
ਨਿਜੀ:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    ਖਾਲੀ annotateCalc();
    ਖਾਲੀ annotateCalcNeeded();
    ਖਾਲੀ annotateOutputFiles(ਅਚਲ string& dirname);

ਜਨਤਕ:
    // CONSTRUCTORS
    Vlc_ਸਿਖਰ() = ਮੂਲ;
    ~Vlc_ਸਿਖਰ() = ਮੂਲ;

    // ACCESSORS
    VlcTests& tests() { ਵਾਪਸ m_tests; }
    VlcPoints& points() { ਵਾਪਸ m_points; }
    VlcSources& sources() { ਵਾਪਸ m_sources; }

    // METHODS
    ਖਾਲੀ annotate(ਅਚਲ string& dirname);
    ਖਾਲੀ printHierarchyReport();
    ਖਾਲੀ printTypeSummary();
    ਖਾਲੀ readCoverage(ਅਚਲ string& filename, bool nonfatal = false);
    ਖਾਲੀ writeCoverage(ਅਚਲ string& filename);
    ਖਾਲੀ writeInfo(ਅਚਲ string& filename);

    ਖਾਲੀ rank();
};

//######################################################################

#ਜੇ_ਅੰਤ  // guard
