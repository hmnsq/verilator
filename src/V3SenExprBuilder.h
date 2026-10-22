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
        གལ་ཏེ (VN_IS(dtypep, BasicDType)) ཕྱིར true;
        གལ་ཏེ (VN_IS(dtypep, PackArrayDType)) ཕྱིར true;
        གལ་ཏེ (VN_IS(dtypep, UnpackArrayDType)) ཕྱིར isSupportedDType(dtypep->subDTypep());
        གལ་ཏེ (VN_IS(dtypep, NodeUOrStructDType)) ཕྱིར true;  // All are packed at the moment
        // Per IEEE, detects reference object pointer changes, not contents of the class changes
        གལ་ཏེ (VN_IS(dtypep, ClassRefDType)) ཕྱིར true;
        ཕྱིར false;
    }

    static bool isSimpleExpr(const AstNode* const exprp) {
        ཕྱིར exprp->forall([](const AstNode* const nodep) {
            ཕྱིར VN_IS(nodep, Const) || VN_IS(nodep, NodeVarRef) || VN_IS(nodep, Sel)
                   || VN_IS(nodep, NodeSel) || VN_IS(nodep, MemberSel)
                   || VN_IS(nodep, CMethodHard);
        });
    }

    // Whether a reference may be null: a class reference or a virtual interface
    static bool isNullableRef(const AstNodeExpr* const refp) {
        གལ་ཏེ (!refp->dtypep()) ཕྱིར false;
        const AstNodeDType* const dtypep = refp->dtypep()->skipRefp();
        const AstIfaceRefDType* const ifaceDtypep = VN_CAST(dtypep, IfaceRefDType);
        ཕྱིར VN_IS(dtypep, ClassRefDType) || (ifaceDtypep && ifaceDtypep->isVirtual());
    }

    // Check if expression contains a member access through a reference that could be null
    // (e.g., accessing an event through a class reference that may not be initialized)
    static bool hasNullableMemberAccess(const AstNode* const exprp) {
        ཕྱིར exprp && exprp->exists([](const AstNode* const nodep) {
            const AstMemberSel* const mselp = VN_CAST(nodep, MemberSel);
            ཕྱིར mselp && isNullableRef(mselp->fromp());
        });
    }

    // Get the base reference expression from a member selection chain
    // Returns the outermost reference that needs to be null-checked
    // Note: Returns a pointer into the original tree - caller must clone if needed
    static const AstNodeExpr* getBaseNullableRef(const AstNodeExpr* exprp) {
        སྐབས (exprp) {
            གལ་ཏེ (const AstMemberSel* const mselp = VN_CAST(exprp, MemberSel)) {
                const AstNodeExpr* const fromp = mselp->fromp();
                གལ་ཏེ (isNullableRef(fromp)) {
                    // Check if the base itself has a member access through a nullable reference
                    གལ་ཏེ (hasNullableMemberAccess(fromp)) {
                        exprp = fromp;
                        continue;
                    }
                    ཕྱིར fromp;
                }
                exprp = fromp;
            } དེ་མིན {
                ཕྱིར nullptr;
            }
        }
        ཕྱིར nullptr;
    }

    // METHODS
    AstVarScope* crateTemp(AstNodeExpr* exprp) {
        // For readability, use the scoped signal name if the trigger is a simple AstVarRef
        string name;
        གལ་ཏེ (AstVarRef* const refp = VN_CAST(exprp, VarRef)) {
            AstVarScope* const vscp = refp->varScopep();
            name = "__" + vscp->scopep()->nameDotless() + "__" + vscp->varp()->name();
            name = m_prevNames.get(name);
        } དེ་མིན {
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
        ཕྱིར vscp;
    }

    // Helper to wrap a statement with a null check: if (baseRef != null) stmt
    AstNodeStmt* wrapStmtWithNullCheck(FileLine* flp, AstNodeStmt* stmtp,
                                       const AstNodeExpr* baseRefp) {
        གལ་ཏེ (!baseRefp) ཕྱིར stmtp;
        AstNodeExpr* const nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        AstNodeExpr* const checkp
            = new AstNeq{flp, const_cast<AstNodeExpr*>(baseRefp)->cloneTree(false), nullp};
        ཕྱིར new AstIf{flp, checkp, stmtp};
    }

    // Helper to wrap a trigger expression with a null check if needed
    // Returns the expression wrapped in: (baseRef != null) ? expr : 0
    AstNodeExpr* wrapExprWithNullCheck(FileLine* flp, AstNodeExpr* exprp,
                                       const AstNodeExpr* baseRefp) {
        གལ་ཏེ (!baseRefp) ཕྱིར exprp;
        AstNodeExpr* const nullp = new AstConst{flp, AstConst::Null{}};
        // const_cast safe: cloneTree doesn't modify the source
        AstNodeExpr* const checkp
            = new AstNeq{flp, const_cast<AstNodeExpr*>(baseRefp)->cloneTree(false), nullp};
        AstNodeExpr* const falsep = new AstConst{flp, AstConst::BitFalse{}};
        AstNodeExpr* const condp = new AstCond{flp, checkp, exprp, falsep};
        condp->dtypeSetBit();
        ཕྱིར condp;
    }

    AstNodeExpr* getCurr(AstNodeExpr* exprp) {
        // For simple expressions like varrefs or selects, just use them directly
        གལ་ཏེ (isSimpleExpr(exprp)) ཕྱིར exprp->cloneTree(false);

        // Create the 'current value' variable
        FileLine* const flp = exprp->fileline();
        auto result = m_curr.emplace(*exprp, nullptr);
        གལ་ཏེ (result.second) result.first->second = crateTemp(exprp);
        AstVarScope* const currp = result.first->second;

        // Check if we need null guards for a member access through a nullable reference
        const AstNodeExpr* const baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        // Add pre update if it does not exist yet in this round
        གལ་ཏེ (m_hasPreUpdate.emplace(*currp).second) {
            m_results.m_preUpdates.push_back(
                wrapStmtWithNullCheck(flp,
                                      new AstAssign{flp, new AstVarRef{flp, currp, VAccess::WRITE},
                                                    exprp->cloneTree(false)},
                                      baseRefp));
        }
        ཕྱིར new AstVarRef{flp, currp, VAccess::READ};
    }

    AstVarScope* getPrev(AstNodeExpr* exprp) {
        FileLine* const flp = exprp->fileline();
        const auto rdCurr = [this, exprp]() { ཕྱིར getCurr(exprp); };

        // Check if we need null guards for a member access through a nullable reference
        const AstNodeExpr* const baseRefp
            = hasNullableMemberAccess(exprp) ? getBaseNullableRef(exprp) : nullptr;

        AstNode* scopeExprp = exprp;
        གལ་ཏེ (AstVarRef* const refp = VN_CAST(exprp, VarRef)) scopeExprp = refp->varScopep();
        // Create the 'previous value' variable
        const auto pair = m_prev.emplace(*scopeExprp, nullptr);
        གལ་ཏེ (pair.second) {
            AstVarScope* const prevp = crateTemp(exprp);
            pair.first->second = prevp;
            // Add the initializer init (guarded if through a nullable reference)
            AstAssign* const initp = new AstAssign{flp, new AstVarRef{flp, prevp, VAccess::WRITE},
                                                   exprp->cloneTree(false)};
            m_results.m_inits.push_back(wrapStmtWithNullCheck(flp, initp, baseRefp));
        }

        AstVarScope* const prevp = pair.first->second;

        const auto wrPrev = [=]() { ཕྱིར new AstVarRef{flp, prevp, VAccess::WRITE}; };

        // Add post update if it does not exist yet
        གལ་ཏེ (m_hasPostUpdate.emplace(*exprp).second) {
            AstNodeDType* const exprDtp = exprp->dtypep()->skipRefp();
            གལ་ཏེ (!isSupportedDType(exprDtp)) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Cannot detect changes on expression of complex type "
                        << exprDtp->prettyDTypeNameQ() << "\n"
                        << exprp->warnMore()
                        << "... May be caused by combinational cycles reported with UNOPTFLAT");
                ཕྱིར prevp;
            }
            གལ་ཏེ (VN_IS(exprDtp, UnpackArrayDType)) {
                AstCMethodHard* const cmhp
                    = new AstCMethodHard{flp, wrPrev(), VCMethod::UNPACKED_ASSIGN, rdCurr()};
                cmhp->dtypeSetVoid();
                m_results.m_postUpdates.push_back(
                    wrapStmtWithNullCheck(flp, cmhp->makeStmt(), baseRefp));
            } དེ་མིན {
                m_results.m_postUpdates.push_back(
                    wrapStmtWithNullCheck(flp, new AstAssign{flp, wrPrev(), rdCurr()}, baseRefp));
            }
        }

        ཕྱིར prevp;
    }

    std::pair<AstNodeExpr*, bool> createTerm(const AstSenItem* senItemp) {
        FileLine* const flp = senItemp->fileline();
        AstNodeExpr* const senp = senItemp->sensp();

        // Check if the sensitivity expression involves accessing through a class reference
        // that may be null (e.g., DynScope handles created in fork blocks, or class member
        // virtual interfaces). If so, we need to guard against null pointer dereference.
        const AstNodeExpr* const baseRefp
            = hasNullableMemberAccess(senp) ? getBaseNullableRef(senp) : nullptr;

        const auto currp = [this, senp]() { ཕྱིར getCurr(senp); };
        const auto prevp
            = [this, flp, senp]() { ཕྱིར new AstVarRef{flp, getPrev(senp), VAccess::READ}; };
        const auto lsb = [=](AstNodeExpr* opp) { ཕྱིར new AstSel{flp, opp, 0, 1}; };

        // All event signals should be 1-bit at this point
        switch (senItemp->edgeType()) {
        pak_sa VEdgeType::ET_CHANGED:
        pak_sa VEdgeType::ET_HYBRID:  //
            གལ་ཏེ (VN_IS(senp->dtypep()->skipRefp(), UnpackArrayDType)) {
                // operand order reversed to avoid calling neq() method on non-VlUnpacked type, see
                // issue #5125
                AstCMethodHard* const resultp
                    = new AstCMethodHard{flp, prevp(), VCMethod::UNPACKED_NEQ, currp()};
                resultp->dtypeSetBit();
                ཕྱིར {wrapExprWithNullCheck(flp, resultp, baseRefp), true};
            }
            ཕྱིར {wrapExprWithNullCheck(flp, new AstNeq{flp, currp(), prevp()}, baseRefp), true};
        pak_sa VEdgeType::ET_BOTHEDGE:  //
            ཕྱིར {wrapExprWithNullCheck(flp, lsb(new AstXor{flp, currp(), prevp()}), baseRefp),
                    false};
        pak_sa VEdgeType::ET_POSEDGE:  //
            ཕྱིར {wrapExprWithNullCheck(
                        flp, lsb(new AstAnd{flp, currp(), new AstNot{flp, prevp()}}), baseRefp),
                    false};
        pak_sa VEdgeType::ET_NEGEDGE:  //
            ཕྱིར {wrapExprWithNullCheck(
                        flp, lsb(new AstAnd{flp, new AstNot{flp, currp()}, prevp()}), baseRefp),
                    false};
        pak_sa VEdgeType::ET_EVENT: {
            UASSERT_OBJ(v3Global.hasEvents(), senItemp, "Inconsistent");

            // Clear 'fired' state when done (guarded if through a nullable reference)
            // The event is modified, so the select (e.g. assoc array) must be an lvalue
            AstNodeExpr* const clearFromp = currp();
            གལ་ཏེ (AstNodeVarRef* const refp
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
            གལ་ཏེ (const AstVarRef* const refp = VN_CAST(senp, VarRef)) {
                // The private NBA event must run so pending assignments finish settling.
                གལ་ཏེ (refp->varScopep() == v3Global.rootp()->nbaEventp()) ཕྱིར {firedp, false};
            }
            // Do not trigger user named-event processes after $finish.
            AstNodeExpr* const notFinishp
                = new AstCExpr{flp,
                               "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish() && "
                               "!vlSymsp->_vm_contextp__->finishPending())",
                               1};
            ཕྱིར {new AstLogAnd{flp, notFinishp, firedp}, false};
        }
        pak_sa VEdgeType::ET_TRUE:  //
            ཕྱིར {currp(), false};
        pak_sa VEdgeType::ET_INITIAL_NBA:  //
            ཕྱིར {new AstConst{flp, AstConst::BitFalse{}}, true};
        default:  // LCOV_EXCL_START
            senItemp->v3fatalSrc("Unknown edge type");
            ཕྱིར {nullptr, false};
        }  // LCOV_EXCL_STOP
    }

public:
    // Returns the expression computing the trigger, and a bool indicating that
    // this trigger should be fired on the first evaluation (at initialization)
    std::pair<AstNodeExpr*, bool> build(const AstSenItem* senItemp) {
        auto term = createTerm(senItemp);
        གལ་ཏེ (AstNodeExpr* const condp = senItemp->condp()) {
            term.first = new AstAnd{senItemp->fileline(), condp->cloneTreePure(false), term.first};
        }
        ཕྱིར term;
    }

    // Like above, but for a whole SenTree
    std::pair<AstNodeExpr*, bool> build(const AstSenTree* senTreep) {
        FileLine* const flp = senTreep->fileline();
        AstNodeExpr* resultp = nullptr;
        bool firedAtInitialization = false;
        ཆེད་དུ (AstSenItem* senItemp = senTreep->sensesp(); senItemp;
             senItemp = VN_AS(senItemp->nextp(), SenItem)) {
            const auto& pair = build(senItemp);
            གལ་ཏེ (AstNodeExpr* termp = pair.first) {
                resultp = resultp ? new AstOr{flp, resultp, termp} : termp;
                firedAtInitialization |= pair.second;
            }
        }
        ཕྱིར {resultp, firedAtInitialization};
    }

    Results getResultsAndClearUpdates() {
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        Results ans = std::move(m_results);
        m_results = {};
        ཕྱིར ans;
    }

    Results getAndClearResults() {
        m_curr.clear();
        m_prev.clear();
        m_hasPreUpdate.clear();
        m_hasPostUpdate.clear();
        ཕྱིར std::move(m_results);
    }

    // CONSTRUCTOR
    explicit SenExprBuilder(AstScope* scopep)
        : m_scopep{scopep} {}
};

#endif  // Guard
