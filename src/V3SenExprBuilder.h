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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3UniqueNames.h"

//######################################################################
// SenExprBuilder constructs the expressions used to compute if an
// AstSenTree have triggered

class SenExprBuilder final {
public:
    // TYPES
    sa_nracanaa Results final {
        std::vector<AstNodeStmt*> m_inits;  // Initialization statements for previous values
        std::vector<AstNodeStmt*> m_preUpdates;  // Pre update assignments
        std::vector<AstNodeStmt*> m_postUpdates;  // Post update assignments
        // Aliases of destructive post updates (for example event clearFired) that may need
        // to run once before entering a fresh dynamic wait loop.
        std::vector<AstNodeStmt*> m_destructivePostUpdates;
        std::vector<AstVar*> m_vars;  // Created temporary variables
    };

private:
    // STATE
    AstScope* const m_scopep;  // The scope

    Results m_results;  // The builder result

    std::unordered_map<VNRef<AstNode>, AstVarScope*> m_prev;  // The 'previous value' signals
    std::unordered_map<VNRef<AstNode>, AstVarScope*> m_curr;  // The 'current value' signals
    std::unordered_set<VNRef<AstNode>> m_hasPreUpdate;  // Whether the given sen expression already
                                                        // has an update statement in m_preUpdates
    std::unordered_set<VNRef<AstNode>> m_hasPostUpdate;  // Likewise for m_postUpdates

    V3UniqueNames m_prevNames{"__Vtrigprevexpr"};  // For generating unique temporary names

    static bool isSupportedDType(AstNodeDType* dtypep) {
        dtypep = dtypep->skipRefp();
        ຖ້າ (VN_IS(dtypep, BasicDType)) ສົ່ງຄືນ true;
        ຖ້າ (VN_IS(dtypep, PackArrayDType)) ສົ່ງຄືນ true;
        ຖ້າ (VN_IS(dtypep, UnpackArrayDType)) ສົ່ງຄືນ isSupportedDType(dtypep->subDTypep());
        ຖ້າ (VN_IS(dtypep, NodeUOrStructDType)) ສົ່ງຄືນ true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        ຖ້າ (VN_IS(dtypep, ClassRefDType)) ສົ່ງຄືນ true;
        ສົ່ງຄືນ false;
    }

