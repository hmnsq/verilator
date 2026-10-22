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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3CoverageJoin.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3DupFinder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CoverageJoin state, as a visitor of each AstNode

ਵਰਗ CoverageJoinVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE - per active
    std::ਸਦਿਸ਼<AstCoverToggle*> m_toggleps;  // List of of all AstCoverToggle's

    // STATE - Statistic tracking
    VDouble0 m_statToggleJoins;  // Statistic tracking

    // METHODS

    ਖਾਲੀ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection
        // Hash all of the original signals we toggle cover
        ਲਈ (ਅਚਲ AstCoverToggle* ਅਚਲ nodep : m_toggleps) dupFinder.ਪਾਓ(nodep->origp());
        ਜੇ (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("coveragejoin") + ".hash", ਝੂਠ);
        // Find if there are any duplicates
        ਲਈ (AstCoverToggle* nodep : m_toggleps) {
            // nodep->backp() is null if we already detected it's a duplicate and unlinked earlier
            ਜੇ (!nodep->backp()) ਜਾਰੀ;
            // Want to choose a base node, and keep finding duplicates that are identical.
            // This prevents making chains where a->b, then c->d, then b->c, as we'll
            // find a->b, a->c, a->d directly.
            ਜਦੋਂ (ਸੱਚ) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ dupit = dupFinder.ਲੱਭੋ_Duplicate(nodep->origp());
                ਜੇ (dupit == dupFinder.ਸਿਰਾ()) ਰੋਕੋ;
                ਅਚਲ Ast_ਗੰਢ* ਅਚਲ duporigp = dupit->ਦੂਜਾ;
                // Remove node from comparison so don't hit it again
                dupFinder.erase(dupit);
                //
                // Note dupFinder will point to the original toggle-increment equation (what's
                // duplicated), not the covertoggle, but we need to get back to the
                // covertoggle which is immediately above, so:
                AstCoverToggle* ਅਚਲ removep = VN_AS(duporigp->backp(), CoverToggle);
                UASSERT_OBJ(removep, nodep, "CoverageJoin duplicate of wrong type");
                UINFO(8, "  Orig " << nodep << " -->> " << nodep->incp()->declp());
                UINFO(8, "   dup " << removep << " -->> " << removep->incp()->declp());
                // The CoverDecl the duplicate pointed to now needs to point to the
                // original's data. I.e. the duplicate will get the coverage number
                // from the non-duplicate
                Ast_ਗੰਢ_Cover_Decl* ਅਚਲ datadeclp = nodep->incp()->declp()->ਅੰਕੜੇ_Decl_Thisp();
                removep->incp()->declp()->ਅੰਕੜੇ_Declp(datadeclp);
                UINFO(8, "   new " << removep->incp()->declp());
                // Mark the found node as a duplicate of the first node
                // (Not vice-versa as we have the iterator for the found node)
                removep->unlink_Fr_ਪਿੱਠ();
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(removep), removep);
                ++m_statToggleJoins;
            }
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        m_toggleps.clear();
        // Find all Coverage's
        iterateChildren(nodep);
        // Simplify
        detectDuplicates();
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle* nodep) override {
        m_toggleps.ਧੱਕੋ_ਪਿੱਠ(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CoverageJoinVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CoverageJoinVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Coverage, Toggle points joined", m_statToggleJoins);
    }
};

//######################################################################
// Coverage class functions

ਖਾਲੀ V3CoverageJoin::coverageJoin(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageJoinVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("coveragejoin", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
