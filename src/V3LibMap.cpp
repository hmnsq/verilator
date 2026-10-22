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
    शून्य दर्शय(AstParseRef* nodep) override {
        यदि (!m_lib.रिक्तानि()) {
            ध्रुव string& आदर्शः_2 = nodep->name();
            v3Global.libMapp()->सङ्कलय_आदर्शः(आदर्शः_2, m_lib, m_rel);
        }
    }

    शून्य दर्शय(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::मेलनम्_Mapping(ध्रुव string& filename) {
    // Check explicit mappings first
    प्रति (ध्रुव स्वचालित& mapping : m_explicitMappings) {
        ध्रुव string& filepath = V3Os::filename_Relative_मार्गः(filename, mapping.आधारः());
        यदि (VString::wildmatch(filepath, mapping.आदर्शः_2())) { प्रत्यागम mapping.libname(); }
    }
    // Then check wildcard mappings
    प्रति (ध्रुव स्वचालित& mapping : m_wildcardMappings) {
        ध्रुव string& filepath = V3Os::filename_Relative_मार्गः(filename, mapping.आधारः());
        यदि (VString::wildmatch(filepath, mapping.आदर्शः_2())) { प्रत्यागम mapping.libname(); }
    }
    // Then check directory mappings
    प्रति (ध्रुव स्वचालित& mapping : m_directoryMappings) {
        ध्रुव string& filepath = V3Os::filename_Relative_मार्गः(filename, mapping.आधारः());
        ध्रुव string& dirpart = V3Os::filenameDir(filepath);
        यदि (VString::wildmatch(dirpart, mapping.आदर्शः_2())) { प्रत्यागम mapping.libname(); }
    }
    प्रत्यागम "work";
}

शून्य V3LibMap::सङ्कलय_आदर्शः(ध्रुव string& आदर्शः_2, ध्रुव string& libname, ध्रुव string& आधारः) {
    UINFO(4, __FUNCTION__ << ": pattern '" << आदर्शः_2 << "' => library '" << libname << "'");

    ध्रुव तार्किक isIncDir = आदर्शः_2.पृष्ठभागः() == '/';
    ध्रुव string& nondir = V3Os::filenameNonDir(आदर्शः_2);
    ध्रुव string& clean_आदर्शः = V3Os::filenameCleanup(आदर्शः_2);

    यदि (isIncDir) {
        m_directoryMappings.प्रक्षिप_पृष्ठभागः({clean_आदर्शः, libname, आधारः});
    } अन्यथा यदि (nondir.find('*') != string::npos || nondir.find('?') != string::npos) {
        m_wildcardMappings.प्रक्षिप_पृष्ठभागः({clean_आदर्शः, libname, आधारः});
    } अन्यथा {
        m_explicitMappings.प्रक्षिप_पृष्ठभागः({clean_आदर्शः, libname, आधारः});
    }
}

शून्य V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
