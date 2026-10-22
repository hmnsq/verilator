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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3SENEXPRBUILDER_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3SENEXPRBUILDER_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_ਵਿਅੰਜਕ_Builder final {
ਜਨਤਕ:
    // TYPES
    ਬਣਤਰ Results final {
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*> m_inits;  // Initialization statements for previous values
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*> m_preUpdates;  // Pre update assignments
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*> m_destructivePostUpdates;
        std::ਸਦਿਸ਼<AstVar*> m_vars;  // Created temporary variables
    };

ਨਿਜੀ:
    // STATE
    AstScope* ਅਚਲ m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_ਗੰਢ>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_ਗੰਢ>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_ਸੈੱਟ<VNRef<Ast_ਗੰਢ>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_ਸੈੱਟ<VNRef<Ast_ਗੰਢ>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3_Unique_ਨਾਂ m_prev_ਨਾਂ{"__Vtrigprevexpr"};  // For generating unique temporary names

    ਸਥਿਰ bool isSupportedDType(Ast_ਗੰਢ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ਜੇ (VN_IS(dtypep, BasicDType)) ਵਾਪਸ true;
        ਜੇ (VN_IS(dtypep, PackArrayDType)) ਵਾਪਸ true;
        ਜੇ (VN_IS(dtypep, UnpackArrayDType)) ਵਾਪਸ isSupportedDType(dtypep->subDTypep());
        ਜੇ (VN_IS(dtypep, ਗੰਢ_UOr_Struct_DType)) ਵਾਪਸ true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        ਜੇ (VN_IS(dtypep, ClassRefDType)) ਵਾਪਸ true;
        ਵਾਪਸ false;
    }

    ਸਥਿਰ bool is_Simple_ਵਿਅੰਜਕ(ਅਚਲ Ast_ਗੰਢ* ਅਚਲ exprp) {
        ਵਾਪਸ exprp->forall([](ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep) {
            ਵਾਪਸ VN_IS(nodep, Const) || VN_IS(nodep, ਗੰਢ_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, ਗੰਢ_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ਸਥਿਰ bool isNullableRef(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ refp) {
        ਜੇ (!refp->dtypep()) ਵਾਪਸ false;
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = refp->dtypep()->skipRefp();
        ਅਚਲ AstIfaceRefDType* ਅਚਲ ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        ਵਾਪਸ VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ਸਥਿਰ bool hasNullableMemberAccess(ਅਚਲ Ast_ਗੰਢ* ਅਚਲ exprp) {
        ਵਾਪਸ exprp && exprp->exists([](ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep) {
            ਅਚਲ AstMemberSel* ਅਚਲ mselp = VN_CAST(nodep, MemberSel);
            ਵਾਪਸ mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ਸਥਿਰ ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* get_ਅਧਾਰ_Nullable_Ref(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਜਦੋਂ (exprp) {
            ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ mselp = VN_CAST(exprp, MemberSel)) {
                ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = mselp->fromp();
                ਜੇ (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    ਜੇ (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        ਜਾਰੀ;
                    }
                    ਵਾਪਸ fromp;
                }
                exprp = fromp;
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ nullptr;
            }
        }
        ਵਾਪਸ nullptr;
    }

    // METHODS
    AstVarScope* crate_ਆਰਜ਼ੀ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ਅਚਲ vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prev_ਨਾਂ.get(name);
        } ਨਹੀਂਤਾਂ {
            name = m_prev_ਨਾਂ.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_ਸਤਰ* ਅਚਲ flp = m_scopep->fileline();
        AstVar* ਅਚਲ varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->is_ਅੰਦਰੂਨੀ(true);
        m_scopep->modp()->ਜੋੜੋ_Stmtsp(varp);
        AstVarScope* ਅਚਲ vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->ਜੋੜੋ_Varsp(vscp);
        m_results.m_vars.ਧੱਕੋ_ਪਿੱਠ(varp);
        ਵਾਪਸ vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_ਗੰਢ_Stmt* wrapStmtWithNullCheck(File_ਸਤਰ* flp, Ast_ਗੰਢ_Stmt* stmtp,
                                       ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਧਾਰ_Refp) {
        ਜੇ (!ਅਧਾਰ_Refp) ਵਾਪਸ stmtp;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ checkp
            = new AstNeq{flp, const_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਅਧਾਰ_Refp)->clone_ਰੁੱਖ(false), nullp};
        ਵਾਪਸ new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* wrap_ਵਿਅੰਜਕ_With_Null_Check(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                                       ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਧਾਰ_Refp) {
        ਜੇ (!ਅਧਾਰ_Refp) ਵਾਪਸ exprp;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ checkp
            = new AstNeq{flp, const_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਅਧਾਰ_Refp)->clone_ਰੁੱਖ(false), nullp};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ falsep = new AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        ਵਾਪਸ condp;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* getCurr(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        ਜੇ (is_Simple_ਵਿਅੰਜਕ(exprp)) ਵਾਪਸ exprp->clone_ਰੁੱਖ(false);

        // Create the 'current value' variable
        File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
        ਸਵੈਚਾਲਿਤ ਨਤੀਜਾ = m_curr.emplace(*exprp, nullptr);
        ਜੇ (ਨਤੀਜਾ.ਦੂਜਾ) ਨਤੀਜਾ.ਪਹਿਲਾ->ਦੂਜਾ = crate_ਆਰਜ਼ੀ(exprp);
        AstVarScope* ਅਚਲ currp = ਨਤੀਜਾ.ਪਹਿਲਾ->ਦੂਜਾ;

        // Check if we need null guards for a member access through a nullable reference
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਅਧਾਰ_Refp
            = hasNullableMemberAccess(exprp) ? get_ਅਧਾਰ_Nullable_Ref(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        ਜੇ (m_hasPreUpdate.emplace(*currp).ਦੂਜਾ) {
            m_results.m_preUpdates.ਧੱਕੋ_ਪਿੱਠ(
                wrapStmtWithNullCheck(flp,
                                      new Ast_ਸੌਂਪੋ{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_ਰੁੱਖ(false)},
                                      ਅਧਾਰ_Refp));
        }
        ਵਾਪਸ new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
        ਅਚਲ ਸਵੈਚਾਲਿਤ rdCurr = [this, exprp]() { ਵਾਪਸ getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਅਧਾਰ_Refp
            = hasNullableMemberAccess(exprp) ? get_ਅਧਾਰ_Nullable_Ref(exprp) : nullptr;

        Ast_ਗੰਢ* scopeExprp = exprp;
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_prev.emplace(*scopeExprp, nullptr);
        ਜੇ (pair.ਦੂਜਾ) {
            AstVarScope* ਅਚਲ prevp = crate_ਆਰਜ਼ੀ(exprp);
            pair.ਪਹਿਲਾ->ਦੂਜਾ = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            Ast_ਸੌਂਪੋ* ਅਚਲ initp = new Ast_ਸੌਂਪੋ{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_ਰੁੱਖ(false)};
            m_results.m_inits.ਧੱਕੋ_ਪਿੱਠ(wrapStmtWithNullCheck(flp, initp, ਅਧਾਰ_Refp));
        }

        AstVarScope* ਅਚਲ prevp = pair.ਪਹਿਲਾ->ਦੂਜਾ;

        ਅਚਲ ਸਵੈਚਾਲਿਤ wrPrev = [=]() { ਵਾਪਸ new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        ਜੇ (m_hasPostUpdate.emplace(*exprp).ਦੂਜਾ) {
            Ast_ਗੰਢ_DType* ਅਚਲ ਵਿਅੰਜਕ_Dtp = exprp->dtypep()->skipRefp();
            ਜੇ (!isSupportedDType(ਵਿਅੰਜਕ_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << ਵਿਅੰਜਕ_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warn_ਹੋਰ()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                ਵਾਪਸ prevp;
            }
            ਜੇ (VN_IS(ਵਿਅੰਜਕ_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ਅਚਲ cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ਸੌਂਪੋ, rdCurr()};
                cmhp->dtype_ਸੈੱਟ_Void();
                m_results.m_postUpdates.ਧੱਕੋ_ਪਿੱਠ(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), ਅਧਾਰ_Refp));
            } ਨਹੀਂਤਾਂ {
                m_results.m_postUpdates.ਧੱਕੋ_ਪਿੱਠ(
                    wrapStmtWithNullCheck(flp, new Ast_ਸੌਂਪੋ{flp, wrPrev(), rdCurr()}, ਅਧਾਰ_Refp));
            }
        }

        ਵਾਪਸ prevp;
    }

    std::pair<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, bool> create_ਪਦ(ਅਚਲ Ast_Sen_ਵਸਤੂ* senItemp) {
        File_ਸਤਰ* ਅਚਲ flp = senItemp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਅਧਾਰ_Refp
            = hasNullableMemberAccess(senp) ? get_ਅਧਾਰ_Nullable_Ref(senp) : nullptr;

        ਅਚਲ ਸਵੈਚਾਲਿਤ currp = [this, senp]() { ਵਾਪਸ getCurr(senp); };
        ਅਚਲ ਸਵੈਚਾਲਿਤ prevp
            = [this, flp, senp]() { ਵਾਪਸ new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ਅਚਲ ਸਵੈਚਾਲਿਤ lsb = [=](Ast_ਗੰਢ_ਵਿਅੰਜਕ* opp) { ਵਾਪਸ new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        ਚੋਣ (senItemp->ਕਿਨਾਰਾ_Type()) {
        ਹਾਲਤ VEdgeType::ET_ਬਦਲਿਆ:
        ਹਾਲਤ VEdgeType::ET_HYBRID:  //
            ਜੇ (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ਅਚਲ resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                ਵਾਪਸ {wrap_ਵਿਅੰਜਕ_With_Null_Check(flp, resultp, ਅਧਾਰ_Refp), true};
            }
            ਵਾਪਸ {wrap_ਵਿਅੰਜਕ_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, ਅਧਾਰ_Refp), true};
        ਹਾਲਤ VEdgeType::ET_BOTHEDGE:  //
            ਵਾਪਸ {wrap_ਵਿਅੰਜਕ_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), ਅਧਾਰ_Refp),
                    false};
        ਹਾਲਤ VEdgeType::ET_POSEDGE:  //
            ਵਾਪਸ {wrap_ਵਿਅੰਜਕ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), ਅਧਾਰ_Refp),
                    false};
        ਹਾਲਤ VEdgeType::ET_NEGEDGE:  //
            ਵਾਪਸ {wrap_ਵਿਅੰਜਕ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), ਅਧਾਰ_Refp),
                    false};
        ਹਾਲਤ VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ clearFromp = currp();
            ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp
                = VN_CAST(clearFromp->get_VAccess_ਨਿਸ਼ਾਨਾ_Recurse(), ਗੰਢ_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ਅਚਲ clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtype_ਸੈੱਟ_Void();
            Ast_ਗੰਢ_Stmt* ਅਚਲ clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), ਅਧਾਰ_Refp);
            m_results.m_postUpdates.ਧੱਕੋ_ਪਿੱਠ(clearStmtp);
            m_results.m_destructivePostUpdates.ਧੱਕੋ_ਪਿੱਠ(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ਅਚਲ callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ firedp = wrap_ਵਿਅੰਜਕ_With_Null_Check(flp, callp, ਅਧਾਰ_Refp);
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                ਜੇ (refp->varScopep() == v3Global.rootp()->nbaEventp()) ਵਾਪਸ {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            ਵਾਪਸ {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        ਹਾਲਤ VEdgeType::ET_TRUE:  //
            ਵਾਪਸ {currp(), false};
        ਹਾਲਤ VEdgeType::ET_INITIAL_NBA:  //
            ਵਾਪਸ {new AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}, true};
        ਮੂਲ:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            ਵਾਪਸ {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

ਜਨਤਕ:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, bool> build(ਅਚਲ Ast_Sen_ਵਸਤੂ* senItemp) {
        ਸਵੈਚਾਲਿਤ ਪਦ = create_ਪਦ(senItemp);
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = senItemp->condp()) {
            ਪਦ.ਪਹਿਲਾ = new AstAnd{senItemp->fileline(), condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(false), ਪਦ.ਪਹਿਲਾ};
        }
        ਵਾਪਸ ਪਦ;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_ਗੰਢ_ਵਿਅੰਜਕ*, bool> build(ਅਚਲ Ast_Sen_ਰੁੱਖ* senTreep) {
        File_ਸਤਰ* ਅਚਲ flp = senTreep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = nullptr;
        bool firedAtInitialization = false;
        ਲਈ (Ast_Sen_ਵਸਤੂ* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), Sen_ਵਸਤੂ)) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ& pair = build(senItemp);
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* termp = pair.ਪਹਿਲਾ) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.ਦੂਜਾ;
            }
        }
        ਵਾਪਸ {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        ਵਾਪਸ ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        ਵਾਪਸ std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_ਵਿਅੰਜਕ_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#ਜੇ_ਅੰਤ  // Guard
