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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Unknown.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം UnknownVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;
    സ്ഥിതം സ്ഥിരം std::string s_xrandPrefix;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3UniqueNames m_lvboundNames;  // For generating unique temporary variable names
    std::unique_ptr<V3UniqueNames> m_xrandNames;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    തർക്കം m_constXCvt = അസത്യം;  // Convert X's
    തർക്കം m_allowXUnique = സത്യം;  // Allow unique assignments

    // METHODS

    ശൂന്യം addVar(AstVar* varp) {
        എങ്കിൽ (m_ftaskp) {
            varp->funcLocal(സത്യം);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } അല്ലെങ്കിൽ {
            m_modp->prependStmtsp(varp);
        }
    }

    ശൂന്യം replaceBoundLvalue(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_കണ്ണി_വ്യഞ്ജകം* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        ആകുമ്പോൾ (VN_IS(prep->backp(), കണ്ണി_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), കണ്ണി_വ്യഞ്ജകം);
        }
        എങ്കിൽ (VN_IS(prep->backp(), AssignForce) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_വൃക്ഷം(), condp);
            തിരികെ;
        }
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        എങ്കിൽ (സ്ഥിരം AstIf* സ്ഥിരം ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം earliercondp = ifp->condp()->unlinkFrBack(&replaceHandle);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            ആകുമ്പോൾ (!VN_IS(stmtp, കണ്ണി_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_കണ്ണി* സ്ഥിരം origStmtp = stmtp->unlinkFrBack(&replaceHandle);
            Ast_കണ്ണി* elseStmtp = nullptr;
            സ്ഥിരം തർക്കം hasSideEffects = origStmtp->exists(
                [](Ast_കണ്ണി* സ്ഥിരം np) { തിരികെ !np->isPure() || np->isTimingControl(); });
            എങ്കിൽ (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_വൃക്ഷം(അസത്യം);
                AstVar* സ്ഥിരം varp = പുതിയ AstVar{fl, VVarType::MODULETEMP, m_lvboundNames.get(prep),
                                                prep->dtypep()};
                addVar(varp);
                Ast_കണ്ണി* സ്ഥിരം prepCopyp = prep->clonep();
                prepCopyp->replaceWith(പുതിയ AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(തള്ളുക_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* സ്ഥിരം newp = പുതിയ AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(സത്യം);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* createAddTemp(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep) {
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrandNames->get(nodep), nodep->dtypep()};
        addVar(varp);
        തിരികെ varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    സ്ഥിതം തർക്കം isStaticlyGte(AstConst* സ്ഥിരം msbConstp, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp) {
        എങ്കിൽ (msbConstp->width() >= exprp->width() && exprp->width() < 32
            && msbConstp->അക്കം().toSInt() >= (1 << exprp->width()) - 1) {
            തിരികെ സത്യം;
        }
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(exprp, Const)) {
            എങ്കിൽ (V3_സംഖ്യ{msbConstp}.opGte(msbConstp->അക്കം(), constp->അക്കം()).isNeqZero()) {
                തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* new_വ്യഞ്ജകം_Stmt_Or_Clone(Ast_കണ്ണി_വ്യഞ്ജകം*& exprp) {
        എങ്കിൽ (!exprp->isPure()) {
            AstVar* സ്ഥിരം varp = createAddTemp(exprp);
            File_വരി* സ്ഥിരം fl = exprp->fileline();
            exprp = പുതിയ Ast_വ്യഞ്ജകം_Stmt{
                fl, പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                പുതിയ AstVarRef{fl, varp, VAccess::READ}};
            തിരികെ പുതിയ AstVarRef{fl, varp, VAccess::READ};
        }
        തിരികെ exprp->clone_വൃക്ഷം_Pure(അസത്യം);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        auto xrandNames = std::make_unique<V3UniqueNames>(s_xrandPrefix);
        {
            m_modp = nodep;
            m_constXCvt = സത്യം;
            // Class X randomization causes Vxrand in strange places, so disable
            എങ്കിൽ (VN_IS(nodep, Class)) m_allowXUnique = അസത്യം;
            m_lvboundNames.reset();
            xrandNames.swap(m_xrandNames);
            iterateChildren(nodep);
            xrandNames.swap(m_xrandNames);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCaseItem* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = അസത്യം;  // Avoid losing the X's in casex
        iterateAndNextNull(nodep->condsp());
        m_constXCvt = സത്യം;
        iterateAndNextNull(nodep->stmtsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = അസത്യം;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        എങ്കിൽ (nodep->isParam()) m_allowXUnique = അസത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visitEqNeqCase(Ast_കണ്ണി_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        എങ്കിൽ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            എങ്കിൽ (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->അക്കം().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->അക്കം().isFourState()))) {
                newp = പുതിയ AstConst(nodep->fileline(), AstConst::Widthed_മൂല്യം{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_വൃക്ഷം(), lhsp);
                VL_DO_DANGLING(rhsp->delete_വൃക്ഷം(), rhsp);
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (VN_IS(nodep, EqCase)) {
                    newp = പുതിയ AstEq{nodep->fileline(), lhsp, rhsp};
                } അല്ലെങ്കിൽ {
                    newp = പുതിയ AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    ശൂന്യം visitEqNeqWild(Ast_കണ്ണി_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        എങ്കിൽ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* newp;
            എങ്കിൽ (!VN_IS(rhsp, Const)) {
                എങ്കിൽ (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = പുതിയ AstEq{nodep->fileline(), lhsp, rhsp};
            } അല്ലെങ്കിൽ {
                // X or Z's become mask, ala case statements.
                V3_സംഖ്യ nummask{rhsp, rhsp->width()};
                nummask.opBitsNonXZ(VN_AS(rhsp, Const)->അക്കം());
                V3_സംഖ്യ numval{rhsp, rhsp->width()};
                numval.opBitsOne(VN_AS(rhsp, Const)->അക്കം());
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം and1p = പുതിയ AstAnd{nodep->fileline(), lhsp,
                                                      പുതിയ AstConst{nodep->fileline(), nummask}};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം and2p = പുതിയ AstConst{nodep->fileline(), numval};
                എങ്കിൽ (VN_IS(nodep, EqWild)) {
                    newp = പുതിയ AstEq{nodep->fileline(), and1p, and2p};
                } അല്ലെങ്കിൽ {
                    newp = പുതിയ AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_വൃക്ഷം(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    ശൂന്യം visit(AstEqCase* nodep) override { visitEqNeqCase(nodep); }
    ശൂന്യം visit(AstNeqCase* nodep) override { visitEqNeqCase(nodep); }
    ശൂന്യം visit(AstEqWild* nodep) override { visitEqNeqWild(nodep); }
    ശൂന്യം visit(AstNeqWild* nodep) override { visitEqNeqWild(nodep); }
    ശൂന്യം visit(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(Ast_എണ്ണം_Bits* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<തർക്കം, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->അക്കം().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->അക്കം().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->അക്കം().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_കണ്ണി_വ്യഞ്ജകം* nonXp = nullptr;
        എങ്കിൽ (!dropop[0]) {
            nonXp = nodep->rhsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (!dropop[1]) {
            nonXp = nodep->thsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (!dropop[2]) {
            nonXp = nodep->fhsp();
        } അല്ലെങ്കിൽ {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        എങ്കിൽ (dropop[0]) {
            nodep->rhsp()->unlinkFrBack()->delete_വൃക്ഷം();
            nodep->rhsp(nonXp->clone_വൃക്ഷം_Pure(സത്യം));
        }
        എങ്കിൽ (dropop[1]) {
            nodep->thsp()->unlinkFrBack()->delete_വൃക്ഷം();
            nodep->thsp(nonXp->clone_വൃക്ഷം_Pure(സത്യം));
        }
        എങ്കിൽ (dropop[2]) {
            nodep->fhsp()->unlinkFrBack()->delete_വൃക്ഷം();
            nodep->fhsp(nonXp->clone_വൃക്ഷം_Pure(സത്യം));
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstConst* nodep) override {
        എങ്കിൽ (m_constXCvt && nodep->അക്കം().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_സംഖ്യ numb1{nodep, nodep->width()};
            numb1.opBitsOne(nodep->അക്കം());
            V3_സംഖ്യ numbx{nodep, nodep->width()};
            numbx.opBitsXZ(nodep->അക്കം());
            എങ്കിൽ (!m_allowXUnique || v3Global.opt.xAssign() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_സംഖ്യ numnew{nodep, numb1.width()};
                എങ്കിൽ (v3Global.opt.xAssign() == "1") {
                    numnew.opOr(numb1, numbx);
                } അല്ലെങ്കിൽ {
                    numnew.opAssign(numb1);
                }
                AstConst* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                UINFO(4, "   -> " << newp);
            } അല്ലെങ്കിൽ {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* സ്ഥിരം newvarp
                    = പുതിയ AstVar{nodep->fileline(), VVarType::XTEMP, m_xrandNames->get(nullptr),
                                 VFlagLogicPacked{}, nodep->width()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                Ast_കണ്ണി_Var_Ref* സ്ഥിരം newref1p
                    = പുതിയ AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* സ്ഥിരം newinitp = പുതിയ AstInitial{
                    nodep->fileline(),
                    പുതിയ AstAssign{
                        nodep->fileline(),
                        പുതിയ AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        പുതിയ AstOr{nodep->fileline(), പുതിയ AstConst{nodep->fileline(), numb1},
                                  പുതിയ AstAnd{
                                      nodep->fileline(), പുതിയ AstConst{nodep->fileline(), numbx},
                                      പുതിയ AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_കണ്ണി* സ്ഥിരം newStmtsp = Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            }
        }
    }

    ശൂന്യം visit(AstSel* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->user1SetOnce()) {
            // Guard against reading/writing past end of bit vector array
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം basefromp = nodep->baseFromp(സത്യം);
            തർക്കം lvalue = അസത്യം;
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(basefromp, കണ്ണി_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            സ്ഥിരം uint32_t maxmsb = nodep->fromp()->dtypep()->width() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* സ്ഥിരം maxmsbConstp = പുതിയ AstConst{
                nodep->fileline(), AstConst::Widthed_മൂല്യം{}, nodep->lsbp()->width(), maxmsb};
            Ast_കണ്ണി_വ്യഞ്ജകം* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlinkFrBack());
            എങ്കിൽ (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_വൃക്ഷം(), maxmsbConstp);
                nodep->lsbp(lsbp);
                തിരികെ;
            }
            nodep->lsbp(new_വ്യഞ്ജകം_Stmt_Or_Clone(lsbp));
            Ast_കണ്ണി_വ്യഞ്ജകം* condp
                = V3Const::constifyEdit(പുതിയ AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            എങ്കിൽ (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                V3_സംഖ്യ xnum{nodep, nodep->width()};
                xnum.setAllBitsX();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം xexprp = പുതിയ AstConst{nodep->fileline(), xnum};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                    എങ്കിൽ (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_വൃക്ഷം(), condp);
                        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                        തിരികെ xexprp;
                    }
                    തിരികെ പുതിയ AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } അല്ലെങ്കിൽ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    ശൂന്യം visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->user1SetOnce()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_കണ്ണി* സ്ഥിരം basefromp = nodep->fromp()->baseFromp(സത്യം);
            തർക്കം lvalue = അസത്യം;
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(basefromp, കണ്ണി_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } അല്ലെങ്കിൽ {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            പൂർണ്ണസംഖ്യ declElements = -1;
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം adtypep = VN_CAST(dtypep, കണ്ണി_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            എങ്കിൽ (സ്ഥിരം AstModDiv* സ്ഥിരം moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    എങ്കിൽ (modconstp->width() <= 32
                        && modconstp->toUInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->toUInt());
                        തിരികെ;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* സ്ഥിരം declElementsp
                = പുതിയ AstConst{nodep->fileline(), AstConst::Widthed_മൂല്യം{}, nodep->bitp()->width(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_കണ്ണി_വ്യഞ്ജകം* bitp = V3Const::constifyEdit(nodep->bitp()->unlinkFrBack());
            എങ്കിൽ (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_വൃക്ഷം(), declElementsp);
                nodep->bitp(bitp);
                തിരികെ;
            }
            nodep->bitp(new_വ്യഞ്ജകം_Stmt_Or_Clone(bitp));
            Ast_കണ്ണി_വ്യഞ്ജകം* condp = പുതിയ AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം കണ്ണി_Dtp = nodep->dtypep()->skipRefp();
            എങ്കിൽ (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(കണ്ണി_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlinkFrBack(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_സംഖ്യ xnum{nodep, nodep->width()};
                എങ്കിൽ (കണ്ണി_Dtp->isDouble()) {
                    xnum = V3_സംഖ്യ{nodep, V3_സംഖ്യ::Double{}, 0.0};
                } അല്ലെങ്കിൽ എങ്കിൽ (കണ്ണി_Dtp->isString()) {
                    xnum = V3_സംഖ്യ{nodep, V3_സംഖ്യ::String{}, ""};
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (കണ്ണി_Dtp->isFourstate()) {
                        xnum.setAllBitsX();
                    } അല്ലെങ്കിൽ {
                        xnum.setAllBits0();
                    }
                }
                Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstCond{nodep->fileline(), condp, nodep,
                                                  പുതിയ AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } അല്ലെങ്കിൽ എങ്കിൽ (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം asBitp = nodep->bitp()->unlinkFrBack(&replaceHandle);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
                    = പുതിയ AstCond{asBitp->fileline(), condp, asBitp,
                                  പുതിയ AstConst{asBitp->fileline(), AstConst::Widthed_മൂല്യം{},
                                               asBitp->width(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } അല്ലെങ്കിൽ {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
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

സ്ഥിരം std::string UnknownVisitor::s_xrandPrefix = "__Vxrand";

//######################################################################
// Unknown class functions

ശൂന്യം V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("unknown", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
