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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3CoverageJoin.h"

#ضمن "V3DupFinder.h"
#ضمن "V3Stats.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CoverageJoin state, as a visitor of each AstNode

صنف CoverageJoinVisitor final : عام VNVisitor {
    // NODE STATE
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE - per active
    std::ویکټور<AstCoverToggle*> m_toggleps;  // List of of all AstCoverToggle's

    // STATE - Statistic tracking
    VDouble0 m_statToggleJoins;  // Statistic tracking

    // METHODS

    فراغ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection
        // Hash all of the original signals we toggle cover
        لكل (ثابت AstCoverToggle* ثابت nodep : m_toggleps) dupFinder.ورننباسه(nodep->origp());
        إذا (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("coveragejoin") + ".hash", خطأ);
        // Find if there are any duplicates
        لكل (AstCoverToggle* nodep : m_toggleps) {
            // nodep->backp() is null if we already detected it's a duplicate and unlinked earlier
            إذا (!nodep->backp()) استمر;
            // Want to choose a base node, and keep finding duplicates that are identical.
            // This prevents making chains where a->b, then c->d, then b->c, as we'll
            // find a->b, a->c, a->d directly.
            طالما (ریښتیا) {
                ثابت خپلکاری dupit = dupFinder.پیدا_کړه_Duplicate(nodep->origp());
                إذا (dupit == dupFinder.پای_ټکی()) اكسر;
                ثابت Ast_غوټه* ثابت duporigp = dupit->دوهم;
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
                Ast_غوټه_Cover_Decl* ثابت datadeclp = nodep->incp()->declp()->معلومات_Decl_Thisp();
                removep->incp()->declp()->معلومات_Declp(datadeclp);
                UINFO(8, "   new " << removep->incp()->declp());
                // Mark the found node as a duplicate of the first node
                // (Not vice-versa as we have the iterator for the found node)
                removep->unlink_Fr_شا();
                VL_DO_DANGLING(ټېل_Deletep(removep), removep);
                ++m_statToggleJoins;
            }
        }
    }

    // VISITORS
    فراغ لیدنه(Ast_فعال* nodep) override {
        m_toggleps.clear();
        // Find all Coverage's
        iterateChildren(nodep);
        // Simplify
        detectDuplicates();
    }
    فراغ لیدنه(AstCoverToggle* nodep) override {
        m_toggleps.ټېل_شا(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    فراغ لیدنه(Ast_غوټه_بیان*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند CoverageJoinVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CoverageJoinVisitor() override {
        V3Stats::جمع_کړه_Stat("Coverage, Toggle points joined", m_statToggleJoins);
    }
};

//######################################################################
// Coverage class functions

فراغ V3CoverageJoin::coverageJoin(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageJoinVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("coveragejoin", 0, dump_ونه_Either_Level() >= 3);
}
