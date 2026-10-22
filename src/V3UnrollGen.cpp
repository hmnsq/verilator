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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Const.h"
#include "V3Simulate.h"
#include "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

class UnrollGenVisitor final : public VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = nullptr;  // Iterator variable
    const AstNode* m_ignoreIncp = nullptr;  // Increment node to ignore
    bool m_varModeCheck = false;  // Just checking RHS assignments
    bool m_varAssignHit = false;  // Assign var hit
    bool m_forkHit = false;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    void replaceVarRef(AstNode* bodyp, AstNode* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([this, varValuep](AstVarRef* refp) {
            ᱡᱩᱫᱤ (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                AstNode* const newconstp = varValuep->cloneTree(false);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(pushDeletep(refp), refp);
            }
        });
    }

    bool cantUnroll(AstNode* nodep, const var_na* reason) const {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        ᱨᱩᱣᱟᱹᱲ false;
    }

    bool forUnrollCheck(
        AstGenFor* const nodep,
        AstNode* const initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        AstNodeExpr* condp,
        AstNode* const incp,  // Maybe under nodep or in bodysp
        AstNode* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        ᱡᱩᱫᱤ (initp) UINFO(6, "    Init " << initp);
        ᱡᱩᱫᱤ (condp) UINFO(6, "    Cond " << condp);
        ᱡᱩᱫᱤ (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* const initAssp = VN_CAST(initp, Assign);
        ᱡᱩᱫᱤ (!initAssp) ᱨᱩᱣᱟᱹᱲ cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        ᱡᱩᱫᱤ (!VN_IS(initAssp->lhsp(), VarRef)) {
            ᱨᱩᱣᱟᱹᱲ cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        const AstAssign* const incAssp = VN_CAST(incp, Assign);
        ᱡᱩᱫᱤ (!incAssp) ᱨᱩᱣᱟᱹᱲ cantUnroll(nodep, "no increment assignment");
        ᱡᱩᱫᱤ (incAssp->nextp()) ᱨᱩᱣᱟᱹᱲ cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        ᱡᱩᱫᱤ (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        const AstConst* const constInitp = VN_CAST(initAssp->rhsp(), Const);
        ᱡᱩᱫᱤ (!constInitp) ᱨᱩᱣᱟᱹᱲ cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = true;
        m_varAssignHit = false;
        m_forkHit = false;
        m_ignoreIncp = incp;
        iterateAndNextNull(bodysp);
        iterateAndNextNull(incp);
        m_varModeCheck = false;
        m_ignoreIncp = nullptr;
        ᱡᱩᱫᱤ (m_varAssignHit) ᱨᱩᱣᱟᱹᱲ cantUnroll(nodep, "genvar assigned *inside* loop");

        ᱡᱩᱫᱤ (m_forkHit) ᱨᱩᱣᱟᱹᱲ cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        ᱡᱩᱫᱤ (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            ᱨᱩᱣᱟᱹᱲ cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        ᱨᱩᱣᱟᱹᱲ true;
    }

    bool simulateTree(AstNodeExpr* nodep, const V3Number* loopValue, const AstNode* dtypep,
                      V3Number& outNum) {
        AstNode* clonep = nodep->cloneTree(true);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        ᱡᱩᱫᱤ (loopValue) {
            AstConst* varValuep = new AstConst{nodep->fileline(), *loopValue};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = new AstBegin{nodep->fileline(), "[EditWrapper]", clonep, false};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->deleteTree(), tempp = nullptr);
            VL_DO_DANGLING(pushDeletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        ᱡᱩᱫᱤ (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->deleteTree(), clonep);
            ᱨᱩᱣᱟᱹᱲ false;
        }
        // Fetch the result
        const V3Number* resp = simvis.fetchNumberNull(clonep);
        ᱡᱩᱫᱤ (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->deleteTree(), clonep);
            ᱨᱩᱣᱟᱹᱲ false;
        }
        // Patch up datatype
        ᱡᱩᱫᱤ (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            outNum = new_con.num();
            outNum.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->deleteTree(), clonep);
            ᱨᱩᱣᱟᱹᱲ true;
        }
        outNum = *resp;
        VL_DO_DANGLING(clonep->deleteTree(), clonep);
        ᱨᱩᱣᱟᱹᱲ true;
    }

    bool forUnroller(AstGenFor* nodep, AstAssign* initp, AstNodeExpr* condp, AstNode* incp,
                     AstNode* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3Number loopValue{nodep};
        ᱡᱩᱫᱤ (!simulateTree(initp->rhsp(), nullptr, initp, loopValue)) {  //
            ᱨᱩᱣᱟᱹᱲ false;
        }
        AstNode* stmtsp = nullptr;
        ᱡᱩᱫᱤ (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            pushDeletep(initp);  // Always cloned below.
        }
        ᱡᱩᱫᱤ (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = AstNode::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(true);

        AstNode* newbodysp = nullptr;
        ᱡᱩᱫᱤ (stmtsp) {
            pushDeletep(stmtsp);  // Always cloned below.
            size_t iterCount = 0;
            const size_t iterLimit = v3Global.opt.unrollLimit();
            ᱡᱚᱠᱷᱚᱱ (true) {
                UINFO(8, "      Looping " << loopValue);
                V3Number res{nodep};
                ᱡᱩᱫᱤ (!simulateTree(condp, &loopValue, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    ᱨᱩᱣᱟᱹᱲ false;
                }
                ᱡᱩᱫᱤ (!res.isEqOne()) {
                    bha_nga;  // Done with the loop
                } ᱵᱟᱝ {
                    ᱡᱩᱫᱤ (++iterCount > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        bha_nga;
                    }

                    // Replace iterator values with constant
                    AstNode* oneloopp = stmtsp->cloneTree(true);
                    AstConst* varValuep = new AstConst{nodep->fileline(), loopValue};
                    ᱡᱩᱫᱤ (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* const tempp
                            = new AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, false};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->deleteTree(), tempp);
                    }
                    const string index = AstNode::encodeNumber(varValuep->toSInt());
                    const string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = new AstGenBlock{oneloopp->fileline(), nname, oneloopp, false};
                    VL_DO_DANGLING(pushDeletep(varValuep), varValuep);
                    newbodysp = AstNode::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    const AstAssign* const incpass = VN_AS(incp, Assign);
                    V3Number newLoopValue{nodep};
                    ᱡᱩᱫᱤ (!simulateTree(incpass->rhsp(), &loopValue, incpass, newLoopValue)) {
                        nodep->v3error("Loop unrolling failed");
                        ᱨᱩᱣᱟᱹᱲ false;
                    }
                    loopValue.opAssign(newLoopValue);
                }
            }
        }
        ᱡᱩᱫᱤ (!newbodysp) {  // initp might have effects after the loop
            ᱡᱩᱫᱤ (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = new AstInitial{initp->fileline(), initp->cloneTree(true)};
            } ᱵᱟᱝ {
                newbodysp = initp ? initp->cloneTree(true) : nullptr;
            }
        }
        // Replace the FOR()
        ᱡᱩᱫᱤ (newbodysp) {
            nodep->replaceWith(newbodysp);
        } ᱵᱟᱝ {
            nodep->unlinkFrBack();
        }
        ᱡᱩᱫᱤ (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        ᱨᱩᱣᱟᱹᱲ true;
    }

    // VISITORS
    void visit(AstVarRef* nodep) override {
        ᱡᱩᱫᱤ (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = true;
        }
    }

    void visit(AstNode* nodep) override {
        ᱡᱩᱫᱤ (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } ᱵᱟᱝ {
            iterateChildren(nodep);
        }
    }

public:
    // CONSTRUCTORS
    UnrollGenVisitor() = default;

    void unroll(AstGenFor* nodep, const std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = false;  // Just checking RHS assignments
        m_varAssignHit = false;  // Assign var hit
        m_forkHit = false;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        ᱡᱩᱫᱤ (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        ᱡᱩᱫᱤ (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        ᱡᱩᱫᱤ (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        ᱡᱩᱫᱤ (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        } ᱵᱟᱝ ᱡᱩᱫᱤ (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(pushDeletep(nodep), nodep);  // Did replacement
        } ᱵᱟᱝ {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{new UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
void GenForUnroller::unroll(AstGenFor* nodep, const std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
