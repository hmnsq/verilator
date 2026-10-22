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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "VlcOptions.h"
#அந்தர்பாவ "VlcPoint.h"
#அந்தர்பாவ "VlcSource.h"
#அந்தர்பாவ "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_உச்சி final {
பொது:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
தனிப்பட்ட:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    வெறுமை annotateCalc();
    வெறுமை annotateCalcNeeded();
    வெறுமை annotateOutputFiles(மாறிலி string& dirname);

பொது:
    // CONSTRUCTORS
    Vlc_உச்சி() = இயல்பு;
    ~Vlc_உச்சி() = இயல்பு;

    // ACCESSORS
    VlcTests& tests() { திரும்பு m_tests; }
    VlcPoints& points() { திரும்பு m_points; }
    VlcSources& sources() { திரும்பு m_sources; }

    // METHODS
    வெறுமை annotate(மாறிலி string& dirname);
    வெறுமை printHierarchyReport();
    வெறுமை printTypeSummary();
    வெறுமை readCoverage(மாறிலி string& filename, bool nonfatal = false);
    வெறுமை writeCoverage(மாறிலி string& filename);
    வெறுமை writeInfo(மாறிலி string& filename);

    வெறுமை rank();
};

//######################################################################

#endif  // guard
