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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "VlcOptions.h"
#অন্তর্ভাৱ "VlcPoint.h"
#অন্তর্ভাৱ "VlcSource.h"
#অন্তর্ভাৱ "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_শীৰ্ষ final {
সার্ৱজনিক:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
গুপ্ত:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    শূন্য annotateCalc();
    শূন্য annotateCalcNeeded();
    শূন্য annotateOutputFiles(ধ্রুৱ string& dirname);

সার্ৱজনিক:
    // CONSTRUCTORS
    Vlc_শীৰ্ষ() = স্ৱভাৱ;
    ~Vlc_শীৰ্ষ() = স্ৱভাৱ;

    // ACCESSORS
    VlcTests& tests() { ঘূৰাই m_tests; }
    VlcPoints& points() { ঘূৰাই m_points; }
    VlcSources& sources() { ঘূৰাই m_sources; }

    // METHODS
    শূন্য annotate(ধ্রুৱ string& dirname);
    শূন্য printHierarchyReport();
    শূন্য printTypeSummary();
    শূন্য readCoverage(ধ্রুৱ string& filename, bool nonfatal = false);
    শূন্য writeCoverage(ধ্রুৱ string& filename);
    শূন্য writeInfo(ধ্রুৱ string& filename);

    শূন্য rank();
};

//######################################################################

#endif  // guard
