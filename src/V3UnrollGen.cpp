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

رده UnrollGenVisitor final : عمومی VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = nullptr;  // Iterator variable
    ثابت Ast_گره* m_ignoreIncp = nullptr;  // Increment node to ignore
    منطقی m_varModeCheck = نادرست;  // Just checking RHS assignments
    منطقی m_varAssignHit = نادرست;  // Assign var hit
    منطقی m_forkHit = نادرست;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    تهی replaceVarRef(Ast_گره* bodyp, Ast_گره* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([این, varValuep](AstVarRef* refp) {
            اگر (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_گره* ثابت newconstp = varValuep->clone_درخت(نادرست);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(افزودن_Deletep(refp), refp);
            }
        });
    }

    منطقی cantUnroll(Ast_گره* nodep, ثابت نویسه* reason) ثابت {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        بازگشت نادرست;
    }

    منطقی forUnrollCheck(
        AstGenFor* ثابت nodep,
        Ast_گره* ثابت initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_گره_عبارت* condp,
        Ast_گره* ثابت incp,  // Maybe under nodep or in bodysp
        Ast_گره* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        اگر (initp) UINFO(6, "    Init " << initp);
        اگر (condp) UINFO(6, "    Cond " << condp);
        اگر (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* ثابت initAssp = VN_CAST(initp, Assign);
        اگر (!initAssp) بازگشت cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        اگر (!VN_IS(initAssp->lhsp(), VarRef)) {
            بازگشت cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ثابت AstAssign* ثابت incAssp = VN_CAST(incp, Assign);
        اگر (!incAssp) بازگشت cantUnroll(nodep, "no increment assignment");
        اگر (incAssp->nextp()) بازگشت cantUnroll(nodep, "multiple increments");

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
        اگر (!constInitp) بازگشت cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = درست;
        m_varAssignHit = نادرست;
        m_forkHit = نادرست;
        m_ignoreIncp = incp;
        iterateAndNextNull(bodysp);
        iterateAndNextNull(incp);
        m_varModeCheck = نادرست;
        m_ignoreIncp = nullptr;
        اگر (m_varAssignHit) بازگشت cantUnroll(nodep, "genvar assigned *inside* loop");

        اگر (m_forkHit) بازگشت cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        اگر (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            بازگشت cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        بازگشت درست;
    }

    منطقی simulate_درخت(Ast_گره_عبارت* nodep, ثابت V3_عدد* loop_مقدار, ثابت Ast_گره* dtypep,
                      V3_عدد& out_رقم) {
        Ast_گره* clonep = nodep->clone_درخت(درست);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        اگر (loop_مقدار) {
            AstConst* varValuep = جدید AstConst{nodep->fileline(), *loop_مقدار};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = جدید AstBegin{nodep->fileline(), "[EditWrapper]", clonep, نادرست};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->delete_درخت(), tempp = nullptr);
            VL_DO_DANGLING(افزودن_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        اگر (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_درخت(), clonep);
            بازگشت نادرست;
        }
        // Fetch the result
        ثابت V3_عدد* resp = simvis.fetch_عدد_Null(clonep);
        اگر (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_درخت(), clonep);
            بازگشت نادرست;
        }
        // Patch up datatype
        اگر (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_رقم = new_con.رقم();
            out_رقم.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_درخت(), clonep);
            بازگشت درست;
        }
        out_رقم = *resp;
        VL_DO_DANGLING(clonep->delete_درخت(), clonep);
        بازگشت درست;
    }

    منطقی forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_گره_عبارت* condp, Ast_گره* incp,
                     Ast_گره* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_عدد loop_مقدار{nodep};
        اگر (!simulate_درخت(initp->rhsp(), nullptr, initp, loop_مقدار)) {  //
            بازگشت نادرست;
        }
        Ast_گره* stmtsp = nullptr;
        اگر (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            افزودن_Deletep(initp);  // Always cloned below.
        }
        اگر (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = Ast_گره::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(درست);

        Ast_گره* newbodysp = nullptr;
        اگر (stmtsp) {
            افزودن_Deletep(stmtsp);  // Always cloned below.
            size_t iter_شمار = 0;
            ثابت size_t iterLimit = v3Global.opt.unrollLimit();
            وقتی (درست) {
                UINFO(8, "      Looping " << loop_مقدار);
                V3_عدد res{nodep};
                اگر (!simulate_درخت(condp, &loop_مقدار, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    بازگشت نادرست;
                }
                اگر (!res.isEqOne()) {
                    بشکن;  // Done with the loop
                } وگرنه {
                    اگر (++iter_شمار > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        بشکن;
                    }

                    // Replace iterator values with constant
                    Ast_گره* oneloopp = stmtsp->clone_درخت(درست);
                    AstConst* varValuep = جدید AstConst{nodep->fileline(), loop_مقدار};
                    اگر (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ثابت tempp
                            = جدید AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, نادرست};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->delete_درخت(), tempp);
                    }
                    ثابت string index = Ast_گره::encode_عدد(varValuep->toSInt());
                    ثابت string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = جدید AstGenBlock{oneloopp->fileline(), nname, oneloopp, نادرست};
                    VL_DO_DANGLING(افزودن_Deletep(varValuep), varValuep);
                    newbodysp = Ast_گره::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    ثابت AstAssign* ثابت incpass = VN_AS(incp, Assign);
                    V3_عدد new_Loop_مقدار{nodep};
                    اگر (!simulate_درخت(incpass->rhsp(), &loop_مقدار, incpass, new_Loop_مقدار)) {
                        nodep->v3error("Loop unrolling failed");
                        بازگشت نادرست;
                    }
                    loop_مقدار.opAssign(new_Loop_مقدار);
                }
            }
        }
        اگر (!newbodysp) {  // initp might have effects after the loop
            اگر (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = جدید AstInitial{initp->fileline(), initp->clone_درخت(درست)};
            } وگرنه {
                newbodysp = initp ? initp->clone_درخت(درست) : nullptr;
            }
        }
        // Replace the FOR()
        اگر (newbodysp) {
            nodep->replaceWith(newbodysp);
        } وگرنه {
            nodep->unlinkFrBack();
        }
        اگر (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        بازگشت درست;
    }

    // VISITORS
    تهی visit(AstVarRef* nodep) override {
        اگر (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = درست;
        }
    }

    تهی visit(Ast_گره* nodep) override {
        اگر (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } وگرنه {
            iterateChildren(nodep);
        }
    }

عمومی:
    // CONSTRUCTORS
    UnrollGenVisitor() = پیشفرض;

    تهی unroll(AstGenFor* nodep, ثابت std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = نادرست;  // Just checking RHS assignments
        m_varAssignHit = نادرست;  // Assign var hit
        m_forkHit = نادرست;  // Fork hit
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
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        } وگرنه اگر (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);  // Did replacement
        } وگرنه {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{جدید UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
تهی GenForUnroller::unroll(AstGenFor* nodep, ثابت std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
