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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Simulate.h"
#અન્તર્ભાવ "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

વર્ગ UnrollGenVisitor final : જાહેર VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = nullptr;  // Iterator variable
    અચલ Ast_ગાંઠ* m_ignoreIncp = nullptr;  // Increment node to ignore
    તાર્કિક m_varModeCheck = ખોટું;  // Just checking RHS assignments
    તાર્કિક m_varAssignHit = ખોટું;  // Assign var hit
    તાર્કિક m_forkHit = ખોટું;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    શૂન્ય replaceVarRef(Ast_ગાંઠ* bodyp, Ast_ગાંઠ* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([આ, varValuep](AstVarRef* refp) {
            જો (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_ગાંઠ* અચલ newconstp = varValuep->clone_વૃક્ષ(ખોટું);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
            }
        });
    }

    તાર્કિક cantUnroll(Ast_ગાંઠ* nodep, અચલ અક્ષર* reason) અચલ {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        પાછા ખોટું;
    }

    તાર્કિક forUnrollCheck(
        AstGenFor* અચલ nodep,
        Ast_ગાંઠ* અચલ initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_ગાંઠ_પદાવલિ* condp,
        Ast_ગાંઠ* અચલ incp,  // Maybe under nodep or in bodysp
        Ast_ગાંઠ* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        જો (initp) UINFO(6, "    Init " << initp);
        જો (condp) UINFO(6, "    Cond " << condp);
        જો (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* અચલ initAssp = VN_CAST(initp, Assign);
        જો (!initAssp) પાછા cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        જો (!VN_IS(initAssp->lhsp(), VarRef)) {
            પાછા cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        અચલ AstAssign* અચલ incAssp = VN_CAST(incp, Assign);
        જો (!incAssp) પાછા cantUnroll(nodep, "no increment assignment");
        જો (incAssp->nextp()) પાછા cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        જો (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        અચલ AstConst* અચલ constInitp = VN_CAST(initAssp->rhsp(), Const);
        જો (!constInitp) પાછા cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = સાચું;
        m_varAssignHit = ખોટું;
        m_forkHit = ખોટું;
        m_ignoreIncp = incp;
        iterateAndNextNull(bodysp);
        iterateAndNextNull(incp);
        m_varModeCheck = ખોટું;
        m_ignoreIncp = nullptr;
        જો (m_varAssignHit) પાછા cantUnroll(nodep, "genvar assigned *inside* loop");

        જો (m_forkHit) પાછા cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        જો (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            પાછા cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        પાછા સાચું;
    }

    તાર્કિક simulate_વૃક્ષ(Ast_ગાંઠ_પદાવલિ* nodep, અચલ V3_સંખ્યા* loop_કિંમત, અચલ Ast_ગાંઠ* dtypep,
                      V3_સંખ્યા& out_અંક) {
        Ast_ગાંઠ* clonep = nodep->clone_વૃક્ષ(સાચું);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        જો (loop_કિંમત) {
            AstConst* varValuep = નવું AstConst{nodep->fileline(), *loop_કિંમત};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = નવું AstBegin{nodep->fileline(), "[EditWrapper]", clonep, ખોટું};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->delete_વૃક્ષ(), tempp = nullptr);
            VL_DO_DANGLING(ધકેલો_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        જો (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_વૃક્ષ(), clonep);
            પાછા ખોટું;
        }
        // Fetch the result
        અચલ V3_સંખ્યા* resp = simvis.fetch_સંખ્યા_Null(clonep);
        જો (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_વૃક્ષ(), clonep);
            પાછા ખોટું;
        }
        // Patch up datatype
        જો (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_અંક = new_con.અંક();
            out_અંક.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_વૃક્ષ(), clonep);
            પાછા સાચું;
        }
        out_અંક = *resp;
        VL_DO_DANGLING(clonep->delete_વૃક્ષ(), clonep);
        પાછા સાચું;
    }

    તાર્કિક forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ* incp,
                     Ast_ગાંઠ* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_સંખ્યા loop_કિંમત{nodep};
        જો (!simulate_વૃક્ષ(initp->rhsp(), nullptr, initp, loop_કિંમત)) {  //
            પાછા ખોટું;
        }
        Ast_ગાંઠ* stmtsp = nullptr;
        જો (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ધકેલો_Deletep(initp);  // Always cloned below.
        }
        જો (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = Ast_ગાંઠ::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(સાચું);

        Ast_ગાંઠ* newbodysp = nullptr;
        જો (stmtsp) {
            ધકેલો_Deletep(stmtsp);  // Always cloned below.
            size_t iter_ગણતરી = 0;
            અચલ size_t iterLimit = v3Global.opt.unrollLimit();
            જ્યારે (સાચું) {
                UINFO(8, "      Looping " << loop_કિંમત);
                V3_સંખ્યા res{nodep};
                જો (!simulate_વૃક્ષ(condp, &loop_કિંમત, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    પાછા ખોટું;
                }
                જો (!res.isEqOne()) {
                    તોડો;  // Done with the loop
                } નહીંતર {
                    જો (++iter_ગણતરી > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        તોડો;
                    }

                    // Replace iterator values with constant
                    Ast_ગાંઠ* oneloopp = stmtsp->clone_વૃક્ષ(સાચું);
                    AstConst* varValuep = નવું AstConst{nodep->fileline(), loop_કિંમત};
                    જો (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* અચલ tempp
                            = નવું AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, ખોટું};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->delete_વૃક્ષ(), tempp);
                    }
                    અચલ string index = Ast_ગાંઠ::encode_સંખ્યા(varValuep->toSInt());
                    અચલ string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = નવું AstGenBlock{oneloopp->fileline(), nname, oneloopp, ખોટું};
                    VL_DO_DANGLING(ધકેલો_Deletep(varValuep), varValuep);
                    newbodysp = Ast_ગાંઠ::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    અચલ AstAssign* અચલ incpass = VN_AS(incp, Assign);
                    V3_સંખ્યા new_Loop_કિંમત{nodep};
                    જો (!simulate_વૃક્ષ(incpass->rhsp(), &loop_કિંમત, incpass, new_Loop_કિંમત)) {
                        nodep->v3error("Loop unrolling failed");
                        પાછા ખોટું;
                    }
                    loop_કિંમત.opAssign(new_Loop_કિંમત);
                }
            }
        }
        જો (!newbodysp) {  // initp might have effects after the loop
            જો (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = નવું AstInitial{initp->fileline(), initp->clone_વૃક્ષ(સાચું)};
            } નહીંતર {
                newbodysp = initp ? initp->clone_વૃક્ષ(સાચું) : nullptr;
            }
        }
        // Replace the FOR()
        જો (newbodysp) {
            nodep->replaceWith(newbodysp);
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        જો (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        પાછા સાચું;
    }

    // VISITORS
    શૂન્ય visit(AstVarRef* nodep) override {
        જો (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = સાચું;
        }
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } નહીંતર {
            iterateChildren(nodep);
        }
    }

જાહેર:
    // CONSTRUCTORS
    UnrollGenVisitor() = મૂળભૂત;

    શૂન્ય unroll(AstGenFor* nodep, અચલ std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = ખોટું;  // Just checking RHS assignments
        m_varAssignHit = ખોટું;  // Assign var hit
        m_forkHit = ખોટું;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        જો (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        જો (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        જો (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        જો (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        } નહીંતર જો (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);  // Did replacement
        } નહીંતર {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{નવું UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
શૂન્ય GenForUnroller::unroll(AstGenFor* nodep, અચલ std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
