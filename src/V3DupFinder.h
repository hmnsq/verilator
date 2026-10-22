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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3DUPFINDER_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3DUPFINDER_H_
#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hasher.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>

//============================================================================

ਬਣਤਰ V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    ਆਭਾਸੀ bool isSame(Ast_ਗੰਢ*, Ast_ਗੰਢ*) = 0;
    V3DupFinderUserSame() = ਮੂਲ;
    ਆਭਾਸੀ ~V3DupFinderUserSame() = ਮੂਲ;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : ਨਿਜੀ std::multimap<V3Hash, Ast_ਗੰਢ*> {
    using Super = std::multimap<V3Hash, Ast_ਗੰਢ*>;

    // MEMBERS
    ਅਚਲ V3Hasher* ਅਚਲ m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    ਅਚਲ V3Hasher& m_hasher;  // Reference to hasher

ਜਨਤਕ:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(ਅਚਲ V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        ਜੇ (m_hasherOwnedp) delete m_hasherOwnedp;
    }

    // METHODS

    // Expose minimal set of superclass interface
    using Super::begin;
    using Super::cbegin;
    using Super::cend;
    using Super::clear;
    using Super::const_iterator;
    using Super::ਖਾਲੀ_ਥਾਵਾਂ;
    using Super::ਸਿਰਾ;
    using Super::erase;
    using Super::iterator;
    using Super::ਮਾਪ_type;

    // Insert node into data structure
    iterator ਪਾਓ(Ast_ਗੰਢ* nodep) { ਵਾਪਸ emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    ਮਾਪ_type erase(Ast_ਗੰਢ* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator ਲੱਭੋ_Duplicate(Ast_ਗੰਢ* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    ਖਾਲੀ dumpFile(ਅਚਲ string& filename, bool ਰੁੱਖ) VL_MT_DISABLED;
    ਖਾਲੀ dumpFilePrefixed(ਅਚਲ string& nameComment, bool ਰੁੱਖ = false) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
