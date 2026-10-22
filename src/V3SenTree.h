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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Hasher.h"

#অন্তর্ভাব <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_বৃক্ষ_Finder final {
    // STATE - across all visitors
    Ast_শীর্ষ_Scope* ধ্রুবক m_শীর্ষ_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_বৃক্ষ>> m_trees;  // Set of global SenTrees
    Ast_Sen_বৃক্ষ* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_বৃক্ষ* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_বৃক্ষ_Finder);

    template <typename T_Domain>  //
    Ast_Sen_বৃক্ষ* makeUnique() {
        File_সারি* ধ্রুবক fl = m_শীর্ষ_Scopep->fileline();
        Ast_Sen_বৃক্ষ* ধ্রুবক senTreep = new Ast_Sen_বৃক্ষ{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_বৃক্ষ* ধ্রুবক restultp = get_Sen_বৃক্ষ(senTreep);
        VL_DO_DANGLING(senTreep->delete_বৃক্ষ(), senTreep);  // getSenTree clones, so can delete
        ফেরত restultp;
    }

সর্বজনীন:
    // CONSTRUCTORS
    Sen_বৃক্ষ_Finder()
        : Sen_বৃক্ষ_Finder{v3Global.rootp()} {}

    explicit Sen_বৃক্ষ_Finder(AstNetlist* netlistp)
        : m_শীর্ষ_Scopep{netlistp->শীর্ষ_Scopep()} {
        // Gather existing global SenTrees
        জন্য (Ast_Sen_বৃক্ষ* senTreep = m_শীর্ষ_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_বৃক্ষ)) {
            m_trees.emplace(*senTreep);
            যদি (senTreep->hasCombo()) m_combop = senTreep;
            যদি (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_বৃক্ষ* get_Sen_বৃক্ষ(Ast_Sen_বৃক্ষ* senTreep) {
        auto it = m_trees.find(*senTreep);
        // If match found, return it.
        যদি (it != m_trees.end()) ফেরত &(*it).get();

        // Not found, create a new one
        Ast_Sen_বৃক্ষ* ধ্রুবক newSenTreep = senTreep->clone_বৃক্ষ(false);
        m_শীর্ষ_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        ফেরত newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_বৃক্ষ* getComb() {
        যদি (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        ফেরত m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_বৃক্ষ* getInitial() {
        যদি (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        ফেরত m_initialp;
    }
};

#endif  // Guard
