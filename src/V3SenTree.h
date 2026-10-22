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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Hasher.h"

#ضمن <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_بشيا_Finder final {
    // STATE - across all visitors
    Ast_كولي_Scope* ثابت m_كولي_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_بشيا>> m_trees;  // Set of global SenTrees
    Ast_Sen_بشيا* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_بشيا* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_بشيا_Finder);

    template <typename T_Domain>  //
    Ast_Sen_بشيا* makeUnique() {
        File_لي* ثابت fl = m_كولي_Scopep->fileline();
        Ast_Sen_بشيا* ثابت senTreep = new Ast_Sen_بشيا{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_بشيا* ثابت restultp = get_Sen_بشيا(senTreep);
        VL_DO_DANGLING(senTreep->delete_بشيا(), senTreep);  // getSenTree clones, so can delete
        أرجع restultp;
    }

عام:
    // CONSTRUCTORS
    Sen_بشيا_Finder()
        : Sen_بشيا_Finder{v3Global.rootp()} {}

    explicit Sen_بشيا_Finder(AstNetlist* netlistp)
        : m_كولي_Scopep{netlistp->كولي_Scopep()} {
        // Gather existing global SenTrees
        لكل (Ast_Sen_بشيا* senTreep = m_كولي_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_بشيا)) {
            m_trees.emplace(*senTreep);
            إذا (senTreep->hasCombo()) m_combop = senTreep;
            إذا (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_بشيا* get_Sen_بشيا(Ast_Sen_بشيا* senTreep) {
        auto it = m_trees.find(*senTreep);
        // If match found, return it.
        إذا (it != m_trees.end()) أرجع &(*it).get();

        // Not found, create a new one
        Ast_Sen_بشيا* ثابت newSenTreep = senTreep->clone_بشيا(false);
        m_كولي_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        أرجع newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_بشيا* getComb() {
        إذا (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        أرجع m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_بشيا* getInitial() {
        إذا (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        أرجع m_initialp;
    }
};

#endif  // Guard
