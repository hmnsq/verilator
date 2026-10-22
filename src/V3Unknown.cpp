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
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;
    ساكن ثابت std::string s_xrand_مختاړی;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3_Unique_نومونه m_lvbound_نومونه;  // For generating unique temporary variable names
    std::unique_ptr<V3_Unique_نومونه> m_xrand_نومونه;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current function/task
    منطقي m_constXCvt = خطأ;  // Convert X's
    منطقي m_allowXUnique = ریښتیا;  // Allow unique assignments

    // METHODS

    فراغ جمع_کړه_Var(AstVar* varp) {
        إذا (m_ftaskp) {
            varp->funcLocal(ریښتیا);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } وإلا {
            m_modp->prependStmtsp(varp);
        }
    }

    فراغ replaceBoundLvalue(Ast_غوټه_بیان* nodep, Ast_غوټه_بیان* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_غوټه_بیان* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        طالما (VN_IS(prep->backp(), غوټه_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), غوټه_بیان);
        }
        إذا (VN_IS(prep->backp(), ټاکل_Force) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_ونه(), condp);
            أرجع;
        }
        File_کرښه* ثابت fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        إذا (ثابت AstIf* ثابت ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_غوټه_بیان* ثابت earliercondp = ifp->condp()->unlink_Fr_شا(&replaceHandle);
            Ast_غوټه_بیان* ثابت newp = جديد AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } وإلا {
            Ast_غوټه* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            طالما (!VN_IS(stmtp, غوټه_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_غوټه* ثابت origStmtp = stmtp->unlink_Fr_شا(&replaceHandle);
            Ast_غوټه* elseStmtp = تش_اشاره;
            ثابت منطقي hasSideEffects = origStmtp->exists(
                [](Ast_غوټه* ثابت np) { أرجع !np->is_خالص() || np->isTimingControl(); });
            إذا (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_ونه(خطأ);
                AstVar* ثابت varp = جديد AstVar{fl, VVarType::MODULETEMP, m_lvbound_نومونه.get(prep),
                                                prep->dtypep()};
                جمع_کړه_Var(varp);
                Ast_غوټه* ثابت prepCopyp = prep->clonep();
                prepCopyp->replaceWith(جديد AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ټېل_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ثابت newp = جديد AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(ریښتیا);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* create_جمع_کړه_لنډمهاله(ثابت Ast_غوټه_بیان* ثابت nodep) {
        AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrand_نومونه->get(nodep), nodep->dtypep()};
        جمع_کړه_Var(varp);
        أرجع varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ساكن منطقي isStaticlyGte(AstConst* ثابت msbConstp, ثابت Ast_غوټه_بیان* ثابت exprp) {
        إذا (msbConstp->پلنوالی() >= exprp->پلنوالی() && exprp->پلنوالی() < 32
            && msbConstp->عدد().منزل_SInt() >= (1 << exprp->پلنوالی()) - 1) {
            أرجع ریښتیا;
        }
        إذا (ثابت AstConst* ثابت constp = VN_CAST(exprp, Const)) {
            إذا (V3_شمېره{msbConstp}.opGte(msbConstp->عدد(), constp->عدد()).isNeqZero()) {
                أرجع ریښتیا;
            }
        }
        أرجع خطأ;
    }

    Ast_غوټه_بیان* new_بیان_Stmt_Or_Clone(Ast_غوټه_بیان*& exprp) {
        إذا (!exprp->is_خالص()) {
            AstVar* ثابت varp = create_جمع_کړه_لنډمهاله(exprp);
            File_کرښه* ثابت fl = exprp->fileline();
            exprp = جديد Ast_بیان_Stmt{
                fl, جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                جديد AstVarRef{fl, varp, VAccess::READ}};
            أرجع جديد AstVarRef{fl, varp, VAccess::READ};
        }
        أرجع exprp->clone_ونه_خالص(خطأ);
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        خپلکاری xrand_نومونه = std::make_unique<V3_Unique_نومونه>(s_xrand_مختاړی);
        {
            m_modp = nodep;
            m_constXCvt = ریښتیا;
            // Class X randomization causes Vxrand in strange places, so disable
            إذا (VN_IS(nodep, Class)) m_allowXUnique = خطأ;
            m_lvbound_نومونه.reset();
            xrand_نومونه.بدلون(m_xrand_نومونه);
            iterateChildren(nodep);
            xrand_نومونه.بدلون(m_xrand_نومونه);
        }
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Case_توکی* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = خطأ;  // Avoid losing the X's in casex
        iterate_And_بل_Null(nodep->condsp());
        m_constXCvt = ریښتیا;
        iterate_And_بل_Null(nodep->stmtsp());
    }
    فراغ لیدنه(Ast_غوټه_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = خطأ;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        إذا (nodep->isParam()) m_allowXUnique = خطأ;
        iterateChildren(nodep);
    }
    فراغ لیدنه_Eq_Neq_Case(Ast_غوټه_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        إذا (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            أرجع;
        } وإلا {
            Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            إذا (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->عدد().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->عدد().isFourState()))) {
                newp = جديد AstConst(nodep->fileline(), AstConst::Widthed_ارزښت{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_ونه(), lhsp);
                VL_DO_DANGLING(rhsp->delete_ونه(), rhsp);
            } وإلا {
                إذا (VN_IS(nodep, EqCase)) {
                    newp = جديد AstEq{nodep->fileline(), lhsp, rhsp};
                } وإلا {
                    newp = جديد AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    فراغ لیدنه_Eq_Neq_Wild(Ast_غوټه_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        إذا (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            أرجع;
        } وإلا {
            Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* newp;
            إذا (!VN_IS(rhsp, Const)) {
                إذا (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = جديد AstEq{nodep->fileline(), lhsp, rhsp};
            } وإلا {
                // X or Z's become mask, ala case statements.
                V3_شمېره nummask{rhsp, rhsp->پلنوالی()};
                nummask.op_بټونه_Non_XZ(VN_AS(rhsp, Const)->عدد());
                V3_شمېره numval{rhsp, rhsp->پلنوالی()};
                numval.op_بټونه_One(VN_AS(rhsp, Const)->عدد());
                Ast_غوټه_بیان* ثابت and1p = جديد AstAnd{nodep->fileline(), lhsp,
                                                      جديد AstConst{nodep->fileline(), nummask}};
                Ast_غوټه_بیان* ثابت and2p = جديد AstConst{nodep->fileline(), numval};
                إذا (VN_IS(nodep, EqWild)) {
                    newp = جديد AstEq{nodep->fileline(), and1p, and2p};
                } وإلا {
                    newp = جديد AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_ونه(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    فراغ لیدنه(AstEqCase* nodep) override { لیدنه_Eq_Neq_Case(nodep); }
    فراغ لیدنه(AstNeqCase* nodep) override { لیدنه_Eq_Neq_Case(nodep); }
    فراغ لیدنه(AstEqWild* nodep) override { لیدنه_Eq_Neq_Wild(nodep); }
    فراغ لیدنه(AstNeqWild* nodep) override { لیدنه_Eq_Neq_Wild(nodep); }
    فراغ لیدنه(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_شمېر_بټونه* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<منطقي, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->عدد().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->عدد().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->عدد().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_غوټه_بیان* nonXp = تش_اشاره;
        إذا (!dropop[0]) {
            nonXp = nodep->rhsp();
        } وإلا إذا (!dropop[1]) {
            nonXp = nodep->thsp();
        } وإلا إذا (!dropop[2]) {
            nonXp = nodep->fhsp();
        } وإلا {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        }
        إذا (dropop[0]) {
            nodep->rhsp()->unlink_Fr_شا()->delete_ونه();
            nodep->rhsp(nonXp->clone_ونه_خالص(ریښتیا));
        }
        إذا (dropop[1]) {
            nodep->thsp()->unlink_Fr_شا()->delete_ونه();
            nodep->thsp(nonXp->clone_ونه_خالص(ریښتیا));
        }
        إذا (dropop[2]) {
            nodep->fhsp()->unlink_Fr_شا()->delete_ونه();
            nodep->fhsp(nonXp->clone_ونه_خالص(ریښتیا));
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstConst* nodep) override {
        إذا (m_constXCvt && nodep->عدد().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_شمېره numb1{nodep, nodep->پلنوالی()};
            numb1.op_بټونه_One(nodep->عدد());
            V3_شمېره numbx{nodep, nodep->پلنوالی()};
            numbx.op_بټونه_XZ(nodep->عدد());
            إذا (!m_allowXUnique || v3Global.opt.x_ټاکل() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_شمېره numnew{nodep, numb1.پلنوالی()};
                إذا (v3Global.opt.x_ټاکل() == "1") {
                    numnew.opOr(numb1, numbx);
                } وإلا {
                    numnew.op_ټاکل(numb1);
                }
                AstConst* ثابت newp = جديد AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                UINFO(4, "   -> " << newp);
            } وإلا {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ثابت newvarp
                    = جديد AstVar{nodep->fileline(), VVarType::XTEMP, m_xrand_نومونه->get(تش_اشاره),
                                 VFlagLogicPacked{}, nodep->پلنوالی()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlink_Fr_شا(&replaceHandle);
                Ast_غوټه_Var_Ref* ثابت newref1p
                    = جديد AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ثابت newinitp = جديد AstInitial{
                    nodep->fileline(),
                    جديد Ast_ټاکل{
                        nodep->fileline(),
                        جديد AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        جديد AstOr{nodep->fileline(), جديد AstConst{nodep->fileline(), numb1},
                                  جديد AstAnd{
                                      nodep->fileline(), جديد AstConst{nodep->fileline(), numbx},
                                      جديد AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_غوټه* ثابت newStmtsp = Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            }
        }
    }

    فراغ لیدنه(AstSel* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->user1_سېټ_Once()) {
            // Guard against reading/writing past end of bit vector array
            ثابت Ast_غوټه* ثابت basefromp = nodep->بنسټ_Fromp(ریښتیا);
            منطقي lvalue = خطأ;
            إذا (ثابت Ast_غوټه_Var_Ref* ثابت varrefp = VN_CAST(basefromp, غوټه_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ثابت uint32_t maxmsb = nodep->fromp()->dtypep()->پلنوالی() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ثابت maxmsbConstp = جديد AstConst{
                nodep->fileline(), AstConst::Widthed_ارزښت{}, nodep->lsbp()->پلنوالی(), maxmsb};
            Ast_غوټه_بیان* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlink_Fr_شا());
            إذا (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_ونه(), maxmsbConstp);
                nodep->lsbp(lsbp);
                أرجع;
            }
            nodep->lsbp(new_بیان_Stmt_Or_Clone(lsbp));
            Ast_غوټه_بیان* condp
                = V3Const::constifyEdit(جديد AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            إذا (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_شا(&replaceHandle);
                V3_شمېره xnum{nodep, nodep->پلنوالی()};
                xnum.سېټ_All_بټونه_X();
                Ast_غوټه_بیان* ثابت xexprp = جديد AstConst{nodep->fileline(), xnum};
                Ast_غوټه_بیان* ثابت newp = [&]() -> Ast_غوټه_بیان* {
                    إذا (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_ونه(), condp);
                        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
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

    فراغ لیدنه(AstArraySel* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->user1_سېټ_Once()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_غوټه* ثابت basefromp = nodep->fromp()->بنسټ_Fromp(ریښتیا);
            منطقي lvalue = خطأ;
            إذا (ثابت Ast_غوټه_Var_Ref* ثابت varrefp = VN_CAST(basefromp, غوټه_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } وإلا إذا (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } وإلا {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            صحيح declElements = -1;
            Ast_غوټه_DType* ثابت dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ثابت Ast_غوټه_Array_DType* ثابت adtypep = VN_CAST(dtypep, غوټه_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            إذا (ثابت AstModDiv* ثابت moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                إذا (ثابت AstConst* ثابت modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    إذا (modconstp->پلنوالی() <= 32
                        && modconstp->منزل_UInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->منزل_UInt());
                        أرجع;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ثابت declElementsp
                = جديد AstConst{nodep->fileline(), AstConst::Widthed_ارزښت{}, nodep->bitp()->پلنوالی(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_غوټه_بیان* bitp = V3Const::constifyEdit(nodep->bitp()->unlink_Fr_شا());
            إذا (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_ونه(), declElementsp);
                nodep->bitp(bitp);
                أرجع;
            }
            nodep->bitp(new_بیان_Stmt_Or_Clone(bitp));
            Ast_غوټه_بیان* condp = جديد AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ثابت Ast_غوټه_DType* ثابت غوټه_Dtp = nodep->dtypep()->skipRefp();
            إذا (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(غوټه_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_شا(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_شمېره xnum{nodep, nodep->پلنوالی()};
                إذا (غوټه_Dtp->isDouble()) {
                    xnum = V3_شمېره{nodep, V3_شمېره::Double{}, 0.0};
                } وإلا إذا (غوټه_Dtp->isString()) {
                    xnum = V3_شمېره{nodep, V3_شمېره::String{}, ""};
                } وإلا {
                    إذا (غوټه_Dtp->isFourstate()) {
                        xnum.سېټ_All_بټونه_X();
                    } وإلا {
                        xnum.سېټ_All_Bits0();
                    }
                }
                Ast_غوټه* ثابت newp = جديد AstCond{nodep->fileline(), condp, nodep,
                                                  جديد AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } وإلا إذا (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_غوټه_بیان* ثابت asBitp = nodep->bitp()->unlink_Fr_شا(&replaceHandle);
                Ast_غوټه_بیان* ثابت newp
                    = جديد AstCond{asBitp->fileline(), condp, asBitp,
                                  جديد AstConst{asBitp->fileline(), AstConst::Widthed_ارزښت{},
                                               asBitp->پلنوالی(), 0}};
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
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند UnknownVisitor(AstNetlist* nodep)
        : m_lvbound_نومونه{"__Vlvbound"}
        , m_xrand_نومونه{std::make_unique<V3_Unique_نومونه>(s_xrand_مختاړی)} {
        iterate(nodep);
    }
    ~UnknownVisitor() override {  //
        V3Stats::جمع_کړه_Stat("Unknowns, variables created", m_statUnkVars);
        V3Stats::جمع_کړه_Stat("Unknowns, else branches created", m_statElses);
    }
};

ثابت std::string UnknownVisitor::s_xrand_مختاړی = "__Vxrand";

//######################################################################
// Unknown class functions

فراغ V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("unknown", 0, dump_ونه_Either_Level() >= 3);
}
