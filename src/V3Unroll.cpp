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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Unroll.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

ਬਣਤਰ UnrollStats final {
    ਵਰਗ Stat final {
        ਮਾਪ_t m_ਮੁੱਲ = 0;  // Statistics value
        ਅਚਲ ਅੱਖਰ* ਅਚਲ m_name;  // Name for stats file and UDEBUG

    ਜਨਤਕ:
        ਸਪਸ਼ਟ Stat(ਅਚਲ ਅੱਖਰ* ਅਚਲ name)
            : m_name{name} {}
        ~Stat() { V3Stats::ਜੋੜੋ_Stat("Optimizations, Loop unrolling, "s + m_name, m_ਮੁੱਲ); }
        ਅਚਲ ਅੱਖਰ* name() ਅਚਲ { ਵਾਪਸ m_name; }
        Stat& ਸੰਚਾਲਕ++() {
            ++m_ਮੁੱਲ;
            ਵਾਪਸ *ਇਹ;
        }
        Stat& ਸੰਚਾਲਕ+=(ਮਾਪ_t v) {
            m_ਮੁੱਲ += v;
            ਵਾਪਸ *ਇਹ;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_ਗਿਣਤੀ{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Lowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_ਗਿਣਤੀ_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
ਵਰਗ UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::ਸਦਿਸ਼<AstConst*> m_curr{ਖਾਲੀ_ਸੂਚਕ};
    // Stack of binding checkpoints
    std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<AstConst*>> m_checkpoints;

ਜਨਤਕ:
    // CONSTRUCTOR
    UnrolllBindings() = ਮੂਲ;
    ~UnrolllBindings() = ਮੂਲ;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    ਖਾਲੀ clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();
    }

    ਖਾਲੀ checkpoint() { m_checkpoints.ਧੱਕੋ_ਪਿੱਠ(m_curr); }

    ਖਾਲੀ revert() {
        m_curr = std::move(m_checkpoints.ਪਿੱਠ());
        m_checkpoints.ਕੱਢੋ_ਪਿੱਠ();
    }

    ਖਾਲੀ commit() { m_checkpoints.ਕੱਢੋ_ਪਿੱਠ(); }

    ਖਾਲੀ ਸੈੱਟ(AstVarScope* vscp, AstConst* valp) {
        ਜੇ (!vscp->user1()) {
            vscp->user1(m_curr.ਮਾਪ());
            m_curr.ਧੱਕੋ_ਪਿੱਠ(ਖਾਲੀ_ਸੂਚਕ);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        ਜੇ (static_cast<ਮਾਪ_t>(vscp->user1()) >= m_curr.ਮਾਪ()) {
            m_curr.resize(vscp->user1() + 1, ਖਾਲੀ_ਸੂਚਕ);
        }
        AstConst* ਅਚਲ valp = vscp->user1() ? m_curr[vscp->user1()] : ਖਾਲੀ_ਸੂਚਕ;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        ਵਾਪਸ valp;
    }
};

//######################################################################
// Unroll one AstLoop

ਵਰਗ UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ਅਚਲ m_loopp;  // The loop we are trying to unroll
    Ast_ਗੰਢ* m_stmtsp = ਖਾਲੀ_ਸੂਚਕ;  // Resulting unrolled statement
    ਮਾਪ_t m_unrolled_ਮਾਪ = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ਅਚਲ m_wrapp = ਨਵਾਂ AstBegin{m_loopp->fileline(), "[EditWrapper]", ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
    ਅਚਲ ਤਾਰਕਿਕ m_unroll_ਭਰਿਆ = m_loopp->unroll().is_ਸੈੱਟ_True();  // Completely unroll the loop?
    ਤਾਰਕਿਕ m_ok = ਸੱਚ;  // Unrolling successful so far, gave up if false

    // METHODS
    ਖਾਲੀ cantUnroll(Ast_ਗੰਢ* nodep, UnrollStats::Stat& stat) {
        m_ok = ਝੂਠ;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    ਤਾਰਕਿਕ unrollOneIteration(Ast_ਗੰਢ* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        ਤਾਰਕਿਕ ਮਿਲਿਆ_Loop_Test = ਝੂਠ;
        // Process one body statement at a time
        ਲਈ (Ast_ਗੰਢ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            ਜੇ (AstLoopTest* ਅਚਲ testp = VN_CAST(stmtp, LoopTest)) {
                Ast_ਗੰਢ* ਅਚਲ condp = V3Const::constifyEdit(testp->condp());
                ਜੇ (condp->isZero()) {
                    // Loop terminates
                    ਵਾਪਸ ਝੂਠ;
                } ਨਹੀਂਤਾਂ ਜੇ (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    ਜਾਰੀ;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->ਜੋੜੋ_Stmtsp(stmtp->clone_ਰੁੱਖ(ਝੂਠ));
            iterate_And_ਅਗਲਾ_Null(m_wrapp->stmtsp());
            // Give up if failed
            ਜੇ (!m_ok) ਵਾਪਸ ਝੂਠ;

            // Add statements to unrolled body
            ਜਦੋਂ (Ast_ਗੰਢ* ਅਚਲ nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                ਜੇ (!m_unroll_ਭਰਿਆ) {
                    m_unrolled_ਮਾਪ += nodep->ਗੰਢ_ਗਿਣਤੀ();
                    ਜੇ (m_unrolled_ਮਾਪ > static_cast<ਮਾਪ_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        ਵਾਪਸ ਝੂਠ;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlink_Fr_ਪਿੱਠ();

                // If a LoopTest, check how it resolved
                ਜੇ (AstLoopTest* ਅਚਲ testp = VN_CAST(nodep, LoopTest)) {
                    ਮਿਲਿਆ_Loop_Test = ਸੱਚ;
                    // Will not actually need it, nor any subsequent
                    ਧੱਕੋ_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    ਜੇ (testp->condp()->isNeqZero()) ਜਾਰੀ;
                    // Won't need any of the trailing statements
                    ਜੇ (m_wrapp->stmtsp()) ਧੱਕੋ_Deletep(m_wrapp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    // Loop terminates
                    ਜੇ (testp->condp()->isZero()) ਵਾਪਸ ਝੂਠ;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    ਵਾਪਸ ਝੂਠ;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                ਜੇ (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    ਵਾਪਸ ਝੂਠ;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        ਜੇ (!ਮਿਲਿਆ_Loop_Test) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            ਵਾਪਸ ਝੂਠ;
        }
        // One iteration done, loop continues
        ਵਾਪਸ ਸੱਚ;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    ਤਾਰਕਿਕ process(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        ਜੇ (!nodep) ਵਾਪਸ ਸੱਚ;
        // Variable references we should try to substitute
        std::ਸਦਿਸ਼<AstVarRef*> ਮੰਜ਼ਿਲ_Substitute;
        // Iterate subtree
        nodep->foreach([&](Ast_ਗੰਢ* np) {
            // Failed earlier
            ਜੇ (!m_ok) ਵਾਪਸ;
            // Check for AstLoopTest
            ਜੇ (AstLoopTest* ਅਚਲ testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                ਜੇ (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                ਵਾਪਸ;
            }
            // Check for AstFork - can't unroll
            ਜੇ (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                ਵਾਪਸ;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            ਜੇ (Ast_ਗੰਢ_CCall* ਅਚਲ callp = VN_CAST(np, ਗੰਢ_CCall)) {
                ਜੇ (!callp->is_ਸ਼ੁੱਧ()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    ਵਾਪਸ;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            ਜੇ (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                ਵਾਪਸ;
            }
            // Process variable references
            AstVarRef* ਅਚਲ refp = VN_CAST(np, VarRef);
            ਜੇ (!refp) ਵਾਪਸ;
            // Ignore if the referenced variable has no binding
            AstConst* ਅਚਲ valp = m_bindings.get(refp->varScopep());
            ਜੇ (!valp) ਵਾਪਸ;
            // If writen, remove the binding
            ਜੇ (refp->access().isWriteOrRW()) {
                m_bindings.ਸੈੱਟ(refp->varScopep(), ਖਾਲੀ_ਸੂਚਕ);
                ਵਾਪਸ;
            }
            // Otherwise add it to the list of variables to substitute
            ਮੰਜ਼ਿਲ_Substitute.ਧੱਕੋ_ਪਿੱਠ(refp);
        });
        // Give up if we have to
        ਜੇ (!m_ok) ਵਾਪਸ ਝੂਠ;
        // Actually substitute the variables that still have bindings
        ਲਈ (AstVarRef* ਅਚਲ refp : ਮੰਜ਼ਿਲ_Substitute) {
            // Pick up bound value
            AstConst* ਅਚਲ valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            ਜੇ (!valp) ਜਾਰੀ;
            // Substitute it
            refp->replaceWith(valp->clone_ਰੁੱਖ(ਝੂਠ));
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        ਜੇ (loopp->unroll().is_ਸੈੱਟ_False()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            ਵਾਪਸ;
        }
        // Attempt to unroll the loop
        ਅਚਲ ਮਾਪ_t iter_ਹੱਦਬੰਦੀ = m_unroll_ਭਰਿਆ ? v3Global.opt.unroll_ਹੱਦਬੰਦੀ()  //
                                              : v3Global.opt.unroll_ਗਿਣਤੀ();
        ਮਾਪ_t iter_ਗਿਣਤੀ = 0;
        ਕਰੋ {
            ਜੇ (iter_ਗਿਣਤੀ > iter_ਹੱਦਬੰਦੀ) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_ਗਿਣਤੀ);
                ਜੇ (m_unroll_ਭਰਿਆ) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iter_ਹੱਦਬੰਦੀ);
                }
                ਵਾਪਸ;
            }
            ++iter_ਗਿਣਤੀ;
        } ਜਦੋਂ (unrollOneIteration(loopp->stmtsp()));

        ਜੇ (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_ਗਿਣਤੀ;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_ਰੁੱਖ(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (!m_ok) ਵਾਪਸ;
        // Generic body statement, just substitute
        process(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override {
        ਜੇ (!m_ok) ਵਾਪਸ;
        // If the condition is a ExprStmt, move it before the LoopTest
        ਜੇ (Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ exprp = VN_CAST(nodep->condp(), ਵਿਅੰਜਕ_Stmt)) {
            Ast_ਗੰਢ* ਅਚਲ stmtsp = exprp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            exprp->replaceWith(exprp->resultp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlink_Fr_ਪਿੱਠ(&relinker);
            stmtsp->ਜੋੜੋ_ਅਗਲਾ(nodep);
            relinker.relink(stmtsp);
            ਵਾਪਸ;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override {
        ਜੇ (!m_ok) ਵਾਪਸ;
        // Can't do it if delayed
        ਜੇ (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            ਵਾਪਸ;
        }
        ਜੇ (!process(nodep->rhsp())) ਵਾਪਸ;
        // If a simple variable assignment, update the binding
        AstVarRef* ਅਚਲ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ਅਚਲ valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        ਜੇ (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.ਸੈੱਟ(lhsp->varScopep(), valp);
            ਵਾਪਸ;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstIf* nodep) override {
        ਜੇ (!m_ok) ਵਾਪਸ;
        ਜੇ (!process(nodep->condp())) ਵਾਪਸ;
        // If condition is constant, replce with the relevant branch
        ਜੇ (AstConst* ਅਚਲ condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(bodyp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());  // This will be iterated next
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstJumpGo* nodep) override {
        ਜੇ (!m_ok) ਵਾਪਸ;
        // Remove trailing dead code
        ਜੇ (nodep->nextp()) ਧੱਕੋ_Deletep(nodep->nextp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        ਜੇ (!m_ok) ਵਾਪਸ;
        m_bindings.checkpoint();
        std::pair<Ast_ਗੰਢ*, ਤਾਰਕਿਕ> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        ਜੇ (!pair.ਦੂਜਾ) {
            m_bindings.revert();
            process(nodep);
            ਵਾਪਸ;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        ਜੇ (pair.ਪਹਿਲਾ) {
            nodep->replaceWith(pair.ਪਹਿਲਾ);
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

ਜਨਤਕ:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ਸਥਿਰ std::pair<Ast_ਗੰਢ*, ਤਾਰਕਿਕ> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        ਜੇ (visitor.m_ok) ਵਾਪਸ {visitor.m_stmtsp, ਸੱਚ};
        // Otherwise delete intermediate results
        ਜੇ (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_ਰੁੱਖ(), visitor.m_stmtsp);
        ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
    }
};

//######################################################################
// Unroll all AstLoop statements

ਵਰਗ UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    ਸਥਿਰ AstVarRef* unwrap_ਮੰਜ਼ਿਲ_Var_Ref(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਪੂਰਨ min_ਚੌੜਾਈ) {
        ਜਦੋਂ (ਸੱਚ) {
            ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) ਵਾਪਸ refp;
            ਜੇ (AstExtend* ਅਚਲ ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } ਨਹੀਂਤਾਂ ਜੇ (AstExtendS* ਅਚਲ ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } ਨਹੀਂਤਾਂ ਜੇ (AstSel* ਅਚਲ sp = VN_CAST(nodep, Sel)) {
                ਅਚਲ AstConst* ਅਚਲ lsbp = VN_CAST(sp->lsbp(), Const);
                ਜੇ (!lsbp || lsbp->ਮੰਜ਼ਿਲ_UInt() != 0 || sp->ਚੌੜਾਈ() < min_ਚੌੜਾਈ) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                nodep = sp->fromp();
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    ਤਾਰਕਿਕ isVarPlus1(Ast_ਗੰਢ* nodep, ਅਚਲ AstVarScope* vscp) {
        Ast_ਜੋੜੋ* ਅਚਲ addp = VN_CAST(nodep, ਜੋੜੋ_2);
        ਜੇ (!addp || !addp->lhsp()->isOne()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVarRef* ਅਚਲ r = unwrap_ਮੰਜ਼ਿਲ_Var_Ref(addp->rhsp(), addp->ਚੌੜਾਈ());
        ਵਾਪਸ r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* resize_ਮੰਜ਼ਿਲ_ਚੌੜਾਈ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ AstVarRef* ਨਿਸ਼ਾਨਾ_Refp) {
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = ਨਿਸ਼ਾਨਾ_Refp->ਚੌੜਾਈ();
        ਜੇ (ਚੌੜਾਈ == 32) ਵਾਪਸ exprp;
        File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
        ਜੇ (ਚੌੜਾਈ < 32) ਵਾਪਸ ਨਵਾਂ AstSel{flp, exprp, 0, ਚੌੜਾਈ};
        AstExtend* ਅਚਲ extp = ਨਵਾਂ AstExtend{flp, exprp};
        extp->dtypeFrom(ਨਿਸ਼ਾਨਾ_Refp);
        ਵਾਪਸ extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ਸਥਿਰ ਤਾਰਕਿਕ ascendingBound(Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        ਜੇ (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) ਵਾਪਸ ਝੂਠ;
        Ast_ਗੰਢ_Biop* ਅਚਲ bp = VN_AS(condp, ਗੰਢ_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        ਵਾਪਸ wp && idxRefp && !wp->ਅੰਕ().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ਸਥਿਰ ਤਾਰਕਿਕ is_ਵਿੱਚ_Range_Guard(Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, ਅਚਲ AstVarScope* idxVscp, uint32_t ਚੌੜਾਈ,
                               ਪੂਰਨ addr_ਬਿੱਟ) {
        ਜੇ (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) ਵਾਪਸ ਝੂਠ;
        Ast_ਗੰਢ_Biop* ਅਚਲ bp = VN_AS(condp, ਗੰਢ_Biop);
        ਅਚਲ AstConst* ਅਚਲ cp = VN_CAST(bp->lhsp(), Const);
        ਜੇ (!cp || cp->ਅੰਕ().isFourState() || cp->ਮੰਜ਼ਿਲ_UInt() < ਚੌੜਾਈ - 1) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVarRef* ਅਚਲ r = unwrap_ਮੰਜ਼ਿਲ_Var_Ref(bp->rhsp(), addr_ਬਿੱਟ);
        ਵਾਪਸ r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    ਤਾਰਕਿਕ try_ਹੇਠਲਾ_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Loop(AstLoop* loopp) {
        AstLoopTest* ਅਚਲ testp = VN_CAST(loopp->stmtsp(), LoopTest);
        ਜੇ (!testp) ਵਾਪਸ ਝੂਠ;
        AstIf* ਅਚਲ ifp = VN_CAST(testp->nextp(), If);
        ਜੇ (!ifp) ਵਾਪਸ ਝੂਠ;
        Ast_ਸੌਂਪੋ* ਅਚਲ incp = VN_CAST(ifp->nextp(), ਸੌਂਪੋ);
        ਜੇ (!incp || incp->nextp()) ਵਾਪਸ ਝੂਠ;
        AstConst* wp = ਖਾਲੀ_ਸੂਚਕ;
        AstVarRef* idxRefp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!ascendingBound(testp->condp(), wp, idxRefp)) ਵਾਪਸ ਝੂਠ;
        AstVarScope* ਅਚਲ idxVscp = idxRefp->varScopep();
        ਅਚਲ uint32_t ਚੌੜਾਈ = wp->ਮੰਜ਼ਿਲ_UInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ਅਚਲ ਪੂਰਨ addr_ਬਿੱਟ = ਚੌੜਾਈ <= 1 ? 1 : V3_ਸੰਖਿਆ::log2b(ਚੌੜਾਈ - 1) + 1;
        ਅਚਲ AstConst* ਅਚਲ idxInitp = m_bindings.get(idxVscp);
        ਜੇ (!idxInitp || !idxInitp->isZero()) ਵਾਪਸ ਝੂਠ;
        AstVarRef* ਅਚਲ incLhsp = VN_CAST(incp->lhsp(), VarRef);
        ਜੇ (!incLhsp || incLhsp->varScopep() != idxVscp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!isVarPlus1(incp->rhsp(), idxVscp)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (ifp->elsesp()) ਵਾਪਸ ਝੂਠ;
        Ast_ਸੌਂਪੋ* ਅਚਲ thenp = VN_CAST(ifp->thensp(), ਸੌਂਪੋ);
        ਜੇ (!thenp || thenp->nextp()) ਵਾਪਸ ਝੂਠ;
        AstVarRef* ਅਚਲ ਨਿਸ਼ਾਨਾ_Refp = VN_CAST(thenp->lhsp(), VarRef);
        ਜੇ (!ਨਿਸ਼ਾਨਾ_Refp) ਵਾਪਸ ਝੂਠ;
        AstVarScope* ਅਚਲ ਨਿਸ਼ਾਨਾ_Vscp = ਨਿਸ਼ਾਨਾ_Refp->varScopep();
        ਜੇ (ਨਿਸ਼ਾਨਾ_Vscp == idxVscp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ ਤਾਰਕਿਕ isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ਅਚਲ ਤਾਰਕਿਕ is_ਗਿਣਤੀ_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), ਨਿਸ਼ਾਨਾ_Vscp);
        ਜੇ (!isLeadingOne && !is_ਗਿਣਤੀ_Ones) ਵਾਪਸ ਝੂਠ;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ifp->condp();
        ਜੇ (AstLogAnd* ਅਚਲ andp = VN_CAST(condp, LogAnd)) {
            ਜੇ (is_ਵਿੱਚ_Range_Guard(andp->lhsp(), idxVscp, ਚੌੜਾਈ, addr_ਬਿੱਟ)) condp = andp->rhsp();
        } ਨਹੀਂਤਾਂ ਜੇ (AstCond* ਅਚਲ ternp = VN_CAST(condp, Cond)) {
            ਜੇ (is_ਵਿੱਚ_Range_Guard(ternp->condp(), idxVscp, ਚੌੜਾਈ, addr_ਬਿੱਟ)) condp = ternp->thenp();
        }
        AstSel* ਅਚਲ selp = VN_CAST(condp, Sel);
        ਜੇ (!selp || selp->ਚੌੜਾਈ() != 1) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVarRef* ਅਚਲ fromp = VN_CAST(selp->fromp(), VarRef);
        ਜੇ (!fromp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVarScope* ਅਚਲ fromVscp = fromp->varScopep();
        ਜੇ (fromVscp == idxVscp || fromVscp == ਨਿਸ਼ਾਨਾ_Vscp) ਵਾਪਸ ਝੂਠ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        ਜੇ (static_cast<ਪੂਰਨ>(ਚੌੜਾਈ) != vecExprp->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVarRef* ਅਚਲ idx_ਵਿੱਚ_Sel = unwrap_ਮੰਜ਼ਿਲ_Var_Ref(selp->lsbp(), addr_ਬਿੱਟ);
        ਜੇ (!idx_ਵਿੱਚ_Sel || idx_ਵਿੱਚ_Sel->varScopep() != idxVscp) ਵਾਪਸ ਝੂਠ;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ਅਚਲ AstConst* ਅਚਲ ਨਿਸ਼ਾਨਾ_Initp = m_bindings.get(ਨਿਸ਼ਾਨਾ_Vscp);
        ਜੇ (!ਨਿਸ਼ਾਨਾ_Initp || !ਨਿਸ਼ਾਨਾ_Initp->isZero()) ਵਾਪਸ ਝੂਠ;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_ਸਤਰ* ਅਚਲ flp = loopp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* reducep;
        ਜੇ (isLeadingOne) {
            reducep = ਨਵਾਂ Ast_Most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1{flp, vecExprp->clone_ਰੁੱਖ(ਝੂਠ)};
        } ਨਹੀਂਤਾਂ {
            Ast_ਗਿਣਤੀ_Ones* ਅਚਲ conep = ਨਵਾਂ Ast_ਗਿਣਤੀ_Ones{flp, vecExprp->clone_ਰੁੱਖ(ਝੂਠ)};
            reducep = conep;
        }
        reducep = resize_ਮੰਜ਼ਿਲ_ਚੌੜਾਈ(reducep, ਨਿਸ਼ਾਨਾ_Refp);
        Ast_ਸੌਂਪੋ* ਅਚਲ newp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਿਸ਼ਾਨਾ_Refp->clone_ਰੁੱਖ(ਝੂਠ), reducep};
        newp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, incLhsp->clone_ਰੁੱਖ(ਝੂਠ), wp->clone_ਰੁੱਖ(ਝੂਠ)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(loopp), loopp);
        ਜੇ (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Lowered;
        } ਨਹੀਂਤਾਂ {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_ਗਿਣਤੀ_Ones_Lowered;
        }
        ਵਾਪਸ ਸੱਚ;
    }

    //ਦੇਖੋ
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        ਲਈ (Ast_ਗੰਢ *succp = nodep, *currp = nodep->backp(); ਸੱਚ;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            ਜੇ (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                ਜੇ (VN_IS(currp, Jump_ਖੰਡ)) ਜਾਰੀ;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                ਰੋਕੋ;
            }
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(currp, ਸੌਂਪੋ);
            ਜੇ (!assignp) ਰੋਕੋ;
            AstConst* ਅਚਲ valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            ਜੇ (!valp) ਰੋਕੋ;
            AstVarRef* ਅਚਲ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ਜੇ (!lhsp) ਰੋਕੋ;
            // Don't bind if volatile
            ਜੇ (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) ਜਾਰੀ;
            // Don't overwrite a later binding
            ਜੇ (m_bindings.get(lhsp->varScopep())) ਜਾਰੀ;
            // Set up the binding
            m_bindings.ਸੈੱਟ(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        ਜੇ (v3Global.opt.f_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Loops() && try_ਹੇਠਲਾ_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Loop(nodep)) ਵਾਪਸ;

        // Attempt to unroll this loop
        ਅਚਲ std::pair<Ast_ਗੰਢ*, ਤਾਰਕਿਕ> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        ਜੇ (!pair.ਦੂਜਾ) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        ਜੇ (pair.ਪਹਿਲਾ) {
            nodep->replaceWith(pair.ਪਹਿਲਾ);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    ਸਪਸ਼ਟ UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

ਖਾਲੀ V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_ਰੁੱਖ("unroll", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
