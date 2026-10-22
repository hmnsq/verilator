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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Graph.h"

#অন্তর্ভাব <memory>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>

//============================================================================

class V3ClassGraphVertex;

// Graph of classes - build by V3ClassGraph::build() and used to get CFuncs that may be
// called by a AstNodeCCall (due to virtualization)
class V3ClassGraph final : সার্বজনিক V3Graph {
    friend class ClassGraphBuilderVisitor;
    std::unordered_map<ধ্রুব AstCFunc*, V3ClassGraphVertex*>
        m_memberFuncToClassVertex;  // Map from member function to its class
    ধ্রুব std::unordered_set<AstCFunc*>
        m_emptySet;  // Always empty set - used to return a reference to an empty set

    V3ClassGraph() = স্বভাব;

সার্বজনিক:
    স্থির std::unique_ptr<V3ClassGraph> build(AstNetlist*);
    ~V3ClassGraph() override = স্বভাব;

    // Returns possible CFuncs that may be called due to a NodeCCall passed as a callp argument
    // Warning: returned set may be empty if NodeCCall is a New call or a super call - in such
    // cases only `callp->funcp()` may be called. Also, if callp is to a non-member function the
    // set will be empty - post V3Task static functions are not member functions anymore.
    ধ্রুব std::unordered_set<AstCFunc*>&
    getCallPossibleCFuncs(ধ্রুব Ast_গ্রন্থি_CCall* ধ্রুব callp) ধ্রুব&;
};

#endif  // Guard
