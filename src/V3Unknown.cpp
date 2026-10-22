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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ std::string s_xrand_ಪೂರ್ವಪ್ರತ್ಯಯ;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3_Unique_ಹೆಸರುಗಳು m_lvbound_ಹೆಸರುಗಳು;  // For generating unique temporary variable names
    std::unique_ptr<V3_Unique_ಹೆಸರುಗಳು> m_xrand_ಹೆಸರುಗಳು;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    ತಾರ್ಕಿಕ m_constXCvt = ಸುಳ್ಳು;  // Convert X's
    ತಾರ್ಕಿಕ m_allowXUnique = ನಿಜ;  // Allow unique assignments

    // METHODS

    ಶೂನ್ಯ ಕೂಡಿಸು_Var(AstVar* varp) {
        ಆದರೆ (m_ftaskp) {
            varp->funcLocal(ನಿಜ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_modp->prependStmtsp(varp);
        }
    }

    ಶೂನ್ಯ replaceBoundLvalue(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        ಆಗ (VN_IS(prep->backp(), ಗಂಟು_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        }
        ಆದರೆ (VN_IS(prep->backp(), ನಿಯೋಜಿಸು_Force) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_ವೃಕ್ಷ(), condp);
            ಹಿಂತಿರುಗಿ;
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ earliercondp = ifp->condp()->unlink_Fr_ಹಿಂಭಾಗ(&replaceHandle);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            ಆಗ (!VN_IS(stmtp, ಗಂಟು_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ origStmtp = stmtp->unlink_Fr_ಹಿಂಭಾಗ(&replaceHandle);
            Ast_ಗಂಟು* elseStmtp = ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasSideEffects = origStmtp->exists(
                [](Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ np) { ಹಿಂತಿರುಗಿ !np->is_ಶುದ್ಧ() || np->isTimingControl(); });
            ಆದರೆ (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{fl, VVarType::MODULETEMP, m_lvbound_ಹೆಸರುಗಳು.get(prep),
                                                prep->dtypep()};
                ಕೂಡಿಸು_Var(varp);
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ prepCopyp = prep->clonep();
                prepCopyp->replaceWith(ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(ನಿಜ);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* create_ಕೂಡಿಸು_ತಾತ್ಕಾಲಿಕ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrand_ಹೆಸರುಗಳು->get(nodep), nodep->dtypep()};
        ಕೂಡಿಸು_Var(varp);
        ಹಿಂತಿರುಗಿ varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isStaticlyGte(AstConst* ಸ್ಥಿರಾಂಕ msbConstp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp) {
        ಆದರೆ (msbConstp->ಅಗಲ() >= exprp->ಅಗಲ() && exprp->ಅಗಲ() < 32
            && msbConstp->ಅಂಕೆ().ಗಮ್ಯ_SInt() >= (1 << exprp->ಅಗಲ()) - 1) {
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(exprp, Const)) {
            ಆದರೆ (V3_ಸಂಖ್ಯೆ{msbConstp}.opGte(msbConstp->ಅಂಕೆ(), constp->ಅಂಕೆ()).isNeqZero()) {
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_ಅಭಿವ್ಯಕ್ತಿ_Stmt_Or_Clone(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& exprp) {
        ಆದರೆ (!exprp->is_ಶುದ್ಧ()) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = create_ಕೂಡಿಸು_ತಾತ್ಕಾಲಿಕ(exprp);
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = exprp->fileline();
            exprp = ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                fl, ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                ಹೊಸ AstVarRef{fl, varp, VAccess::READ}};
            ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, varp, VAccess::READ};
        }
        ಹಿಂತಿರುಗಿ exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        ಸ್ವಯಂಚಾಲಿತ xrand_ಹೆಸರುಗಳು = std::make_unique<V3_Unique_ಹೆಸರುಗಳು>(s_xrand_ಪೂರ್ವಪ್ರತ್ಯಯ);
        {
            m_modp = nodep;
            m_constXCvt = ನಿಜ;
            // Class X randomization causes Vxrand in strange places, so disable
            ಆದರೆ (VN_IS(nodep, Class)) m_allowXUnique = ಸುಳ್ಳು;
            m_lvbound_ಹೆಸರುಗಳು.reset();
            xrand_ಹೆಸರುಗಳು.ಅದಲುಬದಲು(m_xrand_ಹೆಸರುಗಳು);
            iterateChildren(nodep);
            xrand_ಹೆಸರುಗಳು.ಅದಲುಬದಲು(m_xrand_ಹೆಸರುಗಳು);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Case_ಅಂಶ* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ಸುಳ್ಳು;  // Avoid losing the X's in casex
        iterate_And_ಮುಂದಿನ_Null(nodep->condsp());
        m_constXCvt = ನಿಜ;
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ಸುಳ್ಳು;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        ಆದರೆ (nodep->isParam()) m_allowXUnique = ಸುಳ್ಳು;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ_Eq_Neq_Case(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ಆದರೆ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            ಆದರೆ (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->ಅಂಕೆ().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->ಅಂಕೆ().isFourState()))) {
                newp = ಹೊಸ AstConst(nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_ವೃಕ್ಷ(), lhsp);
                VL_DO_DANGLING(rhsp->delete_ವೃಕ್ಷ(), rhsp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (VN_IS(nodep, EqCase)) {
                    newp = ಹೊಸ AstEq{nodep->fileline(), lhsp, rhsp};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newp = ಹೊಸ AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_Eq_Neq_Wild(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ಆದರೆ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            ಆದರೆ (!VN_IS(rhsp, Const)) {
                ಆದರೆ (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = ಹೊಸ AstEq{nodep->fileline(), lhsp, rhsp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                // X or Z's become mask, ala case statements.
                V3_ಸಂಖ್ಯೆ nummask{rhsp, rhsp->ಅಗಲ()};
                nummask.op_ಬಿಟ್ಟುಗಳು_Non_XZ(VN_AS(rhsp, Const)->ಅಂಕೆ());
                V3_ಸಂಖ್ಯೆ numval{rhsp, rhsp->ಅಗಲ()};
                numval.op_ಬಿಟ್ಟುಗಳು_One(VN_AS(rhsp, Const)->ಅಂಕೆ());
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ and1p = ಹೊಸ AstAnd{nodep->fileline(), lhsp,
                                                      ಹೊಸ AstConst{nodep->fileline(), nummask}};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ and2p = ಹೊಸ AstConst{nodep->fileline(), numval};
                ಆದರೆ (VN_IS(nodep, EqWild)) {
                    newp = ಹೊಸ AstEq{nodep->fileline(), and1p, and2p};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newp = ಹೊಸ AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_ವೃಕ್ಷ(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstEqCase* nodep) override { ಭೇಟಿ_Eq_Neq_Case(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqCase* nodep) override { ಭೇಟಿ_Eq_Neq_Case(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstEqWild* nodep) override { ಭೇಟಿ_Eq_Neq_Wild(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqWild* nodep) override { ಭೇಟಿ_Eq_Neq_Wild(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<ತಾರ್ಕಿಕ, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->ಅಂಕೆ().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->ಅಂಕೆ().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nonXp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!dropop[0]) {
            nonXp = nodep->rhsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!dropop[1]) {
            nonXp = nodep->thsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!dropop[2]) {
            nonXp = nodep->fhsp();
        } ಇಲ್ಲದಿದ್ದರೆ {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (dropop[0]) {
            nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            nodep->rhsp(nonXp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
        }
        ಆದರೆ (dropop[1]) {
            nodep->thsp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            nodep->thsp(nonXp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
        }
        ಆದರೆ (dropop[2]) {
            nodep->fhsp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            nodep->fhsp(nonXp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        ಆದರೆ (m_constXCvt && nodep->ಅಂಕೆ().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_ಸಂಖ್ಯೆ numb1{nodep, nodep->ಅಗಲ()};
            numb1.op_ಬಿಟ್ಟುಗಳು_One(nodep->ಅಂಕೆ());
            V3_ಸಂಖ್ಯೆ numbx{nodep, nodep->ಅಗಲ()};
            numbx.op_ಬಿಟ್ಟುಗಳು_XZ(nodep->ಅಂಕೆ());
            ಆದರೆ (!m_allowXUnique || v3Global.opt.x_ನಿಯೋಜಿಸು() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_ಸಂಖ್ಯೆ numnew{nodep, numb1.ಅಗಲ()};
                ಆದರೆ (v3Global.opt.x_ನಿಯೋಜಿಸು() == "1") {
                    numnew.opOr(numb1, numbx);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    numnew.op_ನಿಯೋಜಿಸು(numb1);
                }
                AstConst* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                UINFO(4, "   -> " << newp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ಸ್ಥಿರಾಂಕ newvarp
                    = ಹೊಸ AstVar{nodep->fileline(), VVarType::XTEMP, m_xrand_ಹೆಸರುಗಳು->get(ಶೂನ್ಯಸೂಚಕ),
                                 VFlagLogicPacked{}, nodep->ಅಗಲ()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlink_Fr_ಹಿಂಭಾಗ(&replaceHandle);
                Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ newref1p
                    = ಹೊಸ AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ಸ್ಥಿರಾಂಕ newinitp = ಹೊಸ AstInitial{
                    nodep->fileline(),
                    ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                        nodep->fileline(),
                        ಹೊಸ AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        ಹೊಸ AstOr{nodep->fileline(), ಹೊಸ AstConst{nodep->fileline(), numb1},
                                  ಹೊಸ AstAnd{
                                      nodep->fileline(), ಹೊಸ AstConst{nodep->fileline(), numbx},
                                      ಹೊಸ AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newStmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!nodep->user1_ಗಣ_Once()) {
            // Guard against reading/writing past end of bit vector array
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ basefromp = nodep->ತಳ_Fromp(ನಿಜ);
            ತಾರ್ಕಿಕ lvalue = ಸುಳ್ಳು;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(basefromp, ಗಂಟು_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ಸ್ಥಿರಾಂಕ uint32_t maxmsb = nodep->fromp()->dtypep()->ಅಗಲ() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ಸ್ಥಿರಾಂಕ maxmsbConstp = ಹೊಸ AstConst{
                nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->lsbp()->ಅಗಲ(), maxmsb};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlink_Fr_ಹಿಂಭಾಗ());
            ಆದರೆ (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_ವೃಕ್ಷ(), maxmsbConstp);
                nodep->lsbp(lsbp);
                ಹಿಂತಿರುಗಿ;
            }
            nodep->lsbp(new_ಅಭಿವ್ಯಕ್ತಿ_Stmt_Or_Clone(lsbp));
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp
                = V3Const::constifyEdit(ಹೊಸ AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            ಆದರೆ (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_ಹಿಂಭಾಗ(&replaceHandle);
                V3_ಸಂಖ್ಯೆ xnum{nodep, nodep->ಅಗಲ()};
                xnum.ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ xexprp = ಹೊಸ AstConst{nodep->fileline(), xnum};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                    ಆದರೆ (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_ವೃಕ್ಷ(), condp);
                        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                        ಹಿಂತಿರುಗಿ xexprp;
                    }
                    ಹಿಂತಿರುಗಿ ಹೊಸ AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ಇಲ್ಲದಿದ್ದರೆ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!nodep->user1_ಗಣ_Once()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ basefromp = nodep->fromp()->ತಳ_Fromp(ನಿಜ);
            ತಾರ್ಕಿಕ lvalue = ಸುಳ್ಳು;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(basefromp, ಗಂಟು_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            ಪೂರ್ಣಾಂಕ declElements = -1;
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstModDiv* ಸ್ಥಿರಾಂಕ moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    ಆದರೆ (modconstp->ಅಗಲ() <= 32
                        && modconstp->ಗಮ್ಯ_UInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->ಗಮ್ಯ_UInt());
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ಸ್ಥಿರಾಂಕ declElementsp
                = ಹೊಸ AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->bitp()->ಅಗಲ(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp = V3Const::constifyEdit(nodep->bitp()->unlink_Fr_ಹಿಂಭಾಗ());
            ಆದರೆ (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_ವೃಕ್ಷ(), declElementsp);
                nodep->bitp(bitp);
                ಹಿಂತಿರುಗಿ;
            }
            nodep->bitp(new_ಅಭಿವ್ಯಕ್ತಿ_Stmt_Or_Clone(bitp));
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಹೊಸ AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಗಂಟು_Dtp = nodep->dtypep()->skipRefp();
            ಆದರೆ (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(ಗಂಟು_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_ಹಿಂಭಾಗ(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_ಸಂಖ್ಯೆ xnum{nodep, nodep->ಅಗಲ()};
                ಆದರೆ (ಗಂಟು_Dtp->isDouble()) {
                    xnum = V3_ಸಂಖ್ಯೆ{nodep, V3_ಸಂಖ್ಯೆ::Double{}, 0.0};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಗಂಟು_Dtp->isString()) {
                    xnum = V3_ಸಂಖ್ಯೆ{nodep, V3_ಸಂಖ್ಯೆ::String{}, ""};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (ಗಂಟು_Dtp->isFourstate()) {
                        xnum.ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        xnum.ಗಣ_All_Bits0();
                    }
                }
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCond{nodep->fileline(), condp, nodep,
                                                  ಹೊಸ AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ asBitp = nodep->bitp()->unlink_Fr_ಹಿಂಭಾಗ(&replaceHandle);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstCond{asBitp->fileline(), condp, asBitp,
                                  ಹೊಸ AstConst{asBitp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                               asBitp->ಅಗಲ(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } ಇಲ್ಲದಿದ್ದರೆ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ UnknownVisitor(AstNetlist* nodep)
        : m_lvbound_ಹೆಸರುಗಳು{"__Vlvbound"}
        , m_xrand_ಹೆಸರುಗಳು{std::make_unique<V3_Unique_ಹೆಸರುಗಳು>(s_xrand_ಪೂರ್ವಪ್ರತ್ಯಯ)} {
        iterate(nodep);
    }
    ~UnknownVisitor() override {  //
        V3Stats::ಕೂಡಿಸು_Stat("Unknowns, variables created", m_statUnkVars);
        V3Stats::ಕೂಡಿಸು_Stat("Unknowns, else branches created", m_statElses);
    }
};

ಸ್ಥಿರಾಂಕ std::string UnknownVisitor::s_xrand_ಪೂರ್ವಪ್ರತ್ಯಯ = "__Vxrand";

//######################################################################
// Unknown class functions

ಶೂನ್ಯ V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("unknown", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
