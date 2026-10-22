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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_VLCTOP_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_VLCTOP_H_

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
ಖಾಸಗಿ:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    ಶೂನ್ಯ annotateCalc();
    ಶೂನ್ಯ annotateCalcNeeded();
    ಶೂನ್ಯ annotateOutputFiles(ಸ್ಥಿರಾಂಕ string& dirname);

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Vlc_ತುದಿ() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~Vlc_ತುದಿ() = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    VlcTests& tests() { ಹಿಂತಿರುಗಿ m_tests; }
    VlcPoints& points() { ಹಿಂತಿರುಗಿ m_points; }
    VlcSources& sources() { ಹಿಂತಿರುಗಿ m_sources; }

    // METHODS
    ಶೂನ್ಯ annotate(ಸ್ಥಿರಾಂಕ string& dirname);
    ಶೂನ್ಯ printHierarchyReport();
    ಶೂನ್ಯ printTypeSummary();
    ಶೂನ್ಯ readCoverage(ಸ್ಥಿರಾಂಕ string& filename, bool nonfatal = false);
    ಶೂನ್ಯ writeCoverage(ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ writeInfo(ಸ್ಥಿರಾಂಕ string& filename);

    ಶೂನ್ಯ rank();
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // guard
