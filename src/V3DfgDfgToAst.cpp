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
سانچا <قسم_ناں T_گرہ, قسم_ناں T_راس, قسم_ناں... Ops>
T_گرہ* make_گرہ(ثابت T_راس* vtxp, Ops... ops) {
    T_گرہ* ثابت nodep = جديد T_گرہ{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->چوڑائی() == static_cast<صحيح>(vtxp->چوڑائی()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->چوڑائی() << " vs " << vtxp->چوڑائی());
    أرجع nodep;
}

//======================================================================
// Vertices needing special conversion

سانچا <>
Ast_گنتی_Ones* make_گرہ<Ast_گنتی_Ones, Dfg_گنتی_Ones, Ast_گرہ_اظہاریہ*>(  //
    ثابت Dfg_گنتی_Ones* vtxp, Ast_گرہ_اظہاریہ* op1) {
    Ast_گنتی_Ones* ثابت nodep = جديد Ast_گنتی_Ones{vtxp->fileline(), op1};
    أرجع nodep;
}

سانچا <>
AstExtend* make_گرہ<AstExtend, DfgExtend, Ast_گرہ_اظہاریہ*>(  //
    ثابت DfgExtend* vtxp, Ast_گرہ_اظہاریہ* op1) {
    أرجع جديد AstExtend{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->چوڑائی())};
}

سانچا <>
AstExtendS* make_گرہ<AstExtendS, DfgExtendS, Ast_گرہ_اظہاریہ*>(  //
    ثابت DfgExtendS* vtxp, Ast_گرہ_اظہاریہ* op1) {
    أرجع جديد AstExtendS{vtxp->fileline(), op1, static_cast<صحيح>(vtxp->چوڑائی())};
}

سانچا <>
Ast_کھسکاؤ_L* make_گرہ<Ast_کھسکاؤ_L, Dfg_کھسکاؤ_L, Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*>(  //
    ثابت Dfg_کھسکاؤ_L* vtxp, Ast_گرہ_اظہاریہ* op1, Ast_گرہ_اظہاریہ* op2) {
    أرجع جديد Ast_کھسکاؤ_L{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->چوڑائی())};
}

سانچا <>
Ast_کھسکاؤ_R* make_گرہ<Ast_کھسکاؤ_R, Dfg_کھسکاؤ_R, Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*>(  //
    ثابت Dfg_کھسکاؤ_R* vtxp, Ast_گرہ_اظہاریہ* op1, Ast_گرہ_اظہاریہ* op2) {
    أرجع جديد Ast_کھسکاؤ_R{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->چوڑائی())};
}

سانچا <>
Ast_کھسکاؤ_RS* make_گرہ<Ast_کھسکاؤ_RS, Dfg_کھسکاؤ_RS, Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*>(  //
    ثابت Dfg_کھسکاؤ_RS* vtxp, Ast_گرہ_اظہاریہ* op1, Ast_گرہ_اظہاریہ* op2) {
    أرجع جديد Ast_کھسکاؤ_RS{vtxp->fileline(), op1, op2, static_cast<صحيح>(vtxp->چوڑائی())};
}

}  // namespace

