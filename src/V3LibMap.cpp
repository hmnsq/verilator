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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3LibMap.h"

#समावेश "V3Graph.h"
#समावेश "V3Options.h"
#समावेश "V3Os.h"
#समावेश "V3Parse.h"
#समावेश "V3SymTable.h"

#समावेश <unordered_set>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी LibMapVisitor final : खुला VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    व्योम visit(AstParseRef* nodep) override {
        अगर (!m_lib.empty()) {
            स्थिर string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    व्योम visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(स्थिर string& filename) {
    // Check explicit mappings first
    क्रम (स्थिर स्वतः& mapping : m_explicitMappings) {
        स्थिर string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        अगर (VString::wildmatch(filepath, mapping.pattern())) { वापस mapping.libname(); }
    }
    // Then check wildcard mappings
    क्रम (स्थिर स्वतः& mapping : m_wildcardMappings) {
        स्थिर string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        अगर (VString::wildmatch(filepath, mapping.pattern())) { वापस mapping.libname(); }
    }
    // Then check directory mappings
    क्रम (स्थिर स्वतः& mapping : m_directoryMappings) {
        स्थिर string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        स्थिर string& dirpart = V3Os::filenameDir(filepath);
        अगर (VString::wildmatch(dirpart, mapping.pattern())) { वापस mapping.libname(); }
    }
    वापस "work";
}

व्योम V3LibMap::addPattern(स्थिर string& pattern, स्थिर string& libname, स्थिर string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    स्थिर bool isIncDir = pattern.back() == '/';
    स्थिर string& nondir = V3Os::filenameNonDir(pattern);
    स्थिर string& cleanPattern = V3Os::filenameCleanup(pattern);

    अगर (isIncDir) {
        m_directoryMappings.धकेलो_back({cleanPattern, libname, base});
    } अन्यथा अगर (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.धकेलो_back({cleanPattern, libname, base});
    } अन्यथा {
        m_explicitMappings.धकेलो_back({cleanPattern, libname, base});
    }
}

व्योम V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
