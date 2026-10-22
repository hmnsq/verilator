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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3MergeCond.h"

#include "V3AstUserAllocator.h"
#include "V3DupFinder.h"
#include "V3Hasher.h"
#include "V3Stats.h"

#include <queue>
#include <set>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(AstNode* rhsp) {
    ᨀᨘ (AstCond* const condp = VN_CAST(rhsp, Cond)) {
        ᨀᨘᨒ condp;
    } ᨀᨘᨁ ᨀᨘ (const AstAnd* const andp = VN_CAST(rhsp, And)) {
        ᨀᨘ (AstCond* const condp = VN_CAST(andp->rhsp(), Cond)) {
            ᨀᨘ (VN_IS(andp->lhsp(), Const)) ᨀᨘᨒ condp;
        }
    }
    ᨀᨘᨒ nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
bool areDisjoint(const std::set<const AstVar*>& a, const std::set<const AstVar*>& b) {
    ᨀᨘ (a.empty() || b.empty()) ᨀᨘᨒ true;
    const auto endA = a.end();
    const auto endB = b.end();
    auto itA = a.begin();
    auto itB = b.begin();
    ᨀᨛ (true) {
        ᨀᨘ (*itA == *itB) ᨀᨘᨒ false;
        ᨀᨘ (std::less<const AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            ᨀᨘ (itA == endA) ᨀᨘᨒ true;
        } ᨀᨘᨁ {
            itB = std::lower_bound(++itB, endB, *itA);
            ᨀᨘ (itB == endB) ᨀᨘᨒ true;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

sa_nracanaa StmtProperties final {
    AstNodeExpr* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<const AstVar*> m_rdVars;  // Variables read by this statement
    std::set<const AstVar*> m_wrVars;  // Variables written by this statement
    bool m_isFence = false;  // Nothing should move across this statement, nor should it be merged
    bool m_sideEffect = false;  // Statement may have side effect, without access to model state
    bool m_implPubRd = false;  // Statement may implicitly read public state (without VarRef)
    bool m_implPubWr = false;  // Statement may implicitly write public state (without VarRef)
    bool m_explPubRef = false;  // Statement explicitly references public state via VarRef
    AstNodeStmt* m_prevWithSameCondp = nullptr;  // Previous node in same list, with same condition
    bool writesConditionVar(bool condPubWritable) const {
        // This relies on MarkVarsVisitor having been called on the condition node
        for (const AstVar* const varp : m_wrVars) {
            ᨀᨘ (varp->user1()) ᨀᨘᨒ true;
        }
        // If the condition contains a public variable, check if it might be written
        ᨀᨘ (condPubWritable && m_implPubWr) ᨀᨘᨒ true;
        // Otherwise not written
        ᨀᨘᨒ false;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<AstNodeStmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
class CodeMotionAnalysisVisitor final : public VNVisitorConst {
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
    std::vector<V3DupFinder> m_stack;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    static AstNodeExpr* extractCondition(const AstNodeStmt* nodep) {
        AstNodeExpr* conditionp = nullptr;
        ᨀᨘ (const AstNodeAssign* const assignp = VN_CAST(nodep, NodeAssign)) {
            ᨀᨘ (AstCond* const conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } ᨀᨘᨁ ᨀᨘ (const AstNodeIf* const ifp = VN_CAST(nodep, NodeIf)) {
            conditionp = ifp->condp();
        }
        ᨀᨛ (AstCCast* const castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        ᨀᨘᨒ conditionp;
    }

    void analyzeStmt(AstNodeStmt* nodep, bool tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* const outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        ᨀᨘ (AstNodeExpr* const condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            ᨀᨘ (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_stack.back();
                // Find a duplicate condition
                const V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                ᨀᨘ (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.insert(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } ᨀᨘᨁ {
                    // Seen a conditional with the same condition earlier in the current list
                    AstNodeExpr* const firstp = VN_AS(dit->second, NodeExpr);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<AstNodeStmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyzeNode(nodep);

        // If there is an enclosing statement, propagate properties upwards
        ᨀᨘ (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.insert(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.insert(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    void analyzeVarRef(AstVarRef* nodep) {
        const VAccess access = nodep->access();
        AstVar* const varp = nodep->varp();
        // Gather read and written variables
        ᨀᨘ (access.isReadOrRW()) m_propsp->m_rdVars.insert(varp);
        ᨀᨘ (access.isWriteOrRW()) m_propsp->m_wrVars.insert(varp);
        ᨀᨘ (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = true;
        }
    }

    void checkProperties(AstNode* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        ᨀᨘ (VN_IS(nodep, StmtExpr)) ᨀᨘᨒ;

        // Call to a DPI import
        ᨀᨘ (const AstNodeCCall* const ccallp = VN_CAST(nodep, NodeCCall)) {
            AstCFunc* const funcp = ccallp->funcp();
            ᨀᨘ (funcp->dpiImportWrapper()) {
                ᨀᨘ (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = true;
                    m_propsp->m_implPubRd = true;
                    m_propsp->m_implPubWr = true;
                }
                ᨀᨘᨒ;
            }
        }

        // Side effect, but assume does not access public state
        ᨀᨘ (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = true;
            ᨀᨘᨒ;
        }

        // If impure, or branch, mark statement as fence
        ᨀᨘ (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = true;
    }

    void analyzeNode(AstNode* nodep) {
        // Check properties of this node if under a statement
        ᨀᨘ (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    void visit(AstNode* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        bool singletonListStart = false;
        ᨀᨘ (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            ᨀᨘ (!singletonListStart) m_stack.emplace_back(m_hasher);
        }

        // Analyze node
        ᨀᨘ (AstNodeStmt* const stmtp = VN_CAST(nodep, NodeStmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } ᨀᨘᨁ ᨀᨘ (AstVarRef* const vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } ᨀᨘᨁ {
            analyzeNode(nodep);
        }

        // Pop the stack at the end of a list
        ᨀᨘ (!singletonListStart && !nodep->nextp()) m_stack.pop_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(AstNode* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterateAndNextConstNull(nodep);
    }

public:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    static void analyze(AstNode* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

class CodeMotionOptimizeVisitor final : public VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    static constexpr unsigned MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    const StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    bool areSwappable(const AstNodeStmt* ap, const AstNodeStmt* bp) const {
        const StmtProperties& aProps = m_stmtProperties(ap);
        const StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        ᨀᨘ (aProps.m_isFence) ᨀᨘᨒ false;
        ᨀᨘ (bProps.m_isFence) ᨀᨘᨒ false;
        // Don't swap side effecting statements, but can move others across them
        ᨀᨘ (aProps.m_sideEffect && bProps.m_sideEffect) ᨀᨘᨒ false;
        // Don't swap if there is a hazard around public state - must observe ordering
        const bool bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        ᨀᨘ (aProps.m_implPubWr && bPubRef) ᨀᨘᨒ false;
        const bool aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        ᨀᨘ (aPubRef && bProps.m_implPubWr) ᨀᨘᨒ false;
        // If either statement writes a variable that the other reads, they are not swappable
        ᨀᨘ (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) ᨀᨘᨒ false;
        ᨀᨘ (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) ᨀᨘᨒ false;
        // If they both write to the same variable, they are not swappable
        ᨀᨘ (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) ᨀᨘᨒ false;
        // Otherwise good to go
        ᨀᨘᨒ true;
    }

    // VISITORS
    void visit(AstNodeStmt* nodep) override {
        // Process only on first encounter
        ᨀᨘ (nodep->user4SetOnce()) ᨀᨘᨒ;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        AstNodeStmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        ᨀᨘ (!prevp) ᨀᨘᨒ;
#ifdef VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            const AstNode* currp = prevp;
            ᨀᨛ (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#endif
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        ᨀᨘ (AstNodeStmt* predp = VN_CAST(nodep->backp(), NodeStmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            for (unsigned i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                ᨀᨘ (predp == prevp) bha_nga;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                AstNodeStmt* const backp = VN_CAST(predp->backp(), NodeStmt);
                ᨀᨘ (!backp) bha_nga;
                // Don't swap statements if doing so would change program semantics
                ᨀᨘ (!areSwappable(predp, nodep)) bha_nga;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            ᨀᨘ (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->addNextHere(nodep);
                // If the predecessor is the previous node with the same condition, job done
                ᨀᨘ (predp == prevp) ᨀᨘᨒ;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        for (AstNodeStmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            ᨀᨘ (AstNodeStmt* succp = VN_CAST(prevp->nextp(), NodeStmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                for (unsigned i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    ᨀᨘ (succp == currp) bha_nga;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    AstNodeStmt* const nextp = VN_CAST(succp->nextp(), NodeStmt);
                    ᨀᨘ (!nextp) bha_nga;
                    // Don't swap statements if doing so would change program semantics
                    ᨀᨘ (!areSwappable(prevp, succp)) bha_nga;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                ᨀᨘ (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->addHereThisAsNext(prevp);
                }
            }
        }
    }

    void visit(AstNode* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(AstNode* nodep, const StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterateAndNextNull(nodep);
    }

public:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    static AstNode* optimize(AstNode* nodep, const StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        ᨀᨛ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        ᨀᨘᨒ nodep;
    }
};

//######################################################################
// Conditional merging

class MergeCondVisitor final : public VNVisitor {
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
    VDouble0 m_statMergedItems;  // Statistic tracking
    VDouble0 m_statCandidateItems;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    AstNode* m_mgFirstp = nullptr;  // First node in merged sequence
    AstNodeExpr* m_mgCondp = nullptr;  // The condition of the first node
    const AstNode* m_mgLastp = nullptr;  // Last node in merged sequence
    const AstNode* m_mgNextp = nullptr;  // Next node in list being examined
    bool m_mgCondPubWritable = false;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<AstNode*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    void process(AstNode* nodep) {
        // Set up work queue
        std::queue<AstNode*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->push(nodep);

        do {
            // Set up user* for this iteration
            const VNUser1InUse user1InUse;
            const VNUser2InUse user2InUse;
            const VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            AstNode* currp = m_workQueuep->front();
            m_workQueuep->pop();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            ᨀᨘ (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterateAndNextNull(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            ᨀᨘ (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } ᨀᨛ (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    static AstNode* skipConstSels(AstNode* nodep) {
        ᨀᨛ (const AstArraySel* const aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            ᨀᨘ (!VN_IS(aselp->bitp(), Const)) ᨀᨘᨒ nodep;
            nodep = aselp->fromp();
        }
        ᨀᨛ (const AstWordSel* const wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            ᨀᨘ (!VN_IS(wselp->bitp(), Const)) ᨀᨘᨒ nodep;
            nodep = wselp->fromp();
        }
        ᨀᨘᨒ nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    static bool isCheapNode(AstNode* nodep) {
        // Comments are cheap
        ᨀᨘ (VN_IS(nodep, Comment)) ᨀᨘᨒ true;
        // So are some assignments
        ᨀᨘ (const AstNodeAssign* const assignp = VN_CAST(nodep, NodeAssign)) {
            // Check LHS
            AstNode* const lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            ᨀᨘ (!VN_IS(lhsp, VarRef)) ᨀᨘᨒ false;

            // Check RHS
            AstNode* const rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            ᨀᨘ (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) ᨀᨘᨒ false;

            // Otherwise it is a cheap assignment
            ᨀᨘᨒ true;
        }
        // Others are not
        ᨀᨘᨒ false;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    static bool yieldsOneOrZero(const AstNode* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        ᨀᨘ (const AstConst* const constp = VN_CAST(nodep, Const)) {
            ᨀᨘᨒ constp->num().toUQuad() <= 1;
        }
        ᨀᨘ (const AstVarRef* const vrefp = VN_CAST(nodep, VarRef)) {
            const AstVar* const varp = vrefp->varp();
            ᨀᨘᨒ varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        ᨀᨘ (const AstShiftR* const shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            ᨀᨘ (const AstConst* const constp = VN_CAST(shiftp->rhsp(), Const)) {
                const AstVarRef* const vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                const puur_naa_nka width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                ᨀᨘ (constp->toSInt() >= width - 1) ᨀᨘᨒ true;
            }
            ᨀᨘᨒ false;
        }
        ᨀᨘ (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            ᨀᨘᨒ true;
        }
        ᨀᨘ (const AstNodeBiop* const biopp = VN_CAST(nodep, NodeBiop)) {
            ᨀᨘ (VN_IS(nodep, And))
                ᨀᨘᨒ yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            ᨀᨘ (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                ᨀᨘᨒ yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            ᨀᨘᨒ false;
        }
        ᨀᨘ (const AstCond* const condp = VN_CAST(nodep, Cond)) {
            ᨀᨘᨒ yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        ᨀᨘ (const AstCCast* const castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            ᨀᨘᨒ yieldsOneOrZero(castp->lhsp());
        }
        ᨀᨘᨒ false;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    static AstNodeExpr* maskLsb(AstNodeExpr* nodep) {
        ᨀᨘ (yieldsOneOrZero(nodep)) ᨀᨘᨒ nodep;
        // Otherwise apply masking
        AstConst* const maskp = new AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        ᨀᨘᨒ new AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    AstNodeExpr* foldAndUnlink(AstNodeExpr* rhsp, bool condTrue) {
        ᨀᨘ (rhsp->sameTree(m_mgCondp)) {
            ᨀᨘᨒ new AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } ᨀᨘᨁ ᨀᨘ (const AstCond* const condp = extractCondFromRhs(rhsp)) {
            AstNodeExpr* const resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            ᨀᨘ (condp == rhsp) ᨀᨘᨒ resp;
            ᨀᨘ (const AstAnd* const andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                ᨀᨘᨒ new AstAnd{andp->fileline(), andp->lhsp()->cloneTreePure(false), resp};
            }
        } ᨀᨘᨁ ᨀᨘ (const AstAnd* const andp = VN_CAST(rhsp, And)) {
            ᨀᨘ (andp->lhsp()->sameTree(m_mgCondp)) {
                ᨀᨘᨒ condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : new AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } ᨀᨘᨁ {
                UASSERT_OBJ(andp->rhsp()->sameTree(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                ᨀᨘᨒ condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : new AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } ᨀᨘᨁ ᨀᨘ (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            ᨀᨘᨒ rhsp->cloneTree(false);
        }
        // LCOV_EXCL_START
        ᨀᨘ (debug()) rhsp->dumpTree("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        ᨀᨘᨒ nullptr;
        // LCOV_EXCL_STOP
    }

    void mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        ᨀᨛ (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            const AstNode* const backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        ᨀᨛ (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            const AstNode* const nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        AstNodeIf* recursivep = nullptr;
        // Merge if list is longer than one node
        ᨀᨘ (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            ᨀᨘ (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->cloneTreePure(false);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* const resultp = new AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->addHereThisAsNext(resultp);
            // Try to preserve branch prediction if we can
            size_t nLikely = 0;
            size_t nUnlikely = 0;
            // Unzip the list and insert under branches
            AstNode* nextp = m_mgFirstp;
            do {
                // Grab next pointer and unlink
                AstNode* const currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                ᨀᨘ (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->deleteTree(), currp);
                    continue;
                }
                // Count
                ++m_statMergedItems;
                ᨀᨘ (AstNodeAssign* const assignp = VN_CAST(currp, NodeAssign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    AstNodeExpr* const rhsp = assignp->rhsp()->unlinkFrBack();
                    AstNodeAssign* const thenp = assignp;
                    AstNodeAssign* const elsep = assignp->cloneTree(false);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, true));
                    elsep->rhsp(foldAndUnlink(rhsp, false));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->deleteTree(), rhsp);
                } ᨀᨘᨁ {
                    AstNodeIf* const ifp = VN_AS(currp, NodeIf);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    ᨀᨘ (AstNode* const listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlinkFrBackWithNext());
                    }
                    ᨀᨘ (AstNode* const listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlinkFrBackWithNext());
                    }
                    // Record branch prediction
                    ᨀᨘ (ifp->branchPred().likely()) ++nLikely;
                    ᨀᨘ (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->deleteTree(), ifp);
                }
            } ᨀᨛ (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            ᨀᨘ (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            ᨀᨘ (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            ᨀᨘ (resultp->thensp()) m_workQueuep->push(resultp->thensp());
            ᨀᨘ (resultp->elsesp()) m_workQueuep->push(resultp->elsesp());
        } ᨀᨘᨁ ᨀᨘ (AstNodeIf* const ifp = VN_CAST(m_mgFirstp, NodeIf)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = nullptr;
        m_mgCondp = nullptr;
        m_mgLastp = nullptr;
        m_mgNextp = nullptr;
        m_mgCondPubWritable = false;
        AstNode::user1ClearTree();  // Clear marked variables
        AstNode::user2ClearTree();
        // Merge recursively within the branches of an un-merged AstNodeIF
        ᨀᨘ (recursivep) {
            iterateAndNextNull(recursivep->thensp());
            iterateAndNextNull(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            ᨀᨘ (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    bool isSimplifiableNode(AstNode* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        ᨀᨘ (const AstNodeAssign* const assignp = VN_CAST(nodep, NodeAssign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            ᨀᨘ (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                ᨀᨘ (const AstAnd* const andp = VN_CAST(assignp->rhsp(), And)) {
                    ᨀᨘ (andp->lhsp()->sameTree(m_mgCondp) || andp->rhsp()->sameTree(m_mgCondp)) {
                        ᨀᨘᨒ true;
                    }
                }
                // Is it 'lhs = cond'?
                ᨀᨘ (assignp->rhsp()->sameTree(m_mgCondp)) ᨀᨘᨒ true;
            }
        }
        ᨀᨘᨒ false;
    }

    bool addToList(AstNodeStmt* nodep, AstNodeExpr* condp) {
        // Set up head of new list if node is first in list
        ᨀᨘ (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](const AstVarRef* nodep) {
                AstVar* const varp = nodep->varp();
                varp->user1(1);
                ᨀᨘ (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = true;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            ᨀᨘ ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                AstNode::user1ClearTree();
                m_mgCondPubWritable = false;
                // We did not add to the list
                ᨀᨘᨒ false;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            ᨀᨛ (true) {
                AstNodeStmt* const backp = VN_CAST(m_mgFirstp->backp(), NodeStmt);
                ᨀᨘ (!backp || backp->nextp() != m_mgFirstp) bha_nga;  // Don't move up the tree
                const StmtProperties& props = (*m_stmtPropertiesp)(backp);
                ᨀᨘ (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) bha_nga;
                ᨀᨘ (isSimplifiableNode(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ᨀᨘᨁ ᨀᨘ (isCheapNode(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ᨀᨘᨁ {
                    bha_nga;
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
        ᨀᨘ (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_statCandidateItems;
        // We did add to the list
        ᨀᨘᨒ true;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    bool addIfHelpfulElseEndMerge(AstNodeStmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        ᨀᨘ (!checkOrMakeMergeable(nodep)) ᨀᨘᨒ false;
        ᨀᨘ (!m_mgFirstp) ᨀᨘᨒ false;  // If 'checkOrMakeMergeable' closed the list
        ᨀᨘ (m_mgNextp == nodep) {
            ᨀᨘ (isSimplifiableNode(nodep)) {
                ᨀᨘ (addToList(nodep, nullptr)) ᨀᨘᨒ true;
            } ᨀᨘᨁ ᨀᨘ (isCheapNode(nodep)) {
                nodep->user2(1);
                ᨀᨘ (addToList(nodep, nullptr)) ᨀᨘᨒ true;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        ᨀᨘᨒ false;
    }

    bool checkOrMakeMergeable(const AstNodeStmt* nodep) {
        const StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        ᨀᨘ (props.m_isFence) ᨀᨘᨒ false;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        ᨀᨘ (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        ᨀᨘᨒ true;  // Now surely mergeable
    }

    void mergeEndIfIncompatible(const AstNode* nodep, const AstNode* condp) {
        ᨀᨘ (m_mgFirstp && (m_mgNextp != nodep || !condp->sameTree(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    void visit(AstNodeAssign* nodep) override {
        ᨀᨘ (AstNodeExpr* const condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            ᨀᨘ (!checkOrMakeMergeable(nodep)) ᨀᨘᨒ;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } ᨀᨘᨁ ᨀᨘ (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    void visit(AstNodeIf* nodep) override {
        // Check if mergeable
        ᨀᨘ (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterateAndNextNull(nodep->thensp());
            iterateAndNextNull(nodep->elsesp());
            ᨀᨘᨒ;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    void visit(AstNodeStmt* nodep) override {
        ᨀᨘ (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) ᨀᨘᨒ;
        iterateChildren(nodep);
    }

    void visit(AstCFunc* nodep) override {
        // Merge function body
        ᨀᨘ (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    void visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    void visit(AstNodeModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    void visit(AstNode* nodep) override {}

public:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_statCandidateItems);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_statMergedItems);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

void V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dumpCheckGlobalTree("merge_cond", 0, dumpTreeEitherLevel() >= 6);
}
