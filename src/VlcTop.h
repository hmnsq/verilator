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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "VlcOptions.h"
#ಅನ್ತರ್ಭಾವ "VlcPoint.h"
#ಅನ್ತರ್ಭಾವ "VlcSource.h"
#ಅನ್ತರ್ಭಾವ "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_ತುದಿ final {
ಸಾರ್ವಜನಿಕ:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
ಗುಪ್ತ:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    ಶೂನ್ಯ annotateCalc();
    ಶೂನ್ಯ annotateCalcNeeded();
    ಶೂನ್ಯ annotateOutputFiles(ಧ್ರುವ string& dirname);

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Vlc_ತುದಿ() = ಸ್ವಭಾವ;
    ~Vlc_ತುದಿ() = ಸ್ವಭಾವ;

    // ACCESSORS
    VlcTests& tests() { ಪ್ರತ್ಯಾಗಮ m_tests; }
    VlcPoints& points() { ಪ್ರತ್ಯಾಗಮ m_points; }
    VlcSources& sources() { ಪ್ರತ್ಯಾಗಮ m_sources; }

    // METHODS
    ಶೂನ್ಯ annotate(ಧ್ರುವ string& dirname);
    ಶೂನ್ಯ printHierarchyReport();
    ಶೂನ್ಯ printTypeSummary();
    ಶೂನ್ಯ readCoverage(ಧ್ರುವ string& filename, bool nonfatal = false);
    ಶೂನ್ಯ writeCoverage(ಧ್ರುವ string& filename);
    ಶೂನ್ಯ writeInfo(ಧ್ರುವ string& filename);

    ಶೂನ್ಯ rank();
};

//######################################################################

#endif  // guard
