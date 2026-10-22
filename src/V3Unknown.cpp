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
    Ast_عقدة_Module* m_modp = nullptr;  // Current module
    Ast_عقدة_FTask* m_ftaskp = nullptr;  // Current function/task
    منطقي m_constXCvt = خطأ;  // Convert X's
    منطقي m_allowXUnique = true;  // Allow unique assignments

    // METHODS

    فراغ addVar(AstVar* varp) {
        إذا (m_ftaskp) {
            varp->funcLocal(true);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } وإلا {
            m_modp->prependStmtsp(varp);
        }
    }

    فراغ replaceBoundLvalue(Ast_عقدة_تعبير* nodep, Ast_عقدة_تعبير* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_عقدة_تعبير* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        طالما (VN_IS(prep->backp(), عقدة_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), عقدة_تعبير);
        }
        إذا (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_شجرة(), condp);
            أرجع;
        }
        File_سطر* ثابت fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        إذا (ثابت AstIf* ثابت ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_عقدة_تعبير* ثابت earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_عقدة_تعبير* ثابت newp = جديد AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } وإلا {
            Ast_عقدة* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            طالما (!VN_IS(stmtp, عقدة_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_عقدة* ثابت origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_عقدة* elseStmtp = nullptr;
            ثابت منطقي hasSideEffects = origStmtp->exists(
                [](Ast_عقدة* ثابت np) { أرجع !np->isPure() || np->isTimingControl(); });
            إذا (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_شجرة(خطأ);
                AstVar* ثابت varp = جديد AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_عقدة* ثابت prepCopyp = prep->clonep();
                prepCopyp->replaceWith(جديد AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ادفع_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ثابت newp = جديد AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(true);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(ثابت Ast_عقدة_تعبير* ثابت nodep) {
        AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        أرجع varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ساكن منطقي isStaticlyGte(AstConst* ثابت msbConstp, ثابت Ast_عقدة_تعبير* ثابت exprp) {
        إذا (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->رقم().toSInt() >= (1 << exprp->width()) - 1) {
            أرجع true;
        }
        إذا (ثابت AstConst* ثابت constp = VN_CAST(exprp, Const)) {
            إذا (V3_رقم{msbConstp}.opGte(msbConstp->رقم(), constp->رقم()).isNeqZero()) {
                أرجع true;
            }
        }
        أرجع خطأ;
    }

    Ast_عقدة_تعبير* new_تعبير_Stmt_Or_Clone(Ast_عقدة_تعبير*& exprp) {
        إذا (!exprp->isPure()) {
            AstVar* ثابت varp = createAddTemp(exprp);
            File_سطر* ثابت fl = exprp->fileline();
            exprp = جديد Ast_تعبير_Stmt{
                fl, جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                جديد AstVarRef{fl, varp, VAccess::READ}};
            أرجع جديد AstVarRef{fl, varp, VAccess::READ};
        }
        أرجع exprp->clone_شجرة_Pure(خطأ);
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = true;
            // Class X randomization causes Vxrand in strange places, so disable
            إذا (VN_IS(nodep, Class)) m_allowXUnique = خطأ;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    فراغ visit(Ast_عقدة_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = خطأ;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = true;
        iterateAndNextNull(nodep->stmtsp());
    }
    فراغ visit(Ast_عقدة_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = خطأ;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        إذا (nodep->isParam()) m_allowXUnique = خطأ;
        iterateChildren(nodep);
    }
    فراغ visitEqNeqCase(Ast_عقدة_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        إذا (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            أرجع;
        } وإلا {
            Ast_عقدة_تعبير* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            إذا (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->رقم().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->رقم().isFourState()))) {
                newp = جديد AstConst(nodep->fileline(), AstConst::Widthed_قيمة{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_شجرة(), lhsp);
                VL_DO_DANGLING(rhsp->delete_شجرة(), rhsp);
            } وإلا {
                إذا (VN_IS(nodep, EqCase)) {
                    newp = جديد AstEq{nodep->fileline(), lhsp, rhsp};
                } وإلا {
                    newp = جديد AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    فراغ visitEqNeqWild(Ast_عقدة_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        إذا (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            أرجع;
        } وإلا {
            Ast_عقدة_تعبير* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_عقدة_تعبير* newp;
            إذا (!VN_IS(rhsp, Const)) {
                إذا (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = جديد AstEq{nodep->fileline(), lhsp, rhsp};
            } وإلا {
                // X or Z's become mask, ala case statements.
                V3_رقم nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->رقم());
                V3_رقم numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->رقم());
                Ast_عقدة_تعبير* ثابت and1p = جديد AstAnd{nodep->fileline(), lhsp,
                                                      جديد AstConst{nodep->fileline(), nummask}};
                Ast_عقدة_تعبير* ثابت and2p = جديد AstConst{nodep->fileline(), numval};
                إذا (VN_IS(nodep, EqWild)) {
                    newp = جديد AstEq{nodep->fileline(), and1p, and2p};
                } وإلا {
                    newp = جديد AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_شجرة(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
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
        VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
    }
    فراغ visit(Ast_عدد_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<منطقي, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->رقم().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->رقم().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->رقم().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_عقدة_تعبير* nonXp = nullptr;
        إذا (!dropop[0]) {
            nonXp = nodep->rhsp();
        } وإلا إذا (!dropop[1]) {
            nonXp = nodep->thsp();
        } وإلا إذا (!dropop[2]) {
            nonXp = nodep->fhsp();
        } وإلا {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
            أرجع;
        }
        إذا (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_شجرة();
            nodep->rhsp(nonXp->clone_شجرة_Pure(true));
        }
        إذا (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_شجرة();
            nodep->thsp(nonXp->clone_شجرة_Pure(true));
        }
        إذا (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_شجرة();
            nodep->fhsp(nonXp->clone_شجرة_Pure(true));
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstConst* nodep) override {
        إذا (m_constXCvt && nodep->رقم().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_رقم numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->رقم());
            V3_رقم numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->رقم());
            إذا (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_رقم numnew{nodep, numb1.width()};
                إذا (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } وإلا {
                    numnew.opAssign(numb1);
                }
                AstConst* ثابت newp = جديد AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
                UINFO(4, "   -> " << newp);
            } وإلا {
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
                Ast_عقدة_Var_Ref* ثابت newref1p
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
                Ast_عقدة* ثابت newStmtsp = Ast_عقدة::addNext<Ast_عقدة, Ast_عقدة>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
            }
        }
    }

    فراغ visit(AstSel* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            ثابت Ast_عقدة* ثابت basefromp = nodep->baseFromp(true);
            منطقي lvalue = خطأ;
            إذا (ثابت Ast_عقدة_Var_Ref* ثابت varrefp = VN_CAST(basefromp, عقدة_Var_Ref)) {
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
                nodep->fileline(), AstConst::Widthed_قيمة{}, nodep->lsbp()->width(), maxmsb};
            Ast_عقدة_تعبير* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            إذا (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_شجرة(), maxmsbConstp);
                nodep->lsbp(lsbp);
                أرجع;
            }
            nodep->lsbp(new_تعبير_Stmt_Or_Clone(lsbp));
            Ast_عقدة_تعبير* condp
                = V3Const::constifyEdit(جديد AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            إذا (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_رقم xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_عقدة_تعبير* ثابت xexprp = جديد AstConst{nodep->fileline(), xnum};
                Ast_عقدة_تعبير* ثابت newp = [&]() -> Ast_عقدة_تعبير* {
                    إذا (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_شجرة(), condp);
                        VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
                        أرجع xexprp;
                    }
                    أرجع جديد AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } وإلا {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    فراغ visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_عقدة* ثابت basefromp = nodep->fromp()->baseFromp(true);
            منطقي lvalue = خطأ;
            إذا (ثابت Ast_عقدة_Var_Ref* ثابت varrefp = VN_CAST(basefromp, عقدة_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } وإلا إذا (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } وإلا {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            صحيح declElements = -1;
            Ast_عقدة_DType* ثابت dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ثابت Ast_عقدة_Array_DType* ثابت adtypep = VN_CAST(dtypep, عقدة_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            إذا (ثابت AstModDiv* ثابت moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                إذا (ثابت AstConst* ثابت modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    إذا (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        أرجع;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ثابت declElementsp
                = جديد AstConst{nodep->fileline(), AstConst::Widthed_قيمة{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_عقدة_تعبير* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            إذا (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_شجرة(), declElementsp);
                nodep->bitp(bitp);
                أرجع;
            }
            nodep->bitp(new_تعبير_Stmt_Or_Clone(bitp));
            Ast_عقدة_تعبير* condp = جديد AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ثابت Ast_عقدة_DType* ثابت عقدة_Dtp = nodep->dtypep()->skipRefp();
            إذا (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(عقدة_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_رقم xnum{nodep, nodep->width()};
                إذا (عقدة_Dtp->isDouble()) {
                    xnum = V3_رقم{nodep, V3_رقم::Double{}, 0.0};
                } وإلا إذا (عقدة_Dtp->isString()) {
                    xnum = V3_رقم{nodep, V3_رقم::String{}, ""};
                } وإلا {
                    إذا (عقدة_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } وإلا {
                        xnum.setAllBits0();
                    }
                }
                Ast_عقدة* ثابت newp = جديد AstCond{nodep->fileline(), condp, nodep,
                                                  جديد AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } وإلا إذا (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_عقدة_تعبير* ثابت asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_عقدة_تعبير* ثابت newp
                    = جديد AstCond{asBitp->fileline(), condp, asBitp,
                                  جديد AstConst{asBitp->fileline(), AstConst::Widthed_قيمة{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } وإلا {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_شجرة("unknown", 0, dump_شجرة_Either_Level() >= 3);
}
