// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Virtual function calls - function finder
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3_CLASS_ನಕ್ಷೆ_H
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3_CLASS_ನಕ್ಷೆ_H

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

//============================================================================

class V3_Class_ನಕ್ಷೆ_ಶೃಂಗ;

// Graph of classes - build by V3ClassGraph::build() and used to get CFuncs that may be
// called by a AstNodeCCall (due to virtualization)
class V3_Class_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    ಮಿತ್ರ class Class_ನಕ್ಷೆ_Builder_Visitor;
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstCFunc*, V3_Class_ನಕ್ಷೆ_ಶೃಂಗ*>
        m_member_Func_ಗಮ್ಯ_Class_ಶೃಂಗ;  // Map from member function to its class
    ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<AstCFunc*>
        m_ಖಾಲಿಗಳು_ಗಣ;  // Always empty set - used to return a reference to an empty set

    V3_Class_ನಕ್ಷೆ() = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<V3_Class_ನಕ್ಷೆ> build(AstNetlist*);
    ~V3_Class_ನಕ್ಷೆ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // Returns possible CFuncs that may be called due to a NodeCCall passed as a callp argument
    // Warning: returned set may be empty if NodeCCall is a New call or a super call - in such
    // cases only `callp->funcp()` may be called. Also, if callp is to a non-member function the
    // set will be empty - post V3Task static functions are not member functions anymore.
    ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<AstCFunc*>&
    getCallPossibleCFuncs(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_CCall* ಸ್ಥಿರಾಂಕ callp) ಸ್ಥಿರಾಂಕ&;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
