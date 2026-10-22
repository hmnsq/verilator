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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Simulate.h"
#அந்தர்பாவ "V3Unroll.h"

VL_DEFINE_DEBUG_FUNCTIONS;
//######################################################################
// Unroll AstGenFor

வகுப்பு UnrollGenVisitor final : பொது VNVisitor {
    // STATE - across all visitors
    AstVar* m_forVarp = nullptr;  // Iterator variable
    மாறிலி Ast_முனை* m_ignoreIncp = nullptr;  // Increment node to ignore
    தருக்க m_varModeCheck = பொய்;  // Just checking RHS assignments
    தருக்க m_varAssignHit = பொய்;  // Assign var hit
    தருக்க m_forkHit = பொய்;  // Fork hit
    std::string m_beginName;  // What name to give begin iterations

    // METHODS
    வெறுமை replaceVarRef(Ast_முனை* bodyp, Ast_முனை* varValuep) {
        // Replace all occurances of loop variable in bodyp and next
        bodyp->foreachAndNext([இது, varValuep](AstVarRef* refp) {
            எனில் (refp->varp() == m_forVarp && refp->access().isReadOnly()) {
                Ast_முனை* மாறிலி newconstp = varValuep->clone_மரம்(பொய்);
                refp->replaceWith(newconstp);
                VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
            }
        });
    }

    தருக்க cantUnroll(Ast_முனை* nodep, மாறிலி எழுத்து* reason) மாறிலி {
        UINFO(4, "   Can't Unroll: " << reason << " :" << nodep);
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: Can't unroll generate for; " << reason);
        திரும்பு பொய்;
    }

    தருக்க forUnrollCheck(
        AstGenFor* மாறிலி nodep,
        Ast_முனை* மாறிலி initp,  // Maybe under nodep (no nextp), or standalone (ignore nextp)
        Ast_முனை_கோவை* condp,
        Ast_முனை* மாறிலி incp,  // Maybe under nodep or in bodysp
        Ast_முனை* bodysp) {
        // To keep the IF levels low, we return as each test fails.
        UINFO(4, " FOR Check " << nodep);
        எனில் (initp) UINFO(6, "    Init " << initp);
        எனில் (condp) UINFO(6, "    Cond " << condp);
        எனில் (incp) UINFO(6, "    Inc  " << incp);

        // Initial value check
        AstAssign* மாறிலி initAssp = VN_CAST(initp, Assign);
        எனில் (!initAssp) திரும்பு cantUnroll(nodep, "no initial assignment");
        UASSERT_OBJ(!(initp->nextp() && initp->nextp() != nodep), nodep,
                    "initial assignment shouldn't be a list");
        எனில் (!VN_IS(initAssp->lhsp(), VarRef)) {
            திரும்பு cantUnroll(nodep, "no initial assignment to simple variable");
        }
        //
        // Condition check
        UASSERT_OBJ(!condp->nextp(), nodep, "conditional shouldn't be a list");
        //
        // Assignment of next value check
        மாறிலி AstAssign* மாறிலி incAssp = VN_CAST(incp, Assign);
        எனில் (!incAssp) திரும்பு cantUnroll(nodep, "no increment assignment");
        எனில் (incAssp->nextp()) திரும்பு cantUnroll(nodep, "multiple increments");

        m_forVarp = VN_AS(initAssp->lhsp(), VarRef)->varp();
        எனில் (!m_forVarp->isGenVar()) {
            nodep->v3error("Non-genvar used in generate for: " << m_forVarp->prettyNameQ());
        }
        V3Const::constifyParamsEdit(initAssp->rhsp());  // rhsp may change

        // This check shouldn't be needed when using V3Simulate
        // however, for repeat loops, the loop variable is auto-generated
        // and the initp statements will reference a variable outside of the initp scope
        // alas, failing to simulate.
        மாறிலி AstConst* மாறிலி constInitp = VN_CAST(initAssp->rhsp(), Const);
        எனில் (!constInitp) திரும்பு cantUnroll(nodep, "non-constant initializer");

        //
        // Now, make sure there's no assignment to this variable in the loop
        m_varModeCheck = உண்மை;
        m_varAssignHit = பொய்;
        m_forkHit = பொய்;
        m_ignoreIncp = incp;
        iterateAndNextNull(bodysp);
        iterateAndNextNull(incp);
        m_varModeCheck = பொய்;
        m_ignoreIncp = nullptr;
        எனில் (m_varAssignHit) திரும்பு cantUnroll(nodep, "genvar assigned *inside* loop");

        எனில் (m_forkHit) திரும்பு cantUnroll(nodep, "fork inside loop");

        //
        UINFO(8, "   Loop Variable: " << m_forVarp);
        UINFOTREE(9, nodep, "", "for");

        // Finally, we can do it
        எனில் (!forUnroller(nodep, initAssp, condp, incp, bodysp)) {
            திரும்பு cantUnroll(nodep, "Unable to unroll loop");
        }
        VL_DANGLING(nodep);
        // Cleanup
        திரும்பு உண்மை;
    }

    தருக்க simulate_மரம்(Ast_முனை_கோவை* nodep, மாறிலி V3_எண்* loop_மதிப்பு, மாறிலி Ast_முனை* dtypep,
                      V3_எண்& out_இலக்கம்) {
        Ast_முனை* clonep = nodep->clone_மரம்(உண்மை);
        UASSERT_OBJ(clonep, nodep, "Failed to clone tree");
        எனில் (loop_மதிப்பு) {
            AstConst* varValuep = புதிய AstConst{nodep->fileline(), *loop_மதிப்பு};
            // Iteration requires a back, so put under temporary node
            AstBegin* tempp = புதிய AstBegin{nodep->fileline(), "[EditWrapper]", clonep, பொய்};
            replaceVarRef(tempp->stmtsp(), varValuep);
            clonep = tempp->stmtsp()->unlinkFrBackWithNext();
            VL_DO_CLEAR(tempp->delete_மரம்(), tempp = nullptr);
            VL_DO_DANGLING(தள்ளு_Deletep(varValuep), varValuep);
        }
        SimulateVisitor simvis;
        simvis.mainParamEmulate(clonep);
        எனில் (!simvis.optimizable()) {
            UINFO(4, "Unable to simulate");
            UINFOTREE(9, nodep, "", "_simtree");
            VL_DO_DANGLING(clonep->delete_மரம்(), clonep);
            திரும்பு பொய்;
        }
        // Fetch the result
        மாறிலி V3_எண்* resp = simvis.fetch_எண்_Null(clonep);
        எனில் (!resp) {
            UINFO(3, "No number returned from simulation");
            VL_DO_DANGLING(clonep->delete_மரம்(), clonep);
            திரும்பு பொய்;
        }
        // Patch up datatype
        எனில் (dtypep) {
            AstConst new_con{clonep->fileline(), *resp};
            new_con.dtypeFrom(dtypep);
            out_இலக்கம் = new_con.இலக்கம்();
            out_இலக்கம்.isSigned(dtypep->isSigned());
            VL_DO_DANGLING(clonep->delete_மரம்(), clonep);
            திரும்பு உண்மை;
        }
        out_இலக்கம் = *resp;
        VL_DO_DANGLING(clonep->delete_மரம்(), clonep);
        திரும்பு உண்மை;
    }

    தருக்க forUnroller(AstGenFor* nodep, AstAssign* initp, Ast_முனை_கோவை* condp, Ast_முனை* incp,
                     Ast_முனை* bodysp) {
        UINFO(9, "forUnroller " << nodep);
        V3_எண் loop_மதிப்பு{nodep};
        எனில் (!simulate_மரம்(initp->rhsp(), nullptr, initp, loop_மதிப்பு)) {  //
            திரும்பு பொய்;
        }
        Ast_முனை* stmtsp = nullptr;
        எனில் (initp) {
            initp->unlinkFrBack();  // Always a single statement; nextp() may be nodep
            // Don't add to list, we do it once, and setting loop index isn't
            // needed if we have > 1 loop, as we're constant propagating it
            தள்ளு_Deletep(initp);  // Always cloned below.
        }
        எனில் (bodysp) {
            bodysp->unlinkFrBackWithNext();
            stmtsp = Ast_முனை::addNext(stmtsp, bodysp);  // Maybe null if no body
        }
        // Mark variable to disable some later warnings
        m_forVarp->usedLoopIdx(உண்மை);

        Ast_முனை* newbodysp = nullptr;
        எனில் (stmtsp) {
            தள்ளு_Deletep(stmtsp);  // Always cloned below.
            size_t iter_மொத்தம் = 0;
            மாறிலி size_t iterLimit = v3Global.opt.unrollLimit();
            போது (உண்மை) {
                UINFO(8, "      Looping " << loop_மதிப்பு);
                V3_எண் res{nodep};
                எனில் (!simulate_மரம்(condp, &loop_மதிப்பு, nullptr, res)) {
                    nodep->v3error("Loop unrolling failed.");
                    திரும்பு பொய்;
                }
                எனில் (!res.isEqOne()) {
                    நிறுத்து;  // Done with the loop
                } இல்லையெனில் {
                    எனில் (++iter_மொத்தம் > iterLimit) {
                        nodep->v3error("Unrolling generate loop took too long; probably this is "
                                       "an infinite loop, otherwise set '--unroll-limit' above "
                                       << iterLimit);
                        நிறுத்து;
                    }

                    // Replace iterator values with constant
                    Ast_முனை* oneloopp = stmtsp->clone_மரம்(உண்மை);
                    AstConst* varValuep = புதிய AstConst{nodep->fileline(), loop_மதிப்பு};
                    எனில் (oneloopp) {
                        // Iteration requires a back, so put under temporary node
                        AstBegin* மாறிலி tempp
                            = புதிய AstBegin{oneloopp->fileline(), "[EditWrapper]", oneloopp, பொய்};
                        replaceVarRef(tempp->stmtsp(), varValuep);
                        oneloopp = tempp->stmtsp()->unlinkFrBackWithNext();
                        VL_DO_DANGLING(tempp->delete_மரம்(), tempp);
                    }
                    மாறிலி string index = Ast_முனை::encode_எண்(varValuep->toSInt());
                    மாறிலி string nname = m_beginName + "__BRA__" + index + "__KET__";
                    oneloopp = புதிய AstGenBlock{oneloopp->fileline(), nname, oneloopp, பொய்};
                    VL_DO_DANGLING(தள்ளு_Deletep(varValuep), varValuep);
                    newbodysp = Ast_முனை::addNextNull(newbodysp, oneloopp);

                    // loopValue += valInc
                    மாறிலி AstAssign* மாறிலி incpass = VN_AS(incp, Assign);
                    V3_எண் new_Loop_மதிப்பு{nodep};
                    எனில் (!simulate_மரம்(incpass->rhsp(), &loop_மதிப்பு, incpass, new_Loop_மதிப்பு)) {
                        nodep->v3error("Loop unrolling failed");
                        திரும்பு பொய்;
                    }
                    loop_மதிப்பு.opAssign(new_Loop_மதிப்பு);
                }
            }
        }
        எனில் (!newbodysp) {  // initp might have effects after the loop
            எனில் (initp) {  // GENFOR(ASSIGN(...)) need to move under a new Initial
                newbodysp = புதிய AstInitial{initp->fileline(), initp->clone_மரம்(உண்மை)};
            } இல்லையெனில் {
                newbodysp = initp ? initp->clone_மரம்(உண்மை) : nullptr;
            }
        }
        // Replace the FOR()
        எனில் (newbodysp) {
            nodep->replaceWith(newbodysp);
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        எனில் (newbodysp) UINFOTREE(9, newbodysp, "", "_new");
        திரும்பு உண்மை;
    }

    // VISITORS
    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (m_varModeCheck && nodep->varp() == m_forVarp && nodep->access().isWriteOrRW()) {
            UINFO(8, "   Itervar assigned to: " << nodep);
            m_varAssignHit = உண்மை;
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (m_varModeCheck && nodep == m_ignoreIncp) {
            // Ignore subtree that is the increment
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
    }

பொது:
    // CONSTRUCTORS
    UnrollGenVisitor() = இயல்பு;

    வெறுமை unroll(AstGenFor* nodep, மாறிலி std::string& beginName) {
        m_forVarp = nullptr;  // Iterator variable
        m_ignoreIncp = nullptr;  // Increment node to ignore
        m_varModeCheck = பொய்;  // Just checking RHS assignments
        m_varAssignHit = பொய்;  // Assign var hit
        m_forkHit = பொய்;  // Fork hit
        m_beginName = beginName;

        // Constify before unroll call, as it may change what is underneath.
        எனில் (nodep->initsp()) V3Const::constifyEdit(nodep->initsp());  // initsp may change
        எனில் (nodep->condp()) V3Const::constifyEdit(nodep->condp());  // condp may change
        எனில் (nodep->incsp()) V3Const::constifyEdit(nodep->incsp());  // incsp may change
        எனில் (nodep->condp()->isZero()) {
            // We don't need to do any loops.  Remove the GenFor,
            // Genvar's don't care about any initial assignments.
            //
            // Note normal For's can't do exactly this deletion, as
            // we'd need to initialize the variable to the initial
            // condition, but they'll become while's which can be
            // deleted by V3Const.
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        } இல்லையெனில் எனில் (forUnrollCheck(nodep, nodep->initsp(), nodep->condp(), nodep->incsp(),
                                  nodep->itemsp())) {
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);  // Did replacement
        } இல்லையெனில் {
            nodep->v3error("For loop doesn't have genvar index, or is malformed");
            // We will die, do it gracefully
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }
};

//######################################################################
// GenForUnroller members

GenForUnroller::GenForUnroller()
    : m_unrollerp{புதிய UnrollGenVisitor{}} {}
GenForUnroller::~GenForUnroller() { VL_DO_DANGLING(delete m_unrollerp, m_unrollerp); }
வெறுமை GenForUnroller::unroll(AstGenFor* nodep, மாறிலி std::string& beginName) {
    m_unrollerp->unroll(nodep, beginName);
}
