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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Unknown.h"

#شامل "V3Const.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

رده UnknownVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ایستا ثابت std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گره_Module* m_modp = nullptr;  // Current module
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    منطقی m_constXCvt = نادرست;  // Convert X's
    منطقی m_allowXUnique = درست;  // Allow unique assignments

    // METHODS

    تهی addVar(AstVar* varp) {
        اگر (m_ftaskp) {
            varp->funcLocal(درست);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } وگرنه {
            m_modp->prependStmtsp(varp);
        }
    }

    تهی replaceBoundLvalue(Ast_گره_عبارت* nodep, Ast_گره_عبارت* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_گره_عبارت* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        وقتی (VN_IS(prep->backp(), گره_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), گره_عبارت);
        }
        اگر (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_درخت(), condp);
            بازگشت;
        }
        File_سطر* ثابت fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        اگر (ثابت AstIf* ثابت ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_گره_عبارت* ثابت earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_گره_عبارت* ثابت newp = جدید AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } وگرنه {
            Ast_گره* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            وقتی (!VN_IS(stmtp, گره_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_گره* ثابت origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_گره* elseStmtp = nullptr;
            ثابت منطقی hasSideEffects = origStmtp->exists(
                [](Ast_گره* ثابت np) { بازگشت !np->isPure() || np->isTimingControl(); });
            اگر (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_درخت(نادرست);
                AstVar* ثابت varp = جدید AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_گره* ثابت prepCopyp = prep->clonep();
                prepCopyp->replaceWith(جدید AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(افزودن_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ثابت newp = جدید AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(درست);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(ثابت Ast_گره_عبارت* ثابت nodep) {
        AstVar* ثابت varp = جدید AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        بازگشت varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ایستا منطقی isStaticlyGte(AstConst* ثابت msbConstp, ثابت Ast_گره_عبارت* ثابت exprp) {
        اگر (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->رقم().toSInt() >= (1 << exprp->width()) - 1) {
            بازگشت درست;
        }
        اگر (ثابت AstConst* ثابت constp = VN_CAST(exprp, Const)) {
            اگر (V3_عدد{msbConstp}.opGte(msbConstp->رقم(), constp->رقم()).isNeqZero()) {
                بازگشت درست;
            }
        }
        بازگشت نادرست;
    }

    Ast_گره_عبارت* new_عبارت_Stmt_Or_Clone(Ast_گره_عبارت*& exprp) {
        اگر (!exprp->isPure()) {
            AstVar* ثابت varp = createAddTemp(exprp);
            File_سطر* ثابت fl = exprp->fileline();
            exprp = جدید Ast_عبارت_Stmt{
                fl, جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                جدید AstVarRef{fl, varp, VAccess::READ}};
            بازگشت جدید AstVarRef{fl, varp, VAccess::READ};
        }
        بازگشت exprp->clone_درخت_Pure(نادرست);
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = درست;
            // Class X randomization causes Vxrand in strange places, so disable
            اگر (VN_IS(nodep, Class)) m_allowXUnique = نادرست;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = نادرست;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = درست;
        iterateAndNextNull(nodep->stmtsp());
    }
    تهی visit(Ast_گره_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = نادرست;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    تهی visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        اگر (nodep->isParam()) m_allowXUnique = نادرست;
        iterateChildren(nodep);
    }
    تهی visitEqNeqCase(Ast_گره_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        اگر (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            بازگشت;
        } وگرنه {
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_گره_عبارت* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            اگر (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->رقم().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->رقم().isFourState()))) {
                newp = جدید AstConst(nodep->fileline(), AstConst::Widthed_مقدار{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_درخت(), lhsp);
                VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
            } وگرنه {
                اگر (VN_IS(nodep, EqCase)) {
                    newp = جدید AstEq{nodep->fileline(), lhsp, rhsp};
                } وگرنه {
                    newp = جدید AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    تهی visitEqNeqWild(Ast_گره_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        اگر (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            بازگشت;
        } وگرنه {
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_گره_عبارت* newp;
            اگر (!VN_IS(rhsp, Const)) {
                اگر (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = جدید AstEq{nodep->fileline(), lhsp, rhsp};
            } وگرنه {
                // X or Z's become mask, ala case statements.
                V3_عدد nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->رقم());
                V3_عدد numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->رقم());
                Ast_گره_عبارت* ثابت and1p = جدید AstAnd{nodep->fileline(), lhsp,
                                                      جدید AstConst{nodep->fileline(), nummask}};
                Ast_گره_عبارت* ثابت and2p = جدید AstConst{nodep->fileline(), numval};
                اگر (VN_IS(nodep, EqWild)) {
                    newp = جدید AstEq{nodep->fileline(), and1p, and2p};
                } وگرنه {
                    newp = جدید AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    تهی visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    تهی visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    تهی visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    تهی visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    تهی visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ثابت newp = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(Ast_شمار_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<منطقی, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->رقم().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->رقم().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->رقم().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_گره_عبارت* nonXp = nullptr;
        اگر (!dropop[0]) {
            nonXp = nodep->rhsp();
        } وگرنه اگر (!dropop[1]) {
            nonXp = nodep->thsp();
        } وگرنه اگر (!dropop[2]) {
            nonXp = nodep->fhsp();
        } وگرنه {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ثابت newp = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        }
        اگر (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_درخت();
            nodep->rhsp(nonXp->clone_درخت_Pure(درست));
        }
        اگر (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_درخت();
            nodep->thsp(nonXp->clone_درخت_Pure(درست));
        }
        اگر (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_درخت();
            nodep->fhsp(nonXp->clone_درخت_Pure(درست));
        }
        iterateChildren(nodep);
    }
    تهی visit(AstConst* nodep) override {
        اگر (m_constXCvt && nodep->رقم().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_عدد numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->رقم());
            V3_عدد numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->رقم());
            اگر (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_عدد numnew{nodep, numb1.width()};
                اگر (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } وگرنه {
                    numnew.opAssign(numb1);
                }
                AstConst* ثابت newp = جدید AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                UINFO(4, "   -> " << newp);
            } وگرنه {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ثابت newvarp
                    = جدید AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_گره_Var_Ref* ثابت newref1p
                    = جدید AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ثابت newinitp = جدید AstInitial{
                    nodep->fileline(),
                    جدید AstAssign{
                        nodep->fileline(),
                        جدید AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        جدید AstOr{nodep->fileline(), جدید AstConst{nodep->fileline(), numb1},
                                  جدید AstAnd{
                                      nodep->fileline(), جدید AstConst{nodep->fileline(), numbx},
                                      جدید AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_گره* ثابت newStmtsp = Ast_گره::addNext<Ast_گره, Ast_گره>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            }
        }
    }

    تهی visit(AstSel* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            ثابت Ast_گره* ثابت basefromp = nodep->baseFromp(درست);
            منطقی lvalue = نادرست;
            اگر (ثابت Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(basefromp, گره_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ثابت uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ثابت maxmsbConstp = جدید AstConst{
                nodep->fileline(), AstConst::Widthed_مقدار{}, nodep->lsbp()->width(), maxmsb};
            Ast_گره_عبارت* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            اگر (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_درخت(), maxmsbConstp);
                nodep->lsbp(lsbp);
                بازگشت;
            }
            nodep->lsbp(new_عبارت_Stmt_Or_Clone(lsbp));
            Ast_گره_عبارت* condp
                = V3Const::constifyEdit(جدید AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            اگر (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_عدد xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_گره_عبارت* ثابت xexprp = جدید AstConst{nodep->fileline(), xnum};
                Ast_گره_عبارت* ثابت newp = [&]() -> Ast_گره_عبارت* {
                    اگر (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_درخت(), condp);
                        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                        بازگشت xexprp;
                    }
                    بازگشت جدید AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } وگرنه {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    تهی visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_گره* ثابت basefromp = nodep->fromp()->baseFromp(درست);
            منطقی lvalue = نادرست;
            اگر (ثابت Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(basefromp, گره_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } وگرنه اگر (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } وگرنه {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            صحیح declElements = -1;
            Ast_گره_DType* ثابت dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(dtypep, گره_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            اگر (ثابت AstModDiv* ثابت moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                اگر (ثابت AstConst* ثابت modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    اگر (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        بازگشت;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ثابت declElementsp
                = جدید AstConst{nodep->fileline(), AstConst::Widthed_مقدار{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_گره_عبارت* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            اگر (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_درخت(), declElementsp);
                nodep->bitp(bitp);
                بازگشت;
            }
            nodep->bitp(new_عبارت_Stmt_Or_Clone(bitp));
            Ast_گره_عبارت* condp = جدید AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ثابت Ast_گره_DType* ثابت گره_Dtp = nodep->dtypep()->skipRefp();
            اگر (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(گره_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_عدد xnum{nodep, nodep->width()};
                اگر (گره_Dtp->isDouble()) {
                    xnum = V3_عدد{nodep, V3_عدد::Double{}, 0.0};
                } وگرنه اگر (گره_Dtp->isString()) {
                    xnum = V3_عدد{nodep, V3_عدد::String{}, ""};
                } وگرنه {
                    اگر (گره_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } وگرنه {
                        xnum.setAllBits0();
                    }
                }
                Ast_گره* ثابت newp = جدید AstCond{nodep->fileline(), condp, nodep,
                                                  جدید AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } وگرنه اگر (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_گره_عبارت* ثابت asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_گره_عبارت* ثابت newp
                    = جدید AstCond{asBitp->fileline(), condp, asBitp,
                                  جدید AstConst{asBitp->fileline(), AstConst::Widthed_مقدار{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } وگرنه {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
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

تهی V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("unknown", 0, dump_درخت_Either_Level() >= 3);
}
