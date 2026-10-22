// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert DfgGraph to AstModule
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
//
// Convert DfgGraph back to AstModule. We recursively construct AstNodeExpr expressions for each
// DfgVertex which represents a storage location (e.g.: DfgVarPacked), or has multiple sinks
// without driving a storage location (and hence needs a temporary variable to duplication). The
// recursion stops when we reach a DfgVertex representing a storage location (e.g.: DfgVarPacked),
// or a vertex that that has multiple sinks (as these nodes will have a [potentially new temporary]
// corresponding// storage location). Redundant variables (those whose source vertex drives
// multiple variables) are eliminated when possible. Vertices driving multiple variables are
// rendered once, driving an arbitrarily (but deterministically) chosen canonical variable, and the
// corresponding redundant variables are assigned from the canonical variable.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಗಂಟು, ಬಗೆ_ಹೆಸರು T_ಶೃಂಗ, ಬಗೆ_ಹೆಸರು... Ops>
T_ಗಂಟು* make_ಗಂಟು(ಸ್ಥಿರಾಂಕ T_ಶೃಂಗ* vtxp, Ops... ops) {
    T_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = ಹೊಸ T_ಗಂಟು{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->ಅಗಲ() == static_cast<ಪೂರ್ಣಾಂಕ>(vtxp->ಅಗಲ()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->ಅಗಲ() << " vs " << vtxp->ಅಗಲ());
    ಹಿಂತಿರುಗಿ nodep;
}

//======================================================================
// Vertices needing special conversion

ಮಾದರಿ <>
Ast_ಎಣಿಕೆ_Ones* make_ಗಂಟು<Ast_ಎಣಿಕೆ_Ones, Dfg_ಎಣಿಕೆ_Ones, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಸ್ಥಿರಾಂಕ Dfg_ಎಣಿಕೆ_Ones* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1) {
    Ast_ಎಣಿಕೆ_Ones* ಸ್ಥಿರಾಂಕ nodep = ಹೊಸ Ast_ಎಣಿಕೆ_Ones{vtxp->fileline(), op1};
    ಹಿಂತಿರುಗಿ nodep;
}

ಮಾದರಿ <>
AstExtend* make_ಗಂಟು<AstExtend, DfgExtend, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಸ್ಥಿರಾಂಕ DfgExtend* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1) {
    ಹಿಂತಿರುಗಿ ಹೊಸ AstExtend{vtxp->fileline(), op1, static_cast<ಪೂರ್ಣಾಂಕ>(vtxp->ಅಗಲ())};
}

ಮಾದರಿ <>
AstExtendS* make_ಗಂಟು<AstExtendS, DfgExtendS, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಸ್ಥಿರಾಂಕ DfgExtendS* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1) {
    ಹಿಂತಿರುಗಿ ಹೊಸ AstExtendS{vtxp->fileline(), op1, static_cast<ಪೂರ್ಣಾಂಕ>(vtxp->ಅಗಲ())};
}

ಮಾದರಿ <>
Ast_ಸರಿಸು_L* make_ಗಂಟು<Ast_ಸರಿಸು_L, Dfg_ಸರಿಸು_L, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಸ್ಥಿರಾಂಕ Dfg_ಸರಿಸು_L* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op2) {
    ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಸರಿಸು_L{vtxp->fileline(), op1, op2, static_cast<ಪೂರ್ಣಾಂಕ>(vtxp->ಅಗಲ())};
}

ಮಾದರಿ <>
Ast_ಸರಿಸು_R* make_ಗಂಟು<Ast_ಸರಿಸು_R, Dfg_ಸರಿಸು_R, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಸ್ಥಿರಾಂಕ Dfg_ಸರಿಸು_R* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op2) {
    ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಸರಿಸು_R{vtxp->fileline(), op1, op2, static_cast<ಪೂರ್ಣಾಂಕ>(vtxp->ಅಗಲ())};
}

ಮಾದರಿ <>
Ast_ಸರಿಸು_RS* make_ಗಂಟು<Ast_ಸರಿಸು_RS, Dfg_ಸರಿಸು_RS, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಸ್ಥಿರಾಂಕ Dfg_ಸರಿಸು_RS* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op2) {
    ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಸರಿಸು_RS{vtxp->fileline(), op1, op2, static_cast<ಪೂರ್ಣಾಂಕ>(vtxp->ಅಗಲ())};
}

}  // namespace

