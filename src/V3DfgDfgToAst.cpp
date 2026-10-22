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

نامخلا {

// Create an AstNodeExpr out of a DfgVertex. For most AstNodeExpr subtypes, this can be done
// automatically. For the few special cases, we provide specializations below
سانچہ <قسم_نام T_گرہ, قسم_نام T_راس, قسم_نام... Ops>
T_گرہ* make_گرہ(ثابت T_راس* vtxp, Ops... ops) {
    T_گرہ* ثابت nodep = نیا T_گرہ{vtxp->fileline(), ops...};
    UASSERT_OBJ(nodep->چوڑائی() == static_cast<صحیح>(vtxp->چوڑائی()), vtxp,
                "Incorrect width in AstNode created from DfgVertex "
                    << vtxp->typeName() << ": " << nodep->چوڑائی() << " vs " << vtxp->چوڑائی());
    واپس nodep;
}

//======================================================================
// Vertices needing special conversion

سانچہ <>
Ast_گنتی_Ones* make_گرہ<Ast_گنتی_Ones, Dfg_گنتی_Ones, Ast_گرہ_اظہاریہ*>(  //
    ثابت Dfg_گنتی_Ones* vtxp, Ast_گرہ_اظہاریہ* op1) {
    Ast_گنتی_Ones* ثابت nodep = نیا Ast_گنتی_Ones{vtxp->fileline(), op1};
    واپس nodep;
}

سانچہ <>
AstExtend* make_گرہ<AstExtend, DfgExtend, Ast_گرہ_اظہاریہ*>(  //
    ثابت DfgExtend* vtxp, Ast_گرہ_اظہاریہ* op1) {
    واپس نیا AstExtend{vtxp->fileline(), op1, static_cast<صحیح>(vtxp->چوڑائی())};
}

سانچہ <>
AstExtendS* make_گرہ<AstExtendS, DfgExtendS, Ast_گرہ_اظہاریہ*>(  //
    ثابت DfgExtendS* vtxp, Ast_گرہ_اظہاریہ* op1) {
    واپس نیا AstExtendS{vtxp->fileline(), op1, static_cast<صحیح>(vtxp->چوڑائی())};
}

سانچہ <>
Ast_سرکاؤ_L* make_گرہ<Ast_سرکاؤ_L, Dfg_سرکاؤ_L, Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*>(  //
    ثابت Dfg_سرکاؤ_L* vtxp, Ast_گرہ_اظہاریہ* op1, Ast_گرہ_اظہاریہ* op2) {
    واپس نیا Ast_سرکاؤ_L{vtxp->fileline(), op1, op2, static_cast<صحیح>(vtxp->چوڑائی())};
}

سانچہ <>
Ast_سرکاؤ_R* make_گرہ<Ast_سرکاؤ_R, Dfg_سرکاؤ_R, Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*>(  //
    ثابت Dfg_سرکاؤ_R* vtxp, Ast_گرہ_اظہاریہ* op1, Ast_گرہ_اظہاریہ* op2) {
    واپس نیا Ast_سرکاؤ_R{vtxp->fileline(), op1, op2, static_cast<صحیح>(vtxp->چوڑائی())};
}

سانچہ <>
Ast_سرکاؤ_RS* make_گرہ<Ast_سرکاؤ_RS, Dfg_سرکاؤ_RS, Ast_گرہ_اظہاریہ*, Ast_گرہ_اظہاریہ*>(  //
    ثابت Dfg_سرکاؤ_RS* vtxp, Ast_گرہ_اظہاریہ* op1, Ast_گرہ_اظہاریہ* op2) {
    واپس نیا Ast_سرکاؤ_RS{vtxp->fileline(), op1, op2, static_cast<صحیح>(vtxp->چوڑائی())};
}

}  // namespace

