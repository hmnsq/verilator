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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "VlcOptions.h"
#अन्तर्भाव "VlcPoint.h"
#अन्तर्भाव "VlcSource.h"
#अन्तर्भाव "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_शीर्ष final {
सार्वजनिक:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
गुप्त:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    शून्य annotateCalc();
    शून्य annotateCalcNeeded();
    शून्य annotateOutputFiles(अचर string& स_नाम);

सार्वजनिक:
    // CONSTRUCTORS
    Vlc_शीर्ष() = स्वभाव;
    ~Vlc_शीर्ष() = स्वभाव;

    // ACCESSORS
    VlcTests& tests() { फर्काऊ m_tests; }
    VlcPoints& points() { फर्काऊ m_points; }
    VlcSources& sources() { फर्काऊ m_sources; }

    // METHODS
    शून्य annotate(अचर string& स_नाम);
    शून्य printHierarchyReport();
    शून्य printTypeSummary();
    शून्य readCoverage(अचर string& filename, bool nonfatal = false);
    शून्य writeCoverage(अचर string& filename);
    शून्य writeInfo(अचर string& filename);

    शून्य rank();
};

//######################################################################

#endif  // guard
