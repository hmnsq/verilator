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
#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Hasher.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>

//============================================================================

સંરચના V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    આભાસી bool isSame(Ast_ગાંઠ*, Ast_ગાંઠ*) = 0;
    V3DupFinderUserSame() = મૂળભૂત;
    આભાસી ~V3DupFinderUserSame() = મૂળભૂત;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : ખાનગી std::multimap<V3Hash, Ast_ગાંઠ*> {
    using Super = std::multimap<V3Hash, Ast_ગાંઠ*>;

    // MEMBERS
    અચલ V3Hasher* અચલ m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    અચલ V3Hasher& m_hasher;  // Reference to hasher

જાહેર:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(અચલ V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        જો (m_hasherOwnedp) delete m_hasherOwnedp;
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
    iterator ઉમેરો(Ast_ગાંઠ* nodep) { પાછા emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    size_type erase(Ast_ગાંઠ* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator findDuplicate(Ast_ગાંઠ* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    શૂન્ય dumpFile(અચલ string& filename, bool વૃક્ષ) VL_MT_DISABLED;
    શૂન્ય dumpFilePrefixed(અચલ string& nameComment, bool વૃક્ષ = false) VL_MT_DISABLED;
};

#endif  // Guard
