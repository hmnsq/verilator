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
#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Hasher.h"

#অন্তর্ভাব <map>
#অন্তর্ভাব <memory>

//============================================================================

কাঠামো V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    আভাসী bool isSame(Ast_গ্রন্থি*, Ast_গ্রন্থি*) = 0;
    V3DupFinderUserSame() = পূর্বনির্ধারিত;
    আভাসী ~V3DupFinderUserSame() = পূর্বনির্ধারিত;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : ব্যক্তিগত std::multimap<V3Hash, Ast_গ্রন্থি*> {
    using Super = std::multimap<V3Hash, Ast_গ্রন্থি*>;

    // MEMBERS
    ধ্রুবক V3Hasher* ধ্রুবক m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    ধ্রুবক V3Hasher& m_hasher;  // Reference to hasher

সর্বজনীন:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(ধ্রুবক V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        যদি (m_hasherOwnedp) delete m_hasherOwnedp;
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
    using Super::আকার_type;

    // Insert node into data structure
    iterator ঢোকাও(Ast_গ্রন্থি* nodep) { ফেরত emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    আকার_type erase(Ast_গ্রন্থি* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator findDuplicate(Ast_গ্রন্থি* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    শূন্য dumpFile(ধ্রুবক string& filename, bool বৃক্ষ) VL_MT_DISABLED;
    শূন্য dumpFilePrefixed(ধ্রুবক string& nameComment, bool বৃক্ষ = false) VL_MT_DISABLED;
};

#endif  // Guard
