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

#నిర్వచితం_కాకపోతే VERILATOR_V3_CLASS_లేఖాచిత్రం_H
#నిర్వచించు VERILATOR_V3_CLASS_లేఖాచిత్రం_H

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Graph.h"

#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

//============================================================================

class V3_Class_లేఖాచిత్రం_శీర్షం;

// Graph of classes - build by V3ClassGraph::build() and used to get CFuncs that may be
// called by a AstNodeCCall (due to virtualization)
class V3_Class_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
    మిత్ర class Class_లేఖాచిత్రం_Builder_Visitor;
    std::unordered_map<స్థిరరాశి AstCFunc*, V3_Class_లేఖాచిత్రం_శీర్షం*>
        m_member_Func_గమ్యం_Class_శీర్షం;  // Map from member function to its class
    స్థిరరాశి std::unordered_సమితి<AstCFunc*>
        m_ఖాళీలు_సమితి;  // Always empty set - used to return a reference to an empty set

    V3_Class_లేఖాచిత్రం() = అప్రమేయ;

బహిరంగ:
    స్థిర std::unique_ptr<V3_Class_లేఖాచిత్రం> build(AstNetlist*);
    ~V3_Class_లేఖాచిత్రం() override = అప్రమేయ;

    // Returns possible CFuncs that may be called due to a NodeCCall passed as a callp argument
    // Warning: returned set may be empty if NodeCCall is a New call or a super call - in such
    // cases only `callp->funcp()` may be called. Also, if callp is to a non-member function the
    // set will be empty - post V3Task static functions are not member functions anymore.
    స్థిరరాశి std::unordered_సమితి<AstCFunc*>&
    getCallPossibleCFuncs(స్థిరరాశి Ast_కణుపు_CCall* స్థిరరాశి callp) స్థిరరాశి&;
};

#అయితే_ముగింపు  // Guard
