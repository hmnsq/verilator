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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Const.h"
#समावेश "V3Simulate.h"
#समावेश "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

श्रेणी UnrollGenVisitor final : खुला VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = nullptr;  // Iterator variable
    स्थिर Ast_गाँठ* m_ignoreIncp = nullptr;  // Increment node to ignore
    bool m_varModeCheck = false;  // Just checking RHS assignments
    bool m_varAssignHit = false;  // Assign var hit
    bool m_forkHit = false;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    व्योम replaceVarRef(Ast_गाँठ* bodyp, Ast_गाँठ* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreach_And_अग्र([यह, varValuep](AstVarRef* refp) {
            अगर (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_गाँठ* स्थिर newconstp = varValuep->clone_वृक्ष(false);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
            }
        });
    }

    bool cantUnroll(Ast_गाँठ* nodep, स्थिर अक्षर* reason) स्थिर {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        वापस false;
    }

    bool forUnrollCheck(
        AstGenFor* स्थिर nodep,
        Ast_गाँठ* स्थिर initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_गाँठ_व्यंजक* condp,
        Ast_गाँठ* स्थिर incp,  // Maybe under nodep or in bodysp
        Ast_गाँठ* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        अगर (initp) UINFO(6, "    Init " << initp);
        अगर (condp) UINFO(6, "    Cond " << condp);
        अगर (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* स्थिर initAssp = VN_CAST(initp, Assign);
        अगर (!initAssp) वापस cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        अगर (!VN_IS(initAssp->lhsp(), VarRef)) {
            वापस cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        स्थिर AstAssign* स्थिर incAssp = VN_CAST(incp, Assign);
        अगर (!incAssp) वापस cantUnroll(nodep, "no increment assignment");
        अगर (incAssp->nextp()) वापस cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        अगर (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        स्थिर AstConst* स्थिर constInitp = VN_CAST(initAssp->rhsp(), Const);
        अगर (!constInitp) वापस cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = true;
        m_varAssignHit = false;
        m_forkHit = false;
        m_ignoreIncp = incp;
        iterate_And_अग्र_Null(bodysp);
        iterate_And_अग्र_Null(incp);
        m_varModeCheck = false;
        m_ignoreIncp = nullptr;
        अगर (m_varAssignHit) वापस cantUnroll(nodep, "genvar assigned *inside* loop");

        अगर (m_forkHit) वापस cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        अगर (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            वापस cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        वापस true;
    }

    bool simulate_वृक्ष(Ast_गाँठ_व्यंजक* nodep, स्थिर V3_संख्या* loop_मूल्य, स्थिर Ast_गाँठ* dtypep,
                      V3_संख्या& out_अंक) {
        Ast_गाँठ* clonep = nodep->clone_वृक्ष(true);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        अगर (loop_मूल्य) {
            AstConst* varValuep = नया AstConst{nodep->fileline(), *loop_मूल्य};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = नया AstBegin{nodep->fileline(), "[EditWrapper]", clonep, false};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlink_Fr_Back_With_अग्र();
            VL_DO_CLEAR(tempp->delete_वृक्ष(), tempp = nullptr);
            VL_DO_DANGLING(धकेलो_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        अगर (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
            वापस false;
        }
        // Fetch the result
        स्थिर V3_संख्या* resp = simvis.fetch_संख्या_Null(clonep);
        अगर (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
            वापस false;
        }
        // Patch up datatype
        अगर (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_अंक = new_con.अंक();
            out_अंक.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
            वापस true;
        }
        out_अंक = *resp;
        VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
        वापस true;
    }

    bool forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ* incp,
                     Ast_गाँठ* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_संख्या loop_मूल्य{nodep};
        अगर (!simulate_वृक्ष(initp->rhsp(), nullptr, initp, loop_मूल्य)) {  //
            वापस false;
        }
        Ast_गाँठ* stmtsp = nullptr;
        अगर (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            धकेलो_Deletep(initp);  // Always cloned below.
        }
        अगर (bodysp) {
            bodysp->unlink_Fr_Back_With_अग्र();
            stmtsp = Ast_गाँठ::add_अग्र(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(true);

        Ast_गाँठ* newbodysp = nullptr;
        अगर (stmtsp) {
            धकेलो_Deletep(stmtsp);  // Always cloned below.
            माप_प्रकार iter_गिनती = 0;
            स्थिर माप_प्रकार iterLimit = v3Global.opt.unrollLimit();
            जबतक (true) {
                UINFO(8, "      Looping " << loop_मूल्य);
                V3_संख्या res{nodep};
                अगर (!simulate_वृक्ष(condp, &loop_मूल्य, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    वापस false;
                }
                अगर (!res.isEqOne()) {
                    अवरोध;  // Done with the loop
                } अन्यथा {
                    अगर (++iter_गिनती > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        अवरोध;
                    }

                    // Replace iterator values with constant
                    Ast_गाँठ* oneloopp = stmtsp->clone_वृक्ष(true);
                    AstConst* varValuep = नया AstConst{nodep->fileline(), loop_मूल्य};
                    अगर (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* स्थिर tempp
                            = नया AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, false};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlink_Fr_Back_With_अग्र();
                        VL_DO_DANGLING(tempp->delete_वृक्ष(), tempp);
                    }
                    स्थिर string सूचकांक_2 = Ast_गाँठ::encode_संख्या(varValuep->toSInt());
                    स्थिर string nname = m_beginName + "__BRA__" + सूचकांक_2 + "__KET__";
                    oneloopp = नया AstGenBlock{oneloopp->fileline(), nname, oneloopp, false};
                    VL_DO_DANGLING(धकेलो_Deletep(varValuep), varValuep);
                    newbodysp = Ast_गाँठ::add_अग्र_Null(newbodysp, oneloopp);

                    // loopValue += valInc
                    स्थिर AstAssign* स्थिर incpass = VN_AS(incp, Assign);
                    V3_संख्या new_Loop_मूल्य{nodep};
                    अगर (!simulate_वृक्ष(incpass->rhsp(), &loop_मूल्य, incpass, new_Loop_मूल्य)) {
                        nodep->v3error("Loop unrolling failed");
                        वापस false;
                    }
                    loop_मूल्य.opAssign(new_Loop_मूल्य);
                }
            }
        }
        अगर (!newbodysp) {  // initp might have effects after the loop
            अगर (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = नया AstInitial{initp->fileline(), initp->clone_वृक्ष(true)};
            } अन्यथा {
                newbodysp = initp ? initp->clone_वृक्ष(true) : nullptr;
            }
        }
        // Replace the FOR()
        अगर (newbodysp) {
            nodep->replaceWith(newbodysp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        अगर (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        वापस true;
    }

    // VISITORS
    व्योम visit(AstVarRef* nodep) override {
        अगर (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = true;
        }
    }

    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

खुला:
    // CONSTRUCTORS
    UnrollGenVisitor() = शेष;

    व्योम unroll(AstGenFor* nodep, स्थिर std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = false;  // Just checking RHS assignments
        m_varAssignHit = false;  // Assign var hit
        m_forkHit = false;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        अगर (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        अगर (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        अगर (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        अगर (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा अगर (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);  // Did replacement
        } अन्यथा {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{नया UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(मिटाओ m_unrollerp, m_unrollerp); }
व्योम GenForUnroller::unroll(AstGenFor* nodep, स्थिर std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