ವರ್ಗ Dfg_ಗಮ್ಯ_Ast_Visitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;

    // STATE
    V3_Dfg_Dfg_ಗಮ್ಯ_Ast_Context& m_ctx;  // The context for stats
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_resultp = ಶೂನ್ಯಸೂಚಕ;  // The result node of the current traversal
    AstAlways* m_alwaysp = ಶೂನ್ಯಸೂಚಕ;  // Process to add assignments to, if have a default driver
    Ast_ಸಕ್ರಿಯ* m_activep = ಶೂನ್ಯಸೂಚಕ;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ಸ್ಥಿರ Ast_ಸಕ್ರಿಯ* get_Comb_ಸಕ್ರಿಯ(AstScope* scopep) {
        ಆದರೆ (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            ಗಾಗಿ (Ast_ಗಂಟು* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = VN_CAST(nodep, ಸಕ್ರಿಯ);
                ಆದರೆ (!activep) ಮುಂದುವರಿ;
                ಆದರೆ (activep->hasCombo()) {
                    scopep->user2p(activep);
                    ನಿಲ್ಲಿಸು;
                }
            }
            // If there isn't one, create a new one
            ಆದರೆ (!scopep->user2p()) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = scopep->fileline();
                Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep
                    = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, Ast_Sen_ಅಂಶ::Combo{}}};
                Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "", senTreep};
                activep->sen_ವೃಕ್ಷ_Storep(senTreep);
                scopep->ಕೂಡಿಸು_Blocksp(activep);
                scopep->user2p(activep);
            }
        }
        ಹಿಂತಿರುಗಿ VN_AS(scopep->user2p(), ಸಕ್ರಿಯ);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* convert_Dfg_ಶೃಂಗ_ಗಮ್ಯ_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(Dfg_ಶೃಂಗ* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<Dfg_ಶೃಂಗ_Var>() || vtxp->is<DfgConst>()  //
                        || !vtxp->has_ಅಪವರ್ತ್ಯ_Sinks() || vtxp->is_Cheaper_Than_ತುಂಬು(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ resultp = m_resultp;
        m_resultp = ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ resultp;
    }

    ಶೂನ್ಯ createAssignment(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Dfg_ಶೃಂಗ* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_ಫಲಿತಾಂಶ_Equations;
        // Render the driver
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = convert_Dfg_ಶೃಂಗ_ಗಮ್ಯ_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_ಗಂಟು* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        ಆದರೆ (m_alwaysp) {
            m_alwaysp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
            ಹಿಂತಿರುಗಿ;
        }

        // Otherwise create an AssignW
        Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ap = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{flp, lhsp, rhsp};
        m_activep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{ap});
    }

    ಶೂನ್ಯ convertDriver(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Dfg_ಶೃಂಗ* driverp) {
        ಆದರೆ (DfgSplicePacked* ಸ್ಥಿರಾಂಕ sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](Dfg_ಶೃಂಗ& src, uint32_t lo, File_ಸಾಲು* dflp) {
                // Create Sel
                AstConst* ಸ್ಥಿರಾಂಕ lsbp = ಹೊಸ AstConst{dflp, lo};
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = static_cast<ಪೂರ್ಣಾಂಕ>(src.ಅಗಲ());
                AstSel* ಸ್ಥಿರಾಂಕ nLhsp = ಹೊಸ AstSel{dflp, lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), lsbp, ಅಗಲ};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ವೃಕ್ಷ(), nLhsp);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (DfgSpliceArray* ಸ್ಥಿರಾಂಕ sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](Dfg_ಶೃಂಗ& src, uint32_t lo, File_ಸಾಲು* dflp) {
                UASSERT_OBJ(src.ಗಾತ್ರ_2() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ಸ್ಥಿರಾಂಕ idxp = ಹೊಸ AstConst{dflp, lo};
                AstArraySel* ಸ್ಥಿರಾಂಕ nLhsp = ಹೊಸ AstArraySel{dflp, lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), idxp};
                // Convert source
                ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgUnitArray* ಸ್ಥಿರಾಂಕ uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ವೃಕ್ಷ(), nLhsp);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgUnitArray* ಸ್ಥಿರಾಂಕ uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ಸ್ಥಿರಾಂಕ idxp = ಹೊಸ AstConst{flp, 0};
            AstArraySel* ಸ್ಥಿರಾಂಕ nLhsp = ಹೊಸ AstArraySel{flp, lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_ವೃಕ್ಷ(), nLhsp);
            ಹಿಂತಿರುಗಿ;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), driverp);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಶೃಂಗ* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    ಶೂನ್ಯ ಭೇಟಿ(DfgVarPacked* vtxp) override {
        m_resultp = ಹೊಸ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgVarArray* vtxp) override {
        m_resultp = ಹೊಸ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgPrev* vtxp) override {
        m_resultp = ಹೊಸ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgConst* vtxp) override {  //
        m_resultp = ಹೊಸ AstConst{vtxp->fileline(), vtxp->ಅಂಕೆ()};
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgCReset* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<Dfg_ಶೃಂಗ_Var>(), sinkp, "CReset should drive a variable");
        AstVar* ಸ್ಥಿರಾಂಕ varp = sinkp->as<Dfg_ಶೃಂಗ_Var>()->vscp()->varp();
        m_resultp = ಹೊಸ AstCReset{vtxp->fileline(), varp, ಸುಳ್ಳು};
    }
    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಹೊಂದಿಕೆ_Masked* vtxp) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = convert_Dfg_ಶೃಂಗ_ಗಮ್ಯ_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->lhsp());
        AstVarScope* ಸ್ಥಿರಾಂಕ matchp = vtxp->matchp()->as<Dfg_ಶೃಂಗ_Var>()->vscp();
        m_resultp = ಹೊಸ Ast_ಹೊಂದಿಕೆ_Masked{flp, lhsp, ಹೊಸ AstVarRef{flp, matchp, VAccess::READ}};
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgRep* vtxp) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ srcp = convert_Dfg_ಶೃಂಗ_ಗಮ್ಯ_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->srcp());
        m_resultp = ಹೊಸ AstReplicate{flp, srcp, vtxp->ಎಣಿಕೆ_2()};
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgSel* vtxp) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = convert_Dfg_ಶೃಂಗ_ಗಮ್ಯ_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->fromp());
        AstConst* ಸ್ಥಿರಾಂಕ lsbp = ಹೊಸ AstConst{flp, vtxp->lsb()};
        m_resultp = ಹೊಸ AstSel{flp, fromp, lsbp, static_cast<ಪೂರ್ಣಾಂಕ>(vtxp->ಅಗಲ())};
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgMux* vtxp) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = convert_Dfg_ಶೃಂಗ_ಗಮ್ಯ_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->fromp());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lsbp = convert_Dfg_ಶೃಂಗ_ಗಮ್ಯ_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->lsbp());
        m_resultp = ಹೊಸ AstSel{flp, fromp, lsbp, static_cast<ಪೂರ್ಣಾಂಕ>(vtxp->ಅಗಲ())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#ಅನ್ತರ್ಭಾವ "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    Dfg_ಗಮ್ಯ_Ast_Visitor(Dfg_ನಕ್ಷೆ& dfg, V3_Dfg_Dfg_ಗಮ್ಯ_Ast_Context& ctx)
        : m_ctx{ctx} {
        ಆದರೆ (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            ಆದರೆ (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                ಮುಂದುವರಿ;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtx.driver_File_ಸಾಲು() ? vtx.driver_File_ಸಾಲು() : vtx.fileline();
            AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = get_Comb_ಸಕ್ರಿಯ(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = ಹೊಸ AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, ಶೂನ್ಯಸೂಚಕ};
                m_activep->ಕೂಡಿಸು_Stmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_ವೃಕ್ಷ(), lhsp);
        }

        // Update Ast References
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Ast& vtx : dfg.astVertices()) {
            ಆದರೆ (DfgAstRd* ಸ್ಥಿರಾಂಕ rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = convert_Dfg_ಶೃಂಗ_ಗಮ್ಯ_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                ಆದರೆ (exprp->same_ವೃಕ್ಷ(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_ವೃಕ್ಷ(), exprp);
                    ಮುಂದುವರಿ;
                }
                // Replace the reference with the expression
                ಆದರೆ (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_ವೃಕ್ಷ();
                rVtxp->exprp(exprp);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Dfg_ನಕ್ಷೆ& dfg, V3_Dfg_Dfg_ಗಮ್ಯ_Ast_Context& ctx) { Dfg_ಗಮ್ಯ_Ast_Visitor{dfg, ctx}; }
};

ಶೂನ್ಯ V3DfgPasses::dfg_ಗಮ್ಯ_Ast(Dfg_ನಕ್ಷೆ& dfg, V3DfgContext& ctx) {
    Dfg_ಗಮ್ಯ_Ast_Visitor::apply(dfg, ctx.m_dfg2AstContext);
}
