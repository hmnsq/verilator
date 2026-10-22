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
AstCond* ಹೊರತೆಗೆ_Cond_From_Rhs(Ast_ಗಂಟು* rhsp) {
    ಆದರೆ (AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(rhsp, Cond)) {
        ಹಿಂತಿರುಗಿ condp;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(rhsp, And)) {
        ಆದರೆ (AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(andp->rhsp(), Cond)) {
            ಆದರೆ (VN_IS(andp->lhsp(), Const)) ಹಿಂತಿರುಗಿ condp;
        }
    }
    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
ತಾರ್ಕಿಕ areDisjoint(ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& a, ಸ್ಥಿರಾಂಕ std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& b) {
    ಆದರೆ (a.ಖಾಲಿಗಳು() || b.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ನಿಜ;
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೊನೆ_A = a.ಕೊನೆ();
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೊನೆ_B = b.ಕೊನೆ();
    ಸ್ವಯಂಚಾಲಿತ itA = a.begin();
    ಸ್ವಯಂಚಾಲಿತ itB = b.begin();
    ಆಗ (ನಿಜ) {
        ಆದರೆ (*itA == *itB) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (std::less<ಸ್ಥಿರಾಂಕ AstVar*>{}(*itA, *itB)) {
            itA = std::ಅಧೋ_bound(++itA, ಕೊನೆ_A, *itB);
            ಆದರೆ (itA == ಕೊನೆ_A) ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            itB = std::ಅಧೋ_bound(++itB, ಕೊನೆ_B, *itA);
            ಆದರೆ (itB == ಕೊನೆ_B) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

ರಚನೆ StmtProperties final {
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_condp = ಶೂನ್ಯಸೂಚಕ;  // The condition expression, if a conditional node
    std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> m_rdVars;  // Variables read by this statement
    std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> m_wrVars;  // Variables written by this statement
    ತಾರ್ಕಿಕ m_isFence = ಸುಳ್ಳು;  // Nothing should move across this statement, nor should it be merged
    ತಾರ್ಕಿಕ m_sideEffect = ಸುಳ್ಳು;  // Statement may have side effect, without access to model state
    ತಾರ್ಕಿಕ m_implPubRd = ಸುಳ್ಳು;  // Statement may implicitly read public state (without VarRef)
    ತಾರ್ಕಿಕ m_implPubWr = ಸುಳ್ಳು;  // Statement may implicitly write public state (without VarRef)
    ತಾರ್ಕಿಕ m_explPubRef = ಸುಳ್ಳು;  // Statement explicitly references public state via VarRef
    Ast_ಗಂಟು_Stmt* m_prevWithSameCondp = ಶೂನ್ಯಸೂಚಕ;  // Previous node in same list, with same condition
    ತಾರ್ಕಿಕ writesConditionVar(ತಾರ್ಕಿಕ condPubWritable) ಸ್ಥಿರಾಂಕ {
        // This relies on MarkVarsVisitor having been called on the condition node
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp : m_wrVars) {
            ಆದರೆ (varp->user1()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        // If the condition contains a public variable, check if it might be written
        ಆದರೆ (condPubWritable && m_implPubWr) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Otherwise not written
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
ಬಳಸಿ StmtPropertiesAllocator = AstUser3Allocator<Ast_ಗಂಟು_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
ವರ್ಗ ಸಂಕೇತ_Motion_Analysis_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
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
    std::ಸದಿಶ<V3DupFinder> m_ರಾಶಿ;
    StmtProperties* m_propsp = ಶೂನ್ಯಸೂಚಕ;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊರತೆಗೆ_Condition(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Stmt* nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* conditionp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(nodep, ಗಂಟು_ನಿಯೋಜಿಸು)) {
            ಆದರೆ (AstCond* ಸ್ಥಿರಾಂಕ conditionalp = ಹೊರತೆಗೆ_Cond_From_Rhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(nodep, ಗಂಟು_If)) {
            conditionp = ifp->condp();
        }
        ಆಗ (AstCCast* ಸ್ಥಿರಾಂಕ castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        ಹಿಂತಿರುಗಿ conditionp;
    }

    ಶೂನ್ಯ analyzeStmt(Ast_ಗಂಟು_Stmt* nodep, ತಾರ್ಕಿಕ try_Cond_ಹೊಂದಿಕೆ) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* ಸ್ಥಿರಾಂಕ outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ಹೊರತೆಗೆ_Condition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            ಆದರೆ (try_Cond_ಹೊಂದಿಕೆ) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_ರಾಶಿ.ಹಿಂಭಾಗ();
                // Find a duplicate condition
                ಸ್ಥಿರಾಂಕ V3DupFinder::iterator& dit = dupFinder.ಕಂಡುಹಿಡಿ_Duplicate(condp);
                ಆದರೆ (dit == dupFinder.ಕೊನೆ()) {
                    // First time seeing this condition in the current list
                    dupFinder.ಸೇರಿಸು(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ firstp = VN_AS(dit->ಎರಡನೇ, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
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
        ಆದರೆ (outerPropsp) {
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
        ಸ್ಥಿರಾಂಕ VAccess access = nodep->access();
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        // Gather read and written variables
        ಆದರೆ (access.isReadOrRW()) m_propsp->m_rdVars.ಸೇರಿಸು(varp);
        ಆದರೆ (access.isWriteOrRW()) m_propsp->m_wrVars.ಸೇರಿಸು(varp);
        ಆದರೆ (varp->is_Sig_ಬಹಿರಂಗ() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = ನಿಜ;
        }
    }

    ಶೂನ್ಯ checkProperties(Ast_ಗಂಟು* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        ಆದರೆ (VN_IS(nodep, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) ಹಿಂತಿರುಗಿ;

        // Call to a DPI import
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_CCall* ಸ್ಥಿರಾಂಕ ccallp = VN_CAST(nodep, ಗಂಟು_CCall)) {
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ccallp->funcp();
            ಆದರೆ (funcp->dpiImportWrapper()) {
                ಆದರೆ (!funcp->dpi_ಶುದ್ಧ()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = ನಿಜ;
                    m_propsp->m_implPubRd = ನಿಜ;
                    m_propsp->m_implPubWr = ನಿಜ;
                }
                ಹಿಂತಿರುಗಿ;
            }
        }

        // Side effect, but assume does not access public state
        ಆದರೆ (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = ನಿಜ;
            ಹಿಂತಿರುಗಿ;
        }

        // If impure, or branch, mark statement as fence
        ಆದರೆ (!nodep->is_ಶುದ್ಧ() || nodep->isBrancher()) m_propsp->m_isFence = ನಿಜ;
    }

    ಶೂನ್ಯ analyze_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        // Check properties of this node if under a statement
        ಆದರೆ (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        ತಾರ್ಕಿಕ singleton_List_ಆರಂಭ = ಸುಳ್ಳು;
        ಆದರೆ (nodep->backp()->nextp() != nodep) {  // If at head of list
            singleton_List_ಆರಂಭ = nodep->nextp() == ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!singleton_List_ಆರಂಭ) m_ರಾಶಿ.emplace_ಹಿಂಭಾಗ(m_hasher);
        }

        // Analyze node
        ಆದರೆ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp = VN_CAST(nodep, ಗಂಟು_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singleton_List_ಆರಂಭ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            analyze_ಗಂಟು(nodep);
        }

        // Pop the stack at the end of a list
        ಆದರೆ (!singleton_List_ಆರಂಭ && !nodep->nextp()) m_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
    }

    // CONSTRUCTOR
    ಸಂಕೇತ_Motion_Analysis_Visitor(Ast_ಗಂಟು* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterate_And_ಮುಂದಿನ_Const_Null(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    ಸ್ಥಿರ ಶೂನ್ಯ analyze(Ast_ಗಂಟು* nodep, StmtPropertiesAllocator& stmtProperties) {
        ಸಂಕೇತ_Motion_Analysis_Visitor{nodep, stmtProperties};
    }
};

ವರ್ಗ ಸಂಕೇತ_Motion_Optimize_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    ಸ್ಥಿರ constexpr ಚಿಹ್ನೆರಹಿತ MAX_ದೂರ = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4_ಒಳ_Use m_user4_ಒಳ_Use;

    ಸ್ಥಿರಾಂಕ StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    ತಾರ್ಕಿಕ areSwappable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Stmt* ap, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Stmt* bp) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ StmtProperties& aProps = m_stmtProperties(ap);
        ಸ್ಥಿರಾಂಕ StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        ಆದರೆ (aProps.m_isFence) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (bProps.m_isFence) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Don't swap side effecting statements, but can move others across them
        ಆದರೆ (aProps.m_sideEffect && bProps.m_sideEffect) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Don't swap if there is a hazard around public state - must observe ordering
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        ಆದರೆ (aProps.m_implPubWr && bPubRef) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        ಆದರೆ (aPubRef && bProps.m_implPubWr) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // If either statement writes a variable that the other reads, they are not swappable
        ಆದರೆ (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // If they both write to the same variable, they are not swappable
        ಆದರೆ (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Otherwise good to go
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        // Process only on first encounter
        ಆದರೆ (nodep->user4_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_ಗಂಟು_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        ಆದರೆ (!prevp) ಹಿಂತಿರುಗಿ;
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* currp = prevp;
            ಆಗ (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#ಆದರೆ_ಅಂತ್ಯ
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        ಆದರೆ (Ast_ಗಂಟು_Stmt* predp = VN_CAST(nodep->backp(), ಗಂಟು_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = MAX_ದೂರ; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                ಆದರೆ (predp == prevp) ನಿಲ್ಲಿಸು;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ backp = VN_CAST(predp->backp(), ಗಂಟು_Stmt);
                ಆದರೆ (!backp) ನಿಲ್ಲಿಸು;
                // Don't swap statements if doing so would change program semantics
                ಆದರೆ (!areSwappable(predp, nodep)) ನಿಲ್ಲಿಸು;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            ಆದರೆ (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlink_Fr_ಹಿಂಭಾಗ();
                predp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(nodep);
                // If the predecessor is the previous node with the same condition, job done
                ಆದರೆ (predp == prevp) ಹಿಂತಿರುಗಿ;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        ಗಾಗಿ (Ast_ಗಂಟು_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            ಆದರೆ (Ast_ಗಂಟು_Stmt* succp = VN_CAST(prevp->nextp(), ಗಂಟು_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = MAX_ದೂರ; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    ಆದರೆ (succp == currp) ನಿಲ್ಲಿಸು;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ nextp = VN_CAST(succp->nextp(), ಗಂಟು_Stmt);
                    ಆದರೆ (!nextp) ನಿಲ್ಲಿಸು;
                    // Don't swap statements if doing so would change program semantics
                    ಆದರೆ (!areSwappable(prevp, succp)) ನಿಲ್ಲಿಸು;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                ಆದರೆ (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlink_Fr_ಹಿಂಭಾಗ();
                    succp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(prevp);
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    ಸಂಕೇತ_Motion_Optimize_Visitor(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterate_And_ಮುಂದಿನ_Null(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    ಸ್ಥಿರ Ast_ಗಂಟು* optimize(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ StmtPropertiesAllocator& stmtProperties) {
        { ಸಂಕೇತ_Motion_Optimize_Visitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        ಆಗ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        ಹಿಂತಿರುಗಿ nodep;
    }
};

//######################################################################
// Conditional merging

ವರ್ಗ ವಿಲೀನ_Cond_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
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

    Ast_ಗಂಟು* m_mgFirstp = ಶೂನ್ಯಸೂಚಕ;  // First node in merged sequence
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_mgCondp = ಶೂನ್ಯಸೂಚಕ;  // The condition of the first node
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_mgLastp = ಶೂನ್ಯಸೂಚಕ;  // Last node in merged sequence
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_mgNextp = ಶೂನ್ಯಸೂಚಕ;  // Next node in list being examined
    ತಾರ್ಕಿಕ m_mgCondPubWritable = ಸುಳ್ಳು;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::ಸರತಿ<Ast_ಗಂಟು*>* m_workQueuep = ಶೂನ್ಯಸೂಚಕ;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = ಶೂನ್ಯಸೂಚಕ;

    // METHODS

    // Function that processes a whole sub-tree
    ಶೂನ್ಯ process(Ast_ಗಂಟು* nodep) {
        // Set up work queue
        std::ಸರತಿ<Ast_ಗಂಟು*> work_ಸರತಿ;
        m_workQueuep = &work_ಸರತಿ;
        m_workQueuep->ತಳ್ಳು(nodep);

        ಮಾಡು {
            // Set up user* for this iteration
            ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
            ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
            ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_ಗಂಟು* currp = m_workQueuep->front();
            m_workQueuep->ತೆಗೆ_2();

            // Analyze sub-tree list for code motion and conditional merging
            ಸಂಕೇತ_Motion_Analysis_Visitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            ಆದರೆ (v3Global.opt.f_ವಿಲೀನ_Cond_Motion()) {
                currp = ಸಂಕೇತ_Motion_Optimize_Visitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterate_And_ಮುಂದಿನ_Null(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            ಆದರೆ (m_mgFirstp) ವಿಲೀನ_ಕೊನೆ();
            m_stmtPropertiesp = ಶೂನ್ಯಸೂಚಕ;
        } ಆಗ (!m_workQueuep->ಖಾಲಿಗಳು());

        m_workQueuep = ಶೂನ್ಯಸೂಚಕ;
    }

    // Skip past AstArraySel and AstWordSel with const index
    ಸ್ಥಿರ Ast_ಗಂಟು* skipConstSels(Ast_ಗಂಟು* nodep) {
        ಆಗ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            ಆದರೆ (!VN_IS(aselp->bitp(), Const)) ಹಿಂತಿರುಗಿ nodep;
            nodep = aselp->fromp();
        }
        ಆಗ (ಸ್ಥಿರಾಂಕ Ast_ಶಬ್ದ_Sel* ಸ್ಥಿರಾಂಕ wselp = VN_CAST(nodep, ಶಬ್ದ_Sel)) {
            // WordSel index is not constant, so might be expensive
            ಆದರೆ (!VN_IS(wselp->bitp(), Const)) ಹಿಂತಿರುಗಿ nodep;
            nodep = wselp->fromp();
        }
        ಹಿಂತಿರುಗಿ nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Cheap_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        // Comments are cheap
        ಆದರೆ (VN_IS(nodep, Comment)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // So are some assignments
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(nodep, ಗಂಟು_ನಿಯೋಜಿಸು)) {
            // Check LHS
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            ಆದರೆ (!VN_IS(lhsp, VarRef)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

            // Check RHS
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            ಆದರೆ (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

            // Otherwise it is a cheap assignment
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        // Others are not
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    ಸ್ಥಿರ ತಾರ್ಕಿಕ yieldsOneOrZero(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const)) {
            ಹಿಂತಿರುಗಿ constp->ಅಂಕೆ().ಗಮ್ಯ_UQuad() <= 1;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(nodep, VarRef)) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = vrefp->varp();
            ಹಿಂತಿರುಗಿ varp->ಅಗಲ_Min() == 1 && !varp->dtypep()->isSigned();
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ shiftp = VN_CAST(nodep, ಸರಿಸು_R)) {
            // Shift right by width - 1 or more
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(shiftp->rhsp(), Const)) {
                ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->ಅಗಲ_Min()
                                      : shiftp->ಅಗಲ();
                ಆದರೆ (constp->ಗಮ್ಯ_SInt() >= ಅಗಲ - 1) ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ biopp = VN_CAST(nodep, ಗಂಟು_Biop)) {
            ಆದರೆ (VN_IS(nodep, And))
                ಹಿಂತಿರುಗಿ yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            ಆದರೆ (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                ಹಿಂತಿರುಗಿ yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(nodep, Cond)) {
            ಹಿಂತಿರುಗಿ yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCCast* ಸ್ಥಿರಾಂಕ castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            ಹಿಂತಿರುಗಿ yieldsOneOrZero(castp->lhsp());
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮುಖವಾಡ_Lsb(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (yieldsOneOrZero(nodep)) ಹಿಂತಿರುಗಿ nodep;
        // Otherwise apply masking
        AstConst* ಸ್ಥಿರಾಂಕ maskp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_True{}};
        // Mask on left, as conventional
        ಹಿಂತಿರುಗಿ ಹೊಸ AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* foldAndUnlink(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ತಾರ್ಕಿಕ condTrue) {
        ಆದರೆ (rhsp->same_ವೃಕ್ಷ(m_mgCondp)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{rhsp->fileline(), AstConst::ಬಿಟ್_True{}, condTrue};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCond* ಸ್ಥಿರಾಂಕ condp = ಹೊರತೆಗೆ_Cond_From_Rhs(rhsp)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ resp
                = condTrue ? condp->thenp()->unlink_Fr_ಹಿಂಭಾಗ() : condp->elsep()->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (condp == rhsp) ಹಿಂತಿರುಗಿ resp;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                ಹಿಂತಿರುಗಿ ಹೊಸ AstAnd{andp->fileline(), andp->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), resp};
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(rhsp, And)) {
            ಆದರೆ (andp->lhsp()->same_ವೃಕ್ಷ(m_mgCondp)) {
                ಹಿಂತಿರುಗಿ condTrue ? ಮುಖವಾಡ_Lsb(andp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ())
                                : ಹೊಸ AstConst{rhsp->fileline(), AstConst::ಬಿಟ್_False{}};
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(andp->rhsp()->same_ವೃಕ್ಷ(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                ಹಿಂತಿರುಗಿ condTrue ? ಮುಖವಾಡ_Lsb(andp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ())
                                : ಹೊಸ AstConst{rhsp->fileline(), AstConst::ಬಿಟ್_False{}};
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, ಶಬ್ದ_Sel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            ಹಿಂತಿರುಗಿ rhsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }
        // LCOV_EXCL_START
        ಆದರೆ (debug()) rhsp->dump_ವೃಕ್ಷ("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // LCOV_EXCL_STOP
    }

    ಶೂನ್ಯ ವಿಲೀನ_ಕೊನೆ() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        ಆಗ (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        ಆಗ (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_ಗಂಟು_If* recursivep = ಶೂನ್ಯಸೂಚಕ;
        // Merge if list is longer than one node
        ಆದರೆ (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            ಆದರೆ (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* ಸ್ಥಿರಾಂಕ resultp = ಹೊಸ AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(resultp);
            // Try to preserve branch prediction if we can
            ಗಾತ್ರ_t nLikely = 0;
            ಗಾತ್ರ_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_ಗಂಟು* nextp = m_mgFirstp;
            ಮಾಡು {
                // Grab next pointer and unlink
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : ಶೂನ್ಯಸೂಚಕ;
                currp->unlink_Fr_ಹಿಂಭಾಗ();
                // Skip over comments
                ಆದರೆ (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_ವೃಕ್ಷ(), currp);
                    ಮುಂದುವರಿ;
                }
                //ಎಣಿಕೆ
                ++m_stat_Merged_ವಸ್ತುಗಳು;
                ಆದರೆ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(currp, ಗಂಟು_ನಿಯೋಜಿಸು)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = assignp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ thenp = assignp;
                    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ elsep = assignp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, ನಿಜ));
                    elsep->rhsp(foldAndUnlink(rhsp, ಸುಳ್ಳು));
                    resultp->ಕೂಡಿಸು_Thensp(thenp);
                    resultp->ಕೂಡಿಸು_Elsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_ವೃಕ್ಷ(), rhsp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ ifp = VN_AS(currp, ಗಂಟು_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ listp = ifp->thensp()) {
                        resultp->ಕೂಡಿಸು_Thensp(listp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    }
                    ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ listp = ifp->elsesp()) {
                        resultp->ಕೂಡಿಸು_Elsesp(listp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    }
                    // Record branch prediction
                    ಆದರೆ (ifp->branchPred().likely()) ++nLikely;
                    ಆದರೆ (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_ವೃಕ್ಷ(), ifp);
                }
            } ಆಗ (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            ಆದರೆ (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            ಆದರೆ (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            ಆದರೆ (resultp->thensp()) m_workQueuep->ತಳ್ಳು(resultp->thensp());
            ಆದರೆ (resultp->elsesp()) m_workQueuep->ತಳ್ಳು(resultp->elsesp());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(m_mgFirstp, ಗಂಟು_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = ಶೂನ್ಯಸೂಚಕ;
        m_mgCondp = ಶೂನ್ಯಸೂಚಕ;
        m_mgLastp = ಶೂನ್ಯಸೂಚಕ;
        m_mgNextp = ಶೂನ್ಯಸೂಚಕ;
        m_mgCondPubWritable = ಸುಳ್ಳು;
        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();  // Clear marked variables
        Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
        // Merge recursively within the branches of an un-merged AstNodeIF
        ಆದರೆ (recursivep) {
            iterate_And_ಮುಂದಿನ_Null(recursivep->thensp());
            iterate_And_ಮುಂದಿನ_Null(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            ಆದರೆ (m_mgFirstp) ವಿಲೀನ_ಕೊನೆ();
        }
    }

    // Check if the node can be simplified if included under the if
    ತಾರ್ಕಿಕ is_Simplifiable_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(nodep, ಗಂಟು_ನಿಯೋಜಿಸು)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            ಆದರೆ (assignp->lhsp()->ಅಗಲ_Min() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(assignp->rhsp(), And)) {
                    ಆದರೆ (andp->lhsp()->same_ವೃಕ್ಷ(m_mgCondp) || andp->rhsp()->same_ವೃಕ್ಷ(m_mgCondp)) {
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
                // Is it 'lhs = cond'?
                ಆದರೆ (assignp->rhsp()->same_ವೃಕ್ಷ(m_mgCondp)) ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ತಾರ್ಕಿಕ ಕೂಡಿಸು_ಗಮ್ಯ_List(Ast_ಗಂಟು_Stmt* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        // Set up head of new list if node is first in list
        ಆದರೆ (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](ಸ್ಥಿರಾಂಕ AstVarRef* nodep) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
                varp->user1(1);
                ಆದರೆ (varp->is_Sig_ಬಹಿರಂಗ() || varp->isWrittenByDpi()) m_mgCondPubWritable = ನಿಜ;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            ಆದರೆ ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
                m_mgCondPubWritable = ಸುಳ್ಳು;
                // We did not add to the list
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            ಆಗ (ನಿಜ) {
                Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ backp = VN_CAST(m_mgFirstp->backp(), ಗಂಟು_Stmt);
                ಆದರೆ (!backp || backp->nextp() != m_mgFirstp) ನಿಲ್ಲಿಸು;  // Don't move up the tree
                ಸ್ಥಿರಾಂಕ StmtProperties& props = (*m_stmtPropertiesp)(backp);
                ಆದರೆ (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) ನಿಲ್ಲಿಸು;
                ಆದರೆ (is_Simplifiable_ಗಂಟು(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (is_Cheap_ಗಂಟು(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ನಿಲ್ಲಿಸು;
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
        ಆದರೆ (!m_mgNextp) ವಿಲೀನ_ಕೊನೆ();
        // Statistics
        ++m_stat_Candidate_ವಸ್ತುಗಳು;
        // We did add to the list
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    ತಾರ್ಕಿಕ ಕೂಡಿಸು_If_Helpful_Else_ಕೊನೆ_ವಿಲೀನ(Ast_ಗಂಟು_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        ಆದರೆ (!checkOrMakeMergeable(nodep)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!m_mgFirstp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // If 'checkOrMakeMergeable' closed the list
        ಆದರೆ (m_mgNextp == nodep) {
            ಆದರೆ (is_Simplifiable_ಗಂಟು(nodep)) {
                ಆದರೆ (ಕೂಡಿಸು_ಗಮ್ಯ_List(nodep, ಶೂನ್ಯಸೂಚಕ)) ಹಿಂತಿರುಗಿ ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (is_Cheap_ಗಂಟು(nodep)) {
                nodep->user2(1);
                ಆದರೆ (ಕೂಡಿಸು_ಗಮ್ಯ_List(nodep, ಶೂನ್ಯಸೂಚಕ)) ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        // Not added to list, so we are done with the current list
        ವಿಲೀನ_ಕೊನೆ();
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ತಾರ್ಕಿಕ checkOrMakeMergeable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Stmt* nodep) {
        ಸ್ಥಿರಾಂಕ StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        ಆದರೆ (props.m_isFence) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        ಆದರೆ (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) ವಿಲೀನ_ಕೊನೆ();
        ಹಿಂತಿರುಗಿ ನಿಜ;  // Now surely mergeable
    }

    ಶೂನ್ಯ ವಿಲೀನ_ಕೊನೆ_If_Incompatible(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* condp) {
        ಆದರೆ (m_mgFirstp && (m_mgNextp != nodep || !condp->same_ವೃಕ್ಷ(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            ವಿಲೀನ_ಕೊನೆ();
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            ಆದರೆ (!checkOrMakeMergeable(nodep)) ಹಿಂತಿರುಗಿ;
            // Close potentially incompatible pending merge
            ವಿಲೀನ_ಕೊನೆ_If_Incompatible(nodep, condp);
            // Add current node
            ಕೂಡಿಸು_ಗಮ್ಯ_List(nodep, condp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_mgFirstp) {
            ಕೂಡಿಸು_If_Helpful_Else_ಕೊನೆ_ವಿಲೀನ(nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        // Check if mergeable
        ಆದರೆ (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterate_And_ಮುಂದಿನ_Null(nodep->thensp());
            iterate_And_ಮುಂದಿನ_Null(nodep->elsesp());
            ಹಿಂತಿರುಗಿ;
        }
        // Close potentially incompatible pending merge
        ವಿಲೀನ_ಕೊನೆ_If_Incompatible(nodep, nodep->condp());
        // Add current node
        ಕೂಡಿಸು_ಗಮ್ಯ_List(nodep, nodep->condp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        ಆದರೆ (m_mgFirstp && ಕೂಡಿಸು_If_Helpful_Else_ಕೊನೆ_ವಿಲೀನ(nodep)) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        // Merge function body
        ಆದರೆ (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override { iterate_And_ಮುಂದಿನ_Null(nodep->modulesp()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override { iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {}

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ವಿಲೀನ_Cond_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ವಿಲೀನ_Cond_Visitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, MergeCond candidate items", m_stat_Candidate_ವಸ್ತುಗಳು);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, MergeCond merged items", m_stat_Merged_ವಸ್ತುಗಳು);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

ಶೂನ್ಯ V3_ವಿಲೀನ_Cond::ವಿಲೀನ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ವಿಲೀನ_Cond_Visitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("merge_cond", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
