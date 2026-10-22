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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Unroll.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

কাঠামো UnrollStats final {
    শ্রেণী Stat final {
        আকার_t m_মান = 0;  // Statistics value
        ধ্রুবক অক্ষর* ধ্রুবক m_name;  // Name for stats file and UDEBUG

    সর্বজনীন:
        explicit Stat(ধ্রুবক অক্ষর* ধ্রুবক name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_মান); }
        ধ্রুবক অক্ষর* name() ধ্রুবক { ফেরত m_name; }
        Stat& operator++() {
            ++m_মান;
            ফেরত *এই;
        }
        Stat& operator+=(আকার_t v) {
            m_মান += v;
            ফেরত *এই;
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
শ্রেণী UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ধ্রুবক VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

সর্বজনীন:
    // CONSTRUCTOR
    UnrolllBindings() = পূর্বনির্ধারিত;
    ~UnrolllBindings() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    শূন্য clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_গ্রন্থি::user1_Clear_বৃক্ষ();
    }

    শূন্য checkpoint() { m_checkpoints.ঠেলো_back(m_curr); }

    শূন্য revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.বের_করো_back();
    }

    শূন্য commit() { m_checkpoints.বের_করো_back(); }

    শূন্য set(AstVarScope* vscp, AstConst* valp) {
        যদি (!vscp->user1()) {
            vscp->user1(m_curr.আকার_2());
            m_curr.ঠেলো_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        যদি (static_cast<আকার_t>(vscp->user1()) >= m_curr.আকার_2()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ধ্রুবক valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        ফেরত valp;
    }
};

//######################################################################
// Unroll one AstLoop

শ্রেণী UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ধ্রুবক m_loopp;  // The loop we are trying to unroll
    Ast_গ্রন্থি* m_stmtsp = nullptr;  // Resulting unrolled statement
    আকার_t m_unrolled_আকার = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ধ্রুবক m_wrapp = নতুন AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, মিথ্যা};
    ধ্রুবক যৌক্তিক m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    যৌক্তিক m_ok = সত্য;  // Unrolling successful so far, gave up if false

    // METHODS
    শূন্য cantUnroll(Ast_গ্রন্থি* nodep, UnrollStats::Stat& stat) {
        m_ok = মিথ্যা;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    যৌক্তিক unrollOneIteration(Ast_গ্রন্থি* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        যৌক্তিক foundLoopTest = মিথ্যা;
        // Process one body statement at a time
        জন্য (Ast_গ্রন্থি* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            যদি (AstLoopTest* ধ্রুবক testp = VN_CAST(stmtp, LoopTest)) {
                Ast_গ্রন্থি* ধ্রুবক condp = V3Const::constifyEdit(testp->condp());
                যদি (condp->isZero()) {
                    // Loop terminates
                    ফেরত মিথ্যা;
                } অন্যথা যদি (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    চালু;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_বৃক্ষ(মিথ্যা));
            iterate_And_পরবর্তী_Null(m_wrapp->stmtsp());
            // Give up if failed
            যদি (!m_ok) ফেরত মিথ্যা;

            // Add statements to unrolled body
            যখন (Ast_গ্রন্থি* ধ্রুবক nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                যদি (!m_unrollFull) {
                    m_unrolled_আকার += nodep->গ্রন্থি_গুনতি();
                    যদি (m_unrolled_আকার > static_cast<আকার_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        ফেরত মিথ্যা;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                যদি (AstLoopTest* ধ্রুবক testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = সত্য;
                    // Will not actually need it, nor any subsequent
                    ঠেলো_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    যদি (testp->condp()->isNeqZero()) চালু;
                    // Won't need any of the trailing statements
                    যদি (m_wrapp->stmtsp()) ঠেলো_Deletep(m_wrapp->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
                    // Loop terminates
                    যদি (testp->condp()->isZero()) ফেরত মিথ্যা;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    ফেরত মিথ্যা;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_গ্রন্থি::add_পরবর্তী(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                যদি (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    ফেরত মিথ্যা;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        যদি (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            ফেরত মিথ্যা;
        }
        // One iteration done, loop continues
        ফেরত সত্য;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    যৌক্তিক process(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        যদি (!nodep) ফেরত সত্য;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_গ্রন্থি* np) {
            // Failed earlier
            যদি (!m_ok) ফেরত;
            // Check for AstLoopTest
            যদি (AstLoopTest* ধ্রুবক testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                যদি (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                ফেরত;
            }
            // Check for AstFork - can't unroll
            যদি (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                ফেরত;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            যদি (Ast_গ্রন্থি_CCall* ধ্রুবক callp = VN_CAST(np, গ্রন্থি_CCall)) {
                যদি (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    ফেরত;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            যদি (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                ফেরত;
            }
            // Process variable references
            AstVarRef* ধ্রুবক refp = VN_CAST(np, VarRef);
            যদি (!refp) ফেরত;
            // Ignore if the referenced variable has no binding
            AstConst* ধ্রুবক valp = m_bindings.get(refp->varScopep());
            যদি (!valp) ফেরত;
            // If writen, remove the binding
            যদি (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                ফেরত;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.ঠেলো_back(refp);
        });
        // Give up if we have to
        যদি (!m_ok) ফেরত মিথ্যা;
        // Actually substitute the variables that still have bindings
        জন্য (AstVarRef* ধ্রুবক refp : toSubstitute) {
            // Pick up bound value
            AstConst* ধ্রুবক valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            যদি (!valp) চালু;
            // Substitute it
            refp->replaceWith(valp->clone_বৃক্ষ(মিথ্যা));
            VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
        }
        ফেরত সত্য;
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
            ফেরত;
        }
        // Attempt to unroll the loop
        ধ্রুবক আকার_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_গুনতি();
        আকার_t iter_গুনতি = 0;
        করো {
            যদি (iter_গুনতি > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_গুনতি);
                যদি (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                ফেরত;
            }
            ++iter_গুনতি;
        } যখন (unrollOneIteration(loopp->stmtsp()));

        যদি (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_গুনতি;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_বৃক্ষ(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (!m_ok) ফেরত;
        // Generic body statement, just substitute
        process(nodep);
    }
    শূন্য visit(AstLoopTest* nodep) override {
        যদি (!m_ok) ফেরত;
        // If the condition is a ExprStmt, move it before the LoopTest
        যদি (Ast_রাশিমালা_Stmt* ধ্রুবক exprp = VN_CAST(nodep->condp(), রাশিমালা_Stmt)) {
            Ast_গ্রন্থি* ধ্রুবক stmtsp = exprp->stmtsp()->unlink_Fr_Back_With_পরবর্তী();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ঠেলো_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->add_পরবর্তী(nodep);
            relinker.relink(stmtsp);
            ফেরত;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    শূন্য visit(AstAssign* nodep) override {
        যদি (!m_ok) ফেরত;
        // Can't do it if delayed
        যদি (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            ফেরত;
        }
        যদি (!process(nodep->rhsp())) ফেরত;
        // If a simple variable assignment, update the binding
        AstVarRef* ধ্রুবক lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ধ্রুবক valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        যদি (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            ফেরত;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    শূন্য visit(AstIf* nodep) override {
        যদি (!m_ok) ফেরত;
        যদি (!process(nodep->condp())) ফেরত;
        // If condition is constant, replce with the relevant branch
        যদি (AstConst* ধ্রুবক condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            যদি (Ast_গ্রন্থি* ধ্রুবক bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->add_পরবর্তী_Here(bodyp->unlink_Fr_Back_With_পরবর্তী());  // This will be iterated next
            }
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    শূন্য visit(AstJumpGo* nodep) override {
        যদি (!m_ok) ফেরত;
        // Remove trailing dead code
        যদি (nodep->nextp()) ঠেলো_Deletep(nodep->nextp()->unlink_Fr_Back_With_পরবর্তী());
    }
    শূন্য visit(AstLoop* nodep) override {
        যদি (!m_ok) ফেরত;
        m_bindings.checkpoint();
        std::pair<Ast_গ্রন্থি*, যৌক্তিক> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        যদি (!pair.second) {
            m_bindings.revert();
            process(nodep);
            ফেরত;
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

সর্বজনীন:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    স্থির std::pair<Ast_গ্রন্থি*, যৌক্তিক> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        যদি (visitor.m_ok) ফেরত {visitor.m_stmtsp, সত্য};
        // Otherwise delete intermediate results
        যদি (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_বৃক্ষ(), visitor.m_stmtsp);
        ফেরত {nullptr, মিথ্যা};
    }
};

//######################################################################
// Unroll all AstLoop statements

শ্রেণী UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    স্থির AstVarRef* unwrapToVarRef(Ast_গ্রন্থি_রাশিমালা* nodep, পূর্ণসংখ্যা minWidth) {
        যখন (সত্য) {
            যদি (AstVarRef* ধ্রুবক refp = VN_CAST(nodep, VarRef)) ফেরত refp;
            যদি (AstExtend* ধ্রুবক ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } অন্যথা যদি (AstExtendS* ধ্রুবক ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } অন্যথা যদি (AstSel* ধ্রুবক sp = VN_CAST(nodep, Sel)) {
                ধ্রুবক AstConst* ধ্রুবক lsbp = VN_CAST(sp->lsbp(), Const);
                যদি (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) ফেরত nullptr;
                nodep = sp->fromp();
            } অন্যথা {
                ফেরত nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    যৌক্তিক isVarPlus1(Ast_গ্রন্থি* nodep, ধ্রুবক AstVarScope* vscp) {
        AstAdd* ধ্রুবক addp = VN_CAST(nodep, Add);
        যদি (!addp || !addp->lhsp()->isOne()) ফেরত মিথ্যা;
        ধ্রুবক AstVarRef* ধ্রুবক r = unwrapToVarRef(addp->rhsp(), addp->width());
        ফেরত r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    স্থির Ast_গ্রন্থি_রাশিমালা* resizeToWidth(Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুবক AstVarRef* targetRefp) {
        ধ্রুবক পূর্ণসংখ্যা width = targetRefp->width();
        যদি (width == 32) ফেরত exprp;
        File_সারি* ধ্রুবক flp = exprp->fileline();
        যদি (width < 32) ফেরত নতুন AstSel{flp, exprp, 0, width};
        AstExtend* ধ্রুবক extp = নতুন AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        ফেরত extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    স্থির যৌক্তিক ascendingBound(Ast_গ্রন্থি_রাশিমালা* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        যদি (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) ফেরত মিথ্যা;
        Ast_গ্রন্থি_Biop* ধ্রুবক bp = VN_AS(condp, গ্রন্থি_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        ফেরত wp && idxRefp && !wp->অঙ্ক().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    স্থির যৌক্তিক isInRangeGuard(Ast_গ্রন্থি_রাশিমালা* condp, ধ্রুবক AstVarScope* idxVscp, uint32_t width,
                               পূর্ণসংখ্যা addrBits) {
        যদি (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) ফেরত মিথ্যা;
        Ast_গ্রন্থি_Biop* ধ্রুবক bp = VN_AS(condp, গ্রন্থি_Biop);
        ধ্রুবক AstConst* ধ্রুবক cp = VN_CAST(bp->lhsp(), Const);
        যদি (!cp || cp->অঙ্ক().isFourState() || cp->toUInt() < width - 1) ফেরত মিথ্যা;
        ধ্রুবক AstVarRef* ধ্রুবক r = unwrapToVarRef(bp->rhsp(), addrBits);
        ফেরত r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    যৌক্তিক tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* ধ্রুবক testp = VN_CAST(loopp->stmtsp(), LoopTest);
        যদি (!testp) ফেরত মিথ্যা;
        AstIf* ধ্রুবক ifp = VN_CAST(testp->nextp(), If);
        যদি (!ifp) ফেরত মিথ্যা;
        AstAssign* ধ্রুবক incp = VN_CAST(ifp->nextp(), Assign);
        যদি (!incp || incp->nextp()) ফেরত মিথ্যা;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        যদি (!ascendingBound(testp->condp(), wp, idxRefp)) ফেরত মিথ্যা;
        AstVarScope* ধ্রুবক idxVscp = idxRefp->varScopep();
        ধ্রুবক uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ধ্রুবক পূর্ণসংখ্যা addrBits = width <= 1 ? 1 : V3_সংখ্যা::log2b(width - 1) + 1;
        ধ্রুবক AstConst* ধ্রুবক idxInitp = m_bindings.get(idxVscp);
        যদি (!idxInitp || !idxInitp->isZero()) ফেরত মিথ্যা;
        AstVarRef* ধ্রুবক incLhsp = VN_CAST(incp->lhsp(), VarRef);
        যদি (!incLhsp || incLhsp->varScopep() != idxVscp) ফেরত মিথ্যা;
        যদি (!isVarPlus1(incp->rhsp(), idxVscp)) ফেরত মিথ্যা;
        যদি (ifp->elsesp()) ফেরত মিথ্যা;
        AstAssign* ধ্রুবক thenp = VN_CAST(ifp->thensp(), Assign);
        যদি (!thenp || thenp->nextp()) ফেরত মিথ্যা;
        AstVarRef* ধ্রুবক targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        যদি (!targetRefp) ফেরত মিথ্যা;
        AstVarScope* ধ্রুবক targetVscp = targetRefp->varScopep();
        যদি (targetVscp == idxVscp) ফেরত মিথ্যা;
        ধ্রুবক যৌক্তিক isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ধ্রুবক যৌক্তিক is_গুনতি_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        যদি (!isLeadingOne && !is_গুনতি_Ones) ফেরত মিথ্যা;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_গ্রন্থি_রাশিমালা* condp = ifp->condp();
        যদি (AstLogAnd* ধ্রুবক andp = VN_CAST(condp, LogAnd)) {
            যদি (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } অন্যথা যদি (AstCond* ধ্রুবক ternp = VN_CAST(condp, Cond)) {
            যদি (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ধ্রুবক selp = VN_CAST(condp, Sel);
        যদি (!selp || selp->width() != 1) ফেরত মিথ্যা;
        ধ্রুবক AstVarRef* ধ্রুবক fromp = VN_CAST(selp->fromp(), VarRef);
        যদি (!fromp) ফেরত মিথ্যা;
        ধ্রুবক AstVarScope* ধ্রুবক fromVscp = fromp->varScopep();
        যদি (fromVscp == idxVscp || fromVscp == targetVscp) ফেরত মিথ্যা;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        যদি (static_cast<পূর্ণসংখ্যা>(width) != vecExprp->width()) ফেরত মিথ্যা;
        ধ্রুবক AstVarRef* ধ্রুবক idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        যদি (!idxInSel || idxInSel->varScopep() != idxVscp) ফেরত মিথ্যা;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ধ্রুবক AstConst* ধ্রুবক targetInitp = m_bindings.get(targetVscp);
        যদি (!targetInitp || !targetInitp->isZero()) ফেরত মিথ্যা;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_সারি* ধ্রুবক flp = loopp->fileline();
        Ast_গ্রন্থি_রাশিমালা* reducep;
        যদি (isLeadingOne) {
            reducep = নতুন AstMostSetBitP1{flp, vecExprp->clone_বৃক্ষ(মিথ্যা)};
        } অন্যথা {
            Ast_গুনতি_Ones* ধ্রুবক conep = নতুন Ast_গুনতি_Ones{flp, vecExprp->clone_বৃক্ষ(মিথ্যা)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ধ্রুবক newp = নতুন AstAssign{flp, targetRefp->clone_বৃক্ষ(মিথ্যা), reducep};
        newp->add_পরবর্তী(নতুন AstAssign{flp, incLhsp->clone_বৃক্ষ(মিথ্যা), wp->clone_বৃক্ষ(মিথ্যা)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(loopp), loopp);
        যদি (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } অন্যথা {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_গুনতি_Ones_Lowered;
        }
        ফেরত সত্য;
    }

    // VISIT
    শূন্য visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        জন্য (Ast_গ্রন্থি *succp = nodep, *currp = nodep->backp(); সত্য;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            যদি (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                যদি (VN_IS(currp, JumpBlock)) চালু;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                বিরতি;
            }
            AstAssign* ধ্রুবক assignp = VN_CAST(currp, Assign);
            যদি (!assignp) বিরতি;
            AstConst* ধ্রুবক valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            যদি (!valp) বিরতি;
            AstVarRef* ধ্রুবক lhsp = VN_CAST(assignp->lhsp(), VarRef);
            যদি (!lhsp) বিরতি;
            // Don't bind if volatile
            যদি (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) চালু;
            // Don't overwrite a later binding
            যদি (m_bindings.get(lhsp->varScopep())) চালু;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        যদি (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) ফেরত;

        // Attempt to unroll this loop
        ধ্রুবক std::pair<Ast_গ্রন্থি*, যৌক্তিক> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        যদি (!pair.second) {
            iterateChildren(nodep);
            ফেরত;
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
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

সর্বজনীন:
    স্থির শূন্য apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

শূন্য V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_বৃক্ষ("unroll", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
