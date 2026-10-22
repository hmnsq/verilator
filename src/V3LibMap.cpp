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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LibMap.h"

#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Options.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3Parse.h"
#അന്തര്ഭാവ "V3SymTable.h"

#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം LibMapVisitor final : പൊതു VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    ശൂന്യം visit(AstParseRef* nodep) override {
        എങ്കിൽ (!m_lib.empty()) {
            സ്ഥിരം string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    ശൂന്യം visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(സ്ഥിരം string& filename) {
    // Check explicit mappings first
    വേണ്ടി (സ്ഥിരം auto& mapping : m_explicitMappings) {
        സ്ഥിരം string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        എങ്കിൽ (VString::wildmatch(filepath, mapping.pattern())) { തിരികെ mapping.libname(); }
    }
    // Then check wildcard mappings
    വേണ്ടി (സ്ഥിരം auto& mapping : m_wildcardMappings) {
        സ്ഥിരം string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        എങ്കിൽ (VString::wildmatch(filepath, mapping.pattern())) { തിരികെ mapping.libname(); }
    }
    // Then check directory mappings
    വേണ്ടി (സ്ഥിരം auto& mapping : m_directoryMappings) {
        സ്ഥിരം string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        സ്ഥിരം string& dirpart = V3Os::filenameDir(filepath);
        എങ്കിൽ (VString::wildmatch(dirpart, mapping.pattern())) { തിരികെ mapping.libname(); }
    }
    തിരികെ "work";
}

ശൂന്യം V3LibMap::addPattern(സ്ഥിരം string& pattern, സ്ഥിരം string& libname, സ്ഥിരം string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    സ്ഥിരം തർക്കം isIncDir = pattern.back() == '/';
    സ്ഥിരം string& nondir = V3Os::filenameNonDir(pattern);
    സ്ഥിരം string& cleanPattern = V3Os::filenameCleanup(pattern);

    എങ്കിൽ (isIncDir) {
        m_directoryMappings.തള്ളുക_back({cleanPattern, libname, base});
    } അല്ലെങ്കിൽ എങ്കിൽ (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.തള്ളുക_back({cleanPattern, libname, base});
    } അല്ലെങ്കിൽ {
        m_explicitMappings.തള്ളുക_back({cleanPattern, libname, base});
    }
}

ശൂന്യം V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
