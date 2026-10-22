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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3HASHER_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3HASHER_H_
#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hash.h"

//============================================================================

class V3Hasher final {
    // NODE STATE
    //  AstNode::user4()    -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    ਅਚਲ VNUser4_ਵਿੱਚ_Use m_inuser4;

ਜਨਤਕ:
    // CONSTRUCTORS
    V3Hasher() = ਮੂਲ;
    ~V3Hasher() = ਮੂਲ;

    // METHODS

    // Compute hash of node. This method caches the hash in the node's user4().
    V3Hash ਸੰਚਾਲਕ()(Ast_ਗੰਢ* nodep) ਅਚਲ;

    // Re-compute hash of this node, discarding cached value, but used cached hash of children.
    V3Hash rehash(Ast_ਗੰਢ* nodep) ਅਚਲ;

    // Compute hash of node, without caching in user4.
    ਸਥਿਰ V3Hash uncachedHash(ਅਚਲ Ast_ਗੰਢ* nodep);
};

#ਜੇ_ਅੰਤ  // Guard
