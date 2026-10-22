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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "VlcOptions.h"
#അന്തര്ഭാവ "VlcPoint.h"
#അന്തര്ഭാവ "VlcSource.h"
#അന്തര്ഭാവ "VlcTest.h"

//######################################################################
// VlcTop - Top level options container

class Vlc_മുകൾ final {
പൊതു:
    // PUBLIC MEMBERS
    VlcOptions opt;  //< Runtime options
സ്വകാര്യ:
    // MEMBERS
    VlcTests m_tests;  //< List of all tests (all coverage files)
    VlcPoints m_points;  //< List of all points
    VlcSources m_sources;  //< List of all source files to annotate

    // METHODS
    ശൂന്യം annotateCalc();
    ശൂന്യം annotateCalcNeeded();
    ശൂന്യം annotateOutputFiles(സ്ഥിരം string& dirname);

പൊതു:
    // CONSTRUCTORS
    Vlc_മുകൾ() = സ്ഥിരസ്ഥിതി;
    ~Vlc_മുകൾ() = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    VlcTests& tests() { തിരികെ m_tests; }
    VlcPoints& points() { തിരികെ m_points; }
    VlcSources& sources() { തിരികെ m_sources; }

    // METHODS
    ശൂന്യം annotate(സ്ഥിരം string& dirname);
    ശൂന്യം printHierarchyReport();
    ശൂന്യം printTypeSummary();
    ശൂന്യം readCoverage(സ്ഥിരം string& filename, bool nonfatal = false);
    ശൂന്യം writeCoverage(സ്ഥിരം string& filename);
    ശൂന്യം writeInfo(സ്ഥിരം string& filename);

    ശൂന്യം rank();
};

//######################################################################

#endif  // guard
