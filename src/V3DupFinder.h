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
#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Hasher.h"

#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>

//============================================================================

ସଂରଚନା V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    ଆଭାସୀ bool isSame(Ast_ଗଣ୍ଠି*, Ast_ଗଣ୍ଠି*) = 0;
    V3DupFinderUserSame() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଆଭାସୀ ~V3DupFinderUserSame() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : ବ୍ୟକ୍ତିଗତ std::multimap<V3Hash, Ast_ଗଣ୍ଠି*> {
    using Super = std::multimap<V3Hash, Ast_ଗଣ୍ଠି*>;

    // MEMBERS
    ଅଚଳ V3Hasher* ଅଚଳ m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    ଅଚଳ V3Hasher& m_hasher;  // Reference to hasher

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(ଅଚଳ V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        ଯଦି (m_hasherOwnedp) delete m_hasherOwnedp;
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
    iterator ଭର୍ତ୍ତି(Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    size_type erase(Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator findDuplicate(Ast_ଗଣ୍ଠି* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    ଶୂନ୍ୟ dumpFile(ଅଚଳ string& filename, bool ବୃକ୍ଷ) VL_MT_DISABLED;
    ଶୂନ୍ୟ dumpFilePrefixed(ଅଚଳ string& nameComment, bool ବୃକ୍ଷ = false) VL_MT_DISABLED;
};

#endif  // Guard
