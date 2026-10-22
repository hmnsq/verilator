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

#اگر_غیر_تعریف_شدہ VERILATOR_V3DUPFINDER_H_
#تعریف_کرو VERILATOR_V3DUPFINDER_H_
#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3Hasher.h"

#شامل <map>
#شامل <memory>

//============================================================================

ساخت V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    مجازی bool isSame(Ast_گرہ*, Ast_گرہ*) = 0;
    V3DupFinderUserSame() = طےشدہ;
    مجازی ~V3DupFinderUserSame() = طےشدہ;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : نجی std::multimap<V3Hash, Ast_گرہ*> {
    using Super = std::multimap<V3Hash, Ast_گرہ*>;

    // MEMBERS
    ثابت V3Hasher* ثابت m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    ثابت V3Hasher& m_hasher;  // Reference to hasher

عوامی:
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
    using Super::خالی_خانے;
    using Super::انتہا;
    using Super::erase;
    using Super::iterator;
    using Super::سائز_type;

    // Insert node into data structure
    iterator داخل_کرو(Ast_گرہ* nodep) { واپس emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    سائز_type erase(Ast_گرہ* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator ڈھونڈو_Duplicate(Ast_گرہ* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    خالی dumpFile(ثابت string& filename, bool درخت) VL_MT_DISABLED;
    خالی dumpFilePrefixed(ثابت string& nameComment, bool درخت = false) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
