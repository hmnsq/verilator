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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LibMap.h"

#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Options.h"
#অন্তর্ভাব "V3Os.h"
#অন্তর্ভাব "V3Parse.h"
#অন্তর্ভাব "V3SymTable.h"

#অন্তর্ভাব <unordered_set>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ LibMapVisitor final : সার্বজনিক VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    শূন্য visit(AstParseRef* nodep) override {
        যদি (!m_lib.empty()) {
            ধ্রুব string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    শূন্য visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(ধ্রুব string& filename) {
    // Check explicit mappings first
    প্রতি (ধ্রুব auto& mapping : m_explicitMappings) {
        ধ্রুব string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        যদি (VString::wildmatch(filepath, mapping.pattern())) { প্রত্যাগম mapping.libname(); }
    }
    // Then check wildcard mappings
    প্রতি (ধ্রুব auto& mapping : m_wildcardMappings) {
        ধ্রুব string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        যদি (VString::wildmatch(filepath, mapping.pattern())) { প্রত্যাগম mapping.libname(); }
    }
    // Then check directory mappings
    প্রতি (ধ্রুব auto& mapping : m_directoryMappings) {
        ধ্রুব string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        ধ্রুব string& dirpart = V3Os::filenameDir(filepath);
        যদি (VString::wildmatch(dirpart, mapping.pattern())) { প্রত্যাগম mapping.libname(); }
    }
    প্রত্যাগম "work";
}

শূন্য V3LibMap::addPattern(ধ্রুব string& pattern, ধ্রুব string& libname, ধ্রুব string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    ধ্রুব তার্কিক isIncDir = pattern.back() == '/';
    ধ্রুব string& nondir = V3Os::filenameNonDir(pattern);
    ধ্রুব string& cleanPattern = V3Os::filenameCleanup(pattern);

    যদি (isIncDir) {
        m_directoryMappings.ঠেলো_back({cleanPattern, libname, base});
    } অন্যথা যদি (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.ঠেলো_back({cleanPattern, libname, base});
    } অন্যথা {
        m_explicitMappings.ঠেলো_back({cleanPattern, libname, base});
    }
}

শূন্য V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
