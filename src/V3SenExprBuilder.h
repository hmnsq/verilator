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

class Sen_अभिव्यक्ती_Builder final {
सार्वजनिक:
    // TYPES
    संरचना Results final {
        std::vector<Ast_गाठ_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_गाठ_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_गाठ_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_गाठ_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

खाजगी:
    // STATE
    AstScope* स्थिरांक m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_गाठ>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_गाठ>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_गाठ>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_गाठ>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    स्थिर bool isSupportedDType(Ast_गाठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        जर (VN_IS(dtypep, BasicDType)) परत true;
        जर (VN_IS(dtypep, PackArrayDType)) परत true;
        जर (VN_IS(dtypep, UnpackArrayDType)) परत isSupportedDType(dtypep->subDTypep());
        जर (VN_IS(dtypep, गाठ_UOr_Struct_DType)) परत true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        जर (VN_IS(dtypep, ClassRefDType)) परत true;
        परत false;
    }

    स्थिर bool is_Simple_अभिव्यक्ती(स्थिरांक Ast_गाठ* स्थिरांक exprp) {
        परत exprp->forall([](स्थिरांक Ast_गाठ* स्थिरांक nodep) {
            परत VN_IS(nodep, Const) || VN_IS(nodep, गाठ_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, गाठ_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    स्थिर bool isNullableRef(स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक refp) {
        जर (!refp->dtypep()) परत false;
        स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = refp->dtypep()->skipRefp();
        स्थिरांक AstIfaceRefDType* स्थिरांक ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        परत VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    स्थिर bool hasNullableMemberAccess(स्थिरांक Ast_गाठ* स्थिरांक exprp) {
        परत exprp && exprp->exists([](स्थिरांक Ast_गाठ* स्थिरांक nodep) {
            स्थिरांक AstMemberSel* स्थिरांक mselp = VN_CAST(nodep, MemberSel);
            परत mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    स्थिर स्थिरांक Ast_गाठ_अभिव्यक्ती* getBaseNullableRef(स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp) {
        पर्यंत (exprp) {
            जर (स्थिरांक AstMemberSel* स्थिरांक mselp = VN_CAST(exprp, MemberSel)) {
                स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = mselp->fromp();
                जर (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    जर (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        सुरूठेवा;
                    }
                    परत fromp;
                }
                exprp = fromp;
            } अन्यथा {
                परत nullptr;
            }
        }
        परत nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_गाठ_अभिव्यक्ती* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        जर (AstVarRef* स्थिरांक refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* स्थिरांक vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } अन्यथा {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_ओळ* स्थिरांक flp = m_scopep->fileline();
        AstVar* स्थिरांक varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* स्थिरांक vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.ढकला_back(varp);
        परत vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_गाठ_Stmt* wrapStmtWithNullCheck(File_ओळ* flp, Ast_गाठ_Stmt* stmtp,
                                       स्थिरांक Ast_गाठ_अभिव्यक्ती* baseRefp) {
        जर (!baseRefp) परत stmtp;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_गाठ_अभिव्यक्ती* स्थिरांक checkp
            = new AstNeq{flp, const_cast<Ast_गाठ_अभिव्यक्ती*>(baseRefp)->clone_वृक्ष(false), nullp};
        परत new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_गाठ_अभिव्यक्ती* wrap_अभिव्यक्ती_With_Null_Check(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* exprp,
                                       स्थिरांक Ast_गाठ_अभिव्यक्ती* baseRefp) {
        जर (!baseRefp) परत exprp;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_गाठ_अभिव्यक्ती* स्थिरांक checkp
            = new AstNeq{flp, const_cast<Ast_गाठ_अभिव्यक्ती*>(baseRefp)->clone_वृक्ष(false), nullp};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        परत condp;
    }

    Ast_गाठ_अभिव्यक्ती* getCurr(Ast_गाठ_अभिव्यक्ती* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        जर (is_Simple_अभिव्यक्ती(exprp)) परत exprp->clone_वृक्ष(false);

        // Create the 'current value' variable
        File_ओळ* स्थिरांक flp = exprp->fileline();
        स्वतः result = m_curr.emplace(*exprp, nullptr);
        जर (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* स्थिरांक currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        जर (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.ढकला_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_वृक्ष(false)},
                                      baseRefp));
        }
        परत new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_गाठ_अभिव्यक्ती* exprp) {
        File_ओळ* स्थिरांक flp = exprp->fileline();
        स्थिरांक स्वतः rdCurr = [this, exprp]() { परत getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_गाठ* scopeExprp = exprp;
        जर (AstVarRef* स्थिरांक refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        स्थिरांक स्वतः pair = m_prev.emplace(*scopeExprp, nullptr);
        जर (pair.second) {
            AstVarScope* स्थिरांक prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* स्थिरांक initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_वृक्ष(false)};
            m_results.m_inits.ढकला_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* स्थिरांक prevp = pair.first->second;

        स्थिरांक स्वतः wrPrev = [=]() { परत new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        जर (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_गाठ_DType* स्थिरांक अभिव्यक्ती_Dtp = exprp->dtypep()->skipRefp();
            जर (!isSupportedDType(अभिव्यक्ती_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << अभिव्यक्ती_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                परत prevp;
            }
            जर (VN_IS(अभिव्यक्ती_Dtp, UnpackArrayDType)) {
                AstCMethodHard* स्थिरांक cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.ढकला_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } अन्यथा {
                m_results.m_postUpdates.ढकला_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        परत prevp;
    }

    std::pair<Ast_गाठ_अभिव्यक्ती*, bool> createTerm(स्थिरांक AstSenItem* senItemp) {
        File_ओळ* स्थिरांक flp = senItemp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        स्थिरांक स्वतः currp = [this, senp]() { परत getCurr(senp); };
        स्थिरांक स्वतः prevp
            = [this, flp, senp]() { परत new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        स्थिरांक स्वतः lsb = [=](Ast_गाठ_अभिव्यक्ती* opp) { परत new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        निवड (senItemp->edgeType()) {
        स्थिती VEdgeType::ET_CHANGED:
        स्थिती VEdgeType::ET_HYBRID:  //
            जर (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* स्थिरांक resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                परत {wrap_अभिव्यक्ती_With_Null_Check(flp, resultp, baseRefp), true};
            }
            परत {wrap_अभिव्यक्ती_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        स्थिती VEdgeType::ET_BOTHEDGE:  //
            परत {wrap_अभिव्यक्ती_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        स्थिती VEdgeType::ET_POSEDGE:  //
            परत {wrap_अभिव्यक्ती_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        स्थिती VEdgeType::ET_NEGEDGE:  //
            परत {wrap_अभिव्यक्ती_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        स्थिती VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_गाठ_अभिव्यक्ती* स्थिरांक clearFromp = currp();
            जर (Ast_गाठ_Var_Ref* स्थिरांक refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), गाठ_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* स्थिरांक clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_गाठ_Stmt* स्थिरांक clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.ढकला_back(clearStmtp);
            m_results.m_destructivePostUpdates.ढकला_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* स्थिरांक callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक firedp = wrap_अभिव्यक्ती_With_Null_Check(flp, callp, baseRefp);
            जर (स्थिरांक AstVarRef* स्थिरांक refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                जर (refp->varScopep() == v3Global.rootp()->nbaEventp()) परत {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_गाठ_अभिव्यक्ती* स्थिरांक notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            परत {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        स्थिती VEdgeType::ET_TRUE:  //
            परत {currp(), false};
        स्थिती VEdgeType::ET_INITIAL_NBA:  //
            परत {new AstConst{flp, AstConst::BitFalse{}}, true};
        पूर्वनिर्धारित:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            परत {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

सार्वजनिक:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_गाठ_अभिव्यक्ती*, bool> build(स्थिरांक AstSenItem* senItemp) {
        स्वतः term = createTerm(senItemp);
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_वृक्ष_Pure(false), term.first};
        }
        परत term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_गाठ_अभिव्यक्ती*, bool> build(स्थिरांक Ast_Sen_वृक्ष* senTreep) {
        File_ओळ* स्थिरांक flp = senTreep->fileline();
        Ast_गाठ_अभिव्यक्ती* resultp = nullptr;
        bool firedAtInitialization = false;
        साठी (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            स्थिरांक स्वतः& pair = build(senItemp);
            जर (Ast_गाठ_अभिव्यक्ती* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        परत {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        परत ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        परत std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_अभिव्यक्ती_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
