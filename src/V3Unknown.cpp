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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Unknown.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वग्ग UnknownVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;
    स्थिर ध्रुव std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
    Ast_ग्रन्थिः_FTask* m_ftaskp = nullptr;  // Current function/task
    तार्किक m_constXCvt = मिच्छा;  // Convert X's
    तार्किक m_allowXUnique = सच्च;  // Allow unique assignments

    // METHODS

    शून्य addVar(AstVar* varp) {
        सचे (m_ftaskp) {
            varp->funcLocal(सच्च);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } अञ्ञथा {
            m_modp->prependStmtsp(varp);
        }
    }

    शून्य replaceBoundLvalue(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_ग्रन्थिः_व्यञ्जकम्* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        याव (VN_IS(prep->backp(), ग्रन्थिः_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), ग्रन्थिः_व्यञ्जकम्);
        }
        सचे (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_वृक्षः(), condp);
            पटिनिवत्त;
        }
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        सचे (ध्रुव AstIf* ध्रुव ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } अञ्ञथा {
            Ast_ग्रन्थिः* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            याव (!VN_IS(stmtp, ग्रन्थिः_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_ग्रन्थिः* ध्रुव origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_ग्रन्थिः* elseStmtp = nullptr;
            ध्रुव तार्किक hasSideEffects = origStmtp->exists(
                [](Ast_ग्रन्थिः* ध्रुव np) { पटिनिवत्त !np->isPure() || np->isTimingControl(); });
            सचे (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_वृक्षः(मिच्छा);
                AstVar* ध्रुव varp = नूतन AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_ग्रन्थिः* ध्रुव prepCopyp = prep->clonep();
                prepCopyp->replaceWith(नूतन AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(प्रक्षिप_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* ध्रुव newp = नूतन AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(सच्च);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
        AstVar* ध्रुव varp = नूतन AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        पटिनिवत्त varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    स्थिर तार्किक isStaticlyGte(AstConst* ध्रुव msbConstp, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp) {
        सचे (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->अङ्कः().toSInt() >= (1 << exprp->width()) - 1) {
            पटिनिवत्त सच्च;
        }
        सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(exprp, Const)) {
            सचे (V3_सङ्ख्या{msbConstp}.opGte(msbConstp->अङ्कः(), constp->अङ्कः()).isNeqZero()) {
                पटिनिवत्त सच्च;
            }
        }
        पटिनिवत्त मिच्छा;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* new_व्यञ्जकम्_Stmt_Or_Clone(Ast_ग्रन्थिः_व्यञ्जकम्*& exprp) {
        सचे (!exprp->isPure()) {
            AstVar* ध्रुव varp = createAddTemp(exprp);
            File_पङ्क्तिः* ध्रुव fl = exprp->fileline();
            exprp = नूतन Ast_व्यञ्जकम्_Stmt{
                fl, नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                नूतन AstVarRef{fl, varp, VAccess::READ}};
            पटिनिवत्त नूतन AstVarRef{fl, varp, VAccess::READ};
        }
        पटिनिवत्त exprp->clone_वृक्षः_Pure(मिच्छा);
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = सच्च;
            // Class X randomization causes Vxrand in strange places, so disable
            सचे (VN_IS(nodep, Class)) m_allowXUnique = मिच्छा;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = मिच्छा;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = सच्च;
        iterateAndNextNull(nodep->stmtsp());
    }
    शून्य visit(Ast_ग्रन्थिः_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = मिच्छा;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        सचे (nodep->isParam()) m_allowXUnique = मिच्छा;
        iterateChildren(nodep);
    }
    शून्य visitEqNeqCase(Ast_ग्रन्थिः_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        सचे (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            पटिनिवत्त;
        } अञ्ञथा {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            सचे (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->अङ्कः().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->अङ्कः().isFourState()))) {
                newp = नूतन AstConst(nodep->fileline(), AstConst::Widthed_मानम्{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_वृक्षः(), lhsp);
                VL_DO_DANGLING(rhsp->delete_वृक्षः(), rhsp);
            } अञ्ञथा {
                सचे (VN_IS(nodep, EqCase)) {
                    newp = नूतन AstEq{nodep->fileline(), lhsp, rhsp};
                } अञ्ञथा {
                    newp = नूतन AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    शून्य visitEqNeqWild(Ast_ग्रन्थिः_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        सचे (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            पटिनिवत्त;
        } अञ्ञथा {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* newp;
            सचे (!VN_IS(rhsp, Const)) {
                सचे (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = नूतन AstEq{nodep->fileline(), lhsp, rhsp};
            } अञ्ञथा {
                // X or Z's become mask, ala case statements.
                V3_सङ्ख्या nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->अङ्कः());
                V3_सङ्ख्या numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->अङ्कः());
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव and1p = नूतन AstAnd{nodep->fileline(), lhsp,
                                                      नूतन AstConst{nodep->fileline(), nummask}};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव and2p = नूतन AstConst{nodep->fileline(), numval};
                सचे (VN_IS(nodep, EqWild)) {
                    newp = नूतन AstEq{nodep->fileline(), and1p, and2p};
                } अञ्ञथा {
                    newp = नूतन AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_वृक्षः(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    शून्य visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    शून्य visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    शून्य visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    शून्य visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    शून्य visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य visit(Ast_गणनम्_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<तार्किक, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->अङ्कः().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->अङ्कः().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->अङ्कः().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_ग्रन्थिः_व्यञ्जकम्* nonXp = nullptr;
        सचे (!dropop[0]) {
            nonXp = nodep->rhsp();
        } अञ्ञथा सचे (!dropop[1]) {
            nonXp = nodep->thsp();
        } अञ्ञथा सचे (!dropop[2]) {
            nonXp = nodep->fhsp();
        } अञ्ञथा {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }
        सचे (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_वृक्षः();
            nodep->rhsp(nonXp->clone_वृक्षः_Pure(सच्च));
        }
        सचे (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_वृक्षः();
            nodep->thsp(nonXp->clone_वृक्षः_Pure(सच्च));
        }
        सचे (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_वृक्षः();
            nodep->fhsp(nonXp->clone_वृक्षः_Pure(सच्च));
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstConst* nodep) override {
        सचे (m_constXCvt && nodep->अङ्कः().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_सङ्ख्या numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->अङ्कः());
            V3_सङ्ख्या numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->अङ्कः());
            सचे (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_सङ्ख्या numnew{nodep, numb1.width()};
                सचे (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } अञ्ञथा {
                    numnew.opAssign(numb1);
                }
                AstConst* ध्रुव newp = नूतन AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                UINFO(4, "   -> " << newp);
            } अञ्ञथा {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* ध्रुव newvarp
                    = नूतन AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_ग्रन्थिः_Var_Ref* ध्रुव newref1p
                    = नूतन AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* ध्रुव newinitp = नूतन AstInitial{
                    nodep->fileline(),
                    नूतन AstAssign{
                        nodep->fileline(),
                        नूतन AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        नूतन AstOr{nodep->fileline(), नूतन AstConst{nodep->fileline(), numb1},
                                  नूतन AstAnd{
                                      nodep->fileline(), नूतन AstConst{nodep->fileline(), numbx},
                                      नूतन AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_ग्रन्थिः* ध्रुव newStmtsp = Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            }
        }
    }

    शून्य visit(AstSel* nodep) override {
        iterateChildren(nodep);
        सचे (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            ध्रुव Ast_ग्रन्थिः* ध्रुव basefromp = nodep->baseFromp(सच्च);
            तार्किक lvalue = मिच्छा;
            सचे (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(basefromp, ग्रन्थिः_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            ध्रुव uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* ध्रुव maxmsbConstp = नूतन AstConst{
                nodep->fileline(), AstConst::Widthed_मानम्{}, nodep->lsbp()->width(), maxmsb};
            Ast_ग्रन्थिः_व्यञ्जकम्* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            सचे (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_वृक्षः(), maxmsbConstp);
                nodep->lsbp(lsbp);
                पटिनिवत्त;
            }
            nodep->lsbp(new_व्यञ्जकम्_Stmt_Or_Clone(lsbp));
            Ast_ग्रन्थिः_व्यञ्जकम्* condp
                = V3Const::constifyEdit(नूतन AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            सचे (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_सङ्ख्या xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव xexprp = नूतन AstConst{nodep->fileline(), xnum};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                    सचे (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_वृक्षः(), condp);
                        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                        पटिनिवत्त xexprp;
                    }
                    पटिनिवत्त नूतन AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } अञ्ञथा {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    शून्य visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        सचे (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_ग्रन्थिः* ध्रुव basefromp = nodep->fromp()->baseFromp(सच्च);
            तार्किक lvalue = मिच्छा;
            सचे (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(basefromp, ग्रन्थिः_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } अञ्ञथा सचे (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } अञ्ञथा {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            पूर्णाङ्क declElements = -1;
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव adtypep = VN_CAST(dtypep, ग्रन्थिः_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            सचे (ध्रुव AstModDiv* ध्रुव moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                सचे (ध्रुव AstConst* ध्रुव modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    सचे (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        पटिनिवत्त;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* ध्रुव declElementsp
                = नूतन AstConst{nodep->fileline(), AstConst::Widthed_मानम्{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_ग्रन्थिः_व्यञ्जकम्* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            सचे (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_वृक्षः(), declElementsp);
                nodep->bitp(bitp);
                पटिनिवत्त;
            }
            nodep->bitp(new_व्यञ्जकम्_Stmt_Or_Clone(bitp));
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = नूतन AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव ग्रन्थिः_Dtp = nodep->dtypep()->skipRefp();
            सचे (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(ग्रन्थिः_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_सङ्ख्या xnum{nodep, nodep->width()};
                सचे (ग्रन्थिः_Dtp->isDouble()) {
                    xnum = V3_सङ्ख्या{nodep, V3_सङ्ख्या::Double{}, 0.0};
                } अञ्ञथा सचे (ग्रन्थिः_Dtp->isString()) {
                    xnum = V3_सङ्ख्या{nodep, V3_सङ्ख्या::String{}, ""};
                } अञ्ञथा {
                    सचे (ग्रन्थिः_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } अञ्ञथा {
                        xnum.setAllBits0();
                    }
                }
                Ast_ग्रन्थिः* ध्रुव newp = नूतन AstCond{nodep->fileline(), condp, nodep,
                                                  नूतन AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } अञ्ञथा सचे (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
                    = नूतन AstCond{asBitp->fileline(), condp, asBitp,
                                  नूतन AstConst{asBitp->fileline(), AstConst::Widthed_मानम्{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } अञ्ञथा {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
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

ध्रुव std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

शून्य V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("unknown", 0, dump_वृक्षः_Either_Level() >= 3);
}
