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

#ifndef VERILATOR_V3SENEXPRBUILDER_H_
#define VERILATOR_V3SENEXPRBUILDER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_فرچ_Builder final {
عام:
    // TYPES
    بنية Results final {
        std::vector<Ast_كلي_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_كلي_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_كلي_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_كلي_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

خاص:
    // STATE
    AstScope* ثابت m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_كلي>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_كلي>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_كلي>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_كلي>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    ساكن bool isSupportedDType(Ast_كلي_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (VN_IS(dtypep, BasicDType)) أرجع true;
        إذا (VN_IS(dtypep, PackArrayDType)) أرجع true;
        إذا (VN_IS(dtypep, UnpackArrayDType)) أرجع isSupportedDType(dtypep->subDTypep());
        إذا (VN_IS(dtypep, كلي_UOr_Struct_DType)) أرجع true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        إذا (VN_IS(dtypep, ClassRefDType)) أرجع true;
        أرجع false;
    }

    ساكن bool is_Simple_فرچ(ثابت Ast_كلي* ثابت exprp) {
        أرجع exprp->forall([](ثابت Ast_كلي* ثابت nodep) {
            أرجع VN_IS(nodep, Const) || VN_IS(nodep, كلي_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, كلي_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ساكن bool isNullableRef(ثابت Ast_كلي_فرچ* ثابت refp) {
        إذا (!refp->dtypep()) أرجع false;
        ثابت Ast_كلي_DType* ثابت dtypep = refp->dtypep()->skipRefp();
        ثابت AstIfaceRefDType* ثابت ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        أرجع VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ساكن bool hasNullableMemberAccess(ثابت Ast_كلي* ثابت exprp) {
        أرجع exprp && exprp->exists([](ثابت Ast_كلي* ثابت nodep) {
            ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel);
            أرجع mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ساكن ثابت Ast_كلي_فرچ* getBaseNullableRef(ثابت Ast_كلي_فرچ* exprp) {
        طالما (exprp) {
            إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(exprp, MemberSel)) {
                ثابت Ast_كلي_فرچ* ثابت fromp = mselp->fromp();
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
    AstVarScope* crateTemp(Ast_كلي_فرچ* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ثابت vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } وإلا {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_لي* ثابت flp = m_scopep->fileline();
        AstVar* ثابت varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* ثابت vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.تور_back(varp);
        أرجع vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_كلي_Stmt* wrapStmtWithNullCheck(File_لي* flp, Ast_كلي_Stmt* stmtp,
                                       ثابت Ast_كلي_فرچ* baseRefp) {
        إذا (!baseRefp) أرجع stmtp;
        Ast_كلي_فرچ* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_كلي_فرچ* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_كلي_فرچ*>(baseRefp)->clone_بشيا(false), nullp};
        أرجع new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_كلي_فرچ* wrap_فرچ_With_Null_Check(File_لي* flp, Ast_كلي_فرچ* exprp,
                                       ثابت Ast_كلي_فرچ* baseRefp) {
        إذا (!baseRefp) أرجع exprp;
        Ast_كلي_فرچ* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_كلي_فرچ* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_كلي_فرچ*>(baseRefp)->clone_بشيا(false), nullp};
        Ast_كلي_فرچ* ثابت falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_كلي_فرچ* ثابت condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        أرجع condp;
    }

    Ast_كلي_فرچ* getCurr(Ast_كلي_فرچ* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        إذا (is_Simple_فرچ(exprp)) أرجع exprp->clone_بشيا(false);

        // Create the 'current value' variable
        File_لي* ثابت flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        إذا (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* ثابت currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_كلي_فرچ* ثابت baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        إذا (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.تور_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_بشيا(false)},
                                      baseRefp));
        }
        أرجع new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_كلي_فرچ* exprp) {
        File_لي* ثابت flp = exprp->fileline();
        ثابت auto rdCurr = [this, exprp]() { أرجع getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_كلي_فرچ* ثابت baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_كلي* scopeExprp = exprp;
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ثابت auto pair = m_prev.emplace(*scopeExprp, nullptr);
        إذا (pair.second) {
            AstVarScope* ثابت prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* ثابت initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_بشيا(false)};
            m_results.m_inits.تور_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* ثابت prevp = pair.first->second;

        ثابت auto wrPrev = [=]() { أرجع new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        إذا (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_كلي_DType* ثابت فرچ_Dtp = exprp->dtypep()->skipRefp();
            إذا (!isSupportedDType(فرچ_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << فرچ_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                أرجع prevp;
            }
            إذا (VN_IS(فرچ_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ثابت cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.تور_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } وإلا {
                m_results.m_postUpdates.تور_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        أرجع prevp;
    }

    std::pair<Ast_كلي_فرچ*, bool> createTerm(ثابت AstSenItem* senItemp) {
        File_لي* ثابت flp = senItemp->fileline();
        Ast_كلي_فرچ* ثابت senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ثابت Ast_كلي_فرچ* ثابت baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        ثابت auto currp = [this, senp]() { أرجع getCurr(senp); };
        ثابت auto prevp
            = [this, flp, senp]() { أرجع new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ثابت auto lsb = [=](Ast_كلي_فرچ* opp) { أرجع new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        بدل (senItemp->edgeType()) {
        حالة VEdgeType::ET_CHANGED:
        حالة VEdgeType::ET_HYBRID:  //
            إذا (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ثابت resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                أرجع {wrap_فرچ_With_Null_Check(flp, resultp, baseRefp), true};
            }
            أرجع {wrap_فرچ_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        حالة VEdgeType::ET_BOTHEDGE:  //
            أرجع {wrap_فرچ_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        حالة VEdgeType::ET_POSEDGE:  //
            أرجع {wrap_فرچ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        حالة VEdgeType::ET_NEGEDGE:  //
            أرجع {wrap_فرچ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        حالة VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_كلي_فرچ* ثابت clearFromp = currp();
            إذا (Ast_كلي_Var_Ref* ثابت refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), كلي_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ثابت clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_كلي_Stmt* ثابت clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.تور_back(clearStmtp);
            m_results.m_destructivePostUpdates.تور_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ثابت callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_كلي_فرچ* ثابت firedp = wrap_فرچ_With_Null_Check(flp, callp, baseRefp);
            إذا (ثابت AstVarRef* ثابت refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                إذا (refp->varScopep() == v3Global.rootp()->nbaEventp()) أرجع {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_كلي_فرچ* ثابت notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            أرجع {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        حالة VEdgeType::ET_TRUE:  //
            أرجع {currp(), false};
        حالة VEdgeType::ET_INITIAL_NBA:  //
            أرجع {new AstConst{flp, AstConst::BitFalse{}}, true};
        افتراضي:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            أرجع {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

عام:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_كلي_فرچ*, bool> build(ثابت AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        إذا (Ast_كلي_فرچ* ثابت condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_بشيا_Pure(false), term.first};
        }
        أرجع term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_كلي_فرچ*, bool> build(ثابت Ast_Sen_بشيا* senTreep) {
        File_لي* ثابت flp = senTreep->fileline();
        Ast_كلي_فرچ* resultp = nullptr;
        bool firedAtInitialization = false;
        لكل (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            ثابت auto& pair = build(senItemp);
            إذا (Ast_كلي_فرچ* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
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
    explicit Sen_فرچ_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