جماعت Dfg_مقصد_Ast_Visitor final : DfgVisitor {
    // NODE STATE

    // AstScope::user2p  // The combinational AstActive under this scope
    ثابت VNUser2_اندر_Use m_user2_اندر_Use;

    // STATE
    V3_Dfg_Dfg_مقصد_Ast_Context& m_ctx;  // The context for stats
    Ast_گرہ_اظہاریہ* m_resultp = خالی_اشاریہ;  // The result node of the current traversal
    AstAlways* m_alwaysp = خالی_اشاریہ;  // Process to add assignments to, if have a default driver
    Ast_فعال* m_activep = خالی_اشاریہ;  // The AstNodeModule or AstActive to insert assigns into

    // METHODS
    ساکن Ast_فعال* get_Comb_فعال(AstScope* scopep) {
        اگر (!scopep->user2p()) {
            // Try to find the existing combinational AstActive
            برائے (Ast_گرہ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_فعال* ثابت activep = VN_CAST(nodep, فعال);
                اگر (!activep) جاری;
                اگر (activep->hasCombo()) {
                    scopep->user2p(activep);
                    توڑو;
                }
            }
            // If there isn't one, create a new one
            اگر (!scopep->user2p()) {
                File_سطر* ثابت flp = scopep->fileline();
                Ast_Sen_درخت* ثابت senTreep
                    = نیا Ast_Sen_درخت{flp, نیا Ast_Sen_شے{flp, Ast_Sen_شے::Combo{}}};
                Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "", senTreep};
                activep->sen_درخت_Storep(senTreep);
                scopep->جوڑو_Blocksp(activep);
                scopep->user2p(activep);
            }
        }
        واپس VN_AS(scopep->user2p(), فعال);
    }

    Ast_گرہ_اظہاریہ* convert_Dfg_راس_مقصد_Ast_گرہ_اظہاریہ(Dfg_راس* vtxp) {
        UASSERT_OBJ(!m_resultp, vtxp, "Result already computed");
        UASSERT_OBJ(vtxp->is<Dfg_راس_Var>() || vtxp->is<DfgConst>()  //
                        || !vtxp->has_مضرب_Sinks() || vtxp->is_Cheaper_Than_بھرو(),  //
                    vtxp, "Intermediate DFG value with multiple uses");
        iterate(vtxp);
        UASSERT_OBJ(m_resultp, vtxp, "Missing result");
        Ast_گرہ_اظہاریہ* ثابت resultp = m_resultp;
        m_resultp = خالی_اشاریہ;
        واپس resultp;
    }

    خالی createAssignment(File_سطر* flp, Ast_گرہ_اظہاریہ* lhsp, Dfg_راس* driverp) {
        // Keep track of statisticss
        ++m_ctx.m_نتیجہ_Equations;
        // Render the driver
        Ast_گرہ_اظہاریہ* ثابت rhsp = convert_Dfg_راس_مقصد_Ast_گرہ_اظہاریہ(driverp);
        // Update LHS locations to reflect the location of the original driver
        lhsp->foreach([&](Ast_گرہ* nodep) { nodep->fileline(flp); });

        // If using a process, add Assign there
        اگر (m_alwaysp) {
            m_alwaysp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, lhsp, rhsp});
            واپس;
        }

        // Otherwise create an AssignW
        Ast_تفویض_W* ثابت ap = نیا Ast_تفویض_W{flp, lhsp, rhsp};
        m_activep->جوڑو_Stmtsp(نیا AstAlways{ap});
    }

    خالی convertDriver(File_سطر* flp, Ast_گرہ_اظہاریہ* lhsp, Dfg_راس* driverp) {
        اگر (DfgSplicePacked* ثابت sPackedp = driverp->cast<DfgSplicePacked>()) {
            // Partial assignment of packed value
            sPackedp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سطر* dflp) {
                // Create Sel
                AstConst* ثابت lsbp = نیا AstConst{dflp, lo};
                ثابت صحیح چوڑائی = static_cast<صحیح>(src.چوڑائی());
                AstSel* ثابت nLhsp = نیا AstSel{dflp, lhsp->clone_درخت_خالص(جھوٹ), lsbp, چوڑائی};
                // Convert source
                convertDriver(dflp, nLhsp, &src);
                // Delete Sel - was cloned
                VL_DO_DANGLING(nLhsp->delete_درخت(), nLhsp);
                واپس جھوٹ;
            });
            واپس;
        }

        اگر (DfgSpliceArray* ثابت sArrayp = driverp->cast<DfgSpliceArray>()) {
            // Partial assignment of array variable
            sArrayp->foreachDriver([&](Dfg_راس& src, uint32_t lo, File_سطر* dflp) {
                UASSERT_OBJ(src.سائز() == 1, &src, "We only handle single elements");
                // Create ArraySel
                AstConst* ثابت idxp = نیا AstConst{dflp, lo};
                AstArraySel* ثابت nLhsp = نیا AstArraySel{dflp, lhsp->clone_درخت_خالص(جھوٹ), idxp};
                // Convert source
                اگر (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    convertDriver(dflp, nLhsp, uap->srcp());
                } ورنہ {
                    convertDriver(dflp, nLhsp, &src);
                }
                // Delete ArraySel - was cloned
                VL_DO_DANGLING(nLhsp->delete_درخت(), nLhsp);
                واپس جھوٹ;
            });
            واپس;
        }

        اگر (ثابت DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>()) {
            // Single element array being assigned a unit array. Needs an ArraySel.
            AstConst* ثابت idxp = نیا AstConst{flp, 0};
            AstArraySel* ثابت nLhsp = نیا AstArraySel{flp, lhsp->clone_درخت_خالص(جھوٹ), idxp};
            // Convert source
            convertDriver(flp, nLhsp, uap->srcp());
            // Delete ArraySel - was cloned
            VL_DO_DANGLING(nLhsp->delete_درخت(), nLhsp);
            واپس;
        }

        // Base case: assign vertex to current lhs
        createAssignment(flp, lhsp->clone_درخت_خالص(جھوٹ), driverp);
    }

    // VISITORS
    خالی دورہ(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("Unhandled DfgVertex: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    خالی دورہ(DfgVarPacked* vtxp) override {
        m_resultp = نیا AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    خالی دورہ(DfgVarArray* vtxp) override {
        m_resultp = نیا AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    خالی دورہ(Dfg_پچھلا* vtxp) override {
        m_resultp = نیا AstVarRef{vtxp->fileline(), vtxp->vscp(), VAccess::READ};
    }

    خالی دورہ(DfgConst* vtxp) override {  //
        m_resultp = نیا AstConst{vtxp->fileline(), vtxp->عدد()};
    }
    خالی دورہ(DfgCReset* vtxp) override {
        Dfg_راس* ثابت sinkp = vtxp->singleSink();
        UASSERT_OBJ(sinkp, vtxp, "CReset should only have one sink");
        UASSERT_OBJ(sinkp->is<Dfg_راس_Var>(), sinkp, "CReset should drive a variable");
        AstVar* ثابت varp = sinkp->as<Dfg_راس_Var>()->vscp()->varp();
        m_resultp = نیا AstCReset{vtxp->fileline(), varp, جھوٹ};
    }
    خالی دورہ(Dfg_مطابقت_Masked* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = convert_Dfg_راس_مقصد_Ast_گرہ_اظہاریہ(vtxp->lhsp());
        AstVarScope* ثابت matchp = vtxp->matchp()->as<Dfg_راس_Var>()->vscp();
        m_resultp = نیا Ast_مطابقت_Masked{flp, lhsp, نیا AstVarRef{flp, matchp, VAccess::READ}};
    }

    خالی دورہ(DfgRep* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گرہ_اظہاریہ* ثابت srcp = convert_Dfg_راس_مقصد_Ast_گرہ_اظہاریہ(vtxp->srcp());
        m_resultp = نیا AstReplicate{flp, srcp, vtxp->گنتی()};
    }

    خالی دورہ(DfgSel* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گرہ_اظہاریہ* ثابت fromp = convert_Dfg_راس_مقصد_Ast_گرہ_اظہاریہ(vtxp->fromp());
        AstConst* ثابت lsbp = نیا AstConst{flp, vtxp->lsb()};
        m_resultp = نیا AstSel{flp, fromp, lsbp, static_cast<صحیح>(vtxp->چوڑائی())};
    }

    خالی دورہ(DfgMux* vtxp) override {
        File_سطر* ثابت flp = vtxp->fileline();
        Ast_گرہ_اظہاریہ* ثابت fromp = convert_Dfg_راس_مقصد_Ast_گرہ_اظہاریہ(vtxp->fromp());
        Ast_گرہ_اظہاریہ* ثابت lsbp = convert_Dfg_راس_مقصد_Ast_گرہ_اظہاریہ(vtxp->lsbp());
        m_resultp = نیا AstSel{flp, fromp, lsbp, static_cast<صحیح>(vtxp->چوڑائی())};
    }

    // The rest of the 'visit' methods are generated by 'astgen'
#شامل "V3Dfg__gen_dfg_to_ast.h"

    // Constructor
    Dfg_مقصد_Ast_Visitor(Dfg_گراف& dfg, V3_Dfg_Dfg_مقصد_Ast_Context& ctx)
        : m_ctx{ctx} {
        اگر (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);

        // Convert the graph back to combinational assignments. The graph must have been
        // regularized, so we only need to render variable assignments and update Ast references.

        // Render variable assignments
        برائے (Dfg_راس_Var& vtx : dfg.varVertices()) {
            // If there is no driver (this vertex is an input to the graph), then nothing to do.
            اگر (!vtx.srcp()) {
                UASSERT_OBJ(!vtx.defaultp(), &vtx, "Only default driver on variable");
                جاری;
            }

            ++m_ctx.m_outputVariables;

            // Render variable assignments
            File_سطر* ثابت flp = vtx.driver_File_سطر() ? vtx.driver_File_سطر() : vtx.fileline();
            AstVarRef* ثابت lhsp = نیا AstVarRef{flp, vtx.vscp(), VAccess::WRITE};

            // Add it to the scope holding the target variable
            VL_RESTORER(m_activep);
            m_activep = get_Comb_فعال(vtx.vscp()->scopep());

            // If there is a default value, render all drivers under an AstAlways
            VL_RESTORER(m_alwaysp);
            اگر (Dfg_راس* ثابت defaultp = vtx.defaultp()) {
                ++m_ctx.m_outputVariablesWithDefault;
                m_alwaysp = نیا AstAlways{vtx.fileline(), VAlwaysKwd::ALWAYS_COMB, خالی_اشاریہ};
                m_activep->جوڑو_Stmtsp(m_alwaysp);
                // The default assignment needs to go first
                createAssignment(vtx.fileline(), lhsp->clone_درخت_خالص(جھوٹ), defaultp);
            }

            // Render the drivers
            convertDriver(flp, lhsp, vtx.srcp());

            // convetDriver always clones lhsp
            VL_DO_DANGLING(lhsp->delete_درخت(), lhsp);
        }

        // Update Ast References
        برائے (Dfg_راس_Ast& vtx : dfg.astVertices()) {
            اگر (DfgAstRd* ثابت rVtxp = vtx.cast<DfgAstRd>()) {
                // Render the driver
                Ast_گرہ_اظہاریہ* ثابت exprp = convert_Dfg_راس_مقصد_Ast_گرہ_اظہاریہ(rVtxp->srcp());
                // If it's the same as the reference, do not replace it so FileLines are preserved
                اگر (exprp->same_درخت(rVtxp->exprp())) {
                    VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                    جاری;
                }
                // Replace the reference with the expression
                اگر (VN_IS(exprp, VarRef)) {
                    exprp->fileline(rVtxp->exprp()->fileline());
                    ++m_ctx.m_varRefsSubstituted;
                } ورنہ {
                    ++m_ctx.m_expressionsInlined;
                }
                rVtxp->exprp()->replaceWith(exprp);
                rVtxp->exprp()->delete_درخت();
                rVtxp->exprp(exprp);
            }
        }
    }

عوامی:
    ساکن خالی apply(Dfg_گراف& dfg, V3_Dfg_Dfg_مقصد_Ast_Context& ctx) { Dfg_مقصد_Ast_Visitor{dfg, ctx}; }
};

خالی V3DfgPasses::dfg_مقصد_Ast(Dfg_گراف& dfg, V3DfgContext& ctx) {
    Dfg_مقصد_Ast_Visitor::apply(dfg, ctx.m_dfg2AstContext);
}
