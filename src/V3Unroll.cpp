// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Loop unrolling
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
//
// Unroll AstLoopStmts
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Unroll.h"

#ضمن "V3Const.h"
#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

بنية UnrollStats final {
    מחלקה Stat final {
        size_t m_value = 0;  // Statistics value
        ثابت حرف* ثابت m_name;  // Name for stats file and UDEBUG

    عام:
        explicit Stat(ثابت حرف* ثابت name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_value); }
        ثابت حرف* name() ثابت { החזר m_name; }
        Stat& operator++() {
            ++m_value;
            החזר *هذا;
        }
        Stat& operator+=(size_t v) {
            m_value += v;
            החזר *هذا;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_nFailUnrollCount{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_countOnesLowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
מחלקה UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ثابت VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

عام:
    // CONSTRUCTOR
    UnrolllBindings() = افتراضي;
    ~UnrolllBindings() = افتراضي;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    فراغ clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        AstNode::user1ClearTree();
    }

    فراغ checkpoint() { m_checkpoints.push_back(m_curr); }

    فراغ revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.pop_back();
    }

    فراغ commit() { m_checkpoints.pop_back(); }

    فراغ set(AstVarScope* vscp, AstConst* valp) {
        אם (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.push_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        אם (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ثابت valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        החזר valp;
    }
};

//######################################################################
// Unroll one AstLoop

מחלקה UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ثابت m_loopp;  // The loop we are trying to unroll
    AstNode* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ثابت m_wrapp = جديد AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, שקר};
    ثابت منطقي m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    منطقي m_ok = אמת;  // Unrolling successful so far, gave up if false

    // METHODS
    فراغ cantUnroll(AstNode* nodep, UnrollStats::Stat& stat) {
        m_ok = שקר;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    منطقي unrollOneIteration(AstNode* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        منطقي foundLoopTest = שקר;
        // Process one body statement at a time
        עבור (AstNode* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            אם (AstLoopTest* ثابت testp = VN_CAST(stmtp, LoopTest)) {
                AstNode* ثابت condp = V3Const::constifyEdit(testp->condp());
                אם (condp->isZero()) {
                    // Loop terminates
                    החזר שקר;
                } אחרת אם (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    استمر;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->cloneTree(שקר));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            אם (!m_ok) החזר שקר;

            // Add statements to unrolled body
            כלעוד (AstNode* ثابت nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                אם (!m_unrollFull) {
                    m_unrolledSize += nodep->nodeCount();
                    אם (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        החזר שקר;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                אם (AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = אמת;
                    // Will not actually need it, nor any subsequent
                    pushDeletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    אם (testp->condp()->isNeqZero()) استمر;
                    // Won't need any of the trailing statements
                    אם (m_wrapp->stmtsp()) pushDeletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    אם (testp->condp()->isZero()) החזר שקר;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    החזר שקר;
                }

                // Add this statement to the result list
                m_stmtsp = AstNode::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                אם (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    החזר שקר;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        אם (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            החזר שקר;
        }
        // One iteration done, loop continues
        החזר אמת;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    منطقي process(AstNode* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        אם (!nodep) החזר אמת;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](AstNode* np) {
            // Failed earlier
            אם (!m_ok) החזר;
            // Check for AstLoopTest
            אם (AstLoopTest* ثابت testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                אם (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                החזר;
            }
            // Check for AstFork - can't unroll
            אם (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                החזר;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            אם (AstNodeCCall* ثابت callp = VN_CAST(np, NodeCCall)) {
                אם (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    החזר;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            אם (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                החזר;
            }
            // Process variable references
            AstVarRef* ثابت refp = VN_CAST(np, VarRef);
            אם (!refp) החזר;
            // Ignore if the referenced variable has no binding
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            אם (!valp) החזר;
            // If writen, remove the binding
            אם (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                החזר;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.push_back(refp);
        });
        // Give up if we have to
        אם (!m_ok) החזר שקר;
        // Actually substitute the variables that still have bindings
        עבור (AstVarRef* ثابت refp : toSubstitute) {
            // Pick up bound value
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            אם (!valp) استمر;
            // Substitute it
            refp->replaceWith(valp->cloneTree(שקר));
            VL_DO_DANGLING(pushDeletep(refp), refp);
        }
        החזר אמת;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        אם (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            החזר;
        }
        // Attempt to unroll the loop
        ثابت size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unrollCount();
        size_t iterCount = 0;
        עשה {
            אם (iterCount > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_nFailUnrollCount);
                אם (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                החזר;
            }
            ++iterCount;
        } כלעוד (unrollOneIteration(loopp->stmtsp()));

        אם (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iterCount;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->deleteTree(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    فراغ visit(AstNode* nodep) override {
        אם (!m_ok) החזר;
        // Generic body statement, just substitute
        process(nodep);
    }
    فراغ visit(AstLoopTest* nodep) override {
        אם (!m_ok) החזר;
        // If the condition is a ExprStmt, move it before the LoopTest
        אם (AstExprStmt* ثابت exprp = VN_CAST(nodep->condp(), ExprStmt)) {
            AstNode* ثابت stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            החזר;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    فراغ visit(AstAssign* nodep) override {
        אם (!m_ok) החזר;
        // Can't do it if delayed
        אם (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            החזר;
        }
        אם (!process(nodep->rhsp())) החזר;
        // If a simple variable assignment, update the binding
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        אם (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            החזר;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    فراغ visit(AstIf* nodep) override {
        אם (!m_ok) החזר;
        אם (!process(nodep->condp())) החזר;
        // If condition is constant, replce with the relevant branch
        אם (AstConst* ثابت condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            אם (AstNode* ثابت bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            החזר;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    فراغ visit(AstJumpGo* nodep) override {
        אם (!m_ok) החזר;
        // Remove trailing dead code
        אם (nodep->nextp()) pushDeletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    فراغ visit(AstLoop* nodep) override {
        אם (!m_ok) החזר;
        m_bindings.checkpoint();
        std::pair<AstNode*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        אם (!pair.second) {
            m_bindings.revert();
            process(nodep);
            החזר;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        אם (pair.first) {
            nodep->replaceWith(pair.first);
        } אחרת {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

عام:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ساكن std::pair<AstNode*, منطقي> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        אם (visitor.m_ok) החזר {visitor.m_stmtsp, אמת};
        // Otherwise delete intermediate results
        אם (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->deleteTree(), visitor.m_stmtsp);
        החזר {nullptr, שקר};
    }
};

//######################################################################
// Unroll all AstLoop statements

מחלקה UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    ساكن AstVarRef* unwrapToVarRef(AstNodeExpr* nodep, صحيح minWidth) {
        כלעוד (אמת) {
            אם (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) החזר refp;
            אם (AstExtend* ثابت ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } אחרת אם (AstExtendS* ثابت ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } אחרת אם (AstSel* ثابت sp = VN_CAST(nodep, Sel)) {
                ثابت AstConst* ثابت lsbp = VN_CAST(sp->lsbp(), Const);
                אם (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) החזר nullptr;
                nodep = sp->fromp();
            } אחרת {
                החזר nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    منطقي isVarPlus1(AstNode* nodep, ثابت AstVarScope* vscp) {
        AstAdd* ثابت addp = VN_CAST(nodep, Add);
        אם (!addp || !addp->lhsp()->isOne()) החזר שקר;
        ثابت AstVarRef* ثابت r = unwrapToVarRef(addp->rhsp(), addp->width());
        החזר r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ساكن AstNodeExpr* resizeToWidth(AstNodeExpr* exprp, ثابت AstVarRef* targetRefp) {
        ثابت صحيح width = targetRefp->width();
        אם (width == 32) החזר exprp;
        FileLine* ثابت flp = exprp->fileline();
        אם (width < 32) החזר جديد AstSel{flp, exprp, 0, width};
        AstExtend* ثابت extp = جديد AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        החזר extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ساكن منطقي ascendingBound(AstNodeExpr* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        אם (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) החזר שקר;
        AstNodeBiop* ثابت bp = VN_AS(condp, NodeBiop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        החזר wp && idxRefp && !wp->num().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ساكن منطقي isInRangeGuard(AstNodeExpr* condp, ثابت AstVarScope* idxVscp, uint32_t width,
                               صحيح addrBits) {
        אם (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) החזר שקר;
        AstNodeBiop* ثابت bp = VN_AS(condp, NodeBiop);
        ثابت AstConst* ثابت cp = VN_CAST(bp->lhsp(), Const);
        אם (!cp || cp->num().isFourState() || cp->toUInt() < width - 1) החזר שקר;
        ثابت AstVarRef* ثابت r = unwrapToVarRef(bp->rhsp(), addrBits);
        החזר r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    منطقي tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* ثابت testp = VN_CAST(loopp->stmtsp(), LoopTest);
        אם (!testp) החזר שקר;
        AstIf* ثابت ifp = VN_CAST(testp->nextp(), If);
        אם (!ifp) החזר שקר;
        AstAssign* ثابت incp = VN_CAST(ifp->nextp(), Assign);
        אם (!incp || incp->nextp()) החזר שקר;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        אם (!ascendingBound(testp->condp(), wp, idxRefp)) החזר שקר;
        AstVarScope* ثابت idxVscp = idxRefp->varScopep();
        ثابت uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ثابت صحيح addrBits = width <= 1 ? 1 : V3Number::log2b(width - 1) + 1;
        ثابت AstConst* ثابت idxInitp = m_bindings.get(idxVscp);
        אם (!idxInitp || !idxInitp->isZero()) החזר שקר;
        AstVarRef* ثابت incLhsp = VN_CAST(incp->lhsp(), VarRef);
        אם (!incLhsp || incLhsp->varScopep() != idxVscp) החזר שקר;
        אם (!isVarPlus1(incp->rhsp(), idxVscp)) החזר שקר;
        אם (ifp->elsesp()) החזר שקר;
        AstAssign* ثابت thenp = VN_CAST(ifp->thensp(), Assign);
        אם (!thenp || thenp->nextp()) החזר שקר;
        AstVarRef* ثابت targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        אם (!targetRefp) החזר שקר;
        AstVarScope* ثابت targetVscp = targetRefp->varScopep();
        אם (targetVscp == idxVscp) החזר שקר;
        ثابت منطقي isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ثابت منطقي isCountOnes = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        אם (!isLeadingOne && !isCountOnes) החזר שקר;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        AstNodeExpr* condp = ifp->condp();
        אם (AstLogAnd* ثابت andp = VN_CAST(condp, LogAnd)) {
            אם (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } אחרת אם (AstCond* ثابت ternp = VN_CAST(condp, Cond)) {
            אם (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ثابت selp = VN_CAST(condp, Sel);
        אם (!selp || selp->width() != 1) החזר שקר;
        ثابت AstVarRef* ثابت fromp = VN_CAST(selp->fromp(), VarRef);
        אם (!fromp) החזר שקר;
        ثابت AstVarScope* ثابت fromVscp = fromp->varScopep();
        אם (fromVscp == idxVscp || fromVscp == targetVscp) החזר שקר;
        AstNodeExpr* ثابت vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        אם (static_cast<صحيح>(width) != vecExprp->width()) החזר שקר;
        ثابت AstVarRef* ثابت idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        אם (!idxInSel || idxInSel->varScopep() != idxVscp) החזר שקר;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ثابت AstConst* ثابت targetInitp = m_bindings.get(targetVscp);
        אם (!targetInitp || !targetInitp->isZero()) החזר שקר;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        FileLine* ثابت flp = loopp->fileline();
        AstNodeExpr* reducep;
        אם (isLeadingOne) {
            reducep = جديد AstMostSetBitP1{flp, vecExprp->cloneTree(שקר)};
        } אחרת {
            AstCountOnes* ثابت conep = جديد AstCountOnes{flp, vecExprp->cloneTree(שקר)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ثابت newp = جديد AstAssign{flp, targetRefp->cloneTree(שקר), reducep};
        newp->addNext(جديد AstAssign{flp, incLhsp->cloneTree(שקר), wp->cloneTree(שקר)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(loopp), loopp);
        אם (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } אחרת {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_countOnesLowered;
        }
        החזר אמת;
    }

    // VISIT
    فراغ visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        עבור (AstNode *succp = nodep, *currp = nodep->backp(); אמת;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            אם (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                אם (VN_IS(currp, JumpBlock)) استمر;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                اكسر;
            }
            AstAssign* ثابت assignp = VN_CAST(currp, Assign);
            אם (!assignp) اكسر;
            AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            אם (!valp) اكسر;
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            אם (!lhsp) اكسر;
            // Don't bind if volatile
            אם (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) استمر;
            // Don't overwrite a later binding
            אם (m_bindings.get(lhsp->varScopep())) استمر;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        אם (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) החזר;

        // Attempt to unroll this loop
        ثابت std::pair<AstNode*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        אם (!pair.second) {
            iterateChildren(nodep);
            החזר;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        אם (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } אחרת {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

عام:
    ساكن فراغ apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

فراغ V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dumpCheckGlobalTree("unroll", 0, dumpTreeEitherLevel() >= 3);
}
