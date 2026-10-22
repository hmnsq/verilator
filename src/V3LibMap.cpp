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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LibMap.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Options.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Parse.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SymTable.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ LibMapVisitor final : ਜਨਤਕ VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstParseRef* nodep) override {
        ਜੇ (!m_lib.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ string& ਨਮੂਨਾ_2 = nodep->name();
            v3Global.libMapp()->ਜੋੜੋ_ਨਮੂਨਾ(ਨਮੂਨਾ_2, m_lib, m_rel);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::ਮੇਲ_Mapping(ਅਚਲ string& filename) {
    // Check explicit mappings first
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& mapping : m_explicitMappings) {
        ਅਚਲ string& filepath = V3Os::filename_Relative_ਰਸਤਾ(filename, mapping.ਅਧਾਰ());
        ਜੇ (VString::wildmatch(filepath, mapping.ਨਮੂਨਾ_2())) { ਵਾਪਸ mapping.libname(); }
    }
    // Then check wildcard mappings
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& mapping : m_wildcardMappings) {
        ਅਚਲ string& filepath = V3Os::filename_Relative_ਰਸਤਾ(filename, mapping.ਅਧਾਰ());
        ਜੇ (VString::wildmatch(filepath, mapping.ਨਮੂਨਾ_2())) { ਵਾਪਸ mapping.libname(); }
    }
    // Then check directory mappings
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& mapping : m_directoryMappings) {
        ਅਚਲ string& filepath = V3Os::filename_Relative_ਰਸਤਾ(filename, mapping.ਅਧਾਰ());
        ਅਚਲ string& dirpart = V3Os::filenameDir(filepath);
        ਜੇ (VString::wildmatch(dirpart, mapping.ਨਮੂਨਾ_2())) { ਵਾਪਸ mapping.libname(); }
    }
    ਵਾਪਸ "work";
}

ਖਾਲੀ V3LibMap::ਜੋੜੋ_ਨਮੂਨਾ(ਅਚਲ string& ਨਮੂਨਾ_2, ਅਚਲ string& libname, ਅਚਲ string& ਅਧਾਰ) {
    UINFO(4, __FUNCTION__ << ": pattern '" << ਨਮੂਨਾ_2 << "' => library '" << libname << "'");

    ਅਚਲ ਤਾਰਕਿਕ isIncDir = ਨਮੂਨਾ_2.ਪਿੱਠ() == '/';
    ਅਚਲ string& nondir = V3Os::filenameNonDir(ਨਮੂਨਾ_2);
    ਅਚਲ string& clean_ਨਮੂਨਾ = V3Os::filenameCleanup(ਨਮੂਨਾ_2);

    ਜੇ (isIncDir) {
        m_directoryMappings.ਧੱਕੋ_ਪਿੱਠ({clean_ਨਮੂਨਾ, libname, ਅਧਾਰ});
    } ਨਹੀਂਤਾਂ ਜੇ (nondir.ਲੱਭੋ('*') != string::npos || nondir.ਲੱਭੋ('?') != string::npos) {
        m_wildcardMappings.ਧੱਕੋ_ਪਿੱਠ({clean_ਨਮੂਨਾ, libname, ਅਧਾਰ});
    } ਨਹੀਂਤਾਂ {
        m_explicitMappings.ਧੱਕੋ_ਪਿੱਠ({clean_ਨਮੂਨਾ, libname, ਅਧਾਰ});
    }
}

ਖਾਲੀ V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
