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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Const.h"
#شامل "V3Simulate.h"
#شامل "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

جماعت UnrollGenVisitor final : عوامی VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = خالی_اشاریہ;  // Iterator variable
    ثابت Ast_گرہ* m_ignoreIncp = خالی_اشاریہ;  // Increment node to ignore
    منطقی m_var_منوال_Check = جھوٹ;  // Just checking RHS assignments
    منطقی m_var_تفویض_کامیابی = جھوٹ;  // Assign var hit
    منطقی m_fork_کامیابی = جھوٹ;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    خالی replaceVarRef(Ast_گرہ* bodyp, Ast_گرہ* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreach_And_اگلا([یہ, varValuep](AstVarRef* refp) {
            اگر (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_گرہ* ثابت newconstp = varValuep->clone_درخت(جھوٹ);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
            }
        });
    }

    منطقی cantUnroll(Ast_گرہ* nodep, ثابت حرف* reason) ثابت {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        واپس جھوٹ;
    }

    منطقی forUnrollCheck(
        AstGenFor* ثابت nodep,
        Ast_گرہ* ثابت initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_گرہ_اظہاریہ* condp,
        Ast_گرہ* ثابت incp,  // Maybe under nodep or in bodysp
        Ast_گرہ* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        اگر (initp) UINFO(6, "    Init " << initp);
        اگر (condp) UINFO(6, "    Cond " << condp);
        اگر (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        Ast_تفویض* ثابت initAssp = VN_CAST(initp, تفویض);
        اگر (!initAssp) واپس cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        اگر (!VN_IS(initAssp->lhsp(), VarRef)) {
            واپس cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ثابت Ast_تفویض* ثابت incAssp = VN_CAST(incp, تفویض);
        اگر (!incAssp) واپس cantUnroll(nodep, "no increment assignment");
        اگر (incAssp->nextp()) واپس cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        اگر (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        ثابت AstConst* ثابت constInitp = VN_CAST(initAssp->rhsp(), Const);
        اگر (!constInitp) واپس cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_var_منوال_Check = سچ;
        m_var_تفویض_کامیابی = جھوٹ;
        m_fork_کامیابی = جھوٹ;
        m_ignoreIncp = incp;
        iterate_And_اگلا_Null(bodysp);
        iterate_And_اگلا_Null(incp);
        m_var_منوال_Check = جھوٹ;
        m_ignoreIncp = خالی_اشاریہ;
        اگر (m_var_تفویض_کامیابی) واپس cantUnroll(nodep, "genvar assigned *inside* loop");

        اگر (m_fork_کامیابی) واپس cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        اگر (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            واپس cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        واپس سچ;
    }

    منطقی simulate_درخت(Ast_گرہ_اظہاریہ* nodep, ثابت V3_نمبر* loop_قدر, ثابت Ast_گرہ* dtypep,
                      V3_نمبر& برآمد_عدد) {
        Ast_گرہ* clonep = nodep->clone_درخت(سچ);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        اگر (loop_قدر) {
            AstConst* varValuep = نیا AstConst{nodep->fileline(), *loop_قدر};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = نیا AstBegin{nodep->fileline(), "[EditWrapper]", clonep, جھوٹ};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlink_Fr_عقب_With_اگلا();
            VL_DO_CLEAR(tempp->delete_درخت(), tempp = خالی_اشاریہ);
            VL_DO_DANGLING(ڈالو_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        اگر (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_درخت(), clonep);
            واپس جھوٹ;
        }
        // Fetch the result
        ثابت V3_نمبر* resp = simvis.fetch_نمبر_Null(clonep);
        اگر (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_درخت(), clonep);
            واپس جھوٹ;
        }
        // Patch up datatype
        اگر (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            برآمد_عدد = new_con.عدد();
            برآمد_عدد.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_درخت(), clonep);
            واپس سچ;
        }
        برآمد_عدد = *resp;
        VL_DO_DANGLING(clonep->delete_درخت(), clonep);
        واپس سچ;
    }

    منطقی forUnroller(AstGenFor* nodep, Ast_تفویض* initp, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* incp,
                     Ast_گرہ* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_نمبر loop_قدر{nodep};
        اگر (!simulate_درخت(initp->rhsp(), خالی_اشاریہ, initp, loop_قدر)) {  //
            واپس جھوٹ;
        }
        Ast_گرہ* stmtsp = خالی_اشاریہ;
        اگر (initp) {
            initp->unlink_Fr_عقب();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ڈالو_Deletep(initp);  // Always cloned below.
        }
        اگر (bodysp) {
            bodysp->unlink_Fr_عقب_With_اگلا();
            stmtsp = Ast_گرہ::جوڑو_اگلا(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->مستعمل_Loop_Idx(سچ);

        Ast_گرہ* newbodysp = خالی_اشاریہ;
        اگر (stmtsp) {
            ڈالو_Deletep(stmtsp);  // Always cloned below.
            سائز_t iter_گنتی = 0;
            ثابت سائز_t iter_حد_بندی = v3Global.opt.unroll_حد_بندی();
            جب (سچ) {
                UINFO(8, "      Looping " << loop_قدر);
                V3_نمبر res{nodep};
                اگر (!simulate_درخت(condp, &loop_قدر, خالی_اشاریہ, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    واپس جھوٹ;
                }
                اگر (!res.isEqOne()) {
                    توڑو;  // Done with the loop
                } ورنہ {
                    اگر (++iter_گنتی > iter_حد_بندی) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iter_حد_بندی);
                        توڑو;
                    }

                    // Replace iterator values with constant
                    Ast_گرہ* oneloopp = stmtsp->clone_درخت(سچ);
                    AstConst* varValuep = نیا AstConst{nodep->fileline(), loop_قدر};
                    اگر (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ثابت tempp
                            = نیا AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, جھوٹ};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlink_Fr_عقب_With_اگلا();
                        VL_DO_DANGLING(tempp->delete_درخت(), tempp);
                    }
                    ثابت string اشاریہ = Ast_گرہ::encode_نمبر(varValuep->مقصد_SInt());
                    ثابت string nname = m_beginName + "__BRA__" + اشاریہ + "__KET__";
                    oneloopp = نیا Ast_Gen_بلاک{oneloopp->fileline(), nname, oneloopp, جھوٹ};
                    VL_DO_DANGLING(ڈالو_Deletep(varValuep), varValuep);
                    newbodysp = Ast_گرہ::جوڑو_اگلا_Null(newbodysp, oneloopp);

                    // loopValue += valInc
                    ثابت Ast_تفویض* ثابت incpass = VN_AS(incp, تفویض);
                    V3_نمبر new_Loop_قدر{nodep};
                    اگر (!simulate_درخت(incpass->rhsp(), &loop_قدر, incpass, new_Loop_قدر)) {
                        nodep->v3error("Loop unrolling failed");
                        واپس جھوٹ;
                    }
                    loop_قدر.op_تفویض(new_Loop_قدر);
                }
            }
        }
        اگر (!newbodysp) {  // initp might have effects after the loop
            اگر (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = نیا AstInitial{initp->fileline(), initp->clone_درخت(سچ)};
            } ورنہ {
                newbodysp = initp ? initp->clone_درخت(سچ) : خالی_اشاریہ;
            }
        }
        // Replace the FOR()
        اگر (newbodysp) {
            nodep->replaceWith(newbodysp);
        } ورنہ {
            nodep->unlink_Fr_عقب();
        }
        اگر (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        واپس سچ;
    }

    // VISITORS
    خالی دورہ(AstVarRef* nodep) override {
        اگر (m_var_منوال_Check && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_var_تفویض_کامیابی = سچ;
        }
    }

    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (m_var_منوال_Check && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } ورنہ {
            iterateChildren(nodep);
        }
    }

عوامی:
    // CONSTRUCTORS
    UnrollGenVisitor() = طےشدہ;

    خالی unroll(AstGenFor* nodep, ثابت std::string& beginName) {
        m_forVarp = خالی_اشاریہ;  // Iterator variable
        m_ignoreIncp = خالی_اشاریہ;  // Increment node to ignore
        m_var_منوال_Check = جھوٹ;  // Just checking RHS assignments
        m_var_تفویض_کامیابی = جھوٹ;  // Assign var hit
        m_fork_کامیابی = جھوٹ;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        اگر (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        اگر (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        اگر (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        اگر (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        } ورنہ اگر (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);  // Did replacement
        } ورنہ {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{نیا UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(حذف m_unrollerp, m_unrollerp); }
خالی GenForUnroller::unroll(AstGenFor* nodep, ثابت std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
