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
    فراغ لیدنه(AstParseRef* nodep) override {
        إذا (!m_lib.تشه()) {
            ثابت string& نمونه_2 = nodep->name();
            v3Global.libMapp()->جمع_کړه_نمونه(نمونه_2, m_lib, m_rel);
        }
    }

    فراغ لیدنه(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::سمون_Mapping(ثابت string& filename) {
    // Check explicit mappings first
    لكل (ثابت خپلکاری& mapping : m_explicitMappings) {
        ثابت string& filepath = V3Os::filename_Relative_لار(filename, mapping.بنسټ());
        إذا (VString::wildmatch(filepath, mapping.نمونه_2())) { أرجع mapping.libname(); }
    }
    // Then check wildcard mappings
    لكل (ثابت خپلکاری& mapping : m_wildcardMappings) {
        ثابت string& filepath = V3Os::filename_Relative_لار(filename, mapping.بنسټ());
        إذا (VString::wildmatch(filepath, mapping.نمونه_2())) { أرجع mapping.libname(); }
    }
    // Then check directory mappings
    لكل (ثابت خپلکاری& mapping : m_directoryMappings) {
        ثابت string& filepath = V3Os::filename_Relative_لار(filename, mapping.بنسټ());
        ثابت string& dirpart = V3Os::filenameDir(filepath);
        إذا (VString::wildmatch(dirpart, mapping.نمونه_2())) { أرجع mapping.libname(); }
    }
    أرجع "work";
}

فراغ V3LibMap::جمع_کړه_نمونه(ثابت string& نمونه_2, ثابت string& libname, ثابت string& بنسټ) {
    UINFO(4, __FUNCTION__ << ": pattern '" << نمونه_2 << "' => library '" << libname << "'");

    ثابت منطقي isIncDir = نمونه_2.شا() == '/';
    ثابت string& nondir = V3Os::filenameNonDir(نمونه_2);
    ثابت string& clean_نمونه = V3Os::filenameCleanup(نمونه_2);

    إذا (isIncDir) {
        m_directoryMappings.ټېل_شا({clean_نمونه, libname, بنسټ});
    } وإلا إذا (nondir.پیدا_کړه('*') != string::npos || nondir.پیدا_کړه('?') != string::npos) {
        m_wildcardMappings.ټېل_شا({clean_نمونه, libname, بنسټ});
    } وإلا {
        m_explicitMappings.ټېل_شا({clean_نمونه, libname, بنسټ});
    }
}

فراغ V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
