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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3MergeCond.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3DupFinder.h"
#شامل "V3Hasher.h"
#شامل "V3Stats.h"

#شامل <queue>
#شامل <set>

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_گره* rhsp) {
    اگر (AstCond* ثابت condp = VN_CAST(rhsp, Cond)) {
        بازگشت condp;
    } وگرنه اگر (ثابت AstAnd* ثابت andp = VN_CAST(rhsp, And)) {
        اگر (AstCond* ثابت condp = VN_CAST(andp->rhsp(), Cond)) {
            اگر (VN_IS(andp->lhsp(), Const)) بازگشت condp;
        }
    }
    بازگشت nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
منطقی areDisjoint(ثابت std::set<ثابت AstVar*>& a, ثابت std::set<ثابت AstVar*>& b) {
    اگر (a.empty() || b.empty()) بازگشت درست;
    ثابت auto endA = a.end();
    ثابت auto endB = b.end();
    auto itA = a.begin();
    auto itB = b.begin();
    وقتی (درست) {
        اگر (*itA == *itB) بازگشت نادرست;
        اگر (std::less<ثابت AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            اگر (itA == endA) بازگشت درست;
        } وگرنه {
            itB = std::lower_bound(++itB, endB, *itA);
            اگر (itB == endB) بازگشت درست;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

ساختار StmtProperties final {
    Ast_گره_عبارت* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<ثابت AstVar*> m_rdVars;  // Variables read by this statement
    std::set<ثابت AstVar*> m_wrVars;  // Variables written by this statement
    منطقی m_isFence = نادرست;  // Nothing should move across this statement, nor should it be merged
    منطقی m_sideEffect = نادرست;  // Statement may have side effect, without access to model state
    منطقی m_implPubRd = نادرست;  // Statement may implicitly read public state (without VarRef)
    منطقی m_implPubWr = نادرست;  // Statement may implicitly write public state (without VarRef)
    منطقی m_explPubRef = نادرست;  // Statement explicitly references public state via VarRef
    Ast_گره_Stmt* m_prevWithSameCondp = nullptr;  // Previous node in same list, with same condition
    منطقی writesConditionVar(منطقی condPubWritable) ثابت {
        // This relies on MarkVarsVisitor having been called on the condition node
        برای (ثابت AstVar* ثابت varp : m_wrVars) {
            اگر (varp->user1()) بازگشت درست;
        }
        // If the condition contains a public variable, check if it might be written
        اگر (condPubWritable && m_implPubWr) بازگشت درست;
        // Otherwise not written
        بازگشت نادرست;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<Ast_گره_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
رده CodeMotionAnalysisVisitor final : عمومی VNVisitorConst {
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
    std::vector<V3DupFinder> m_پشته;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    ایستا Ast_گره_عبارت* extractCondition(ثابت Ast_گره_Stmt* nodep) {
        Ast_گره_عبارت* conditionp = nullptr;
        اگر (ثابت Ast_گره_Assign* ثابت assignp = VN_CAST(nodep, گره_Assign)) {
            اگر (AstCond* ثابت conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } وگرنه اگر (ثابت Ast_گره_If* ثابت ifp = VN_CAST(nodep, گره_If)) {
            conditionp = ifp->condp();
        }
        وقتی (AstCCast* ثابت castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        بازگشت conditionp;
    }

    تهی analyzeStmt(Ast_گره_Stmt* nodep, منطقی tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* ثابت outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        اگر (Ast_گره_عبارت* ثابت condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            اگر (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_پشته.back();
                // Find a duplicate condition
                ثابت V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                اگر (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.درج(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } وگرنه {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_گره_عبارت* ثابت firstp = VN_AS(dit->second, گره_عبارت);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_گره_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_گره(nodep);

        // If there is an enclosing statement, propagate properties upwards
        اگر (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.درج(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.درج(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    تهی analyzeVarRef(AstVarRef* nodep) {
        ثابت VAccess access = nodep->access();
        AstVar* ثابت varp = nodep->varp();
        // Gather read and written variables
        اگر (access.isReadOrRW()) m_propsp->m_rdVars.درج(varp);
        اگر (access.isWriteOrRW()) m_propsp->m_wrVars.درج(varp);
        اگر (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = درست;
        }
    }

    تهی checkProperties(Ast_گره* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        اگر (VN_IS(nodep, Stmt_عبارت)) بازگشت;

        // Call to a DPI import
        اگر (ثابت Ast_گره_CCall* ثابت ccallp = VN_CAST(nodep, گره_CCall)) {
            AstCFunc* ثابت funcp = ccallp->funcp();
            اگر (funcp->dpiImportWrapper()) {
                اگر (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = درست;
                    m_propsp->m_implPubRd = درست;
                    m_propsp->m_implPubWr = درست;
                }
                بازگشت;
            }
        }

        // Side effect, but assume does not access public state
        اگر (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = درست;
            بازگشت;
        }

        // If impure, or branch, mark statement as fence
        اگر (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = درست;
    }

    تهی analyze_گره(Ast_گره* nodep) {
        // Check properties of this node if under a statement
        اگر (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    تهی visit(Ast_گره* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        منطقی singletonListStart = نادرست;
        اگر (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            اگر (!singletonListStart) m_پشته.emplace_back(m_hasher);
        }

        // Analyze node
        اگر (Ast_گره_Stmt* ثابت stmtp = VN_CAST(nodep, گره_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } وگرنه اگر (AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } وگرنه {
            analyze_گره(nodep);
        }

        // Pop the stack at the end of a list
        اگر (!singletonListStart && !nodep->nextp()) m_پشته.برداشت_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(Ast_گره* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterateAndNextConstNull(nodep);
    }

عمومی:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    ایستا تهی analyze(Ast_گره* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

رده CodeMotionOptimizeVisitor final : عمومی VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    ایستا constexpr unsigned MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    ثابت StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    منطقی areSwappable(ثابت Ast_گره_Stmt* ap, ثابت Ast_گره_Stmt* bp) ثابت {
        ثابت StmtProperties& aProps = m_stmtProperties(ap);
        ثابت StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        اگر (aProps.m_isFence) بازگشت نادرست;
        اگر (bProps.m_isFence) بازگشت نادرست;
        // Don't swap side effecting statements, but can move others across them
        اگر (aProps.m_sideEffect && bProps.m_sideEffect) بازگشت نادرست;
        // Don't swap if there is a hazard around public state - must observe ordering
        ثابت منطقی bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        اگر (aProps.m_implPubWr && bPubRef) بازگشت نادرست;
        ثابت منطقی aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        اگر (aPubRef && bProps.m_implPubWr) بازگشت نادرست;
        // If either statement writes a variable that the other reads, they are not swappable
        اگر (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) بازگشت نادرست;
        اگر (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) بازگشت نادرست;
        // If they both write to the same variable, they are not swappable
        اگر (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) بازگشت نادرست;
        // Otherwise good to go
        بازگشت درست;
    }

    // VISITORS
    تهی visit(Ast_گره_Stmt* nodep) override {
        // Process only on first encounter
        اگر (nodep->user4SetOnce()) بازگشت;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_گره_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        اگر (!prevp) بازگشت;
#ifdef VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            ثابت Ast_گره* currp = prevp;
            وقتی (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#endif
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        اگر (Ast_گره_Stmt* predp = VN_CAST(nodep->backp(), گره_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            برای (unsigned i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                اگر (predp == prevp) بشکن;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_گره_Stmt* ثابت backp = VN_CAST(predp->backp(), گره_Stmt);
                اگر (!backp) بشکن;
                // Don't swap statements if doing so would change program semantics
                اگر (!areSwappable(predp, nodep)) بشکن;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            اگر (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->addNextHere(nodep);
                // If the predecessor is the previous node with the same condition, job done
                اگر (predp == prevp) بازگشت;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        برای (Ast_گره_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            اگر (Ast_گره_Stmt* succp = VN_CAST(prevp->nextp(), گره_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                برای (unsigned i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    اگر (succp == currp) بشکن;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_گره_Stmt* ثابت nextp = VN_CAST(succp->nextp(), گره_Stmt);
                    اگر (!nextp) بشکن;
                    // Don't swap statements if doing so would change program semantics
                    اگر (!areSwappable(prevp, succp)) بشکن;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                اگر (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->addHereThisAsNext(prevp);
                }
            }
        }
    }

    تهی visit(Ast_گره* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(Ast_گره* nodep, ثابت StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterateAndNextNull(nodep);
    }

عمومی:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    ایستا Ast_گره* optimize(Ast_گره* nodep, ثابت StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        وقتی (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        بازگشت nodep;
    }
};

//######################################################################
// Conditional merging

رده MergeCondVisitor final : عمومی VNVisitor {
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
    VDouble0 m_stat_Merged_اقلام;  // Statistic tracking
    VDouble0 m_stat_Candidate_اقلام;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_گره* m_mgFirstp = nullptr;  // First node in merged sequence
    Ast_گره_عبارت* m_mgCondp = nullptr;  // The condition of the first node
    ثابت Ast_گره* m_mgLastp = nullptr;  // Last node in merged sequence
    ثابت Ast_گره* m_mgNextp = nullptr;  // Next node in list being examined
    منطقی m_mgCondPubWritable = نادرست;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_گره*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    تهی process(Ast_گره* nodep) {
        // Set up work queue
        std::queue<Ast_گره*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->افزودن(nodep);

        انجامبده {
            // Set up user* for this iteration
            ثابت VNUser1InUse user1InUse;
            ثابت VNUser2InUse user2InUse;
            ثابت VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_گره* currp = m_workQueuep->front();
            m_workQueuep->برداشت();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            اگر (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterateAndNextNull(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            اگر (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } وقتی (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    ایستا Ast_گره* skipConstSels(Ast_گره* nodep) {
        وقتی (ثابت AstArraySel* ثابت aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            اگر (!VN_IS(aselp->bitp(), Const)) بازگشت nodep;
            nodep = aselp->fromp();
        }
        وقتی (ثابت AstWordSel* ثابت wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            اگر (!VN_IS(wselp->bitp(), Const)) بازگشت nodep;
            nodep = wselp->fromp();
        }
        بازگشت nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    ایستا منطقی is_Cheap_گره(Ast_گره* nodep) {
        // Comments are cheap
        اگر (VN_IS(nodep, Comment)) بازگشت درست;
        // So are some assignments
        اگر (ثابت Ast_گره_Assign* ثابت assignp = VN_CAST(nodep, گره_Assign)) {
            // Check LHS
            Ast_گره* ثابت lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            اگر (!VN_IS(lhsp, VarRef)) بازگشت نادرست;

            // Check RHS
            Ast_گره* ثابت rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            اگر (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) بازگشت نادرست;

            // Otherwise it is a cheap assignment
            بازگشت درست;
        }
        // Others are not
        بازگشت نادرست;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    ایستا منطقی yieldsOneOrZero(ثابت Ast_گره* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            بازگشت constp->رقم().toUQuad() <= 1;
        }
        اگر (ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            ثابت AstVar* ثابت varp = vrefp->varp();
            بازگشت varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        اگر (ثابت AstShiftR* ثابت shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            اگر (ثابت AstConst* ثابت constp = VN_CAST(shiftp->rhsp(), Const)) {
                ثابت AstVarRef* ثابت vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                ثابت صحیح width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                اگر (constp->toSInt() >= width - 1) بازگشت درست;
            }
            بازگشت نادرست;
        }
        اگر (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            بازگشت درست;
        }
        اگر (ثابت Ast_گره_Biop* ثابت biopp = VN_CAST(nodep, گره_Biop)) {
            اگر (VN_IS(nodep, And))
                بازگشت yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            اگر (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                بازگشت yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            بازگشت نادرست;
        }
        اگر (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            بازگشت yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        اگر (ثابت AstCCast* ثابت castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            بازگشت yieldsOneOrZero(castp->lhsp());
        }
        بازگشت نادرست;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    ایستا Ast_گره_عبارت* maskLsb(Ast_گره_عبارت* nodep) {
        اگر (yieldsOneOrZero(nodep)) بازگشت nodep;
        // Otherwise apply masking
        AstConst* ثابت maskp = جدید AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        بازگشت جدید AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_گره_عبارت* foldAndUnlink(Ast_گره_عبارت* rhsp, منطقی condTrue) {
        اگر (rhsp->same_درخت(m_mgCondp)) {
            بازگشت جدید AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } وگرنه اگر (ثابت AstCond* ثابت condp = extractCondFromRhs(rhsp)) {
            Ast_گره_عبارت* ثابت resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            اگر (condp == rhsp) بازگشت resp;
            اگر (ثابت AstAnd* ثابت andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                بازگشت جدید AstAnd{andp->fileline(), andp->lhsp()->clone_درخت_Pure(نادرست), resp};
            }
        } وگرنه اگر (ثابت AstAnd* ثابت andp = VN_CAST(rhsp, And)) {
            اگر (andp->lhsp()->same_درخت(m_mgCondp)) {
                بازگشت condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : جدید AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } وگرنه {
                UASSERT_OBJ(andp->rhsp()->same_درخت(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                بازگشت condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : جدید AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } وگرنه اگر (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            بازگشت rhsp->clone_درخت(نادرست);
        }
        // LCOV_EXCL_START
        اگر (debug()) rhsp->dump_درخت("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        بازگشت nullptr;
        // LCOV_EXCL_STOP
    }

    تهی mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        وقتی (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            ثابت Ast_گره* ثابت backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        وقتی (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            ثابت Ast_گره* ثابت nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_گره_If* recursivep = nullptr;
        // Merge if list is longer than one node
        اگر (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            اگر (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_درخت_Pure(نادرست);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* ثابت resultp = جدید AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->addHereThisAsNext(resultp);
            // Try to preserve branch prediction if we can
            size_t nLikely = 0;
            size_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_گره* nextp = m_mgFirstp;
            انجامبده {
                // Grab next pointer and unlink
                Ast_گره* ثابت currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                اگر (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_درخت(), currp);
                    ادامه;
                }
                //شمار
                ++m_stat_Merged_اقلام;
                اگر (Ast_گره_Assign* ثابت assignp = VN_CAST(currp, گره_Assign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_گره_عبارت* ثابت rhsp = assignp->rhsp()->unlinkFrBack();
                    Ast_گره_Assign* ثابت thenp = assignp;
                    Ast_گره_Assign* ثابت elsep = assignp->clone_درخت(نادرست);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, درست));
                    elsep->rhsp(foldAndUnlink(rhsp, نادرست));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
                } وگرنه {
                    Ast_گره_If* ثابت ifp = VN_AS(currp, گره_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    اگر (Ast_گره* ثابت listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlinkFrBackWithNext());
                    }
                    اگر (Ast_گره* ثابت listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlinkFrBackWithNext());
                    }
                    // Record branch prediction
                    اگر (ifp->branchPred().likely()) ++nLikely;
                    اگر (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_درخت(), ifp);
                }
            } وقتی (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            اگر (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            اگر (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            اگر (resultp->thensp()) m_workQueuep->افزودن(resultp->thensp());
            اگر (resultp->elsesp()) m_workQueuep->افزودن(resultp->elsesp());
        } وگرنه اگر (Ast_گره_If* ثابت ifp = VN_CAST(m_mgFirstp, گره_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = nullptr;
        m_mgCondp = nullptr;
        m_mgLastp = nullptr;
        m_mgNextp = nullptr;
        m_mgCondPubWritable = نادرست;
        Ast_گره::user1_Clear_درخت();  // Clear marked variables
        Ast_گره::user2_Clear_درخت();
        // Merge recursively within the branches of an un-merged AstNodeIF
        اگر (recursivep) {
            iterateAndNextNull(recursivep->thensp());
            iterateAndNextNull(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            اگر (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    منطقی is_Simplifiable_گره(Ast_گره* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        اگر (ثابت Ast_گره_Assign* ثابت assignp = VN_CAST(nodep, گره_Assign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            اگر (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                اگر (ثابت AstAnd* ثابت andp = VN_CAST(assignp->rhsp(), And)) {
                    اگر (andp->lhsp()->same_درخت(m_mgCondp) || andp->rhsp()->same_درخت(m_mgCondp)) {
                        بازگشت درست;
                    }
                }
                // Is it 'lhs = cond'?
                اگر (assignp->rhsp()->same_درخت(m_mgCondp)) بازگشت درست;
            }
        }
        بازگشت نادرست;
    }

    منطقی addToList(Ast_گره_Stmt* nodep, Ast_گره_عبارت* condp) {
        // Set up head of new list if node is first in list
        اگر (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](ثابت AstVarRef* nodep) {
                AstVar* ثابت varp = nodep->varp();
                varp->user1(1);
                اگر (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = درست;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            اگر ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_گره::user1_Clear_درخت();
                m_mgCondPubWritable = نادرست;
                // We did not add to the list
                بازگشت نادرست;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            وقتی (درست) {
                Ast_گره_Stmt* ثابت backp = VN_CAST(m_mgFirstp->backp(), گره_Stmt);
                اگر (!backp || backp->nextp() != m_mgFirstp) بشکن;  // Don't move up the tree
                ثابت StmtProperties& props = (*m_stmtPropertiesp)(backp);
                اگر (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) بشکن;
                اگر (is_Simplifiable_گره(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } وگرنه اگر (is_Cheap_گره(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } وگرنه {
                    بشکن;
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
        اگر (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_stat_Candidate_اقلام;
        // We did add to the list
        بازگشت درست;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    منطقی addIfHelpfulElseEndMerge(Ast_گره_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        اگر (!checkOrMakeMergeable(nodep)) بازگشت نادرست;
        اگر (!m_mgFirstp) بازگشت نادرست;  // If 'checkOrMakeMergeable' closed the list
        اگر (m_mgNextp == nodep) {
            اگر (is_Simplifiable_گره(nodep)) {
                اگر (addToList(nodep, nullptr)) بازگشت درست;
            } وگرنه اگر (is_Cheap_گره(nodep)) {
                nodep->user2(1);
                اگر (addToList(nodep, nullptr)) بازگشت درست;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        بازگشت نادرست;
    }

    منطقی checkOrMakeMergeable(ثابت Ast_گره_Stmt* nodep) {
        ثابت StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        اگر (props.m_isFence) بازگشت نادرست;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        اگر (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        بازگشت درست;  // Now surely mergeable
    }

    تهی mergeEndIfIncompatible(ثابت Ast_گره* nodep, ثابت Ast_گره* condp) {
        اگر (m_mgFirstp && (m_mgNextp != nodep || !condp->same_درخت(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (Ast_گره_عبارت* ثابت condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            اگر (!checkOrMakeMergeable(nodep)) بازگشت;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } وگرنه اگر (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    تهی visit(Ast_گره_If* nodep) override {
        // Check if mergeable
        اگر (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterateAndNextNull(nodep->thensp());
            iterateAndNextNull(nodep->elsesp());
            بازگشت;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    تهی visit(Ast_گره_Stmt* nodep) override {
        اگر (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) بازگشت;
        iterateChildren(nodep);
    }

    تهی visit(AstCFunc* nodep) override {
        // Merge function body
        اگر (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    تهی visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    تهی visit(Ast_گره_Module* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    تهی visit(Ast_گره* nodep) override {}

عمومی:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_stat_Candidate_اقلام);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_stat_Merged_اقلام);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

تهی V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("merge_cond", 0, dump_درخت_Either_Level() >= 6);
}
