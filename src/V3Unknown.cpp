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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Unknown.h"

#समावेश "V3Const.h"
#समावेश "V3Stats.h"
#समावेश "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी UnknownVisitor final : खुला VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;
    अटल स्थिर std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    bool m_constXCvt = false;  // Convert X's
    bool m_allowXUnique = true;  // Allow unique assignments

    // METHODS

    व्योम addVar(AstVar* varp) {
        अगर (m_ftaskp) {
            varp->funcLocal(true);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } अन्यथा {
            m_modp->prependStmtsp(varp);
        }
    }

    व्योम replaceBoundLvalue(Ast_गाँठ_व्यंजक* nodep, Ast_गाँठ_व्यंजक* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_गाँठ_व्यंजक* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        जबतक (VN_IS(prep->backp(), गाँठ_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), गाँठ_व्यंजक);
        }
        अगर (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_वृक्ष(), condp);
            वापस;
        }
        File_पंक्ति* स्थिर fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        अगर (स्थिर AstIf* स्थिर ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_गाँठ_व्यंजक* स्थिर earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_गाँठ_व्यंजक* स्थिर newp = नया AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } अन्यथा {
            Ast_गाँठ* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            जबतक (!VN_IS(stmtp, गाँठ_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_गाँठ* स्थिर origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_गाँठ* elseStmtp = nullptr;
            स्थिर bool hasSideEffects = origStmtp->exists(
                [](Ast_गाँठ* स्थिर np) { वापस !np->isPure() || np->isTimingControl(); });
            अगर (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_वृक्ष(false);
                AstVar* स्थिर varp = नया AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.लो(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_गाँठ* स्थिर prepCopyp = prep->clonep();
                prepCopyp->replaceWith(नया AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(धकेलो_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* स्थिर newp = नया AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(true);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* create_Add_अस्थायी(स्थिर Ast_गाँठ_व्यंजक* स्थिर nodep) {
        AstVar* स्थिर varp = नया AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->लो(nodep), nodep->dtypep()};
        addVar(varp);
        वापस varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    अटल bool isStaticlyGte(AstConst* स्थिर msbConstp, स्थिर Ast_गाँठ_व्यंजक* स्थिर exprp) {
        अगर (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->अंक().toSInt() >= (1 << exprp->width()) - 1) {
            वापस true;
        }
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(exprp, Const)) {
            अगर (V3_संख्या{msbConstp}.opGte(msbConstp->अंक(), constp->अंक()).isNeqZero()) {
                वापस true;
            }
        }
        वापस false;
    }

    Ast_गाँठ_व्यंजक* new_व्यंजक_Stmt_Or_Clone(Ast_गाँठ_व्यंजक*& exprp) {
        अगर (!exprp->isPure()) {
            AstVar* स्थिर varp = create_Add_अस्थायी(exprp);
            File_पंक्ति* स्थिर fl = exprp->fileline();
            exprp = नया Ast_व्यंजक_Stmt{
                fl, नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                नया AstVarRef{fl, varp, VAccess::READ}};
            वापस नया AstVarRef{fl, varp, VAccess::READ};
        }
        वापस exprp->clone_वृक्ष_Pure(false);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        स्वतः xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = true;
            // Class X randomization causes Vxrand in strange places, so disable
            अगर (VN_IS(nodep, Class)) m_allowXUnique = false;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = false;  // Avoid losing the X's in casex
        iterate_And_अग्र_Null(nodep->condsp());
        m_constXCvt = true;
        iterate_And_अग्र_Null(nodep->stmtsp());
    }
    व्योम visit(Ast_गाँठ_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = false;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    व्योम visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        अगर (nodep->isParam()) m_allowXUnique = false;
        iterateChildren(nodep);
    }
    व्योम visitEqNeqCase(Ast_गाँठ_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        अगर (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            वापस;
        } अन्यथा {
            Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            अगर (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->अंक().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->अंक().isFourState()))) {
                newp = नया AstConst(nodep->fileline(), AstConst::Widthed_मूल्य{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_वृक्ष(), lhsp);
                VL_DO_DANGLING(rhsp->delete_वृक्ष(), rhsp);
            } अन्यथा {
                अगर (VN_IS(nodep, EqCase)) {
                    newp = नया AstEq{nodep->fileline(), lhsp, rhsp};
                } अन्यथा {
                    newp = नया AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    व्योम visitEqNeqWild(Ast_गाँठ_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        अगर (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            वापस;
        } अन्यथा {
            Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* newp;
            अगर (!VN_IS(rhsp, Const)) {
                अगर (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = नया AstEq{nodep->fileline(), lhsp, rhsp};
            } अन्यथा {
                // X or Z's become mask, ala case statements.
                V3_संख्या nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->अंक());
                V3_संख्या numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->अंक());
                Ast_गाँठ_व्यंजक* स्थिर and1p = नया AstAnd{nodep->fileline(), lhsp,
                                                      नया AstConst{nodep->fileline(), nummask}};
                Ast_गाँठ_व्यंजक* स्थिर and2p = नया AstConst{nodep->fileline(), numval};
                अगर (VN_IS(nodep, EqWild)) {
                    newp = नया AstEq{nodep->fileline(), and1p, and2p};
                } अन्यथा {
                    newp = नया AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_वृक्ष(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    व्योम visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    व्योम visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    व्योम visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    व्योम visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    व्योम visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* स्थिर newp = नया AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(Ast_गिनती_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<bool, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->अंक().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->अंक().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->अंक().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_गाँठ_व्यंजक* nonXp = nullptr;
        अगर (!dropop[0]) {
            nonXp = nodep->rhsp();
        } अन्यथा अगर (!dropop[1]) {
            nonXp = nodep->thsp();
        } अन्यथा अगर (!dropop[2]) {
            nonXp = nodep->fhsp();
        } अन्यथा {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* स्थिर newp = नया AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            वापस;
        }
        अगर (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_वृक्ष();
            nodep->rhsp(nonXp->clone_वृक्ष_Pure(true));
        }
        अगर (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_वृक्ष();
            nodep->thsp(nonXp->clone_वृक्ष_Pure(true));
        }
        अगर (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_वृक्ष();
            nodep->fhsp(nonXp->clone_वृक्ष_Pure(true));
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstConst* nodep) override {
        अगर (m_constXCvt && nodep->अंक().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_संख्या numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->अंक());
            V3_संख्या numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->अंक());
            अगर (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_संख्या numnew{nodep, numb1.width()};
                अगर (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } अन्यथा {
                    numnew.opAssign(numb1);
                }
                AstConst* स्थिर newp = नया AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                UINFO(4, "   -> " << newp);
            } अन्यथा {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* स्थिर newvarp
                    = नया AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->लो(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_गाँठ_Var_Ref* स्थिर newref1p
                    = नया AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* स्थिर newinitp = नया AstInitial{
                    nodep->fileline(),
                    नया AstAssign{
                        nodep->fileline(),
                        नया AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        नया AstOr{nodep->fileline(), नया AstConst{nodep->fileline(), numb1},
                                  नया AstAnd{
                                      nodep->fileline(), नया AstConst{nodep->fileline(), numbx},
                                      नया AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_गाँठ* स्थिर newStmtsp = Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            }
        }
    }

    व्योम visit(AstSel* nodep) override {
        iterateChildren(nodep);
        अगर (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            स्थिर Ast_गाँठ* स्थिर basefromp = nodep->baseFromp(true);
            bool lvalue = false;
            अगर (स्थिर Ast_गाँठ_Var_Ref* स्थिर varrefp = VN_CAST(basefromp, गाँठ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            स्थिर uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* स्थिर maxmsbConstp = नया AstConst{
                nodep->fileline(), AstConst::Widthed_मूल्य{}, nodep->lsbp()->width(), maxmsb};
            Ast_गाँठ_व्यंजक* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            अगर (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_वृक्ष(), maxmsbConstp);
                nodep->lsbp(lsbp);
                वापस;
            }
            nodep->lsbp(new_व्यंजक_Stmt_Or_Clone(lsbp));
            Ast_गाँठ_व्यंजक* condp
                = V3Const::constifyEdit(नया AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            अगर (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_संख्या xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_गाँठ_व्यंजक* स्थिर xexprp = नया AstConst{nodep->fileline(), xnum};
                Ast_गाँठ_व्यंजक* स्थिर newp = [&]() -> Ast_गाँठ_व्यंजक* {
                    अगर (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_वृक्ष(), condp);
                        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                        वापस xexprp;
                    }
                    वापस नया AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } अन्यथा {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    व्योम visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        अगर (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_गाँठ* स्थिर basefromp = nodep->fromp()->baseFromp(true);
            bool lvalue = false;
            अगर (स्थिर Ast_गाँठ_Var_Ref* स्थिर varrefp = VN_CAST(basefromp, गाँठ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } अन्यथा अगर (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } अन्यथा {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            पूर्णांक declElements = -1;
            Ast_गाँठ_DType* स्थिर dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            स्थिर Ast_गाँठ_Array_DType* स्थिर adtypep = VN_CAST(dtypep, गाँठ_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            अगर (स्थिर AstModDiv* स्थिर moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                अगर (स्थिर AstConst* स्थिर modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    अगर (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        वापस;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* स्थिर declElementsp
                = नया AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_गाँठ_व्यंजक* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            अगर (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_वृक्ष(), declElementsp);
                nodep->bitp(bitp);
                वापस;
            }
            nodep->bitp(new_व्यंजक_Stmt_Or_Clone(bitp));
            Ast_गाँठ_व्यंजक* condp = नया AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            स्थिर Ast_गाँठ_DType* स्थिर गाँठ_Dtp = nodep->dtypep()->skipRefp();
            अगर (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(गाँठ_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_संख्या xnum{nodep, nodep->width()};
                अगर (गाँठ_Dtp->isDouble()) {
                    xnum = V3_संख्या{nodep, V3_संख्या::Double{}, 0.0};
                } अन्यथा अगर (गाँठ_Dtp->isString()) {
                    xnum = V3_संख्या{nodep, V3_संख्या::String{}, ""};
                } अन्यथा {
                    अगर (गाँठ_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } अन्यथा {
                        xnum.setAllBits0();
                    }
                }
                Ast_गाँठ* स्थिर newp = नया AstCond{nodep->fileline(), condp, nodep,
                                                  नया AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } अन्यथा अगर (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_गाँठ_व्यंजक* स्थिर asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_गाँठ_व्यंजक* स्थिर newp
                    = नया AstCond{asBitp->fileline(), condp, asBitp,
                                  नया AstConst{asBitp->fileline(), AstConst::Widthed_मूल्य{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } अन्यथा {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
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

स्थिर std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

व्योम V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("unknown", 0, dump_वृक्ष_Either_Level() >= 3);
}
