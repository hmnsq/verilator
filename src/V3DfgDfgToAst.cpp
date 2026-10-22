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
template <typename T_ಗಂಟು, typename T_Vertex, typename... Ops>
T_ಗಂಟು* make_ಗಂಟು(ಧ್ರುವ T_Vertex* vtxp, Ops... ops) {
    T_ಗಂಟು* ಧ್ರುವ nodep = ನೂತನ T_ಗಂಟು{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    ಪ್ರತ್ಯಾಗಮ nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_ಎಣಿಕೆ_Ones* make_ಗಂಟು<Ast_ಎಣಿಕೆ_Ones, Dfg_ಎಣಿಕೆ_Ones, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಧ್ರುವ Dfg_ಎಣಿಕೆ_Ones* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1) {
    Ast_ಎಣಿಕೆ_Ones* ಧ್ರುವ nodep = ನೂತನ Ast_ಎಣಿಕೆ_Ones{vtxp->fileline(), op1};
    ಪ್ರತ್ಯಾಗಮ nodep;
}

template <>
AstExtend* make_ಗಂಟು<AstExtend, DfgExtend, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಧ್ರುವ DfgExtend* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1) {
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstExtend{vtxp->fileline(), op1, static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtxp->width())};
}

template <>
AstExtendS* make_ಗಂಟು<AstExtendS, DfgExtendS, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಧ್ರುವ DfgExtendS* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1) {
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstExtendS{vtxp->fileline(), op1, static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtxp->width())};
}

template <>
AstShiftL* make_ಗಂಟು<AstShiftL, DfgShiftL, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಧ್ರುವ DfgShiftL* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op2) {
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstShiftL{vtxp->fileline(), op1, op2, static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtxp->width())};
}

template <>
AstShiftR* make_ಗಂಟು<AstShiftR, DfgShiftR, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಧ್ರುವ DfgShiftR* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op2) {
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstShiftR{vtxp->fileline(), op1, op2, static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtxp->width())};
}

template <>
AstShiftRS* make_ಗಂಟು<AstShiftRS, DfgShiftRS, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  //
    ಧ್ರುವ DfgShiftRS* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op1, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* op2) {
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstShiftRS{vtxp->fileline(), op1, op2, static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtxp->width())};
}

}  // namespace

