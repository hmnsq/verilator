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

ರಚನೆ UnrollStats final {
    ವರ್ಗ Stat final {
        ಗಾತ್ರ_t m_ಮೌಲ್ಯ = 0;  // Statistics value
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ m_name;  // Name for stats file and UDEBUG

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ Stat(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ name)
            : m_name{name} {}
        ~Stat() { V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Loop unrolling, "s + m_name, m_ಮೌಲ್ಯ); }
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }
        Stat& ಕ್ರಿಯಾಕಾರಕ++() {
            ++m_ಮೌಲ್ಯ;
            ಹಿಂತಿರುಗಿ *ಇದು;
        }
        Stat& ಕ್ರಿಯಾಕಾರಕ+=(ಗಾತ್ರ_t v) {
            m_ಮೌಲ್ಯ += v;
            ಹಿಂತಿರುಗಿ *ಇದು;
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
    Stat m_ಬಿಟ್_ಪರಿಶೀಲನೆ_Lowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_ಎಣಿಕೆ_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
ವರ್ಗ UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::ಸದಿಶ<AstConst*> m_curr{ಶೂನ್ಯಸೂಚಕ};
    // Stack of binding checkpoints
    std::ಸದಿಶ<std::ಸದಿಶ<AstConst*>> m_checkpoints;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    UnrolllBindings() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~UnrolllBindings() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    ಶೂನ್ಯ clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
    }

    ಶೂನ್ಯ checkpoint() { m_checkpoints.ತಳ್ಳು_ಹಿಂಭಾಗ(m_curr); }

    ಶೂನ್ಯ revert() {
        m_curr = std::move(m_checkpoints.ಹಿಂಭಾಗ());
        m_checkpoints.ತೆಗೆ_ಹಿಂಭಾಗ();
    }

    ಶೂನ್ಯ commit() { m_checkpoints.ತೆಗೆ_ಹಿಂಭಾಗ(); }

    ಶೂನ್ಯ ಗಣ(AstVarScope* vscp, AstConst* valp) {
        ಆದರೆ (!vscp->user1()) {
            vscp->user1(m_curr.ಗಾತ್ರ_2());
            m_curr.ತಳ್ಳು_ಹಿಂಭಾಗ(ಶೂನ್ಯಸೂಚಕ);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        ಆದರೆ (static_cast<ಗಾತ್ರ_t>(vscp->user1()) >= m_curr.ಗಾತ್ರ_2()) {
            m_curr.resize(vscp->user1() + 1, ಶೂನ್ಯಸೂಚಕ);
        }
        AstConst* ಸ್ಥಿರಾಂಕ valp = vscp->user1() ? m_curr[vscp->user1()] : ಶೂನ್ಯಸೂಚಕ;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        ಹಿಂತಿರುಗಿ valp;
    }
};

//######################################################################
// Unroll one AstLoop

ವರ್ಗ UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ಸ್ಥಿರಾಂಕ m_loopp;  // The loop we are trying to unroll
    Ast_ಗಂಟು* m_stmtsp = ಶೂನ್ಯಸೂಚಕ;  // Resulting unrolled statement
    ಗಾತ್ರ_t m_unrolled_ಗಾತ್ರ = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ಸ್ಥಿರಾಂಕ m_wrapp = ಹೊಸ AstBegin{m_loopp->fileline(), "[EditWrapper]", ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_unroll_ತುಂಬಿದ = m_loopp->unroll().is_ಗಣ_True();  // Completely unroll the loop?
    ತಾರ್ಕಿಕ m_ok = ನಿಜ;  // Unrolling successful so far, gave up if false

    // METHODS
    ಶೂನ್ಯ cantUnroll(Ast_ಗಂಟು* nodep, UnrollStats::Stat& stat) {
        m_ok = ಸುಳ್ಳು;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    ತಾರ್ಕಿಕ unrollOneIteration(Ast_ಗಂಟು* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು_Loop_Test = ಸುಳ್ಳು;
        // Process one body statement at a time
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            ಆದರೆ (AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(stmtp, LoopTest)) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ condp = V3Const::constifyEdit(testp->condp());
                ಆದರೆ (condp->isZero()) {
                    // Loop terminates
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    ಮುಂದುವರಿ;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->ಕೂಡಿಸು_Stmtsp(stmtp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
            iterate_And_ಮುಂದಿನ_Null(m_wrapp->stmtsp());
            // Give up if failed
            ಆದರೆ (!m_ok) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

            // Add statements to unrolled body
            ಆಗ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                ಆದರೆ (!m_unroll_ತುಂಬಿದ) {
                    m_unrolled_ಗಾತ್ರ += nodep->ಗಂಟು_ಎಣಿಕೆ();
                    ಆದರೆ (m_unrolled_ಗಾತ್ರ > static_cast<ಗಾತ್ರ_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlink_Fr_ಹಿಂಭಾಗ();

                // If a LoopTest, check how it resolved
                ಆದರೆ (AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(nodep, LoopTest)) {
                    ಸಿಕ್ಕಿತು_Loop_Test = ನಿಜ;
                    // Will not actually need it, nor any subsequent
                    ತಳ್ಳು_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    ಆದರೆ (testp->condp()->isNeqZero()) ಮುಂದುವರಿ;
                    // Won't need any of the trailing statements
                    ಆದರೆ (m_wrapp->stmtsp()) ತಳ್ಳು_Deletep(m_wrapp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    // Loop terminates
                    ಆದರೆ (testp->condp()->isZero()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                ಆದರೆ (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        ಆದರೆ (!ಸಿಕ್ಕಿತು_Loop_Test) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // One iteration done, loop continues
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    ತಾರ್ಕಿಕ process(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Variable references we should try to substitute
        std::ಸದಿಶ<AstVarRef*> ಗಮ್ಯ_Substitute;
        // Iterate subtree
        nodep->foreach([&](Ast_ಗಂಟು* np) {
            // Failed earlier
            ಆದರೆ (!m_ok) ಹಿಂತಿರುಗಿ;
            // Check for AstLoopTest
            ಆದರೆ (AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                ಆದರೆ (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                ಹಿಂತಿರುಗಿ;
            }
            // Check for AstFork - can't unroll
            ಆದರೆ (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                ಹಿಂತಿರುಗಿ;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            ಆದರೆ (Ast_ಗಂಟು_CCall* ಸ್ಥಿರಾಂಕ callp = VN_CAST(np, ಗಂಟು_CCall)) {
                ಆದರೆ (!callp->is_ಶುದ್ಧ()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            ಆದರೆ (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                ಹಿಂತಿರುಗಿ;
            }
            // Process variable references
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(np, VarRef);
            ಆದರೆ (!refp) ಹಿಂತಿರುಗಿ;
            // Ignore if the referenced variable has no binding
            AstConst* ಸ್ಥಿರಾಂಕ valp = m_bindings.get(refp->varScopep());
            ಆದರೆ (!valp) ಹಿಂತಿರುಗಿ;
            // If writen, remove the binding
            ಆದರೆ (refp->access().isWriteOrRW()) {
                m_bindings.ಗಣ(refp->varScopep(), ಶೂನ್ಯಸೂಚಕ);
                ಹಿಂತಿರುಗಿ;
            }
            // Otherwise add it to the list of variables to substitute
            ಗಮ್ಯ_Substitute.ತಳ್ಳು_ಹಿಂಭಾಗ(refp);
        });
        // Give up if we have to
        ಆದರೆ (!m_ok) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Actually substitute the variables that still have bindings
        ಗಾಗಿ (AstVarRef* ಸ್ಥಿರಾಂಕ refp : ಗಮ್ಯ_Substitute) {
            // Pick up bound value
            AstConst* ಸ್ಥಿರಾಂಕ valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            ಆದರೆ (!valp) ಮುಂದುವರಿ;
            // Substitute it
            refp->replaceWith(valp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        ಆದರೆ (loopp->unroll().is_ಗಣ_False()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            ಹಿಂತಿರುಗಿ;
        }
        // Attempt to unroll the loop
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t iter_ಎಲ್ಲೆ = m_unroll_ತುಂಬಿದ ? v3Global.opt.unroll_ಎಲ್ಲೆ()  //
                                              : v3Global.opt.unroll_ಎಣಿಕೆ();
        ಗಾತ್ರ_t iter_ಎಣಿಕೆ = 0;
        ಮಾಡು {
            ಆದರೆ (iter_ಎಣಿಕೆ > iter_ಎಲ್ಲೆ) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_ಎಣಿಕೆ);
                ಆದರೆ (m_unroll_ತುಂಬಿದ) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iter_ಎಲ್ಲೆ);
                }
                ಹಿಂತಿರುಗಿ;
            }
            ++iter_ಎಣಿಕೆ;
        } ಆಗ (unrollOneIteration(loopp->stmtsp()));

        ಆದರೆ (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_ಎಣಿಕೆ;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_ವೃಕ್ಷ(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (!m_ok) ಹಿಂತಿರುಗಿ;
        // Generic body statement, just substitute
        process(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override {
        ಆದರೆ (!m_ok) ಹಿಂತಿರುಗಿ;
        // If the condition is a ExprStmt, move it before the LoopTest
        ಆದರೆ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(nodep->condp(), ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = exprp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            exprp->replaceWith(exprp->resultp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
            stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(nodep);
            relinker.relink(stmtsp);
            ಹಿಂತಿರುಗಿ;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (!m_ok) ಹಿಂತಿರುಗಿ;
        // Can't do it if delayed
        ಆದರೆ (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (!process(nodep->rhsp())) ಹಿಂತಿರುಗಿ;
        // If a simple variable assignment, update the binding
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ಸ್ಥಿರಾಂಕ valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        ಆದರೆ (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.ಗಣ(lhsp->varScopep(), valp);
            ಹಿಂತಿರುಗಿ;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIf* nodep) override {
        ಆದರೆ (!m_ok) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!process(nodep->condp())) ಹಿಂತಿರುಗಿ;
        // If condition is constant, replce with the relevant branch
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(bodyp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());  // This will be iterated next
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstJumpGo* nodep) override {
        ಆದರೆ (!m_ok) ಹಿಂತಿರುಗಿ;
        // Remove trailing dead code
        ಆದರೆ (nodep->nextp()) ತಳ್ಳು_Deletep(nodep->nextp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        ಆದರೆ (!m_ok) ಹಿಂತಿರುಗಿ;
        m_bindings.checkpoint();
        std::pair<Ast_ಗಂಟು*, ತಾರ್ಕಿಕ> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        ಆದರೆ (!pair.ಎರಡನೇ) {
            m_bindings.revert();
            process(nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        ಆದರೆ (pair.ಮೊದಲ) {
            nodep->replaceWith(pair.ಮೊದಲ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ಸ್ಥಿರ std::pair<Ast_ಗಂಟು*, ತಾರ್ಕಿಕ> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        ಆದರೆ (visitor.m_ok) ಹಿಂತಿರುಗಿ {visitor.m_stmtsp, ನಿಜ};
        // Otherwise delete intermediate results
        ಆದರೆ (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_ವೃಕ್ಷ(), visitor.m_stmtsp);
        ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
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
    ಸ್ಥಿರ AstVarRef* unwrap_ಗಮ್ಯ_Var_Ref(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಂಕ min_ಅಗಲ) {
        ಆಗ (ನಿಜ) {
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) ಹಿಂತಿರುಗಿ refp;
            ಆದರೆ (AstExtend* ಸ್ಥಿರಾಂಕ ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstExtendS* ಸ್ಥಿರಾಂಕ ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ sp = VN_CAST(nodep, Sel)) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lsbp = VN_CAST(sp->lsbp(), Const);
                ಆದರೆ (!lsbp || lsbp->ಗಮ್ಯ_UInt() != 0 || sp->ಅಗಲ() < min_ಅಗಲ) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                nodep = sp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    ತಾರ್ಕಿಕ isVarPlus1(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ AstVarScope* vscp) {
        Ast_ಕೂಡಿಸು* ಸ್ಥಿರಾಂಕ addp = VN_CAST(nodep, ಕೂಡಿಸು_2);
        ಆದರೆ (!addp || !addp->lhsp()->isOne()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ r = unwrap_ಗಮ್ಯ_Var_Ref(addp->rhsp(), addp->ಅಗಲ());
        ಹಿಂತಿರುಗಿ r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resize_ಗಮ್ಯ_ಅಗಲ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ AstVarRef* ಗುರಿ_Refp) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = ಗುರಿ_Refp->ಅಗಲ();
        ಆದರೆ (ಅಗಲ == 32) ಹಿಂತಿರುಗಿ exprp;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
        ಆದರೆ (ಅಗಲ < 32) ಹಿಂತಿರುಗಿ ಹೊಸ AstSel{flp, exprp, 0, ಅಗಲ};
        AstExtend* ಸ್ಥಿರಾಂಕ extp = ಹೊಸ AstExtend{flp, exprp};
        extp->dtypeFrom(ಗುರಿ_Refp);
        ಹಿಂತಿರುಗಿ extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ascendingBound(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        ಆದರೆ (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ bp = VN_AS(condp, ಗಂಟು_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        ಹಿಂತಿರುಗಿ wp && idxRefp && !wp->ಅಂಕೆ().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_ಒಳ_Range_Guard(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, ಸ್ಥಿರಾಂಕ AstVarScope* idxVscp, uint32_t ಅಗಲ,
                               ಪೂರ್ಣಾಂಕ addr_ಬಿಟ್ಟುಗಳು) {
        ಆದರೆ (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ bp = VN_AS(condp, ಗಂಟು_Biop);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cp = VN_CAST(bp->lhsp(), Const);
        ಆದರೆ (!cp || cp->ಅಂಕೆ().isFourState() || cp->ಗಮ್ಯ_UInt() < ಅಗಲ - 1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ r = unwrap_ಗಮ್ಯ_Var_Ref(bp->rhsp(), addr_ಬಿಟ್ಟುಗಳು);
        ಹಿಂತಿರುಗಿ r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    ತಾರ್ಕಿಕ try_ಅಧೋ_ಬಿಟ್_ಪರಿಶೀಲನೆ_Loop(AstLoop* loopp) {
        AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(loopp->stmtsp(), LoopTest);
        ಆದರೆ (!testp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(testp->nextp(), If);
        ಆದರೆ (!ifp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ incp = VN_CAST(ifp->nextp(), ನಿಯೋಜಿಸು);
        ಆದರೆ (!incp || incp->nextp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstConst* wp = ಶೂನ್ಯಸೂಚಕ;
        AstVarRef* idxRefp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!ascendingBound(testp->condp(), wp, idxRefp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstVarScope* ಸ್ಥಿರಾಂಕ idxVscp = idxRefp->varScopep();
        ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = wp->ಗಮ್ಯ_UInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ addr_ಬಿಟ್ಟುಗಳು = ಅಗಲ <= 1 ? 1 : V3_ಸಂಖ್ಯೆ::log2b(ಅಗಲ - 1) + 1;
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ idxInitp = m_bindings.get(idxVscp);
        ಆದರೆ (!idxInitp || !idxInitp->isZero()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstVarRef* ಸ್ಥಿರಾಂಕ incLhsp = VN_CAST(incp->lhsp(), VarRef);
        ಆದರೆ (!incLhsp || incLhsp->varScopep() != idxVscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!isVarPlus1(incp->rhsp(), idxVscp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (ifp->elsesp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ thenp = VN_CAST(ifp->thensp(), ನಿಯೋಜಿಸು);
        ಆದರೆ (!thenp || thenp->nextp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstVarRef* ಸ್ಥಿರಾಂಕ ಗುರಿ_Refp = VN_CAST(thenp->lhsp(), VarRef);
        ಆದರೆ (!ಗುರಿ_Refp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstVarScope* ಸ್ಥಿರಾಂಕ ಗುರಿ_Vscp = ಗುರಿ_Refp->varScopep();
        ಆದರೆ (ಗುರಿ_Vscp == idxVscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ is_ಎಣಿಕೆ_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), ಗುರಿ_Vscp);
        ಆದರೆ (!isLeadingOne && !is_ಎಣಿಕೆ_Ones) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ifp->condp();
        ಆದರೆ (AstLogAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(condp, LogAnd)) {
            ಆದರೆ (is_ಒಳ_Range_Guard(andp->lhsp(), idxVscp, ಅಗಲ, addr_ಬಿಟ್ಟುಗಳು)) condp = andp->rhsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCond* ಸ್ಥಿರಾಂಕ ternp = VN_CAST(condp, Cond)) {
            ಆದರೆ (is_ಒಳ_Range_Guard(ternp->condp(), idxVscp, ಅಗಲ, addr_ಬಿಟ್ಟುಗಳು)) condp = ternp->thenp();
        }
        AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(condp, Sel);
        ಆದರೆ (!selp || selp->ಅಗಲ() != 1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ fromp = VN_CAST(selp->fromp(), VarRef);
        ಆದರೆ (!fromp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ fromVscp = fromp->varScopep();
        ಆದರೆ (fromVscp == idxVscp || fromVscp == ಗುರಿ_Vscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        ಆದರೆ (static_cast<ಪೂರ್ಣಾಂಕ>(ಅಗಲ) != vecExprp->ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ idx_ಒಳ_Sel = unwrap_ಗಮ್ಯ_Var_Ref(selp->lsbp(), addr_ಬಿಟ್ಟುಗಳು);
        ಆದರೆ (!idx_ಒಳ_Sel || idx_ಒಳ_Sel->varScopep() != idxVscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ ಗುರಿ_Initp = m_bindings.get(ಗುರಿ_Vscp);
        ಆದರೆ (!ಗುರಿ_Initp || !ಗುರಿ_Initp->isZero()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = loopp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* reducep;
        ಆದರೆ (isLeadingOne) {
            reducep = ಹೊಸ Ast_Most_ಗಣ_ಬಿಟ್_P1{flp, vecExprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಎಣಿಕೆ_Ones* ಸ್ಥಿರಾಂಕ conep = ಹೊಸ Ast_ಎಣಿಕೆ_Ones{flp, vecExprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
            reducep = conep;
        }
        reducep = resize_ಗಮ್ಯ_ಅಗಲ(reducep, ಗುರಿ_Refp);
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಗುರಿ_Refp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), reducep};
        newp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, incLhsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), wp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(loopp), loopp);
        ಆದರೆ (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_ಬಿಟ್_ಪರಿಶೀಲನೆ_Lowered;
        } ಇಲ್ಲದಿದ್ದರೆ {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_ಎಣಿಕೆ_Ones_Lowered;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    //ಭೇಟಿ
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        ಗಾಗಿ (Ast_ಗಂಟು *succp = nodep, *currp = nodep->backp(); ನಿಜ;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            ಆದರೆ (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                ಆದರೆ (VN_IS(currp, Jump_ಖಂಡ)) ಮುಂದುವರಿ;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                ನಿಲ್ಲಿಸು;
            }
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(currp, ನಿಯೋಜಿಸು);
            ಆದರೆ (!assignp) ನಿಲ್ಲಿಸು;
            AstConst* ಸ್ಥಿರಾಂಕ valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            ಆದರೆ (!valp) ನಿಲ್ಲಿಸು;
            AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ಆದರೆ (!lhsp) ನಿಲ್ಲಿಸು;
            // Don't bind if volatile
            ಆದರೆ (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) ಮುಂದುವರಿ;
            // Don't overwrite a later binding
            ಆದರೆ (m_bindings.get(lhsp->varScopep())) ಮುಂದುವರಿ;
            // Set up the binding
            m_bindings.ಗಣ(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        ಆದರೆ (v3Global.opt.f_ಬಿಟ್_ಪರಿಶೀಲನೆ_Loops() && try_ಅಧೋ_ಬಿಟ್_ಪರಿಶೀಲನೆ_Loop(nodep)) ಹಿಂತಿರುಗಿ;

        // Attempt to unroll this loop
        ಸ್ಥಿರಾಂಕ std::pair<Ast_ಗಂಟು*, ತಾರ್ಕಿಕ> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        ಆದರೆ (!pair.ಎರಡನೇ) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        ಆದರೆ (pair.ಮೊದಲ) {
            nodep->replaceWith(pair.ಮೊದಲ);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    ಸ್ಪಷ್ಟ UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

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
