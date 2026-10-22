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
    AstVar* m_forVarp = خالي_اشارو;  // Iterator variable
    ثابت Ast_ڳنڍ* m_ignoreIncp = خالي_اشارو;  // Increment node to ignore
    منطقي m_var_گهڻائي_Check = خطأ;  // Just checking RHS assignments
    منطقي m_var_سونپيو_ڪاميابي = خطأ;  // Assign var hit
    منطقي m_fork_ڪاميابي = خطأ;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    فراغ replaceVarRef(Ast_ڳنڍ* bodyp, Ast_ڳنڍ* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreach_And_ايندڙ([هذا, varValuep](AstVarRef* refp) {
            إذا (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_ڳنڍ* ثابت newconstp = varValuep->clone_وڻ(خطأ);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
            }
        });
    }

    منطقي cantUnroll(Ast_ڳنڍ* nodep, ثابت حرف* reason) ثابت {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        أرجع خطأ;
    }

    منطقي forUnrollCheck(
        AstGenFor* ثابت nodep,
        Ast_ڳنڍ* ثابت initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_ڳنڍ_اظهار* condp,
        Ast_ڳنڍ* ثابت incp,  // Maybe under nodep or in bodysp
        Ast_ڳنڍ* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        إذا (initp) UINFO(6, "    Init " << initp);
        إذا (condp) UINFO(6, "    Cond " << condp);
        إذا (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        Ast_سونپيو* ثابت initAssp = VN_CAST(initp, سونپيو);
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
        ثابت Ast_سونپيو* ثابت incAssp = VN_CAST(incp, سونپيو);
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
        m_var_گهڻائي_Check = سچ;
        m_var_سونپيو_ڪاميابي = خطأ;
        m_fork_ڪاميابي = خطأ;
        m_ignoreIncp = incp;
        iterate_And_ايندڙ_Null(bodysp);
        iterate_And_ايندڙ_Null(incp);
        m_var_گهڻائي_Check = خطأ;
        m_ignoreIncp = خالي_اشارو;
        إذا (m_var_سونپيو_ڪاميابي) أرجع cantUnroll(nodep, "genvar assigned *inside* loop");

        إذا (m_fork_ڪاميابي) أرجع cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        إذا (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            أرجع cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        أرجع سچ;
    }

    منطقي simulate_وڻ(Ast_ڳنڍ_اظهار* nodep, ثابت V3_عدد* loop_قدر, ثابت Ast_ڳنڍ* dtypep,
                      V3_عدد& ٻاهريون_انگ) {
        Ast_ڳنڍ* clonep = nodep->clone_وڻ(سچ);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        إذا (loop_قدر) {
            AstConst* varValuep = جديد AstConst{nodep->fileline(), *loop_قدر};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", clonep, خطأ};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ();
            VL_DO_CLEAR(tempp->delete_وڻ(), tempp = خالي_اشارو);
            VL_DO_DANGLING(ڌڪو_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        إذا (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_وڻ(), clonep);
            أرجع خطأ;
        }
        // Fetch the result
        ثابت V3_عدد* resp = simvis.fetch_عدد_Null(clonep);
        إذا (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_وڻ(), clonep);
            أرجع خطأ;
        }
        // Patch up datatype
        إذا (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            ٻاهريون_انگ = new_con.انگ();
            ٻاهريون_انگ.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_وڻ(), clonep);
            أرجع سچ;
        }
        ٻاهريون_انگ = *resp;
        VL_DO_DANGLING(clonep->delete_وڻ(), clonep);
        أرجع سچ;
    }

    منطقي forUnroller(AstGenFor* nodep, Ast_سونپيو* initp, Ast_ڳنڍ_اظهار* condp, Ast_ڳنڍ* incp,
                     Ast_ڳنڍ* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_عدد loop_قدر{nodep};
        إذا (!simulate_وڻ(initp->rhsp(), خالي_اشارو, initp, loop_قدر)) {  //
            أرجع خطأ;
        }
        Ast_ڳنڍ* stmtsp = خالي_اشارو;
        إذا (initp) {
            initp->unlink_Fr_پٺ();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ڌڪو_Deletep(initp);  // Always cloned below.
        }
        إذا (bodysp) {
            bodysp->unlink_Fr_پٺ_With_ايندڙ();
            stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->استعمال_ٿيل_Loop_Idx(سچ);

        Ast_ڳنڍ* newbodysp = خالي_اشارو;
        إذا (stmtsp) {
            ڌڪو_Deletep(stmtsp);  // Always cloned below.
            سائيز_t iter_ڳڻپ = 0;
            ثابت سائيز_t iter_حد_بندي = v3Global.opt.unroll_حد_بندي();
            طالما (سچ) {
                UINFO(8, "      Looping " << loop_قدر);
                V3_عدد res{nodep};
                إذا (!simulate_وڻ(condp, &loop_قدر, خالي_اشارو, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    أرجع خطأ;
                }
                إذا (!res.isEqOne()) {
                    اكسر;  // Done with the loop
                } وإلا {
                    إذا (++iter_ڳڻپ > iter_حد_بندي) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iter_حد_بندي);
                        اكسر;
                    }

                    // Replace iterator values with constant
                    Ast_ڳنڍ* oneloopp = stmtsp->clone_وڻ(سچ);
                    AstConst* varValuep = جديد AstConst{nodep->fileline(), loop_قدر};
                    إذا (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ثابت tempp
                            = جديد AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, خطأ};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ();
                        VL_DO_DANGLING(tempp->delete_وڻ(), tempp);
                    }
                    ثابت string اشاريو = Ast_ڳنڍ::encode_عدد(varValuep->منزل_SInt());
                    ثابت string nname = m_beginName + "__BRA__" + اشاريو + "__KET__";
                    oneloopp = جديد Ast_Gen_بلاڪ{oneloopp->fileline(), nname, oneloopp, خطأ};
                    VL_DO_DANGLING(ڌڪو_Deletep(varValuep), varValuep);
                    newbodysp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(newbodysp, oneloopp);

                    // loopValue += valInc
                    ثابت Ast_سونپيو* ثابت incpass = VN_AS(incp, سونپيو);
                    V3_عدد new_Loop_قدر{nodep};
                    إذا (!simulate_وڻ(incpass->rhsp(), &loop_قدر, incpass, new_Loop_قدر)) {
                        nodep->v3error("Loop unrolling failed");
                        أرجع خطأ;
                    }
                    loop_قدر.op_سونپيو(new_Loop_قدر);
                }
            }
        }
        إذا (!newbodysp) {  // initp might have effects after the loop
            إذا (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = جديد AstInitial{initp->fileline(), initp->clone_وڻ(سچ)};
            } وإلا {
                newbodysp = initp ? initp->clone_وڻ(سچ) : خالي_اشارو;
            }
        }
        // Replace the FOR()
        إذا (newbodysp) {
            nodep->replaceWith(newbodysp);
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        إذا (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        أرجع سچ;
    }

    // VISITORS
    فراغ گهمو(AstVarRef* nodep) override {
        إذا (m_var_گهڻائي_Check && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_var_سونپيو_ڪاميابي = سچ;
        }
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (m_var_گهڻائي_Check && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } وإلا {
            iterateChildren(nodep);
        }
    }

عام:
    // CONSTRUCTORS
    UnrollGenVisitor() = افتراضي;

    فراغ unroll(AstGenFor* nodep, ثابت std::string& beginName) {
        m_forVarp = خالي_اشارو;  // Iterator variable
        m_ignoreIncp = خالي_اشارو;  // Increment node to ignore
        m_var_گهڻائي_Check = خطأ;  // Just checking RHS assignments
        m_var_سونپيو_ڪاميابي = خطأ;  // Assign var hit
        m_fork_ڪاميابي = خطأ;  // Fork hit
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
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
        } وإلا إذا (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);  // Did replacement
        } وإلا {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{جديد UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(ميٽيو m_unrollerp, m_unrollerp); }
فراغ GenForUnroller::unroll(AstGenFor* nodep, ثابت std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
