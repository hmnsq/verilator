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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3SENEXPRBUILDER_H_
#وصف_ڪريو VERILATOR_V3SENEXPRBUILDER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_اظهار_Builder final {
عام:
    // TYPES
    بنية Results final {
        std::ويڪٽر<Ast_ڳنڍ_Stmt*> m_inits;  // Initialization statements for previous values
        std::ويڪٽر<Ast_ڳنڍ_Stmt*> m_preUpdates;  // Pre update assignments
        std::ويڪٽر<Ast_ڳنڍ_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::ويڪٽر<Ast_ڳنڍ_Stmt*> m_destructivePostUpdates;
        std::ويڪٽر<AstVar*> m_vars;  // Created temporary variables
    };

خاص:
    // STATE
    AstScope* ثابت m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_ڳنڍ>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_ڳنڍ>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_سيٽ<VNRef<Ast_ڳنڍ>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_سيٽ<VNRef<Ast_ڳنڍ>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3_Unique_نالا m_prev_نالا{"__Vtrigprevexpr"};  // For generating unique temporary names

    ساكن bool isSupportedDType(Ast_ڳنڍ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (VN_IS(dtypep, BasicDType)) أرجع true;
        إذا (VN_IS(dtypep, PackArrayDType)) أرجع true;
        إذا (VN_IS(dtypep, UnpackArrayDType)) أرجع isSupportedDType(dtypep->subDTypep());
        إذا (VN_IS(dtypep, ڳنڍ_UOr_Struct_DType)) أرجع true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        إذا (VN_IS(dtypep, ClassRefDType)) أرجع true;
        أرجع false;
    }

    ساكن bool is_Simple_اظهار(ثابت Ast_ڳنڍ* ثابت exprp) {
        أرجع exprp->forall([](ثابت Ast_ڳنڍ* ثابت nodep) {
            أرجع VN_IS(nodep, Const) || VN_IS(nodep, ڳنڍ_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, ڳنڍ_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ساكن bool isNullableRef(ثابت Ast_ڳنڍ_اظهار* ثابت refp) {
        إذا (!refp->dtypep()) أرجع false;
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = refp->dtypep()->skipRefp();
        ثابت AstIfaceRefDType* ثابت ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        أرجع VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ساكن bool hasNullableMemberAccess(ثابت Ast_ڳنڍ* ثابت exprp) {
        أرجع exprp && exprp->exists([](ثابت Ast_ڳنڍ* ثابت nodep) {
            ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel);
            أرجع mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ساكن ثابت Ast_ڳنڍ_اظهار* get_بنياد_Nullable_Ref(ثابت Ast_ڳنڍ_اظهار* exprp) {
        طالما (exprp) {
            إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(exprp, MemberSel)) {
                ثابت Ast_ڳنڍ_اظهار* ثابت fromp = mselp->fromp();
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
    AstVarScope* crate_عارضي(Ast_ڳنڍ_اظهار* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ثابت vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prev_نالا.get(name);
        } وإلا {
            name = m_prev_نالا.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_سٽ* ثابت flp = m_scopep->fileline();
        AstVar* ثابت varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->is_اندروني(true);
        m_scopep->modp()->جوڙيو_Stmtsp(varp);
        AstVarScope* ثابت vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->جوڙيو_Varsp(vscp);
        m_results.m_vars.ڌڪو_پٺ(varp);
        أرجع vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_ڳنڍ_Stmt* wrapStmtWithNullCheck(File_سٽ* flp, Ast_ڳنڍ_Stmt* stmtp,
                                       ثابت Ast_ڳنڍ_اظهار* بنياد_Refp) {
        إذا (!بنياد_Refp) أرجع stmtp;
        Ast_ڳنڍ_اظهار* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ڳنڍ_اظهار* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_ڳنڍ_اظهار*>(بنياد_Refp)->clone_وڻ(false), nullp};
        أرجع new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_ڳنڍ_اظهار* wrap_اظهار_With_Null_Check(File_سٽ* flp, Ast_ڳنڍ_اظهار* exprp,
                                       ثابت Ast_ڳنڍ_اظهار* بنياد_Refp) {
        إذا (!بنياد_Refp) أرجع exprp;
        Ast_ڳنڍ_اظهار* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ڳنڍ_اظهار* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_ڳنڍ_اظهار*>(بنياد_Refp)->clone_وڻ(false), nullp};
        Ast_ڳنڍ_اظهار* ثابت falsep = new AstConst{flp, AstConst::ٻٽو_انگ_False{}};
        Ast_ڳنڍ_اظهار* ثابت condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtype_سيٽ_ٻٽو_انگ();
        أرجع condp;
    }

    Ast_ڳنڍ_اظهار* getCurr(Ast_ڳنڍ_اظهار* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        إذا (is_Simple_اظهار(exprp)) أرجع exprp->clone_وڻ(false);

        // Create the 'current value' variable
        File_سٽ* ثابت flp = exprp->fileline();
        خودڪار نتيجو = m_curr.emplace(*exprp, nullptr);
        إذا (نتيجو.ٻيو) نتيجو.پهريون->ٻيو = crate_عارضي(exprp);
        AstVarScope* ثابت currp = نتيجو.پهريون->ٻيو;

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_ڳنڍ_اظهار* ثابت بنياد_Refp
            = hasNullableMemberAccess(exprp) ? get_بنياد_Nullable_Ref(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        إذا (m_hasPreUpdate.emplace(*currp).ٻيو) {
            m_results.m_preUpdates.ڌڪو_پٺ(
                wrapStmtWithNullCheck(flp,
                                      new Ast_سونپيو{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_وڻ(false)},
                                      بنياد_Refp));
        }
        أرجع new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_ڳنڍ_اظهار* exprp) {
        File_سٽ* ثابت flp = exprp->fileline();
        ثابت خودڪار rdCurr = [this, exprp]() { أرجع getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_ڳنڍ_اظهار* ثابت بنياد_Refp
            = hasNullableMemberAccess(exprp) ? get_بنياد_Nullable_Ref(exprp) : nullptr;

        Ast_ڳنڍ* scopeExprp = exprp;
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ثابت خودڪار pair = m_prev.emplace(*scopeExprp, nullptr);
        إذا (pair.ٻيو) {
            AstVarScope* ثابت prevp = crate_عارضي(exprp);
            pair.پهريون->ٻيو = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            Ast_سونپيو* ثابت initp = new Ast_سونپيو{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_وڻ(false)};
            m_results.m_inits.ڌڪو_پٺ(wrapStmtWithNullCheck(flp, initp, بنياد_Refp));
        }

        AstVarScope* ثابت prevp = pair.پهريون->ٻيو;

        ثابت خودڪار wrPrev = [=]() { أرجع new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        إذا (m_hasPostUpdate.emplace(*exprp).ٻيو) {
            Ast_ڳنڍ_DType* ثابت اظهار_Dtp = exprp->dtypep()->skipRefp();
            إذا (!isSupportedDType(اظهار_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << اظهار_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warn_وڌيڪ()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                أرجع prevp;
            }
            إذا (VN_IS(اظهار_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ثابت cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_سونپيو, rdCurr()};
                cmhp->dtype_سيٽ_Void();
                m_results.m_postUpdates.ڌڪو_پٺ(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), بنياد_Refp));
            } وإلا {
                m_results.m_postUpdates.ڌڪو_پٺ(
                    wrapStmtWithNullCheck(flp, new Ast_سونپيو{flp, wrPrev(), rdCurr()}, بنياد_Refp));
            }
        }

        أرجع prevp;
    }

    std::pair<Ast_ڳنڍ_اظهار*, bool> create_رڪن(ثابت Ast_Sen_شي* senItemp) {
        File_سٽ* ثابت flp = senItemp->fileline();
        Ast_ڳنڍ_اظهار* ثابت senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ثابت Ast_ڳنڍ_اظهار* ثابت بنياد_Refp
            = hasNullableMemberAccess(senp) ? get_بنياد_Nullable_Ref(senp) : nullptr;

        ثابت خودڪار currp = [this, senp]() { أرجع getCurr(senp); };
        ثابت خودڪار prevp
            = [this, flp, senp]() { أرجع new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ثابت خودڪار lsb = [=](Ast_ڳنڍ_اظهار* opp) { أرجع new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        بدل (senItemp->ڪنارو_Type()) {
        حالة VEdgeType::ET_بدليو:
        حالة VEdgeType::ET_HYBRID:  //
            إذا (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ثابت resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtype_سيٽ_ٻٽو_انگ();
                أرجع {wrap_اظهار_With_Null_Check(flp, resultp, بنياد_Refp), true};
            }
            أرجع {wrap_اظهار_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, بنياد_Refp), true};
        حالة VEdgeType::ET_BOTHEDGE:  //
            أرجع {wrap_اظهار_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), بنياد_Refp),
                    false};
        حالة VEdgeType::ET_POSEDGE:  //
            أرجع {wrap_اظهار_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), بنياد_Refp),
                    false};
        حالة VEdgeType::ET_NEGEDGE:  //
            أرجع {wrap_اظهار_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), بنياد_Refp),
                    false};
        حالة VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_ڳنڍ_اظهار* ثابت clearFromp = currp();
            إذا (Ast_ڳنڍ_Var_Ref* ثابت refp
                = VN_CAST(clearFromp->get_VAccess_هدف_Recurse(), ڳنڍ_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ثابت clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtype_سيٽ_Void();
            Ast_ڳنڍ_Stmt* ثابت clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), بنياد_Refp);
            m_results.m_postUpdates.ڌڪو_پٺ(clearStmtp);
            m_results.m_destructivePostUpdates.ڌڪو_پٺ(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ثابت callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtype_سيٽ_ٻٽو_انگ();
            Ast_ڳنڍ_اظهار* ثابت firedp = wrap_اظهار_With_Null_Check(flp, callp, بنياد_Refp);
            إذا (ثابت AstVarRef* ثابت refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                إذا (refp->varScopep() == v3Global.rootp()->nbaEventp()) أرجع {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_ڳنڍ_اظهار* ثابت notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            أرجع {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        حالة VEdgeType::ET_TRUE:  //
            أرجع {currp(), false};
        حالة VEdgeType::ET_INITIAL_NBA:  //
            أرجع {new AstConst{flp, AstConst::ٻٽو_انگ_False{}}, true};
        افتراضي:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            أرجع {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

عام:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_ڳنڍ_اظهار*, bool> build(ثابت Ast_Sen_شي* senItemp) {
        خودڪار رڪن = create_رڪن(senItemp);
        إذا (Ast_ڳنڍ_اظهار* ثابت condp = senItemp->condp()) {
            رڪن.پهريون = new AstAnd{senItemp->fileline(), condp->clone_وڻ_خالص(false), رڪن.پهريون};
        }
        أرجع رڪن;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_ڳنڍ_اظهار*, bool> build(ثابت Ast_Sen_وڻ* senTreep) {
        File_سٽ* ثابت flp = senTreep->fileline();
        Ast_ڳنڍ_اظهار* resultp = nullptr;
        bool firedAtInitialization = false;
        لكل (Ast_Sen_شي* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), Sen_شي)) {
            ثابت خودڪار& pair = build(senItemp);
            إذا (Ast_ڳنڍ_اظهار* termp = pair.پهريون) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.ٻيو;
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
    explicit Sen_اظهار_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#جيڪڏهن_پورو  // Guard
