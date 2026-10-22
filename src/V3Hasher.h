// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Hash AST trees to find duplicates
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
//  V3Hasher handles computation of AstNode hashes
//
//*************************************************************************

#నిర్వచితం_కాకపోతే VERILATOR_V3HASHER_H_
#నిర్వచించు VERILATOR_V3HASHER_H_
#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Hash.h"

//============================================================================

class V3Hasher final {
    // NODE STATE
    //  AstNode::user4()    -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    స్థిరరాశి VNUser4_లోపల_Use m_inuser4;

బహిరంగ:
    // CONSTRUCTORS
    V3Hasher() = అప్రమేయ;
    ~V3Hasher() = అప్రమేయ;

    // METHODS

    // Compute hash of node. This method caches the hash in the node's user4().
    V3Hash క్రియాకారకం()(Ast_కణుపు* nodep) స్థిరరాశి;

    // Re-compute hash of this node, discarding cached value, but used cached hash of children.
    V3Hash rehash(Ast_కణుపు* nodep) స్థిరరాశి;

    // Compute hash of node, without caching in user4.
    స్థిర V3Hash uncachedHash(స్థిరరాశి Ast_కణుపు* nodep);
};

#అయితే_ముగింపు  // Guard
