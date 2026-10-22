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
        const var_na* name() const { ལོག m_name; }
        Stat& operator++() {
            ++m_value;
            ལོག *this;
        }
        Stat& operator+=(size_t v) {
            m_value += v;
            ལོག *this;
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
        གལ་སྲིད (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.push_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        གལ་སྲིད (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* const valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        ལོག valp;
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
        དོན་ལུ (AstNode* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            གལ་སྲིད (AstLoopTest* const testp = VN_CAST(stmtp, LoopTest)) {
                AstNode* const condp = V3Const::constifyEdit(testp->condp());
                གལ་སྲིད (condp->isZero()) {
                    // Loop terminates
                    ལོག false;
                } དེ་མིན གལ་སྲིད (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    continue;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->cloneTree(false));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            གལ་སྲིད (!m_ok) ལོག false;

            // Add statements to unrolled body
            སྐབས (AstNode* const nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                གལ་སྲིད (!m_unrollFull) {
                    m_unrolledSize += nodep->nodeCount();
                    གལ་སྲིད (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        ལོག false;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                གལ་སྲིད (AstLoopTest* const testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = true;
                    // Will not actually need it, nor any subsequent
                    pushDeletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    གལ་སྲིད (testp->condp()->isNeqZero()) continue;
                    // Won't need any of the trailing statements
                    གལ་སྲིད (m_wrapp->stmtsp()) pushDeletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    གལ་སྲིད (testp->condp()->isZero()) ལོག false;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    ལོག false;
                }

                // Add this statement to the result list
                m_stmtsp = AstNode::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                གལ་སྲིད (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    ལོག false;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        གལ་སྲིད (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            ལོག false;
        }
        // One iteration done, loop continues
        ལོག true;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    bool process(AstNode* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        གལ་སྲིད (!nodep) ལོག true;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](AstNode* np) {
            // Failed earlier
            གལ་སྲིད (!m_ok) ལོག;
            // Check for AstLoopTest
            གལ་སྲིད (AstLoopTest* const testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                གལ་སྲིད (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                ལོག;
            }
            // Check for AstFork - can't unroll
            གལ་སྲིད (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                ལོག;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            གལ་སྲིད (AstNodeCCall* const callp = VN_CAST(np, NodeCCall)) {
                གལ་སྲིད (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    ལོག;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            གལ་སྲིད (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                ལོག;
            }
            // Process variable references
            AstVarRef* const refp = VN_CAST(np, VarRef);
            གལ་སྲིད (!refp) ལོག;
            // Ignore if the referenced variable has no binding
            AstConst* const valp = m_bindings.get(refp->varScopep());
            གལ་སྲིད (!valp) ལོག;
            // If writen, remove the binding
            གལ་སྲིད (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                ལོག;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.push_back(refp);
        });
        // Give up if we have to
        གལ་སྲིད (!m_ok) ལོག false;
        // Actually substitute the variables that still have bindings
        དོན་ལུ (AstVarRef* const refp : toSubstitute) {
            // Pick up bound value
            AstConst* const valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            གལ་སྲིད (!valp) continue;
            // Substitute it
            refp->replaceWith(valp->cloneTree(false));
            VL_DO_DANGLING(pushDeletep(refp), refp);
        }
        ལོག true;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        གལ་སྲིད (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            ལོག;
        }
        // Attempt to unroll the loop
        const size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unrollCount();
        size_t iterCount = 0;
        do {
            གལ་སྲིད (iterCount > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_nFailUnrollCount);
                གལ་སྲིད (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                ལོག;
            }
            ++iterCount;
        } སྐབས (unrollOneIteration(loopp->stmtsp()));

        གལ་སྲིད (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iterCount;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->deleteTree(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    void visit(AstNode* nodep) override {
        གལ་སྲིད (!m_ok) ལོག;
        // Generic body statement, just substitute
        process(nodep);
    }
    void visit(AstLoopTest* nodep) override {
        གལ་སྲིད (!m_ok) ལོག;
        // If the condition is a ExprStmt, move it before the LoopTest
        གལ་སྲིད (AstExprStmt* const exprp = VN_CAST(nodep->condp(), ExprStmt)) {
            AstNode* const stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            ལོག;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    void visit(AstAssign* nodep) override {
        གལ་སྲིད (!m_ok) ལོག;
        // Can't do it if delayed
        གལ་སྲིད (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            ལོག;
        }
        གལ་སྲིད (!process(nodep->rhsp())) ལོག;
        // If a simple variable assignment, update the binding
        AstVarRef* const lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* const valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        གལ་སྲིད (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            ལོག;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    void visit(AstIf* nodep) override {
        གལ་སྲིད (!m_ok) ལོག;
        གལ་སྲིད (!process(nodep->condp())) ལོག;
        // If condition is constant, replce with the relevant branch
        གལ་སྲིད (AstConst* const condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            གལ་སྲིད (AstNode* const bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ལོག;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    void visit(AstJumpGo* nodep) override {
        གལ་སྲིད (!m_ok) ལོག;
        // Remove trailing dead code
        གལ་སྲིད (nodep->nextp()) pushDeletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    void visit(AstLoop* nodep) override {
        གལ་སྲིད (!m_ok) ལོག;
        m_bindings.checkpoint();
        std::pair<AstNode*, bool> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        གལ་སྲིད (!pair.second) {
            m_bindings.revert();
            process(nodep);
            ལོག;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        གལ་སྲིད (pair.first) {
            nodep->replaceWith(pair.first);
        } དེ་མིན {
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
        གལ་སྲིད (visitor.m_ok) ལོག {visitor.m_stmtsp, true};
        // Otherwise delete intermediate results
        གལ་སྲིད (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->deleteTree(), visitor.m_stmtsp);
        ལོག {nullptr, false};
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
    static AstVarRef* unwrapToVarRef(AstNodeExpr* nodep, ཧྲིལ minWidth) {
        སྐབས (true) {
            གལ་སྲིད (AstVarRef* const refp = VN_CAST(nodep, VarRef)) ལོག refp;
            གལ་སྲིད (AstExtend* const ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } དེ་མིན གལ་སྲིད (AstExtendS* const ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } དེ་མིན གལ་སྲིད (AstSel* const sp = VN_CAST(nodep, Sel)) {
                const AstConst* const lsbp = VN_CAST(sp->lsbp(), Const);
                གལ་སྲིད (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) ལོག nullptr;
                nodep = sp->fromp();
            } དེ་མིན {
                ལོག nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    bool isVarPlus1(AstNode* nodep, const AstVarScope* vscp) {
        AstAdd* const addp = VN_CAST(nodep, Add);
        གལ་སྲིད (!addp || !addp->lhsp()->isOne()) ལོག false;
        const AstVarRef* const r = unwrapToVarRef(addp->rhsp(), addp->width());
        ལོག r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    static AstNodeExpr* resizeToWidth(AstNodeExpr* exprp, const AstVarRef* targetRefp) {
        const ཧྲིལ width = targetRefp->width();
        གལ་སྲིད (width == 32) ལོག exprp;
        FileLine* const flp = exprp->fileline();
        གལ་སྲིད (width < 32) ལོག new AstSel{flp, exprp, 0, width};
        AstExtend* const extp = new AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        ལོག extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    static bool ascendingBound(AstNodeExpr* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        གལ་སྲིད (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) ལོག false;
        AstNodeBiop* const bp = VN_AS(condp, NodeBiop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        ལོག wp && idxRefp && !wp->num().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    static bool isInRangeGuard(AstNodeExpr* condp, const AstVarScope* idxVscp, uint32_t width,
                               ཧྲིལ addrBits) {
        གལ་སྲིད (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) ལོག false;
        AstNodeBiop* const bp = VN_AS(condp, NodeBiop);
        const AstConst* const cp = VN_CAST(bp->lhsp(), Const);
        གལ་སྲིད (!cp || cp->num().isFourState() || cp->toUInt() < width - 1) ལོག false;
        const AstVarRef* const r = unwrapToVarRef(bp->rhsp(), addrBits);
        ལོག r && r->varScopep() == idxVscp;
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
        གལ་སྲིད (!testp) ལོག false;
        AstIf* const ifp = VN_CAST(testp->nextp(), If);
        གལ་སྲིད (!ifp) ལོག false;
        AstAssign* const incp = VN_CAST(ifp->nextp(), Assign);
        གལ་སྲིད (!incp || incp->nextp()) ལོག false;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        གལ་སྲིད (!ascendingBound(testp->condp(), wp, idxRefp)) ལོག false;
        AstVarScope* const idxVscp = idxRefp->varScopep();
        const uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        const ཧྲིལ addrBits = width <= 1 ? 1 : V3Number::log2b(width - 1) + 1;
        const AstConst* const idxInitp = m_bindings.get(idxVscp);
        གལ་སྲིད (!idxInitp || !idxInitp->isZero()) ལོག false;
        AstVarRef* const incLhsp = VN_CAST(incp->lhsp(), VarRef);
        གལ་སྲིད (!incLhsp || incLhsp->varScopep() != idxVscp) ལོག false;
        གལ་སྲིད (!isVarPlus1(incp->rhsp(), idxVscp)) ལོག false;
        གལ་སྲིད (ifp->elsesp()) ལོག false;
        AstAssign* const thenp = VN_CAST(ifp->thensp(), Assign);
        གལ་སྲིད (!thenp || thenp->nextp()) ལོག false;
        AstVarRef* const targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        གལ་སྲིད (!targetRefp) ལོག false;
        AstVarScope* const targetVscp = targetRefp->varScopep();
        གལ་སྲིད (targetVscp == idxVscp) ལོག false;
        const bool isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        const bool isCountOnes = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        གལ་སྲིད (!isLeadingOne && !isCountOnes) ལོག false;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        AstNodeExpr* condp = ifp->condp();
        གལ་སྲིད (AstLogAnd* const andp = VN_CAST(condp, LogAnd)) {
            གལ་སྲིད (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } དེ་མིན གལ་སྲིད (AstCond* const ternp = VN_CAST(condp, Cond)) {
            གལ་སྲིད (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* const selp = VN_CAST(condp, Sel);
        གལ་སྲིད (!selp || selp->width() != 1) ལོག false;
        const AstVarRef* const fromp = VN_CAST(selp->fromp(), VarRef);
        གལ་སྲིད (!fromp) ལོག false;
        const AstVarScope* const fromVscp = fromp->varScopep();
        གལ་སྲིད (fromVscp == idxVscp || fromVscp == targetVscp) ལོག false;
        AstNodeExpr* const vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        གལ་སྲིད (static_cast<ཧྲིལ>(width) != vecExprp->width()) ལོག false;
        const AstVarRef* const idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        གལ་སྲིད (!idxInSel || idxInSel->varScopep() != idxVscp) ལོག false;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        const AstConst* const targetInitp = m_bindings.get(targetVscp);
        གལ་སྲིད (!targetInitp || !targetInitp->isZero()) ལོག false;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        FileLine* const flp = loopp->fileline();
        AstNodeExpr* reducep;
        གལ་སྲིད (isLeadingOne) {
            reducep = new AstMostSetBitP1{flp, vecExprp->cloneTree(false)};
        } དེ་མིན {
            AstCountOnes* const conep = new AstCountOnes{flp, vecExprp->cloneTree(false)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* const newp = new AstAssign{flp, targetRefp->cloneTree(false), reducep};
        newp->addNext(new AstAssign{flp, incLhsp->cloneTree(false), wp->cloneTree(false)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(loopp), loopp);
        གལ་སྲིད (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } དེ་མིན {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_countOnesLowered;
        }
        ལོག true;
    }

    // VISIT
    void visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        དོན་ལུ (AstNode *succp = nodep, *currp = nodep->backp(); true;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            གལ་སྲིད (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                གལ་སྲིད (VN_IS(currp, JumpBlock)) continue;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                bha_nga;
            }
            AstAssign* const assignp = VN_CAST(currp, Assign);
            གལ་སྲིད (!assignp) bha_nga;
            AstConst* const valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            གལ་སྲིད (!valp) bha_nga;
            AstVarRef* const lhsp = VN_CAST(assignp->lhsp(), VarRef);
            གལ་སྲིད (!lhsp) bha_nga;
            // Don't bind if volatile
            གལ་སྲིད (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) continue;
            // Don't overwrite a later binding
            གལ་སྲིད (m_bindings.get(lhsp->varScopep())) continue;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        གལ་སྲིད (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) ལོག;

        // Attempt to unroll this loop
        const std::pair<AstNode*, bool> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        གལ་སྲིད (!pair.second) {
            iterateChildren(nodep);
            ལོག;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        གལ་སྲིད (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } དེ་མིན {
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
