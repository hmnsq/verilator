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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LibMap.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Options.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3Parse.h"
#ಅನ್ತರ್ಭಾವ "V3SymTable.h"

#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ LibMapVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    string m_lib;  // Library currently being mapped
    string m_rel;  // Relative path of current libmap file

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstParseRef* nodep) override {
        ಆದರೆ (!m_lib.ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ string& ನಮೂನೆ_2 = nodep->name();
            v3Global.libMapp()->ಕೂಡಿಸು_ನಮೂನೆ(ನಮೂನೆ_2, m_lib, m_rel);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstLibrary* nodep) override {
        VL_RESTORER_CLEAR(m_lib);
        VL_RESTORER_CLEAR(m_rel);
        m_lib = nodep->name();
        m_rel = V3Os::filenameDir(nodep->fileline()->filename());
        iterateChildren(nodep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ LibMapVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// LibMap class functions

string V3LibMap::ಹೊಂದಿಕೆ_Mapping(ಸ್ಥಿರಾಂಕ string& filename) {
    // Check explicit mappings first
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& mapping : m_explicitMappings) {
        ಸ್ಥಿರಾಂಕ string& filepath = V3Os::filename_Relative_ಮಾರ್ಗ(filename, mapping.ತಳ());
        ಆದರೆ (VString::wildmatch(filepath, mapping.ನಮೂನೆ_2())) { ಹಿಂತಿರುಗಿ mapping.libname(); }
    }
    // Then check wildcard mappings
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& mapping : m_wildcardMappings) {
        ಸ್ಥಿರಾಂಕ string& filepath = V3Os::filename_Relative_ಮಾರ್ಗ(filename, mapping.ತಳ());
        ಆದರೆ (VString::wildmatch(filepath, mapping.ನಮೂನೆ_2())) { ಹಿಂತಿರುಗಿ mapping.libname(); }
    }
    // Then check directory mappings
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& mapping : m_directoryMappings) {
        ಸ್ಥಿರಾಂಕ string& filepath = V3Os::filename_Relative_ಮಾರ್ಗ(filename, mapping.ತಳ());
        ಸ್ಥಿರಾಂಕ string& dirpart = V3Os::filenameDir(filepath);
        ಆದರೆ (VString::wildmatch(dirpart, mapping.ನಮೂನೆ_2())) { ಹಿಂತಿರುಗಿ mapping.libname(); }
    }
    ಹಿಂತಿರುಗಿ "work";
}

ಶೂನ್ಯ V3LibMap::ಕೂಡಿಸು_ನಮೂನೆ(ಸ್ಥಿರಾಂಕ string& ನಮೂನೆ_2, ಸ್ಥಿರಾಂಕ string& libname, ಸ್ಥಿರಾಂಕ string& ತಳ) {
    UINFO(4, __FUNCTION__ << ": pattern '" << ನಮೂನೆ_2 << "' => library '" << libname << "'");

    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isIncDir = ನಮೂನೆ_2.ಹಿಂಭಾಗ() == '/';
    ಸ್ಥಿರಾಂಕ string& nondir = V3Os::filenameNonDir(ನಮೂನೆ_2);
    ಸ್ಥಿರಾಂಕ string& clean_ನಮೂನೆ = V3Os::filenameCleanup(ನಮೂನೆ_2);

    ಆದರೆ (isIncDir) {
        m_directoryMappings.ತಳ್ಳು_ಹಿಂಭಾಗ({clean_ನಮೂನೆ, libname, ತಳ});
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nondir.ಕಂಡುಹಿಡಿ('*') != string::npos || nondir.ಕಂಡುಹಿಡಿ('?') != string::npos) {
        m_wildcardMappings.ತಳ್ಳು_ಹಿಂಭಾಗ({clean_ನಮೂನೆ, libname, ತಳ});
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_explicitMappings.ತಳ್ಳು_ಹಿಂಭಾಗ({clean_ನಮೂನೆ, libname, ತಳ});
    }
}

ಶೂನ್ಯ V3LibMap::map(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    { LibMapVisitor{nodep}; }
}
