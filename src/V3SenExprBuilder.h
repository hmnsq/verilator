// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Builder for sensitivity checking expressions.
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

#నిర్వచితం_కాకపోతే VERILATOR_V3SENEXPRBUILDER_H_
#నిర్వచించు VERILATOR_V3SENEXPRBUILDER_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_సమాసం_Builder final {
బహిరంగ:
    // TYPES
    నిర్మాణం Results final {
        std::సదిశ<Ast_కణుపు_Stmt*> m_inits;  // Initialization statements for previous values
        std::సదిశ<Ast_కణుపు_Stmt*> m_preUpdates;  // Pre update assignments
        std::సదిశ<Ast_కణుపు_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::సదిశ<Ast_కణుపు_Stmt*> m_destructivePostUpdates;
        std::సదిశ<AstVar*> m_vars;  // Created temporary variables
    };

అంతరంగ:
    // STATE
    AstScope* స్థిరరాశి m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_కణుపు>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_కణుపు>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_సమితి<VNRef<Ast_కణుపు>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_సమితి<VNRef<Ast_కణుపు>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3_Unique_పేర్లు m_prev_పేర్లు{"__Vtrigprevexpr"};  // For generating unique temporary names

    స్థిర bool isSupportedDType(Ast_కణుపు_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ఒకవేళ (VN_IS(dtypep, BasicDType)) తిరిగి true;
        ఒకవేళ (VN_IS(dtypep, PackArrayDType)) తిరిగి true;
        ఒకవేళ (VN_IS(dtypep, UnpackArrayDType)) తిరిగి isSupportedDType(dtypep->subDTypep());
        ఒకవేళ (VN_IS(dtypep, కణుపు_UOr_Struct_DType)) తిరిగి true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        ఒకవేళ (VN_IS(dtypep, ClassRefDType)) తిరిగి true;
        తిరిగి false;
    }

    స్థిర bool is_Simple_సమాసం(స్థిరరాశి Ast_కణుపు* స్థిరరాశి exprp) {
        తిరిగి exprp->forall([](స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep) {
            తిరిగి VN_IS(nodep, Const) || VN_IS(nodep, కణుపు_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, కణుపు_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    స్థిర bool isNullableRef(స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి refp) {
        ఒకవేళ (!refp->dtypep()) తిరిగి false;
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = refp->dtypep()->skipRefp();
        స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        తిరిగి VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    స్థిర bool hasNullableMemberAccess(స్థిరరాశి Ast_కణుపు* స్థిరరాశి exprp) {
        తిరిగి exprp && exprp->exists([](స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep) {
            స్థిరరాశి AstMemberSel* స్థిరరాశి mselp = VN_CAST(nodep, MemberSel);
            తిరిగి mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    స్థిర స్థిరరాశి Ast_కణుపు_సమాసం* get_పునాది_Nullable_Ref(స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        అయితే (exprp) {
            ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి mselp = VN_CAST(exprp, MemberSel)) {
                స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి fromp = mselp->fromp();
                ఒకవేళ (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    ఒకవేళ (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        కొనసాగు;
                    }
                    తిరిగి fromp;
                }
                exprp = fromp;
            } లేకపోతే {
                తిరిగి nullptr;
            }
        }
        తిరిగి nullptr;
    }

    // METHODS
    AstVarScope* crate_తాత్కాలిక(Ast_కణుపు_సమాసం* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* స్థిరరాశి vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prev_పేర్లు.get(name);
        } లేకపోతే {
            name = m_prev_పేర్లు.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_పంక్తి* స్థిరరాశి flp = m_scopep->fileline();
        AstVar* స్థిరరాశి varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->is_అంతర్గత(true);
        m_scopep->modp()->కూడు_Stmtsp(varp);
        AstVarScope* స్థిరరాశి vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->కూడు_Varsp(vscp);
        m_results.m_vars.నెట్టు_వెనుక(varp);
        తిరిగి vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_కణుపు_Stmt* wrapStmtWithNullCheck(File_పంక్తి* flp, Ast_కణుపు_Stmt* stmtp,
                                       స్థిరరాశి Ast_కణుపు_సమాసం* పునాది_Refp) {
        ఒకవేళ (!పునాది_Refp) తిరిగి stmtp;
        Ast_కణుపు_సమాసం* స్థిరరాశి nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_కణుపు_సమాసం* స్థిరరాశి checkp
            = new AstNeq{flp, const_cast<Ast_కణుపు_సమాసం*>(పునాది_Refp)->clone_వృక్షం(false), nullp};
        తిరిగి new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_కణుపు_సమాసం* wrap_సమాసం_With_Null_Check(File_పంక్తి* flp, Ast_కణుపు_సమాసం* exprp,
                                       స్థిరరాశి Ast_కణుపు_సమాసం* పునాది_Refp) {
        ఒకవేళ (!పునాది_Refp) తిరిగి exprp;
        Ast_కణుపు_సమాసం* స్థిరరాశి nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_కణుపు_సమాసం* స్థిరరాశి checkp
            = new AstNeq{flp, const_cast<Ast_కణుపు_సమాసం*>(పునాది_Refp)->clone_వృక్షం(false), nullp};
        Ast_కణుపు_సమాసం* స్థిరరాశి falsep = new AstConst{flp, AstConst::బిట్టు_False{}};
        Ast_కణుపు_సమాసం* స్థిరరాశి condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtype_సమితి_బిట్టు();
        తిరిగి condp;
    }

    Ast_కణుపు_సమాసం* getCurr(Ast_కణుపు_సమాసం* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        ఒకవేళ (is_Simple_సమాసం(exprp)) తిరిగి exprp->clone_వృక్షం(false);

        // Create the 'current value' variable
        File_పంక్తి* స్థిరరాశి flp = exprp->fileline();
        స్వయంచాలక ఫలితం = m_curr.emplace(*exprp, nullptr);
        ఒకవేళ (ఫలితం.రెండవ) ఫలితం.మొదటి->రెండవ = crate_తాత్కాలిక(exprp);
        AstVarScope* స్థిరరాశి currp = ఫలితం.మొదటి->రెండవ;

        // Check if we need null guards for a member access through a nullable reference
        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి పునాది_Refp
            = hasNullableMemberAccess(exprp) ? get_పునాది_Nullable_Ref(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        ఒకవేళ (m_hasPreUpdate.emplace(*currp).రెండవ) {
            m_results.m_preUpdates.నెట్టు_వెనుక(
                wrapStmtWithNullCheck(flp,
                                      new Ast_కేటాయించు{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_వృక్షం(false)},
                                      పునాది_Refp));
        }
        తిరిగి new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_కణుపు_సమాసం* exprp) {
        File_పంక్తి* స్థిరరాశి flp = exprp->fileline();
        స్థిరరాశి స్వయంచాలక rdCurr = [this, exprp]() { తిరిగి getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి పునాది_Refp
            = hasNullableMemberAccess(exprp) ? get_పునాది_Nullable_Ref(exprp) : nullptr;

        Ast_కణుపు* scopeExprp = exprp;
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        స్థిరరాశి స్వయంచాలక pair = m_prev.emplace(*scopeExprp, nullptr);
        ఒకవేళ (pair.రెండవ) {
            AstVarScope* స్థిరరాశి prevp = crate_తాత్కాలిక(exprp);
            pair.మొదటి->రెండవ = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            Ast_కేటాయించు* స్థిరరాశి initp = new Ast_కేటాయించు{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_వృక్షం(false)};
            m_results.m_inits.నెట్టు_వెనుక(wrapStmtWithNullCheck(flp, initp, పునాది_Refp));
        }

        AstVarScope* స్థిరరాశి prevp = pair.మొదటి->రెండవ;

        స్థిరరాశి స్వయంచాలక wrPrev = [=]() { తిరిగి new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        ఒకవేళ (m_hasPostUpdate.emplace(*exprp).రెండవ) {
            Ast_కణుపు_DType* స్థిరరాశి సమాసం_Dtp = exprp->dtypep()->skipRefp();
            ఒకవేళ (!isSupportedDType(సమాసం_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << సమాసం_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warn_మరిన్ని()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                తిరిగి prevp;
            }
            ఒకవేళ (VN_IS(సమాసం_Dtp, UnpackArrayDType)) {
                AstCMethodHard* స్థిరరాశి cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_కేటాయించు, rdCurr()};
                cmhp->dtype_సమితి_Void();
                m_results.m_postUpdates.నెట్టు_వెనుక(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), పునాది_Refp));
            } లేకపోతే {
                m_results.m_postUpdates.నెట్టు_వెనుక(
                    wrapStmtWithNullCheck(flp, new Ast_కేటాయించు{flp, wrPrev(), rdCurr()}, పునాది_Refp));
            }
        }

        తిరిగి prevp;
    }

    std::pair<Ast_కణుపు_సమాసం*, bool> create_పదం(స్థిరరాశి Ast_Sen_అంశం* senItemp) {
        File_పంక్తి* స్థిరరాశి flp = senItemp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి పునాది_Refp
            = hasNullableMemberAccess(senp) ? get_పునాది_Nullable_Ref(senp) : nullptr;

        స్థిరరాశి స్వయంచాలక currp = [this, senp]() { తిరిగి getCurr(senp); };
        స్థిరరాశి స్వయంచాలక prevp
            = [this, flp, senp]() { తిరిగి new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        స్థిరరాశి స్వయంచాలక lsb = [=](Ast_కణుపు_సమాసం* opp) { తిరిగి new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        ఎంపిక (senItemp->అంచు_Type()) {
        స్థితి VEdgeType::ET_మారింది:
        స్థితి VEdgeType::ET_HYBRID:  //
            ఒకవేళ (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* స్థిరరాశి resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtype_సమితి_బిట్టు();
                తిరిగి {wrap_సమాసం_With_Null_Check(flp, resultp, పునాది_Refp), true};
            }
            తిరిగి {wrap_సమాసం_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, పునాది_Refp), true};
        స్థితి VEdgeType::ET_BOTHEDGE:  //
            తిరిగి {wrap_సమాసం_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), పునాది_Refp),
                    false};
        స్థితి VEdgeType::ET_POSEDGE:  //
            తిరిగి {wrap_సమాసం_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), పునాది_Refp),
                    false};
        స్థితి VEdgeType::ET_NEGEDGE:  //
            తిరిగి {wrap_సమాసం_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), పునాది_Refp),
                    false};
        స్థితి VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_కణుపు_సమాసం* స్థిరరాశి clearFromp = currp();
            ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి refp
                = VN_CAST(clearFromp->get_VAccess_లక్ష్యం_Recurse(), కణుపు_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* స్థిరరాశి clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtype_సమితి_Void();
            Ast_కణుపు_Stmt* స్థిరరాశి clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), పునాది_Refp);
            m_results.m_postUpdates.నెట్టు_వెనుక(clearStmtp);
            m_results.m_destructivePostUpdates.నెట్టు_వెనుక(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* స్థిరరాశి callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtype_సమితి_బిట్టు();
            Ast_కణుపు_సమాసం* స్థిరరాశి firedp = wrap_సమాసం_With_Null_Check(flp, callp, పునాది_Refp);
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                ఒకవేళ (refp->varScopep() == v3Global.rootp()->nbaEventp()) తిరిగి {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_కణుపు_సమాసం* స్థిరరాశి notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            తిరిగి {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        స్థితి VEdgeType::ET_TRUE:  //
            తిరిగి {currp(), false};
        స్థితి VEdgeType::ET_INITIAL_NBA:  //
            తిరిగి {new AstConst{flp, AstConst::బిట్టు_False{}}, true};
        అప్రమేయ:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            తిరిగి {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

బహిరంగ:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_కణుపు_సమాసం*, bool> build(స్థిరరాశి Ast_Sen_అంశం* senItemp) {
        స్వయంచాలక పదం = create_పదం(senItemp);
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి condp = senItemp->condp()) {
            పదం.మొదటి = new AstAnd{senItemp->fileline(), condp->clone_వృక్షం_శుద్ధ(false), పదం.మొదటి};
        }
        తిరిగి పదం;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_కణుపు_సమాసం*, bool> build(స్థిరరాశి Ast_Sen_వృక్షం* senTreep) {
        File_పంక్తి* స్థిరరాశి flp = senTreep->fileline();
        Ast_కణుపు_సమాసం* resultp = nullptr;
        bool firedAtInitialization = false;
        కొరకు (Ast_Sen_అంశం* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), Sen_అంశం)) {
            స్థిరరాశి స్వయంచాలక& pair = build(senItemp);
            ఒకవేళ (Ast_కణుపు_సమాసం* termp = pair.మొదటి) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.రెండవ;
            }
        }
        తిరిగి {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        తిరిగి ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        తిరిగి std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_సమాసం_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#అయితే_ముగింపు  // Guard
