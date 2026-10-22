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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Unroll.h"

#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

সংরচনা UnrollStats final {
    শ্ৰেণী Stat final {
        size_t m_মান = 0;  // Statistics value
        ধ্রুৱ ৱর্ণ* ধ্রুৱ m_name;  // Name for stats file and UDEBUG

    সার্ৱজনিক:
        explicit Stat(ধ্রুৱ ৱর্ণ* ধ্রুৱ name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_মান); }
        ধ্রুৱ ৱর্ণ* name() ধ্রুৱ { ঘূৰাই m_name; }
        Stat& operator++() {
            ++m_মান;
            ঘূৰাই *এতত্;
        }
        Stat& operator+=(size_t v) {
            m_মান += v;
            ঘূৰাই *এতত্;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_গুনতি{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_গুনতি_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
শ্ৰেণী UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ধ্রুৱ VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

সার্ৱজনিক:
    // CONSTRUCTOR
    UnrolllBindings() = স্ৱভাৱ;
    ~UnrolllBindings() = স্ৱভাৱ;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    শূন্য clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_গ্ৰন্থি::user1_Clear_বৃক্ষ();
    }

    শূন্য checkpoint() { m_checkpoints.ঠেলো_back(m_curr); }

    শূন্য revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.বেৰ_কৰো_back();
    }

    শূন্য commit() { m_checkpoints.বেৰ_কৰো_back(); }

    শূন্য set(AstVarScope* vscp, AstConst* valp) {
        যদি (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.ঠেলো_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        যদি (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ধ্রুৱ valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        ঘূৰাই valp;
    }
};

//######################################################################
// Unroll one AstLoop

শ্ৰেণী UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ধ্রুৱ m_loopp;  // The loop we are trying to unroll
    Ast_গ্ৰন্থি* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ধ্রুৱ m_wrapp = নূতন AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, মিছা};
    ধ্রুৱ তার্কিক m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    তার্কিক m_ok = সঁচা;  // Unrolling successful so far, gave up if false

    // METHODS
    শূন্য cantUnroll(Ast_গ্ৰন্থি* nodep, UnrollStats::Stat& stat) {
        m_ok = মিছা;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    তার্কিক unrollOneIteration(Ast_গ্ৰন্থি* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        তার্কিক foundLoopTest = মিছা;
        // Process one body statement at a time
        বাবে (Ast_গ্ৰন্থি* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            যদি (AstLoopTest* ধ্রুৱ testp = VN_CAST(stmtp, LoopTest)) {
                Ast_গ্ৰন্থি* ধ্রুৱ condp = V3Const::constifyEdit(testp->condp());
                যদি (condp->isZero()) {
                    // Loop terminates
                    ঘূৰাই মিছা;
                } অন্যথা যদি (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    অনুৱর্ত;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_বৃক্ষ(মিছা));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            যদি (!m_ok) ঘূৰাই মিছা;

            // Add statements to unrolled body
            যেতিয়ালৈ (Ast_গ্ৰন্থি* ধ্রুৱ nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                যদি (!m_unrollFull) {
                    m_unrolledSize += nodep->গ্ৰন্থি_গুনতি();
                    যদি (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        ঘূৰাই মিছা;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                যদি (AstLoopTest* ধ্রুৱ testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = সঁচা;
                    // Will not actually need it, nor any subsequent
                    ঠেলো_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    যদি (testp->condp()->isNeqZero()) অনুৱর্ত;
                    // Won't need any of the trailing statements
                    যদি (m_wrapp->stmtsp()) ঠেলো_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    যদি (testp->condp()->isZero()) ঘূৰাই মিছা;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    ঘূৰাই মিছা;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_গ্ৰন্থি::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                যদি (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    ঘূৰাই মিছা;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        যদি (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            ঘূৰাই মিছা;
        }
        // One iteration done, loop continues
        ঘূৰাই সঁচা;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    তার্কিক process(Ast_গ্ৰন্থি* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        যদি (!nodep) ঘূৰাই সঁচা;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_গ্ৰন্থি* np) {
            // Failed earlier
            যদি (!m_ok) ঘূৰাই;
            // Check for AstLoopTest
            যদি (AstLoopTest* ধ্রুৱ testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                যদি (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                ঘূৰাই;
            }
            // Check for AstFork - can't unroll
            যদি (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                ঘূৰাই;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            যদি (Ast_গ্ৰন্থি_CCall* ধ্রুৱ callp = VN_CAST(np, গ্ৰন্থি_CCall)) {
                যদি (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    ঘূৰাই;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            যদি (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                ঘূৰাই;
            }
            // Process variable references
            AstVarRef* ধ্রুৱ refp = VN_CAST(np, VarRef);
            যদি (!refp) ঘূৰাই;
            // Ignore if the referenced variable has no binding
            AstConst* ধ্রুৱ valp = m_bindings.get(refp->varScopep());
            যদি (!valp) ঘূৰাই;
            // If writen, remove the binding
            যদি (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                ঘূৰাই;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.ঠেলো_back(refp);
        });
        // Give up if we have to
        যদি (!m_ok) ঘূৰাই মিছা;
        // Actually substitute the variables that still have bindings
        বাবে (AstVarRef* ধ্রুৱ refp : toSubstitute) {
            // Pick up bound value
            AstConst* ধ্রুৱ valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            যদি (!valp) অনুৱর্ত;
            // Substitute it
            refp->replaceWith(valp->clone_বৃক্ষ(মিছা));
            VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
        }
        ঘূৰাই সঁচা;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        যদি (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            ঘূৰাই;
        }
        // Attempt to unroll the loop
        ধ্রুৱ size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_গুনতি();
        size_t iter_গুনতি = 0;
        কৰা {
            যদি (iter_গুনতি > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_গুনতি);
                যদি (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                ঘূৰাই;
            }
            ++iter_গুনতি;
        } যেতিয়ালৈ (unrollOneIteration(loopp->stmtsp()));

        যদি (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_গুনতি;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_বৃক্ষ(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        যদি (!m_ok) ঘূৰাই;
        // Generic body statement, just substitute
        process(nodep);
    }
    শূন্য visit(AstLoopTest* nodep) override {
        যদি (!m_ok) ঘূৰাই;
        // If the condition is a ExprStmt, move it before the LoopTest
        যদি (Ast_ৰাশিমালা_Stmt* ধ্রুৱ exprp = VN_CAST(nodep->condp(), ৰাশিমালা_Stmt)) {
            Ast_গ্ৰন্থি* ধ্রুৱ stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ঠেলো_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            ঘূৰাই;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    শূন্য visit(AstAssign* nodep) override {
        যদি (!m_ok) ঘূৰাই;
        // Can't do it if delayed
        যদি (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            ঘূৰাই;
        }
        যদি (!process(nodep->rhsp())) ঘূৰাই;
        // If a simple variable assignment, update the binding
        AstVarRef* ধ্রুৱ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ধ্রুৱ valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        যদি (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            ঘূৰাই;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    শূন্য visit(AstIf* nodep) override {
        যদি (!m_ok) ঘূৰাই;
        যদি (!process(nodep->condp())) ঘূৰাই;
        // If condition is constant, replce with the relevant branch
        যদি (AstConst* ধ্রুৱ condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            যদি (Ast_গ্ৰন্থি* ধ্রুৱ bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    শূন্য visit(AstJumpGo* nodep) override {
        যদি (!m_ok) ঘূৰাই;
        // Remove trailing dead code
        যদি (nodep->nextp()) ঠেলো_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    শূন্য visit(AstLoop* nodep) override {
        যদি (!m_ok) ঘূৰাই;
        m_bindings.checkpoint();
        std::pair<Ast_গ্ৰন্থি*, তার্কিক> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        যদি (!pair.second) {
            m_bindings.revert();
            process(nodep);
            ঘূৰাই;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        যদি (pair.first) {
            nodep->replaceWith(pair.first);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

সার্ৱজনিক:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    স্থির std::pair<Ast_গ্ৰন্থি*, তার্কিক> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        যদি (visitor.m_ok) ঘূৰাই {visitor.m_stmtsp, সঁচা};
        // Otherwise delete intermediate results
        যদি (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_বৃক্ষ(), visitor.m_stmtsp);
        ঘূৰাই {nullptr, মিছা};
    }
};

//######################################################################
// Unroll all AstLoop statements

শ্ৰেণী UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    স্থির AstVarRef* unwrapToVarRef(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, পূর্ণাঙ্ক minWidth) {
        যেতিয়ালৈ (সঁচা) {
            যদি (AstVarRef* ধ্রুৱ refp = VN_CAST(nodep, VarRef)) ঘূৰাই refp;
            যদি (AstExtend* ধ্রুৱ ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } অন্যথা যদি (AstExtendS* ধ্রুৱ ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } অন্যথা যদি (AstSel* ধ্রুৱ sp = VN_CAST(nodep, Sel)) {
                ধ্রুৱ AstConst* ধ্রুৱ lsbp = VN_CAST(sp->lsbp(), Const);
                যদি (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) ঘূৰাই nullptr;
                nodep = sp->fromp();
            } অন্যথা {
                ঘূৰাই nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    তার্কিক isVarPlus1(Ast_গ্ৰন্থি* nodep, ধ্রুৱ AstVarScope* vscp) {
        AstAdd* ধ্রুৱ addp = VN_CAST(nodep, Add);
        যদি (!addp || !addp->lhsp()->isOne()) ঘূৰাই মিছা;
        ধ্রুৱ AstVarRef* ধ্রুৱ r = unwrapToVarRef(addp->rhsp(), addp->width());
        ঘূৰাই r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* resizeToWidth(Ast_গ্ৰন্থি_ৰাশিমালা* exprp, ধ্রুৱ AstVarRef* targetRefp) {
        ধ্রুৱ পূর্ণাঙ্ক width = targetRefp->width();
        যদি (width == 32) ঘূৰাই exprp;
        File_সাৰি* ধ্রুৱ flp = exprp->fileline();
        যদি (width < 32) ঘূৰাই নূতন AstSel{flp, exprp, 0, width};
        AstExtend* ধ্রুৱ extp = নূতন AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        ঘূৰাই extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    স্থির তার্কিক ascendingBound(Ast_গ্ৰন্থি_ৰাশিমালা* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        যদি (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) ঘূৰাই মিছা;
        Ast_গ্ৰন্থি_Biop* ধ্রুৱ bp = VN_AS(condp, গ্ৰন্থি_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        ঘূৰাই wp && idxRefp && !wp->অঙ্ক().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    স্থির তার্কিক isInRangeGuard(Ast_গ্ৰন্থি_ৰাশিমালা* condp, ধ্রুৱ AstVarScope* idxVscp, uint32_t width,
                               পূর্ণাঙ্ক addrBits) {
        যদি (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) ঘূৰাই মিছা;
        Ast_গ্ৰন্থি_Biop* ধ্রুৱ bp = VN_AS(condp, গ্ৰন্থি_Biop);
        ধ্রুৱ AstConst* ধ্রুৱ cp = VN_CAST(bp->lhsp(), Const);
        যদি (!cp || cp->অঙ্ক().isFourState() || cp->toUInt() < width - 1) ঘূৰাই মিছা;
        ধ্রুৱ AstVarRef* ধ্রুৱ r = unwrapToVarRef(bp->rhsp(), addrBits);
        ঘূৰাই r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    তার্কিক tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* ধ্রুৱ testp = VN_CAST(loopp->stmtsp(), LoopTest);
        যদি (!testp) ঘূৰাই মিছা;
        AstIf* ধ্রুৱ ifp = VN_CAST(testp->nextp(), If);
        যদি (!ifp) ঘূৰাই মিছা;
        AstAssign* ধ্রুৱ incp = VN_CAST(ifp->nextp(), Assign);
        যদি (!incp || incp->nextp()) ঘূৰাই মিছা;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        যদি (!ascendingBound(testp->condp(), wp, idxRefp)) ঘূৰাই মিছা;
        AstVarScope* ধ্রুৱ idxVscp = idxRefp->varScopep();
        ধ্রুৱ uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ধ্রুৱ পূর্ণাঙ্ক addrBits = width <= 1 ? 1 : V3_সংখ্যা::log2b(width - 1) + 1;
        ধ্রুৱ AstConst* ধ্রুৱ idxInitp = m_bindings.get(idxVscp);
        যদি (!idxInitp || !idxInitp->isZero()) ঘূৰাই মিছা;
        AstVarRef* ধ্রুৱ incLhsp = VN_CAST(incp->lhsp(), VarRef);
        যদি (!incLhsp || incLhsp->varScopep() != idxVscp) ঘূৰাই মিছা;
        যদি (!isVarPlus1(incp->rhsp(), idxVscp)) ঘূৰাই মিছা;
        যদি (ifp->elsesp()) ঘূৰাই মিছা;
        AstAssign* ধ্রুৱ thenp = VN_CAST(ifp->thensp(), Assign);
        যদি (!thenp || thenp->nextp()) ঘূৰাই মিছা;
        AstVarRef* ধ্রুৱ targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        যদি (!targetRefp) ঘূৰাই মিছা;
        AstVarScope* ধ্রুৱ targetVscp = targetRefp->varScopep();
        যদি (targetVscp == idxVscp) ঘূৰাই মিছা;
        ধ্রুৱ তার্কিক isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ধ্রুৱ তার্কিক is_গুনতি_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        যদি (!isLeadingOne && !is_গুনতি_Ones) ঘূৰাই মিছা;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_গ্ৰন্থি_ৰাশিমালা* condp = ifp->condp();
        যদি (AstLogAnd* ধ্রুৱ andp = VN_CAST(condp, LogAnd)) {
            যদি (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } অন্যথা যদি (AstCond* ধ্রুৱ ternp = VN_CAST(condp, Cond)) {
            যদি (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ধ্রুৱ selp = VN_CAST(condp, Sel);
        যদি (!selp || selp->width() != 1) ঘূৰাই মিছা;
        ধ্রুৱ AstVarRef* ধ্রুৱ fromp = VN_CAST(selp->fromp(), VarRef);
        যদি (!fromp) ঘূৰাই মিছা;
        ধ্রুৱ AstVarScope* ধ্রুৱ fromVscp = fromp->varScopep();
        যদি (fromVscp == idxVscp || fromVscp == targetVscp) ঘূৰাই মিছা;
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        যদি (static_cast<পূর্ণাঙ্ক>(width) != vecExprp->width()) ঘূৰাই মিছা;
        ধ্রুৱ AstVarRef* ধ্রুৱ idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        যদি (!idxInSel || idxInSel->varScopep() != idxVscp) ঘূৰাই মিছা;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ধ্রুৱ AstConst* ধ্রুৱ targetInitp = m_bindings.get(targetVscp);
        যদি (!targetInitp || !targetInitp->isZero()) ঘূৰাই মিছা;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_সাৰি* ধ্রুৱ flp = loopp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* reducep;
        যদি (isLeadingOne) {
            reducep = নূতন AstMostSetBitP1{flp, vecExprp->clone_বৃক্ষ(মিছা)};
        } অন্যথা {
            Ast_গুনতি_Ones* ধ্রুৱ conep = নূতন Ast_গুনতি_Ones{flp, vecExprp->clone_বৃক্ষ(মিছা)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ধ্রুৱ newp = নূতন AstAssign{flp, targetRefp->clone_বৃক্ষ(মিছা), reducep};
        newp->addNext(নূতন AstAssign{flp, incLhsp->clone_বৃক্ষ(মিছা), wp->clone_বৃক্ষ(মিছা)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(loopp), loopp);
        যদি (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } অন্যথা {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_গুনতি_Ones_Lowered;
        }
        ঘূৰাই সঁচা;
    }

    // VISIT
    শূন্য visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        বাবে (Ast_গ্ৰন্থি *succp = nodep, *currp = nodep->backp(); সঁচা;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            যদি (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                যদি (VN_IS(currp, JumpBlock)) অনুৱর্ত;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                ভঙ্গ;
            }
            AstAssign* ধ্রুৱ assignp = VN_CAST(currp, Assign);
            যদি (!assignp) ভঙ্গ;
            AstConst* ধ্রুৱ valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            যদি (!valp) ভঙ্গ;
            AstVarRef* ধ্রুৱ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            যদি (!lhsp) ভঙ্গ;
            // Don't bind if volatile
            যদি (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) অনুৱর্ত;
            // Don't overwrite a later binding
            যদি (m_bindings.get(lhsp->varScopep())) অনুৱর্ত;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        যদি (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) ঘূৰাই;

        // Attempt to unroll this loop
        ধ্রুৱ std::pair<Ast_গ্ৰন্থি*, তার্কিক> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        যদি (!pair.second) {
            iterateChildren(nodep);
            ঘূৰাই;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        যদি (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা {
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

সার্ৱজনিক:
    স্থির শূন্য apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

শূন্য V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_বৃক্ষ("unroll", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
