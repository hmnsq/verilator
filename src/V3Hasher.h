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

#اگر_تعریف_نشده VERILATOR_V3HASHER_H_
#تعریف_کن VERILATOR_V3HASHER_H_
#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3Hash.h"

//============================================================================

class V3Hasher final {
    // NODE STATE
    //  AstNode::user4()    -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    ثابت VNUser4_درون_Use m_inuser4;

عام:
    // CONSTRUCTORS
    V3Hasher() = افتراضي;
    ~V3Hasher() = افتراضي;

    // METHODS

    // Compute hash of node. This method caches the hash in the node's user4().
    V3Hash عملگر()(Ast_گره* nodep) ثابت;

    // Re-compute hash of this node, discarding cached value, but used cached hash of children.
    V3Hash rehash(Ast_گره* nodep) ثابت;

    // Compute hash of node, without caching in user4.
    ساكن V3Hash uncachedHash(ثابت Ast_گره* nodep);
};

#پایان_اگر  // Guard
