// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity block domains
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// AstSenTree related utilities.
//*************************************************************************

#ifndef VERILATOR_V3SENTREE_H_
#define VERILATOR_V3SENTREE_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Hasher.h"

#அந்தர்பாவ <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_மரம்_Finder final {
    // STATE - across all visitors
    Ast_உச்சி_Scope* மாறிலி m_உச்சி_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_மரம்>> m_trees;  // Set of global SenTrees
    Ast_Sen_மரம்* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_மரம்* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_மரம்_Finder);

    template <typename T_Domain>  //
    Ast_Sen_மரம்* makeUnique() {
        File_வரி* மாறிலி fl = m_உச்சி_Scopep->fileline();
        Ast_Sen_மரம்* மாறிலி senTreep = new Ast_Sen_மரம்{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_மரம்* மாறிலி restultp = get_Sen_மரம்(senTreep);
        VL_DO_DANGLING(senTreep->delete_மரம்(), senTreep);  // getSenTree clones, so can delete
        திரும்பு restultp;
    }

பொது:
    // CONSTRUCTORS
    Sen_மரம்_Finder()
        : Sen_மரம்_Finder{v3Global.rootp()} {}

    explicit Sen_மரம்_Finder(AstNetlist* netlistp)
        : m_உச்சி_Scopep{netlistp->உச்சி_Scopep()} {
        // Gather existing global SenTrees
        க்கு (Ast_Sen_மரம்* senTreep = m_உச்சி_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_மரம்)) {
            m_trees.emplace(*senTreep);
            எனில் (senTreep->hasCombo()) m_combop = senTreep;
            எனில் (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_மரம்* get_Sen_மரம்(Ast_Sen_மரம்* senTreep) {
        auto it = m_trees.find(*senTreep);
        // If match found, return it.
        எனில் (it != m_trees.end()) திரும்பு &(*it).get();

        // Not found, create a new one
        Ast_Sen_மரம்* மாறிலி newSenTreep = senTreep->clone_மரம்(false);
        m_உச்சி_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        திரும்பு newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_மரம்* getComb() {
        எனில் (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        திரும்பு m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_மரம்* getInitial() {
        எனில் (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        திரும்பு m_initialp;
    }
};

#endif  // Guard