ವರ್ಗ DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ಧ್ರುವ VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ಸ್ಥಿರ AstActive* getCombActive(AstScope* scopep) {
        ಯದಿ (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            ಪ್ರತಿ (Ast_ಗಂಟು* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ಧ್ರುವ activep = VN_CAST(nodep, Active);
                ಯದಿ (!activep) ಅನುವರ್ತ;
                ಯದಿ (activep->hasCombo()) {
                    scopep->user2p(activep);
                    ಭಙ್ಗ;
                }
            }
            // If there isn't one, create a new one
            ಯದಿ (!scopep->user2p()) {
                File_ಸಾಲು* ಧ್ರುವ flp = scopep->fileline();
                Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep
                    = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "", senTreep};
                activep->sen_ವೃಕ್ಷ_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        ಪ್ರತ್ಯಾಗಮ VN_AS(scopep->user2p(), Active);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* convert_Dfg_Vertex_To_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resultp = m_resultp;
        m_resultp = nullptr;
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    ಶೂನ್ಯ createAssignment(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = convert_Dfg_Vertex_To_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_ಗಂಟು* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        ಯದಿ (m_alwaysp) {
            m_alwaysp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, rhsp});
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Otherwise create an AssignW
        AstAssignW* ಧ್ರುವ ap = ನೂತನ AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(ನೂತನ AstAlways{ap});
    }

    ಶೂನ್ಯ convertDriver(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, DfgVertex* driverp) {
        ಯದಿ (DfgSplicePacked* ಧ್ರುವ sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_ಸಾಲು* dflp) {
                // Create Sel
                AstConst* ಧ್ರುವ lsbp = ನೂತನ AstConst{dflp, lo};
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = static_cast<ಪೂರ್ಣಾಙ್ಕ>(src.width());
                AstSel* ಧ್ರುವ nLhsp = ನೂತನ AstSel{dflp, lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ವೃಕ್ಷ(), nLhsp);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (DfgSpliceArray* ಧ್ರುವ sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_ಸಾಲು* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ಧ್ರುವ idxp = ನೂತನ AstConst{dflp, lo};
                AstArraySel* ಧ್ರುವ nLhsp = ನೂತನ AstArraySel{dflp, lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), idxp};
                // Convert source
                ಯದಿ (ಧ್ರುವ DfgUnitArray* ಧ್ರುವ uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } ಅನ್ಯಥಾ {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ವೃಕ್ಷ(), nLhsp);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (ಧ್ರುವ DfgUnitArray* ಧ್ರುವ uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ಧ್ರುವ idxp = ನೂತನ AstConst{flp, 0};
            AstArraySel* ಧ್ರುವ nLhsp = ನೂತನ AstArraySel{flp, lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_ವೃಕ್ಷ(), nLhsp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), driverp);
    }

    // VISITORS
    ಶೂನ್ಯ visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    ಶೂನ್ಯ visit(DfgVarPacked* vtxp) override {
        m_resultp = ನೂತನ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ಶೂನ್ಯ visit(DfgVarArray* vtxp) override {
        m_resultp = ನೂತನ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ಶೂನ್ಯ visit(DfgPrev* vtxp) override {
        m_resultp = ನೂತನ AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    ಶೂನ್ಯ visit(DfgConst* vtxp) override {  //
        m_resultp = ನೂತನ AstConst{vtxp->fileline(), vtxp->ಅಂಕೆ()};
    }
    ಶೂನ್ಯ visit(DfgCReset* vtxp) override {
        DfgVertex* ಧ್ರುವ sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ಧ್ರುವ varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = ನೂತನ AstCReset{vtxp->fileline(), varp, ಅಸತ್ಯ};
    }
    ಶೂನ್ಯ visit(DfgMatchMasked* vtxp) override {
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = convert_Dfg_Vertex_To_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->lhsp());
        AstVarScope* ಧ್ರುವ matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = ನೂತನ AstMatchMasked{flp, lhsp, ನೂತನ AstVarRef{flp, matchp, VAccess::READ}};
    }

    ಶೂನ್ಯ visit(DfgRep* vtxp) override {
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ srcp = convert_Dfg_Vertex_To_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->srcp());
        m_resultp = ನೂತನ AstReplicate{flp, srcp, vtxp->ಎಣಿಕೆ()};
    }

    ಶೂನ್ಯ visit(DfgSel* vtxp) override {
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = convert_Dfg_Vertex_To_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->fromp());
        AstConst* ಧ್ರುವ lsbp = ನೂತನ AstConst{flp, vtxp->lsb()};
        m_resultp = ನೂತನ AstSel{flp, fromp, lsbp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtxp->width())};
    }

    ಶೂನ್ಯ visit(DfgMux* vtxp) override {
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = convert_Dfg_Vertex_To_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->fromp());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsbp = convert_Dfg_Vertex_To_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(vtxp->lsbp());
        m_resultp = ನೂತನ AstSel{flp, fromp, lsbp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#ಅನ್ತರ್ಭಾವ "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        ಯದಿ (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        ಪ್ರತಿ (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            ಯದಿ (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                ಅನುವರ್ತ;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_ಸಾಲು* ಧ್ರುವ flp = vtx.driver_File_ಸಾಲು() ? vtx.driver_File_ಸಾಲು() : vtx.fileline();
            AstVarRef* ಧ್ರುವ lhsp = ನೂತನ AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            ಯದಿ (DfgVertex* ಧ್ರುವ defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = ನೂತನ AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_ವೃಕ್ಷ(), lhsp);
        }

        // Update Ast References
        ಪ್ರತಿ (DfgVertexAst& vtx : dfg.astVertices()) {
            ಯದಿ (DfgAstRd* ಧ್ರುವ rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = convert_Dfg_Vertex_To_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                ಯದಿ (exprp->same_ವೃಕ್ಷ(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_ವೃಕ್ಷ(), exprp);
                    ಅನುವರ್ತ;
                }
                // Replace the reference with the expression
                ಯದಿ (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } ಅನ್ಯಥಾ {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_ವೃಕ್ಷ();
                rVtxp->exprp(exprp);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

ಶೂನ್ಯ V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
