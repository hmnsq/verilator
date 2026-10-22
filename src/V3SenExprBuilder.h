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

class Sen_अभिव्यक्ति_Builder final {
सार्वजनिक:
    // TYPES
    संरचना Results final {
        std::vector<Ast_गाँठो_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_गाँठो_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_गाँठो_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_गाँठो_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

गुप्त:
    // STATE
    AstScope* अचर m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_गाँठो>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_गाँठो>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_गाँठो>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_गाँठो>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    स्थिर bool isSupportedDType(Ast_गाँठो_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (VN_IS(dtypep, BasicDType)) फर्काऊ true;
        यदि (VN_IS(dtypep, PackArrayDType)) फर्काऊ true;
        यदि (VN_IS(dtypep, UnpackArrayDType)) फर्काऊ isSupportedDType(dtypep->subDTypep());
        यदि (VN_IS(dtypep, गाँठो_UOr_Struct_DType)) फर्काऊ true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        यदि (VN_IS(dtypep, ClassRefDType)) फर्काऊ true;
        फर्काऊ false;
    }

    स्थिर bool is_Simple_अभिव्यक्ति(अचर Ast_गाँठो* अचर exprp) {
        फर्काऊ exprp->forall([](अचर Ast_गाँठो* अचर nodep) {
            फर्काऊ VN_IS(nodep, Const) || VN_IS(nodep, गाँठो_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, गाँठो_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    स्थिर bool isNullableRef(अचर Ast_गाँठो_अभिव्यक्ति* अचर refp) {
        यदि (!refp->dtypep()) फर्काऊ false;
        अचर Ast_गाँठो_DType* अचर dtypep = refp->dtypep()->skipRefp();
        अचर AstIfaceRefDType* अचर ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        फर्काऊ VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    स्थिर bool hasNullableMemberAccess(अचर Ast_गाँठो* अचर exprp) {
        फर्काऊ exprp && exprp->exists([](अचर Ast_गाँठो* अचर nodep) {
            अचर AstMemberSel* अचर mselp = VN_CAST(nodep, MemberSel);
            फर्काऊ mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    स्थिर अचर Ast_गाँठो_अभिव्यक्ति* getBaseNullableRef(अचर Ast_गाँठो_अभिव्यक्ति* exprp) {
        जबसम्म (exprp) {
            यदि (अचर AstMemberSel* अचर mselp = VN_CAST(exprp, MemberSel)) {
                अचर Ast_गाँठो_अभिव्यक्ति* अचर fromp = mselp->fromp();
                यदि (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    यदि (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        जारी;
                    }
                    फर्काऊ fromp;
                }
                exprp = fromp;
            } अन्यथा {
                फर्काऊ nullptr;
            }
        }
        फर्काऊ nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_गाँठो_अभिव्यक्ति* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        यदि (AstVarRef* अचर refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* अचर vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } अन्यथा {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_पङ्क्ति* अचर flp = m_scopep->fileline();
        AstVar* अचर varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* अचर vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.थप_back(varp);
        फर्काऊ vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_गाँठो_Stmt* wrapStmtWithNullCheck(File_पङ्क्ति* flp, Ast_गाँठो_Stmt* stmtp,
                                       अचर Ast_गाँठो_अभिव्यक्ति* baseRefp) {
        यदि (!baseRefp) फर्काऊ stmtp;
        Ast_गाँठो_अभिव्यक्ति* अचर nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_गाँठो_अभिव्यक्ति* अचर checkp
            = new AstNeq{flp, const_cast<Ast_गाँठो_अभिव्यक्ति*>(baseRefp)->clone_रूख(false), nullp};
        फर्काऊ new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_गाँठो_अभिव्यक्ति* wrap_अभिव्यक्ति_With_Null_Check(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* exprp,
                                       अचर Ast_गाँठो_अभिव्यक्ति* baseRefp) {
        यदि (!baseRefp) फर्काऊ exprp;
        Ast_गाँठो_अभिव्यक्ति* अचर nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_गाँठो_अभिव्यक्ति* अचर checkp
            = new AstNeq{flp, const_cast<Ast_गाँठो_अभिव्यक्ति*>(baseRefp)->clone_रूख(false), nullp};
        Ast_गाँठो_अभिव्यक्ति* अचर falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_गाँठो_अभिव्यक्ति* अचर condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        फर्काऊ condp;
    }

    Ast_गाँठो_अभिव्यक्ति* getCurr(Ast_गाँठो_अभिव्यक्ति* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        यदि (is_Simple_अभिव्यक्ति(exprp)) फर्काऊ exprp->clone_रूख(false);

        // Create the 'current value' variable
        File_पङ्क्ति* अचर flp = exprp->fileline();
        स्वतः result = m_curr.emplace(*exprp, nullptr);
        यदि (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* अचर currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        अचर Ast_गाँठो_अभिव्यक्ति* अचर baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        यदि (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.थप_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_रूख(false)},
                                      baseRefp));
        }
        फर्काऊ new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_गाँठो_अभिव्यक्ति* exprp) {
        File_पङ्क्ति* अचर flp = exprp->fileline();
        अचर स्वतः rdCurr = [this, exprp]() { फर्काऊ getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        अचर Ast_गाँठो_अभिव्यक्ति* अचर baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_गाँठो* scopeExprp = exprp;
        यदि (AstVarRef* अचर refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        अचर स्वतः pair = m_prev.emplace(*scopeExprp, nullptr);
        यदि (pair.second) {
            AstVarScope* अचर prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* अचर initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_रूख(false)};
            m_results.m_inits.थप_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* अचर prevp = pair.first->second;

        अचर स्वतः wrPrev = [=]() { फर्काऊ new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        यदि (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_गाँठो_DType* अचर अभिव्यक्ति_Dtp = exprp->dtypep()->skipRefp();
            यदि (!isSupportedDType(अभिव्यक्ति_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << अभिव्यक्ति_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                फर्काऊ prevp;
            }
            यदि (VN_IS(अभिव्यक्ति_Dtp, UnpackArrayDType)) {
                AstCMethodHard* अचर cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.थप_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } अन्यथा {
                m_results.m_postUpdates.थप_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        फर्काऊ prevp;
    }

    std::pair<Ast_गाँठो_अभिव्यक्ति*, bool> createTerm(अचर AstSenItem* senItemp) {
        File_पङ्क्ति* अचर flp = senItemp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        अचर Ast_गाँठो_अभिव्यक्ति* अचर baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        अचर स्वतः currp = [this, senp]() { फर्काऊ getCurr(senp); };
        अचर स्वतः prevp
            = [this, flp, senp]() { फर्काऊ new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        अचर स्वतः lsb = [=](Ast_गाँठो_अभिव्यक्ति* opp) { फर्काऊ new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        विकल्प (senItemp->edgeType()) {
        पक्ष VEdgeType::ET_CHANGED:
        पक्ष VEdgeType::ET_HYBRID:  //
            यदि (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* अचर resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                फर्काऊ {wrap_अभिव्यक्ति_With_Null_Check(flp, resultp, baseRefp), true};
            }
            फर्काऊ {wrap_अभिव्यक्ति_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        पक्ष VEdgeType::ET_BOTHEDGE:  //
            फर्काऊ {wrap_अभिव्यक्ति_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        पक्ष VEdgeType::ET_POSEDGE:  //
            फर्काऊ {wrap_अभिव्यक्ति_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        पक्ष VEdgeType::ET_NEGEDGE:  //
            फर्काऊ {wrap_अभिव्यक्ति_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        पक्ष VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_गाँठो_अभिव्यक्ति* अचर clearFromp = currp();
            यदि (Ast_गाँठो_Var_Ref* अचर refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), गाँठो_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* अचर clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_गाँठो_Stmt* अचर clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.थप_back(clearStmtp);
            m_results.m_destructivePostUpdates.थप_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* अचर callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_गाँठो_अभिव्यक्ति* अचर firedp = wrap_अभिव्यक्ति_With_Null_Check(flp, callp, baseRefp);
            यदि (अचर AstVarRef* अचर refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                यदि (refp->varScopep() == v3Global.rootp()->nbaEventp()) फर्काऊ {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_गाँठो_अभिव्यक्ति* अचर notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            फर्काऊ {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        पक्ष VEdgeType::ET_TRUE:  //
            फर्काऊ {currp(), false};
        पक्ष VEdgeType::ET_INITIAL_NBA:  //
            फर्काऊ {new AstConst{flp, AstConst::BitFalse{}}, true};
        स्वभाव:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            फर्काऊ {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

सार्वजनिक:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_गाँठो_अभिव्यक्ति*, bool> build(अचर AstSenItem* senItemp) {
        स्वतः term = createTerm(senItemp);
        यदि (Ast_गाँठो_अभिव्यक्ति* अचर condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_रूख_Pure(false), term.first};
        }
        फर्काऊ term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_गाँठो_अभिव्यक्ति*, bool> build(अचर Ast_Sen_रूख* senTreep) {
        File_पङ्क्ति* अचर flp = senTreep->fileline();
        Ast_गाँठो_अभिव्यक्ति* resultp = nullptr;
        bool firedAtInitialization = false;
        लागि (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            अचर स्वतः& pair = build(senItemp);
            यदि (Ast_गाँठो_अभिव्यक्ति* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        फर्काऊ {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        फर्काऊ ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        फर्काऊ std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_अभिव्यक्ति_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
