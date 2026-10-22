// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Resolve module/signal name references
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
// Visitor to build library mapping patterns from --libmap files, which may
// be overridden on command line with --work. Patterns are relative to
// libmap file location.
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LibMap.h"

#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Options.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3Parse.h"
#అన్తర్భావ "V3SymTable.h"

#అన్తర్భావ <unordered_set>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి LibMapVisitor final : బహిరంగ VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    శూన్య సందర్శించు(AstParseRef* nodep) override {
        ఒకవేళ (!m_lib.ఖాళీలు()) {
            స్థిరరాశి string& నమూనా_2 = nodep->name();
            v3Global.libMapp()->కూడు_నమూనా(నమూనా_2, m_lib, m_rel);
        }
    }

    శూన్య సందర్శించు(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::సరిపోలిక_Mapping(స్థిరరాశి string& filename) {
    // Check explicit mappings first
    కొరకు (స్థిరరాశి స్వయంచాలక& mapping : m_explicitMappings) {
        స్థిరరాశి string& filepath = V3Os::filename_Relative_మార్గం(filename, mapping.పునాది());
        ఒకవేళ (VString::wildmatch(filepath, mapping.నమూనా_2())) { తిరిగి mapping.libname(); }
    }
    // Then check wildcard mappings
    కొరకు (స్థిరరాశి స్వయంచాలక& mapping : m_wildcardMappings) {
        స్థిరరాశి string& filepath = V3Os::filename_Relative_మార్గం(filename, mapping.పునాది());
        ఒకవేళ (VString::wildmatch(filepath, mapping.నమూనా_2())) { తిరిగి mapping.libname(); }
    }
    // Then check directory mappings
    కొరకు (స్థిరరాశి స్వయంచాలక& mapping : m_directoryMappings) {
        స్థిరరాశి string& filepath = V3Os::filename_Relative_మార్గం(filename, mapping.పునాది());
        స్థిరరాశి string& dirpart = V3Os::filenameDir(filepath);
        ఒకవేళ (VString::wildmatch(dirpart, mapping.నమూనా_2())) { తిరిగి mapping.libname(); }
    }
    తిరిగి "work";
}

శూన్య V3LibMap::కూడు_నమూనా(స్థిరరాశి string& నమూనా_2, స్థిరరాశి string& libname, స్థిరరాశి string& పునాది) {
    UINFO(4, __FUNCTION__ << ": pattern '" << నమూనా_2 << "' => library '" << libname << "'");

    స్థిరరాశి తార్కిక isIncDir = నమూనా_2.వెనుక() == '/';
    స్థిరరాశి string& nondir = V3Os::filenameNonDir(నమూనా_2);
    స్థిరరాశి string& clean_నమూనా = V3Os::filenameCleanup(నమూనా_2);

    ఒకవేళ (isIncDir) {
        m_directoryMappings.నెట్టు_వెనుక({clean_నమూనా, libname, పునాది});
    } లేకపోతే ఒకవేళ (nondir.కనుగొను('*') != string::npos || nondir.కనుగొను('?') != string::npos) {
        m_wildcardMappings.నెట్టు_వెనుక({clean_నమూనా, libname, పునాది});
    } లేకపోతే {
        m_explicitMappings.నెట్టు_వెనుక({clean_నమూనా, libname, పునాది});
    }
}

శూన్య V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
