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
//  Datastructure for finding duplicate AstNode trees via hashing
//
//*************************************************************************

#अगर_अघोषित VERILATOR_V3DUPFINDER_H_
#घोषणा VERILATOR_V3DUPFINDER_H_
#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Error.h"
#समावेश "V3Hasher.h"

#समावेश <map>
#समावेश <memory>

//============================================================================

काष्ठा V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    भव bool isSame(Ast_गाँठ*, Ast_गाँठ*) = 0;
    V3DupFinderUserSame() = शेष;
    भव ~V3DupFinderUserSame() = शेष;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : निजी std::multimap<V3Hash, Ast_गाँठ*> {
    using Super = std::multimap<V3Hash, Ast_गाँठ*>;

    // MEMBERS
    स्थिर V3Hasher* स्थिर m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    स्थिर V3Hasher& m_hasher;  // Reference to hasher

खुला:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(स्थिर V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        अगर (m_hasherOwnedp) delete m_hasherOwnedp;
    }

    // METHODS

    // Expose minimal set of superclass interface
    using Super::begin;
    using Super::cbegin;
    using Super::cend;
    using Super::clear;
    using Super::const_iterator;
    using Super::empty;
    using Super::end;
    using Super::erase;
    using Super::iterator;
    using Super::आकार_type;

    // Insert node into data structure
    iterator जोड़ो(Ast_गाँठ* nodep) { वापस emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    आकार_type erase(Ast_गाँठ* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator findDuplicate(Ast_गाँठ* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    व्योम dumpFile(स्थिर string& filename, bool वृक्ष) VL_MT_DISABLED;
    व्योम dumpFilePrefixed(स्थिर string& nameComment, bool वृक्ष = false) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
