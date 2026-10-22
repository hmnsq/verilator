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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3MergeCond.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3DupFinder.h"
#അന്തര്ഭാവ "V3Hasher.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <queue>
#അന്തര്ഭാവ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_കണ്ണി* rhsp) {
    എങ്കിൽ (AstCond* സ്ഥിരം condp = VN_CAST(rhsp, Cond)) {
        തിരികെ condp;
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം andp = VN_CAST(rhsp, And)) {
        എങ്കിൽ (AstCond* സ്ഥിരം condp = VN_CAST(andp->rhsp(), Cond)) {
            എങ്കിൽ (VN_IS(andp->lhsp(), Const)) തിരികെ condp;
        }
    }
    തിരികെ nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
തർക്കം areDisjoint(സ്ഥിരം std::set<സ്ഥിരം AstVar*>& a, സ്ഥിരം std::set<സ്ഥിരം AstVar*>& b) {
    എങ്കിൽ (a.empty() || b.empty()) തിരികെ സത്യം;
    സ്ഥിരം auto endA = a.end();
    സ്ഥിരം auto endB = b.end();
    auto itA = a.begin();
    auto itB = b.begin();
    ആകുമ്പോൾ (സത്യം) {
        എങ്കിൽ (*itA == *itB) തിരികെ അസത്യം;
        എങ്കിൽ (std::less<സ്ഥിരം AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            എങ്കിൽ (itA == endA) തിരികെ സത്യം;
        } അല്ലെങ്കിൽ {
            itB = std::lower_bound(++itB, endB, *itA);
            എങ്കിൽ (itB == endB) തിരികെ സത്യം;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

ഘടന StmtProperties final {
    Ast_കണ്ണി_വ്യഞ്ജകം* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<സ്ഥിരം AstVar*> m_rdVars;  // Variables read by this statement
    std::set<സ്ഥിരം AstVar*> m_wrVars;  // Variables written by this statement
    തർക്കം m_isFence = അസത്യം;  // Nothing should move across this statement, nor should it be merged
    തർക്കം m_sideEffect = അസത്യം;  // Statement may have side effect, without access to model state
    തർക്കം m_implPubRd = അസത്യം;  // Statement may implicitly read public state (without VarRef)
    തർക്കം m_implPubWr = അസത്യം;  // Statement may implicitly write public state (without VarRef)
    തർക്കം m_explPubRef = അസത്യം;  // Statement explicitly references public state via VarRef
    Ast_കണ്ണി_Stmt* m_prevWithSameCondp = nullptr;  // Previous node in same list, with same condition
    തർക്കം writesConditionVar(തർക്കം condPubWritable) സ്ഥിരം {
        // This relies on MarkVarsVisitor having been called on the condition node
        വേണ്ടി (സ്ഥിരം AstVar* സ്ഥിരം varp : m_wrVars) {
            എങ്കിൽ (varp->user1()) തിരികെ സത്യം;
        }
        // If the condition contains a public variable, check if it might be written
        എങ്കിൽ (condPubWritable && m_implPubWr) തിരികെ സത്യം;
        // Otherwise not written
        തിരികെ അസത്യം;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<Ast_കണ്ണി_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
വർഗ്ഗം CodeMotionAnalysisVisitor final : പൊതു VNVisitorConst {
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
    std::vector<V3DupFinder> m_അട്ടി;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* extractCondition(സ്ഥിരം Ast_കണ്ണി_Stmt* nodep) {
        Ast_കണ്ണി_വ്യഞ്ജകം* conditionp = nullptr;
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Assign* സ്ഥിരം assignp = VN_CAST(nodep, കണ്ണി_Assign)) {
            എങ്കിൽ (AstCond* സ്ഥിരം conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_If* സ്ഥിരം ifp = VN_CAST(nodep, കണ്ണി_If)) {
            conditionp = ifp->condp();
        }
        ആകുമ്പോൾ (AstCCast* സ്ഥിരം castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        തിരികെ conditionp;
    }

    ശൂന്യം analyzeStmt(Ast_കണ്ണി_Stmt* nodep, തർക്കം tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* സ്ഥിരം outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            എങ്കിൽ (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_അട്ടി.back();
                // Find a duplicate condition
                സ്ഥിരം V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                എങ്കിൽ (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.തിരുകുക(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } അല്ലെങ്കിൽ {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം firstp = VN_AS(dit->second, കണ്ണി_വ്യഞ്ജകം);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_കണ്ണി_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_കണ്ണി(nodep);

        // If there is an enclosing statement, propagate properties upwards
        എങ്കിൽ (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.തിരുകുക(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.തിരുകുക(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    ശൂന്യം analyzeVarRef(AstVarRef* nodep) {
        സ്ഥിരം VAccess access = nodep->access();
        AstVar* സ്ഥിരം varp = nodep->varp();
        // Gather read and written variables
        എങ്കിൽ (access.isReadOrRW()) m_propsp->m_rdVars.തിരുകുക(varp);
        എങ്കിൽ (access.isWriteOrRW()) m_propsp->m_wrVars.തിരുകുക(varp);
        എങ്കിൽ (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = സത്യം;
        }
    }

    ശൂന്യം checkProperties(Ast_കണ്ണി* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        എങ്കിൽ (VN_IS(nodep, Stmt_വ്യഞ്ജകം)) തിരികെ;

        // Call to a DPI import
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_CCall* സ്ഥിരം ccallp = VN_CAST(nodep, കണ്ണി_CCall)) {
            AstCFunc* സ്ഥിരം funcp = ccallp->funcp();
            എങ്കിൽ (funcp->dpiImportWrapper()) {
                എങ്കിൽ (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = സത്യം;
                    m_propsp->m_implPubRd = സത്യം;
                    m_propsp->m_implPubWr = സത്യം;
                }
                തിരികെ;
            }
        }

        // Side effect, but assume does not access public state
        എങ്കിൽ (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = സത്യം;
            തിരികെ;
        }

        // If impure, or branch, mark statement as fence
        എങ്കിൽ (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = സത്യം;
    }

    ശൂന്യം analyze_കണ്ണി(Ast_കണ്ണി* nodep) {
        // Check properties of this node if under a statement
        എങ്കിൽ (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        തർക്കം singletonListStart = അസത്യം;
        എങ്കിൽ (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            എങ്കിൽ (!singletonListStart) m_അട്ടി.emplace_back(m_hasher);
        }

        // Analyze node
        എങ്കിൽ (Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp = VN_CAST(nodep, കണ്ണി_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVarRef* സ്ഥിരം vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } അല്ലെങ്കിൽ {
            analyze_കണ്ണി(nodep);
        }

        // Pop the stack at the end of a list
        എങ്കിൽ (!singletonListStart && !nodep->nextp()) m_അട്ടി.എടുക്കുക_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(Ast_കണ്ണി* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterateAndNextConstNull(nodep);
    }

പൊതു:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    സ്ഥിതം ശൂന്യം analyze(Ast_കണ്ണി* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

വർഗ്ഗം CodeMotionOptimizeVisitor final : പൊതു VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    സ്ഥിതം constexpr unsigned MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    സ്ഥിരം StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    തർക്കം areSwappable(സ്ഥിരം Ast_കണ്ണി_Stmt* ap, സ്ഥിരം Ast_കണ്ണി_Stmt* bp) സ്ഥിരം {
        സ്ഥിരം StmtProperties& aProps = m_stmtProperties(ap);
        സ്ഥിരം StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        എങ്കിൽ (aProps.m_isFence) തിരികെ അസത്യം;
        എങ്കിൽ (bProps.m_isFence) തിരികെ അസത്യം;
        // Don't swap side effecting statements, but can move others across them
        എങ്കിൽ (aProps.m_sideEffect && bProps.m_sideEffect) തിരികെ അസത്യം;
        // Don't swap if there is a hazard around public state - must observe ordering
        സ്ഥിരം തർക്കം bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        എങ്കിൽ (aProps.m_implPubWr && bPubRef) തിരികെ അസത്യം;
        സ്ഥിരം തർക്കം aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        എങ്കിൽ (aPubRef && bProps.m_implPubWr) തിരികെ അസത്യം;
        // If either statement writes a variable that the other reads, they are not swappable
        എങ്കിൽ (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) തിരികെ അസത്യം;
        എങ്കിൽ (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) തിരികെ അസത്യം;
        // If they both write to the same variable, they are not swappable
        എങ്കിൽ (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) തിരികെ അസത്യം;
        // Otherwise good to go
        തിരികെ സത്യം;
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        // Process only on first encounter
        എങ്കിൽ (nodep->user4SetOnce()) തിരികെ;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_കണ്ണി_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        എങ്കിൽ (!prevp) തിരികെ;
#ifdef VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            സ്ഥിരം Ast_കണ്ണി* currp = prevp;
            ആകുമ്പോൾ (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#endif
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        എങ്കിൽ (Ast_കണ്ണി_Stmt* predp = VN_CAST(nodep->backp(), കണ്ണി_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            വേണ്ടി (unsigned i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                എങ്കിൽ (predp == prevp) നിർത്തുക;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_കണ്ണി_Stmt* സ്ഥിരം backp = VN_CAST(predp->backp(), കണ്ണി_Stmt);
                എങ്കിൽ (!backp) നിർത്തുക;
                // Don't swap statements if doing so would change program semantics
                എങ്കിൽ (!areSwappable(predp, nodep)) നിർത്തുക;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            എങ്കിൽ (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->addNextHere(nodep);
                // If the predecessor is the previous node with the same condition, job done
                എങ്കിൽ (predp == prevp) തിരികെ;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        വേണ്ടി (Ast_കണ്ണി_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            എങ്കിൽ (Ast_കണ്ണി_Stmt* succp = VN_CAST(prevp->nextp(), കണ്ണി_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                വേണ്ടി (unsigned i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    എങ്കിൽ (succp == currp) നിർത്തുക;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_കണ്ണി_Stmt* സ്ഥിരം nextp = VN_CAST(succp->nextp(), കണ്ണി_Stmt);
                    എങ്കിൽ (!nextp) നിർത്തുക;
                    // Don't swap statements if doing so would change program semantics
                    എങ്കിൽ (!areSwappable(prevp, succp)) നിർത്തുക;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                എങ്കിൽ (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->addHereThisAsNext(prevp);
                }
            }
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(Ast_കണ്ണി* nodep, സ്ഥിരം StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterateAndNextNull(nodep);
    }

പൊതു:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    സ്ഥിതം Ast_കണ്ണി* optimize(Ast_കണ്ണി* nodep, സ്ഥിരം StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        ആകുമ്പോൾ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        തിരികെ nodep;
    }
};

//######################################################################
// Conditional merging

വർഗ്ഗം MergeCondVisitor final : പൊതു VNVisitor {
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
    VDouble0 m_stat_Merged_ഇനങ്ങൾ;  // Statistic tracking
    VDouble0 m_stat_Candidate_ഇനങ്ങൾ;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_കണ്ണി* m_mgFirstp = nullptr;  // First node in merged sequence
    Ast_കണ്ണി_വ്യഞ്ജകം* m_mgCondp = nullptr;  // The condition of the first node
    സ്ഥിരം Ast_കണ്ണി* m_mgLastp = nullptr;  // Last node in merged sequence
    സ്ഥിരം Ast_കണ്ണി* m_mgNextp = nullptr;  // Next node in list being examined
    തർക്കം m_mgCondPubWritable = അസത്യം;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_കണ്ണി*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    ശൂന്യം process(Ast_കണ്ണി* nodep) {
        // Set up work queue
        std::queue<Ast_കണ്ണി*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->തള്ളുക(nodep);

        ചെയ്യുക {
            // Set up user* for this iteration
            സ്ഥിരം VNUser1InUse user1InUse;
            സ്ഥിരം VNUser2InUse user2InUse;
            സ്ഥിരം VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_കണ്ണി* currp = m_workQueuep->front();
            m_workQueuep->എടുക്കുക();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            എങ്കിൽ (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterateAndNextNull(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            എങ്കിൽ (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } ആകുമ്പോൾ (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    സ്ഥിതം Ast_കണ്ണി* skipConstSels(Ast_കണ്ണി* nodep) {
        ആകുമ്പോൾ (സ്ഥിരം AstArraySel* സ്ഥിരം aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            എങ്കിൽ (!VN_IS(aselp->bitp(), Const)) തിരികെ nodep;
            nodep = aselp->fromp();
        }
        ആകുമ്പോൾ (സ്ഥിരം AstWordSel* സ്ഥിരം wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            എങ്കിൽ (!VN_IS(wselp->bitp(), Const)) തിരികെ nodep;
            nodep = wselp->fromp();
        }
        തിരികെ nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    സ്ഥിതം തർക്കം is_Cheap_കണ്ണി(Ast_കണ്ണി* nodep) {
        // Comments are cheap
        എങ്കിൽ (VN_IS(nodep, Comment)) തിരികെ സത്യം;
        // So are some assignments
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Assign* സ്ഥിരം assignp = VN_CAST(nodep, കണ്ണി_Assign)) {
            // Check LHS
            Ast_കണ്ണി* സ്ഥിരം lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            എങ്കിൽ (!VN_IS(lhsp, VarRef)) തിരികെ അസത്യം;

            // Check RHS
            Ast_കണ്ണി* സ്ഥിരം rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            എങ്കിൽ (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) തിരികെ അസത്യം;

            // Otherwise it is a cheap assignment
            തിരികെ സത്യം;
        }
        // Others are not
        തിരികെ അസത്യം;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    സ്ഥിതം തർക്കം yieldsOneOrZero(സ്ഥിരം Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) {
            തിരികെ constp->അക്കം().toUQuad() <= 1;
        }
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം vrefp = VN_CAST(nodep, VarRef)) {
            സ്ഥിരം AstVar* സ്ഥിരം varp = vrefp->varp();
            തിരികെ varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        എങ്കിൽ (സ്ഥിരം AstShiftR* സ്ഥിരം shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(shiftp->rhsp(), Const)) {
                സ്ഥിരം AstVarRef* സ്ഥിരം vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                സ്ഥിരം പൂർണ്ണസംഖ്യ width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                എങ്കിൽ (constp->toSInt() >= width - 1) തിരികെ സത്യം;
            }
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            തിരികെ സത്യം;
        }
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം biopp = VN_CAST(nodep, കണ്ണി_Biop)) {
            എങ്കിൽ (VN_IS(nodep, And))
                തിരികെ yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            എങ്കിൽ (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                തിരികെ yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (സ്ഥിരം AstCond* സ്ഥിരം condp = VN_CAST(nodep, Cond)) {
            തിരികെ yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        എങ്കിൽ (സ്ഥിരം AstCCast* സ്ഥിരം castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            തിരികെ yieldsOneOrZero(castp->lhsp());
        }
        തിരികെ അസത്യം;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* maskLsb(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (yieldsOneOrZero(nodep)) തിരികെ nodep;
        // Otherwise apply masking
        AstConst* സ്ഥിരം maskp = പുതിയ AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        തിരികെ പുതിയ AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_കണ്ണി_വ്യഞ്ജകം* foldAndUnlink(Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, തർക്കം condTrue) {
        എങ്കിൽ (rhsp->same_വൃക്ഷം(m_mgCondp)) {
            തിരികെ പുതിയ AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstCond* സ്ഥിരം condp = extractCondFromRhs(rhsp)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            എങ്കിൽ (condp == rhsp) തിരികെ resp;
            എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                തിരികെ പുതിയ AstAnd{andp->fileline(), andp->lhsp()->clone_വൃക്ഷം_Pure(അസത്യം), resp};
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം andp = VN_CAST(rhsp, And)) {
            എങ്കിൽ (andp->lhsp()->same_വൃക്ഷം(m_mgCondp)) {
                തിരികെ condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : പുതിയ AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(andp->rhsp()->same_വൃക്ഷം(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                തിരികെ condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : പുതിയ AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            തിരികെ rhsp->clone_വൃക്ഷം(അസത്യം);
        }
        // LCOV_EXCL_START
        എങ്കിൽ (debug()) rhsp->dump_വൃക്ഷം("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        തിരികെ nullptr;
        // LCOV_EXCL_STOP
    }

    ശൂന്യം mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        ആകുമ്പോൾ (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        ആകുമ്പോൾ (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_കണ്ണി_If* recursivep = nullptr;
        // Merge if list is longer than one node
        എങ്കിൽ (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            എങ്കിൽ (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_വൃക്ഷം_Pure(അസത്യം);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* സ്ഥിരം resultp = പുതിയ AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->addHereThisAsNext(resultp);
            // Try to preserve branch prediction if we can
            size_t nLikely = 0;
            size_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_കണ്ണി* nextp = m_mgFirstp;
            ചെയ്യുക {
                // Grab next pointer and unlink
                Ast_കണ്ണി* സ്ഥിരം currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                എങ്കിൽ (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_വൃക്ഷം(), currp);
                    തുടരുക;
                }
                //എണ്ണം
                ++m_stat_Merged_ഇനങ്ങൾ;
                എങ്കിൽ (Ast_കണ്ണി_Assign* സ്ഥിരം assignp = VN_CAST(currp, കണ്ണി_Assign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = assignp->rhsp()->unlinkFrBack();
                    Ast_കണ്ണി_Assign* സ്ഥിരം thenp = assignp;
                    Ast_കണ്ണി_Assign* സ്ഥിരം elsep = assignp->clone_വൃക്ഷം(അസത്യം);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, സത്യം));
                    elsep->rhsp(foldAndUnlink(rhsp, അസത്യം));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_വൃക്ഷം(), rhsp);
                } അല്ലെങ്കിൽ {
                    Ast_കണ്ണി_If* സ്ഥിരം ifp = VN_AS(currp, കണ്ണി_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlinkFrBackWithNext());
                    }
                    എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlinkFrBackWithNext());
                    }
                    // Record branch prediction
                    എങ്കിൽ (ifp->branchPred().likely()) ++nLikely;
                    എങ്കിൽ (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_വൃക്ഷം(), ifp);
                }
            } ആകുമ്പോൾ (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            എങ്കിൽ (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            എങ്കിൽ (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            എങ്കിൽ (resultp->thensp()) m_workQueuep->തള്ളുക(resultp->thensp());
            എങ്കിൽ (resultp->elsesp()) m_workQueuep->തള്ളുക(resultp->elsesp());
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_If* സ്ഥിരം ifp = VN_CAST(m_mgFirstp, കണ്ണി_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = nullptr;
        m_mgCondp = nullptr;
        m_mgLastp = nullptr;
        m_mgNextp = nullptr;
        m_mgCondPubWritable = അസത്യം;
        Ast_കണ്ണി::user1_Clear_വൃക്ഷം();  // Clear marked variables
        Ast_കണ്ണി::user2_Clear_വൃക്ഷം();
        // Merge recursively within the branches of an un-merged AstNodeIF
        എങ്കിൽ (recursivep) {
            iterateAndNextNull(recursivep->thensp());
            iterateAndNextNull(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            എങ്കിൽ (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    തർക്കം is_Simplifiable_കണ്ണി(Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Assign* സ്ഥിരം assignp = VN_CAST(nodep, കണ്ണി_Assign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            എങ്കിൽ (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം andp = VN_CAST(assignp->rhsp(), And)) {
                    എങ്കിൽ (andp->lhsp()->same_വൃക്ഷം(m_mgCondp) || andp->rhsp()->same_വൃക്ഷം(m_mgCondp)) {
                        തിരികെ സത്യം;
                    }
                }
                // Is it 'lhs = cond'?
                എങ്കിൽ (assignp->rhsp()->same_വൃക്ഷം(m_mgCondp)) തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }

    തർക്കം addToList(Ast_കണ്ണി_Stmt* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        // Set up head of new list if node is first in list
        എങ്കിൽ (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](സ്ഥിരം AstVarRef* nodep) {
                AstVar* സ്ഥിരം varp = nodep->varp();
                varp->user1(1);
                എങ്കിൽ (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = സത്യം;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            എങ്കിൽ ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_കണ്ണി::user1_Clear_വൃക്ഷം();
                m_mgCondPubWritable = അസത്യം;
                // We did not add to the list
                തിരികെ അസത്യം;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            ആകുമ്പോൾ (സത്യം) {
                Ast_കണ്ണി_Stmt* സ്ഥിരം backp = VN_CAST(m_mgFirstp->backp(), കണ്ണി_Stmt);
                എങ്കിൽ (!backp || backp->nextp() != m_mgFirstp) നിർത്തുക;  // Don't move up the tree
                സ്ഥിരം StmtProperties& props = (*m_stmtPropertiesp)(backp);
                എങ്കിൽ (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) നിർത്തുക;
                എങ്കിൽ (is_Simplifiable_കണ്ണി(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } അല്ലെങ്കിൽ എങ്കിൽ (is_Cheap_കണ്ണി(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } അല്ലെങ്കിൽ {
                    നിർത്തുക;
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
        എങ്കിൽ (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_stat_Candidate_ഇനങ്ങൾ;
        // We did add to the list
        തിരികെ സത്യം;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    തർക്കം addIfHelpfulElseEndMerge(Ast_കണ്ണി_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        എങ്കിൽ (!checkOrMakeMergeable(nodep)) തിരികെ അസത്യം;
        എങ്കിൽ (!m_mgFirstp) തിരികെ അസത്യം;  // If 'checkOrMakeMergeable' closed the list
        എങ്കിൽ (m_mgNextp == nodep) {
            എങ്കിൽ (is_Simplifiable_കണ്ണി(nodep)) {
                എങ്കിൽ (addToList(nodep, nullptr)) തിരികെ സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (is_Cheap_കണ്ണി(nodep)) {
                nodep->user2(1);
                എങ്കിൽ (addToList(nodep, nullptr)) തിരികെ സത്യം;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        തിരികെ അസത്യം;
    }

    തർക്കം checkOrMakeMergeable(സ്ഥിരം Ast_കണ്ണി_Stmt* nodep) {
        സ്ഥിരം StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        എങ്കിൽ (props.m_isFence) തിരികെ അസത്യം;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        എങ്കിൽ (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        തിരികെ സത്യം;  // Now surely mergeable
    }

    ശൂന്യം mergeEndIfIncompatible(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം Ast_കണ്ണി* condp) {
        എങ്കിൽ (m_mgFirstp && (m_mgNextp != nodep || !condp->same_വൃക്ഷം(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            എങ്കിൽ (!checkOrMakeMergeable(nodep)) തിരികെ;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } അല്ലെങ്കിൽ എങ്കിൽ (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        // Check if mergeable
        എങ്കിൽ (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterateAndNextNull(nodep->thensp());
            iterateAndNextNull(nodep->elsesp());
            തിരികെ;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        എങ്കിൽ (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) തിരികെ;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCFunc* nodep) override {
        // Merge function body
        എങ്കിൽ (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    ശൂന്യം visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {}

പൊതു:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_stat_Candidate_ഇനങ്ങൾ);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_stat_Merged_ഇനങ്ങൾ);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

ശൂന്യം V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_വൃക്ഷം("merge_cond", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}
