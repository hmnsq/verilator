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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_கோவை_Builder final {
பொது:
    // TYPES
    கட்டமைப்பு Results final {
        std::vector<Ast_முனை_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_முனை_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_முனை_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_முனை_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

தனிப்பட்ட:
    // STATE
    AstScope* மாறிலி m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_முனை>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_முனை>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_முனை>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_முனை>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    நிலையான bool isSupportedDType(Ast_முனை_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        எனில் (VN_IS(dtypep, BasicDType)) திரும்பு true;
        எனில் (VN_IS(dtypep, PackArrayDType)) திரும்பு true;
        எனில் (VN_IS(dtypep, UnpackArrayDType)) திரும்பு isSupportedDType(dtypep->subDTypep());
        எனில் (VN_IS(dtypep, முனை_UOr_Struct_DType)) திரும்பு true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        எனில் (VN_IS(dtypep, ClassRefDType)) திரும்பு true;
        திரும்பு false;
    }

    நிலையான bool is_Simple_கோவை(மாறிலி Ast_முனை* மாறிலி exprp) {
        திரும்பு exprp->forall([](மாறிலி Ast_முனை* மாறிலி nodep) {
            திரும்பு VN_IS(nodep, Const) || VN_IS(nodep, முனை_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, முனை_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    நிலையான bool isNullableRef(மாறிலி Ast_முனை_கோவை* மாறிலி refp) {
        எனில் (!refp->dtypep()) திரும்பு false;
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = refp->dtypep()->skipRefp();
        மாறிலி AstIfaceRefDType* மாறிலி ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        திரும்பு VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    நிலையான bool hasNullableMemberAccess(மாறிலி Ast_முனை* மாறிலி exprp) {
        திரும்பு exprp && exprp->exists([](மாறிலி Ast_முனை* மாறிலி nodep) {
            மாறிலி AstMemberSel* மாறிலி mselp = VN_CAST(nodep, MemberSel);
            திரும்பு mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    நிலையான மாறிலி Ast_முனை_கோவை* getBaseNullableRef(மாறிலி Ast_முனை_கோவை* exprp) {
        போது (exprp) {
            எனில் (மாறிலி AstMemberSel* மாறிலி mselp = VN_CAST(exprp, MemberSel)) {
                மாறிலி Ast_முனை_கோவை* மாறிலி fromp = mselp->fromp();
                எனில் (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    எனில் (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        தொடர்;
                    }
                    திரும்பு fromp;
                }
                exprp = fromp;
            } இல்லையெனில் {
                திரும்பு nullptr;
            }
        }
        திரும்பு nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_முனை_கோவை* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        எனில் (AstVarRef* மாறிலி refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* மாறிலி vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } இல்லையெனில் {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_வரி* மாறிலி flp = m_scopep->fileline();
        AstVar* மாறிலி varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* மாறிலி vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.தள்ளு_back(varp);
        திரும்பு vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_முனை_Stmt* wrapStmtWithNullCheck(File_வரி* flp, Ast_முனை_Stmt* stmtp,
                                       மாறிலி Ast_முனை_கோவை* baseRefp) {
        எனில் (!baseRefp) திரும்பு stmtp;
        Ast_முனை_கோவை* மாறிலி nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_முனை_கோவை* மாறிலி checkp
            = new AstNeq{flp, const_cast<Ast_முனை_கோவை*>(baseRefp)->clone_மரம்(false), nullp};
        திரும்பு new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_முனை_கோவை* wrap_கோவை_With_Null_Check(File_வரி* flp, Ast_முனை_கோவை* exprp,
                                       மாறிலி Ast_முனை_கோவை* baseRefp) {
        எனில் (!baseRefp) திரும்பு exprp;
        Ast_முனை_கோவை* மாறிலி nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_முனை_கோவை* மாறிலி checkp
            = new AstNeq{flp, const_cast<Ast_முனை_கோவை*>(baseRefp)->clone_மரம்(false), nullp};
        Ast_முனை_கோவை* மாறிலி falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_முனை_கோவை* மாறிலி condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        திரும்பு condp;
    }

    Ast_முனை_கோவை* getCurr(Ast_முனை_கோவை* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        எனில் (is_Simple_கோவை(exprp)) திரும்பு exprp->clone_மரம்(false);

        // Create the 'current value' variable
        File_வரி* மாறிலி flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        எனில் (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* மாறிலி currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        மாறிலி Ast_முனை_கோவை* மாறிலி baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        எனில் (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.தள்ளு_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_மரம்(false)},
                                      baseRefp));
        }
        திரும்பு new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_முனை_கோவை* exprp) {
        File_வரி* மாறிலி flp = exprp->fileline();
        மாறிலி auto rdCurr = [this, exprp]() { திரும்பு getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        மாறிலி Ast_முனை_கோவை* மாறிலி baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_முனை* scopeExprp = exprp;
        எனில் (AstVarRef* மாறிலி refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        மாறிலி auto pair = m_prev.emplace(*scopeExprp, nullptr);
        எனில் (pair.second) {
            AstVarScope* மாறிலி prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* மாறிலி initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_மரம்(false)};
            m_results.m_inits.தள்ளு_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* மாறிலி prevp = pair.first->second;

        மாறிலி auto wrPrev = [=]() { திரும்பு new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        எனில் (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_முனை_DType* மாறிலி கோவை_Dtp = exprp->dtypep()->skipRefp();
            எனில் (!isSupportedDType(கோவை_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << கோவை_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                திரும்பு prevp;
            }
            எனில் (VN_IS(கோவை_Dtp, UnpackArrayDType)) {
                AstCMethodHard* மாறிலி cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.தள்ளு_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } இல்லையெனில் {
                m_results.m_postUpdates.தள்ளு_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        திரும்பு prevp;
    }

    std::pair<Ast_முனை_கோவை*, bool> createTerm(மாறிலி AstSenItem* senItemp) {
        File_வரி* மாறிலி flp = senItemp->fileline();
        Ast_முனை_கோவை* மாறிலி senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        மாறிலி Ast_முனை_கோவை* மாறிலி baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        மாறிலி auto currp = [this, senp]() { திரும்பு getCurr(senp); };
        மாறிலி auto prevp
            = [this, flp, senp]() { திரும்பு new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        மாறிலி auto lsb = [=](Ast_முனை_கோவை* opp) { திரும்பு new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        தேர்வு (senItemp->edgeType()) {
        நிலை VEdgeType::ET_CHANGED:
        நிலை VEdgeType::ET_HYBRID:  //
            எனில் (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* மாறிலி resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                திரும்பு {wrap_கோவை_With_Null_Check(flp, resultp, baseRefp), true};
            }
            திரும்பு {wrap_கோவை_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        நிலை VEdgeType::ET_BOTHEDGE:  //
            திரும்பு {wrap_கோவை_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        நிலை VEdgeType::ET_POSEDGE:  //
            திரும்பு {wrap_கோவை_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        நிலை VEdgeType::ET_NEGEDGE:  //
            திரும்பு {wrap_கோவை_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        நிலை VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_முனை_கோவை* மாறிலி clearFromp = currp();
            எனில் (Ast_முனை_Var_Ref* மாறிலி refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), முனை_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* மாறிலி clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_முனை_Stmt* மாறிலி clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.தள்ளு_back(clearStmtp);
            m_results.m_destructivePostUpdates.தள்ளு_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* மாறிலி callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_முனை_கோவை* மாறிலி firedp = wrap_கோவை_With_Null_Check(flp, callp, baseRefp);
            எனில் (மாறிலி AstVarRef* மாறிலி refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                எனில் (refp->varScopep() == v3Global.rootp()->nbaEventp()) திரும்பு {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_முனை_கோவை* மாறிலி notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            திரும்பு {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        நிலை VEdgeType::ET_TRUE:  //
            திரும்பு {currp(), false};
        நிலை VEdgeType::ET_INITIAL_NBA:  //
            திரும்பு {new AstConst{flp, AstConst::BitFalse{}}, true};
        இயல்பு:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            திரும்பு {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

பொது:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_முனை_கோவை*, bool> build(மாறிலி AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        எனில் (Ast_முனை_கோவை* மாறிலி condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_மரம்_Pure(false), term.first};
        }
        திரும்பு term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_முனை_கோவை*, bool> build(மாறிலி Ast_Sen_மரம்* senTreep) {
        File_வரி* மாறிலி flp = senTreep->fileline();
        Ast_முனை_கோவை* resultp = nullptr;
        bool firedAtInitialization = false;
        க்கு (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            மாறிலி auto& pair = build(senItemp);
            எனில் (Ast_முனை_கோவை* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        திரும்பு {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        திரும்பு ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        திரும்பு std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_கோவை_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
