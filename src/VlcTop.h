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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "VlcOptions.h"
#અન્તર્ભાવ "VlcPoint.h"
#અન્તર્ભાવ "VlcSource.h"
#અન્તર્ભાવ "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_ટોચ final {
જાહેર:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
ખાનગી:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    શૂન્ય annotateCalc();
    શૂન્ય annotateCalcNeeded();
    શૂન્ય annotateOutputFiles(અચલ string& dirname);

જાહેર:
    // CONSTRUCTORS
    Vlc_ટોચ() = મૂળભૂત;
    ~Vlc_ટોચ() = મૂળભૂત;

    // ACCESSORS
    VlcTests& tests() { પાછા m_tests; }
    VlcPoints& points() { પાછા m_points; }
    VlcSources& sources() { પાછા m_sources; }

    // METHODS
    શૂન્ય annotate(અચલ string& dirname);
    શૂન્ય printHierarchyReport();
    શૂન્ય printTypeSummary();
    શૂન્ય readCoverage(અચલ string& filename, bool nonfatal = false);
    શૂન્ય writeCoverage(અચલ string& filename);
    શૂન્ય writeInfo(અચલ string& filename);

    શૂન્ય rank();
};

//######################################################################

#endif  // guard
