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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LibMap.h"

#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Options.h"
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3Parse.h"
#અન્તર્ભાવ "V3SymTable.h"

#અન્તર્ભાવ <unordered_set>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ LibMapVisitor final : જાહેર VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    શૂન્ય visit(AstParseRef* nodep) override {
        જો (!m_lib.empty()) {
            અચલ string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    શૂન્ય visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(અચલ string& filename) {
    // Check explicit mappings first
    માટે (અચલ auto& mapping : m_explicitMappings) {
        અચલ string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        જો (VString::wildmatch(filepath, mapping.pattern())) { પાછા mapping.libname(); }
    }
    // Then check wildcard mappings
    માટે (અચલ auto& mapping : m_wildcardMappings) {
        અચલ string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        જો (VString::wildmatch(filepath, mapping.pattern())) { પાછા mapping.libname(); }
    }
    // Then check directory mappings
    માટે (અચલ auto& mapping : m_directoryMappings) {
        અચલ string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        અચલ string& dirpart = V3Os::filenameDir(filepath);
        જો (VString::wildmatch(dirpart, mapping.pattern())) { પાછા mapping.libname(); }
    }
    પાછા "work";
}

શૂન્ય V3LibMap::addPattern(અચલ string& pattern, અચલ string& libname, અચલ string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    અચલ તાર્કિક isIncDir = pattern.back() == '/';
    અચલ string& nondir = V3Os::filenameNonDir(pattern);
    અચલ string& cleanPattern = V3Os::filenameCleanup(pattern);

    જો (isIncDir) {
        m_directoryMappings.ધકેલો_back({cleanPattern, libname, base});
    } નહીંતર જો (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.ધકેલો_back({cleanPattern, libname, base});
    } નહીંતર {
        m_explicitMappings.ધકેલો_back({cleanPattern, libname, base});
    }
}

શૂન્ય V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
