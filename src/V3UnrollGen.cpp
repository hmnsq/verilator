// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Generate loop unrolling
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
// Unroll AstLoopStmts
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Const.h"
#ضمن "V3Simulate.h"
#ضمن "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

صنف UnrollGenVisitor final : عام VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = تش_اشاره;  // Iterator variable
    ثابت Ast_غوټه* m_ignoreIncp = تش_اشاره;  // Increment node to ignore
    منطقي m_var_ډېرپېښ_Check = خطأ;  // Just checking RHS assignments
    منطقي m_var_ټاکل_موندنه = خطأ;  // Assign var hit
    منطقي m_fork_موندنه = خطأ;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    فراغ replaceVarRef(Ast_غوټه* bodyp, Ast_غوټه* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreach_And_بل([هذا, varValuep](AstVarRef* refp) {
            إذا (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_غوټه* ثابت newconstp = varValuep->clone_ونه(خطأ);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ټېل_Deletep(refp), refp);
            }
        });
    }

    منطقي cantUnroll(Ast_غوټه* nodep, ثابت حرف* reason) ثابت {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        أرجع خطأ;
    }

    منطقي forUnrollCheck(
        AstGenFor* ثابت nodep,
        Ast_غوټه* ثابت initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_غوټه_بیان* condp,
        Ast_غوټه* ثابت incp,  // Maybe under nodep or in bodysp
        Ast_غوټه* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        إذا (initp) UINFO(6, "    Init " << initp);
        إذا (condp) UINFO(6, "    Cond " << condp);
        إذا (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        Ast_ټاکل* ثابت initAssp = VN_CAST(initp, ټاکل);
        إذا (!initAssp) أرجع cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        إذا (!VN_IS(initAssp->lhsp(), VarRef)) {
            أرجع cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ثابت Ast_ټاکل* ثابت incAssp = VN_CAST(incp, ټاکل);
        إذا (!incAssp) أرجع cantUnroll(nodep, "no increment assignment");
        إذا (incAssp->nextp()) أرجع cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        إذا (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        ثابت AstConst* ثابت constInitp = VN_CAST(initAssp->rhsp(), Const);
        إذا (!constInitp) أرجع cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_var_ډېرپېښ_Check = ریښتیا;
        m_var_ټاکل_موندنه = خطأ;
        m_fork_موندنه = خطأ;
        m_ignoreIncp = incp;
        iterate_And_بل_Null(bodysp);
        iterate_And_بل_Null(incp);
        m_var_ډېرپېښ_Check = خطأ;
        m_ignoreIncp = تش_اشاره;
        إذا (m_var_ټاکل_موندنه) أرجع cantUnroll(nodep, "genvar assigned *inside* loop");

        إذا (m_fork_موندنه) أرجع cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        إذا (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            أرجع cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        أرجع ریښتیا;
    }

    منطقي simulate_ونه(Ast_غوټه_بیان* nodep, ثابت V3_شمېره* loop_ارزښت, ثابت Ast_غوټه* dtypep,
                      V3_شمېره& بهر_عدد) {
        Ast_غوټه* clonep = nodep->clone_ونه(ریښتیا);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        إذا (loop_ارزښت) {
            AstConst* varValuep = جديد AstConst{nodep->fileline(), *loop_ارزښت};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", clonep, خطأ};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlink_Fr_شا_With_بل();
            VL_DO_CLEAR(tempp->delete_ونه(), tempp = تش_اشاره);
            VL_DO_DANGLING(ټېل_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        إذا (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_ونه(), clonep);
            أرجع خطأ;
        }
        // Fetch the result
        ثابت V3_شمېره* resp = simvis.fetch_شمېره_Null(clonep);
        إذا (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_ونه(), clonep);
            أرجع خطأ;
        }
        // Patch up datatype
        إذا (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            بهر_عدد = new_con.عدد();
            بهر_عدد.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_ونه(), clonep);
            أرجع ریښتیا;
        }
        بهر_عدد = *resp;
        VL_DO_DANGLING(clonep->delete_ونه(), clonep);
        أرجع ریښتیا;
    }

    منطقي forUnroller(AstGenFor* nodep, Ast_ټاکل* initp, Ast_غوټه_بیان* condp, Ast_غوټه* incp,
                     Ast_غوټه* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_شمېره loop_ارزښت{nodep};
        إذا (!simulate_ونه(initp->rhsp(), تش_اشاره, initp, loop_ارزښت)) {  //
            أرجع خطأ;
        }
        Ast_غوټه* stmtsp = تش_اشاره;
        إذا (initp) {
            initp->unlink_Fr_شا();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ټېل_Deletep(initp);  // Always cloned below.
        }
        إذا (bodysp) {
            bodysp->unlink_Fr_شا_With_بل();
            stmtsp = Ast_غوټه::جمع_کړه_بل(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->کارول_شوی_Loop_Idx(ریښتیا);

        Ast_غوټه* newbodysp = تش_اشاره;
        إذا (stmtsp) {
            ټېل_Deletep(stmtsp);  // Always cloned below.
            اندازه_t iter_شمېر = 0;
            ثابت اندازه_t iter_حد = v3Global.opt.unroll_حد();
            طالما (ریښتیا) {
                UINFO(8, "      Looping " << loop_ارزښت);
                V3_شمېره res{nodep};
                إذا (!simulate_ونه(condp, &loop_ارزښت, تش_اشاره, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    أرجع خطأ;
                }
                إذا (!res.isEqOne()) {
                    اكسر;  // Done with the loop
                } وإلا {
                    إذا (++iter_شمېر > iter_حد) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iter_حد);
                        اكسر;
                    }

                    // Replace iterator values with constant
                    Ast_غوټه* oneloopp = stmtsp->clone_ونه(ریښتیا);
                    AstConst* varValuep = جديد AstConst{nodep->fileline(), loop_ارزښت};
                    إذا (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ثابت tempp
                            = جديد AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, خطأ};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlink_Fr_شا_With_بل();
                        VL_DO_DANGLING(tempp->delete_ونه(), tempp);
                    }
                    ثابت string شاخص = Ast_غوټه::encode_شمېره(varValuep->منزل_SInt());
                    ثابت string nname = m_beginName + "__BRA__" + شاخص + "__KET__";
                    oneloopp = جديد Ast_Gen_بلاک{oneloopp->fileline(), nname, oneloopp, خطأ};
                    VL_DO_DANGLING(ټېل_Deletep(varValuep), varValuep);
                    newbodysp = Ast_غوټه::جمع_کړه_بل_Null(newbodysp, oneloopp);

                    // loopValue += valInc
                    ثابت Ast_ټاکل* ثابت incpass = VN_AS(incp, ټاکل);
                    V3_شمېره new_Loop_ارزښت{nodep};
                    إذا (!simulate_ونه(incpass->rhsp(), &loop_ارزښت, incpass, new_Loop_ارزښت)) {
                        nodep->v3error("Loop unrolling failed");
                        أرجع خطأ;
                    }
                    loop_ارزښت.op_ټاکل(new_Loop_ارزښت);
                }
            }
        }
        إذا (!newbodysp) {  // initp might have effects after the loop
            إذا (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = جديد AstInitial{initp->fileline(), initp->clone_ونه(ریښتیا)};
            } وإلا {
                newbodysp = initp ? initp->clone_ونه(ریښتیا) : تش_اشاره;
            }
        }
        // Replace the FOR()
        إذا (newbodysp) {
            nodep->replaceWith(newbodysp);
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        إذا (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        أرجع ریښتیا;
    }

    // VISITORS
    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (m_var_ډېرپېښ_Check && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_var_ټاکل_موندنه = ریښتیا;
        }
    }

    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (m_var_ډېرپېښ_Check && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } وإلا {
            iterateChildren(nodep);
        }
    }

عام:
    // CONSTRUCTORS
    UnrollGenVisitor() = افتراضي;

    فراغ unroll(AstGenFor* nodep, ثابت std::string& beginName) {
        m_forVarp = تش_اشاره;  // Iterator variable
        m_ignoreIncp = تش_اشاره;  // Increment node to ignore
        m_var_ډېرپېښ_Check = خطأ;  // Just checking RHS assignments
        m_var_ټاکل_موندنه = خطأ;  // Assign var hit
        m_fork_موندنه = خطأ;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        إذا (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        إذا (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        إذا (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        إذا (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        } وإلا إذا (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);  // Did replacement
        } وإلا {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{جديد UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(ړنګول m_unrollerp, m_unrollerp); }
فراغ GenForUnroller::unroll(AstGenFor* nodep, ثابت std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
