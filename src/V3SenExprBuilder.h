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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_রাশিমালা_Builder final {
সার্বজনিক:
    // TYPES
    সংরচনা Results final {
        std::vector<Ast_গ্রন্থি_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_গ্রন্থি_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_গ্রন্থি_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_গ্রন্থি_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

গুপ্ত:
    // STATE
    AstScope* ধ্রুব m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_গ্রন্থি>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_গ্রন্থি>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_গ্রন্থি>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_গ্রন্থি>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    স্থির bool isSupportedDType(Ast_গ্রন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        যদি (VN_IS(dtypep, BasicDType)) প্রত্যাগম true;
        যদি (VN_IS(dtypep, PackArrayDType)) প্রত্যাগম true;
        যদি (VN_IS(dtypep, UnpackArrayDType)) প্রত্যাগম isSupportedDType(dtypep->subDTypep());
        যদি (VN_IS(dtypep, গ্রন্থি_UOr_Struct_DType)) প্রত্যাগম true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        যদি (VN_IS(dtypep, ClassRefDType)) প্রত্যাগম true;
        প্রত্যাগম false;
    }

    স্থির bool is_Simple_রাশিমালা(ধ্রুব Ast_গ্রন্থি* ধ্রুব exprp) {
        প্রত্যাগম exprp->forall([](ধ্রুব Ast_গ্রন্থি* ধ্রুব nodep) {
            প্রত্যাগম VN_IS(nodep, Const) || VN_IS(nodep, গ্রন্থি_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, গ্রন্থি_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    স্থির bool isNullableRef(ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব refp) {
        যদি (!refp->dtypep()) প্রত্যাগম false;
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = refp->dtypep()->skipRefp();
        ধ্রুব AstIfaceRefDType* ধ্রুব ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        প্রত্যাগম VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    স্থির bool hasNullableMemberAccess(ধ্রুব Ast_গ্রন্থি* ধ্রুব exprp) {
        প্রত্যাগম exprp && exprp->exists([](ধ্রুব Ast_গ্রন্থি* ধ্রুব nodep) {
            ধ্রুব AstMemberSel* ধ্রুব mselp = VN_CAST(nodep, MemberSel);
            প্রত্যাগম mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    স্থির ধ্রুব Ast_গ্রন্থি_রাশিমালা* getBaseNullableRef(ধ্রুব Ast_গ্রন্থি_রাশিমালা* exprp) {
        যাবত্ (exprp) {
            যদি (ধ্রুব AstMemberSel* ধ্রুব mselp = VN_CAST(exprp, MemberSel)) {
                ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব fromp = mselp->fromp();
                যদি (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    যদি (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        অনুবর্ত;
                    }
                    প্রত্যাগম fromp;
                }
                exprp = fromp;
            } অন্যথা {
                প্রত্যাগম nullptr;
            }
        }
        প্রত্যাগম nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_গ্রন্থি_রাশিমালা* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        যদি (AstVarRef* ধ্রুব refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ধ্রুব vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } অন্যথা {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_সারি* ধ্রুব flp = m_scopep->fileline();
        AstVar* ধ্রুব varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* ধ্রুব vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.ঠেলো_back(varp);
        প্রত্যাগম vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_গ্রন্থি_Stmt* wrapStmtWithNullCheck(File_সারি* flp, Ast_গ্রন্থি_Stmt* stmtp,
                                       ধ্রুব Ast_গ্রন্থি_রাশিমালা* baseRefp) {
        যদি (!baseRefp) প্রত্যাগম stmtp;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব checkp
            = new AstNeq{flp, const_cast<Ast_গ্রন্থি_রাশিমালা*>(baseRefp)->clone_বৃক্ষ(false), nullp};
        প্রত্যাগম new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_গ্রন্থি_রাশিমালা* wrap_রাশিমালা_With_Null_Check(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* exprp,
                                       ধ্রুব Ast_গ্রন্থি_রাশিমালা* baseRefp) {
        যদি (!baseRefp) প্রত্যাগম exprp;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব checkp
            = new AstNeq{flp, const_cast<Ast_গ্রন্থি_রাশিমালা*>(baseRefp)->clone_বৃক্ষ(false), nullp};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        প্রত্যাগম condp;
    }

    Ast_গ্রন্থি_রাশিমালা* getCurr(Ast_গ্রন্থি_রাশিমালা* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        যদি (is_Simple_রাশিমালা(exprp)) প্রত্যাগম exprp->clone_বৃক্ষ(false);

        // Create the 'current value' variable
        File_সারি* ধ্রুব flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        যদি (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* ধ্রুব currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        যদি (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.ঠেলো_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_বৃক্ষ(false)},
                                      baseRefp));
        }
        প্রত্যাগম new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_গ্রন্থি_রাশিমালা* exprp) {
        File_সারি* ধ্রুব flp = exprp->fileline();
        ধ্রুব auto rdCurr = [this, exprp]() { প্রত্যাগম getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_গ্রন্থি* scopeExprp = exprp;
        যদি (AstVarRef* ধ্রুব refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ধ্রুব auto pair = m_prev.emplace(*scopeExprp, nullptr);
        যদি (pair.second) {
            AstVarScope* ধ্রুব prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* ধ্রুব initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_বৃক্ষ(false)};
            m_results.m_inits.ঠেলো_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* ধ্রুব prevp = pair.first->second;

        ধ্রুব auto wrPrev = [=]() { প্রত্যাগম new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        যদি (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_গ্রন্থি_DType* ধ্রুব রাশিমালা_Dtp = exprp->dtypep()->skipRefp();
            যদি (!isSupportedDType(রাশিমালা_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << রাশিমালা_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                প্রত্যাগম prevp;
            }
            যদি (VN_IS(রাশিমালা_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ধ্রুব cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.ঠেলো_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } অন্যথা {
                m_results.m_postUpdates.ঠেলো_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        প্রত্যাগম prevp;
    }

    std::pair<Ast_গ্রন্থি_রাশিমালা*, bool> createTerm(ধ্রুব AstSenItem* senItemp) {
        File_সারি* ধ্রুব flp = senItemp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        ধ্রুব auto currp = [this, senp]() { প্রত্যাগম getCurr(senp); };
        ধ্রুব auto prevp
            = [this, flp, senp]() { প্রত্যাগম new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ধ্রুব auto lsb = [=](Ast_গ্রন্থি_রাশিমালা* opp) { প্রত্যাগম new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        বিকল্প (senItemp->edgeType()) {
        পক্ষ VEdgeType::ET_CHANGED:
        পক্ষ VEdgeType::ET_HYBRID:  //
            যদি (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ধ্রুব resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                প্রত্যাগম {wrap_রাশিমালা_With_Null_Check(flp, resultp, baseRefp), true};
            }
            প্রত্যাগম {wrap_রাশিমালা_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        পক্ষ VEdgeType::ET_BOTHEDGE:  //
            প্রত্যাগম {wrap_রাশিমালা_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        পক্ষ VEdgeType::ET_POSEDGE:  //
            প্রত্যাগম {wrap_রাশিমালা_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        পক্ষ VEdgeType::ET_NEGEDGE:  //
            প্রত্যাগম {wrap_রাশিমালা_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        পক্ষ VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব clearFromp = currp();
            যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুব refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), গ্রন্থি_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ধ্রুব clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_গ্রন্থি_Stmt* ধ্রুব clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.ঠেলো_back(clearStmtp);
            m_results.m_destructivePostUpdates.ঠেলো_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ধ্রুব callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব firedp = wrap_রাশিমালা_With_Null_Check(flp, callp, baseRefp);
            যদি (ধ্রুব AstVarRef* ধ্রুব refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                যদি (refp->varScopep() == v3Global.rootp()->nbaEventp()) প্রত্যাগম {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            প্রত্যাগম {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        পক্ষ VEdgeType::ET_TRUE:  //
            প্রত্যাগম {currp(), false};
        পক্ষ VEdgeType::ET_INITIAL_NBA:  //
            প্রত্যাগম {new AstConst{flp, AstConst::BitFalse{}}, true};
        স্বভাব:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            প্রত্যাগম {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

সার্বজনিক:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_গ্রন্থি_রাশিমালা*, bool> build(ধ্রুব AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_বৃক্ষ_Pure(false), term.first};
        }
        প্রত্যাগম term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_গ্রন্থি_রাশিমালা*, bool> build(ধ্রুব Ast_Sen_বৃক্ষ* senTreep) {
        File_সারি* ধ্রুব flp = senTreep->fileline();
        Ast_গ্রন্থি_রাশিমালা* resultp = nullptr;
        bool firedAtInitialization = false;
        প্রতি (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            ধ্রুব auto& pair = build(senItemp);
            যদি (Ast_গ্রন্থি_রাশিমালা* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        প্রত্যাগম {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        প্রত্যাগম ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        প্রত্যাগম std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_রাশিমালা_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
