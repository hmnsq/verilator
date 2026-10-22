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
সর্বজনীন:
    // TYPES
    কাঠামো Results final {
        std::vector<Ast_গ্রন্থি_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_গ্রন্থি_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_গ্রন্থি_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_গ্রন্থি_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

ব্যক্তিগত:
    // STATE
    AstScope* ধ্রুবক m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_গ্রন্থি>, AstVarScope*> m_পূর্ব;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_গ্রন্থি>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_গ্রন্থি>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_গ্রন্থি>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_পূর্ব_Names{"__Vtrigprevexpr"};  // For generating unique temporary names

    স্থির bool isSupportedDType(Ast_গ্রন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        যদি (VN_IS(dtypep, BasicDType)) ফেরত true;
        যদি (VN_IS(dtypep, PackArrayDType)) ফেরত true;
        যদি (VN_IS(dtypep, UnpackArrayDType)) ফেরত isSupportedDType(dtypep->subDTypep());
        যদি (VN_IS(dtypep, গ্রন্থি_UOr_Struct_DType)) ফেরত true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        যদি (VN_IS(dtypep, ClassRefDType)) ফেরত true;
        ফেরত false;
    }

    স্থির bool is_Simple_রাশিমালা(ধ্রুবক Ast_গ্রন্থি* ধ্রুবক exprp) {
        ফেরত exprp->forall([](ধ্রুবক Ast_গ্রন্থি* ধ্রুবক nodep) {
            ফেরত VN_IS(nodep, Const) || VN_IS(nodep, গ্রন্থি_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, গ্রন্থি_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    স্থির bool isNullableRef(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক refp) {
        যদি (!refp->dtypep()) ফেরত false;
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = refp->dtypep()->skipRefp();
        ধ্রুবক AstIfaceRefDType* ধ্রুবক ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        ফেরত VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    স্থির bool hasNullableMemberAccess(ধ্রুবক Ast_গ্রন্থি* ধ্রুবক exprp) {
        ফেরত exprp && exprp->exists([](ধ্রুবক Ast_গ্রন্থি* ধ্রুবক nodep) {
            ধ্রুবক AstMemberSel* ধ্রুবক mselp = VN_CAST(nodep, MemberSel);
            ফেরত mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    স্থির ধ্রুবক Ast_গ্রন্থি_রাশিমালা* getBaseNullableRef(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* exprp) {
        যখন (exprp) {
            যদি (ধ্রুবক AstMemberSel* ধ্রুবক mselp = VN_CAST(exprp, MemberSel)) {
                ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = mselp->fromp();
                যদি (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    যদি (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        চালু;
                    }
                    ফেরত fromp;
                }
                exprp = fromp;
            } অন্যথা {
                ফেরত nullptr;
            }
        }
        ফেরত nullptr;
    }

    // METHODS
    AstVarScope* crate_অস্থায়ী(Ast_গ্রন্থি_রাশিমালা* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        যদি (AstVarRef* ধ্রুবক refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ধ্রুবক vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_পূর্ব_Names.get(name);
        } অন্যথা {
            name = m_পূর্ব_Names.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_সারি* ধ্রুবক flp = m_scopep->fileline();
        AstVar* ধ্রুবক varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* ধ্রুবক vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.ঠেলো_back(varp);
        ফেরত vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_গ্রন্থি_Stmt* wrapStmtWithNullCheck(File_সারি* flp, Ast_গ্রন্থি_Stmt* stmtp,
                                       ধ্রুবক Ast_গ্রন্থি_রাশিমালা* baseRefp) {
        যদি (!baseRefp) ফেরত stmtp;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক checkp
            = new AstNeq{flp, const_cast<Ast_গ্রন্থি_রাশিমালা*>(baseRefp)->clone_বৃক্ষ(false), nullp};
        ফেরত new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_গ্রন্থি_রাশিমালা* wrap_রাশিমালা_With_Null_Check(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* exprp,
                                       ধ্রুবক Ast_গ্রন্থি_রাশিমালা* baseRefp) {
        যদি (!baseRefp) ফেরত exprp;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক checkp
            = new AstNeq{flp, const_cast<Ast_গ্রন্থি_রাশিমালা*>(baseRefp)->clone_বৃক্ষ(false), nullp};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        ফেরত condp;
    }

    Ast_গ্রন্থি_রাশিমালা* getCurr(Ast_গ্রন্থি_রাশিমালা* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        যদি (is_Simple_রাশিমালা(exprp)) ফেরত exprp->clone_বৃক্ষ(false);

        // Create the 'current value' variable
        File_সারি* ধ্রুবক flp = exprp->fileline();
        auto ফলাফল = m_curr.emplace(*exprp, nullptr);
        যদি (ফলাফল.second) ফলাফল.first->second = crate_অস্থায়ী(exprp);
        AstVarScope* ধ্রুবক currp = ফলাফল.first->second;

        // Check if we need null guards for a member access through a nullable reference
        ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        যদি (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.ঠেলো_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_বৃক্ষ(false)},
                                      baseRefp));
        }
        ফেরত new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* get_পূর্ব(Ast_গ্রন্থি_রাশিমালা* exprp) {
        File_সারি* ধ্রুবক flp = exprp->fileline();
        ধ্রুবক auto rdCurr = [this, exprp]() { ফেরত getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_গ্রন্থি* scopeExprp = exprp;
        যদি (AstVarRef* ধ্রুবক refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ধ্রুবক auto pair = m_পূর্ব.emplace(*scopeExprp, nullptr);
        যদি (pair.second) {
            AstVarScope* ধ্রুবক prevp = crate_অস্থায়ী(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* ধ্রুবক initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_বৃক্ষ(false)};
            m_results.m_inits.ঠেলো_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* ধ্রুবক prevp = pair.first->second;

        ধ্রুবক auto wr_পূর্ব = [=]() { ফেরত new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        যদি (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_গ্রন্থি_DType* ধ্রুবক রাশিমালা_Dtp = exprp->dtypep()->skipRefp();
            যদি (!isSupportedDType(রাশিমালা_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << রাশিমালা_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                ফেরত prevp;
            }
            যদি (VN_IS(রাশিমালা_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ধ্রুবক cmhp
                    = new AstCMethodHard{flp, wr_পূর্ব(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.ঠেলো_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } অন্যথা {
                m_results.m_postUpdates.ঠেলো_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wr_পূর্ব(), rdCurr()}, baseRefp));
            }
        }

        ফেরত prevp;
    }

    std::pair<Ast_গ্রন্থি_রাশিমালা*, bool> createTerm(ধ্রুবক AstSenItem* senItemp) {
        File_সারি* ধ্রুবক flp = senItemp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        ধ্রুবক auto currp = [this, senp]() { ফেরত getCurr(senp); };
        ধ্রুবক auto prevp
            = [this, flp, senp]() { ফেরত new AstVarRef{flp, get_পূর্ব(senp), VAccess::READ}; };
        ধ্রুবক auto lsb = [=](Ast_গ্রন্থি_রাশিমালা* opp) { ফেরত new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        নির্বাচন (senItemp->edgeType()) {
        ক্ষেত্র VEdgeType::ET_CHANGED:
        ক্ষেত্র VEdgeType::ET_HYBRID:  //
            যদি (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ধ্রুবক resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                ফেরত {wrap_রাশিমালা_With_Null_Check(flp, resultp, baseRefp), true};
            }
            ফেরত {wrap_রাশিমালা_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        ক্ষেত্র VEdgeType::ET_BOTHEDGE:  //
            ফেরত {wrap_রাশিমালা_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        ক্ষেত্র VEdgeType::ET_POSEDGE:  //
            ফেরত {wrap_রাশিমালা_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        ক্ষেত্র VEdgeType::ET_NEGEDGE:  //
            ফেরত {wrap_রাশিমালা_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        ক্ষেত্র VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক clearFromp = currp();
            যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুবক refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), গ্রন্থি_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ধ্রুবক clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_গ্রন্থি_Stmt* ধ্রুবক clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.ঠেলো_back(clearStmtp);
            m_results.m_destructivePostUpdates.ঠেলো_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ধ্রুবক callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক firedp = wrap_রাশিমালা_With_Null_Check(flp, callp, baseRefp);
            যদি (ধ্রুবক AstVarRef* ধ্রুবক refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                যদি (refp->varScopep() == v3Global.rootp()->nbaEventp()) ফেরত {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            ফেরত {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        ক্ষেত্র VEdgeType::ET_TRUE:  //
            ফেরত {currp(), false};
        ক্ষেত্র VEdgeType::ET_INITIAL_NBA:  //
            ফেরত {new AstConst{flp, AstConst::BitFalse{}}, true};
        পূর্বনির্ধারিত:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            ফেরত {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

সর্বজনীন:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_গ্রন্থি_রাশিমালা*, bool> build(ধ্রুবক AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_বৃক্ষ_Pure(false), term.first};
        }
        ফেরত term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_গ্রন্থি_রাশিমালা*, bool> build(ধ্রুবক Ast_Sen_বৃক্ষ* senTreep) {
        File_সারি* ধ্রুবক flp = senTreep->fileline();
        Ast_গ্রন্থি_রাশিমালা* resultp = nullptr;
        bool firedAtInitialization = false;
        জন্য (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            ধ্রুবক auto& pair = build(senItemp);
            যদি (Ast_গ্রন্থি_রাশিমালা* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        ফেরত {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        ফেরত ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_পূর্ব.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        ফেরত std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_রাশিমালা_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
