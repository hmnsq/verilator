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

#నిర్వచితం_కాకపోతే VERILATOR_V3DUPFINDER_H_
#నిర్వచించు VERILATOR_V3DUPFINDER_H_
#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Hasher.h"

#అన్తర్భావ <map>
#అన్తర్భావ <memory>

//============================================================================

నిర్మాణం V3DupFinderUserSame VL_NOT_FINAL {
    // Functor for V3DupFinder::findDuplicate
    మాయా bool isSame(Ast_కణుపు*, Ast_కణుపు*) = 0;
    V3DupFinderUserSame() = అప్రమేయ;
    మాయా ~V3DupFinderUserSame() = అప్రమేయ;
};

// This really is just a multimap from 'node hash' to 'node pointer', with some minor extensions.
class V3DupFinder final : అంతరంగ std::multimap<V3Hash, Ast_కణుపు*> {
    using Super = std::multimap<V3Hash, Ast_కణుపు*>;

    // MEMBERS
    స్థిరరాశి V3Hasher* స్థిరరాశి m_hasherOwnedp = nullptr;  // Pointer to owned hasher
    స్థిరరాశి V3Hasher& m_hasher;  // Reference to hasher

బహిరంగ:
    // CONSTRUCTORS
    V3DupFinder()
        : m_hasherOwnedp{new V3Hasher}
        , m_hasher{*m_hasherOwnedp} {}
    explicit V3DupFinder(స్థిరరాశి V3Hasher& hasher)
        : m_hasher{hasher} {}
    ~V3DupFinder() {
        ఒకవేళ (m_hasherOwnedp) delete m_hasherOwnedp;
    }

    // METHODS

    // Expose minimal set of superclass interface
    using Super::begin;
    using Super::cbegin;
    using Super::cend;
    using Super::clear;
    using Super::const_iterator;
    using Super::ఖాళీలు;
    using Super::చివర;
    using Super::erase;
    using Super::iterator;
    using Super::కొలత_type;

    // Insert node into data structure
    iterator చొప్పించు(Ast_కణుపు* nodep) { తిరిగి emplace(m_hasher(nodep), nodep); }

    // Erase node from data structure
    కొలత_type erase(Ast_కణుపు* nodep) VL_MT_DISABLED;

    // Return duplicate, if one was inserted, with optional user check for sameness
    iterator కనుగొను_Duplicate(Ast_కణుపు* nodep, V3DupFinderUserSame* checkp = nullptr) VL_MT_DISABLED;

    // Dump for debug
    శూన్య dumpFile(స్థిరరాశి string& filename, bool వృక్షం) VL_MT_DISABLED;
    శూన్య dumpFilePrefixed(స్థిరరాశి string& nameComment, bool వృక్షం = false) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
