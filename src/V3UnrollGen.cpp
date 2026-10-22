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
    ध्रुव Ast_गाँठ* m_ignoreIncp = nullptr;  // Increment node to ignore
    तार्किक m_varModeCheck = असत्य;  // Just checking RHS assignments
    तार्किक m_varAssignHit = असत्य;  // Assign var hit
    तार्किक m_forkHit = असत्य;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    शून्य replaceVarRef(Ast_गाँठ* bodyp, Ast_गाँठ* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([एतत्, varValuep](AstVarRef* refp) {
            यदि (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_गाँठ* ध्रुव newconstp = varValuep->clone_वृक्ष(असत्य);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
            }
        });
    }

    तार्किक cantUnroll(Ast_गाँठ* nodep, ध्रुव वर्ण* reason) ध्रुव {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        प्रत्यागम असत्य;
    }

    तार्किक forUnrollCheck(
        AstGenFor* ध्रुव nodep,
        Ast_गाँठ* ध्रुव initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_गाँठ_व्यंजक* condp,
        Ast_गाँठ* ध्रुव incp,  // Maybe under nodep or in bodysp
        Ast_गाँठ* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        यदि (initp) UINFO(6, "    Init " << initp);
        यदि (condp) UINFO(6, "    Cond " << condp);
        यदि (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* ध्रुव initAssp = VN_CAST(initp, Assign);
        यदि (!initAssp) प्रत्यागम cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        यदि (!VN_IS(initAssp->lhsp(), VarRef)) {
            प्रत्यागम cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ध्रुव AstAssign* ध्रुव incAssp = VN_CAST(incp, Assign);
        यदि (!incAssp) प्रत्यागम cantUnroll(nodep, "no increment assignment");
        यदि (incAssp->nextp()) प्रत्यागम cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        यदि (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        ध्रुव AstConst* ध्रुव constInitp = VN_CAST(initAssp->rhsp(), Const);
        यदि (!constInitp) प्रत्यागम cantUnroll(nodep, "non-constant initializer");

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
        यदि (m_varAssignHit) प्रत्यागम cantUnroll(nodep, "genvar assigned *inside* loop");

        यदि (m_forkHit) प्रत्यागम cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        यदि (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            प्रत्यागम cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        प्रत्यागम सत्य;
    }

    तार्किक simulate_वृक्ष(Ast_गाँठ_व्यंजक* nodep, ध्रुव V3_संख्या* loop_मूल्य, ध्रुव Ast_गाँठ* dtypep,
                      V3_संख्या& out_अंक) {
        Ast_गाँठ* clonep = nodep->clone_वृक्ष(सत्य);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        यदि (loop_मूल्य) {
            AstConst* varValuep = नूतन AstConst{nodep->fileline(), *loop_मूल्य};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = नूतन AstBegin{nodep->fileline(), "[EditWrapper]", clonep, असत्य};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->delete_वृक्ष(), tempp = nullptr);
            VL_DO_DANGLING(धकेलो_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        यदि (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
            प्रत्यागम असत्य;
        }
        // Fetch the result
        ध्रुव V3_संख्या* resp = simvis.fetch_संख्या_Null(clonep);
        यदि (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
            प्रत्यागम असत्य;
        }
        // Patch up datatype
        यदि (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_अंक = new_con.अंक();
            out_अंक.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
            प्रत्यागम सत्य;
        }
        out_अंक = *resp;
        VL_DO_DANGLING(clonep->delete_वृक्ष(), clonep);
        प्रत्यागम सत्य;
    }

    तार्किक forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ* incp,
                     Ast_गाँठ* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_संख्या loop_मूल्य{nodep};
        यदि (!simulate_वृक्ष(initp->rhsp(), nullptr, initp, loop_मूल्य)) {  //
            प्रत्यागम असत्य;
        }
        Ast_गाँठ* stmtsp = nullptr;
        यदि (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            धकेलो_Deletep(initp);  // Always cloned below.
        }
        यदि (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = Ast_गाँठ::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(सत्य);

        Ast_गाँठ* newbodysp = nullptr;
        यदि (stmtsp) {
            धकेलो_Deletep(stmtsp);  // Always cloned below.
            माप_प्रकार iter_गिनती = 0;
            ध्रुव माप_प्रकार iterLimit = v3Global.opt.unrollLimit();
            यावत् (सत्य) {
                UINFO(8, "      Looping " << loop_मूल्य);
                V3_संख्या res{nodep};
                यदि (!simulate_वृक्ष(condp, &loop_मूल्य, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    प्रत्यागम असत्य;
                }
                यदि (!res.isEqOne()) {
                    भङ्ग;  // Done with the loop
                } अन्यथा {
                    यदि (++iter_गिनती > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        भङ्ग;
                    }

                    // Replace iterator values with constant
                    Ast_गाँठ* oneloopp = stmtsp->clone_वृक्ष(सत्य);
                    AstConst* varValuep = नूतन AstConst{nodep->fileline(), loop_मूल्य};
                    यदि (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ध्रुव tempp
                            = नूतन AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, असत्य};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->delete_वृक्ष(), tempp);
                    }
                    ध्रुव string index = Ast_गाँठ::encode_संख्या(varValuep->toSInt());
                    ध्रुव string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = नूतन AstGenBlock{oneloopp->fileline(), nname, oneloopp, असत्य};
                    VL_DO_DANGLING(धकेलो_Deletep(varValuep), varValuep);
                    newbodysp = Ast_गाँठ::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    ध्रुव AstAssign* ध्रुव incpass = VN_AS(incp, Assign);
                    V3_संख्या new_Loop_मूल्य{nodep};
                    यदि (!simulate_वृक्ष(incpass->rhsp(), &loop_मूल्य, incpass, new_Loop_मूल्य)) {
                        nodep->v3error("Loop unrolling failed");
                        प्रत्यागम असत्य;
                    }
                    loop_मूल्य.opAssign(new_Loop_मूल्य);
                }
            }
        }
        यदि (!newbodysp) {  // initp might have effects after the loop
            यदि (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = नूतन AstInitial{initp->fileline(), initp->clone_वृक्ष(सत्य)};
            } अन्यथा {
                newbodysp = initp ? initp->clone_वृक्ष(सत्य) : nullptr;
            }
        }
        // Replace the FOR()
        यदि (newbodysp) {
            nodep->replaceWith(newbodysp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        यदि (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        प्रत्यागम सत्य;
    }

    // VISITORS
    शून्य visit(AstVarRef* nodep) override {
        यदि (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = सत्य;
        }
    }

    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    UnrollGenVisitor() = स्वभाव;

    शून्य unroll(AstGenFor* nodep, ध्रुव std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = असत्य;  // Just checking RHS assignments
        m_varAssignHit = असत्य;  // Assign var hit
        m_forkHit = असत्य;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        यदि (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        यदि (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        यदि (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        यदि (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा यदि (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
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
    : m_unrollerp{नूतन UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
शून्य GenForUnroller::unroll(AstGenFor* nodep, ध्रुव std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
