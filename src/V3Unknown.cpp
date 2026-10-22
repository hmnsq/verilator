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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Unknown.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ UnknownVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;
    ସ୍ଥିର ଅଚଳ std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    ତାର୍କିକ m_constXCvt = ମିଥ୍ୟା;  // Convert X's
    ତାର୍କିକ m_allowXUnique = ସତ୍ୟ;  // Allow unique assignments

    // METHODS

    ଶୂନ୍ୟ addVar(AstVar* varp) {
        ଯଦି (m_ftaskp) {
            varp->funcLocal(ସତ୍ୟ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } ଅନ୍ୟଥା {
            m_modp->prependStmtsp(varp);
        }
    }

    ଶୂନ୍ୟ replaceBoundLvalue(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        ଯେତେବେଳେ (VN_IS(prep->backp(), ଗଣ୍ଠି_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        }
        ଯଦି (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_ବୃକ୍ଷ(), condp);
            ଫେରନ୍ତୁ;
        }
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        ଯଦି (ଅଚଳ AstIf* ଅଚଳ ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            ଯେତେବେଳେ (!VN_IS(stmtp, ଗଣ୍ଠି_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_ଗଣ୍ଠି* ଅଚଳ origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_ଗଣ୍ଠି* elseStmtp = nullptr;
            ଅଚଳ ତାର୍କିକ hasSideEffects = origStmtp->exists(
                [](Ast_ଗଣ୍ଠି* ଅଚଳ np) { ଫେରନ୍ତୁ !np->isPure() || np->isTimingControl(); });
            ଯଦି (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                AstVar* ଅଚଳ varp = ନୂତନ AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_ଗଣ୍ଠି* ଅଚଳ prepCopyp = prep->clonep();
                prepCopyp->replaceWith(ନୂତନ AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ଠେଲ_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ଅଚଳ newp = ନୂତନ AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(ସତ୍ୟ);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nodep) {
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        ଫେରନ୍ତୁ varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    ସ୍ଥିର ତାର୍କିକ isStaticlyGte(AstConst* ଅଚଳ msbConstp, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp) {
        ଯଦି (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->ଅଙ୍କ().toSInt() >= (1 << exprp->width()) - 1) {
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(exprp, Const)) {
            ଯଦି (V3_ସଂଖ୍ୟା{msbConstp}.opGte(msbConstp->ଅଙ୍କ(), constp->ଅଙ୍କ()).isNeqZero()) {
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* new_ପଦସମଷ୍ଟି_Stmt_Or_Clone(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& exprp) {
        ଯଦି (!exprp->isPure()) {
            AstVar* ଅଚଳ varp = createAddTemp(exprp);
            File_ଧାଡ଼ି* ଅଚଳ fl = exprp->fileline();
            exprp = ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{
                fl, ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                ନୂତନ AstVarRef{fl, varp, VAccess::READ}};
            ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, varp, VAccess::READ};
        }
        ଫେରନ୍ତୁ exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = ସତ୍ୟ;
            // Class X randomization causes Vxrand in strange places, so disable
            ଯଦି (VN_IS(nodep, Class)) m_allowXUnique = ମିଥ୍ୟା;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ମିଥ୍ୟା;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = ସତ୍ୟ;
        iterateAndNextNull(nodep->stmtsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = ମିଥ୍ୟା;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        ଯଦି (nodep->isParam()) m_allowXUnique = ମିଥ୍ୟା;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visitEqNeqCase(Ast_ଗଣ୍ଠି_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ଯଦି (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            ଯଦି (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->ଅଙ୍କ().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->ଅଙ୍କ().isFourState()))) {
                newp = ନୂତନ AstConst(nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_ବୃକ୍ଷ(), lhsp);
                VL_DO_DANGLING(rhsp->delete_ବୃକ୍ଷ(), rhsp);
            } ଅନ୍ୟଥା {
                ଯଦି (VN_IS(nodep, EqCase)) {
                    newp = ନୂତନ AstEq{nodep->fileline(), lhsp, rhsp};
                } ଅନ୍ୟଥା {
                    newp = ନୂତନ AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    ଶୂନ୍ୟ visitEqNeqWild(Ast_ଗଣ୍ଠି_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ଯଦି (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
            ଯଦି (!VN_IS(rhsp, Const)) {
                ଯଦି (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = ନୂତନ AstEq{nodep->fileline(), lhsp, rhsp};
            } ଅନ୍ୟଥା {
                // X or Z's become mask, ala case statements.
                V3_ସଂଖ୍ୟା nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->ଅଙ୍କ());
                V3_ସଂଖ୍ୟା numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->ଅଙ୍କ());
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ and1p = ନୂତନ AstAnd{nodep->fileline(), lhsp,
                                                      ନୂତନ AstConst{nodep->fileline(), nummask}};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ and2p = ନୂତନ AstConst{nodep->fileline(), numval};
                ଯଦି (VN_IS(nodep, EqWild)) {
                    newp = ନୂତନ AstEq{nodep->fileline(), and1p, and2p};
                } ଅନ୍ୟଥା {
                    newp = ନୂତନ AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_ବୃକ୍ଷ(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    ଶୂନ୍ୟ visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    ଶୂନ୍ୟ visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    ଶୂନ୍ୟ visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    ଶୂନ୍ୟ visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    ଶୂନ୍ୟ visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣନା_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<ତାର୍କିକ, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->ଅଙ୍କ().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->ଅଙ୍କ().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->ଅଙ୍କ().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nonXp = nullptr;
        ଯଦି (!dropop[0]) {
            nonXp = nodep->rhsp();
        } ଅନ୍ୟଥା ଯଦି (!dropop[1]) {
            nonXp = nodep->thsp();
        } ଅନ୍ୟଥା ଯଦି (!dropop[2]) {
            nonXp = nodep->fhsp();
        } ଅନ୍ୟଥା {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
            nodep->rhsp(nonXp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
        }
        ଯଦି (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
            nodep->thsp(nonXp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
        }
        ଯଦି (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
            nodep->fhsp(nonXp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        ଯଦି (m_constXCvt && nodep->ଅଙ୍କ().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_ସଂଖ୍ୟା numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->ଅଙ୍କ());
            V3_ସଂଖ୍ୟା numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->ଅଙ୍କ());
            ଯଦି (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_ସଂଖ୍ୟା numnew{nodep, numb1.width()};
                ଯଦି (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } ଅନ୍ୟଥା {
                    numnew.opAssign(numb1);
                }
                AstConst* ଅଚଳ newp = ନୂତନ AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                UINFO(4, "   -> " << newp);
            } ଅନ୍ୟଥା {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ଅଚଳ newvarp
                    = ନୂତନ AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ newref1p
                    = ନୂତନ AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ଅଚଳ newinitp = ନୂତନ AstInitial{
                    nodep->fileline(),
                    ନୂତନ AstAssign{
                        nodep->fileline(),
                        ନୂତନ AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        ନୂତନ AstOr{nodep->fileline(), ନୂତନ AstConst{nodep->fileline(), numb1},
                                  ନୂତନ AstAnd{
                                      nodep->fileline(), ନୂତନ AstConst{nodep->fileline(), numbx},
                                      ନୂତନ AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_ଗଣ୍ଠି* ଅଚଳ newStmtsp = Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            }
        }
    }

    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ basefromp = nodep->baseFromp(ସତ୍ୟ);
            ତାର୍କିକ lvalue = ମିଥ୍ୟା;
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(basefromp, ଗଣ୍ଠି_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ଅଚଳ uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ଅଚଳ maxmsbConstp = ନୂତନ AstConst{
                nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, nodep->lsbp()->width(), maxmsb};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            ଯଦି (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_ବୃକ୍ଷ(), maxmsbConstp);
                nodep->lsbp(lsbp);
                ଫେରନ୍ତୁ;
            }
            nodep->lsbp(new_ପଦସମଷ୍ଟି_Stmt_Or_Clone(lsbp));
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp
                = V3Const::constifyEdit(ନୂତନ AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            ଯଦି (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_ସଂଖ୍ୟା xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ xexprp = ନୂତନ AstConst{nodep->fileline(), xnum};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                    ଯଦି (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_ବୃକ୍ଷ(), condp);
                        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                        ଫେରନ୍ତୁ xexprp;
                    }
                    ଫେରନ୍ତୁ ନୂତନ AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ଅନ୍ୟଥା {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_ଗଣ୍ଠି* ଅଚଳ basefromp = nodep->fromp()->baseFromp(ସତ୍ୟ);
            ତାର୍କିକ lvalue = ମିଥ୍ୟା;
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(basefromp, ଗଣ୍ଠି_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } ଅନ୍ୟଥା ଯଦି (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } ଅନ୍ୟଥା {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା declElements = -1;
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ adtypep = VN_CAST(dtypep, ଗଣ୍ଠି_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            ଯଦି (ଅଚଳ AstModDiv* ଅଚଳ moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                ଯଦି (ଅଚଳ AstConst* ଅଚଳ modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    ଯଦି (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        ଫେରନ୍ତୁ;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ଅଚଳ declElementsp
                = ନୂତନ AstConst{nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            ଯଦି (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_ବୃକ୍ଷ(), declElementsp);
                nodep->bitp(bitp);
                ଫେରନ୍ତୁ;
            }
            nodep->bitp(new_ପଦସମଷ୍ଟି_Stmt_Or_Clone(bitp));
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = ନୂତନ AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ ଗଣ୍ଠି_Dtp = nodep->dtypep()->skipRefp();
            ଯଦି (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(ଗଣ୍ଠି_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_ସଂଖ୍ୟା xnum{nodep, nodep->width()};
                ଯଦି (ଗଣ୍ଠି_Dtp->isDouble()) {
                    xnum = V3_ସଂଖ୍ୟା{nodep, V3_ସଂଖ୍ୟା::Double{}, 0.0};
                } ଅନ୍ୟଥା ଯଦି (ଗଣ୍ଠି_Dtp->isString()) {
                    xnum = V3_ସଂଖ୍ୟା{nodep, V3_ସଂଖ୍ୟା::String{}, ""};
                } ଅନ୍ୟଥା {
                    ଯଦି (ଗଣ୍ଠି_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } ଅନ୍ୟଥା {
                        xnum.setAllBits0();
                    }
                }
                Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstCond{nodep->fileline(), condp, nodep,
                                                  ନୂତନ AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } ଅନ୍ୟଥା ଯଦି (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
                    = ନୂତନ AstCond{asBitp->fileline(), condp, asBitp,
                                  ନୂତନ AstConst{asBitp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } ଅନ୍ୟଥା {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
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

ଅଚଳ std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

ଶୂନ୍ୟ V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("unknown", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
