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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"
#شامل "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_گره, typename T_Vertex, typename... Ops>
T_گره* make_گره(ثابت T_Vertex* vtxp, Ops... ops) {
    T_گره* ثابت nodep = جدید T_گره{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<صحیح>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    بازگشت nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_شمار_Ones* make_گره<Ast_شمار_Ones, Dfg_شمار_Ones, Ast_گره_عبارت*>(  //
    ثابت Dfg_شمار_Ones* vtxp, Ast_گره_عبارت* op1) {
    Ast_شمار_Ones* ثابت nodep = جدید Ast_شمار_Ones{vtxp->fileline(), op1};
    بازگشت nodep;
}

template <>
AstExtend* make_گره<AstExtend, DfgExtend, Ast_گره_عبارت*>(  //
    ثابت DfgExtend* vtxp, Ast_گره_عبارت* op1) {
    بازگشت جدید AstExtend{vtxp->fileline(), op1, static_cast<صحیح>(vtxp->width())};
}

template <>
AstExtendS* make_گره<AstExtendS, DfgExtendS, Ast_گره_عبارت*>(  //
    ثابت DfgExtendS* vtxp, Ast_گره_عبارت* op1) {
    بازگشت جدید AstExtendS{vtxp->fileline(), op1, static_cast<صحیح>(vtxp->width())};
}

template <>
AstShiftL* make_گره<AstShiftL, DfgShiftL, Ast_گره_عبارت*, Ast_گره_عبارت*>(  //
    ثابت DfgShiftL* vtxp, Ast_گره_عبارت* op1, Ast_گره_عبارت* op2) {
    بازگشت جدید AstShiftL{vtxp->fileline(), op1, op2, static_cast<صحیح>(vtxp->width())};
}

template <>
AstShiftR* make_گره<AstShiftR, DfgShiftR, Ast_گره_عبارت*, Ast_گره_عبارت*>(  //
    ثابت DfgShiftR* vtxp, Ast_گره_عبارت* op1, Ast_گره_عبارت* op2) {
    بازگشت جدید AstShiftR{vtxp->fileline(), op1, op2, static_cast<صحیح>(vtxp->width())};
}

template <>
AstShiftRS* make_گره<AstShiftRS, DfgShiftRS, Ast_گره_عبارت*, Ast_گره_عبارت*>(  //
    ثابت DfgShiftRS* vtxp, Ast_گره_عبارت* op1, Ast_گره_عبارت* op2) {
    بازگشت جدید AstShiftRS{vtxp->fileline(), op1, op2, static_cast<صحیح>(vtxp->width())};
}

}  // namespace

رده DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ثابت VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_گره_عبارت* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ایستا AstActive* getCombActive(AstScope* scopep) {
        اگر (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            برای (Ast_گره* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ثابت activep = VN_CAST(nodep, Active);
                اگر (!activep) ادامه;
                اگر (activep->hasCombo()) {
                    scopep->user2p(activep);
                    بشکن;
                }
            }
            // If there isn't one, create a new one
            اگر (!scopep->user2p()) {
                File_سطر* ثابت flp = scopep->fileline();
                Ast_Sen_درخت* ثابت senTreep
                    = جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ثابت activep = جدید AstActive{flp, "", senTreep};
                activep->sen_درخت_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        بازگشت VN_AS(scopep->user2p(), Active);
    }

    Ast_گره_عبارت* convert_Dfg_Vertex_To_Ast_گره_عبارت(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_گره_عبارت* ثابت resultp = m_resultp;
        m_resultp = nullptr;
        بازگشت resultp;
    }

    تهی createAssignment(File_سطر* flp, Ast_گره_عبارت* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_گره_عبارت* ثابت rhsp = convert_Dfg_Vertex_To_Ast_گره_عبارت(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_گره* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        اگر (m_alwaysp) {
            m_alwaysp->addStmtsp(جدید AstAssign{flp, lhsp, rhsp});
            بازگشت;
        }

        // Otherwise create an AssignW
        AstAssignW* ثابت ap = جدید AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(جدید AstAlways{ap});
    }

    تهی convertDriver(File_سطر* flp, Ast_گره_عبارت* lhsp, DfgVertex* driverp) {
        اگر (DfgSplicePacked* ثابت sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_سطر* dflp) {
                // Create Sel
                AstConst* ثابت lsbp = جدید AstConst{dflp, lo};
                ثابت صحیح width = static_cast<صحیح>(src.width());
                AstSel* ثابت nLhsp = جدید AstSel{dflp, lhsp->clone_درخت_Pure(نادرست), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_درخت(), nLhsp);
                بازگشت نادرست;
            });
            بازگشت;
        }

        اگر (DfgSpliceArray* ثابت sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_سطر* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ثابت idxp = جدید AstConst{dflp, lo};
                AstArraySel* ثابت nLhsp = جدید AstArraySel{dflp, lhsp->clone_درخت_Pure(نادرست), idxp};
                // Convert source
                اگر (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } وگرنه {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_درخت(), nLhsp);
                بازگشت نادرست;
            });
            بازگشت;
        }

        اگر (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ثابت idxp = جدید AstConst{flp, 0};
            AstArraySel* ثابت nLhsp = جدید AstArraySel{flp, lhsp->clone_درخت_Pure(نادرست), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_درخت(), nLhsp);
            بازگشت;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_درخت_Pure(نادرست), driverp);
    }

    // VISITORS
    تهی visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    تهی visit(DfgVarPacked* vtxp) override {
        m_resultp = جدید AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    تهی visit(DfgVarArray* vtxp) override {
        m_resultp = جدید AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    تهی visit(DfgPrev* vtxp) override {
        m_resultp = جدید AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    تهی visit(DfgConst* vtxp) override {  //
        m_resultp = جدید AstConst{vtxp->fileline(), vtxp->رقم()};
    }
    تهی visit(DfgCReset* vtxp) override {
        DfgVertex* ثابت sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ثابت varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = جدید AstCReset{vtxp->fileline(), varp, نادرست};
    }
    تهی visit(DfgMatchMasked* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گره_عبارت* ثابت lhsp = convert_Dfg_Vertex_To_Ast_گره_عبارت(vtxp->lhsp());
        AstVarScope* ثابت matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = جدید AstMatchMasked{flp, lhsp, جدید AstVarRef{flp, matchp, VAccess::READ}};
    }

    تهی visit(DfgRep* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گره_عبارت* ثابت srcp = convert_Dfg_Vertex_To_Ast_گره_عبارت(vtxp->srcp());
        m_resultp = جدید AstReplicate{flp, srcp, vtxp->شمار()};
    }

    تهی visit(DfgSel* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گره_عبارت* ثابت fromp = convert_Dfg_Vertex_To_Ast_گره_عبارت(vtxp->fromp());
        AstConst* ثابت lsbp = جدید AstConst{flp, vtxp->lsb()};
        m_resultp = جدید AstSel{flp, fromp, lsbp, static_cast<صحیح>(vtxp->width())};
    }

    تهی visit(DfgMux* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گره_عبارت* ثابت fromp = convert_Dfg_Vertex_To_Ast_گره_عبارت(vtxp->fromp());
        Ast_گره_عبارت* ثابت lsbp = convert_Dfg_Vertex_To_Ast_گره_عبارت(vtxp->lsbp());
        m_resultp = جدید AstSel{flp, fromp, lsbp, static_cast<صحیح>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#شامل "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        اگر (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        برای (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            اگر (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                ادامه;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_سطر* ثابت flp = vtx.driver_File_سطر() ? vtx.driver_File_سطر() : vtx.fileline();
            AstVarRef* ثابت lhsp = جدید AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            اگر (DfgVertex* ثابت defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = جدید AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_درخت_Pure(نادرست), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_درخت(), lhsp);
        }

        // Update Ast References
        برای (DfgVertexAst& vtx : dfg.astVertices()) {
            اگر (DfgAstRd* ثابت rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_گره_عبارت* ثابت exprp = convert_Dfg_Vertex_To_Ast_گره_عبارت(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                اگر (exprp->same_درخت(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                    ادامه;
                }
                // Replace the reference with the expression
                اگر (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } وگرنه {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_درخت();
                rVtxp->exprp(exprp);
            }
        }
    }

عمومی:
    ایستا تهی apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

تهی V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
