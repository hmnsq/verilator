// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link modules/signals together
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

#నిర్వచితం_కాకపోతే VERILATOR_V3LIBMAP_H_
#నిర్వచించు VERILATOR_V3LIBMAP_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    స్థిరరాశి string m_నమూనా;  // Pattern to match
    స్థిరరాశి string m_libname;  // Library name
    స్థిరరాశి string m_పునాది;  // Relative path of the libmap file

బహిరంగ:
    LibMapping(స్థిరరాశి string& నమూనా_3, స్థిరరాశి string& libname_, స్థిరరాశి string& పునాది_2)
        : m_నమూనా{నమూనా_3}
        , m_libname{libname_}
        , m_పునాది{పునాది_2} {}
    string నమూనా_2() స్థిరరాశి { తిరిగి m_నమూనా; }
    string libname() స్థిరరాశి { తిరిగి m_libname; }
    string పునాది() స్థిరరాశి { తిరిగి m_పునాది; }
};

class V3LibMap final {
అంతరంగ:
    // STATE
    // 33.3.1.1 File path resolution
    //   If a file name potentially matches multiple file path specifications, the path
    //   specifications shall be resolved in the following order: a) File path specifications that
    //   end with an explicit file name b) File path specifications that end with a wildcarded file
    //   name c) File path specifications that end with a directory name
    std::సదిశ<LibMapping> m_explicitMappings;
    std::సదిశ<LibMapping> m_wildcardMappings;
    std::సదిశ<LibMapping> m_directoryMappings;

    // METHODS
బహిరంగ:
    string సరిపోలిక_Mapping(స్థిరరాశి string& filename);
    శూన్య కూడు_నమూనా(స్థిరరాశి string& నమూనా_2, స్థిరరాశి string& libname, స్థిరరాశి string& పునాది);

    స్థిర శూన్య map(AstNetlist* nodep) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
