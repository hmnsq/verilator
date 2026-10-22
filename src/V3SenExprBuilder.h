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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_عبارت_Builder final {
عمومی:
    // TYPES
    ساختار Results final {
        std::vector<Ast_گره_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_گره_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_گره_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_گره_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

خصوصی:
    // STATE
    AstScope* ثابت m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_گره>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_گره>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_گره>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_گره>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    ایستا bool isSupportedDType(Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (VN_IS(dtypep, BasicDType)) بازگشت true;
        اگر (VN_IS(dtypep, PackArrayDType)) بازگشت true;
        اگر (VN_IS(dtypep, UnpackArrayDType)) بازگشت isSupportedDType(dtypep->subDTypep());
        اگر (VN_IS(dtypep, گره_UOr_Struct_DType)) بازگشت true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        اگر (VN_IS(dtypep, ClassRefDType)) بازگشت true;
        بازگشت false;
    }

    ایستا bool is_Simple_عبارت(ثابت Ast_گره* ثابت exprp) {
        بازگشت exprp->forall([](ثابت Ast_گره* ثابت nodep) {
            بازگشت VN_IS(nodep, Const) || VN_IS(nodep, گره_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, گره_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ایستا bool isNullableRef(ثابت Ast_گره_عبارت* ثابت refp) {
        اگر (!refp->dtypep()) بازگشت false;
        ثابت Ast_گره_DType* ثابت dtypep = refp->dtypep()->skipRefp();
        ثابت AstIfaceRefDType* ثابت ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        بازگشت VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ایستا bool hasNullableMemberAccess(ثابت Ast_گره* ثابت exprp) {
        بازگشت exprp && exprp->exists([](ثابت Ast_گره* ثابت nodep) {
            ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel);
            بازگشت mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ایستا ثابت Ast_گره_عبارت* getBaseNullableRef(ثابت Ast_گره_عبارت* exprp) {
        وقتی (exprp) {
            اگر (ثابت AstMemberSel* ثابت mselp = VN_CAST(exprp, MemberSel)) {
                ثابت Ast_گره_عبارت* ثابت fromp = mselp->fromp();
                اگر (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    اگر (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        ادامه;
                    }
                    بازگشت fromp;
                }
                exprp = fromp;
            } وگرنه {
                بازگشت nullptr;
            }
        }
        بازگشت nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_گره_عبارت* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ثابت vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } وگرنه {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_سطر* ثابت flp = m_scopep->fileline();
        AstVar* ثابت varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* ثابت vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.افزودن_back(varp);
        بازگشت vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_گره_Stmt* wrapStmtWithNullCheck(File_سطر* flp, Ast_گره_Stmt* stmtp,
                                       ثابت Ast_گره_عبارت* baseRefp) {
        اگر (!baseRefp) بازگشت stmtp;
        Ast_گره_عبارت* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_گره_عبارت* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_گره_عبارت*>(baseRefp)->clone_درخت(false), nullp};
        بازگشت new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_گره_عبارت* wrap_عبارت_With_Null_Check(File_سطر* flp, Ast_گره_عبارت* exprp,
                                       ثابت Ast_گره_عبارت* baseRefp) {
        اگر (!baseRefp) بازگشت exprp;
        Ast_گره_عبارت* ثابت nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_گره_عبارت* ثابت checkp
            = new AstNeq{flp, const_cast<Ast_گره_عبارت*>(baseRefp)->clone_درخت(false), nullp};
        Ast_گره_عبارت* ثابت falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_گره_عبارت* ثابت condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        بازگشت condp;
    }

    Ast_گره_عبارت* getCurr(Ast_گره_عبارت* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        اگر (is_Simple_عبارت(exprp)) بازگشت exprp->clone_درخت(false);

        // Create the 'current value' variable
        File_سطر* ثابت flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        اگر (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* ثابت currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_گره_عبارت* ثابت baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        اگر (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.افزودن_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_درخت(false)},
                                      baseRefp));
        }
        بازگشت new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_گره_عبارت* exprp) {
        File_سطر* ثابت flp = exprp->fileline();
        ثابت auto rdCurr = [this, exprp]() { بازگشت getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ثابت Ast_گره_عبارت* ثابت baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_گره* scopeExprp = exprp;
        اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ثابت auto pair = m_prev.emplace(*scopeExprp, nullptr);
        اگر (pair.second) {
            AstVarScope* ثابت prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* ثابت initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_درخت(false)};
            m_results.m_inits.افزودن_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* ثابت prevp = pair.first->second;

        ثابت auto wrPrev = [=]() { بازگشت new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        اگر (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_گره_DType* ثابت عبارت_Dtp = exprp->dtypep()->skipRefp();
            اگر (!isSupportedDType(عبارت_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << عبارت_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                بازگشت prevp;
            }
            اگر (VN_IS(عبارت_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ثابت cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.افزودن_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } وگرنه {
                m_results.m_postUpdates.افزودن_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        بازگشت prevp;
    }

    std::pair<Ast_گره_عبارت*, bool> createTerm(ثابت AstSenItem* senItemp) {
        File_سطر* ثابت flp = senItemp->fileline();
        Ast_گره_عبارت* ثابت senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ثابت Ast_گره_عبارت* ثابت baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        ثابت auto currp = [this, senp]() { بازگشت getCurr(senp); };
        ثابت auto prevp
            = [this, flp, senp]() { بازگشت new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ثابت auto lsb = [=](Ast_گره_عبارت* opp) { بازگشت new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        انتخاب (senItemp->edgeType()) {
        حالت VEdgeType::ET_CHANGED:
        حالت VEdgeType::ET_HYBRID:  //
            اگر (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ثابت resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                بازگشت {wrap_عبارت_With_Null_Check(flp, resultp, baseRefp), true};
            }
            بازگشت {wrap_عبارت_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        حالت VEdgeType::ET_BOTHEDGE:  //
            بازگشت {wrap_عبارت_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        حالت VEdgeType::ET_POSEDGE:  //
            بازگشت {wrap_عبارت_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        حالت VEdgeType::ET_NEGEDGE:  //
            بازگشت {wrap_عبارت_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        حالت VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_گره_عبارت* ثابت clearFromp = currp();
            اگر (Ast_گره_Var_Ref* ثابت refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), گره_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ثابت clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_گره_Stmt* ثابت clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.افزودن_back(clearStmtp);
            m_results.m_destructivePostUpdates.افزودن_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ثابت callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_گره_عبارت* ثابت firedp = wrap_عبارت_With_Null_Check(flp, callp, baseRefp);
            اگر (ثابت AstVarRef* ثابت refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                اگر (refp->varScopep() == v3Global.rootp()->nbaEventp()) بازگشت {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_گره_عبارت* ثابت notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            بازگشت {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        حالت VEdgeType::ET_TRUE:  //
            بازگشت {currp(), false};
        حالت VEdgeType::ET_INITIAL_NBA:  //
            بازگشت {new AstConst{flp, AstConst::BitFalse{}}, true};
        پیشفرض:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            بازگشت {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

عمومی:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_گره_عبارت*, bool> build(ثابت AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        اگر (Ast_گره_عبارت* ثابت condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_درخت_Pure(false), term.first};
        }
        بازگشت term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_گره_عبارت*, bool> build(ثابت Ast_Sen_درخت* senTreep) {
        File_سطر* ثابت flp = senTreep->fileline();
        Ast_گره_عبارت* resultp = nullptr;
        bool firedAtInitialization = false;
        برای (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            ثابت auto& pair = build(senItemp);
            اگر (Ast_گره_عبارت* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        بازگشت {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        بازگشت ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        بازگشت std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_عبارت_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
