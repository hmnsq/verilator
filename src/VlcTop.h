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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "VlcOptions.h"
#অন্তর্ভাব "VlcPoint.h"
#অন্তর্ভাব "VlcSource.h"
#অন্তর্ভাব "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_শীর্ষ final {
সার্বজনিক:
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
    শূন্য annotateOutputFiles(ধ্রুব string& dirname);

সার্বজনিক:
    // CONSTRUCTORS
    Vlc_শীর্ষ() = স্বভাব;
    ~Vlc_শীর্ষ() = স্বভাব;

    // ACCESSORS
    VlcTests& tests() { প্রত্যাগম m_tests; }
    VlcPoints& points() { প্রত্যাগম m_points; }
    VlcSources& sources() { প্রত্যাগম m_sources; }

    // METHODS
    শূন্য annotate(ধ্রুব string& dirname);
    শূন্য printHierarchyReport();
    শূন্য printTypeSummary();
    শূন্য readCoverage(ধ্রুব string& filename, bool nonfatal = false);
    শূন্য writeCoverage(ধ্রুব string& filename);
    শূন্য writeInfo(ধ্রুব string& filename);

    শূন্য rank();
};

//######################################################################

#endif  // guard
