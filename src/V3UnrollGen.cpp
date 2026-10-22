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
    AstVar* m_forVarp = nullptr;  // Iterator variable
    ثابت Ast_گرہ* m_ignoreIncp = nullptr;  // Increment node to ignore
    منطقي m_varModeCheck = خطأ;  // Just checking RHS assignments
    منطقي m_varAssignHit = خطأ;  // Assign var hit
    منطقي m_forkHit = خطأ;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    فراغ replaceVarRef(Ast_گرہ* bodyp, Ast_گرہ* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([هذا, varValuep](AstVarRef* refp) {
            إذا (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_گرہ* ثابت newconstp = varValuep->clone_درخت(خطأ);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
            }
        });
    }

    منطقي cantUnroll(Ast_گرہ* nodep, ثابت حرف* reason) ثابت {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        أرجع خطأ;
    }

    منطقي forUnrollCheck(
        AstGenFor* ثابت nodep,
        Ast_گرہ* ثابت initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_گرہ_اظہاریہ* condp,
        Ast_گرہ* ثابت incp,  // Maybe under nodep or in bodysp
        Ast_گرہ* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        إذا (initp) UINFO(6, "    Init " << initp);
        إذا (condp) UINFO(6, "    Cond " << condp);
        إذا (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* ثابت initAssp = VN_CAST(initp, Assign);
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
        ثابت AstAssign* ثابت incAssp = VN_CAST(incp, Assign);
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
        m_varModeCheck = true;
        m_varAssignHit = خطأ;
        m_forkHit = خطأ;
        m_ignoreIncp = incp;
        iterateAndNextNull(bodysp);
        iterateAndNextNull(incp);
        m_varModeCheck = خطأ;
        m_ignoreIncp = nullptr;
        إذا (m_varAssignHit) أرجع cantUnroll(nodep, "genvar assigned *inside* loop");

        إذا (m_forkHit) أرجع cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        إذا (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            أرجع cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        أرجع true;
    }

    منطقي simulate_درخت(Ast_گرہ_اظہاریہ* nodep, ثابت V3_نمبر* loop_قدر, ثابت Ast_گرہ* dtypep,
                      V3_نمبر& out_عدد) {
        Ast_گرہ* clonep = nodep->clone_درخت(true);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        إذا (loop_قدر) {
            AstConst* varValuep = جديد AstConst{nodep->fileline(), *loop_قدر};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", clonep, خطأ};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->delete_درخت(), tempp = nullptr);
            VL_DO_DANGLING(ڈالو_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        إذا (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_درخت(), clonep);
            أرجع خطأ;
        }
        // Fetch the result
        ثابت V3_نمبر* resp = simvis.fetch_نمبر_Null(clonep);
        إذا (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_درخت(), clonep);
            أرجع خطأ;
        }
        // Patch up datatype
        إذا (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_عدد = new_con.عدد();
            out_عدد.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_درخت(), clonep);
            أرجع true;
        }
        out_عدد = *resp;
        VL_DO_DANGLING(clonep->delete_درخت(), clonep);
        أرجع true;
    }

    منطقي forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* incp,
                     Ast_گرہ* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_نمبر loop_قدر{nodep};
        إذا (!simulate_درخت(initp->rhsp(), nullptr, initp, loop_قدر)) {  //
            أرجع خطأ;
        }
        Ast_گرہ* stmtsp = nullptr;
        إذا (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ڈالو_Deletep(initp);  // Always cloned below.
        }
        إذا (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = Ast_گرہ::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(true);

        Ast_گرہ* newbodysp = nullptr;
        إذا (stmtsp) {
            ڈالو_Deletep(stmtsp);  // Always cloned below.
            size_t iter_گنتی = 0;
            ثابت size_t iterLimit = v3Global.opt.unrollLimit();
            طالما (true) {
                UINFO(8, "      Looping " << loop_قدر);
                V3_نمبر res{nodep};
                إذا (!simulate_درخت(condp, &loop_قدر, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    أرجع خطأ;
                }
                إذا (!res.isEqOne()) {
                    اكسر;  // Done with the loop
                } وإلا {
                    إذا (++iter_گنتی > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        اكسر;
                    }

                    // Replace iterator values with constant
                    Ast_گرہ* oneloopp = stmtsp->clone_درخت(true);
                    AstConst* varValuep = جديد AstConst{nodep->fileline(), loop_قدر};
                    إذا (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ثابت tempp
                            = جديد AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, خطأ};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->delete_درخت(), tempp);
                    }
                    ثابت string index = Ast_گرہ::encode_نمبر(varValuep->toSInt());
                    ثابت string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = جديد AstGenBlock{oneloopp->fileline(), nname, oneloopp, خطأ};
                    VL_DO_DANGLING(ڈالو_Deletep(varValuep), varValuep);
                    newbodysp = Ast_گرہ::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    ثابت AstAssign* ثابت incpass = VN_AS(incp, Assign);
                    V3_نمبر new_Loop_قدر{nodep};
                    إذا (!simulate_درخت(incpass->rhsp(), &loop_قدر, incpass, new_Loop_قدر)) {
                        nodep->v3error("Loop unrolling failed");
                        أرجع خطأ;
                    }
                    loop_قدر.opAssign(new_Loop_قدر);
                }
            }
        }
        إذا (!newbodysp) {  // initp might have effects after the loop
            إذا (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = جديد AstInitial{initp->fileline(), initp->clone_درخت(true)};
            } وإلا {
                newbodysp = initp ? initp->clone_درخت(true) : nullptr;
            }
        }
        // Replace the FOR()
        إذا (newbodysp) {
            nodep->replaceWith(newbodysp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        إذا (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        أرجع true;
    }

    // VISITORS
    فراغ visit(AstVarRef* nodep) override {
        إذا (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = true;
        }
    }

    فراغ visit(Ast_گرہ* nodep) override {
        إذا (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } وإلا {
            iterateChildren(nodep);
        }
    }

عام:
    // CONSTRUCTORS
    UnrollGenVisitor() = افتراضي;

    فراغ unroll(AstGenFor* nodep, ثابت std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = خطأ;  // Just checking RHS assignments
        m_varAssignHit = خطأ;  // Assign var hit
        m_forkHit = خطأ;  // Fork hit
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
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlinkFrBack()), nodep);
        } وإلا إذا (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);  // Did replacement
        } وإلا {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{جديد UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
فراغ GenForUnroller::unroll(AstGenFor* nodep, ثابت std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
