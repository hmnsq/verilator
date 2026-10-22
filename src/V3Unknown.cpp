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
    स्थिरांक VNUser1InUse m_inuser1;
    स्थिरांक VNUser2InUse m_inuser2;
    स्थिर स्थिरांक std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    Ast_गाठ_FTask* m_ftaskp = nullptr;  // Current function/task
    तार्किक m_constXCvt = असत्य;  // Convert X's
    तार्किक m_allowXUnique = सत्य;  // Allow unique assignments

    // METHODS

    शून्य addVar(AstVar* varp) {
        जर (m_ftaskp) {
            varp->funcLocal(सत्य);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } अन्यथा {
            m_modp->prependStmtsp(varp);
        }
    }

    शून्य replaceBoundLvalue(Ast_गाठ_अभिव्यक्ती* nodep, Ast_गाठ_अभिव्यक्ती* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_गाठ_अभिव्यक्ती* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        पर्यंत (VN_IS(prep->backp(), गाठ_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), गाठ_अभिव्यक्ती);
        }
        जर (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_वृक्ष(), condp);
            परत;
        }
        File_ओळ* स्थिरांक fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        जर (स्थिरांक AstIf* स्थिरांक ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } अन्यथा {
            Ast_गाठ* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            पर्यंत (!VN_IS(stmtp, गाठ_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_गाठ* स्थिरांक origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_गाठ* elseStmtp = nullptr;
            स्थिरांक तार्किक hasSideEffects = origStmtp->exists(
                [](Ast_गाठ* स्थिरांक np) { परत !np->isPure() || np->isTimingControl(); });
            जर (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_वृक्ष(असत्य);
                AstVar* स्थिरांक varp = नवीन AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_गाठ* स्थिरांक prepCopyp = prep->clonep();
                prepCopyp->replaceWith(नवीन AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(ढकला_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* स्थिरांक newp = नवीन AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(सत्य);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक nodep) {
        AstVar* स्थिरांक varp = नवीन AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        परत varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    स्थिर तार्किक isStaticlyGte(AstConst* स्थिरांक msbConstp, स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp) {
        जर (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->अंक().toSInt() >= (1 << exprp->width()) - 1) {
            परत सत्य;
        }
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(exprp, Const)) {
            जर (V3_संख्या{msbConstp}.opGte(msbConstp->अंक(), constp->अंक()).isNeqZero()) {
                परत सत्य;
            }
        }
        परत असत्य;
    }

    Ast_गाठ_अभिव्यक्ती* new_अभिव्यक्ती_Stmt_Or_Clone(Ast_गाठ_अभिव्यक्ती*& exprp) {
        जर (!exprp->isPure()) {
            AstVar* स्थिरांक varp = createAddTemp(exprp);
            File_ओळ* स्थिरांक fl = exprp->fileline();
            exprp = नवीन Ast_अभिव्यक्ती_Stmt{
                fl, नवीन AstAssign{fl, नवीन AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                नवीन AstVarRef{fl, varp, VAccess::READ}};
            परत नवीन AstVarRef{fl, varp, VAccess::READ};
        }
        परत exprp->clone_वृक्ष_Pure(असत्य);
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        स्वतः xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = सत्य;
            // Class X randomization causes Vxrand in strange places, so disable
            जर (VN_IS(nodep, Class)) m_allowXUnique = असत्य;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
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
    शून्य visit(Ast_गाठ_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = असत्य;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        जर (nodep->isParam()) m_allowXUnique = असत्य;
        iterateChildren(nodep);
    }
    शून्य visitEqNeqCase(Ast_गाठ_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        जर (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            परत;
        } अन्यथा {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            जर (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->अंक().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->अंक().isFourState()))) {
                newp = नवीन AstConst(nodep->fileline(), AstConst::Widthed_मूल्य{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_वृक्ष(), lhsp);
                VL_DO_DANGLING(rhsp->delete_वृक्ष(), rhsp);
            } अन्यथा {
                जर (VN_IS(nodep, EqCase)) {
                    newp = नवीन AstEq{nodep->fileline(), lhsp, rhsp};
                } अन्यथा {
                    newp = नवीन AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    शून्य visitEqNeqWild(Ast_गाठ_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        जर (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            परत;
        } अन्यथा {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* newp;
            जर (!VN_IS(rhsp, Const)) {
                जर (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = नवीन AstEq{nodep->fileline(), lhsp, rhsp};
            } अन्यथा {
                // X or Z's become mask, ala case statements.
                V3_संख्या nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->अंक());
                V3_संख्या numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->अंक());
                Ast_गाठ_अभिव्यक्ती* स्थिरांक and1p = नवीन AstAnd{nodep->fileline(), lhsp,
                                                      नवीन AstConst{nodep->fileline(), nummask}};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक and2p = नवीन AstConst{nodep->fileline(), numval};
                जर (VN_IS(nodep, EqWild)) {
                    newp = नवीन AstEq{nodep->fileline(), and1p, and2p};
                } अन्यथा {
                    newp = नवीन AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_वृक्ष(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
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
        AstConst* स्थिरांक newp = नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(Ast_मोजणी_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<तार्किक, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->अंक().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->अंक().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->अंक().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_गाठ_अभिव्यक्ती* nonXp = nullptr;
        जर (!dropop[0]) {
            nonXp = nodep->rhsp();
        } अन्यथा जर (!dropop[1]) {
            nonXp = nodep->thsp();
        } अन्यथा जर (!dropop[2]) {
            nonXp = nodep->fhsp();
        } अन्यथा {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* स्थिरांक newp = नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            परत;
        }
        जर (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_वृक्ष();
            nodep->rhsp(nonXp->clone_वृक्ष_Pure(सत्य));
        }
        जर (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_वृक्ष();
            nodep->thsp(nonXp->clone_वृक्ष_Pure(सत्य));
        }
        जर (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_वृक्ष();
            nodep->fhsp(nonXp->clone_वृक्ष_Pure(सत्य));
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstConst* nodep) override {
        जर (m_constXCvt && nodep->अंक().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_संख्या numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->अंक());
            V3_संख्या numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->अंक());
            जर (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_संख्या numnew{nodep, numb1.width()};
                जर (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } अन्यथा {
                    numnew.opAssign(numb1);
                }
                AstConst* स्थिरांक newp = नवीन AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                UINFO(4, "   -> " << newp);
            } अन्यथा {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* स्थिरांक newvarp
                    = नवीन AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_गाठ_Var_Ref* स्थिरांक newref1p
                    = नवीन AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* स्थिरांक newinitp = नवीन AstInitial{
                    nodep->fileline(),
                    नवीन AstAssign{
                        nodep->fileline(),
                        नवीन AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        नवीन AstOr{nodep->fileline(), नवीन AstConst{nodep->fileline(), numb1},
                                  नवीन AstAnd{
                                      nodep->fileline(), नवीन AstConst{nodep->fileline(), numbx},
                                      नवीन AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_गाठ* स्थिरांक newStmtsp = Ast_गाठ::addNext<Ast_गाठ, Ast_गाठ>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            }
        }
    }

    शून्य visit(AstSel* nodep) override {
        iterateChildren(nodep);
        जर (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            स्थिरांक Ast_गाठ* स्थिरांक basefromp = nodep->baseFromp(सत्य);
            तार्किक lvalue = असत्य;
            जर (स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक varrefp = VN_CAST(basefromp, गाठ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            स्थिरांक uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* स्थिरांक maxmsbConstp = नवीन AstConst{
                nodep->fileline(), AstConst::Widthed_मूल्य{}, nodep->lsbp()->width(), maxmsb};
            Ast_गाठ_अभिव्यक्ती* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            जर (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_वृक्ष(), maxmsbConstp);
                nodep->lsbp(lsbp);
                परत;
            }
            nodep->lsbp(new_अभिव्यक्ती_Stmt_Or_Clone(lsbp));
            Ast_गाठ_अभिव्यक्ती* condp
                = V3Const::constifyEdit(नवीन AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            जर (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_संख्या xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक xexprp = नवीन AstConst{nodep->fileline(), xnum};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = [&]() -> Ast_गाठ_अभिव्यक्ती* {
                    जर (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_वृक्ष(), condp);
                        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                        परत xexprp;
                    }
                    परत नवीन AstCond{nodep->fileline(), condp, nodep, xexprp};
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
        जर (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_गाठ* स्थिरांक basefromp = nodep->fromp()->baseFromp(सत्य);
            तार्किक lvalue = असत्य;
            जर (स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक varrefp = VN_CAST(basefromp, गाठ_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } अन्यथा जर (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } अन्यथा {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            पूर्णांक declElements = -1;
            Ast_गाठ_DType* स्थिरांक dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            स्थिरांक Ast_गाठ_Array_DType* स्थिरांक adtypep = VN_CAST(dtypep, गाठ_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            जर (स्थिरांक AstModDiv* स्थिरांक moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                जर (स्थिरांक AstConst* स्थिरांक modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    जर (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        परत;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* स्थिरांक declElementsp
                = नवीन AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_गाठ_अभिव्यक्ती* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            जर (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_वृक्ष(), declElementsp);
                nodep->bitp(bitp);
                परत;
            }
            nodep->bitp(new_अभिव्यक्ती_Stmt_Or_Clone(bitp));
            Ast_गाठ_अभिव्यक्ती* condp = नवीन AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            स्थिरांक Ast_गाठ_DType* स्थिरांक गाठ_Dtp = nodep->dtypep()->skipRefp();
            जर (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(गाठ_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_संख्या xnum{nodep, nodep->width()};
                जर (गाठ_Dtp->isDouble()) {
                    xnum = V3_संख्या{nodep, V3_संख्या::Double{}, 0.0};
                } अन्यथा जर (गाठ_Dtp->isString()) {
                    xnum = V3_संख्या{nodep, V3_संख्या::String{}, ""};
                } अन्यथा {
                    जर (गाठ_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } अन्यथा {
                        xnum.setAllBits0();
                    }
                }
                Ast_गाठ* स्थिरांक newp = नवीन AstCond{nodep->fileline(), condp, nodep,
                                                  नवीन AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } अन्यथा जर (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_गाठ_अभिव्यक्ती* स्थिरांक asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक newp
                    = नवीन AstCond{asBitp->fileline(), condp, asBitp,
                                  नवीन AstConst{asBitp->fileline(), AstConst::Widthed_मूल्य{},
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
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

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

स्थिरांक std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

शून्य V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("unknown", 0, dump_वृक्ष_Either_Level() >= 3);
}
