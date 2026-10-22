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

#నిర్వచితం_కాకపోతే VERILATOR_V3SENTREE_H_
#నిర్వచించు VERILATOR_V3SENTREE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Hasher.h"

#అన్తర్భావ <unordered_set>

//######################################################################
// Collect SenTrees under the entire scope
// And provide functions to find/add a new one

class Sen_వృక్షం_Finder final {
    // STATE - across all visitors
    Ast_శిఖరం_Scope* స్థిరరాశి m_శిఖరం_Scopep;  // Top scope to add global SenTrees to
    std::unordered_సమితి<VNRef<Ast_Sen_వృక్షం>> m_trees;  // Set of global SenTrees
    Ast_Sen_వృక్షం* m_combop = nullptr;  // The unique combinational domain SenTree
    Ast_Sen_వృక్షం* m_initialp = nullptr;  // The unique initial domain SenTree

    VL_UNCOPYABLE(Sen_వృక్షం_Finder);

    మూస <typename T_Domain>  //
    Ast_Sen_వృక్షం* makeUnique() {
        File_పంక్తి* స్థిరరాశి fl = m_శిఖరం_Scopep->fileline();
        Ast_Sen_వృక్షం* స్థిరరాశి senTreep = new Ast_Sen_వృక్షం{fl, new Ast_Sen_అంశం{fl, T_Domain{}}};
        Ast_Sen_వృక్షం* స్థిరరాశి restultp = get_Sen_వృక్షం(senTreep);
        VL_DO_DANGLING(senTreep->delete_వృక్షం(), senTreep);  // getSenTree clones, so can delete
        తిరిగి restultp;
    }

బహిరంగ:
    // CONSTRUCTORS
    Sen_వృక్షం_Finder()
        : Sen_వృక్షం_Finder{v3Global.rootp()} {}

    explicit Sen_వృక్షం_Finder(AstNetlist* netlistp)
        : m_శిఖరం_Scopep{netlistp->శిఖరం_Scopep()} {
        // Gather existing global SenTrees
        కొరకు (Ast_Sen_వృక్షం* senTreep = m_శిఖరం_Scopep->senTreesp(); senTreep;
             senTreep = VN_AS(senTreep->nextp(), Sen_వృక్షం)) {
            m_trees.emplace(*senTreep);
            ఒకవేళ (senTreep->hasCombo()) m_combop = senTreep;
            ఒకవేళ (senTreep->hasInitial()) m_initialp = senTreep;
        }
    }

    // METHODS

    // Return a global AstSenTree equivalent to the given senTreep.
    // If no such global AstSenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_వృక్షం* get_Sen_వృక్షం(Ast_Sen_వృక్షం* senTreep) {
        స్వయంచాలక it = m_trees.కనుగొను(*senTreep);
        // If match found, return it.
        ఒకవేళ (it != m_trees.చివర()) తిరిగి &(*it).get();

        // Not found, create a new one
        Ast_Sen_వృక్షం* స్థిరరాశి newSenTreep = senTreep->clone_వృక్షం(false);
        m_శిఖరం_Scopep->కూడు_Sen_Treesp(newSenTreep);
        m_trees.emplace(*newSenTreep);
        తిరిగి newSenTreep;
    }

    // Return the global combinational AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_వృక్షం* getComb() {
        ఒకవేళ (!m_combop) m_combop = makeUnique<Ast_Sen_అంశం::Combo>();
        తిరిగి m_combop;
    }

    // Return the global initial AstSenTree.
    // If no such global SenTree exists create one and add it to the stored AstTopScope.
    Ast_Sen_వృక్షం* getInitial() {
        ఒకవేళ (!m_initialp) m_initialp = makeUnique<Ast_Sen_అంశం::Initial>();
        తిరిగి m_initialp;
    }
};

#అయితే_ముగింపు  // Guard
