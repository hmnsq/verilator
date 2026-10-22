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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Unroll.h"

#include "V3Const.h"
#include "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

sa_nracanaa UnrollStats final {
    class Stat final {
        size_t m_value = 0;  // Statistics value
        const var_na* const m_name;  // Name for stats file and UDEBUG

    public:
        explicit Stat(const var_na* const name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_value); }
        const var_na* name() const { ᯀᯮᯞ m_name; }
        Stat& operator++() {
            ++m_value;
            ᯀᯮᯞ *this;
        }
        Stat& operator+=(size_t v) {
            m_value += v;
            ᯀᯮᯞ *this;
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
class UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    const VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

public:
    // CONSTRUCTOR
    UnrolllBindings() = default;
    ~UnrolllBindings() = default;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    void clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        AstNode::user1ClearTree();
    }

    void checkpoint() { m_checkpoints.push_back(m_curr); }

    void revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.pop_back();
    }

    void commit() { m_checkpoints.pop_back(); }

    void set(AstVarScope* vscp, AstConst* valp) {
        ᯀᯮᯝ (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.push_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        ᯀᯮᯝ (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* const valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        ᯀᯮᯞ valp;
    }
};

//######################################################################
// Unroll one AstLoop

class UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* const m_loopp;  // The loop we are trying to unroll
    AstNode* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* const m_wrapp = new AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, false};
    const bool m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    bool m_ok = true;  // Unrolling successful so far, gave up if false

    // METHODS
    void cantUnroll(AstNode* nodep, UnrollStats::Stat& stat) {
        m_ok = false;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    bool unrollOneIteration(AstNode* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        bool foundLoopTest = false;
        // Process one body statement at a time
        for (AstNode* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            ᯀᯮᯝ (AstLoopTest* const testp = VN_CAST(stmtp, LoopTest)) {
                AstNode* const condp = V3Const::constifyEdit(testp->condp());
                ᯀᯮᯝ (condp->isZero()) {
                    // Loop terminates
                    ᯀᯮᯞ false;
                } ᯀᯮᯝᯂᯪ ᯀᯮᯝ (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    continue;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->cloneTree(false));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            ᯀᯮᯝ (!m_ok) ᯀᯮᯞ false;

            // Add statements to unrolled body
            ᯀᯬᯝ (AstNode* const nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                ᯀᯮᯝ (!m_unrollFull) {
                    m_unrolledSize += nodep->nodeCount();
                    ᯀᯮᯝ (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        ᯀᯮᯞ false;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                ᯀᯮᯝ (AstLoopTest* const testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = true;
                    // Will not actually need it, nor any subsequent
                    pushDeletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    ᯀᯮᯝ (testp->condp()->isNeqZero()) continue;
                    // Won't need any of the trailing statements
                    ᯀᯮᯝ (m_wrapp->stmtsp()) pushDeletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    ᯀᯮᯝ (testp->condp()->isZero()) ᯀᯮᯞ false;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    ᯀᯮᯞ false;
                }

                // Add this statement to the result list
                m_stmtsp = AstNode::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                ᯀᯮᯝ (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    ᯀᯮᯞ false;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        ᯀᯮᯝ (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            ᯀᯮᯞ false;
        }
        // One iteration done, loop continues
        ᯀᯮᯞ true;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    bool process(AstNode* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        ᯀᯮᯝ (!nodep) ᯀᯮᯞ true;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](AstNode* np) {
            // Failed earlier
            ᯀᯮᯝ (!m_ok) ᯀᯮᯞ;
            // Check for AstLoopTest
            ᯀᯮᯝ (AstLoopTest* const testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                ᯀᯮᯝ (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                ᯀᯮᯞ;
            }
            // Check for AstFork - can't unroll
            ᯀᯮᯝ (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                ᯀᯮᯞ;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            ᯀᯮᯝ (AstNodeCCall* const callp = VN_CAST(np, NodeCCall)) {
                ᯀᯮᯝ (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    ᯀᯮᯞ;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            ᯀᯮᯝ (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                ᯀᯮᯞ;
            }
            // Process variable references
            AstVarRef* const refp = VN_CAST(np, VarRef);
            ᯀᯮᯝ (!refp) ᯀᯮᯞ;
            // Ignore if the referenced variable has no binding
            AstConst* const valp = m_bindings.get(refp->varScopep());
            ᯀᯮᯝ (!valp) ᯀᯮᯞ;
            // If writen, remove the binding
            ᯀᯮᯝ (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                ᯀᯮᯞ;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.push_back(refp);
        });
        // Give up if we have to
        ᯀᯮᯝ (!m_ok) ᯀᯮᯞ false;
        // Actually substitute the variables that still have bindings
        for (AstVarRef* const refp : toSubstitute) {
            // Pick up bound value
            AstConst* const valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            ᯀᯮᯝ (!valp) continue;
            // Substitute it
            refp->replaceWith(valp->cloneTree(false));
            VL_DO_DANGLING(pushDeletep(refp), refp);
        }
        ᯀᯮᯞ true;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        ᯀᯮᯝ (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            ᯀᯮᯞ;
        }
        // Attempt to unroll the loop
        const size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unrollCount();
        size_t iterCount = 0;
        do {
            ᯀᯮᯝ (iterCount > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_nFailUnrollCount);
                ᯀᯮᯝ (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                ᯀᯮᯞ;
            }
            ++iterCount;
        } ᯀᯬᯝ (unrollOneIteration(loopp->stmtsp()));

        ᯀᯮᯝ (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iterCount;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->deleteTree(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    void visit(AstNode* nodep) override {
        ᯀᯮᯝ (!m_ok) ᯀᯮᯞ;
        // Generic body statement, just substitute
        process(nodep);
    }
    void visit(AstLoopTest* nodep) override {
        ᯀᯮᯝ (!m_ok) ᯀᯮᯞ;
        // If the condition is a ExprStmt, move it before the LoopTest
        ᯀᯮᯝ (AstExprStmt* const exprp = VN_CAST(nodep->condp(), ExprStmt)) {
            AstNode* const stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            ᯀᯮᯞ;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    void visit(AstAssign* nodep) override {
        ᯀᯮᯝ (!m_ok) ᯀᯮᯞ;
        // Can't do it if delayed
        ᯀᯮᯝ (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            ᯀᯮᯞ;
        }
        ᯀᯮᯝ (!process(nodep->rhsp())) ᯀᯮᯞ;
        // If a simple variable assignment, update the binding
        AstVarRef* const lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* const valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        ᯀᯮᯝ (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            ᯀᯮᯞ;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    void visit(AstIf* nodep) override {
        ᯀᯮᯝ (!m_ok) ᯀᯮᯞ;
        ᯀᯮᯝ (!process(nodep->condp())) ᯀᯮᯞ;
        // If condition is constant, replce with the relevant branch
        ᯀᯮᯝ (AstConst* const condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            ᯀᯮᯝ (AstNode* const bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ᯀᯮᯞ;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    void visit(AstJumpGo* nodep) override {
        ᯀᯮᯝ (!m_ok) ᯀᯮᯞ;
        // Remove trailing dead code
        ᯀᯮᯝ (nodep->nextp()) pushDeletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    void visit(AstLoop* nodep) override {
        ᯀᯮᯝ (!m_ok) ᯀᯮᯞ;
        m_bindings.checkpoint();
        std::pair<AstNode*, bool> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        ᯀᯮᯝ (!pair.second) {
            m_bindings.revert();
            process(nodep);
            ᯀᯮᯞ;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        ᯀᯮᯝ (pair.first) {
            nodep->replaceWith(pair.first);
        } ᯀᯮᯝᯂᯪ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

public:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    static std::pair<AstNode*, bool> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        ᯀᯮᯝ (visitor.m_ok) ᯀᯮᯞ {visitor.m_stmtsp, true};
        // Otherwise delete intermediate results
        ᯀᯮᯝ (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->deleteTree(), visitor.m_stmtsp);
        ᯀᯮᯞ {nullptr, false};
    }
};

//######################################################################
// Unroll all AstLoop statements

class UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    static AstVarRef* unwrapToVarRef(AstNodeExpr* nodep, puur_naa_nka minWidth) {
        ᯀᯬᯝ (true) {
            ᯀᯮᯝ (AstVarRef* const refp = VN_CAST(nodep, VarRef)) ᯀᯮᯞ refp;
            ᯀᯮᯝ (AstExtend* const ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } ᯀᯮᯝᯂᯪ ᯀᯮᯝ (AstExtendS* const ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } ᯀᯮᯝᯂᯪ ᯀᯮᯝ (AstSel* const sp = VN_CAST(nodep, Sel)) {
                const AstConst* const lsbp = VN_CAST(sp->lsbp(), Const);
                ᯀᯮᯝ (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) ᯀᯮᯞ nullptr;
                nodep = sp->fromp();
            } ᯀᯮᯝᯂᯪ {
                ᯀᯮᯞ nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    bool isVarPlus1(AstNode* nodep, const AstVarScope* vscp) {
        AstAdd* const addp = VN_CAST(nodep, Add);
        ᯀᯮᯝ (!addp || !addp->lhsp()->isOne()) ᯀᯮᯞ false;
        const AstVarRef* const r = unwrapToVarRef(addp->rhsp(), addp->width());
        ᯀᯮᯞ r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    static AstNodeExpr* resizeToWidth(AstNodeExpr* exprp, const AstVarRef* targetRefp) {
        const puur_naa_nka width = targetRefp->width();
        ᯀᯮᯝ (width == 32) ᯀᯮᯞ exprp;
        FileLine* const flp = exprp->fileline();
        ᯀᯮᯝ (width < 32) ᯀᯮᯞ new AstSel{flp, exprp, 0, width};
        AstExtend* const extp = new AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        ᯀᯮᯞ extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    static bool ascendingBound(AstNodeExpr* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        ᯀᯮᯝ (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) ᯀᯮᯞ false;
        AstNodeBiop* const bp = VN_AS(condp, NodeBiop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        ᯀᯮᯞ wp && idxRefp && !wp->num().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    static bool isInRangeGuard(AstNodeExpr* condp, const AstVarScope* idxVscp, uint32_t width,
                               puur_naa_nka addrBits) {
        ᯀᯮᯝ (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) ᯀᯮᯞ false;
        AstNodeBiop* const bp = VN_AS(condp, NodeBiop);
        const AstConst* const cp = VN_CAST(bp->lhsp(), Const);
        ᯀᯮᯝ (!cp || cp->num().isFourState() || cp->toUInt() < width - 1) ᯀᯮᯞ false;
        const AstVarRef* const r = unwrapToVarRef(bp->rhsp(), addrBits);
        ᯀᯮᯞ r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    bool tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* const testp = VN_CAST(loopp->stmtsp(), LoopTest);
        ᯀᯮᯝ (!testp) ᯀᯮᯞ false;
        AstIf* const ifp = VN_CAST(testp->nextp(), If);
        ᯀᯮᯝ (!ifp) ᯀᯮᯞ false;
        AstAssign* const incp = VN_CAST(ifp->nextp(), Assign);
        ᯀᯮᯝ (!incp || incp->nextp()) ᯀᯮᯞ false;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        ᯀᯮᯝ (!ascendingBound(testp->condp(), wp, idxRefp)) ᯀᯮᯞ false;
        AstVarScope* const idxVscp = idxRefp->varScopep();
        const uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        const puur_naa_nka addrBits = width <= 1 ? 1 : V3Number::log2b(width - 1) + 1;
        const AstConst* const idxInitp = m_bindings.get(idxVscp);
        ᯀᯮᯝ (!idxInitp || !idxInitp->isZero()) ᯀᯮᯞ false;
        AstVarRef* const incLhsp = VN_CAST(incp->lhsp(), VarRef);
        ᯀᯮᯝ (!incLhsp || incLhsp->varScopep() != idxVscp) ᯀᯮᯞ false;
        ᯀᯮᯝ (!isVarPlus1(incp->rhsp(), idxVscp)) ᯀᯮᯞ false;
        ᯀᯮᯝ (ifp->elsesp()) ᯀᯮᯞ false;
        AstAssign* const thenp = VN_CAST(ifp->thensp(), Assign);
        ᯀᯮᯝ (!thenp || thenp->nextp()) ᯀᯮᯞ false;
        AstVarRef* const targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        ᯀᯮᯝ (!targetRefp) ᯀᯮᯞ false;
        AstVarScope* const targetVscp = targetRefp->varScopep();
        ᯀᯮᯝ (targetVscp == idxVscp) ᯀᯮᯞ false;
        const bool isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        const bool isCountOnes = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        ᯀᯮᯝ (!isLeadingOne && !isCountOnes) ᯀᯮᯞ false;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        AstNodeExpr* condp = ifp->condp();
        ᯀᯮᯝ (AstLogAnd* const andp = VN_CAST(condp, LogAnd)) {
            ᯀᯮᯝ (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } ᯀᯮᯝᯂᯪ ᯀᯮᯝ (AstCond* const ternp = VN_CAST(condp, Cond)) {
            ᯀᯮᯝ (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* const selp = VN_CAST(condp, Sel);
        ᯀᯮᯝ (!selp || selp->width() != 1) ᯀᯮᯞ false;
        const AstVarRef* const fromp = VN_CAST(selp->fromp(), VarRef);
        ᯀᯮᯝ (!fromp) ᯀᯮᯞ false;
        const AstVarScope* const fromVscp = fromp->varScopep();
        ᯀᯮᯝ (fromVscp == idxVscp || fromVscp == targetVscp) ᯀᯮᯞ false;
        AstNodeExpr* const vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        ᯀᯮᯝ (static_cast<puur_naa_nka>(width) != vecExprp->width()) ᯀᯮᯞ false;
        const AstVarRef* const idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        ᯀᯮᯝ (!idxInSel || idxInSel->varScopep() != idxVscp) ᯀᯮᯞ false;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        const AstConst* const targetInitp = m_bindings.get(targetVscp);
        ᯀᯮᯝ (!targetInitp || !targetInitp->isZero()) ᯀᯮᯞ false;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        FileLine* const flp = loopp->fileline();
        AstNodeExpr* reducep;
        ᯀᯮᯝ (isLeadingOne) {
            reducep = new AstMostSetBitP1{flp, vecExprp->cloneTree(false)};
        } ᯀᯮᯝᯂᯪ {
            AstCountOnes* const conep = new AstCountOnes{flp, vecExprp->cloneTree(false)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* const newp = new AstAssign{flp, targetRefp->cloneTree(false), reducep};
        newp->addNext(new AstAssign{flp, incLhsp->cloneTree(false), wp->cloneTree(false)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(loopp), loopp);
        ᯀᯮᯝ (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } ᯀᯮᯝᯂᯪ {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_countOnesLowered;
        }
        ᯀᯮᯞ true;
    }

    // VISIT
    void visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        for (AstNode *succp = nodep, *currp = nodep->backp(); true;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            ᯀᯮᯝ (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                ᯀᯮᯝ (VN_IS(currp, JumpBlock)) continue;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                bha_nga;
            }
            AstAssign* const assignp = VN_CAST(currp, Assign);
            ᯀᯮᯝ (!assignp) bha_nga;
            AstConst* const valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            ᯀᯮᯝ (!valp) bha_nga;
            AstVarRef* const lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ᯀᯮᯝ (!lhsp) bha_nga;
            // Don't bind if volatile
            ᯀᯮᯝ (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) continue;
            // Don't overwrite a later binding
            ᯀᯮᯝ (m_bindings.get(lhsp->varScopep())) continue;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        ᯀᯮᯝ (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) ᯀᯮᯞ;

        // Attempt to unroll this loop
        const std::pair<AstNode*, bool> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        ᯀᯮᯝ (!pair.second) {
            iterateChildren(nodep);
            ᯀᯮᯞ;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        ᯀᯮᯝ (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } ᯀᯮᯝᯂᯪ {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

public:
    static void apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

void V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dumpCheckGlobalTree("unroll", 0, dumpTreeEitherLevel() >= 3);
}
