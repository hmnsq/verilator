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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Unroll.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

ସଂରଚନା UnrollStats final {
    ଶ୍ରେଣୀ Stat final {
        size_t m_ମୂଲ୍ୟ = 0;  // Statistics value
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ m_name;  // Name for stats file and UDEBUG

    ସର୍ବସାଧାରଣ:
        explicit Stat(ଅଚଳ ଅକ୍ଷର* ଅଚଳ name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_ମୂଲ୍ୟ); }
        ଅଚଳ ଅକ୍ଷର* name() ଅଚଳ { ଫେରନ୍ତୁ m_name; }
        Stat& operator++() {
            ++m_ମୂଲ୍ୟ;
            ଫେରନ୍ତୁ *ଏହି;
        }
        Stat& operator+=(size_t v) {
            m_ମୂଲ୍ୟ += v;
            ଫେରନ୍ତୁ *ଏହି;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_ଗଣନା{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_ଗଣନା_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
ଶ୍ରେଣୀ UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    UnrolllBindings() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~UnrolllBindings() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    ଶୂନ୍ୟ clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();
    }

    ଶୂନ୍ୟ checkpoint() { m_checkpoints.ଠେଲ_back(m_curr); }

    ଶୂନ୍ୟ revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.କାଢ଼_back();
    }

    ଶୂନ୍ୟ commit() { m_checkpoints.କାଢ଼_back(); }

    ଶୂନ୍ୟ set(AstVarScope* vscp, AstConst* valp) {
        ଯଦି (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.ଠେଲ_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        ଯଦି (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ଅଚଳ valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        ଫେରନ୍ତୁ valp;
    }
};

//######################################################################
// Unroll one AstLoop

ଶ୍ରେଣୀ UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ଅଚଳ m_loopp;  // The loop we are trying to unroll
    Ast_ଗଣ୍ଠି* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ଅଚଳ m_wrapp = ନୂତନ AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, ମିଥ୍ୟା};
    ଅଚଳ ତାର୍କିକ m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    ତାର୍କିକ m_ok = ସତ୍ୟ;  // Unrolling successful so far, gave up if false

    // METHODS
    ଶୂନ୍ୟ cantUnroll(Ast_ଗଣ୍ଠି* nodep, UnrollStats::Stat& stat) {
        m_ok = ମିଥ୍ୟା;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    ତାର୍କିକ unrollOneIteration(Ast_ଗଣ୍ଠି* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        ତାର୍କିକ foundLoopTest = ମିଥ୍ୟା;
        // Process one body statement at a time
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            ଯଦି (AstLoopTest* ଅଚଳ testp = VN_CAST(stmtp, LoopTest)) {
                Ast_ଗଣ୍ଠି* ଅଚଳ condp = V3Const::constifyEdit(testp->condp());
                ଯଦି (condp->isZero()) {
                    // Loop terminates
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                } ଅନ୍ୟଥା ଯଦି (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    ଜାରି;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            ଯଦି (!m_ok) ଫେରନ୍ତୁ ମିଥ୍ୟା;

            // Add statements to unrolled body
            ଯେତେବେଳେ (Ast_ଗଣ୍ଠି* ଅଚଳ nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                ଯଦି (!m_unrollFull) {
                    m_unrolledSize += nodep->ଗଣ୍ଠି_ଗଣନା();
                    ଯଦି (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        ଫେରନ୍ତୁ ମିଥ୍ୟା;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                ଯଦି (AstLoopTest* ଅଚଳ testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = ସତ୍ୟ;
                    // Will not actually need it, nor any subsequent
                    ଠେଲ_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    ଯଦି (testp->condp()->isNeqZero()) ଜାରି;
                    // Won't need any of the trailing statements
                    ଯଦି (m_wrapp->stmtsp()) ଠେଲ_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    ଯଦି (testp->condp()->isZero()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_ଗଣ୍ଠି::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                ଯଦି (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        ଯଦି (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        // One iteration done, loop continues
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    ତାର୍କିକ process(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        ଯଦି (!nodep) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_ଗଣ୍ଠି* np) {
            // Failed earlier
            ଯଦି (!m_ok) ଫେରନ୍ତୁ;
            // Check for AstLoopTest
            ଯଦି (AstLoopTest* ଅଚଳ testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                ଯଦି (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                ଫେରନ୍ତୁ;
            }
            // Check for AstFork - can't unroll
            ଯଦି (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                ଫେରନ୍ତୁ;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            ଯଦି (Ast_ଗଣ୍ଠି_CCall* ଅଚଳ callp = VN_CAST(np, ଗଣ୍ଠି_CCall)) {
                ଯଦି (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    ଫେରନ୍ତୁ;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            ଯଦି (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                ଫେରନ୍ତୁ;
            }
            // Process variable references
            AstVarRef* ଅଚଳ refp = VN_CAST(np, VarRef);
            ଯଦି (!refp) ଫେରନ୍ତୁ;
            // Ignore if the referenced variable has no binding
            AstConst* ଅଚଳ valp = m_bindings.get(refp->varScopep());
            ଯଦି (!valp) ଫେରନ୍ତୁ;
            // If writen, remove the binding
            ଯଦି (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                ଫେରନ୍ତୁ;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.ଠେଲ_back(refp);
        });
        // Give up if we have to
        ଯଦି (!m_ok) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Actually substitute the variables that still have bindings
        ପାଇଁ (AstVarRef* ଅଚଳ refp : toSubstitute) {
            // Pick up bound value
            AstConst* ଅଚଳ valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            ଯଦି (!valp) ଜାରି;
            // Substitute it
            refp->replaceWith(valp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
            VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        ଯଦି (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            ଫେରନ୍ତୁ;
        }
        // Attempt to unroll the loop
        ଅଚଳ size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_ଗଣନା();
        size_t iter_ଗଣନା = 0;
        କର {
            ଯଦି (iter_ଗଣନା > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_ଗଣନା);
                ଯଦି (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                ଫେରନ୍ତୁ;
            }
            ++iter_ଗଣନା;
        } ଯେତେବେଳେ (unrollOneIteration(loopp->stmtsp()));

        ଯଦି (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_ଗଣନା;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_ବୃକ୍ଷ(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (!m_ok) ଫେରନ୍ତୁ;
        // Generic body statement, just substitute
        process(nodep);
    }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override {
        ଯଦି (!m_ok) ଫେରନ୍ତୁ;
        // If the condition is a ExprStmt, move it before the LoopTest
        ଯଦି (Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ exprp = VN_CAST(nodep->condp(), ପଦସମଷ୍ଟି_Stmt)) {
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            ଫେରନ୍ତୁ;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    ଶୂନ୍ୟ visit(AstAssign* nodep) override {
        ଯଦି (!m_ok) ଫେରନ୍ତୁ;
        // Can't do it if delayed
        ଯଦି (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (!process(nodep->rhsp())) ଫେରନ୍ତୁ;
        // If a simple variable assignment, update the binding
        AstVarRef* ଅଚଳ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ଅଚଳ valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        ଯଦି (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            ଫେରନ୍ତୁ;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(AstIf* nodep) override {
        ଯଦି (!m_ok) ଫେରନ୍ତୁ;
        ଯଦି (!process(nodep->condp())) ଫେରନ୍ତୁ;
        // If condition is constant, replce with the relevant branch
        ଯଦି (AstConst* ଅଚଳ condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    ଶୂନ୍ୟ visit(AstJumpGo* nodep) override {
        ଯଦି (!m_ok) ଫେରନ୍ତୁ;
        // Remove trailing dead code
        ଯଦି (nodep->nextp()) ଠେଲ_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        ଯଦି (!m_ok) ଫେରନ୍ତୁ;
        m_bindings.checkpoint();
        std::pair<Ast_ଗଣ୍ଠି*, ତାର୍କିକ> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        ଯଦି (!pair.second) {
            m_bindings.revert();
            process(nodep);
            ଫେରନ୍ତୁ;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        ଯଦି (pair.first) {
            nodep->replaceWith(pair.first);
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

ସର୍ବସାଧାରଣ:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ସ୍ଥିର std::pair<Ast_ଗଣ୍ଠି*, ତାର୍କିକ> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        ଯଦି (visitor.m_ok) ଫେରନ୍ତୁ {visitor.m_stmtsp, ସତ୍ୟ};
        // Otherwise delete intermediate results
        ଯଦି (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_ବୃକ୍ଷ(), visitor.m_stmtsp);
        ଫେରନ୍ତୁ {nullptr, ମିଥ୍ୟା};
    }
};

//######################################################################
// Unroll all AstLoop statements

ଶ୍ରେଣୀ UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    ସ୍ଥିର AstVarRef* unwrapToVarRef(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା minWidth) {
        ଯେତେବେଳେ (ସତ୍ୟ) {
            ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) ଫେରନ୍ତୁ refp;
            ଯଦି (AstExtend* ଅଚଳ ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } ଅନ୍ୟଥା ଯଦି (AstExtendS* ଅଚଳ ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } ଅନ୍ୟଥା ଯଦି (AstSel* ଅଚଳ sp = VN_CAST(nodep, Sel)) {
                ଅଚଳ AstConst* ଅଚଳ lsbp = VN_CAST(sp->lsbp(), Const);
                ଯଦି (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) ଫେରନ୍ତୁ nullptr;
                nodep = sp->fromp();
            } ଅନ୍ୟଥା {
                ଫେରନ୍ତୁ nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    ତାର୍କିକ isVarPlus1(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ AstVarScope* vscp) {
        AstAdd* ଅଚଳ addp = VN_CAST(nodep, Add);
        ଯଦି (!addp || !addp->lhsp()->isOne()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVarRef* ଅଚଳ r = unwrapToVarRef(addp->rhsp(), addp->width());
        ଫେରନ୍ତୁ r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resizeToWidth(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ AstVarRef* targetRefp) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = targetRefp->width();
        ଯଦି (width == 32) ଫେରନ୍ତୁ exprp;
        File_ଧାଡ଼ି* ଅଚଳ flp = exprp->fileline();
        ଯଦି (width < 32) ଫେରନ୍ତୁ ନୂତନ AstSel{flp, exprp, 0, width};
        AstExtend* ଅଚଳ extp = ନୂତନ AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        ଫେରନ୍ତୁ extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ସ୍ଥିର ତାର୍କିକ ascendingBound(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        ଯଦି (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ bp = VN_AS(condp, ଗଣ୍ଠି_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        ଫେରନ୍ତୁ wp && idxRefp && !wp->ଅଙ୍କ().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ସ୍ଥିର ତାର୍କିକ isInRangeGuard(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, ଅଚଳ AstVarScope* idxVscp, uint32_t width,
                               ପୂର୍ଣ୍ଣସଂଖ୍ୟା addrBits) {
        ଯଦି (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି_Biop* ଅଚଳ bp = VN_AS(condp, ଗଣ୍ଠି_Biop);
        ଅଚଳ AstConst* ଅଚଳ cp = VN_CAST(bp->lhsp(), Const);
        ଯଦି (!cp || cp->ଅଙ୍କ().isFourState() || cp->toUInt() < width - 1) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVarRef* ଅଚଳ r = unwrapToVarRef(bp->rhsp(), addrBits);
        ଫେରନ୍ତୁ r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    ତାର୍କିକ tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* ଅଚଳ testp = VN_CAST(loopp->stmtsp(), LoopTest);
        ଯଦି (!testp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstIf* ଅଚଳ ifp = VN_CAST(testp->nextp(), If);
        ଯଦି (!ifp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstAssign* ଅଚଳ incp = VN_CAST(ifp->nextp(), Assign);
        ଯଦି (!incp || incp->nextp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        ଯଦି (!ascendingBound(testp->condp(), wp, idxRefp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstVarScope* ଅଚଳ idxVscp = idxRefp->varScopep();
        ଅଚଳ uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା addrBits = width <= 1 ? 1 : V3_ସଂଖ୍ୟା::log2b(width - 1) + 1;
        ଅଚଳ AstConst* ଅଚଳ idxInitp = m_bindings.get(idxVscp);
        ଯଦି (!idxInitp || !idxInitp->isZero()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstVarRef* ଅଚଳ incLhsp = VN_CAST(incp->lhsp(), VarRef);
        ଯଦି (!incLhsp || incLhsp->varScopep() != idxVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!isVarPlus1(incp->rhsp(), idxVscp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (ifp->elsesp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstAssign* ଅଚଳ thenp = VN_CAST(ifp->thensp(), Assign);
        ଯଦି (!thenp || thenp->nextp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstVarRef* ଅଚଳ targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        ଯଦି (!targetRefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstVarScope* ଅଚଳ targetVscp = targetRefp->varScopep();
        ଯଦି (targetVscp == idxVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ ତାର୍କିକ isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ଅଚଳ ତାର୍କିକ is_ଗଣନା_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        ଯଦି (!isLeadingOne && !is_ଗଣନା_Ones) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = ifp->condp();
        ଯଦି (AstLogAnd* ଅଚଳ andp = VN_CAST(condp, LogAnd)) {
            ଯଦି (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } ଅନ୍ୟଥା ଯଦି (AstCond* ଅଚଳ ternp = VN_CAST(condp, Cond)) {
            ଯଦି (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ଅଚଳ selp = VN_CAST(condp, Sel);
        ଯଦି (!selp || selp->width() != 1) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVarRef* ଅଚଳ fromp = VN_CAST(selp->fromp(), VarRef);
        ଯଦି (!fromp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVarScope* ଅଚଳ fromVscp = fromp->varScopep();
        ଯଦି (fromVscp == idxVscp || fromVscp == targetVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        ଯଦି (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(width) != vecExprp->width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVarRef* ଅଚଳ idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        ଯଦି (!idxInSel || idxInSel->varScopep() != idxVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ଅଚଳ AstConst* ଅଚଳ targetInitp = m_bindings.get(targetVscp);
        ଯଦି (!targetInitp || !targetInitp->isZero()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_ଧାଡ଼ି* ଅଚଳ flp = loopp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* reducep;
        ଯଦି (isLeadingOne) {
            reducep = ନୂତନ AstMostSetBitP1{flp, vecExprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
        } ଅନ୍ୟଥା {
            Ast_ଗଣନା_Ones* ଅଚଳ conep = ନୂତନ Ast_ଗଣନା_Ones{flp, vecExprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ଅଚଳ newp = ନୂତନ AstAssign{flp, targetRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), reducep};
        newp->addNext(ନୂତନ AstAssign{flp, incLhsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), wp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(loopp), loopp);
        ଯଦି (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } ଅନ୍ୟଥା {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_ଗଣନା_Ones_Lowered;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // VISIT
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        ପାଇଁ (Ast_ଗଣ୍ଠି *succp = nodep, *currp = nodep->backp(); ସତ୍ୟ;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            ଯଦି (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                ଯଦି (VN_IS(currp, JumpBlock)) ଜାରି;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                ଭାଙ୍ଗ;
            }
            AstAssign* ଅଚଳ assignp = VN_CAST(currp, Assign);
            ଯଦି (!assignp) ଭାଙ୍ଗ;
            AstConst* ଅଚଳ valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            ଯଦି (!valp) ଭାଙ୍ଗ;
            AstVarRef* ଅଚଳ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ଯଦି (!lhsp) ଭାଙ୍ଗ;
            // Don't bind if volatile
            ଯଦି (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) ଜାରି;
            // Don't overwrite a later binding
            ଯଦି (m_bindings.get(lhsp->varScopep())) ଜାରି;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        ଯଦି (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) ଫେରନ୍ତୁ;

        // Attempt to unroll this loop
        ଅଚଳ std::pair<Ast_ଗଣ୍ଠି*, ତାର୍କିକ> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        ଯଦି (!pair.second) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        ଯଦି (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା {
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

ଶୂନ୍ୟ V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_ବୃକ୍ଷ("unroll", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
