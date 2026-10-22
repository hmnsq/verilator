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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Unknown.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ UnknownVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;
    ಸ್ಥಿರ ಧ್ರುವ std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    ತಾರ್ಕಿಕ m_constXCvt = ಅಸತ್ಯ;  // Convert X's
    ತಾರ್ಕಿಕ m_allowXUnique = ಸತ್ಯ;  // Allow unique assignments

    // METHODS

    ಶೂನ್ಯ addVar(AstVar* varp) {
        ಯದಿ (m_ftaskp) {
            varp->funcLocal(ಸತ್ಯ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } ಅನ್ಯಥಾ {
            m_modp->prependStmtsp(varp);
        }
    }

    ಶೂನ್ಯ replaceBoundLvalue(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        ಯಾವತ್ (VN_IS(prep->backp(), ಗಂಟು_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        }
        ಯದಿ (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_ವೃಕ್ಷ(), condp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        ಯದಿ (ಧ್ರುವ AstIf* ಧ್ರುವ ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            ಯಾವತ್ (!VN_IS(stmtp, ಗಂಟು_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_ಗಂಟು* ಧ್ರುವ origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_ಗಂಟು* elseStmtp = nullptr;
            ಧ್ರುವ ತಾರ್ಕಿಕ hasSideEffects = origStmtp->exists(
                [](Ast_ಗಂಟು* ಧ್ರುವ np) { ಪ್ರತ್ಯಾಗಮ !np->isPure() || np->isTimingControl(); });
            ಯದಿ (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_ಗಂಟು* ಧ್ರುವ prepCopyp = prep->clonep();
                prepCopyp->replaceWith(ನೂತನ AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ಧ್ರುವ newp = ನೂತನ AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(ಸತ್ಯ);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep) {
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        ಪ್ರತ್ಯಾಗಮ varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isStaticlyGte(AstConst* ಧ್ರುವ msbConstp, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp) {
        ಯದಿ (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->ಅಂಕೆ().toSInt() >= (1 << exprp->width()) - 1) {
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(exprp, Const)) {
            ಯದಿ (V3_ಸಂಖ್ಯೆ{msbConstp}.opGte(msbConstp->ಅಂಕೆ(), constp->ಅಂಕೆ()).isNeqZero()) {
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_ಅಭಿವ್ಯಕ್ತಿ_Stmt_Or_Clone(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& exprp) {
        ಯದಿ (!exprp->isPure()) {
            AstVar* ಧ್ರುವ varp = createAddTemp(exprp);
            File_ಸಾಲು* ಧ್ರುವ fl = exprp->fileline();
            exprp = ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                fl, ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                ನೂತನ AstVarRef{fl, varp, VAccess::READ}};
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, varp, VAccess::READ};
        }
        ಪ್ರತ್ಯಾಗಮ exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = ಸತ್ಯ;
            // Class X randomization causes Vxrand in strange places, so disable
            ಯದಿ (VN_IS(nodep, Class)) m_allowXUnique = ಅಸತ್ಯ;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ಅಸತ್ಯ;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = ಸತ್ಯ;
        iterateAndNextNull(nodep->stmtsp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ಅಸತ್ಯ;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        ಯದಿ (nodep->isParam()) m_allowXUnique = ಅಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visitEqNeqCase(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ಯದಿ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            ಯದಿ (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->ಅಂಕೆ().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->ಅಂಕೆ().isFourState()))) {
                newp = ನೂತನ AstConst(nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_ವೃಕ್ಷ(), lhsp);
                VL_DO_DANGLING(rhsp->delete_ವೃಕ್ಷ(), rhsp);
            } ಅನ್ಯಥಾ {
                ಯದಿ (VN_IS(nodep, EqCase)) {
                    newp = ನೂತನ AstEq{nodep->fileline(), lhsp, rhsp};
                } ಅನ್ಯಥಾ {
                    newp = ನೂತನ AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    ಶೂನ್ಯ visitEqNeqWild(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ಯದಿ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            ಯದಿ (!VN_IS(rhsp, Const)) {
                ಯದಿ (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = ನೂತನ AstEq{nodep->fileline(), lhsp, rhsp};
            } ಅನ್ಯಥಾ {
                // X or Z's become mask, ala case statements.
                V3_ಸಂಖ್ಯೆ nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->ಅಂಕೆ());
                V3_ಸಂಖ್ಯೆ numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->ಅಂಕೆ());
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ and1p = ನೂತನ AstAnd{nodep->fileline(), lhsp,
                                                      ನೂತನ AstConst{nodep->fileline(), nummask}};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ and2p = ನೂತನ AstConst{nodep->fileline(), numval};
                ಯದಿ (VN_IS(nodep, EqWild)) {
                    newp = ನೂತನ AstEq{nodep->fileline(), and1p, and2p};
                } ಅನ್ಯಥಾ {
                    newp = ನೂತನ AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_ವೃಕ್ಷ(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    ಶೂನ್ಯ visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    ಶೂನ್ಯ visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    ಶೂನ್ಯ visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    ಶೂನ್ಯ visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    ಶೂನ್ಯ visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಎಣಿಕೆ_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<ತಾರ್ಕಿಕ, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->ಅಂಕೆ().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->ಅಂಕೆ().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nonXp = nullptr;
        ಯದಿ (!dropop[0]) {
            nonXp = nodep->rhsp();
        } ಅನ್ಯಥಾ ಯದಿ (!dropop[1]) {
            nonXp = nodep->thsp();
        } ಅನ್ಯಥಾ ಯದಿ (!dropop[2]) {
            nonXp = nodep->fhsp();
        } ಅನ್ಯಥಾ {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
            nodep->rhsp(nonXp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
        }
        ಯದಿ (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
            nodep->thsp(nonXp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
        }
        ಯದಿ (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
            nodep->fhsp(nonXp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        ಯದಿ (m_constXCvt && nodep->ಅಂಕೆ().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_ಸಂಖ್ಯೆ numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->ಅಂಕೆ());
            V3_ಸಂಖ್ಯೆ numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->ಅಂಕೆ());
            ಯದಿ (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_ಸಂಖ್ಯೆ numnew{nodep, numb1.width()};
                ಯದಿ (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } ಅನ್ಯಥಾ {
                    numnew.opAssign(numb1);
                }
                AstConst* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                UINFO(4, "   -> " << newp);
            } ಅನ್ಯಥಾ {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ಧ್ರುವ newvarp
                    = ನೂತನ AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ newref1p
                    = ನೂತನ AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ಧ್ರುವ newinitp = ನೂತನ AstInitial{
                    nodep->fileline(),
                    ನೂತನ AstAssign{
                        nodep->fileline(),
                        ನೂತನ AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        ನೂತನ AstOr{nodep->fileline(), ನೂತನ AstConst{nodep->fileline(), numb1},
                                  ನೂತನ AstAnd{
                                      nodep->fileline(), ನೂತನ AstConst{nodep->fileline(), numbx},
                                      ನೂತನ AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_ಗಂಟು* ಧ್ರುವ newStmtsp = Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            }
        }
    }

    ಶೂನ್ಯ visit(AstSel* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ basefromp = nodep->baseFromp(ಸತ್ಯ);
            ತಾರ್ಕಿಕ lvalue = ಅಸತ್ಯ;
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varrefp = VN_CAST(basefromp, ಗಂಟು_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ಧ್ರುವ uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ಧ್ರುವ maxmsbConstp = ನೂತನ AstConst{
                nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->lsbp()->width(), maxmsb};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            ಯದಿ (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_ವೃಕ್ಷ(), maxmsbConstp);
                nodep->lsbp(lsbp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            nodep->lsbp(new_ಅಭಿವ್ಯಕ್ತಿ_Stmt_Or_Clone(lsbp));
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp
                = V3Const::constifyEdit(ನೂತನ AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            ಯದಿ (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_ಸಂಖ್ಯೆ xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ xexprp = ನೂತನ AstConst{nodep->fileline(), xnum};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                    ಯದಿ (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_ವೃಕ್ಷ(), condp);
                        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                        ಪ್ರತ್ಯಾಗಮ xexprp;
                    }
                    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ಅನ್ಯಥಾ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    ಶೂನ್ಯ visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_ಗಂಟು* ಧ್ರುವ basefromp = nodep->fromp()->baseFromp(ಸತ್ಯ);
            ತಾರ್ಕಿಕ lvalue = ಅಸತ್ಯ;
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varrefp = VN_CAST(basefromp, ಗಂಟು_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } ಅನ್ಯಥಾ {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            ಪೂರ್ಣಾಙ್ಕ declElements = -1;
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ adtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            ಯದಿ (ಧ್ರುವ AstModDiv* ಧ್ರುವ moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    ಯದಿ (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ಧ್ರುವ declElementsp
                = ನೂತನ AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            ಯದಿ (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_ವೃಕ್ಷ(), declElementsp);
                nodep->bitp(bitp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            nodep->bitp(new_ಅಭಿವ್ಯಕ್ತಿ_Stmt_Or_Clone(bitp));
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ನೂತನ AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ ಗಂಟು_Dtp = nodep->dtypep()->skipRefp();
            ಯದಿ (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(ಗಂಟು_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_ಸಂಖ್ಯೆ xnum{nodep, nodep->width()};
                ಯದಿ (ಗಂಟು_Dtp->isDouble()) {
                    xnum = V3_ಸಂಖ್ಯೆ{nodep, V3_ಸಂಖ್ಯೆ::Double{}, 0.0};
                } ಅನ್ಯಥಾ ಯದಿ (ಗಂಟು_Dtp->isString()) {
                    xnum = V3_ಸಂಖ್ಯೆ{nodep, V3_ಸಂಖ್ಯೆ::String{}, ""};
                } ಅನ್ಯಥಾ {
                    ಯದಿ (ಗಂಟು_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } ಅನ್ಯಥಾ {
                        xnum.setAllBits0();
                    }
                }
                Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstCond{nodep->fileline(), condp, nodep,
                                                  ನೂತನ AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ಅನ್ಯಥಾ ಯದಿ (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
                    = ನೂತನ AstCond{asBitp->fileline(), condp, asBitp,
                                  ನೂತನ AstConst{asBitp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } ಅನ್ಯಥಾ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
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

ಧ್ರುವ std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

ಶೂನ್ಯ V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("unknown", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
