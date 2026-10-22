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
#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Hash.h"

//============================================================================

class V3Hasher final {
    // NODE STATE
    //  AstNode::user4()    -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    ध्रुव VNUser4InUse m_inuser4;

सार्वजनिक:
    // CONSTRUCTORS
    V3Hasher() = स्वभाव;
    ~V3Hasher() = स्वभाव;

    // METHODS

    // Compute hash of node. This method caches the hash in the node's user4().
    V3Hash operator()(Ast_गाठ* nodep) ध्रुव;

    // Re-compute hash of this node, discarding cached value, but used cached hash of children.
    V3Hash rehash(Ast_गाठ* nodep) ध्रुव;

    // Compute hash of node, without caching in user4.
    स्थिर V3Hash uncachedHash(ध्रुव Ast_गाठ* nodep);
};

#endif  // Guard
