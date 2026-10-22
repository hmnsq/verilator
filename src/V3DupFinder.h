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

#ifndef VERILATOR_V3DUPFINDER_H_
#define VERILATOR_V3DUPFINDER_H_
#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Hasher.h"

#अन्तर्भाव <map>
#अन्तर्भाव <memory>

//============================================================================

संरचना V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    आभास bool isSame(Ast_गाँठ*, Ast_गाँठ*) = 0;
    V3DupFinderUserSame() = स्वभाव;
    आभास ~V3DupFinderUserSame() = स्वभाव;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : गुप्त std::multimap<V3Hash, Ast_गाँठ*> {
    using Super = std::multimap<V3Hash, Ast_गाँठ*>;

    // MEMBERS
    ध्रुव V3Hasher* ध्रुव m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    ध्रुव V3Hasher& m_hasher;  // Reference to hasher

सार्वजनिक:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(ध्रुव V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        यदि (m_hasherOwnedp) delete m_hasherOwnedp;
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
    using Super::size_type;

    // Insert node into data structure
    iterator जोड़ो(Ast_गाँठ* nodep) { प्रत्यागम emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    size_type erase(Ast_गाँठ* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator findDuplicate(Ast_गाँठ* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    शून्य dumpFile(ध्रुव string& filename, bool वृक्ष) VL_MT_DISABLED;
    शून्य dumpFilePrefixed(ध्रुव string& nameComment, bool वृक्ष = false) VL_MT_DISABLED;
};

#endif  // Guard
