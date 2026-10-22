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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3SENEXPRBUILDER_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3SENEXPRBUILDER_H_

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
    ರಚನೆ Results final {
        std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*> m_inits;  // Initialization statements for previous values
        std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*> m_preUpdates;  // Pre update assignments
        std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*> m_destructivePostUpdates;
        std::ಸದಿಶ<AstVar*> m_vars;  // Created temporary variables
    };

ಖಾಸಗಿ:
    // STATE
    AstScope* ಸ್ಥಿರಾಂಕ m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_ಗಂಟು>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_ಗಂಟು>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_ಗಣ<VNRef<Ast_ಗಂಟು>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_ಗಣ<VNRef<Ast_ಗಂಟು>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3_Unique_ಹೆಸರುಗಳು m_prev_ಹೆಸರುಗಳು{"__Vtrigprevexpr"};  // For generating unique temporary names

    ಸ್ಥಿರ bool isSupportedDType(Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಆದರೆ (VN_IS(dtypep, BasicDType)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (VN_IS(dtypep, PackArrayDType)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (VN_IS(dtypep, UnpackArrayDType)) ಹಿಂತಿರುಗಿ isSupportedDType(dtypep->subDTypep());
        ಆದರೆ (VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType)) ಹಿಂತಿರುಗಿ true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        ಆದರೆ (VN_IS(dtypep, ClassRefDType)) ಹಿಂತಿರುಗಿ true;
        ಹಿಂತಿರುಗಿ false;
    }

    ಸ್ಥಿರ bool is_Simple_ಅಭಿವ್ಯಕ್ತಿ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ exprp) {
        ಹಿಂತಿರುಗಿ exprp->forall([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
            ಹಿಂತಿರುಗಿ VN_IS(nodep, Const) || VN_IS(nodep, ಗಂಟು_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, ಗಂಟು_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ಸ್ಥಿರ bool isNullableRef(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ refp) {
        ಆದರೆ (!refp->dtypep()) ಹಿಂತಿರುಗಿ false;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = refp->dtypep()->skipRefp();
        ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        ಹಿಂತಿರುಗಿ VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ಸ್ಥಿರ bool hasNullableMemberAccess(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ exprp) {
        ಹಿಂತಿರುಗಿ exprp && exprp->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
            ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ mselp = VN_CAST(nodep, MemberSel);
            ಹಿಂತಿರುಗಿ mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* get_ತಳ_Nullable_Ref(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಆಗ (exprp) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ mselp = VN_CAST(exprp, MemberSel)) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = mselp->fromp();
                ಆದರೆ (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    ಆದರೆ (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        ಮುಂದುವರಿ;
                    }
                    ಹಿಂತಿರುಗಿ fromp;
                }
                exprp = fromp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ nullptr;
            }
        }
        ಹಿಂತಿರುಗಿ nullptr;
    }

    // METHODS
    AstVarScope* crate_ತಾತ್ಕಾಲಿಕ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prev_ಹೆಸರುಗಳು.get(name);
        } ಇಲ್ಲದಿದ್ದರೆ {
            name = m_prev_ಹೆಸರುಗಳು.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_scopep->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->is_ಆಂತರಿಕ(true);
        m_scopep->modp()->ಕೂಡಿಸು_Stmtsp(varp);
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->ಕೂಡಿಸು_Varsp(vscp);
        m_results.m_vars.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
        ಹಿಂತಿರುಗಿ vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_ಗಂಟು_Stmt* wrapStmtWithNullCheck(File_ಸಾಲು* flp, Ast_ಗಂಟು_Stmt* stmtp,
                                       ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತಳ_Refp) {
        ಆದರೆ (!ತಳ_Refp) ಹಿಂತಿರುಗಿ stmtp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ checkp
            = new AstNeq{flp, const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ತಳ_Refp)->clone_ವೃಕ್ಷ(false), nullp};
        ಹಿಂತಿರುಗಿ new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                                       ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತಳ_Refp) {
        ಆದರೆ (!ತಳ_Refp) ಹಿಂತಿರುಗಿ exprp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ checkp
            = new AstNeq{flp, const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ತಳ_Refp)->clone_ವೃಕ್ಷ(false), nullp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ falsep = new AstConst{flp, AstConst::ಬಿಟ್_False{}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtype_ಗಣ_ಬಿಟ್();
        ಹಿಂತಿರುಗಿ condp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getCurr(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        ಆದರೆ (is_Simple_ಅಭಿವ್ಯಕ್ತಿ(exprp)) ಹಿಂತಿರುಗಿ exprp->clone_ವೃಕ್ಷ(false);

        // Create the 'current value' variable
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
        ಸ್ವಯಂಚಾಲಿತ ಫಲಿತಾಂಶ = m_curr.emplace(*exprp, nullptr);
        ಆದರೆ (ಫಲಿತಾಂಶ.ಎರಡನೇ) ಫಲಿತಾಂಶ.ಮೊದಲ->ಎರಡನೇ = crate_ತಾತ್ಕಾಲಿಕ(exprp);
        AstVarScope* ಸ್ಥಿರಾಂಕ currp = ಫಲಿತಾಂಶ.ಮೊದಲ->ಎರಡನೇ;

        // Check if we need null guards for a member access through a nullable reference
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ತಳ_Refp
            = hasNullableMemberAccess(exprp) ? get_ತಳ_Nullable_Ref(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        ಆದರೆ (m_hasPreUpdate.emplace(*currp).ಎರಡನೇ) {
            m_results.m_preUpdates.ತಳ್ಳು_ಹಿಂಭಾಗ(
                wrapStmtWithNullCheck(flp,
                                      new Ast_ನಿಯೋಜಿಸು{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_ವೃಕ್ಷ(false)},
                                      ತಳ_Refp));
        }
        ಹಿಂತಿರುಗಿ new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ rdCurr = [this, exprp]() { ಹಿಂತಿರುಗಿ getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ತಳ_Refp
            = hasNullableMemberAccess(exprp) ? get_ತಳ_Nullable_Ref(exprp) : nullptr;

        Ast_ಗಂಟು* scopeExprp = exprp;
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_prev.emplace(*scopeExprp, nullptr);
        ಆದರೆ (pair.ಎರಡನೇ) {
            AstVarScope* ಸ್ಥಿರಾಂಕ prevp = crate_ತಾತ್ಕಾಲಿಕ(exprp);
            pair.ಮೊದಲ->ಎರಡನೇ = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ initp = new Ast_ನಿಯೋಜಿಸು{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_ವೃಕ್ಷ(false)};
            m_results.m_inits.ತಳ್ಳು_ಹಿಂಭಾಗ(wrapStmtWithNullCheck(flp, initp, ತಳ_Refp));
        }

        AstVarScope* ಸ್ಥಿರಾಂಕ prevp = pair.ಮೊದಲ->ಎರಡನೇ;

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ wrPrev = [=]() { ಹಿಂತಿರುಗಿ new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        ಆದರೆ (m_hasPostUpdate.emplace(*exprp).ಎರಡನೇ) {
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Dtp = exprp->dtypep()->skipRefp();
            ಆದರೆ (!isSupportedDType(ಅಭಿವ್ಯಕ್ತಿ_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << ಅಭಿವ್ಯಕ್ತಿ_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warn_ಇನ್ನಷ್ಟು()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                ಹಿಂತಿರುಗಿ prevp;
            }
            ಆದರೆ (VN_IS(ಅಭಿವ್ಯಕ್ತಿ_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ಸ್ಥಿರಾಂಕ cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ನಿಯೋಜಿಸು, rdCurr()};
                cmhp->dtype_ಗಣ_Void();
                m_results.m_postUpdates.ತಳ್ಳು_ಹಿಂಭಾಗ(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), ತಳ_Refp));
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_results.m_postUpdates.ತಳ್ಳು_ಹಿಂಭಾಗ(
                    wrapStmtWithNullCheck(flp, new Ast_ನಿಯೋಜಿಸು{flp, wrPrev(), rdCurr()}, ತಳ_Refp));
            }
        }

        ಹಿಂತಿರುಗಿ prevp;
    }

    std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, bool> create_ಪದ(ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* senItemp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = senItemp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ತಳ_Refp
            = hasNullableMemberAccess(senp) ? get_ತಳ_Nullable_Ref(senp) : nullptr;

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ currp = [this, senp]() { ಹಿಂತಿರುಗಿ getCurr(senp); };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ prevp
            = [this, flp, senp]() { ಹಿಂತಿರುಗಿ new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ lsb = [=](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* opp) { ಹಿಂತಿರುಗಿ new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        ಆಯ್ಕೆ (senItemp->ಅಂಚು_Type()) {
        ಸ್ಥಿತಿ VEdgeType::ET_ಬದಲಾಯಿತು:
        ಸ್ಥಿತಿ VEdgeType::ET_HYBRID:  //
            ಆದರೆ (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ಸ್ಥಿರಾಂಕ resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtype_ಗಣ_ಬಿಟ್();
                ಹಿಂತಿರುಗಿ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(flp, resultp, ತಳ_Refp), true};
            }
            ಹಿಂತಿರುಗಿ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, ತಳ_Refp), true};
        ಸ್ಥಿತಿ VEdgeType::ET_BOTHEDGE:  //
            ಹಿಂತಿರುಗಿ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), ತಳ_Refp),
                    false};
        ಸ್ಥಿತಿ VEdgeType::ET_POSEDGE:  //
            ಹಿಂತಿರುಗಿ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), ತಳ_Refp),
                    false};
        ಸ್ಥಿತಿ VEdgeType::ET_NEGEDGE:  //
            ಹಿಂತಿರುಗಿ {wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), ತಳ_Refp),
                    false};
        ಸ್ಥಿತಿ VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ clearFromp = currp();
            ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp
                = VN_CAST(clearFromp->get_VAccess_ಗುರಿ_Recurse(), ಗಂಟು_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ಸ್ಥಿರಾಂಕ clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtype_ಗಣ_Void();
            Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), ತಳ_Refp);
            m_results.m_postUpdates.ತಳ್ಳು_ಹಿಂಭಾಗ(clearStmtp);
            m_results.m_destructivePostUpdates.ತಳ್ಳು_ಹಿಂಭಾಗ(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ಸ್ಥಿರಾಂಕ callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtype_ಗಣ_ಬಿಟ್();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ firedp = wrap_ಅಭಿವ್ಯಕ್ತಿ_With_Null_Check(flp, callp, ತಳ_Refp);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                ಆದರೆ (refp->varScopep() == v3Global.rootp()->nbaEventp()) ಹಿಂತಿರುಗಿ {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            ಹಿಂತಿರುಗಿ {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        ಸ್ಥಿತಿ VEdgeType::ET_TRUE:  //
            ಹಿಂತಿರುಗಿ {currp(), false};
        ಸ್ಥಿತಿ VEdgeType::ET_INITIAL_NBA:  //
            ಹಿಂತಿರುಗಿ {new AstConst{flp, AstConst::ಬಿಟ್_False{}}, true};
        ಪೂರ್ವನಿಯೋಜಿತ:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            ಹಿಂತಿರುಗಿ {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

ಸಾರ್ವಜನಿಕ:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, bool> build(ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* senItemp) {
        ಸ್ವಯಂಚಾಲಿತ ಪದ = create_ಪದ(senItemp);
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = senItemp->condp()) {
            ಪದ.ಮೊದಲ = new AstAnd{senItemp->fileline(), condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(false), ಪದ.ಮೊದಲ};
        }
        ಹಿಂತಿರುಗಿ ಪದ;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*, bool> build(ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* senTreep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = senTreep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = nullptr;
        bool firedAtInitialization = false;
        ಗಾಗಿ (Ast_Sen_ಅಂಶ* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), Sen_ಅಂಶ)) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair = build(senItemp);
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* termp = pair.ಮೊದಲ) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.ಎರಡನೇ;
            }
        }
        ಹಿಂತಿರುಗಿ {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        ಹಿಂತಿರುಗಿ ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        ಹಿಂತಿರುಗಿ std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
