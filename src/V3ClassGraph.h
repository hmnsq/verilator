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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Graph.h"

#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <unordered_set>

//============================================================================

class V3ClassGraphVertex;

// Graph of classes - build by V3ClassGraph::build() and used to get CFuncs that may be
// called by a AstNodeCCall (due to virtualization)
class V3ClassGraph final : সার্ৱজনিক V3Graph {
    friend class ClassGraphBuilderVisitor;
    std::unordered_map<ধ্রুৱ AstCFunc*, V3ClassGraphVertex*>
        m_memberFuncToClassVertex;  // Map from member function to its class
    ধ্রুৱ std::unordered_set<AstCFunc*>
        m_emptySet;  // Always empty set - used to return a reference to an empty set

    V3ClassGraph() = স্ৱভাৱ;

সার্ৱজনিক:
    স্থির std::unique_ptr<V3ClassGraph> build(AstNetlist*);
    ~V3ClassGraph() override = স্ৱভাৱ;

    // Returns possible CFuncs that may be called due to a NodeCCall passed as a callp argument
    // Warning: returned set may be empty if NodeCCall is a New call or a super call - in such
    // cases only `callp->funcp()` may be called. Also, if callp is to a non-member function the
    // set will be empty - post V3Task static functions are not member functions anymore.
    ধ্রুৱ std::unordered_set<AstCFunc*>&
    getCallPossibleCFuncs(ধ্রুৱ Ast_গ্ৰন্থি_CCall* ধ্রুৱ callp) ধ্রুৱ&;
};

#endif  // Guard
