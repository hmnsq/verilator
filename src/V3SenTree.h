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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3SENTREE_H_
#وصف_ڪريو VERILATOR_V3SENTREE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Hasher.h"

#ضمن <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_وڻ_Finder final {
    // STATE - across all visitors
    Ast_چوٽي_Scope* ثابت m_چوٽي_Scopep;  // Top scope to add global SenTrees to
    std::unordered_سيٽ<VNRef<Ast_Sen_وڻ>> m_trees;  // Set of global SenTrees
    Ast_Sen_وڻ* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_وڻ* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_وڻ_Finder);

    سانچو <typename T_Domain>  //
    Ast_Sen_وڻ* makeUnique() {
        File_سٽ* ثابت fl = m_چوٽي_Scopep->fileline();
        Ast_Sen_وڻ* ثابت senTreep = new Ast_Sen_وڻ{fl, new Ast_Sen_شي{fl, T_Domain{}}};
        Ast_Sen_وڻ* ثابت restultp = get_Sen_وڻ(senTreep);
        VL_DO_DANGLING(senTreep->delete_وڻ(), senTreep);  // getSenTree clones, so can delete
        أرجع restultp;
    }

عام:
    // CONSTRUCTORS
    Sen_وڻ_Finder()
        : Sen_وڻ_Finder{v3Global.rootp()} {}

    explicit Sen_وڻ_Finder(AstNetlist* netlistp)
        : m_چوٽي_Scopep{netlistp->چوٽي_Scopep()} {
        // Gather existing global SenTrees
        لكل (Ast_Sen_وڻ* senTreep = m_چوٽي_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_وڻ)) {
            m_trees.emplace(*senTreep);
            إذا (senTreep->hasCombo()) m_combop = senTreep;
            إذا (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_وڻ* get_Sen_وڻ(Ast_Sen_وڻ* senTreep) {
        خودڪار it = m_trees.ڳوليو(*senTreep);
        // If match found, return it.
        إذا (it != m_trees.پڇاڙي()) أرجع &(*it).get();

        // Not found, create a new one
        Ast_Sen_وڻ* ثابت newSenTreep = senTreep->clone_وڻ(false);
        m_چوٽي_Scopep->جوڙيو_Sen_Treesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        أرجع newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_وڻ* getComb() {
        إذا (!m_combop) m_combop = makeUnique<Ast_Sen_شي::Combo>();
        أرجع m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_وڻ* getInitial() {
        إذا (!m_initialp) m_initialp = makeUnique<Ast_Sen_شي::Initial>();
        أرجع m_initialp;
    }
};

#جيڪڏهن_پورو  // Guard
