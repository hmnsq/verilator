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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Unknown.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ UnknownVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    ਸਥਿਰ ਅਚਲ std::string s_xrand_ਅਗੇਤਰ;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3_Unique_ਨਾਂ m_lvbound_ਨਾਂ;  // For generating unique temporary variable names
    std::unique_ptr<V3_Unique_ਨਾਂ> m_xrand_ਨਾਂ;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    ਤਾਰਕਿਕ m_constXCvt = ਝੂਠ;  // Convert X's
    ਤਾਰਕਿਕ m_allowXUnique = ਸੱਚ;  // Allow unique assignments

    // METHODS

    ਖਾਲੀ ਜੋੜੋ_Var(AstVar* varp) {
        ਜੇ (m_ftaskp) {
            varp->funcLocal(ਸੱਚ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } ਨਹੀਂਤਾਂ {
            m_modp->prependStmtsp(varp);
        }
    }

    ਖਾਲੀ replaceBoundLvalue(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        ਜਦੋਂ (VN_IS(prep->backp(), ਗੰਢ_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), ਗੰਢ_ਵਿਅੰਜਕ);
        }
        ਜੇ (VN_IS(prep->backp(), ਸੌਂਪੋ_Force) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_ਰੁੱਖ(), condp);
            ਵਾਪਸ;
        }
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        ਜੇ (ਅਚਲ AstIf* ਅਚਲ ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ earliercondp = ifp->condp()->unlink_Fr_ਪਿੱਠ(&replaceHandle);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            ਜਦੋਂ (!VN_IS(stmtp, ਗੰਢ_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_ਗੰਢ* ਅਚਲ origStmtp = stmtp->unlink_Fr_ਪਿੱਠ(&replaceHandle);
            Ast_ਗੰਢ* elseStmtp = ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ ਤਾਰਕਿਕ hasSideEffects = origStmtp->exists(
                [](Ast_ਗੰਢ* ਅਚਲ np) { ਵਾਪਸ !np->is_ਸ਼ੁੱਧ() || np->isTimingControl(); });
            ਜੇ (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_ਰੁੱਖ(ਝੂਠ);
                AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{fl, VVarType::MODULETEMP, m_lvbound_ਨਾਂ.get(prep),
                                                prep->dtypep()};
                ਜੋੜੋ_Var(varp);
                Ast_ਗੰਢ* ਅਚਲ prepCopyp = prep->clonep();
                prepCopyp->replaceWith(ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ਅਚਲ newp = ਨਵਾਂ AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(ਸੱਚ);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* create_ਜੋੜੋ_ਆਰਜ਼ੀ(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep) {
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrand_ਨਾਂ->get(nodep), nodep->dtypep()};
        ਜੋੜੋ_Var(varp);
        ਵਾਪਸ varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ਸਥਿਰ ਤਾਰਕਿਕ isStaticlyGte(AstConst* ਅਚਲ msbConstp, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp) {
        ਜੇ (msbConstp->ਚੌੜਾਈ() >= exprp->ਚੌੜਾਈ() && exprp->ਚੌੜਾਈ() < 32
            && msbConstp->ਅੰਕ().ਮੰਜ਼ਿਲ_SInt() >= (1 << exprp->ਚੌੜਾਈ()) - 1) {
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(exprp, Const)) {
            ਜੇ (V3_ਸੰਖਿਆ{msbConstp}.opGte(msbConstp->ਅੰਕ(), constp->ਅੰਕ()).isNeqZero()) {
                ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਵਿਅੰਜਕ_Stmt_Or_Clone(Ast_ਗੰਢ_ਵਿਅੰਜਕ*& exprp) {
        ਜੇ (!exprp->is_ਸ਼ੁੱਧ()) {
            AstVar* ਅਚਲ varp = create_ਜੋੜੋ_ਆਰਜ਼ੀ(exprp);
            File_ਸਤਰ* ਅਚਲ fl = exprp->fileline();
            exprp = ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{
                fl, ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}};
            ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ};
        }
        ਵਾਪਸ exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        ਸਵੈਚਾਲਿਤ xrand_ਨਾਂ = std::make_unique<V3_Unique_ਨਾਂ>(s_xrand_ਅਗੇਤਰ);
        {
            m_modp = nodep;
            m_constXCvt = ਸੱਚ;
            // Class X randomization causes Vxrand in strange places, so disable
            ਜੇ (VN_IS(nodep, Class)) m_allowXUnique = ਝੂਠ;
            m_lvbound_ਨਾਂ.reset();
            xrand_ਨਾਂ.ਅਦਲਾ_ਬਦਲੀ(m_xrand_ਨਾਂ);
            iterateChildren(nodep);
            xrand_ਨਾਂ.ਅਦਲਾ_ਬਦਲੀ(m_xrand_ਨਾਂ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Case_ਵਸਤੂ* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ਝੂਠ;  // Avoid losing the X's in casex
        iterate_And_ਅਗਲਾ_Null(nodep->condsp());
        m_constXCvt = ਸੱਚ;
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ਝੂਠ;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        ਜੇ (nodep->isParam()) m_allowXUnique = ਝੂਠ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ_Eq_Neq_Case(Ast_ਗੰਢ_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ਜੇ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            ਜੇ (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->ਅੰਕ().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->ਅੰਕ().isFourState()))) {
                newp = ਨਵਾਂ AstConst(nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_ਰੁੱਖ(), lhsp);
                VL_DO_DANGLING(rhsp->delete_ਰੁੱਖ(), rhsp);
            } ਨਹੀਂਤਾਂ {
                ਜੇ (VN_IS(nodep, EqCase)) {
                    newp = ਨਵਾਂ AstEq{nodep->fileline(), lhsp, rhsp};
                } ਨਹੀਂਤਾਂ {
                    newp = ਨਵਾਂ AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ_Eq_Neq_Wild(Ast_ਗੰਢ_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ਜੇ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
            ਜੇ (!VN_IS(rhsp, Const)) {
                ਜੇ (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = ਨਵਾਂ AstEq{nodep->fileline(), lhsp, rhsp};
            } ਨਹੀਂਤਾਂ {
                // X or Z's become mask, ala case statements.
                V3_ਸੰਖਿਆ nummask{rhsp, rhsp->ਚੌੜਾਈ()};
                nummask.op_ਬਿੱਟ_Non_XZ(VN_AS(rhsp, Const)->ਅੰਕ());
                V3_ਸੰਖਿਆ numval{rhsp, rhsp->ਚੌੜਾਈ()};
                numval.op_ਬਿੱਟ_One(VN_AS(rhsp, Const)->ਅੰਕ());
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ and1p = ਨਵਾਂ AstAnd{nodep->fileline(), lhsp,
                                                      ਨਵਾਂ AstConst{nodep->fileline(), nummask}};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ and2p = ਨਵਾਂ AstConst{nodep->fileline(), numval};
                ਜੇ (VN_IS(nodep, EqWild)) {
                    newp = ਨਵਾਂ AstEq{nodep->fileline(), and1p, and2p};
                } ਨਹੀਂਤਾਂ {
                    newp = ਨਵਾਂ AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_ਰੁੱਖ(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstEqCase* nodep) override { ਦੇਖੋ_Eq_Neq_Case(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqCase* nodep) override { ਦੇਖੋ_Eq_Neq_Case(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstEqWild* nodep) override { ਦੇਖੋ_Eq_Neq_Wild(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqWild* nodep) override { ਦੇਖੋ_Eq_Neq_Wild(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗਿਣਤੀ_ਬਿੱਟ* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<ਤਾਰਕਿਕ, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->ਅੰਕ().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->ਅੰਕ().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->ਅੰਕ().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* nonXp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!dropop[0]) {
            nonXp = nodep->rhsp();
        } ਨਹੀਂਤਾਂ ਜੇ (!dropop[1]) {
            nonXp = nodep->thsp();
        } ਨਹੀਂਤਾਂ ਜੇ (!dropop[2]) {
            nonXp = nodep->fhsp();
        } ਨਹੀਂਤਾਂ {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (dropop[0]) {
            nodep->rhsp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            nodep->rhsp(nonXp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
        }
        ਜੇ (dropop[1]) {
            nodep->thsp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            nodep->thsp(nonXp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
        }
        ਜੇ (dropop[2]) {
            nodep->fhsp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            nodep->fhsp(nonXp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        ਜੇ (m_constXCvt && nodep->ਅੰਕ().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_ਸੰਖਿਆ numb1{nodep, nodep->ਚੌੜਾਈ()};
            numb1.op_ਬਿੱਟ_One(nodep->ਅੰਕ());
            V3_ਸੰਖਿਆ numbx{nodep, nodep->ਚੌੜਾਈ()};
            numbx.op_ਬਿੱਟ_XZ(nodep->ਅੰਕ());
            ਜੇ (!m_allowXUnique || v3Global.opt.x_ਸੌਂਪੋ() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_ਸੰਖਿਆ numnew{nodep, numb1.ਚੌੜਾਈ()};
                ਜੇ (v3Global.opt.x_ਸੌਂਪੋ() == "1") {
                    numnew.opOr(numb1, numbx);
                } ਨਹੀਂਤਾਂ {
                    numnew.op_ਸੌਂਪੋ(numb1);
                }
                AstConst* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                UINFO(4, "   -> " << newp);
            } ਨਹੀਂਤਾਂ {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ਅਚਲ newvarp
                    = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::XTEMP, m_xrand_ਨਾਂ->get(ਖਾਲੀ_ਸੂਚਕ),
                                 VFlagLogicPacked{}, nodep->ਚੌੜਾਈ()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlink_Fr_ਪਿੱਠ(&replaceHandle);
                Ast_ਗੰਢ_Var_Ref* ਅਚਲ newref1p
                    = ਨਵਾਂ AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ਅਚਲ newinitp = ਨਵਾਂ AstInitial{
                    nodep->fileline(),
                    ਨਵਾਂ Ast_ਸੌਂਪੋ{
                        nodep->fileline(),
                        ਨਵਾਂ AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        ਨਵਾਂ AstOr{nodep->fileline(), ਨਵਾਂ AstConst{nodep->fileline(), numb1},
                                  ਨਵਾਂ AstAnd{
                                      nodep->fileline(), ਨਵਾਂ AstConst{nodep->fileline(), numbx},
                                      ਨਵਾਂ AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_ਗੰਢ* ਅਚਲ newStmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!nodep->user1_ਸੈੱਟ_Once()) {
            // Guard against reading/writing past end of bit vector array
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ basefromp = nodep->ਅਧਾਰ_Fromp(ਸੱਚ);
            ਤਾਰਕਿਕ lvalue = ਝੂਠ;
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(basefromp, ਗੰਢ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ਅਚਲ uint32_t maxmsb = nodep->fromp()->dtypep()->ਚੌੜਾਈ() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ਅਚਲ maxmsbConstp = ਨਵਾਂ AstConst{
                nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{}, nodep->lsbp()->ਚੌੜਾਈ(), maxmsb};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlink_Fr_ਪਿੱਠ());
            ਜੇ (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_ਰੁੱਖ(), maxmsbConstp);
                nodep->lsbp(lsbp);
                ਵਾਪਸ;
            }
            nodep->lsbp(new_ਵਿਅੰਜਕ_Stmt_Or_Clone(lsbp));
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp
                = V3Const::constifyEdit(ਨਵਾਂ AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            ਜੇ (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_ਪਿੱਠ(&replaceHandle);
                V3_ਸੰਖਿਆ xnum{nodep, nodep->ਚੌੜਾਈ()};
                xnum.ਸੈੱਟ_All_ਬਿੱਟ_X();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ xexprp = ਨਵਾਂ AstConst{nodep->fileline(), xnum};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                    ਜੇ (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_ਰੁੱਖ(), condp);
                        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                        ਵਾਪਸ xexprp;
                    }
                    ਵਾਪਸ ਨਵਾਂ AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ਨਹੀਂਤਾਂ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!nodep->user1_ਸੈੱਟ_Once()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_ਗੰਢ* ਅਚਲ basefromp = nodep->fromp()->ਅਧਾਰ_Fromp(ਸੱਚ);
            ਤਾਰਕਿਕ lvalue = ਝੂਠ;
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(basefromp, ਗੰਢ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } ਨਹੀਂਤਾਂ {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            ਪੂਰਨ declElements = -1;
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ adtypep = VN_CAST(dtypep, ਗੰਢ_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            ਜੇ (ਅਚਲ AstModDiv* ਅਚਲ moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                ਜੇ (ਅਚਲ AstConst* ਅਚਲ modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    ਜੇ (modconstp->ਚੌੜਾਈ() <= 32
                        && modconstp->ਮੰਜ਼ਿਲ_UInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->ਮੰਜ਼ਿਲ_UInt());
                        ਵਾਪਸ;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ਅਚਲ declElementsp
                = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{}, nodep->bitp()->ਚੌੜਾਈ(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitp = V3Const::constifyEdit(nodep->bitp()->unlink_Fr_ਪਿੱਠ());
            ਜੇ (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_ਰੁੱਖ(), declElementsp);
                nodep->bitp(bitp);
                ਵਾਪਸ;
            }
            nodep->bitp(new_ਵਿਅੰਜਕ_Stmt_Or_Clone(bitp));
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਨਵਾਂ AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਗੰਢ_Dtp = nodep->dtypep()->skipRefp();
            ਜੇ (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(ਗੰਢ_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_ਪਿੱਠ(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_ਸੰਖਿਆ xnum{nodep, nodep->ਚੌੜਾਈ()};
                ਜੇ (ਗੰਢ_Dtp->isDouble()) {
                    xnum = V3_ਸੰਖਿਆ{nodep, V3_ਸੰਖਿਆ::Double{}, 0.0};
                } ਨਹੀਂਤਾਂ ਜੇ (ਗੰਢ_Dtp->isString()) {
                    xnum = V3_ਸੰਖਿਆ{nodep, V3_ਸੰਖਿਆ::String{}, ""};
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (ਗੰਢ_Dtp->isFourstate()) {
                        xnum.ਸੈੱਟ_All_ਬਿੱਟ_X();
                    } ਨਹੀਂਤਾਂ {
                        xnum.ਸੈੱਟ_All_Bits0();
                    }
                }
                Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstCond{nodep->fileline(), condp, nodep,
                                                  ਨਵਾਂ AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ਨਹੀਂਤਾਂ ਜੇ (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ asBitp = nodep->bitp()->unlink_Fr_ਪਿੱਠ(&replaceHandle);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
                    = ਨਵਾਂ AstCond{asBitp->fileline(), condp, asBitp,
                                  ਨਵਾਂ AstConst{asBitp->fileline(), AstConst::Widthed_ਮੁੱਲ{},
                                               asBitp->ਚੌੜਾਈ(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } ਨਹੀਂਤਾਂ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ UnknownVisitor(AstNetlist* nodep)
        : m_lvbound_ਨਾਂ{"__Vlvbound"}
        , m_xrand_ਨਾਂ{std::make_unique<V3_Unique_ਨਾਂ>(s_xrand_ਅਗੇਤਰ)} {
        iterate(nodep);
    }
    ~UnknownVisitor() override {  //
        V3Stats::ਜੋੜੋ_Stat("Unknowns, variables created", m_statUnkVars);
        V3Stats::ਜੋੜੋ_Stat("Unknowns, else branches created", m_statElses);
    }
};

ਅਚਲ std::string UnknownVisitor::s_xrand_ਅਗੇਤਰ = "__Vxrand";

//######################################################################
// Unknown class functions

ਖਾਲੀ V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("unknown", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
