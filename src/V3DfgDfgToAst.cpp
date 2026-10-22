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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgPasses.h"
#অন্তর্ভাব "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_গ্রন্থি, typename T_Vertex, typename... Ops>
T_গ্রন্থি* make_গ্রন্থি(ধ্রুবক T_Vertex* vtxp, Ops... ops) {
    T_গ্রন্থি* ধ্রুবক nodep = নতুন T_গ্রন্থি{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<পূর্ণসংখ্যা>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    ফেরত nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_গুনতি_Ones* make_গ্রন্থি<Ast_গুনতি_Ones, Dfg_গুনতি_Ones, Ast_গ্রন্থি_রাশিমালা*>(  //
    ধ্রুবক Dfg_গুনতি_Ones* vtxp, Ast_গ্রন্থি_রাশিমালা* op1) {
    Ast_গুনতি_Ones* ধ্রুবক nodep = নতুন Ast_গুনতি_Ones{vtxp->fileline(), op1};
    ফেরত nodep;
}

template <>
AstExtend* make_গ্রন্থি<AstExtend, DfgExtend, Ast_গ্রন্থি_রাশিমালা*>(  //
    ধ্রুবক DfgExtend* vtxp, Ast_গ্রন্থি_রাশিমালা* op1) {
    ফেরত নতুন AstExtend{vtxp->fileline(), op1, static_cast<পূর্ণসংখ্যা>(vtxp->width())};
}

template <>
AstExtendS* make_গ্রন্থি<AstExtendS, DfgExtendS, Ast_গ্রন্থি_রাশিমালা*>(  //
    ধ্রুবক DfgExtendS* vtxp, Ast_গ্রন্থি_রাশিমালা* op1) {
    ফেরত নতুন AstExtendS{vtxp->fileline(), op1, static_cast<পূর্ণসংখ্যা>(vtxp->width())};
}

template <>
AstShiftL* make_গ্রন্থি<AstShiftL, DfgShiftL, Ast_গ্রন্থি_রাশিমালা*, Ast_গ্রন্থি_রাশিমালা*>(  //
    ধ্রুবক DfgShiftL* vtxp, Ast_গ্রন্থি_রাশিমালা* op1, Ast_গ্রন্থি_রাশিমালা* op2) {
    ফেরত নতুন AstShiftL{vtxp->fileline(), op1, op2, static_cast<পূর্ণসংখ্যা>(vtxp->width())};
}

template <>
AstShiftR* make_গ্রন্থি<AstShiftR, DfgShiftR, Ast_গ্রন্থি_রাশিমালা*, Ast_গ্রন্থি_রাশিমালা*>(  //
    ধ্রুবক DfgShiftR* vtxp, Ast_গ্রন্থি_রাশিমালা* op1, Ast_গ্রন্থি_রাশিমালা* op2) {
    ফেরত নতুন AstShiftR{vtxp->fileline(), op1, op2, static_cast<পূর্ণসংখ্যা>(vtxp->width())};
}

template <>
AstShiftRS* make_গ্রন্থি<AstShiftRS, DfgShiftRS, Ast_গ্রন্থি_রাশিমালা*, Ast_গ্রন্থি_রাশিমালা*>(  //
    ধ্রুবক DfgShiftRS* vtxp, Ast_গ্রন্থি_রাশিমালা* op1, Ast_গ্রন্থি_রাশিমালা* op2) {
    ফেরত নতুন AstShiftRS{vtxp->fileline(), op1, op2, static_cast<পূর্ণসংখ্যা>(vtxp->width())};
}

}  // namespace

শ্রেণী DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ধ্রুবক VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_গ্রন্থি_রাশিমালা* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    স্থির AstActive* getCombActive(AstScope* scopep) {
        যদি (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            জন্য (Ast_গ্রন্থি* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ধ্রুবক activep = VN_CAST(nodep, Active);
                যদি (!activep) চালু;
                যদি (activep->hasCombo()) {
                    scopep->user2p(activep);
                    বিরতি;
                }
            }
            // If there isn't one, create a new one
            যদি (!scopep->user2p()) {
                File_সারি* ধ্রুবক flp = scopep->fileline();
                Ast_Sen_বৃক্ষ* ধ্রুবক senTreep
                    = নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ধ্রুবক activep = নতুন AstActive{flp, "", senTreep};
                activep->sen_বৃক্ষ_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        ফেরত VN_AS(scopep->user2p(), Active);
    }

    Ast_গ্রন্থি_রাশিমালা* convert_Dfg_Vertex_To_Ast_গ্রন্থি_রাশিমালা(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক resultp = m_resultp;
        m_resultp = nullptr;
        ফেরত resultp;
    }

    শূন্য createAssignment(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_ফলাফল_Equations;
        // Render the driver
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = convert_Dfg_Vertex_To_Ast_গ্রন্থি_রাশিমালা(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_গ্রন্থি* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        যদি (m_alwaysp) {
            m_alwaysp->addStmtsp(নতুন AstAssign{flp, lhsp, rhsp});
            ফেরত;
        }

        // Otherwise create an AssignW
        AstAssignW* ধ্রুবক ap = নতুন AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(নতুন AstAlways{ap});
    }

    শূন্য convertDriver(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* lhsp, DfgVertex* driverp) {
        যদি (DfgSplicePacked* ধ্রুবক sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_সারি* dflp) {
                // Create Sel
                AstConst* ধ্রুবক lsbp = নতুন AstConst{dflp, lo};
                ধ্রুবক পূর্ণসংখ্যা width = static_cast<পূর্ণসংখ্যা>(src.width());
                AstSel* ধ্রুবক nLhsp = নতুন AstSel{dflp, lhsp->clone_বৃক্ষ_Pure(মিথ্যা), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_বৃক্ষ(), nLhsp);
                ফেরত মিথ্যা;
            });
            ফেরত;
        }

        যদি (DfgSpliceArray* ধ্রুবক sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_সারি* dflp) {
                UASSERT_OBJ(src.আকার_2() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ধ্রুবক idxp = নতুন AstConst{dflp, lo};
                AstArraySel* ধ্রুবক nLhsp = নতুন AstArraySel{dflp, lhsp->clone_বৃক্ষ_Pure(মিথ্যা), idxp};
                // Convert source
                যদি (ধ্রুবক DfgUnitArray* ধ্রুবক uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } অন্যথা {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_বৃক্ষ(), nLhsp);
                ফেরত মিথ্যা;
            });
            ফেরত;
        }

        যদি (ধ্রুবক DfgUnitArray* ধ্রুবক uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ধ্রুবক idxp = নতুন AstConst{flp, 0};
            AstArraySel* ধ্রুবক nLhsp = নতুন AstArraySel{flp, lhsp->clone_বৃক্ষ_Pure(মিথ্যা), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_বৃক্ষ(), nLhsp);
            ফেরত;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_বৃক্ষ_Pure(মিথ্যা), driverp);
    }

    // VISITORS
    শূন্য visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    শূন্য visit(DfgVarPacked* vtxp) override {
        m_resultp = নতুন AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    শূন্য visit(DfgVarArray* vtxp) override {
        m_resultp = নতুন AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    শূন্য visit(Dfg_পূর্ব* vtxp) override {
        m_resultp = নতুন AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    শূন্য visit(DfgConst* vtxp) override {  //
        m_resultp = নতুন AstConst{vtxp->fileline(), vtxp->অঙ্ক()};
    }
    শূন্য visit(DfgCReset* vtxp) override {
        DfgVertex* ধ্রুবক sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ধ্রুবক varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = নতুন AstCReset{vtxp->fileline(), varp, মিথ্যা};
    }
    শূন্য visit(DfgMatchMasked* vtxp) override {
        File_সারি* ধ্রুবক flp = vtxp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = convert_Dfg_Vertex_To_Ast_গ্রন্থি_রাশিমালা(vtxp->lhsp());
        AstVarScope* ধ্রুবক matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = নতুন AstMatchMasked{flp, lhsp, নতুন AstVarRef{flp, matchp, VAccess::READ}};
    }

    শূন্য visit(DfgRep* vtxp) override {
        File_সারি* ধ্রুবক flp = vtxp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক srcp = convert_Dfg_Vertex_To_Ast_গ্রন্থি_রাশিমালা(vtxp->srcp());
        m_resultp = নতুন AstReplicate{flp, srcp, vtxp->গুনতি()};
    }

    শূন্য visit(DfgSel* vtxp) override {
        File_সারি* ধ্রুবক flp = vtxp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = convert_Dfg_Vertex_To_Ast_গ্রন্থি_রাশিমালা(vtxp->fromp());
        AstConst* ধ্রুবক lsbp = নতুন AstConst{flp, vtxp->lsb()};
        m_resultp = নতুন AstSel{flp, fromp, lsbp, static_cast<পূর্ণসংখ্যা>(vtxp->width())};
    }

    শূন্য visit(DfgMux* vtxp) override {
        File_সারি* ধ্রুবক flp = vtxp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = convert_Dfg_Vertex_To_Ast_গ্রন্থি_রাশিমালা(vtxp->fromp());
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lsbp = convert_Dfg_Vertex_To_Ast_গ্রন্থি_রাশিমালা(vtxp->lsbp());
        m_resultp = নতুন AstSel{flp, fromp, lsbp, static_cast<পূর্ণসংখ্যা>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#অন্তর্ভাব "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        যদি (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        জন্য (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            যদি (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                চালু;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_সারি* ধ্রুবক flp = vtx.driver_File_সারি() ? vtx.driver_File_সারি() : vtx.fileline();
            AstVarRef* ধ্রুবক lhsp = নতুন AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            যদি (DfgVertex* ধ্রুবক defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = নতুন AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_বৃক্ষ_Pure(মিথ্যা), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_বৃক্ষ(), lhsp);
        }

        // Update Ast References
        জন্য (DfgVertexAst& vtx : dfg.astVertices()) {
            যদি (DfgAstRd* ধ্রুবক rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = convert_Dfg_Vertex_To_Ast_গ্রন্থি_রাশিমালা(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                যদি (exprp->same_বৃক্ষ(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_বৃক্ষ(), exprp);
                    চালু;
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

সর্বজনীন:
    স্থির শূন্য apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

শূন্য V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
