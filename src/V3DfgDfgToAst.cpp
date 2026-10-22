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
template <typename T_كلي, typename T_Vertex, typename... Ops>
T_كلي* make_كلي(ثابت T_Vertex* vtxp, Ops... ops) {
    T_كلي* ثابت nodep = جديد T_كلي{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->width() == static_cast<صحيح>(vtxp->width()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->width() << " vs " << vtxp->width());
    أرجع nodep;
}

//======================================================================
// Vertices needing special conversion

template <>
Ast_كدي_Ones* make_كلي<Ast_كدي_Ones, Dfg_كدي_Ones, Ast_كلي_فرچ*>(  //
    ثابت Dfg_كدي_Ones* vtxp, Ast_كلي_فرچ* op1) {
    Ast_كدي_Ones* ثابت nodep = جديد Ast_كدي_Ones{vtxp->fileline(), op1};
    أرجع nodep;
}

template <>
AstExtend* make_كلي<AstExtend, DfgExtend, Ast_كلي_فرچ*>(  //
    ثابت DfgExtend* vtxp, Ast_كلي_فرچ* op1) {
    أرجع جديد AstExtend{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->width())};
}

template <>
AstExtendS* make_كلي<AstExtendS, DfgExtendS, Ast_كلي_فرچ*>(  //
    ثابت DfgExtendS* vtxp, Ast_كلي_فرچ* op1) {
    أرجع جديد AstExtendS{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->width())};
}

template <>
AstShiftL* make_كلي<AstShiftL, DfgShiftL, Ast_كلي_فرچ*, Ast_كلي_فرچ*>(  //
    ثابت DfgShiftL* vtxp, Ast_كلي_فرچ* op1, Ast_كلي_فرچ* op2) {
    أرجع جديد AstShiftL{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->width())};
}

template <>
AstShiftR* make_كلي<AstShiftR, DfgShiftR, Ast_كلي_فرچ*, Ast_كلي_فرچ*>(  //
    ثابت DfgShiftR* vtxp, Ast_كلي_فرچ* op1, Ast_كلي_فرچ* op2) {
    أرجع جديد AstShiftR{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->width())};
}

template <>
AstShiftRS* make_كلي<AstShiftRS, DfgShiftRS, Ast_كلي_فرچ*, Ast_كلي_فرچ*>(  //
    ثابت DfgShiftRS* vtxp, Ast_كلي_فرچ* op1, Ast_كلي_فرچ* op2) {
    أرجع جديد AstShiftRS{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->width())};
}

}  // namespace

