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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Unknown.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ UnknownVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;
    સ્થિર અચલ std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current function/task
    તાર્કિક m_constXCvt = ખોટું;  // Convert X's
    તાર્કિક m_allowXUnique = સાચું;  // Allow unique assignments

    // METHODS

    શૂન્ય addVar(AstVar* varp) {
        જો (m_ftaskp) {
            varp->funcLocal(સાચું);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } નહીંતર {
            m_modp->prependStmtsp(varp);
        }
    }

    શૂન્ય replaceBoundLvalue(Ast_ગાંઠ_પદાવલિ* nodep, Ast_ગાંઠ_પદાવલિ* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_ગાંઠ_પદાવલિ* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        જ્યારે (VN_IS(prep->backp(), ગાંઠ_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), ગાંઠ_પદાવલિ);
        }
        જો (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_વૃક્ષ(), condp);
            પાછા;
        }
        File_લીટી* અચલ fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        જો (અચલ AstIf* અચલ ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_ગાંઠ_પદાવલિ* અચલ earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } નહીંતર {
            Ast_ગાંઠ* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            જ્યારે (!VN_IS(stmtp, ગાંઠ_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_ગાંઠ* અચલ origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_ગાંઠ* elseStmtp = nullptr;
            અચલ તાર્કિક hasSideEffects = origStmtp->exists(
                [](Ast_ગાંઠ* અચલ np) { પાછા !np->isPure() || np->isTimingControl(); });
            જો (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_વૃક્ષ(ખોટું);
                AstVar* અચલ varp = નવું AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_ગાંઠ* અચલ prepCopyp = prep->clonep();
                prepCopyp->replaceWith(નવું AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ધકેલો_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* અચલ newp = નવું AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(સાચું);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(અચલ Ast_ગાંઠ_પદાવલિ* અચલ nodep) {
        AstVar* અચલ varp = નવું AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        પાછા varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    સ્થિર તાર્કિક isStaticlyGte(AstConst* અચલ msbConstp, અચલ Ast_ગાંઠ_પદાવલિ* અચલ exprp) {
        જો (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->અંક().toSInt() >= (1 << exprp->width()) - 1) {
            પાછા સાચું;
        }
        જો (અચલ AstConst* અચલ constp = VN_CAST(exprp, Const)) {
            જો (V3_સંખ્યા{msbConstp}.opGte(msbConstp->અંક(), constp->અંક()).isNeqZero()) {
                પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }

    Ast_ગાંઠ_પદાવલિ* new_પદાવલિ_Stmt_Or_Clone(Ast_ગાંઠ_પદાવલિ*& exprp) {
        જો (!exprp->isPure()) {
            AstVar* અચલ varp = createAddTemp(exprp);
            File_લીટી* અચલ fl = exprp->fileline();
            exprp = નવું Ast_પદાવલિ_Stmt{
                fl, નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                નવું AstVarRef{fl, varp, VAccess::READ}};
            પાછા નવું AstVarRef{fl, varp, VAccess::READ};
        }
        પાછા exprp->clone_વૃક્ષ_Pure(ખોટું);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = સાચું;
            // Class X randomization causes Vxrand in strange places, so disable
            જો (VN_IS(nodep, Class)) m_allowXUnique = ખોટું;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ખોટું;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = સાચું;
        iterateAndNextNull(nodep->stmtsp());
    }
    શૂન્ય visit(Ast_ગાંઠ_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ખોટું;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        જો (nodep->isParam()) m_allowXUnique = ખોટું;
        iterateChildren(nodep);
    }
    શૂન્ય visitEqNeqCase(Ast_ગાંઠ_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        જો (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            પાછા;
        } નહીંતર {
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            જો (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->અંક().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->અંક().isFourState()))) {
                newp = નવું AstConst(nodep->fileline(), AstConst::Widthed_કિંમત{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_વૃક્ષ(), lhsp);
                VL_DO_DANGLING(rhsp->delete_વૃક્ષ(), rhsp);
            } નહીંતર {
                જો (VN_IS(nodep, EqCase)) {
                    newp = નવું AstEq{nodep->fileline(), lhsp, rhsp};
                } નહીંતર {
                    newp = નવું AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    શૂન્ય visitEqNeqWild(Ast_ગાંઠ_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        જો (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            પાછા;
        } નહીંતર {
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* newp;
            જો (!VN_IS(rhsp, Const)) {
                જો (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = નવું AstEq{nodep->fileline(), lhsp, rhsp};
            } નહીંતર {
                // X or Z's become mask, ala case statements.
                V3_સંખ્યા nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->અંક());
                V3_સંખ્યા numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->અંક());
                Ast_ગાંઠ_પદાવલિ* અચલ and1p = નવું AstAnd{nodep->fileline(), lhsp,
                                                      નવું AstConst{nodep->fileline(), nummask}};
                Ast_ગાંઠ_પદાવલિ* અચલ and2p = નવું AstConst{nodep->fileline(), numval};
                જો (VN_IS(nodep, EqWild)) {
                    newp = નવું AstEq{nodep->fileline(), and1p, and2p};
                } નહીંતર {
                    newp = નવું AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_વૃક્ષ(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    શૂન્ય visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    શૂન્ય visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    શૂન્ય visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    શૂન્ય visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    શૂન્ય visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* અચલ newp = નવું AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(Ast_ગણતરી_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<તાર્કિક, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->અંક().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->અંક().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->અંક().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_ગાંઠ_પદાવલિ* nonXp = nullptr;
        જો (!dropop[0]) {
            nonXp = nodep->rhsp();
        } નહીંતર જો (!dropop[1]) {
            nonXp = nodep->thsp();
        } નહીંતર જો (!dropop[2]) {
            nonXp = nodep->fhsp();
        } નહીંતર {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* અચલ newp = નવું AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        જો (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_વૃક્ષ();
            nodep->rhsp(nonXp->clone_વૃક્ષ_Pure(સાચું));
        }
        જો (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_વૃક્ષ();
            nodep->thsp(nonXp->clone_વૃક્ષ_Pure(સાચું));
        }
        જો (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_વૃક્ષ();
            nodep->fhsp(nonXp->clone_વૃક્ષ_Pure(સાચું));
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstConst* nodep) override {
        જો (m_constXCvt && nodep->અંક().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_સંખ્યા numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->અંક());
            V3_સંખ્યા numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->અંક());
            જો (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_સંખ્યા numnew{nodep, numb1.width()};
                જો (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } નહીંતર {
                    numnew.opAssign(numb1);
                }
                AstConst* અચલ newp = નવું AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
                UINFO(4, "   -> " << newp);
            } નહીંતર {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* અચલ newvarp
                    = નવું AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_ગાંઠ_Var_Ref* અચલ newref1p
                    = નવું AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* અચલ newinitp = નવું AstInitial{
                    nodep->fileline(),
                    નવું AstAssign{
                        nodep->fileline(),
                        નવું AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        નવું AstOr{nodep->fileline(), નવું AstConst{nodep->fileline(), numb1},
                                  નવું AstAnd{
                                      nodep->fileline(), નવું AstConst{nodep->fileline(), numbx},
                                      નવું AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_ગાંઠ* અચલ newStmtsp = Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            }
        }
    }

    શૂન્ય visit(AstSel* nodep) override {
        iterateChildren(nodep);
        જો (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            અચલ Ast_ગાંઠ* અચલ basefromp = nodep->baseFromp(સાચું);
            તાર્કિક lvalue = ખોટું;
            જો (અચલ Ast_ગાંઠ_Var_Ref* અચલ varrefp = VN_CAST(basefromp, ગાંઠ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            અચલ uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* અચલ maxmsbConstp = નવું AstConst{
                nodep->fileline(), AstConst::Widthed_કિંમત{}, nodep->lsbp()->width(), maxmsb};
            Ast_ગાંઠ_પદાવલિ* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            જો (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_વૃક્ષ(), maxmsbConstp);
                nodep->lsbp(lsbp);
                પાછા;
            }
            nodep->lsbp(new_પદાવલિ_Stmt_Or_Clone(lsbp));
            Ast_ગાંઠ_પદાવલિ* condp
                = V3Const::constifyEdit(નવું AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            જો (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_સંખ્યા xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_ગાંઠ_પદાવલિ* અચલ xexprp = નવું AstConst{nodep->fileline(), xnum};
                Ast_ગાંઠ_પદાવલિ* અચલ newp = [&]() -> Ast_ગાંઠ_પદાવલિ* {
                    જો (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_વૃક્ષ(), condp);
                        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
                        પાછા xexprp;
                    }
                    પાછા નવું AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } નહીંતર {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    શૂન્ય visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        જો (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_ગાંઠ* અચલ basefromp = nodep->fromp()->baseFromp(સાચું);
            તાર્કિક lvalue = ખોટું;
            જો (અચલ Ast_ગાંઠ_Var_Ref* અચલ varrefp = VN_CAST(basefromp, ગાંઠ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } નહીંતર જો (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } નહીંતર {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            પૂર્ણાંક declElements = -1;
            Ast_ગાંઠ_DType* અચલ dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            અચલ Ast_ગાંઠ_Array_DType* અચલ adtypep = VN_CAST(dtypep, ગાંઠ_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            જો (અચલ AstModDiv* અચલ moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                જો (અચલ AstConst* અચલ modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    જો (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        પાછા;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* અચલ declElementsp
                = નવું AstConst{nodep->fileline(), AstConst::Widthed_કિંમત{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_ગાંઠ_પદાવલિ* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            જો (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_વૃક્ષ(), declElementsp);
                nodep->bitp(bitp);
                પાછા;
            }
            nodep->bitp(new_પદાવલિ_Stmt_Or_Clone(bitp));
            Ast_ગાંઠ_પદાવલિ* condp = નવું AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            અચલ Ast_ગાંઠ_DType* અચલ ગાંઠ_Dtp = nodep->dtypep()->skipRefp();
            જો (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(ગાંઠ_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_સંખ્યા xnum{nodep, nodep->width()};
                જો (ગાંઠ_Dtp->isDouble()) {
                    xnum = V3_સંખ્યા{nodep, V3_સંખ્યા::Double{}, 0.0};
                } નહીંતર જો (ગાંઠ_Dtp->isString()) {
                    xnum = V3_સંખ્યા{nodep, V3_સંખ્યા::String{}, ""};
                } નહીંતર {
                    જો (ગાંઠ_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } નહીંતર {
                        xnum.setAllBits0();
                    }
                }
                Ast_ગાંઠ* અચલ newp = નવું AstCond{nodep->fileline(), condp, nodep,
                                                  નવું AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } નહીંતર જો (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_ગાંઠ_પદાવલિ* અચલ asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_ગાંઠ_પદાવલિ* અચલ newp
                    = નવું AstCond{asBitp->fileline(), condp, asBitp,
                                  નવું AstConst{asBitp->fileline(), AstConst::Widthed_કિંમત{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } નહીંતર {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
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

અચલ std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

શૂન્ય V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("unknown", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
