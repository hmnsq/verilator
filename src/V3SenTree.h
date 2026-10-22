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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Hasher.h"

#ಅನ್ತರ್ಭಾವ <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_ವೃಕ್ಷ_Finder final {
    // STATE - across all visitors
    Ast_ತುದಿ_Scope* ಧ್ರುವ m_ತುದಿ_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_ವೃಕ್ಷ>> m_trees;  // Set of global SenTrees
    Ast_Sen_ವೃಕ್ಷ* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_ವೃಕ್ಷ* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_ವೃಕ್ಷ_Finder);

    template <typename T_Domain>  //
    Ast_Sen_ವೃಕ್ಷ* makeUnique() {
        File_ಸಾಲು* ಧ್ರುವ fl = m_ತುದಿ_Scopep->fileline();
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = new Ast_Sen_ವೃಕ್ಷ{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ restultp = get_Sen_ವೃಕ್ಷ(senTreep);
        VL_DO_DANGLING(senTreep->delete_ವೃಕ್ಷ(), senTreep);  // getSenTree clones, so can delete
        ಪ್ರತ್ಯಾಗಮ restultp;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Sen_ವೃಕ್ಷ_Finder()
        : Sen_ವೃಕ್ಷ_Finder{v3Global.rootp()} {}

    explicit Sen_ವೃಕ್ಷ_Finder(AstNetlist* netlistp)
        : m_ತುದಿ_Scopep{netlistp->ತುದಿ_Scopep()} {
        // Gather existing global SenTrees
        ಪ್ರತಿ (Ast_Sen_ವೃಕ್ಷ* senTreep = m_ತುದಿ_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_ವೃಕ್ಷ)) {
            m_trees.emplace(*senTreep);
            ಯದಿ (senTreep->hasCombo()) m_combop = senTreep;
            ಯದಿ (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ವೃಕ್ಷ* get_Sen_ವೃಕ್ಷ(Ast_Sen_ವೃಕ್ಷ* senTreep) {
        auto it = m_trees.find(*senTreep);
        // If match found, return it.
        ಯದಿ (it != m_trees.end()) ಪ್ರತ್ಯಾಗಮ &(*it).get();

        // Not found, create a new one
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ newSenTreep = senTreep->clone_ವೃಕ್ಷ(false);
        m_ತುದಿ_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        ಪ್ರತ್ಯಾಗಮ newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ವೃಕ್ಷ* getComb() {
        ಯದಿ (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        ಪ್ರತ್ಯಾಗಮ m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ವೃಕ್ಷ* getInitial() {
        ಯದಿ (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        ಪ್ರತ್ಯಾಗಮ m_initialp;
    }
};

#endif  // Guard
