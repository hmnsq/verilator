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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Simulate.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

ਵਰਗ UnrollGenVisitor final : ਜਨਤਕ VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = ਖਾਲੀ_ਸੂਚਕ;  // Iterator variable
    ਅਚਲ Ast_ਗੰਢ* m_ignoreIncp = ਖਾਲੀ_ਸੂਚਕ;  // Increment node to ignore
    ਤਾਰਕਿਕ m_var_ਬਹੁਲਕ_Check = ਝੂਠ;  // Just checking RHS assignments
    ਤਾਰਕਿਕ m_var_ਸੌਂਪੋ_ਪ੍ਰਾਪਤੀ = ਝੂਠ;  // Assign var hit
    ਤਾਰਕਿਕ m_fork_ਪ੍ਰਾਪਤੀ = ਝੂਠ;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    ਖਾਲੀ replaceVarRef(Ast_ਗੰਢ* bodyp, Ast_ਗੰਢ* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreach_And_ਅਗਲਾ([ਇਹ, varValuep](AstVarRef* refp) {
            ਜੇ (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_ਗੰਢ* ਅਚਲ newconstp = varValuep->clone_ਰੁੱਖ(ਝੂਠ);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
            }
        });
    }

    ਤਾਰਕਿਕ cantUnroll(Ast_ਗੰਢ* nodep, ਅਚਲ ਅੱਖਰ* reason) ਅਚਲ {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        ਵਾਪਸ ਝੂਠ;
    }

    ਤਾਰਕਿਕ forUnrollCheck(
        AstGenFor* ਅਚਲ nodep,
        Ast_ਗੰਢ* ਅਚਲ initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp,
        Ast_ਗੰਢ* ਅਚਲ incp,  // Maybe under nodep or in bodysp
        Ast_ਗੰਢ* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        ਜੇ (initp) UINFO(6, "    Init " << initp);
        ਜੇ (condp) UINFO(6, "    Cond " << condp);
        ਜੇ (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        Ast_ਸੌਂਪੋ* ਅਚਲ initAssp = VN_CAST(initp, ਸੌਂਪੋ);
        ਜੇ (!initAssp) ਵਾਪਸ cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        ਜੇ (!VN_IS(initAssp->lhsp(), VarRef)) {
            ਵਾਪਸ cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ਅਚਲ Ast_ਸੌਂਪੋ* ਅਚਲ incAssp = VN_CAST(incp, ਸੌਂਪੋ);
        ਜੇ (!incAssp) ਵਾਪਸ cantUnroll(nodep, "no increment assignment");
        ਜੇ (incAssp->nextp()) ਵਾਪਸ cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        ਜੇ (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        ਅਚਲ AstConst* ਅਚਲ constInitp = VN_CAST(initAssp->rhsp(), Const);
        ਜੇ (!constInitp) ਵਾਪਸ cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_var_ਬਹੁਲਕ_Check = ਸੱਚ;
        m_var_ਸੌਂਪੋ_ਪ੍ਰਾਪਤੀ = ਝੂਠ;
        m_fork_ਪ੍ਰਾਪਤੀ = ਝੂਠ;
        m_ignoreIncp = incp;
        iterate_And_ਅਗਲਾ_Null(bodysp);
        iterate_And_ਅਗਲਾ_Null(incp);
        m_var_ਬਹੁਲਕ_Check = ਝੂਠ;
        m_ignoreIncp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (m_var_ਸੌਂਪੋ_ਪ੍ਰਾਪਤੀ) ਵਾਪਸ cantUnroll(nodep, "genvar assigned *inside* loop");

        ਜੇ (m_fork_ਪ੍ਰਾਪਤੀ) ਵਾਪਸ cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        ਜੇ (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            ਵਾਪਸ cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ simulate_ਰੁੱਖ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਅਚਲ V3_ਸੰਖਿਆ* loop_ਮੁੱਲ, ਅਚਲ Ast_ਗੰਢ* dtypep,
                      V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ_ਅੰਕ) {
        Ast_ਗੰਢ* clonep = nodep->clone_ਰੁੱਖ(ਸੱਚ);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        ਜੇ (loop_ਮੁੱਲ) {
            AstConst* varValuep = ਨਵਾਂ AstConst{nodep->fileline(), *loop_ਮੁੱਲ};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = ਨਵਾਂ AstBegin{nodep->fileline(), "[EditWrapper]", clonep, ਝੂਠ};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            VL_DO_CLEAR(tempp->delete_ਰੁੱਖ(), tempp = ਖਾਲੀ_ਸੂਚਕ);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        ਜੇ (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_ਰੁੱਖ(), clonep);
            ਵਾਪਸ ਝੂਠ;
        }
        // Fetch the result
        ਅਚਲ V3_ਸੰਖਿਆ* resp = simvis.fetch_ਸੰਖਿਆ_Null(clonep);
        ਜੇ (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_ਰੁੱਖ(), clonep);
            ਵਾਪਸ ਝੂਠ;
        }
        // Patch up datatype
        ਜੇ (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            ਬਾਹਰਲਾ_ਅੰਕ = new_con.ਅੰਕ();
            ਬਾਹਰਲਾ_ਅੰਕ.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_ਰੁੱਖ(), clonep);
            ਵਾਪਸ ਸੱਚ;
        }
        ਬਾਹਰਲਾ_ਅੰਕ = *resp;
        VL_DO_DANGLING(clonep->delete_ਰੁੱਖ(), clonep);
        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ forUnroller(AstGenFor* nodep, Ast_ਸੌਂਪੋ* initp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ* incp,
                     Ast_ਗੰਢ* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_ਸੰਖਿਆ loop_ਮੁੱਲ{nodep};
        ਜੇ (!simulate_ਰੁੱਖ(initp->rhsp(), ਖਾਲੀ_ਸੂਚਕ, initp, loop_ਮੁੱਲ)) {  //
            ਵਾਪਸ ਝੂਠ;
        }
        Ast_ਗੰਢ* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (initp) {
            initp->unlink_Fr_ਪਿੱਠ();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ਧੱਕੋ_Deletep(initp);  // Always cloned below.
        }
        ਜੇ (bodysp) {
            bodysp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->ਵਰਤਿਆ_Loop_Idx(ਸੱਚ);

        Ast_ਗੰਢ* newbodysp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (stmtsp) {
            ਧੱਕੋ_Deletep(stmtsp);  // Always cloned below.
            ਮਾਪ_t iter_ਗਿਣਤੀ = 0;
            ਅਚਲ ਮਾਪ_t iter_ਹੱਦਬੰਦੀ = v3Global.opt.unroll_ਹੱਦਬੰਦੀ();
            ਜਦੋਂ (ਸੱਚ) {
                UINFO(8, "      Looping " << loop_ਮੁੱਲ);
                V3_ਸੰਖਿਆ res{nodep};
                ਜੇ (!simulate_ਰੁੱਖ(condp, &loop_ਮੁੱਲ, ਖਾਲੀ_ਸੂਚਕ, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    ਵਾਪਸ ਝੂਠ;
                }
                ਜੇ (!res.isEqOne()) {
                    ਰੋਕੋ;  // Done with the loop
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (++iter_ਗਿਣਤੀ > iter_ਹੱਦਬੰਦੀ) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iter_ਹੱਦਬੰਦੀ);
                        ਰੋਕੋ;
                    }

                    // Replace iterator values with constant
                    Ast_ਗੰਢ* oneloopp = stmtsp->clone_ਰੁੱਖ(ਸੱਚ);
                    AstConst* varValuep = ਨਵਾਂ AstConst{nodep->fileline(), loop_ਮੁੱਲ};
                    ਜੇ (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ਅਚਲ tempp
                            = ਨਵਾਂ AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, ਝੂਠ};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                        VL_DO_DANGLING(tempp->delete_ਰੁੱਖ(), tempp);
                    }
                    ਅਚਲ string ਸੂਚਕਾਂਕ = Ast_ਗੰਢ::encode_ਸੰਖਿਆ(varValuep->ਮੰਜ਼ਿਲ_SInt());
                    ਅਚਲ string nname = m_beginName + "__BRA__" + ਸੂਚਕਾਂਕ + "__KET__";
                    oneloopp = ਨਵਾਂ Ast_Gen_ਖੰਡ{oneloopp->fileline(), nname, oneloopp, ਝੂਠ};
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(varValuep), varValuep);
                    newbodysp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(newbodysp, oneloopp);

                    // loopValue += valInc
                    ਅਚਲ Ast_ਸੌਂਪੋ* ਅਚਲ incpass = VN_AS(incp, ਸੌਂਪੋ);
                    V3_ਸੰਖਿਆ new_Loop_ਮੁੱਲ{nodep};
                    ਜੇ (!simulate_ਰੁੱਖ(incpass->rhsp(), &loop_ਮੁੱਲ, incpass, new_Loop_ਮੁੱਲ)) {
                        nodep->v3error("Loop unrolling failed");
                        ਵਾਪਸ ਝੂਠ;
                    }
                    loop_ਮੁੱਲ.op_ਸੌਂਪੋ(new_Loop_ਮੁੱਲ);
                }
            }
        }
        ਜੇ (!newbodysp) {  // initp might have effects after the loop
            ਜੇ (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = ਨਵਾਂ AstInitial{initp->fileline(), initp->clone_ਰੁੱਖ(ਸੱਚ)};
            } ਨਹੀਂਤਾਂ {
                newbodysp = initp ? initp->clone_ਰੁੱਖ(ਸੱਚ) : ਖਾਲੀ_ਸੂਚਕ;
            }
        }
        // Replace the FOR()
        ਜੇ (newbodysp) {
            nodep->replaceWith(newbodysp);
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        ਜੇ (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        ਵਾਪਸ ਸੱਚ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (m_var_ਬਹੁਲਕ_Check && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_var_ਸੌਂਪੋ_ਪ੍ਰਾਪਤੀ = ਸੱਚ;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (m_var_ਬਹੁਲਕ_Check && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    UnrollGenVisitor() = ਮੂਲ;

    ਖਾਲੀ unroll(AstGenFor* nodep, ਅਚਲ std::string& beginName) {
        m_forVarp = ਖਾਲੀ_ਸੂਚਕ;  // Iterator variable
        m_ignoreIncp = ਖਾਲੀ_ਸੂਚਕ;  // Increment node to ignore
        m_var_ਬਹੁਲਕ_Check = ਝੂਠ;  // Just checking RHS assignments
        m_var_ਸੌਂਪੋ_ਪ੍ਰਾਪਤੀ = ਝੂਠ;  // Assign var hit
        m_fork_ਪ੍ਰਾਪਤੀ = ਝੂਠ;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        ਜੇ (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        ਜੇ (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        ਜੇ (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        ਜੇ (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);  // Did replacement
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{ਨਵਾਂ UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(ਮਿਟਾਓ m_unrollerp, m_unrollerp); }
ਖਾਲੀ GenForUnroller::unroll(AstGenFor* nodep, ਅਚਲ std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
