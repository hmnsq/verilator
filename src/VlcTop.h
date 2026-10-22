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

#अगर_अघोषित VERILATOR_VLCTOP_H_
#घोषणा VERILATOR_VLCTOP_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "VlcOptions.h"
#समावेश "VlcPoint.h"
#समावेश "VlcSource.h"
#समावेश "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_शीर्ष final {
खुला:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
निजी:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    व्योम annotateCalc();
    व्योम annotateCalcNeeded();
    व्योम annotateOutputFiles(स्थिर string& स_नाम);

खुला:
    // CONSTRUCTORS
    Vlc_शीर्ष() = शेष;
    ~Vlc_शीर्ष() = शेष;

    // ACCESSORS
    VlcTests& tests() { वापस m_tests; }
    VlcPoints& points() { वापस m_points; }
    VlcSources& sources() { वापस m_sources; }

    // METHODS
    व्योम annotate(स्थिर string& स_नाम);
    व्योम printHierarchyReport();
    व्योम printTypeSummary();
    व्योम readCoverage(स्थिर string& filename, bool nonfatal = false);
    व्योम writeCoverage(स्थिर string& filename);
    व्योम writeInfo(स्थिर string& filename);

    व्योम rank();
};

//######################################################################

#पूर्ण_अगर  // guard
