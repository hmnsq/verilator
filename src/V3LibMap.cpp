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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LibMap.h"

#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Options.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3Parse.h"
#ଅନ୍ତର୍ଭାଵ "V3SymTable.h"

#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ LibMapVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    ଶୂନ୍ୟ visit(AstParseRef* nodep) override {
        ଯଦି (!m_lib.empty()) {
            ଅଚଳ string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    ଶୂନ୍ୟ visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(ଅଚଳ string& filename) {
    // Check explicit mappings first
    ପାଇଁ (ଅଚଳ auto& mapping : m_explicitMappings) {
        ଅଚଳ string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        ଯଦି (VString::wildmatch(filepath, mapping.pattern())) { ଫେରନ୍ତୁ mapping.libname(); }
    }
    // Then check wildcard mappings
    ପାଇଁ (ଅଚଳ auto& mapping : m_wildcardMappings) {
        ଅଚଳ string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        ଯଦି (VString::wildmatch(filepath, mapping.pattern())) { ଫେରନ୍ତୁ mapping.libname(); }
    }
    // Then check directory mappings
    ପାଇଁ (ଅଚଳ auto& mapping : m_directoryMappings) {
        ଅଚଳ string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        ଅଚଳ string& dirpart = V3Os::filenameDir(filepath);
        ଯଦି (VString::wildmatch(dirpart, mapping.pattern())) { ଫେରନ୍ତୁ mapping.libname(); }
    }
    ଫେରନ୍ତୁ "work";
}

ଶୂନ୍ୟ V3LibMap::addPattern(ଅଚଳ string& pattern, ଅଚଳ string& libname, ଅଚଳ string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    ଅଚଳ ତାର୍କିକ isIncDir = pattern.back() == '/';
    ଅଚଳ string& nondir = V3Os::filenameNonDir(pattern);
    ଅଚଳ string& cleanPattern = V3Os::filenameCleanup(pattern);

    ଯଦି (isIncDir) {
        m_directoryMappings.ଠେଲ_back({cleanPattern, libname, base});
    } ଅନ୍ୟଥା ଯଦି (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.ଠେଲ_back({cleanPattern, libname, base});
    } ଅନ୍ୟଥା {
        m_explicitMappings.ଠେଲ_back({cleanPattern, libname, base});
    }
}

ଶୂନ୍ୟ V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
