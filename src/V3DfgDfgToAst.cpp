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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Dfg.h"
#অন্তর্ভাৱ "V3DfgPasses.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_গ্ৰন্থি, typename T_Vertex, typename... Ops>
T_গ্ৰন্থি* make_গ্ৰন্থি(ধ্রুৱ T_Vertex* vtxp, Ops... ops) {
    T_গ্ৰন্থি* ধ্রুৱ nodep = নূতন T_গ্ৰন্থি{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<পূর্ণাঙ্ক>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    ঘূৰাই nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_গুনতি_Ones* make_গ্ৰন্থি<Ast_গুনতি_Ones, Dfg_গুনতি_Ones, Ast_গ্ৰন্থি_ৰাশিমালা*>(  //
    ধ্রুৱ Dfg_গুনতি_Ones* vtxp, Ast_গ্ৰন্থি_ৰাশিমালা* op1) {
    Ast_গুনতি_Ones* ধ্রুৱ nodep = নূতন Ast_গুনতি_Ones{vtxp->fileline(), op1};
    ঘূৰাই nodep;
}

template <>
AstExtend* make_গ্ৰন্থি<AstExtend, DfgExtend, Ast_গ্ৰন্থি_ৰাশিমালা*>(  //
    ধ্রুৱ DfgExtend* vtxp, Ast_গ্ৰন্থি_ৰাশিমালা* op1) {
    ঘূৰাই নূতন AstExtend{vtxp->fileline(), op1, static_cast<পূর্ণাঙ্ক>(vtxp->width())};
}

template <>
AstExtendS* make_গ্ৰন্থি<AstExtendS, DfgExtendS, Ast_গ্ৰন্থি_ৰাশিমালা*>(  //
    ধ্রুৱ DfgExtendS* vtxp, Ast_গ্ৰন্থি_ৰাশিমালা* op1) {
    ঘূৰাই নূতন AstExtendS{vtxp->fileline(), op1, static_cast<পূর্ণাঙ্ক>(vtxp->width())};
}

template <>
AstShiftL* make_গ্ৰন্থি<AstShiftL, DfgShiftL, Ast_গ্ৰন্থি_ৰাশিমালা*, Ast_গ্ৰন্থি_ৰাশিমালা*>(  //
    ধ্রুৱ DfgShiftL* vtxp, Ast_গ্ৰন্থি_ৰাশিমালা* op1, Ast_গ্ৰন্থি_ৰাশিমালা* op2) {
    ঘূৰাই নূতন AstShiftL{vtxp->fileline(), op1, op2, static_cast<পূর্ণাঙ্ক>(vtxp->width())};
}

template <>
AstShiftR* make_গ্ৰন্থি<AstShiftR, DfgShiftR, Ast_গ্ৰন্থি_ৰাশিমালা*, Ast_গ্ৰন্থি_ৰাশিমালা*>(  //
    ধ্রুৱ DfgShiftR* vtxp, Ast_গ্ৰন্থি_ৰাশিমালা* op1, Ast_গ্ৰন্থি_ৰাশিমালা* op2) {
    ঘূৰাই নূতন AstShiftR{vtxp->fileline(), op1, op2, static_cast<পূর্ণাঙ্ক>(vtxp->width())};
}

template <>
AstShiftRS* make_গ্ৰন্থি<AstShiftRS, DfgShiftRS, Ast_গ্ৰন্থি_ৰাশিমালা*, Ast_গ্ৰন্থি_ৰাশিমালা*>(  //
    ধ্রুৱ DfgShiftRS* vtxp, Ast_গ্ৰন্থি_ৰাশিমালা* op1, Ast_গ্ৰন্থি_ৰাশিমালা* op2) {
    ঘূৰাই নূতন AstShiftRS{vtxp->fileline(), op1, op2, static_cast<পূর্ণাঙ্ক>(vtxp->width())};
}

}  // namespace

শ্ৰেণী DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ধ্রুৱ VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_গ্ৰন্থি_ৰাশিমালা* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    স্থির AstActive* getCombActive(AstScope* scopep) {
        যদি (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            বাবে (Ast_গ্ৰন্থি* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ধ্রুৱ activep = VN_CAST(nodep, Active);
                যদি (!activep) অনুৱর্ত;
                যদি (activep->hasCombo()) {
                    scopep->user2p(activep);
                    ভঙ্গ;
                }
            }
            // If there isn't one, create a new one
            যদি (!scopep->user2p()) {
                File_সাৰি* ধ্রুৱ flp = scopep->fileline();
                Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep
                    = নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ধ্রুৱ activep = নূতন AstActive{flp, "", senTreep};
                activep->sen_বৃক্ষ_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        ঘূৰাই VN_AS(scopep->user2p(), Active);
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* convert_Dfg_Vertex_To_Ast_গ্ৰন্থি_ৰাশিমালা(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ resultp = m_resultp;
        m_resultp = nullptr;
        ঘূৰাই resultp;
    }

    শূন্য createAssignment(File_সাৰি* flp, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = convert_Dfg_Vertex_To_Ast_গ্ৰন্থি_ৰাশিমালা(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_গ্ৰন্থি* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        যদি (m_alwaysp) {
            m_alwaysp->addStmtsp(নূতন AstAssign{flp, lhsp, rhsp});
            ঘূৰাই;
        }

        // Otherwise create an AssignW
        AstAssignW* ধ্রুৱ ap = নূতন AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(নূতন AstAlways{ap});
    }

    শূন্য convertDriver(File_সাৰি* flp, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, DfgVertex* driverp) {
        যদি (DfgSplicePacked* ধ্রুৱ sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_সাৰি* dflp) {
                // Create Sel
                AstConst* ধ্রুৱ lsbp = নূতন AstConst{dflp, lo};
                ধ্রুৱ পূর্ণাঙ্ক width = static_cast<পূর্ণাঙ্ক>(src.width());
                AstSel* ধ্রুৱ nLhsp = নূতন AstSel{dflp, lhsp->clone_বৃক্ষ_Pure(মিছা), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_বৃক্ষ(), nLhsp);
                ঘূৰাই মিছা;
            });
            ঘূৰাই;
        }

        যদি (DfgSpliceArray* ধ্রুৱ sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_সাৰি* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ধ্রুৱ idxp = নূতন AstConst{dflp, lo};
                AstArraySel* ধ্রুৱ nLhsp = নূতন AstArraySel{dflp, lhsp->clone_বৃক্ষ_Pure(মিছা), idxp};
                // Convert source
                যদি (ধ্রুৱ DfgUnitArray* ধ্রুৱ uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } অন্যথা {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_বৃক্ষ(), nLhsp);
                ঘূৰাই মিছা;
            });
            ঘূৰাই;
        }

        যদি (ধ্রুৱ DfgUnitArray* ধ্রুৱ uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ধ্রুৱ idxp = নূতন AstConst{flp, 0};
            AstArraySel* ধ্রুৱ nLhsp = নূতন AstArraySel{flp, lhsp->clone_বৃক্ষ_Pure(মিছা), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_বৃক্ষ(), nLhsp);
            ঘূৰাই;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_বৃক্ষ_Pure(মিছা), driverp);
    }

    // VISITORS
    শূন্য visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    শূন্য visit(DfgVarPacked* vtxp) override {
        m_resultp = নূতন AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    শূন্য visit(DfgVarArray* vtxp) override {
        m_resultp = নূতন AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    শূন্য visit(DfgPrev* vtxp) override {
        m_resultp = নূতন AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    শূন্য visit(DfgConst* vtxp) override {  //
        m_resultp = নূতন AstConst{vtxp->fileline(), vtxp->অঙ্ক()};
    }
    শূন্য visit(DfgCReset* vtxp) override {
        DfgVertex* ধ্রুৱ sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ধ্রুৱ varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = নূতন AstCReset{vtxp->fileline(), varp, মিছা};
    }
    শূন্য visit(DfgMatchMasked* vtxp) override {
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = convert_Dfg_Vertex_To_Ast_গ্ৰন্থি_ৰাশিমালা(vtxp->lhsp());
        AstVarScope* ধ্রুৱ matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = নূতন AstMatchMasked{flp, lhsp, নূতন AstVarRef{flp, matchp, VAccess::READ}};
    }

    শূন্য visit(DfgRep* vtxp) override {
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ srcp = convert_Dfg_Vertex_To_Ast_গ্ৰন্থি_ৰাশিমালা(vtxp->srcp());
        m_resultp = নূতন AstReplicate{flp, srcp, vtxp->গুনতি()};
    }

    শূন্য visit(DfgSel* vtxp) override {
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = convert_Dfg_Vertex_To_Ast_গ্ৰন্থি_ৰাশিমালা(vtxp->fromp());
        AstConst* ধ্রুৱ lsbp = নূতন AstConst{flp, vtxp->lsb()};
        m_resultp = নূতন AstSel{flp, fromp, lsbp, static_cast<পূর্ণাঙ্ক>(vtxp->width())};
    }

    শূন্য visit(DfgMux* vtxp) override {
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = convert_Dfg_Vertex_To_Ast_গ্ৰন্থি_ৰাশিমালা(vtxp->fromp());
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lsbp = convert_Dfg_Vertex_To_Ast_গ্ৰন্থি_ৰাশিমালা(vtxp->lsbp());
        m_resultp = নূতন AstSel{flp, fromp, lsbp, static_cast<পূর্ণাঙ্ক>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#অন্তর্ভাৱ "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        যদি (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        বাবে (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            যদি (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                অনুৱর্ত;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_সাৰি* ধ্রুৱ flp = vtx.driver_File_সাৰি() ? vtx.driver_File_সাৰি() : vtx.fileline();
            AstVarRef* ধ্রুৱ lhsp = নূতন AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            যদি (DfgVertex* ধ্রুৱ defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = নূতন AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_বৃক্ষ_Pure(মিছা), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_বৃক্ষ(), lhsp);
        }

        // Update Ast References
        বাবে (DfgVertexAst& vtx : dfg.astVertices()) {
            যদি (DfgAstRd* ধ্রুৱ rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = convert_Dfg_Vertex_To_Ast_গ্ৰন্থি_ৰাশিমালা(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                যদি (exprp->same_বৃক্ষ(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_বৃক্ষ(), exprp);
                    অনুৱর্ত;
                }
                // Replace the reference with the expression
                যদি (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } অন্যথা {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_বৃক্ষ();
                rVtxp->exprp(exprp);
            }
        }
    }

সার্ৱজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

শূন্য V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
