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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Simulate.h"
#ಅನ್ತರ್ಭಾವ "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

ವರ್ಗ UnrollGenVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = nullptr;  // Iterator variable
    ಧ್ರುವ Ast_ಗಂಟು* m_ignoreIncp = nullptr;  // Increment node to ignore
    ತಾರ್ಕಿಕ m_varModeCheck = ಅಸತ್ಯ;  // Just checking RHS assignments
    ತಾರ್ಕಿಕ m_varAssignHit = ಅಸತ್ಯ;  // Assign var hit
    ತಾರ್ಕಿಕ m_forkHit = ಅಸತ್ಯ;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    ಶೂನ್ಯ replaceVarRef(Ast_ಗಂಟು* bodyp, Ast_ಗಂಟು* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([ಏತತ್, varValuep](AstVarRef* refp) {
            ಯದಿ (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_ಗಂಟು* ಧ್ರುವ newconstp = varValuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
            }
        });
    }

    ತಾರ್ಕಿಕ cantUnroll(Ast_ಗಂಟು* nodep, ಧ್ರುವ ವರ್ಣ* reason) ಧ್ರುವ {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ forUnrollCheck(
        AstGenFor* ಧ್ರುವ nodep,
        Ast_ಗಂಟು* ಧ್ರುವ initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
        Ast_ಗಂಟು* ಧ್ರುವ incp,  // Maybe under nodep or in bodysp
        Ast_ಗಂಟು* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        ಯದಿ (initp) UINFO(6, "    Init " << initp);
        ಯದಿ (condp) UINFO(6, "    Cond " << condp);
        ಯದಿ (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* ಧ್ರುವ initAssp = VN_CAST(initp, Assign);
        ಯದಿ (!initAssp) ಪ್ರತ್ಯಾಗಮ cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        ಯದಿ (!VN_IS(initAssp->lhsp(), VarRef)) {
            ಪ್ರತ್ಯಾಗಮ cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ಧ್ರುವ AstAssign* ಧ್ರುವ incAssp = VN_CAST(incp, Assign);
        ಯದಿ (!incAssp) ಪ್ರತ್ಯಾಗಮ cantUnroll(nodep, "no increment assignment");
        ಯದಿ (incAssp->nextp()) ಪ್ರತ್ಯಾಗಮ cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        ಯದಿ (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        ಧ್ರುವ AstConst* ಧ್ರುವ constInitp = VN_CAST(initAssp->rhsp(), Const);
        ಯದಿ (!constInitp) ಪ್ರತ್ಯಾಗಮ cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = ಸತ್ಯ;
        m_varAssignHit = ಅಸತ್ಯ;
        m_forkHit = ಅಸತ್ಯ;
        m_ignoreIncp = incp;
        iterateAndNextNull(bodysp);
        iterateAndNextNull(incp);
        m_varModeCheck = ಅಸತ್ಯ;
        m_ignoreIncp = nullptr;
        ಯದಿ (m_varAssignHit) ಪ್ರತ್ಯಾಗಮ cantUnroll(nodep, "genvar assigned *inside* loop");

        ಯದಿ (m_forkHit) ಪ್ರತ್ಯಾಗಮ cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        ಯದಿ (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            ಪ್ರತ್ಯಾಗಮ cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ simulate_ವೃಕ್ಷ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಧ್ರುವ V3_ಸಂಖ್ಯೆ* loop_ಮೌಲ್ಯ, ಧ್ರುವ Ast_ಗಂಟು* dtypep,
                      V3_ಸಂಖ್ಯೆ& out_ಅಂಕೆ) {
        Ast_ಗಂಟು* clonep = nodep->clone_ವೃಕ್ಷ(ಸತ್ಯ);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        ಯದಿ (loop_ಮೌಲ್ಯ) {
            AstConst* varValuep = ನೂತನ AstConst{nodep->fileline(), *loop_ಮೌಲ್ಯ};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = ನೂತನ AstBegin{nodep->fileline(), "[EditWrapper]", clonep, ಅಸತ್ಯ};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->delete_ವೃಕ್ಷ(), tempp = nullptr);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        ಯದಿ (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        // Fetch the result
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ* resp = simvis.fetch_ಸಂಖ್ಯೆ_Null(clonep);
        ಯದಿ (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        // Patch up datatype
        ಯದಿ (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_ಅಂಕೆ = new_con.ಅಂಕೆ();
            out_ಅಂಕೆ.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        out_ಅಂಕೆ = *resp;
        VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* incp,
                     Ast_ಗಂಟು* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_ಸಂಖ್ಯೆ loop_ಮೌಲ್ಯ{nodep};
        ಯದಿ (!simulate_ವೃಕ್ಷ(initp->rhsp(), nullptr, initp, loop_ಮೌಲ್ಯ)) {  //
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        Ast_ಗಂಟು* stmtsp = nullptr;
        ಯದಿ (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ತಳ್ಳು_Deletep(initp);  // Always cloned below.
        }
        ಯದಿ (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = Ast_ಗಂಟು::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(ಸತ್ಯ);

        Ast_ಗಂಟು* newbodysp = nullptr;
        ಯದಿ (stmtsp) {
            ತಳ್ಳು_Deletep(stmtsp);  // Always cloned below.
            size_t iter_ಎಣಿಕೆ = 0;
            ಧ್ರುವ size_t iterLimit = v3Global.opt.unrollLimit();
            ಯಾವತ್ (ಸತ್ಯ) {
                UINFO(8, "      Looping " << loop_ಮೌಲ್ಯ);
                V3_ಸಂಖ್ಯೆ res{nodep};
                ಯದಿ (!simulate_ವೃಕ್ಷ(condp, &loop_ಮೌಲ್ಯ, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
                ಯದಿ (!res.isEqOne()) {
                    ಭಙ್ಗ;  // Done with the loop
                } ಅನ್ಯಥಾ {
                    ಯದಿ (++iter_ಎಣಿಕೆ > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        ಭಙ್ಗ;
                    }

                    // Replace iterator values with constant
                    Ast_ಗಂಟು* oneloopp = stmtsp->clone_ವೃಕ್ಷ(ಸತ್ಯ);
                    AstConst* varValuep = ನೂತನ AstConst{nodep->fileline(), loop_ಮೌಲ್ಯ};
                    ಯದಿ (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ಧ್ರುವ tempp
                            = ನೂತನ AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, ಅಸತ್ಯ};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->delete_ವೃಕ್ಷ(), tempp);
                    }
                    ಧ್ರುವ string index = Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(varValuep->toSInt());
                    ಧ್ರುವ string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = ನೂತನ AstGenBlock{oneloopp->fileline(), nname, oneloopp, ಅಸತ್ಯ};
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(varValuep), varValuep);
                    newbodysp = Ast_ಗಂಟು::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    ಧ್ರುವ AstAssign* ಧ್ರುವ incpass = VN_AS(incp, Assign);
                    V3_ಸಂಖ್ಯೆ new_Loop_ಮೌಲ್ಯ{nodep};
                    ಯದಿ (!simulate_ವೃಕ್ಷ(incpass->rhsp(), &loop_ಮೌಲ್ಯ, incpass, new_Loop_ಮೌಲ್ಯ)) {
                        nodep->v3error("Loop unrolling failed");
                        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                    }
                    loop_ಮೌಲ್ಯ.opAssign(new_Loop_ಮೌಲ್ಯ);
                }
            }
        }
        ಯದಿ (!newbodysp) {  // initp might have effects after the loop
            ಯದಿ (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = ನೂತನ AstInitial{initp->fileline(), initp->clone_ವೃಕ್ಷ(ಸತ್ಯ)};
            } ಅನ್ಯಥಾ {
                newbodysp = initp ? initp->clone_ವೃಕ್ಷ(ಸತ್ಯ) : nullptr;
            }
        }
        // Replace the FOR()
        ಯದಿ (newbodysp) {
            nodep->replaceWith(newbodysp);
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        ಯದಿ (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = ಸತ್ಯ;
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    UnrollGenVisitor() = ಸ್ವಭಾವ;

    ಶೂನ್ಯ unroll(AstGenFor* nodep, ಧ್ರುವ std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = ಅಸತ್ಯ;  // Just checking RHS assignments
        m_varAssignHit = ಅಸತ್ಯ;  // Assign var hit
        m_forkHit = ಅಸತ್ಯ;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        ಯದಿ (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        ಯದಿ (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        ಯದಿ (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        ಯದಿ (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);  // Did replacement
        } ಅನ್ಯಥಾ {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{ನೂತನ UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
ಶೂನ್ಯ GenForUnroller::unroll(AstGenFor* nodep, ಧ್ರುವ std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
