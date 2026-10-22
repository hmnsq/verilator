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

class Sen_रूख_Finder final {
    // STATE - across all visitors
    Ast_शीर्ष_Scope* अचर m_शीर्ष_Scopep;  // Top scope to add global SenTrees to
    std::unordered_set<VNRef<Ast_Sen_रूख>> m_trees;  // Set of global SenTrees
    Ast_Sen_रूख* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_रूख* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_रूख_Finder);

    ढाँचा <typename T_Domain>  //
    Ast_Sen_रूख* makeUnique() {
        File_पङ्क्ति* अचर fl = m_शीर्ष_Scopep->fileline();
        Ast_Sen_रूख* अचर senTreep = new Ast_Sen_रूख{fl, new AstSenItem{fl, T_Domain{}}};
        Ast_Sen_रूख* अचर restultp = get_Sen_रूख(senTreep);
        VL_DO_DANGLING(senTreep->delete_रूख(), senTreep);  // getSenTree clones, so can delete
        फर्काऊ restultp;
    }

सार्वजनिक:
    // CONSTRUCTORS
    Sen_रूख_Finder()
        : Sen_रूख_Finder{v3Global.rootp()} {}

    explicit Sen_रूख_Finder(AstNetlist* netlistp)
        : m_शीर्ष_Scopep{netlistp->शीर्ष_Scopep()} {
        // Gather existing global SenTrees
        लागि (Ast_Sen_रूख* senTreep = m_शीर्ष_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_रूख)) {
            m_trees.emplace(*senTreep);
            यदि (senTreep->hasCombo()) m_combop = senTreep;
            यदि (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_रूख* get_Sen_रूख(Ast_Sen_रूख* senTreep) {
        स्वतः it = m_trees.find(*senTreep);
        // If match found, return it.
        यदि (it != m_trees.end()) फर्काऊ &(*it).get();

        // Not found, create a new one
        Ast_Sen_रूख* अचर newSenTreep = senTreep->clone_रूख(false);
        m_शीर्ष_Scopep->addSenTreesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        फर्काऊ newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_रूख* getComb() {
        यदि (!m_combop) m_combop = makeUnique<AstSenItem::Combo>();
        फर्काऊ m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_रूख* getInitial() {
        यदि (!m_initialp) m_initialp = makeUnique<AstSenItem::Initial>();
        फर्काऊ m_initialp;
    }
};

#endif  // Guard
