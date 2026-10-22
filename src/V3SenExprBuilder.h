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

#اگر_تعریف_نشده VERILATOR_V3SENEXPRBUILDER_H_
#تعریف_کن VERILATOR_V3SENEXPRBUILDER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_عبارت_Builder final {
عام:
    // TYPES
    بنية Results final {
        std::وکتور<Ast_گره_Stmt*> m_inits;  // Initialization statements for previous values
        std::وکتور<Ast_گره_Stmt*> m_preUpdates;  // Pre update assignments
        std::وکتور<Ast_گره_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::وکتور<Ast_گره_Stmt*> m_destructivePostUpdates;
        std::وکتور<AstVar*> m_vars;  // Created temporary variables
    };

خاص:
    // STATE
    AstScope* ثابت m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_گره>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_گره>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_مجموعه<VNRef<Ast_گره>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_مجموعه<VNRef<Ast_گره>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3_Unique_نامها m_prev_نامها{"__Vtrigprevexpr"};  // For generating unique temporary names

    ساكن bool isSupportedDType(Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (VN_IS(dtypep, BasicDType)) أرجع true;
        إذا (VN_IS(dtypep, PackArrayDType)) أرجع true;
        إذا (VN_IS(dtypep, UnpackArrayDType)) أرجع isSupportedDType(dtypep->subDTypep());
        إذا (VN_IS(dtypep, گره_UOr_Struct_DType)) أرجع true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        إذا (VN_IS(dtypep, ClassRefDType)) أرجع true;
        أرجع false;
    }

    ساكن bool is_Simple_عبارت(ثابت Ast_گره* ثابت exprp) {
        أرجع exprp->forall([](ثابت Ast_گره* ثابت nodep) {
            أرجع VN_IS(nodep, Const) || VN_IS(nodep, گره_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, گره_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ساكن bool isNullableRef(ثابت Ast_گره_عبارت* ثابت refp) {
        إذا (!refp->dtypep()) أرجع false;
        ثابت Ast_گره_DType* ثابت dtypep = refp->dtypep()->skipRefp();
        ثابت AstIfaceRefDType* ثابت ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        أرجع VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ساكن bool hasNullableMemberAccess(ثابت Ast_گره* ثابت exprp) {
        أرجع exprp && exprp->exists([](ثابت Ast_گره* ثابت nodep) {
            ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel);
            أرجع mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ساكن ثابت Ast_گره_عبارت* get_پایه_Nullable_Ref(ثابت Ast_گره_عبارت* exprp) {
        طالما (exprp) {
            إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(exprp, MemberSel)) {
                ثابت Ast_گره_عبارت* ثابت fromp = mselp->fromp();
                إذا (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    إذا (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        استمر;
                    }
                    أرجع fromp;
                }
                exprp = fromp;
            } وإلا {
                أرجع nullptr;
            }
        }
        أرجع nullptr;
    }

    // METHODS
    AstVarScope* crate_موقت(Ast_گره_عبارت* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ثابت vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prev_نامها.get(name);
        } وإلا {
            name = m_prev_نامها.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_سطر* ثابت flp = m_scopep->fileline();
        AstVar* ثابت varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->is_داخلی(true);
        m_scopep->modp()->جمع_کن_Stmtsp(varp);
        AstVarScope* ثابت vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->جمع_کن_Varsp(vscp);
        m_results.m_vars.افزودن_پشت(varp);
        أرجع vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_گره_Stmt* wrapStmtWithNullCheck(File_سطر* flp, Ast_گره_Stmt* stmtp,
                                       ثابت Ast_گره_عبارت* پایه_Refp) {
        إذا (!پایه_Refp) أرجع stmtp;
        Ast_گره_عبارت* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_گره_عبارت* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_گره_عبارت*>(پایه_Refp)->clone_درخت(false), nullp};
        أرجع new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_گره_عبارت* wrap_عبارت_With_Null_Check(File_سطر* flp, Ast_گره_عبارت* exprp,
                                       ثابت Ast_گره_عبارت* پایه_Refp) {
        إذا (!پایه_Refp) أرجع exprp;
        Ast_گره_عبارت* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_گره_عبارت* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_گره_عبارت*>(پایه_Refp)->clone_درخت(false), nullp};
        Ast_گره_عبارت* ثابت falsep = new AstConst{flp, AstConst::بیت_False{}};
        Ast_گره_عبارت* ثابت condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtype_مجموعه_بیت();
        أرجع condp;
    }

    Ast_گره_عبارت* getCurr(Ast_گره_عبارت* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        إذا (is_Simple_عبارت(exprp)) أرجع exprp->clone_درخت(false);

        // Create the 'current value' variable
        File_سطر* ثابت flp = exprp->fileline();
        خودکار نتیجه = m_curr.emplace(*exprp, nullptr);
        إذا (نتیجه.دومی) نتیجه.اولی->دومی = crate_موقت(exprp);
        AstVarScope* ثابت currp = نتیجه.اولی->دومی;

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_گره_عبارت* ثابت پایه_Refp
            = hasNullableMemberAccess(exprp) ? get_پایه_Nullable_Ref(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        إذا (m_hasPreUpdate.emplace(*currp).دومی) {
            m_results.m_preUpdates.افزودن_پشت(
                wrapStmtWithNullCheck(flp,
                                      new Ast_تخصیص_بده{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_درخت(false)},
                                      پایه_Refp));
        }
        أرجع new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_گره_عبارت* exprp) {
        File_سطر* ثابت flp = exprp->fileline();
        ثابت خودکار rdCurr = [this, exprp]() { أرجع getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_گره_عبارت* ثابت پایه_Refp
            = hasNullableMemberAccess(exprp) ? get_پایه_Nullable_Ref(exprp) : nullptr;

        Ast_گره* scopeExprp = exprp;
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ثابت خودکار pair = m_prev.emplace(*scopeExprp, nullptr);
        إذا (pair.دومی) {
            AstVarScope* ثابت prevp = crate_موقت(exprp);
            pair.اولی->دومی = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            Ast_تخصیص_بده* ثابت initp = new Ast_تخصیص_بده{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_درخت(false)};
            m_results.m_inits.افزودن_پشت(wrapStmtWithNullCheck(flp, initp, پایه_Refp));
        }

        AstVarScope* ثابت prevp = pair.اولی->دومی;

        ثابت خودکار wrPrev = [=]() { أرجع new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        إذا (m_hasPostUpdate.emplace(*exprp).دومی) {
            Ast_گره_DType* ثابت عبارت_Dtp = exprp->dtypep()->skipRefp();
            إذا (!isSupportedDType(عبارت_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << عبارت_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warn_بیشتر()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                أرجع prevp;
            }
            إذا (VN_IS(عبارت_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ثابت cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_تخصیص_بده, rdCurr()};
                cmhp->dtype_مجموعه_Void();
                m_results.m_postUpdates.افزودن_پشت(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), پایه_Refp));
            } وإلا {
                m_results.m_postUpdates.افزودن_پشت(
                    wrapStmtWithNullCheck(flp, new Ast_تخصیص_بده{flp, wrPrev(), rdCurr()}, پایه_Refp));
            }
        }

        أرجع prevp;
    }

    std::pair<Ast_گره_عبارت*, bool> create_جمله(ثابت Ast_Sen_قلم* senItemp) {
        File_سطر* ثابت flp = senItemp->fileline();
        Ast_گره_عبارت* ثابت senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ثابت Ast_گره_عبارت* ثابت پایه_Refp
            = hasNullableMemberAccess(senp) ? get_پایه_Nullable_Ref(senp) : nullptr;

        ثابت خودکار currp = [this, senp]() { أرجع getCurr(senp); };
        ثابت خودکار prevp
            = [this, flp, senp]() { أرجع new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ثابت خودکار lsb = [=](Ast_گره_عبارت* opp) { أرجع new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        بدل (senItemp->یال_Type()) {
        حالة VEdgeType::ET_تغییریافته:
        حالة VEdgeType::ET_HYBRID:  //
            إذا (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ثابت resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtype_مجموعه_بیت();
                أرجع {wrap_عبارت_With_Null_Check(flp, resultp, پایه_Refp), true};
            }
            أرجع {wrap_عبارت_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, پایه_Refp), true};
        حالة VEdgeType::ET_BOTHEDGE:  //
            أرجع {wrap_عبارت_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), پایه_Refp),
                    false};
        حالة VEdgeType::ET_POSEDGE:  //
            أرجع {wrap_عبارت_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), پایه_Refp),
                    false};
        حالة VEdgeType::ET_NEGEDGE:  //
            أرجع {wrap_عبارت_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), پایه_Refp),
                    false};
        حالة VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_گره_عبارت* ثابت clearFromp = currp();
            إذا (Ast_گره_Var_Ref* ثابت refp
                = VN_CAST(clearFromp->get_VAccess_هدف_Recurse(), گره_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ثابت clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtype_مجموعه_Void();
            Ast_گره_Stmt* ثابت clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), پایه_Refp);
            m_results.m_postUpdates.افزودن_پشت(clearStmtp);
            m_results.m_destructivePostUpdates.افزودن_پشت(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ثابت callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtype_مجموعه_بیت();
            Ast_گره_عبارت* ثابت firedp = wrap_عبارت_With_Null_Check(flp, callp, پایه_Refp);
            إذا (ثابت AstVarRef* ثابت refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                إذا (refp->varScopep() == v3Global.rootp()->nbaEventp()) أرجع {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_گره_عبارت* ثابت notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            أرجع {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        حالة VEdgeType::ET_TRUE:  //
            أرجع {currp(), false};
        حالة VEdgeType::ET_INITIAL_NBA:  //
            أرجع {new AstConst{flp, AstConst::بیت_False{}}, true};
        افتراضي:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            أرجع {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

عام:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_گره_عبارت*, bool> build(ثابت Ast_Sen_قلم* senItemp) {
        خودکار جمله = create_جمله(senItemp);
        إذا (Ast_گره_عبارت* ثابت condp = senItemp->condp()) {
            جمله.اولی = new AstAnd{senItemp->fileline(), condp->clone_درخت_خالص(false), جمله.اولی};
        }
        أرجع جمله;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_گره_عبارت*, bool> build(ثابت Ast_Sen_درخت* senTreep) {
        File_سطر* ثابت flp = senTreep->fileline();
        Ast_گره_عبارت* resultp = nullptr;
        bool firedAtInitialization = false;
        لكل (Ast_Sen_قلم* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), Sen_قلم)) {
            ثابت خودکار& pair = build(senItemp);
            إذا (Ast_گره_عبارت* termp = pair.اولی) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.دومی;
            }
        }
        أرجع {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        أرجع ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        أرجع std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_عبارت_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#پایان_اگر  // Guard
