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

#నిర్వచితం_కాకపోతే VERILATOR_VLCTOP_H_
#నిర్వచించు VERILATOR_VLCTOP_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "VlcOptions.h"
#అన్తర్భావ "VlcPoint.h"
#అన్తర్భావ "VlcSource.h"
#అన్తర్భావ "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_శిఖరం final {
బహిరంగ:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
అంతరంగ:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    శూన్య annotateCalc();
    శూన్య annotateCalcNeeded();
    శూన్య annotateOutputFiles(స్థిరరాశి string& dirname);

బహిరంగ:
    // CONSTRUCTORS
    Vlc_శిఖరం() = అప్రమేయ;
    ~Vlc_శిఖరం() = అప్రమేయ;

    // ACCESSORS
    VlcTests& tests() { తిరిగి m_tests; }
    VlcPoints& points() { తిరిగి m_points; }
    VlcSources& sources() { తిరిగి m_sources; }

    // METHODS
    శూన్య annotate(స్థిరరాశి string& dirname);
    శూన్య printHierarchyReport();
    శూన్య printTypeSummary();
    శూన్య readCoverage(స్థిరరాశి string& filename, bool nonfatal = false);
    శూన్య writeCoverage(స్థిరరాశి string& filename);
    శూన్య writeInfo(స్థిరరాశి string& filename);

    శూన్య rank();
};

//######################################################################

#అయితే_ముగింపు  // guard
