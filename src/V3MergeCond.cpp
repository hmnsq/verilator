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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3MergeCond.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3DupFinder.h"
#ಅನ್ತರ್ಭಾವ "V3Hasher.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <queue>
#ಅನ್ತರ್ಭಾವ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_ಗಂಟು* rhsp) {
    ಯದಿ (AstCond* ಧ್ರುವ condp = VN_CAST(rhsp, Cond)) {
        ಪ್ರತ್ಯಾಗಮ condp;
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ andp = VN_CAST(rhsp, And)) {
        ಯದಿ (AstCond* ಧ್ರುವ condp = VN_CAST(andp->rhsp(), Cond)) {
            ಯದಿ (VN_IS(andp->lhsp(), Const)) ಪ್ರತ್ಯಾಗಮ condp;
        }
    }
    ಪ್ರತ್ಯಾಗಮ nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
ತಾರ್ಕಿಕ areDisjoint(ಧ್ರುವ std::set<ಧ್ರುವ AstVar*>& a, ಧ್ರುವ std::set<ಧ್ರುವ AstVar*>& b) {
    ಯದಿ (a.empty() || b.empty()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    ಧ್ರುವ auto endA = a.end();
    ಧ್ರುವ auto endB = b.end();
    auto itA = a.begin();
    auto itB = b.begin();
    ಯಾವತ್ (ಸತ್ಯ) {
        ಯದಿ (*itA == *itB) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (std::less<ಧ್ರುವ AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            ಯದಿ (itA == endA) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            itB = std::lower_bound(++itB, endB, *itA);
            ಯದಿ (itB == endB) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

ಸಂರಚನಾ StmtProperties final {
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<ಧ್ರುವ AstVar*> m_rdVars;  // Variables read by this statement
    std::set<ಧ್ರುವ AstVar*> m_wrVars;  // Variables written by this statement
    ತಾರ್ಕಿಕ m_isFence = ಅಸತ್ಯ;  // Nothing should move across this statement, nor should it be merged
    ತಾರ್ಕಿಕ m_sideEffect = ಅಸತ್ಯ;  // Statement may have side effect, without access to model state
    ತಾರ್ಕಿಕ m_implPubRd = ಅಸತ್ಯ;  // Statement may implicitly read public state (without VarRef)
    ತಾರ್ಕಿಕ m_implPubWr = ಅಸತ್ಯ;  // Statement may implicitly write public state (without VarRef)
    ತಾರ್ಕಿಕ m_explPubRef = ಅಸತ್ಯ;  // Statement explicitly references public state via VarRef
    Ast_ಗಂಟು_Stmt* m_prevWithSameCondp = nullptr;  // Previous node in same list, with same condition
    ತಾರ್ಕಿಕ writesConditionVar(ತಾರ್ಕಿಕ condPubWritable) ಧ್ರುವ {
        // This relies on MarkVarsVisitor having been called on the condition node
        ಪ್ರತಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp : m_wrVars) {
            ಯದಿ (varp->user1()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        // If the condition contains a public variable, check if it might be written
        ಯದಿ (condPubWritable && m_implPubWr) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Otherwise not written
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<Ast_ಗಂಟು_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
ವರ್ಗ CodeMotionAnalysisVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
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
    std::vector<V3DupFinder> m_ರಾಶಿ;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* extractCondition(ಧ್ರುವ Ast_ಗಂಟು_Stmt* nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* conditionp = nullptr;
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Assign* ಧ್ರುವ assignp = VN_CAST(nodep, ಗಂಟು_Assign)) {
            ಯದಿ (AstCond* ಧ್ರುವ conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_If* ಧ್ರುವ ifp = VN_CAST(nodep, ಗಂಟು_If)) {
            conditionp = ifp->condp();
        }
        ಯಾವತ್ (AstCCast* ಧ್ರುವ castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        ಪ್ರತ್ಯಾಗಮ conditionp;
    }

    ಶೂನ್ಯ analyzeStmt(Ast_ಗಂಟು_Stmt* nodep, ತಾರ್ಕಿಕ tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* ಧ್ರುವ outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            ಯದಿ (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_ರಾಶಿ.back();
                // Find a duplicate condition
                ಧ್ರುವ V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                ಯದಿ (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.ಸೇರಿಸು(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } ಅನ್ಯಥಾ {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ firstp = VN_AS(dit->second, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_ಗಂಟು_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_ಗಂಟು(nodep);

        // If there is an enclosing statement, propagate properties upwards
        ಯದಿ (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.ಸೇರಿಸು(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.ಸೇರಿಸು(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    ಶೂನ್ಯ analyzeVarRef(AstVarRef* nodep) {
        ಧ್ರುವ VAccess access = nodep->access();
        AstVar* ಧ್ರುವ varp = nodep->varp();
        // Gather read and written variables
        ಯದಿ (access.isReadOrRW()) m_propsp->m_rdVars.ಸೇರಿಸು(varp);
        ಯದಿ (access.isWriteOrRW()) m_propsp->m_wrVars.ಸೇರಿಸು(varp);
        ಯದಿ (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = ಸತ್ಯ;
        }
    }

    ಶೂನ್ಯ checkProperties(Ast_ಗಂಟು* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        ಯದಿ (VN_IS(nodep, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) ಪ್ರತ್ಯಾಗಮ;

        // Call to a DPI import
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_CCall* ಧ್ರುವ ccallp = VN_CAST(nodep, ಗಂಟು_CCall)) {
            AstCFunc* ಧ್ರುವ funcp = ccallp->funcp();
            ಯದಿ (funcp->dpiImportWrapper()) {
                ಯದಿ (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = ಸತ್ಯ;
                    m_propsp->m_implPubRd = ಸತ್ಯ;
                    m_propsp->m_implPubWr = ಸತ್ಯ;
                }
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        // Side effect, but assume does not access public state
        ಯದಿ (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ;
        }

        // If impure, or branch, mark statement as fence
        ಯದಿ (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = ಸತ್ಯ;
    }

    ಶೂನ್ಯ analyze_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        // Check properties of this node if under a statement
        ಯದಿ (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        ತಾರ್ಕಿಕ singletonListStart = ಅಸತ್ಯ;
        ಯದಿ (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            ಯದಿ (!singletonListStart) m_ರಾಶಿ.emplace_back(m_hasher);
        }

        // Analyze node
        ಯದಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp = VN_CAST(nodep, ಗಂಟು_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } ಅನ್ಯಥಾ ಯದಿ (AstVarRef* ಧ್ರುವ vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } ಅನ್ಯಥಾ {
            analyze_ಗಂಟು(nodep);
        }

        // Pop the stack at the end of a list
        ಯದಿ (!singletonListStart && !nodep->nextp()) m_ರಾಶಿ.ತೆಗೆ_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(Ast_ಗಂಟು* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterateAndNextConstNull(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    ಸ್ಥಿರ ಶೂನ್ಯ analyze(Ast_ಗಂಟು* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

ವರ್ಗ CodeMotionOptimizeVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    ಸ್ಥಿರ constexpr unsigned MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    ಧ್ರುವ StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    ತಾರ್ಕಿಕ areSwappable(ಧ್ರುವ Ast_ಗಂಟು_Stmt* ap, ಧ್ರುವ Ast_ಗಂಟು_Stmt* bp) ಧ್ರುವ {
        ಧ್ರುವ StmtProperties& aProps = m_stmtProperties(ap);
        ಧ್ರುವ StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        ಯದಿ (aProps.m_isFence) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (bProps.m_isFence) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Don't swap side effecting statements, but can move others across them
        ಯದಿ (aProps.m_sideEffect && bProps.m_sideEffect) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Don't swap if there is a hazard around public state - must observe ordering
        ಧ್ರುವ ತಾರ್ಕಿಕ bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        ಯದಿ (aProps.m_implPubWr && bPubRef) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ ತಾರ್ಕಿಕ aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        ಯದಿ (aPubRef && bProps.m_implPubWr) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // If either statement writes a variable that the other reads, they are not swappable
        ಯದಿ (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // If they both write to the same variable, they are not swappable
        ಯದಿ (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Otherwise good to go
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        // Process only on first encounter
        ಯದಿ (nodep->user4SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_ಗಂಟು_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        ಯದಿ (!prevp) ಪ್ರತ್ಯಾಗಮ;
#ifdef VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            ಧ್ರುವ Ast_ಗಂಟು* currp = prevp;
            ಯಾವತ್ (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#endif
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        ಯದಿ (Ast_ಗಂಟು_Stmt* predp = VN_CAST(nodep->backp(), ಗಂಟು_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            ಪ್ರತಿ (unsigned i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                ಯದಿ (predp == prevp) ಭಙ್ಗ;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_ಗಂಟು_Stmt* ಧ್ರುವ backp = VN_CAST(predp->backp(), ಗಂಟು_Stmt);
                ಯದಿ (!backp) ಭಙ್ಗ;
                // Don't swap statements if doing so would change program semantics
                ಯದಿ (!areSwappable(predp, nodep)) ಭಙ್ಗ;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            ಯದಿ (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->addNextHere(nodep);
                // If the predecessor is the previous node with the same condition, job done
                ಯದಿ (predp == prevp) ಪ್ರತ್ಯಾಗಮ;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            ಯದಿ (Ast_ಗಂಟು_Stmt* succp = VN_CAST(prevp->nextp(), ಗಂಟು_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                ಪ್ರತಿ (unsigned i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    ಯದಿ (succp == currp) ಭಙ್ಗ;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_ಗಂಟು_Stmt* ಧ್ರುವ nextp = VN_CAST(succp->nextp(), ಗಂಟು_Stmt);
                    ಯದಿ (!nextp) ಭಙ್ಗ;
                    // Don't swap statements if doing so would change program semantics
                    ಯದಿ (!areSwappable(prevp, succp)) ಭಙ್ಗ;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                ಯದಿ (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->addHereThisAsNext(prevp);
                }
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(Ast_ಗಂಟು* nodep, ಧ್ರುವ StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterateAndNextNull(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    ಸ್ಥಿರ Ast_ಗಂಟು* optimize(Ast_ಗಂಟು* nodep, ಧ್ರುವ StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        ಯಾವತ್ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
};

//######################################################################
// Conditional merging

ವರ್ಗ MergeCondVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
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
    VDouble0 m_stat_Merged_ವಸ್ತುಗಳು;  // Statistic tracking
    VDouble0 m_stat_Candidate_ವಸ್ತುಗಳು;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_ಗಂಟು* m_mgFirstp = nullptr;  // First node in merged sequence
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_mgCondp = nullptr;  // The condition of the first node
    ಧ್ರುವ Ast_ಗಂಟು* m_mgLastp = nullptr;  // Last node in merged sequence
    ಧ್ರುವ Ast_ಗಂಟು* m_mgNextp = nullptr;  // Next node in list being examined
    ತಾರ್ಕಿಕ m_mgCondPubWritable = ಅಸತ್ಯ;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_ಗಂಟು*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    ಶೂನ್ಯ process(Ast_ಗಂಟು* nodep) {
        // Set up work queue
        std::queue<Ast_ಗಂಟು*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->ತಳ್ಳು(nodep);

        ಕುರು {
            // Set up user* for this iteration
            ಧ್ರುವ VNUser1InUse user1InUse;
            ಧ್ರುವ VNUser2InUse user2InUse;
            ಧ್ರುವ VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_ಗಂಟು* currp = m_workQueuep->front();
            m_workQueuep->ತೆಗೆ();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            ಯದಿ (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterateAndNextNull(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            ಯದಿ (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } ಯಾವತ್ (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    ಸ್ಥಿರ Ast_ಗಂಟು* skipConstSels(Ast_ಗಂಟು* nodep) {
        ಯಾವತ್ (ಧ್ರುವ AstArraySel* ಧ್ರುವ aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            ಯದಿ (!VN_IS(aselp->bitp(), Const)) ಪ್ರತ್ಯಾಗಮ nodep;
            nodep = aselp->fromp();
        }
        ಯಾವತ್ (ಧ್ರುವ AstWordSel* ಧ್ರುವ wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            ಯದಿ (!VN_IS(wselp->bitp(), Const)) ಪ್ರತ್ಯಾಗಮ nodep;
            nodep = wselp->fromp();
        }
        ಪ್ರತ್ಯಾಗಮ nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Cheap_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        // Comments are cheap
        ಯದಿ (VN_IS(nodep, Comment)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // So are some assignments
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Assign* ಧ್ರುವ assignp = VN_CAST(nodep, ಗಂಟು_Assign)) {
            // Check LHS
            Ast_ಗಂಟು* ಧ್ರುವ lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            ಯದಿ (!VN_IS(lhsp, VarRef)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

            // Check RHS
            Ast_ಗಂಟು* ಧ್ರುವ rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            ಯದಿ (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

            // Otherwise it is a cheap assignment
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        // Others are not
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    ಸ್ಥಿರ ತಾರ್ಕಿಕ yieldsOneOrZero(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) {
            ಪ್ರತ್ಯಾಗಮ constp->ಅಂಕೆ().toUQuad() <= 1;
        }
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ vrefp = VN_CAST(nodep, VarRef)) {
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = vrefp->varp();
            ಪ್ರತ್ಯಾಗಮ varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        ಯದಿ (ಧ್ರುವ AstShiftR* ಧ್ರುವ shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(shiftp->rhsp(), Const)) {
                ಧ್ರುವ AstVarRef* ಧ್ರುವ vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                ಯದಿ (constp->toSInt() >= width - 1) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ biopp = VN_CAST(nodep, ಗಂಟು_Biop)) {
            ಯದಿ (VN_IS(nodep, And))
                ಪ್ರತ್ಯಾಗಮ yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            ಯದಿ (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                ಪ್ರತ್ಯಾಗಮ yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (ಧ್ರುವ AstCond* ಧ್ರುವ condp = VN_CAST(nodep, Cond)) {
            ಪ್ರತ್ಯಾಗಮ yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        ಯದಿ (ಧ್ರುವ AstCCast* ಧ್ರುವ castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            ಪ್ರತ್ಯಾಗಮ yieldsOneOrZero(castp->lhsp());
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maskLsb(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (yieldsOneOrZero(nodep)) ಪ್ರತ್ಯಾಗಮ nodep;
        // Otherwise apply masking
        AstConst* ಧ್ರುವ maskp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* foldAndUnlink(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ತಾರ್ಕಿಕ condTrue) {
        ಯದಿ (rhsp->same_ವೃಕ್ಷ(m_mgCondp)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstCond* ಧ್ರುವ condp = extractCondFromRhs(rhsp)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            ಯದಿ (condp == rhsp) ಪ್ರತ್ಯಾಗಮ resp;
            ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAnd{andp->fileline(), andp->lhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), resp};
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ andp = VN_CAST(rhsp, And)) {
            ಯದಿ (andp->lhsp()->same_ವೃಕ್ಷ(m_mgCondp)) {
                ಪ್ರತ್ಯಾಗಮ condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : ನೂತನ AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(andp->rhsp()->same_ವೃಕ್ಷ(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                ಪ್ರತ್ಯಾಗಮ condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : ನೂತನ AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            ಪ್ರತ್ಯಾಗಮ rhsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }
        // LCOV_EXCL_START
        ಯದಿ (debug()) rhsp->dump_ವೃಕ್ಷ("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        ಪ್ರತ್ಯಾಗಮ nullptr;
        // LCOV_EXCL_STOP
    }

    ಶೂನ್ಯ mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        ಯಾವತ್ (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        ಯಾವತ್ (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_ಗಂಟು_If* recursivep = nullptr;
        // Merge if list is longer than one node
        ಯದಿ (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            ಯದಿ (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* ಧ್ರುವ resultp = ನೂತನ AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->addHereThisAsNext(resultp);
            // Try to preserve branch prediction if we can
            size_t nLikely = 0;
            size_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_ಗಂಟು* nextp = m_mgFirstp;
            ಕುರು {
                // Grab next pointer and unlink
                Ast_ಗಂಟು* ಧ್ರುವ currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                ಯದಿ (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_ವೃಕ್ಷ(), currp);
                    ಅನುವರ್ತ;
                }
                //ಎಣಿಕೆ
                ++m_stat_Merged_ವಸ್ತುಗಳು;
                ಯದಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ assignp = VN_CAST(currp, ಗಂಟು_Assign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = assignp->rhsp()->unlinkFrBack();
                    Ast_ಗಂಟು_Assign* ಧ್ರುವ thenp = assignp;
                    Ast_ಗಂಟು_Assign* ಧ್ರುವ elsep = assignp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, ಸತ್ಯ));
                    elsep->rhsp(foldAndUnlink(rhsp, ಅಸತ್ಯ));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_ವೃಕ್ಷ(), rhsp);
                } ಅನ್ಯಥಾ {
                    Ast_ಗಂಟು_If* ಧ್ರುವ ifp = VN_AS(currp, ಗಂಟು_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlinkFrBackWithNext());
                    }
                    ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlinkFrBackWithNext());
                    }
                    // Record branch prediction
                    ಯದಿ (ifp->branchPred().likely()) ++nLikely;
                    ಯದಿ (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_ವೃಕ್ಷ(), ifp);
                }
            } ಯಾವತ್ (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            ಯದಿ (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            ಯದಿ (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            ಯದಿ (resultp->thensp()) m_workQueuep->ತಳ್ಳು(resultp->thensp());
            ಯದಿ (resultp->elsesp()) m_workQueuep->ತಳ್ಳು(resultp->elsesp());
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_If* ಧ್ರುವ ifp = VN_CAST(m_mgFirstp, ಗಂಟು_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = nullptr;
        m_mgCondp = nullptr;
        m_mgLastp = nullptr;
        m_mgNextp = nullptr;
        m_mgCondPubWritable = ಅಸತ್ಯ;
        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();  // Clear marked variables
        Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
        // Merge recursively within the branches of an un-merged AstNodeIF
        ಯದಿ (recursivep) {
            iterateAndNextNull(recursivep->thensp());
            iterateAndNextNull(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            ಯದಿ (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    ತಾರ್ಕಿಕ is_Simplifiable_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Assign* ಧ್ರುವ assignp = VN_CAST(nodep, ಗಂಟು_Assign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            ಯದಿ (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                ಯದಿ (ಧ್ರುವ AstAnd* ಧ್ರುವ andp = VN_CAST(assignp->rhsp(), And)) {
                    ಯದಿ (andp->lhsp()->same_ವೃಕ್ಷ(m_mgCondp) || andp->rhsp()->same_ವೃಕ್ಷ(m_mgCondp)) {
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
                // Is it 'lhs = cond'?
                ಯದಿ (assignp->rhsp()->same_ವೃಕ್ಷ(m_mgCondp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ addToList(Ast_ಗಂಟು_Stmt* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        // Set up head of new list if node is first in list
        ಯದಿ (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](ಧ್ರುವ AstVarRef* nodep) {
                AstVar* ಧ್ರುವ varp = nodep->varp();
                varp->user1(1);
                ಯದಿ (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = ಸತ್ಯ;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            ಯದಿ ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
                m_mgCondPubWritable = ಅಸತ್ಯ;
                // We did not add to the list
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            ಯಾವತ್ (ಸತ್ಯ) {
                Ast_ಗಂಟು_Stmt* ಧ್ರುವ backp = VN_CAST(m_mgFirstp->backp(), ಗಂಟು_Stmt);
                ಯದಿ (!backp || backp->nextp() != m_mgFirstp) ಭಙ್ಗ;  // Don't move up the tree
                ಧ್ರುವ StmtProperties& props = (*m_stmtPropertiesp)(backp);
                ಯದಿ (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) ಭಙ್ಗ;
                ಯದಿ (is_Simplifiable_ಗಂಟು(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ಅನ್ಯಥಾ ಯದಿ (is_Cheap_ಗಂಟು(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ಅನ್ಯಥಾ {
                    ಭಙ್ಗ;
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
        ಯದಿ (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_stat_Candidate_ವಸ್ತುಗಳು;
        // We did add to the list
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    ತಾರ್ಕಿಕ addIfHelpfulElseEndMerge(Ast_ಗಂಟು_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        ಯದಿ (!checkOrMakeMergeable(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!m_mgFirstp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // If 'checkOrMakeMergeable' closed the list
        ಯದಿ (m_mgNextp == nodep) {
            ಯದಿ (is_Simplifiable_ಗಂಟು(nodep)) {
                ಯದಿ (addToList(nodep, nullptr)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (is_Cheap_ಗಂಟು(nodep)) {
                nodep->user2(1);
                ಯದಿ (addToList(nodep, nullptr)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ checkOrMakeMergeable(ಧ್ರುವ Ast_ಗಂಟು_Stmt* nodep) {
        ಧ್ರುವ StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        ಯದಿ (props.m_isFence) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        ಯದಿ (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Now surely mergeable
    }

    ಶೂನ್ಯ mergeEndIfIncompatible(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ Ast_ಗಂಟು* condp) {
        ಯದಿ (m_mgFirstp && (m_mgNextp != nodep || !condp->same_ವೃಕ್ಷ(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            ಯದಿ (!checkOrMakeMergeable(nodep)) ಪ್ರತ್ಯಾಗಮ;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } ಅನ್ಯಥಾ ಯದಿ (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        // Check if mergeable
        ಯದಿ (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterateAndNextNull(nodep->thensp());
            iterateAndNextNull(nodep->elsesp());
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        ಯದಿ (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        // Merge function body
        ಯದಿ (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    ಶೂನ್ಯ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {}

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_stat_Candidate_ವಸ್ತುಗಳು);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_stat_Merged_ವಸ್ತುಗಳು);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

ಶೂನ್ಯ V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("merge_cond", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
