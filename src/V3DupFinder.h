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
#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Hasher.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <memory>

//============================================================================

கட்டமைப்பு V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    மெய்நிகர் bool isSame(Ast_முனை*, Ast_முனை*) = 0;
    V3DupFinderUserSame() = இயல்பு;
    மெய்நிகர் ~V3DupFinderUserSame() = இயல்பு;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : தனிப்பட்ட std::multimap<V3Hash, Ast_முனை*> {
    using Super = std::multimap<V3Hash, Ast_முனை*>;

    // MEMBERS
    மாறிலி V3Hasher* மாறிலி m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    மாறிலி V3Hasher& m_hasher;  // Reference to hasher

பொது:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(மாறிலி V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        எனில் (m_hasherOwnedp) delete m_hasherOwnedp;
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
    iterator செருகு(Ast_முனை* nodep) { திரும்பு emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    size_type erase(Ast_முனை* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator findDuplicate(Ast_முனை* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    வெறுமை dumpFile(மாறிலி string& filename, bool மரம்) VL_MT_DISABLED;
    வெறுமை dumpFilePrefixed(மாறிலி string& nameComment, bool மரம் = false) VL_MT_DISABLED;
};

#endif  // Guard
