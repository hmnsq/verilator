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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Unknown.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి UnknownVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstSel::user()          -> bool.  Set true if already processed
    //  AstArraySel::user()     -> bool.  Set true if already processed
    //  AstNode::user2p()       -> AstIf* Inserted if assignment for conditional
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    స్థిర స్థిరరాశి std::string s_xrand_ఉపసర్గ;

    // STATE - across all visitors
    VDouble0 m_statUnkVars;  // Statistic of xrand variable created
    VDouble0 m_statElses;  // Statistic of else branches created for array selects
    V3_Unique_పేర్లు m_lvbound_పేర్లు;  // For generating unique temporary variable names
    std::unique_ptr<V3_Unique_పేర్లు> m_xrand_పేర్లు;  // For generating unique temporary variable names

    // STATE - for current visit position (use VL_RESTORER)
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    తార్కిక m_constXCvt = అబద్ధం;  // Convert X's
    తార్కిక m_allowXUnique = నిజం;  // Allow unique assignments

    // METHODS

    శూన్య కూడు_Var(AstVar* varp) {
        ఒకవేళ (m_ftaskp) {
            varp->funcLocal(నిజం);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_ftaskp->prependStmtsp(varp);
        } లేకపోతే {
            m_modp->prependStmtsp(varp);
        }
    }

    శూన్య replaceBoundLvalue(Ast_కణుపు_సమాసం* nodep, Ast_కణుపు_సమాసం* condp) {
        // Spec says a out-of-range LHS SEL results in a NOP.
        // We wrap the expression into IF
        Ast_కణుపు_సమాసం* prep = nodep;

        // Scan back to put the condlvalue above all selects (IE top of the lvalue)
        అయితే (VN_IS(prep->backp(), కణుపు_Sel) || VN_IS(prep->backp(), Sel)
               || VN_IS(prep->backp(), MemberSel) || VN_IS(prep->backp(), StructSel)) {
            prep = VN_AS(prep->backp(), కణుపు_సమాసం);
        }
        ఒకవేళ (VN_IS(prep->backp(), కేటాయించు_Force) || VN_IS(prep->backp(), Release)) {
            // The conversion done in this function breaks force and release statements
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Force / release statement with complex select expression");
            VL_DO_DANGLING(condp->delete_వృక్షం(), condp);
            తిరిగి;
        }
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        VL_DANGLING(nodep);  // Zap it so we don't use it by mistake - use prep

        // Already exists; rather than IF(a,... IF(b... optimize to IF(a&&b,
        // Saves us teaching V3Const how to optimize, and it won't be needed again.
        ఒకవేళ (స్థిరరాశి AstIf* స్థిరరాశి ifp = VN_AS(prep->user2p(), If)) {
            VNRelinker replaceHandle;
            Ast_కణుపు_సమాసం* స్థిరరాశి earliercondp = ifp->condp()->unlink_Fr_వెనుక(&replaceHandle);
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstLogAnd{condp->fileline(), condp, earliercondp};
            UINFO(4, "Edit BOUNDLVALUE " << newp);
            replaceHandle.relink(newp);
        } లేకపోతే {
            Ast_కణుపు* stmtp = prep->backp();  // Grab above point before we replace 'prep'
            అయితే (!VN_IS(stmtp, కణుపు_Stmt)) stmtp = stmtp->backp();
            VNRelinker replaceHandle;
            Ast_కణుపు* స్థిరరాశి origStmtp = stmtp->unlink_Fr_వెనుక(&replaceHandle);
            Ast_కణుపు* elseStmtp = శూన్యసూచిక;
            స్థిరరాశి తార్కిక hasSideEffects = origStmtp->exists(
                [](Ast_కణుపు* స్థిరరాశి np) { తిరిగి !np->is_శుద్ధ() || np->isTimingControl(); });
            ఒకవేళ (hasSideEffects) {
                // Copy original statement and replace `prep` with reference to tmp var to make
                // sure that side effect will take place
                m_statElses++;
                elseStmtp = origStmtp->clone_వృక్షం(అబద్ధం);
                AstVar* స్థిరరాశి varp = కొత్త AstVar{fl, VVarType::MODULETEMP, m_lvbound_పేర్లు.get(prep),
                                                prep->dtypep()};
                కూడు_Var(varp);
                Ast_కణుపు* స్థిరరాశి prepCopyp = prep->clonep();
                prepCopyp->replaceWith(కొత్త AstVarRef{fl, varp, VAccess::WRITE});
                VL_DO_DANGLING(నెట్టు_Deletep(prepCopyp), prepCopyp);
            }
            AstIf* స్థిరరాశి newp = కొత్త AstIf{fl, condp, origStmtp, elseStmtp};
            replaceHandle.relink(newp);
            newp->branchPred(VBranchPred::BP_LIKELY);
            newp->isBoundsCheck(నిజం);
            prep->user2p(newp);  // Save so we may LogAnd it next time
        }
    }

    AstVar* create_కూడు_తాత్కాలిక(స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి nodep) {
        AstVar* స్థిరరాశి varp = కొత్త AstVar{nodep->fileline(), VVarType::XTEMP,
                                        m_xrand_పేర్లు->get(nodep), nodep->dtypep()};
        కూడు_Var(varp);
        తిరిగి varp;
    }

    // Returns true if it is known at compile time that `msbConstp` is greater than or equal
    // `exprp`
    స్థిర తార్కిక isStaticlyGte(AstConst* స్థిరరాశి msbConstp, స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి exprp) {
        ఒకవేళ (msbConstp->వెడల్పు() >= exprp->వెడల్పు() && exprp->వెడల్పు() < 32
            && msbConstp->అంకె().గమ్యం_SInt() >= (1 << exprp->వెడల్పు()) - 1) {
            తిరిగి నిజం;
        }
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(exprp, Const)) {
            ఒకవేళ (V3_సంఖ్య{msbConstp}.opGte(msbConstp->అంకె(), constp->అంకె()).isNeqZero()) {
                తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }

    Ast_కణుపు_సమాసం* new_సమాసం_Stmt_Or_Clone(Ast_కణుపు_సమాసం*& exprp) {
        ఒకవేళ (!exprp->is_శుద్ధ()) {
            AstVar* స్థిరరాశి varp = create_కూడు_తాత్కాలిక(exprp);
            File_పంక్తి* స్థిరరాశి fl = exprp->fileline();
            exprp = కొత్త Ast_సమాసం_Stmt{
                fl, కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE}, exprp},
                కొత్త AstVarRef{fl, varp, VAccess::READ}};
            తిరిగి కొత్త AstVarRef{fl, varp, VAccess::READ};
        }
        తిరిగి exprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_constXCvt);
        VL_RESTORER(m_allowXUnique);
        స్వయంచాలక xrand_పేర్లు = std::make_unique<V3_Unique_పేర్లు>(s_xrand_ఉపసర్గ);
        {
            m_modp = nodep;
            m_constXCvt = నిజం;
            // Class X randomization causes Vxrand in strange places, so disable
            ఒకవేళ (VN_IS(nodep, Class)) m_allowXUnique = అబద్ధం;
            m_lvbound_పేర్లు.reset();
            xrand_పేర్లు.మార్పిడి(m_xrand_పేర్లు);
            iterateChildren(nodep);
            xrand_పేర్లు.మార్పిడి(m_xrand_పేర్లు);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Case_అంశం* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = అబద్ధం;  // Avoid losing the X's in casex
        iterate_And_తదుపరి_Null(nodep->condsp());
        m_constXCvt = నిజం;
        iterate_And_తదుపరి_Null(nodep->stmtsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_DType* nodep) override {
        VL_RESTORER(m_constXCvt);
        m_constXCvt = అబద్ధం;  // Avoid losing the X's in casex
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        VL_RESTORER(m_allowXUnique);
        ఒకవేళ (nodep->isParam()) m_allowXUnique = అబద్ధం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు_Eq_Neq_Case(Ast_కణుపు_Biop* nodep) {
        UINFO(4, " N/EQCASE->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ఒకవేళ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            తిరిగి;
        } లేకపోతే {
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* newp;
            // If we got ==1'bx it can never be true (but 1'bx==1'bx can be!)
            ఒకవేళ (((VN_IS(lhsp, Const) && VN_AS(lhsp, Const)->అంకె().isFourState())
                 || (VN_IS(rhsp, Const) && VN_AS(rhsp, Const)->అంకె().isFourState()))) {
                newp = కొత్త AstConst(nodep->fileline(), AstConst::Widthed_విలువ{}, 1,
                                    (VN_IS(nodep, EqCase) ? 0 : 1));
                VL_DO_DANGLING(lhsp->delete_వృక్షం(), lhsp);
                VL_DO_DANGLING(rhsp->delete_వృక్షం(), rhsp);
            } లేకపోతే {
                ఒకవేళ (VN_IS(nodep, EqCase)) {
                    newp = కొత్త AstEq{nodep->fileline(), lhsp, rhsp};
                } లేకపోతే {
                    newp = కొత్త AstNeq{nodep->fileline(), lhsp, rhsp};
                }
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            // Iterate tree now that we may have gotten rid of Xs
            iterateChildren(newp);
        }
    }
    శూన్య సందర్శించు_Eq_Neq_Wild(Ast_కణుపు_Biop* nodep) {
        UINFO(4, " N/EQWILD->EQ " << nodep);
        V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
        V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        ఒకవేళ (VN_IS(nodep->lhsp(), Const) && VN_IS(nodep->rhsp(), Const)) {
            // Both sides are constant, node can be constant
            VL_DO_DANGLING(V3Const::constifyEdit(nodep), nodep);
            తిరిగి;
        } లేకపోతే {
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* newp;
            ఒకవేళ (!VN_IS(rhsp, Const)) {
                ఒకవేళ (rhsp->dtypep()->isFourstate()) {
                    nodep->v3warn(
                        E_UNSUPPORTED,
                        "Unsupported: RHS of ==? or !=? is fourstate but not a constant");
                }
                newp = కొత్త AstEq{nodep->fileline(), lhsp, rhsp};
            } లేకపోతే {
                // X or Z's become mask, ala case statements.
                V3_సంఖ్య nummask{rhsp, rhsp->వెడల్పు()};
                nummask.op_బిట్లు_Non_XZ(VN_AS(rhsp, Const)->అంకె());
                V3_సంఖ్య numval{rhsp, rhsp->వెడల్పు()};
                numval.op_బిట్లు_One(VN_AS(rhsp, Const)->అంకె());
                Ast_కణుపు_సమాసం* స్థిరరాశి and1p = కొత్త AstAnd{nodep->fileline(), lhsp,
                                                      కొత్త AstConst{nodep->fileline(), nummask}};
                Ast_కణుపు_సమాసం* స్థిరరాశి and2p = కొత్త AstConst{nodep->fileline(), numval};
                ఒకవేళ (VN_IS(nodep, EqWild)) {
                    newp = కొత్త AstEq{nodep->fileline(), and1p, and2p};
                } లేకపోతే {
                    newp = కొత్త AstNeq{nodep->fileline(), and1p, and2p};
                }
                VL_DO_DANGLING(rhsp->delete_వృక్షం(), rhsp);
            }
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            // Iterate tree now that we may have gotten rid of the compare
            iterateChildren(newp);
        }
    }

    శూన్య సందర్శించు(AstEqCase* nodep) override { సందర్శించు_Eq_Neq_Case(nodep); }
    శూన్య సందర్శించు(AstNeqCase* nodep) override { సందర్శించు_Eq_Neq_Case(nodep); }
    శూన్య సందర్శించు(AstEqWild* nodep) override { సందర్శించు_Eq_Neq_Wild(nodep); }
    శూన్య సందర్శించు(AstNeqWild* nodep) override { సందర్శించు_Eq_Neq_Wild(nodep); }
    శూన్య సందర్శించు(AstIsUnknown* nodep) override {
        iterateChildren(nodep);
        // Ahh, we're two state, so this is easy
        UINFO(4, " ISUNKNOWN->0 " << nodep);
        AstConst* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_లెక్క_బిట్లు* nodep) override {
        // Ahh, we're two state, so this is easy
        std::array<తార్కిక, 3> dropop;
        dropop[0] = VN_IS(nodep->rhsp(), Const) && VN_AS(nodep->rhsp(), Const)->అంకె().isAnyX();
        dropop[1] = VN_IS(nodep->thsp(), Const) && VN_AS(nodep->thsp(), Const)->అంకె().isAnyX();
        dropop[2] = VN_IS(nodep->fhsp(), Const) && VN_AS(nodep->fhsp(), Const)->అంకె().isAnyX();
        UINFO(4, " COUNTBITS(" << dropop[0] << dropop[1] << dropop[2] << " " << nodep);

        Ast_కణుపు_సమాసం* nonXp = శూన్యసూచిక;
        ఒకవేళ (!dropop[0]) {
            nonXp = nodep->rhsp();
        } లేకపోతే ఒకవేళ (!dropop[1]) {
            nonXp = nodep->thsp();
        } లేకపోతే ఒకవేళ (!dropop[2]) {
            nonXp = nodep->fhsp();
        } లేకపోతే {  // Was all X-s
            UINFO(4, " COUNTBITS('x)->0 " << nodep);
            AstConst* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        ఒకవేళ (dropop[0]) {
            nodep->rhsp()->unlink_Fr_వెనుక()->delete_వృక్షం();
            nodep->rhsp(nonXp->clone_వృక్షం_శుద్ధ(నిజం));
        }
        ఒకవేళ (dropop[1]) {
            nodep->thsp()->unlink_Fr_వెనుక()->delete_వృక్షం();
            nodep->thsp(nonXp->clone_వృక్షం_శుద్ధ(నిజం));
        }
        ఒకవేళ (dropop[2]) {
            nodep->fhsp()->unlink_Fr_వెనుక()->delete_వృక్షం();
            nodep->fhsp(nonXp->clone_వృక్షం_శుద్ధ(నిజం));
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        ఒకవేళ (m_constXCvt && nodep->అంకె().isFourState()) {
            UINFO(4, " CONST4 " << nodep);
            UINFOTREE(9, nodep, "", "Const_old");
            // CONST(num) -> VARREF(newvarp)
            //          -> VAR(newvarp)
            //          -> INITIAL(VARREF(newvarp, OR(num_No_Xs,AND(random,num_1s_Where_X))
            V3_సంఖ్య numb1{nodep, nodep->వెడల్పు()};
            numb1.op_బిట్లు_One(nodep->అంకె());
            V3_సంఖ్య numbx{nodep, nodep->వెడల్పు()};
            numbx.op_బిట్లు_XZ(nodep->అంకె());
            ఒకవేళ (!m_allowXUnique || v3Global.opt.x_కేటాయించు() != "unique") {
                // All X bits just become 0; fastest simulation, but not nice
                V3_సంఖ్య numnew{nodep, numb1.వెడల్పు()};
                ఒకవేళ (v3Global.opt.x_కేటాయించు() == "1") {
                    numnew.opOr(numb1, numbx);
                } లేకపోతే {
                    numnew.op_కేటాయించు(numb1);
                }
                AstConst* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), numnew};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                UINFO(4, "   -> " << newp);
            } లేకపోతే {
                // Make a Vxrand variable
                // We use the special XTEMP type so it doesn't break pure functions
                UASSERT_OBJ(m_modp, nodep, "X number not under module");
                AstVar* స్థిరరాశి newvarp
                    = కొత్త AstVar{nodep->fileline(), VVarType::XTEMP, m_xrand_పేర్లు->get(శూన్యసూచిక),
                                 VFlagLogicPacked{}, nodep->వెడల్పు()};
                newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
                ++m_statUnkVars;
                VNRelinker replaceHandle;
                nodep->unlink_Fr_వెనుక(&replaceHandle);
                Ast_కణుపు_Var_Ref* స్థిరరాశి newref1p
                    = కొత్త AstVarRef{nodep->fileline(), newvarp, VAccess::READ};
                replaceHandle.relink(newref1p);  // Replace const with varref
                AstInitial* స్థిరరాశి newinitp = కొత్త AstInitial{
                    nodep->fileline(),
                    కొత్త Ast_కేటాయించు{
                        nodep->fileline(),
                        కొత్త AstVarRef{nodep->fileline(), newvarp, VAccess::WRITE},
                        కొత్త AstOr{nodep->fileline(), కొత్త AstConst{nodep->fileline(), numb1},
                                  కొత్త AstAnd{
                                      nodep->fileline(), కొత్త AstConst{nodep->fileline(), numbx},
                                      కొత్త AstVarRef{nodep->fileline(), newvarp, VAccess::READ}}}}};
                // Add inits in front of other statement.
                // In the future, we should stuff the initp into the module's constructor.
                // Must not unlink the statements, as one of them is being iterated
                Ast_కణుపు* స్థిరరాశి newStmtsp = Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(newvarp, newinitp);
                m_modp->prependStmtsp(newStmtsp);
                UINFOTREE(9, newref1p, "", "_newref");
                UINFOTREE(9, newvarp, "", "_newvar");
                UINFOTREE(9, newinitp, "", "_newini");
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            }
        }
    }

    శూన్య సందర్శించు(AstSel* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!nodep->user1_సమితి_Once()) {
            // Guard against reading/writing past end of bit vector array
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి basefromp = nodep->పునాది_Fromp(నిజం);
            తార్కిక lvalue = అబద్ధం;
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(basefromp, కణుపు_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            }
            // Find range of dtype we are selecting from
            // Similar code in V3Const::warnSelect
            స్థిరరాశి uint32_t maxmsb = nodep->fromp()->dtypep()->వెడల్పు() - 1;
            UINFOTREE(9, nodep, "", "sel_old");

            // If (maxmsb >= selected), we're in bound
            // See if the condition is constant true (e.g. always in bound due to constant select)
            // Note below has null backp(); the Edit function knows how to deal with that.
            AstConst* స్థిరరాశి maxmsbConstp = కొత్త AstConst{
                nodep->fileline(), AstConst::Widthed_విలువ{}, nodep->lsbp()->వెడల్పు(), maxmsb};
            Ast_కణుపు_సమాసం* lsbp = V3Const::constifyEdit(nodep->lsbp()->unlink_Fr_వెనుక());
            ఒకవేళ (isStaticlyGte(maxmsbConstp, lsbp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(maxmsbConstp->delete_వృక్షం(), maxmsbConstp);
                nodep->lsbp(lsbp);
                తిరిగి;
            }
            nodep->lsbp(new_సమాసం_Stmt_Or_Clone(lsbp));
            Ast_కణుపు_సమాసం* condp
                = V3Const::constifyEdit(కొత్త AstGte{nodep->fileline(), maxmsbConstp, lsbp});
            ఒకవేళ (!lvalue) {
                // SEL(...) -> COND(LTE(bit<=maxmsb), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_వెనుక(&replaceHandle);
                V3_సంఖ్య xnum{nodep, nodep->వెడల్పు()};
                xnum.సమితి_All_బిట్లు_X();
                Ast_కణుపు_సమాసం* స్థిరరాశి xexprp = కొత్త AstConst{nodep->fileline(), xnum};
                Ast_కణుపు_సమాసం* స్థిరరాశి newp = [&]() -> Ast_కణుపు_సమాసం* {
                    ఒకవేళ (condp->isZero()) {
                        VL_DO_DANGLING(condp->delete_వృక్షం(), condp);
                        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                        తిరిగి xexprp;
                    }
                    తిరిగి కొత్త AstCond{nodep->fileline(), condp, nodep, xexprp};
                }();
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } లేకపోతే {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }

    // visit(AstSliceSel) not needed as its bounds are constant and checked
    // in V3Width.

    శూన్య సందర్శించు(AstArraySel* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!nodep->user1_సమితి_Once()) {
            UINFOTREE(9, nodep, "", "in");
            // Guard against reading/writing past end of arrays
            Ast_కణుపు* స్థిరరాశి basefromp = nodep->fromp()->పునాది_Fromp(నిజం);
            తార్కిక lvalue = అబద్ధం;
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(basefromp, కణుపు_Var_Ref)) {
                lvalue = varrefp->access().isWriteOrRW();
            } లేకపోతే ఒకవేళ (VN_IS(basefromp, Const)) {
                // If it's a PARAMETER[bit], then basefromp may be a constant instead of a varrefp
            } లేకపోతే {
                // Normally one of above, but might have MEMBERSEL or otherwise
            }
            // Find range of dtype we are selecting from
            పూర్ణాంకం declElements = -1;
            Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->fromp()->dtypep()->skipRefp();
            UASSERT_OBJ(dtypep, nodep, "Select of non-selectable type");
            స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి adtypep = VN_CAST(dtypep, కణుపు_Array_DType);
            UASSERT_OBJ(adtypep, nodep, "Select from non-array " << dtypep->prettyTypeName());
            declElements = adtypep->elementsConst();
            UINFOTREE(9, nodep, "", "arraysel_old");

            // If value MODDIV constant, where constant <= declElements, known ok
            // V3Random makes these to intentionally prevent exceeding enum array bounds.
            ఒకవేళ (స్థిరరాశి AstModDiv* స్థిరరాశి moddivp = VN_CAST(nodep->bitp(), ModDiv)) {
                ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి modconstp = VN_CAST(moddivp->rhsp(), Const)) {
                    ఒకవేళ (modconstp->వెడల్పు() <= 32
                        && modconstp->గమ్యం_UInt() <= static_cast<uint32_t>(declElements)) {
                        UINFO(9, "arraysel mod const " << declElements
                                                       << " >= " << modconstp->గమ్యం_UInt());
                        తిరిగి;
                    }
                }
            }

            // See if the condition is constant true
            AstConst* స్థిరరాశి declElementsp
                = కొత్త AstConst{nodep->fileline(), AstConst::Widthed_విలువ{}, nodep->bitp()->వెడల్పు(),
                               static_cast<uint32_t>(declElements - 1)};
            Ast_కణుపు_సమాసం* bitp = V3Const::constifyEdit(nodep->bitp()->unlink_Fr_వెనుక());
            ఒకవేళ (isStaticlyGte(declElementsp, bitp)) {
                // We don't need to add a conditional; we know the existing expression is ok
                VL_DO_DANGLING(declElementsp->delete_వృక్షం(), declElementsp);
                nodep->bitp(bitp);
                తిరిగి;
            }
            nodep->bitp(new_సమాసం_Stmt_Or_Clone(bitp));
            Ast_కణుపు_సమాసం* condp = కొత్త AstGte{nodep->fileline(), declElementsp, bitp};
            // Note below has null backp(); the Edit function knows how to deal with that.
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి కణుపు_Dtp = nodep->dtypep()->skipRefp();
            ఒకవేళ (!lvalue
                // Making a scalar would break if we're making an array
                && VN_IS(కణుపు_Dtp, BasicDType)) {
                // ARRAYSEL(...) -> COND(LT(bit<maxbit), ARRAYSEL(...), {width{1'bx}})
                VNRelinker replaceHandle;
                nodep->unlink_Fr_వెనుక(&replaceHandle);
                // TODO make a tieoff function that takes AstNode and returns typed value
                V3_సంఖ్య xnum{nodep, nodep->వెడల్పు()};
                ఒకవేళ (కణుపు_Dtp->isDouble()) {
                    xnum = V3_సంఖ్య{nodep, V3_సంఖ్య::Double{}, 0.0};
                } లేకపోతే ఒకవేళ (కణుపు_Dtp->isString()) {
                    xnum = V3_సంఖ్య{nodep, V3_సంఖ్య::String{}, ""};
                } లేకపోతే {
                    ఒకవేళ (కణుపు_Dtp->isFourstate()) {
                        xnum.సమితి_All_బిట్లు_X();
                    } లేకపోతే {
                        xnum.సమితి_All_Bits0();
                    }
                }
                Ast_కణుపు* స్థిరరాశి newp = కొత్త AstCond{nodep->fileline(), condp, nodep,
                                                  కొత్త AstConst{nodep->fileline(), xnum}};
                UINFOTREE(9, newp, "", "_new");
                // Link in conditional, can blow away temp xor
                replaceHandle.relink(newp);
                // Added X's, tristate them too
                iterate(newp);
            } లేకపోతే ఒకవేళ (!lvalue) {  // Mid-multidimension read, just use zero
                // ARRAYSEL(...) -> ARRAYSEL(COND(LT(bit<maxbit), bit, 0))
                VNRelinker replaceHandle;
                Ast_కణుపు_సమాసం* స్థిరరాశి asBitp = nodep->bitp()->unlink_Fr_వెనుక(&replaceHandle);
                Ast_కణుపు_సమాసం* స్థిరరాశి newp
                    = కొత్త AstCond{asBitp->fileline(), condp, asBitp,
                                  కొత్త AstConst{asBitp->fileline(), AstConst::Widthed_విలువ{},
                                               asBitp->వెడల్పు(), 0}};
                // Added X's, tristate them too
                UINFOTREE(9, newp, "", "_new");
                replaceHandle.relink(newp);
                iterate(newp);
            } లేకపోతే {  // lvalue
                replaceBoundLvalue(nodep, condp);
            }
        }
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన UnknownVisitor(AstNetlist* nodep)
        : m_lvbound_పేర్లు{"__Vlvbound"}
        , m_xrand_పేర్లు{std::make_unique<V3_Unique_పేర్లు>(s_xrand_ఉపసర్గ)} {
        iterate(nodep);
    }
    ~UnknownVisitor() override {  //
        V3Stats::కూడు_Stat("Unknowns, variables created", m_statUnkVars);
        V3Stats::కూడు_Stat("Unknowns, else branches created", m_statElses);
    }
};

స్థిరరాశి std::string UnknownVisitor::s_xrand_ఉపసర్గ = "__Vxrand";

//######################################################################
// Unknown class functions

శూన్య V3Unknown::unknownAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { UnknownVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("unknown", 0, dump_వృక్షం_Either_Level() >= 3);
}
