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

#ifndef VERILATOR_VLCTOP_H_
#define VERILATOR_VLCTOP_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "VlcOptions.h"
#شامل "VlcPoint.h"
#شامل "VlcSource.h"
#شامل "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_بالاترین final {
عمومی:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
خصوصی:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    تهی annotateCalc();
    تهی annotateCalcNeeded();
    تهی annotateOutputFiles(ثابت string& dirname);

عمومی:
    // CONSTRUCTORS
    Vlc_بالاترین() = پیشفرض;
    ~Vlc_بالاترین() = پیشفرض;

    // ACCESSORS
    VlcTests& tests() { بازگشت m_tests; }
    VlcPoints& points() { بازگشت m_points; }
    VlcSources& sources() { بازگشت m_sources; }

    // METHODS
    تهی annotate(ثابت string& dirname);
    تهی printHierarchyReport();
    تهی printTypeSummary();
    تهی readCoverage(ثابت string& filename, bool nonfatal = false);
    تهی writeCoverage(ثابت string& filename);
    تهی writeInfo(ثابت string& filename);

    تهی rank();
};

//######################################################################

#endif  // guard
