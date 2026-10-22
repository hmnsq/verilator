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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Simulate.h"
#अन्तर्भाव "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

वर्ग UnrollGenVisitor final : सार्वजनिक VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = nullptr;  // Iterator variable
    स्थिरांक Ast_गाठ* m_ignoreIncp = nullptr;  // Increment node to ignore
    तार्किक m_varModeCheck = असत्य;  // Just checking RHS assignments
    तार्किक m_varAssignHit = असत्य;  // Assign var hit
    तार्किक m_forkHit = असत्य;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    शून्य replaceVarRef(Ast_गाठ* bodyp, Ast_गाठ* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([हे, varValuep](AstVarRef* refp) {
            जर (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_गाठ* स्थिरांक newconstp = varValuep->clone_वृक्ष(असत्य);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ढकला_Deletep(refp), refp);
            }
        });
    }

    तार्किक cantUnroll(Ast_गाठ* nodep, स्थिरांक अक्षर* reason) स्थिरांक {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        परत असत्य;
    }

    तार्किक forUnrollCheck(
        AstGenFor* स्थिरांक nodep,
        Ast_गाठ* स्थिरांक initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_गाठ_अभिव्यक्ती* condp,
        Ast_गाठ* स्थिरांक incp,  // Maybe under nodep or in bodysp
        Ast_गाठ* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        जर (initp) UINFO(6, "    Init " << initp);
        जर (condp) UINFO(6, "    Cond " << condp);
        जर (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* स्थिरांक initAssp = VN_CAST(initp, Assign);
        जर (!initAssp) परत cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        जर (!VN_IS(initAssp->lhsp(), VarRef)) {
            परत cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        स्थिरांक AstAssign* स्थिरांक incAssp = VN_CAST(incp, Assign);
        जर (!incAssp) परत cantUnroll(nodep, "no increment assignment");
        जर (incAssp->nextp()) परत cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        जर (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        स्थिरांक AstConst* स्थिरांक constInitp = VN_CAST(initAssp->rhsp(), Const);
        जर (!constInitp) परत cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = सत्य;
        m_varAssignHit = असत्य;
        m_forkHit = असत्य;
        m_ignoreIncp = incp;
        iterateAndNextNull(bodysp);
        iterateAndNextNull(incp);
        m_varModeCheck = असत्य;
        m_ignoreIncp = nullptr;
        जर (m_varAssignHit) परत cantUnroll(nodep, "genvar assigned *inside* loop");

        जर (m_forkHit) परत cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        जर (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            परत cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        परत सत्य;
    }

    तार्किक simulate_वृक्ष(Ast_गाठ_अभिव्यक्ती* nodep, स्थिरांक V3_संख्या* loop_मूल्य, स्थिरांक Ast_गाठ* dtypep,
                      V3_संख्या& out_अंक) {
        Ast_गाठ* clonep = nodep->clone_वृक्ष(सत्य);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        जर (loop_मूल्य) {
            AstConst* varValuep = नवीन AstConst{nodep->fileline(), *loop_मूल्य};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = नवीन AstBegin{nodep->fileline(), "[EditWrapper]", clonep, असत्य};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->delete_वृक्ष(), tempp = nullptr);
            VL_DO_DANGLING(ढकला_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        जर (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
            परत असत्य;
        }
        // Fetch the result
        स्थिरांक V3_संख्या* resp = simvis.fetch_संख्या_Null(clonep);
        जर (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
            परत असत्य;
        }
        // Patch up datatype
        जर (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_अंक = new_con.अंक();
            out_अंक.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
            परत सत्य;
        }
        out_अंक = *resp;
        VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
        परत सत्य;
    }

    तार्किक forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_गाठ_अभिव्यक्ती* condp, Ast_गाठ* incp,
                     Ast_गाठ* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_संख्या loop_मूल्य{nodep};
        जर (!simulate_वृक्ष(initp->rhsp(), nullptr, initp, loop_मूल्य)) {  //
            परत असत्य;
        }
        Ast_गाठ* stmtsp = nullptr;
        जर (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ढकला_Deletep(initp);  // Always cloned below.
        }
        जर (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = Ast_गाठ::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(सत्य);

        Ast_गाठ* newbodysp = nullptr;
        जर (stmtsp) {
            ढकला_Deletep(stmtsp);  // Always cloned below.
            माप_प्रकार iter_मोजणी = 0;
            स्थिरांक माप_प्रकार iterLimit = v3Global.opt.unrollLimit();
            पर्यंत (सत्य) {
                UINFO(8, "      Looping " << loop_मूल्य);
                V3_संख्या res{nodep};
                जर (!simulate_वृक्ष(condp, &loop_मूल्य, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    परत असत्य;
                }
                जर (!res.isEqOne()) {
                    थांबा;  // Done with the loop
                } अन्यथा {
                    जर (++iter_मोजणी > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        थांबा;
                    }

                    // Replace iterator values with constant
                    Ast_गाठ* oneloopp = stmtsp->clone_वृक्ष(सत्य);
                    AstConst* varValuep = नवीन AstConst{nodep->fileline(), loop_मूल्य};
                    जर (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* स्थिरांक tempp
                            = नवीन AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, असत्य};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->delete_वृक्ष(), tempp);
                    }
                    स्थिरांक string index = Ast_गाठ::encode_संख्या(varValuep->toSInt());
                    स्थिरांक string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = नवीन AstGenBlock{oneloopp->fileline(), nname, oneloopp, असत्य};
                    VL_DO_DANGLING(ढकला_Deletep(varValuep), varValuep);
                    newbodysp = Ast_गाठ::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    स्थिरांक AstAssign* स्थिरांक incpass = VN_AS(incp, Assign);
                    V3_संख्या new_Loop_मूल्य{nodep};
                    जर (!simulate_वृक्ष(incpass->rhsp(), &loop_मूल्य, incpass, new_Loop_मूल्य)) {
                        nodep->v3error("Loop unrolling failed");
                        परत असत्य;
                    }
                    loop_मूल्य.opAssign(new_Loop_मूल्य);
                }
            }
        }
        जर (!newbodysp) {  // initp might have effects after the loop
            जर (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = नवीन AstInitial{initp->fileline(), initp->clone_वृक्ष(सत्य)};
            } अन्यथा {
                newbodysp = initp ? initp->clone_वृक्ष(सत्य) : nullptr;
            }
        }
        // Replace the FOR()
        जर (newbodysp) {
            nodep->replaceWith(newbodysp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        जर (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        परत सत्य;
    }

    // VISITORS
    शून्य visit(AstVarRef* nodep) override {
        जर (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = सत्य;
        }
    }

    शून्य visit(Ast_गाठ* nodep) override {
        जर (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    UnrollGenVisitor() = पूर्वनिर्धारित;

    शून्य unroll(AstGenFor* nodep, स्थिरांक std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = असत्य;  // Just checking RHS assignments
        m_varAssignHit = असत्य;  // Assign var hit
        m_forkHit = असत्य;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        जर (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        जर (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        जर (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        जर (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा जर (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);  // Did replacement
        } अन्यथा {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{नवीन UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
शून्य GenForUnroller::unroll(AstGenFor* nodep, स्थिरांक std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
