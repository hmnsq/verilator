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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Hasher.h"

#अन्तर्भाव <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_वृक्ष_Finder final {
    // STATE - across all visitors
    Ast_शीर्ष_Scope* स्थिरांक m_शीर्ष_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_वृक्ष>> m_trees;  // Set of global SenTrees
    Ast_Sen_वृक्ष* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_वृक्ष* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_वृक्ष_Finder);

    ढाँचा <typename T_Domain>  //
    Ast_Sen_वृक्ष* makeUnique() {
        File_ओळ* स्थिरांक fl = m_शीर्ष_Scopep->fileline();
        Ast_Sen_वृक्ष* स्थिरांक senTreep = new Ast_Sen_वृक्ष{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_वृक्ष* स्थिरांक restultp = get_Sen_वृक्ष(senTreep);
        VL_DO_DANGLING(senTreep->delete_वृक्ष(), senTreep);  // getSenTree clones, so can delete
        परत restultp;
    }

सार्वजनिक:
    // CONSTRUCTORS
    Sen_वृक्ष_Finder()
        : Sen_वृक्ष_Finder{v3Global.rootp()} {}

    explicit Sen_वृक्ष_Finder(AstNetlist* netlistp)
        : m_शीर्ष_Scopep{netlistp->शीर्ष_Scopep()} {
        // Gather existing global SenTrees
        साठी (Ast_Sen_वृक्ष* senTreep = m_शीर्ष_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_वृक्ष)) {
            m_trees.emplace(*senTreep);
            जर (senTreep->hasCombo()) m_combop = senTreep;
            जर (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_वृक्ष* get_Sen_वृक्ष(Ast_Sen_वृक्ष* senTreep) {
        स्वतः it = m_trees.find(*senTreep);
        // If match found, return it.
        जर (it != m_trees.end()) परत &(*it).get();

        // Not found, create a new one
        Ast_Sen_वृक्ष* स्थिरांक newSenTreep = senTreep->clone_वृक्ष(false);
        m_शीर्ष_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        परत newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_वृक्ष* getComb() {
        जर (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        परत m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_वृक्ष* getInitial() {
        जर (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        परत m_initialp;
    }
};

#endif  // Guard
