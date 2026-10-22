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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3CoverageJoin.h"

#अन्तर्भाव "V3DupFinder.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// CoverageJoin state, as a visitor of each AstNode

वर्ग CoverageJoinVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE - per active
    std::vector<AstCoverToggle*> m_toggleps;  // List of of all AstCoverToggle's

    // STATE - Statistic tracking
    VDouble0 m_statToggleJoins;  // Statistic tracking

    // METHODS

    शून्य detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection
        // Hash all of the original signals we toggle cover
        लागि (अचर AstCoverToggle* अचर nodep : m_toggleps) dupFinder.घुसाउ(nodep->origp());
        यदि (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("coveragejoin") + ".hash", असत्य);
        // Find if there are any duplicates
        लागि (AstCoverToggle* nodep : m_toggleps) {
            // nodep->backp() is null if we already detected it's a duplicate and unlinked earlier
            यदि (!nodep->backp()) जारी;
            // Want to choose a base node, and keep finding duplicates that are identical.
            // This prevents making chains where a->b, then c->d, then b->c, as we'll
            // find a->b, a->c, a->d directly.
            जबसम्म (सत्य) {
                अचर स्वतः dupit = dupFinder.findDuplicate(nodep->origp());
                यदि (dupit == dupFinder.end()) रोक;
                अचर Ast_गाँठो* अचर duporigp = dupit->second;
                // Remove node from comparison so don't hit it again
                dupFinder.erase(dupit);
                //
                // Note dupFinder will point to the original toggle-increment equation (what's
                // duplicated), not the covertoggle, but we need to get back to the
                // covertoggle which is immediately above, so:
                AstCoverToggle* अचर removep = VN_AS(duporigp->backp(), CoverToggle);
                UASSERT_OBJ(removep, nodep, "CoverageJoin duplicate of wrong type");
                UINFO(8, "  Orig " << nodep << " -->> " << nodep->incp()->declp());
                UINFO(8, "   dup " << removep << " -->> " << removep->incp()->declp());
                // The CoverDecl the duplicate pointed to now needs to point to the
                // original's data. I.e. the duplicate will get the coverage number
                // from the non-duplicate
                Ast_गाँठो_Cover_Decl* अचर datadeclp = nodep->incp()->declp()->dataDeclThisp();
                removep->incp()->declp()->dataDeclp(datadeclp);
                UINFO(8, "   new " << removep->incp()->declp());
                // Mark the found node as a duplicate of the first node
                // (Not vice-versa as we have the iterator for the found node)
                removep->unlinkFrBack();
                VL_DO_DANGLING(थप_Deletep(removep), removep);
                ++m_statToggleJoins;
            }
        }
    }

    // VISITORS
    शून्य visit(AstActive* nodep) override {
        m_toggleps.clear();
        // Find all Coverage's
        iterateChildren(nodep);
        // Simplify
        detectDuplicates();
    }
    शून्य visit(AstCoverToggle* nodep) override {
        m_toggleps.थप_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    शून्य visit(Ast_गाँठो_अभिव्यक्ति*) override {}  // Accelerate
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CoverageJoinVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CoverageJoinVisitor() override {
        V3Stats::addStat("Coverage, Toggle points joined", m_statToggleJoins);
    }
};

//######################################################################
// Coverage class functions

शून्य V3CoverageJoin::coverageJoin(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    { CoverageJoinVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("coveragejoin", 0, dump_रूख_Either_Level() >= 3);
}
