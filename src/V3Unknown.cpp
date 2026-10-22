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

वर्ग UnknownVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    अचर VNUser1InUse m_inuser1;
    अचर VNUser2InUse m_inuser2;
    स्थिर अचर std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    Ast_गाँठो_FTask* m_ftaskp = nullptr;  // Current function/task
    तार्किक m_constXCvt = असत्य;  // Convert X's
    तार्किक m_allowXUnique = सत्य;  // Allow unique assignments

    // METHODS

    शून्य addVar(AstVar* varp) {
        यदि (m_ftaskp) {
            varp->funcLocal(सत्य);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } अन्यथा {
            m_modp->prependStmtsp(varp);
        }
    }

    शून्य replaceBoundLvalue(Ast_गाँठो_अभिव्यक्ति* nodep, Ast_गाँठो_अभिव्यक्ति* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_गाँठो_अभिव्यक्ति* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        जबसम्म (VN_IS(prep->backp(), गाँठो_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), गाँठो_अभिव्यक्ति);
        }
        यदि (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_रूख(), condp);
            फर्काऊ;
        }
        File_पङ्क्ति* अचर fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        यदि (अचर AstIf* अचर ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_गाँठो_अभिव्यक्ति* अचर earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_गाँठो_अभिव्यक्ति* अचर newp = नूतन AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } अन्यथा {
            Ast_गाँठो* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            जबसम्म (!VN_IS(stmtp, गाँठो_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_गाँठो* अचर origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_गाँठो* elseStmtp = nullptr;
            अचर तार्किक hasSideEffects = origStmtp->exists(
                [](Ast_गाँठो* अचर np) { फर्काऊ !np->isPure() || np->isTimingControl(); });
            यदि (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_रूख(असत्य);
                AstVar* अचर varp = नूतन AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_गाँठो* अचर prepCopyp = prep->clonep();
                prepCopyp->replaceWith(नूतन AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(थप_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* अचर newp = नूतन AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(सत्य);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(अचर Ast_गाँठो_अभिव्यक्ति* अचर nodep) {
        AstVar* अचर varp = नूतन AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        फर्काऊ varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    स्थिर तार्किक isStaticlyGte(AstConst* अचर msbConstp, अचर Ast_गाँठो_अभिव्यक्ति* अचर exprp) {
        यदि (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->अङ्क().toSInt() >= (1 << exprp->width()) - 1) {
            फर्काऊ सत्य;
        }
        यदि (अचर AstConst* अचर constp = VN_CAST(exprp, Const)) {
            यदि (V3_सङ्ख्या{msbConstp}.opGte(msbConstp->अङ्क(), constp->अङ्क()).isNeqZero()) {
                फर्काऊ सत्य;
            }
        }
        फर्काऊ असत्य;
    }

    Ast_गाँठो_अभिव्यक्ति* new_अभिव्यक्ति_Stmt_Or_Clone(Ast_गाँठो_अभिव्यक्ति*& exprp) {
        यदि (!exprp->isPure()) {
            AstVar* अचर varp = createAddTemp(exprp);
            File_पङ्क्ति* अचर fl = exprp->fileline();
            exprp = नूतन Ast_अभिव्यक्ति_Stmt{
                fl, नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                नूतन AstVarRef{fl, varp, VAccess::READ}};
            फर्काऊ नूतन AstVarRef{fl, varp, VAccess::READ};
        }
        फर्काऊ exprp->clone_रूख_Pure(असत्य);
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        स्वतः xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = सत्य;
            // Class X randomization causes Vxrand in strange places, so disable
            यदि (VN_IS(nodep, Class)) m_allowXUnique = असत्य;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = असत्य;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = सत्य;
        iterateAndNextNull(nodep->stmtsp());
    }
    शून्य visit(Ast_गाँठो_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = असत्य;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        यदि (nodep->isParam()) m_allowXUnique = असत्य;
        iterateChildren(nodep);
    }
    शून्य visitEqNeqCase(Ast_गाँठो_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        यदि (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            फर्काऊ;
        } अन्यथा {
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            यदि (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->अङ्क().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->अङ्क().isFourState()))) {
                newp = नूतन AstConst(nodep->fileline(), AstConst::Widthed_मान{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_रूख(), lhsp);
                VL_DO_DANGLING(rhsp->delete_रूख(), rhsp);
            } अन्यथा {
                यदि (VN_IS(nodep, EqCase)) {
                    newp = नूतन AstEq{nodep->fileline(), lhsp, rhsp};
                } अन्यथा {
                    newp = नूतन AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    शून्य visitEqNeqWild(Ast_गाँठो_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        यदि (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            फर्काऊ;
        } अन्यथा {
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* newp;
            यदि (!VN_IS(rhsp, Const)) {
                यदि (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = नूतन AstEq{nodep->fileline(), lhsp, rhsp};
            } अन्यथा {
                // X or Z's become mask, ala case statements.
                V3_सङ्ख्या nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->अङ्क());
                V3_सङ्ख्या numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->अङ्क());
                Ast_गाँठो_अभिव्यक्ति* अचर and1p = नूतन AstAnd{nodep->fileline(), lhsp,
                                                      नूतन AstConst{nodep->fileline(), nummask}};
                Ast_गाँठो_अभिव्यक्ति* अचर and2p = नूतन AstConst{nodep->fileline(), numval};
                यदि (VN_IS(nodep, EqWild)) {
                    newp = नूतन AstEq{nodep->fileline(), and1p, and2p};
                } अन्यथा {
                    newp = नूतन AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_रूख(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
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
        AstConst* अचर newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    }
    शून्य visit(Ast_गन्ती_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<तार्किक, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->अङ्क().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->अङ्क().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->अङ्क().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_गाँठो_अभिव्यक्ति* nonXp = nullptr;
        यदि (!dropop[0]) {
            nonXp = nodep->rhsp();
        } अन्यथा यदि (!dropop[1]) {
            nonXp = nodep->thsp();
        } अन्यथा यदि (!dropop[2]) {
            nonXp = nodep->fhsp();
        } अन्यथा {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* अचर newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
            फर्काऊ;
        }
        यदि (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_रूख();
            nodep->rhsp(nonXp->clone_रूख_Pure(सत्य));
        }
        यदि (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_रूख();
            nodep->thsp(nonXp->clone_रूख_Pure(सत्य));
        }
        यदि (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_रूख();
            nodep->fhsp(nonXp->clone_रूख_Pure(सत्य));
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstConst* nodep) override {
        यदि (m_constXCvt && nodep->अङ्क().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_सङ्ख्या numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->अङ्क());
            V3_सङ्ख्या numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->अङ्क());
            यदि (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_सङ्ख्या numnew{nodep, numb1.width()};
                यदि (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } अन्यथा {
                    numnew.opAssign(numb1);
                }
                AstConst* अचर newp = नूतन AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_रूख(), nodep);
                UINFO(4, "   -> " << newp);
            } अन्यथा {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* अचर newvarp
                    = नूतन AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_गाँठो_Var_Ref* अचर newref1p
                    = नूतन AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* अचर newinitp = नूतन AstInitial{
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
                Ast_गाँठो* अचर newStmtsp = Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_रूख(), nodep);
            }
        }
    }

    शून्य visit(AstSel* nodep) override {
        iterateChildren(nodep);
        यदि (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            अचर Ast_गाँठो* अचर basefromp = nodep->baseFromp(सत्य);
            तार्किक lvalue = असत्य;
            यदि (अचर Ast_गाँठो_Var_Ref* अचर varrefp = VN_CAST(basefromp, गाँठो_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            अचर uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* अचर maxmsbConstp = नूतन AstConst{
                nodep->fileline(), AstConst::Widthed_मान{}, nodep->lsbp()->width(), maxmsb};
            Ast_गाँठो_अभिव्यक्ति* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            यदि (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_रूख(), maxmsbConstp);
                nodep->lsbp(lsbp);
                फर्काऊ;
            }
            nodep->lsbp(new_अभिव्यक्ति_Stmt_Or_Clone(lsbp));
            Ast_गाँठो_अभिव्यक्ति* condp
                = V3Const::constifyEdit(नूतन AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            यदि (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_सङ्ख्या xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_गाँठो_अभिव्यक्ति* अचर xexprp = नूतन AstConst{nodep->fileline(), xnum};
                Ast_गाँठो_अभिव्यक्ति* अचर newp = [&]() -> Ast_गाँठो_अभिव्यक्ति* {
                    यदि (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_रूख(), condp);
                        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
                        फर्काऊ xexprp;
                    }
                    फर्काऊ नूतन AstCond{nodep->fileline(), condp, nodep, xexprp};
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

    शून्य visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        यदि (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_गाँठो* अचर basefromp = nodep->fromp()->baseFromp(सत्य);
            तार्किक lvalue = असत्य;
            यदि (अचर Ast_गाँठो_Var_Ref* अचर varrefp = VN_CAST(basefromp, गाँठो_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } अन्यथा यदि (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } अन्यथा {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            पूर्णाङ्क declElements = -1;
            Ast_गाँठो_DType* अचर dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            अचर Ast_गाँठो_Array_DType* अचर adtypep = VN_CAST(dtypep, गाँठो_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            यदि (अचर AstModDiv* अचर moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                यदि (अचर AstConst* अचर modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    यदि (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        फर्काऊ;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* अचर declElementsp
                = नूतन AstConst{nodep->fileline(), AstConst::Widthed_मान{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_गाँठो_अभिव्यक्ति* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            यदि (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_रूख(), declElementsp);
                nodep->bitp(bitp);
                फर्काऊ;
            }
            nodep->bitp(new_अभिव्यक्ति_Stmt_Or_Clone(bitp));
            Ast_गाँठो_अभिव्यक्ति* condp = नूतन AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            अचर Ast_गाँठो_DType* अचर गाँठो_Dtp = nodep->dtypep()->skipRefp();
            यदि (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(गाँठो_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_सङ्ख्या xnum{nodep, nodep->width()};
                यदि (गाँठो_Dtp->isDouble()) {
                    xnum = V3_सङ्ख्या{nodep, V3_सङ्ख्या::Double{}, 0.0};
                } अन्यथा यदि (गाँठो_Dtp->isString()) {
                    xnum = V3_सङ्ख्या{nodep, V3_सङ्ख्या::String{}, ""};
                } अन्यथा {
                    यदि (गाँठो_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } अन्यथा {
                        xnum.setAllBits0();
                    }
                }
                Ast_गाँठो* अचर newp = नूतन AstCond{nodep->fileline(), condp, nodep,
                                                  नूतन AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } अन्यथा यदि (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_गाँठो_अभिव्यक्ति* अचर asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_गाँठो_अभिव्यक्ति* अचर newp
                    = नूतन AstCond{asBitp->fileline(), condp, asBitp,
                                  नूतन AstConst{asBitp->fileline(), AstConst::Widthed_मान{},
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
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

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

अचर std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

शून्य V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("unknown", 0, dump_रूख_Either_Level() >= 3);
}
