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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3MergeCond.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3DupFinder.h"
#समावेश "V3Hasher.h"
#समावेश "V3Stats.h"

#समावेश <queue>
#समावेश <set>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_गाँठ* rhsp) {
    अगर (AstCond* स्थिर condp = VN_CAST(rhsp, Cond)) {
        वापस condp;
    } अन्यथा अगर (स्थिर AstAnd* स्थिर andp = VN_CAST(rhsp, And)) {
        अगर (AstCond* स्थिर condp = VN_CAST(andp->rhsp(), Cond)) {
            अगर (VN_IS(andp->lhsp(), Const)) वापस condp;
        }
    }
    वापस nullptr;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
bool areDisjoint(स्थिर std::set<स्थिर AstVar*>& a, स्थिर std::set<स्थिर AstVar*>& b) {
    अगर (a.empty() || b.empty()) वापस true;
    स्थिर स्वतः endA = a.end();
    स्थिर स्वतः endB = b.end();
    स्वतः itA = a.begin();
    स्वतः itB = b.begin();
    जबतक (true) {
        अगर (*itA == *itB) वापस false;
        अगर (std::less<स्थिर AstVar*>{}(*itA, *itB)) {
            itA = std::lower_bound(++itA, endA, *itB);
            अगर (itA == endA) वापस true;
        } अन्यथा {
            itB = std::lower_bound(++itB, endB, *itA);
            अगर (itB == endB) वापस true;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

काष्ठा StmtProperties final {
    Ast_गाँठ_व्यंजक* m_condp = nullptr;  // The condition expression, if a conditional node
    std::set<स्थिर AstVar*> m_rdVars;  // Variables read by this statement
    std::set<स्थिर AstVar*> m_wrVars;  // Variables written by this statement
    bool m_isFence = false;  // Nothing should move across this statement, nor should it be merged
    bool m_sideEffect = false;  // Statement may have side effect, without access to model state
    bool m_implPubRd = false;  // Statement may implicitly read public state (without VarRef)
    bool m_implPubWr = false;  // Statement may implicitly write public state (without VarRef)
    bool m_explPubRef = false;  // Statement explicitly references public state via VarRef
    Ast_गाँठ_Stmt* m_पूर्व_With_Same_Condp = nullptr;  // Previous node in same list, with same condition
    bool writesConditionVar(bool condPubWritable) स्थिर {
        // This relies on MarkVarsVisitor having been called on the condition node
        क्रम (स्थिर AstVar* स्थिर varp : m_wrVars) {
            अगर (varp->user1()) वापस true;
        }
        // If the condition contains a public variable, check if it might be written
        अगर (condPubWritable && m_implPubWr) वापस true;
        // Otherwise not written
        वापस false;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
using StmtPropertiesAllocator = AstUser3Allocator<Ast_गाँठ_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
श्रेणी CodeMotionAnalysisVisitor final : खुला VNVisitorConst {
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
    std::vector<V3DupFinder> m_ढेर;
    StmtProperties* m_propsp = nullptr;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    अटल Ast_गाँठ_व्यंजक* extractCondition(स्थिर Ast_गाँठ_Stmt* nodep) {
        Ast_गाँठ_व्यंजक* conditionp = nullptr;
        अगर (स्थिर Ast_गाँठ_Assign* स्थिर assignp = VN_CAST(nodep, गाँठ_Assign)) {
            अगर (AstCond* स्थिर conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } अन्यथा अगर (स्थिर Ast_गाँठ_If* स्थिर ifp = VN_CAST(nodep, गाँठ_If)) {
            conditionp = ifp->condp();
        }
        जबतक (AstCCast* स्थिर castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        वापस conditionp;
    }

    व्योम analyzeStmt(Ast_गाँठ_Stmt* nodep, bool tryCondMatch) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* स्थिर outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        अगर (Ast_गाँठ_व्यंजक* स्थिर condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            अगर (tryCondMatch) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_ढेर.back();
                // Find a duplicate condition
                स्थिर V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                अगर (dit == dupFinder.end()) {
                    // First time seeing this condition in the current list
                    dupFinder.जोड़ो(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } अन्यथा {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_गाँठ_व्यंजक* स्थिर firstp = VN_AS(dit->second, गाँठ_व्यंजक);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_पूर्व_With_Same_Condp = static_cast<Ast_गाँठ_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_गाँठ(nodep);

        // If there is an enclosing statement, propagate properties upwards
        अगर (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.जोड़ो(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.जोड़ो(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    व्योम analyzeVarRef(AstVarRef* nodep) {
        स्थिर VAccess access = nodep->access();
        AstVar* स्थिर varp = nodep->varp();
        // Gather read and written variables
        अगर (access.isReadOrRW()) m_propsp->m_rdVars.जोड़ो(varp);
        अगर (access.isWriteOrRW()) m_propsp->m_wrVars.जोड़ो(varp);
        अगर (varp->isSigPublic() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = true;
        }
    }

    व्योम checkProperties(Ast_गाँठ* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        अगर (VN_IS(nodep, Stmt_व्यंजक)) वापस;

        // Call to a DPI import
        अगर (स्थिर Ast_गाँठ_CCall* स्थिर ccallp = VN_CAST(nodep, गाँठ_CCall)) {
            AstCFunc* स्थिर funcp = ccallp->funcp();
            अगर (funcp->dpiImportWrapper()) {
                अगर (!funcp->dpiPure()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = true;
                    m_propsp->m_implPubRd = true;
                    m_propsp->m_implPubWr = true;
                }
                वापस;
            }
        }

        // Side effect, but assume does not access public state
        अगर (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = true;
            वापस;
        }

        // If impure, or branch, mark statement as fence
        अगर (!nodep->isPure() || nodep->isBrancher()) m_propsp->m_isFence = true;
    }

    व्योम analyze_गाँठ(Ast_गाँठ* nodep) {
        // Check properties of this node if under a statement
        अगर (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        bool singletonListStart = false;
        अगर (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == nullptr;
            अगर (!singletonListStart) m_ढेर.emplace_back(m_hasher);
        }

        // Analyze node
        अगर (Ast_गाँठ_Stmt* स्थिर stmtp = VN_CAST(nodep, गाँठ_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } अन्यथा अगर (AstVarRef* स्थिर vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } अन्यथा {
            analyze_गाँठ(nodep);
        }

        // Pop the stack at the end of a list
        अगर (!singletonListStart && !nodep->nextp()) m_ढेर.निकालो_back();
    }

    // CONSTRUCTOR
    CodeMotionAnalysisVisitor(Ast_गाँठ* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterate_And_अग्र_Const_Null(nodep);
    }

खुला:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    अटल व्योम analyze(Ast_गाँठ* nodep, StmtPropertiesAllocator& stmtProperties) {
        CodeMotionAnalysisVisitor{nodep, stmtProperties};
    }
};

श्रेणी CodeMotionOptimizeVisitor final : खुला VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    अटल constexpr अचिन्हित MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4InUse m_user4InUse;

    स्थिर StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    bool areSwappable(स्थिर Ast_गाँठ_Stmt* ap, स्थिर Ast_गाँठ_Stmt* bp) स्थिर {
        स्थिर StmtProperties& aProps = m_stmtProperties(ap);
        स्थिर StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        अगर (aProps.m_isFence) वापस false;
        अगर (bProps.m_isFence) वापस false;
        // Don't swap side effecting statements, but can move others across them
        अगर (aProps.m_sideEffect && bProps.m_sideEffect) वापस false;
        // Don't swap if there is a hazard around public state - must observe ordering
        स्थिर bool bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        अगर (aProps.m_implPubWr && bPubRef) वापस false;
        स्थिर bool aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        अगर (aPubRef && bProps.m_implPubWr) वापस false;
        // If either statement writes a variable that the other reads, they are not swappable
        अगर (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) वापस false;
        अगर (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) वापस false;
        // If they both write to the same variable, they are not swappable
        अगर (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) वापस false;
        // Otherwise good to go
        वापस true;
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Stmt* nodep) override {
        // Process only on first encounter
        अगर (nodep->user4SetOnce()) वापस;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_गाँठ_Stmt* prevp = m_stmtProperties(nodep).m_पूर्व_With_Same_Condp;
        // If no previous node with same condition, we are done
        अगर (!prevp) वापस;
#अगर_घोषित VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            स्थिर Ast_गाँठ* currp = prevp;
            जबतक (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#पूर्ण_अगर
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        अगर (Ast_गाँठ_Stmt* predp = VN_CAST(nodep->backp(), गाँठ_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            क्रम (अचिन्हित i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                अगर (predp == prevp) अवरोध;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_गाँठ_Stmt* स्थिर backp = VN_CAST(predp->backp(), गाँठ_Stmt);
                अगर (!backp) अवरोध;
                // Don't swap statements if doing so would change program semantics
                अगर (!areSwappable(predp, nodep)) अवरोध;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            अगर (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlinkFrBack();
                predp->add_अग्र_Here(nodep);
                // If the predecessor is the previous node with the same condition, job done
                अगर (predp == prevp) वापस;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        क्रम (Ast_गाँठ_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_पूर्व_With_Same_Condp) {
            // Move prevp (previous statement with same condition) towards currp
            अगर (Ast_गाँठ_Stmt* succp = VN_CAST(prevp->nextp(), गाँठ_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                क्रम (अचिन्हित i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    अगर (succp == currp) अवरोध;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_गाँठ_Stmt* स्थिर nextp = VN_CAST(succp->nextp(), गाँठ_Stmt);
                    अगर (!nextp) अवरोध;
                    // Don't swap statements if doing so would change program semantics
                    अगर (!areSwappable(prevp, succp)) अवरोध;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                अगर (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlinkFrBack();
                    succp->add_Here_This_As_अग्र(prevp);
                }
            }
        }
    }

    व्योम visit(Ast_गाँठ* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    CodeMotionOptimizeVisitor(Ast_गाँठ* nodep, स्थिर StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterate_And_अग्र_Null(nodep);
    }

खुला:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    अटल Ast_गाँठ* optimize(Ast_गाँठ* nodep, स्थिर StmtPropertiesAllocator& stmtProperties) {
        { CodeMotionOptimizeVisitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        जबतक (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        वापस nodep;
    }
};

//######################################################################
// Conditional merging

श्रेणी MergeCondVisitor final : खुला VNVisitor {
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
    VDouble0 m_stat_Merged_वस्तुएँ;  // Statistic tracking
    VDouble0 m_stat_Candidate_वस्तुएँ;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_गाँठ* m_mgFirstp = nullptr;  // First node in merged sequence
    Ast_गाँठ_व्यंजक* m_mgCondp = nullptr;  // The condition of the first node
    स्थिर Ast_गाँठ* m_mgLastp = nullptr;  // Last node in merged sequence
    स्थिर Ast_गाँठ* m_mgNextp = nullptr;  // Next node in list being examined
    bool m_mgCondPubWritable = false;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_गाँठ*>* m_workQueuep = nullptr;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = nullptr;

    // METHODS

    // Function that processes a whole sub-tree
    व्योम process(Ast_गाँठ* nodep) {
        // Set up work queue
        std::queue<Ast_गाँठ*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->धकेलो(nodep);

        करो {
            // Set up user* for this iteration
            स्थिर VNUser1InUse user1InUse;
            स्थिर VNUser2InUse user2InUse;
            स्थिर VNUser3InUse user3InUse;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_गाँठ* currp = m_workQueuep->front();
            m_workQueuep->निकालो();

            // Analyze sub-tree list for code motion and conditional merging
            CodeMotionAnalysisVisitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            अगर (v3Global.opt.fMergeCondMotion()) {
                currp = CodeMotionOptimizeVisitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterate_And_अग्र_Null(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            अगर (m_mgFirstp) mergeEnd();
            m_stmtPropertiesp = nullptr;
        } जबतक (!m_workQueuep->empty());

        m_workQueuep = nullptr;
    }

    // Skip past AstArraySel and AstWordSel with const index
    अटल Ast_गाँठ* skipConstSels(Ast_गाँठ* nodep) {
        जबतक (स्थिर AstArraySel* स्थिर aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            अगर (!VN_IS(aselp->bitp(), Const)) वापस nodep;
            nodep = aselp->fromp();
        }
        जबतक (स्थिर AstWordSel* स्थिर wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            अगर (!VN_IS(wselp->bitp(), Const)) वापस nodep;
            nodep = wselp->fromp();
        }
        वापस nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    अटल bool is_Cheap_गाँठ(Ast_गाँठ* nodep) {
        // Comments are cheap
        अगर (VN_IS(nodep, Comment)) वापस true;
        // So are some assignments
        अगर (स्थिर Ast_गाँठ_Assign* स्थिर assignp = VN_CAST(nodep, गाँठ_Assign)) {
            // Check LHS
            Ast_गाँठ* स्थिर lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            अगर (!VN_IS(lhsp, VarRef)) वापस false;

            // Check RHS
            Ast_गाँठ* स्थिर rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            अगर (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) वापस false;

            // Otherwise it is a cheap assignment
            वापस true;
        }
        // Others are not
        वापस false;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    अटल bool yieldsOneOrZero(स्थिर Ast_गाँठ* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep, Const)) {
            वापस constp->अंक().toUQuad() <= 1;
        }
        अगर (स्थिर AstVarRef* स्थिर vrefp = VN_CAST(nodep, VarRef)) {
            स्थिर AstVar* स्थिर varp = vrefp->varp();
            वापस varp->widthMin() == 1 && !varp->dtypep()->isSigned();
        }
        अगर (स्थिर AstShiftR* स्थिर shiftp = VN_CAST(nodep, ShiftR)) {
            // Shift right by width - 1 or more
            अगर (स्थिर AstConst* स्थिर constp = VN_CAST(shiftp->rhsp(), Const)) {
                स्थिर AstVarRef* स्थिर vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                स्थिर पूर्णांक width = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->widthMin()
                                      : shiftp->width();
                अगर (constp->toSInt() >= width - 1) वापस true;
            }
            वापस false;
        }
        अगर (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            वापस true;
        }
        अगर (स्थिर Ast_गाँठ_Biop* स्थिर biopp = VN_CAST(nodep, गाँठ_Biop)) {
            अगर (VN_IS(nodep, And))
                वापस yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            अगर (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                वापस yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            वापस false;
        }
        अगर (स्थिर AstCond* स्थिर condp = VN_CAST(nodep, Cond)) {
            वापस yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        अगर (स्थिर AstCCast* स्थिर castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            वापस yieldsOneOrZero(castp->lhsp());
        }
        वापस false;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    अटल Ast_गाँठ_व्यंजक* maskLsb(Ast_गाँठ_व्यंजक* nodep) {
        अगर (yieldsOneOrZero(nodep)) वापस nodep;
        // Otherwise apply masking
        AstConst* स्थिर maskp = नया AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        वापस नया AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_गाँठ_व्यंजक* foldAndUnlink(Ast_गाँठ_व्यंजक* rhsp, bool condTrue) {
        अगर (rhsp->same_वृक्ष(m_mgCondp)) {
            वापस नया AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } अन्यथा अगर (स्थिर AstCond* स्थिर condp = extractCondFromRhs(rhsp)) {
            Ast_गाँठ_व्यंजक* स्थिर resp
                = condTrue ? condp->thenp()->unlinkFrBack() : condp->elsep()->unlinkFrBack();
            अगर (condp == rhsp) वापस resp;
            अगर (स्थिर AstAnd* स्थिर andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                वापस नया AstAnd{andp->fileline(), andp->lhsp()->clone_वृक्ष_Pure(false), resp};
            }
        } अन्यथा अगर (स्थिर AstAnd* स्थिर andp = VN_CAST(rhsp, And)) {
            अगर (andp->lhsp()->same_वृक्ष(m_mgCondp)) {
                वापस condTrue ? maskLsb(andp->rhsp()->unlinkFrBack())
                                : नया AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } अन्यथा {
                UASSERT_OBJ(andp->rhsp()->same_वृक्ष(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                वापस condTrue ? maskLsb(andp->lhsp()->unlinkFrBack())
                                : नया AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } अन्यथा अगर (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            वापस rhsp->clone_वृक्ष(false);
        }
        // LCOV_EXCL_START
        अगर (debug()) rhsp->dump_वृक्ष("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        वापस nullptr;
        // LCOV_EXCL_STOP
    }

    व्योम mergeEnd() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        जबतक (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            स्थिर Ast_गाँठ* स्थिर backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        जबतक (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            स्थिर Ast_गाँठ* स्थिर nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_गाँठ_If* recursivep = nullptr;
        // Merge if list is longer than one node
        अगर (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            अगर (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_वृक्ष_Pure(false);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* स्थिर resultp = नया AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->add_Here_This_As_अग्र(resultp);
            // Try to preserve branch prediction if we can
            माप_प्रकार nLikely = 0;
            माप_प्रकार nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_गाँठ* nextp = m_mgFirstp;
            करो {
                // Grab next pointer and unlink
                Ast_गाँठ* स्थिर currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : nullptr;
                currp->unlinkFrBack();
                // Skip over comments
                अगर (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_वृक्ष(), currp);
                    जारी;
                }
                //गिनती
                ++m_stat_Merged_वस्तुएँ;
                अगर (Ast_गाँठ_Assign* स्थिर assignp = VN_CAST(currp, गाँठ_Assign)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_गाँठ_व्यंजक* स्थिर rhsp = assignp->rhsp()->unlinkFrBack();
                    Ast_गाँठ_Assign* स्थिर thenp = assignp;
                    Ast_गाँठ_Assign* स्थिर elsep = assignp->clone_वृक्ष(false);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, true));
                    elsep->rhsp(foldAndUnlink(rhsp, false));
                    resultp->addThensp(thenp);
                    resultp->addElsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_वृक्ष(), rhsp);
                } अन्यथा {
                    Ast_गाँठ_If* स्थिर ifp = VN_AS(currp, गाँठ_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    अगर (Ast_गाँठ* स्थिर listp = ifp->thensp()) {
                        resultp->addThensp(listp->unlink_Fr_Back_With_अग्र());
                    }
                    अगर (Ast_गाँठ* स्थिर listp = ifp->elsesp()) {
                        resultp->addElsesp(listp->unlink_Fr_Back_With_अग्र());
                    }
                    // Record branch prediction
                    अगर (ifp->branchPred().likely()) ++nLikely;
                    अगर (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_वृक्ष(), ifp);
                }
            } जबतक (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            अगर (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            अगर (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            अगर (resultp->thensp()) m_workQueuep->धकेलो(resultp->thensp());
            अगर (resultp->elsesp()) m_workQueuep->धकेलो(resultp->elsesp());
        } अन्यथा अगर (Ast_गाँठ_If* स्थिर ifp = VN_CAST(m_mgFirstp, गाँठ_If)) {
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
        Ast_गाँठ::user1_Clear_वृक्ष();  // Clear marked variables
        Ast_गाँठ::user2_Clear_वृक्ष();
        // Merge recursively within the branches of an un-merged AstNodeIF
        अगर (recursivep) {
            iterate_And_अग्र_Null(recursivep->thensp());
            iterate_And_अग्र_Null(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            अगर (m_mgFirstp) mergeEnd();
        }
    }

    // Check if the node can be simplified if included under the if
    bool is_Simplifiable_गाँठ(Ast_गाँठ* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        अगर (स्थिर Ast_गाँठ_Assign* स्थिर assignp = VN_CAST(nodep, गाँठ_Assign)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            अगर (assignp->lhsp()->widthMin() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                अगर (स्थिर AstAnd* स्थिर andp = VN_CAST(assignp->rhsp(), And)) {
                    अगर (andp->lhsp()->same_वृक्ष(m_mgCondp) || andp->rhsp()->same_वृक्ष(m_mgCondp)) {
                        वापस true;
                    }
                }
                // Is it 'lhs = cond'?
                अगर (assignp->rhsp()->same_वृक्ष(m_mgCondp)) वापस true;
            }
        }
        वापस false;
    }

    bool addToList(Ast_गाँठ_Stmt* nodep, Ast_गाँठ_व्यंजक* condp) {
        // Set up head of new list if node is first in list
        अगर (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](स्थिर AstVarRef* nodep) {
                AstVar* स्थिर varp = nodep->varp();
                varp->user1(1);
                अगर (varp->isSigPublic() || varp->isWrittenByDpi()) m_mgCondPubWritable = true;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            अगर ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_गाँठ::user1_Clear_वृक्ष();
                m_mgCondPubWritable = false;
                // We did not add to the list
                वापस false;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            जबतक (true) {
                Ast_गाँठ_Stmt* स्थिर backp = VN_CAST(m_mgFirstp->backp(), गाँठ_Stmt);
                अगर (!backp || backp->nextp() != m_mgFirstp) अवरोध;  // Don't move up the tree
                स्थिर StmtProperties& props = (*m_stmtPropertiesp)(backp);
                अगर (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) अवरोध;
                अगर (is_Simplifiable_गाँठ(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } अन्यथा अगर (is_Cheap_गाँठ(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } अन्यथा {
                    अवरोध;
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
        अगर (!m_mgNextp) mergeEnd();
        // Statistics
        ++m_stat_Candidate_वस्तुएँ;
        // We did add to the list
        वापस true;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    bool addIfHelpfulElseEndMerge(Ast_गाँठ_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        अगर (!checkOrMakeMergeable(nodep)) वापस false;
        अगर (!m_mgFirstp) वापस false;  // If 'checkOrMakeMergeable' closed the list
        अगर (m_mgNextp == nodep) {
            अगर (is_Simplifiable_गाँठ(nodep)) {
                अगर (addToList(nodep, nullptr)) वापस true;
            } अन्यथा अगर (is_Cheap_गाँठ(nodep)) {
                nodep->user2(1);
                अगर (addToList(nodep, nullptr)) वापस true;
            }
        }
        // Not added to list, so we are done with the current list
        mergeEnd();
        वापस false;
    }

    bool checkOrMakeMergeable(स्थिर Ast_गाँठ_Stmt* nodep) {
        स्थिर StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        अगर (props.m_isFence) वापस false;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        अगर (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) mergeEnd();
        वापस true;  // Now surely mergeable
    }

    व्योम mergeEndIfIncompatible(स्थिर Ast_गाँठ* nodep, स्थिर Ast_गाँठ* condp) {
        अगर (m_mgFirstp && (m_mgNextp != nodep || !condp->same_वृक्ष(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            mergeEnd();
        }
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        अगर (Ast_गाँठ_व्यंजक* स्थिर condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            अगर (!checkOrMakeMergeable(nodep)) वापस;
            // Close potentially incompatible pending merge
            mergeEndIfIncompatible(nodep, condp);
            // Add current node
            addToList(nodep, condp);
        } अन्यथा अगर (m_mgFirstp) {
            addIfHelpfulElseEndMerge(nodep);
        }
    }

    व्योम visit(Ast_गाँठ_If* nodep) override {
        // Check if mergeable
        अगर (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterate_And_अग्र_Null(nodep->thensp());
            iterate_And_अग्र_Null(nodep->elsesp());
            वापस;
        }
        // Close potentially incompatible pending merge
        mergeEndIfIncompatible(nodep, nodep->condp());
        // Add current node
        addToList(nodep, nodep->condp());
    }

    व्योम visit(Ast_गाँठ_Stmt* nodep) override {
        अगर (m_mgFirstp && addIfHelpfulElseEndMerge(nodep)) वापस;
        iterateChildren(nodep);
    }

    व्योम visit(AstCFunc* nodep) override {
        // Merge function body
        अगर (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    व्योम visit(AstNetlist* nodep) override { iterate_And_अग्र_Null(nodep->modulesp()); }
    व्योम visit(Ast_गाँठ_Module* nodep) override { iterate_And_अग्र_Null(nodep->stmtsp()); }
    व्योम visit(Ast_गाँठ* nodep) override {}

खुला:
    // CONSTRUCTORS
    explicit MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::addStat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::addStat("Optimizations, MergeCond candidate items", m_stat_Candidate_वस्तुएँ);
        V3Stats::addStat("Optimizations, MergeCond merged items", m_stat_Merged_वस्तुएँ);
        V3Stats::addStat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

व्योम V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("merge_cond", 0, dump_वृक्ष_Either_Level() >= 6);
}
