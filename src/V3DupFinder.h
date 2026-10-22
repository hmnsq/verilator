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
#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3Hasher.h"

#ضمن <map>
#ضمن <memory>

//============================================================================

بنية V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    virtual bool isSame(Ast_كلي*, Ast_كلي*) = 0;
    V3DupFinderUserSame() = افتراضي;
    virtual ~V3DupFinderUserSame() = افتراضي;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : خاص std::multimap<V3Hash, Ast_كلي*> {
    using Super = std::multimap<V3Hash, Ast_كلي*>;

    // MEMBERS
    ثابت V3Hasher* ثابت m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    ثابت V3Hasher& m_hasher;  // Reference to hasher

عام:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(ثابت V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        إذا (m_hasherOwnedp) delete m_hasherOwnedp;
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
    iterator سك(Ast_كلي* nodep) { أرجع emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    size_type erase(Ast_كلي* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator findDuplicate(Ast_كلي* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    فراغ dumpFile(ثابت string& filename, bool بشيا) VL_MT_DISABLED;
    فراغ dumpFilePrefixed(ثابت string& nameComment, bool بشيا = false) VL_MT_DISABLED;
};

#endif  // Guard
