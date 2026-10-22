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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
template <typename T_گرێ, typename T_Vertex, typename... Ops>
T_گرێ* make_گرێ(ثابت T_Vertex* vtxp, Ops... ops) {
    T_گرێ* ثابت nodep = جديد T_گرێ{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<صحيح>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    أرجع nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_ژماردن_Ones* make_گرێ<Ast_ژماردن_Ones, Dfg_ژماردن_Ones, Ast_گرێ_دەربڕین*>(  //
    ثابت Dfg_ژماردن_Ones* vtxp, Ast_گرێ_دەربڕین* op1) {
    Ast_ژماردن_Ones* ثابت nodep = جديد Ast_ژماردن_Ones{vtxp->fileline(), op1};
    أرجع nodep;
}

template <>
AstExtend* make_گرێ<AstExtend, DfgExtend, Ast_گرێ_دەربڕین*>(  //
    ثابت DfgExtend* vtxp, Ast_گرێ_دەربڕین* op1) {
    أرجع جديد AstExtend{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->width())};
}

template <>
AstExtendS* make_گرێ<AstExtendS, DfgExtendS, Ast_گرێ_دەربڕین*>(  //
    ثابت DfgExtendS* vtxp, Ast_گرێ_دەربڕین* op1) {
    أرجع جديد AstExtendS{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->width())};
}

template <>
AstShiftL* make_گرێ<AstShiftL, DfgShiftL, Ast_گرێ_دەربڕین*, Ast_گرێ_دەربڕین*>(  //
    ثابت DfgShiftL* vtxp, Ast_گرێ_دەربڕین* op1, Ast_گرێ_دەربڕین* op2) {
    أرجع جديد AstShiftL{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->width())};
}

template <>
AstShiftR* make_گرێ<AstShiftR, DfgShiftR, Ast_گرێ_دەربڕین*, Ast_گرێ_دەربڕین*>(  //
    ثابت DfgShiftR* vtxp, Ast_گرێ_دەربڕین* op1, Ast_گرێ_دەربڕین* op2) {
    أرجع جديد AstShiftR{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->width())};
}

template <>
AstShiftRS* make_گرێ<AstShiftRS, DfgShiftRS, Ast_گرێ_دەربڕین*, Ast_گرێ_دەربڕین*>(  //
    ثابت DfgShiftRS* vtxp, Ast_گرێ_دەربڕین* op1, Ast_گرێ_دەربڕین* op2) {
    أرجع جديد AstShiftRS{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->width())};
}

}  // namespace

