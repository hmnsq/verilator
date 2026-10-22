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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3_CLASS_ਗ੍ਰਾਫ_H
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3_CLASS_ਗ੍ਰਾਫ_H

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

//============================================================================

class V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼;

// Graph of classes - build by V3ClassGraph::build() and used to get CFuncs that may be
// called by a AstNodeCCall (due to virtualization)
class V3_Class_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    ਮਿੱਤਰ class Class_ਗ੍ਰਾਫ_Builder_Visitor;
    std::unordered_map<ਅਚਲ AstCFunc*, V3_Class_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*>
        m_member_Func_ਮੰਜ਼ਿਲ_Class_ਸ਼ੀਰਸ਼;  // Map from member function to its class
    ਅਚਲ std::unordered_ਸੈੱਟ<AstCFunc*>
        m_ਖਾਲੀ_ਥਾਵਾਂ_ਸੈੱਟ;  // Always empty set - used to return a reference to an empty set

    V3_Class_ਗ੍ਰਾਫ() = ਮੂਲ;

ਜਨਤਕ:
    ਸਥਿਰ std::unique_ptr<V3_Class_ਗ੍ਰਾਫ> build(AstNetlist*);
    ~V3_Class_ਗ੍ਰਾਫ() override = ਮੂਲ;

    // Returns possible CFuncs that may be called due to a NodeCCall passed as a callp argument
    // Warning: returned set may be empty if NodeCCall is a New call or a super call - in such
    // cases only `callp->funcp()` may be called. Also, if callp is to a non-member function the
    // set will be empty - post V3Task static functions are not member functions anymore.
    ਅਚਲ std::unordered_ਸੈੱਟ<AstCFunc*>&
    getCallPossibleCFuncs(ਅਚਲ Ast_ਗੰਢ_CCall* ਅਚਲ callp) ਅਚਲ&;
};

#ਜੇ_ਅੰਤ  // Guard
