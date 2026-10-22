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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Hasher.h"

#അന്തര്ഭാവ <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_വൃക്ഷം_Finder final {
    // STATE - across all visitors
    Ast_മുകൾ_Scope* സ്ഥിരം m_മുകൾ_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_വൃക്ഷം>> m_trees;  // Set of global SenTrees
    Ast_Sen_വൃക്ഷം* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_വൃക്ഷം* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_വൃക്ഷം_Finder);

    template <typename T_Domain>  //
    Ast_Sen_വൃക്ഷം* makeUnique() {
        File_വരി* സ്ഥിരം fl = m_മുകൾ_Scopep->fileline();
        Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = new Ast_Sen_വൃക്ഷം{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_വൃക്ഷം* സ്ഥിരം restultp = get_Sen_വൃക്ഷം(senTreep);
        VL_DO_DANGLING(senTreep->delete_വൃക്ഷം(), senTreep);  // getSenTree clones, so can delete
        തിരികെ restultp;
    }

പൊതു:
    // CONSTRUCTORS
    Sen_വൃക്ഷം_Finder()
        : Sen_വൃക്ഷം_Finder{v3Global.rootp()} {}

    explicit Sen_വൃക്ഷം_Finder(AstNetlist* netlistp)
        : m_മുകൾ_Scopep{netlistp->മുകൾ_Scopep()} {
        // Gather existing global SenTrees
        വേണ്ടി (Ast_Sen_വൃക്ഷം* senTreep = m_മുകൾ_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_വൃക്ഷം)) {
            m_trees.emplace(*senTreep);
            എങ്കിൽ (senTreep->hasCombo()) m_combop = senTreep;
            എങ്കിൽ (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_വൃക്ഷം* get_Sen_വൃക്ഷം(Ast_Sen_വൃക്ഷം* senTreep) {
        auto it = m_trees.find(*senTreep);
        // If match found, return it.
        എങ്കിൽ (it != m_trees.end()) തിരികെ &(*it).get();

        // Not found, create a new one
        Ast_Sen_വൃക്ഷം* സ്ഥിരം newSenTreep = senTreep->clone_വൃക്ഷം(false);
        m_മുകൾ_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        തിരികെ newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_വൃക്ഷം* getComb() {
        എങ്കിൽ (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        തിരികെ m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_വൃക്ഷം* getInitial() {
        എങ്കിൽ (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        തിരികെ m_initialp;
    }
};

#endif  // Guard
