// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Netlist (top level) functions
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
// COVERAGEJOIN TRANSFORMATIONS:
//      If two COVERTOGGLEs have same VARSCOPE, combine them
//*************************************************************************

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3CoverageJoin.h"

#অন্তর্ভাব "V3DupFinder.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CoverageJoin state, as a visitor of each AstNode

বর্গ CoverageJoinVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE - per active
    std::vector<AstCoverToggle*> m_toggleps;  // List of of all AstCoverToggle's

    // STATE - Statistic tracking
    VDouble0 m_statToggleJoins;  // Statistic tracking

    // METHODS

    শূন্য detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection
        // Hash all of the original signals we toggle cover
        প্রতি (ধ্রুব AstCoverToggle* ধ্রুব nodep : m_toggleps) dupFinder.ঢোকাও(nodep->origp());
        যদি (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("coveragejoin") + ".hash", অসত্য);
        // Find if there are any duplicates
        প্রতি (AstCoverToggle* nodep : m_toggleps) {
            // nodep->backp() is null if we already detected it's a duplicate and unlinked earlier
            যদি (!nodep->backp()) অনুবর্ত;
            // Want to choose a base node, and keep finding duplicates that are identical.
            // This prevents making chains where a->b, then c->d, then b->c, as we'll
            // find a->b, a->c, a->d directly.
            যাবত্ (সত্য) {
                ধ্রুব auto dupit = dupFinder.findDuplicate(nodep->origp());
                যদি (dupit == dupFinder.end()) ভঙ্গ;
                ধ্রুব Ast_গ্রন্থি* ধ্রুব duporigp = dupit->second;
                // Remove node from comparison so don't hit it again
                dupFinder.erase(dupit);
                //
                // Note dupFinder will point to the original toggle-increment equation (what's
                // duplicated), not the covertoggle, but we need to get back to the
                // covertoggle which is immediately above, so:
                AstCoverToggle* ধ্রুব removep = VN_AS(duporigp->backp(), CoverToggle);
                UASSERT_OBJ(removep, nodep, "CoverageJoin duplicate of wrong type");
                UINFO(8, "  Orig " << nodep << " -->> " << nodep->incp()->declp());
                UINFO(8, "   dup " << removep << " -->> " << removep->incp()->declp());
                // The CoverDecl the duplicate pointed to now needs to point to the
                // original's data. I.e. the duplicate will get the coverage number
                // from the non-duplicate
                Ast_গ্রন্থি_Cover_Decl* ধ্রুব datadeclp = nodep->incp()->declp()->dataDeclThisp();
                removep->incp()->declp()->dataDeclp(datadeclp);
                UINFO(8, "   new " << removep->incp()->declp());
                // Mark the found node as a duplicate of the first node
                // (Not vice-versa as we have the iterator for the found node)
                removep->unlinkFrBack();
                VL_DO_DANGLING(ঠেলো_Deletep(removep), removep);
                ++m_statToggleJoins;
            }
        }
    }

    // VISITORS
    শূন্য visit(AstActive* nodep) override {
        m_toggleps.clear();
        // Find all Coverage's
        iterateChildren(nodep);
        // Simplify
        detectDuplicates();
    }
    শূন্য visit(AstCoverToggle* nodep) override {
        m_toggleps.ঠেলো_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit CoverageJoinVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CoverageJoinVisitor() override {
        V3Stats::addStat("Coverage, Toggle points joined", m_statToggleJoins);
    }
};

//######################################################################
// Coverage class functions

শূন্য V3CoverageJoin::coverageJoin(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageJoinVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("coveragejoin", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
