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
کينډۍ <ډول_نوم T_غوټه, ډول_نوم T_راس, ډول_نوم... Ops>
T_غوټه* make_غوټه(ثابت T_راس* vtxp, Ops... ops) {
    T_غوټه* ثابت nodep = جديد T_غوټه{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->پلنوالی() == static_cast<صحيح>(vtxp->پلنوالی()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->پلنوالی() << " vs " << vtxp->پلنوالی());
    أرجع nodep;
}

//======================================================================
// Vertices needing special conversion

کينډۍ <>
Ast_شمېر_Ones* make_غوټه<Ast_شمېر_Ones, Dfg_شمېر_Ones, Ast_غوټه_بیان*>(  //
    ثابت Dfg_شمېر_Ones* vtxp, Ast_غوټه_بیان* op1) {
    Ast_شمېر_Ones* ثابت nodep = جديد Ast_شمېر_Ones{vtxp->fileline(), op1};
    أرجع nodep;
}

کينډۍ <>
AstExtend* make_غوټه<AstExtend, DfgExtend, Ast_غوټه_بیان*>(  //
    ثابت DfgExtend* vtxp, Ast_غوټه_بیان* op1) {
    أرجع جديد AstExtend{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->پلنوالی())};
}

کينډۍ <>
AstExtendS* make_غوټه<AstExtendS, DfgExtendS, Ast_غوټه_بیان*>(  //
    ثابت DfgExtendS* vtxp, Ast_غوټه_بیان* op1) {
    أرجع جديد AstExtendS{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->پلنوالی())};
}

کينډۍ <>
Ast_ښويدنه_L* make_غوټه<Ast_ښويدنه_L, Dfg_ښويدنه_L, Ast_غوټه_بیان*, Ast_غوټه_بیان*>(  //
    ثابت Dfg_ښويدنه_L* vtxp, Ast_غوټه_بیان* op1, Ast_غوټه_بیان* op2) {
    أرجع جديد Ast_ښويدنه_L{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->پلنوالی())};
}

کينډۍ <>
Ast_ښويدنه_R* make_غوټه<Ast_ښويدنه_R, Dfg_ښويدنه_R, Ast_غوټه_بیان*, Ast_غوټه_بیان*>(  //
    ثابت Dfg_ښويدنه_R* vtxp, Ast_غوټه_بیان* op1, Ast_غوټه_بیان* op2) {
    أرجع جديد Ast_ښويدنه_R{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->پلنوالی())};
}

کينډۍ <>
Ast_ښويدنه_RS* make_غوټه<Ast_ښويدنه_RS, Dfg_ښويدنه_RS, Ast_غوټه_بیان*, Ast_غوټه_بیان*>(  //
    ثابت Dfg_ښويدنه_RS* vtxp, Ast_غوټه_بیان* op1, Ast_غوټه_بیان* op2) {
    أرجع جديد Ast_ښويدنه_RS{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->پلنوالی())};
}

}  // namespace