    static bool isSimpleExpr(const AstNode* const exprp) {
        ສົ່ງຄືນ exprp->forall([](const AstNode* const nodep) {
            ສົ່ງຄືນ VN_IS(nodep, Const) || VN_IS(nodep, NodeVarRef) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, NodeSel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    static bool isNullableRef(const AstNodeExpr* const refp) {
        ຖ້າ (!refp->dtypep()) ສົ່ງຄືນ false;
        const AstNodeDType* const dtypep = refp->dtypep()->skipRefp();
        const AstIfaceRefDType* const ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        ສົ່ງຄືນ VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    static bool hasNullableMemberAccess(const AstNode* const exprp) {
        ສົ່ງຄືນ exprp && exprp->exists([](const AstNode* const nodep) {
            const AstMemberSel* const mselp = VN_CAST(nodep, MemberSel);
            ສົ່ງຄືນ mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    static const AstNodeExpr* getBaseNullableRef(const AstNodeExpr* exprp) {
        ໃນຂະນະທີ່ (exprp) {
            ຖ້າ (const AstMemberSel* const mselp = VN_CAST(exprp, MemberSel)) {
                const AstNodeExpr* const fromp = mselp->fromp();
                ຖ້າ (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    ຖ້າ (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        continue;
                    }
                    ສົ່ງຄືນ fromp;
                }
                exprp = fromp;
            } ບໍ່ດັ່ງນັ້ນ {
                ສົ່ງຄືນ nullptr;
            }
        }
        ສົ່ງຄືນ nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(AstNodeExpr* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        ຖ້າ (AstVarRef* const refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* const vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } ບໍ່ດັ່ງນັ້ນ {
            name = m_prevNames.get(exprp);
        }
        // Not shared between instances, as the users might move the variables into processes
        FileLine* const flp = m_scopep->fileline();
        AstVar* const varp = new AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        varp->isInternal(true);
        m_scopep->modp()->addStmtsp(varp);
        AstVarScope* const vscp = new AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        m_results.m_vars.push_back(varp);
        ສົ່ງຄືນ vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    AstNodeStmt* wrapStmtWithNullCheck(FileLine* flp, AstNodeStmt* stmtp,
                                       const AstNodeExpr* baseRefp) {
        ຖ້າ (!baseRefp) ສົ່ງຄືນ stmtp;
        AstNodeExpr* const nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        AstNodeExpr* const checkp
            = new AstNeq{flp, const_cast<AstNodeExpr*>(baseRefp)->cloneTree(false), nullp};
        ສົ່ງຄືນ new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    AstNodeExpr* wrapExprWithNullCheck(FileLine* flp, AstNodeExpr* exprp,
                                       const AstNodeExpr* baseRefp) {
        ຖ້າ (!baseRefp) ສົ່ງຄືນ exprp;
        AstNodeExpr* const nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        AstNodeExpr* const checkp
            = new AstNeq{flp, const_cast<AstNodeExpr*>(baseRefp)->cloneTree(false), nullp};
        AstNodeExpr* const falsep = new AstConst{flp, AstConst::BitFalse{}};
        AstNodeExpr* const condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        ສົ່ງຄືນ condp;
    }

    AstNodeExpr* getCurr(AstNodeExpr* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        ຖ້າ (isSimpleExpr(exprp)) ສົ່ງຄືນ exprp->cloneTree(false);

        // Create the 'current value' variable
        FileLine* const flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        ຖ້າ (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* const currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        const AstNodeExpr* const baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        ຖ້າ (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.push_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->cloneTree(false)},
                                      baseRefp));
        }
        ສົ່ງຄືນ new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(AstNodeExpr* exprp) {
        FileLine* const flp = exprp->fileline();
        const auto rdCurr = [this, exprp]() { ສົ່ງຄືນ getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        const AstNodeExpr* const baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        AstNode* scopeExprp = exprp;
        ຖ້າ (AstVarRef* const refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        const auto pair = m_prev.emplace(*scopeExprp, nullptr);
        ຖ້າ (pair.second) {
            AstVarScope* const prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* const initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->cloneTree(false)};
            m_results.m_inits.push_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* const prevp = pair.first->second;

        const auto wrPrev = [=]() { ສົ່ງຄືນ new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        ຖ້າ (m_hasPostUpdate.emplace(*exprp).second) {
            AstNodeDType* const exprDtp = exprp->dtypep()->skipRefp();
            ຖ້າ (!isSupportedDType(exprDtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << exprDtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                ສົ່ງຄືນ prevp;
            }
            ຖ້າ (VN_IS(exprDtp, UnpackArrayDType)) {
                AstCMethodHard* const cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.push_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } ບໍ່ດັ່ງນັ້ນ {
                m_results.m_postUpdates.push_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        ສົ່ງຄືນ prevp;
    }

    std::pair<AstNodeExpr*, bool> createTerm(const AstSenItem* senItemp) {
        FileLine* const flp = senItemp->fileline();
        AstNodeExpr* const senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        const AstNodeExpr* const baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        const auto currp = [this, senp]() { ສົ່ງຄືນ getCurr(senp); };
        const auto prevp
            = [this, flp, senp]() { ສົ່ງຄືນ new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        const auto lsb = [=](AstNodeExpr* opp) { ສົ່ງຄືນ new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        switch (senItemp->edgeType()) {
        pak_sa VEdgeType::ET_CHANGED:
        pak_sa VEdgeType::ET_HYBRID:  //
            ຖ້າ (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* const resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                ສົ່ງຄືນ {wrapExprWithNullCheck(flp, resultp, baseRefp), true};
            }
            ສົ່ງຄືນ {wrapExprWithNullCheck(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        pak_sa VEdgeType::ET_BOTHEDGE:  //
            ສົ່ງຄືນ {wrapExprWithNullCheck(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        pak_sa VEdgeType::ET_POSEDGE:  //
            ສົ່ງຄືນ {wrapExprWithNullCheck(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        pak_sa VEdgeType::ET_NEGEDGE:  //
            ສົ່ງຄືນ {wrapExprWithNullCheck(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        pak_sa VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            AstNodeExpr* const clearFromp = currp();
            ຖ້າ (AstNodeVarRef* const refp
                = VN_CAST(clearFromp->getVAccessTargetRecurse(), NodeVarRef)) {
                refp->access(VAccess::WRITE);
            }
            AstCMethodHard* const clearp
                = new AstCMethodHard{flp, clearFromp, VCMethod::EVENT_CLEAR_FIRED};
            clearp->dtypeSetVoid();
            AstNodeStmt* const clearStmtp
                = wrapStmtWithNullCheck(flp, clearp->makeStmt(), baseRefp);
            m_results.m_postUpdates.push_back(clearStmtp);
            m_results.m_destructivePostUpdates.push_back(clearStmtp);

            // Get 'fired' state
            AstCMethodHard* const callp
                = new AstCMethodHard{flp, currp(), VCMethod::EVENT_IS_FIRED};
            callp->dtypeSetBit();
            AstNodeExpr* const firedp = wrapExprWithNullCheck(flp, callp, baseRefp);
            ຖ້າ (const AstVarRef* const refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                ຖ້າ (refp->varScopep() == v3Global.rootp()->nbaEventp()) ສົ່ງຄືນ {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            AstNodeExpr* const notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            ສົ່ງຄືນ {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        pak_sa VEdgeType::ET_TRUE:  //
            ສົ່ງຄືນ {currp(), false};
        pak_sa VEdgeType::ET_INITIAL_NBA:  //
            ສົ່ງຄືນ {new AstConst{flp, AstConst::BitFalse{}}, true};
        default:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            ສົ່ງຄືນ {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

public:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<AstNodeExpr*, bool> build(const AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        ຖ້າ (AstNodeExpr* const condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->cloneTreePure(false), term.first};
        }
        ສົ່ງຄືນ term;
    }

    // Like above, but for a whole SenTree
    std::pair<AstNodeExpr*, bool> build(const AstSenTree* senTreep) {
        FileLine* const flp = senTreep->fileline();
        AstNodeExpr* resultp = nullptr;
        bool firedAtInitialization = false;
        ສຳລັບ (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            const auto& pair = build(senItemp);
            ຖ້າ (AstNodeExpr* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        ສົ່ງຄືນ {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        ສົ່ງຄືນ ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        ສົ່ງຄືນ std::move(m_results);
    }

    // CONSTRUCTOR
    explicit SenExprBuilder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
