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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Simulate.h"
#ଅନ୍ତର୍ଭାଵ "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

ଶ୍ରେଣୀ UnrollGenVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = nullptr;  // Iterator variable
    ଅଚଳ Ast_ଗଣ୍ଠି* m_ignoreIncp = nullptr;  // Increment node to ignore
    ତାର୍କିକ m_varModeCheck = ମିଥ୍ୟା;  // Just checking RHS assignments
    ତାର୍କିକ m_varAssignHit = ମିଥ୍ୟା;  // Assign var hit
    ତାର୍କିକ m_forkHit = ମିଥ୍ୟା;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    ଶୂନ୍ୟ replaceVarRef(Ast_ଗଣ୍ଠି* bodyp, Ast_ଗଣ୍ଠି* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([ଏହି, varValuep](AstVarRef* refp) {
            ଯଦି (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_ଗଣ୍ଠି* ଅଚଳ newconstp = varValuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
            }
        });
    }

    ତାର୍କିକ cantUnroll(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ ଅକ୍ଷର* reason) ଅଚଳ {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ତାର୍କିକ forUnrollCheck(
        AstGenFor* ଅଚଳ nodep,
        Ast_ଗଣ୍ଠି* ଅଚଳ initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp,
        Ast_ଗଣ୍ଠି* ଅଚଳ incp,  // Maybe under nodep or in bodysp
        Ast_ଗଣ୍ଠି* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        ଯଦି (initp) UINFO(6, "    Init " << initp);
        ଯଦି (condp) UINFO(6, "    Cond " << condp);
        ଯଦି (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* ଅଚଳ initAssp = VN_CAST(initp, Assign);
        ଯଦି (!initAssp) ଫେରନ୍ତୁ cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        ଯଦି (!VN_IS(initAssp->lhsp(), VarRef)) {
            ଫେରନ୍ତୁ cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ଅଚଳ AstAssign* ଅଚଳ incAssp = VN_CAST(incp, Assign);
        ଯଦି (!incAssp) ଫେରନ୍ତୁ cantUnroll(nodep, "no increment assignment");
        ଯଦି (incAssp->nextp()) ଫେରନ୍ତୁ cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        ଯଦି (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        ଅଚଳ AstConst* ଅଚଳ constInitp = VN_CAST(initAssp->rhsp(), Const);
        ଯଦି (!constInitp) ଫେରନ୍ତୁ cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = ସତ୍ୟ;
        m_varAssignHit = ମିଥ୍ୟା;
        m_forkHit = ମିଥ୍ୟା;
        m_ignoreIncp = incp;
        iterateAndNextNull(bodysp);
        iterateAndNextNull(incp);
        m_varModeCheck = ମିଥ୍ୟା;
        m_ignoreIncp = nullptr;
        ଯଦି (m_varAssignHit) ଫେରନ୍ତୁ cantUnroll(nodep, "genvar assigned *inside* loop");

        ଯଦି (m_forkHit) ଫେରନ୍ତୁ cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        ଯଦି (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            ଫେରନ୍ତୁ cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ simulate_ବୃକ୍ଷ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ଅଚଳ V3_ସଂଖ୍ୟା* loop_ମୂଲ୍ୟ, ଅଚଳ Ast_ଗଣ୍ଠି* dtypep,
                      V3_ସଂଖ୍ୟା& out_ଅଙ୍କ) {
        Ast_ଗଣ୍ଠି* clonep = nodep->clone_ବୃକ୍ଷ(ସତ୍ୟ);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        ଯଦି (loop_ମୂଲ୍ୟ) {
            AstConst* varValuep = ନୂତନ AstConst{nodep->fileline(), *loop_ମୂଲ୍ୟ};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = ନୂତନ AstBegin{nodep->fileline(), "[EditWrapper]", clonep, ମିଥ୍ୟା};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->delete_ବୃକ୍ଷ(), tempp = nullptr);
            VL_DO_DANGLING(ଠେଲ_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        ଯଦି (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_ବୃକ୍ଷ(), clonep);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        // Fetch the result
        ଅଚଳ V3_ସଂଖ୍ୟା* resp = simvis.fetch_ସଂଖ୍ୟା_Null(clonep);
        ଯଦି (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_ବୃକ୍ଷ(), clonep);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        // Patch up datatype
        ଯଦି (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_ଅଙ୍କ = new_con.ଅଙ୍କ();
            out_ଅଙ୍କ.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_ବୃକ୍ଷ(), clonep);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        out_ଅଙ୍କ = *resp;
        VL_DO_DANGLING(clonep->delete_ବୃକ୍ଷ(), clonep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି* incp,
                     Ast_ଗଣ୍ଠି* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_ସଂଖ୍ୟା loop_ମୂଲ୍ୟ{nodep};
        ଯଦି (!simulate_ବୃକ୍ଷ(initp->rhsp(), nullptr, initp, loop_ମୂଲ୍ୟ)) {  //
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        Ast_ଗଣ୍ଠି* stmtsp = nullptr;
        ଯଦି (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ଠେଲ_Deletep(initp);  // Always cloned below.
        }
        ଯଦି (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(ସତ୍ୟ);

        Ast_ଗଣ୍ଠି* newbodysp = nullptr;
        ଯଦି (stmtsp) {
            ଠେଲ_Deletep(stmtsp);  // Always cloned below.
            size_t iter_ଗଣନା = 0;
            ଅଚଳ size_t iterLimit = v3Global.opt.unrollLimit();
            ଯେତେବେଳେ (ସତ୍ୟ) {
                UINFO(8, "      Looping " << loop_ମୂଲ୍ୟ);
                V3_ସଂଖ୍ୟା res{nodep};
                ଯଦି (!simulate_ବୃକ୍ଷ(condp, &loop_ମୂଲ୍ୟ, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
                ଯଦି (!res.isEqOne()) {
                    ଭାଙ୍ଗ;  // Done with the loop
                } ଅନ୍ୟଥା {
                    ଯଦି (++iter_ଗଣନା > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        ଭାଙ୍ଗ;
                    }

                    // Replace iterator values with constant
                    Ast_ଗଣ୍ଠି* oneloopp = stmtsp->clone_ବୃକ୍ଷ(ସତ୍ୟ);
                    AstConst* varValuep = ନୂତନ AstConst{nodep->fileline(), loop_ମୂଲ୍ୟ};
                    ଯଦି (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ଅଚଳ tempp
                            = ନୂତନ AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, ମିଥ୍ୟା};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->delete_ବୃକ୍ଷ(), tempp);
                    }
                    ଅଚଳ string index = Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(varValuep->toSInt());
                    ଅଚଳ string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = ନୂତନ AstGenBlock{oneloopp->fileline(), nname, oneloopp, ମିଥ୍ୟା};
                    VL_DO_DANGLING(ଠେଲ_Deletep(varValuep), varValuep);
                    newbodysp = Ast_ଗଣ୍ଠି::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    ଅଚଳ AstAssign* ଅଚଳ incpass = VN_AS(incp, Assign);
                    V3_ସଂଖ୍ୟା new_Loop_ମୂଲ୍ୟ{nodep};
                    ଯଦି (!simulate_ବୃକ୍ଷ(incpass->rhsp(), &loop_ମୂଲ୍ୟ, incpass, new_Loop_ମୂଲ୍ୟ)) {
                        nodep->v3error("Loop unrolling failed");
                        ଫେରନ୍ତୁ ମିଥ୍ୟା;
                    }
                    loop_ମୂଲ୍ୟ.opAssign(new_Loop_ମୂଲ୍ୟ);
                }
            }
        }
        ଯଦି (!newbodysp) {  // initp might have effects after the loop
            ଯଦି (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = ନୂତନ AstInitial{initp->fileline(), initp->clone_ବୃକ୍ଷ(ସତ୍ୟ)};
            } ଅନ୍ୟଥା {
                newbodysp = initp ? initp->clone_ବୃକ୍ଷ(ସତ୍ୟ) : nullptr;
            }
        }
        // Replace the FOR()
        ଯଦି (newbodysp) {
            nodep->replaceWith(newbodysp);
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        ଯଦି (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = ସତ୍ୟ;
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    UnrollGenVisitor() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ଶୂନ୍ୟ unroll(AstGenFor* nodep, ଅଚଳ std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = ମିଥ୍ୟା;  // Just checking RHS assignments
        m_varAssignHit = ମିଥ୍ୟା;  // Assign var hit
        m_forkHit = ମିଥ୍ୟା;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        ଯଦି (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        ଯଦି (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        ଯଦି (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        ଯଦି (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        } ଅନ୍ୟଥା ଯଦି (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);  // Did replacement
        } ଅନ୍ୟଥା {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{ନୂତନ UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
ଶୂନ୍ୟ GenForUnroller::unroll(AstGenFor* nodep, ଅଚଳ std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
