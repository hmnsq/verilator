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
#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Hasher.h"

#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>

//============================================================================

ഘടന V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    മായാ bool isSame(Ast_കണ്ണി*, Ast_കണ്ണി*) = 0;
    V3DupFinderUserSame() = സ്ഥിരസ്ഥിതി;
    മായാ ~V3DupFinderUserSame() = സ്ഥിരസ്ഥിതി;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : സ്വകാര്യ std::multimap<V3Hash, Ast_കണ്ണി*> {
    using Super = std::multimap<V3Hash, Ast_കണ്ണി*>;

    // MEMBERS
    സ്ഥിരം V3Hasher* സ്ഥിരം m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    സ്ഥിരം V3Hasher& m_hasher;  // Reference to hasher

പൊതു:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(സ്ഥിരം V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        എങ്കിൽ (m_hasherOwnedp) delete m_hasherOwnedp;
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
    iterator തിരുകുക(Ast_കണ്ണി* nodep) { തിരികെ emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    size_type erase(Ast_കണ്ണി* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator findDuplicate(Ast_കണ്ണി* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    ശൂന്യം dumpFile(സ്ഥിരം string& filename, bool വൃക്ഷം) VL_MT_DISABLED;
    ശൂന്യം dumpFilePrefixed(സ്ഥിരം string& nameComment, bool വൃക്ഷം = false) VL_MT_DISABLED;
};

#endif  // Guard
