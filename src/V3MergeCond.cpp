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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3MergeCond.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3DupFinder.h"
#అన్తర్భావ "V3Hasher.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <queue>
#అన్తర్భావ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ {

//######################################################################
// Utilities

// This function extracts the Cond node from the RHS of an assignment,
// if there is one and it is in a supported position, which are:
// - RHS is the Cond
// - RHS is And(Const, Cond). This And is inserted often by V3Clean.
AstCond* వెలికితీయి_Cond_From_Rhs(Ast_కణుపు* rhsp) {
    ఒకవేళ (AstCond* స్థిరరాశి condp = VN_CAST(rhsp, Cond)) {
        తిరిగి condp;
    } లేకపోతే ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి andp = VN_CAST(rhsp, And)) {
        ఒకవేళ (AstCond* స్థిరరాశి condp = VN_CAST(andp->rhsp(), Cond)) {
            ఒకవేళ (VN_IS(andp->lhsp(), Const)) తిరిగి condp;
        }
    }
    తిరిగి శూన్యసూచిక;
}

// Predicate to check if two sets are disjoint. This is stable, as we only need
// to determine if the sets contain a shared element, which is a boolean
// property. It is also efficient as we use sorted sets, and therefore can
// enumerate elements in order (what the ordering is, is unimportant), meaning
// the worst case complexity is O(size of smaller set).
తార్కిక areDisjoint(స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*>& a, స్థిరరాశి std::సమితి<స్థిరరాశి AstVar*>& b) {
    ఒకవేళ (a.ఖాళీలు() || b.ఖాళీలు()) తిరిగి నిజం;
    స్థిరరాశి స్వయంచాలక చివర_A = a.చివర();
    స్థిరరాశి స్వయంచాలక చివర_B = b.చివర();
    స్వయంచాలక itA = a.begin();
    స్వయంచాలక itB = b.begin();
    అయితే (నిజం) {
        ఒకవేళ (*itA == *itB) తిరిగి అబద్ధం;
        ఒకవేళ (std::less<స్థిరరాశి AstVar*>{}(*itA, *itB)) {
            itA = std::క్రింది_bound(++itA, చివర_A, *itB);
            ఒకవేళ (itA == చివర_A) తిరిగి నిజం;
        } లేకపోతే {
            itB = std::క్రింది_bound(++itB, చివర_B, *itA);
            ఒకవేళ (itB == చివర_B) తిరిగి నిజం;
        }
    }
}

//######################################################################
// Structure containing information required for code motion/merging

నిర్మాణం StmtProperties final {
    Ast_కణుపు_సమాసం* m_condp = శూన్యసూచిక;  // The condition expression, if a conditional node
    std::సమితి<స్థిరరాశి AstVar*> m_rdVars;  // Variables read by this statement
    std::సమితి<స్థిరరాశి AstVar*> m_wrVars;  // Variables written by this statement
    తార్కిక m_isFence = అబద్ధం;  // Nothing should move across this statement, nor should it be merged
    తార్కిక m_sideEffect = అబద్ధం;  // Statement may have side effect, without access to model state
    తార్కిక m_implPubRd = అబద్ధం;  // Statement may implicitly read public state (without VarRef)
    తార్కిక m_implPubWr = అబద్ధం;  // Statement may implicitly write public state (without VarRef)
    తార్కిక m_explPubRef = అబద్ధం;  // Statement explicitly references public state via VarRef
    Ast_కణుపు_Stmt* m_prevWithSameCondp = శూన్యసూచిక;  // Previous node in same list, with same condition
    తార్కిక writesConditionVar(తార్కిక condPubWritable) స్థిరరాశి {
        // This relies on MarkVarsVisitor having been called on the condition node
        కొరకు (స్థిరరాశి AstVar* స్థిరరాశి varp : m_wrVars) {
            ఒకవేళ (varp->user1()) తిరిగి నిజం;
        }
        // If the condition contains a public variable, check if it might be written
        ఒకవేళ (condPubWritable && m_implPubWr) తిరిగి నిజం;
        // Otherwise not written
        తిరిగి అబద్ధం;
    }
};

// We store the statement properties in user3 via AstUser3Allocator
ఉపయోగించి StmtPropertiesAllocator = AstUser3Allocator<Ast_కణుపు_Stmt, StmtProperties>;

//######################################################################
// Code motion analysis and implementation

