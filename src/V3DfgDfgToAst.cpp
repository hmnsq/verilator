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
سانچو <قسم_نالو T_ڳنڍ, قسم_نالو T_راس, قسم_نالو... Ops>
T_ڳنڍ* make_ڳنڍ(ثابت T_راس* vtxp, Ops... ops) {
    T_ڳنڍ* ثابت nodep = جديد T_ڳنڍ{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->ويڪر() == static_cast<صحيح>(vtxp->ويڪر()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->ويڪر() << " vs " << vtxp->ويڪر());
    أرجع nodep;
}

//======================================================================
// Vertices needing special conversion

سانچو <>
Ast_ڳڻپ_Ones* make_ڳنڍ<Ast_ڳڻپ_Ones, Dfg_ڳڻپ_Ones, Ast_ڳنڍ_اظهار*>(  //
    ثابت Dfg_ڳڻپ_Ones* vtxp, Ast_ڳنڍ_اظهار* op1) {
    Ast_ڳڻپ_Ones* ثابت nodep = جديد Ast_ڳڻپ_Ones{vtxp->fileline(), op1};
    أرجع nodep;
}

سانچو <>
AstExtend* make_ڳنڍ<AstExtend, DfgExtend, Ast_ڳنڍ_اظهار*>(  //
    ثابت DfgExtend* vtxp, Ast_ڳنڍ_اظهار* op1) {
    أرجع جديد AstExtend{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->ويڪر())};
}

سانچو <>
AstExtendS* make_ڳنڍ<AstExtendS, DfgExtendS, Ast_ڳنڍ_اظهار*>(  //
    ثابت DfgExtendS* vtxp, Ast_ڳنڍ_اظهار* op1) {
    أرجع جديد AstExtendS{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->ويڪر())};
}

سانچو <>
Ast_سرڪايو_L* make_ڳنڍ<Ast_سرڪايو_L, Dfg_سرڪايو_L, Ast_ڳنڍ_اظهار*, Ast_ڳنڍ_اظهار*>(  //
    ثابت Dfg_سرڪايو_L* vtxp, Ast_ڳنڍ_اظهار* op1, Ast_ڳنڍ_اظهار* op2) {
    أرجع جديد Ast_سرڪايو_L{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->ويڪر())};
}

سانچو <>
Ast_سرڪايو_R* make_ڳنڍ<Ast_سرڪايو_R, Dfg_سرڪايو_R, Ast_ڳنڍ_اظهار*, Ast_ڳنڍ_اظهار*>(  //
    ثابت Dfg_سرڪايو_R* vtxp, Ast_ڳنڍ_اظهار* op1, Ast_ڳنڍ_اظهار* op2) {
    أرجع جديد Ast_سرڪايو_R{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->ويڪر())};
}

سانچو <>
Ast_سرڪايو_RS* make_ڳنڍ<Ast_سرڪايو_RS, Dfg_سرڪايو_RS, Ast_ڳنڍ_اظهار*, Ast_ڳنڍ_اظهار*>(  //
    ثابت Dfg_سرڪايو_RS* vtxp, Ast_ڳنڍ_اظهار* op1, Ast_ڳنڍ_اظهار* op2) {
    أرجع جديد Ast_سرڪايو_RS{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->ويڪر())};
}

}  // namespace

