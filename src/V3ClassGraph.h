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

#اگر_غیر_تعریف_شدہ VERILATOR_V3_CLASS_گراف_H
#تعریف_کرو VERILATOR_V3_CLASS_گراف_H

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Graph.h"

#شامل <memory>
#شامل <unordered_map>
#شامل <unordered_set>

//============================================================================

class V3_Class_گراف_راس;

// Graph of classes - build by V3ClassGraph::build() and used to get CFuncs that may be
// called by a AstNodeCCall (due to virtualization)
class V3_Class_گراف final : عوامی V3_گراف {
    دوست class Class_گراف_Builder_Visitor;
    std::unordered_map<ثابت AstCFunc*, V3_Class_گراف_راس*>
        m_member_Func_مقصد_Class_راس;  // Map from member function to its class
    ثابت std::unordered_سیٹ<AstCFunc*>
        m_خالی_خانے_سیٹ;  // Always empty set - used to return a reference to an empty set

    V3_Class_گراف() = طےشدہ;

عوامی:
    ساکن std::unique_ptr<V3_Class_گراف> build(AstNetlist*);
    ~V3_Class_گراف() override = طےشدہ;

    // Returns possible CFuncs that may be called due to a NodeCCall passed as a callp argument
    // Warning: returned set may be empty if NodeCCall is a New call or a super call - in such
    // cases only `callp->funcp()` may be called. Also, if callp is to a non-member function the
    // set will be empty - post V3Task static functions are not member functions anymore.
    ثابت std::unordered_سیٹ<AstCFunc*>&
    getCallPossibleCFuncs(ثابت Ast_گرہ_CCall* ثابت callp) ثابت&;
};

#اگر_ختم  // Guard
