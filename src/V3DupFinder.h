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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3DUPFINDER_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3DUPFINDER_H_
#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Hasher.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>

//============================================================================

ರಚನೆ V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    ವಾಸ್ತವಿಕ bool isSame(Ast_ಗಂಟು*, Ast_ಗಂಟು*) = 0;
    V3DupFinderUserSame() = ಪೂರ್ವನಿಯೋಜಿತ;
    ವಾಸ್ತವಿಕ ~V3DupFinderUserSame() = ಪೂರ್ವನಿಯೋಜಿತ;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : ಖಾಸಗಿ std::multimap<V3Hash, Ast_ಗಂಟು*> {
    using Super = std::multimap<V3Hash, Ast_ಗಂಟು*>;

    // MEMBERS
    ಸ್ಥಿರಾಂಕ V3Hasher* ಸ್ಥಿರಾಂಕ m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    ಸ್ಥಿರಾಂಕ V3Hasher& m_hasher;  // Reference to hasher

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(ಸ್ಥಿರಾಂಕ V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        ಆದರೆ (m_hasherOwnedp) delete m_hasherOwnedp;
    }

    // METHODS

    // Expose minimal set of superclass interface
    using Super::begin;
    using Super::cbegin;
    using Super::cend;
    using Super::clear;
    using Super::const_iterator;
    using Super::ಖಾಲಿಗಳು;
    using Super::ಕೊನೆ;
    using Super::erase;
    using Super::iterator;
    using Super::ಗಾತ್ರ_type;

    // Insert node into data structure
    iterator ಸೇರಿಸು(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    ಗಾತ್ರ_type erase(Ast_ಗಂಟು* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator ಕಂಡುಹಿಡಿ_Duplicate(Ast_ಗಂಟು* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    ಶೂನ್ಯ dumpFile(ಸ್ಥಿರಾಂಕ string& filename, bool ವೃಕ್ಷ) VL_MT_DISABLED;
    ಶೂನ್ಯ dumpFilePrefixed(ಸ್ಥಿರಾಂಕ string& nameComment, bool ವೃಕ್ಷ = false) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
