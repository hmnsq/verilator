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

class Sen_व्यंजक_Builder final {
सार्वजनिक:
    // TYPES
    संरचना Results final {
        std::vector<Ast_गाँठ_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_गाँठ_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_गाँठ_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_गाँठ_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

गुप्त:
    // STATE
    AstScope* ध्रुव m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_गाँठ>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_गाँठ>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_गाँठ>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_गाँठ>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    स्थिर bool isSupportedDType(Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (VN_IS(dtypep, BasicDType)) प्रत्यागम true;
        यदि (VN_IS(dtypep, PackArrayDType)) प्रत्यागम true;
        यदि (VN_IS(dtypep, UnpackArrayDType)) प्रत्यागम isSupportedDType(dtypep->subDTypep());
        यदि (VN_IS(dtypep, गाँठ_UOr_Struct_DType)) प्रत्यागम true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        यदि (VN_IS(dtypep, ClassRefDType)) प्रत्यागम true;
        प्रत्यागम false;
    }

    स्थिर bool is_Simple_व्यंजक(ध्रुव Ast_गाँठ* ध्रुव exprp) {
        प्रत्यागम exprp->forall([](ध्रुव Ast_गाँठ* ध्रुव nodep) {
            प्रत्यागम VN_IS(nodep, Const) || VN_IS(nodep, गाँठ_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, गाँठ_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    स्थिर bool isNullableRef(ध्रुव Ast_गाँठ_व्यंजक* ध्रुव refp) {
        यदि (!refp->dtypep()) प्रत्यागम false;
        ध्रुव Ast_गाँठ_DType* ध्रुव dtypep = refp->dtypep()->skipRefp();
        ध्रुव AstIfaceRefDType* ध्रुव ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        प्रत्यागम VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    स्थिर bool hasNullableMemberAccess(ध्रुव Ast_गाँठ* ध्रुव exprp) {
        प्रत्यागम exprp && exprp->exists([](ध्रुव Ast_गाँठ* ध्रुव nodep) {
            ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(nodep, MemberSel);
            प्रत्यागम mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    स्थिर ध्रुव Ast_गाँठ_व्यंजक* getBaseNullableRef(ध्रुव Ast_गाँठ_व्यंजक* exprp) {
        यावत् (exprp) {
            यदि (ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(exprp, MemberSel)) {
                ध्रुव Ast_गाँठ_व्यंजक* ध्रुव fromp = mselp->fromp();
                यदि (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    यदि (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        अनुवर्त;
                    }
                    प्रत्यागम fromp;
                }
                exprp = fromp;
            } अन्यथा {
                प्रत्यागम nullptr;
            }
        }
        प्रत्यागम nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_गाँठ_व्यंजक* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        यदि (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ध्रुव vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } अन्यथा {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_पंक्ति* ध्रुव flp = m_scopep->fileline();
        AstVar* ध्रुव varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* ध्रुव vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.धकेलो_back(varp);
        प्रत्यागम vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_गाँठ_Stmt* wrapStmtWithNullCheck(File_पंक्ति* flp, Ast_गाँठ_Stmt* stmtp,
                                       ध्रुव Ast_गाँठ_व्यंजक* baseRefp) {
        यदि (!baseRefp) प्रत्यागम stmtp;
        Ast_गाँठ_व्यंजक* ध्रुव nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_गाँठ_व्यंजक* ध्रुव checkp
            = new AstNeq{flp, const_cast<Ast_गाँठ_व्यंजक*>(baseRefp)->clone_वृक्ष(false), nullp};
        प्रत्यागम new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_गाँठ_व्यंजक* wrap_व्यंजक_With_Null_Check(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* exprp,
                                       ध्रुव Ast_गाँठ_व्यंजक* baseRefp) {
        यदि (!baseRefp) प्रत्यागम exprp;
        Ast_गाँठ_व्यंजक* ध्रुव nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_गाँठ_व्यंजक* ध्रुव checkp
            = new AstNeq{flp, const_cast<Ast_गाँठ_व्यंजक*>(baseRefp)->clone_वृक्ष(false), nullp};
        Ast_गाँठ_व्यंजक* ध्रुव falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_गाँठ_व्यंजक* ध्रुव condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        प्रत्यागम condp;
    }

    Ast_गाँठ_व्यंजक* getCurr(Ast_गाँठ_व्यंजक* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        यदि (is_Simple_व्यंजक(exprp)) प्रत्यागम exprp->clone_वृक्ष(false);

        // Create the 'current value' variable
        File_पंक्ति* ध्रुव flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        यदि (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* ध्रुव currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        ध्रुव Ast_गाँठ_व्यंजक* ध्रुव baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        यदि (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.धकेलो_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_वृक्ष(false)},
                                      baseRefp));
        }
        प्रत्यागम new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_गाँठ_व्यंजक* exprp) {
        File_पंक्ति* ध्रुव flp = exprp->fileline();
        ध्रुव auto rdCurr = [this, exprp]() { प्रत्यागम getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ध्रुव Ast_गाँठ_व्यंजक* ध्रुव baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_गाँठ* scopeExprp = exprp;
        यदि (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ध्रुव auto pair = m_prev.emplace(*scopeExprp, nullptr);
        यदि (pair.second) {
            AstVarScope* ध्रुव prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* ध्रुव initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_वृक्ष(false)};
            m_results.m_inits.धकेलो_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* ध्रुव prevp = pair.first->second;

        ध्रुव auto wrPrev = [=]() { प्रत्यागम new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        यदि (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_गाँठ_DType* ध्रुव व्यंजक_Dtp = exprp->dtypep()->skipRefp();
            यदि (!isSupportedDType(व्यंजक_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << व्यंजक_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                प्रत्यागम prevp;
            }
            यदि (VN_IS(व्यंजक_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ध्रुव cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.धकेलो_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } अन्यथा {
                m_results.m_postUpdates.धकेलो_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        प्रत्यागम prevp;
    }

    std::pair<Ast_गाँठ_व्यंजक*, bool> createTerm(ध्रुव AstSenItem* senItemp) {
        File_पंक्ति* ध्रुव flp = senItemp->fileline();
        Ast_गाँठ_व्यंजक* ध्रुव senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ध्रुव Ast_गाँठ_व्यंजक* ध्रुव baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        ध्रुव auto currp = [this, senp]() { प्रत्यागम getCurr(senp); };
        ध्रुव auto prevp
            = [this, flp, senp]() { प्रत्यागम new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ध्रुव auto lsb = [=](Ast_गाँठ_व्यंजक* opp) { प्रत्यागम new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        विकल्प (senItemp->edgeType()) {
        पक्ष VEdgeType::ET_CHANGED:
        पक्ष VEdgeType::ET_HYBRID:  //
            यदि (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ध्रुव resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                प्रत्यागम {wrap_व्यंजक_With_Null_Check(flp, resultp, baseRefp), true};
            }
            प्रत्यागम {wrap_व्यंजक_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        पक्ष VEdgeType::ET_BOTHEDGE:  //
            प्रत्यागम {wrap_व्यंजक_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        पक्ष VEdgeType::ET_POSEDGE:  //
            प्रत्यागम {wrap_व्यंजक_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        पक्ष VEdgeType::ET_NEGEDGE:  //
            प्रत्यागम {wrap_व्यंजक_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        पक्ष VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_गाँठ_व्यंजक* ध्रुव clearFromp = currp();
            यदि (Ast_गाँठ_Var_Ref* ध्रुव refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), गाँठ_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ध्रुव clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_गाँठ_Stmt* ध्रुव clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.धकेलो_back(clearStmtp);
            m_results.m_destructivePostUpdates.धकेलो_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ध्रुव callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_गाँठ_व्यंजक* ध्रुव firedp = wrap_व्यंजक_With_Null_Check(flp, callp, baseRefp);
            यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                यदि (refp->varScopep() == v3Global.rootp()->nbaEventp()) प्रत्यागम {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_गाँठ_व्यंजक* ध्रुव notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            प्रत्यागम {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        पक्ष VEdgeType::ET_TRUE:  //
            प्रत्यागम {currp(), false};
        पक्ष VEdgeType::ET_INITIAL_NBA:  //
            प्रत्यागम {new AstConst{flp, AstConst::BitFalse{}}, true};
        स्वभाव:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            प्रत्यागम {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

सार्वजनिक:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_गाँठ_व्यंजक*, bool> build(ध्रुव AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        यदि (Ast_गाँठ_व्यंजक* ध्रुव condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_वृक्ष_Pure(false), term.first};
        }
        प्रत्यागम term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_गाँठ_व्यंजक*, bool> build(ध्रुव Ast_Sen_वृक्ष* senTreep) {
        File_पंक्ति* ध्रुव flp = senTreep->fileline();
        Ast_गाँठ_व्यंजक* resultp = nullptr;
        bool firedAtInitialization = false;
        प्रति (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            ध्रुव auto& pair = build(senItemp);
            यदि (Ast_गाँठ_व्यंजक* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        प्रत्यागम {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        प्रत्यागम ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        प्रत्यागम std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_व्यंजक_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
