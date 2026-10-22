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
    Ast_تۈگۈن_Module* m_modp = nullptr;  // Current module
    Ast_تۈگۈن_FTask* m_ftaskp = nullptr;  // Current function/task
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

    فراغ replaceBoundLvalue(Ast_تۈگۈن_ئىپادە* nodep, Ast_تۈگۈن_ئىپادە* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_تۈگۈن_ئىپادە* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        طالما (VN_IS(prep->backp(), تۈگۈن_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), تۈگۈن_ئىپادە);
        }
        إذا (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_دەرەخ(), condp);
            أرجع;
        }
        File_قۇر* ثابت fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        إذا (ثابت AstIf* ثابت ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_تۈگۈن_ئىپادە* ثابت earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_تۈگۈن_ئىپادە* ثابت newp = جديد AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } وإلا {
            Ast_تۈگۈن* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            طالما (!VN_IS(stmtp, تۈگۈن_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_تۈگۈن* ثابت origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_تۈگۈن* elseStmtp = nullptr;
            ثابت منطقي hasSideEffects = origStmtp->exists(
                [](Ast_تۈگۈن* ثابت np) { أرجع !np->isPure() || np->isTimingControl(); });
            إذا (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_دەرەخ(خطأ);
                AstVar* ثابت varp = جديد AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_تۈگۈن* ثابت prepCopyp = prep->clonep();
                prepCopyp->replaceWith(جديد AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ئىتتىرىش_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ثابت newp = جديد AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(true);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(ثابت Ast_تۈگۈن_ئىپادە* ثابت nodep) {
        AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        أرجع varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ساكن منطقي isStaticlyGte(AstConst* ثابت msbConstp, ثابت Ast_تۈگۈن_ئىپادە* ثابت exprp) {
        إذا (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->رەقەم().toSInt() >= (1 << exprp->width()) - 1) {
            أرجع true;
        }
        إذا (ثابت AstConst* ثابت constp = VN_CAST(exprp, Const)) {
            إذا (V3_سان{msbConstp}.opGte(msbConstp->رەقەم(), constp->رەقەم()).isNeqZero()) {
                أرجع true;
            }
        }
        أرجع خطأ;
    }

    Ast_تۈگۈن_ئىپادە* new_ئىپادە_Stmt_Or_Clone(Ast_تۈگۈن_ئىپادە*& exprp) {
        إذا (!exprp->isPure()) {
            AstVar* ثابت varp = createAddTemp(exprp);
            File_قۇر* ثابت fl = exprp->fileline();
            exprp = جديد Ast_ئىپادە_Stmt{
                fl, جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                جديد AstVarRef{fl, varp, VAccess::READ}};
            أرجع جديد AstVarRef{fl, varp, VAccess::READ};
        }
        أرجع exprp->clone_دەرەخ_Pure(خطأ);
    }

    // VISITORS
    فراغ visit(Ast_تۈگۈن_Module* nodep) override {
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
    فراغ visit(Ast_تۈگۈن_FTask* nodep) override {
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
    فراغ visit(Ast_تۈگۈن_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = خطأ;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        إذا (nodep->isParam()) m_allowXUnique = خطأ;
        iterateChildren(nodep);
    }
    فراغ visitEqNeqCase(Ast_تۈگۈن_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        إذا (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            أرجع;
        } وإلا {
            Ast_تۈگۈن_ئىپادە* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_تۈگۈن_ئىپادە* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_تۈگۈن_ئىپادە* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            إذا (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->رەقەم().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->رەقەم().isFourState()))) {
                newp = جديد AstConst(nodep->fileline(), AstConst::Widthed_قىممەت{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_دەرەخ(), lhsp);
                VL_DO_DANGLING(rhsp->delete_دەرەخ(), rhsp);
            } وإلا {
                إذا (VN_IS(nodep, EqCase)) {
                    newp = جديد AstEq{nodep->fileline(), lhsp, rhsp};
                } وإلا {
                    newp = جديد AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_دەرەخ(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    فراغ visitEqNeqWild(Ast_تۈگۈن_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        إذا (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            أرجع;
        } وإلا {
            Ast_تۈگۈن_ئىپادە* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_تۈگۈن_ئىپادە* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_تۈگۈن_ئىپادە* newp;
            إذا (!VN_IS(rhsp, Const)) {
                إذا (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = جديد AstEq{nodep->fileline(), lhsp, rhsp};
            } وإلا {
                // X or Z's become mask, ala case statements.
                V3_سان nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->رەقەم());
                V3_سان numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->رەقەم());
                Ast_تۈگۈن_ئىپادە* ثابت and1p = جديد AstAnd{nodep->fileline(), lhsp,
                                                      جديد AstConst{nodep->fileline(), nummask}};
                Ast_تۈگۈن_ئىپادە* ثابت and2p = جديد AstConst{nodep->fileline(), numval};
                إذا (VN_IS(nodep, EqWild)) {
                    newp = جديد AstEq{nodep->fileline(), and1p, and2p};
                } وإلا {
                    newp = جديد AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_دەرەخ(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_دەرەخ(), nodep);
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
        VL_DO_DANGLING(nodep->delete_دەرەخ(), nodep);
    }
    فراغ visit(Ast_سانى_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<منطقي, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->رەقەم().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->رەقەم().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->رەقەم().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_تۈگۈن_ئىپادە* nonXp = nullptr;
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
            VL_DO_DANGLING(nodep->delete_دەرەخ(), nodep);
            أرجع;
        }
        إذا (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_دەرەخ();
            nodep->rhsp(nonXp->clone_دەرەخ_Pure(true));
        }
        إذا (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_دەرەخ();
            nodep->thsp(nonXp->clone_دەرەخ_Pure(true));
        }
        إذا (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_دەرەخ();
            nodep->fhsp(nonXp->clone_دەرەخ_Pure(true));
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstConst* nodep) override {
        إذا (m_constXCvt && nodep->رەقەم().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_سان numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->رەقەم());
            V3_سان numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->رەقەم());
            إذا (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_سان numnew{nodep, numb1.width()};
                إذا (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } وإلا {
                    numnew.opAssign(numb1);
                }
                AstConst* ثابت newp = جديد AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_دەرەخ(), nodep);
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
                Ast_تۈگۈن_Var_Ref* ثابت newref1p
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
                Ast_تۈگۈن* ثابت newStmtsp = Ast_تۈگۈن::addNext<Ast_تۈگۈن, Ast_تۈگۈن>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_دەرەخ(), nodep);
            }
        }
    }

    فراغ visit(AstSel* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            ثابت Ast_تۈگۈن* ثابت basefromp = nodep->baseFromp(true);
            منطقي lvalue = خطأ;
            إذا (ثابت Ast_تۈگۈن_Var_Ref* ثابت varrefp = VN_CAST(basefromp, تۈگۈن_Var_Ref)) {
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
                nodep->fileline(), AstConst::Widthed_قىممەت{}, nodep->lsbp()->width(), maxmsb};
            Ast_تۈگۈن_ئىپادە* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            إذا (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_دەرەخ(), maxmsbConstp);
                nodep->lsbp(lsbp);
                أرجع;
            }
            nodep->lsbp(new_ئىپادە_Stmt_Or_Clone(lsbp));
            Ast_تۈگۈن_ئىپادە* condp
                = V3Const::constifyEdit(جديد AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            إذا (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_سان xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_تۈگۈن_ئىپادە* ثابت xexprp = جديد AstConst{nodep->fileline(), xnum};
                Ast_تۈگۈن_ئىپادە* ثابت newp = [&]() -> Ast_تۈگۈن_ئىپادە* {
                    إذا (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_دەرەخ(), condp);
                        VL_DO_DANGLING(nodep->delete_دەرەخ(), nodep);
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
            Ast_تۈگۈن* ثابت basefromp = nodep->fromp()->baseFromp(true);
            منطقي lvalue = خطأ;
            إذا (ثابت Ast_تۈگۈن_Var_Ref* ثابت varrefp = VN_CAST(basefromp, تۈگۈن_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } وإلا إذا (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } وإلا {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            صحيح declElements = -1;
            Ast_تۈگۈن_DType* ثابت dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ثابت Ast_تۈگۈن_Array_DType* ثابت adtypep = VN_CAST(dtypep, تۈگۈن_Array_DType);
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
                = جديد AstConst{nodep->fileline(), AstConst::Widthed_قىممەت{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_تۈگۈن_ئىپادە* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            إذا (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_دەرەخ(), declElementsp);
                nodep->bitp(bitp);
                أرجع;
            }
            nodep->bitp(new_ئىپادە_Stmt_Or_Clone(bitp));
            Ast_تۈگۈن_ئىپادە* condp = جديد AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ثابت Ast_تۈگۈن_DType* ثابت تۈگۈن_Dtp = nodep->dtypep()->skipRefp();
            إذا (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(تۈگۈن_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_سان xnum{nodep, nodep->width()};
                إذا (تۈگۈن_Dtp->isDouble()) {
                    xnum = V3_سان{nodep, V3_سان::Double{}, 0.0};
                } وإلا إذا (تۈگۈن_Dtp->isString()) {
                    xnum = V3_سان{nodep, V3_سان::String{}, ""};
                } وإلا {
                    إذا (تۈگۈن_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } وإلا {
                        xnum.setAllBits0();
                    }
                }
                Ast_تۈگۈن* ثابت newp = جديد AstCond{nodep->fileline(), condp, nodep,
                                                  جديد AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } وإلا إذا (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_تۈگۈن_ئىپادە* ثابت asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_تۈگۈن_ئىپادە* ثابت newp
                    = جديد AstCond{asBitp->fileline(), condp, asBitp,
                                  جديد AstConst{asBitp->fileline(), AstConst::Widthed_قىممەت{},
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
    فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_دەرەخ("unknown", 0, dump_دەرەخ_Either_Level() >= 3);
}
