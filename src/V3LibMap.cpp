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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3LibMap.h"

#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Options.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3Parse.h"
#அந்தர்பாவ "V3SymTable.h"

#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு LibMapVisitor final : பொது VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    வெறுமை visit(AstParseRef* nodep) override {
        எனில் (!m_lib.empty()) {
            மாறிலி string& pattern = nodep->name();
            v3Global.libMapp()->addPattern(pattern, m_lib, m_rel);
        }
    }

    வெறுமை visit(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::matchMapping(மாறிலி string& filename) {
    // Check explicit mappings first
    க்கு (மாறிலி auto& mapping : m_explicitMappings) {
        மாறிலி string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        எனில் (VString::wildmatch(filepath, mapping.pattern())) { திரும்பு mapping.libname(); }
    }
    // Then check wildcard mappings
    க்கு (மாறிலி auto& mapping : m_wildcardMappings) {
        மாறிலி string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        எனில் (VString::wildmatch(filepath, mapping.pattern())) { திரும்பு mapping.libname(); }
    }
    // Then check directory mappings
    க்கு (மாறிலி auto& mapping : m_directoryMappings) {
        மாறிலி string& filepath = V3Os::filenameRelativePath(filename, mapping.base());
        மாறிலி string& dirpart = V3Os::filenameDir(filepath);
        எனில் (VString::wildmatch(dirpart, mapping.pattern())) { திரும்பு mapping.libname(); }
    }
    திரும்பு "work";
}

வெறுமை V3LibMap::addPattern(மாறிலி string& pattern, மாறிலி string& libname, மாறிலி string& base) {
    UINFO(4, __FUNCTION__ << ": pattern '" << pattern << "' => library '" << libname << "'");

    மாறிலி தருக்க isIncDir = pattern.back() == '/';
    மாறிலி string& nondir = V3Os::filenameNonDir(pattern);
    மாறிலி string& cleanPattern = V3Os::filenameCleanup(pattern);

    எனில் (isIncDir) {
        m_directoryMappings.தள்ளு_back({cleanPattern, libname, base});
    } இல்லையெனில் எனில் (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.தள்ளு_back({cleanPattern, libname, base});
    } இல்லையெனில் {
        m_explicitMappings.தள்ளு_back({cleanPattern, libname, base});
    }
}

வெறுமை V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