صنف Dfg_منزل_Ast_Visitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ثابت VNUser2_دننه_Use m_user2_دننه_Use;

    // STATE
    V3_Dfg_Dfg_منزل_Ast_Context& m_ctx;  // The context for stats
    Ast_غوټه_بیان* m_resultp = تش_اشاره;  // The result node of the current traversal
    AstAlways* m_alwaysp = تش_اشاره;  // Process to add assignments to, if have a default driver
    Ast_فعال* m_activep = تش_اشاره;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ساكن Ast_فعال* get_Comb_فعال(AstScope* scopep) {
        إذا (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            لكل (Ast_غوټه* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_فعال* ثابت activep = VN_CAST(nodep, فعال);
                إذا (!activep) استمر;
                إذا (activep->hasCombo()) {
                    scopep->user2p(activep);
                    اكسر;
                }
            }
            // If there isn't one, create a new one
            إذا (!scopep->user2p()) {
                File_کرښه* ثابت flp = scopep->fileline();
                Ast_Sen_ونه* ثابت senTreep
                    = جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, Ast_Sen_توکی::Combo{}}};
                Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "", senTreep};
                activep->sen_ونه_Storep(senTreep);
                scopep->جمع_کړه_Blocksp(activep);
                scopep->user2p(activep);
            }
        }
        أرجع VN_AS(scopep->user2p(), فعال);
    }

    Ast_غوټه_بیان* convert_Dfg_راس_منزل_Ast_غوټه_بیان(Dfg_راس* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<Dfg_راس_Var>() || vtxp->is<DfgConst>()  //
                        || !vtxp->has_مضرب_Sinks() || vtxp->is_Cheaper_Than_بارول(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_غوټه_بیان* ثابت resultp = m_resultp;
        m_resultp = تش_اشاره;
        أرجع resultp;
    }

    فراغ createAssignment(File_کرښه* flp, Ast_غوټه_بیان* lhsp, Dfg_راس* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_پایله_Equations;
        // Render the driver
        Ast_غوټه_بیان* ثابت rhsp = convert_Dfg_راس_منزل_Ast_غوټه_بیان(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_غوټه* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        إذا (m_alwaysp) {
            m_alwaysp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, lhsp, rhsp});
            أرجع;
        }

        // Otherwise create an AssignW
        Ast_ټاکل_W* ثابت ap = جديد Ast_ټاکل_W{flp, lhsp, rhsp};
        m_activep->جمع_کړه_Stmtsp(جديد AstAlways{ap});
    }

    فراغ convertDriver(File_کرښه* flp, Ast_غوټه_بیان* lhsp, Dfg_راس* driverp) {
        إذا (DfgSplicePacked* ثابت sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_کرښه* dflp) {
                // Create Sel
                AstConst* ثابت lsbp = جديد AstConst{dflp, lo};
                ثابت صحيح پلنوالی = static_cast<صحيح>(src.پلنوالی());
                AstSel* ثابت nLhsp = جديد AstSel{dflp, lhsp->clone_ونه_خالص(خطأ), lsbp, پلنوالی};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ونه(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (DfgSpliceArray* ثابت sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_کرښه* dflp) {
                UASSERT_OBJ(src.اندازه() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ثابت idxp = جديد AstConst{dflp, lo};
                AstArraySel* ثابت nLhsp = جديد AstArraySel{dflp, lhsp->clone_ونه_خالص(خطأ), idxp};
                // Convert source
                إذا (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } وإلا {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_ونه(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ثابت idxp = جديد AstConst{flp, 0};
            AstArraySel* ثابت nLhsp = جديد AstArraySel{flp, lhsp->clone_ونه_خالص(خطأ), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_ونه(), nLhsp);
            أرجع;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_ونه_خالص(خطأ), driverp);
    }

    // VISITORS
    فراغ لیدنه(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ لیدنه(DfgVarPacked* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ لیدنه(DfgVarArray* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ لیدنه(DfgPrev* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ لیدنه(DfgConst* vtxp) override {  //
        m_resultp = جديد AstConst{vtxp->fileline(), vtxp->عدد()};
    }
    فراغ لیدنه(DfgCReset* vtxp) override {
        Dfg_راس* ثابت sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<Dfg_راس_Var>(), sinkp, "CReset should drive a variable");
        AstVar* ثابت varp = sinkp->as<Dfg_راس_Var>()->vscp()->varp();
        m_resultp = جديد AstCReset{vtxp->fileline(), varp, خطأ};
    }
    فراغ لیدنه(Dfg_سمون_Masked* vtxp) override {
        File_کرښه* ثابت flp = vtxp->fileline();
        Ast_غوټه_بیان* ثابت lhsp = convert_Dfg_راس_منزل_Ast_غوټه_بیان(vtxp->lhsp());
        AstVarScope* ثابت matchp = vtxp->matchp()->as<Dfg_راس_Var>()->vscp();
        m_resultp = جديد Ast_سمون_Masked{flp, lhsp, جديد AstVarRef{flp, matchp, VAccess::READ}};
    }

    فراغ لیدنه(DfgRep* vtxp) override {
        File_کرښه* ثابت flp = vtxp->fileline();
        Ast_غوټه_بیان* ثابت srcp = convert_Dfg_راس_منزل_Ast_غوټه_بیان(vtxp->srcp());
        m_resultp = جديد AstReplicate{flp, srcp, vtxp->شمېر()};
    }

    فراغ لیدنه(DfgSel* vtxp) override {
        File_کرښه* ثابت flp = vtxp->fileline();
        Ast_غوټه_بیان* ثابت fromp = convert_Dfg_راس_منزل_Ast_غوټه_بیان(vtxp->fromp());
        AstConst* ثابت lsbp = جديد AstConst{flp, vtxp->lsb()};
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->پلنوالی())};
    }

    فراغ لیدنه(DfgMux* vtxp) override {
        File_کرښه* ثابت flp = vtxp->fileline();
        Ast_غوټه_بیان* ثابت fromp = convert_Dfg_راس_منزل_Ast_غوټه_بیان(vtxp->fromp());
        Ast_غوټه_بیان* ثابت lsbp = convert_Dfg_راس_منزل_Ast_غوټه_بیان(vtxp->lsbp());
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->پلنوالی())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#ضمن "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    Dfg_منزل_Ast_Visitor(Dfg_ګراف& dfg, V3_Dfg_Dfg_منزل_Ast_Context& ctx)
        : m_ctx{ctx} {
        إذا (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            إذا (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                استمر;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_کرښه* ثابت flp = vtx.driver_File_کرښه() ? vtx.driver_File_کرښه() : vtx.fileline();
            AstVarRef* ثابت lhsp = جديد AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = get_Comb_فعال(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            إذا (Dfg_راس* ثابت defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = جديد AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, تش_اشاره};
                m_activep->جمع_کړه_Stmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_ونه_خالص(خطأ), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_ونه(), lhsp);
        }

        // Update Ast References
        لكل (Dfg_راس_Ast& vtx : dfg.astVertices()) {
            إذا (DfgAstRd* ثابت rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_غوټه_بیان* ثابت exprp = convert_Dfg_راس_منزل_Ast_غوټه_بیان(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                إذا (exprp->same_ونه(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_ونه(), exprp);
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
                rVtxp->exprp()->delete_ونه();
                rVtxp->exprp(exprp);
            }
        }
    }

عام:
    ساكن فراغ apply(Dfg_ګراف& dfg, V3_Dfg_Dfg_منزل_Ast_Context& ctx) { Dfg_منزل_Ast_Visitor{dfg, ctx}; }
};

فراغ V3DfgPasses::dfg_منزل_Ast(Dfg_ګراف& dfg, V3DfgContext& ctx) {
    Dfg_منزل_Ast_Visitor::apply(dfg, ctx.m_dfg2AstContext);
}
