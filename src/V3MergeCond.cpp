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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3MergeCond.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3DupFinder.h"
#ଅନ୍ତର୍ଭାଵ "V3Hasher.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <queue>
#ଅନ୍ତର୍ଭାଵ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_ଗଣ୍ଠି* rhsp) {
    ଯଦି (AstCond* ଅଚଳ condp = VN_CAST(rhsp, Cond)) {
        ଫେରନ୍ତୁ condp;
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstAnd* ଅଚଳ andp = VN_CAST(rhsp, And)) {
        ଯଦି (AstCond* ଅଚଳ condp = VN_CAST(andp->rhsp(), Cond)) {
            ଯଦି (VN_IS(andp->lhsp(), Const)) ଫେରନ୍ତୁ condp;
        }
    }
    ଫେରନ୍ତୁ nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
ତାର୍କିକ areDisjoint(ଅଚଳ std::set<ଅଚଳ AstVar*>& a, ଅଚଳ std::set<ଅଚଳ AstVar*>& b) {
    ଯଦି (a.empty() || b.empty()) ଫେରନ୍ତୁ ସତ୍ୟ;
    ଅଚଳ auto endA = a.end();
    ଅଚଳ auto endB = b.end();
    auto itA = a.begin();
    auto itB = b.begin();
    ଯେତେବେଳେ (ସତ୍ୟ) {
        ଯଦି (*itA == *itB) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (std::less<ଅଚଳ AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            ଯଦି (itA == endA) ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା {
            itB = std::lower_bound(++itB, endB, *itA);
            ଯଦି (itB == endB) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

ସଂରଚନା StmtProperties final {
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<ଅଚଳ AstVar*> m_rdVars;  // Variables read by this statement
    std::set<ଅଚଳ AstVar*> m_wrVars;  // Variables written by this statement
    ତାର୍କିକ m_isFence = ମିଥ୍ୟା;  // Nothing should move across this statement, nor should it be merged
    ତାର୍କିକ m_sideEffect = ମିଥ୍ୟା;  // Statement may have side effect, without access to model state
    ତାର୍କିକ m_implPubRd = ମିଥ୍ୟା;  // Statement may implicitly read public state (without VarRef)
    ତାର୍କିକ m_implPubWr = ମିଥ୍ୟା;  // Statement may implicitly write public state (without VarRef)
    ତାର୍କିକ m_explPubRef = ମିଥ୍ୟା;  // Statement explicitly references public state via VarRef
    Ast_ଗଣ୍ଠି_Stmt* m_prevWithSameCondp = nullptr;  // Previous node in same list, with same condition
    ତାର୍କିକ writesConditionVar(ତାର୍କିକ condPubWritable) ଅଚଳ {
        // This relies on MarkVarsVisitor having been called on the condition node
        ପାଇଁ (ଅଚଳ AstVar* ଅଚଳ varp : m_wrVars) {
            ଯଦି (varp->user1()) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        // If the condition contains a public variable, check if it might be written
        ଯଦି (condPubWritable && m_implPubWr) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Otherwise not written
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<Ast_ଗଣ୍ଠି_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
ଶ୍ରେଣୀ CodeMotionAnalysisVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
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
    std::vector<V3DupFinder> m_ଗଦା;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* extractCondition(ଅଚଳ Ast_ଗଣ୍ଠି_Stmt* nodep) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* conditionp = nullptr;
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assignp = VN_CAST(nodep, ଗଣ୍ଠି_Assign)) {
            ଯଦି (AstCond* ଅଚଳ conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_If* ଅଚଳ ifp = VN_CAST(nodep, ଗଣ୍ଠି_If)) {
            conditionp = ifp->condp();
        }
        ଯେତେବେଳେ (AstCCast* ଅଚଳ castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        ଫେରନ୍ତୁ conditionp;
    }

    ଶୂନ୍ୟ analyzeStmt(Ast_ଗଣ୍ଠି_Stmt* nodep, ତାର୍କିକ tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* ଅଚଳ outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            ଯଦି (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_ଗଦା.back();
                // Find a duplicate condition
                ଅଚଳ V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                ଯଦି (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.ଭର୍ତ୍ତି(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } ଅନ୍ୟଥା {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ firstp = VN_AS(dit->second, ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_ଗଣ୍ଠି_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_ଗଣ୍ଠି(nodep);

        // If there is an enclosing statement, propagate properties upwards
        ଯଦି (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.ଭର୍ତ୍ତି(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.ଭର୍ତ୍ତି(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    ଶୂନ୍ୟ analyzeVarRef(AstVarRef* nodep) {
        ଅଚଳ VAccess access = nodep->access();
        AstVar* ଅଚଳ varp = nodep->varp();
        // Gather read and written variables
        ଯଦି (access.isReadOrRW()) m_propsp->m_rdVars.ଭର୍ତ୍ତି(varp);
        ଯଦି (access.isWriteOrRW()) m_propsp->m_wrVars.ଭର୍ତ୍ତି(varp);
        ଯଦି (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = ସତ୍ୟ;
        }
    }

    ଶୂନ୍ୟ checkProperties(Ast_ଗଣ୍ଠି* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        ଯଦି (VN_IS(nodep, Stmt_ପଦସମଷ୍ଟି)) ଫେରନ୍ତୁ;

        // Call to a DPI import
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_CCall* ଅଚଳ ccallp = VN_CAST(nodep, ଗଣ୍ଠି_CCall)) {
            AstCFunc* ଅଚଳ funcp = ccallp->funcp();
            ଯଦି (funcp->dpiImportWrapper()) {
                ଯଦି (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = ସତ୍ୟ;
                    m_propsp->m_implPubRd = ସତ୍ୟ;
                    m_propsp->m_implPubWr = ସତ୍ୟ;
                }
                ଫେରନ୍ତୁ;
            }
        }

        // Side effect, but assume does not access public state
        ଯଦି (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = ସତ୍ୟ;
            ଫେରନ୍ତୁ;
        }

        // If impure, or branch, mark statement as fence
        ଯଦି (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = ସତ୍ୟ;
    }

    ଶୂନ୍ୟ analyze_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି* nodep) {
        // Check properties of this node if under a statement
        ଯଦି (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        ତାର୍କିକ singletonListStart = ମିଥ୍ୟା;
        ଯଦି (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            ଯଦି (!singletonListStart) m_ଗଦା.emplace_back(m_hasher);
        }

        // Analyze node
        ଯଦି (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp = VN_CAST(nodep, ଗଣ୍ଠି_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } ଅନ୍ୟଥା ଯଦି (AstVarRef* ଅଚଳ vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } ଅନ୍ୟଥା {
            analyze_ଗଣ୍ଠି(nodep);
        }

        // Pop the stack at the end of a list
        ଯଦି (!singletonListStart && !nodep->nextp()) m_ଗଦା.କାଢ଼_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(Ast_ଗଣ୍ଠି* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterateAndNextConstNull(nodep);
    }

ସର୍ବସାଧାରଣ:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    ସ୍ଥିର ଶୂନ୍ୟ analyze(Ast_ଗଣ୍ଠି* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

ଶ୍ରେଣୀ CodeMotionOptimizeVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    ସ୍ଥିର constexpr unsigned MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    ଅଚଳ StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    ତାର୍କିକ areSwappable(ଅଚଳ Ast_ଗଣ୍ଠି_Stmt* ap, ଅଚଳ Ast_ଗଣ୍ଠି_Stmt* bp) ଅଚଳ {
        ଅଚଳ StmtProperties& aProps = m_stmtProperties(ap);
        ଅଚଳ StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        ଯଦି (aProps.m_isFence) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (bProps.m_isFence) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Don't swap side effecting statements, but can move others across them
        ଯଦି (aProps.m_sideEffect && bProps.m_sideEffect) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Don't swap if there is a hazard around public state - must observe ordering
        ଅଚଳ ତାର୍କିକ bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        ଯଦି (aProps.m_implPubWr && bPubRef) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ ତାର୍କିକ aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        ଯଦି (aPubRef && bProps.m_implPubWr) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // If either statement writes a variable that the other reads, they are not swappable
        ଯଦି (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // If they both write to the same variable, they are not swappable
        ଯଦି (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Otherwise good to go
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        // Process only on first encounter
        ଯଦି (nodep->user4SetOnce()) ଫେରନ୍ତୁ;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_ଗଣ୍ଠି_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        ଯଦି (!prevp) ଫେରନ୍ତୁ;
#ifdef VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            ଅଚଳ Ast_ଗଣ୍ଠି* currp = prevp;
            ଯେତେବେଳେ (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#endif
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        ଯଦି (Ast_ଗଣ୍ଠି_Stmt* predp = VN_CAST(nodep->backp(), ଗଣ୍ଠି_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            ପାଇଁ (unsigned i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                ଯଦି (predp == prevp) ଭାଙ୍ଗ;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ backp = VN_CAST(predp->backp(), ଗଣ୍ଠି_Stmt);
                ଯଦି (!backp) ଭାଙ୍ଗ;
                // Don't swap statements if doing so would change program semantics
                ଯଦି (!areSwappable(predp, nodep)) ଭାଙ୍ଗ;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            ଯଦି (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->addNextHere(nodep);
                // If the predecessor is the previous node with the same condition, job done
                ଯଦି (predp == prevp) ଫେରନ୍ତୁ;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            ଯଦି (Ast_ଗଣ୍ଠି_Stmt* succp = VN_CAST(prevp->nextp(), ଗଣ୍ଠି_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                ପାଇଁ (unsigned i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    ଯଦି (succp == currp) ଭାଙ୍ଗ;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ nextp = VN_CAST(succp->nextp(), ଗଣ୍ଠି_Stmt);
                    ଯଦି (!nextp) ଭାଙ୍ଗ;
                    // Don't swap statements if doing so would change program semantics
                    ଯଦି (!areSwappable(prevp, succp)) ଭାଙ୍ଗ;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                ଯଦି (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->addHereThisAsNext(prevp);
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterateAndNextNull(nodep);
    }

ସର୍ବସାଧାରଣ:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    ସ୍ଥିର Ast_ଗଣ୍ଠି* optimize(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        ଯେତେବେଳେ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        ଫେରନ୍ତୁ nodep;
    }
};

//######################################################################
// Conditional merging

ଶ୍ରେଣୀ MergeCondVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
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
    VDouble0 m_stat_Merged_ବସ୍ତୁଗୁଡ଼ିକ;  // Statistic tracking
    VDouble0 m_stat_Candidate_ବସ୍ତୁଗୁଡ଼ିକ;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_ଗଣ୍ଠି* m_mgFirstp = nullptr;  // First node in merged sequence
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_mgCondp = nullptr;  // The condition of the first node
    ଅଚଳ Ast_ଗଣ୍ଠି* m_mgLastp = nullptr;  // Last node in merged sequence
    ଅଚଳ Ast_ଗଣ୍ଠି* m_mgNextp = nullptr;  // Next node in list being examined
    ତାର୍କିକ m_mgCondPubWritable = ମିଥ୍ୟା;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_ଗଣ୍ଠି*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    ଶୂନ୍ୟ process(Ast_ଗଣ୍ଠି* nodep) {
        // Set up work queue
        std::queue<Ast_ଗଣ୍ଠି*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->ଠେଲ(nodep);

        କର {
            // Set up user* for this iteration
            ଅଚଳ VNUser1InUse user1InUse;
            ଅଚଳ VNUser2InUse user2InUse;
            ଅଚଳ VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_ଗଣ୍ଠି* currp = m_workQueuep->front();
            m_workQueuep->କାଢ଼();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            ଯଦି (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterateAndNextNull(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            ଯଦି (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } ଯେତେବେଳେ (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    ସ୍ଥିର Ast_ଗଣ୍ଠି* skipConstSels(Ast_ଗଣ୍ଠି* nodep) {
        ଯେତେବେଳେ (ଅଚଳ AstArraySel* ଅଚଳ aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            ଯଦି (!VN_IS(aselp->bitp(), Const)) ଫେରନ୍ତୁ nodep;
            nodep = aselp->fromp();
        }
        ଯେତେବେଳେ (ଅଚଳ AstWordSel* ଅଚଳ wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            ଯଦି (!VN_IS(wselp->bitp(), Const)) ଫେରନ୍ତୁ nodep;
            nodep = wselp->fromp();
        }
        ଫେରନ୍ତୁ nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    ସ୍ଥିର ତାର୍କିକ is_Cheap_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି* nodep) {
        // Comments are cheap
        ଯଦି (VN_IS(nodep, Comment)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // So are some assignments
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assignp = VN_CAST(nodep, ଗଣ୍ଠି_Assign)) {
            // Check LHS
            Ast_ଗଣ୍ଠି* ଅଚଳ lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            ଯଦି (!VN_IS(lhsp, VarRef)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

            // Check RHS
            Ast_ଗଣ୍ଠି* ଅଚଳ rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            ଯଦି (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

            // Otherwise it is a cheap assignment
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        // Others are not
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    ସ୍ଥିର ତାର୍କିକ yieldsOneOrZero(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) {
            ଫେରନ୍ତୁ constp->ଅଙ୍କ().toUQuad() <= 1;
        }
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ vrefp = VN_CAST(nodep, VarRef)) {
            ଅଚଳ AstVar* ଅଚଳ varp = vrefp->varp();
            ଫେରନ୍ତୁ varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        ଯଦି (ଅଚଳ AstShiftR* ଅଚଳ shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(shiftp->rhsp(), Const)) {
                ଅଚଳ AstVarRef* ଅଚଳ vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                ଯଦି (constp->toSInt() >= width - 1) ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ biopp = VN_CAST(nodep, ଗଣ୍ଠି_Biop)) {
            ଯଦି (VN_IS(nodep, And))
                ଫେରନ୍ତୁ yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            ଯଦି (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                ଫେରନ୍ତୁ yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (ଅଚଳ AstCond* ଅଚଳ condp = VN_CAST(nodep, Cond)) {
            ଫେରନ୍ତୁ yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        ଯଦି (ଅଚଳ AstCCast* ଅଚଳ castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            ଫେରନ୍ତୁ yieldsOneOrZero(castp->lhsp());
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* maskLsb(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (yieldsOneOrZero(nodep)) ଫେରନ୍ତୁ nodep;
        // Otherwise apply masking
        AstConst* ଅଚଳ maskp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        ଫେରନ୍ତୁ ନୂତନ AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* foldAndUnlink(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, ତାର୍କିକ condTrue) {
        ଯଦି (rhsp->same_ବୃକ୍ଷ(m_mgCondp)) {
            ଫେରନ୍ତୁ ନୂତନ AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstCond* ଅଚଳ condp = extractCondFromRhs(rhsp)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            ଯଦି (condp == rhsp) ଫେରନ୍ତୁ resp;
            ଯଦି (ଅଚଳ AstAnd* ଅଚଳ andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                ଫେରନ୍ତୁ ନୂତନ AstAnd{andp->fileline(), andp->lhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), resp};
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstAnd* ଅଚଳ andp = VN_CAST(rhsp, And)) {
            ଯଦି (andp->lhsp()->same_ବୃକ୍ଷ(m_mgCondp)) {
                ଫେରନ୍ତୁ condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : ନୂତନ AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(andp->rhsp()->same_ବୃକ୍ଷ(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                ଫେରନ୍ତୁ condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : ନୂତନ AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            ଫେରନ୍ତୁ rhsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        // LCOV_EXCL_START
        ଯଦି (debug()) rhsp->dump_ବୃକ୍ଷ("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        ଫେରନ୍ତୁ nullptr;
        // LCOV_EXCL_STOP
    }

    ଶୂନ୍ୟ mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        ଯେତେବେଳେ (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        ଯେତେବେଳେ (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_ଗଣ୍ଠି_If* recursivep = nullptr;
        // Merge if list is longer than one node
        ଯଦି (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            ଯଦି (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* ଅଚଳ resultp = ନୂତନ AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->addHereThisAsNext(resultp);
            // Try to preserve branch prediction if we can
            size_t nLikely = 0;
            size_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_ଗଣ୍ଠି* nextp = m_mgFirstp;
            କର {
                // Grab next pointer and unlink
                Ast_ଗଣ୍ଠି* ଅଚଳ currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                ଯଦି (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_ବୃକ୍ଷ(), currp);
                    ଜାରି;
                }
                //ଗଣନା
                ++m_stat_Merged_ବସ୍ତୁଗୁଡ଼ିକ;
                ଯଦି (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assignp = VN_CAST(currp, ଗଣ୍ଠି_Assign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = assignp->rhsp()->unlinkFrBack();
                    Ast_ଗଣ୍ଠି_Assign* ଅଚଳ thenp = assignp;
                    Ast_ଗଣ୍ଠି_Assign* ଅଚଳ elsep = assignp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, ସତ୍ୟ));
                    elsep->rhsp(foldAndUnlink(rhsp, ମିଥ୍ୟା));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_ବୃକ୍ଷ(), rhsp);
                } ଅନ୍ୟଥା {
                    Ast_ଗଣ୍ଠି_If* ଅଚଳ ifp = VN_AS(currp, ଗଣ୍ଠି_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlinkFrBackWithNext());
                    }
                    ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlinkFrBackWithNext());
                    }
                    // Record branch prediction
                    ଯଦି (ifp->branchPred().likely()) ++nLikely;
                    ଯଦି (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_ବୃକ୍ଷ(), ifp);
                }
            } ଯେତେବେଳେ (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            ଯଦି (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            ଯଦି (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            ଯଦି (resultp->thensp()) m_workQueuep->ଠେଲ(resultp->thensp());
            ଯଦି (resultp->elsesp()) m_workQueuep->ଠେଲ(resultp->elsesp());
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_If* ଅଚଳ ifp = VN_CAST(m_mgFirstp, ଗଣ୍ଠି_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = nullptr;
        m_mgCondp = nullptr;
        m_mgLastp = nullptr;
        m_mgNextp = nullptr;
        m_mgCondPubWritable = ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();  // Clear marked variables
        Ast_ଗଣ୍ଠି::user2_Clear_ବୃକ୍ଷ();
        // Merge recursively within the branches of an un-merged AstNodeIF
        ଯଦି (recursivep) {
            iterateAndNextNull(recursivep->thensp());
            iterateAndNextNull(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            ଯଦି (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    ତାର୍କିକ is_Simplifiable_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assignp = VN_CAST(nodep, ଗଣ୍ଠି_Assign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            ଯଦି (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                ଯଦି (ଅଚଳ AstAnd* ଅଚଳ andp = VN_CAST(assignp->rhsp(), And)) {
                    ଯଦି (andp->lhsp()->same_ବୃକ୍ଷ(m_mgCondp) || andp->rhsp()->same_ବୃକ୍ଷ(m_mgCondp)) {
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
                // Is it 'lhs = cond'?
                ଯଦି (assignp->rhsp()->same_ବୃକ୍ଷ(m_mgCondp)) ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ତାର୍କିକ addToList(Ast_ଗଣ୍ଠି_Stmt* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        // Set up head of new list if node is first in list
        ଯଦି (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](ଅଚଳ AstVarRef* nodep) {
                AstVar* ଅଚଳ varp = nodep->varp();
                varp->user1(1);
                ଯଦି (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = ସତ୍ୟ;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            ଯଦି ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();
                m_mgCondPubWritable = ମିଥ୍ୟା;
                // We did not add to the list
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            ଯେତେବେଳେ (ସତ୍ୟ) {
                Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ backp = VN_CAST(m_mgFirstp->backp(), ଗଣ୍ଠି_Stmt);
                ଯଦି (!backp || backp->nextp() != m_mgFirstp) ଭାଙ୍ଗ;  // Don't move up the tree
                ଅଚଳ StmtProperties& props = (*m_stmtPropertiesp)(backp);
                ଯଦି (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) ଭାଙ୍ଗ;
                ଯଦି (is_Simplifiable_ଗଣ୍ଠି(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ଅନ୍ୟଥା ଯଦି (is_Cheap_ଗଣ୍ଠି(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } ଅନ୍ୟଥା {
                    ଭାଙ୍ଗ;
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
        ଯଦି (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_stat_Candidate_ବସ୍ତୁଗୁଡ଼ିକ;
        // We did add to the list
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    ତାର୍କିକ addIfHelpfulElseEndMerge(Ast_ଗଣ୍ଠି_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        ଯଦି (!checkOrMakeMergeable(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!m_mgFirstp) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // If 'checkOrMakeMergeable' closed the list
        ଯଦି (m_mgNextp == nodep) {
            ଯଦି (is_Simplifiable_ଗଣ୍ଠି(nodep)) {
                ଯଦି (addToList(nodep, nullptr)) ଫେରନ୍ତୁ ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (is_Cheap_ଗଣ୍ଠି(nodep)) {
                nodep->user2(1);
                ଯଦି (addToList(nodep, nullptr)) ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ତାର୍କିକ checkOrMakeMergeable(ଅଚଳ Ast_ଗଣ୍ଠି_Stmt* nodep) {
        ଅଚଳ StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        ଯଦି (props.m_isFence) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        ଯଦି (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        ଫେରନ୍ତୁ ସତ୍ୟ;  // Now surely mergeable
    }

    ଶୂନ୍ୟ mergeEndIfIncompatible(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ Ast_ଗଣ୍ଠି* condp) {
        ଯଦି (m_mgFirstp && (m_mgNextp != nodep || !condp->same_ବୃକ୍ଷ(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            ଯଦି (!checkOrMakeMergeable(nodep)) ଫେରନ୍ତୁ;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } ଅନ୍ୟଥା ଯଦି (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        // Check if mergeable
        ଯଦି (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterateAndNextNull(nodep->thensp());
            iterateAndNextNull(nodep->elsesp());
            ଫେରନ୍ତୁ;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        ଯଦି (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        // Merge function body
        ଯଦି (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {}

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_stat_Candidate_ବସ୍ତୁଗୁଡ଼ିକ);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_stat_Merged_ବସ୍ତୁଗୁଡ଼ିକ);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

ଶୂନ୍ୟ V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("merge_cond", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}