صنف DfgToAstVisitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ثابت VNUser2InUse m_user2InUse;

    // STATE
    V3DfgDfgToAstContext& m_ctx;  // The context for stats
    Ast_كلي_فرچ* m_resultp = nullptr;  // The result node of the current traversal
    AstAlways* m_alwaysp = nullptr;  // Process to add assignments to, if have a default driver
    AstActive* m_activep = nullptr;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ساكن AstActive* getCombActive(AstScope* scopep) {
        إذا (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            لكل (Ast_كلي* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ثابت activep = VN_CAST(nodep, Active);
                إذا (!activep) استمر;
                إذا (activep->hasCombo()) {
                    scopep->user2p(activep);
                    اكسر;
                }
            }
            // If there isn't one, create a new one
            إذا (!scopep->user2p()) {
                File_لي* ثابت flp = scopep->fileline();
                Ast_Sen_بشيا* ثابت senTreep
                    = جديد Ast_Sen_بشيا{flp, جديد AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ثابت activep = جديد AstActive{flp, "", senTreep};
                activep->sen_بشيا_Storep(senTreep);
                scopep->addBlocksp(activep);
                scopep->user2p(activep);
            }
        }
        أرجع VN_AS(scopep->user2p(), Active);
    }

    Ast_كلي_فرچ* convert_Dfg_Vertex_To_Ast_كلي_فرچ(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<DfgVertexVar>() || vtxp->is<DfgConst>()  //
                        || !vtxp->hasMultipleSinks() || vtxp->isCheaperThanLoad(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_كلي_فرچ* ثابت resultp = m_resultp;
        m_resultp = nullptr;
        أرجع resultp;
    }

    فراغ createAssignment(File_لي* flp, Ast_كلي_فرچ* lhsp, DfgVertex* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_resultEquations;
        // Render the driver
        Ast_كلي_فرچ* ثابت rhsp = convert_Dfg_Vertex_To_Ast_كلي_فرچ(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_كلي* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        إذا (m_alwaysp) {
            m_alwaysp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});
            أرجع;
        }

        // Otherwise create an AssignW
        AstAssignW* ثابت ap = جديد AstAssignW{flp, lhsp, rhsp};
        m_activep->addStmtsp(جديد AstAlways{ap});
    }

    فراغ convertDriver(File_لي* flp, Ast_كلي_فرچ* lhsp, DfgVertex* driverp) {
        إذا (DfgSplicePacked* ثابت sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_لي* dflp) {
                // Create Sel
                AstConst* ثابت lsbp = جديد AstConst{dflp, lo};
                ثابت صحيح width = static_cast<صحيح>(src.width());
                AstSel* ثابت nLhsp = جديد AstSel{dflp, lhsp->clone_بشيا_Pure(خطأ), lsbp, width};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_بشيا(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (DfgSpliceArray* ثابت sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](DfgVertex& src, uint32_t lo, File_لي* dflp) {
                UASSERT_OBJ(src.size() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ثابت idxp = جديد AstConst{dflp, lo};
                AstArraySel* ثابت nLhsp = جديد AstArraySel{dflp, lhsp->clone_بشيا_Pure(خطأ), idxp};
                // Convert source
                إذا (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } وإلا {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_بشيا(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ثابت idxp = جديد AstConst{flp, 0};
            AstArraySel* ثابت nLhsp = جديد AstArraySel{flp, lhsp->clone_بشيا_Pure(خطأ), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_بشيا(), nLhsp);
            أرجع;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_بشيا_Pure(خطأ), driverp);
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
        m_resultp = جديد AstConst{vtxp->fileline(), vtxp->لمب()};
    }
    فراغ visit(DfgCReset* vtxp) override {
        DfgVertex* ثابت sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<DfgVertexVar>(), sinkp, "CReset should drive a variable");
        AstVar* ثابت varp = sinkp->as<DfgVertexVar>()->vscp()->varp();
        m_resultp = جديد AstCReset{vtxp->fileline(), varp, خطأ};
    }
    فراغ visit(DfgMatchMasked* vtxp) override {
        File_لي* ثابت flp = vtxp->fileline();
        Ast_كلي_فرچ* ثابت lhsp = convert_Dfg_Vertex_To_Ast_كلي_فرچ(vtxp->lhsp());
        AstVarScope* ثابت matchp = vtxp->matchp()->as<DfgVertexVar>()->vscp();
        m_resultp = جديد AstMatchMasked{flp, lhsp, جديد AstVarRef{flp, matchp, VAccess::READ}};
    }

    فراغ visit(DfgRep* vtxp) override {
        File_لي* ثابت flp = vtxp->fileline();
        Ast_كلي_فرچ* ثابت srcp = convert_Dfg_Vertex_To_Ast_كلي_فرچ(vtxp->srcp());
        m_resultp = جديد AstReplicate{flp, srcp, vtxp->كدي()};
    }

    فراغ visit(DfgSel* vtxp) override {
        File_لي* ثابت flp = vtxp->fileline();
        Ast_كلي_فرچ* ثابت fromp = convert_Dfg_Vertex_To_Ast_كلي_فرچ(vtxp->fromp());
        AstConst* ثابت lsbp = جديد AstConst{flp, vtxp->lsb()};
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->width())};
    }

    فراغ visit(DfgMux* vtxp) override {
        File_لي* ثابت flp = vtxp->fileline();
        Ast_كلي_فرچ* ثابت fromp = convert_Dfg_Vertex_To_Ast_كلي_فرچ(vtxp->fromp());
        Ast_كلي_فرچ* ثابت lsbp = convert_Dfg_Vertex_To_Ast_كلي_فرچ(vtxp->lsbp());
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
            File_لي* ثابت flp = vtx.driver_File_لي() ? vtx.driver_File_لي() : vtx.fileline();
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
                createAssignment(vtx.fileline(), lhsp->clone_بشيا_Pure(خطأ), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_بشيا(), lhsp);
        }

        // Update Ast References
        لكل (DfgVertexAst& vtx : dfg.astVertices()) {
            إذا (DfgAstRd* ثابت rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_كلي_فرچ* ثابت exprp = convert_Dfg_Vertex_To_Ast_كلي_فرچ(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                إذا (exprp->same_بشيا(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_بشيا(), exprp);
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
                rVtxp->exprp()->delete_بشيا();
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