// Pure analysis visitor that build the StmtProperties for each statement in the given
// AstNode list (following AstNode::nextp())
తరగతి సంకేతం_Motion_Analysis_Visitor final : బహిరంగ VNVisitorConst {
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
    std::సదిశ<V3DupFinder> m_దొంతర;
    StmtProperties* m_propsp = శూన్యసూచిక;  // StmtProperties structure of current AstNodeStmt

    // Extract condition expression from a mergeable conditional statement, if any
    స్థిర Ast_కణుపు_సమాసం* వెలికితీయి_Condition(స్థిరరాశి Ast_కణుపు_Stmt* nodep) {
        Ast_కణుపు_సమాసం* conditionp = శూన్యసూచిక;
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp = VN_CAST(nodep, కణుపు_కేటాయించు)) {
            ఒకవేళ (AstCond* స్థిరరాశి conditionalp = వెలికితీయి_Cond_From_Rhs(assignp->rhsp())) {
                conditionp = conditionalp->condp();
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_If* స్థిరరాశి ifp = VN_CAST(nodep, కణుపు_If)) {
            conditionp = ifp->condp();
        }
        అయితే (AstCCast* స్థిరరాశి castp = VN_CAST(conditionp, CCast)) conditionp = castp->lhsp();
        తిరిగి conditionp;
    }

    శూన్య analyzeStmt(Ast_కణుపు_Stmt* nodep, తార్కిక try_Cond_సరిపోలిక) {
        VL_RESTORER(m_propsp);
        // Keep hold of props of enclosing statement
        StmtProperties* స్థిరరాశి outerPropsp = m_propsp;
        // Grab the props of this statement
        m_propsp = &m_stmtProperties(nodep);

        // Extract condition from statement
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి condp = వెలికితీయి_Condition(nodep)) {
            // Remember condition node. We always need this as it is used in the later
            // traversal.
            m_propsp->m_condp = condp;
            // If this is a conditional statement, try to find an earlier one with the same
            // condition in the same list (unless we have been told not to bother because we know
            // this node is in a singleton list).
            ఒకవేళ (try_Cond_సరిపోలిక) {
                // Grab the duplicate finder of this list
                V3DupFinder& dupFinder = m_దొంతర.వెనుక();
                // Find a duplicate condition
                స్థిరరాశి V3DupFinder::iterator& dit = dupFinder.కనుగొను_Duplicate(condp);
                ఒకవేళ (dit == dupFinder.చివర()) {
                    // First time seeing this condition in the current list
                    dupFinder.చొప్పించు(condp);
                    // Remember last statement with this condition (which is this statement)
                    condp->user3p(nodep);
                } లేకపోతే {
                    // Seen a conditional with the same condition earlier in the current list
                    Ast_కణుపు_సమాసం* స్థిరరాశి firstp = VN_AS(dit->రెండవ, కణుపు_సమాసం);
                    // Add to properties for easy retrieval during optimization
                    m_propsp->m_prevWithSameCondp = static_cast<Ast_కణుపు_Stmt*>(firstp->user3p());
                    // Remember last statement with this condition (which is this statement)
                    firstp->user3p(nodep);
                }
            }
        }

        // Analyze this statement
        analyze_కణుపు(nodep);

        // If there is an enclosing statement, propagate properties upwards
        ఒకవేళ (outerPropsp) {
            // Add all rd/wr vars to outer statement
            outerPropsp->m_rdVars.చొప్పించు(m_propsp->m_rdVars.cbegin(), m_propsp->m_rdVars.cend());
            outerPropsp->m_wrVars.చొప్పించు(m_propsp->m_wrVars.cbegin(), m_propsp->m_wrVars.cend());
            // Propagate flags to enclosing statement
            outerPropsp->m_isFence |= m_propsp->m_isFence;
            outerPropsp->m_sideEffect |= m_propsp->m_sideEffect;
            outerPropsp->m_implPubRd |= m_propsp->m_implPubRd;
            outerPropsp->m_implPubWr |= m_propsp->m_implPubWr;
            outerPropsp->m_explPubRef |= m_propsp->m_explPubRef;
        }
    }

    శూన్య analyzeVarRef(AstVarRef* nodep) {
        స్థిరరాశి VAccess access = nodep->access();
        AstVar* స్థిరరాశి varp = nodep->varp();
        // Gather read and written variables
        ఒకవేళ (access.isReadOrRW()) m_propsp->m_rdVars.చొప్పించు(varp);
        ఒకవేళ (access.isWriteOrRW()) m_propsp->m_wrVars.చొప్పించు(varp);
        ఒకవేళ (varp->is_Sig_ప్రజా() || varp->isWrittenByDpi() || varp->isReadByDpi()) {
            m_propsp->m_explPubRef = నిజం;
        }
    }

    శూన్య checkProperties(Ast_కణుపు* nodep) {
        // Ignore StmtExpr. It interferes with special casing DPI calls below,
        // and it only checks if the child expr is impure, which is checked
        // explicitly during the analysis.
        ఒకవేళ (VN_IS(nodep, Stmt_సమాసం)) తిరిగి;

        // Call to a DPI import
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_CCall* స్థిరరాశి ccallp = VN_CAST(nodep, కణుపు_CCall)) {
            AstCFunc* స్థిరరాశి funcp = ccallp->funcp();
            ఒకవేళ (funcp->dpiImportWrapper()) {
                ఒకవేళ (!funcp->dpi_శుద్ధ()) {
                    // Assume has side effect and accesses public state
                    m_propsp->m_sideEffect = నిజం;
                    m_propsp->m_implPubRd = నిజం;
                    m_propsp->m_implPubWr = నిజం;
                }
                తిరిగి;
            }
        }

        // Side effect, but assume does not access public state
        ఒకవేళ (VN_IS(nodep, Display) || VN_IS(nodep, Stop)) {
            m_propsp->m_sideEffect = నిజం;
            తిరిగి;
        }

        // If impure, or branch, mark statement as fence
        ఒకవేళ (!nodep->is_శుద్ధ() || nodep->isBrancher()) m_propsp->m_isFence = నిజం;
    }

    శూన్య analyze_కణుపు(Ast_కణుపు* nodep) {
        // Check properties of this node if under a statement
        ఒకవేళ (m_propsp) checkProperties(nodep);
        // Analyze children
        iterateChildrenConst(nodep);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        // Push a new stack entry at the start of a list, but only if the list is not a
        // single element (this saves a lot of allocations in expressions)
        తార్కిక singleton_List_ప్రారంభం = అబద్ధం;
        ఒకవేళ (nodep->backp()->nextp() != nodep) {  // If at head of list
            singleton_List_ప్రారంభం = nodep->nextp() == శూన్యసూచిక;
            ఒకవేళ (!singleton_List_ప్రారంభం) m_దొంతర.emplace_వెనుక(m_hasher);
        }

        // Analyze node
        ఒకవేళ (Ast_కణుపు_Stmt* స్థిరరాశి stmtp = VN_CAST(nodep, కణుపు_Stmt)) {
            analyzeStmt(stmtp, /*tryCondMatch:*/ !singleton_List_ప్రారంభం);
        } లేకపోతే ఒకవేళ (AstVarRef* స్థిరరాశి vrefp = VN_CAST(nodep, VarRef)) {
            analyzeVarRef(vrefp);
        } లేకపోతే {
            analyze_కణుపు(nodep);
        }

        // Pop the stack at the end of a list
        ఒకవేళ (!singleton_List_ప్రారంభం && !nodep->nextp()) m_దొంతర.తీయి_వెనుక();
    }

    // CONSTRUCTOR
    సంకేతం_Motion_Analysis_Visitor(Ast_కణుపు* nodep, StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        iterate_And_తదుపరి_Const_Null(nodep);
    }

