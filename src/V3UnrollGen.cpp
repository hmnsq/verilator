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
    ध्रुव Ast_ग्रन्थिः* m_ignoreIncp = nullptr;  // Increment node to ignore
    तार्किक m_varModeCheck = मिच्छ;  // Just checking RHS assignments
    तार्किक m_varAssignHit = मिच्छ;  // Assign var hit
    तार्किक m_forkHit = मिच्छ;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    शून्य replaceVarRef(Ast_ग्रन्थिः* bodyp, Ast_ग्रन्थिः* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([एतत्, varValuep](AstVarRef* refp) {
            जइ (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_ग्रन्थिः* ध्रुव newconstp = varValuep->clone_वृक्षः(मिच्छ);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
            }
        });
    }

    तार्किक cantUnroll(Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* reason) ध्रुव {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        प्रत्यागम मिच्छ;
    }

    तार्किक forUnrollCheck(
        AstGenFor* ध्रुव nodep,
        Ast_ग्रन्थिः* ध्रुव initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_ग्रन्थिः_व्यञ्जकम्* condp,
        Ast_ग्रन्थिः* ध्रुव incp,  // Maybe under nodep or in bodysp
        Ast_ग्रन्थिः* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        जइ (initp) UINFO(6, "    Init " << initp);
        जइ (condp) UINFO(6, "    Cond " << condp);
        जइ (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* ध्रुव initAssp = VN_CAST(initp, Assign);
        जइ (!initAssp) प्रत्यागम cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        जइ (!VN_IS(initAssp->lhsp(), VarRef)) {
            प्रत्यागम cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ध्रुव AstAssign* ध्रुव incAssp = VN_CAST(incp, Assign);
        जइ (!incAssp) प्रत्यागम cantUnroll(nodep, "no increment assignment");
        जइ (incAssp->nextp()) प्रत्यागम cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        जइ (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        ध्रुव AstConst* ध्रुव constInitp = VN_CAST(initAssp->rhsp(), Const);
        जइ (!constInitp) प्रत्यागम cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = सच्च;
        m_varAssignHit = मिच्छ;
        m_forkHit = मिच्छ;
        m_ignoreIncp = incp;
        iterateAndNextNull(bodysp);
        iterateAndNextNull(incp);
        m_varModeCheck = मिच्छ;
        m_ignoreIncp = nullptr;
        जइ (m_varAssignHit) प्रत्यागम cantUnroll(nodep, "genvar assigned *inside* loop");

        जइ (m_forkHit) प्रत्यागम cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        जइ (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            प्रत्यागम cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        प्रत्यागम सच्च;
    }

    तार्किक simulate_वृक्षः(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, ध्रुव V3_सङ्ख्या* loop_मानम्, ध्रुव Ast_ग्रन्थिः* dtypep,
                      V3_सङ्ख्या& out_अङ्कः) {
        Ast_ग्रन्थिः* clonep = nodep->clone_वृक्षः(सच्च);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        जइ (loop_मानम्) {
            AstConst* varValuep = नूतन AstConst{nodep->fileline(), *loop_मानम्};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = नूतन AstBegin{nodep->fileline(), "[EditWrapper]", clonep, मिच्छ};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->delete_वृक्षः(), tempp = nullptr);
            VL_DO_DANGLING(प्रक्षिप_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        जइ (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_वृक्षः(), clonep);
            प्रत्यागम मिच्छ;
        }
        // Fetch the result
        ध्रुव V3_सङ्ख्या* resp = simvis.fetch_सङ्ख्या_Null(clonep);
        जइ (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_वृक्षः(), clonep);
            प्रत्यागम मिच्छ;
        }
        // Patch up datatype
        जइ (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_अङ्कः = new_con.अङ्कः();
            out_अङ्कः.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_वृक्षः(), clonep);
            प्रत्यागम सच्च;
        }
        out_अङ्कः = *resp;
        VL_DO_DANGLING(clonep->delete_वृक्षः(), clonep);
        प्रत्यागम सच्च;
    }

    तार्किक forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः* incp,
                     Ast_ग्रन्थिः* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_सङ्ख्या loop_मानम्{nodep};
        जइ (!simulate_वृक्षः(initp->rhsp(), nullptr, initp, loop_मानम्)) {  //
            प्रत्यागम मिच्छ;
        }
        Ast_ग्रन्थिः* stmtsp = nullptr;
        जइ (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            प्रक्षिप_Deletep(initp);  // Always cloned below.
        }
        जइ (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = Ast_ग्रन्थिः::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(सच्च);

        Ast_ग्रन्थिः* newbodysp = nullptr;
        जइ (stmtsp) {
            प्रक्षिप_Deletep(stmtsp);  // Always cloned below.
            size_t iter_गणनम् = 0;
            ध्रुव size_t iterLimit = v3Global.opt.unrollLimit();
            जाव (सच्च) {
                UINFO(8, "      Looping " << loop_मानम्);
                V3_सङ्ख्या res{nodep};
                जइ (!simulate_वृक्षः(condp, &loop_मानम्, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    प्रत्यागम मिच्छ;
                }
                जइ (!res.isEqOne()) {
                    भङ्ग;  // Done with the loop
                } अण्णहा {
                    जइ (++iter_गणनम् > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        भङ्ग;
                    }

                    // Replace iterator values with constant
                    Ast_ग्रन्थिः* oneloopp = stmtsp->clone_वृक्षः(सच्च);
                    AstConst* varValuep = नूतन AstConst{nodep->fileline(), loop_मानम्};
                    जइ (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ध्रुव tempp
                            = नूतन AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, मिच्छ};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->delete_वृक्षः(), tempp);
                    }
                    ध्रुव string index = Ast_ग्रन्थिः::encode_सङ्ख्या(varValuep->toSInt());
                    ध्रुव string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = नूतन AstGenBlock{oneloopp->fileline(), nname, oneloopp, मिच्छ};
                    VL_DO_DANGLING(प्रक्षिप_Deletep(varValuep), varValuep);
                    newbodysp = Ast_ग्रन्थिः::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    ध्रुव AstAssign* ध्रुव incpass = VN_AS(incp, Assign);
                    V3_सङ्ख्या new_Loop_मानम्{nodep};
                    जइ (!simulate_वृक्षः(incpass->rhsp(), &loop_मानम्, incpass, new_Loop_मानम्)) {
                        nodep->v3error("Loop unrolling failed");
                        प्रत्यागम मिच्छ;
                    }
                    loop_मानम्.opAssign(new_Loop_मानम्);
                }
            }
        }
        जइ (!newbodysp) {  // initp might have effects after the loop
            जइ (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = नूतन AstInitial{initp->fileline(), initp->clone_वृक्षः(सच्च)};
            } अण्णहा {
                newbodysp = initp ? initp->clone_वृक्षः(सच्च) : nullptr;
            }
        }
        // Replace the FOR()
        जइ (newbodysp) {
            nodep->replaceWith(newbodysp);
        } अण्णहा {
            nodep->unlinkFrBack();
        }
        जइ (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        प्रत्यागम सच्च;
    }

    // VISITORS
    शून्य visit(AstVarRef* nodep) override {
        जइ (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = सच्च;
        }
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        जइ (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } अण्णहा {
            iterateChildren(nodep);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    UnrollGenVisitor() = स्वभाव;

    शून्य unroll(AstGenFor* nodep, ध्रुव std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = मिच्छ;  // Just checking RHS assignments
        m_varAssignHit = मिच्छ;  // Assign var hit
        m_forkHit = मिच्छ;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        जइ (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        जइ (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        जइ (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        जइ (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
        } अण्णहा जइ (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);  // Did replacement
        } अण्णहा {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
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
