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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Hasher.h"

#شامل <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_درخت_Finder final {
    // STATE - across all visitors
    Ast_بالاترین_Scope* ثابت m_بالاترین_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_درخت>> m_trees;  // Set of global SenTrees
    Ast_Sen_درخت* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_درخت* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_درخت_Finder);

    template <typename T_Domain>  //
    Ast_Sen_درخت* makeUnique() {
        File_سطر* ثابت fl = m_بالاترین_Scopep->fileline();
        Ast_Sen_درخت* ثابت senTreep = new Ast_Sen_درخت{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_درخت* ثابت restultp = get_Sen_درخت(senTreep);
        VL_DO_DANGLING(senTreep->delete_درخت(), senTreep);  // getSenTree clones, so can delete
        بازگشت restultp;
    }

عمومی:
    // CONSTRUCTORS
    Sen_درخت_Finder()
        : Sen_درخت_Finder{v3Global.rootp()} {}

    explicit Sen_درخت_Finder(AstNetlist* netlistp)
        : m_بالاترین_Scopep{netlistp->بالاترین_Scopep()} {
        // Gather existing global SenTrees
        برای (Ast_Sen_درخت* senTreep = m_بالاترین_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_درخت)) {
            m_trees.emplace(*senTreep);
            اگر (senTreep->hasCombo()) m_combop = senTreep;
            اگر (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_درخت* get_Sen_درخت(Ast_Sen_درخت* senTreep) {
        auto it = m_trees.find(*senTreep);
        // If match found, return it.
        اگر (it != m_trees.end()) بازگشت &(*it).get();

        // Not found, create a new one
        Ast_Sen_درخت* ثابت newSenTreep = senTreep->clone_درخت(false);
        m_بالاترین_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        بازگشت newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_درخت* getComb() {
        اگر (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        بازگشت m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_درخت* getInitial() {
        اگر (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        بازگشت m_initialp;
    }
};

#endif  // Guard
