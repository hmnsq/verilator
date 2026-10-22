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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3CoverageJoin.h"

#شامل "V3DupFinder.h"
#شامل "V3Stats.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CoverageJoin state, as a visitor of each AstNode

رده CoverageJoinVisitor final : عمومی VNVisitor {
    // NODE STATE
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE - per active
    std::vector<AstCoverToggle*> m_toggleps;  // List of of all AstCoverToggle's

    // STATE - Statistic tracking
    VDouble0 m_statToggleJoins;  // Statistic tracking

    // METHODS

    تهی detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection
        // Hash all of the original signals we toggle cover
        برای (ثابت AstCoverToggle* ثابت nodep : m_toggleps) dupFinder.درج(nodep->origp());
        اگر (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("coveragejoin") + ".hash", نادرست);
        // Find if there are any duplicates
        برای (AstCoverToggle* nodep : m_toggleps) {
            // nodep->backp() is null if we already detected it's a duplicate and unlinked earlier
            اگر (!nodep->backp()) ادامه;
            // Want to choose a base node, and keep finding duplicates that are identical.
            // This prevents making chains where a->b, then c->d, then b->c, as we'll
            // find a->b, a->c, a->d directly.
            وقتی (درست) {
                ثابت auto dupit = dupFinder.findDuplicate(nodep->origp());
                اگر (dupit == dupFinder.end()) بشکن;
                ثابت Ast_گره* ثابت duporigp = dupit->second;
                // Remove node from comparison so don't hit it again
                dupFinder.erase(dupit);
                //
                // Note dupFinder will point to the original toggle-increment equation (what's
                // duplicated), not the covertoggle, but we need to get back to the
                // covertoggle which is immediately above, so:
                AstCoverToggle* ثابت removep = VN_AS(duporigp->backp(), CoverToggle);
                UASSERT_OBJ(removep, nodep, "CoverageJoin duplicate of wrong type");
                UINFO(8, "  Orig " << nodep << " -->> " << nodep->incp()->declp());
                UINFO(8, "   dup " << removep << " -->> " << removep->incp()->declp());
                // The CoverDecl the duplicate pointed to now needs to point to the
                // original's data. I.e. the duplicate will get the coverage number
                // from the non-duplicate
                Ast_گره_Cover_Decl* ثابت datadeclp = nodep->incp()->declp()->dataDeclThisp();
                removep->incp()->declp()->dataDeclp(datadeclp);
                UINFO(8, "   new " << removep->incp()->declp());
                // Mark the found node as a duplicate of the first node
                // (Not vice-versa as we have the iterator for the found node)
                removep->unlinkFrBack();
                VL_DO_DANGLING(افزودن_Deletep(removep), removep);
                ++m_statToggleJoins;
            }
        }
    }

    // VISITORS
    تهی visit(AstActive* nodep) override {
        m_toggleps.clear();
        // Find all Coverage's
        iterateChildren(nodep);
        // Simplify
        detectDuplicates();
    }
    تهی visit(AstCoverToggle* nodep) override {
        m_toggleps.افزودن_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    تهی visit(Ast_گره_عبارت*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit CoverageJoinVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CoverageJoinVisitor() override {
        V3Stats::addStat("Coverage, Toggle points joined", m_statToggleJoins);
    }
};

//######################################################################
// Coverage class functions

تهی V3CoverageJoin::coverageJoin(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageJoinVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("coveragejoin", 0, dump_درخت_Either_Level() >= 3);
}
