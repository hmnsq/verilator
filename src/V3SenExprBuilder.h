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

#اگر_غیر_تعریف_شدہ VERILATOR_V3SENEXPRBUILDER_H_
#تعریف_کرو VERILATOR_V3SENEXPRBUILDER_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_اظہاریہ_Builder final {
عوامی:
    // TYPES
    ساخت Results final {
        std::سمتیہ<Ast_گرہ_Stmt*> m_inits;  // Initialization statements for previous values
        std::سمتیہ<Ast_گرہ_Stmt*> m_preUpdates;  // Pre update assignments
        std::سمتیہ<Ast_گرہ_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::سمتیہ<Ast_گرہ_Stmt*> m_destructivePostUpdates;
        std::سمتیہ<AstVar*> m_vars;  // Created temporary variables
    };

نجی:
    // STATE
    AstScope* ثابت m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_گرہ>, AstVarScope*> m_پچھلا;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_گرہ>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_سیٹ<VNRef<Ast_گرہ>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_سیٹ<VNRef<Ast_گرہ>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3_Unique_نام m_پچھلا_نام{"__Vtrigprevexpr"};  // For generating unique temporary names

    ساکن bool isSupportedDType(Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (VN_IS(dtypep, BasicDType)) واپس true;
        اگر (VN_IS(dtypep, PackArrayDType)) واپس true;
        اگر (VN_IS(dtypep, UnpackArrayDType)) واپس isSupportedDType(dtypep->subDTypep());
        اگر (VN_IS(dtypep, گرہ_UOr_Struct_DType)) واپس true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        اگر (VN_IS(dtypep, ClassRefDType)) واپس true;
        واپس false;
    }

    ساکن bool is_Simple_اظہاریہ(ثابت Ast_گرہ* ثابت exprp) {
        واپس exprp->forall([](ثابت Ast_گرہ* ثابت nodep) {
            واپس VN_IS(nodep, Const) || VN_IS(nodep, گرہ_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, گرہ_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ساکن bool isNullableRef(ثابت Ast_گرہ_اظہاریہ* ثابت refp) {
        اگر (!refp->dtypep()) واپس false;
        ثابت Ast_گرہ_DType* ثابت dtypep = refp->dtypep()->skipRefp();
        ثابت AstIfaceRefDType* ثابت ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        واپس VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ساکن bool hasNullableMemberAccess(ثابت Ast_گرہ* ثابت exprp) {
        واپس exprp && exprp->exists([](ثابت Ast_گرہ* ثابت nodep) {
            ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel);
            واپس mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ساکن ثابت Ast_گرہ_اظہاریہ* get_بنیاد_Nullable_Ref(ثابت Ast_گرہ_اظہاریہ* exprp) {
        جب (exprp) {
            اگر (ثابت AstMemberSel* ثابت mselp = VN_CAST(exprp, MemberSel)) {
                ثابت Ast_گرہ_اظہاریہ* ثابت fromp = mselp->fromp();
                اگر (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    اگر (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        جاری;
                    }
                    واپس fromp;
                }
                exprp = fromp;
            } ورنہ {
                واپس nullptr;
            }
        }
        واپس nullptr;
    }

    // METHODS
    AstVarScope* crate_عارضی(Ast_گرہ_اظہاریہ* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ثابت vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_پچھلا_نام.get(name);
        } ورنہ {
            name = m_پچھلا_نام.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_سطر* ثابت flp = m_scopep->fileline();
        AstVar* ثابت varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->is_اندرونی(true);
        m_scopep->modp()->جوڑو_Stmtsp(varp);
        AstVarScope* ثابت vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->جوڑو_Varsp(vscp);
        m_results.m_vars.ڈالو_عقب(varp);
        واپس vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_گرہ_Stmt* wrapStmtWithNullCheck(File_سطر* flp, Ast_گرہ_Stmt* stmtp,
                                       ثابت Ast_گرہ_اظہاریہ* بنیاد_Refp) {
        اگر (!بنیاد_Refp) واپس stmtp;
        Ast_گرہ_اظہاریہ* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_گرہ_اظہاریہ* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_گرہ_اظہاریہ*>(بنیاد_Refp)->clone_درخت(false), nullp};
        واپس new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_گرہ_اظہاریہ* wrap_اظہاریہ_With_Null_Check(File_سطر* flp, Ast_گرہ_اظہاریہ* exprp,
                                       ثابت Ast_گرہ_اظہاریہ* بنیاد_Refp) {
        اگر (!بنیاد_Refp) واپس exprp;
        Ast_گرہ_اظہاریہ* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_گرہ_اظہاریہ* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_گرہ_اظہاریہ*>(بنیاد_Refp)->clone_درخت(false), nullp};
        Ast_گرہ_اظہاریہ* ثابت falsep = new AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};
        Ast_گرہ_اظہاریہ* ثابت condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtype_سیٹ_ثنائی_ہندسہ();
        واپس condp;
    }

    Ast_گرہ_اظہاریہ* getCurr(Ast_گرہ_اظہاریہ* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        اگر (is_Simple_اظہاریہ(exprp)) واپس exprp->clone_درخت(false);

        // Create the 'current value' variable
        File_سطر* ثابت flp = exprp->fileline();
        خودکار نتیجہ = m_curr.emplace(*exprp, nullptr);
        اگر (نتیجہ.دوسرا) نتیجہ.پہلا->دوسرا = crate_عارضی(exprp);
        AstVarScope* ثابت currp = نتیجہ.پہلا->دوسرا;

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_گرہ_اظہاریہ* ثابت بنیاد_Refp
            = hasNullableMemberAccess(exprp) ? get_بنیاد_Nullable_Ref(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        اگر (m_hasPreUpdate.emplace(*currp).دوسرا) {
            m_results.m_preUpdates.ڈالو_عقب(
                wrapStmtWithNullCheck(flp,
                                      new Ast_تفویض{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_درخت(false)},
                                      بنیاد_Refp));
        }
        واپس new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* get_پچھلا(Ast_گرہ_اظہاریہ* exprp) {
        File_سطر* ثابت flp = exprp->fileline();
        ثابت خودکار rdCurr = [this, exprp]() { واپس getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_گرہ_اظہاریہ* ثابت بنیاد_Refp
            = hasNullableMemberAccess(exprp) ? get_بنیاد_Nullable_Ref(exprp) : nullptr;

        Ast_گرہ* scopeExprp = exprp;
        اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ثابت خودکار pair = m_پچھلا.emplace(*scopeExprp, nullptr);
        اگر (pair.دوسرا) {
            AstVarScope* ثابت prevp = crate_عارضی(exprp);
            pair.پہلا->دوسرا = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            Ast_تفویض* ثابت initp = new Ast_تفویض{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_درخت(false)};
            m_results.m_inits.ڈالو_عقب(wrapStmtWithNullCheck(flp, initp, بنیاد_Refp));
        }

        AstVarScope* ثابت prevp = pair.پہلا->دوسرا;

        ثابت خودکار wr_پچھلا = [=]() { واپس new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        اگر (m_hasPostUpdate.emplace(*exprp).دوسرا) {
            Ast_گرہ_DType* ثابت اظہاریہ_Dtp = exprp->dtypep()->skipRefp();
            اگر (!isSupportedDType(اظہاریہ_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << اظہاریہ_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warn_مزید()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                واپس prevp;
            }
            اگر (VN_IS(اظہاریہ_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ثابت cmhp
                    = new AstCMethodHard{flp, wr_پچھلا(), VCMethod::UNPACKED_تفویض, rdCurr()};
                cmhp->dtype_سیٹ_Void();
                m_results.m_postUpdates.ڈالو_عقب(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), بنیاد_Refp));
            } ورنہ {
                m_results.m_postUpdates.ڈالو_عقب(
                    wrapStmtWithNullCheck(flp, new Ast_تفویض{flp, wr_پچھلا(), rdCurr()}, بنیاد_Refp));
            }
        }

        واپس prevp;
    }

    std::pair<Ast_گرہ_اظہاریہ*, bool> create_رکن(ثابت Ast_Sen_شے* senItemp) {
        File_سطر* ثابت flp = senItemp->fileline();
        Ast_گرہ_اظہاریہ* ثابت senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ثابت Ast_گرہ_اظہاریہ* ثابت بنیاد_Refp
            = hasNullableMemberAccess(senp) ? get_بنیاد_Nullable_Ref(senp) : nullptr;

        ثابت خودکار currp = [this, senp]() { واپس getCurr(senp); };
        ثابت خودکار prevp
            = [this, flp, senp]() { واپس new AstVarRef{flp, get_پچھلا(senp), VAccess::READ}; };
        ثابت خودکار lsb = [=](Ast_گرہ_اظہاریہ* opp) { واپس new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        انتخاب (senItemp->کنارہ_Type()) {
        حالت VEdgeType::ET_بدلا:
        حالت VEdgeType::ET_HYBRID:  //
            اگر (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ثابت resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtype_سیٹ_ثنائی_ہندسہ();
                واپس {wrap_اظہاریہ_With_Null_Check(flp, resultp, بنیاد_Refp), true};
            }
            واپس {wrap_اظہاریہ_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, بنیاد_Refp), true};
        حالت VEdgeType::ET_BOTHEDGE:  //
            واپس {wrap_اظہاریہ_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), بنیاد_Refp),
                    false};
        حالت VEdgeType::ET_POSEDGE:  //
            واپس {wrap_اظہاریہ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), بنیاد_Refp),
                    false};
        حالت VEdgeType::ET_NEGEDGE:  //
            واپس {wrap_اظہاریہ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), بنیاد_Refp),
                    false};
        حالت VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_گرہ_اظہاریہ* ثابت clearFromp = currp();
            اگر (Ast_گرہ_Var_Ref* ثابت refp
                = VN_CAST(clearFromp->get_VAccess_ہدف_Recurse(), گرہ_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ثابت clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtype_سیٹ_Void();
            Ast_گرہ_Stmt* ثابت clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), بنیاد_Refp);
            m_results.m_postUpdates.ڈالو_عقب(clearStmtp);
            m_results.m_destructivePostUpdates.ڈالو_عقب(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ثابت callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtype_سیٹ_ثنائی_ہندسہ();
            Ast_گرہ_اظہاریہ* ثابت firedp = wrap_اظہاریہ_With_Null_Check(flp, callp, بنیاد_Refp);
            اگر (ثابت AstVarRef* ثابت refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                اگر (refp->varScopep() == v3Global.rootp()->nbaEventp()) واپس {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_گرہ_اظہاریہ* ثابت notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            واپس {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        حالت VEdgeType::ET_TRUE:  //
            واپس {currp(), false};
        حالت VEdgeType::ET_INITIAL_NBA:  //
            واپس {new AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}, true};
        طےشدہ:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            واپس {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

عوامی:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_گرہ_اظہاریہ*, bool> build(ثابت Ast_Sen_شے* senItemp) {
        خودکار رکن = create_رکن(senItemp);
        اگر (Ast_گرہ_اظہاریہ* ثابت condp = senItemp->condp()) {
            رکن.پہلا = new AstAnd{senItemp->fileline(), condp->clone_درخت_خالص(false), رکن.پہلا};
        }
        واپس رکن;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_گرہ_اظہاریہ*, bool> build(ثابت Ast_Sen_درخت* senTreep) {
        File_سطر* ثابت flp = senTreep->fileline();
        Ast_گرہ_اظہاریہ* resultp = nullptr;
        bool firedAtInitialization = false;
        برائے (Ast_Sen_شے* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), Sen_شے)) {
            ثابت خودکار& pair = build(senItemp);
            اگر (Ast_گرہ_اظہاریہ* termp = pair.پہلا) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.دوسرا;
            }
        }
        واپس {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        واپس ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_پچھلا.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        واپس std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_اظہاریہ_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#اگر_ختم  // Guard
