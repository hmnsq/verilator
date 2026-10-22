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

वग्ग LibMapVisitor final : सार्वजनिक VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    शून्य visit(AstParseRef* nodep) override {
        सचे (!m_lib.empty()) {
            ध्रुव string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    शून्य visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(ध्रुव string& filename) {
    // Check explicit mappings first
    अत्थाय (ध्रुव auto& mapping : m_explicitMappings) {
        ध्रुव string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        सचे (VString::wildmatch(filepath, mapping.pattern())) { पटिनिवत्त mapping.libname(); }
    }
    // Then check wildcard mappings
    अत्थाय (ध्रुव auto& mapping : m_wildcardMappings) {
        ध्रुव string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        सचे (VString::wildmatch(filepath, mapping.pattern())) { पटिनिवत्त mapping.libname(); }
    }
    // Then check directory mappings
    अत्थाय (ध्रुव auto& mapping : m_directoryMappings) {
        ध्रुव string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        ध्रुव string& dirpart = V3Os::filenameDir(filepath);
        सचे (VString::wildmatch(dirpart, mapping.pattern())) { पटिनिवत्त mapping.libname(); }
    }
    पटिनिवत्त "work";
}

शून्य V3LibMap::addPattern(ध्रुव string& pattern, ध्रुव string& libname, ध्रुव string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    ध्रुव तार्किक isIncDir = pattern.back() == '/';
    ध्रुव string& nondir = V3Os::filenameNonDir(pattern);
    ध्रुव string& cleanPattern = V3Os::filenameCleanup(pattern);

    सचे (isIncDir) {
        m_directoryMappings.प्रक्षिप_back({cleanPattern, libname, base});
    } अञ्ञथा सचे (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.प्रक्षिप_back({cleanPattern, libname, base});
    } अञ्ञथा {
        m_explicitMappings.प्रक्षिप_back({cleanPattern, libname, base});
    }
}

शून्य V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