బహిరంగ:
    // Analyze the statement list starting at nodep, filling in stmtProperties.
    స్థిర శూన్య analyze(Ast_కణుపు* nodep, StmtPropertiesAllocator& stmtProperties) {
        సంకేతం_Motion_Analysis_Visitor{nodep, stmtProperties};
    }
};

తరగతి సంకేతం_Motion_Optimize_Visitor final : బహిరంగ VNVisitor {
    // Do not move a node more than this many statements.
    // This bounds complexity at O(N), rather than O(N^2).
    స్థిర constexpr గుర్తులేని MAX_దూరం = 500;

    // NODE STATE
    // AstNodeStmt::user3   -> StmtProperties (accessed via m_stmtProperties, managed externally,
    //                         see MergeCondVisitor::process)
    // AstNodeStmt::user4   -> bool: Already processed this node

    VNUser4_లోపల_Use m_user4_లోపల_Use;

    స్థిరరాశి StmtPropertiesAllocator& m_stmtProperties;

    // MEMBERS

    // Predicate that checks if the order of two statements can be swapped
    తార్కిక areSwappable(స్థిరరాశి Ast_కణుపు_Stmt* ap, స్థిరరాశి Ast_కణుపు_Stmt* bp) స్థిరరాశి {
        స్థిరరాశి StmtProperties& aProps = m_stmtProperties(ap);
        స్థిరరాశి StmtProperties& bProps = m_stmtProperties(bp);
        // Don't move across fences
        ఒకవేళ (aProps.m_isFence) తిరిగి అబద్ధం;
        ఒకవేళ (bProps.m_isFence) తిరిగి అబద్ధం;
        // Don't swap side effecting statements, but can move others across them
        ఒకవేళ (aProps.m_sideEffect && bProps.m_sideEffect) తిరిగి అబద్ధం;
        // Don't swap if there is a hazard around public state - must observe ordering
        స్థిరరాశి తార్కిక bPubRef = bProps.m_implPubWr || bProps.m_implPubRd || bProps.m_explPubRef;
        ఒకవేళ (aProps.m_implPubWr && bPubRef) తిరిగి అబద్ధం;
        స్థిరరాశి తార్కిక aPubRef = aProps.m_implPubWr || aProps.m_implPubRd || aProps.m_explPubRef;
        ఒకవేళ (aPubRef && bProps.m_implPubWr) తిరిగి అబద్ధం;
        // If either statement writes a variable that the other reads, they are not swappable
        ఒకవేళ (!areDisjoint(aProps.m_rdVars, bProps.m_wrVars)) తిరిగి అబద్ధం;
        ఒకవేళ (!areDisjoint(bProps.m_rdVars, aProps.m_wrVars)) తిరిగి అబద్ధం;
        // If they both write to the same variable, they are not swappable
        ఒకవేళ (!areDisjoint(aProps.m_wrVars, bProps.m_wrVars)) తిరిగి అబద్ధం;
        // Otherwise good to go
        తిరిగి నిజం;
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        // Process only on first encounter
        ఒకవేళ (nodep->user4_సమితి_Once()) తిరిగి;
        // First re-order children
        iterateChildren(nodep);
        // Grab hold of previous node with same condition
        Ast_కణుపు_Stmt* prevp = m_stmtProperties(nodep).m_prevWithSameCondp;
        // If no previous node with same condition, we are done
        ఒకవేళ (!prevp) తిరిగి;
#నిర్వచితమైతే VL_DEBUG
        {  // Sanity check, only in debug build, otherwise expensive
            స్థిరరాశి Ast_కణుపు* currp = prevp;
            అయితే (currp && currp != nodep) currp = currp->nextp();
            UASSERT_OBJ(currp, nodep, "Predecessor not in same list as " << currp);
        }
#అయితే_ముగింపు
        // Otherwise try to move this node backwards, as close as we can to the previous node
        // with the same condition
        ఒకవేళ (Ast_కణుపు_Stmt* predp = VN_CAST(nodep->backp(), కణుపు_Stmt)) {
            // 'predp' is the newly computed predecessor node of 'nodep', which is initially
            // (without movement) the 'backp' of the node.
            కొరకు (గుర్తులేని i = MAX_దూరం; i; --i) {
                // If the predecessor is the previous node with the same condition, job done
                ఒకవేళ (predp == prevp) ఆపు;
                // Don't move past a non-statement (e.g.: AstVar), or end of list
                Ast_కణుపు_Stmt* స్థిరరాశి backp = VN_CAST(predp->backp(), కణుపు_Stmt);
                ఒకవేళ (!backp) ఆపు;
                // Don't swap statements if doing so would change program semantics
                ఒకవేళ (!areSwappable(predp, nodep)) ఆపు;
                // Otherwise move 'nodep' back
                predp = backp;
            }

            // If we decided that 'nodep' should be moved back
            ఒకవేళ (nodep->backp() != predp) {
                // Move the current node to directly follow the computed predecessor
                nodep->unlink_Fr_వెనుక();
                predp->కూడు_తదుపరి_ఇక్కడ(nodep);
                // If the predecessor is the previous node with the same condition, job done
                ఒకవేళ (predp == prevp) తిరిగి;
            }
        }
        // If we reach here, it means we were unable to move the current node all the way back
        // such that it immediately follows the previous statement with the same condition. Now
        // try to move all previous statements with the same condition forward, in the hope of
        // compacting the list further.
        కొరకు (Ast_కణుపు_Stmt* currp = nodep; prevp;
             currp = prevp, prevp = m_stmtProperties(currp).m_prevWithSameCondp) {
            // Move prevp (previous statement with same condition) towards currp
            ఒకవేళ (Ast_కణుపు_Stmt* succp = VN_CAST(prevp->nextp(), కణుపు_Stmt)) {
                // 'succp' is the newly computed successor node of 'prevp', which is initially
                // (without movement) the 'nextp' of the node.
                కొరకు (గుర్తులేని i = MAX_దూరం; --i;) {
                    // If the successor of the previous statement with same condition is the
                    // target node, we are done with this predecessor
                    ఒకవేళ (succp == currp) ఆపు;
                    // Don't move past a non-statement (e.g.: AstVar), or end of list
                    Ast_కణుపు_Stmt* స్థిరరాశి nextp = VN_CAST(succp->nextp(), కణుపు_Stmt);
                    ఒకవేళ (!nextp) ఆపు;
                    // Don't swap statements if doing so would change program semantics
                    ఒకవేళ (!areSwappable(prevp, succp)) ఆపు;
                    // Otherwise move further forward
                    succp = nextp;
                }

                // If we decided that 'prevp' should be moved forward
                ఒకవేళ (prevp->nextp() != succp) {
                    // Move the current node to directly before the computed successor
                    prevp->unlink_Fr_వెనుక();
                    succp->కూడు_ఇక్కడ_This_As_తదుపరి(prevp);
                }
            }
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {}  // Ignore all non-statements

    // CONSTRUCTOR
    సంకేతం_Motion_Optimize_Visitor(Ast_కణుపు* nodep, స్థిరరాశి StmtPropertiesAllocator& stmtProperties)
        : m_stmtProperties{stmtProperties} {
        // We assert the given node is at the head of the list otherwise we might move a node
        // before the given node. This is easy to fix in the above iteration with a check on a
        // boundary node we should not move past, if we ever need to do so.
        // Note: we will do iterateAndNextNull which requires nodep->backp() != nullptr anyway
        UASSERT_OBJ(nodep->backp()->nextp() != nodep, nodep, "Must be at head of list");
        // Optimize the list
        iterate_And_తదుపరి_Null(nodep);
    }

బహిరంగ:
    // Given an AstNode list (held via AstNode::nextp()), move conditional statements as close
    // together as possible
    స్థిర Ast_కణుపు* optimize(Ast_కణుపు* nodep, స్థిరరాశి StmtPropertiesAllocator& stmtProperties) {
        { సంకేతం_Motion_Optimize_Visitor{nodep, stmtProperties}; }
        // It is possible for the head of the list to be moved later such that it is no longer
        // in head position. If so, rewind the list and return the new head.
        అయితే (nodep->backp()->nextp() == nodep) nodep = nodep->backp();
        తిరిగి nodep;
    }
};

//######################################################################
// Conditional merging

తరగతి విలీనం_Cond_Visitor final : బహిరంగ VNVisitor {
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
    VDouble0 m_stat_Merged_వస్తువులు;  // Statistic tracking
    VDouble0 m_stat_Candidate_వస్తువులు;  // Statistic tracking
    VDouble0 m_statLongestList;  // Statistic tracking

    Ast_కణుపు* m_mgFirstp = శూన్యసూచిక;  // First node in merged sequence
    Ast_కణుపు_సమాసం* m_mgCondp = శూన్యసూచిక;  // The condition of the first node
    స్థిరరాశి Ast_కణుపు* m_mgLastp = శూన్యసూచిక;  // Last node in merged sequence
    స్థిరరాశి Ast_కణుపు* m_mgNextp = శూన్యసూచిక;  // Next node in list being examined
    తార్కిక m_mgCondPubWritable = అబద్ధం;  // True if m_mgCondp contains a public writable variable
    uint32_t m_listLenght = 0;  // Length of current list

    std::వరుస<Ast_కణుపు*>* m_workQueuep = శూన్యసూచిక;  // Node lists (via AstNode::nextp()) to merge
    // Statement properties for code motion and merging
    StmtPropertiesAllocator* m_stmtPropertiesp = శూన్యసూచిక;

    // METHODS

    // Function that processes a whole sub-tree
    శూన్య process(Ast_కణుపు* nodep) {
        // Set up work queue
        std::వరుస<Ast_కణుపు*> work_వరుస;
        m_workQueuep = &work_వరుస;
        m_workQueuep->నెట్టు(nodep);

        చేయి {
            // Set up user* for this iteration
            స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
            స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
            స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
            // Statement properties only preserved for this iteration,
            // then memory is released immediately.
            StmtPropertiesAllocator stmtProperties;
            m_stmtPropertiesp = &stmtProperties;

            // Pop off current work item
            Ast_కణుపు* currp = m_workQueuep->front();
            m_workQueuep->తీయి();

            // Analyze sub-tree list for code motion and conditional merging
            సంకేతం_Motion_Analysis_Visitor::analyze(currp, stmtProperties);
            // Perform the code motion within the whole sub-tree list
            ఒకవేళ (v3Global.opt.f_విలీనం_Cond_Motion()) {
                currp = సంకేతం_Motion_Optimize_Visitor::optimize(currp, stmtProperties);
            }

            // Merge conditionals in the whole sub-tree list (this might create new work items)
            iterate_And_తదుపరి_Null(currp);

            // Close pending merge, if there is one at the end of the whole sub-tree list
            ఒకవేళ (m_mgFirstp) విలీనం_చివర();
            m_stmtPropertiesp = శూన్యసూచిక;
        } అయితే (!m_workQueuep->ఖాళీలు());

        m_workQueuep = శూన్యసూచిక;
    }

    // Skip past AstArraySel and AstWordSel with const index
    స్థిర Ast_కణుపు* skipConstSels(Ast_కణుపు* nodep) {
        అయితే (స్థిరరాశి AstArraySel* స్థిరరాశి aselp = VN_CAST(nodep, ArraySel)) {
            // ArraySel index is not constant, so might be expensive
            ఒకవేళ (!VN_IS(aselp->bitp(), Const)) తిరిగి nodep;
            nodep = aselp->fromp();
        }
        అయితే (స్థిరరాశి Ast_మాట_Sel* స్థిరరాశి wselp = VN_CAST(nodep, మాట_Sel)) {
            // WordSel index is not constant, so might be expensive
            ఒకవేళ (!VN_IS(wselp->bitp(), Const)) తిరిగి nodep;
            nodep = wselp->fromp();
        }
        తిరిగి nodep;
    }

    // Check if this node is cheap enough that duplicating it in two branches of an
    // AstIf is not likely to cause a performance degradation.
    స్థిర తార్కిక is_Cheap_కణుపు(Ast_కణుపు* nodep) {
        // Comments are cheap
        ఒకవేళ (VN_IS(nodep, Comment)) తిరిగి నిజం;
        // So are some assignments
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp = VN_CAST(nodep, కణుపు_కేటాయించు)) {
            // Check LHS
            Ast_కణుపు* స్థిరరాశి lhsp = skipConstSels(assignp->lhsp());
            // LHS is not a VarRef, so might be expensive
            ఒకవేళ (!VN_IS(lhsp, VarRef)) తిరిగి అబద్ధం;

            // Check RHS
            Ast_కణుపు* స్థిరరాశి rhsp = skipConstSels(assignp->rhsp());
            // RHS is not a VarRef or Constant so might be expensive
            ఒకవేళ (!VN_IS(rhsp, VarRef) && !VN_IS(rhsp, Const)) తిరిగి అబద్ధం;

            // Otherwise it is a cheap assignment
            తిరిగి నిజం;
        }
        // Others are not
        తిరిగి అబద్ధం;
    }

    // Predicate to check if an expression yields only 0 or 1 (i.e.: a 1-bit value)
    స్థిర తార్కిక yieldsOneOrZero(స్థిరరాశి Ast_కణుపు* nodep) {
        UASSERT_OBJ(!nodep->isWide(), nodep, "Cannot handle wide nodes");
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) {
            తిరిగి constp->అంకె().గమ్యం_UQuad() <= 1;
        }
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి vrefp = VN_CAST(nodep, VarRef)) {
            స్థిరరాశి AstVar* స్థిరరాశి varp = vrefp->varp();
            తిరిగి varp->వెడల్పు_Min() == 1 && !varp->dtypep()->isSigned();
        }
        ఒకవేళ (స్థిరరాశి Ast_జరుపు_R* స్థిరరాశి shiftp = VN_CAST(nodep, జరుపు_R)) {
            // Shift right by width - 1 or more
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(shiftp->rhsp(), Const)) {
                స్థిరరాశి AstVarRef* స్థిరరాశి vrefp = VN_CAST(shiftp->lhsp(), VarRef);
                స్థిరరాశి పూర్ణాంకం వెడల్పు = vrefp && !vrefp->varp()->dtypep()->isSigned()
                                      ? vrefp->varp()->వెడల్పు_Min()
                                      : shiftp->వెడల్పు();
                ఒకవేళ (constp->గమ్యం_SInt() >= వెడల్పు - 1) తిరిగి నిజం;
            }
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (VN_IS(nodep, Eq) || VN_IS(nodep, Neq) || VN_IS(nodep, Lt) || VN_IS(nodep, Lte)
            || VN_IS(nodep, Gt) || VN_IS(nodep, Gte)) {
            తిరిగి నిజం;
        }
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి biopp = VN_CAST(nodep, కణుపు_Biop)) {
            ఒకవేళ (VN_IS(nodep, And))
                తిరిగి yieldsOneOrZero(biopp->lhsp()) || yieldsOneOrZero(biopp->rhsp());
            ఒకవేళ (VN_IS(nodep, Or) || VN_IS(nodep, Xor))
                తిరిగి yieldsOneOrZero(biopp->lhsp()) && yieldsOneOrZero(biopp->rhsp());
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (స్థిరరాశి AstCond* స్థిరరాశి condp = VN_CAST(nodep, Cond)) {
            తిరిగి yieldsOneOrZero(condp->thenp()) && yieldsOneOrZero(condp->elsep());
        }
        ఒకవేళ (స్థిరరాశి AstCCast* స్థిరరాశి castp = VN_CAST(nodep, CCast)) {
            // Cast never sign extends
            తిరిగి yieldsOneOrZero(castp->lhsp());
        }
        తిరిగి అబద్ధం;
    }

    // Apply (1'b1 & _) cleaning mask if necessary. This is required because this pass is after
    // V3Clean, and sometimes we have an AstAnd with a 1-bit condition on one side, but a more
    // than 1-bit value on the other side, so we need to keep only the LSB.
    స్థిర Ast_కణుపు_సమాసం* ముసుగు_Lsb(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (yieldsOneOrZero(nodep)) తిరిగి nodep;
        // Otherwise apply masking
        AstConst* స్థిరరాశి maskp = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_True{}};
        // Mask on left, as conventional
        తిరిగి కొత్త AstAnd{nodep->fileline(), maskp, nodep};
    }

    // Fold the RHS expression of an assignment assuming the given condition state.
    // Unlink bits from the RHS which is only used once, and can be reused (is an unmodified
    // sub-tree). What remains of the RHS is expected to be deleted by the caller.
    Ast_కణుపు_సమాసం* foldAndUnlink(Ast_కణుపు_సమాసం* rhsp, తార్కిక condTrue) {
        ఒకవేళ (rhsp->same_వృక్షం(m_mgCondp)) {
            తిరిగి కొత్త AstConst{rhsp->fileline(), AstConst::బిట్టు_True{}, condTrue};
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstCond* స్థిరరాశి condp = వెలికితీయి_Cond_From_Rhs(rhsp)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి resp
                = condTrue ? condp->thenp()->unlink_Fr_వెనుక() : condp->elsep()->unlink_Fr_వెనుక();
            ఒకవేళ (condp == rhsp) తిరిగి resp;
            ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి andp = VN_CAST(rhsp, And)) {
                UASSERT_OBJ(andp->rhsp() == condp, rhsp, "Should not try to fold this");
                తిరిగి కొత్త AstAnd{andp->fileline(), andp->lhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), resp};
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి andp = VN_CAST(rhsp, And)) {
            ఒకవేళ (andp->lhsp()->same_వృక్షం(m_mgCondp)) {
                తిరిగి condTrue ? ముసుగు_Lsb(andp->rhsp()->unlink_Fr_వెనుక())
                                : కొత్త AstConst{rhsp->fileline(), AstConst::బిట్టు_False{}};
            } లేకపోతే {
                UASSERT_OBJ(andp->rhsp()->same_వృక్షం(m_mgCondp), rhsp,
                            "AstAnd doesn't hold condition expression");
                తిరిగి condTrue ? ముసుగు_Lsb(andp->lhsp()->unlink_Fr_వెనుక())
                                : కొత్త AstConst{rhsp->fileline(), AstConst::బిట్టు_False{}};
            }
        } లేకపోతే ఒకవేళ (VN_IS(rhsp, ArraySel) || VN_IS(rhsp, మాట_Sel) || VN_IS(rhsp, VarRef)
                   || VN_IS(rhsp, Const)) {
            తిరిగి rhsp->clone_వృక్షం(అబద్ధం);
        }
        // LCOV_EXCL_START
        ఒకవేళ (debug()) rhsp->dump_వృక్షం("Don't know how to fold expression: ");
        rhsp->v3fatalSrc("Should not try to fold this during conditional merging");
        తిరిగి శూన్యసూచిక;
        // LCOV_EXCL_STOP
    }

    శూన్య విలీనం_చివర() {
        UASSERT(m_mgFirstp, "mergeEnd without list");
        // Drop leading cheap nodes. These were only added in the hope of finding
        // an earlier reduced form, but we failed to do so.
        అయితే (m_mgFirstp->user2() && m_mgFirstp != m_mgLastp) {
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి backp = m_mgFirstp;
            m_mgFirstp = m_mgFirstp->nextp();
            --m_listLenght;
            UASSERT_OBJ(m_mgFirstp && m_mgFirstp->backp() == backp, m_mgLastp,
                        "The list should have a non-cheap element");
        }
        // Drop trailing cheap nodes. These were only added in the hope of finding
        // a later conditional to merge, but we failed to do so.
        అయితే (m_mgLastp->user2() && m_mgFirstp != m_mgLastp) {
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి nextp = m_mgLastp;
            m_mgLastp = m_mgLastp->backp();
            --m_listLenght;
            UASSERT_OBJ(m_mgLastp && m_mgLastp->nextp() == nextp, m_mgFirstp,
                        "Cheap statement should not be at the front of the list");
        }
        // If the list contains a single AstNodeIf, we will want to merge its branches.
        // If so, keep hold of the AstNodeIf in this variable.
        Ast_కణుపు_If* recursivep = శూన్యసూచిక;
        // Merge if list is longer than one node
        ఒకవేళ (m_mgFirstp != m_mgLastp) {
            UINFO(6, "MergeCond - First: " << m_mgFirstp << " Last: " << m_mgLastp);
            ++m_statMerges;
            ఒకవేళ (m_listLenght > m_statLongestList) m_statLongestList = m_listLenght;

            // We need a copy of the condition in the new equivalent 'if' statement,
            // and we also need to keep track of it for comparisons later.
            m_mgCondp = m_mgCondp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            // Create equivalent 'if' statement and insert it before the first node
            AstIf* స్థిరరాశి resultp = కొత్త AstIf{m_mgCondp->fileline(), m_mgCondp};
            m_mgFirstp->కూడు_ఇక్కడ_This_As_తదుపరి(resultp);
            // Try to preserve branch prediction if we can
            కొలత_t nLikely = 0;
            కొలత_t nUnlikely = 0;
            // Unzip the list and insert under branches
            Ast_కణుపు* nextp = m_mgFirstp;
            చేయి {
                // Grab next pointer and unlink
                Ast_కణుపు* స్థిరరాశి currp = nextp;
                nextp = currp != m_mgLastp ? currp->nextp() : శూన్యసూచిక;
                currp->unlink_Fr_వెనుక();
                // Skip over comments
                ఒకవేళ (VN_IS(currp, Comment)) {
                    VL_DO_DANGLING(currp->delete_వృక్షం(), currp);
                    కొనసాగు;
                }
                //లెక్క
                ++m_stat_Merged_వస్తువులు;
                ఒకవేళ (Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp = VN_CAST(currp, కణుపు_కేటాయించు)) {
                    // Unlink RHS and clone to get the 2 assignments (reusing assignp)
                    Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = assignp->rhsp()->unlink_Fr_వెనుక();
                    Ast_కణుపు_కేటాయించు* స్థిరరాశి thenp = assignp;
                    Ast_కణుపు_కేటాయించు* స్థిరరాశి elsep = assignp->clone_వృక్షం(అబద్ధం);
                    // Construct the new RHSs and add to branches
                    thenp->rhsp(foldAndUnlink(rhsp, నిజం));
                    elsep->rhsp(foldAndUnlink(rhsp, అబద్ధం));
                    resultp->కూడు_Thensp(thenp);
                    resultp->కూడు_Elsesp(elsep);
                    // Cleanup
                    VL_DO_DANGLING(rhsp->delete_వృక్షం(), rhsp);
                } లేకపోతే {
                    Ast_కణుపు_If* స్థిరరాశి ifp = VN_AS(currp, కణుపు_If);
                    UASSERT_OBJ(ifp, currp, "Must be AstNodeIf");
                    // Move branch contents under new if
                    ఒకవేళ (Ast_కణుపు* స్థిరరాశి listp = ifp->thensp()) {
                        resultp->కూడు_Thensp(listp->unlink_Fr_వెనుక_With_తదుపరి());
                    }
                    ఒకవేళ (Ast_కణుపు* స్థిరరాశి listp = ifp->elsesp()) {
                        resultp->కూడు_Elsesp(listp->unlink_Fr_వెనుక_With_తదుపరి());
                    }
                    // Record branch prediction
                    ఒకవేళ (ifp->branchPred().likely()) ++nLikely;
                    ఒకవేళ (ifp->branchPred().unlikely()) ++nUnlikely;
                    // Cleanup
                    VL_DO_DANGLING(ifp->delete_వృక్షం(), ifp);
                }
            } అయితే (nextp);
            // If prediction is unanimous, assign it to the merged AstIf
            ఒకవేళ (nLikely && !nUnlikely) resultp->branchPred(VBranchPred::BP_LIKELY);
            ఒకవేళ (!nLikely && nUnlikely) resultp->branchPred(VBranchPred::BP_UNLIKELY);
            // Merge the branches of the resulting AstIf after re-analysis
            ఒకవేళ (resultp->thensp()) m_workQueuep->నెట్టు(resultp->thensp());
            ఒకవేళ (resultp->elsesp()) m_workQueuep->నెట్టు(resultp->elsesp());
        } లేకపోతే ఒకవేళ (Ast_కణుపు_If* స్థిరరాశి ifp = VN_CAST(m_mgFirstp, కణుపు_If)) {
            // There was nothing to merge this AstNodeIf with, so try to merge its branches.
            // No re-analysis is required for this, so do it directly below
            recursivep = ifp;
        }
        // Reset state
        m_mgFirstp = శూన్యసూచిక;
        m_mgCondp = శూన్యసూచిక;
        m_mgLastp = శూన్యసూచిక;
        m_mgNextp = శూన్యసూచిక;
        m_mgCondPubWritable = అబద్ధం;
        Ast_కణుపు::user1_Clear_వృక్షం();  // Clear marked variables
        Ast_కణుపు::user2_Clear_వృక్షం();
        // Merge recursively within the branches of an un-merged AstNodeIF
        ఒకవేళ (recursivep) {
            iterate_And_తదుపరి_Null(recursivep->thensp());
            iterate_And_తదుపరి_Null(recursivep->elsesp());
            // Close a pending merge to ensure merge state is
            // reset as expected at the end of this function
            // cppcheck-has-bug-suppress knownConditionTrueFalse
            ఒకవేళ (m_mgFirstp) విలీనం_చివర();
        }
    }

    // Check if the node can be simplified if included under the if
    తార్కిక is_Simplifiable_కణుపు(Ast_కణుపు* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "Cannot check with empty list");
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp = VN_CAST(nodep, కణుపు_కేటాయించు)) {
            // If it's an assignment to a 1-bit signal, try reduced forms
            ఒకవేళ (assignp->lhsp()->వెడల్పు_Min() == 1) {
                // Is it a 'lhs = cond & value' or 'lhs = value & cond'?
                ఒకవేళ (స్థిరరాశి AstAnd* స్థిరరాశి andp = VN_CAST(assignp->rhsp(), And)) {
                    ఒకవేళ (andp->lhsp()->same_వృక్షం(m_mgCondp) || andp->rhsp()->same_వృక్షం(m_mgCondp)) {
                        తిరిగి నిజం;
                    }
                }
                // Is it 'lhs = cond'?
                ఒకవేళ (assignp->rhsp()->same_వృక్షం(m_mgCondp)) తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }

    తార్కిక కూడు_గమ్యం_List(Ast_కణుపు_Stmt* nodep, Ast_కణుపు_సమాసం* condp) {
        // Set up head of new list if node is first in list
        ఒకవేళ (!m_mgFirstp) {
            UASSERT_OBJ(condp, nodep, "Cannot start new list without condition");
            // Mark variable references in the condition, record if a public variable is involved
            condp->foreach([&](స్థిరరాశి AstVarRef* nodep) {
                AstVar* స్థిరరాశి varp = nodep->varp();
                varp->user1(1);
                ఒకవేళ (varp->is_Sig_ప్రజా() || varp->isWrittenByDpi()) m_mgCondPubWritable = నిజం;
            });
            // Now check again if mergeable. We need this to pick up assignments to conditions,
            // e.g.: 'c = c ? a : b' at the beginning of the list, which is in fact not mergeable
            // because it updates the condition. We simply bail on these.
            ఒకవేళ ((*m_stmtPropertiesp)(nodep).writesConditionVar(m_mgCondPubWritable)) {
                // Clear marked variables
                Ast_కణుపు::user1_Clear_వృక్షం();
                m_mgCondPubWritable = అబద్ధం;
                // We did not add to the list
                తిరిగి అబద్ధం;
            }
            m_mgFirstp = nodep;
            m_mgCondp = condp;
            m_listLenght = 0;
            // Add any preceding nodes to the list that would allow us to extend the merge
            // range
            అయితే (నిజం) {
                Ast_కణుపు_Stmt* స్థిరరాశి backp = VN_CAST(m_mgFirstp->backp(), కణుపు_Stmt);
                ఒకవేళ (!backp || backp->nextp() != m_mgFirstp) ఆపు;  // Don't move up the tree
                స్థిరరాశి StmtProperties& props = (*m_stmtPropertiesp)(backp);
                ఒకవేళ (props.m_isFence || props.writesConditionVar(m_mgCondPubWritable)) ఆపు;
                ఒకవేళ (is_Simplifiable_కణుపు(backp)) {
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } లేకపోతే ఒకవేళ (is_Cheap_కణుపు(backp)) {
                    backp->user2(1);
                    ++m_listLenght;
                    m_mgFirstp = backp;
                } లేకపోతే {
                    ఆపు;
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
        ఒకవేళ (!m_mgNextp) విలీనం_చివర();
        // Statistics
        ++m_stat_Candidate_వస్తువులు;
        // We did add to the list
        తిరిగి నిజం;
    }

    // If this node is the next expected node and is helpful to add to the list, do so,
    // otherwise end the current merge. Return true if added, false if ended merge.
    తార్కిక కూడు_If_Helpful_Else_చివర_విలీనం(Ast_కణుపు_Stmt* nodep) {
        UASSERT_OBJ(m_mgFirstp, nodep, "List must be open");
        ఒకవేళ (!checkOrMakeMergeable(nodep)) తిరిగి అబద్ధం;
        ఒకవేళ (!m_mgFirstp) తిరిగి అబద్ధం;  // If 'checkOrMakeMergeable' closed the list
        ఒకవేళ (m_mgNextp == nodep) {
            ఒకవేళ (is_Simplifiable_కణుపు(nodep)) {
                ఒకవేళ (కూడు_గమ్యం_List(nodep, శూన్యసూచిక)) తిరిగి నిజం;
            } లేకపోతే ఒకవేళ (is_Cheap_కణుపు(nodep)) {
                nodep->user2(1);
                ఒకవేళ (కూడు_గమ్యం_List(nodep, శూన్యసూచిక)) తిరిగి నిజం;
            }
        }
        // Not added to list, so we are done with the current list
        విలీనం_చివర();
        తిరిగి అబద్ధం;
    }

    తార్కిక checkOrMakeMergeable(స్థిరరాశి Ast_కణుపు_Stmt* nodep) {
        స్థిరరాశి StmtProperties& props = (*m_stmtPropertiesp)(nodep);
        ఒకవేళ (props.m_isFence) తిరిగి అబద్ధం;  // Fence node never mergeable
        // If the statement writes a condition variable of a pending merge,
        // we must end the pending merge
        ఒకవేళ (m_mgFirstp && props.writesConditionVar(m_mgCondPubWritable)) విలీనం_చివర();
        తిరిగి నిజం;  // Now surely mergeable
    }

    శూన్య విలీనం_చివర_If_Incompatible(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి Ast_కణుపు* condp) {
        ఒకవేళ (m_mgFirstp && (m_mgNextp != nodep || !condp->same_వృక్షం(m_mgCondp))) {
            // Node in different list, or has different condition. Finish current list.
            విలీనం_చివర();
        }
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి condp = (*m_stmtPropertiesp)(nodep).m_condp) {
            // Check if mergeable
            ఒకవేళ (!checkOrMakeMergeable(nodep)) తిరిగి;
            // Close potentially incompatible pending merge
            విలీనం_చివర_If_Incompatible(nodep, condp);
            // Add current node
            కూడు_గమ్యం_List(nodep, condp);
        } లేకపోతే ఒకవేళ (m_mgFirstp) {
            కూడు_If_Helpful_Else_చివర_విలీనం(nodep);
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        // Check if mergeable
        ఒకవేళ (!checkOrMakeMergeable(nodep)) {
            // If not mergeable, try to merge the branches
            iterate_And_తదుపరి_Null(nodep->thensp());
            iterate_And_తదుపరి_Null(nodep->elsesp());
            తిరిగి;
        }
        // Close potentially incompatible pending merge
        విలీనం_చివర_If_Incompatible(nodep, nodep->condp());
        // Add current node
        కూడు_గమ్యం_List(nodep, nodep->condp());
    }

    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        ఒకవేళ (m_mgFirstp && కూడు_If_Helpful_Else_చివర_విలీనం(nodep)) తిరిగి;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCFunc* nodep) override {
        // Merge function body
        ఒకవేళ (nodep->stmtsp()) process(nodep->stmtsp());
    }

    // For speed, only iterate what is necessary.
    శూన్య సందర్శించు(AstNetlist* nodep) override { iterate_And_తదుపరి_Null(nodep->modulesp()); }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override { iterate_And_తదుపరి_Null(nodep->stmtsp()); }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {}

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన విలీనం_Cond_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~విలీనం_Cond_Visitor() override {
        V3Stats::కూడు_Stat("Optimizations, MergeCond merges", m_statMerges);
        V3Stats::కూడు_Stat("Optimizations, MergeCond candidate items", m_stat_Candidate_వస్తువులు);
        V3Stats::కూడు_Stat("Optimizations, MergeCond merged items", m_stat_Merged_వస్తువులు);
        V3Stats::కూడు_Stat("Optimizations, MergeCond longest merge", m_statLongestList);
    }
};

}  // namespace

//######################################################################
// MergeConditionals class functions

శూన్య V3_విలీనం_Cond::విలీనం_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { విలీనం_Cond_Visitor{nodep}; }
    V3Global::dump_Check_Global_వృక్షం("merge_cond", 0, dump_వృక్షం_Either_Level() >= 6);
}
