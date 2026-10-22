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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LibMap.h"

#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Options.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3Parse.h"
#अन्तर्भाव "V3SymTable.h"

#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग LibMapVisitor final : सार्वजनिक VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    शून्य visit(AstParseRef* nodep) override {
        जर (!m_lib.empty()) {
            स्थिरांक string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    शून्य visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(स्थिरांक string& filename) {
    // Check explicit mappings first
    साठी (स्थिरांक स्वतः& mapping : m_explicitMappings) {
        स्थिरांक string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        जर (VString::wildmatch(filepath, mapping.pattern())) { परत mapping.libname(); }
    }
    // Then check wildcard mappings
    साठी (स्थिरांक स्वतः& mapping : m_wildcardMappings) {
        स्थिरांक string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        जर (VString::wildmatch(filepath, mapping.pattern())) { परत mapping.libname(); }
    }
    // Then check directory mappings
    साठी (स्थिरांक स्वतः& mapping : m_directoryMappings) {
        स्थिरांक string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        स्थिरांक string& dirpart = V3Os::filenameDir(filepath);
        जर (VString::wildmatch(dirpart, mapping.pattern())) { परत mapping.libname(); }
    }
    परत "work";
}

शून्य V3LibMap::addPattern(स्थिरांक string& pattern, स्थिरांक string& libname, स्थिरांक string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    स्थिरांक तार्किक isIncDir = pattern.back() == '/';
    स्थिरांक string& nondir = V3Os::filenameNonDir(pattern);
    स्थिरांक string& cleanPattern = V3Os::filenameCleanup(pattern);

    जर (isIncDir) {
        m_directoryMappings.ढकला_back({cleanPattern, libname, base});
    } अन्यथा जर (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.ढकला_back({cleanPattern, libname, base});
    } अन्यथा {
        m_explicitMappings.ढकला_back({cleanPattern, libname, base});
    }
}

शून्य V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
