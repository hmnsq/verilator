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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Unroll.h"

#شامل "V3Const.h"
#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

ساخت UnrollStats final {
    جماعت Stat final {
        سائز_t m_قدر = 0;  // Statistics value
        ثابت حرف* ثابت m_name;  // Name for stats file and UDEBUG

    عوامی:
        صریح Stat(ثابت حرف* ثابت name)
            : m_name{name} {}
        ~Stat() { V3Stats::جوڑو_Stat("Optimizations, Loop unrolling, "s + m_name, m_قدر); }
        ثابت حرف* name() ثابت { واپس m_name; }
        Stat& عامل++() {
            ++m_قدر;
            واپس *یہ;
        }
        Stat& عامل+=(سائز_t v) {
            m_قدر += v;
            واپس *یہ;
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
    Stat m_ثنائی_ہندسہ_جائزہ_Lowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_گنتی_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
جماعت UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::سمتیہ<AstConst*> m_curr{خالی_اشاریہ};
    // Stack of binding checkpoints
    std::سمتیہ<std::سمتیہ<AstConst*>> m_checkpoints;

عوامی:
    // CONSTRUCTOR
    UnrolllBindings() = طےشدہ;
    ~UnrolllBindings() = طےشدہ;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    خالی clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_گرہ::user1_Clear_درخت();
    }

    خالی checkpoint() { m_checkpoints.ڈالو_عقب(m_curr); }

    خالی revert() {
        m_curr = std::move(m_checkpoints.عقب());
        m_checkpoints.نکالو_عقب();
    }

    خالی commit() { m_checkpoints.نکالو_عقب(); }

    خالی سیٹ(AstVarScope* vscp, AstConst* valp) {
        اگر (!vscp->user1()) {
            vscp->user1(m_curr.سائز());
            m_curr.ڈالو_عقب(خالی_اشاریہ);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        اگر (static_cast<سائز_t>(vscp->user1()) >= m_curr.سائز()) {
            m_curr.resize(vscp->user1() + 1, خالی_اشاریہ);
        }
        AstConst* ثابت valp = vscp->user1() ? m_curr[vscp->user1()] : خالی_اشاریہ;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        واپس valp;
    }
};

//######################################################################
// Unroll one AstLoop

جماعت UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ثابت m_loopp;  // The loop we are trying to unroll
    Ast_گرہ* m_stmtsp = خالی_اشاریہ;  // Resulting unrolled statement
    سائز_t m_unrolled_سائز = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ثابت m_wrapp = نیا AstBegin{m_loopp->fileline(), "[EditWrapper]", خالی_اشاریہ, جھوٹ};
    ثابت منطقی m_unroll_بھرا = m_loopp->unroll().is_سیٹ_True();  // Completely unroll the loop?
    منطقی m_ok = سچ;  // Unrolling successful so far, gave up if false

    // METHODS
    خالی cantUnroll(Ast_گرہ* nodep, UnrollStats::Stat& stat) {
        m_ok = جھوٹ;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    منطقی unrollOneIteration(Ast_گرہ* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        منطقی ملا_Loop_Test = جھوٹ;
        // Process one body statement at a time
        برائے (Ast_گرہ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            اگر (AstLoopTest* ثابت testp = VN_CAST(stmtp, LoopTest)) {
                Ast_گرہ* ثابت condp = V3Const::constifyEdit(testp->condp());
                اگر (condp->isZero()) {
                    // Loop terminates
                    واپس جھوٹ;
                } ورنہ اگر (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    جاری;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->جوڑو_Stmtsp(stmtp->clone_درخت(جھوٹ));
            iterate_And_اگلا_Null(m_wrapp->stmtsp());
            // Give up if failed
            اگر (!m_ok) واپس جھوٹ;

            // Add statements to unrolled body
            جب (Ast_گرہ* ثابت nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                اگر (!m_unroll_بھرا) {
                    m_unrolled_سائز += nodep->گرہ_گنتی();
                    اگر (m_unrolled_سائز > static_cast<سائز_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        واپس جھوٹ;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlink_Fr_عقب();

                // If a LoopTest, check how it resolved
                اگر (AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
                    ملا_Loop_Test = سچ;
                    // Will not actually need it, nor any subsequent
                    ڈالو_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    اگر (testp->condp()->isNeqZero()) جاری;
                    // Won't need any of the trailing statements
                    اگر (m_wrapp->stmtsp()) ڈالو_Deletep(m_wrapp->stmtsp()->unlink_Fr_عقب_With_اگلا());
                    // Loop terminates
                    اگر (testp->condp()->isZero()) واپس جھوٹ;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    واپس جھوٹ;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_گرہ::جوڑو_اگلا(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                اگر (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    واپس جھوٹ;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        اگر (!ملا_Loop_Test) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            واپس جھوٹ;
        }
        // One iteration done, loop continues
        واپس سچ;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    منطقی process(Ast_گرہ* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        اگر (!nodep) واپس سچ;
        // Variable references we should try to substitute
        std::سمتیہ<AstVarRef*> مقصد_Substitute;
        // Iterate subtree
        nodep->foreach([&](Ast_گرہ* np) {
            // Failed earlier
            اگر (!m_ok) واپس;
            // Check for AstLoopTest
            اگر (AstLoopTest* ثابت testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                اگر (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                واپس;
            }
            // Check for AstFork - can't unroll
            اگر (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                واپس;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            اگر (Ast_گرہ_CCall* ثابت callp = VN_CAST(np, گرہ_CCall)) {
                اگر (!callp->is_خالص()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    واپس;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            اگر (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                واپس;
            }
            // Process variable references
            AstVarRef* ثابت refp = VN_CAST(np, VarRef);
            اگر (!refp) واپس;
            // Ignore if the referenced variable has no binding
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            اگر (!valp) واپس;
            // If writen, remove the binding
            اگر (refp->access().isWriteOrRW()) {
                m_bindings.سیٹ(refp->varScopep(), خالی_اشاریہ);
                واپس;
            }
            // Otherwise add it to the list of variables to substitute
            مقصد_Substitute.ڈالو_عقب(refp);
        });
        // Give up if we have to
        اگر (!m_ok) واپس جھوٹ;
        // Actually substitute the variables that still have bindings
        برائے (AstVarRef* ثابت refp : مقصد_Substitute) {
            // Pick up bound value
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            اگر (!valp) جاری;
            // Substitute it
            refp->replaceWith(valp->clone_درخت(جھوٹ));
            VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
        }
        واپس سچ;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        اگر (loopp->unroll().is_سیٹ_False()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            واپس;
        }
        // Attempt to unroll the loop
        ثابت سائز_t iter_حد_بندی = m_unroll_بھرا ? v3Global.opt.unroll_حد_بندی()  //
                                              : v3Global.opt.unroll_گنتی();
        سائز_t iter_گنتی = 0;
        کرو {
            اگر (iter_گنتی > iter_حد_بندی) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_گنتی);
                اگر (m_unroll_بھرا) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iter_حد_بندی);
                }
                واپس;
            }
            ++iter_گنتی;
        } جب (unrollOneIteration(loopp->stmtsp()));

        اگر (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_گنتی;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_درخت(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (!m_ok) واپس;
        // Generic body statement, just substitute
        process(nodep);
    }
    خالی دورہ(AstLoopTest* nodep) override {
        اگر (!m_ok) واپس;
        // If the condition is a ExprStmt, move it before the LoopTest
        اگر (Ast_اظہاریہ_Stmt* ثابت exprp = VN_CAST(nodep->condp(), اظہاریہ_Stmt)) {
            Ast_گرہ* ثابت stmtsp = exprp->stmtsp()->unlink_Fr_عقب_With_اگلا();
            exprp->replaceWith(exprp->resultp()->unlink_Fr_عقب());
            VL_DO_DANGLING(ڈالو_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlink_Fr_عقب(&relinker);
            stmtsp->جوڑو_اگلا(nodep);
            relinker.relink(stmtsp);
            واپس;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    خالی دورہ(Ast_تفویض* nodep) override {
        اگر (!m_ok) واپس;
        // Can't do it if delayed
        اگر (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            واپس;
        }
        اگر (!process(nodep->rhsp())) واپس;
        // If a simple variable assignment, update the binding
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        اگر (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.سیٹ(lhsp->varScopep(), valp);
            واپس;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    خالی دورہ(AstIf* nodep) override {
        اگر (!m_ok) واپس;
        اگر (!process(nodep->condp())) واپس;
        // If condition is constant, replce with the relevant branch
        اگر (AstConst* ثابت condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            اگر (Ast_گرہ* ثابت bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->جوڑو_اگلا_یہاں(bodyp->unlink_Fr_عقب_With_اگلا());  // This will be iterated next
            }
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    خالی دورہ(AstJumpGo* nodep) override {
        اگر (!m_ok) واپس;
        // Remove trailing dead code
        اگر (nodep->nextp()) ڈالو_Deletep(nodep->nextp()->unlink_Fr_عقب_With_اگلا());
    }
    خالی دورہ(AstLoop* nodep) override {
        اگر (!m_ok) واپس;
        m_bindings.checkpoint();
        std::pair<Ast_گرہ*, منطقی> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        اگر (!pair.دوسرا) {
            m_bindings.revert();
            process(nodep);
            واپس;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        اگر (pair.پہلا) {
            nodep->replaceWith(pair.پہلا);
        } ورنہ {
            nodep->unlink_Fr_عقب();
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

عوامی:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ساکن std::pair<Ast_گرہ*, منطقی> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        اگر (visitor.m_ok) واپس {visitor.m_stmtsp, سچ};
        // Otherwise delete intermediate results
        اگر (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_درخت(), visitor.m_stmtsp);
        واپس {خالی_اشاریہ, جھوٹ};
    }
};

//######################################################################
// Unroll all AstLoop statements

جماعت UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    ساکن AstVarRef* unwrap_مقصد_Var_Ref(Ast_گرہ_اظہاریہ* nodep, صحیح min_چوڑائی) {
        جب (سچ) {
            اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) واپس refp;
            اگر (AstExtend* ثابت ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } ورنہ اگر (AstExtendS* ثابت ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } ورنہ اگر (AstSel* ثابت sp = VN_CAST(nodep, Sel)) {
                ثابت AstConst* ثابت lsbp = VN_CAST(sp->lsbp(), Const);
                اگر (!lsbp || lsbp->مقصد_UInt() != 0 || sp->چوڑائی() < min_چوڑائی) واپس خالی_اشاریہ;
                nodep = sp->fromp();
            } ورنہ {
                واپس خالی_اشاریہ;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    منطقی isVarPlus1(Ast_گرہ* nodep, ثابت AstVarScope* vscp) {
        Ast_جوڑو* ثابت addp = VN_CAST(nodep, جوڑو_2);
        اگر (!addp || !addp->lhsp()->isOne()) واپس جھوٹ;
        ثابت AstVarRef* ثابت r = unwrap_مقصد_Var_Ref(addp->rhsp(), addp->چوڑائی());
        واپس r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ساکن Ast_گرہ_اظہاریہ* resize_مقصد_چوڑائی(Ast_گرہ_اظہاریہ* exprp, ثابت AstVarRef* ہدف_Refp) {
        ثابت صحیح چوڑائی = ہدف_Refp->چوڑائی();
        اگر (چوڑائی == 32) واپس exprp;
        File_سطر* ثابت flp = exprp->fileline();
        اگر (چوڑائی < 32) واپس نیا AstSel{flp, exprp, 0, چوڑائی};
        AstExtend* ثابت extp = نیا AstExtend{flp, exprp};
        extp->dtypeFrom(ہدف_Refp);
        واپس extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ساکن منطقی ascendingBound(Ast_گرہ_اظہاریہ* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        اگر (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) واپس جھوٹ;
        Ast_گرہ_Biop* ثابت bp = VN_AS(condp, گرہ_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        واپس wp && idxRefp && !wp->عدد().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ساکن منطقی is_اندر_Range_Guard(Ast_گرہ_اظہاریہ* condp, ثابت AstVarScope* idxVscp, uint32_t چوڑائی,
                               صحیح addr_بٹ) {
        اگر (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) واپس جھوٹ;
        Ast_گرہ_Biop* ثابت bp = VN_AS(condp, گرہ_Biop);
        ثابت AstConst* ثابت cp = VN_CAST(bp->lhsp(), Const);
        اگر (!cp || cp->عدد().isFourState() || cp->مقصد_UInt() < چوڑائی - 1) واپس جھوٹ;
        ثابت AstVarRef* ثابت r = unwrap_مقصد_Var_Ref(bp->rhsp(), addr_بٹ);
        واپس r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    منطقی try_زیریں_ثنائی_ہندسہ_جائزہ_Loop(AstLoop* loopp) {
        AstLoopTest* ثابت testp = VN_CAST(loopp->stmtsp(), LoopTest);
        اگر (!testp) واپس جھوٹ;
        AstIf* ثابت ifp = VN_CAST(testp->nextp(), If);
        اگر (!ifp) واپس جھوٹ;
        Ast_تفویض* ثابت incp = VN_CAST(ifp->nextp(), تفویض);
        اگر (!incp || incp->nextp()) واپس جھوٹ;
        AstConst* wp = خالی_اشاریہ;
        AstVarRef* idxRefp = خالی_اشاریہ;
        اگر (!ascendingBound(testp->condp(), wp, idxRefp)) واپس جھوٹ;
        AstVarScope* ثابت idxVscp = idxRefp->varScopep();
        ثابت uint32_t چوڑائی = wp->مقصد_UInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ثابت صحیح addr_بٹ = چوڑائی <= 1 ? 1 : V3_نمبر::log2b(چوڑائی - 1) + 1;
        ثابت AstConst* ثابت idxInitp = m_bindings.get(idxVscp);
        اگر (!idxInitp || !idxInitp->isZero()) واپس جھوٹ;
        AstVarRef* ثابت incLhsp = VN_CAST(incp->lhsp(), VarRef);
        اگر (!incLhsp || incLhsp->varScopep() != idxVscp) واپس جھوٹ;
        اگر (!isVarPlus1(incp->rhsp(), idxVscp)) واپس جھوٹ;
        اگر (ifp->elsesp()) واپس جھوٹ;
        Ast_تفویض* ثابت thenp = VN_CAST(ifp->thensp(), تفویض);
        اگر (!thenp || thenp->nextp()) واپس جھوٹ;
        AstVarRef* ثابت ہدف_Refp = VN_CAST(thenp->lhsp(), VarRef);
        اگر (!ہدف_Refp) واپس جھوٹ;
        AstVarScope* ثابت ہدف_Vscp = ہدف_Refp->varScopep();
        اگر (ہدف_Vscp == idxVscp) واپس جھوٹ;
        ثابت منطقی isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ثابت منطقی is_گنتی_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), ہدف_Vscp);
        اگر (!isLeadingOne && !is_گنتی_Ones) واپس جھوٹ;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_گرہ_اظہاریہ* condp = ifp->condp();
        اگر (AstLogAnd* ثابت andp = VN_CAST(condp, LogAnd)) {
            اگر (is_اندر_Range_Guard(andp->lhsp(), idxVscp, چوڑائی, addr_بٹ)) condp = andp->rhsp();
        } ورنہ اگر (AstCond* ثابت ternp = VN_CAST(condp, Cond)) {
            اگر (is_اندر_Range_Guard(ternp->condp(), idxVscp, چوڑائی, addr_بٹ)) condp = ternp->thenp();
        }
        AstSel* ثابت selp = VN_CAST(condp, Sel);
        اگر (!selp || selp->چوڑائی() != 1) واپس جھوٹ;
        ثابت AstVarRef* ثابت fromp = VN_CAST(selp->fromp(), VarRef);
        اگر (!fromp) واپس جھوٹ;
        ثابت AstVarScope* ثابت fromVscp = fromp->varScopep();
        اگر (fromVscp == idxVscp || fromVscp == ہدف_Vscp) واپس جھوٹ;
        Ast_گرہ_اظہاریہ* ثابت vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        اگر (static_cast<صحیح>(چوڑائی) != vecExprp->چوڑائی()) واپس جھوٹ;
        ثابت AstVarRef* ثابت idx_اندر_Sel = unwrap_مقصد_Var_Ref(selp->lsbp(), addr_بٹ);
        اگر (!idx_اندر_Sel || idx_اندر_Sel->varScopep() != idxVscp) واپس جھوٹ;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ثابت AstConst* ثابت ہدف_Initp = m_bindings.get(ہدف_Vscp);
        اگر (!ہدف_Initp || !ہدف_Initp->isZero()) واپس جھوٹ;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_سطر* ثابت flp = loopp->fileline();
        Ast_گرہ_اظہاریہ* reducep;
        اگر (isLeadingOne) {
            reducep = نیا Ast_Most_سیٹ_ثنائی_ہندسہ_P1{flp, vecExprp->clone_درخت(جھوٹ)};
        } ورنہ {
            Ast_گنتی_Ones* ثابت conep = نیا Ast_گنتی_Ones{flp, vecExprp->clone_درخت(جھوٹ)};
            reducep = conep;
        }
        reducep = resize_مقصد_چوڑائی(reducep, ہدف_Refp);
        Ast_تفویض* ثابت newp = نیا Ast_تفویض{flp, ہدف_Refp->clone_درخت(جھوٹ), reducep};
        newp->جوڑو_اگلا(نیا Ast_تفویض{flp, incLhsp->clone_درخت(جھوٹ), wp->clone_درخت(جھوٹ)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(loopp), loopp);
        اگر (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_ثنائی_ہندسہ_جائزہ_Lowered;
        } ورنہ {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_گنتی_Ones_Lowered;
        }
        واپس سچ;
    }

    //دورہ
    خالی دورہ(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        برائے (Ast_گرہ *succp = nodep, *currp = nodep->backp(); سچ;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            اگر (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                اگر (VN_IS(currp, Jump_بلاک)) جاری;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                توڑو;
            }
            Ast_تفویض* ثابت assignp = VN_CAST(currp, تفویض);
            اگر (!assignp) توڑو;
            AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            اگر (!valp) توڑو;
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            اگر (!lhsp) توڑو;
            // Don't bind if volatile
            اگر (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) جاری;
            // Don't overwrite a later binding
            اگر (m_bindings.get(lhsp->varScopep())) جاری;
            // Set up the binding
            m_bindings.سیٹ(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        اگر (v3Global.opt.f_ثنائی_ہندسہ_جائزہ_Loops() && try_زیریں_ثنائی_ہندسہ_جائزہ_Loop(nodep)) واپس;

        // Attempt to unroll this loop
        ثابت std::pair<Ast_گرہ*, منطقی> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        اگر (!pair.دوسرا) {
            iterateChildren(nodep);
            واپس;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        اگر (pair.پہلا) {
            nodep->replaceWith(pair.پہلا);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        } ورنہ {
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    صریح UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

عوامی:
    ساکن خالی apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

خالی V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_درخت("unroll", 0, dump_درخت_Either_Level() >= 3);
}
