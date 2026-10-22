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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Unknown.h"

#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Stats.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্ৰেণী UnknownVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    ধ্রুৱ VNUser1InUse m_inuser1;
    ধ্রুৱ VNUser2InUse m_inuser2;
    স্থির ধ্রুৱ std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    Ast_গ্ৰন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    তার্কিক m_constXCvt = মিছা;  // Convert X's
    তার্কিক m_allowXUnique = সঁচা;  // Allow unique assignments

    // METHODS

    শূন্য addVar(AstVar* varp) {
        যদি (m_ftaskp) {
            varp->funcLocal(সঁচা);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } অন্যথা {
            m_modp->prependStmtsp(varp);
        }
    }

    শূন্য replaceBoundLvalue(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, Ast_গ্ৰন্থি_ৰাশিমালা* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_গ্ৰন্থি_ৰাশিমালা* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        যেতিয়ালৈ (VN_IS(prep->backp(), গ্ৰন্থি_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), গ্ৰন্থি_ৰাশিমালা);
        }
        যদি (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_বৃক্ষ(), condp);
            ঘূৰাই;
        }
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        যদি (ধ্রুৱ AstIf* ধ্রুৱ ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } অন্যথা {
            Ast_গ্ৰন্থি* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            যেতিয়ালৈ (!VN_IS(stmtp, গ্ৰন্থি_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_গ্ৰন্থি* ধ্রুৱ origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_গ্ৰন্থি* elseStmtp = nullptr;
            ধ্রুৱ তার্কিক hasSideEffects = origStmtp->exists(
                [](Ast_গ্ৰন্থি* ধ্রুৱ np) { ঘূৰাই !np->isPure() || np->isTimingControl(); });
            যদি (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_বৃক্ষ(মিছা);
                AstVar* ধ্রুৱ varp = নূতন AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_গ্ৰন্থি* ধ্রুৱ prepCopyp = prep->clonep();
                prepCopyp->replaceWith(নূতন AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ঠেলো_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ধ্রুৱ newp = নূতন AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(সঁচা);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nodep) {
        AstVar* ধ্রুৱ varp = নূতন AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        ঘূৰাই varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    স্থির তার্কিক isStaticlyGte(AstConst* ধ্রুৱ msbConstp, ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp) {
        যদি (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->অঙ্ক().toSInt() >= (1 << exprp->width()) - 1) {
            ঘূৰাই সঁচা;
        }
        যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(exprp, Const)) {
            যদি (V3_সংখ্যা{msbConstp}.opGte(msbConstp->অঙ্ক(), constp->অঙ্ক()).isNeqZero()) {
                ঘূৰাই সঁচা;
            }
        }
        ঘূৰাই মিছা;
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* new_ৰাশিমালা_Stmt_Or_Clone(Ast_গ্ৰন্থি_ৰাশিমালা*& exprp) {
        যদি (!exprp->isPure()) {
            AstVar* ধ্রুৱ varp = createAddTemp(exprp);
            File_সাৰি* ধ্রুৱ fl = exprp->fileline();
            exprp = নূতন Ast_ৰাশিমালা_Stmt{
                fl, নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                নূতন AstVarRef{fl, varp, VAccess::READ}};
            ঘূৰাই নূতন AstVarRef{fl, varp, VAccess::READ};
        }
        ঘূৰাই exprp->clone_বৃক্ষ_Pure(মিছা);
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = সঁচা;
            // Class X randomization causes Vxrand in strange places, so disable
            যদি (VN_IS(nodep, Class)) m_allowXUnique = মিছা;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = মিছা;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = সঁচা;
        iterateAndNextNull(nodep->stmtsp());
    }
    শূন্য visit(Ast_গ্ৰন্থি_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = মিছা;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    শূন্য visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        যদি (nodep->isParam()) m_allowXUnique = মিছা;
        iterateChildren(nodep);
    }
    শূন্য visitEqNeqCase(Ast_গ্ৰন্থি_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        যদি (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ঘূৰাই;
        } অন্যথা {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            যদি (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->অঙ্ক().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->অঙ্ক().isFourState()))) {
                newp = নূতন AstConst(nodep->fileline(), AstConst::Widthed_মান{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_বৃক্ষ(), lhsp);
                VL_DO_DANGLING(rhsp->delete_বৃক্ষ(), rhsp);
            } অন্যথা {
                যদি (VN_IS(nodep, EqCase)) {
                    newp = নূতন AstEq{nodep->fileline(), lhsp, rhsp};
                } অন্যথা {
                    newp = নূতন AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    শূন্য visitEqNeqWild(Ast_গ্ৰন্থি_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        যদি (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ঘূৰাই;
        } অন্যথা {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* newp;
            যদি (!VN_IS(rhsp, Const)) {
                যদি (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = নূতন AstEq{nodep->fileline(), lhsp, rhsp};
            } অন্যথা {
                // X or Z's become mask, ala case statements.
                V3_সংখ্যা nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->অঙ্ক());
                V3_সংখ্যা numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->অঙ্ক());
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ and1p = নূতন AstAnd{nodep->fileline(), lhsp,
                                                      নূতন AstConst{nodep->fileline(), nummask}};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ and2p = নূতন AstConst{nodep->fileline(), numval};
                যদি (VN_IS(nodep, EqWild)) {
                    newp = নূতন AstEq{nodep->fileline(), and1p, and2p};
                } অন্যথা {
                    newp = নূতন AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_বৃক্ষ(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    শূন্য visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    শূন্য visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    শূন্য visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    শূন্য visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    শূন্য visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ধ্রুৱ newp = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(Ast_গুনতি_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<তার্কিক, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->অঙ্ক().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->অঙ্ক().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->অঙ্ক().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_গ্ৰন্থি_ৰাশিমালা* nonXp = nullptr;
        যদি (!dropop[0]) {
            nonXp = nodep->rhsp();
        } অন্যথা যদি (!dropop[1]) {
            nonXp = nodep->thsp();
        } অন্যথা যদি (!dropop[2]) {
            nonXp = nodep->fhsp();
        } অন্যথা {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ধ্রুৱ newp = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }
        যদি (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_বৃক্ষ();
            nodep->rhsp(nonXp->clone_বৃক্ষ_Pure(সঁচা));
        }
        যদি (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_বৃক্ষ();
            nodep->thsp(nonXp->clone_বৃক্ষ_Pure(সঁচা));
        }
        যদি (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_বৃক্ষ();
            nodep->fhsp(nonXp->clone_বৃক্ষ_Pure(সঁচা));
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstConst* nodep) override {
        যদি (m_constXCvt && nodep->অঙ্ক().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_সংখ্যা numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->অঙ্ক());
            V3_সংখ্যা numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->অঙ্ক());
            যদি (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_সংখ্যা numnew{nodep, numb1.width()};
                যদি (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } অন্যথা {
                    numnew.opAssign(numb1);
                }
                AstConst* ধ্রুৱ newp = নূতন AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                UINFO(4, "   -> " << newp);
            } অন্যথা {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ধ্রুৱ newvarp
                    = নূতন AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ newref1p
                    = নূতন AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ধ্রুৱ newinitp = নূতন AstInitial{
                    nodep->fileline(),
                    নূতন AstAssign{
                        nodep->fileline(),
                        নূতন AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        নূতন AstOr{nodep->fileline(), নূতন AstConst{nodep->fileline(), numb1},
                                  নূতন AstAnd{
                                      nodep->fileline(), নূতন AstConst{nodep->fileline(), numbx},
                                      নূতন AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_গ্ৰন্থি* ধ্রুৱ newStmtsp = Ast_গ্ৰন্থি::addNext<Ast_গ্ৰন্থি, Ast_গ্ৰন্থি>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            }
        }
    }

    শূন্য visit(AstSel* nodep) override {
        iterateChildren(nodep);
        যদি (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ basefromp = nodep->baseFromp(সঁচা);
            তার্কিক lvalue = মিছা;
            যদি (ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ varrefp = VN_CAST(basefromp, গ্ৰন্থি_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ধ্রুৱ uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ধ্রুৱ maxmsbConstp = নূতন AstConst{
                nodep->fileline(), AstConst::Widthed_মান{}, nodep->lsbp()->width(), maxmsb};
            Ast_গ্ৰন্থি_ৰাশিমালা* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            যদি (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_বৃক্ষ(), maxmsbConstp);
                nodep->lsbp(lsbp);
                ঘূৰাই;
            }
            nodep->lsbp(new_ৰাশিমালা_Stmt_Or_Clone(lsbp));
            Ast_গ্ৰন্থি_ৰাশিমালা* condp
                = V3Const::constifyEdit(নূতন AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            যদি (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_সংখ্যা xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ xexprp = নূতন AstConst{nodep->fileline(), xnum};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = [&]() -> Ast_গ্ৰন্থি_ৰাশিমালা* {
                    যদি (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_বৃক্ষ(), condp);
                        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                        ঘূৰাই xexprp;
                    }
                    ঘূৰাই নূতন AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } অন্যথা {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    শূন্য visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        যদি (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_গ্ৰন্থি* ধ্রুৱ basefromp = nodep->fromp()->baseFromp(সঁচা);
            তার্কিক lvalue = মিছা;
            যদি (ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ varrefp = VN_CAST(basefromp, গ্ৰন্থি_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } অন্যথা যদি (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } অন্যথা {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            পূর্ণাঙ্ক declElements = -1;
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ adtypep = VN_CAST(dtypep, গ্ৰন্থি_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            যদি (ধ্রুৱ AstModDiv* ধ্রুৱ moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                যদি (ধ্রুৱ AstConst* ধ্রুৱ modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    যদি (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        ঘূৰাই;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ধ্রুৱ declElementsp
                = নূতন AstConst{nodep->fileline(), AstConst::Widthed_মান{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_গ্ৰন্থি_ৰাশিমালা* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            যদি (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_বৃক্ষ(), declElementsp);
                nodep->bitp(bitp);
                ঘূৰাই;
            }
            nodep->bitp(new_ৰাশিমালা_Stmt_Or_Clone(bitp));
            Ast_গ্ৰন্থি_ৰাশিমালা* condp = নূতন AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ গ্ৰন্থি_Dtp = nodep->dtypep()->skipRefp();
            যদি (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(গ্ৰন্থি_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_সংখ্যা xnum{nodep, nodep->width()};
                যদি (গ্ৰন্থি_Dtp->isDouble()) {
                    xnum = V3_সংখ্যা{nodep, V3_সংখ্যা::Double{}, 0.0};
                } অন্যথা যদি (গ্ৰন্থি_Dtp->isString()) {
                    xnum = V3_সংখ্যা{nodep, V3_সংখ্যা::String{}, ""};
                } অন্যথা {
                    যদি (গ্ৰন্থি_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } অন্যথা {
                        xnum.setAllBits0();
                    }
                }
                Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstCond{nodep->fileline(), condp, nodep,
                                                  নূতন AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } অন্যথা যদি (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp
                    = নূতন AstCond{asBitp->fileline(), condp, asBitp,
                                  নূতন AstConst{asBitp->fileline(), AstConst::Widthed_মান{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } অন্যথা {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
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

ধ্রুৱ std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

শূন্য V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("unknown", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
