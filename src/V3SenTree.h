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

#که_نه_تعریف_شوی VERILATOR_V3SENTREE_H_
#تعریف_کړه VERILATOR_V3SENTREE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Hasher.h"

#ضمن <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_ونه_Finder final {
    // STATE - across all visitors
    Ast_سر_Scope* ثابت m_سر_Scopep;  // Top scope to add global SenTrees to
    std::unordered_سېټ<VNRef<Ast_Sen_ونه>> m_trees;  // Set of global SenTrees
    Ast_Sen_ونه* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_ونه* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_ونه_Finder);

    کينډۍ <typename T_Domain>  //
    Ast_Sen_ونه* makeUnique() {
        File_کرښه* ثابت fl = m_سر_Scopep->fileline();
        Ast_Sen_ونه* ثابت senTreep = new Ast_Sen_ونه{fl, new Ast_Sen_توکی{fl, T_Domain{}}};
        Ast_Sen_ونه* ثابت restultp = get_Sen_ونه(senTreep);
        VL_DO_DANGLING(senTreep->delete_ونه(), senTreep);  // getSenTree clones, so can delete
        أرجع restultp;
    }

عام:
    // CONSTRUCTORS
    Sen_ونه_Finder()
        : Sen_ونه_Finder{v3Global.rootp()} {}

    explicit Sen_ونه_Finder(AstNetlist* netlistp)
        : m_سر_Scopep{netlistp->سر_Scopep()} {
        // Gather existing global SenTrees
        لكل (Ast_Sen_ونه* senTreep = m_سر_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_ونه)) {
            m_trees.emplace(*senTreep);
            إذا (senTreep->hasCombo()) m_combop = senTreep;
            إذا (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ونه* get_Sen_ونه(Ast_Sen_ونه* senTreep) {
        خپلکاری it = m_trees.پیدا_کړه(*senTreep);
        // If match found, return it.
        إذا (it != m_trees.پای_ټکی()) أرجع &(*it).get();

        // Not found, create a new one
        Ast_Sen_ونه* ثابت newSenTreep = senTreep->clone_ونه(false);
        m_سر_Scopep->جمع_کړه_Sen_Treesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        أرجع newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ونه* getComb() {
        إذا (!m_combop) m_combop = makeUnique<Ast_Sen_توکی::Combo>();
        أرجع m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_ونه* getInitial() {
        إذا (!m_initialp) m_initialp = makeUnique<Ast_Sen_توکی::Initial>();
        أرجع m_initialp;
    }
};

#د_که_پای  // Guard
