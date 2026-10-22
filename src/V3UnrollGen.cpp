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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Simulate.h"
#অন্তর্ভাব "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

শ্রেণী UnrollGenVisitor final : সর্বজনীন VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = nullptr;  // Iterator variable
    ধ্রুবক Ast_গ্রন্থি* m_ignoreIncp = nullptr;  // Increment node to ignore
    যৌক্তিক m_varModeCheck = মিথ্যা;  // Just checking RHS assignments
    যৌক্তিক m_varAssignHit = মিথ্যা;  // Assign var hit
    যৌক্তিক m_forkHit = মিথ্যা;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    শূন্য replaceVarRef(Ast_গ্রন্থি* bodyp, Ast_গ্রন্থি* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreach_And_পরবর্তী([এই, varValuep](AstVarRef* refp) {
            যদি (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_গ্রন্থি* ধ্রুবক newconstp = varValuep->clone_বৃক্ষ(মিথ্যা);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
            }
        });
    }

    যৌক্তিক cantUnroll(Ast_গ্রন্থি* nodep, ধ্রুবক অক্ষর* reason) ধ্রুবক {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        ফেরত মিথ্যা;
    }

    যৌক্তিক forUnrollCheck(
        AstGenFor* ধ্রুবক nodep,
        Ast_গ্রন্থি* ধ্রুবক initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_গ্রন্থি_রাশিমালা* condp,
        Ast_গ্রন্থি* ধ্রুবক incp,  // Maybe under nodep or in bodysp
        Ast_গ্রন্থি* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        যদি (initp) UINFO(6, "    Init " << initp);
        যদি (condp) UINFO(6, "    Cond " << condp);
        যদি (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* ধ্রুবক initAssp = VN_CAST(initp, Assign);
        যদি (!initAssp) ফেরত cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        যদি (!VN_IS(initAssp->lhsp(), VarRef)) {
            ফেরত cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        ধ্রুবক AstAssign* ধ্রুবক incAssp = VN_CAST(incp, Assign);
        যদি (!incAssp) ফেরত cantUnroll(nodep, "no increment assignment");
        যদি (incAssp->nextp()) ফেরত cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        যদি (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        ধ্রুবক AstConst* ধ্রুবক constInitp = VN_CAST(initAssp->rhsp(), Const);
        যদি (!constInitp) ফেরত cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = সত্য;
        m_varAssignHit = মিথ্যা;
        m_forkHit = মিথ্যা;
        m_ignoreIncp = incp;
        iterate_And_পরবর্তী_Null(bodysp);
        iterate_And_পরবর্তী_Null(incp);
        m_varModeCheck = মিথ্যা;
        m_ignoreIncp = nullptr;
        যদি (m_varAssignHit) ফেরত cantUnroll(nodep, "genvar assigned *inside* loop");

        যদি (m_forkHit) ফেরত cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        যদি (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            ফেরত cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        ফেরত সত্য;
    }

    যৌক্তিক simulate_বৃক্ষ(Ast_গ্রন্থি_রাশিমালা* nodep, ধ্রুবক V3_সংখ্যা* loop_মান, ধ্রুবক Ast_গ্রন্থি* dtypep,
                      V3_সংখ্যা& out_অঙ্ক) {
        Ast_গ্রন্থি* clonep = nodep->clone_বৃক্ষ(সত্য);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        যদি (loop_মান) {
            AstConst* varValuep = নতুন AstConst{nodep->fileline(), *loop_মান};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = নতুন AstBegin{nodep->fileline(), "[EditWrapper]", clonep, মিথ্যা};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlink_Fr_Back_With_পরবর্তী();
            VL_DO_CLEAR(tempp->delete_বৃক্ষ(), tempp = nullptr);
            VL_DO_DANGLING(ঠেলো_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        যদি (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_বৃক্ষ(), clonep);
            ফেরত মিথ্যা;
        }
        // Fetch the result
        ধ্রুবক V3_সংখ্যা* resp = simvis.fetch_সংখ্যা_Null(clonep);
        যদি (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_বৃক্ষ(), clonep);
            ফেরত মিথ্যা;
        }
        // Patch up datatype
        যদি (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_অঙ্ক = new_con.অঙ্ক();
            out_অঙ্ক.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_বৃক্ষ(), clonep);
            ফেরত সত্য;
        }
        out_অঙ্ক = *resp;
        VL_DO_DANGLING(clonep->delete_বৃক্ষ(), clonep);
        ফেরত সত্য;
    }

    যৌক্তিক forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_গ্রন্থি_রাশিমালা* condp, Ast_গ্রন্থি* incp,
                     Ast_গ্রন্থি* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_সংখ্যা loop_মান{nodep};
        যদি (!simulate_বৃক্ষ(initp->rhsp(), nullptr, initp, loop_মান)) {  //
            ফেরত মিথ্যা;
        }
        Ast_গ্রন্থি* stmtsp = nullptr;
        যদি (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            ঠেলো_Deletep(initp);  // Always cloned below.
        }
        যদি (bodysp) {
            bodysp->unlink_Fr_Back_With_পরবর্তী();
            stmtsp = Ast_গ্রন্থি::add_পরবর্তী(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(সত্য);

        Ast_গ্রন্থি* newbodysp = nullptr;
        যদি (stmtsp) {
            ঠেলো_Deletep(stmtsp);  // Always cloned below.
            আকার_t iter_গুনতি = 0;
            ধ্রুবক আকার_t iterLimit = v3Global.opt.unrollLimit();
            যখন (সত্য) {
                UINFO(8, "      Looping " << loop_মান);
                V3_সংখ্যা res{nodep};
                যদি (!simulate_বৃক্ষ(condp, &loop_মান, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    ফেরত মিথ্যা;
                }
                যদি (!res.isEqOne()) {
                    বিরতি;  // Done with the loop
                } অন্যথা {
                    যদি (++iter_গুনতি > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        বিরতি;
                    }

                    // Replace iterator values with constant
                    Ast_গ্রন্থি* oneloopp = stmtsp->clone_বৃক্ষ(সত্য);
                    AstConst* varValuep = নতুন AstConst{nodep->fileline(), loop_মান};
                    যদি (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* ধ্রুবক tempp
                            = নতুন AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, মিথ্যা};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlink_Fr_Back_With_পরবর্তী();
                        VL_DO_DANGLING(tempp->delete_বৃক্ষ(), tempp);
                    }
                    ধ্রুবক string সূচক = Ast_গ্রন্থি::encode_সংখ্যা(varValuep->toSInt());
                    ধ্রুবক string nname = m_beginName + "__BRA__" + সূচক + "__KET__";
                    oneloopp = নতুন AstGenBlock{oneloopp->fileline(), nname, oneloopp, মিথ্যা};
                    VL_DO_DANGLING(ঠেলো_Deletep(varValuep), varValuep);
                    newbodysp = Ast_গ্রন্থি::add_পরবর্তী_Null(newbodysp, oneloopp);

                    // loopValue += valInc
                    ধ্রুবক AstAssign* ধ্রুবক incpass = VN_AS(incp, Assign);
                    V3_সংখ্যা new_Loop_মান{nodep};
                    যদি (!simulate_বৃক্ষ(incpass->rhsp(), &loop_মান, incpass, new_Loop_মান)) {
                        nodep->v3error("Loop unrolling failed");
                        ফেরত মিথ্যা;
                    }
                    loop_মান.opAssign(new_Loop_মান);
                }
            }
        }
        যদি (!newbodysp) {  // initp might have effects after the loop
            যদি (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = নতুন AstInitial{initp->fileline(), initp->clone_বৃক্ষ(সত্য)};
            } অন্যথা {
                newbodysp = initp ? initp->clone_বৃক্ষ(সত্য) : nullptr;
            }
        }
        // Replace the FOR()
        যদি (newbodysp) {
            nodep->replaceWith(newbodysp);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        যদি (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        ফেরত সত্য;
    }

    // VISITORS
    শূন্য visit(AstVarRef* nodep) override {
        যদি (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = সত্য;
        }
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } অন্যথা {
            iterateChildren(nodep);
        }
    }

সর্বজনীন:
    // CONSTRUCTORS
    UnrollGenVisitor() = পূর্বনির্ধারিত;

    শূন্য unroll(AstGenFor* nodep, ধ্রুবক std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = মিথ্যা;  // Just checking RHS assignments
        m_varAssignHit = মিথ্যা;  // Assign var hit
        m_forkHit = মিথ্যা;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        যদি (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        যদি (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        যদি (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        যদি (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        } অন্যথা যদি (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);  // Did replacement
        } অন্যথা {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{নতুন UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
শূন্য GenForUnroller::unroll(AstGenFor* nodep, ধ্রুবক std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
