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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "VlcOptions.h"
#ଅନ୍ତର୍ଭାଵ "VlcPoint.h"
#ଅନ୍ତର୍ଭାଵ "VlcSource.h"
#ଅନ୍ତର୍ଭାଵ "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_ଶୀର୍ଷ final {
ସର୍ବସାଧାରଣ:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    ଶୂନ୍ୟ annotateCalc();
    ଶୂନ୍ୟ annotateCalcNeeded();
    ଶୂନ୍ୟ annotateOutputFiles(ଅଚଳ string& dirname);

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    Vlc_ଶୀର୍ଷ() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~Vlc_ଶୀର୍ଷ() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    VlcTests& tests() { ଫେରନ୍ତୁ m_tests; }
    VlcPoints& points() { ଫେରନ୍ତୁ m_points; }
    VlcSources& sources() { ଫେରନ୍ତୁ m_sources; }

    // METHODS
    ଶୂନ୍ୟ annotate(ଅଚଳ string& dirname);
    ଶୂନ୍ୟ printHierarchyReport();
    ଶୂନ୍ୟ printTypeSummary();
    ଶୂନ୍ୟ readCoverage(ଅଚଳ string& filename, bool nonfatal = false);
    ଶୂନ୍ୟ writeCoverage(ଅଚଳ string& filename);
    ଶୂନ୍ୟ writeInfo(ଅଚଳ string& filename);

    ଶୂନ୍ୟ rank();
};

//######################################################################

#endif  // guard
