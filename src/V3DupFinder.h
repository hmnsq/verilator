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
#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3Hasher.h"

#شامل <map>
#شامل <memory>

//============================================================================

ساختار V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    مجازی bool isSame(Ast_گره*, Ast_گره*) = 0;
    V3DupFinderUserSame() = پیشفرض;
    مجازی ~V3DupFinderUserSame() = پیشفرض;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : خصوصی std::multimap<V3Hash, Ast_گره*> {
    using Super = std::multimap<V3Hash, Ast_گره*>;

    // MEMBERS
    ثابت V3Hasher* ثابت m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    ثابت V3Hasher& m_hasher;  // Reference to hasher

عمومی:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(ثابت V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        اگر (m_hasherOwnedp) delete m_hasherOwnedp;
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
    iterator درج(Ast_گره* nodep) { بازگشت emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    size_type erase(Ast_گره* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator findDuplicate(Ast_گره* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    تهی dumpFile(ثابت string& filename, bool درخت) VL_MT_DISABLED;
    تهی dumpFilePrefixed(ثابت string& nameComment, bool درخت = false) VL_MT_DISABLED;
};

#endif  // Guard
