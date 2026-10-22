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

#अगर_अघोषित VERILATOR_V3SENEXPRBUILDER_H_
#घोषणा VERILATOR_V3SENEXPRBUILDER_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_व्यंजक_Builder final {
खुला:
    // TYPES
    काष्ठा Results final {
        std::vector<Ast_गाँठ_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_गाँठ_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_गाँठ_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_गाँठ_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

निजी:
    // STATE
    AstScope* स्थिर m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_गाँठ>, AstVarScope*> m_पूर्व;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_गाँठ>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_गाँठ>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_गाँठ>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_पूर्व_Names{"__Vtrigprevexpr"};  // For generating unique temporary names

    अटल bool isSupportedDType(Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        अगर (VN_IS(dtypep, BasicDType)) वापस true;
        अगर (VN_IS(dtypep, PackArrayDType)) वापस true;
        अगर (VN_IS(dtypep, UnpackArrayDType)) वापस isSupportedDType(dtypep->subDTypep());
        अगर (VN_IS(dtypep, गाँठ_UOr_Struct_DType)) वापस true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        अगर (VN_IS(dtypep, ClassRefDType)) वापस true;
        वापस false;
    }

    अटल bool is_Simple_व्यंजक(स्थिर Ast_गाँठ* स्थिर exprp) {
        वापस exprp->forall([](स्थिर Ast_गाँठ* स्थिर nodep) {
            वापस VN_IS(nodep, Const) || VN_IS(nodep, गाँठ_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, गाँठ_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    अटल bool isNullableRef(स्थिर Ast_गाँठ_व्यंजक* स्थिर refp) {
        अगर (!refp->dtypep()) वापस false;
        स्थिर Ast_गाँठ_DType* स्थिर dtypep = refp->dtypep()->skipRefp();
        स्थिर AstIfaceRefDType* स्थिर ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        वापस VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    अटल bool hasNullableMemberAccess(स्थिर Ast_गाँठ* स्थिर exprp) {
        वापस exprp && exprp->exists([](स्थिर Ast_गाँठ* स्थिर nodep) {
            स्थिर AstMemberSel* स्थिर mselp = VN_CAST(nodep, MemberSel);
            वापस mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    अटल स्थिर Ast_गाँठ_व्यंजक* getBaseNullableRef(स्थिर Ast_गाँठ_व्यंजक* exprp) {
        जबतक (exprp) {
            अगर (स्थिर AstMemberSel* स्थिर mselp = VN_CAST(exprp, MemberSel)) {
                स्थिर Ast_गाँठ_व्यंजक* स्थिर fromp = mselp->fromp();
                अगर (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    अगर (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        जारी;
                    }
                    वापस fromp;
                }
                exprp = fromp;
            } अन्यथा {
                वापस nullptr;
            }
        }
        वापस nullptr;
    }

    // METHODS
    AstVarScope* crate_अस्थायी(Ast_गाँठ_व्यंजक* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        अगर (AstVarRef* स्थिर refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* स्थिर vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_पूर्व_Names.get(name);
        } अन्यथा {
            name = m_पूर्व_Names.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_पंक्ति* स्थिर flp = m_scopep->fileline();
        AstVar* स्थिर varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* स्थिर vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.धकेलो_back(varp);
        वापस vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_गाँठ_Stmt* wrapStmtWithNullCheck(File_पंक्ति* flp, Ast_गाँठ_Stmt* stmtp,
                                       स्थिर Ast_गाँठ_व्यंजक* baseRefp) {
        अगर (!baseRefp) वापस stmtp;
        Ast_गाँठ_व्यंजक* स्थिर nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_गाँठ_व्यंजक* स्थिर checkp
            = new AstNeq{flp, const_cast<Ast_गाँठ_व्यंजक*>(baseRefp)->clone_वृक्ष(false), nullp};
        वापस new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_गाँठ_व्यंजक* wrap_व्यंजक_With_Null_Check(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* exprp,
                                       स्थिर Ast_गाँठ_व्यंजक* baseRefp) {
        अगर (!baseRefp) वापस exprp;
        Ast_गाँठ_व्यंजक* स्थिर nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_गाँठ_व्यंजक* स्थिर checkp
            = new AstNeq{flp, const_cast<Ast_गाँठ_व्यंजक*>(baseRefp)->clone_वृक्ष(false), nullp};
        Ast_गाँठ_व्यंजक* स्थिर falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_गाँठ_व्यंजक* स्थिर condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        वापस condp;
    }

    Ast_गाँठ_व्यंजक* getCurr(Ast_गाँठ_व्यंजक* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        अगर (is_Simple_व्यंजक(exprp)) वापस exprp->clone_वृक्ष(false);

        // Create the 'current value' variable
        File_पंक्ति* स्थिर flp = exprp->fileline();
        स्वतः परिणाम = m_curr.emplace(*exprp, nullptr);
        अगर (परिणाम.second) परिणाम.first->second = crate_अस्थायी(exprp);
        AstVarScope* स्थिर currp = परिणाम.first->second;

        // Check if we need null guards for a member access through a nullable reference
        स्थिर Ast_गाँठ_व्यंजक* स्थिर baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        अगर (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.धकेलो_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_वृक्ष(false)},
                                      baseRefp));
        }
        वापस new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* get_पूर्व(Ast_गाँठ_व्यंजक* exprp) {
        File_पंक्ति* स्थिर flp = exprp->fileline();
        स्थिर स्वतः rdCurr = [this, exprp]() { वापस getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        स्थिर Ast_गाँठ_व्यंजक* स्थिर baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_गाँठ* scopeExprp = exprp;
        अगर (AstVarRef* स्थिर refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        स्थिर स्वतः pair = m_पूर्व.emplace(*scopeExprp, nullptr);
        अगर (pair.second) {
            AstVarScope* स्थिर prevp = crate_अस्थायी(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* स्थिर initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_वृक्ष(false)};
            m_results.m_inits.धकेलो_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* स्थिर prevp = pair.first->second;

        स्थिर स्वतः wr_पूर्व = [=]() { वापस new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        अगर (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_गाँठ_DType* स्थिर व्यंजक_Dtp = exprp->dtypep()->skipRefp();
            अगर (!isSupportedDType(व्यंजक_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << व्यंजक_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                वापस prevp;
            }
            अगर (VN_IS(व्यंजक_Dtp, UnpackArrayDType)) {
                AstCMethodHard* स्थिर cmhp
                    = new AstCMethodHard{flp, wr_पूर्व(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.धकेलो_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } अन्यथा {
                m_results.m_postUpdates.धकेलो_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wr_पूर्व(), rdCurr()}, baseRefp));
            }
        }

        वापस prevp;
    }

    std::pair<Ast_गाँठ_व्यंजक*, bool> createTerm(स्थिर AstSenItem* senItemp) {
        File_पंक्ति* स्थिर flp = senItemp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        स्थिर Ast_गाँठ_व्यंजक* स्थिर baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        स्थिर स्वतः currp = [this, senp]() { वापस getCurr(senp); };
        स्थिर स्वतः prevp
            = [this, flp, senp]() { वापस new AstVarRef{flp, get_पूर्व(senp), VAccess::READ}; };
        स्थिर स्वतः lsb = [=](Ast_गाँठ_व्यंजक* opp) { वापस new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        चयन (senItemp->edgeType()) {
        हाल VEdgeType::ET_CHANGED:
        हाल VEdgeType::ET_HYBRID:  //
            अगर (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* स्थिर resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                वापस {wrap_व्यंजक_With_Null_Check(flp, resultp, baseRefp), true};
            }
            वापस {wrap_व्यंजक_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        हाल VEdgeType::ET_BOTHEDGE:  //
            वापस {wrap_व्यंजक_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        हाल VEdgeType::ET_POSEDGE:  //
            वापस {wrap_व्यंजक_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        हाल VEdgeType::ET_NEGEDGE:  //
            वापस {wrap_व्यंजक_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        हाल VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_गाँठ_व्यंजक* स्थिर clearFromp = currp();
            अगर (Ast_गाँठ_Var_Ref* स्थिर refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), गाँठ_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* स्थिर clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_गाँठ_Stmt* स्थिर clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.धकेलो_back(clearStmtp);
            m_results.m_destructivePostUpdates.धकेलो_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* स्थिर callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_गाँठ_व्यंजक* स्थिर firedp = wrap_व्यंजक_With_Null_Check(flp, callp, baseRefp);
            अगर (स्थिर AstVarRef* स्थिर refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                अगर (refp->varScopep() == v3Global.rootp()->nbaEventp()) वापस {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_गाँठ_व्यंजक* स्थिर notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            वापस {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        हाल VEdgeType::ET_TRUE:  //
            वापस {currp(), false};
        हाल VEdgeType::ET_INITIAL_NBA:  //
            वापस {new AstConst{flp, AstConst::BitFalse{}}, true};
        शेष:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            वापस {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

खुला:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_गाँठ_व्यंजक*, bool> build(स्थिर AstSenItem* senItemp) {
        स्वतः term = createTerm(senItemp);
        अगर (Ast_गाँठ_व्यंजक* स्थिर condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_वृक्ष_Pure(false), term.first};
        }
        वापस term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_गाँठ_व्यंजक*, bool> build(स्थिर Ast_Sen_वृक्ष* senTreep) {
        File_पंक्ति* स्थिर flp = senTreep->fileline();
        Ast_गाँठ_व्यंजक* resultp = nullptr;
        bool firedAtInitialization = false;
        क्रम (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            स्थिर स्वतः& pair = build(senItemp);
            अगर (Ast_गाँठ_व्यंजक* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        वापस {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        वापस ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_पूर्व.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        वापस std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_व्यंजक_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#पूर्ण_अगर  // Guard
