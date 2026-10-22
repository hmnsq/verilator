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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class Sen_ପଦସମଷ୍ଟି_Builder final {
ସର୍ବସାଧାରଣ:
    // TYPES
    ସଂରଚନା Results final {
        std::vector<Ast_ଗଣ୍ଠି_Stmt*> m_inits;  // Initialization statements for previous values
        std::vector<Ast_ଗଣ୍ଠି_Stmt*> m_preUpdates;  // Pre update assignments
        std::vector<Ast_ଗଣ୍ଠି_Stmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<Ast_ଗଣ୍ଠି_Stmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

ବ୍ୟକ୍ତିଗତ:
    // STATE
    AstScope* ଅଚଳ m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<Ast_ଗଣ୍ଠି>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<Ast_ଗଣ୍ଠି>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<Ast_ଗଣ୍ଠି>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<Ast_ଗଣ୍ଠି>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    ସ୍ଥିର bool isSupportedDType(Ast_ଗଣ୍ଠି_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ଯଦି (VN_IS(dtypep, BasicDType)) ଫେରନ୍ତୁ true;
        ଯଦି (VN_IS(dtypep, PackArrayDType)) ଫେରନ୍ତୁ true;
        ଯଦି (VN_IS(dtypep, UnpackArrayDType)) ଫେରନ୍ତୁ isSupportedDType(dtypep->subDTypep());
        ଯଦି (VN_IS(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) ଫେରନ୍ତୁ true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        ଯଦି (VN_IS(dtypep, ClassRefDType)) ଫେରନ୍ତୁ true;
        ଫେରନ୍ତୁ false;
    }

    ସ୍ଥିର bool is_Simple_ପଦସମଷ୍ଟି(ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ exprp) {
        ଫେରନ୍ତୁ exprp->forall([](ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
            ଫେରନ୍ତୁ VN_IS(nodep, Const) || VN_IS(nodep, ଗଣ୍ଠି_Var_Ref) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, ଗଣ୍ଠି_Sel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    ସ୍ଥିର bool isNullableRef(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ refp) {
        ଯଦି (!refp->dtypep()) ଫେରନ୍ତୁ false;
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = refp->dtypep()->skipRefp();
        ଅଚଳ AstIfaceRefDType* ଅଚଳ ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        ଫେରନ୍ତୁ VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    ସ୍ଥିର bool hasNullableMemberAccess(ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ exprp) {
        ଫେରନ୍ତୁ exprp && exprp->exists([](ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
            ଅଚଳ AstMemberSel* ଅଚଳ mselp = VN_CAST(nodep, MemberSel);
            ଫେରନ୍ତୁ mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    ସ୍ଥିର ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getBaseNullableRef(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଯେତେବେଳେ (exprp) {
            ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ mselp = VN_CAST(exprp, MemberSel)) {
                ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = mselp->fromp();
                ଯଦି (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    ଯଦି (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        ଜାରି;
                    }
                    ଫେରନ୍ତୁ fromp;
                }
                exprp = fromp;
            } ଅନ୍ୟଥା {
                ଫେରନ୍ତୁ nullptr;
            }
        }
        ଫେରନ୍ତୁ nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* ଅଚଳ vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } ଅନ୍ୟଥା {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        File_ଧାଡ଼ି* ଅଚଳ flp = m_scopep->fileline();
        AstVar* ଅଚଳ varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* ଅଚଳ vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.ଠେଲ_back(varp);
        ଫେରନ୍ତୁ vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    Ast_ଗଣ୍ଠି_Stmt* wrapStmtWithNullCheck(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_Stmt* stmtp,
                                       ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* baseRefp) {
        ଯଦି (!baseRefp) ଫେରନ୍ତୁ stmtp;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ checkp
            = new AstNeq{flp, const_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(baseRefp)->clone_ବୃକ୍ଷ(false), nullp};
        ଫେରନ୍ତୁ new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* wrap_ପଦସମଷ୍ଟି_With_Null_Check(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                                       ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* baseRefp) {
        ଯଦି (!baseRefp) ଫେରନ୍ତୁ exprp;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ checkp
            = new AstNeq{flp, const_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(baseRefp)->clone_ବୃକ୍ଷ(false), nullp};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ falsep = new AstConst{flp, AstConst::BitFalse{}};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        ଫେରନ୍ତୁ condp;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getCurr(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        ଯଦି (is_Simple_ପଦସମଷ୍ଟି(exprp)) ଫେରନ୍ତୁ exprp->clone_ବୃକ୍ଷ(false);

        // Create the 'current value' variable
        File_ଧାଡ଼ି* ଅଚଳ flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        ଯଦି (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* ଅଚଳ currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        ଯଦି (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.ଠେଲ_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->clone_ବୃକ୍ଷ(false)},
                                      baseRefp));
        }
        ଫେରନ୍ତୁ new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = exprp->fileline();
        ଅଚଳ auto rdCurr = [this, exprp]() { ଫେରନ୍ତୁ getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        Ast_ଗଣ୍ଠି* scopeExprp = exprp;
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        ଅଚଳ auto pair = m_prev.emplace(*scopeExprp, nullptr);
        ଯଦି (pair.second) {
            AstVarScope* ଅଚଳ prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* ଅଚଳ initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->clone_ବୃକ୍ଷ(false)};
            m_results.m_inits.ଠେଲ_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* ଅଚଳ prevp = pair.first->second;

        ଅଚଳ auto wrPrev = [=]() { ଫେରନ୍ତୁ new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        ଯଦି (m_hasPostUpdate.emplace(*exprp).second) {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ ପଦସମଷ୍ଟି_Dtp = exprp->dtypep()->skipRefp();
            ଯଦି (!isSupportedDType(ପଦସମଷ୍ଟି_Dtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << ପଦସମଷ୍ଟି_Dtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                ଫେରନ୍ତୁ prevp;
            }
            ଯଦି (VN_IS(ପଦସମଷ୍ଟି_Dtp, UnpackArrayDType)) {
                AstCMethodHard* ଅଚଳ cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.ଠେଲ_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } ଅନ୍ୟଥା {
                m_results.m_postUpdates.ଠେଲ_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        ଫେରନ୍ତୁ prevp;
    }

    std::pair<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, bool> createTerm(ଅଚଳ AstSenItem* senItemp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = senItemp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        ଅଚଳ auto currp = [this, senp]() { ଫେରନ୍ତୁ getCurr(senp); };
        ଅଚଳ auto prevp
            = [this, flp, senp]() { ଫେରନ୍ତୁ new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        ଅଚଳ auto lsb = [=](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* opp) { ଫେରନ୍ତୁ new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        ବାଛ (senItemp->edgeType()) {
        ସ୍ଥିତି VEdgeType::ET_CHANGED:
        ସ୍ଥିତି VEdgeType::ET_HYBRID:  //
            ଯଦି (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* ଅଚଳ resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                ଫେରନ୍ତୁ {wrap_ପଦସମଷ୍ଟି_With_Null_Check(flp, resultp, baseRefp), true};
            }
            ଫେରନ୍ତୁ {wrap_ପଦସମଷ୍ଟି_With_Null_Check(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        ସ୍ଥିତି VEdgeType::ET_BOTHEDGE:  //
            ଫେରନ୍ତୁ {wrap_ପଦସମଷ୍ଟି_With_Null_Check(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        ସ୍ଥିତି VEdgeType::ET_POSEDGE:  //
            ଫେରନ୍ତୁ {wrap_ପଦସମଷ୍ଟି_With_Null_Check(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        ସ୍ଥିତି VEdgeType::ET_NEGEDGE:  //
            ଫେରନ୍ତୁ {wrap_ପଦସମଷ୍ଟି_With_Null_Check(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        ସ୍ଥିତି VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ clearFromp = currp();
            ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), ଗଣ୍ଠି_Var_Ref)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* ଅଚଳ clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.ଠେଲ_back(clearStmtp);
            m_results.m_destructivePostUpdates.ଠେଲ_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* ଅଚଳ callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ firedp = wrap_ପଦସମଷ୍ଟି_With_Null_Check(flp, callp, baseRefp);
            ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                ଯଦି (refp->varScopep() == v3Global.rootp()->nbaEventp()) ଫେରନ୍ତୁ {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            ଫେରନ୍ତୁ {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        ସ୍ଥିତି VEdgeType::ET_TRUE:  //
            ଫେରନ୍ତୁ {currp(), false};
        ସ୍ଥିତି VEdgeType::ET_INITIAL_NBA:  //
            ଫେରନ୍ତୁ {new AstConst{flp, AstConst::BitFalse{}}, true};
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            ଫେରନ୍ତୁ {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

ସର୍ବସାଧାରଣ:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, bool> build(ଅଚଳ AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->clone_ବୃକ୍ଷ_Pure(false), term.first};
        }
        ଫେରନ୍ତୁ term;
    }

    // Like above, but for a whole SenTree
    std::pair<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*, bool> build(ଅଚଳ Ast_Sen_ବୃକ୍ଷ* senTreep) {
        File_ଧାଡ଼ି* ଅଚଳ flp = senTreep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = nullptr;
        bool firedAtInitialization = false;
        ପାଇଁ (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            ଅଚଳ auto& pair = build(senItemp);
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        ଫେରନ୍ତୁ {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        ଫେରନ୍ତୁ ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        ଫେରନ୍ତୁ std::move(m_results);
    }

    // CONSTRUCTOR
    explicit Sen_ପଦସମଷ୍ଟି_Builder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
