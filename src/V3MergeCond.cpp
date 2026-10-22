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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3MergeCond.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3DupFinder.h"
#ضمن "V3Hasher.h"
#ضمن "V3Stats.h"

#ضمن <queue>
#ضمن <set>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_عقدة* rhsp) {
    إذا (AstCond* ثابت condp = VN_CAST(rhsp, Cond)) {
        أرجع condp;
    } وإلا إذا (ثابت AstAnd* ثابت andp = VN_CAST(rhsp, And)) {
        إذا (AstCond* ثابت condp = VN_CAST(andp->rhsp(), Cond)) {
            إذا (VN_IS(andp->lhsp(), Const)) أرجع condp;
        }
    }
    أرجع nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
منطقي areDisjoint(ثابت std::set<ثابت AstVar*>& a, ثابت std::set<ثابت AstVar*>& b) {
    إذا (a.empty() || b.empty()) أرجع true;
    ثابت auto endA = a.end();
    ثابت auto endB = b.end();
    auto itA = a.begin();
    auto itB = b.begin();
    طالما (true) {
        إذا (*itA == *itB) أرجع خطأ;
        إذا (std::less<ثابت AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            إذا (itA == endA) أرجع true;
        } وإلا {
            itB = std::lower_bound(++itB, endB, *itA);
            إذا (itB == endB) أرجع true;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

بنية StmtProperties final {
    Ast_عقدة_تعبير* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<ثابت AstVar*> m_rdVars;  // Variables read by this statement
    std::set<ثابت AstVar*> m_wrVars;  // Variables written by this statement
    منطقي m_isFence = خطأ;  // Nothing should move across this statement, nor should it be merged
    منطقي m_sideEffect = خطأ;  // Statement may have side effect, without access to model state
    منطقي m_implPubRd = خطأ;  // Statement may implicitly read public state (without VarRef)
    منطقي m_implPubWr = خطأ;  // Statement may implicitly write public state (without VarRef)
    منطقي m_explPubRef = خطأ;  // Statement explicitly references public state via VarRef
    Ast_عقدة_Stmt* m_prevWithSameCondp = nullptr;  // Previous node in same list, with same condition
    منطقي writesConditionVar(منطقي condPubWritable) ثابت {
        // This relies on MarkVarsVisitor having been called on the condition node
        لكل (ثابت AstVar* ثابت varp : m_wrVars) {
            إذا (varp->user1()) أرجع true;
        }
        // If the condition contains a public variable, check if it might be written
        إذا (condPubWritable && m_implPubWr) أرجع true;
        // Otherwise not written
        أرجع خطأ;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<Ast_عقدة_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
صنف CodeMotionAnalysisVisitor final : عام VNVisitorConst {
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
    std::vector<V3DupFinder> m_مكدس;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    ساكن Ast_عقدة_تعبير* extractCondition(ثابت Ast_عقدة_Stmt* nodep) {
        Ast_عقدة_تعبير* conditionp = nullptr;
        إذا (ثابت Ast_عقدة_Assign* ثابت assignp = VN_CAST(nodep, عقدة_Assign)) {
            إذا (AstCond* ثابت conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } وإلا إذا (ثابت Ast_عقدة_If* ثابت ifp = VN_CAST(nodep, عقدة_If)) {
            conditionp = ifp->condp();
        }
        طالما (AstCCast* ثابت castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        أرجع conditionp;
    }

    فراغ analyzeStmt(Ast_عقدة_Stmt* nodep, منطقي tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* ثابت outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        إذا (Ast_عقدة_تعبير* ثابت condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            إذا (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_مكدس.back();
                // Find a duplicate condition
                ثابت V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                إذا (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.أدرج(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } وإلا {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_عقدة_تعبير* ثابت firstp = VN_AS(dit->second, عقدة_تعبير);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_عقدة_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_عقدة(nodep);

        // If there is an enclosing statement, propagate properties upwards
        إذا (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.أدرج(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.أدرج(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    فراغ analyzeVarRef(AstVarRef* nodep) {
        ثابت VAccess access = nodep->access();
        AstVar* ثابت varp = nodep->varp();
        // Gather read and written variables
        إذا (access.isReadOrRW()) m_propsp->m_rdVars.أدرج(varp);
        إذا (access.isWriteOrRW()) m_propsp->m_wrVars.أدرج(varp);
        إذا (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = true;
        }
    }

    فراغ checkProperties(Ast_عقدة* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        إذا (VN_IS(nodep, Stmt_تعبير)) أرجع;

        // Call to a DPI import
        إذا (ثابت Ast_عقدة_CCall* ثابت ccallp = VN_CAST(nodep, عقدة_CCall)) {
            AstCFunc* ثابت funcp = ccallp->funcp();
            إذا (funcp->dpiImportWrapper()) {
                إذا (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = true;
                    m_propsp->m_implPubRd = true;
                    m_propsp->m_implPubWr = true;
                }
                أرجع;
            }
        }

        // Side effect, but assume does not access public state
        إذا (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = true;
            أرجع;
        }

        // If impure, or branch, mark statement as fence
        إذا (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = true;
    }

    فراغ analyze_عقدة(Ast_عقدة* nodep) {
        // Check properties of this node if under a statement
        إذا (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ visit(Ast_عقدة* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        منطقي singletonListStart = خطأ;
        إذا (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            إذا (!singletonListStart) m_مكدس.emplace_back(m_hasher);
        }

        // Analyze node
        إذا (Ast_عقدة_Stmt* ثابت stmtp = VN_CAST(nodep, عقدة_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } وإلا إذا (AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } وإلا {
            analyze_عقدة(nodep);
        }

        // Pop the stack at the end of a list
        إذا (!singletonListStart && !nodep->nextp()) m_مكدس.اسحب_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(Ast_عقدة* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterateAndNextConstNull(nodep);
    }

عام:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    ساكن فراغ analyze(Ast_عقدة* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

صنف CodeMotionOptimizeVisitor final : عام VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    ساكن constexpr unsigned MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    ثابت StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    منطقي areSwappable(ثابت Ast_عقدة_Stmt* ap, ثابت Ast_عقدة_Stmt* bp) ثابت {
        ثابت StmtProperties& aProps = m_stmtProperties(ap);
        ثابت StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        إذا (aProps.m_isFence) أرجع خطأ;
        إذا (bProps.m_isFence) أرجع خطأ;
        // Don't swap side effecting statements, but can move others across them
        إذا (aProps.m_sideEffect && bProps.m_sideEffect) أرجع خطأ;
        // Don't swap if there is a hazard around public state - must observe ordering
        ثابت منطقي bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        إذا (aProps.m_implPubWr && bPubRef) أرجع خطأ;
        ثابت منطقي aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        إذا (aPubRef && bProps.m_implPubWr) أرجع خطأ;
        // If either statement writes a variable that the other reads, they are not swappable
        إذا (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) أرجع خطأ;
        إذا (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) أرجع خطأ;
        // If they both write to the same variable, they are not swappable
        إذا (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) أرجع خطأ;
        // Otherwise good to go
        أرجع true;
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Stmt* nodep) override {
        // Process only on first encounter
        إذا (nodep->user4SetOnce()) أرجع;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_عقدة_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        إذا (!prevp) أرجع;
#ifdef VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            ثابت Ast_عقدة* currp = prevp;
            طالما (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#endif
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        إذا (Ast_عقدة_Stmt* predp = VN_CAST(nodep->backp(), عقدة_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            لكل (unsigned i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                إذا (predp == prevp) اكسر;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_عقدة_Stmt* ثابت backp = VN_CAST(predp->backp(), عقدة_Stmt);
                إذا (!backp) اكسر;
                // Don't swap statements if doing so would change program semantics
                إذا (!areSwappable(predp, nodep)) اكسر;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            إذا (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->addNextHere(nodep);
                // If the predecessor is the previous node with the same condition, job done
                إذا (predp == prevp) أرجع;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        لكل (Ast_عقدة_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            إذا (Ast_عقدة_Stmt* succp = VN_CAST(prevp->nextp(), عقدة_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                لكل (unsigned i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    إذا (succp == currp) اكسر;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_عقدة_Stmt* ثابت nextp = VN_CAST(succp->nextp(), عقدة_Stmt);
                    إذا (!nextp) اكسر;
                    // Don't swap statements if doing so would change program semantics
                    إذا (!areSwappable(prevp, succp)) اكسر;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                إذا (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->addHereThisAsNext(prevp);
                }
            }
        }
    }

    فراغ visit(Ast_عقدة* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(Ast_عقدة* nodep, ثابت StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterateAndNextNull(nodep);
    }

عام:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    ساكن Ast_عقدة* optimize(Ast_عقدة* nodep, ثابت StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        طالما (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        أرجع nodep;
    }
};

//######################################################################
// Conditional merging

صنف MergeCondVisitor final : عام VNVisitor {
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
    VDouble0 m_stat_Merged_عناصر;  // Statistic tracking
    VDouble0 m_stat_Candidate_عناصر;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_عقدة* m_mgFirstp = nullptr;  // First node in merged sequence
    Ast_عقدة_تعبير* m_mgCondp = nullptr;  // The condition of the first node
    ثابت Ast_عقدة* m_mgLastp = nullptr;  // Last node in merged sequence
    ثابت Ast_عقدة* m_mgNextp = nullptr;  // Next node in list being examined
    منطقي m_mgCondPubWritable = خطأ;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_عقدة*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    فراغ process(Ast_عقدة* nodep) {
        // Set up work queue
        std::queue<Ast_عقدة*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->ادفع(nodep);

        افعل {
            // Set up user* for this iteration
            ثابت VNUser1InUse user1InUse;
            ثابت VNUser2InUse user2InUse;
            ثابت VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_عقدة* currp = m_workQueuep->front();
            m_workQueuep->اسحب();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            إذا (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterateAndNextNull(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            إذا (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } طالما (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    ساكن Ast_عقدة* skipConstSels(Ast_عقدة* nodep) {
        طالما (ثابت AstArraySel* ثابت aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            إذا (!VN_IS(aselp->bitp(), Const)) أرجع nodep;
            nodep = aselp->fromp();
        }
        طالما (ثابت AstWordSel* ثابت wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            إذا (!VN_IS(wselp->bitp(), Const)) أرجع nodep;
            nodep = wselp->fromp();
        }
        أرجع nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    ساكن منطقي is_Cheap_عقدة(Ast_عقدة* nodep) {
        // Comments are cheap
        إذا (VN_IS(nodep, Comment)) أرجع true;
        // So are some assignments
        إذا (ثابت Ast_عقدة_Assign* ثابت assignp = VN_CAST(nodep, عقدة_Assign)) {
            // Check LHS
            Ast_عقدة* ثابت lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            إذا (!VN_IS(lhsp, VarRef)) أرجع خطأ;

            // Check RHS
            Ast_عقدة* ثابت rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            إذا (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) أرجع خطأ;

            // Otherwise it is a cheap assignment
            أرجع true;
        }
        // Others are not
        أرجع خطأ;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    ساكن منطقي yieldsOneOrZero(ثابت Ast_عقدة* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            أرجع constp->رقم().toUQuad() <= 1;
        }
        إذا (ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            ثابت AstVar* ثابت varp = vrefp->varp();
            أرجع varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        إذا (ثابت AstShiftR* ثابت shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            إذا (ثابت AstConst* ثابت constp = VN_CAST(shiftp->rhsp(), Const)) {
                ثابت AstVarRef* ثابت vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                ثابت صحيح width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                إذا (constp->toSInt() >= width - 1) أرجع true;
            }
            أرجع خطأ;
        }
        إذا (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            أرجع true;
        }
        إذا (ثابت Ast_عقدة_Biop* ثابت biopp = VN_CAST(nodep, عقدة_Biop)) {
            إذا (VN_IS(nodep, And))
                أرجع yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            إذا (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                أرجع yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            أرجع خطأ;
        }
        إذا (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            أرجع yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        إذا (ثابت AstCCast* ثابت castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            أرجع yieldsOneOrZero(castp->lhsp());
        }
        أرجع خطأ;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    ساكن Ast_عقدة_تعبير* maskLsb(Ast_عقدة_تعبير* nodep) {
        إذا (yieldsOneOrZero(nodep)) أرجع nodep;
        // Otherwise apply masking
        AstConst* ثابت maskp = جديد AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        أرجع جديد AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_عقدة_تعبير* foldAndUnlink(Ast_عقدة_تعبير* rhsp, منطقي condTrue) {
        إذا (rhsp->same_شجرة(m_mgCondp)) {
            أرجع جديد AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } وإلا إذا (ثابت AstCond* ثابت condp = extractCondFromRhs(rhsp)) {
            Ast_عقدة_تعبير* ثابت resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            إذا (condp == rhsp) أرجع resp;
            إذا (ثابت AstAnd* ثابت andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                أرجع جديد AstAnd{andp->fileline(), andp->lhsp()->clone_شجرة_Pure(خطأ), resp};
            }
        } وإلا إذا (ثابت AstAnd* ثابت andp = VN_CAST(rhsp, And)) {
            إذا (andp->lhsp()->same_شجرة(m_mgCondp)) {
                أرجع condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : جديد AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } وإلا {
                UASSERT_OBJ(andp->rhsp()->same_شجرة(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                أرجع condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : جديد AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } وإلا إذا (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            أرجع rhsp->clone_شجرة(خطأ);
        }
        // LCOV_EXCL_START
        إذا (debug()) rhsp->dump_شجرة("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        أرجع nullptr;
        // LCOV_EXCL_STOP
    }

    فراغ mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        طالما (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            ثابت Ast_عقدة* ثابت backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        طالما (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            ثابت Ast_عقدة* ثابت nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_عقدة_If* recursivep = nullptr;
        // Merge if list is longer than one node
        إذا (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            إذا (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_شجرة_Pure(خطأ);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* ثابت resultp = جديد AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->addHereThisAsNext(resultp);
            // Try to preserve branch prediction if we can
            size_t nLikely = 0;
            size_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_عقدة* nextp = m_mgFirstp;
            افعل {
                // Grab next pointer and unlink
                Ast_عقدة* ثابت currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                إذا (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_شجرة(), currp);
                    استمر;
                }
                //عدد
                ++m_stat_Merged_عناصر;
                إذا (Ast_عقدة_Assign* ثابت assignp = VN_CAST(currp, عقدة_Assign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_عقدة_تعبير* ثابت rhsp = assignp->rhsp()->unlinkFrBack();
                    Ast_عقدة_Assign* ثابت thenp = assignp;
                    Ast_عقدة_Assign* ثابت elsep = assignp->clone_شجرة(خطأ);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, true));
                    elsep->rhsp(foldAndUnlink(rhsp, خطأ));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_شجرة(), rhsp);
                } وإلا {
                    Ast_عقدة_If* ثابت ifp = VN_AS(currp, عقدة_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    إذا (Ast_عقدة* ثابت listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlinkFrBackWithNext());
                    }
                    إذا (Ast_عقدة* ثابت listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlinkFrBackWithNext());
                    }
                    // Record branch prediction
                    إذا (ifp->branchPred().likely()) ++nLikely;
                    إذا (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_شجرة(), ifp);
                }
            } طالما (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            إذا (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            إذا (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            إذا (resultp->thensp()) m_workQueuep->ادفع(resultp->thensp());
            إذا (resultp->elsesp()) m_workQueuep->ادفع(resultp->elsesp());
        } وإلا إذا (Ast_عقدة_If* ثابت ifp = VN_CAST(m_mgFirstp, عقدة_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = nullptr;
        m_mgCondp = nullptr;
        m_mgLastp = nullptr;
        m_mgNextp = nullptr;
        m_mgCondPubWritable = خطأ;
        Ast_عقدة::user1_Clear_شجرة();  // Clear marked variables
        Ast_عقدة::user2_Clear_شجرة();
        // Merge recursively within the branches of an un-merged AstNodeIF
        إذا (recursivep) {
            iterateAndNextNull(recursivep->thensp());
            iterateAndNextNull(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            إذا (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    منطقي is_Simplifiable_عقدة(Ast_عقدة* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        إذا (ثابت Ast_عقدة_Assign* ثابت assignp = VN_CAST(nodep, عقدة_Assign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            إذا (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                إذا (ثابت AstAnd* ثابت andp = VN_CAST(assignp->rhsp(), And)) {
                    إذا (andp->lhsp()->same_شجرة(m_mgCondp) || andp->rhsp()->same_شجرة(m_mgCondp)) {
                        أرجع true;
                    }
                }
                // Is it 'lhs = cond'?
                إذا (assignp->rhsp()->same_شجرة(m_mgCondp)) أرجع true;
            }
        }
        أرجع خطأ;
    }

    منطقي addToList(Ast_عقدة_Stmt* nodep, Ast_عقدة_تعبير* condp) {
        // Set up head of new list if node is first in list
        إذا (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](ثابت AstVarRef* nodep) {
                AstVar* ثابت varp = nodep->varp();
                varp->user1(1);
                إذا (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = true;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            إذا ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_عقدة::user1_Clear_شجرة();
                m_mgCondPubWritable = خطأ;
                // We did not add to the list
                أرجع خطأ;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            طالما (true) {
                Ast_عقدة_Stmt* ثابت backp = VN_CAST(m_mgFirstp->backp(), عقدة_Stmt);
                إذا (!backp || backp->nextp() != m_mgFirstp) اكسر;  // Don't move up the tree
                ثابت StmtProperties& props = (*m_stmtPropertiesp)(backp);
                إذا (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) اكسر;
                إذا (is_Simplifiable_عقدة(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } وإلا إذا (is_Cheap_عقدة(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } وإلا {
                    اكسر;
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
        إذا (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_stat_Candidate_عناصر;
        // We did add to the list
        أرجع true;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    منطقي addIfHelpfulElseEndMerge(Ast_عقدة_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        إذا (!checkOrMakeMergeable(nodep)) أرجع خطأ;
        إذا (!m_mgFirstp) أرجع خطأ;  // If 'checkOrMakeMergeable' closed the list
        إذا (m_mgNextp == nodep) {
            إذا (is_Simplifiable_عقدة(nodep)) {
                إذا (addToList(nodep, nullptr)) أرجع true;
            } وإلا إذا (is_Cheap_عقدة(nodep)) {
                nodep->user2(1);
                إذا (addToList(nodep, nullptr)) أرجع true;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        أرجع خطأ;
    }

    منطقي checkOrMakeMergeable(ثابت Ast_عقدة_Stmt* nodep) {
        ثابت StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        إذا (props.m_isFence) أرجع خطأ;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        إذا (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        أرجع true;  // Now surely mergeable
    }

    فراغ mergeEndIfIncompatible(ثابت Ast_عقدة* nodep, ثابت Ast_عقدة* condp) {
        إذا (m_mgFirstp && (m_mgNextp != nodep || !condp->same_شجرة(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Assign* nodep) override {
        إذا (Ast_عقدة_تعبير* ثابت condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            إذا (!checkOrMakeMergeable(nodep)) أرجع;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } وإلا إذا (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    فراغ visit(Ast_عقدة_If* nodep) override {
        // Check if mergeable
        إذا (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterateAndNextNull(nodep->thensp());
            iterateAndNextNull(nodep->elsesp());
            أرجع;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    فراغ visit(Ast_عقدة_Stmt* nodep) override {
        إذا (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) أرجع;
        iterateChildren(nodep);
    }

    فراغ visit(AstCFunc* nodep) override {
        // Merge function body
        إذا (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    فراغ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    فراغ visit(Ast_عقدة_Module* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    فراغ visit(Ast_عقدة* nodep) override {}

عام:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_stat_Candidate_عناصر);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_stat_Merged_عناصر);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

فراغ V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_شجرة("merge_cond", 0, dump_شجرة_Either_Level() >= 6);
}
