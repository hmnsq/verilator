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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Unroll.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

నిర్మాణం UnrollStats final {
    తరగతి Stat final {
        కొలత_t m_విలువ = 0;  // Statistics value
        స్థిరరాశి అక్షరం* స్థిరరాశి m_name;  // Name for stats file and UDEBUG

    బహిరంగ:
        స్పష్టమైన Stat(స్థిరరాశి అక్షరం* స్థిరరాశి name)
            : m_name{name} {}
        ~Stat() { V3Stats::కూడు_Stat("Optimizations, Loop unrolling, "s + m_name, m_విలువ); }
        స్థిరరాశి అక్షరం* name() స్థిరరాశి { తిరిగి m_name; }
        Stat& క్రియాకారకం++() {
            ++m_విలువ;
            తిరిగి *ఇది;
        }
        Stat& క్రియాకారకం+=(కొలత_t v) {
            m_విలువ += v;
            తిరిగి *ఇది;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_లెక్క{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_బిట్టు_పరిశీలన_Lowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_లెక్క_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
తరగతి UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::సదిశ<AstConst*> m_curr{శూన్యసూచిక};
    // Stack of binding checkpoints
    std::సదిశ<std::సదిశ<AstConst*>> m_checkpoints;

బహిరంగ:
    // CONSTRUCTOR
    UnrolllBindings() = అప్రమేయ;
    ~UnrolllBindings() = అప్రమేయ;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    శూన్య clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_కణుపు::user1_Clear_వృక్షం();
    }

    శూన్య checkpoint() { m_checkpoints.నెట్టు_వెనుక(m_curr); }

    శూన్య revert() {
        m_curr = std::move(m_checkpoints.వెనుక());
        m_checkpoints.తీయి_వెనుక();
    }

    శూన్య commit() { m_checkpoints.తీయి_వెనుక(); }

    శూన్య సమితి(AstVarScope* vscp, AstConst* valp) {
        ఒకవేళ (!vscp->user1()) {
            vscp->user1(m_curr.కొలత());
            m_curr.నెట్టు_వెనుక(శూన్యసూచిక);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        ఒకవేళ (static_cast<కొలత_t>(vscp->user1()) >= m_curr.కొలత()) {
            m_curr.resize(vscp->user1() + 1, శూన్యసూచిక);
        }
        AstConst* స్థిరరాశి valp = vscp->user1() ? m_curr[vscp->user1()] : శూన్యసూచిక;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        తిరిగి valp;
    }
};

//######################################################################
// Unroll one AstLoop

తరగతి UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* స్థిరరాశి m_loopp;  // The loop we are trying to unroll
    Ast_కణుపు* m_stmtsp = శూన్యసూచిక;  // Resulting unrolled statement
    కొలత_t m_unrolled_కొలత = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* స్థిరరాశి m_wrapp = కొత్త AstBegin{m_loopp->fileline(), "[EditWrapper]", శూన్యసూచిక, అబద్ధం};
    స్థిరరాశి తార్కిక m_unroll_నిండిన = m_loopp->unroll().is_సమితి_True();  // Completely unroll the loop?
    తార్కిక m_ok = నిజం;  // Unrolling successful so far, gave up if false

    // METHODS
    శూన్య cantUnroll(Ast_కణుపు* nodep, UnrollStats::Stat& stat) {
        m_ok = అబద్ధం;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    తార్కిక unrollOneIteration(Ast_కణుపు* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        తార్కిక దొరికింది_Loop_Test = అబద్ధం;
        // Process one body statement at a time
        కొరకు (Ast_కణుపు* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            ఒకవేళ (AstLoopTest* స్థిరరాశి testp = VN_CAST(stmtp, LoopTest)) {
                Ast_కణుపు* స్థిరరాశి condp = V3Const::constifyEdit(testp->condp());
                ఒకవేళ (condp->isZero()) {
                    // Loop terminates
                    తిరిగి అబద్ధం;
                } లేకపోతే ఒకవేళ (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    కొనసాగు;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->కూడు_Stmtsp(stmtp->clone_వృక్షం(అబద్ధం));
            iterate_And_తదుపరి_Null(m_wrapp->stmtsp());
            // Give up if failed
            ఒకవేళ (!m_ok) తిరిగి అబద్ధం;

            // Add statements to unrolled body
            అయితే (Ast_కణుపు* స్థిరరాశి nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                ఒకవేళ (!m_unroll_నిండిన) {
                    m_unrolled_కొలత += nodep->కణుపు_లెక్క();
                    ఒకవేళ (m_unrolled_కొలత > static_cast<కొలత_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        తిరిగి అబద్ధం;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlink_Fr_వెనుక();

                // If a LoopTest, check how it resolved
                ఒకవేళ (AstLoopTest* స్థిరరాశి testp = VN_CAST(nodep, LoopTest)) {
                    దొరికింది_Loop_Test = నిజం;
                    // Will not actually need it, nor any subsequent
                    నెట్టు_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    ఒకవేళ (testp->condp()->isNeqZero()) కొనసాగు;
                    // Won't need any of the trailing statements
                    ఒకవేళ (m_wrapp->stmtsp()) నెట్టు_Deletep(m_wrapp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
                    // Loop terminates
                    ఒకవేళ (testp->condp()->isZero()) తిరిగి అబద్ధం;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    తిరిగి అబద్ధం;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_కణుపు::కూడు_తదుపరి(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                ఒకవేళ (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    తిరిగి అబద్ధం;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        ఒకవేళ (!దొరికింది_Loop_Test) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            తిరిగి అబద్ధం;
        }
        // One iteration done, loop continues
        తిరిగి నిజం;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    తార్కిక process(Ast_కణుపు* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        ఒకవేళ (!nodep) తిరిగి నిజం;
        // Variable references we should try to substitute
        std::సదిశ<AstVarRef*> గమ్యం_Substitute;
        // Iterate subtree
        nodep->foreach([&](Ast_కణుపు* np) {
            // Failed earlier
            ఒకవేళ (!m_ok) తిరిగి;
            // Check for AstLoopTest
            ఒకవేళ (AstLoopTest* స్థిరరాశి testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                ఒకవేళ (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                తిరిగి;
            }
            // Check for AstFork - can't unroll
            ఒకవేళ (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                తిరిగి;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            ఒకవేళ (Ast_కణుపు_CCall* స్థిరరాశి callp = VN_CAST(np, కణుపు_CCall)) {
                ఒకవేళ (!callp->is_శుద్ధ()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    తిరిగి;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            ఒకవేళ (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                తిరిగి;
            }
            // Process variable references
            AstVarRef* స్థిరరాశి refp = VN_CAST(np, VarRef);
            ఒకవేళ (!refp) తిరిగి;
            // Ignore if the referenced variable has no binding
            AstConst* స్థిరరాశి valp = m_bindings.get(refp->varScopep());
            ఒకవేళ (!valp) తిరిగి;
            // If writen, remove the binding
            ఒకవేళ (refp->access().isWriteOrRW()) {
                m_bindings.సమితి(refp->varScopep(), శూన్యసూచిక);
                తిరిగి;
            }
            // Otherwise add it to the list of variables to substitute
            గమ్యం_Substitute.నెట్టు_వెనుక(refp);
        });
        // Give up if we have to
        ఒకవేళ (!m_ok) తిరిగి అబద్ధం;
        // Actually substitute the variables that still have bindings
        కొరకు (AstVarRef* స్థిరరాశి refp : గమ్యం_Substitute) {
            // Pick up bound value
            AstConst* స్థిరరాశి valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            ఒకవేళ (!valp) కొనసాగు;
            // Substitute it
            refp->replaceWith(valp->clone_వృక్షం(అబద్ధం));
            VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
        }
        తిరిగి నిజం;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        ఒకవేళ (loopp->unroll().is_సమితి_False()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            తిరిగి;
        }
        // Attempt to unroll the loop
        స్థిరరాశి కొలత_t iter_హద్దు = m_unroll_నిండిన ? v3Global.opt.unroll_హద్దు()  //
                                              : v3Global.opt.unroll_లెక్క();
        కొలత_t iter_లెక్క = 0;
        చేయి {
            ఒకవేళ (iter_లెక్క > iter_హద్దు) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_లెక్క);
                ఒకవేళ (m_unroll_నిండిన) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iter_హద్దు);
                }
                తిరిగి;
            }
            ++iter_లెక్క;
        } అయితే (unrollOneIteration(loopp->stmtsp()));

        ఒకవేళ (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_లెక్క;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_వృక్షం(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (!m_ok) తిరిగి;
        // Generic body statement, just substitute
        process(nodep);
    }
    శూన్య సందర్శించు(AstLoopTest* nodep) override {
        ఒకవేళ (!m_ok) తిరిగి;
        // If the condition is a ExprStmt, move it before the LoopTest
        ఒకవేళ (Ast_సమాసం_Stmt* స్థిరరాశి exprp = VN_CAST(nodep->condp(), సమాసం_Stmt)) {
            Ast_కణుపు* స్థిరరాశి stmtsp = exprp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();
            exprp->replaceWith(exprp->resultp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlink_Fr_వెనుక(&relinker);
            stmtsp->కూడు_తదుపరి(nodep);
            relinker.relink(stmtsp);
            తిరిగి;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override {
        ఒకవేళ (!m_ok) తిరిగి;
        // Can't do it if delayed
        ఒకవేళ (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            తిరిగి;
        }
        ఒకవేళ (!process(nodep->rhsp())) తిరిగి;
        // If a simple variable assignment, update the binding
        AstVarRef* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* స్థిరరాశి valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        ఒకవేళ (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.సమితి(lhsp->varScopep(), valp);
            తిరిగి;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    శూన్య సందర్శించు(AstIf* nodep) override {
        ఒకవేళ (!m_ok) తిరిగి;
        ఒకవేళ (!process(nodep->condp())) తిరిగి;
        // If condition is constant, replce with the relevant branch
        ఒకవేళ (AstConst* స్థిరరాశి condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->కూడు_తదుపరి_ఇక్కడ(bodyp->unlink_Fr_వెనుక_With_తదుపరి());  // This will be iterated next
            }
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    శూన్య సందర్శించు(AstJumpGo* nodep) override {
        ఒకవేళ (!m_ok) తిరిగి;
        // Remove trailing dead code
        ఒకవేళ (nodep->nextp()) నెట్టు_Deletep(nodep->nextp()->unlink_Fr_వెనుక_With_తదుపరి());
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        ఒకవేళ (!m_ok) తిరిగి;
        m_bindings.checkpoint();
        std::pair<Ast_కణుపు*, తార్కిక> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        ఒకవేళ (!pair.రెండవ) {
            m_bindings.revert();
            process(nodep);
            తిరిగి;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        ఒకవేళ (pair.మొదటి) {
            nodep->replaceWith(pair.మొదటి);
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

బహిరంగ:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    స్థిర std::pair<Ast_కణుపు*, తార్కిక> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        ఒకవేళ (visitor.m_ok) తిరిగి {visitor.m_stmtsp, నిజం};
        // Otherwise delete intermediate results
        ఒకవేళ (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_వృక్షం(), visitor.m_stmtsp);
        తిరిగి {శూన్యసూచిక, అబద్ధం};
    }
};

//######################################################################
// Unroll all AstLoop statements

తరగతి UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    స్థిర AstVarRef* unwrap_గమ్యం_Var_Ref(Ast_కణుపు_సమాసం* nodep, పూర్ణాంకం min_వెడల్పు) {
        అయితే (నిజం) {
            ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) తిరిగి refp;
            ఒకవేళ (AstExtend* స్థిరరాశి ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } లేకపోతే ఒకవేళ (AstExtendS* స్థిరరాశి ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } లేకపోతే ఒకవేళ (AstSel* స్థిరరాశి sp = VN_CAST(nodep, Sel)) {
                స్థిరరాశి AstConst* స్థిరరాశి lsbp = VN_CAST(sp->lsbp(), Const);
                ఒకవేళ (!lsbp || lsbp->గమ్యం_UInt() != 0 || sp->వెడల్పు() < min_వెడల్పు) తిరిగి శూన్యసూచిక;
                nodep = sp->fromp();
            } లేకపోతే {
                తిరిగి శూన్యసూచిక;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    తార్కిక isVarPlus1(Ast_కణుపు* nodep, స్థిరరాశి AstVarScope* vscp) {
        Ast_కూడు* స్థిరరాశి addp = VN_CAST(nodep, కూడు_2);
        ఒకవేళ (!addp || !addp->lhsp()->isOne()) తిరిగి అబద్ధం;
        స్థిరరాశి AstVarRef* స్థిరరాశి r = unwrap_గమ్యం_Var_Ref(addp->rhsp(), addp->వెడల్పు());
        తిరిగి r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    స్థిర Ast_కణుపు_సమాసం* resize_గమ్యం_వెడల్పు(Ast_కణుపు_సమాసం* exprp, స్థిరరాశి AstVarRef* లక్ష్యం_Refp) {
        స్థిరరాశి పూర్ణాంకం వెడల్పు = లక్ష్యం_Refp->వెడల్పు();
        ఒకవేళ (వెడల్పు == 32) తిరిగి exprp;
        File_పంక్తి* స్థిరరాశి flp = exprp->fileline();
        ఒకవేళ (వెడల్పు < 32) తిరిగి కొత్త AstSel{flp, exprp, 0, వెడల్పు};
        AstExtend* స్థిరరాశి extp = కొత్త AstExtend{flp, exprp};
        extp->dtypeFrom(లక్ష్యం_Refp);
        తిరిగి extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    స్థిర తార్కిక ascendingBound(Ast_కణుపు_సమాసం* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        ఒకవేళ (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) తిరిగి అబద్ధం;
        Ast_కణుపు_Biop* స్థిరరాశి bp = VN_AS(condp, కణుపు_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        తిరిగి wp && idxRefp && !wp->అంకె().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    స్థిర తార్కిక is_లోపల_Range_Guard(Ast_కణుపు_సమాసం* condp, స్థిరరాశి AstVarScope* idxVscp, uint32_t వెడల్పు,
                               పూర్ణాంకం addr_బిట్లు) {
        ఒకవేళ (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) తిరిగి అబద్ధం;
        Ast_కణుపు_Biop* స్థిరరాశి bp = VN_AS(condp, కణుపు_Biop);
        స్థిరరాశి AstConst* స్థిరరాశి cp = VN_CAST(bp->lhsp(), Const);
        ఒకవేళ (!cp || cp->అంకె().isFourState() || cp->గమ్యం_UInt() < వెడల్పు - 1) తిరిగి అబద్ధం;
        స్థిరరాశి AstVarRef* స్థిరరాశి r = unwrap_గమ్యం_Var_Ref(bp->rhsp(), addr_బిట్లు);
        తిరిగి r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    తార్కిక try_క్రింది_బిట్టు_పరిశీలన_Loop(AstLoop* loopp) {
        AstLoopTest* స్థిరరాశి testp = VN_CAST(loopp->stmtsp(), LoopTest);
        ఒకవేళ (!testp) తిరిగి అబద్ధం;
        AstIf* స్థిరరాశి ifp = VN_CAST(testp->nextp(), If);
        ఒకవేళ (!ifp) తిరిగి అబద్ధం;
        Ast_కేటాయించు* స్థిరరాశి incp = VN_CAST(ifp->nextp(), కేటాయించు);
        ఒకవేళ (!incp || incp->nextp()) తిరిగి అబద్ధం;
        AstConst* wp = శూన్యసూచిక;
        AstVarRef* idxRefp = శూన్యసూచిక;
        ఒకవేళ (!ascendingBound(testp->condp(), wp, idxRefp)) తిరిగి అబద్ధం;
        AstVarScope* స్థిరరాశి idxVscp = idxRefp->varScopep();
        స్థిరరాశి uint32_t వెడల్పు = wp->గమ్యం_UInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        స్థిరరాశి పూర్ణాంకం addr_బిట్లు = వెడల్పు <= 1 ? 1 : V3_సంఖ్య::log2b(వెడల్పు - 1) + 1;
        స్థిరరాశి AstConst* స్థిరరాశి idxInitp = m_bindings.get(idxVscp);
        ఒకవేళ (!idxInitp || !idxInitp->isZero()) తిరిగి అబద్ధం;
        AstVarRef* స్థిరరాశి incLhsp = VN_CAST(incp->lhsp(), VarRef);
        ఒకవేళ (!incLhsp || incLhsp->varScopep() != idxVscp) తిరిగి అబద్ధం;
        ఒకవేళ (!isVarPlus1(incp->rhsp(), idxVscp)) తిరిగి అబద్ధం;
        ఒకవేళ (ifp->elsesp()) తిరిగి అబద్ధం;
        Ast_కేటాయించు* స్థిరరాశి thenp = VN_CAST(ifp->thensp(), కేటాయించు);
        ఒకవేళ (!thenp || thenp->nextp()) తిరిగి అబద్ధం;
        AstVarRef* స్థిరరాశి లక్ష్యం_Refp = VN_CAST(thenp->lhsp(), VarRef);
        ఒకవేళ (!లక్ష్యం_Refp) తిరిగి అబద్ధం;
        AstVarScope* స్థిరరాశి లక్ష్యం_Vscp = లక్ష్యం_Refp->varScopep();
        ఒకవేళ (లక్ష్యం_Vscp == idxVscp) తిరిగి అబద్ధం;
        స్థిరరాశి తార్కిక isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        స్థిరరాశి తార్కిక is_లెక్క_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), లక్ష్యం_Vscp);
        ఒకవేళ (!isLeadingOne && !is_లెక్క_Ones) తిరిగి అబద్ధం;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_కణుపు_సమాసం* condp = ifp->condp();
        ఒకవేళ (AstLogAnd* స్థిరరాశి andp = VN_CAST(condp, LogAnd)) {
            ఒకవేళ (is_లోపల_Range_Guard(andp->lhsp(), idxVscp, వెడల్పు, addr_బిట్లు)) condp = andp->rhsp();
        } లేకపోతే ఒకవేళ (AstCond* స్థిరరాశి ternp = VN_CAST(condp, Cond)) {
            ఒకవేళ (is_లోపల_Range_Guard(ternp->condp(), idxVscp, వెడల్పు, addr_బిట్లు)) condp = ternp->thenp();
        }
        AstSel* స్థిరరాశి selp = VN_CAST(condp, Sel);
        ఒకవేళ (!selp || selp->వెడల్పు() != 1) తిరిగి అబద్ధం;
        స్థిరరాశి AstVarRef* స్థిరరాశి fromp = VN_CAST(selp->fromp(), VarRef);
        ఒకవేళ (!fromp) తిరిగి అబద్ధం;
        స్థిరరాశి AstVarScope* స్థిరరాశి fromVscp = fromp->varScopep();
        ఒకవేళ (fromVscp == idxVscp || fromVscp == లక్ష్యం_Vscp) తిరిగి అబద్ధం;
        Ast_కణుపు_సమాసం* స్థిరరాశి vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        ఒకవేళ (static_cast<పూర్ణాంకం>(వెడల్పు) != vecExprp->వెడల్పు()) తిరిగి అబద్ధం;
        స్థిరరాశి AstVarRef* స్థిరరాశి idx_లోపల_Sel = unwrap_గమ్యం_Var_Ref(selp->lsbp(), addr_బిట్లు);
        ఒకవేళ (!idx_లోపల_Sel || idx_లోపల_Sel->varScopep() != idxVscp) తిరిగి అబద్ధం;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        స్థిరరాశి AstConst* స్థిరరాశి లక్ష్యం_Initp = m_bindings.get(లక్ష్యం_Vscp);
        ఒకవేళ (!లక్ష్యం_Initp || !లక్ష్యం_Initp->isZero()) తిరిగి అబద్ధం;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_పంక్తి* స్థిరరాశి flp = loopp->fileline();
        Ast_కణుపు_సమాసం* reducep;
        ఒకవేళ (isLeadingOne) {
            reducep = కొత్త Ast_Most_సమితి_బిట్టు_P1{flp, vecExprp->clone_వృక్షం(అబద్ధం)};
        } లేకపోతే {
            Ast_లెక్క_Ones* స్థిరరాశి conep = కొత్త Ast_లెక్క_Ones{flp, vecExprp->clone_వృక్షం(అబద్ధం)};
            reducep = conep;
        }
        reducep = resize_గమ్యం_వెడల్పు(reducep, లక్ష్యం_Refp);
        Ast_కేటాయించు* స్థిరరాశి newp = కొత్త Ast_కేటాయించు{flp, లక్ష్యం_Refp->clone_వృక్షం(అబద్ధం), reducep};
        newp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{flp, incLhsp->clone_వృక్షం(అబద్ధం), wp->clone_వృక్షం(అబద్ధం)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(loopp), loopp);
        ఒకవేళ (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_బిట్టు_పరిశీలన_Lowered;
        } లేకపోతే {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_లెక్క_Ones_Lowered;
        }
        తిరిగి నిజం;
    }

    //సందర్శించు
    శూన్య సందర్శించు(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        కొరకు (Ast_కణుపు *succp = nodep, *currp = nodep->backp(); నిజం;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            ఒకవేళ (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                ఒకవేళ (VN_IS(currp, Jump_ఖండం)) కొనసాగు;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                ఆపు;
            }
            Ast_కేటాయించు* స్థిరరాశి assignp = VN_CAST(currp, కేటాయించు);
            ఒకవేళ (!assignp) ఆపు;
            AstConst* స్థిరరాశి valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            ఒకవేళ (!valp) ఆపు;
            AstVarRef* స్థిరరాశి lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ఒకవేళ (!lhsp) ఆపు;
            // Don't bind if volatile
            ఒకవేళ (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) కొనసాగు;
            // Don't overwrite a later binding
            ఒకవేళ (m_bindings.get(lhsp->varScopep())) కొనసాగు;
            // Set up the binding
            m_bindings.సమితి(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        ఒకవేళ (v3Global.opt.f_బిట్టు_పరిశీలన_Loops() && try_క్రింది_బిట్టు_పరిశీలన_Loop(nodep)) తిరిగి;

        // Attempt to unroll this loop
        స్థిరరాశి std::pair<Ast_కణుపు*, తార్కిక> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        ఒకవేళ (!pair.రెండవ) {
            iterateChildren(nodep);
            తిరిగి;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        ఒకవేళ (pair.మొదటి) {
            nodep->replaceWith(pair.మొదటి);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే {
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    స్పష్టమైన UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

బహిరంగ:
    స్థిర శూన్య apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

శూన్య V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_వృక్షం("unroll", 0, dump_వృక్షం_Either_Level() >= 3);
}
