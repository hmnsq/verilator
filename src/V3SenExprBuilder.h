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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_પદાવલિ_Builder final {
જાહેર:
    // TYPES
    સંરચના Results final {
        std::vector<Ast_ગાંઠ_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_ગાંઠ_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_ગાંઠ_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_ગાંઠ_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

ખાનગી:
    // STATE
    AstScope* અચલ m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_ગાંઠ>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_ગાંઠ>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_ગાંઠ>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_ગાંઠ>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    સ્થિર bool isSupportedDType(Ast_ગાંઠ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        જો (VN_IS(dtypep, BasicDType)) પાછા true;
        જો (VN_IS(dtypep, PackArrayDType)) પાછા true;
        જો (VN_IS(dtypep, UnpackArrayDType)) પાછા isSupportedDType(dtypep->subDTypep());
        જો (VN_IS(dtypep, ગાંઠ_UOr_Struct_DType)) પાછા true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        જો (VN_IS(dtypep, ClassRefDType)) પાછા true;
        પાછા false;
    }

    સ્થિર bool is_Simple_પદાવલિ(અચલ Ast_ગાંઠ* અચલ exprp) {
        પાછા exprp->forall([](અચલ Ast_ગાંઠ* અચલ nodep) {
            પાછા VN_IS(nodep, Const) || VN_IS(nodep, ગાંઠ_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, ગાંઠ_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    સ્થિર bool isNullableRef(અચલ Ast_ગાંઠ_પદાવલિ* અચલ refp) {
        જો (!refp->dtypep()) પાછા false;
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = refp->dtypep()->skipRefp();
        અચલ AstIfaceRefDType* અચલ ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        પાછા VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    સ્થિર bool hasNullableMemberAccess(અચલ Ast_ગાંઠ* અચલ exprp) {
        પાછા exprp && exprp->exists([](અચલ Ast_ગાંઠ* અચલ nodep) {
            અચલ AstMemberSel* અચલ mselp = VN_CAST(nodep, MemberSel);
            પાછા mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    સ્થિર અચલ Ast_ગાંઠ_પદાવલિ* getBaseNullableRef(અચલ Ast_ગાંઠ_પદાવલિ* exprp) {
        જ્યારે (exprp) {
            જો (અચલ AstMemberSel* અચલ mselp = VN_CAST(exprp, MemberSel)) {
                અચલ Ast_ગાંઠ_પદાવલિ* અચલ fromp = mselp->fromp();
                જો (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    જો (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        ચાલુ;
                    }
                    પાછા fromp;
                }
                exprp = fromp;
            } નહીંતર {
                પાછા nullptr;
            }
        }
        પાછા nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_ગાંઠ_પદાવલિ* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        જો (AstVarRef* અચલ refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* અચલ vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } નહીંતર {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_લીટી* અચલ flp = m_scopep->fileline();
        AstVar* અચલ varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* અચલ vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.ધકેલો_back(varp);
        પાછા vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_ગાંઠ_Stmt* wrapStmtWithNullCheck(File_લીટી* flp, Ast_ગાંઠ_Stmt* stmtp,
                                       અચલ Ast_ગાંઠ_પદાવલિ* baseRefp) {
        જો (!baseRefp) પાછા stmtp;
        Ast_ગાંઠ_પદાવલિ* અચલ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ગાંઠ_પદાવલિ* અચલ checkp
            = new AstNeq{flp, const_cast<Ast_ગાંઠ_પદાવલિ*>(baseRefp)->clone_વૃક્ષ(false), nullp};
        પાછા new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_ગાંઠ_પદાવલિ* wrap_પદાવલિ_With_Null_Check(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* exprp,
                                       અચલ Ast_ગાંઠ_પદાવલિ* baseRefp) {
        જો (!baseRefp) પાછા exprp;
        Ast_ગાંઠ_પદાવલિ* અચલ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ગાંઠ_પદાવલિ* અચલ checkp
            = new AstNeq{flp, const_cast<Ast_ગાંઠ_પદાવલિ*>(baseRefp)->clone_વૃક્ષ(false), nullp};
        Ast_ગાંઠ_પદાવલિ* અચલ falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_ગાંઠ_પદાવલિ* અચલ condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        પાછા condp;
    }

    Ast_ગાંઠ_પદાવલિ* getCurr(Ast_ગાંઠ_પદાવલિ* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        જો (is_Simple_પદાવલિ(exprp)) પાછા exprp->clone_વૃક્ષ(false);

        // Create the 'current value' variable
        File_લીટી* અચલ flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        જો (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* અચલ currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        અચલ Ast_ગાંઠ_પદાવલિ* અચલ baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        જો (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.ધકેલો_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_વૃક્ષ(false)},
                                      baseRefp));
        }
        પાછા new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_ગાંઠ_પદાવલિ* exprp) {
        File_લીટી* અચલ flp = exprp->fileline();
        અચલ auto rdCurr = [this, exprp]() { પાછા getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        અચલ Ast_ગાંઠ_પદાવલિ* અચલ baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_ગાંઠ* scopeExprp = exprp;
        જો (AstVarRef* અચલ refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        અચલ auto pair = m_prev.emplace(*scopeExprp, nullptr);
        જો (pair.second) {
            AstVarScope* અચલ prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* અચલ initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_વૃક્ષ(false)};
            m_results.m_inits.ધકેલો_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* અચલ prevp = pair.first->second;

        અચલ auto wrPrev = [=]() { પાછા new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        જો (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_ગાંઠ_DType* અચલ પદાવલિ_Dtp = exprp->dtypep()->skipRefp();
            જો (!isSupportedDType(પદાવલિ_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << પદાવલિ_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                પાછા prevp;
            }
            જો (VN_IS(પદાવલિ_Dtp, UnpackArrayDType)) {
                AstCMethodHard* અચલ cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.ધકેલો_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } નહીંતર {
                m_results.m_postUpdates.ધકેલો_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        પાછા prevp;
    }

    std::pair<Ast_ગાંઠ_પદાવલિ*, bool> createTerm(અચલ AstSenItem* senItemp) {
        File_લીટી* અચલ flp = senItemp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        અચલ Ast_ગાંઠ_પદાવલિ* અચલ baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        અચલ auto currp = [this, senp]() { પાછા getCurr(senp); };
        અચલ auto prevp
            = [this, flp, senp]() { પાછા new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        અચલ auto lsb = [=](Ast_ગાંઠ_પદાવલિ* opp) { પાછા new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        પસંદગી (senItemp->edgeType()) {
        સ્થિતિ VEdgeType::ET_CHANGED:
        સ્થિતિ VEdgeType::ET_HYBRID:  //
            જો (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* અચલ resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                પાછા {wrap_પદાવલિ_With_Null_Check(flp, resultp, baseRefp), true};
            }
            પાછા {wrap_પદાવલિ_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        સ્થિતિ VEdgeType::ET_BOTHEDGE:  //
            પાછા {wrap_પદાવલિ_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        સ્થિતિ VEdgeType::ET_POSEDGE:  //
            પાછા {wrap_પદાવલિ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        સ્થિતિ VEdgeType::ET_NEGEDGE:  //
            પાછા {wrap_પદાવલિ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        સ્થિતિ VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_ગાંઠ_પદાવલિ* અચલ clearFromp = currp();
            જો (Ast_ગાંઠ_Var_Ref* અચલ refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), ગાંઠ_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* અચલ clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_ગાંઠ_Stmt* અચલ clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.ધકેલો_back(clearStmtp);
            m_results.m_destructivePostUpdates.ધકેલો_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* અચલ callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_ગાંઠ_પદાવલિ* અચલ firedp = wrap_પદાવલિ_With_Null_Check(flp, callp, baseRefp);
            જો (અચલ AstVarRef* અચલ refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                જો (refp->varScopep() == v3Global.rootp()->nbaEventp()) પાછા {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_ગાંઠ_પદાવલિ* અચલ notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            પાછા {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        સ્થિતિ VEdgeType::ET_TRUE:  //
            પાછા {currp(), false};
        સ્થિતિ VEdgeType::ET_INITIAL_NBA:  //
            પાછા {new AstConst{flp, AstConst::BitFalse{}}, true};
        મૂળભૂત:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            પાછા {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

જાહેર:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_ગાંઠ_પદાવલિ*, bool> build(અચલ AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        જો (Ast_ગાંઠ_પદાવલિ* અચલ condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_વૃક્ષ_Pure(false), term.first};
        }
        પાછા term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_ગાંઠ_પદાવલિ*, bool> build(અચલ Ast_Sen_વૃક્ષ* senTreep) {
        File_લીટી* અચલ flp = senTreep->fileline();
        Ast_ગાંઠ_પદાવલિ* resultp = nullptr;
        bool firedAtInitialization = false;
        માટે (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            અચલ auto& pair = build(senItemp);
            જો (Ast_ગાંઠ_પદાવલિ* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        પાછા {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        પાછા ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        પાછા std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_પદાવલિ_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
