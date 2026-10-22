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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Unroll.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

ಸಂರಚನಾ UnrollStats final {
    ವರ್ಗ Stat final {
        size_t m_ಮೌಲ್ಯ = 0;  // Statistics value
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ m_name;  // Name for stats file and UDEBUG

    ಸಾರ್ವಜನಿಕ:
        explicit Stat(ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_ಮೌಲ್ಯ); }
        ಧ್ರುವ ವರ್ಣ* name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_name; }
        Stat& operator++() {
            ++m_ಮೌಲ್ಯ;
            ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
        }
        Stat& operator+=(size_t v) {
            m_ಮೌಲ್ಯ += v;
            ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_ಎಣಿಕೆ{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_ಎಣಿಕೆ_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
ವರ್ಗ UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    UnrolllBindings() = ಸ್ವಭಾವ;
    ~UnrolllBindings() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    ಶೂನ್ಯ clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
    }

    ಶೂನ್ಯ checkpoint() { m_checkpoints.ತಳ್ಳು_back(m_curr); }

    ಶೂನ್ಯ revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.ತೆಗೆ_back();
    }

    ಶೂನ್ಯ commit() { m_checkpoints.ತೆಗೆ_back(); }

    ಶೂನ್ಯ set(AstVarScope* vscp, AstConst* valp) {
        ಯದಿ (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.ತಳ್ಳು_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        ಯದಿ (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ಧ್ರುವ valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        ಪ್ರತ್ಯಾಗಮ valp;
    }
};

//######################################################################
// Unroll one AstLoop

ವರ್ಗ UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ಧ್ರುವ m_loopp;  // The loop we are trying to unroll
    Ast_ಗಂಟು* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ಧ್ರುವ m_wrapp = ನೂತನ AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, ಅಸತ್ಯ};
    ಧ್ರುವ ತಾರ್ಕಿಕ m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    ತಾರ್ಕಿಕ m_ok = ಸತ್ಯ;  // Unrolling successful so far, gave up if false

    // METHODS
    ಶೂನ್ಯ cantUnroll(Ast_ಗಂಟು* nodep, UnrollStats::Stat& stat) {
        m_ok = ಅಸತ್ಯ;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    ತಾರ್ಕಿಕ unrollOneIteration(Ast_ಗಂಟು* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        ತಾರ್ಕಿಕ foundLoopTest = ಅಸತ್ಯ;
        // Process one body statement at a time
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            ಯದಿ (AstLoopTest* ಧ್ರುವ testp = VN_CAST(stmtp, LoopTest)) {
                Ast_ಗಂಟು* ಧ್ರುವ condp = V3Const::constifyEdit(testp->condp());
                ಯದಿ (condp->isZero()) {
                    // Loop terminates
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    ಅನುವರ್ತ;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            ಯದಿ (!m_ok) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

            // Add statements to unrolled body
            ಯಾವತ್ (Ast_ಗಂಟು* ಧ್ರುವ nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                ಯದಿ (!m_unrollFull) {
                    m_unrolledSize += nodep->ಗಂಟು_ಎಣಿಕೆ();
                    ಯದಿ (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                ಯದಿ (AstLoopTest* ಧ್ರುವ testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = ಸತ್ಯ;
                    // Will not actually need it, nor any subsequent
                    ತಳ್ಳು_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    ಯದಿ (testp->condp()->isNeqZero()) ಅನುವರ್ತ;
                    // Won't need any of the trailing statements
                    ಯದಿ (m_wrapp->stmtsp()) ತಳ್ಳು_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    ಯದಿ (testp->condp()->isZero()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_ಗಂಟು::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                ಯದಿ (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        ಯದಿ (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        // One iteration done, loop continues
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    ತಾರ್ಕಿಕ process(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_ಗಂಟು* np) {
            // Failed earlier
            ಯದಿ (!m_ok) ಪ್ರತ್ಯಾಗಮ;
            // Check for AstLoopTest
            ಯದಿ (AstLoopTest* ಧ್ರುವ testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                ಯದಿ (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Check for AstFork - can't unroll
            ಯದಿ (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            ಯದಿ (Ast_ಗಂಟು_CCall* ಧ್ರುವ callp = VN_CAST(np, ಗಂಟು_CCall)) {
                ಯದಿ (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            ಯದಿ (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Process variable references
            AstVarRef* ಧ್ರುವ refp = VN_CAST(np, VarRef);
            ಯದಿ (!refp) ಪ್ರತ್ಯಾಗಮ;
            // Ignore if the referenced variable has no binding
            AstConst* ಧ್ರುವ valp = m_bindings.get(refp->varScopep());
            ಯದಿ (!valp) ಪ್ರತ್ಯಾಗಮ;
            // If writen, remove the binding
            ಯದಿ (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.ತಳ್ಳು_back(refp);
        });
        // Give up if we have to
        ಯದಿ (!m_ok) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Actually substitute the variables that still have bindings
        ಪ್ರತಿ (AstVarRef* ಧ್ರುವ refp : toSubstitute) {
            // Pick up bound value
            AstConst* ಧ್ರುವ valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            ಯದಿ (!valp) ಅನುವರ್ತ;
            // Substitute it
            refp->replaceWith(valp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        ಯದಿ (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Attempt to unroll the loop
        ಧ್ರುವ size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_ಎಣಿಕೆ();
        size_t iter_ಎಣಿಕೆ = 0;
        ಕುರು {
            ಯದಿ (iter_ಎಣಿಕೆ > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_ಎಣಿಕೆ);
                ಯದಿ (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                ಪ್ರತ್ಯಾಗಮ;
            }
            ++iter_ಎಣಿಕೆ;
        } ಯಾವತ್ (unrollOneIteration(loopp->stmtsp()));

        ಯದಿ (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_ಎಣಿಕೆ;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_ವೃಕ್ಷ(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (!m_ok) ಪ್ರತ್ಯಾಗಮ;
        // Generic body statement, just substitute
        process(nodep);
    }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override {
        ಯದಿ (!m_ok) ಪ್ರತ್ಯಾಗಮ;
        // If the condition is a ExprStmt, move it before the LoopTest
        ಯದಿ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ exprp = VN_CAST(nodep->condp(), ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    ಶೂನ್ಯ visit(AstAssign* nodep) override {
        ಯದಿ (!m_ok) ಪ್ರತ್ಯಾಗಮ;
        // Can't do it if delayed
        ಯದಿ (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (!process(nodep->rhsp())) ಪ್ರತ್ಯಾಗಮ;
        // If a simple variable assignment, update the binding
        AstVarRef* ಧ್ರುವ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ಧ್ರುವ valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        ಯದಿ (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    ಶೂನ್ಯ visit(AstIf* nodep) override {
        ಯದಿ (!m_ok) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!process(nodep->condp())) ಪ್ರತ್ಯಾಗಮ;
        // If condition is constant, replce with the relevant branch
        ಯದಿ (AstConst* ಧ್ರುವ condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    ಶೂನ್ಯ visit(AstJumpGo* nodep) override {
        ಯದಿ (!m_ok) ಪ್ರತ್ಯಾಗಮ;
        // Remove trailing dead code
        ಯದಿ (nodep->nextp()) ತಳ್ಳು_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        ಯದಿ (!m_ok) ಪ್ರತ್ಯಾಗಮ;
        m_bindings.checkpoint();
        std::pair<Ast_ಗಂಟು*, ತಾರ್ಕಿಕ> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        ಯದಿ (!pair.second) {
            m_bindings.revert();
            process(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        ಯದಿ (pair.first) {
            nodep->replaceWith(pair.first);
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ಸ್ಥಿರ std::pair<Ast_ಗಂಟು*, ತಾರ್ಕಿಕ> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        ಯದಿ (visitor.m_ok) ಪ್ರತ್ಯಾಗಮ {visitor.m_stmtsp, ಸತ್ಯ};
        // Otherwise delete intermediate results
        ಯದಿ (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_ವೃಕ್ಷ(), visitor.m_stmtsp);
        ಪ್ರತ್ಯಾಗಮ {nullptr, ಅಸತ್ಯ};
    }
};

//######################################################################
// Unroll all AstLoop statements

ವರ್ಗ UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    ಸ್ಥಿರ AstVarRef* unwrapToVarRef(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಙ್ಕ minWidth) {
        ಯಾವತ್ (ಸತ್ಯ) {
            ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) ಪ್ರತ್ಯಾಗಮ refp;
            ಯದಿ (AstExtend* ಧ್ರುವ ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } ಅನ್ಯಥಾ ಯದಿ (AstExtendS* ಧ್ರುವ ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } ಅನ್ಯಥಾ ಯದಿ (AstSel* ಧ್ರುವ sp = VN_CAST(nodep, Sel)) {
                ಧ್ರುವ AstConst* ಧ್ರುವ lsbp = VN_CAST(sp->lsbp(), Const);
                ಯದಿ (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) ಪ್ರತ್ಯಾಗಮ nullptr;
                nodep = sp->fromp();
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    ತಾರ್ಕಿಕ isVarPlus1(Ast_ಗಂಟು* nodep, ಧ್ರುವ AstVarScope* vscp) {
        AstAdd* ಧ್ರುವ addp = VN_CAST(nodep, Add);
        ಯದಿ (!addp || !addp->lhsp()->isOne()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVarRef* ಧ್ರುವ r = unwrapToVarRef(addp->rhsp(), addp->width());
        ಪ್ರತ್ಯಾಗಮ r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resizeToWidth(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ AstVarRef* targetRefp) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = targetRefp->width();
        ಯದಿ (width == 32) ಪ್ರತ್ಯಾಗಮ exprp;
        File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
        ಯದಿ (width < 32) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSel{flp, exprp, 0, width};
        AstExtend* ಧ್ರುವ extp = ನೂತನ AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        ಪ್ರತ್ಯಾಗಮ extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ascendingBound(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        ಯದಿ (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        Ast_ಗಂಟು_Biop* ಧ್ರುವ bp = VN_AS(condp, ಗಂಟು_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        ಪ್ರತ್ಯಾಗಮ wp && idxRefp && !wp->ಅಂಕೆ().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isInRangeGuard(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, ಧ್ರುವ AstVarScope* idxVscp, uint32_t width,
                               ಪೂರ್ಣಾಙ್ಕ addrBits) {
        ಯದಿ (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        Ast_ಗಂಟು_Biop* ಧ್ರುವ bp = VN_AS(condp, ಗಂಟು_Biop);
        ಧ್ರುವ AstConst* ಧ್ರುವ cp = VN_CAST(bp->lhsp(), Const);
        ಯದಿ (!cp || cp->ಅಂಕೆ().isFourState() || cp->toUInt() < width - 1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVarRef* ಧ್ರುವ r = unwrapToVarRef(bp->rhsp(), addrBits);
        ಪ್ರತ್ಯಾಗಮ r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    ತಾರ್ಕಿಕ tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* ಧ್ರುವ testp = VN_CAST(loopp->stmtsp(), LoopTest);
        ಯದಿ (!testp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstIf* ಧ್ರುವ ifp = VN_CAST(testp->nextp(), If);
        ಯದಿ (!ifp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstAssign* ಧ್ರುವ incp = VN_CAST(ifp->nextp(), Assign);
        ಯದಿ (!incp || incp->nextp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        ಯದಿ (!ascendingBound(testp->condp(), wp, idxRefp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstVarScope* ಧ್ರುವ idxVscp = idxRefp->varScopep();
        ಧ್ರುವ uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ addrBits = width <= 1 ? 1 : V3_ಸಂಖ್ಯೆ::log2b(width - 1) + 1;
        ಧ್ರುವ AstConst* ಧ್ರುವ idxInitp = m_bindings.get(idxVscp);
        ಯದಿ (!idxInitp || !idxInitp->isZero()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstVarRef* ಧ್ರುವ incLhsp = VN_CAST(incp->lhsp(), VarRef);
        ಯದಿ (!incLhsp || incLhsp->varScopep() != idxVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!isVarPlus1(incp->rhsp(), idxVscp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (ifp->elsesp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstAssign* ಧ್ರುವ thenp = VN_CAST(ifp->thensp(), Assign);
        ಯದಿ (!thenp || thenp->nextp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstVarRef* ಧ್ರುವ targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        ಯದಿ (!targetRefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstVarScope* ಧ್ರುವ targetVscp = targetRefp->varScopep();
        ಯದಿ (targetVscp == idxVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ ತಾರ್ಕಿಕ isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ಧ್ರುವ ತಾರ್ಕಿಕ is_ಎಣಿಕೆ_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        ಯದಿ (!isLeadingOne && !is_ಎಣಿಕೆ_Ones) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ifp->condp();
        ಯದಿ (AstLogAnd* ಧ್ರುವ andp = VN_CAST(condp, LogAnd)) {
            ಯದಿ (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } ಅನ್ಯಥಾ ಯದಿ (AstCond* ಧ್ರುವ ternp = VN_CAST(condp, Cond)) {
            ಯದಿ (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ಧ್ರುವ selp = VN_CAST(condp, Sel);
        ಯದಿ (!selp || selp->width() != 1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVarRef* ಧ್ರುವ fromp = VN_CAST(selp->fromp(), VarRef);
        ಯದಿ (!fromp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVarScope* ಧ್ರುವ fromVscp = fromp->varScopep();
        ಯದಿ (fromVscp == idxVscp || fromVscp == targetVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        ಯದಿ (static_cast<ಪೂರ್ಣಾಙ್ಕ>(width) != vecExprp->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVarRef* ಧ್ರುವ idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        ಯದಿ (!idxInSel || idxInSel->varScopep() != idxVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ಧ್ರುವ AstConst* ಧ್ರುವ targetInitp = m_bindings.get(targetVscp);
        ಯದಿ (!targetInitp || !targetInitp->isZero()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_ಸಾಲು* ಧ್ರುವ flp = loopp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* reducep;
        ಯದಿ (isLeadingOne) {
            reducep = ನೂತನ AstMostSetBitP1{flp, vecExprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
        } ಅನ್ಯಥಾ {
            Ast_ಎಣಿಕೆ_Ones* ಧ್ರುವ conep = ನೂತನ Ast_ಎಣಿಕೆ_Ones{flp, vecExprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ಧ್ರುವ newp = ನೂತನ AstAssign{flp, targetRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), reducep};
        newp->addNext(ನೂತನ AstAssign{flp, incLhsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), wp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(loopp), loopp);
        ಯದಿ (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } ಅನ್ಯಥಾ {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_ಎಣಿಕೆ_Ones_Lowered;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // VISIT
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        ಪ್ರತಿ (Ast_ಗಂಟು *succp = nodep, *currp = nodep->backp(); ಸತ್ಯ;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            ಯದಿ (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                ಯದಿ (VN_IS(currp, JumpBlock)) ಅನುವರ್ತ;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                ಭಙ್ಗ;
            }
            AstAssign* ಧ್ರುವ assignp = VN_CAST(currp, Assign);
            ಯದಿ (!assignp) ಭಙ್ಗ;
            AstConst* ಧ್ರುವ valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            ಯದಿ (!valp) ಭಙ್ಗ;
            AstVarRef* ಧ್ರುವ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ಯದಿ (!lhsp) ಭಙ್ಗ;
            // Don't bind if volatile
            ಯದಿ (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) ಅನುವರ್ತ;
            // Don't overwrite a later binding
            ಯದಿ (m_bindings.get(lhsp->varScopep())) ಅನುವರ್ತ;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        ಯದಿ (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) ಪ್ರತ್ಯಾಗಮ;

        // Attempt to unroll this loop
        ಧ್ರುವ std::pair<Ast_ಗಂಟು*, ತಾರ್ಕಿಕ> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        ಯದಿ (!pair.second) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        ಯದಿ (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

ಶೂನ್ಯ V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("unroll", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
