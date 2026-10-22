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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3HASHER_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3HASHER_H_
#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Hash.h"

//============================================================================

class V3Hasher final {
    // NODE STATE
    //  AstNode::user4()    -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_inuser4;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3Hasher() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3Hasher() = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS

    // Compute hash of node. This method caches the hash in the node's user4().
    V3Hash ಕ್ರಿಯಾಕಾರಕ()(Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ;

    // Re-compute hash of this node, discarding cached value, but used cached hash of children.
    V3Hash rehash(Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ;

    // Compute hash of node, without caching in user4.
    ಸ್ಥಿರ V3Hash uncachedHash(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
