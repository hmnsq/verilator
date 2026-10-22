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
قالب <نام_نوع T_گره, نام_نوع T_رأس, نام_نوع... Ops>
T_گره* make_گره(ثابت T_رأس* vtxp, Ops... ops) {
    T_گره* ثابت nodep = جديد T_گره{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->عرض() == static_cast<صحيح>(vtxp->عرض()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->عرض() << " vs " << vtxp->عرض());
    أرجع nodep;
}

//======================================================================
// Vertices needing special conversion

قالب <>
Ast_شمار_Ones* make_گره<Ast_شمار_Ones, Dfg_شمار_Ones, Ast_گره_عبارت*>(  //
    ثابت Dfg_شمار_Ones* vtxp, Ast_گره_عبارت* op1) {
    Ast_شمار_Ones* ثابت nodep = جديد Ast_شمار_Ones{vtxp->fileline(), op1};
    أرجع nodep;
}

قالب <>
AstExtend* make_گره<AstExtend, DfgExtend, Ast_گره_عبارت*>(  //
    ثابت DfgExtend* vtxp, Ast_گره_عبارت* op1) {
    أرجع جديد AstExtend{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->عرض())};
}

قالب <>
AstExtendS* make_گره<AstExtendS, DfgExtendS, Ast_گره_عبارت*>(  //
    ثابت DfgExtendS* vtxp, Ast_گره_عبارت* op1) {
    أرجع جديد AstExtendS{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->عرض())};
}

قالب <>
Ast_لغزش_L* make_گره<Ast_لغزش_L, Dfg_لغزش_L, Ast_گره_عبارت*, Ast_گره_عبارت*>(  //
    ثابت Dfg_لغزش_L* vtxp, Ast_گره_عبارت* op1, Ast_گره_عبارت* op2) {
    أرجع جديد Ast_لغزش_L{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->عرض())};
}

قالب <>
Ast_لغزش_R* make_گره<Ast_لغزش_R, Dfg_لغزش_R, Ast_گره_عبارت*, Ast_گره_عبارت*>(  //
    ثابت Dfg_لغزش_R* vtxp, Ast_گره_عبارت* op1, Ast_گره_عبارت* op2) {
    أرجع جديد Ast_لغزش_R{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->عرض())};
}

قالب <>
Ast_لغزش_RS* make_گره<Ast_لغزش_RS, Dfg_لغزش_RS, Ast_گره_عبارت*, Ast_گره_عبارت*>(  //
    ثابت Dfg_لغزش_RS* vtxp, Ast_گره_عبارت* op1, Ast_گره_عبارت* op2) {
    أرجع جديد Ast_لغزش_RS{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->عرض())};
}

}  // namespace

