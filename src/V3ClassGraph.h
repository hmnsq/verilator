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

#ifndef VERILATOR_V3CLASS_GRAPH_H_
#define VERILATOR_V3CLASS_GRAPH_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"

#ضمن <memory>
#ضمن <unordered_map>
#ضمن <unordered_set>

//============================================================================

class V3ClassGraphVertex;

// Graph of classes - build by V3ClassGraph::build() and used to get CFuncs that may be
// called by a AstNodeCCall (due to virtualization)
class V3ClassGraph final : عام V3Graph {
    friend class ClassGraphBuilderVisitor;
    std::unordered_map<ثابت AstCFunc*, V3ClassGraphVertex*>
        m_memberFuncToClassVertex;  // Map from member function to its class
    ثابت std::unordered_set<AstCFunc*>
        m_emptySet;  // Always empty set - used to return a reference to an empty set

    V3ClassGraph() = افتراضي;

عام:
    ساكن std::unique_ptr<V3ClassGraph> build(AstNetlist*);
    ~V3ClassGraph() override = افتراضي;

    // Returns possible CFuncs that may be called due to a NodeCCall passed as a callp argument
    // Warning: returned set may be empty if NodeCCall is a New call or a super call - in such
    // cases only `callp->funcp()` may be called. Also, if callp is to a non-member function the
    // set will be empty - post V3Task static functions are not member functions anymore.
    ثابت std::unordered_set<AstCFunc*>&
    getCallPossibleCFuncs(ثابت Ast_گرێ_CCall* ثابت callp) ثابت&;
};

#endif  // Guard
