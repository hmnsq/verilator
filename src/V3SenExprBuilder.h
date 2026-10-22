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

#که_نه_تعریف_شوی VERILATOR_V3SENEXPRBUILDER_H_
#تعریف_کړه VERILATOR_V3SENEXPRBUILDER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_بیان_Builder final {
عام:
    // TYPES
    بنية Results final {
        std::ویکټور<Ast_غوټه_Stmt*> m_inits;  // Initialization statements for previous values
        std::ویکټور<Ast_غوټه_Stmt*> m_preUpdates;  // Pre update assignments
        std::ویکټور<Ast_غوټه_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::ویکټور<Ast_غوټه_Stmt*> m_destructivePostUpdates;
        std::ویکټور<AstVar*> m_vars;  // Created temporary variables
    };

خاص:
    // STATE
    AstScope* ثابت m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_غوټه>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_غوټه>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_سېټ<VNRef<Ast_غوټه>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_سېټ<VNRef<Ast_غوټه>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3_Unique_نومونه m_prev_نومونه{"__Vtrigprevexpr"};  // For generating unique temporary names

    ساكن bool isSupportedDType(Ast_غوټه_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (VN_IS(dtypep, BasicDType)) أرجع true;
        إذا (VN_IS(dtypep, PackArrayDType)) أرجع true;
        إذا (VN_IS(dtypep, UnpackArrayDType)) أرجع isSupportedDType(dtypep->subDTypep());
        إذا (VN_IS(dtypep, غوټه_UOr_Struct_DType)) أرجع true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        إذا (VN_IS(dtypep, ClassRefDType)) أرجع true;
        أرجع false;
    }

    ساكن bool is_Simple_بیان(ثابت Ast_غوټه* ثابت exprp) {
        أرجع exprp->forall([](ثابت Ast_غوټه* ثابت nodep) {
            أرجع VN_IS(nodep, Const) || VN_IS(nodep, غوټه_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, غوټه_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ساكن bool isNullableRef(ثابت Ast_غوټه_بیان* ثابت refp) {
        إذا (!refp->dtypep()) أرجع false;
        ثابت Ast_غوټه_DType* ثابت dtypep = refp->dtypep()->skipRefp();
        ثابت AstIfaceRefDType* ثابت ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        أرجع VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ساكن bool hasNullableMemberAccess(ثابت Ast_غوټه* ثابت exprp) {
        أرجع exprp && exprp->exists([](ثابت Ast_غوټه* ثابت nodep) {
            ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel);
            أرجع mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ساكن ثابت Ast_غوټه_بیان* get_بنسټ_Nullable_Ref(ثابت Ast_غوټه_بیان* exprp) {
        طالما (exprp) {
            إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(exprp, MemberSel)) {
                ثابت Ast_غوټه_بیان* ثابت fromp = mselp->fromp();
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
    AstVarScope* crate_لنډمهاله(Ast_غوټه_بیان* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ثابت vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prev_نومونه.get(name);
        } وإلا {
            name = m_prev_نومونه.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_کرښه* ثابت flp = m_scopep->fileline();
        AstVar* ثابت varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->is_دننی(true);
        m_scopep->modp()->جمع_کړه_Stmtsp(varp);
        AstVarScope* ثابت vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->جمع_کړه_Varsp(vscp);
        m_results.m_vars.ټېل_شا(varp);
        أرجع vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_غوټه_Stmt* wrapStmtWithNullCheck(File_کرښه* flp, Ast_غوټه_Stmt* stmtp,
                                       ثابت Ast_غوټه_بیان* بنسټ_Refp) {
        إذا (!بنسټ_Refp) أرجع stmtp;
        Ast_غوټه_بیان* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_غوټه_بیان* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_غوټه_بیان*>(بنسټ_Refp)->clone_ونه(false), nullp};
        أرجع new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_غوټه_بیان* wrap_بیان_With_Null_Check(File_کرښه* flp, Ast_غوټه_بیان* exprp,
                                       ثابت Ast_غوټه_بیان* بنسټ_Refp) {
        إذا (!بنسټ_Refp) أرجع exprp;
        Ast_غوټه_بیان* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_غوټه_بیان* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_غوټه_بیان*>(بنسټ_Refp)->clone_ونه(false), nullp};
        Ast_غوټه_بیان* ثابت falsep = new AstConst{flp, AstConst::بټ_False{}};
        Ast_غوټه_بیان* ثابت condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtype_سېټ_بټ();
        أرجع condp;
    }

    Ast_غوټه_بیان* getCurr(Ast_غوټه_بیان* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        إذا (is_Simple_بیان(exprp)) أرجع exprp->clone_ونه(false);

        // Create the 'current value' variable
        File_کرښه* ثابت flp = exprp->fileline();
        خپلکاری پایله = m_curr.emplace(*exprp, nullptr);
        إذا (پایله.دوهم) پایله.لومړی->دوهم = crate_لنډمهاله(exprp);
        AstVarScope* ثابت currp = پایله.لومړی->دوهم;

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_غوټه_بیان* ثابت بنسټ_Refp
            = hasNullableMemberAccess(exprp) ? get_بنسټ_Nullable_Ref(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        إذا (m_hasPreUpdate.emplace(*currp).دوهم) {
            m_results.m_preUpdates.ټېل_شا(
                wrapStmtWithNullCheck(flp,
                                      new Ast_ټاکل{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_ونه(false)},
                                      بنسټ_Refp));
        }
        أرجع new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_غوټه_بیان* exprp) {
        File_کرښه* ثابت flp = exprp->fileline();
        ثابت خپلکاری rdCurr = [this, exprp]() { أرجع getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_غوټه_بیان* ثابت بنسټ_Refp
            = hasNullableMemberAccess(exprp) ? get_بنسټ_Nullable_Ref(exprp) : nullptr;

        Ast_غوټه* scopeExprp = exprp;
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ثابت خپلکاری pair = m_prev.emplace(*scopeExprp, nullptr);
        إذا (pair.دوهم) {
            AstVarScope* ثابت prevp = crate_لنډمهاله(exprp);
            pair.لومړی->دوهم = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            Ast_ټاکل* ثابت initp = new Ast_ټاکل{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_ونه(false)};
            m_results.m_inits.ټېل_شا(wrapStmtWithNullCheck(flp, initp, بنسټ_Refp));
        }

        AstVarScope* ثابت prevp = pair.لومړی->دوهم;

        ثابت خپلکاری wrPrev = [=]() { أرجع new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        إذا (m_hasPostUpdate.emplace(*exprp).دوهم) {
            Ast_غوټه_DType* ثابت بیان_Dtp = exprp->dtypep()->skipRefp();
            إذا (!isSupportedDType(بیان_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << بیان_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warn_نور()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                أرجع prevp;
            }
            إذا (VN_IS(بیان_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ثابت cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ټاکل, rdCurr()};
                cmhp->dtype_سېټ_Void();
                m_results.m_postUpdates.ټېل_شا(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), بنسټ_Refp));
            } وإلا {
                m_results.m_postUpdates.ټېل_شا(
                    wrapStmtWithNullCheck(flp, new Ast_ټاکل{flp, wrPrev(), rdCurr()}, بنسټ_Refp));
            }
        }

        أرجع prevp;
    }

    std::pair<Ast_غوټه_بیان*, bool> create_غړی(ثابت Ast_Sen_توکی* senItemp) {
        File_کرښه* ثابت flp = senItemp->fileline();
        Ast_غوټه_بیان* ثابت senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ثابت Ast_غوټه_بیان* ثابت بنسټ_Refp
            = hasNullableMemberAccess(senp) ? get_بنسټ_Nullable_Ref(senp) : nullptr;

        ثابت خپلکاری currp = [this, senp]() { أرجع getCurr(senp); };
        ثابت خپلکاری prevp
            = [this, flp, senp]() { أرجع new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ثابت خپلکاری lsb = [=](Ast_غوټه_بیان* opp) { أرجع new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        بدل (senItemp->څنډه_Type()) {
        حالة VEdgeType::ET_بدل_شوی:
        حالة VEdgeType::ET_HYBRID:  //
            إذا (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ثابت resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtype_سېټ_بټ();
                أرجع {wrap_بیان_With_Null_Check(flp, resultp, بنسټ_Refp), true};
            }
            أرجع {wrap_بیان_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, بنسټ_Refp), true};
        حالة VEdgeType::ET_BOTHEDGE:  //
            أرجع {wrap_بیان_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), بنسټ_Refp),
                    false};
        حالة VEdgeType::ET_POSEDGE:  //
            أرجع {wrap_بیان_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), بنسټ_Refp),
                    false};
        حالة VEdgeType::ET_NEGEDGE:  //
            أرجع {wrap_بیان_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), بنسټ_Refp),
                    false};
        حالة VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_غوټه_بیان* ثابت clearFromp = currp();
            إذا (Ast_غوټه_Var_Ref* ثابت refp
                = VN_CAST(clearFromp->get_VAccess_موخه_Recurse(), غوټه_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ثابت clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtype_سېټ_Void();
            Ast_غوټه_Stmt* ثابت clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), بنسټ_Refp);
            m_results.m_postUpdates.ټېل_شا(clearStmtp);
            m_results.m_destructivePostUpdates.ټېل_شا(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ثابت callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtype_سېټ_بټ();
            Ast_غوټه_بیان* ثابت firedp = wrap_بیان_With_Null_Check(flp, callp, بنسټ_Refp);
            إذا (ثابت AstVarRef* ثابت refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                إذا (refp->varScopep() == v3Global.rootp()->nbaEventp()) أرجع {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_غوټه_بیان* ثابت notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            أرجع {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        حالة VEdgeType::ET_TRUE:  //
            أرجع {currp(), false};
        حالة VEdgeType::ET_INITIAL_NBA:  //
            أرجع {new AstConst{flp, AstConst::بټ_False{}}, true};
        افتراضي:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            أرجع {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

عام:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_غوټه_بیان*, bool> build(ثابت Ast_Sen_توکی* senItemp) {
        خپلکاری غړی = create_غړی(senItemp);
        إذا (Ast_غوټه_بیان* ثابت condp = senItemp->condp()) {
            غړی.لومړی = new AstAnd{senItemp->fileline(), condp->clone_ونه_خالص(false), غړی.لومړی};
        }
        أرجع غړی;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_غوټه_بیان*, bool> build(ثابت Ast_Sen_ونه* senTreep) {
        File_کرښه* ثابت flp = senTreep->fileline();
        Ast_غوټه_بیان* resultp = nullptr;
        bool firedAtInitialization = false;
        لكل (Ast_Sen_توکی* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), Sen_توکی)) {
            ثابت خپلکاری& pair = build(senItemp);
            إذا (Ast_غوټه_بیان* termp = pair.لومړی) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.دوهم;
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
    explicit Sen_بیان_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#د_که_پای  // Guard
