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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_व्यञ्जकम्_Builder final {
सार्वजनिक:
    // TYPES
    संरचना Results final {
        std::vector<Ast_ग्रन्थिः_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_ग्रन्थिः_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_ग्रन्थिः_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_ग्रन्थिः_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

गुप्त:
    // STATE
    AstScope* ध्रुव m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_ग्रन्थिः>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_ग्रन्थिः>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_ग्रन्थिः>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_ग्रन्थिः>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    स्थिर bool isSupportedDType(Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        सचे (VN_IS(dtypep, BasicDType)) पटिनिवत्त true;
        सचे (VN_IS(dtypep, PackArrayDType)) पटिनिवत्त true;
        सचे (VN_IS(dtypep, UnpackArrayDType)) पटिनिवत्त isSupportedDType(dtypep->subDTypep());
        सचे (VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType)) पटिनिवत्त true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        सचे (VN_IS(dtypep, ClassRefDType)) पटिनिवत्त true;
        पटिनिवत्त false;
    }

    स्थिर bool is_Simple_व्यञ्जकम्(ध्रुव Ast_ग्रन्थिः* ध्रुव exprp) {
        पटिनिवत्त exprp->forall([](ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
            पटिनिवत्त VN_IS(nodep, Const) || VN_IS(nodep, ग्रन्थिः_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, ग्रन्थिः_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    स्थिर bool isNullableRef(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव refp) {
        सचे (!refp->dtypep()) पटिनिवत्त false;
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = refp->dtypep()->skipRefp();
        ध्रुव AstIfaceRefDType* ध्रुव ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        पटिनिवत्त VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    स्थिर bool hasNullableMemberAccess(ध्रुव Ast_ग्रन्थिः* ध्रुव exprp) {
        पटिनिवत्त exprp && exprp->exists([](ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
            ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(nodep, MemberSel);
            पटिनिवत्त mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    स्थिर ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* getBaseNullableRef(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        याव (exprp) {
            सचे (ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(exprp, MemberSel)) {
                ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = mselp->fromp();
                सचे (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    सचे (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        अनुवर्त;
                    }
                    पटिनिवत्त fromp;
                }
                exprp = fromp;
            } अञ्ञथा {
                पटिनिवत्त nullptr;
            }
        }
        पटिनिवत्त nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        सचे (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ध्रुव vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } अञ्ञथा {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_पङ्क्तिः* ध्रुव flp = m_scopep->fileline();
        AstVar* ध्रुव varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* ध्रुव vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.प्रक्षिप_back(varp);
        पटिनिवत्त vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_ग्रन्थिः_Stmt* wrapStmtWithNullCheck(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_Stmt* stmtp,
                                       ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* baseRefp) {
        सचे (!baseRefp) पटिनिवत्त stmtp;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव checkp
            = new AstNeq{flp, const_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(baseRefp)->clone_वृक्षः(false), nullp};
        पटिनिवत्त new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_ग्रन्थिः_व्यञ्जकम्* wrap_व्यञ्जकम्_With_Null_Check(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                                       ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* baseRefp) {
        सचे (!baseRefp) पटिनिवत्त exprp;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव checkp
            = new AstNeq{flp, const_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(baseRefp)->clone_वृक्षः(false), nullp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        पटिनिवत्त condp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* getCurr(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        सचे (is_Simple_व्यञ्जकम्(exprp)) पटिनिवत्त exprp->clone_वृक्षः(false);

        // Create the 'current value' variable
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        सचे (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* ध्रुव currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        सचे (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.प्रक्षिप_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_वृक्षः(false)},
                                      baseRefp));
        }
        पटिनिवत्त new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        ध्रुव auto rdCurr = [this, exprp]() { पटिनिवत्त getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_ग्रन्थिः* scopeExprp = exprp;
        सचे (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ध्रुव auto pair = m_prev.emplace(*scopeExprp, nullptr);
        सचे (pair.second) {
            AstVarScope* ध्रुव prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* ध्रुव initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_वृक्षः(false)};
            m_results.m_inits.प्रक्षिप_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* ध्रुव prevp = pair.first->second;

        ध्रुव auto wrPrev = [=]() { पटिनिवत्त new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        सचे (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_ग्रन्थिः_DType* ध्रुव व्यञ्जकम्_Dtp = exprp->dtypep()->skipRefp();
            सचे (!isSupportedDType(व्यञ्जकम्_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << व्यञ्जकम्_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                पटिनिवत्त prevp;
            }
            सचे (VN_IS(व्यञ्जकम्_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ध्रुव cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.प्रक्षिप_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } अञ्ञथा {
                m_results.m_postUpdates.प्रक्षिप_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        पटिनिवत्त prevp;
    }

    std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, bool> createTerm(ध्रुव AstSenItem* senItemp) {
        File_पङ्क्तिः* ध्रुव flp = senItemp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        ध्रुव auto currp = [this, senp]() { पटिनिवत्त getCurr(senp); };
        ध्रुव auto prevp
            = [this, flp, senp]() { पटिनिवत्त new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ध्रुव auto lsb = [=](Ast_ग्रन्थिः_व्यञ्जकम्* opp) { पटिनिवत्त new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        विकल्प (senItemp->edgeType()) {
        पक्ष VEdgeType::ET_CHANGED:
        पक्ष VEdgeType::ET_HYBRID:  //
            सचे (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ध्रुव resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                पटिनिवत्त {wrap_व्यञ्जकम्_With_Null_Check(flp, resultp, baseRefp), true};
            }
            पटिनिवत्त {wrap_व्यञ्जकम्_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        पक्ष VEdgeType::ET_BOTHEDGE:  //
            पटिनिवत्त {wrap_व्यञ्जकम्_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        पक्ष VEdgeType::ET_POSEDGE:  //
            पटिनिवत्त {wrap_व्यञ्जकम्_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        पक्ष VEdgeType::ET_NEGEDGE:  //
            पटिनिवत्त {wrap_व्यञ्जकम्_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        पक्ष VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clearFromp = currp();
            सचे (Ast_ग्रन्थिः_Var_Ref* ध्रुव refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), ग्रन्थिः_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ध्रुव clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_ग्रन्थिः_Stmt* ध्रुव clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.प्रक्षिप_back(clearStmtp);
            m_results.m_destructivePostUpdates.प्रक्षिप_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ध्रुव callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firedp = wrap_व्यञ्जकम्_With_Null_Check(flp, callp, baseRefp);
            सचे (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                सचे (refp->varScopep() == v3Global.rootp()->nbaEventp()) पटिनिवत्त {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            पटिनिवत्त {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        पक्ष VEdgeType::ET_TRUE:  //
            पटिनिवत्त {currp(), false};
        पक्ष VEdgeType::ET_INITIAL_NBA:  //
            पटिनिवत्त {new AstConst{flp, AstConst::BitFalse{}}, true};
        स्वभाव:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            पटिनिवत्त {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

सार्वजनिक:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, bool> build(ध्रुव AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_वृक्षः_Pure(false), term.first};
        }
        पटिनिवत्त term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_ग्रन्थिः_व्यञ्जकम्*, bool> build(ध्रुव Ast_Sen_वृक्षः* senTreep) {
        File_पङ्क्तिः* ध्रुव flp = senTreep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = nullptr;
        bool firedAtInitialization = false;
        अत्थाय (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            ध्रुव auto& pair = build(senItemp);
            सचे (Ast_ग्रन्थिः_व्यञ्जकम्* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        पटिनिवत्त {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        पटिनिवत्त ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        पटिनिवत्त std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_व्यञ्जकम्_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
