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

#اگر_غیر_تعریف_شدہ VERILATOR_VLCTOP_H_
#تعریف_کرو VERILATOR_VLCTOP_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "VlcOptions.h"
#شامل "VlcPoint.h"
#شامل "VlcSource.h"
#شامل "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_چوٹی final {
عوامی:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
نجی:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    خالی annotateCalc();
    خالی annotateCalcNeeded();
    خالی annotateOutputFiles(ثابت string& dirname);

عوامی:
    // CONSTRUCTORS
    Vlc_چوٹی() = طےشدہ;
    ~Vlc_چوٹی() = طےشدہ;

    // ACCESSORS
    VlcTests& tests() { واپس m_tests; }
    VlcPoints& points() { واپس m_points; }
    VlcSources& sources() { واپس m_sources; }

    // METHODS
    خالی annotate(ثابت string& dirname);
    خالی printHierarchyReport();
    خالی printTypeSummary();
    خالی readCoverage(ثابت string& filename, bool nonfatal = false);
    خالی writeCoverage(ثابت string& filename);
    خالی writeInfo(ثابت string& filename);

    خالی rank();
};

//######################################################################

#اگر_ختم  // guard
