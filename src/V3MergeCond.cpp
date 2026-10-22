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
AstCond* کڈھو_Cond_From_Rhs(Ast_گرہ* rhsp) {
    إذا (AstCond* ثابت condp = VN_CAST(rhsp, Cond)) {
        أرجع condp;
    } وإلا إذا (ثابت AstAnd* ثابت andp = VN_CAST(rhsp, And)) {
        إذا (AstCond* ثابت condp = VN_CAST(andp->rhsp(), Cond)) {
            إذا (VN_IS(andp->lhsp(), Const)) أرجع condp;
        }
    }
    أرجع خالی_سوچک;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
منطقي areDisjoint(ثابت std::سیٹ<ثابت AstVar*>& a, ثابت std::سیٹ<ثابت AstVar*>& b) {
    إذا (a.خالی_تھاواں() || b.خالی_تھاواں()) أرجع سچ;
    ثابت خودکار سرا_A = a.سرا();
    ثابت خودکار سرا_B = b.سرا();
    خودکار itA = a.begin();
    خودکار itB = b.begin();
    طالما (سچ) {
        إذا (*itA == *itB) أرجع خطأ;
        إذا (std::less<ثابت AstVar*>{}(*itA, *itB)) {
            itA = std::ہیٹھلا_bound(++itA, سرا_A, *itB);
            إذا (itA == سرا_A) أرجع سچ;
        } وإلا {
            itB = std::ہیٹھلا_bound(++itB, سرا_B, *itA);
            إذا (itB == سرا_B) أرجع سچ;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

بنية StmtProperties final {
    Ast_گرہ_اظہاریہ* m_condp = خالی_سوچک;  // The condition expression, if a conditional node
    std::سیٹ<ثابت AstVar*> m_rdVars;  // Variables read by this statement
    std::سیٹ<ثابت AstVar*> m_wrVars;  // Variables written by this statement
    منطقي m_isFence = خطأ;  // Nothing should move across this statement, nor should it be merged
    منطقي m_sideEffect = خطأ;  // Statement may have side effect, without access to model state
    منطقي m_implPubRd = خطأ;  // Statement may implicitly read public state (without VarRef)
    منطقي m_implPubWr = خطأ;  // Statement may implicitly write public state (without VarRef)
    منطقي m_explPubRef = خطأ;  // Statement explicitly references public state via VarRef
    Ast_گرہ_Stmt* m_prevWithSameCondp = خالی_سوچک;  // Previous node in same list, with same condition
    منطقي writesConditionVar(منطقي condPubWritable) ثابت {
        // This relies on MarkVarsVisitor having been called on the condition node
        لكل (ثابت AstVar* ثابت varp : m_wrVars) {
            إذا (varp->user1()) أرجع سچ;
        }
        // If the condition contains a public variable, check if it might be written
        إذا (condPubWritable && m_implPubWr) أرجع سچ;
        // Otherwise not written
        أرجع خطأ;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
ورتوں StmtPropertiesAllocator = AstUser3Allocator<Ast_گرہ_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
صنف کوڈ_Motion_Analysis_Visitor final : عام VNVisitorConst {
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
    std::سمتیہ<V3DupFinder> m_ڈھیر;
    StmtProperties* m_propsp = خالی_سوچک;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    ساكن Ast_گرہ_اظہاریہ* کڈھو_Condition(ثابت Ast_گرہ_Stmt* nodep) {
        Ast_گرہ_اظہاریہ* conditionp = خالی_سوچک;
        إذا (ثابت Ast_گرہ_سونپو* ثابت assignp = VN_CAST(nodep, گرہ_سونپو)) {
            إذا (AstCond* ثابت conditionalp = کڈھو_Cond_From_Rhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } وإلا إذا (ثابت Ast_گرہ_If* ثابت ifp = VN_CAST(nodep, گرہ_If)) {
            conditionp = ifp->condp();
        }
        طالما (AstCCast* ثابت castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        أرجع conditionp;
    }

    فراغ analyzeStmt(Ast_گرہ_Stmt* nodep, منطقي try_Cond_میل) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* ثابت outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        إذا (Ast_گرہ_اظہاریہ* ثابت condp = کڈھو_Condition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            إذا (try_Cond_میل) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_ڈھیر.پٹھ();
                // Find a duplicate condition
                ثابت V3DupFinder::iterator& dit = dupFinder.لبھو_Duplicate(condp);
                إذا (dit == dupFinder.سرا()) {
                    // First time seeing this condition in the current list
                    dupFinder.داخل_کرو(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } وإلا {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_گرہ_اظہاریہ* ثابت firstp = VN_AS(dit->دوجا, گرہ_اظہاریہ);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_گرہ_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_گرہ(nodep);

        // If there is an enclosing statement, propagate properties upwards
        إذا (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.داخل_کرو(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.داخل_کرو(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
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
        إذا (access.isReadOrRW()) m_propsp->m_rdVars.داخل_کرو(varp);
        إذا (access.isWriteOrRW()) m_propsp->m_wrVars.داخل_کرو(varp);
        إذا (varp->is_Sig_عوامی() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = سچ;
        }
    }

    فراغ checkProperties(Ast_گرہ* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        إذا (VN_IS(nodep, Stmt_اظہاریہ)) أرجع;

        // Call to a DPI import
        إذا (ثابت Ast_گرہ_CCall* ثابت ccallp = VN_CAST(nodep, گرہ_CCall)) {
            AstCFunc* ثابت funcp = ccallp->funcp();
            إذا (funcp->dpiImportWrapper()) {
                إذا (!funcp->dpi_خالص()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = سچ;
                    m_propsp->m_implPubRd = سچ;
                    m_propsp->m_implPubWr = سچ;
                }
                أرجع;
            }
        }

        // Side effect, but assume does not access public state
        إذا (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = سچ;
            أرجع;
        }

        // If impure, or branch, mark statement as fence
        إذا (!nodep->is_خالص() || nodep->isBrancher()) m_propsp->m_isFence = سچ;
    }

    فراغ analyze_گرہ(Ast_گرہ* nodep) {
        // Check properties of this node if under a statement
        إذا (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        منطقي singleton_List_شروع = خطأ;
        إذا (nodep->backp()->nextp() != nodep) {  // If at head of list
            singleton_List_شروع = nodep->nextp() == خالی_سوچک;
            إذا (!singleton_List_شروع) m_ڈھیر.emplace_پٹھ(m_hasher);
        }

        // Analyze node
        إذا (Ast_گرہ_Stmt* ثابت stmtp = VN_CAST(nodep, گرہ_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singleton_List_شروع);
        } وإلا إذا (AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } وإلا {
            analyze_گرہ(nodep);
        }

        // Pop the stack at the end of a list
        إذا (!singleton_List_شروع && !nodep->nextp()) m_ڈھیر.نکالو_پٹھ();
    }

    // CONSTRUCTOR
    کوڈ_Motion_Analysis_Visitor(Ast_گرہ* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterate_And_اگلا_Const_Null(nodep);
    }

عام:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    ساكن فراغ analyze(Ast_گرہ* nodep, StmtPropertiesAllocator& stmtProperties) {
        کوڈ_Motion_Analysis_Visitor{nodep, stmtProperties};
    }
};

صنف کوڈ_Motion_Optimize_Visitor final : عام VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    ساكن constexpr بنانشانی MAX_فاصلہ = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4_وچ_Use m_user4_وچ_Use;

    ثابت StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    منطقي areSwappable(ثابت Ast_گرہ_Stmt* ap, ثابت Ast_گرہ_Stmt* bp) ثابت {
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
        أرجع سچ;
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {
        // Process only on first encounter
        إذا (nodep->user4_سیٹ_Once()) أرجع;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_گرہ_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        إذا (!prevp) أرجع;
#جے_تعریف_شدہ VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            ثابت Ast_گرہ* currp = prevp;
            طالما (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#جے_انت
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        إذا (Ast_گرہ_Stmt* predp = VN_CAST(nodep->backp(), گرہ_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            لكل (بنانشانی i = MAX_فاصلہ; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                إذا (predp == prevp) اكسر;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_گرہ_Stmt* ثابت backp = VN_CAST(predp->backp(), گرہ_Stmt);
                إذا (!backp) اكسر;
                // Don't swap statements if doing so would change program semantics
                إذا (!areSwappable(predp, nodep)) اكسر;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            إذا (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlink_Fr_پٹھ();
                predp->جوڑو_اگلا_ایتھے(nodep);
                // If the predecessor is the previous node with the same condition, job done
                إذا (predp == prevp) أرجع;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        لكل (Ast_گرہ_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            إذا (Ast_گرہ_Stmt* succp = VN_CAST(prevp->nextp(), گرہ_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                لكل (بنانشانی i = MAX_فاصلہ; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    إذا (succp == currp) اكسر;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_گرہ_Stmt* ثابت nextp = VN_CAST(succp->nextp(), گرہ_Stmt);
                    إذا (!nextp) اكسر;
                    // Don't swap statements if doing so would change program semantics
                    إذا (!areSwappable(prevp, succp)) اكسر;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                إذا (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlink_Fr_پٹھ();
                    succp->جوڑو_ایتھے_This_As_اگلا(prevp);
                }
            }
        }
    }

    فراغ ویکھو(Ast_گرہ* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    کوڈ_Motion_Optimize_Visitor(Ast_گرہ* nodep, ثابت StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterate_And_اگلا_Null(nodep);
    }

عام:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    ساكن Ast_گرہ* optimize(Ast_گرہ* nodep, ثابت StmtPropertiesAllocator& stmtProperties) {
        { کوڈ_Motion_Optimize_Visitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        طالما (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        أرجع nodep;
    }
};

//######################################################################
// Conditional merging

صنف ملاؤ_Cond_Visitor final : عام VNVisitor {
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
    VDouble0 m_stat_Merged_اشیاء;  // Statistic tracking
    VDouble0 m_stat_Candidate_اشیاء;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_گرہ* m_mgFirstp = خالی_سوچک;  // First node in merged sequence
    Ast_گرہ_اظہاریہ* m_mgCondp = خالی_سوچک;  // The condition of the first node
    ثابت Ast_گرہ* m_mgLastp = خالی_سوچک;  // Last node in merged sequence
    ثابت Ast_گرہ* m_mgNextp = خالی_سوچک;  // Next node in list being examined
    منطقي m_mgCondPubWritable = خطأ;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::قطار<Ast_گرہ*>* m_workQueuep = خالی_سوچک;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = خالی_سوچک;

    // METHODS

    // Function that processes a whole sub-tree
    فراغ process(Ast_گرہ* nodep) {
        // Set up work queue
        std::قطار<Ast_گرہ*> work_قطار;
        m_workQueuep = &work_قطار;
        m_workQueuep->ڈالو(nodep);

        افعل {
            // Set up user* for this iteration
            ثابت VNUser1_وچ_Use user1_وچ_Use;
            ثابت VNUser2_وچ_Use user2_وچ_Use;
            ثابت VNUser3_وچ_Use user3_وچ_Use;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_گرہ* currp = m_workQueuep->front();
            m_workQueuep->نکالو();

            // Analyze sub-tree list for code motion and conditional merging
            کوڈ_Motion_Analysis_Visitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            إذا (v3Global.opt.f_ملاؤ_Cond_Motion()) {
                currp = کوڈ_Motion_Optimize_Visitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterate_And_اگلا_Null(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            إذا (m_mgFirstp) ملاؤ_سرا();
            m_stmtPropertiesp = خالی_سوچک;
        } طالما (!m_workQueuep->خالی_تھاواں());

        m_workQueuep = خالی_سوچک;
    }

    // Skip past AstArraySel and AstWordSel with const index
    ساكن Ast_گرہ* skipConstSels(Ast_گرہ* nodep) {
        طالما (ثابت AstArraySel* ثابت aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            إذا (!VN_IS(aselp->bitp(), Const)) أرجع nodep;
            nodep = aselp->fromp();
        }
        طالما (ثابت Ast_لفظ_Sel* ثابت wselp = VN_CAST(nodep, لفظ_Sel)) {
            // WordSel index is not constant, so might be expensive
            إذا (!VN_IS(wselp->bitp(), Const)) أرجع nodep;
            nodep = wselp->fromp();
        }
        أرجع nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    ساكن منطقي is_Cheap_گرہ(Ast_گرہ* nodep) {
        // Comments are cheap
        إذا (VN_IS(nodep, Comment)) أرجع سچ;
        // So are some assignments
        إذا (ثابت Ast_گرہ_سونپو* ثابت assignp = VN_CAST(nodep, گرہ_سونپو)) {
            // Check LHS
            Ast_گرہ* ثابت lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            إذا (!VN_IS(lhsp, VarRef)) أرجع خطأ;

            // Check RHS
            Ast_گرہ* ثابت rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            إذا (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) أرجع خطأ;

            // Otherwise it is a cheap assignment
            أرجع سچ;
        }
        // Others are not
        أرجع خطأ;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    ساكن منطقي yieldsOneOrZero(ثابت Ast_گرہ* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            أرجع constp->عدد().منزل_UQuad() <= 1;
        }
        إذا (ثابت AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef)) {
            ثابت AstVar* ثابت varp = vrefp->varp();
            أرجع varp->چوڑائی_Min() == 1 && !varp->dtypep()->isSigned();
        }
        إذا (ثابت Ast_کھسکاؤ_R* ثابت shiftp = VN_CAST(nodep, کھسکاؤ_R)) {
            // Shift right by width - 1 or more
            إذا (ثابت AstConst* ثابت constp = VN_CAST(shiftp->rhsp(), Const)) {
                ثابت AstVarRef* ثابت vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                ثابت صحيح چوڑائی = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->چوڑائی_Min()
                                      : shiftp->چوڑائی();
                إذا (constp->منزل_SInt() >= چوڑائی - 1) أرجع سچ;
            }
            أرجع خطأ;
        }
        إذا (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            أرجع سچ;
        }
        إذا (ثابت Ast_گرہ_Biop* ثابت biopp = VN_CAST(nodep, گرہ_Biop)) {
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
    ساكن Ast_گرہ_اظہاریہ* نقاب_Lsb(Ast_گرہ_اظہاریہ* nodep) {
        إذا (yieldsOneOrZero(nodep)) أرجع nodep;
        // Otherwise apply masking
        AstConst* ثابت maskp = جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_True{}};
        // Mask on left, as conventional
        أرجع جديد AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_گرہ_اظہاریہ* foldAndUnlink(Ast_گرہ_اظہاریہ* rhsp, منطقي condTrue) {
        إذا (rhsp->same_درخت(m_mgCondp)) {
            أرجع جديد AstConst{rhsp->fileline(), AstConst::ثنائی_ہندسہ_True{}, condTrue};
        } وإلا إذا (ثابت AstCond* ثابت condp = کڈھو_Cond_From_Rhs(rhsp)) {
            Ast_گرہ_اظہاریہ* ثابت resp
                = condTrue ? condp->thenp()->unlink_Fr_پٹھ() : condp->elsep()->unlink_Fr_پٹھ();
            إذا (condp == rhsp) أرجع resp;
            إذا (ثابت AstAnd* ثابت andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                أرجع جديد AstAnd{andp->fileline(), andp->lhsp()->clone_درخت_خالص(خطأ), resp};
            }
        } وإلا إذا (ثابت AstAnd* ثابت andp = VN_CAST(rhsp, And)) {
            إذا (andp->lhsp()->same_درخت(m_mgCondp)) {
                أرجع condTrue ? نقاب_Lsb(andp->rhsp()->unlink_Fr_پٹھ())
                                : جديد AstConst{rhsp->fileline(), AstConst::ثنائی_ہندسہ_False{}};
            } وإلا {
                UASSERT_OBJ(andp->rhsp()->same_درخت(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                أرجع condTrue ? نقاب_Lsb(andp->lhsp()->unlink_Fr_پٹھ())
                                : جديد AstConst{rhsp->fileline(), AstConst::ثنائی_ہندسہ_False{}};
            }
        } وإلا إذا (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, لفظ_Sel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            أرجع rhsp->clone_درخت(خطأ);
        }
        // LCOV_EXCL_START
        إذا (debug()) rhsp->dump_درخت("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        أرجع خالی_سوچک;
        // LCOV_EXCL_STOP
    }

    فراغ ملاؤ_سرا() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        طالما (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            ثابت Ast_گرہ* ثابت backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        طالما (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            ثابت Ast_گرہ* ثابت nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_گرہ_If* recursivep = خالی_سوچک;
        // Merge if list is longer than one node
        إذا (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            إذا (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_درخت_خالص(خطأ);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* ثابت resultp = جديد AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->جوڑو_ایتھے_This_As_اگلا(resultp);
            // Try to preserve branch prediction if we can
            ناپ_t nLikely = 0;
            ناپ_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_گرہ* nextp = m_mgFirstp;
            افعل {
                // Grab next pointer and unlink
                Ast_گرہ* ثابت currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : خالی_سوچک;
                currp->unlink_Fr_پٹھ();
                // Skip over comments
                إذا (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_درخت(), currp);
                    استمر;
                }
                //گنتی
                ++m_stat_Merged_اشیاء;
                إذا (Ast_گرہ_سونپو* ثابت assignp = VN_CAST(currp, گرہ_سونپو)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_گرہ_اظہاریہ* ثابت rhsp = assignp->rhsp()->unlink_Fr_پٹھ();
                    Ast_گرہ_سونپو* ثابت thenp = assignp;
                    Ast_گرہ_سونپو* ثابت elsep = assignp->clone_درخت(خطأ);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, سچ));
                    elsep->rhsp(foldAndUnlink(rhsp, خطأ));
                    resultp->جوڑو_Thensp(thenp);
                    resultp->جوڑو_Elsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
                } وإلا {
                    Ast_گرہ_If* ثابت ifp = VN_AS(currp, گرہ_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    إذا (Ast_گرہ* ثابت listp = ifp->thensp()) {
                        resultp->جوڑو_Thensp(listp->unlink_Fr_پٹھ_With_اگلا());
                    }
                    إذا (Ast_گرہ* ثابت listp = ifp->elsesp()) {
                        resultp->جوڑو_Elsesp(listp->unlink_Fr_پٹھ_With_اگلا());
                    }
                    // Record branch prediction
                    إذا (ifp->branchPred().likely()) ++nLikely;
                    إذا (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_درخت(), ifp);
                }
            } طالما (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            إذا (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            إذا (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            إذا (resultp->thensp()) m_workQueuep->ڈالو(resultp->thensp());
            إذا (resultp->elsesp()) m_workQueuep->ڈالو(resultp->elsesp());
        } وإلا إذا (Ast_گرہ_If* ثابت ifp = VN_CAST(m_mgFirstp, گرہ_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = خالی_سوچک;
        m_mgCondp = خالی_سوچک;
        m_mgLastp = خالی_سوچک;
        m_mgNextp = خالی_سوچک;
        m_mgCondPubWritable = خطأ;
        Ast_گرہ::user1_Clear_درخت();  // Clear marked variables
        Ast_گرہ::user2_Clear_درخت();
        // Merge recursively within the branches of an un-merged AstNodeIF
        إذا (recursivep) {
            iterate_And_اگلا_Null(recursivep->thensp());
            iterate_And_اگلا_Null(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            إذا (m_mgFirstp) ملاؤ_سرا();
        }
    }

    // Check if the node can be simplified if included under the if
    منطقي is_Simplifiable_گرہ(Ast_گرہ* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        إذا (ثابت Ast_گرہ_سونپو* ثابت assignp = VN_CAST(nodep, گرہ_سونپو)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            إذا (assignp->lhsp()->چوڑائی_Min() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                إذا (ثابت AstAnd* ثابت andp = VN_CAST(assignp->rhsp(), And)) {
                    إذا (andp->lhsp()->same_درخت(m_mgCondp) || andp->rhsp()->same_درخت(m_mgCondp)) {
                        أرجع سچ;
                    }
                }
                // Is it 'lhs = cond'?
                إذا (assignp->rhsp()->same_درخت(m_mgCondp)) أرجع سچ;
            }
        }
        أرجع خطأ;
    }

    منطقي جوڑو_منزل_List(Ast_گرہ_Stmt* nodep, Ast_گرہ_اظہاریہ* condp) {
        // Set up head of new list if node is first in list
        إذا (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](ثابت AstVarRef* nodep) {
                AstVar* ثابت varp = nodep->varp();
                varp->user1(1);
                إذا (varp->is_Sig_عوامی() || varp->isWrittenByDpi()) m_mgCondPubWritable = سچ;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            إذا ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_گرہ::user1_Clear_درخت();
                m_mgCondPubWritable = خطأ;
                // We did not add to the list
                أرجع خطأ;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            طالما (سچ) {
                Ast_گرہ_Stmt* ثابت backp = VN_CAST(m_mgFirstp->backp(), گرہ_Stmt);
                إذا (!backp || backp->nextp() != m_mgFirstp) اكسر;  // Don't move up the tree
                ثابت StmtProperties& props = (*m_stmtPropertiesp)(backp);
                إذا (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) اكسر;
                إذا (is_Simplifiable_گرہ(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } وإلا إذا (is_Cheap_گرہ(backp)) {
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
        إذا (!m_mgNextp) ملاؤ_سرا();
        // Statistics
        ++m_stat_Candidate_اشیاء;
        // We did add to the list
        أرجع سچ;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    منطقي جوڑو_If_Helpful_Else_سرا_ملاؤ(Ast_گرہ_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        إذا (!checkOrMakeMergeable(nodep)) أرجع خطأ;
        إذا (!m_mgFirstp) أرجع خطأ;  // If 'checkOrMakeMergeable' closed the list
        إذا (m_mgNextp == nodep) {
            إذا (is_Simplifiable_گرہ(nodep)) {
                إذا (جوڑو_منزل_List(nodep, خالی_سوچک)) أرجع سچ;
            } وإلا إذا (is_Cheap_گرہ(nodep)) {
                nodep->user2(1);
                إذا (جوڑو_منزل_List(nodep, خالی_سوچک)) أرجع سچ;
            }
        }
        // Not added to list, so we are done with the current list
        ملاؤ_سرا();
        أرجع خطأ;
    }

    منطقي checkOrMakeMergeable(ثابت Ast_گرہ_Stmt* nodep) {
        ثابت StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        إذا (props.m_isFence) أرجع خطأ;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        إذا (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) ملاؤ_سرا();
        أرجع سچ;  // Now surely mergeable
    }

    فراغ ملاؤ_سرا_If_Incompatible(ثابت Ast_گرہ* nodep, ثابت Ast_گرہ* condp) {
        إذا (m_mgFirstp && (m_mgNextp != nodep || !condp->same_درخت(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            ملاؤ_سرا();
        }
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        إذا (Ast_گرہ_اظہاریہ* ثابت condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            إذا (!checkOrMakeMergeable(nodep)) أرجع;
            // Close potentially incompatible pending merge
            ملاؤ_سرا_If_Incompatible(nodep, condp);
            // Add current node
            جوڑو_منزل_List(nodep, condp);
        } وإلا إذا (m_mgFirstp) {
            جوڑو_If_Helpful_Else_سرا_ملاؤ(nodep);
        }
    }

    فراغ ویکھو(Ast_گرہ_If* nodep) override {
        // Check if mergeable
        إذا (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterate_And_اگلا_Null(nodep->thensp());
            iterate_And_اگلا_Null(nodep->elsesp());
            أرجع;
        }
        // Close potentially incompatible pending merge
        ملاؤ_سرا_If_Incompatible(nodep, nodep->condp());
        // Add current node
        جوڑو_منزل_List(nodep, nodep->condp());
    }

    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {
        إذا (m_mgFirstp && جوڑو_If_Helpful_Else_سرا_ملاؤ(nodep)) أرجع;
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstCFunc* nodep) override {
        // Merge function body
        إذا (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    فراغ ویکھو(AstNetlist* nodep) override { iterate_And_اگلا_Null(nodep->modulesp()); }
    فراغ ویکھو(Ast_گرہ_Module* nodep) override { iterate_And_اگلا_Null(nodep->stmtsp()); }
    فراغ ویکھو(Ast_گرہ* nodep) override {}

عام:
    // CONSTRUCTORS
    واضح ملاؤ_Cond_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ملاؤ_Cond_Visitor() override {
        V3Stats::جوڑو_Stat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::جوڑو_Stat("Optimizations, MergeCond candidate items", m_stat_Candidate_اشیاء);
        V3Stats::جوڑو_Stat("Optimizations, MergeCond merged items", m_stat_Merged_اشیاء);
        V3Stats::جوڑو_Stat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

فراغ V3_ملاؤ_Cond::ملاؤ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ملاؤ_Cond_Visitor{nodep}; }
    V3Global::dump_Check_Global_درخت("merge_cond", 0, dump_درخت_Either_Level() >= 6);
}
