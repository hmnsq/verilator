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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3MergeCond.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3DupFinder.h"
#অন্তর্ভাব "V3Hasher.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <queue>
#অন্তর্ভাব <set>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_গ্রন্থি* rhsp) {
    যদি (AstCond* ধ্রুব condp = VN_CAST(rhsp, Cond)) {
        প্রত্যাগম condp;
    } অন্যথা যদি (ধ্রুব AstAnd* ধ্রুব andp = VN_CAST(rhsp, And)) {
        যদি (AstCond* ধ্রুব condp = VN_CAST(andp->rhsp(), Cond)) {
            যদি (VN_IS(andp->lhsp(), Const)) প্রত্যাগম condp;
        }
    }
    প্রত্যাগম nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
তার্কিক areDisjoint(ধ্রুব std::set<ধ্রুব AstVar*>& a, ধ্রুব std::set<ধ্রুব AstVar*>& b) {
    যদি (a.empty() || b.empty()) প্রত্যাগম সত্য;
    ধ্রুব auto endA = a.end();
    ধ্রুব auto endB = b.end();
    auto itA = a.begin();
    auto itB = b.begin();
    যাবত্ (সত্য) {
        যদি (*itA == *itB) প্রত্যাগম অসত্য;
        যদি (std::less<ধ্রুব AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            যদি (itA == endA) প্রত্যাগম সত্য;
        } অন্যথা {
            itB = std::lower_bound(++itB, endB, *itA);
            যদি (itB == endB) প্রত্যাগম সত্য;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

সংরচনা StmtProperties final {
    Ast_গ্রন্থি_রাশিমালা* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<ধ্রুব AstVar*> m_rdVars;  // Variables read by this statement
    std::set<ধ্রুব AstVar*> m_wrVars;  // Variables written by this statement
    তার্কিক m_isFence = অসত্য;  // Nothing should move across this statement, nor should it be merged
    তার্কিক m_sideEffect = অসত্য;  // Statement may have side effect, without access to model state
    তার্কিক m_implPubRd = অসত্য;  // Statement may implicitly read public state (without VarRef)
    তার্কিক m_implPubWr = অসত্য;  // Statement may implicitly write public state (without VarRef)
    তার্কিক m_explPubRef = অসত্য;  // Statement explicitly references public state via VarRef
    Ast_গ্রন্থি_Stmt* m_prevWithSameCondp = nullptr;  // Previous node in same list, with same condition
    তার্কিক writesConditionVar(তার্কিক condPubWritable) ধ্রুব {
        // This relies on MarkVarsVisitor having been called on the condition node
        প্রতি (ধ্রুব AstVar* ধ্রুব varp : m_wrVars) {
            যদি (varp->user1()) প্রত্যাগম সত্য;
        }
        // If the condition contains a public variable, check if it might be written
        যদি (condPubWritable && m_implPubWr) প্রত্যাগম সত্য;
        // Otherwise not written
        প্রত্যাগম অসত্য;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<Ast_গ্রন্থি_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
বর্গ CodeMotionAnalysisVisitor final : সার্বজনিক VNVisitorConst {
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
    std::vector<V3DupFinder> m_স্তূপ;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    স্থির Ast_গ্রন্থি_রাশিমালা* extractCondition(ধ্রুব Ast_গ্রন্থি_Stmt* nodep) {
        Ast_গ্রন্থি_রাশিমালা* conditionp = nullptr;
        যদি (ধ্রুব Ast_গ্রন্থি_Assign* ধ্রুব assignp = VN_CAST(nodep, গ্রন্থি_Assign)) {
            যদি (AstCond* ধ্রুব conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } অন্যথা যদি (ধ্রুব Ast_গ্রন্থি_If* ধ্রুব ifp = VN_CAST(nodep, গ্রন্থি_If)) {
            conditionp = ifp->condp();
        }
        যাবত্ (AstCCast* ধ্রুব castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        প্রত্যাগম conditionp;
    }

    শূন্য analyzeStmt(Ast_গ্রন্থি_Stmt* nodep, তার্কিক tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* ধ্রুব outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            যদি (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_স্তূপ.back();
                // Find a duplicate condition
                ধ্রুব V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                যদি (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.ঢোকাও(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } অন্যথা {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব firstp = VN_AS(dit->second, গ্রন্থি_রাশিমালা);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_গ্রন্থি_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_গ্রন্থি(nodep);

        // If there is an enclosing statement, propagate properties upwards
        যদি (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.ঢোকাও(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.ঢোকাও(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    শূন্য analyzeVarRef(AstVarRef* nodep) {
        ধ্রুব VAccess access = nodep->access();
        AstVar* ধ্রুব varp = nodep->varp();
        // Gather read and written variables
        যদি (access.isReadOrRW()) m_propsp->m_rdVars.ঢোকাও(varp);
        যদি (access.isWriteOrRW()) m_propsp->m_wrVars.ঢোকাও(varp);
        যদি (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = সত্য;
        }
    }

    শূন্য checkProperties(Ast_গ্রন্থি* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        যদি (VN_IS(nodep, Stmt_রাশিমালা)) প্রত্যাগম;

        // Call to a DPI import
        যদি (ধ্রুব Ast_গ্রন্থি_CCall* ধ্রুব ccallp = VN_CAST(nodep, গ্রন্থি_CCall)) {
            AstCFunc* ধ্রুব funcp = ccallp->funcp();
            যদি (funcp->dpiImportWrapper()) {
                যদি (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = সত্য;
                    m_propsp->m_implPubRd = সত্য;
                    m_propsp->m_implPubWr = সত্য;
                }
                প্রত্যাগম;
            }
        }

        // Side effect, but assume does not access public state
        যদি (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = সত্য;
            প্রত্যাগম;
        }

        // If impure, or branch, mark statement as fence
        যদি (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = সত্য;
    }

    শূন্য analyze_গ্রন্থি(Ast_গ্রন্থি* nodep) {
        // Check properties of this node if under a statement
        যদি (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        তার্কিক singletonListStart = অসত্য;
        যদি (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            যদি (!singletonListStart) m_স্তূপ.emplace_back(m_hasher);
        }

        // Analyze node
        যদি (Ast_গ্রন্থি_Stmt* ধ্রুব stmtp = VN_CAST(nodep, গ্রন্থি_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } অন্যথা যদি (AstVarRef* ধ্রুব vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } অন্যথা {
            analyze_গ্রন্থি(nodep);
        }

        // Pop the stack at the end of a list
        যদি (!singletonListStart && !nodep->nextp()) m_স্তূপ.বের_করো_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(Ast_গ্রন্থি* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterateAndNextConstNull(nodep);
    }

সার্বজনিক:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    স্থির শূন্য analyze(Ast_গ্রন্থি* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

বর্গ CodeMotionOptimizeVisitor final : সার্বজনিক VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    স্থির constexpr unsigned MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    ধ্রুব StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    তার্কিক areSwappable(ধ্রুব Ast_গ্রন্থি_Stmt* ap, ধ্রুব Ast_গ্রন্থি_Stmt* bp) ধ্রুব {
        ধ্রুব StmtProperties& aProps = m_stmtProperties(ap);
        ধ্রুব StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        যদি (aProps.m_isFence) প্রত্যাগম অসত্য;
        যদি (bProps.m_isFence) প্রত্যাগম অসত্য;
        // Don't swap side effecting statements, but can move others across them
        যদি (aProps.m_sideEffect && bProps.m_sideEffect) প্রত্যাগম অসত্য;
        // Don't swap if there is a hazard around public state - must observe ordering
        ধ্রুব তার্কিক bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        যদি (aProps.m_implPubWr && bPubRef) প্রত্যাগম অসত্য;
        ধ্রুব তার্কিক aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        যদি (aPubRef && bProps.m_implPubWr) প্রত্যাগম অসত্য;
        // If either statement writes a variable that the other reads, they are not swappable
        যদি (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) প্রত্যাগম অসত্য;
        যদি (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) প্রত্যাগম অসত্য;
        // If they both write to the same variable, they are not swappable
        যদি (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) প্রত্যাগম অসত্য;
        // Otherwise good to go
        প্রত্যাগম সত্য;
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Stmt* nodep) override {
        // Process only on first encounter
        যদি (nodep->user4SetOnce()) প্রত্যাগম;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_গ্রন্থি_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        যদি (!prevp) প্রত্যাগম;
#ifdef VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            ধ্রুব Ast_গ্রন্থি* currp = prevp;
            যাবত্ (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#endif
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        যদি (Ast_গ্রন্থি_Stmt* predp = VN_CAST(nodep->backp(), গ্রন্থি_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            প্রতি (unsigned i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                যদি (predp == prevp) ভঙ্গ;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_গ্রন্থি_Stmt* ধ্রুব backp = VN_CAST(predp->backp(), গ্রন্থি_Stmt);
                যদি (!backp) ভঙ্গ;
                // Don't swap statements if doing so would change program semantics
                যদি (!areSwappable(predp, nodep)) ভঙ্গ;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            যদি (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->addNextHere(nodep);
                // If the predecessor is the previous node with the same condition, job done
                যদি (predp == prevp) প্রত্যাগম;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        প্রতি (Ast_গ্রন্থি_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            যদি (Ast_গ্রন্থি_Stmt* succp = VN_CAST(prevp->nextp(), গ্রন্থি_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                প্রতি (unsigned i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    যদি (succp == currp) ভঙ্গ;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_গ্রন্থি_Stmt* ধ্রুব nextp = VN_CAST(succp->nextp(), গ্রন্থি_Stmt);
                    যদি (!nextp) ভঙ্গ;
                    // Don't swap statements if doing so would change program semantics
                    যদি (!areSwappable(prevp, succp)) ভঙ্গ;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                যদি (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->addHereThisAsNext(prevp);
                }
            }
        }
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(Ast_গ্রন্থি* nodep, ধ্রুব StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterateAndNextNull(nodep);
    }

সার্বজনিক:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    স্থির Ast_গ্রন্থি* optimize(Ast_গ্রন্থি* nodep, ধ্রুব StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        যাবত্ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        প্রত্যাগম nodep;
    }
};

//######################################################################
// Conditional merging

বর্গ MergeCondVisitor final : সার্বজনিক VNVisitor {
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
    VDouble0 m_stat_Merged_বস্তুসমূহ;  // Statistic tracking
    VDouble0 m_stat_Candidate_বস্তুসমূহ;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_গ্রন্থি* m_mgFirstp = nullptr;  // First node in merged sequence
    Ast_গ্রন্থি_রাশিমালা* m_mgCondp = nullptr;  // The condition of the first node
    ধ্রুব Ast_গ্রন্থি* m_mgLastp = nullptr;  // Last node in merged sequence
    ধ্রুব Ast_গ্রন্থি* m_mgNextp = nullptr;  // Next node in list being examined
    তার্কিক m_mgCondPubWritable = অসত্য;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_গ্রন্থি*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    শূন্য process(Ast_গ্রন্থি* nodep) {
        // Set up work queue
        std::queue<Ast_গ্রন্থি*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->ঠেলো(nodep);

        কুরু {
            // Set up user* for this iteration
            ধ্রুব VNUser1InUse user1InUse;
            ধ্রুব VNUser2InUse user2InUse;
            ধ্রুব VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_গ্রন্থি* currp = m_workQueuep->front();
            m_workQueuep->বের_করো();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            যদি (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterateAndNextNull(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            যদি (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } যাবত্ (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    স্থির Ast_গ্রন্থি* skipConstSels(Ast_গ্রন্থি* nodep) {
        যাবত্ (ধ্রুব AstArraySel* ধ্রুব aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            যদি (!VN_IS(aselp->bitp(), Const)) প্রত্যাগম nodep;
            nodep = aselp->fromp();
        }
        যাবত্ (ধ্রুব AstWordSel* ধ্রুব wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            যদি (!VN_IS(wselp->bitp(), Const)) প্রত্যাগম nodep;
            nodep = wselp->fromp();
        }
        প্রত্যাগম nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    স্থির তার্কিক is_Cheap_গ্রন্থি(Ast_গ্রন্থি* nodep) {
        // Comments are cheap
        যদি (VN_IS(nodep, Comment)) প্রত্যাগম সত্য;
        // So are some assignments
        যদি (ধ্রুব Ast_গ্রন্থি_Assign* ধ্রুব assignp = VN_CAST(nodep, গ্রন্থি_Assign)) {
            // Check LHS
            Ast_গ্রন্থি* ধ্রুব lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            যদি (!VN_IS(lhsp, VarRef)) প্রত্যাগম অসত্য;

            // Check RHS
            Ast_গ্রন্থি* ধ্রুব rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            যদি (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) প্রত্যাগম অসত্য;

            // Otherwise it is a cheap assignment
            প্রত্যাগম সত্য;
        }
        // Others are not
        প্রত্যাগম অসত্য;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    স্থির তার্কিক yieldsOneOrZero(ধ্রুব Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep, Const)) {
            প্রত্যাগম constp->অঙ্ক().toUQuad() <= 1;
        }
        যদি (ধ্রুব AstVarRef* ধ্রুব vrefp = VN_CAST(nodep, VarRef)) {
            ধ্রুব AstVar* ধ্রুব varp = vrefp->varp();
            প্রত্যাগম varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        যদি (ধ্রুব AstShiftR* ধ্রুব shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(shiftp->rhsp(), Const)) {
                ধ্রুব AstVarRef* ধ্রুব vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                ধ্রুব পূর্ণাঙ্ক width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                যদি (constp->toSInt() >= width - 1) প্রত্যাগম সত্য;
            }
            প্রত্যাগম অসত্য;
        }
        যদি (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            প্রত্যাগম সত্য;
        }
        যদি (ধ্রুব Ast_গ্রন্থি_Biop* ধ্রুব biopp = VN_CAST(nodep, গ্রন্থি_Biop)) {
            যদি (VN_IS(nodep, And))
                প্রত্যাগম yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            যদি (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                প্রত্যাগম yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            প্রত্যাগম অসত্য;
        }
        যদি (ধ্রুব AstCond* ধ্রুব condp = VN_CAST(nodep, Cond)) {
            প্রত্যাগম yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        যদি (ধ্রুব AstCCast* ধ্রুব castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            প্রত্যাগম yieldsOneOrZero(castp->lhsp());
        }
        প্রত্যাগম অসত্য;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    স্থির Ast_গ্রন্থি_রাশিমালা* maskLsb(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (yieldsOneOrZero(nodep)) প্রত্যাগম nodep;
        // Otherwise apply masking
        AstConst* ধ্রুব maskp = নূতন AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        প্রত্যাগম নূতন AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_গ্রন্থি_রাশিমালা* foldAndUnlink(Ast_গ্রন্থি_রাশিমালা* rhsp, তার্কিক condTrue) {
        যদি (rhsp->same_বৃক্ষ(m_mgCondp)) {
            প্রত্যাগম নূতন AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } অন্যথা যদি (ধ্রুব AstCond* ধ্রুব condp = extractCondFromRhs(rhsp)) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            যদি (condp == rhsp) প্রত্যাগম resp;
            যদি (ধ্রুব AstAnd* ধ্রুব andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                প্রত্যাগম নূতন AstAnd{andp->fileline(), andp->lhsp()->clone_বৃক্ষ_Pure(অসত্য), resp};
            }
        } অন্যথা যদি (ধ্রুব AstAnd* ধ্রুব andp = VN_CAST(rhsp, And)) {
            যদি (andp->lhsp()->same_বৃক্ষ(m_mgCondp)) {
                প্রত্যাগম condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : নূতন AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } অন্যথা {
                UASSERT_OBJ(andp->rhsp()->same_বৃক্ষ(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                প্রত্যাগম condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : নূতন AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } অন্যথা যদি (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            প্রত্যাগম rhsp->clone_বৃক্ষ(অসত্য);
        }
        // LCOV_EXCL_START
        যদি (debug()) rhsp->dump_বৃক্ষ("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        প্রত্যাগম nullptr;
        // LCOV_EXCL_STOP
    }

    শূন্য mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        যাবত্ (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            ধ্রুব Ast_গ্রন্থি* ধ্রুব backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        যাবত্ (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            ধ্রুব Ast_গ্রন্থি* ধ্রুব nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_গ্রন্থি_If* recursivep = nullptr;
        // Merge if list is longer than one node
        যদি (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            যদি (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_বৃক্ষ_Pure(অসত্য);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* ধ্রুব resultp = নূতন AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->addHereThisAsNext(resultp);
            // Try to preserve branch prediction if we can
            size_t nLikely = 0;
            size_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_গ্রন্থি* nextp = m_mgFirstp;
            কুরু {
                // Grab next pointer and unlink
                Ast_গ্রন্থি* ধ্রুব currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                যদি (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_বৃক্ষ(), currp);
                    অনুবর্ত;
                }
                //গুনতি
                ++m_stat_Merged_বস্তুসমূহ;
                যদি (Ast_গ্রন্থি_Assign* ধ্রুব assignp = VN_CAST(currp, গ্রন্থি_Assign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = assignp->rhsp()->unlinkFrBack();
                    Ast_গ্রন্থি_Assign* ধ্রুব thenp = assignp;
                    Ast_গ্রন্থি_Assign* ধ্রুব elsep = assignp->clone_বৃক্ষ(অসত্য);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, সত্য));
                    elsep->rhsp(foldAndUnlink(rhsp, অসত্য));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_বৃক্ষ(), rhsp);
                } অন্যথা {
                    Ast_গ্রন্থি_If* ধ্রুব ifp = VN_AS(currp, গ্রন্থি_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    যদি (Ast_গ্রন্থি* ধ্রুব listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlinkFrBackWithNext());
                    }
                    যদি (Ast_গ্রন্থি* ধ্রুব listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlinkFrBackWithNext());
                    }
                    // Record branch prediction
                    যদি (ifp->branchPred().likely()) ++nLikely;
                    যদি (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_বৃক্ষ(), ifp);
                }
            } যাবত্ (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            যদি (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            যদি (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            যদি (resultp->thensp()) m_workQueuep->ঠেলো(resultp->thensp());
            যদি (resultp->elsesp()) m_workQueuep->ঠেলো(resultp->elsesp());
        } অন্যথা যদি (Ast_গ্রন্থি_If* ধ্রুব ifp = VN_CAST(m_mgFirstp, গ্রন্থি_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = nullptr;
        m_mgCondp = nullptr;
        m_mgLastp = nullptr;
        m_mgNextp = nullptr;
        m_mgCondPubWritable = অসত্য;
        Ast_গ্রন্থি::user1_Clear_বৃক্ষ();  // Clear marked variables
        Ast_গ্রন্থি::user2_Clear_বৃক্ষ();
        // Merge recursively within the branches of an un-merged AstNodeIF
        যদি (recursivep) {
            iterateAndNextNull(recursivep->thensp());
            iterateAndNextNull(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            যদি (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    তার্কিক is_Simplifiable_গ্রন্থি(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        যদি (ধ্রুব Ast_গ্রন্থি_Assign* ধ্রুব assignp = VN_CAST(nodep, গ্রন্থি_Assign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            যদি (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                যদি (ধ্রুব AstAnd* ধ্রুব andp = VN_CAST(assignp->rhsp(), And)) {
                    যদি (andp->lhsp()->same_বৃক্ষ(m_mgCondp) || andp->rhsp()->same_বৃক্ষ(m_mgCondp)) {
                        প্রত্যাগম সত্য;
                    }
                }
                // Is it 'lhs = cond'?
                যদি (assignp->rhsp()->same_বৃক্ষ(m_mgCondp)) প্রত্যাগম সত্য;
            }
        }
        প্রত্যাগম অসত্য;
    }

    তার্কিক addToList(Ast_গ্রন্থি_Stmt* nodep, Ast_গ্রন্থি_রাশিমালা* condp) {
        // Set up head of new list if node is first in list
        যদি (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](ধ্রুব AstVarRef* nodep) {
                AstVar* ধ্রুব varp = nodep->varp();
                varp->user1(1);
                যদি (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = সত্য;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            যদি ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_গ্রন্থি::user1_Clear_বৃক্ষ();
                m_mgCondPubWritable = অসত্য;
                // We did not add to the list
                প্রত্যাগম অসত্য;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            যাবত্ (সত্য) {
                Ast_গ্রন্থি_Stmt* ধ্রুব backp = VN_CAST(m_mgFirstp->backp(), গ্রন্থি_Stmt);
                যদি (!backp || backp->nextp() != m_mgFirstp) ভঙ্গ;  // Don't move up the tree
                ধ্রুব StmtProperties& props = (*m_stmtPropertiesp)(backp);
                যদি (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) ভঙ্গ;
                যদি (is_Simplifiable_গ্রন্থি(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } অন্যথা যদি (is_Cheap_গ্রন্থি(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } অন্যথা {
                    ভঙ্গ;
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
        যদি (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_stat_Candidate_বস্তুসমূহ;
        // We did add to the list
        প্রত্যাগম সত্য;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    তার্কিক addIfHelpfulElseEndMerge(Ast_গ্রন্থি_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        যদি (!checkOrMakeMergeable(nodep)) প্রত্যাগম অসত্য;
        যদি (!m_mgFirstp) প্রত্যাগম অসত্য;  // If 'checkOrMakeMergeable' closed the list
        যদি (m_mgNextp == nodep) {
            যদি (is_Simplifiable_গ্রন্থি(nodep)) {
                যদি (addToList(nodep, nullptr)) প্রত্যাগম সত্য;
            } অন্যথা যদি (is_Cheap_গ্রন্থি(nodep)) {
                nodep->user2(1);
                যদি (addToList(nodep, nullptr)) প্রত্যাগম সত্য;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        প্রত্যাগম অসত্য;
    }

    তার্কিক checkOrMakeMergeable(ধ্রুব Ast_গ্রন্থি_Stmt* nodep) {
        ধ্রুব StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        যদি (props.m_isFence) প্রত্যাগম অসত্য;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        যদি (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        প্রত্যাগম সত্য;  // Now surely mergeable
    }

    শূন্য mergeEndIfIncompatible(ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব Ast_গ্রন্থি* condp) {
        যদি (m_mgFirstp && (m_mgNextp != nodep || !condp->same_বৃক্ষ(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            যদি (!checkOrMakeMergeable(nodep)) প্রত্যাগম;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } অন্যথা যদি (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        // Check if mergeable
        যদি (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterateAndNextNull(nodep->thensp());
            iterateAndNextNull(nodep->elsesp());
            প্রত্যাগম;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    শূন্য visit(Ast_গ্রন্থি_Stmt* nodep) override {
        যদি (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) প্রত্যাগম;
        iterateChildren(nodep);
    }

    শূন্য visit(AstCFunc* nodep) override {
        // Merge function body
        যদি (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    শূন্য visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {}

সার্বজনিক:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_stat_Candidate_বস্তুসমূহ);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_stat_Merged_বস্তুসমূহ);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

শূন্য V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_বৃক্ষ("merge_cond", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
