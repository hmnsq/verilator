// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add Unknown assigns
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
// V3Unknown's Transformations:
//
// Each module:
//      TBD: Eliminate tristates by adding __in, __inen, __en wires in parallel
//          Need __en in changed list if a signal is on the LHS of a assign
//      Constants:
//          RHS, Replace 5'bx_1_x with a module global we init to a random value
//              CONST(5'bx_1_x) -> VARREF(_{numberedtemp})
//                              -> VAR(_{numberedtemp})
//                              -> INITIAL(VARREF(_{numberedtemp}),
//                                         OR(5'bx_1_x,AND(random,5'b0_1_x))
//              OPTIMIZE: Must not collapse this initial back into the equation.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Unknown.h"

#ضمن "V3Const.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف UnknownVisitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ساكن ثابت std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    AstNodeModule* m_modp = nullptr;  // Current module
    AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    منطقي m_constXCvt = ܕܓܠܐ;  // Convert X's
    منطقي m_allowXUnique = ܫܪܪܐ;  // Allow unique assignments

    // METHODS

    فراغ addVar(AstVar* varp) {
        ܐܢ (m_ftaskp) {
            varp->funcLocal(ܫܪܪܐ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } ܐܚܪܢܐ {
            m_modp->prependStmtsp(varp);
        }
    }

    فراغ replaceBoundLvalue(AstNodeExpr* nodep, AstNodeExpr* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        AstNodeExpr* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        ܟܡܐ (VN_IS(prep->backp(), NodeSel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), NodeExpr);
        }
        ܐܢ (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->deleteTree(), condp);
            ܦܢܐ;
        }
        FileLine* ثابت fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        ܐܢ (ثابت AstIf* ثابت ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            AstNodeExpr* ثابت earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            AstNodeExpr* ثابت newp = جديد AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } ܐܚܪܢܐ {
            AstNode* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            ܟܡܐ (!VN_IS(stmtp, NodeStmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            AstNode* ثابت origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            AstNode* elseStmtp = nullptr;
            ثابت منطقي hasSideEffects = origStmtp->exists(
                [](AstNode* ثابت np) { ܦܢܐ !np->isPure() || np->isTimingControl(); });
            ܐܢ (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->cloneTree(ܕܓܠܐ);
                AstVar* ثابت varp = جديد AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                AstNode* ثابت prepCopyp = prep->clonep();
                prepCopyp->replaceWith(جديد AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(pushDeletep(prepCopyp), prepCopyp);
            }
            AstIf* ثابت newp = جديد AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(ܫܪܪܐ);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(ثابت AstNodeExpr* ثابت nodep) {
        AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        ܦܢܐ varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ساكن منطقي isStaticlyGte(AstConst* ثابت msbConstp, ثابت AstNodeExpr* ثابت exprp) {
        ܐܢ (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->num().toSInt() >= (1 << exprp->width()) - 1) {
            ܦܢܐ ܫܪܪܐ;
        }
        ܐܢ (ثابت AstConst* ثابت constp = VN_CAST(exprp, Const)) {
            ܐܢ (V3Number{msbConstp}.opGte(msbConstp->num(), constp->num()).isNeqZero()) {
                ܦܢܐ ܫܪܪܐ;
            }
        }
        ܦܢܐ ܕܓܠܐ;
    }

    AstNodeExpr* newExprStmtOrClone(AstNodeExpr*& exprp) {
        ܐܢ (!exprp->isPure()) {
            AstVar* ثابت varp = createAddTemp(exprp);
            FileLine* ثابت fl = exprp->fileline();
            exprp = جديد AstExprStmt{
                fl, جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                جديد AstVarRef{fl, varp, VAccess::READ}};
            ܦܢܐ جديد AstVarRef{fl, varp, VAccess::READ};
        }
        ܦܢܐ exprp->cloneTreePure(ܕܓܠܐ);
    }

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = ܫܪܪܐ;
            // Class X randomization causes Vxrand in strange places, so disable
            ܐܢ (VN_IS(nodep, Class)) m_allowXUnique = ܕܓܠܐ;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    فراغ visit(AstNodeFTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ܕܓܠܐ;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = ܫܪܪܐ;
        iterateAndNextNull(nodep->stmtsp());
    }
    فراغ visit(AstNodeDType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ܕܓܠܐ;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        ܐܢ (nodep->isParam()) m_allowXUnique = ܕܓܠܐ;
        iterateChildren(nodep);
    }
    فراغ visitEqNeqCase(AstNodeBiop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ܐܢ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ܦܢܐ;
        } ܐܚܪܢܐ {
            AstNodeExpr* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            AstNodeExpr* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            ܐܢ (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->num().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->num().isFourState()))) {
                newp = جديد AstConst(nodep->fileline(), AstConst::WidthedValue{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->deleteTree(), lhsp);
                VL_DO_DANGLING(rhsp->deleteTree(), rhsp);
            } ܐܚܪܢܐ {
                ܐܢ (VN_IS(nodep, EqCase)) {
                    newp = جديد AstEq{nodep->fileline(), lhsp, rhsp};
                } ܐܚܪܢܐ {
                    newp = جديد AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    فراغ visitEqNeqWild(AstNodeBiop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ܐܢ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ܦܢܐ;
        } ܐܚܪܢܐ {
            AstNodeExpr* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            AstNodeExpr* newp;
            ܐܢ (!VN_IS(rhsp, Const)) {
                ܐܢ (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = جديد AstEq{nodep->fileline(), lhsp, rhsp};
            } ܐܚܪܢܐ {
                // X or Z's become mask, ala case statements.
                V3Number nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->num());
                V3Number numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->num());
                AstNodeExpr* ثابت and1p = جديد AstAnd{nodep->fileline(), lhsp,
                                                      جديد AstConst{nodep->fileline(), nummask}};
                AstNodeExpr* ثابت and2p = جديد AstConst{nodep->fileline(), numval};
                ܐܢ (VN_IS(nodep, EqWild)) {
                    newp = جديد AstEq{nodep->fileline(), and1p, and2p};
                } ܐܚܪܢܐ {
                    newp = جديد AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->deleteTree(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    فراغ visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    فراغ visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    فراغ visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    فراغ visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    فراغ visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
    فراغ visit(AstCountBits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<منطقي, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->num().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->num().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->num().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        AstNodeExpr* nonXp = nullptr;
        ܐܢ (!dropop[0]) {
            nonXp = nodep->rhsp();
        } ܐܚܪܢܐ ܐܢ (!dropop[1]) {
            nonXp = nodep->thsp();
        } ܐܚܪܢܐ ܐܢ (!dropop[2]) {
            nonXp = nodep->fhsp();
        } ܐܚܪܢܐ {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            ܦܢܐ;
        }
        ܐܢ (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->deleteTree();
            nodep->rhsp(nonXp->cloneTreePure(ܫܪܪܐ));
        }
        ܐܢ (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->deleteTree();
            nodep->thsp(nonXp->cloneTreePure(ܫܪܪܐ));
        }
        ܐܢ (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->deleteTree();
            nodep->fhsp(nonXp->cloneTreePure(ܫܪܪܐ));
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstConst* nodep) override {
        ܐܢ (m_constXCvt && nodep->num().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3Number numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->num());
            V3Number numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->num());
            ܐܢ (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3Number numnew{nodep, numb1.width()};
                ܐܢ (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } ܐܚܪܢܐ {
                    numnew.opAssign(numb1);
                }
                AstConst* ثابت newp = جديد AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->deleteTree(), nodep);
                UINFO(4, "   -> " << newp);
            } ܐܚܪܢܐ {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ثابت newvarp
                    = جديد AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                AstNodeVarRef* ثابت newref1p
                    = جديد AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ثابت newinitp = جديد AstInitial{
                    nodep->fileline(),
                    جديد AstAssign{
                        nodep->fileline(),
                        جديد AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        جديد AstOr{nodep->fileline(), جديد AstConst{nodep->fileline(), numb1},
                                  جديد AstAnd{
                                      nodep->fileline(), جديد AstConst{nodep->fileline(), numbx},
                                      جديد AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                AstNode* ثابت newStmtsp = AstNode::addNext<AstNode, AstNode>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->deleteTree(), nodep);
            }
        }
    }

    فراغ visit(AstSel* nodep) override {
        iterateChildren(nodep);
        ܐܢ (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            ثابت AstNode* ثابت basefromp = nodep->baseFromp(ܫܪܪܐ);
            منطقي lvalue = ܕܓܠܐ;
            ܐܢ (ثابت AstNodeVarRef* ثابت varrefp = VN_CAST(basefromp, NodeVarRef)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ثابت uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ثابت maxmsbConstp = جديد AstConst{
                nodep->fileline(), AstConst::WidthedValue{}, nodep->lsbp()->width(), maxmsb};
            AstNodeExpr* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            ܐܢ (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->deleteTree(), maxmsbConstp);
                nodep->lsbp(lsbp);
                ܦܢܐ;
            }
            nodep->lsbp(newExprStmtOrClone(lsbp));
            AstNodeExpr* condp
                = V3Const::constifyEdit(جديد AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            ܐܢ (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3Number xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                AstNodeExpr* ثابت xexprp = جديد AstConst{nodep->fileline(), xnum};
                AstNodeExpr* ثابت newp = [&]() -> AstNodeExpr* {
                    ܐܢ (condp->isZero()) {
                        VL_DO_DANGLING(condp->deleteTree(), condp);
                        VL_DO_DANGLING(nodep->deleteTree(), nodep);
                        ܦܢܐ xexprp;
                    }
                    ܦܢܐ جديد AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ܐܚܪܢܐ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    فراغ visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        ܐܢ (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            AstNode* ثابت basefromp = nodep->fromp()->baseFromp(ܫܪܪܐ);
            منطقي lvalue = ܕܓܠܐ;
            ܐܢ (ثابت AstNodeVarRef* ثابت varrefp = VN_CAST(basefromp, NodeVarRef)) {
                lvalue = varrefp->access().isWriteOrRW();
            } ܐܚܪܢܐ ܐܢ (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } ܐܚܪܢܐ {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            صحيح declElements = -1;
            AstNodeDType* ثابت dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ثابت AstNodeArrayDType* ثابت adtypep = VN_CAST(dtypep, NodeArrayDType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            ܐܢ (ثابت AstModDiv* ثابت moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                ܐܢ (ثابت AstConst* ثابت modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    ܐܢ (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        ܦܢܐ;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ثابت declElementsp
                = جديد AstConst{nodep->fileline(), AstConst::WidthedValue{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            AstNodeExpr* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            ܐܢ (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->deleteTree(), declElementsp);
                nodep->bitp(bitp);
                ܦܢܐ;
            }
            nodep->bitp(newExprStmtOrClone(bitp));
            AstNodeExpr* condp = جديد AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ثابت AstNodeDType* ثابت nodeDtp = nodep->dtypep()->skipRefp();
            ܐܢ (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(nodeDtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3Number xnum{nodep, nodep->width()};
                ܐܢ (nodeDtp->isDouble()) {
                    xnum = V3Number{nodep, V3Number::Double{}, 0.0};
                } ܐܚܪܢܐ ܐܢ (nodeDtp->isString()) {
                    xnum = V3Number{nodep, V3Number::String{}, ""};
                } ܐܚܪܢܐ {
                    ܐܢ (nodeDtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } ܐܚܪܢܐ {
                        xnum.setAllBits0();
                    }
                }
                AstNode* ثابت newp = جديد AstCond{nodep->fileline(), condp, nodep,
                                                  جديد AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ܐܚܪܢܐ ܐܢ (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                AstNodeExpr* ثابت asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                AstNodeExpr* ثابت newp
                    = جديد AstCond{asBitp->fileline(), condp, asBitp,
                                  جديد AstConst{asBitp->fileline(), AstConst::WidthedValue{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } ܐܚܪܢܐ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit UnknownVisitor(AstNetlist* nodep)
        : m_lvboundNames{"__Vlvbound"}
        , m_xrandNames{std::make_unique<V3UniqueNames>(s_xrandPrefix)} {
        iterate(nodep);
    }
    ~UnknownVisitor() override {  //
        V3Stats::addStat("Unknowns, variables created", m_statUnkVars);
        V3Stats::addStat("Unknowns, else branches created", m_statElses);
    }
};

ثابت std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

فراغ V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("unknown", 0, dumpTreeEitherLevel() >= 3);
}
