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

#ifndef VERILATOR_V3HASHER_H_
#define VERILATOR_V3HASHER_H_
#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Hash.h"

//============================================================================

class V3Hasher final {
    // NODE STATE
    //  AstNode::user4()    -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    ধ্রুব VNUser4InUse m_inuser4;

সার্বজনিক:
    // CONSTRUCTORS
    V3Hasher() = স্বভাব;
    ~V3Hasher() = স্বভাব;

    // METHODS

    // Compute hash of node. This method caches the hash in the node's user4().
    V3Hash operator()(Ast_গ্রন্থি* nodep) ধ্রুব;

    // Re-compute hash of this node, discarding cached value, but used cached hash of children.
    V3Hash rehash(Ast_গ্রন্থি* nodep) ধ্রুব;

    // Compute hash of node, without caching in user4.
    স্থির V3Hash uncachedHash(ধ্রুব Ast_গ্রন্থি* nodep);
};

#endif  // Guard
