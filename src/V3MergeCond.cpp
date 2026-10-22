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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3MergeCond.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3DupFinder.h"
#અન્તર્ભાવ "V3Hasher.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <queue>
#અન્તર્ભાવ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_ગાંઠ* rhsp) {
    જો (AstCond* અચલ condp = VN_CAST(rhsp, Cond)) {
        પાછા condp;
    } નહીંતર જો (અચલ AstAnd* અચલ andp = VN_CAST(rhsp, And)) {
        જો (AstCond* અચલ condp = VN_CAST(andp->rhsp(), Cond)) {
            જો (VN_IS(andp->lhsp(), Const)) પાછા condp;
        }
    }
    પાછા nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
તાર્કિક areDisjoint(અચલ std::set<અચલ AstVar*>& a, અચલ std::set<અચલ AstVar*>& b) {
    જો (a.empty() || b.empty()) પાછા સાચું;
    અચલ auto endA = a.end();
    અચલ auto endB = b.end();
    auto itA = a.begin();
    auto itB = b.begin();
    જ્યારે (સાચું) {
        જો (*itA == *itB) પાછા ખોટું;
        જો (std::less<અચલ AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            જો (itA == endA) પાછા સાચું;
        } નહીંતર {
            itB = std::lower_bound(++itB, endB, *itA);
            જો (itB == endB) પાછા સાચું;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

સંરચના StmtProperties final {
    Ast_ગાંઠ_પદાવલિ* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<અચલ AstVar*> m_rdVars;  // Variables read by this statement
    std::set<અચલ AstVar*> m_wrVars;  // Variables written by this statement
    તાર્કિક m_isFence = ખોટું;  // Nothing should move across this statement, nor should it be merged
    તાર્કિક m_sideEffect = ખોટું;  // Statement may have side effect, without access to model state
    તાર્કિક m_implPubRd = ખોટું;  // Statement may implicitly read public state (without VarRef)
    તાર્કિક m_implPubWr = ખોટું;  // Statement may implicitly write public state (without VarRef)
    તાર્કિક m_explPubRef = ખોટું;  // Statement explicitly references public state via VarRef
    Ast_ગાંઠ_Stmt* m_prevWithSameCondp = nullptr;  // Previous node in same list, with same condition
    તાર્કિક writesConditionVar(તાર્કિક condPubWritable) અચલ {
        // This relies on MarkVarsVisitor having been called on the condition node
        માટે (અચલ AstVar* અચલ varp : m_wrVars) {
            જો (varp->user1()) પાછા સાચું;
        }
        // If the condition contains a public variable, check if it might be written
        જો (condPubWritable && m_implPubWr) પાછા સાચું;
        // Otherwise not written
        પાછા ખોટું;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<Ast_ગાંઠ_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
વર્ગ CodeMotionAnalysisVisitor final : જાહેર VNVisitorConst {
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
    std::vector<V3DupFinder> m_ઢગલો;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    સ્થિર Ast_ગાંઠ_પદાવલિ* extractCondition(અચલ Ast_ગાંઠ_Stmt* nodep) {
        Ast_ગાંઠ_પદાવલિ* conditionp = nullptr;
        જો (અચલ Ast_ગાંઠ_Assign* અચલ assignp = VN_CAST(nodep, ગાંઠ_Assign)) {
            જો (AstCond* અચલ conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } નહીંતર જો (અચલ Ast_ગાંઠ_If* અચલ ifp = VN_CAST(nodep, ગાંઠ_If)) {
            conditionp = ifp->condp();
        }
        જ્યારે (AstCCast* અચલ castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        પાછા conditionp;
    }

    શૂન્ય analyzeStmt(Ast_ગાંઠ_Stmt* nodep, તાર્કિક tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* અચલ outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        જો (Ast_ગાંઠ_પદાવલિ* અચલ condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            જો (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_ઢગલો.back();
                // Find a duplicate condition
                અચલ V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                જો (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.ઉમેરો(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } નહીંતર {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_ગાંઠ_પદાવલિ* અચલ firstp = VN_AS(dit->second, ગાંઠ_પદાવલિ);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_ગાંઠ_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_ગાંઠ(nodep);

        // If there is an enclosing statement, propagate properties upwards
        જો (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.ઉમેરો(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.ઉમેરો(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    શૂન્ય analyzeVarRef(AstVarRef* nodep) {
        અચલ VAccess access = nodep->access();
        AstVar* અચલ varp = nodep->varp();
        // Gather read and written variables
        જો (access.isReadOrRW()) m_propsp->m_rdVars.ઉમેરો(varp);
        જો (access.isWriteOrRW()) m_propsp->m_wrVars.ઉમેરો(varp);
        જો (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = સાચું;
        }
    }

    શૂન્ય checkProperties(Ast_ગાંઠ* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        જો (VN_IS(nodep, Stmt_પદાવલિ)) પાછા;

        // Call to a DPI import
        જો (અચલ Ast_ગાંઠ_CCall* અચલ ccallp = VN_CAST(nodep, ગાંઠ_CCall)) {
            AstCFunc* અચલ funcp = ccallp->funcp();
            જો (funcp->dpiImportWrapper()) {
                જો (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = સાચું;
                    m_propsp->m_implPubRd = સાચું;
                    m_propsp->m_implPubWr = સાચું;
                }
                પાછા;
            }
        }

        // Side effect, but assume does not access public state
        જો (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = સાચું;
            પાછા;
        }

        // If impure, or branch, mark statement as fence
        જો (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = સાચું;
    }

    શૂન્ય analyze_ગાંઠ(Ast_ગાંઠ* nodep) {
        // Check properties of this node if under a statement
        જો (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        તાર્કિક singletonListStart = ખોટું;
        જો (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            જો (!singletonListStart) m_ઢગલો.emplace_back(m_hasher);
        }

        // Analyze node
        જો (Ast_ગાંઠ_Stmt* અચલ stmtp = VN_CAST(nodep, ગાંઠ_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } નહીંતર જો (AstVarRef* અચલ vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } નહીંતર {
            analyze_ગાંઠ(nodep);
        }

        // Pop the stack at the end of a list
        જો (!singletonListStart && !nodep->nextp()) m_ઢગલો.કાઢો_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(Ast_ગાંઠ* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterateAndNextConstNull(nodep);
    }

જાહેર:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    સ્થિર શૂન્ય analyze(Ast_ગાંઠ* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

વર્ગ CodeMotionOptimizeVisitor final : જાહેર VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    સ્થિર constexpr unsigned MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    અચલ StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    તાર્કિક areSwappable(અચલ Ast_ગાંઠ_Stmt* ap, અચલ Ast_ગાંઠ_Stmt* bp) અચલ {
        અચલ StmtProperties& aProps = m_stmtProperties(ap);
        અચલ StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        જો (aProps.m_isFence) પાછા ખોટું;
        જો (bProps.m_isFence) પાછા ખોટું;
        // Don't swap side effecting statements, but can move others across them
        જો (aProps.m_sideEffect && bProps.m_sideEffect) પાછા ખોટું;
        // Don't swap if there is a hazard around public state - must observe ordering
        અચલ તાર્કિક bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        જો (aProps.m_implPubWr && bPubRef) પાછા ખોટું;
        અચલ તાર્કિક aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        જો (aPubRef && bProps.m_implPubWr) પાછા ખોટું;
        // If either statement writes a variable that the other reads, they are not swappable
        જો (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) પાછા ખોટું;
        જો (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) પાછા ખોટું;
        // If they both write to the same variable, they are not swappable
        જો (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) પાછા ખોટું;
        // Otherwise good to go
        પાછા સાચું;
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        // Process only on first encounter
        જો (nodep->user4SetOnce()) પાછા;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_ગાંઠ_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        જો (!prevp) પાછા;
#ifdef VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            અચલ Ast_ગાંઠ* currp = prevp;
            જ્યારે (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#endif
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        જો (Ast_ગાંઠ_Stmt* predp = VN_CAST(nodep->backp(), ગાંઠ_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            માટે (unsigned i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                જો (predp == prevp) તોડો;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_ગાંઠ_Stmt* અચલ backp = VN_CAST(predp->backp(), ગાંઠ_Stmt);
                જો (!backp) તોડો;
                // Don't swap statements if doing so would change program semantics
                જો (!areSwappable(predp, nodep)) તોડો;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            જો (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->addNextHere(nodep);
                // If the predecessor is the previous node with the same condition, job done
                જો (predp == prevp) પાછા;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        માટે (Ast_ગાંઠ_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            જો (Ast_ગાંઠ_Stmt* succp = VN_CAST(prevp->nextp(), ગાંઠ_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                માટે (unsigned i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    જો (succp == currp) તોડો;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_ગાંઠ_Stmt* અચલ nextp = VN_CAST(succp->nextp(), ગાંઠ_Stmt);
                    જો (!nextp) તોડો;
                    // Don't swap statements if doing so would change program semantics
                    જો (!areSwappable(prevp, succp)) તોડો;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                જો (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->addHereThisAsNext(prevp);
                }
            }
        }
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(Ast_ગાંઠ* nodep, અચલ StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterateAndNextNull(nodep);
    }

જાહેર:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    સ્થિર Ast_ગાંઠ* optimize(Ast_ગાંઠ* nodep, અચલ StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        જ્યારે (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        પાછા nodep;
    }
};

//######################################################################
// Conditional merging

વર્ગ MergeCondVisitor final : જાહેર VNVisitor {
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
    VDouble0 m_stat_Merged_વસ્તુઓ;  // Statistic tracking
    VDouble0 m_stat_Candidate_વસ્તુઓ;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_ગાંઠ* m_mgFirstp = nullptr;  // First node in merged sequence
    Ast_ગાંઠ_પદાવલિ* m_mgCondp = nullptr;  // The condition of the first node
    અચલ Ast_ગાંઠ* m_mgLastp = nullptr;  // Last node in merged sequence
    અચલ Ast_ગાંઠ* m_mgNextp = nullptr;  // Next node in list being examined
    તાર્કિક m_mgCondPubWritable = ખોટું;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_ગાંઠ*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    શૂન્ય process(Ast_ગાંઠ* nodep) {
        // Set up work queue
        std::queue<Ast_ગાંઠ*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->ધકેલો(nodep);

        કરો {
            // Set up user* for this iteration
            અચલ VNUser1InUse user1InUse;
            અચલ VNUser2InUse user2InUse;
            અચલ VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_ગાંઠ* currp = m_workQueuep->front();
            m_workQueuep->કાઢો();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            જો (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterateAndNextNull(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            જો (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } જ્યારે (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    સ્થિર Ast_ગાંઠ* skipConstSels(Ast_ગાંઠ* nodep) {
        જ્યારે (અચલ AstArraySel* અચલ aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            જો (!VN_IS(aselp->bitp(), Const)) પાછા nodep;
            nodep = aselp->fromp();
        }
        જ્યારે (અચલ AstWordSel* અચલ wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            જો (!VN_IS(wselp->bitp(), Const)) પાછા nodep;
            nodep = wselp->fromp();
        }
        પાછા nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    સ્થિર તાર્કિક is_Cheap_ગાંઠ(Ast_ગાંઠ* nodep) {
        // Comments are cheap
        જો (VN_IS(nodep, Comment)) પાછા સાચું;
        // So are some assignments
        જો (અચલ Ast_ગાંઠ_Assign* અચલ assignp = VN_CAST(nodep, ગાંઠ_Assign)) {
            // Check LHS
            Ast_ગાંઠ* અચલ lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            જો (!VN_IS(lhsp, VarRef)) પાછા ખોટું;

            // Check RHS
            Ast_ગાંઠ* અચલ rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            જો (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) પાછા ખોટું;

            // Otherwise it is a cheap assignment
            પાછા સાચું;
        }
        // Others are not
        પાછા ખોટું;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    સ્થિર તાર્કિક yieldsOneOrZero(અચલ Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        જો (અચલ AstConst* અચલ constp = VN_CAST(nodep, Const)) {
            પાછા constp->અંક().toUQuad() <= 1;
        }
        જો (અચલ AstVarRef* અચલ vrefp = VN_CAST(nodep, VarRef)) {
            અચલ AstVar* અચલ varp = vrefp->varp();
            પાછા varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        જો (અચલ AstShiftR* અચલ shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            જો (અચલ AstConst* અચલ constp = VN_CAST(shiftp->rhsp(), Const)) {
                અચલ AstVarRef* અચલ vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                અચલ પૂર્ણાંક width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                જો (constp->toSInt() >= width - 1) પાછા સાચું;
            }
            પાછા ખોટું;
        }
        જો (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            પાછા સાચું;
        }
        જો (અચલ Ast_ગાંઠ_Biop* અચલ biopp = VN_CAST(nodep, ગાંઠ_Biop)) {
            જો (VN_IS(nodep, And))
                પાછા yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            જો (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                પાછા yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            પાછા ખોટું;
        }
        જો (અચલ AstCond* અચલ condp = VN_CAST(nodep, Cond)) {
            પાછા yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        જો (અચલ AstCCast* અચલ castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            પાછા yieldsOneOrZero(castp->lhsp());
        }
        પાછા ખોટું;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    સ્થિર Ast_ગાંઠ_પદાવલિ* maskLsb(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (yieldsOneOrZero(nodep)) પાછા nodep;
        // Otherwise apply masking
        AstConst* અચલ maskp = નવું AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        પાછા નવું AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_ગાંઠ_પદાવલિ* foldAndUnlink(Ast_ગાંઠ_પદાવલિ* rhsp, તાર્કિક condTrue) {
        જો (rhsp->same_વૃક્ષ(m_mgCondp)) {
            પાછા નવું AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } નહીંતર જો (અચલ AstCond* અચલ condp = extractCondFromRhs(rhsp)) {
            Ast_ગાંઠ_પદાવલિ* અચલ resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            જો (condp == rhsp) પાછા resp;
            જો (અચલ AstAnd* અચલ andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                પાછા નવું AstAnd{andp->fileline(), andp->lhsp()->clone_વૃક્ષ_Pure(ખોટું), resp};
            }
        } નહીંતર જો (અચલ AstAnd* અચલ andp = VN_CAST(rhsp, And)) {
            જો (andp->lhsp()->same_વૃક્ષ(m_mgCondp)) {
                પાછા condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : નવું AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } નહીંતર {
                UASSERT_OBJ(andp->rhsp()->same_વૃક્ષ(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                પાછા condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : નવું AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } નહીંતર જો (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            પાછા rhsp->clone_વૃક્ષ(ખોટું);
        }
        // LCOV_EXCL_START
        જો (debug()) rhsp->dump_વૃક્ષ("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        પાછા nullptr;
        // LCOV_EXCL_STOP
    }

    શૂન્ય mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        જ્યારે (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            અચલ Ast_ગાંઠ* અચલ backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        જ્યારે (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            અચલ Ast_ગાંઠ* અચલ nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_ગાંઠ_If* recursivep = nullptr;
        // Merge if list is longer than one node
        જો (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            જો (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_વૃક્ષ_Pure(ખોટું);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* અચલ resultp = નવું AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->addHereThisAsNext(resultp);
            // Try to preserve branch prediction if we can
            size_t nLikely = 0;
            size_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_ગાંઠ* nextp = m_mgFirstp;
            કરો {
                // Grab next pointer and unlink
                Ast_ગાંઠ* અચલ currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                જો (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_વૃક્ષ(), currp);
                    ચાલુ;
                }
                //ગણતરી
                ++m_stat_Merged_વસ્તુઓ;
                જો (Ast_ગાંઠ_Assign* અચલ assignp = VN_CAST(currp, ગાંઠ_Assign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_ગાંઠ_પદાવલિ* અચલ rhsp = assignp->rhsp()->unlinkFrBack();
                    Ast_ગાંઠ_Assign* અચલ thenp = assignp;
                    Ast_ગાંઠ_Assign* અચલ elsep = assignp->clone_વૃક્ષ(ખોટું);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, સાચું));
                    elsep->rhsp(foldAndUnlink(rhsp, ખોટું));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_વૃક્ષ(), rhsp);
                } નહીંતર {
                    Ast_ગાંઠ_If* અચલ ifp = VN_AS(currp, ગાંઠ_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    જો (Ast_ગાંઠ* અચલ listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlinkFrBackWithNext());
                    }
                    જો (Ast_ગાંઠ* અચલ listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlinkFrBackWithNext());
                    }
                    // Record branch prediction
                    જો (ifp->branchPred().likely()) ++nLikely;
                    જો (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_વૃક્ષ(), ifp);
                }
            } જ્યારે (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            જો (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            જો (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            જો (resultp->thensp()) m_workQueuep->ધકેલો(resultp->thensp());
            જો (resultp->elsesp()) m_workQueuep->ધકેલો(resultp->elsesp());
        } નહીંતર જો (Ast_ગાંઠ_If* અચલ ifp = VN_CAST(m_mgFirstp, ગાંઠ_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = nullptr;
        m_mgCondp = nullptr;
        m_mgLastp = nullptr;
        m_mgNextp = nullptr;
        m_mgCondPubWritable = ખોટું;
        Ast_ગાંઠ::user1_Clear_વૃક્ષ();  // Clear marked variables
        Ast_ગાંઠ::user2_Clear_વૃક્ષ();
        // Merge recursively within the branches of an un-merged AstNodeIF
        જો (recursivep) {
            iterateAndNextNull(recursivep->thensp());
            iterateAndNextNull(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            જો (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    તાર્કિક is_Simplifiable_ગાંઠ(Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        જો (અચલ Ast_ગાંઠ_Assign* અચલ assignp = VN_CAST(nodep, ગાંઠ_Assign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            જો (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                જો (અચલ AstAnd* અચલ andp = VN_CAST(assignp->rhsp(), And)) {
                    જો (andp->lhsp()->same_વૃક્ષ(m_mgCondp) || andp->rhsp()->same_વૃક્ષ(m_mgCondp)) {
                        પાછા સાચું;
                    }
                }
                // Is it 'lhs = cond'?
                જો (assignp->rhsp()->same_વૃક્ષ(m_mgCondp)) પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }

    તાર્કિક addToList(Ast_ગાંઠ_Stmt* nodep, Ast_ગાંઠ_પદાવલિ* condp) {
        // Set up head of new list if node is first in list
        જો (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](અચલ AstVarRef* nodep) {
                AstVar* અચલ varp = nodep->varp();
                varp->user1(1);
                જો (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = સાચું;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            જો ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_ગાંઠ::user1_Clear_વૃક્ષ();
                m_mgCondPubWritable = ખોટું;
                // We did not add to the list
                પાછા ખોટું;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            જ્યારે (સાચું) {
                Ast_ગાંઠ_Stmt* અચલ backp = VN_CAST(m_mgFirstp->backp(), ગાંઠ_Stmt);
                જો (!backp || backp->nextp() != m_mgFirstp) તોડો;  // Don't move up the tree
                અચલ StmtProperties& props = (*m_stmtPropertiesp)(backp);
                જો (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) તોડો;
                જો (is_Simplifiable_ગાંઠ(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } નહીંતર જો (is_Cheap_ગાંઠ(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } નહીંતર {
                    તોડો;
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
        જો (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_stat_Candidate_વસ્તુઓ;
        // We did add to the list
        પાછા સાચું;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    તાર્કિક addIfHelpfulElseEndMerge(Ast_ગાંઠ_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        જો (!checkOrMakeMergeable(nodep)) પાછા ખોટું;
        જો (!m_mgFirstp) પાછા ખોટું;  // If 'checkOrMakeMergeable' closed the list
        જો (m_mgNextp == nodep) {
            જો (is_Simplifiable_ગાંઠ(nodep)) {
                જો (addToList(nodep, nullptr)) પાછા સાચું;
            } નહીંતર જો (is_Cheap_ગાંઠ(nodep)) {
                nodep->user2(1);
                જો (addToList(nodep, nullptr)) પાછા સાચું;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        પાછા ખોટું;
    }

    તાર્કિક checkOrMakeMergeable(અચલ Ast_ગાંઠ_Stmt* nodep) {
        અચલ StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        જો (props.m_isFence) પાછા ખોટું;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        જો (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        પાછા સાચું;  // Now surely mergeable
    }

    શૂન્ય mergeEndIfIncompatible(અચલ Ast_ગાંઠ* nodep, અચલ Ast_ગાંઠ* condp) {
        જો (m_mgFirstp && (m_mgNextp != nodep || !condp->same_વૃક્ષ(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (Ast_ગાંઠ_પદાવલિ* અચલ condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            જો (!checkOrMakeMergeable(nodep)) પાછા;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } નહીંતર જો (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        // Check if mergeable
        જો (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterateAndNextNull(nodep->thensp());
            iterateAndNextNull(nodep->elsesp());
            પાછા;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        જો (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) પાછા;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstCFunc* nodep) override {
        // Merge function body
        જો (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    શૂન્ય visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {}

જાહેર:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_stat_Candidate_વસ્તુઓ);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_stat_Merged_વસ્તુઓ);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

શૂન્ય V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_વૃક્ષ("merge_cond", 0, dump_વૃક્ષ_Either_Level() >= 6);
}
