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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    ಸಂರಚನಾ Results final {
        std::vector<Ast_ಗಂಟು_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_ಗಂಟು_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_ಗಂಟು_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_ಗಂಟು_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

ಗುಪ್ತ:
    // STATE
    AstScope* ಧ್ರುವ m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_ಗಂಟು>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_ಗಂಟು>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_ಗಂಟು>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_ಗಂಟು>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    ಸ್ಥಿರ bool isSupportedDType(Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಯದಿ (VN_IS(dtypep, BasicDType)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (VN_IS(dtypep, PackArrayDType)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (VN_IS(dtypep, UnpackArrayDType)) ಪ್ರತ್ಯಾಗಮ isSupportedDType(dtypep->subDTypep());
        ಯದಿ (VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType)) ಪ್ರತ್ಯಾಗಮ true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        ಯದಿ (VN_IS(dtypep, ClassRefDType)) ಪ್ರತ್ಯಾಗಮ true;
        ಪ್ರತ್ಯಾಗಮ false;
    }

    ಸ್ಥಿರ bool is_Simple_ಅಭಿವ್ಯಕ್ತಿ(ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ exprp) {
        ಪ್ರತ್ಯಾಗಮ exprp->forall([](ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep) {
            ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, Const) || VN_IS(nodep, ಗಂಟು_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, ಗಂಟು_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ಸ್ಥಿರ bool isNullableRef(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ refp) {
        ಯದಿ (!refp->dtypep()) ಪ್ರತ್ಯಾಗಮ false;
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = refp->dtypep()->skipRefp();
        ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        ಪ್ರತ್ಯಾಗಮ VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ಸ್ಥಿರ bool hasNullableMemberAccess(ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ exprp) {
        ಪ್ರತ್ಯಾಗಮ exprp && exprp->exists([](ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep) {
            ಧ್ರುವ AstMemberSel* ಧ್ರುವ mselp = VN_CAST(nodep, MemberSel);
            ಪ್ರತ್ಯಾಗಮ mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ಸ್ಥಿರ ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getBaseNullableRef(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಯಾವತ್ (exprp) {
            ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ mselp = VN_CAST(exprp, MemberSel)) {
                ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = mselp->fromp();
                ಯದಿ (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    ಯದಿ (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        ಅನುವರ್ತ;
                    }
                    ಪ್ರತ್ಯಾಗಮ fromp;
                }
                exprp = fromp;
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } ಅನ್ಯಥಾ {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_ಸಾಲು* ಧ್ರುವ flp = m_scopep->fileline();
        AstVar* ಧ್ರುವ varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* ಧ್ರುವ vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.ತಳ್ಳು_back(varp);
        ಪ್ರತ್ಯಾಗಮ vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_ಗಂಟು_Stmt* wrapStmtWithNullCheck(File_ಸಾಲು* flp, Ast_ಗಂಟು_Stmt* stmtp,
                                       ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* baseRefp) {
        ಯದಿ (!baseRefp) ಪ್ರತ್ಯಾಗಮ stmtp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ checkp
            = new AstNeq{flp, const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(baseRefp)->clone_ವೃಕ್ಷ(false), nullp};
        ಪ್ರತ್ಯಾಗಮ new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                                       ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* baseRefp) {
        ಯದಿ (!baseRefp) ಪ್ರತ್ಯಾಗಮ exprp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ checkp
            = new AstNeq{flp, const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(baseRefp)->clone_ವೃಕ್ಷ(false), nullp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        ಪ್ರತ್ಯಾಗಮ condp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getCurr(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        ಯದಿ (is_Simple_ಅಭಿವ್ಯಕ್ತಿ(exprp)) ಪ್ರತ್ಯಾಗಮ exprp->clone_ವೃಕ್ಷ(false);

        // Create the 'current value' variable
        File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        ಯದಿ (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* ಧ್ರುವ currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        ಯದಿ (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.ತಳ್ಳು_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_ವೃಕ್ಷ(false)},
                                      baseRefp));
        }
        ಪ್ರತ್ಯಾಗಮ new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
        ಧ್ರುವ auto rdCurr = [this, exprp]() { ಪ್ರತ್ಯಾಗಮ getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_ಗಂಟು* scopeExprp = exprp;
        ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ಧ್ರುವ auto pair = m_prev.emplace(*scopeExprp, nullptr);
        ಯದಿ (pair.second) {
            AstVarScope* ಧ್ರುವ prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* ಧ್ರುವ initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_ವೃಕ್ಷ(false)};
            m_results.m_inits.ತಳ್ಳು_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* ಧ್ರುವ prevp = pair.first->second;

        ಧ್ರುವ auto wrPrev = [=]() { ಪ್ರತ್ಯಾಗಮ new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        ಯದಿ (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Dtp = exprp->dtypep()->skipRefp();
            ಯದಿ (!isSupportedDType(ಅಭಿವ್ಯಕ್ತಿ_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << ಅಭಿವ್ಯಕ್ತಿ_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                ಪ್ರತ್ಯಾಗಮ prevp;
            }
            ಯದಿ (VN_IS(ಅಭಿವ್ಯಕ್ತಿ_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ಧ್ರುವ cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.ತಳ್ಳು_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } ಅನ್ಯಥಾ {
                m_results.m_postUpdates.ತಳ್ಳು_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        ಪ್ರತ್ಯಾಗಮ prevp;
    }

    std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, bool> createTerm(ಧ್ರುವ AstSenItem* senItemp) {
        File_ಸಾಲು* ಧ್ರುವ flp = senItemp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        ಧ್ರುವ auto currp = [this, senp]() { ಪ್ರತ್ಯಾಗಮ getCurr(senp); };
        ಧ್ರುವ auto prevp
            = [this, flp, senp]() { ಪ್ರತ್ಯಾಗಮ new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ಧ್ರುವ auto lsb = [=](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* opp) { ಪ್ರತ್ಯಾಗಮ new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        ವಿಕಲ್ಪ (senItemp->edgeType()) {
        ಪಕ್ಷ VEdgeType::ET_CHANGED:
        ಪಕ್ಷ VEdgeType::ET_HYBRID:  //
            ಯದಿ (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ಧ್ರುವ resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                ಪ್ರತ್ಯಾಗಮ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(flp, resultp, baseRefp), true};
            }
            ಪ್ರತ್ಯಾಗಮ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        ಪಕ್ಷ VEdgeType::ET_BOTHEDGE:  //
            ಪ್ರತ್ಯಾಗಮ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        ಪಕ್ಷ VEdgeType::ET_POSEDGE:  //
            ಪ್ರತ್ಯಾಗಮ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        ಪಕ್ಷ VEdgeType::ET_NEGEDGE:  //
            ಪ್ರತ್ಯಾಗಮ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        ಪಕ್ಷ VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ clearFromp = currp();
            ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), ಗಂಟು_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ಧ್ರುವ clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_ಗಂಟು_Stmt* ಧ್ರುವ clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.ತಳ್ಳು_back(clearStmtp);
            m_results.m_destructivePostUpdates.ತಳ್ಳು_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ಧ್ರುವ callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ firedp = wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(flp, callp, baseRefp);
            ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                ಯದಿ (refp->varScopep() == v3Global.rootp()->nbaEventp()) ಪ್ರತ್ಯಾಗಮ {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            ಪ್ರತ್ಯಾಗಮ {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        ಪಕ್ಷ VEdgeType::ET_TRUE:  //
            ಪ್ರತ್ಯಾಗಮ {currp(), false};
        ಪಕ್ಷ VEdgeType::ET_INITIAL_NBA:  //
            ಪ್ರತ್ಯಾಗಮ {new AstConst{flp, AstConst::BitFalse{}}, true};
        ಸ್ವಭಾವ:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            ಪ್ರತ್ಯಾಗಮ {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

ಸಾರ್ವಜನಿಕ:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, bool> build(ಧ್ರುವ AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_ವೃಕ್ಷ_Pure(false), term.first};
        }
        ಪ್ರತ್ಯಾಗಮ term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, bool> build(ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* senTreep) {
        File_ಸಾಲು* ಧ್ರುವ flp = senTreep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = nullptr;
        bool firedAtInitialization = false;
        ಪ್ರತಿ (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            ಧ್ರುವ auto& pair = build(senItemp);
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        ಪ್ರತ್ಯಾಗಮ {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        ಪ್ರತ್ಯಾಗಮ ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        ಪ್ರತ್ಯಾಗಮ std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
