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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Simulate.h"
#అన్తర్భావ "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

తరగతి UnrollGenVisitor final : బహిరంగ VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = శూన్యసూచిక;  // Iterator variable
    స్థిరరాశి Ast_కణుపు* m_ignoreIncp = శూన్యసూచిక;  // Increment node to ignore
    తార్కిక m_var_బహుళకం_Check = అబద్ధం;  // Just checking RHS assignments
    తార్కిక m_var_కేటాయించు_తగిలింది = అబద్ధం;  // Assign var hit
    తార్కిక m_fork_తగిలింది = అబద్ధం;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    శూన్య replaceVarRef(Ast_కణుపు* bodyp, Ast_కణుపు* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreach_And_తదుపరి([ఇది, varValuep](AstVarRef* refp) {
            ఒకవేళ (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_కణుపు* స్థిరరాశి newconstp = varValuep->clone_వృక్షం(అబద్ధం);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
            }
        });
    }

    తార్కిక cantUnroll(Ast_కణుపు* nodep, స్థిరరాశి అక్షరం* reason) స్థిరరాశి {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        తిరిగి అబద్ధం;
    }

    తార్కిక forUnrollCheck(
        AstGenFor* స్థిరరాశి nodep,
        Ast_కణుపు* స్థిరరాశి initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_కణుపు_సమాసం* condp,
        Ast_కణుపు* స్థిరరాశి incp,  // Maybe under nodep or in bodysp
        Ast_కణుపు* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        ఒకవేళ (initp) UINFO(6, "    Init " << initp);
        ఒకవేళ (condp) UINFO(6, "    Cond " << condp);
        ఒకవేళ (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        Ast_కేటాయించు* స్థిరరాశి initAssp = VN_CAST(initp, కేటాయించు);
        ఒకవేళ (!initAssp) తిరిగి cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        ఒకవేళ (!VN_IS(initAssp->lhsp(), VarRef)) {
            తిరిగి cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        స్థిరరాశి Ast_కేటాయించు* స్థిరరాశి incAssp = VN_CAST(incp, కేటాయించు);
        ఒకవేళ (!incAssp) తిరిగి cantUnroll(nodep, "no increment assignment");
        ఒకవేళ (incAssp->nextp()) తిరిగి cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        ఒకవేళ (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        స్థిరరాశి AstConst* స్థిరరాశి constInitp = VN_CAST(initAssp->rhsp(), Const);
        ఒకవేళ (!constInitp) తిరిగి cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_var_బహుళకం_Check = నిజం;
        m_var_కేటాయించు_తగిలింది = అబద్ధం;
        m_fork_తగిలింది = అబద్ధం;
        m_ignoreIncp = incp;
        iterate_And_తదుపరి_Null(bodysp);
        iterate_And_తదుపరి_Null(incp);
        m_var_బహుళకం_Check = అబద్ధం;
        m_ignoreIncp = శూన్యసూచిక;
        ఒకవేళ (m_var_కేటాయించు_తగిలింది) తిరిగి cantUnroll(nodep, "genvar assigned *inside* loop");

        ఒకవేళ (m_fork_తగిలింది) తిరిగి cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        ఒకవేళ (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            తిరిగి cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        తిరిగి నిజం;
    }

    తార్కిక simulate_వృక్షం(Ast_కణుపు_సమాసం* nodep, స్థిరరాశి V3_సంఖ్య* loop_విలువ, స్థిరరాశి Ast_కణుపు* dtypep,
                      V3_సంఖ్య& బహిర్గతం_అంకె) {
        Ast_కణుపు* clonep = nodep->clone_వృక్షం(నిజం);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        ఒకవేళ (loop_విలువ) {
            AstConst* varValuep = కొత్త AstConst{nodep->fileline(), *loop_విలువ};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = కొత్త AstBegin{nodep->fileline(), "[EditWrapper]", clonep, అబద్ధం};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();
            VL_DO_CLEAR(tempp->delete_వృక్షం(), tempp = శూన్యసూచిక);
            VL_DO_DANGLING(నెట్టు_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        ఒకవేళ (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_వృక్షం(), clonep);
            తిరిగి అబద్ధం;
        }
        // Fetch the result
        స్థిరరాశి V3_సంఖ్య* resp = simvis.fetch_సంఖ్య_Null(clonep);
        ఒకవేళ (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_వృక్షం(), clonep);
            తిరిగి అబద్ధం;
        }
        // Patch up datatype
        ఒకవేళ (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            బహిర్గతం_అంకె = new_con.అంకె();
            బహిర్గతం_అంకె.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_వృక్షం(), clonep);
            తిరిగి నిజం;
        }
        బహిర్గతం_అంకె = *resp;
        VL_DO_DANGLING(clonep->delete_వృక్షం(), clonep);
        తిరిగి నిజం;
    }

    తార్కిక forUnroller(AstGenFor* nodep, Ast_కేటాయించు* initp, Ast_కణుపు_సమాసం* condp, Ast_కణుపు* incp,
                     Ast_కణుపు* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_సంఖ్య loop_విలువ{nodep};
        ఒకవేళ (!simulate_వృక్షం(initp->rhsp(), శూన్యసూచిక, initp, loop_విలువ)) {  //
            తిరిగి అబద్ధం;
        }
        Ast_కణుపు* stmtsp = శూన్యసూచిక;
        ఒకవేళ (initp) {
            initp->unlink_Fr_వెనుక();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            నెట్టు_Deletep(initp);  // Always cloned below.
        }
        ఒకవేళ (bodysp) {
            bodysp->unlink_Fr_వెనుక_With_తదుపరి();
            stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->వాడిన_Loop_Idx(నిజం);

        Ast_కణుపు* newbodysp = శూన్యసూచిక;
        ఒకవేళ (stmtsp) {
            నెట్టు_Deletep(stmtsp);  // Always cloned below.
            కొలత_t iter_లెక్క = 0;
            స్థిరరాశి కొలత_t iter_హద్దు = v3Global.opt.unroll_హద్దు();
            అయితే (నిజం) {
                UINFO(8, "      Looping " << loop_విలువ);
                V3_సంఖ్య res{nodep};
                ఒకవేళ (!simulate_వృక్షం(condp, &loop_విలువ, శూన్యసూచిక, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    తిరిగి అబద్ధం;
                }
                ఒకవేళ (!res.isEqOne()) {
                    ఆపు;  // Done with the loop
                } లేకపోతే {
                    ఒకవేళ (++iter_లెక్క > iter_హద్దు) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iter_హద్దు);
                        ఆపు;
                    }

                    // Replace iterator values with constant
                    Ast_కణుపు* oneloopp = stmtsp->clone_వృక్షం(నిజం);
                    AstConst* varValuep = కొత్త AstConst{nodep->fileline(), loop_విలువ};
                    ఒకవేళ (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* స్థిరరాశి tempp
                            = కొత్త AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, అబద్ధం};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();
                        VL_DO_DANGLING(tempp->delete_వృక్షం(), tempp);
                    }
                    స్థిరరాశి string సూచీ = Ast_కణుపు::encode_సంఖ్య(varValuep->గమ్యం_SInt());
                    స్థిరరాశి string nname = m_beginName + "__BRA__" + సూచీ + "__KET__";
                    oneloopp = కొత్త Ast_Gen_ఖండం{oneloopp->fileline(), nname, oneloopp, అబద్ధం};
                    VL_DO_DANGLING(నెట్టు_Deletep(varValuep), varValuep);
                    newbodysp = Ast_కణుపు::కూడు_తదుపరి_Null(newbodysp, oneloopp);

                    // loopValue += valInc
                    స్థిరరాశి Ast_కేటాయించు* స్థిరరాశి incpass = VN_AS(incp, కేటాయించు);
                    V3_సంఖ్య new_Loop_విలువ{nodep};
                    ఒకవేళ (!simulate_వృక్షం(incpass->rhsp(), &loop_విలువ, incpass, new_Loop_విలువ)) {
                        nodep->v3error("Loop unrolling failed");
                        తిరిగి అబద్ధం;
                    }
                    loop_విలువ.op_కేటాయించు(new_Loop_విలువ);
                }
            }
        }
        ఒకవేళ (!newbodysp) {  // initp might have effects after the loop
            ఒకవేళ (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = కొత్త AstInitial{initp->fileline(), initp->clone_వృక్షం(నిజం)};
            } లేకపోతే {
                newbodysp = initp ? initp->clone_వృక్షం(నిజం) : శూన్యసూచిక;
            }
        }
        // Replace the FOR()
        ఒకవేళ (newbodysp) {
            nodep->replaceWith(newbodysp);
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        ఒకవేళ (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        తిరిగి నిజం;
    }

    // VISITORS
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (m_var_బహుళకం_Check && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_var_కేటాయించు_తగిలింది = నిజం;
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (m_var_బహుళకం_Check && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } లేకపోతే {
            iterateChildren(nodep);
        }
    }

బహిరంగ:
    // CONSTRUCTORS
    UnrollGenVisitor() = అప్రమేయ;

    శూన్య unroll(AstGenFor* nodep, స్థిరరాశి std::string& beginName) {
        m_forVarp = శూన్యసూచిక;  // Iterator variable
        m_ignoreIncp = శూన్యసూచిక;  // Increment node to ignore
        m_var_బహుళకం_Check = అబద్ధం;  // Just checking RHS assignments
        m_var_కేటాయించు_తగిలింది = అబద్ధం;  // Assign var hit
        m_fork_తగిలింది = అబద్ధం;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        ఒకవేళ (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        ఒకవేళ (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        ఒకవేళ (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        ఒకవేళ (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        } లేకపోతే ఒకవేళ (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);  // Did replacement
        } లేకపోతే {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{కొత్త UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(తొలగించు m_unrollerp, m_unrollerp); }
శూన్య GenForUnroller::unroll(AstGenFor* nodep, స్థిరరాశి std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
