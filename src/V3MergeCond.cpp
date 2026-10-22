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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3MergeCond.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3DupFinder.h"
#அந்தர்பாவ "V3Hasher.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <queue>
#அந்தர்பாவ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_முனை* rhsp) {
    எனில் (AstCond* மாறிலி condp = VN_CAST(rhsp, Cond)) {
        திரும்பு condp;
    } இல்லையெனில் எனில் (மாறிலி AstAnd* மாறிலி andp = VN_CAST(rhsp, And)) {
        எனில் (AstCond* மாறிலி condp = VN_CAST(andp->rhsp(), Cond)) {
            எனில் (VN_IS(andp->lhsp(), Const)) திரும்பு condp;
        }
    }
    திரும்பு nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
தருக்க areDisjoint(மாறிலி std::set<மாறிலி AstVar*>& a, மாறிலி std::set<மாறிலி AstVar*>& b) {
    எனில் (a.empty() || b.empty()) திரும்பு உண்மை;
    மாறிலி auto endA = a.end();
    மாறிலி auto endB = b.end();
    auto itA = a.begin();
    auto itB = b.begin();
    போது (உண்மை) {
        எனில் (*itA == *itB) திரும்பு பொய்;
        எனில் (std::less<மாறிலி AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            எனில் (itA == endA) திரும்பு உண்மை;
        } இல்லையெனில் {
            itB = std::lower_bound(++itB, endB, *itA);
            எனில் (itB == endB) திரும்பு உண்மை;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

கட்டமைப்பு StmtProperties final {
    Ast_முனை_கோவை* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<மாறிலி AstVar*> m_rdVars;  // Variables read by this statement
    std::set<மாறிலி AstVar*> m_wrVars;  // Variables written by this statement
    தருக்க m_isFence = பொய்;  // Nothing should move across this statement, nor should it be merged
    தருக்க m_sideEffect = பொய்;  // Statement may have side effect, without access to model state
    தருக்க m_implPubRd = பொய்;  // Statement may implicitly read public state (without VarRef)
    தருக்க m_implPubWr = பொய்;  // Statement may implicitly write public state (without VarRef)
    தருக்க m_explPubRef = பொய்;  // Statement explicitly references public state via VarRef
    Ast_முனை_Stmt* m_prevWithSameCondp = nullptr;  // Previous node in same list, with same condition
    தருக்க writesConditionVar(தருக்க condPubWritable) மாறிலி {
        // This relies on MarkVarsVisitor having been called on the condition node
        க்கு (மாறிலி AstVar* மாறிலி varp : m_wrVars) {
            எனில் (varp->user1()) திரும்பு உண்மை;
        }
        // If the condition contains a public variable, check if it might be written
        எனில் (condPubWritable && m_implPubWr) திரும்பு உண்மை;
        // Otherwise not written
        திரும்பு பொய்;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<Ast_முனை_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
வகுப்பு CodeMotionAnalysisVisitor final : பொது VNVisitorConst {
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
    std::vector<V3DupFinder> m_அடுக்கு;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    நிலையான Ast_முனை_கோவை* extractCondition(மாறிலி Ast_முனை_Stmt* nodep) {
        Ast_முனை_கோவை* conditionp = nullptr;
        எனில் (மாறிலி Ast_முனை_Assign* மாறிலி assignp = VN_CAST(nodep, முனை_Assign)) {
            எனில் (AstCond* மாறிலி conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_If* மாறிலி ifp = VN_CAST(nodep, முனை_If)) {
            conditionp = ifp->condp();
        }
        போது (AstCCast* மாறிலி castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        திரும்பு conditionp;
    }

    வெறுமை analyzeStmt(Ast_முனை_Stmt* nodep, தருக்க tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* மாறிலி outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        எனில் (Ast_முனை_கோவை* மாறிலி condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            எனில் (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_அடுக்கு.back();
                // Find a duplicate condition
                மாறிலி V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                எனில் (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.செருகு(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } இல்லையெனில் {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_முனை_கோவை* மாறிலி firstp = VN_AS(dit->second, முனை_கோவை);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_முனை_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_முனை(nodep);

        // If there is an enclosing statement, propagate properties upwards
        எனில் (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.செருகு(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.செருகு(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    வெறுமை analyzeVarRef(AstVarRef* nodep) {
        மாறிலி VAccess access = nodep->access();
        AstVar* மாறிலி varp = nodep->varp();
        // Gather read and written variables
        எனில் (access.isReadOrRW()) m_propsp->m_rdVars.செருகு(varp);
        எனில் (access.isWriteOrRW()) m_propsp->m_wrVars.செருகு(varp);
        எனில் (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = உண்மை;
        }
    }

    வெறுமை checkProperties(Ast_முனை* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        எனில் (VN_IS(nodep, Stmt_கோவை)) திரும்பு;

        // Call to a DPI import
        எனில் (மாறிலி Ast_முனை_CCall* மாறிலி ccallp = VN_CAST(nodep, முனை_CCall)) {
            AstCFunc* மாறிலி funcp = ccallp->funcp();
            எனில் (funcp->dpiImportWrapper()) {
                எனில் (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = உண்மை;
                    m_propsp->m_implPubRd = உண்மை;
                    m_propsp->m_implPubWr = உண்மை;
                }
                திரும்பு;
            }
        }

        // Side effect, but assume does not access public state
        எனில் (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = உண்மை;
            திரும்பு;
        }

        // If impure, or branch, mark statement as fence
        எனில் (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = உண்மை;
    }

    வெறுமை analyze_முனை(Ast_முனை* nodep) {
        // Check properties of this node if under a statement
        எனில் (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        தருக்க singletonListStart = பொய்;
        எனில் (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            எனில் (!singletonListStart) m_அடுக்கு.emplace_back(m_hasher);
        }

        // Analyze node
        எனில் (Ast_முனை_Stmt* மாறிலி stmtp = VN_CAST(nodep, முனை_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } இல்லையெனில் எனில் (AstVarRef* மாறிலி vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } இல்லையெனில் {
            analyze_முனை(nodep);
        }

        // Pop the stack at the end of a list
        எனில் (!singletonListStart && !nodep->nextp()) m_அடுக்கு.எடு_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(Ast_முனை* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterateAndNextConstNull(nodep);
    }

பொது:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    நிலையான வெறுமை analyze(Ast_முனை* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

வகுப்பு CodeMotionOptimizeVisitor final : பொது VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    நிலையான constexpr unsigned MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    மாறிலி StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    தருக்க areSwappable(மாறிலி Ast_முனை_Stmt* ap, மாறிலி Ast_முனை_Stmt* bp) மாறிலி {
        மாறிலி StmtProperties& aProps = m_stmtProperties(ap);
        மாறிலி StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        எனில் (aProps.m_isFence) திரும்பு பொய்;
        எனில் (bProps.m_isFence) திரும்பு பொய்;
        // Don't swap side effecting statements, but can move others across them
        எனில் (aProps.m_sideEffect && bProps.m_sideEffect) திரும்பு பொய்;
        // Don't swap if there is a hazard around public state - must observe ordering
        மாறிலி தருக்க bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        எனில் (aProps.m_implPubWr && bPubRef) திரும்பு பொய்;
        மாறிலி தருக்க aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        எனில் (aPubRef && bProps.m_implPubWr) திரும்பு பொய்;
        // If either statement writes a variable that the other reads, they are not swappable
        எனில் (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) திரும்பு பொய்;
        எனில் (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) திரும்பு பொய்;
        // If they both write to the same variable, they are not swappable
        எனில் (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) திரும்பு பொய்;
        // Otherwise good to go
        திரும்பு உண்மை;
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        // Process only on first encounter
        எனில் (nodep->user4SetOnce()) திரும்பு;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_முனை_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        எனில் (!prevp) திரும்பு;
#ifdef VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            மாறிலி Ast_முனை* currp = prevp;
            போது (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#endif
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        எனில் (Ast_முனை_Stmt* predp = VN_CAST(nodep->backp(), முனை_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            க்கு (unsigned i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                எனில் (predp == prevp) நிறுத்து;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_முனை_Stmt* மாறிலி backp = VN_CAST(predp->backp(), முனை_Stmt);
                எனில் (!backp) நிறுத்து;
                // Don't swap statements if doing so would change program semantics
                எனில் (!areSwappable(predp, nodep)) நிறுத்து;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            எனில் (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->addNextHere(nodep);
                // If the predecessor is the previous node with the same condition, job done
                எனில் (predp == prevp) திரும்பு;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        க்கு (Ast_முனை_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            எனில் (Ast_முனை_Stmt* succp = VN_CAST(prevp->nextp(), முனை_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                க்கு (unsigned i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    எனில் (succp == currp) நிறுத்து;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_முனை_Stmt* மாறிலி nextp = VN_CAST(succp->nextp(), முனை_Stmt);
                    எனில் (!nextp) நிறுத்து;
                    // Don't swap statements if doing so would change program semantics
                    எனில் (!areSwappable(prevp, succp)) நிறுத்து;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                எனில் (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->addHereThisAsNext(prevp);
                }
            }
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(Ast_முனை* nodep, மாறிலி StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterateAndNextNull(nodep);
    }

பொது:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    நிலையான Ast_முனை* optimize(Ast_முனை* nodep, மாறிலி StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        போது (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        திரும்பு nodep;
    }
};

//######################################################################
// Conditional merging

வகுப்பு MergeCondVisitor final : பொது VNVisitor {
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
    VDouble0 m_stat_Merged_பொருட்கள்;  // Statistic tracking
    VDouble0 m_stat_Candidate_பொருட்கள்;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_முனை* m_mgFirstp = nullptr;  // First node in merged sequence
    Ast_முனை_கோவை* m_mgCondp = nullptr;  // The condition of the first node
    மாறிலி Ast_முனை* m_mgLastp = nullptr;  // Last node in merged sequence
    மாறிலி Ast_முனை* m_mgNextp = nullptr;  // Next node in list being examined
    தருக்க m_mgCondPubWritable = பொய்;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_முனை*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    வெறுமை process(Ast_முனை* nodep) {
        // Set up work queue
        std::queue<Ast_முனை*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->தள்ளு(nodep);

        செய் {
            // Set up user* for this iteration
            மாறிலி VNUser1InUse user1InUse;
            மாறிலி VNUser2InUse user2InUse;
            மாறிலி VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_முனை* currp = m_workQueuep->front();
            m_workQueuep->எடு();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            எனில் (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterateAndNextNull(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            எனில் (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } போது (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    நிலையான Ast_முனை* skipConstSels(Ast_முனை* nodep) {
        போது (மாறிலி AstArraySel* மாறிலி aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            எனில் (!VN_IS(aselp->bitp(), Const)) திரும்பு nodep;
            nodep = aselp->fromp();
        }
        போது (மாறிலி AstWordSel* மாறிலி wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            எனில் (!VN_IS(wselp->bitp(), Const)) திரும்பு nodep;
            nodep = wselp->fromp();
        }
        திரும்பு nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    நிலையான தருக்க is_Cheap_முனை(Ast_முனை* nodep) {
        // Comments are cheap
        எனில் (VN_IS(nodep, Comment)) திரும்பு உண்மை;
        // So are some assignments
        எனில் (மாறிலி Ast_முனை_Assign* மாறிலி assignp = VN_CAST(nodep, முனை_Assign)) {
            // Check LHS
            Ast_முனை* மாறிலி lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            எனில் (!VN_IS(lhsp, VarRef)) திரும்பு பொய்;

            // Check RHS
            Ast_முனை* மாறிலி rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            எனில் (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) திரும்பு பொய்;

            // Otherwise it is a cheap assignment
            திரும்பு உண்மை;
        }
        // Others are not
        திரும்பு பொய்;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    நிலையான தருக்க yieldsOneOrZero(மாறிலி Ast_முனை* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const)) {
            திரும்பு constp->இலக்கம்().toUQuad() <= 1;
        }
        எனில் (மாறிலி AstVarRef* மாறிலி vrefp = VN_CAST(nodep, VarRef)) {
            மாறிலி AstVar* மாறிலி varp = vrefp->varp();
            திரும்பு varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        எனில் (மாறிலி AstShiftR* மாறிலி shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(shiftp->rhsp(), Const)) {
                மாறிலி AstVarRef* மாறிலி vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                மாறிலி முழு width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                எனில் (constp->toSInt() >= width - 1) திரும்பு உண்மை;
            }
            திரும்பு பொய்;
        }
        எனில் (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            திரும்பு உண்மை;
        }
        எனில் (மாறிலி Ast_முனை_Biop* மாறிலி biopp = VN_CAST(nodep, முனை_Biop)) {
            எனில் (VN_IS(nodep, And))
                திரும்பு yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            எனில் (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                திரும்பு yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            திரும்பு பொய்;
        }
        எனில் (மாறிலி AstCond* மாறிலி condp = VN_CAST(nodep, Cond)) {
            திரும்பு yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        எனில் (மாறிலி AstCCast* மாறிலி castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            திரும்பு yieldsOneOrZero(castp->lhsp());
        }
        திரும்பு பொய்;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    நிலையான Ast_முனை_கோவை* maskLsb(Ast_முனை_கோவை* nodep) {
        எனில் (yieldsOneOrZero(nodep)) திரும்பு nodep;
        // Otherwise apply masking
        AstConst* மாறிலி maskp = புதிய AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        திரும்பு புதிய AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_முனை_கோவை* foldAndUnlink(Ast_முனை_கோவை* rhsp, தருக்க condTrue) {
        எனில் (rhsp->same_மரம்(m_mgCondp)) {
            திரும்பு புதிய AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } இல்லையெனில் எனில் (மாறிலி AstCond* மாறிலி condp = extractCondFromRhs(rhsp)) {
            Ast_முனை_கோவை* மாறிலி resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            எனில் (condp == rhsp) திரும்பு resp;
            எனில் (மாறிலி AstAnd* மாறிலி andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                திரும்பு புதிய AstAnd{andp->fileline(), andp->lhsp()->clone_மரம்_Pure(பொய்), resp};
            }
        } இல்லையெனில் எனில் (மாறிலி AstAnd* மாறிலி andp = VN_CAST(rhsp, And)) {
            எனில் (andp->lhsp()->same_மரம்(m_mgCondp)) {
                திரும்பு condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : புதிய AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } இல்லையெனில் {
                UASSERT_OBJ(andp->rhsp()->same_மரம்(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                திரும்பு condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : புதிய AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } இல்லையெனில் எனில் (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            திரும்பு rhsp->clone_மரம்(பொய்);
        }
        // LCOV_EXCL_START
        எனில் (debug()) rhsp->dump_மரம்("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        திரும்பு nullptr;
        // LCOV_EXCL_STOP
    }

    வெறுமை mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        போது (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            மாறிலி Ast_முனை* மாறிலி backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        போது (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            மாறிலி Ast_முனை* மாறிலி nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_முனை_If* recursivep = nullptr;
        // Merge if list is longer than one node
        எனில் (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            எனில் (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_மரம்_Pure(பொய்);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* மாறிலி resultp = புதிய AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->addHereThisAsNext(resultp);
            // Try to preserve branch prediction if we can
            size_t nLikely = 0;
            size_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_முனை* nextp = m_mgFirstp;
            செய் {
                // Grab next pointer and unlink
                Ast_முனை* மாறிலி currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                எனில் (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_மரம்(), currp);
                    தொடர்;
                }
                //மொத்தம்
                ++m_stat_Merged_பொருட்கள்;
                எனில் (Ast_முனை_Assign* மாறிலி assignp = VN_CAST(currp, முனை_Assign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_முனை_கோவை* மாறிலி rhsp = assignp->rhsp()->unlinkFrBack();
                    Ast_முனை_Assign* மாறிலி thenp = assignp;
                    Ast_முனை_Assign* மாறிலி elsep = assignp->clone_மரம்(பொய்);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, உண்மை));
                    elsep->rhsp(foldAndUnlink(rhsp, பொய்));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_மரம்(), rhsp);
                } இல்லையெனில் {
                    Ast_முனை_If* மாறிலி ifp = VN_AS(currp, முனை_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    எனில் (Ast_முனை* மாறிலி listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlinkFrBackWithNext());
                    }
                    எனில் (Ast_முனை* மாறிலி listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlinkFrBackWithNext());
                    }
                    // Record branch prediction
                    எனில் (ifp->branchPred().likely()) ++nLikely;
                    எனில் (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_மரம்(), ifp);
                }
            } போது (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            எனில் (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            எனில் (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            எனில் (resultp->thensp()) m_workQueuep->தள்ளு(resultp->thensp());
            எனில் (resultp->elsesp()) m_workQueuep->தள்ளு(resultp->elsesp());
        } இல்லையெனில் எனில் (Ast_முனை_If* மாறிலி ifp = VN_CAST(m_mgFirstp, முனை_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = nullptr;
        m_mgCondp = nullptr;
        m_mgLastp = nullptr;
        m_mgNextp = nullptr;
        m_mgCondPubWritable = பொய்;
        Ast_முனை::user1_Clear_மரம்();  // Clear marked variables
        Ast_முனை::user2_Clear_மரம்();
        // Merge recursively within the branches of an un-merged AstNodeIF
        எனில் (recursivep) {
            iterateAndNextNull(recursivep->thensp());
            iterateAndNextNull(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            எனில் (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    தருக்க is_Simplifiable_முனை(Ast_முனை* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        எனில் (மாறிலி Ast_முனை_Assign* மாறிலி assignp = VN_CAST(nodep, முனை_Assign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            எனில் (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                எனில் (மாறிலி AstAnd* மாறிலி andp = VN_CAST(assignp->rhsp(), And)) {
                    எனில் (andp->lhsp()->same_மரம்(m_mgCondp) || andp->rhsp()->same_மரம்(m_mgCondp)) {
                        திரும்பு உண்மை;
                    }
                }
                // Is it 'lhs = cond'?
                எனில் (assignp->rhsp()->same_மரம்(m_mgCondp)) திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;
    }

    தருக்க addToList(Ast_முனை_Stmt* nodep, Ast_முனை_கோவை* condp) {
        // Set up head of new list if node is first in list
        எனில் (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](மாறிலி AstVarRef* nodep) {
                AstVar* மாறிலி varp = nodep->varp();
                varp->user1(1);
                எனில் (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = உண்மை;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            எனில் ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_முனை::user1_Clear_மரம்();
                m_mgCondPubWritable = பொய்;
                // We did not add to the list
                திரும்பு பொய்;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            போது (உண்மை) {
                Ast_முனை_Stmt* மாறிலி backp = VN_CAST(m_mgFirstp->backp(), முனை_Stmt);
                எனில் (!backp || backp->nextp() != m_mgFirstp) நிறுத்து;  // Don't move up the tree
                மாறிலி StmtProperties& props = (*m_stmtPropertiesp)(backp);
                எனில் (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) நிறுத்து;
                எனில் (is_Simplifiable_முனை(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } இல்லையெனில் எனில் (is_Cheap_முனை(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } இல்லையெனில் {
                    நிறுத்து;
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
        எனில் (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_stat_Candidate_பொருட்கள்;
        // We did add to the list
        திரும்பு உண்மை;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    தருக்க addIfHelpfulElseEndMerge(Ast_முனை_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        எனில் (!checkOrMakeMergeable(nodep)) திரும்பு பொய்;
        எனில் (!m_mgFirstp) திரும்பு பொய்;  // If 'checkOrMakeMergeable' closed the list
        எனில் (m_mgNextp == nodep) {
            எனில் (is_Simplifiable_முனை(nodep)) {
                எனில் (addToList(nodep, nullptr)) திரும்பு உண்மை;
            } இல்லையெனில் எனில் (is_Cheap_முனை(nodep)) {
                nodep->user2(1);
                எனில் (addToList(nodep, nullptr)) திரும்பு உண்மை;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        திரும்பு பொய்;
    }

    தருக்க checkOrMakeMergeable(மாறிலி Ast_முனை_Stmt* nodep) {
        மாறிலி StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        எனில் (props.m_isFence) திரும்பு பொய்;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        எனில் (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        திரும்பு உண்மை;  // Now surely mergeable
    }

    வெறுமை mergeEndIfIncompatible(மாறிலி Ast_முனை* nodep, மாறிலி Ast_முனை* condp) {
        எனில் (m_mgFirstp && (m_mgNextp != nodep || !condp->same_மரம்(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (Ast_முனை_கோவை* மாறிலி condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            எனில் (!checkOrMakeMergeable(nodep)) திரும்பு;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } இல்லையெனில் எனில் (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    வெறுமை visit(Ast_முனை_If* nodep) override {
        // Check if mergeable
        எனில் (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterateAndNextNull(nodep->thensp());
            iterateAndNextNull(nodep->elsesp());
            திரும்பு;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        எனில் (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) திரும்பு;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstCFunc* nodep) override {
        // Merge function body
        எனில் (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    வெறுமை visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    வெறுமை visit(Ast_முனை_Module* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    வெறுமை visit(Ast_முனை* nodep) override {}

பொது:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_stat_Candidate_பொருட்கள்);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_stat_Merged_பொருட்கள்);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

வெறுமை V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_மரம்("merge_cond", 0, dump_மரம்_Either_Level() >= 6);
}
