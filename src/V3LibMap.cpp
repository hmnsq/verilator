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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LibMap.h"

#ضمن "V3Graph.h"
#ضمن "V3Options.h"
#ضمن "V3Os.h"
#ضمن "V3Parse.h"
#ضمن "V3SymTable.h"

#ضمن <unordered_set>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف LibMapVisitor final : عام VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    فراغ visit(AstParseRef* nodep) override {
        إذا (!m_lib.empty()) {
            ثابت string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    فراغ visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(تولق_Deletep(nodep->unlinkFrBack()), nodep);
    }

    فراغ visit(Ast_نود* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(ثابت string& filename) {
    // Check explicit mappings first
    لكل (ثابت auto& mapping : m_explicitMappings) {
        ثابت string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        إذا (VString::wildmatch(filepath, mapping.pattern())) { أرجع mapping.libname(); }
    }
    // Then check wildcard mappings
    لكل (ثابت auto& mapping : m_wildcardMappings) {
        ثابت string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        إذا (VString::wildmatch(filepath, mapping.pattern())) { أرجع mapping.libname(); }
    }
    // Then check directory mappings
    لكل (ثابت auto& mapping : m_directoryMappings) {
        ثابت string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        ثابت string& dirpart = V3Os::filenameDir(filepath);
        إذا (VString::wildmatch(dirpart, mapping.pattern())) { أرجع mapping.libname(); }
    }
    أرجع "work";
}

فراغ V3LibMap::addPattern(ثابت string& pattern, ثابت string& libname, ثابت string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    ثابت منطقي isIncDir = pattern.back() == '/';
    ثابت string& nondir = V3Os::filenameNonDir(pattern);
    ثابت string& cleanPattern = V3Os::filenameCleanup(pattern);

    إذا (isIncDir) {
        m_directoryMappings.تولق_back({cleanPattern, libname, base});
    } وإلا إذا (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.تولق_back({cleanPattern, libname, base});
    } وإلا {
        m_explicitMappings.تولق_back({cleanPattern, libname, base});
    }
}

فراغ V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
