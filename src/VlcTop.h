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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "VlcOptions.h"
#ضمن "VlcPoint.h"
#ضمن "VlcSource.h"
#ضمن "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_لووتکە final {
عام:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
خاص:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    فراغ annotateCalc();
    فراغ annotateCalcNeeded();
    فراغ annotateOutputFiles(ثابت string& dirname);

عام:
    // CONSTRUCTORS
    Vlc_لووتکە() = افتراضي;
    ~Vlc_لووتکە() = افتراضي;

    // ACCESSORS
    VlcTests& tests() { أرجع m_tests; }
    VlcPoints& points() { أرجع m_points; }
    VlcSources& sources() { أرجع m_sources; }

    // METHODS
    فراغ annotate(ثابت string& dirname);
    فراغ printHierarchyReport();
    فراغ printTypeSummary();
    فراغ readCoverage(ثابت string& filename, bool nonfatal = false);
    فراغ writeCoverage(ثابت string& filename);
    فراغ writeInfo(ثابت string& filename);

    فراغ rank();
};

//######################################################################

#endif  // guard
