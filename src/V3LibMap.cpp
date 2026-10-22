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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LibMap.h"

#شامل "V3Graph.h"
#شامل "V3Options.h"
#شامل "V3Os.h"
#شامل "V3Parse.h"
#شامل "V3SymTable.h"

#شامل <unordered_set>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

رده LibMapVisitor final : عمومی VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    تهی visit(AstParseRef* nodep) override {
        اگر (!m_lib.empty()) {
            ثابت string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    تهی visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(ثابت string& filename) {
    // Check explicit mappings first
    برای (ثابت auto& mapping : m_explicitMappings) {
        ثابت string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        اگر (VString::wildmatch(filepath, mapping.pattern())) { بازگشت mapping.libname(); }
    }
    // Then check wildcard mappings
    برای (ثابت auto& mapping : m_wildcardMappings) {
        ثابت string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        اگر (VString::wildmatch(filepath, mapping.pattern())) { بازگشت mapping.libname(); }
    }
    // Then check directory mappings
    برای (ثابت auto& mapping : m_directoryMappings) {
        ثابت string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        ثابت string& dirpart = V3Os::filenameDir(filepath);
        اگر (VString::wildmatch(dirpart, mapping.pattern())) { بازگشت mapping.libname(); }
    }
    بازگشت "work";
}

تهی V3LibMap::addPattern(ثابت string& pattern, ثابت string& libname, ثابت string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    ثابت منطقی isIncDir = pattern.back() == '/';
    ثابت string& nondir = V3Os::filenameNonDir(pattern);
    ثابت string& cleanPattern = V3Os::filenameCleanup(pattern);

    اگر (isIncDir) {
        m_directoryMappings.افزودن_back({cleanPattern, libname, base});
    } وگرنه اگر (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.افزودن_back({cleanPattern, libname, base});
    } وگرنه {
        m_explicitMappings.افزودن_back({cleanPattern, libname, base});
    }
}

تهی V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
