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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Hasher.h"

#અન્તર્ભાવ <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_વૃક્ષ_Finder final {
    // STATE - across all visitors
    Ast_ટોચ_Scope* અચલ m_ટોચ_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_વૃક્ષ>> m_trees;  // Set of global SenTrees
    Ast_Sen_વૃક્ષ* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_વૃક્ષ* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_વૃક્ષ_Finder);

    template <typename T_Domain>  //
    Ast_Sen_વૃક્ષ* makeUnique() {
        File_લીટી* અચલ fl = m_ટોચ_Scopep->fileline();
        Ast_Sen_વૃક્ષ* અચલ senTreep = new Ast_Sen_વૃક્ષ{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_વૃક્ષ* અચલ restultp = get_Sen_વૃક્ષ(senTreep);
        VL_DO_DANGLING(senTreep->delete_વૃક્ષ(), senTreep);  // getSenTree clones, so can delete
        પાછા restultp;
    }

જાહેર:
    // CONSTRUCTORS
    Sen_વૃક્ષ_Finder()
        : Sen_વૃક્ષ_Finder{v3Global.rootp()} {}

    explicit Sen_વૃક્ષ_Finder(AstNetlist* netlistp)
        : m_ટોચ_Scopep{netlistp->ટોચ_Scopep()} {
        // Gather existing global SenTrees
        માટે (Ast_Sen_વૃક્ષ* senTreep = m_ટોચ_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_વૃક્ષ)) {
            m_trees.emplace(*senTreep);
            જો (senTreep->hasCombo()) m_combop = senTreep;
            જો (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_વૃક્ષ* get_Sen_વૃક્ષ(Ast_Sen_વૃક્ષ* senTreep) {
        auto it = m_trees.find(*senTreep);
        // If match found, return it.
        જો (it != m_trees.end()) પાછા &(*it).get();

        // Not found, create a new one
        Ast_Sen_વૃક્ષ* અચલ newSenTreep = senTreep->clone_વૃક્ષ(false);
        m_ટોચ_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        પાછા newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_વૃક્ષ* getComb() {
        જો (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        પાછા m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_વૃક્ષ* getInitial() {
        જો (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        પાછા m_initialp;
    }
};

#endif  // Guard
