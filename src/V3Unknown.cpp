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

جماعت UnknownVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;
    ساکن ثابت std::string s_xrand_سابقہ;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3_Unique_نام m_lvbound_نام;  // For generating unique temporary variable names
    std::unique_ptr<V3_Unique_نام> m_xrand_نام;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current function/task
    منطقی m_constXCvt = جھوٹ;  // Convert X's
    منطقی m_allowXUnique = سچ;  // Allow unique assignments

    // METHODS

    خالی جوڑو_Var(AstVar* varp) {
        اگر (m_ftaskp) {
            varp->funcLocal(سچ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } ورنہ {
            m_modp->prependStmtsp(varp);
        }
    }

    خالی replaceBoundLvalue(Ast_گرہ_اظہاریہ* nodep, Ast_گرہ_اظہاریہ* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_گرہ_اظہاریہ* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        جب (VN_IS(prep->backp(), گرہ_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), گرہ_اظہاریہ);
        }
        اگر (VN_IS(prep->backp(), تفویض_Force) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_درخت(), condp);
            واپس;
        }
        File_سطر* ثابت fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        اگر (ثابت AstIf* ثابت ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_گرہ_اظہاریہ* ثابت earliercondp = ifp->condp()->unlink_Fr_عقب(&replaceHandle);
            Ast_گرہ_اظہاریہ* ثابت newp = نیا AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } ورنہ {
            Ast_گرہ* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            جب (!VN_IS(stmtp, گرہ_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_گرہ* ثابت origStmtp = stmtp->unlink_Fr_عقب(&replaceHandle);
            Ast_گرہ* elseStmtp = خالی_اشاریہ;
            ثابت منطقی hasSideEffects = origStmtp->exists(
                [](Ast_گرہ* ثابت np) { واپس !np->is_خالص() || np->isTimingControl(); });
            اگر (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_درخت(جھوٹ);
                AstVar* ثابت varp = نیا AstVar{fl, VVarType::MODULETEMP, m_lvbound_نام.get(prep),
                                                prep->dtypep()};
                جوڑو_Var(varp);
                Ast_گرہ* ثابت prepCopyp = prep->clonep();
                prepCopyp->replaceWith(نیا AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ڈالو_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ثابت newp = نیا AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(سچ);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* create_جوڑو_عارضی(ثابت Ast_گرہ_اظہاریہ* ثابت nodep) {
        AstVar* ثابت varp = نیا AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrand_نام->get(nodep), nodep->dtypep()};
        جوڑو_Var(varp);
        واپس varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ساکن منطقی isStaticlyGte(AstConst* ثابت msbConstp, ثابت Ast_گرہ_اظہاریہ* ثابت exprp) {
        اگر (msbConstp->چوڑائی() >= exprp->چوڑائی() && exprp->چوڑائی() < 32
            && msbConstp->عدد().مقصد_SInt() >= (1 << exprp->چوڑائی()) - 1) {
            واپس سچ;
        }
        اگر (ثابت AstConst* ثابت constp = VN_CAST(exprp, Const)) {
            اگر (V3_نمبر{msbConstp}.opGte(msbConstp->عدد(), constp->عدد()).isNeqZero()) {
                واپس سچ;
            }
        }
        واپس جھوٹ;
    }

    Ast_گرہ_اظہاریہ* new_اظہاریہ_Stmt_Or_Clone(Ast_گرہ_اظہاریہ*& exprp) {
        اگر (!exprp->is_خالص()) {
            AstVar* ثابت varp = create_جوڑو_عارضی(exprp);
            File_سطر* ثابت fl = exprp->fileline();
            exprp = نیا Ast_اظہاریہ_Stmt{
                fl, نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                نیا AstVarRef{fl, varp, VAccess::READ}};
            واپس نیا AstVarRef{fl, varp, VAccess::READ};
        }
        واپس exprp->clone_درخت_خالص(جھوٹ);
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        خودکار xrand_نام = std::make_unique<V3_Unique_نام>(s_xrand_سابقہ);
        {
            m_modp = nodep;
            m_constXCvt = سچ;
            // Class X randomization causes Vxrand in strange places, so disable
            اگر (VN_IS(nodep, Class)) m_allowXUnique = جھوٹ;
            m_lvbound_نام.reset();
            xrand_نام.تبادلہ(m_xrand_نام);
            iterateChildren(nodep);
            xrand_نام.تبادلہ(m_xrand_نام);
        }
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Case_شے* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = جھوٹ;  // Avoid losing the X's in casex
        iterate_And_اگلا_Null(nodep->condsp());
        m_constXCvt = سچ;
        iterate_And_اگلا_Null(nodep->stmtsp());
    }
    خالی دورہ(Ast_گرہ_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = جھوٹ;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    خالی دورہ(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        اگر (nodep->isParam()) m_allowXUnique = جھوٹ;
        iterateChildren(nodep);
    }
    خالی دورہ_Eq_Neq_Case(Ast_گرہ_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        اگر (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            واپس;
        } ورنہ {
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            اگر (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->عدد().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->عدد().isFourState()))) {
                newp = نیا AstConst(nodep->fileline(), AstConst::Widthed_قدر{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_درخت(), lhsp);
                VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
            } ورنہ {
                اگر (VN_IS(nodep, EqCase)) {
                    newp = نیا AstEq{nodep->fileline(), lhsp, rhsp};
                } ورنہ {
                    newp = نیا AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    خالی دورہ_Eq_Neq_Wild(Ast_گرہ_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        اگر (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            واپس;
        } ورنہ {
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
            Ast_گرہ_اظہاریہ* newp;
            اگر (!VN_IS(rhsp, Const)) {
                اگر (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = نیا AstEq{nodep->fileline(), lhsp, rhsp};
            } ورنہ {
                // X or Z's become mask, ala case statements.
                V3_نمبر nummask{rhsp, rhsp->چوڑائی()};
                nummask.op_بٹ_Non_XZ(VN_AS(rhsp, Const)->عدد());
                V3_نمبر numval{rhsp, rhsp->چوڑائی()};
                numval.op_بٹ_One(VN_AS(rhsp, Const)->عدد());
                Ast_گرہ_اظہاریہ* ثابت and1p = نیا AstAnd{nodep->fileline(), lhsp,
                                                      نیا AstConst{nodep->fileline(), nummask}};
                Ast_گرہ_اظہاریہ* ثابت and2p = نیا AstConst{nodep->fileline(), numval};
                اگر (VN_IS(nodep, EqWild)) {
                    newp = نیا AstEq{nodep->fileline(), and1p, and2p};
                } ورنہ {
                    newp = نیا AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    خالی دورہ(AstEqCase* nodep) override { دورہ_Eq_Neq_Case(nodep); }
    خالی دورہ(AstNeqCase* nodep) override { دورہ_Eq_Neq_Case(nodep); }
    خالی دورہ(AstEqWild* nodep) override { دورہ_Eq_Neq_Wild(nodep); }
    خالی دورہ(AstNeqWild* nodep) override { دورہ_Eq_Neq_Wild(nodep); }
    خالی دورہ(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ثابت newp = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(Ast_گنتی_بٹ* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<منطقی, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->عدد().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->عدد().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->عدد().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_گرہ_اظہاریہ* nonXp = خالی_اشاریہ;
        اگر (!dropop[0]) {
            nonXp = nodep->rhsp();
        } ورنہ اگر (!dropop[1]) {
            nonXp = nodep->thsp();
        } ورنہ اگر (!dropop[2]) {
            nonXp = nodep->fhsp();
        } ورنہ {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ثابت newp = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            واپس;
        }
        اگر (dropop[0]) {
            nodep->rhsp()->unlink_Fr_عقب()->delete_درخت();
            nodep->rhsp(nonXp->clone_درخت_خالص(سچ));
        }
        اگر (dropop[1]) {
            nodep->thsp()->unlink_Fr_عقب()->delete_درخت();
            nodep->thsp(nonXp->clone_درخت_خالص(سچ));
        }
        اگر (dropop[2]) {
            nodep->fhsp()->unlink_Fr_عقب()->delete_درخت();
            nodep->fhsp(nonXp->clone_درخت_خالص(سچ));
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstConst* nodep) override {
        اگر (m_constXCvt && nodep->عدد().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_نمبر numb1{nodep, nodep->چوڑائی()};
            numb1.op_بٹ_One(nodep->عدد());
            V3_نمبر numbx{nodep, nodep->چوڑائی()};
            numbx.op_بٹ_XZ(nodep->عدد());
            اگر (!m_allowXUnique || v3Global.opt.x_تفویض() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_نمبر numnew{nodep, numb1.چوڑائی()};
                اگر (v3Global.opt.x_تفویض() == "1") {
                    numnew.opOr(numb1, numbx);
                } ورنہ {
                    numnew.op_تفویض(numb1);
                }
                AstConst* ثابت newp = نیا AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                UINFO(4, "   -> " << newp);
            } ورنہ {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ثابت newvarp
                    = نیا AstVar{nodep->fileline(), VVarType::XTEMP, m_xrand_نام->get(خالی_اشاریہ),
                                 VFlagLogicPacked{}, nodep->چوڑائی()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlink_Fr_عقب(&replaceHandle);
                Ast_گرہ_Var_Ref* ثابت newref1p
                    = نیا AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ثابت newinitp = نیا AstInitial{
                    nodep->fileline(),
                    نیا Ast_تفویض{
                        nodep->fileline(),
                        نیا AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        نیا AstOr{nodep->fileline(), نیا AstConst{nodep->fileline(), numb1},
                                  نیا AstAnd{
                                      nodep->fileline(), نیا AstConst{nodep->fileline(), numbx},
                                      نیا AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_گرہ* ثابت newStmtsp = Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            }
        }
    }

    خالی دورہ(AstSel* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->user1_سیٹ_Once()) {
            // Guard against reading/writing past end of bit vector array
            ثابت Ast_گرہ* ثابت basefromp = nodep->بنیاد_Fromp(سچ);
            منطقی lvalue = جھوٹ;
            اگر (ثابت Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(basefromp, گرہ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ثابت uint32_t maxmsb = nodep->fromp()->dtypep()->چوڑائی() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ثابت maxmsbConstp = نیا AstConst{
                nodep->fileline(), AstConst::Widthed_قدر{}, nodep->lsbp()->چوڑائی(), maxmsb};
            Ast_گرہ_اظہاریہ* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlink_Fr_عقب());
            اگر (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_درخت(), maxmsbConstp);
                nodep->lsbp(lsbp);
                واپس;
            }
            nodep->lsbp(new_اظہاریہ_Stmt_Or_Clone(lsbp));
            Ast_گرہ_اظہاریہ* condp
                = V3Const::constifyEdit(نیا AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            اگر (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_عقب(&replaceHandle);
                V3_نمبر xnum{nodep, nodep->چوڑائی()};
                xnum.سیٹ_All_بٹ_X();
                Ast_گرہ_اظہاریہ* ثابت xexprp = نیا AstConst{nodep->fileline(), xnum};
                Ast_گرہ_اظہاریہ* ثابت newp = [&]() -> Ast_گرہ_اظہاریہ* {
                    اگر (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_درخت(), condp);
                        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                        واپس xexprp;
                    }
                    واپس نیا AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ورنہ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    خالی دورہ(AstArraySel* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->user1_سیٹ_Once()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_گرہ* ثابت basefromp = nodep->fromp()->بنیاد_Fromp(سچ);
            منطقی lvalue = جھوٹ;
            اگر (ثابت Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(basefromp, گرہ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } ورنہ اگر (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } ورنہ {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            صحیح declElements = -1;
            Ast_گرہ_DType* ثابت dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ثابت Ast_گرہ_Array_DType* ثابت adtypep = VN_CAST(dtypep, گرہ_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            اگر (ثابت AstModDiv* ثابت moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                اگر (ثابت AstConst* ثابت modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    اگر (modconstp->چوڑائی() <= 32
                        && modconstp->مقصد_UInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->مقصد_UInt());
                        واپس;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ثابت declElementsp
                = نیا AstConst{nodep->fileline(), AstConst::Widthed_قدر{}, nodep->bitp()->چوڑائی(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_گرہ_اظہاریہ* bitp = V3Const::constifyEdit(nodep->bitp()->unlink_Fr_عقب());
            اگر (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_درخت(), declElementsp);
                nodep->bitp(bitp);
                واپس;
            }
            nodep->bitp(new_اظہاریہ_Stmt_Or_Clone(bitp));
            Ast_گرہ_اظہاریہ* condp = نیا AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ثابت Ast_گرہ_DType* ثابت گرہ_Dtp = nodep->dtypep()->skipRefp();
            اگر (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(گرہ_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_عقب(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_نمبر xnum{nodep, nodep->چوڑائی()};
                اگر (گرہ_Dtp->isDouble()) {
                    xnum = V3_نمبر{nodep, V3_نمبر::Double{}, 0.0};
                } ورنہ اگر (گرہ_Dtp->isString()) {
                    xnum = V3_نمبر{nodep, V3_نمبر::String{}, ""};
                } ورنہ {
                    اگر (گرہ_Dtp->isFourstate()) {
                        xnum.سیٹ_All_بٹ_X();
                    } ورنہ {
                        xnum.سیٹ_All_Bits0();
                    }
                }
                Ast_گرہ* ثابت newp = نیا AstCond{nodep->fileline(), condp, nodep,
                                                  نیا AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ورنہ اگر (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_گرہ_اظہاریہ* ثابت asBitp = nodep->bitp()->unlink_Fr_عقب(&replaceHandle);
                Ast_گرہ_اظہاریہ* ثابت newp
                    = نیا AstCond{asBitp->fileline(), condp, asBitp,
                                  نیا AstConst{asBitp->fileline(), AstConst::Widthed_قدر{},
                                               asBitp->چوڑائی(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } ورنہ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح UnknownVisitor(AstNetlist* nodep)
        : m_lvbound_نام{"__Vlvbound"}
        , m_xrand_نام{std::make_unique<V3_Unique_نام>(s_xrand_سابقہ)} {
        iterate(nodep);
    }
    ~UnknownVisitor() override {  //
        V3Stats::جوڑو_Stat("Unknowns, variables created", m_statUnkVars);
        V3Stats::جوڑو_Stat("Unknowns, else branches created", m_statElses);
    }
};

ثابت std::string UnknownVisitor::s_xrand_سابقہ = "__Vxrand";

//######################################################################
// Unknown class functions

خالی V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("unknown", 0, dump_درخت_Either_Level() >= 3);
}
