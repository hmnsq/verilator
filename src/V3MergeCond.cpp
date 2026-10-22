// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Merge branches/ternary ?:
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
// V3BranchMerge's Transformations:
//
//    Look for sequences of assignments with ternary conditional on the right
//    hand side with the same condition:
//      lhs0 = cond ? then0 : else0;
//      lhs1 = cond ? then1 : else1;
//      lhs2 = cond ? then2 : else2;
//
//    This seems to be a common pattern and can make the C compiler take a
//    long time when compiling it with optimization. For us it's easy and fast
//    to convert this to 'if' statements because we know the pattern is common:
//      if (cond) {
//          lhs0 = then0;
//          lhs1 = then1;
//          lhs2 = then2;
//      } else {
//          lhs0 = else0;
//          lhs1 = else1;
//          lhs2 = else2;
//      }
//
//   For 1-bit signals, we consider strength reduced forms to be conditionals,
//   but only if we already encountered a true conditional we can merge with.
//   If we did, then act as if:
//      'lhs = cond & value' is actually 'lhs = cond ? value : 1'd0'
//      'lhs = cond' is actually 'lhs = cond ? 1'd1 : 1'd0'.
//
//  Also merges consecutive AstNodeIf statements with the same condition.
//
//  Because this optimization has notable performance impact, we go further
//  and perform code motion to try to move mergeable conditionals next to each
//  other, which in turn enable us to merge more conditionals. To do this, we
//  perform an analysis pass, followed by an optimization pass on the whole
//  AstCFunc we are optimizing.
//
//  The analysis pass gathers, for each statement in the tree, the information
//  relevant for determining whether two statements can be swapped, and some
//  other additional information that is useful during optimization.
//
//  The optimization pass tries to move conditionals near each other, first by
//  trying to move a conditional node backwards in the list, so it becomes the
//  direct successor of another earlier conditional with the same condition.
//  If this is not possible due to variable interference, then we additionally
//  try to pull earlier conditionals with the same condition closer forward to
//  be the immediate predecessor of the conditional node. We limit maximum
//  distance a node can travel to an empirically chosen but otherwise arbitrary
//  constant. This limits worst case complexity to be O(n) rather than O(n^2).
//  The worst case complexity manifests when N/2 conditionals, all with unique
//  conditions are succeeded by N/2 conditionals with the same unique
//  conditions, such that each unique condition is used by exactly 2
//  conditionals. In this case N/2 all nodes need to travel approx N/2 distance.
//  Limiting the distance bounds the latter, hence limiting complexity.
//
//  Once the analysis and optimization passes have been applied to the whole
//  function, any merged conditionals will then undergo the same analysis,
//  optimization, and merging again in their individual branches.
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3MergeCond.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DupFinder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hasher.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <queue>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* ਕੱਢੋ_Cond_From_Rhs(Ast_ਗੰਢ* rhsp) {
    ਜੇ (AstCond* ਅਚਲ condp = VN_CAST(rhsp, Cond)) {
        ਵਾਪਸ condp;
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstAnd* ਅਚਲ andp = VN_CAST(rhsp, And)) {
        ਜੇ (AstCond* ਅਚਲ condp = VN_CAST(andp->rhsp(), Cond)) {
            ਜੇ (VN_IS(andp->lhsp(), Const)) ਵਾਪਸ condp;
        }
    }
    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
ਤਾਰਕਿਕ areDisjoint(ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*>& a, ਅਚਲ std::ਸੈੱਟ<ਅਚਲ AstVar*>& b) {
    ਜੇ (a.ਖਾਲੀ_ਥਾਵਾਂ() || b.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਸੱਚ;
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਸਿਰਾ_A = a.ਸਿਰਾ();
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਸਿਰਾ_B = b.ਸਿਰਾ();
    ਸਵੈਚਾਲਿਤ itA = a.begin();
    ਸਵੈਚਾਲਿਤ itB = b.begin();
    ਜਦੋਂ (ਸੱਚ) {
        ਜੇ (*itA == *itB) ਵਾਪਸ ਝੂਠ;
        ਜੇ (std::less<ਅਚਲ AstVar*>{}(*itA, *itB)) {
            itA = std::ਹੇਠਲਾ_bound(++itA, ਸਿਰਾ_A, *itB);
            ਜੇ (itA == ਸਿਰਾ_A) ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ {
            itB = std::ਹੇਠਲਾ_bound(++itB, ਸਿਰਾ_B, *itA);
            ਜੇ (itB == ਸਿਰਾ_B) ਵਾਪਸ ਸੱਚ;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

ਬਣਤਰ StmtProperties final {
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_condp = ਖਾਲੀ_ਸੂਚਕ;  // The condition expression, if a conditional node
    std::ਸੈੱਟ<ਅਚਲ AstVar*> m_rdVars;  // Variables read by this statement
    std::ਸੈੱਟ<ਅਚਲ AstVar*> m_wrVars;  // Variables written by this statement
    ਤਾਰਕਿਕ m_isFence = ਝੂਠ;  // Nothing should move across this statement, nor should it be merged
    ਤਾਰਕਿਕ m_sideEffect = ਝੂਠ;  // Statement may have side effect, without access to model state
    ਤਾਰਕਿਕ m_implPubRd = ਝੂਠ;  // Statement may implicitly read public state (without VarRef)
    ਤਾਰਕਿਕ m_implPubWr = ਝੂਠ;  // Statement may implicitly write public state (without VarRef)
    ਤਾਰਕਿਕ m_explPubRef = ਝੂਠ;  // Statement explicitly references public state via VarRef
    Ast_ਗੰਢ_Stmt* m_prevWithSameCondp = ਖਾਲੀ_ਸੂਚਕ;  // Previous node in same list, with same condition
    ਤਾਰਕਿਕ writesConditionVar(ਤਾਰਕਿਕ condPubWritable) ਅਚਲ {
        // This relies on MarkVarsVisitor having been called on the condition node
        ਲਈ (ਅਚਲ AstVar* ਅਚਲ varp : m_wrVars) {
            ਜੇ (varp->user1()) ਵਾਪਸ ਸੱਚ;
        }
        // If the condition contains a public variable, check if it might be written
        ਜੇ (condPubWritable && m_implPubWr) ਵਾਪਸ ਸੱਚ;
        // Otherwise not written
        ਵਾਪਸ ਝੂਠ;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
ਵਰਤੋਂ StmtPropertiesAllocator = AstUser3Allocator<Ast_ਗੰਢ_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
ਵਰਗ ਕੋਡ_Motion_Analysis_Visitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeExpr::user3   -> AstNodeStmt*: Set on a condition node, points to the last
    //                         conditional with that condition so far encountered in the same
    //                         AstNode list
    // AstNode::user4       -> Used by V3Hasher

    StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS
    V3Hasher m_hasher;  // Used by V3DupFinder
    // Stack of a V3DupFinder used for finding identical condition expressions within one
    // statement list.
    std::ਸਦਿਸ਼<V3DupFinder> m_ਢੇਰ;
    StmtProperties* m_propsp = ਖਾਲੀ_ਸੂਚਕ;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਕੱਢੋ_Condition(ਅਚਲ Ast_ਗੰਢ_Stmt* nodep) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* conditionp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(nodep, ਗੰਢ_ਸੌਂਪੋ)) {
            ਜੇ (AstCond* ਅਚਲ conditionalp = ਕੱਢੋ_Cond_From_Rhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_If* ਅਚਲ ifp = VN_CAST(nodep, ਗੰਢ_If)) {
            conditionp = ifp->condp();
        }
        ਜਦੋਂ (AstCCast* ਅਚਲ castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        ਵਾਪਸ conditionp;
    }

    ਖਾਲੀ analyzeStmt(Ast_ਗੰਢ_Stmt* nodep, ਤਾਰਕਿਕ try_Cond_ਮੇਲ) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* ਅਚਲ outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਕੱਢੋ_Condition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            ਜੇ (try_Cond_ਮੇਲ) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_ਢੇਰ.ਪਿੱਠ();
                // Find a duplicate condition
                ਅਚਲ V3DupFinder::iterator& dit = dupFinder.ਲੱਭੋ_Duplicate(condp);
                ਜੇ (dit == dupFinder.ਸਿਰਾ()) {
                    // First time seeing this condition in the current list
                    dupFinder.ਪਾਓ(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } ਨਹੀਂਤਾਂ {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ firstp = VN_AS(dit->ਦੂਜਾ, ਗੰਢ_ਵਿਅੰਜਕ);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_ਗੰਢ_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_ਗੰਢ(nodep);

        // If there is an enclosing statement, propagate properties upwards
        ਜੇ (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.ਪਾਓ(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.ਪਾਓ(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    ਖਾਲੀ analyzeVarRef(AstVarRef* nodep) {
        ਅਚਲ VAccess access = nodep->access();
        AstVar* ਅਚਲ varp = nodep->varp();
        // Gather read and written variables
        ਜੇ (access.isReadOrRW()) m_propsp->m_rdVars.ਪਾਓ(varp);
        ਜੇ (access.isWriteOrRW()) m_propsp->m_wrVars.ਪਾਓ(varp);
        ਜੇ (varp->is_Sig_ਜਨਤਕ() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = ਸੱਚ;
        }
    }

    ਖਾਲੀ checkProperties(Ast_ਗੰਢ* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        ਜੇ (VN_IS(nodep, Stmt_ਵਿਅੰਜਕ)) ਵਾਪਸ;

        // Call to a DPI import
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_CCall* ਅਚਲ ccallp = VN_CAST(nodep, ਗੰਢ_CCall)) {
            AstCFunc* ਅਚਲ funcp = ccallp->funcp();
            ਜੇ (funcp->dpiImportWrapper()) {
                ਜੇ (!funcp->dpi_ਸ਼ੁੱਧ()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = ਸੱਚ;
                    m_propsp->m_implPubRd = ਸੱਚ;
                    m_propsp->m_implPubWr = ਸੱਚ;
                }
                ਵਾਪਸ;
            }
        }

        // Side effect, but assume does not access public state
        ਜੇ (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = ਸੱਚ;
            ਵਾਪਸ;
        }

        // If impure, or branch, mark statement as fence
        ਜੇ (!nodep->is_ਸ਼ੁੱਧ() || nodep->isBrancher()) m_propsp->m_isFence = ਸੱਚ;
    }

    ਖਾਲੀ analyze_ਗੰਢ(Ast_ਗੰਢ* nodep) {
        // Check properties of this node if under a statement
        ਜੇ (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        ਤਾਰਕਿਕ singleton_List_ਸ਼ੁਰੂ = ਝੂਠ;
        ਜੇ (nodep->backp()->nextp() != nodep) {  // If at head of list
            singleton_List_ਸ਼ੁਰੂ = nodep->nextp() == ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!singleton_List_ਸ਼ੁਰੂ) m_ਢੇਰ.emplace_ਪਿੱਠ(m_hasher);
        }

        // Analyze node
        ਜੇ (Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp = VN_CAST(nodep, ਗੰਢ_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singleton_List_ਸ਼ੁਰੂ);
        } ਨਹੀਂਤਾਂ ਜੇ (AstVarRef* ਅਚਲ vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } ਨਹੀਂਤਾਂ {
            analyze_ਗੰਢ(nodep);
        }

        // Pop the stack at the end of a list
        ਜੇ (!singleton_List_ਸ਼ੁਰੂ && !nodep->nextp()) m_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
    }

    // CONSTRUCTOR
    ਕੋਡ_Motion_Analysis_Visitor(Ast_ਗੰਢ* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterate_And_ਅਗਲਾ_Const_Null(nodep);
    }

ਜਨਤਕ:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    ਸਥਿਰ ਖਾਲੀ analyze(Ast_ਗੰਢ* nodep, StmtPropertiesAllocator& stmtProperties) {
        ਕੋਡ_Motion_Analysis_Visitor{nodep, stmtProperties};
    }
};

ਵਰਗ ਕੋਡ_Motion_Optimize_Visitor final : ਜਨਤਕ VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    ਸਥਿਰ constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ MAX_ਦੂਰੀ = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4_ਵਿੱਚ_Use m_user4_ਵਿੱਚ_Use;

    ਅਚਲ StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    ਤਾਰਕਿਕ areSwappable(ਅਚਲ Ast_ਗੰਢ_Stmt* ap, ਅਚਲ Ast_ਗੰਢ_Stmt* bp) ਅਚਲ {
        ਅਚਲ StmtProperties& aProps = m_stmtProperties(ap);
        ਅਚਲ StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        ਜੇ (aProps.m_isFence) ਵਾਪਸ ਝੂਠ;
        ਜੇ (bProps.m_isFence) ਵਾਪਸ ਝੂਠ;
        // Don't swap side effecting statements, but can move others across them
        ਜੇ (aProps.m_sideEffect && bProps.m_sideEffect) ਵਾਪਸ ਝੂਠ;
        // Don't swap if there is a hazard around public state - must observe ordering
        ਅਚਲ ਤਾਰਕਿਕ bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        ਜੇ (aProps.m_implPubWr && bPubRef) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ ਤਾਰਕਿਕ aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        ਜੇ (aPubRef && bProps.m_implPubWr) ਵਾਪਸ ਝੂਠ;
        // If either statement writes a variable that the other reads, they are not swappable
        ਜੇ (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) ਵਾਪਸ ਝੂਠ;
        // If they both write to the same variable, they are not swappable
        ਜੇ (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) ਵਾਪਸ ਝੂਠ;
        // Otherwise good to go
        ਵਾਪਸ ਸੱਚ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        // Process only on first encounter
        ਜੇ (nodep->user4_ਸੈੱਟ_Once()) ਵਾਪਸ;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_ਗੰਢ_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        ਜੇ (!prevp) ਵਾਪਸ;
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            ਅਚਲ Ast_ਗੰਢ* currp = prevp;
            ਜਦੋਂ (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#ਜੇ_ਅੰਤ
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        ਜੇ (Ast_ਗੰਢ_Stmt* predp = VN_CAST(nodep->backp(), ਗੰਢ_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = MAX_ਦੂਰੀ; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                ਜੇ (predp == prevp) ਰੋਕੋ;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_ਗੰਢ_Stmt* ਅਚਲ backp = VN_CAST(predp->backp(), ਗੰਢ_Stmt);
                ਜੇ (!backp) ਰੋਕੋ;
                // Don't swap statements if doing so would change program semantics
                ਜੇ (!areSwappable(predp, nodep)) ਰੋਕੋ;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            ਜੇ (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlink_Fr_ਪਿੱਠ();
                predp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(nodep);
                // If the predecessor is the previous node with the same condition, job done
                ਜੇ (predp == prevp) ਵਾਪਸ;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        ਲਈ (Ast_ਗੰਢ_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            ਜੇ (Ast_ਗੰਢ_Stmt* succp = VN_CAST(prevp->nextp(), ਗੰਢ_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = MAX_ਦੂਰੀ; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    ਜੇ (succp == currp) ਰੋਕੋ;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_ਗੰਢ_Stmt* ਅਚਲ nextp = VN_CAST(succp->nextp(), ਗੰਢ_Stmt);
                    ਜੇ (!nextp) ਰੋਕੋ;
                    // Don't swap statements if doing so would change program semantics
                    ਜੇ (!areSwappable(prevp, succp)) ਰੋਕੋ;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                ਜੇ (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlink_Fr_ਪਿੱਠ();
                    succp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(prevp);
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    ਕੋਡ_Motion_Optimize_Visitor(Ast_ਗੰਢ* nodep, ਅਚਲ StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterate_And_ਅਗਲਾ_Null(nodep);
    }

ਜਨਤਕ:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    ਸਥਿਰ Ast_ਗੰਢ* optimize(Ast_ਗੰਢ* nodep, ਅਚਲ StmtPropertiesAllocator& stmtProperties) {
        { ਕੋਡ_Motion_Optimize_Visitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        ਜਦੋਂ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        ਵਾਪਸ nodep;
    }
};

//######################################################################
// Conditional merging

ਵਰਗ ਮਿਲਾਓ_Cond_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstVar::user1        -> bool: Set for variables referenced by m_mgCondp
    //                         (Only below MergeCondVisitor::process).
    // AstNode::user2       -> bool: Marking node as included in merge because cheap to duplicate
    //                         (Only below MergeCondVisitor::process).
    // AstNodeStmt::user3   -> StmtProperties
    //                         (Only below MergeCondVisitor::process).
    // AstNodeExpr::user3   -> See CodeMotionAnalysisVisitor
    //                         (Only below MergeCondVisitor::process).
    // AstNode::user4       -> See CodeMotionAnalysisVisitor/CodeMotionOptimizeVisitor

    // STATE
    VDouble0 m_statMerges;  // Statistic tracking
    VDouble0 m_stat_Merged_ਚੀਜ਼ਾਂ;  // Statistic tracking
    VDouble0 m_stat_Candidate_ਚੀਜ਼ਾਂ;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_ਗੰਢ* m_mgFirstp = ਖਾਲੀ_ਸੂਚਕ;  // First node in merged sequence
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_mgCondp = ਖਾਲੀ_ਸੂਚਕ;  // The condition of the first node
    ਅਚਲ Ast_ਗੰਢ* m_mgLastp = ਖਾਲੀ_ਸੂਚਕ;  // Last node in merged sequence
    ਅਚਲ Ast_ਗੰਢ* m_mgNextp = ਖਾਲੀ_ਸੂਚਕ;  // Next node in list being examined
    ਤਾਰਕਿਕ m_mgCondPubWritable = ਝੂਠ;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::ਕਤਾਰ<Ast_ਗੰਢ*>* m_workQueuep = ਖਾਲੀ_ਸੂਚਕ;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = ਖਾਲੀ_ਸੂਚਕ;

    // METHODS

    // Function that processes a whole sub-tree
    ਖਾਲੀ process(Ast_ਗੰਢ* nodep) {
        // Set up work queue
        std::ਕਤਾਰ<Ast_ਗੰਢ*> work_ਕਤਾਰ;
        m_workQueuep = &work_ਕਤਾਰ;
        m_workQueuep->ਧੱਕੋ(nodep);

        ਕਰੋ {
            // Set up user* for this iteration
            ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
            ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
            ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_ਗੰਢ* currp = m_workQueuep->front();
            m_workQueuep->ਕੱਢੋ();

            // Analyze sub-tree list for code motion and conditional merging
            ਕੋਡ_Motion_Analysis_Visitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            ਜੇ (v3Global.opt.f_ਮਿਲਾਓ_Cond_Motion()) {
                currp = ਕੋਡ_Motion_Optimize_Visitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterate_And_ਅਗਲਾ_Null(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            ਜੇ (m_mgFirstp) ਮਿਲਾਓ_ਸਿਰਾ();
            m_stmtPropertiesp = ਖਾਲੀ_ਸੂਚਕ;
        } ਜਦੋਂ (!m_workQueuep->ਖਾਲੀ_ਥਾਵਾਂ());

        m_workQueuep = ਖਾਲੀ_ਸੂਚਕ;
    }

    // Skip past AstArraySel and AstWordSel with const index
    ਸਥਿਰ Ast_ਗੰਢ* skipConstSels(Ast_ਗੰਢ* nodep) {
        ਜਦੋਂ (ਅਚਲ AstArraySel* ਅਚਲ aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            ਜੇ (!VN_IS(aselp->bitp(), Const)) ਵਾਪਸ nodep;
            nodep = aselp->fromp();
        }
        ਜਦੋਂ (ਅਚਲ Ast_ਸ਼ਬਦ_Sel* ਅਚਲ wselp = VN_CAST(nodep, ਸ਼ਬਦ_Sel)) {
            // WordSel index is not constant, so might be expensive
            ਜੇ (!VN_IS(wselp->bitp(), Const)) ਵਾਪਸ nodep;
            nodep = wselp->fromp();
        }
        ਵਾਪਸ nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    ਸਥਿਰ ਤਾਰਕਿਕ is_Cheap_ਗੰਢ(Ast_ਗੰਢ* nodep) {
        // Comments are cheap
        ਜੇ (VN_IS(nodep, Comment)) ਵਾਪਸ ਸੱਚ;
        // So are some assignments
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(nodep, ਗੰਢ_ਸੌਂਪੋ)) {
            // Check LHS
            Ast_ਗੰਢ* ਅਚਲ lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            ਜੇ (!VN_IS(lhsp, VarRef)) ਵਾਪਸ ਝੂਠ;

            // Check RHS
            Ast_ਗੰਢ* ਅਚਲ rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            ਜੇ (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) ਵਾਪਸ ਝੂਠ;

            // Otherwise it is a cheap assignment
            ਵਾਪਸ ਸੱਚ;
        }
        // Others are not
        ਵਾਪਸ ਝੂਠ;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    ਸਥਿਰ ਤਾਰਕਿਕ yieldsOneOrZero(ਅਚਲ Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) {
            ਵਾਪਸ constp->ਅੰਕ().ਮੰਜ਼ਿਲ_UQuad() <= 1;
        }
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ vrefp = VN_CAST(nodep, VarRef)) {
            ਅਚਲ AstVar* ਅਚਲ varp = vrefp->varp();
            ਵਾਪਸ varp->ਚੌੜਾਈ_Min() == 1 && !varp->dtypep()->isSigned();
        }
        ਜੇ (ਅਚਲ Ast_ਖਿਸਕਾਓ_R* ਅਚਲ shiftp = VN_CAST(nodep, ਖਿਸਕਾਓ_R)) {
            // Shift right by width - 1 or more
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(shiftp->rhsp(), Const)) {
                ਅਚਲ AstVarRef* ਅਚਲ vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->ਚੌੜਾਈ_Min()
                                      : shiftp->ਚੌੜਾਈ();
                ਜੇ (constp->ਮੰਜ਼ਿਲ_SInt() >= ਚੌੜਾਈ - 1) ਵਾਪਸ ਸੱਚ;
            }
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ biopp = VN_CAST(nodep, ਗੰਢ_Biop)) {
            ਜੇ (VN_IS(nodep, And))
                ਵਾਪਸ yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            ਜੇ (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                ਵਾਪਸ yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (ਅਚਲ AstCond* ਅਚਲ condp = VN_CAST(nodep, Cond)) {
            ਵਾਪਸ yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        ਜੇ (ਅਚਲ AstCCast* ਅਚਲ castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            ਵਾਪਸ yieldsOneOrZero(castp->lhsp());
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਨਕਾਬ_Lsb(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (yieldsOneOrZero(nodep)) ਵਾਪਸ nodep;
        // Otherwise apply masking
        AstConst* ਅਚਲ maskp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
        // Mask on left, as conventional
        ਵਾਪਸ ਨਵਾਂ AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* foldAndUnlink(Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਤਾਰਕਿਕ condTrue) {
        ਜੇ (rhsp->same_ਰੁੱਖ(m_mgCondp)) {
            ਵਾਪਸ ਨਵਾਂ AstConst{rhsp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}, condTrue};
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstCond* ਅਚਲ condp = ਕੱਢੋ_Cond_From_Rhs(rhsp)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ resp
                = condTrue ? condp->thenp()->unlink_Fr_ਪਿੱਠ() : condp->elsep()->unlink_Fr_ਪਿੱਠ();
            ਜੇ (condp == rhsp) ਵਾਪਸ resp;
            ਜੇ (ਅਚਲ AstAnd* ਅਚਲ andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                ਵਾਪਸ ਨਵਾਂ AstAnd{andp->fileline(), andp->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), resp};
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstAnd* ਅਚਲ andp = VN_CAST(rhsp, And)) {
            ਜੇ (andp->lhsp()->same_ਰੁੱਖ(m_mgCondp)) {
                ਵਾਪਸ condTrue ? ਨਕਾਬ_Lsb(andp->rhsp()->unlink_Fr_ਪਿੱਠ())
                                : ਨਵਾਂ AstConst{rhsp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(andp->rhsp()->same_ਰੁੱਖ(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                ਵਾਪਸ condTrue ? ਨਕਾਬ_Lsb(andp->lhsp()->unlink_Fr_ਪਿੱਠ())
                                : ਨਵਾਂ AstConst{rhsp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, ਸ਼ਬਦ_Sel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            ਵਾਪਸ rhsp->clone_ਰੁੱਖ(ਝੂਠ);
        }
        // LCOV_EXCL_START
        ਜੇ (debug()) rhsp->dump_ਰੁੱਖ("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // LCOV_EXCL_STOP
    }

    ਖਾਲੀ ਮਿਲਾਓ_ਸਿਰਾ() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        ਜਦੋਂ (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        ਜਦੋਂ (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_ਗੰਢ_If* recursivep = ਖਾਲੀ_ਸੂਚਕ;
        // Merge if list is longer than one node
        ਜੇ (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            ਜੇ (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* ਅਚਲ resultp = ਨਵਾਂ AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(resultp);
            // Try to preserve branch prediction if we can
            ਮਾਪ_t nLikely = 0;
            ਮਾਪ_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_ਗੰਢ* nextp = m_mgFirstp;
            ਕਰੋ {
                // Grab next pointer and unlink
                Ast_ਗੰਢ* ਅਚਲ currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : ਖਾਲੀ_ਸੂਚਕ;
                currp->unlink_Fr_ਪਿੱਠ();
                // Skip over comments
                ਜੇ (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_ਰੁੱਖ(), currp);
                    ਜਾਰੀ;
                }
                //ਗਿਣਤੀ
                ++m_stat_Merged_ਚੀਜ਼ਾਂ;
                ਜੇ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(currp, ਗੰਢ_ਸੌਂਪੋ)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = assignp->rhsp()->unlink_Fr_ਪਿੱਠ();
                    Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ thenp = assignp;
                    Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ elsep = assignp->clone_ਰੁੱਖ(ਝੂਠ);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, ਸੱਚ));
                    elsep->rhsp(foldAndUnlink(rhsp, ਝੂਠ));
                    resultp->ਜੋੜੋ_Thensp(thenp);
                    resultp->ਜੋੜੋ_Elsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_ਰੁੱਖ(), rhsp);
                } ਨਹੀਂਤਾਂ {
                    Ast_ਗੰਢ_If* ਅਚਲ ifp = VN_AS(currp, ਗੰਢ_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    ਜੇ (Ast_ਗੰਢ* ਅਚਲ listp = ifp->thensp()) {
                        resultp->ਜੋੜੋ_Thensp(listp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    }
                    ਜੇ (Ast_ਗੰਢ* ਅਚਲ listp = ifp->elsesp()) {
                        resultp->ਜੋੜੋ_Elsesp(listp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    }
                    // Record branch prediction
                    ਜੇ (ifp->branchPred().likely()) ++nLikely;
                    ਜੇ (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_ਰੁੱਖ(), ifp);
                }
            } ਜਦੋਂ (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            ਜੇ (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            ਜੇ (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            ਜੇ (resultp->thensp()) m_workQueuep->ਧੱਕੋ(resultp->thensp());
            ਜੇ (resultp->elsesp()) m_workQueuep->ਧੱਕੋ(resultp->elsesp());
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_If* ਅਚਲ ifp = VN_CAST(m_mgFirstp, ਗੰਢ_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = ਖਾਲੀ_ਸੂਚਕ;
        m_mgCondp = ਖਾਲੀ_ਸੂਚਕ;
        m_mgLastp = ਖਾਲੀ_ਸੂਚਕ;
        m_mgNextp = ਖਾਲੀ_ਸੂਚਕ;
        m_mgCondPubWritable = ਝੂਠ;
        Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();  // Clear marked variables
        Ast_ਗੰਢ::user2_Clear_ਰੁੱਖ();
        // Merge recursively within the branches of an un-merged AstNodeIF
        ਜੇ (recursivep) {
            iterate_And_ਅਗਲਾ_Null(recursivep->thensp());
            iterate_And_ਅਗਲਾ_Null(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            ਜੇ (m_mgFirstp) ਮਿਲਾਓ_ਸਿਰਾ();
        }
    }

    // Check if the node can be simplified if included under the if
    ਤਾਰਕਿਕ is_Simplifiable_ਗੰਢ(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(nodep, ਗੰਢ_ਸੌਂਪੋ)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            ਜੇ (assignp->lhsp()->ਚੌੜਾਈ_Min() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                ਜੇ (ਅਚਲ AstAnd* ਅਚਲ andp = VN_CAST(assignp->rhsp(), And)) {
                    ਜੇ (andp->lhsp()->same_ਰੁੱਖ(m_mgCondp) || andp->rhsp()->same_ਰੁੱਖ(m_mgCondp)) {
                        ਵਾਪਸ ਸੱਚ;
                    }
                }
                // Is it 'lhs = cond'?
                ਜੇ (assignp->rhsp()->same_ਰੁੱਖ(m_mgCondp)) ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਤਾਰਕਿਕ ਜੋੜੋ_ਮੰਜ਼ਿਲ_List(Ast_ਗੰਢ_Stmt* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        // Set up head of new list if node is first in list
        ਜੇ (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](ਅਚਲ AstVarRef* nodep) {
                AstVar* ਅਚਲ varp = nodep->varp();
                varp->user1(1);
                ਜੇ (varp->is_Sig_ਜਨਤਕ() || varp->isWrittenByDpi()) m_mgCondPubWritable = ਸੱਚ;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            ਜੇ ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();
                m_mgCondPubWritable = ਝੂਠ;
                // We did not add to the list
                ਵਾਪਸ ਝੂਠ;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            ਜਦੋਂ (ਸੱਚ) {
                Ast_ਗੰਢ_Stmt* ਅਚਲ backp = VN_CAST(m_mgFirstp->backp(), ਗੰਢ_Stmt);
                ਜੇ (!backp || backp->nextp() != m_mgFirstp) ਰੋਕੋ;  // Don't move up the tree
                ਅਚਲ StmtProperties& props = (*m_stmtPropertiesp)(backp);
                ਜੇ (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) ਰੋਕੋ;
                ਜੇ (is_Simplifiable_ਗੰਢ(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ਨਹੀਂਤਾਂ ਜੇ (is_Cheap_ਗੰਢ(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ਨਹੀਂਤਾਂ {
                    ਰੋਕੋ;
                }
            }
        }
        // Add node
        ++m_listLenght;
        // Track end of list
        m_mgLastp = nodep;
        // Set up expected next node in list.
        m_mgNextp = nodep->nextp();
        // If last under parent, done with current list
        ਜੇ (!m_mgNextp) ਮਿਲਾਓ_ਸਿਰਾ();
        // Statistics
        ++m_stat_Candidate_ਚੀਜ਼ਾਂ;
        // We did add to the list
        ਵਾਪਸ ਸੱਚ;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    ਤਾਰਕਿਕ ਜੋੜੋ_If_Helpful_Else_ਸਿਰਾ_ਮਿਲਾਓ(Ast_ਗੰਢ_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        ਜੇ (!checkOrMakeMergeable(nodep)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!m_mgFirstp) ਵਾਪਸ ਝੂਠ;  // If 'checkOrMakeMergeable' closed the list
        ਜੇ (m_mgNextp == nodep) {
            ਜੇ (is_Simplifiable_ਗੰਢ(nodep)) {
                ਜੇ (ਜੋੜੋ_ਮੰਜ਼ਿਲ_List(nodep, ਖਾਲੀ_ਸੂਚਕ)) ਵਾਪਸ ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (is_Cheap_ਗੰਢ(nodep)) {
                nodep->user2(1);
                ਜੇ (ਜੋੜੋ_ਮੰਜ਼ਿਲ_List(nodep, ਖਾਲੀ_ਸੂਚਕ)) ਵਾਪਸ ਸੱਚ;
            }
        }
        // Not added to list, so we are done with the current list
        ਮਿਲਾਓ_ਸਿਰਾ();
        ਵਾਪਸ ਝੂਠ;
    }

    ਤਾਰਕਿਕ checkOrMakeMergeable(ਅਚਲ Ast_ਗੰਢ_Stmt* nodep) {
        ਅਚਲ StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        ਜੇ (props.m_isFence) ਵਾਪਸ ਝੂਠ;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        ਜੇ (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) ਮਿਲਾਓ_ਸਿਰਾ();
        ਵਾਪਸ ਸੱਚ;  // Now surely mergeable
    }

    ਖਾਲੀ ਮਿਲਾਓ_ਸਿਰਾ_If_Incompatible(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ Ast_ਗੰਢ* condp) {
        ਜੇ (m_mgFirstp && (m_mgNextp != nodep || !condp->same_ਰੁੱਖ(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            ਮਿਲਾਓ_ਸਿਰਾ();
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            ਜੇ (!checkOrMakeMergeable(nodep)) ਵਾਪਸ;
            // Close potentially incompatible pending merge
            ਮਿਲਾਓ_ਸਿਰਾ_If_Incompatible(nodep, condp);
            // Add current node
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_List(nodep, condp);
        } ਨਹੀਂਤਾਂ ਜੇ (m_mgFirstp) {
            ਜੋੜੋ_If_Helpful_Else_ਸਿਰਾ_ਮਿਲਾਓ(nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        // Check if mergeable
        ਜੇ (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterate_And_ਅਗਲਾ_Null(nodep->thensp());
            iterate_And_ਅਗਲਾ_Null(nodep->elsesp());
            ਵਾਪਸ;
        }
        // Close potentially incompatible pending merge
        ਮਿਲਾਓ_ਸਿਰਾ_If_Incompatible(nodep, nodep->condp());
        // Add current node
        ਜੋੜੋ_ਮੰਜ਼ਿਲ_List(nodep, nodep->condp());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        ਜੇ (m_mgFirstp && ਜੋੜੋ_If_Helpful_Else_ਸਿਰਾ_ਮਿਲਾਓ(nodep)) ਵਾਪਸ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        // Merge function body
        ਜੇ (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override { iterate_And_ਅਗਲਾ_Null(nodep->modulesp()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override { iterate_And_ਅਗਲਾ_Null(nodep->stmtsp()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {}

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਮਿਲਾਓ_Cond_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ਮਿਲਾਓ_Cond_Visitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, MergeCond candidate items", m_stat_Candidate_ਚੀਜ਼ਾਂ);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, MergeCond merged items", m_stat_Merged_ਚੀਜ਼ਾਂ);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

ਖਾਲੀ V3_ਮਿਲਾਓ_Cond::ਮਿਲਾਓ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ਮਿਲਾਓ_Cond_Visitor{nodep}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("merge_cond", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
