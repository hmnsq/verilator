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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3MergeCond.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3DupFinder.h"
#अन्तर्भाव "V3Hasher.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <queue>
#अन्तर्भाव <set>

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* extractCondFromRhs(Ast_ग्रन्थिः* rhsp) {
    यदि (AstCond* ध्रुव condp = VN_CAST(rhsp, Cond)) {
        प्रत्यागम condp;
    } अन्यथा यदि (ध्रुव AstAnd* ध्रुव andp = VN_CAST(rhsp, And)) {
        यदि (AstCond* ध्रुव condp = VN_CAST(andp->rhsp(), Cond)) {
            यदि (VN_IS(andp->lhsp(), Const)) प्रत्यागम condp;
        }
    }
    प्रत्यागम शून्यसूचकः;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
तार्किक areDisjoint(ध्रुव std::समुच्चयः<ध्रुव AstVar*>& a, ध्रुव std::समुच्चयः<ध्रुव AstVar*>& b) {
    यदि (a.रिक्तानि() || b.रिक्तानि()) प्रत्यागम सत्य;
    ध्रुव स्वचालित अन्तः_A = a.अन्तः_2();
    ध्रुव स्वचालित अन्तः_B = b.अन्तः_2();
    स्वचालित itA = a.begin();
    स्वचालित itB = b.begin();
    यावत् (सत्य) {
        यदि (*itA == *itB) प्रत्यागम असत्य;
        यदि (std::less<ध्रुव AstVar*>{}(*itA, *itB)) {
            itA = std::अधस्तनम्_bound(++itA, अन्तः_A, *itB);
            यदि (itA == अन्तः_A) प्रत्यागम सत्य;
        } अन्यथा {
            itB = std::अधस्तनम्_bound(++itB, अन्तः_B, *itA);
            यदि (itB == अन्तः_B) प्रत्यागम सत्य;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

संरचना StmtProperties final {
    Ast_ग्रन्थिः_व्यञ्जकम्* m_condp = शून्यसूचकः;  // The condition expression, if a conditional node
    std::समुच्चयः<ध्रुव AstVar*> m_rdVars;  // Variables read by this statement
    std::समुच्चयः<ध्रुव AstVar*> m_wrVars;  // Variables written by this statement
    तार्किक m_isFence = असत्य;  // Nothing should move across this statement, nor should it be merged
    तार्किक m_sideEffect = असत्य;  // Statement may have side effect, without access to model state
    तार्किक m_implPubRd = असत्य;  // Statement may implicitly read public state (without VarRef)
    तार्किक m_implPubWr = असत्य;  // Statement may implicitly write public state (without VarRef)
    तार्किक m_explPubRef = असत्य;  // Statement explicitly references public state via VarRef
    Ast_ग्रन्थिः_Stmt* m_पूर्व_With_Same_Condp = शून्यसूचकः;  // Previous node in same list, with same condition
    तार्किक writesConditionVar(तार्किक condPubWritable) ध्रुव {
        // This relies on MarkVarsVisitor having been called on the condition node
        प्रति (ध्रुव AstVar* ध्रुव varp : m_wrVars) {
            यदि (varp->user1()) प्रत्यागम सत्य;
        }
        // If the condition contains a public variable, check if it might be written
        यदि (condPubWritable && m_implPubWr) प्रत्यागम सत्य;
        // Otherwise not written
        प्रत्यागम असत्य;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
उपयुज्य StmtPropertiesAllocator = AstUser3Allocator<Ast_ग्रन्थिः_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
वर्ग संहिता_Motion_Analysis_Visitor final : सार्वजनिक VNVisitorConst {
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
    std::सदिशम्<V3DupFinder> m_राशिः;
    StmtProperties* m_propsp = शून्यसूचकः;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* extractCondition(ध्रुव Ast_ग्रन्थिः_Stmt* nodep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* conditionp = शून्यसूचकः;
        यदि (ध्रुव Ast_ग्रन्थिः_नियोजय* ध्रुव assignp = VN_CAST(nodep, ग्रन्थिः_नियोजय)) {
            यदि (AstCond* ध्रुव conditionalp = extractCondFromRhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_If* ध्रुव ifp = VN_CAST(nodep, ग्रन्थिः_If)) {
            conditionp = ifp->condp();
        }
        यावत् (AstCCast* ध्रुव castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        प्रत्यागम conditionp;
    }

    शून्य analyzeStmt(Ast_ग्रन्थिः_Stmt* nodep, तार्किक try_Cond_मेलनम्) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* ध्रुव outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = extractCondition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            यदि (try_Cond_मेलनम्) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_राशिः.पृष्ठभागः();
                // Find a duplicate condition
                ध्रुव V3DupFinder::iterator& dit = dupFinder.findDuplicate(condp);
                यदि (dit == dupFinder.अन्तः_2()) {
                    // First time seeing this condition in the current list
                    dupFinder.निवेशय(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } अन्यथा {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firstp = VN_AS(dit->second, ग्रन्थिः_व्यञ्जकम्);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_पूर्व_With_Same_Condp = static_cast<Ast_ग्रन्थिः_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_ग्रन्थिः(nodep);

        // If there is an enclosing statement, propagate properties upwards
        यदि (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.निवेशय(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.निवेशय(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    शून्य analyzeVarRef(AstVarRef* nodep) {
        ध्रुव VAccess access = nodep->access();
        AstVar* ध्रुव varp = nodep->varp();
        // Gather read and written variables
        यदि (access.isReadOrRW()) m_propsp->m_rdVars.निवेशय(varp);
        यदि (access.isWriteOrRW()) m_propsp->m_wrVars.निवेशय(varp);
        यदि (varp->is_Sig_लौकिकम्() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = सत्य;
        }
    }

    शून्य checkProperties(Ast_ग्रन्थिः* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        यदि (VN_IS(nodep, Stmt_व्यञ्जकम्)) प्रत्यागम;

        // Call to a DPI import
        यदि (ध्रुव Ast_ग्रन्थिः_CCall* ध्रुव ccallp = VN_CAST(nodep, ग्रन्थिः_CCall)) {
            AstCFunc* ध्रुव funcp = ccallp->funcp();
            यदि (funcp->dpiImportWrapper()) {
                यदि (!funcp->dpi_विशुद्धम्()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = सत्य;
                    m_propsp->m_implPubRd = सत्य;
                    m_propsp->m_implPubWr = सत्य;
                }
                प्रत्यागम;
            }
        }

        // Side effect, but assume does not access public state
        यदि (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = सत्य;
            प्रत्यागम;
        }

        // If impure, or branch, mark statement as fence
        यदि (!nodep->is_विशुद्धम्() || nodep->isBrancher()) m_propsp->m_isFence = सत्य;
    }

    शून्य analyze_ग्रन्थिः(Ast_ग्रन्थिः* nodep) {
        // Check properties of this node if under a statement
        यदि (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        तार्किक singletonListStart = असत्य;
        यदि (nodep->backp()->nextp() != nodep) {  // If at head of list
            singletonListStart = nodep->nextp() == शून्यसूचकः;
            यदि (!singletonListStart) m_राशिः.emplace_पृष्ठभागः(m_hasher);
        }

        // Analyze node
        यदि (Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = VN_CAST(nodep, ग्रन्थिः_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singletonListStart);
        } अन्यथा यदि (AstVarRef* ध्रुव vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } अन्यथा {
            analyze_ग्रन्थिः(nodep);
        }

        // Pop the stack at the end of a list
        यदि (!singletonListStart && !nodep->nextp()) m_राशिः.निष्कासय_पृष्ठभागः();
    }

    // CONSTRUCTOR
    संहिता_Motion_Analysis_Visitor(Ast_ग्रन्थिः* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterate_And_अग्र_Const_Null(nodep);
    }

सार्वजनिक:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    स्थिर शून्य analyze(Ast_ग्रन्थिः* nodep, StmtPropertiesAllocator& stmtProperties) {
        संहिता_Motion_Analysis_Visitor{nodep, stmtProperties};
    }
};

वर्ग संहिता_Motion_Optimize_Visitor final : सार्वजनिक VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    स्थिर constexpr अचिह्नित MAX_DISTANCE = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4_अन्तर्_Use m_user4_अन्तर्_Use;

    ध्रुव StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    तार्किक areSwappable(ध्रुव Ast_ग्रन्थिः_Stmt* ap, ध्रुव Ast_ग्रन्थिः_Stmt* bp) ध्रुव {
        ध्रुव StmtProperties& aProps = m_stmtProperties(ap);
        ध्रुव StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        यदि (aProps.m_isFence) प्रत्यागम असत्य;
        यदि (bProps.m_isFence) प्रत्यागम असत्य;
        // Don't swap side effecting statements, but can move others across them
        यदि (aProps.m_sideEffect && bProps.m_sideEffect) प्रत्यागम असत्य;
        // Don't swap if there is a hazard around public state - must observe ordering
        ध्रुव तार्किक bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        यदि (aProps.m_implPubWr && bPubRef) प्रत्यागम असत्य;
        ध्रुव तार्किक aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        यदि (aPubRef && bProps.m_implPubWr) प्रत्यागम असत्य;
        // If either statement writes a variable that the other reads, they are not swappable
        यदि (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) प्रत्यागम असत्य;
        यदि (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) प्रत्यागम असत्य;
        // If they both write to the same variable, they are not swappable
        यदि (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) प्रत्यागम असत्य;
        // Otherwise good to go
        प्रत्यागम सत्य;
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        // Process only on first encounter
        यदि (nodep->user4_समुच्चयः_Once()) प्रत्यागम;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_ग्रन्थिः_Stmt* prevp = m_stmtProperties(nodep).m_पूर्व_With_Same_Condp;
        // If no previous node with same condition, we are done
        यदि (!prevp) प्रत्यागम;
#यदि_परिभाषितम् VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            ध्रुव Ast_ग्रन्थिः* currp = prevp;
            यावत् (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#यदि_अन्तः
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        यदि (Ast_ग्रन्थिः_Stmt* predp = VN_CAST(nodep->backp(), ग्रन्थिः_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            प्रति (अचिह्नित i = MAX_DISTANCE; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                यदि (predp == prevp) भङ्ग;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_ग्रन्थिः_Stmt* ध्रुव backp = VN_CAST(predp->backp(), ग्रन्थिः_Stmt);
                यदि (!backp) भङ्ग;
                // Don't swap statements if doing so would change program semantics
                यदि (!areSwappable(predp, nodep)) भङ्ग;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            यदि (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlink_Fr_पृष्ठभागः();
                predp->सङ्कलय_अग्र_अत्र(nodep);
                // If the predecessor is the previous node with the same condition, job done
                यदि (predp == prevp) प्रत्यागम;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        प्रति (Ast_ग्रन्थिः_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_पूर्व_With_Same_Condp) {
            // Move prevp (previous statement with same condition) towards currp
            यदि (Ast_ग्रन्थिः_Stmt* succp = VN_CAST(prevp->nextp(), ग्रन्थिः_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                प्रति (अचिह्नित i = MAX_DISTANCE; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    यदि (succp == currp) भङ्ग;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_ग्रन्थिः_Stmt* ध्रुव nextp = VN_CAST(succp->nextp(), ग्रन्थिः_Stmt);
                    यदि (!nextp) भङ्ग;
                    // Don't swap statements if doing so would change program semantics
                    यदि (!areSwappable(prevp, succp)) भङ्ग;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                यदि (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlink_Fr_पृष्ठभागः();
                    succp->सङ्कलय_अत्र_This_As_अग्र(prevp);
                }
            }
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    संहिता_Motion_Optimize_Visitor(Ast_ग्रन्थिः* nodep, ध्रुव StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterate_And_अग्र_Null(nodep);
    }

सार्वजनिक:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    स्थिर Ast_ग्रन्थिः* optimize(Ast_ग्रन्थिः* nodep, ध्रुव StmtPropertiesAllocator& stmtProperties) {
        { संहिता_Motion_Optimize_Visitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        यावत् (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        प्रत्यागम nodep;
    }
};

//######################################################################
// Conditional merging

वर्ग MergeCondVisitor final : सार्वजनिक VNVisitor {
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
    VDouble0 m_stat_Merged_वस्तूनि;  // Statistic tracking
    VDouble0 m_stat_Candidate_वस्तूनि;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_ग्रन्थिः* m_mgFirstp = शून्यसूचकः;  // First node in merged sequence
    Ast_ग्रन्थिः_व्यञ्जकम्* m_mgCondp = शून्यसूचकः;  // The condition of the first node
    ध्रुव Ast_ग्रन्थिः* m_mgLastp = शून्यसूचकः;  // Last node in merged sequence
    ध्रुव Ast_ग्रन्थिः* m_mgNextp = शून्यसूचकः;  // Next node in list being examined
    तार्किक m_mgCondPubWritable = असत्य;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::queue<Ast_ग्रन्थिः*>* m_workQueuep = शून्यसूचकः;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = शून्यसूचकः;

    // METHODS

    // Function that processes a whole sub-tree
    शून्य process(Ast_ग्रन्थिः* nodep) {
        // Set up work queue
        std::queue<Ast_ग्रन्थिः*> workQueue;
        m_workQueuep = &workQueue;
        m_workQueuep->प्रक्षिप(nodep);

        कुरु {
            // Set up user* for this iteration
            ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;
            ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
            ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_ग्रन्थिः* currp = m_workQueuep->front();
            m_workQueuep->निष्कासय();

            // Analyze sub-tree list for code motion and conditional merging
            संहिता_Motion_Analysis_Visitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            यदि (v3Global.opt.fMergeCondMotion()) {
                currp = संहिता_Motion_Optimize_Visitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterate_And_अग्र_Null(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            यदि (m_mgFirstp) merge_अन्तः();
            m_stmtPropertiesp = शून्यसूचकः;
        } यावत् (!m_workQueuep->रिक्तानि());

        m_workQueuep = शून्यसूचकः;
    }

    // Skip past AstArraySel and AstWordSel with const index
    स्थिर Ast_ग्रन्थिः* skipConstSels(Ast_ग्रन्थिः* nodep) {
        यावत् (ध्रुव AstArraySel* ध्रुव aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            यदि (!VN_IS(aselp->bitp(), Const)) प्रत्यागम nodep;
            nodep = aselp->fromp();
        }
        यावत् (ध्रुव AstWordSel* ध्रुव wselp = VN_CAST(nodep, WordSel)) {
            // WordSel index is not constant, so might be expensive
            यदि (!VN_IS(wselp->bitp(), Const)) प्रत्यागम nodep;
            nodep = wselp->fromp();
        }
        प्रत्यागम nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    स्थिर तार्किक is_Cheap_ग्रन्थिः(Ast_ग्रन्थिः* nodep) {
        // Comments are cheap
        यदि (VN_IS(nodep, Comment)) प्रत्यागम सत्य;
        // So are some assignments
        यदि (ध्रुव Ast_ग्रन्थिः_नियोजय* ध्रुव assignp = VN_CAST(nodep, ग्रन्थिः_नियोजय)) {
            // Check LHS
            Ast_ग्रन्थिः* ध्रुव lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            यदि (!VN_IS(lhsp, VarRef)) प्रत्यागम असत्य;

            // Check RHS
            Ast_ग्रन्थिः* ध्रुव rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            यदि (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) प्रत्यागम असत्य;

            // Otherwise it is a cheap assignment
            प्रत्यागम सत्य;
        }
        // Others are not
        प्रत्यागम असत्य;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    स्थिर तार्किक yieldsOneOrZero(ध्रुव Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            प्रत्यागम constp->अङ्कः().गन्तव्यम्_UQuad() <= 1;
        }
        यदि (ध्रुव AstVarRef* ध्रुव vrefp = VN_CAST(nodep, VarRef)) {
            ध्रुव AstVar* ध्रुव varp = vrefp->varp();
            प्रत्यागम varp->विस्तारः_Min() == 1 && !varp->dtypep()->isSigned();
        }
        यदि (ध्रुव Ast_सरणम्_R* ध्रुव shiftp = VN_CAST(nodep, सरणम्_R)) {
            // Shift right by width - 1 or more
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(shiftp->rhsp(), Const)) {
                ध्रुव AstVarRef* ध्रुव vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                ध्रुव पूर्णाङ्क विस्तारः = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->विस्तारः_Min()
                                      : shiftp->विस्तारः();
                यदि (constp->गन्तव्यम्_SInt() >= विस्तारः - 1) प्रत्यागम सत्य;
            }
            प्रत्यागम असत्य;
        }
        यदि (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            प्रत्यागम सत्य;
        }
        यदि (ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव biopp = VN_CAST(nodep, ग्रन्थिः_Biop)) {
            यदि (VN_IS(nodep, And))
                प्रत्यागम yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            यदि (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                प्रत्यागम yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            प्रत्यागम असत्य;
        }
        यदि (ध्रुव AstCond* ध्रुव condp = VN_CAST(nodep, Cond)) {
            प्रत्यागम yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        यदि (ध्रुव AstCCast* ध्रुव castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            प्रत्यागम yieldsOneOrZero(castp->lhsp());
        }
        प्रत्यागम असत्य;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* आवरणम्_Lsb(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (yieldsOneOrZero(nodep)) प्रत्यागम nodep;
        // Otherwise apply masking
        AstConst* ध्रुव maskp = नूतन AstConst{nodep->fileline(), AstConst::BitTrue{}};
        // Mask on left, as conventional
        प्रत्यागम नूतन AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_ग्रन्थिः_व्यञ्जकम्* foldAndUnlink(Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, तार्किक condTrue) {
        यदि (rhsp->same_वृक्षः(m_mgCondp)) {
            प्रत्यागम नूतन AstConst{rhsp->fileline(), AstConst::BitTrue{}, condTrue};
        } अन्यथा यदि (ध्रुव AstCond* ध्रुव condp = extractCondFromRhs(rhsp)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resp
                = condTrue ? condp->thenp()->unlink_Fr_पृष्ठभागः() : condp->elsep()->unlink_Fr_पृष्ठभागः();
            यदि (condp == rhsp) प्रत्यागम resp;
            यदि (ध्रुव AstAnd* ध्रुव andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                प्रत्यागम नूतन AstAnd{andp->fileline(), andp->lhsp()->clone_वृक्षः_विशुद्धम्(असत्य), resp};
            }
        } अन्यथा यदि (ध्रुव AstAnd* ध्रुव andp = VN_CAST(rhsp, And)) {
            यदि (andp->lhsp()->same_वृक्षः(m_mgCondp)) {
                प्रत्यागम condTrue ? आवरणम्_Lsb(andp->rhsp()->unlink_Fr_पृष्ठभागः())
                                : नूतन AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            } अन्यथा {
                UASSERT_OBJ(andp->rhsp()->same_वृक्षः(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                प्रत्यागम condTrue ? आवरणम्_Lsb(andp->lhsp()->unlink_Fr_पृष्ठभागः())
                                : नूतन AstConst{rhsp->fileline(), AstConst::BitFalse{}};
            }
        } अन्यथा यदि (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, WordSel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            प्रत्यागम rhsp->clone_वृक्षः(असत्य);
        }
        // LCOV_EXCL_START
        यदि (debug()) rhsp->dump_वृक्षः("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        प्रत्यागम शून्यसूचकः;
        // LCOV_EXCL_STOP
    }

    शून्य merge_अन्तः() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        यावत् (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            ध्रुव Ast_ग्रन्थिः* ध्रुव backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        यावत् (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            ध्रुव Ast_ग्रन्थिः* ध्रुव nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_ग्रन्थिः_If* recursivep = शून्यसूचकः;
        // Merge if list is longer than one node
        यदि (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            यदि (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_वृक्षः_विशुद्धम्(असत्य);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* ध्रुव resultp = नूतन AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->सङ्कलय_अत्र_This_As_अग्र(resultp);
            // Try to preserve branch prediction if we can
            आकार_t nLikely = 0;
            आकार_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_ग्रन्थिः* nextp = m_mgFirstp;
            कुरु {
                // Grab next pointer and unlink
                Ast_ग्रन्थिः* ध्रुव currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : शून्यसूचकः;
                currp->unlink_Fr_पृष्ठभागः();
                // Skip over comments
                यदि (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_वृक्षः(), currp);
                    अनुवर्त;
                }
                //गणनम्
                ++m_stat_Merged_वस्तूनि;
                यदि (Ast_ग्रन्थिः_नियोजय* ध्रुव assignp = VN_CAST(currp, ग्रन्थिः_नियोजय)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = assignp->rhsp()->unlink_Fr_पृष्ठभागः();
                    Ast_ग्रन्थिः_नियोजय* ध्रुव thenp = assignp;
                    Ast_ग्रन्थिः_नियोजय* ध्रुव elsep = assignp->clone_वृक्षः(असत्य);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, सत्य));
                    elsep->rhsp(foldAndUnlink(rhsp, असत्य));
                    resultp->सङ्कलय_Thensp(thenp);
                    resultp->सङ्कलय_Elsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_वृक्षः(), rhsp);
                } अन्यथा {
                    Ast_ग्रन्थिः_If* ध्रुव ifp = VN_AS(currp, ग्रन्थिः_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    यदि (Ast_ग्रन्थिः* ध्रुव listp = ifp->thensp()) {
                        resultp->सङ्कलय_Thensp(listp->unlink_Fr_पृष्ठभागः_With_अग्र());
                    }
                    यदि (Ast_ग्रन्थिः* ध्रुव listp = ifp->elsesp()) {
                        resultp->सङ्कलय_Elsesp(listp->unlink_Fr_पृष्ठभागः_With_अग्र());
                    }
                    // Record branch prediction
                    यदि (ifp->branchPred().likely()) ++nLikely;
                    यदि (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_वृक्षः(), ifp);
                }
            } यावत् (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            यदि (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            यदि (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            यदि (resultp->thensp()) m_workQueuep->प्रक्षिप(resultp->thensp());
            यदि (resultp->elsesp()) m_workQueuep->प्रक्षिप(resultp->elsesp());
        } अन्यथा यदि (Ast_ग्रन्थिः_If* ध्रुव ifp = VN_CAST(m_mgFirstp, ग्रन्थिः_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = शून्यसूचकः;
        m_mgCondp = शून्यसूचकः;
        m_mgLastp = शून्यसूचकः;
        m_mgNextp = शून्यसूचकः;
        m_mgCondPubWritable = असत्य;
        Ast_ग्रन्थिः::user1_Clear_वृक्षः();  // Clear marked variables
        Ast_ग्रन्थिः::user2_Clear_वृक्षः();
        // Merge recursively within the branches of an un-merged AstNodeIF
        यदि (recursivep) {
            iterate_And_अग्र_Null(recursivep->thensp());
            iterate_And_अग्र_Null(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            यदि (m_mgFirstp) merge_अन्तः();
        }
    }

    // Check if the node can be simplified if included under the if
    तार्किक is_Simplifiable_ग्रन्थिः(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        यदि (ध्रुव Ast_ग्रन्थिः_नियोजय* ध्रुव assignp = VN_CAST(nodep, ग्रन्थिः_नियोजय)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            यदि (assignp->lhsp()->विस्तारः_Min() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                यदि (ध्रुव AstAnd* ध्रुव andp = VN_CAST(assignp->rhsp(), And)) {
                    यदि (andp->lhsp()->same_वृक्षः(m_mgCondp) || andp->rhsp()->same_वृक्षः(m_mgCondp)) {
                        प्रत्यागम सत्य;
                    }
                }
                // Is it 'lhs = cond'?
                यदि (assignp->rhsp()->same_वृक्षः(m_mgCondp)) प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;
    }

    तार्किक सङ्कलय_गन्तव्यम्_List(Ast_ग्रन्थिः_Stmt* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        // Set up head of new list if node is first in list
        यदि (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](ध्रुव AstVarRef* nodep) {
                AstVar* ध्रुव varp = nodep->varp();
                varp->user1(1);
                यदि (varp->is_Sig_लौकिकम्() || varp->isWrittenByDpi()) m_mgCondPubWritable = सत्य;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            यदि ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_ग्रन्थिः::user1_Clear_वृक्षः();
                m_mgCondPubWritable = असत्य;
                // We did not add to the list
                प्रत्यागम असत्य;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            यावत् (सत्य) {
                Ast_ग्रन्थिः_Stmt* ध्रुव backp = VN_CAST(m_mgFirstp->backp(), ग्रन्थिः_Stmt);
                यदि (!backp || backp->nextp() != m_mgFirstp) भङ्ग;  // Don't move up the tree
                ध्रुव StmtProperties& props = (*m_stmtPropertiesp)(backp);
                यदि (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) भङ्ग;
                यदि (is_Simplifiable_ग्रन्थिः(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } अन्यथा यदि (is_Cheap_ग्रन्थिः(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } अन्यथा {
                    भङ्ग;
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
        यदि (!m_mgNextp) merge_अन्तः();
        // Statistics
        ++m_stat_Candidate_वस्तूनि;
        // We did add to the list
        प्रत्यागम सत्य;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    तार्किक सङ्कलय_If_Helpful_Else_अन्तः_Merge(Ast_ग्रन्थिः_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        यदि (!checkOrMakeMergeable(nodep)) प्रत्यागम असत्य;
        यदि (!m_mgFirstp) प्रत्यागम असत्य;  // If 'checkOrMakeMergeable' closed the list
        यदि (m_mgNextp == nodep) {
            यदि (is_Simplifiable_ग्रन्थिः(nodep)) {
                यदि (सङ्कलय_गन्तव्यम्_List(nodep, शून्यसूचकः)) प्रत्यागम सत्य;
            } अन्यथा यदि (is_Cheap_ग्रन्थिः(nodep)) {
                nodep->user2(1);
                यदि (सङ्कलय_गन्तव्यम्_List(nodep, शून्यसूचकः)) प्रत्यागम सत्य;
            }
        }
        // Not added to list, so we are done with the current list
        merge_अन्तः();
        प्रत्यागम असत्य;
    }

    तार्किक checkOrMakeMergeable(ध्रुव Ast_ग्रन्थिः_Stmt* nodep) {
        ध्रुव StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        यदि (props.m_isFence) प्रत्यागम असत्य;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        यदि (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) merge_अन्तः();
        प्रत्यागम सत्य;  // Now surely mergeable
    }

    शून्य merge_अन्तः_If_Incompatible(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव Ast_ग्रन्थिः* condp) {
        यदि (m_mgFirstp && (m_mgNextp != nodep || !condp->same_वृक्षः(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            merge_अन्तः();
        }
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            यदि (!checkOrMakeMergeable(nodep)) प्रत्यागम;
            // Close potentially incompatible pending merge
            merge_अन्तः_If_Incompatible(nodep, condp);
            // Add current node
            सङ्कलय_गन्तव्यम्_List(nodep, condp);
        } अन्यथा यदि (m_mgFirstp) {
            सङ्कलय_If_Helpful_Else_अन्तः_Merge(nodep);
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        // Check if mergeable
        यदि (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterate_And_अग्र_Null(nodep->thensp());
            iterate_And_अग्र_Null(nodep->elsesp());
            प्रत्यागम;
        }
        // Close potentially incompatible pending merge
        merge_अन्तः_If_Incompatible(nodep, nodep->condp());
        // Add current node
        सङ्कलय_गन्तव्यम्_List(nodep, nodep->condp());
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        यदि (m_mgFirstp && सङ्कलय_If_Helpful_Else_अन्तः_Merge(nodep)) प्रत्यागम;
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCFunc* nodep) override {
        // Merge function body
        यदि (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    शून्य दर्शय(AstNetlist* nodep) override { iterate_And_अग्र_Null(nodep->modulesp()); }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override { iterate_And_अग्र_Null(nodep->stmtsp()); }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {}

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट MergeCondVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~MergeCondVisitor() override {
        V3Stats::सङ्कलय_Stat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::सङ्कलय_Stat("Optimizations, MergeCond candidate items", m_stat_Candidate_वस्तूनि);
        V3Stats::सङ्कलय_Stat("Optimizations, MergeCond merged items", m_stat_Merged_वस्तूनि);
        V3Stats::सङ्कलय_Stat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

शून्य V3MergeCond::mergeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { MergeCondVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("merge_cond", 0, dump_वृक्षः_Either_Level() >= 6);
}
