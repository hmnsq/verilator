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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3LibMap.h"

#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3Options.h"
#অন্তর্ভাৱ "V3Os.h"
#অন্তর্ভাৱ "V3Parse.h"
#অন্তর্ভাৱ "V3SymTable.h"

#অন্তর্ভাৱ <unordered_set>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

শ্ৰেণী LibMapVisitor final : সার্ৱজনিক VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    শূন্য visit(AstParseRef* nodep) override {
        যদি (!m_lib.empty()) {
            ধ্রুৱ string& pattern = nodep->name();
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

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(ধ্রুৱ string& filename) {
    // Check explicit mappings first
    বাবে (ধ্রুৱ auto& mapping : m_explicitMappings) {
        ধ্রুৱ string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        যদি (VString::wildmatch(filepath, mapping.pattern())) { ঘূৰাই mapping.libname(); }
    }
    // Then check wildcard mappings
    বাবে (ধ্রুৱ auto& mapping : m_wildcardMappings) {
        ধ্রুৱ string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        যদি (VString::wildmatch(filepath, mapping.pattern())) { ঘূৰাই mapping.libname(); }
    }
    // Then check directory mappings
    বাবে (ধ্রুৱ auto& mapping : m_directoryMappings) {
        ধ্রুৱ string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        ধ্রুৱ string& dirpart = V3Os::filenameDir(filepath);
        যদি (VString::wildmatch(dirpart, mapping.pattern())) { ঘূৰাই mapping.libname(); }
    }
    ঘূৰাই "work";
}

শূন্য V3LibMap::addPattern(ধ্রুৱ string& pattern, ধ্রুৱ string& libname, ধ্রুৱ string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    ধ্রুৱ তার্কিক isIncDir = pattern.back() == '/';
    ধ্রুৱ string& nondir = V3Os::filenameNonDir(pattern);
    ধ্রুৱ string& cleanPattern = V3Os::filenameCleanup(pattern);

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