صنف Dfg_منزل_Ast_Visitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ثابت VNUser2_وچ_Use m_user2_وچ_Use;

    // STATE
    V3_Dfg_Dfg_منزل_Ast_Context& m_ctx;  // The context for stats
    Ast_گرہ_اظہاریہ* m_resultp = خالی_سوچک;  // The result node of the current traversal
    AstAlways* m_alwaysp = خالی_سوچک;  // Process to add assignments to, if have a default driver
    Ast_سرگرم* m_activep = خالی_سوچک;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ساكن Ast_سرگرم* get_Comb_سرگرم(AstScope* scopep) {
        إذا (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            لكل (Ast_گرہ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_سرگرم* ثابت activep = VN_CAST(nodep, سرگرم);
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
                    = جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, Ast_Sen_شے::Combo{}}};
                Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "", senTreep};
                activep->sen_درخت_Storep(senTreep);
                scopep->جوڑو_Blocksp(activep);
                scopep->user2p(activep);
            }
        }
        أرجع VN_AS(scopep->user2p(), سرگرم);
    }

    Ast_گرہ_اظہاریہ* convert_Dfg_راس_منزل_Ast_گرہ_اظہاریہ(Dfg_راس* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<Dfg_راس_Var>() || vtxp->is<DfgConst>()  //
                        || !vtxp->has_مضرب_Sinks() || vtxp->is_Cheaper_Than_لدو(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_گرہ_اظہاریہ* ثابت resultp = m_resultp;
        m_resultp = خالی_سوچک;
        أرجع resultp;
    }

    فراغ createAssignment(File_سطر* flp, Ast_گرہ_اظہاریہ* lhsp, Dfg_راس* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_نتیجہ_Equations;
        // Render the driver
        Ast_گرہ_اظہاریہ* ثابت rhsp = convert_Dfg_راس_منزل_Ast_گرہ_اظہاریہ(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_گرہ* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        إذا (m_alwaysp) {
            m_alwaysp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, lhsp, rhsp});
            أرجع;
        }

        // Otherwise create an AssignW
        Ast_سونپو_W* ثابت ap = جديد Ast_سونپو_W{flp, lhsp, rhsp};
        m_activep->جوڑو_Stmtsp(جديد AstAlways{ap});
    }

    فراغ convertDriver(File_سطر* flp, Ast_گرہ_اظہاریہ* lhsp, Dfg_راس* driverp) {
        إذا (DfgSplicePacked* ثابت sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سطر* dflp) {
                // Create Sel
                AstConst* ثابت lsbp = جديد AstConst{dflp, lo};
                ثابت صحيح چوڑائی = static_cast<صحيح>(src.چوڑائی());
                AstSel* ثابت nLhsp = جديد AstSel{dflp, lhsp->clone_درخت_خالص(خطأ), lsbp, چوڑائی};
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
            sArrayp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سطر* dflp) {
                UASSERT_OBJ(src.ناپ() == 1, &src, "We only handle single elements");
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
    فراغ ویکھو(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ ویکھو(DfgVarPacked* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ ویکھو(DfgVarArray* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ ویکھو(DfgPrev* vtxp) override {
        m_resultp = جديد AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    فراغ ویکھو(DfgConst* vtxp) override {  //
        m_resultp = جديد AstConst{vtxp->fileline(), vtxp->عدد()};
    }
    فراغ ویکھو(DfgCReset* vtxp) override {
        Dfg_راس* ثابت sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<Dfg_راس_Var>(), sinkp, "CReset should drive a variable");
        AstVar* ثابت varp = sinkp->as<Dfg_راس_Var>()->vscp()->varp();
        m_resultp = جديد AstCReset{vtxp->fileline(), varp, خطأ};
    }
    فراغ ویکھو(Dfg_میل_Masked* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = convert_Dfg_راس_منزل_Ast_گرہ_اظہاریہ(vtxp->lhsp());
        AstVarScope* ثابت matchp = vtxp->matchp()->as<Dfg_راس_Var>()->vscp();
        m_resultp = جديد Ast_میل_Masked{flp, lhsp, جديد AstVarRef{flp, matchp, VAccess::READ}};
    }

    فراغ ویکھو(DfgRep* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گرہ_اظہاریہ* ثابت srcp = convert_Dfg_راس_منزل_Ast_گرہ_اظہاریہ(vtxp->srcp());
        m_resultp = جديد AstReplicate{flp, srcp, vtxp->گنتی()};
    }

    فراغ ویکھو(DfgSel* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گرہ_اظہاریہ* ثابت fromp = convert_Dfg_راس_منزل_Ast_گرہ_اظہاریہ(vtxp->fromp());
        AstConst* ثابت lsbp = جديد AstConst{flp, vtxp->lsb()};
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->چوڑائی())};
    }

    فراغ ویکھو(DfgMux* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گرہ_اظہاریہ* ثابت fromp = convert_Dfg_راس_منزل_Ast_گرہ_اظہاریہ(vtxp->fromp());
        Ast_گرہ_اظہاریہ* ثابت lsbp = convert_Dfg_راس_منزل_Ast_گرہ_اظہاریہ(vtxp->lsbp());
        m_resultp = جديد AstSel{flp, fromp, lsbp, static_cast<صحيح>(vtxp->چوڑائی())};
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
            File_سطر* ثابت flp = vtx.driver_File_سطر() ? vtx.driver_File_سطر() : vtx.fileline();
            AstVarRef* ثابت lhsp = جديد AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = get_Comb_سرگرم(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            إذا (Dfg_راس* ثابت defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = جديد AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, خالی_سوچک};
                m_activep->جوڑو_Stmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_درخت_خالص(خطأ), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_درخت(), lhsp);
        }

        // Update Ast References
        لكل (Dfg_راس_Ast& vtx : dfg.astVertices()) {
            إذا (DfgAstRd* ثابت rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_گرہ_اظہاریہ* ثابت exprp = convert_Dfg_راس_منزل_Ast_گرہ_اظہاریہ(rVtxp->srcp());
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
    ساكن فراغ apply(Dfg_گراف& dfg, V3_Dfg_Dfg_منزل_Ast_Context& ctx) { Dfg_منزل_Ast_Visitor{dfg, ctx}; }
};

فراغ V3DfgPasses::dfg_منزل_Ast(Dfg_گراف& dfg, V3DfgContext& ctx) {
    Dfg_منزل_Ast_Visitor::apply(dfg, ctx.m_dfg2AstContext);
}
