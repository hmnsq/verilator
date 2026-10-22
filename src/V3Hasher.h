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

#अगर_अघोषित VERILATOR_V3HASHER_H_
#घोषणा VERILATOR_V3HASHER_H_
#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Error.h"
#समावेश "V3Hash.h"

//============================================================================

class V3Hasher final {
    // NODE STATE
    //  AstNode::user4()    -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    स्थिर VNUser4InUse m_inuser4;

खुला:
    // CONSTRUCTORS
    V3Hasher() = शेष;
    ~V3Hasher() = शेष;

    // METHODS

    // Compute hash of node. This method caches the hash in the node's user4().
    V3Hash चालक()(Ast_गाँठ* nodep) स्थिर;

    // Re-compute hash of this node, discarding cached value, but used cached hash of children.
    V3Hash rehash(Ast_गाँठ* nodep) स्थिर;

    // Compute hash of node, without caching in user4.
    अटल V3Hash uncachedHash(स्थिर Ast_गाँठ* nodep);
};

#पूर्ण_अगर  // Guard
