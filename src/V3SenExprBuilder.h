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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_വ്യഞ്ജകം_Builder final {
പൊതു:
    // TYPES
    ഘടന Results final {
        std::vector<Ast_കണ്ണി_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_കണ്ണി_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_കണ്ണി_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_കണ്ണി_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

സ്വകാര്യ:
    // STATE
    AstScope* സ്ഥിരം m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_കണ്ണി>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_കണ്ണി>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_കണ്ണി>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_കണ്ണി>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    സ്ഥിതം bool isSupportedDType(Ast_കണ്ണി_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        എങ്കിൽ (VN_IS(dtypep, BasicDType)) തിരികെ true;
        എങ്കിൽ (VN_IS(dtypep, PackArrayDType)) തിരികെ true;
        എങ്കിൽ (VN_IS(dtypep, UnpackArrayDType)) തിരികെ isSupportedDType(dtypep->subDTypep());
        എങ്കിൽ (VN_IS(dtypep, കണ്ണി_UOr_Struct_DType)) തിരികെ true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        എങ്കിൽ (VN_IS(dtypep, ClassRefDType)) തിരികെ true;
        തിരികെ false;
    }

    സ്ഥിതം bool is_Simple_വ്യഞ്ജകം(സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം exprp) {
        തിരികെ exprp->forall([](സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep) {
            തിരികെ VN_IS(nodep, Const) || VN_IS(nodep, കണ്ണി_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, കണ്ണി_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    സ്ഥിതം bool isNullableRef(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം refp) {
        എങ്കിൽ (!refp->dtypep()) തിരികെ false;
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = refp->dtypep()->skipRefp();
        സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        തിരികെ VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    സ്ഥിതം bool hasNullableMemberAccess(സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം exprp) {
        തിരികെ exprp && exprp->exists([](സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep) {
            സ്ഥിരം AstMemberSel* സ്ഥിരം mselp = VN_CAST(nodep, MemberSel);
            തിരികെ mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    സ്ഥിതം സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* getBaseNullableRef(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        ആകുമ്പോൾ (exprp) {
            എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം mselp = VN_CAST(exprp, MemberSel)) {
                സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = mselp->fromp();
                എങ്കിൽ (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    എങ്കിൽ (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        തുടരുക;
                    }
                    തിരികെ fromp;
                }
                exprp = fromp;
            } അല്ലെങ്കിൽ {
                തിരികെ nullptr;
            }
        }
        തിരികെ nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* സ്ഥിരം vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } അല്ലെങ്കിൽ {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_വരി* സ്ഥിരം flp = m_scopep->fileline();
        AstVar* സ്ഥിരം varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* സ്ഥിരം vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.തള്ളുക_back(varp);
        തിരികെ vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_കണ്ണി_Stmt* wrapStmtWithNullCheck(File_വരി* flp, Ast_കണ്ണി_Stmt* stmtp,
                                       സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* baseRefp) {
        എങ്കിൽ (!baseRefp) തിരികെ stmtp;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം checkp
            = new AstNeq{flp, const_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(baseRefp)->clone_വൃക്ഷം(false), nullp};
        തിരികെ new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_കണ്ണി_വ്യഞ്ജകം* wrap_വ്യഞ്ജകം_With_Null_Check(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                                       സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* baseRefp) {
        എങ്കിൽ (!baseRefp) തിരികെ exprp;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം checkp
            = new AstNeq{flp, const_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(baseRefp)->clone_വൃക്ഷം(false), nullp};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        തിരികെ condp;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* getCurr(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        എങ്കിൽ (is_Simple_വ്യഞ്ജകം(exprp)) തിരികെ exprp->clone_വൃക്ഷം(false);

        // Create the 'current value' variable
        File_വരി* സ്ഥിരം flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        എങ്കിൽ (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* സ്ഥിരം currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        എങ്കിൽ (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.തള്ളുക_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_വൃക്ഷം(false)},
                                      baseRefp));
        }
        തിരികെ new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        File_വരി* സ്ഥിരം flp = exprp->fileline();
        സ്ഥിരം auto rdCurr = [this, exprp]() { തിരികെ getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_കണ്ണി* scopeExprp = exprp;
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        സ്ഥിരം auto pair = m_prev.emplace(*scopeExprp, nullptr);
        എങ്കിൽ (pair.second) {
            AstVarScope* സ്ഥിരം prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* സ്ഥിരം initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_വൃക്ഷം(false)};
            m_results.m_inits.തള്ളുക_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* സ്ഥിരം prevp = pair.first->second;

        സ്ഥിരം auto wrPrev = [=]() { തിരികെ new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        എങ്കിൽ (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_കണ്ണി_DType* സ്ഥിരം വ്യഞ്ജകം_Dtp = exprp->dtypep()->skipRefp();
            എങ്കിൽ (!isSupportedDType(വ്യഞ്ജകം_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << വ്യഞ്ജകം_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                തിരികെ prevp;
            }
            എങ്കിൽ (VN_IS(വ്യഞ്ജകം_Dtp, UnpackArrayDType)) {
                AstCMethodHard* സ്ഥിരം cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.തള്ളുക_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } അല്ലെങ്കിൽ {
                m_results.m_postUpdates.തള്ളുക_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        തിരികെ prevp;
    }

    std::pair<Ast_കണ്ണി_വ്യഞ്ജകം*, bool> createTerm(സ്ഥിരം AstSenItem* senItemp) {
        File_വരി* സ്ഥിരം flp = senItemp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        സ്ഥിരം auto currp = [this, senp]() { തിരികെ getCurr(senp); };
        സ്ഥിരം auto prevp
            = [this, flp, senp]() { തിരികെ new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        സ്ഥിരം auto lsb = [=](Ast_കണ്ണി_വ്യഞ്ജകം* opp) { തിരികെ new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        തിരഞ്ഞെടുപ്പ് (senItemp->edgeType()) {
        അവസ്ഥ VEdgeType::ET_CHANGED:
        അവസ്ഥ VEdgeType::ET_HYBRID:  //
            എങ്കിൽ (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* സ്ഥിരം resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                തിരികെ {wrap_വ്യഞ്ജകം_With_Null_Check(flp, resultp, baseRefp), true};
            }
            തിരികെ {wrap_വ്യഞ്ജകം_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        അവസ്ഥ VEdgeType::ET_BOTHEDGE:  //
            തിരികെ {wrap_വ്യഞ്ജകം_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        അവസ്ഥ VEdgeType::ET_POSEDGE:  //
            തിരികെ {wrap_വ്യഞ്ജകം_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        അവസ്ഥ VEdgeType::ET_NEGEDGE:  //
            തിരികെ {wrap_വ്യഞ്ജകം_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        അവസ്ഥ VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം clearFromp = currp();
            എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), കണ്ണി_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* സ്ഥിരം clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_കണ്ണി_Stmt* സ്ഥിരം clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.തള്ളുക_back(clearStmtp);
            m_results.m_destructivePostUpdates.തള്ളുക_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* സ്ഥിരം callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം firedp = wrap_വ്യഞ്ജകം_With_Null_Check(flp, callp, baseRefp);
            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                എങ്കിൽ (refp->varScopep() == v3Global.rootp()->nbaEventp()) തിരികെ {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            തിരികെ {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        അവസ്ഥ VEdgeType::ET_TRUE:  //
            തിരികെ {currp(), false};
        അവസ്ഥ VEdgeType::ET_INITIAL_NBA:  //
            തിരികെ {new AstConst{flp, AstConst::BitFalse{}}, true};
        സ്ഥിരസ്ഥിതി:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            തിരികെ {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

പൊതു:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_കണ്ണി_വ്യഞ്ജകം*, bool> build(സ്ഥിരം AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_വൃക്ഷം_Pure(false), term.first};
        }
        തിരികെ term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_കണ്ണി_വ്യഞ്ജകം*, bool> build(സ്ഥിരം Ast_Sen_വൃക്ഷം* senTreep) {
        File_വരി* സ്ഥിരം flp = senTreep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* resultp = nullptr;
        bool firedAtInitialization = false;
        വേണ്ടി (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            സ്ഥിരം auto& pair = build(senItemp);
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        തിരികെ {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        തിരികെ ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        തിരികെ std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_വ്യഞ്ജകം_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
