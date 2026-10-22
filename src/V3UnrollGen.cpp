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
    AstVar* m_forVarp = ಶೂನ್ಯಸೂಚಕ;  // Iterator variable
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_ignoreIncp = ಶೂನ್ಯಸೂಚಕ;  // Increment node to ignore
    ತಾರ್ಕಿಕ m_var_ಬಹುಲಕ_Check = ಸುಳ್ಳು;  // Just checking RHS assignments
    ತಾರ್ಕಿಕ m_var_ನಿಯೋಜಿಸು_ದೊರಕಿತು = ಸುಳ್ಳು;  // Assign var hit
    ತಾರ್ಕಿಕ m_fork_ದೊರಕಿತು = ಸುಳ್ಳು;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    ಶೂನ್ಯ replaceVarRef(Ast_ಗಂಟು* bodyp, Ast_ಗಂಟು* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreach_And_ಮುಂದಿನ([ಇದು, varValuep](AstVarRef* refp) {
            ಆದರೆ (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newconstp = varValuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
            }
        });
    }

    ತಾರ್ಕಿಕ cantUnroll(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* reason) ಸ್ಥಿರಾಂಕ {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ತಾರ್ಕಿಕ forUnrollCheck(
        AstGenFor* ಸ್ಥಿರಾಂಕ nodep,
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ incp,  // Maybe under nodep or in bodysp
        Ast_ಗಂಟು* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        ಆದರೆ (initp) UINFO(6, "    Init " << initp);
        ಆದರೆ (condp) UINFO(6, "    Cond " << condp);
        ಆದರೆ (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ initAssp = VN_CAST(initp, ನಿಯೋಜಿಸು);
        ಆದರೆ (!initAssp) ಹಿಂತಿರುಗಿ cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        ಆದರೆ (!VN_IS(initAssp->lhsp(), VarRef)) {
            ಹಿಂತಿರುಗಿ cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ incAssp = VN_CAST(incp, ನಿಯೋಜಿಸು);
        ಆದರೆ (!incAssp) ಹಿಂತಿರುಗಿ cantUnroll(nodep, "no increment assignment");
        ಆದರೆ (incAssp->nextp()) ಹಿಂತಿರುಗಿ cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        ಆದರೆ (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constInitp = VN_CAST(initAssp->rhsp(), Const);
        ಆದರೆ (!constInitp) ಹಿಂತಿರುಗಿ cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_var_ಬಹುಲಕ_Check = ನಿಜ;
        m_var_ನಿಯೋಜಿಸು_ದೊರಕಿತು = ಸುಳ್ಳು;
        m_fork_ದೊರಕಿತು = ಸುಳ್ಳು;
        m_ignoreIncp = incp;
        iterate_And_ಮುಂದಿನ_Null(bodysp);
        iterate_And_ಮುಂದಿನ_Null(incp);
        m_var_ಬಹುಲಕ_Check = ಸುಳ್ಳು;
        m_ignoreIncp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (m_var_ನಿಯೋಜಿಸು_ದೊರಕಿತು) ಹಿಂತಿರುಗಿ cantUnroll(nodep, "genvar assigned *inside* loop");

        ಆದರೆ (m_fork_ದೊರಕಿತು) ಹಿಂತಿರುಗಿ cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        ಆದರೆ (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            ಹಿಂತಿರುಗಿ cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ತಾರ್ಕಿಕ simulate_ವೃಕ್ಷ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ* loop_ಮೌಲ್ಯ, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* dtypep,
                      V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು_ಅಂಕೆ) {
        Ast_ಗಂಟು* clonep = nodep->clone_ವೃಕ್ಷ(ನಿಜ);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        ಆದರೆ (loop_ಮೌಲ್ಯ) {
            AstConst* varValuep = ಹೊಸ AstConst{nodep->fileline(), *loop_ಮೌಲ್ಯ};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = ಹೊಸ AstBegin{nodep->fileline(), "[EditWrapper]", clonep, ಸುಳ್ಳು};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            VL_DO_CLEAR(tempp->delete_ವೃಕ್ಷ(), tempp = ಶೂನ್ಯಸೂಚಕ);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        ಆದರೆ (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // Fetch the result
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ* resp = simvis.fetch_ಸಂಖ್ಯೆ_Null(clonep);
        ಆದರೆ (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // Patch up datatype
        ಆದರೆ (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            ಹೊರಹೋಗು_ಅಂಕೆ = new_con.ಅಂಕೆ();
            ಹೊರಹೋಗು_ಅಂಕೆ.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹೊರಹೋಗು_ಅಂಕೆ = *resp;
        VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ತಾರ್ಕಿಕ forUnroller(AstGenFor* nodep, Ast_ನಿಯೋಜಿಸು* initp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* incp,
                     Ast_ಗಂಟು* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_ಸಂಖ್ಯೆ loop_ಮೌಲ್ಯ{nodep};
        ಆದರೆ (!simulate_ವೃಕ್ಷ(initp->rhsp(), ಶೂನ್ಯಸೂಚಕ, initp, loop_ಮೌಲ್ಯ)) {  //
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        Ast_ಗಂಟು* stmtsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (initp) {
            initp->unlink_Fr_ಹಿಂಭಾಗ();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ತಳ್ಳು_Deletep(initp);  // Always cloned below.
        }
        ಆದರೆ (bodysp) {
            bodysp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->ಬಳಸಿದ_Loop_Idx(ನಿಜ);

        Ast_ಗಂಟು* newbodysp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (stmtsp) {
            ತಳ್ಳು_Deletep(stmtsp);  // Always cloned below.
            ಗಾತ್ರ_t iter_ಎಣಿಕೆ = 0;
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t iter_ಎಲ್ಲೆ = v3Global.opt.unroll_ಎಲ್ಲೆ();
            ಆಗ (ನಿಜ) {
                UINFO(8, "      Looping " << loop_ಮೌಲ್ಯ);
                V3_ಸಂಖ್ಯೆ res{nodep};
                ಆದರೆ (!simulate_ವೃಕ್ಷ(condp, &loop_ಮೌಲ್ಯ, ಶೂನ್ಯಸೂಚಕ, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                ಆದರೆ (!res.isEqOne()) {
                    ನಿಲ್ಲಿಸು;  // Done with the loop
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (++iter_ಎಣಿಕೆ > iter_ಎಲ್ಲೆ) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iter_ಎಲ್ಲೆ);
                        ನಿಲ್ಲಿಸು;
                    }

                    // Replace iterator values with constant
                    Ast_ಗಂಟು* oneloopp = stmtsp->clone_ವೃಕ್ಷ(ನಿಜ);
                    AstConst* varValuep = ಹೊಸ AstConst{nodep->fileline(), loop_ಮೌಲ್ಯ};
                    ಆದರೆ (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ಸ್ಥಿರಾಂಕ tempp
                            = ಹೊಸ AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, ಸುಳ್ಳು};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                        VL_DO_DANGLING(tempp->delete_ವೃಕ್ಷ(), tempp);
                    }
                    ಸ್ಥಿರಾಂಕ string ಸೂಚ್ಯಂಕ = Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(varValuep->ಗಮ್ಯ_SInt());
                    ಸ್ಥಿರಾಂಕ string nname = m_beginName + "__BRA__" + ಸೂಚ್ಯಂಕ + "__KET__";
                    oneloopp = ಹೊಸ Ast_Gen_ಖಂಡ{oneloopp->fileline(), nname, oneloopp, ಸುಳ್ಳು};
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(varValuep), varValuep);
                    newbodysp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(newbodysp, oneloopp);

                    // loopValue += valInc
                    ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ incpass = VN_AS(incp, ನಿಯೋಜಿಸು);
                    V3_ಸಂಖ್ಯೆ new_Loop_ಮೌಲ್ಯ{nodep};
                    ಆದರೆ (!simulate_ವೃಕ್ಷ(incpass->rhsp(), &loop_ಮೌಲ್ಯ, incpass, new_Loop_ಮೌಲ್ಯ)) {
                        nodep->v3error("Loop unrolling failed");
                        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    }
                    loop_ಮೌಲ್ಯ.op_ನಿಯೋಜಿಸು(new_Loop_ಮೌಲ್ಯ);
                }
            }
        }
        ಆದರೆ (!newbodysp) {  // initp might have effects after the loop
            ಆದರೆ (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = ಹೊಸ AstInitial{initp->fileline(), initp->clone_ವೃಕ್ಷ(ನಿಜ)};
            } ಇಲ್ಲದಿದ್ದರೆ {
                newbodysp = initp ? initp->clone_ವೃಕ್ಷ(ನಿಜ) : ಶೂನ್ಯಸೂಚಕ;
            }
        }
        // Replace the FOR()
        ಆದರೆ (newbodysp) {
            nodep->replaceWith(newbodysp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        ಆದರೆ (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (m_var_ಬಹುಲಕ_Check && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_var_ನಿಯೋಜಿಸು_ದೊರಕಿತು = ನಿಜ;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (m_var_ಬಹುಲಕ_Check && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    UnrollGenVisitor() = ಪೂರ್ವನಿಯೋಜಿತ;

    ಶೂನ್ಯ unroll(AstGenFor* nodep, ಸ್ಥಿರಾಂಕ std::string& beginName) {
        m_forVarp = ಶೂನ್ಯಸೂಚಕ;  // Iterator variable
        m_ignoreIncp = ಶೂನ್ಯಸೂಚಕ;  // Increment node to ignore
        m_var_ಬಹುಲಕ_Check = ಸುಳ್ಳು;  // Just checking RHS assignments
        m_var_ನಿಯೋಜಿಸು_ದೊರಕಿತು = ಸುಳ್ಳು;  // Assign var hit
        m_fork_ದೊರಕಿತು = ಸುಳ್ಳು;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        ಆದರೆ (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        ಆದರೆ (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        ಆದರೆ (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        ಆದರೆ (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);  // Did replacement
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{ಹೊಸ UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(ಅಳಿಸು m_unrollerp, m_unrollerp); }
ಶೂನ್ಯ GenForUnroller::unroll(AstGenFor* nodep, ಸ್ಥಿರಾಂಕ std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
