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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3SENTREE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3SENTREE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hasher.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_ਰੁੱਖ_Finder final {
    // STATE - across all visitors
    Ast_ਸਿਖਰ_Scope* ਅਚਲ m_ਸਿਖਰ_Scopep;  // Top scope to add global SenTrees to
    std::unordered_ਸੈੱਟ<VNRef<Ast_Sen_ਰੁੱਖ>> m_trees;  // Set of global SenTrees
    Ast_Sen_ਰੁੱਖ* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_ਰੁੱਖ* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_ਰੁੱਖ_Finder);

    ਸਾਂਚਾ <typename T_Domain>  //
    Ast_Sen_ਰੁੱਖ* makeUnique() {
        File_ਸਤਰ* ਅਚਲ fl = m_ਸਿਖਰ_Scopep->fileline();
        Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = new Ast_Sen_ਰੁੱਖ{fl, new Ast_Sen_ਵਸਤੂ{fl, T_Domain{}}};
        Ast_Sen_ਰੁੱਖ* ਅਚਲ restultp = get_Sen_ਰੁੱਖ(senTreep);
        VL_DO_DANGLING(senTreep->delete_ਰੁੱਖ(), senTreep);  // getSenTree clones, so can delete
        ਵਾਪਸ restultp;
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    Sen_ਰੁੱਖ_Finder()
        : Sen_ਰੁੱਖ_Finder{v3Global.rootp()} {}

    explicit Sen_ਰੁੱਖ_Finder(AstNetlist* netlistp)
        : m_ਸਿਖਰ_Scopep{netlistp->ਸਿਖਰ_Scopep()} {
        // Gather existing global SenTrees
        ਲਈ (Ast_Sen_ਰੁੱਖ* senTreep = m_ਸਿਖਰ_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_ਰੁੱਖ)) {
            m_trees.emplace(*senTreep);
            ਜੇ (senTreep->hasCombo()) m_combop = senTreep;
            ਜੇ (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ਰੁੱਖ* get_Sen_ਰੁੱਖ(Ast_Sen_ਰੁੱਖ* senTreep) {
        ਸਵੈਚਾਲਿਤ it = m_trees.ਲੱਭੋ(*senTreep);
        // If match found, return it.
        ਜੇ (it != m_trees.ਸਿਰਾ()) ਵਾਪਸ &(*it).get();

        // Not found, create a new one
        Ast_Sen_ਰੁੱਖ* ਅਚਲ newSenTreep = senTreep->clone_ਰੁੱਖ(false);
        m_ਸਿਖਰ_Scopep->ਜੋੜੋ_Sen_Treesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        ਵਾਪਸ newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ਰੁੱਖ* getComb() {
        ਜੇ (!m_combop) m_combop = makeUnique<Ast_Sen_ਵਸਤੂ::Combo>();
        ਵਾਪਸ m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ਰੁੱਖ* getInitial() {
        ਜੇ (!m_initialp) m_initialp = makeUnique<Ast_Sen_ਵਸਤੂ::Initial>();
        ਵਾਪਸ m_initialp;
    }
};

#ਜੇ_ਅੰਤ  // Guard