صنف Dfg_مقصد_Ast_Visitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ثابت VNUser2_درون_Use m_user2_درون_Use;

    // STATE
    V3_Dfg_Dfg_مقصد_Ast_Context& m_ctx;  // The context for stats
    Ast_گره_عبارت* m_resultp = اشاره_تهی;  // The result node of the current traversal
    AstAlways* m_alwaysp = اشاره_تهی;  // Process to add assignments to, if have a default driver
    Ast_فعال* m_activep = اشاره_تهی;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ساكن Ast_فعال* get_Comb_فعال(AstScope* scopep) {
        إذا (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            لكل (Ast_گره* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_فعال* ثابت activep = VN_CAST(nodep, فعال);
                إذا (!activep) استمر;
                إذا (activep->hasCombo()) {
                    scopep->user2p(activep);
                    اكسر;
                }
            }
            // If there isn't one, create a new one
            إذا (!scopep->user2p()) {
                File_سطر* ثابت flp = scopep->fileline();
                Ast_Sen_درخت* ثابت senTreep
                    = جديد Ast_Sen_درخت{flp, جديد Ast_Sen_قلم{flp, Ast_Sen_قلم::Combo{}}};
                Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "", senTreep};
                activep->sen_درخت_Storep(senTreep);
                scopep->جمع_کن_Blocksp(activep);
                scopep->user2p(activep);
            }
        }
        أرجع VN_AS(scopep->user2p(), فعال);
    }

    Ast_گره_عبارت* convert_Dfg_رأس_مقصد_Ast_گره_عبارت(Dfg_رأس* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<Dfg_رأس_Var>() || vtxp->is<DfgConst>()  //
                        || !vtxp->has_مضرب_Sinks() || vtxp->is_Cheaper_Than_بارگیری(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_گره_عبارت* ثابت resultp = m_resultp;
        m_resultp = اشاره_تهی;
        أرجع resultp;
    }

    فراغ createAssignment(File_سطر* flp, Ast_گره_عبارت* lhsp, Dfg_رأس* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_نتیجه_Equations;
        // Render the driver
        Ast_گره_عبارت* ثابت rhsp = convert_Dfg_رأس_مقصد_Ast_گره_عبارت(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_گره* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        إذا (m_alwaysp) {
            m_alwaysp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});
            أرجع;
        }

        // Otherwise create an AssignW
        Ast_تخصیص_بده_W* ثابت ap = جديد Ast_تخصیص_بده_W{flp, lhsp, rhsp};
        m_activep->جمع_کن_Stmtsp(جديد AstAlways{ap});
    }

    فراغ convertDriver(File_سطر* flp, Ast_گره_عبارت* lhsp, Dfg_رأس* driverp) {
        إذا (DfgSplicePacked* ثابت sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](Dfg_رأس& src, uint32_t lo, File_سطر* dflp) {
                // Create Sel
                AstConst* ثابت lsbp = جديد AstConst{dflp, lo};
                ثابت صحيح عرض = static_cast<صحيح>(src.عرض());
                AstSel* ثابت nLhsp = جديد AstSel{dflp, lhsp->clone_درخت_خالص(خطأ), lsbp, عرض};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_درخت(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (DfgSpliceArray* ثابت sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](Dfg_رأس& src, uint32_t lo, File_سطر* dflp) {
                UASSERT_OBJ(src.اندازه() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ثابت idxp = جديد AstConst{dflp, lo};
                AstArraySel* ثابت nLhsp = جديد AstArraySel{dflp, lhsp->clone_درخت_خالص(خطأ), idxp};
                // Convert source
                إذا (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } وإلا {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_درخت(), nLhsp);
                أرجع خطأ;
            });
            أرجع;
        }

        إذا (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ثابت idxp = جديد AstConst{flp, 0};
            AstArraySel* ثابت nLhsp = جديد AstArraySel{flp, lhsp->clone_درخت_خالص(خطأ), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_درخت(), nLhsp);
            أرجع;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_درخت_خالص(خطأ), driverp);
    }

    // VISITORS
    فراغ بازدید(Dfg_رأس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ بازدید(DfgVarPacked* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ بازدید(DfgVarArray* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ بازدید(DfgPrev* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ بازدید(DfgConst* vtxp) override {  //
        m_resultp = جديد AstConst{vtxp->fileline(), vtxp->رقم()};
    }
    فراغ بازدید(DfgCReset* vtxp) override {
        Dfg_رأس* ثابت sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<Dfg_رأس_Var>(), sinkp, "CReset should drive a variable");
        AstVar* ثابت varp = sinkp->as<Dfg_رأس_Var>()->vscp()->varp();
        m_resultp = جديد AstCReset{vtxp->fileline(), varp, خطأ};
    }
    فراغ بازدید(Dfg_تطابق_Masked* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گره_عبارت* ثابت lhsp = convert_Dfg_رأس_مقصد_Ast_گره_عبارت(vtxp->lhsp());
        AstVarScope* ثابت matchp = vtxp->matchp()->as<Dfg_رأس_Var>()->vscp();
        m_resultp = جديد Ast_تطابق_Masked{flp, lhsp, جديد AstVarRef{flp, matchp, VAccess::READ}};
    }

    فراغ بازدید(DfgRep* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گره_عبارت* ثابت srcp = convert_Dfg_رأس_مقصد_Ast_گره_عبارت(vtxp->srcp());
        m_resultp = جديد AstReplicate{flp, srcp, vtxp->شمار()};
    }

    فراغ بازدید(DfgSel* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گره_عبارت* ثابت fromp = convert_Dfg_رأس_مقصد_Ast_گره_عبارت(vtxp->fromp());
        AstConst* ثابت lsbp = جديد AstConst{flp, vtxp->lsb()};
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->عرض())};
    }

    فراغ بازدید(DfgMux* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گره_عبارت* ثابت fromp = convert_Dfg_رأس_مقصد_Ast_گره_عبارت(vtxp->fromp());
        Ast_گره_عبارت* ثابت lsbp = convert_Dfg_رأس_مقصد_Ast_گره_عبارت(vtxp->lsbp());
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->عرض())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#ضمن "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    Dfg_مقصد_Ast_Visitor(Dfg_گراف& dfg, V3_Dfg_Dfg_مقصد_Ast_Context& ctx)
        : m_ctx{ctx} {
        إذا (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        لكل (Dfg_رأس_Var& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            إذا (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                استمر;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_سطر* ثابت flp = vtx.driver_File_سطر() ? vtx.driver_File_سطر() : vtx.fileline();
            AstVarRef* ثابت lhsp = جديد AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = get_Comb_فعال(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            إذا (Dfg_رأس* ثابت defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = جديد AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, اشاره_تهی};
                m_activep->جمع_کن_Stmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_درخت_خالص(خطأ), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_درخت(), lhsp);
        }

        // Update Ast References
        لكل (Dfg_رأس_Ast& vtx : dfg.astVertices()) {
            إذا (DfgAstRd* ثابت rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_گره_عبارت* ثابت exprp = convert_Dfg_رأس_مقصد_Ast_گره_عبارت(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                إذا (exprp->same_درخت(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_درخت(), exprp);
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
                rVtxp->exprp()->delete_درخت();
                rVtxp->exprp(exprp);
            }
        }
    }

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, V3_Dfg_Dfg_مقصد_Ast_Context& ctx) { Dfg_مقصد_Ast_Visitor{dfg, ctx}; }
};

فراغ V3DfgPasses::dfg_مقصد_Ast(Dfg_گراف& dfg, V3DfgContext& ctx) {
    Dfg_مقصد_Ast_Visitor::apply(dfg, ctx.m_dfg2AstContext);
}