صنف DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ثابت VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_گرێ_دەربڕین* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ساكن AstActive* getCombActive(AstScope* scopep) {
        إذا (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            لكل (Ast_گرێ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ثابت activep = VN_CAST(nodep, Active);
                إذا (!activep) استمر;
                إذا (activep->hasCombo()) {
                    scopep->user2p(activep);
                    اكسر;
                }
            }
            // If there isn't one, create a new one
            إذا (!scopep->user2p()) {
                File_هێڵ* ثابت flp = scopep->fileline();
                Ast_Sen_درەخت* ثابت senTreep
                    = جديد Ast_Sen_درەخت{flp, جديد AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ثابت activep = جديد AstActive{flp, "", senTreep};
                activep->sen_درەخت_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        أرجع VN_AS(scopep->user2p(), Active);
    }

    Ast_گرێ_دەربڕین* convert_Dfg_Vertex_To_Ast_گرێ_دەربڕین(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_گرێ_دەربڕین* ثابت resultp = m_resultp;
        m_resultp = nullptr;
        أرجع resultp;
    }

    فراغ createAssignment(File_هێڵ* flp, Ast_گرێ_دەربڕین* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_گرێ_دەربڕین* ثابت rhsp = convert_Dfg_Vertex_To_Ast_گرێ_دەربڕین(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_گرێ* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        إذا (m_alwaysp) {
            m_alwaysp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
            أرجع;
        }

        // Otherwise create an AssignW
        AstAssignW* ثابت ap = جديد AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(جديد AstAlways{ap});
    }

    فراغ convertDriver(File_هێڵ* flp, Ast_گرێ_دەربڕین* lhsp, DfgVertex* driverp) {
        إذا (DfgSplicePacked* ثابت sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_هێڵ* dflp) {
                // Create Sel
                AstConst* ثابت lsbp = جديد AstConst{dflp, lo};
                ثابت صحيح width = static_cast<صحيح>(src.width());
                AstSel* ثابت nLhsp = جديد AstSel{dflp, lhsp->clone_درەخت_Pure(خطأ), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_درەخت(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (DfgSpliceArray* ثابت sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_هێڵ* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ثابت idxp = جديد AstConst{dflp, lo};
                AstArraySel* ثابت nLhsp = جديد AstArraySel{dflp, lhsp->clone_درەخت_Pure(خطأ), idxp};
                // Convert source
                إذا (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } وإلا {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_درەخت(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ثابت idxp = جديد AstConst{flp, 0};
            AstArraySel* ثابت nLhsp = جديد AstArraySel{flp, lhsp->clone_درەخت_Pure(خطأ), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_درەخت(), nLhsp);
            أرجع;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_درەخت_Pure(خطأ), driverp);
    }

    // VISITORS
    فراغ visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ visit(DfgVarPacked* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ visit(DfgVarArray* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ visit(DfgPrev* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ visit(DfgConst* vtxp) override {  //
        m_resultp = جديد AstConst{vtxp->fileline(), vtxp->ژمارە()};
    }
    فراغ visit(DfgCReset* vtxp) override {
        DfgVertex* ثابت sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ثابت varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = جديد AstCReset{vtxp->fileline(), varp, خطأ};
    }
    فراغ visit(DfgMatchMasked* vtxp) override {
        File_هێڵ* ثابت flp = vtxp->fileline();
        Ast_گرێ_دەربڕین* ثابت lhsp = convert_Dfg_Vertex_To_Ast_گرێ_دەربڕین(vtxp->lhsp());
        AstVarScope* ثابت matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = جديد AstMatchMasked{flp, lhsp, جديد AstVarRef{flp, matchp, VAccess::READ}};
    }

    فراغ visit(DfgRep* vtxp) override {
        File_هێڵ* ثابت flp = vtxp->fileline();
        Ast_گرێ_دەربڕین* ثابت srcp = convert_Dfg_Vertex_To_Ast_گرێ_دەربڕین(vtxp->srcp());
        m_resultp = جديد AstReplicate{flp, srcp, vtxp->ژماردن()};
    }

    فراغ visit(DfgSel* vtxp) override {
        File_هێڵ* ثابت flp = vtxp->fileline();
        Ast_گرێ_دەربڕین* ثابت fromp = convert_Dfg_Vertex_To_Ast_گرێ_دەربڕین(vtxp->fromp());
        AstConst* ثابت lsbp = جديد AstConst{flp, vtxp->lsb()};
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->width())};
    }

    فراغ visit(DfgMux* vtxp) override {
        File_هێڵ* ثابت flp = vtxp->fileline();
        Ast_گرێ_دەربڕین* ثابت fromp = convert_Dfg_Vertex_To_Ast_گرێ_دەربڕین(vtxp->fromp());
        Ast_گرێ_دەربڕین* ثابت lsbp = convert_Dfg_Vertex_To_Ast_گرێ_دەربڕین(vtxp->lsbp());
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->width())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#ضمن "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    DfgToAstVisitor(DfgGraph& dfg, V3DfgDfgToAstContext& ctx)
        : m_ctx{ctx} {
        إذا (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        لكل (DfgVertexVar& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            إذا (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                استمر;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_هێڵ* ثابت flp = vtx.driver_File_هێڵ() ? vtx.driver_File_هێڵ() : vtx.fileline();
            AstVarRef* ثابت lhsp = جديد AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = getCombActive(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            إذا (DfgVertex* ثابت defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = جديد AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr};
                m_activep->addStmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_درەخت_Pure(خطأ), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_درەخت(), lhsp);
        }

        // Update Ast References
        لكل (DfgVertexAst& vtx : dfg.astVertices()) {
            إذا (DfgAstRd* ثابت rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_گرێ_دەربڕین* ثابت exprp = convert_Dfg_Vertex_To_Ast_گرێ_دەربڕین(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                إذا (exprp->same_درەخت(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_درەخت(), exprp);
                    استمر;
                }
                // Replace the reference with the expression
                إذا (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } وإلا {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_درەخت();
                rVtxp->exprp(exprp);
            }
        }
    }

عام:
    ساكن فراغ apply(DfgGraph& dfg, V3DfgDfgToAstContext& ctx) { DfgToAstVisitor{dfg, ctx}; }
};

فراغ V3DfgPasses::dfgToAst(DfgGraph& dfg, V3DfgContext& ctx) {
    DfgToAstVisitor::apply(dfg, ctx.m_dfg2AstContext);
}
