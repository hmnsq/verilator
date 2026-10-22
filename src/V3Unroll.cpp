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
    صنف Stat final {
        size_t m_value = 0;  // Statistics value
        ثابت حرف* ثابت m_name;  // Name for stats file and UDEBUG

    عام:
        explicit Stat(ثابت حرف* ثابت name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_value); }
        ثابت حرف* name() ثابت { أرجع m_name; }
        Stat& operator++() {
            ++m_value;
            أرجع *هذا;
        }
        Stat& operator+=(size_t v) {
            m_value += v;
            أرجع *هذا;
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
صنف UnrolllBindings final {
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
        𐡄𐡍 (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.push_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        𐡄𐡍 (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ثابت valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        أرجع valp;
    }
};

//######################################################################
// Unroll one AstLoop

صنف UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ثابت m_loopp;  // The loop we are trying to unroll
    AstNode* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ثابت m_wrapp = جديد AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, خطأ};
    ثابت منطقي m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    منطقي m_ok = 𐡒𐡔𐡈;  // Unrolling successful so far, gave up if false

    // METHODS
    فراغ cantUnroll(AstNode* nodep, UnrollStats::Stat& stat) {
        m_ok = خطأ;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    منطقي unrollOneIteration(AstNode* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        منطقي foundLoopTest = خطأ;
        // Process one body statement at a time
        لكل (AstNode* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            𐡄𐡍 (AstLoopTest* ثابت testp = VN_CAST(stmtp, LoopTest)) {
                AstNode* ثابت condp = V3Const::constifyEdit(testp->condp());
                𐡄𐡍 (condp->isZero()) {
                    // Loop terminates
                    أرجع خطأ;
                } وإلا 𐡄𐡍 (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    استمر;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->cloneTree(خطأ));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            𐡄𐡍 (!m_ok) أرجع خطأ;

            // Add statements to unrolled body
            طالما (AstNode* ثابت nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                𐡄𐡍 (!m_unrollFull) {
                    m_unrolledSize += nodep->nodeCount();
                    𐡄𐡍 (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        أرجع خطأ;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                𐡄𐡍 (AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = 𐡒𐡔𐡈;
                    // Will not actually need it, nor any subsequent
                    pushDeletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    𐡄𐡍 (testp->condp()->isNeqZero()) استمر;
                    // Won't need any of the trailing statements
                    𐡄𐡍 (m_wrapp->stmtsp()) pushDeletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    𐡄𐡍 (testp->condp()->isZero()) أرجع خطأ;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    أرجع خطأ;
                }

                // Add this statement to the result list
                m_stmtsp = AstNode::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                𐡄𐡍 (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    أرجع خطأ;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        𐡄𐡍 (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            أرجع خطأ;
        }
        // One iteration done, loop continues
        أرجع 𐡒𐡔𐡈;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    منطقي process(AstNode* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        𐡄𐡍 (!nodep) أرجع 𐡒𐡔𐡈;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](AstNode* np) {
            // Failed earlier
            𐡄𐡍 (!m_ok) أرجع;
            // Check for AstLoopTest
            𐡄𐡍 (AstLoopTest* ثابت testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                𐡄𐡍 (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                أرجع;
            }
            // Check for AstFork - can't unroll
            𐡄𐡍 (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                أرجع;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            𐡄𐡍 (AstNodeCCall* ثابت callp = VN_CAST(np, NodeCCall)) {
                𐡄𐡍 (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    أرجع;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            𐡄𐡍 (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                أرجع;
            }
            // Process variable references
            AstVarRef* ثابت refp = VN_CAST(np, VarRef);
            𐡄𐡍 (!refp) أرجع;
            // Ignore if the referenced variable has no binding
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            𐡄𐡍 (!valp) أرجع;
            // If writen, remove the binding
            𐡄𐡍 (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                أرجع;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.push_back(refp);
        });
        // Give up if we have to
        𐡄𐡍 (!m_ok) أرجع خطأ;
        // Actually substitute the variables that still have bindings
        لكل (AstVarRef* ثابت refp : toSubstitute) {
            // Pick up bound value
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            𐡄𐡍 (!valp) استمر;
            // Substitute it
            refp->replaceWith(valp->cloneTree(خطأ));
            VL_DO_DANGLING(pushDeletep(refp), refp);
        }
        أرجع 𐡒𐡔𐡈;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        𐡄𐡍 (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            أرجع;
        }
        // Attempt to unroll the loop
        ثابت size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unrollCount();
        size_t iterCount = 0;
        𐡏𐡁𐡃 {
            𐡄𐡍 (iterCount > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_nFailUnrollCount);
                𐡄𐡍 (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                أرجع;
            }
            ++iterCount;
        } طالما (unrollOneIteration(loopp->stmtsp()));

        𐡄𐡍 (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iterCount;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->deleteTree(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    فراغ visit(AstNode* nodep) override {
        𐡄𐡍 (!m_ok) أرجع;
        // Generic body statement, just substitute
        process(nodep);
    }
    فراغ visit(AstLoopTest* nodep) override {
        𐡄𐡍 (!m_ok) أرجع;
        // If the condition is a ExprStmt, move it before the LoopTest
        𐡄𐡍 (AstExprStmt* ثابت exprp = VN_CAST(nodep->condp(), ExprStmt)) {
            AstNode* ثابت stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            أرجع;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    فراغ visit(AstAssign* nodep) override {
        𐡄𐡍 (!m_ok) أرجع;
        // Can't do it if delayed
        𐡄𐡍 (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            أرجع;
        }
        𐡄𐡍 (!process(nodep->rhsp())) أرجع;
        // If a simple variable assignment, update the binding
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        𐡄𐡍 (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            أرجع;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    فراغ visit(AstIf* nodep) override {
        𐡄𐡍 (!m_ok) أرجع;
        𐡄𐡍 (!process(nodep->condp())) أرجع;
        // If condition is constant, replce with the relevant branch
        𐡄𐡍 (AstConst* ثابت condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            𐡄𐡍 (AstNode* ثابت bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    فراغ visit(AstJumpGo* nodep) override {
        𐡄𐡍 (!m_ok) أرجع;
        // Remove trailing dead code
        𐡄𐡍 (nodep->nextp()) pushDeletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    فراغ visit(AstLoop* nodep) override {
        𐡄𐡍 (!m_ok) أرجع;
        m_bindings.checkpoint();
        std::pair<AstNode*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        𐡄𐡍 (!pair.second) {
            m_bindings.revert();
            process(nodep);
            أرجع;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        𐡄𐡍 (pair.first) {
            nodep->replaceWith(pair.first);
        } وإلا {
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
        𐡄𐡍 (visitor.m_ok) أرجع {visitor.m_stmtsp, 𐡒𐡔𐡈};
        // Otherwise delete intermediate results
        𐡄𐡍 (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->deleteTree(), visitor.m_stmtsp);
        أرجع {nullptr, خطأ};
    }
};

//######################################################################
// Unroll all AstLoop statements

صنف UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    ساكن AstVarRef* unwrapToVarRef(AstNodeExpr* nodep, صحيح minWidth) {
        طالما (𐡒𐡔𐡈) {
            𐡄𐡍 (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) أرجع refp;
            𐡄𐡍 (AstExtend* ثابت ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } وإلا 𐡄𐡍 (AstExtendS* ثابت ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } وإلا 𐡄𐡍 (AstSel* ثابت sp = VN_CAST(nodep, Sel)) {
                ثابت AstConst* ثابت lsbp = VN_CAST(sp->lsbp(), Const);
                𐡄𐡍 (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) أرجع nullptr;
                nodep = sp->fromp();
            } وإلا {
                أرجع nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    منطقي isVarPlus1(AstNode* nodep, ثابت AstVarScope* vscp) {
        AstAdd* ثابت addp = VN_CAST(nodep, Add);
        𐡄𐡍 (!addp || !addp->lhsp()->isOne()) أرجع خطأ;
        ثابت AstVarRef* ثابت r = unwrapToVarRef(addp->rhsp(), addp->width());
        أرجع r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ساكن AstNodeExpr* resizeToWidth(AstNodeExpr* exprp, ثابت AstVarRef* targetRefp) {
        ثابت صحيح width = targetRefp->width();
        𐡄𐡍 (width == 32) أرجع exprp;
        FileLine* ثابت flp = exprp->fileline();
        𐡄𐡍 (width < 32) أرجع جديد AstSel{flp, exprp, 0, width};
        AstExtend* ثابت extp = جديد AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        أرجع extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ساكن منطقي ascendingBound(AstNodeExpr* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        𐡄𐡍 (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) أرجع خطأ;
        AstNodeBiop* ثابت bp = VN_AS(condp, NodeBiop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        أرجع wp && idxRefp && !wp->num().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ساكن منطقي isInRangeGuard(AstNodeExpr* condp, ثابت AstVarScope* idxVscp, uint32_t width,
                               صحيح addrBits) {
        𐡄𐡍 (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) أرجع خطأ;
        AstNodeBiop* ثابت bp = VN_AS(condp, NodeBiop);
        ثابت AstConst* ثابت cp = VN_CAST(bp->lhsp(), Const);
        𐡄𐡍 (!cp || cp->num().isFourState() || cp->toUInt() < width - 1) أرجع خطأ;
        ثابت AstVarRef* ثابت r = unwrapToVarRef(bp->rhsp(), addrBits);
        أرجع r && r->varScopep() == idxVscp;
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
        𐡄𐡍 (!testp) أرجع خطأ;
        AstIf* ثابت ifp = VN_CAST(testp->nextp(), If);
        𐡄𐡍 (!ifp) أرجع خطأ;
        AstAssign* ثابت incp = VN_CAST(ifp->nextp(), Assign);
        𐡄𐡍 (!incp || incp->nextp()) أرجع خطأ;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        𐡄𐡍 (!ascendingBound(testp->condp(), wp, idxRefp)) أرجع خطأ;
        AstVarScope* ثابت idxVscp = idxRefp->varScopep();
        ثابت uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ثابت صحيح addrBits = width <= 1 ? 1 : V3Number::log2b(width - 1) + 1;
        ثابت AstConst* ثابت idxInitp = m_bindings.get(idxVscp);
        𐡄𐡍 (!idxInitp || !idxInitp->isZero()) أرجع خطأ;
        AstVarRef* ثابت incLhsp = VN_CAST(incp->lhsp(), VarRef);
        𐡄𐡍 (!incLhsp || incLhsp->varScopep() != idxVscp) أرجع خطأ;
        𐡄𐡍 (!isVarPlus1(incp->rhsp(), idxVscp)) أرجع خطأ;
        𐡄𐡍 (ifp->elsesp()) أرجع خطأ;
        AstAssign* ثابت thenp = VN_CAST(ifp->thensp(), Assign);
        𐡄𐡍 (!thenp || thenp->nextp()) أرجع خطأ;
        AstVarRef* ثابت targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        𐡄𐡍 (!targetRefp) أرجع خطأ;
        AstVarScope* ثابت targetVscp = targetRefp->varScopep();
        𐡄𐡍 (targetVscp == idxVscp) أرجع خطأ;
        ثابت منطقي isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ثابت منطقي isCountOnes = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        𐡄𐡍 (!isLeadingOne && !isCountOnes) أرجع خطأ;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        AstNodeExpr* condp = ifp->condp();
        𐡄𐡍 (AstLogAnd* ثابت andp = VN_CAST(condp, LogAnd)) {
            𐡄𐡍 (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } وإلا 𐡄𐡍 (AstCond* ثابت ternp = VN_CAST(condp, Cond)) {
            𐡄𐡍 (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ثابت selp = VN_CAST(condp, Sel);
        𐡄𐡍 (!selp || selp->width() != 1) أرجع خطأ;
        ثابت AstVarRef* ثابت fromp = VN_CAST(selp->fromp(), VarRef);
        𐡄𐡍 (!fromp) أرجع خطأ;
        ثابت AstVarScope* ثابت fromVscp = fromp->varScopep();
        𐡄𐡍 (fromVscp == idxVscp || fromVscp == targetVscp) أرجع خطأ;
        AstNodeExpr* ثابت vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        𐡄𐡍 (static_cast<صحيح>(width) != vecExprp->width()) أرجع خطأ;
        ثابت AstVarRef* ثابت idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        𐡄𐡍 (!idxInSel || idxInSel->varScopep() != idxVscp) أرجع خطأ;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ثابت AstConst* ثابت targetInitp = m_bindings.get(targetVscp);
        𐡄𐡍 (!targetInitp || !targetInitp->isZero()) أرجع خطأ;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        FileLine* ثابت flp = loopp->fileline();
        AstNodeExpr* reducep;
        𐡄𐡍 (isLeadingOne) {
            reducep = جديد AstMostSetBitP1{flp, vecExprp->cloneTree(خطأ)};
        } وإلا {
            AstCountOnes* ثابت conep = جديد AstCountOnes{flp, vecExprp->cloneTree(خطأ)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ثابت newp = جديد AstAssign{flp, targetRefp->cloneTree(خطأ), reducep};
        newp->addNext(جديد AstAssign{flp, incLhsp->cloneTree(خطأ), wp->cloneTree(خطأ)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(loopp), loopp);
        𐡄𐡍 (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } وإلا {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_countOnesLowered;
        }
        أرجع 𐡒𐡔𐡈;
    }

    // VISIT
    فراغ visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        لكل (AstNode *succp = nodep, *currp = nodep->backp(); 𐡒𐡔𐡈;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            𐡄𐡍 (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                𐡄𐡍 (VN_IS(currp, JumpBlock)) استمر;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                اكسر;
            }
            AstAssign* ثابت assignp = VN_CAST(currp, Assign);
            𐡄𐡍 (!assignp) اكسر;
            AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            𐡄𐡍 (!valp) اكسر;
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            𐡄𐡍 (!lhsp) اكسر;
            // Don't bind if volatile
            𐡄𐡍 (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) استمر;
            // Don't overwrite a later binding
            𐡄𐡍 (m_bindings.get(lhsp->varScopep())) استمر;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        𐡄𐡍 (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) أرجع;

        // Attempt to unroll this loop
        ثابت std::pair<AstNode*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        𐡄𐡍 (!pair.second) {
            iterateChildren(nodep);
            أرجع;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        𐡄𐡍 (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا {
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
