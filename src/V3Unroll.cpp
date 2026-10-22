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
        سائيز_t m_قدر = 0;  // Statistics value
        ثابت حرف* ثابت m_name;  // Name for stats file and UDEBUG

    عام:
        واضح Stat(ثابت حرف* ثابت name)
            : m_name{name} {}
        ~Stat() { V3Stats::جوڙيو_Stat("Optimizations, Loop unrolling, "s + m_name, m_قدر); }
        ثابت حرف* name() ثابت { أرجع m_name; }
        Stat& عامل++() {
            ++m_قدر;
            أرجع *هذا;
        }
        Stat& عامل+=(سائيز_t v) {
            m_قدر += v;
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
    Stat m_n_Fail_Unroll_ڳڻپ{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_ٻٽو_انگ_جاچ_Lowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_ڳڻپ_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
صنف UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::ويڪٽر<AstConst*> m_curr{خالي_اشارو};
    // Stack of binding checkpoints
    std::ويڪٽر<std::ويڪٽر<AstConst*>> m_checkpoints;

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
        Ast_ڳنڍ::user1_Clear_وڻ();
    }

    فراغ checkpoint() { m_checkpoints.ڌڪو_پٺ(m_curr); }

    فراغ revert() {
        m_curr = std::move(m_checkpoints.پٺ());
        m_checkpoints.ڪڍو_پٺ();
    }

    فراغ commit() { m_checkpoints.ڪڍو_پٺ(); }

    فراغ سيٽ(AstVarScope* vscp, AstConst* valp) {
        إذا (!vscp->user1()) {
            vscp->user1(m_curr.سائيز());
            m_curr.ڌڪو_پٺ(خالي_اشارو);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        إذا (static_cast<سائيز_t>(vscp->user1()) >= m_curr.سائيز()) {
            m_curr.resize(vscp->user1() + 1, خالي_اشارو);
        }
        AstConst* ثابت valp = vscp->user1() ? m_curr[vscp->user1()] : خالي_اشارو;
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
    Ast_ڳنڍ* m_stmtsp = خالي_اشارو;  // Resulting unrolled statement
    سائيز_t m_unrolled_سائيز = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ثابت m_wrapp = جديد AstBegin{m_loopp->fileline(), "[EditWrapper]", خالي_اشارو, خطأ};
    ثابت منطقي m_unroll_ڀريل = m_loopp->unroll().is_سيٽ_True();  // Completely unroll the loop?
    منطقي m_ok = سچ;  // Unrolling successful so far, gave up if false

    // METHODS
    فراغ cantUnroll(Ast_ڳنڍ* nodep, UnrollStats::Stat& stat) {
        m_ok = خطأ;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    منطقي unrollOneIteration(Ast_ڳنڍ* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        منطقي مليو_Loop_Test = خطأ;
        // Process one body statement at a time
        لكل (Ast_ڳنڍ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            إذا (AstLoopTest* ثابت testp = VN_CAST(stmtp, LoopTest)) {
                Ast_ڳنڍ* ثابت condp = V3Const::constifyEdit(testp->condp());
                إذا (condp->isZero()) {
                    // Loop terminates
                    أرجع خطأ;
                } وإلا إذا (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    استمر;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->جوڙيو_Stmtsp(stmtp->clone_وڻ(خطأ));
            iterate_And_ايندڙ_Null(m_wrapp->stmtsp());
            // Give up if failed
            إذا (!m_ok) أرجع خطأ;

            // Add statements to unrolled body
            طالما (Ast_ڳنڍ* ثابت nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                إذا (!m_unroll_ڀريل) {
                    m_unrolled_سائيز += nodep->ڳنڍ_ڳڻپ();
                    إذا (m_unrolled_سائيز > static_cast<سائيز_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        أرجع خطأ;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlink_Fr_پٺ();

                // If a LoopTest, check how it resolved
                إذا (AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
                    مليو_Loop_Test = سچ;
                    // Will not actually need it, nor any subsequent
                    ڌڪو_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    إذا (testp->condp()->isNeqZero()) استمر;
                    // Won't need any of the trailing statements
                    إذا (m_wrapp->stmtsp()) ڌڪو_Deletep(m_wrapp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
                    // Loop terminates
                    إذا (testp->condp()->isZero()) أرجع خطأ;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    أرجع خطأ;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                إذا (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    أرجع خطأ;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        إذا (!مليو_Loop_Test) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            أرجع خطأ;
        }
        // One iteration done, loop continues
        أرجع سچ;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    منطقي process(Ast_ڳنڍ* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        إذا (!nodep) أرجع سچ;
        // Variable references we should try to substitute
        std::ويڪٽر<AstVarRef*> منزل_Substitute;
        // Iterate subtree
        nodep->foreach([&](Ast_ڳنڍ* np) {
            // Failed earlier
            إذا (!m_ok) أرجع;
            // Check for AstLoopTest
            إذا (AstLoopTest* ثابت testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                إذا (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                أرجع;
            }
            // Check for AstFork - can't unroll
            إذا (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                أرجع;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            إذا (Ast_ڳنڍ_CCall* ثابت callp = VN_CAST(np, ڳنڍ_CCall)) {
                إذا (!callp->is_خالص()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    أرجع;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            إذا (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                أرجع;
            }
            // Process variable references
            AstVarRef* ثابت refp = VN_CAST(np, VarRef);
            إذا (!refp) أرجع;
            // Ignore if the referenced variable has no binding
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            إذا (!valp) أرجع;
            // If writen, remove the binding
            إذا (refp->access().isWriteOrRW()) {
                m_bindings.سيٽ(refp->varScopep(), خالي_اشارو);
                أرجع;
            }
            // Otherwise add it to the list of variables to substitute
            منزل_Substitute.ڌڪو_پٺ(refp);
        });
        // Give up if we have to
        إذا (!m_ok) أرجع خطأ;
        // Actually substitute the variables that still have bindings
        لكل (AstVarRef* ثابت refp : منزل_Substitute) {
            // Pick up bound value
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            إذا (!valp) استمر;
            // Substitute it
            refp->replaceWith(valp->clone_وڻ(خطأ));
            VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
        }
        أرجع سچ;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        إذا (loopp->unroll().is_سيٽ_False()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            أرجع;
        }
        // Attempt to unroll the loop
        ثابت سائيز_t iter_حد_بندي = m_unroll_ڀريل ? v3Global.opt.unroll_حد_بندي()  //
                                              : v3Global.opt.unroll_ڳڻپ();
        سائيز_t iter_ڳڻپ = 0;
        افعل {
            إذا (iter_ڳڻپ > iter_حد_بندي) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_ڳڻپ);
                إذا (m_unroll_ڀريل) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iter_حد_بندي);
                }
                أرجع;
            }
            ++iter_ڳڻپ;
        } طالما (unrollOneIteration(loopp->stmtsp()));

        إذا (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_ڳڻپ;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_وڻ(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (!m_ok) أرجع;
        // Generic body statement, just substitute
        process(nodep);
    }
    فراغ گهمو(AstLoopTest* nodep) override {
        إذا (!m_ok) أرجع;
        // If the condition is a ExprStmt, move it before the LoopTest
        إذا (Ast_اظهار_Stmt* ثابت exprp = VN_CAST(nodep->condp(), اظهار_Stmt)) {
            Ast_ڳنڍ* ثابت stmtsp = exprp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ();
            exprp->replaceWith(exprp->resultp()->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlink_Fr_پٺ(&relinker);
            stmtsp->جوڙيو_ايندڙ(nodep);
            relinker.relink(stmtsp);
            أرجع;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    فراغ گهمو(Ast_سونپيو* nodep) override {
        إذا (!m_ok) أرجع;
        // Can't do it if delayed
        إذا (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            أرجع;
        }
        إذا (!process(nodep->rhsp())) أرجع;
        // If a simple variable assignment, update the binding
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        إذا (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.سيٽ(lhsp->varScopep(), valp);
            أرجع;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    فراغ گهمو(AstIf* nodep) override {
        إذا (!m_ok) أرجع;
        إذا (!process(nodep->condp())) أرجع;
        // If condition is constant, replce with the relevant branch
        إذا (AstConst* ثابت condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            إذا (Ast_ڳنڍ* ثابت bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->جوڙيو_ايندڙ_هتي(bodyp->unlink_Fr_پٺ_With_ايندڙ());  // This will be iterated next
            }
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    فراغ گهمو(AstJumpGo* nodep) override {
        إذا (!m_ok) أرجع;
        // Remove trailing dead code
        إذا (nodep->nextp()) ڌڪو_Deletep(nodep->nextp()->unlink_Fr_پٺ_With_ايندڙ());
    }
    فراغ گهمو(AstLoop* nodep) override {
        إذا (!m_ok) أرجع;
        m_bindings.checkpoint();
        std::pair<Ast_ڳنڍ*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        إذا (!pair.ٻيو) {
            m_bindings.revert();
            process(nodep);
            أرجع;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        إذا (pair.پهريون) {
            nodep->replaceWith(pair.پهريون);
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

عام:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ساكن std::pair<Ast_ڳنڍ*, منطقي> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        إذا (visitor.m_ok) أرجع {visitor.m_stmtsp, سچ};
        // Otherwise delete intermediate results
        إذا (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_وڻ(), visitor.m_stmtsp);
        أرجع {خالي_اشارو, خطأ};
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
    ساكن AstVarRef* unwrap_منزل_Var_Ref(Ast_ڳنڍ_اظهار* nodep, صحيح min_ويڪر) {
        طالما (سچ) {
            إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) أرجع refp;
            إذا (AstExtend* ثابت ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } وإلا إذا (AstExtendS* ثابت ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } وإلا إذا (AstSel* ثابت sp = VN_CAST(nodep, Sel)) {
                ثابت AstConst* ثابت lsbp = VN_CAST(sp->lsbp(), Const);
                إذا (!lsbp || lsbp->منزل_UInt() != 0 || sp->ويڪر() < min_ويڪر) أرجع خالي_اشارو;
                nodep = sp->fromp();
            } وإلا {
                أرجع خالي_اشارو;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    منطقي isVarPlus1(Ast_ڳنڍ* nodep, ثابت AstVarScope* vscp) {
        Ast_جوڙيو* ثابت addp = VN_CAST(nodep, جوڙيو_2);
        إذا (!addp || !addp->lhsp()->isOne()) أرجع خطأ;
        ثابت AstVarRef* ثابت r = unwrap_منزل_Var_Ref(addp->rhsp(), addp->ويڪر());
        أرجع r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ساكن Ast_ڳنڍ_اظهار* resize_منزل_ويڪر(Ast_ڳنڍ_اظهار* exprp, ثابت AstVarRef* هدف_Refp) {
        ثابت صحيح ويڪر = هدف_Refp->ويڪر();
        إذا (ويڪر == 32) أرجع exprp;
        File_سٽ* ثابت flp = exprp->fileline();
        إذا (ويڪر < 32) أرجع جديد AstSel{flp, exprp, 0, ويڪر};
        AstExtend* ثابت extp = جديد AstExtend{flp, exprp};
        extp->dtypeFrom(هدف_Refp);
        أرجع extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ساكن منطقي ascendingBound(Ast_ڳنڍ_اظهار* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        إذا (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) أرجع خطأ;
        Ast_ڳنڍ_Biop* ثابت bp = VN_AS(condp, ڳنڍ_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        أرجع wp && idxRefp && !wp->انگ().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ساكن منطقي is_اندريون_Range_Guard(Ast_ڳنڍ_اظهار* condp, ثابت AstVarScope* idxVscp, uint32_t ويڪر,
                               صحيح addr_بٽ) {
        إذا (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) أرجع خطأ;
        Ast_ڳنڍ_Biop* ثابت bp = VN_AS(condp, ڳنڍ_Biop);
        ثابت AstConst* ثابت cp = VN_CAST(bp->lhsp(), Const);
        إذا (!cp || cp->انگ().isFourState() || cp->منزل_UInt() < ويڪر - 1) أرجع خطأ;
        ثابت AstVarRef* ثابت r = unwrap_منزل_Var_Ref(bp->rhsp(), addr_بٽ);
        أرجع r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    منطقي try_هيٺاهون_ٻٽو_انگ_جاچ_Loop(AstLoop* loopp) {
        AstLoopTest* ثابت testp = VN_CAST(loopp->stmtsp(), LoopTest);
        إذا (!testp) أرجع خطأ;
        AstIf* ثابت ifp = VN_CAST(testp->nextp(), If);
        إذا (!ifp) أرجع خطأ;
        Ast_سونپيو* ثابت incp = VN_CAST(ifp->nextp(), سونپيو);
        إذا (!incp || incp->nextp()) أرجع خطأ;
        AstConst* wp = خالي_اشارو;
        AstVarRef* idxRefp = خالي_اشارو;
        إذا (!ascendingBound(testp->condp(), wp, idxRefp)) أرجع خطأ;
        AstVarScope* ثابت idxVscp = idxRefp->varScopep();
        ثابت uint32_t ويڪر = wp->منزل_UInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ثابت صحيح addr_بٽ = ويڪر <= 1 ? 1 : V3_عدد::log2b(ويڪر - 1) + 1;
        ثابت AstConst* ثابت idxInitp = m_bindings.get(idxVscp);
        إذا (!idxInitp || !idxInitp->isZero()) أرجع خطأ;
        AstVarRef* ثابت incLhsp = VN_CAST(incp->lhsp(), VarRef);
        إذا (!incLhsp || incLhsp->varScopep() != idxVscp) أرجع خطأ;
        إذا (!isVarPlus1(incp->rhsp(), idxVscp)) أرجع خطأ;
        إذا (ifp->elsesp()) أرجع خطأ;
        Ast_سونپيو* ثابت thenp = VN_CAST(ifp->thensp(), سونپيو);
        إذا (!thenp || thenp->nextp()) أرجع خطأ;
        AstVarRef* ثابت هدف_Refp = VN_CAST(thenp->lhsp(), VarRef);
        إذا (!هدف_Refp) أرجع خطأ;
        AstVarScope* ثابت هدف_Vscp = هدف_Refp->varScopep();
        إذا (هدف_Vscp == idxVscp) أرجع خطأ;
        ثابت منطقي isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ثابت منطقي is_ڳڻپ_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), هدف_Vscp);
        إذا (!isLeadingOne && !is_ڳڻپ_Ones) أرجع خطأ;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_ڳنڍ_اظهار* condp = ifp->condp();
        إذا (AstLogAnd* ثابت andp = VN_CAST(condp, LogAnd)) {
            إذا (is_اندريون_Range_Guard(andp->lhsp(), idxVscp, ويڪر, addr_بٽ)) condp = andp->rhsp();
        } وإلا إذا (AstCond* ثابت ternp = VN_CAST(condp, Cond)) {
            إذا (is_اندريون_Range_Guard(ternp->condp(), idxVscp, ويڪر, addr_بٽ)) condp = ternp->thenp();
        }
        AstSel* ثابت selp = VN_CAST(condp, Sel);
        إذا (!selp || selp->ويڪر() != 1) أرجع خطأ;
        ثابت AstVarRef* ثابت fromp = VN_CAST(selp->fromp(), VarRef);
        إذا (!fromp) أرجع خطأ;
        ثابت AstVarScope* ثابت fromVscp = fromp->varScopep();
        إذا (fromVscp == idxVscp || fromVscp == هدف_Vscp) أرجع خطأ;
        Ast_ڳنڍ_اظهار* ثابت vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        إذا (static_cast<صحيح>(ويڪر) != vecExprp->ويڪر()) أرجع خطأ;
        ثابت AstVarRef* ثابت idx_اندريون_Sel = unwrap_منزل_Var_Ref(selp->lsbp(), addr_بٽ);
        إذا (!idx_اندريون_Sel || idx_اندريون_Sel->varScopep() != idxVscp) أرجع خطأ;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ثابت AstConst* ثابت هدف_Initp = m_bindings.get(هدف_Vscp);
        إذا (!هدف_Initp || !هدف_Initp->isZero()) أرجع خطأ;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_سٽ* ثابت flp = loopp->fileline();
        Ast_ڳنڍ_اظهار* reducep;
        إذا (isLeadingOne) {
            reducep = جديد Ast_Most_سيٽ_ٻٽو_انگ_P1{flp, vecExprp->clone_وڻ(خطأ)};
        } وإلا {
            Ast_ڳڻپ_Ones* ثابت conep = جديد Ast_ڳڻپ_Ones{flp, vecExprp->clone_وڻ(خطأ)};
            reducep = conep;
        }
        reducep = resize_منزل_ويڪر(reducep, هدف_Refp);
        Ast_سونپيو* ثابت newp = جديد Ast_سونپيو{flp, هدف_Refp->clone_وڻ(خطأ), reducep};
        newp->جوڙيو_ايندڙ(جديد Ast_سونپيو{flp, incLhsp->clone_وڻ(خطأ), wp->clone_وڻ(خطأ)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(loopp), loopp);
        إذا (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_ٻٽو_انگ_جاچ_Lowered;
        } وإلا {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_ڳڻپ_Ones_Lowered;
        }
        أرجع سچ;
    }

    //گهمو
    فراغ گهمو(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        لكل (Ast_ڳنڍ *succp = nodep, *currp = nodep->backp(); سچ;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            إذا (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                إذا (VN_IS(currp, Jump_بلاڪ)) استمر;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                اكسر;
            }
            Ast_سونپيو* ثابت assignp = VN_CAST(currp, سونپيو);
            إذا (!assignp) اكسر;
            AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            إذا (!valp) اكسر;
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            إذا (!lhsp) اكسر;
            // Don't bind if volatile
            إذا (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) استمر;
            // Don't overwrite a later binding
            إذا (m_bindings.get(lhsp->varScopep())) استمر;
            // Set up the binding
            m_bindings.سيٽ(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        إذا (v3Global.opt.f_ٻٽو_انگ_جاچ_Loops() && try_هيٺاهون_ٻٽو_انگ_جاچ_Loop(nodep)) أرجع;

        // Attempt to unroll this loop
        ثابت std::pair<Ast_ڳنڍ*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        إذا (!pair.ٻيو) {
            iterateChildren(nodep);
            أرجع;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        إذا (pair.پهريون) {
            nodep->replaceWith(pair.پهريون);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        } وإلا {
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    واضح UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

عام:
    ساكن فراغ apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

فراغ V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_وڻ("unroll", 0, dump_وڻ_Either_Level() >= 3);
}
