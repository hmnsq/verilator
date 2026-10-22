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
        ناپ_t m_قدر = 0;  // Statistics value
        ثابت حرف* ثابت m_name;  // Name for stats file and UDEBUG

    عام:
        واضح Stat(ثابت حرف* ثابت name)
            : m_name{name} {}
        ~Stat() { V3Stats::جوڑو_Stat("Optimizations, Loop unrolling, "s + m_name, m_قدر); }
        ثابت حرف* name() ثابت { أرجع m_name; }
        Stat& عامل++() {
            ++m_قدر;
            أرجع *هذا;
        }
        Stat& عامل+=(ناپ_t v) {
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
    Stat m_n_Fail_Unroll_گنتی{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_ثنائی_ہندسہ_جانچ_Lowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_گنتی_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
صنف UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ثابت VNUser1_وچ_Use m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::سمتیہ<AstConst*> m_curr{خالی_سوچک};
    // Stack of binding checkpoints
    std::سمتیہ<std::سمتیہ<AstConst*>> m_checkpoints;

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
        Ast_گرہ::user1_Clear_درخت();
    }

    فراغ checkpoint() { m_checkpoints.ڈالو_پٹھ(m_curr); }

    فراغ revert() {
        m_curr = std::move(m_checkpoints.پٹھ());
        m_checkpoints.نکالو_پٹھ();
    }

    فراغ commit() { m_checkpoints.نکالو_پٹھ(); }

    فراغ سیٹ(AstVarScope* vscp, AstConst* valp) {
        إذا (!vscp->user1()) {
            vscp->user1(m_curr.ناپ());
            m_curr.ڈالو_پٹھ(خالی_سوچک);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        إذا (static_cast<ناپ_t>(vscp->user1()) >= m_curr.ناپ()) {
            m_curr.resize(vscp->user1() + 1, خالی_سوچک);
        }
        AstConst* ثابت valp = vscp->user1() ? m_curr[vscp->user1()] : خالی_سوچک;
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
    Ast_گرہ* m_stmtsp = خالی_سوچک;  // Resulting unrolled statement
    ناپ_t m_unrolled_ناپ = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ثابت m_wrapp = جديد AstBegin{m_loopp->fileline(), "[EditWrapper]", خالی_سوچک, خطأ};
    ثابت منطقي m_unroll_بھریا = m_loopp->unroll().is_سیٹ_True();  // Completely unroll the loop?
    منطقي m_ok = سچ;  // Unrolling successful so far, gave up if false

    // METHODS
    فراغ cantUnroll(Ast_گرہ* nodep, UnrollStats::Stat& stat) {
        m_ok = خطأ;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    منطقي unrollOneIteration(Ast_گرہ* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        منطقي لبھیا_Loop_Test = خطأ;
        // Process one body statement at a time
        لكل (Ast_گرہ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            إذا (AstLoopTest* ثابت testp = VN_CAST(stmtp, LoopTest)) {
                Ast_گرہ* ثابت condp = V3Const::constifyEdit(testp->condp());
                إذا (condp->isZero()) {
                    // Loop terminates
                    أرجع خطأ;
                } وإلا إذا (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    استمر;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->جوڑو_Stmtsp(stmtp->clone_درخت(خطأ));
            iterate_And_اگلا_Null(m_wrapp->stmtsp());
            // Give up if failed
            إذا (!m_ok) أرجع خطأ;

            // Add statements to unrolled body
            طالما (Ast_گرہ* ثابت nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                إذا (!m_unroll_بھریا) {
                    m_unrolled_ناپ += nodep->گرہ_گنتی();
                    إذا (m_unrolled_ناپ > static_cast<ناپ_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        أرجع خطأ;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlink_Fr_پٹھ();

                // If a LoopTest, check how it resolved
                إذا (AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
                    لبھیا_Loop_Test = سچ;
                    // Will not actually need it, nor any subsequent
                    ڈالو_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    إذا (testp->condp()->isNeqZero()) استمر;
                    // Won't need any of the trailing statements
                    إذا (m_wrapp->stmtsp()) ڈالو_Deletep(m_wrapp->stmtsp()->unlink_Fr_پٹھ_With_اگلا());
                    // Loop terminates
                    إذا (testp->condp()->isZero()) أرجع خطأ;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    أرجع خطأ;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_گرہ::جوڑو_اگلا(m_stmtsp, nodep);

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
        إذا (!لبھیا_Loop_Test) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            أرجع خطأ;
        }
        // One iteration done, loop continues
        أرجع سچ;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    منطقي process(Ast_گرہ* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        إذا (!nodep) أرجع سچ;
        // Variable references we should try to substitute
        std::سمتیہ<AstVarRef*> منزل_Substitute;
        // Iterate subtree
        nodep->foreach([&](Ast_گرہ* np) {
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
            إذا (Ast_گرہ_CCall* ثابت callp = VN_CAST(np, گرہ_CCall)) {
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
                m_bindings.سیٹ(refp->varScopep(), خالی_سوچک);
                أرجع;
            }
            // Otherwise add it to the list of variables to substitute
            منزل_Substitute.ڈالو_پٹھ(refp);
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
            refp->replaceWith(valp->clone_درخت(خطأ));
            VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
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
        إذا (loopp->unroll().is_سیٹ_False()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            أرجع;
        }
        // Attempt to unroll the loop
        ثابت ناپ_t iter_حد_بندی = m_unroll_بھریا ? v3Global.opt.unroll_حد_بندی()  //
                                              : v3Global.opt.unroll_گنتی();
        ناپ_t iter_گنتی = 0;
        افعل {
            إذا (iter_گنتی > iter_حد_بندی) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_گنتی);
                إذا (m_unroll_بھریا) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iter_حد_بندی);
                }
                أرجع;
            }
            ++iter_گنتی;
        } طالما (unrollOneIteration(loopp->stmtsp()));

        إذا (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_گنتی;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_درخت(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    فراغ ویکھو(Ast_گرہ* nodep) override {
        إذا (!m_ok) أرجع;
        // Generic body statement, just substitute
        process(nodep);
    }
    فراغ ویکھو(AstLoopTest* nodep) override {
        إذا (!m_ok) أرجع;
        // If the condition is a ExprStmt, move it before the LoopTest
        إذا (Ast_اظہاریہ_Stmt* ثابت exprp = VN_CAST(nodep->condp(), اظہاریہ_Stmt)) {
            Ast_گرہ* ثابت stmtsp = exprp->stmtsp()->unlink_Fr_پٹھ_With_اگلا();
            exprp->replaceWith(exprp->resultp()->unlink_Fr_پٹھ());
            VL_DO_DANGLING(ڈالو_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlink_Fr_پٹھ(&relinker);
            stmtsp->جوڑو_اگلا(nodep);
            relinker.relink(stmtsp);
            أرجع;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    فراغ ویکھو(Ast_سونپو* nodep) override {
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
            m_bindings.سیٹ(lhsp->varScopep(), valp);
            أرجع;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    فراغ ویکھو(AstIf* nodep) override {
        إذا (!m_ok) أرجع;
        إذا (!process(nodep->condp())) أرجع;
        // If condition is constant, replce with the relevant branch
        إذا (AstConst* ثابت condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            إذا (Ast_گرہ* ثابت bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->جوڑو_اگلا_ایتھے(bodyp->unlink_Fr_پٹھ_With_اگلا());  // This will be iterated next
            }
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
            أرجع;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    فراغ ویکھو(AstJumpGo* nodep) override {
        إذا (!m_ok) أرجع;
        // Remove trailing dead code
        إذا (nodep->nextp()) ڈالو_Deletep(nodep->nextp()->unlink_Fr_پٹھ_With_اگلا());
    }
    فراغ ویکھو(AstLoop* nodep) override {
        إذا (!m_ok) أرجع;
        m_bindings.checkpoint();
        std::pair<Ast_گرہ*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        إذا (!pair.دوجا) {
            m_bindings.revert();
            process(nodep);
            أرجع;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        إذا (pair.پہلا) {
            nodep->replaceWith(pair.پہلا);
        } وإلا {
            nodep->unlink_Fr_پٹھ();
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

عام:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ساكن std::pair<Ast_گرہ*, منطقي> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        إذا (visitor.m_ok) أرجع {visitor.m_stmtsp, سچ};
        // Otherwise delete intermediate results
        إذا (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_درخت(), visitor.m_stmtsp);
        أرجع {خالی_سوچک, خطأ};
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
    ساكن AstVarRef* unwrap_منزل_Var_Ref(Ast_گرہ_اظہاریہ* nodep, صحيح min_چوڑائی) {
        طالما (سچ) {
            إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) أرجع refp;
            إذا (AstExtend* ثابت ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } وإلا إذا (AstExtendS* ثابت ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } وإلا إذا (AstSel* ثابت sp = VN_CAST(nodep, Sel)) {
                ثابت AstConst* ثابت lsbp = VN_CAST(sp->lsbp(), Const);
                إذا (!lsbp || lsbp->منزل_UInt() != 0 || sp->چوڑائی() < min_چوڑائی) أرجع خالی_سوچک;
                nodep = sp->fromp();
            } وإلا {
                أرجع خالی_سوچک;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    منطقي isVarPlus1(Ast_گرہ* nodep, ثابت AstVarScope* vscp) {
        Ast_جوڑو* ثابت addp = VN_CAST(nodep, جوڑو_2);
        إذا (!addp || !addp->lhsp()->isOne()) أرجع خطأ;
        ثابت AstVarRef* ثابت r = unwrap_منزل_Var_Ref(addp->rhsp(), addp->چوڑائی());
        أرجع r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ساكن Ast_گرہ_اظہاریہ* resize_منزل_چوڑائی(Ast_گرہ_اظہاریہ* exprp, ثابت AstVarRef* نشانہ_Refp) {
        ثابت صحيح چوڑائی = نشانہ_Refp->چوڑائی();
        إذا (چوڑائی == 32) أرجع exprp;
        File_سطر* ثابت flp = exprp->fileline();
        إذا (چوڑائی < 32) أرجع جديد AstSel{flp, exprp, 0, چوڑائی};
        AstExtend* ثابت extp = جديد AstExtend{flp, exprp};
        extp->dtypeFrom(نشانہ_Refp);
        أرجع extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ساكن منطقي ascendingBound(Ast_گرہ_اظہاریہ* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        إذا (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) أرجع خطأ;
        Ast_گرہ_Biop* ثابت bp = VN_AS(condp, گرہ_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        أرجع wp && idxRefp && !wp->عدد().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ساكن منطقي is_وچ_Range_Guard(Ast_گرہ_اظہاریہ* condp, ثابت AstVarScope* idxVscp, uint32_t چوڑائی,
                               صحيح addr_بٹ) {
        إذا (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) أرجع خطأ;
        Ast_گرہ_Biop* ثابت bp = VN_AS(condp, گرہ_Biop);
        ثابت AstConst* ثابت cp = VN_CAST(bp->lhsp(), Const);
        إذا (!cp || cp->عدد().isFourState() || cp->منزل_UInt() < چوڑائی - 1) أرجع خطأ;
        ثابت AstVarRef* ثابت r = unwrap_منزل_Var_Ref(bp->rhsp(), addr_بٹ);
        أرجع r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    منطقي try_ہیٹھلا_ثنائی_ہندسہ_جانچ_Loop(AstLoop* loopp) {
        AstLoopTest* ثابت testp = VN_CAST(loopp->stmtsp(), LoopTest);
        إذا (!testp) أرجع خطأ;
        AstIf* ثابت ifp = VN_CAST(testp->nextp(), If);
        إذا (!ifp) أرجع خطأ;
        Ast_سونپو* ثابت incp = VN_CAST(ifp->nextp(), سونپو);
        إذا (!incp || incp->nextp()) أرجع خطأ;
        AstConst* wp = خالی_سوچک;
        AstVarRef* idxRefp = خالی_سوچک;
        إذا (!ascendingBound(testp->condp(), wp, idxRefp)) أرجع خطأ;
        AstVarScope* ثابت idxVscp = idxRefp->varScopep();
        ثابت uint32_t چوڑائی = wp->منزل_UInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ثابت صحيح addr_بٹ = چوڑائی <= 1 ? 1 : V3_نمبر::log2b(چوڑائی - 1) + 1;
        ثابت AstConst* ثابت idxInitp = m_bindings.get(idxVscp);
        إذا (!idxInitp || !idxInitp->isZero()) أرجع خطأ;
        AstVarRef* ثابت incLhsp = VN_CAST(incp->lhsp(), VarRef);
        إذا (!incLhsp || incLhsp->varScopep() != idxVscp) أرجع خطأ;
        إذا (!isVarPlus1(incp->rhsp(), idxVscp)) أرجع خطأ;
        إذا (ifp->elsesp()) أرجع خطأ;
        Ast_سونپو* ثابت thenp = VN_CAST(ifp->thensp(), سونپو);
        إذا (!thenp || thenp->nextp()) أرجع خطأ;
        AstVarRef* ثابت نشانہ_Refp = VN_CAST(thenp->lhsp(), VarRef);
        إذا (!نشانہ_Refp) أرجع خطأ;
        AstVarScope* ثابت نشانہ_Vscp = نشانہ_Refp->varScopep();
        إذا (نشانہ_Vscp == idxVscp) أرجع خطأ;
        ثابت منطقي isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ثابت منطقي is_گنتی_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), نشانہ_Vscp);
        إذا (!isLeadingOne && !is_گنتی_Ones) أرجع خطأ;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_گرہ_اظہاریہ* condp = ifp->condp();
        إذا (AstLogAnd* ثابت andp = VN_CAST(condp, LogAnd)) {
            إذا (is_وچ_Range_Guard(andp->lhsp(), idxVscp, چوڑائی, addr_بٹ)) condp = andp->rhsp();
        } وإلا إذا (AstCond* ثابت ternp = VN_CAST(condp, Cond)) {
            إذا (is_وچ_Range_Guard(ternp->condp(), idxVscp, چوڑائی, addr_بٹ)) condp = ternp->thenp();
        }
        AstSel* ثابت selp = VN_CAST(condp, Sel);
        إذا (!selp || selp->چوڑائی() != 1) أرجع خطأ;
        ثابت AstVarRef* ثابت fromp = VN_CAST(selp->fromp(), VarRef);
        إذا (!fromp) أرجع خطأ;
        ثابت AstVarScope* ثابت fromVscp = fromp->varScopep();
        إذا (fromVscp == idxVscp || fromVscp == نشانہ_Vscp) أرجع خطأ;
        Ast_گرہ_اظہاریہ* ثابت vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        إذا (static_cast<صحيح>(چوڑائی) != vecExprp->چوڑائی()) أرجع خطأ;
        ثابت AstVarRef* ثابت idx_وچ_Sel = unwrap_منزل_Var_Ref(selp->lsbp(), addr_بٹ);
        إذا (!idx_وچ_Sel || idx_وچ_Sel->varScopep() != idxVscp) أرجع خطأ;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ثابت AstConst* ثابت نشانہ_Initp = m_bindings.get(نشانہ_Vscp);
        إذا (!نشانہ_Initp || !نشانہ_Initp->isZero()) أرجع خطأ;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_سطر* ثابت flp = loopp->fileline();
        Ast_گرہ_اظہاریہ* reducep;
        إذا (isLeadingOne) {
            reducep = جديد Ast_Most_سیٹ_ثنائی_ہندسہ_P1{flp, vecExprp->clone_درخت(خطأ)};
        } وإلا {
            Ast_گنتی_Ones* ثابت conep = جديد Ast_گنتی_Ones{flp, vecExprp->clone_درخت(خطأ)};
            reducep = conep;
        }
        reducep = resize_منزل_چوڑائی(reducep, نشانہ_Refp);
        Ast_سونپو* ثابت newp = جديد Ast_سونپو{flp, نشانہ_Refp->clone_درخت(خطأ), reducep};
        newp->جوڑو_اگلا(جديد Ast_سونپو{flp, incLhsp->clone_درخت(خطأ), wp->clone_درخت(خطأ)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(loopp), loopp);
        إذا (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_ثنائی_ہندسہ_جانچ_Lowered;
        } وإلا {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_گنتی_Ones_Lowered;
        }
        أرجع سچ;
    }

    //ویکھو
    فراغ ویکھو(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        لكل (Ast_گرہ *succp = nodep, *currp = nodep->backp(); سچ;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            إذا (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                إذا (VN_IS(currp, Jump_بلاک)) استمر;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                اكسر;
            }
            Ast_سونپو* ثابت assignp = VN_CAST(currp, سونپو);
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
            m_bindings.سیٹ(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        إذا (v3Global.opt.f_ثنائی_ہندسہ_جانچ_Loops() && try_ہیٹھلا_ثنائی_ہندسہ_جانچ_Loop(nodep)) أرجع;

        // Attempt to unroll this loop
        ثابت std::pair<Ast_گرہ*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        إذا (!pair.دوجا) {
            iterateChildren(nodep);
            أرجع;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        إذا (pair.پہلا) {
            nodep->replaceWith(pair.پہلا);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } وإلا {
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_درخت("unroll", 0, dump_درخت_Either_Level() >= 3);
}
