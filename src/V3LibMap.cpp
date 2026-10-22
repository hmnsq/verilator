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

جماعت LibMapVisitor final : عوامی VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    خالی دورہ(AstParseRef* nodep) override {
        اگر (!m_lib.خالی_خانے()) {
            ثابت string& نمونہ_2 = nodep->name();
            v3Global.libMapp()->جوڑو_نمونہ(نمونہ_2, m_lib, m_rel);
        }
    }

    خالی دورہ(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::مطابقت_Mapping(ثابت string& filename) {
    // Check explicit mappings first
    برائے (ثابت خودکار& mapping : m_explicitMappings) {
        ثابت string& filepath = V3Os::filename_Relative_راستہ(filename, mapping.بنیاد());
        اگر (VString::wildmatch(filepath, mapping.نمونہ_2())) { واپس mapping.libname(); }
    }
    // Then check wildcard mappings
    برائے (ثابت خودکار& mapping : m_wildcardMappings) {
        ثابت string& filepath = V3Os::filename_Relative_راستہ(filename, mapping.بنیاد());
        اگر (VString::wildmatch(filepath, mapping.نمونہ_2())) { واپس mapping.libname(); }
    }
    // Then check directory mappings
    برائے (ثابت خودکار& mapping : m_directoryMappings) {
        ثابت string& filepath = V3Os::filename_Relative_راستہ(filename, mapping.بنیاد());
        ثابت string& dirpart = V3Os::filenameDir(filepath);
        اگر (VString::wildmatch(dirpart, mapping.نمونہ_2())) { واپس mapping.libname(); }
    }
    واپس "work";
}

خالی V3LibMap::جوڑو_نمونہ(ثابت string& نمونہ_2, ثابت string& libname, ثابت string& بنیاد) {
    UINFO(4, __FUNCTION__ << ": pattern '" << نمونہ_2 << "' => library '" << libname << "'");

    ثابت منطقی isIncDir = نمونہ_2.عقب() == '/';
    ثابت string& nondir = V3Os::filenameNonDir(نمونہ_2);
    ثابت string& clean_نمونہ = V3Os::filenameCleanup(نمونہ_2);

    اگر (isIncDir) {
        m_directoryMappings.ڈالو_عقب({clean_نمونہ, libname, بنیاد});
    } ورنہ اگر (nondir.ڈھونڈو('*') != string::npos || nondir.ڈھونڈو('?') != string::npos) {
        m_wildcardMappings.ڈالو_عقب({clean_نمونہ, libname, بنیاد});
    } ورنہ {
        m_explicitMappings.ڈالو_عقب({clean_نمونہ, libname, بنیاد});
    }
}

خالی V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