صنف Dfg_منزل_Ast_Visitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ثابت VNUser2_اندريون_Use m_user2_اندريون_Use;

    // STATE
    V3_Dfg_Dfg_منزل_Ast_Context& m_ctx;  // The context for stats
    Ast_ڳنڍ_اظهار* m_resultp = خالي_اشارو;  // The result node of the current traversal
    AstAlways* m_alwaysp = خالي_اشارو;  // Process to add assignments to, if have a default driver
    Ast_سرگرم* m_activep = خالي_اشارو;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ساكن Ast_سرگرم* get_Comb_سرگرم(AstScope* scopep) {
        إذا (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            لكل (Ast_ڳنڍ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_سرگرم* ثابت activep = VN_CAST(nodep, سرگرم);
                إذا (!activep) استمر;
                إذا (activep->hasCombo()) {
                    scopep->user2p(activep);
                    اكسر;
                }
            }
            // If there isn't one, create a new one
            إذا (!scopep->user2p()) {
                File_سٽ* ثابت flp = scopep->fileline();
                Ast_Sen_وڻ* ثابت senTreep
                    = جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, Ast_Sen_شي::Combo{}}};
                Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "", senTreep};
                activep->sen_وڻ_Storep(senTreep);
                scopep->جوڙيو_Blocksp(activep);
                scopep->user2p(activep);
            }
        }
        أرجع VN_AS(scopep->user2p(), سرگرم);
    }

    Ast_ڳنڍ_اظهار* convert_Dfg_راس_منزل_Ast_ڳنڍ_اظهار(Dfg_راس* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<Dfg_راس_Var>() || vtxp->is<DfgConst>()  //
                        || !vtxp->has_گڻو_Sinks() || vtxp->is_Cheaper_Than_ڀريو(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_ڳنڍ_اظهار* ثابت resultp = m_resultp;
        m_resultp = خالي_اشارو;
        أرجع resultp;
    }

    فراغ createAssignment(File_سٽ* flp, Ast_ڳنڍ_اظهار* lhsp, Dfg_راس* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_نتيجو_Equations;
        // Render the driver
        Ast_ڳنڍ_اظهار* ثابت rhsp = convert_Dfg_راس_منزل_Ast_ڳنڍ_اظهار(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_ڳنڍ* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        إذا (m_alwaysp) {
            m_alwaysp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, lhsp, rhsp});
            أرجع;
        }

        // Otherwise create an AssignW
        Ast_سونپيو_W* ثابت ap = جديد Ast_سونپيو_W{flp, lhsp, rhsp};
        m_activep->جوڙيو_Stmtsp(جديد AstAlways{ap});
    }

    فراغ convertDriver(File_سٽ* flp, Ast_ڳنڍ_اظهار* lhsp, Dfg_راس* driverp) {
        إذا (DfgSplicePacked* ثابت sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سٽ* dflp) {
                // Create Sel
                AstConst* ثابت lsbp = جديد AstConst{dflp, lo};
                ثابت صحيح ويڪر = static_cast<صحيح>(src.ويڪر());
                AstSel* ثابت nLhsp = جديد AstSel{dflp, lhsp->clone_وڻ_خالص(خطأ), lsbp, ويڪر};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_وڻ(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (DfgSpliceArray* ثابت sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سٽ* dflp) {
                UASSERT_OBJ(src.سائيز() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ثابت idxp = جديد AstConst{dflp, lo};
                AstArraySel* ثابت nLhsp = جديد AstArraySel{dflp, lhsp->clone_وڻ_خالص(خطأ), idxp};
                // Convert source
                إذا (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } وإلا {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_وڻ(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ثابت idxp = جديد AstConst{flp, 0};
            AstArraySel* ثابت nLhsp = جديد AstArraySel{flp, lhsp->clone_وڻ_خالص(خطأ), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_وڻ(), nLhsp);
            أرجع;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_وڻ_خالص(خطأ), driverp);
    }

    // VISITORS
    فراغ گهمو(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ گهمو(DfgVarPacked* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ گهمو(DfgVarArray* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ گهمو(DfgPrev* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ گهمو(DfgConst* vtxp) override {  //
        m_resultp = جديد AstConst{vtxp->fileline(), vtxp->انگ()};
    }
    فراغ گهمو(DfgCReset* vtxp) override {
        Dfg_راس* ثابت sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<Dfg_راس_Var>(), sinkp, "CReset should drive a variable");
        AstVar* ثابت varp = sinkp->as<Dfg_راس_Var>()->vscp()->varp();
        m_resultp = جديد AstCReset{vtxp->fileline(), varp, خطأ};
    }
    فراغ گهمو(Dfg_ميلاپ_Masked* vtxp) override {
        File_سٽ* ثابت flp = vtxp->fileline();
        Ast_ڳنڍ_اظهار* ثابت lhsp = convert_Dfg_راس_منزل_Ast_ڳنڍ_اظهار(vtxp->lhsp());
        AstVarScope* ثابت matchp = vtxp->matchp()->as<Dfg_راس_Var>()->vscp();
        m_resultp = جديد Ast_ميلاپ_Masked{flp, lhsp, جديد AstVarRef{flp, matchp, VAccess::READ}};
    }

    فراغ گهمو(DfgRep* vtxp) override {
        File_سٽ* ثابت flp = vtxp->fileline();
        Ast_ڳنڍ_اظهار* ثابت srcp = convert_Dfg_راس_منزل_Ast_ڳنڍ_اظهار(vtxp->srcp());
        m_resultp = جديد AstReplicate{flp, srcp, vtxp->ڳڻپ()};
    }

    فراغ گهمو(DfgSel* vtxp) override {
        File_سٽ* ثابت flp = vtxp->fileline();
        Ast_ڳنڍ_اظهار* ثابت fromp = convert_Dfg_راس_منزل_Ast_ڳنڍ_اظهار(vtxp->fromp());
        AstConst* ثابت lsbp = جديد AstConst{flp, vtxp->lsb()};
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->ويڪر())};
    }

    فراغ گهمو(DfgMux* vtxp) override {
        File_سٽ* ثابت flp = vtxp->fileline();
        Ast_ڳنڍ_اظهار* ثابت fromp = convert_Dfg_راس_منزل_Ast_ڳنڍ_اظهار(vtxp->fromp());
        Ast_ڳنڍ_اظهار* ثابت lsbp = convert_Dfg_راس_منزل_Ast_ڳنڍ_اظهار(vtxp->lsbp());
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->ويڪر())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#ضمن "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    Dfg_منزل_Ast_Visitor(Dfg_گراف& dfg, V3_Dfg_Dfg_منزل_Ast_Context& ctx)
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
            File_سٽ* ثابت flp = vtx.driver_File_سٽ() ? vtx.driver_File_سٽ() : vtx.fileline();
            AstVarRef* ثابت lhsp = جديد AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = get_Comb_سرگرم(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            إذا (Dfg_راس* ثابت defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = جديد AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, خالي_اشارو};
                m_activep->جوڙيو_Stmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_وڻ_خالص(خطأ), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_وڻ(), lhsp);
        }

        // Update Ast References
        لكل (Dfg_راس_Ast& vtx : dfg.astVertices()) {
            إذا (DfgAstRd* ثابت rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_ڳنڍ_اظهار* ثابت exprp = convert_Dfg_راس_منزل_Ast_ڳنڍ_اظهار(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                إذا (exprp->same_وڻ(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_وڻ(), exprp);
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
                rVtxp->exprp()->delete_وڻ();
                rVtxp->exprp(exprp);
            }
        }
    }

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, V3_Dfg_Dfg_منزل_Ast_Context& ctx) { Dfg_منزل_Ast_Visitor{dfg, ctx}; }
};

فراغ V3DfgPasses::dfg_منزل_Ast(Dfg_گراف& dfg, V3DfgContext& ctx) {
    Dfg_منزل_Ast_Visitor::apply(dfg, ctx.m_dfg2AstContext);
}
