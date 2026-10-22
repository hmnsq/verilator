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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Unknown.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு UnknownVisitor final : பொது VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;
    நிலையான மாறிலி std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    தருக்க m_constXCvt = பொய்;  // Convert X's
    தருக்க m_allowXUnique = உண்மை;  // Allow unique assignments

    // METHODS

    வெறுமை addVar(AstVar* varp) {
        எனில் (m_ftaskp) {
            varp->funcLocal(உண்மை);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } இல்லையெனில் {
            m_modp->prependStmtsp(varp);
        }
    }

    வெறுமை replaceBoundLvalue(Ast_முனை_கோவை* nodep, Ast_முனை_கோவை* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_முனை_கோவை* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        போது (VN_IS(prep->backp(), முனை_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), முனை_கோவை);
        }
        எனில் (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_மரம்(), condp);
            திரும்பு;
        }
        File_வரி* மாறிலி fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        எனில் (மாறிலி AstIf* மாறிலி ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_முனை_கோவை* மாறிலி earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } இல்லையெனில் {
            Ast_முனை* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            போது (!VN_IS(stmtp, முனை_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_முனை* மாறிலி origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_முனை* elseStmtp = nullptr;
            மாறிலி தருக்க hasSideEffects = origStmtp->exists(
                [](Ast_முனை* மாறிலி np) { திரும்பு !np->isPure() || np->isTimingControl(); });
            எனில் (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_மரம்(பொய்);
                AstVar* மாறிலி varp = புதிய AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_முனை* மாறிலி prepCopyp = prep->clonep();
                prepCopyp->replaceWith(புதிய AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(தள்ளு_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* மாறிலி newp = புதிய AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(உண்மை);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(மாறிலி Ast_முனை_கோவை* மாறிலி nodep) {
        AstVar* மாறிலி varp = புதிய AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        திரும்பு varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    நிலையான தருக்க isStaticlyGte(AstConst* மாறிலி msbConstp, மாறிலி Ast_முனை_கோவை* மாறிலி exprp) {
        எனில் (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->இலக்கம்().toSInt() >= (1 << exprp->width()) - 1) {
            திரும்பு உண்மை;
        }
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(exprp, Const)) {
            எனில் (V3_எண்{msbConstp}.opGte(msbConstp->இலக்கம்(), constp->இலக்கம்()).isNeqZero()) {
                திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;
    }

    Ast_முனை_கோவை* new_கோவை_Stmt_Or_Clone(Ast_முனை_கோவை*& exprp) {
        எனில் (!exprp->isPure()) {
            AstVar* மாறிலி varp = createAddTemp(exprp);
            File_வரி* மாறிலி fl = exprp->fileline();
            exprp = புதிய Ast_கோவை_Stmt{
                fl, புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                புதிய AstVarRef{fl, varp, VAccess::READ}};
            திரும்பு புதிய AstVarRef{fl, varp, VAccess::READ};
        }
        திரும்பு exprp->clone_மரம்_Pure(பொய்);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = உண்மை;
            // Class X randomization causes Vxrand in strange places, so disable
            எனில் (VN_IS(nodep, Class)) m_allowXUnique = பொய்;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = பொய்;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = உண்மை;
        iterateAndNextNull(nodep->stmtsp());
    }
    வெறுமை visit(Ast_முனை_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = பொய்;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        எனில் (nodep->isParam()) m_allowXUnique = பொய்;
        iterateChildren(nodep);
    }
    வெறுமை visitEqNeqCase(Ast_முனை_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        எனில் (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            திரும்பு;
        } இல்லையெனில் {
            Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_முனை_கோவை* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            எனில் (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->இலக்கம்().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->இலக்கம்().isFourState()))) {
                newp = புதிய AstConst(nodep->fileline(), AstConst::Widthed_மதிப்பு{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_மரம்(), lhsp);
                VL_DO_DANGLING(rhsp->delete_மரம்(), rhsp);
            } இல்லையெனில் {
                எனில் (VN_IS(nodep, EqCase)) {
                    newp = புதிய AstEq{nodep->fileline(), lhsp, rhsp};
                } இல்லையெனில் {
                    newp = புதிய AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    வெறுமை visitEqNeqWild(Ast_முனை_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        எனில் (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            திரும்பு;
        } இல்லையெனில் {
            Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_முனை_கோவை* newp;
            எனில் (!VN_IS(rhsp, Const)) {
                எனில் (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = புதிய AstEq{nodep->fileline(), lhsp, rhsp};
            } இல்லையெனில் {
                // X or Z's become mask, ala case statements.
                V3_எண் nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->இலக்கம்());
                V3_எண் numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->இலக்கம்());
                Ast_முனை_கோவை* மாறிலி and1p = புதிய AstAnd{nodep->fileline(), lhsp,
                                                      புதிய AstConst{nodep->fileline(), nummask}};
                Ast_முனை_கோவை* மாறிலி and2p = புதிய AstConst{nodep->fileline(), numval};
                எனில் (VN_IS(nodep, EqWild)) {
                    newp = புதிய AstEq{nodep->fileline(), and1p, and2p};
                } இல்லையெனில் {
                    newp = புதிய AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_மரம்(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    வெறுமை visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    வெறுமை visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    வெறுமை visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    வெறுமை visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    வெறுமை visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* மாறிலி newp = புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(Ast_மொத்தம்_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<தருக்க, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->இலக்கம்().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->இலக்கம்().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->இலக்கம்().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_முனை_கோவை* nonXp = nullptr;
        எனில் (!dropop[0]) {
            nonXp = nodep->rhsp();
        } இல்லையெனில் எனில் (!dropop[1]) {
            nonXp = nodep->thsp();
        } இல்லையெனில் எனில் (!dropop[2]) {
            nonXp = nodep->fhsp();
        } இல்லையெனில் {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* மாறிலி newp = புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            திரும்பு;
        }
        எனில் (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_மரம்();
            nodep->rhsp(nonXp->clone_மரம்_Pure(உண்மை));
        }
        எனில் (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_மரம்();
            nodep->thsp(nonXp->clone_மரம்_Pure(உண்மை));
        }
        எனில் (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_மரம்();
            nodep->fhsp(nonXp->clone_மரம்_Pure(உண்மை));
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstConst* nodep) override {
        எனில் (m_constXCvt && nodep->இலக்கம்().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_எண் numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->இலக்கம்());
            V3_எண் numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->இலக்கம்());
            எனில் (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_எண் numnew{nodep, numb1.width()};
                எனில் (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } இல்லையெனில் {
                    numnew.opAssign(numb1);
                }
                AstConst* மாறிலி newp = புதிய AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
                UINFO(4, "   -> " << newp);
            } இல்லையெனில் {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* மாறிலி newvarp
                    = புதிய AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_முனை_Var_Ref* மாறிலி newref1p
                    = புதிய AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* மாறிலி newinitp = புதிய AstInitial{
                    nodep->fileline(),
                    புதிய AstAssign{
                        nodep->fileline(),
                        புதிய AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        புதிய AstOr{nodep->fileline(), புதிய AstConst{nodep->fileline(), numb1},
                                  புதிய AstAnd{
                                      nodep->fileline(), புதிய AstConst{nodep->fileline(), numbx},
                                      புதிய AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_முனை* மாறிலி newStmtsp = Ast_முனை::addNext<Ast_முனை, Ast_முனை>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            }
        }
    }

    வெறுமை visit(AstSel* nodep) override {
        iterateChildren(nodep);
        எனில் (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            மாறிலி Ast_முனை* மாறிலி basefromp = nodep->baseFromp(உண்மை);
            தருக்க lvalue = பொய்;
            எனில் (மாறிலி Ast_முனை_Var_Ref* மாறிலி varrefp = VN_CAST(basefromp, முனை_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            மாறிலி uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* மாறிலி maxmsbConstp = புதிய AstConst{
                nodep->fileline(), AstConst::Widthed_மதிப்பு{}, nodep->lsbp()->width(), maxmsb};
            Ast_முனை_கோவை* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            எனில் (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_மரம்(), maxmsbConstp);
                nodep->lsbp(lsbp);
                திரும்பு;
            }
            nodep->lsbp(new_கோவை_Stmt_Or_Clone(lsbp));
            Ast_முனை_கோவை* condp
                = V3Const::constifyEdit(புதிய AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            எனில் (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_எண் xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_முனை_கோவை* மாறிலி xexprp = புதிய AstConst{nodep->fileline(), xnum};
                Ast_முனை_கோவை* மாறிலி newp = [&]() -> Ast_முனை_கோவை* {
                    எனில் (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_மரம்(), condp);
                        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
                        திரும்பு xexprp;
                    }
                    திரும்பு புதிய AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } இல்லையெனில் {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    வெறுமை visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        எனில் (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_முனை* மாறிலி basefromp = nodep->fromp()->baseFromp(உண்மை);
            தருக்க lvalue = பொய்;
            எனில் (மாறிலி Ast_முனை_Var_Ref* மாறிலி varrefp = VN_CAST(basefromp, முனை_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } இல்லையெனில் எனில் (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } இல்லையெனில் {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            முழு declElements = -1;
            Ast_முனை_DType* மாறிலி dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            மாறிலி Ast_முனை_Array_DType* மாறிலி adtypep = VN_CAST(dtypep, முனை_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            எனில் (மாறிலி AstModDiv* மாறிலி moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                எனில் (மாறிலி AstConst* மாறிலி modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    எனில் (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        திரும்பு;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* மாறிலி declElementsp
                = புதிய AstConst{nodep->fileline(), AstConst::Widthed_மதிப்பு{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_முனை_கோவை* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            எனில் (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_மரம்(), declElementsp);
                nodep->bitp(bitp);
                திரும்பு;
            }
            nodep->bitp(new_கோவை_Stmt_Or_Clone(bitp));
            Ast_முனை_கோவை* condp = புதிய AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            மாறிலி Ast_முனை_DType* மாறிலி முனை_Dtp = nodep->dtypep()->skipRefp();
            எனில் (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(முனை_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_எண் xnum{nodep, nodep->width()};
                எனில் (முனை_Dtp->isDouble()) {
                    xnum = V3_எண்{nodep, V3_எண்::Double{}, 0.0};
                } இல்லையெனில் எனில் (முனை_Dtp->isString()) {
                    xnum = V3_எண்{nodep, V3_எண்::String{}, ""};
                } இல்லையெனில் {
                    எனில் (முனை_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } இல்லையெனில் {
                        xnum.setAllBits0();
                    }
                }
                Ast_முனை* மாறிலி newp = புதிய AstCond{nodep->fileline(), condp, nodep,
                                                  புதிய AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } இல்லையெனில் எனில் (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_முனை_கோவை* மாறிலி asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_முனை_கோவை* மாறிலி newp
                    = புதிய AstCond{asBitp->fileline(), condp, asBitp,
                                  புதிய AstConst{asBitp->fileline(), AstConst::Widthed_மதிப்பு{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } இல்லையெனில் {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
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

மாறிலி std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

வெறுமை V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("unknown", 0, dump_மரம்_Either_Level() >= 3);
}
