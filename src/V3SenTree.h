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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Hasher.h"

#ଅନ୍ତର୍ଭାଵ <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_ବୃକ୍ଷ_Finder final {
    // STATE - across all visitors
    Ast_ଶୀର୍ଷ_Scope* ଅଚଳ m_ଶୀର୍ଷ_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_ବୃକ୍ଷ>> m_trees;  // Set of global SenTrees
    Ast_Sen_ବୃକ୍ଷ* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_ବୃକ୍ଷ* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_ବୃକ୍ଷ_Finder);

    template <typename T_Domain>  //
    Ast_Sen_ବୃକ୍ଷ* makeUnique() {
        File_ଧାଡ଼ି* ଅଚଳ fl = m_ଶୀର୍ଷ_Scopep->fileline();
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = new Ast_Sen_ବୃକ୍ଷ{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ restultp = get_Sen_ବୃକ୍ଷ(senTreep);
        VL_DO_DANGLING(senTreep->delete_ବୃକ୍ଷ(), senTreep);  // getSenTree clones, so can delete
        ଫେରନ୍ତୁ restultp;
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    Sen_ବୃକ୍ଷ_Finder()
        : Sen_ବୃକ୍ଷ_Finder{v3Global.rootp()} {}

    explicit Sen_ବୃକ୍ଷ_Finder(AstNetlist* netlistp)
        : m_ଶୀର୍ଷ_Scopep{netlistp->ଶୀର୍ଷ_Scopep()} {
        // Gather existing global SenTrees
        ପାଇଁ (Ast_Sen_ବୃକ୍ଷ* senTreep = m_ଶୀର୍ଷ_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_ବୃକ୍ଷ)) {
            m_trees.emplace(*senTreep);
            ଯଦି (senTreep->hasCombo()) m_combop = senTreep;
            ଯଦି (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ବୃକ୍ଷ* get_Sen_ବୃକ୍ଷ(Ast_Sen_ବୃକ୍ଷ* senTreep) {
        auto it = m_trees.find(*senTreep);
        // If match found, return it.
        ଯଦି (it != m_trees.end()) ଫେରନ୍ତୁ &(*it).get();

        // Not found, create a new one
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ newSenTreep = senTreep->clone_ବୃକ୍ଷ(false);
        m_ଶୀର୍ଷ_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        ଫେରନ୍ତୁ newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ବୃକ୍ଷ* getComb() {
        ଯଦି (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        ଫେରନ୍ତୁ m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ବୃକ୍ଷ* getInitial() {
        ଯଦି (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        ଫେରନ୍ତୁ m_initialp;
    }
};

#endif  // Guard
