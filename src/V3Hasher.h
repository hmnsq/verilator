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

#اگر_غیر_تعریف_شدہ VERILATOR_V3HASHER_H_
#تعریف_کرو VERILATOR_V3HASHER_H_
#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3Hash.h"

//============================================================================

class V3Hasher final {
    // NODE STATE
    //  AstNode::user4()    -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    ثابت VNUser4_اندر_Use m_inuser4;

عوامی:
    // CONSTRUCTORS
    V3Hasher() = طےشدہ;
    ~V3Hasher() = طےشدہ;

    // METHODS

    // Compute hash of node. This method caches the hash in the node's user4().
    V3Hash عامل()(Ast_گرہ* nodep) ثابت;

    // Re-compute hash of this node, discarding cached value, but used cached hash of children.
    V3Hash rehash(Ast_گرہ* nodep) ثابت;

    // Compute hash of node, without caching in user4.
    ساکن V3Hash uncachedHash(ثابت Ast_گرہ* nodep);
};

#اگر_ختم  // Guard
