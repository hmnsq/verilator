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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3SENTREE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3SENTREE_H_

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
    Ast_ತುದಿ_Scope* ಸ್ಥಿರಾಂಕ m_ತುದಿ_Scopep;  // Top scope to add global SenTrees to
    std::unordered_ಗಣ<VNRef<Ast_Sen_ವೃಕ್ಷ>> m_trees;  // Set of global SenTrees
    Ast_Sen_ವೃಕ್ಷ* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_ವೃಕ್ಷ* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_ವೃಕ್ಷ_Finder);

    ಮಾದರಿ <typename T_Domain>  //
    Ast_Sen_ವೃಕ್ಷ* makeUnique() {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = m_ತುದಿ_Scopep->fileline();
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = new Ast_Sen_ವೃಕ್ಷ{fl, new Ast_Sen_ಅಂಶ{fl, T_Domain{}}};
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ restultp = get_Sen_ವೃಕ್ಷ(senTreep);
        VL_DO_DANGLING(senTreep->delete_ವೃಕ್ಷ(), senTreep);  // getSenTree clones, so can delete
        ಹಿಂತಿರುಗಿ restultp;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Sen_ವೃಕ್ಷ_Finder()
        : Sen_ವೃಕ್ಷ_Finder{v3Global.rootp()} {}

    explicit Sen_ವೃಕ್ಷ_Finder(AstNetlist* netlistp)
        : m_ತುದಿ_Scopep{netlistp->ತುದಿ_Scopep()} {
        // Gather existing global SenTrees
        ಗಾಗಿ (Ast_Sen_ವೃಕ್ಷ* senTreep = m_ತುದಿ_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_ವೃಕ್ಷ)) {
            m_trees.emplace(*senTreep);
            ಆದರೆ (senTreep->hasCombo()) m_combop = senTreep;
            ಆದರೆ (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ವೃಕ್ಷ* get_Sen_ವೃಕ್ಷ(Ast_Sen_ವೃಕ್ಷ* senTreep) {
        ಸ್ವಯಂಚಾಲಿತ it = m_trees.ಕಂಡುಹಿಡಿ(*senTreep);
        // If match found, return it.
        ಆದರೆ (it != m_trees.ಕೊನೆ()) ಹಿಂತಿರುಗಿ &(*it).get();

        // Not found, create a new one
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ newSenTreep = senTreep->clone_ವೃಕ್ಷ(false);
        m_ತುದಿ_Scopep->ಕೂಡಿಸು_Sen_Treesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        ಹಿಂತಿರುಗಿ newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ವೃಕ್ಷ* getComb() {
        ಆದರೆ (!m_combop) m_combop = makeUnique<Ast_Sen_ಅಂಶ::Combo>();
        ಹಿಂತಿರುಗಿ m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ವೃಕ್ಷ* getInitial() {
        ಆದರೆ (!m_initialp) m_initialp = makeUnique<Ast_Sen_ಅಂಶ::Initial>();
        ಹಿಂತಿರುಗಿ m_initialp;
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
