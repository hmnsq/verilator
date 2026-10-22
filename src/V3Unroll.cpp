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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Unroll.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

સંરચના UnrollStats final {
    વર્ગ Stat final {
        size_t m_કિંમત = 0;  // Statistics value
        અચલ અક્ષર* અચલ m_name;  // Name for stats file and UDEBUG

    જાહેર:
        explicit Stat(અચલ અક્ષર* અચલ name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_કિંમત); }
        અચલ અક્ષર* name() અચલ { પાછા m_name; }
        Stat& operator++() {
            ++m_કિંમત;
            પાછા *આ;
        }
        Stat& operator+=(size_t v) {
            m_કિંમત += v;
            પાછા *આ;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_ગણતરી{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_ગણતરી_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
વર્ગ UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    અચલ VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

જાહેર:
    // CONSTRUCTOR
    UnrolllBindings() = મૂળભૂત;
    ~UnrolllBindings() = મૂળભૂત;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    શૂન્ય clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_ગાંઠ::user1_Clear_વૃક્ષ();
    }

    શૂન્ય checkpoint() { m_checkpoints.ધકેલો_back(m_curr); }

    શૂન્ય revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.કાઢો_back();
    }

    શૂન્ય commit() { m_checkpoints.કાઢો_back(); }

    શૂન્ય set(AstVarScope* vscp, AstConst* valp) {
        જો (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.ધકેલો_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        જો (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* અચલ valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        પાછા valp;
    }
};

//######################################################################
// Unroll one AstLoop

વર્ગ UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* અચલ m_loopp;  // The loop we are trying to unroll
    Ast_ગાંઠ* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* અચલ m_wrapp = નવું AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, ખોટું};
    અચલ તાર્કિક m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    તાર્કિક m_ok = સાચું;  // Unrolling successful so far, gave up if false

    // METHODS
    શૂન્ય cantUnroll(Ast_ગાંઠ* nodep, UnrollStats::Stat& stat) {
        m_ok = ખોટું;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    તાર્કિક unrollOneIteration(Ast_ગાંઠ* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        તાર્કિક foundLoopTest = ખોટું;
        // Process one body statement at a time
        માટે (Ast_ગાંઠ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            જો (AstLoopTest* અચલ testp = VN_CAST(stmtp, LoopTest)) {
                Ast_ગાંઠ* અચલ condp = V3Const::constifyEdit(testp->condp());
                જો (condp->isZero()) {
                    // Loop terminates
                    પાછા ખોટું;
                } નહીંતર જો (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    ચાલુ;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_વૃક્ષ(ખોટું));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            જો (!m_ok) પાછા ખોટું;

            // Add statements to unrolled body
            જ્યારે (Ast_ગાંઠ* અચલ nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                જો (!m_unrollFull) {
                    m_unrolledSize += nodep->ગાંઠ_ગણતરી();
                    જો (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        પાછા ખોટું;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                જો (AstLoopTest* અચલ testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = સાચું;
                    // Will not actually need it, nor any subsequent
                    ધકેલો_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    જો (testp->condp()->isNeqZero()) ચાલુ;
                    // Won't need any of the trailing statements
                    જો (m_wrapp->stmtsp()) ધકેલો_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    જો (testp->condp()->isZero()) પાછા ખોટું;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    પાછા ખોટું;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_ગાંઠ::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                જો (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    પાછા ખોટું;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        જો (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            પાછા ખોટું;
        }
        // One iteration done, loop continues
        પાછા સાચું;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    તાર્કિક process(Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        જો (!nodep) પાછા સાચું;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_ગાંઠ* np) {
            // Failed earlier
            જો (!m_ok) પાછા;
            // Check for AstLoopTest
            જો (AstLoopTest* અચલ testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                જો (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                પાછા;
            }
            // Check for AstFork - can't unroll
            જો (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                પાછા;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            જો (Ast_ગાંઠ_CCall* અચલ callp = VN_CAST(np, ગાંઠ_CCall)) {
                જો (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    પાછા;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            જો (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                પાછા;
            }
            // Process variable references
            AstVarRef* અચલ refp = VN_CAST(np, VarRef);
            જો (!refp) પાછા;
            // Ignore if the referenced variable has no binding
            AstConst* અચલ valp = m_bindings.get(refp->varScopep());
            જો (!valp) પાછા;
            // If writen, remove the binding
            જો (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                પાછા;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.ધકેલો_back(refp);
        });
        // Give up if we have to
        જો (!m_ok) પાછા ખોટું;
        // Actually substitute the variables that still have bindings
        માટે (AstVarRef* અચલ refp : toSubstitute) {
            // Pick up bound value
            AstConst* અચલ valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            જો (!valp) ચાલુ;
            // Substitute it
            refp->replaceWith(valp->clone_વૃક્ષ(ખોટું));
            VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
        }
        પાછા સાચું;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        જો (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            પાછા;
        }
        // Attempt to unroll the loop
        અચલ size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_ગણતરી();
        size_t iter_ગણતરી = 0;
        કરો {
            જો (iter_ગણતરી > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_ગણતરી);
                જો (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                પાછા;
            }
            ++iter_ગણતરી;
        } જ્યારે (unrollOneIteration(loopp->stmtsp()));

        જો (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_ગણતરી;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_વૃક્ષ(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (!m_ok) પાછા;
        // Generic body statement, just substitute
        process(nodep);
    }
    શૂન્ય visit(AstLoopTest* nodep) override {
        જો (!m_ok) પાછા;
        // If the condition is a ExprStmt, move it before the LoopTest
        જો (Ast_પદાવલિ_Stmt* અચલ exprp = VN_CAST(nodep->condp(), પદાવલિ_Stmt)) {
            Ast_ગાંઠ* અચલ stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            પાછા;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    શૂન્ય visit(AstAssign* nodep) override {
        જો (!m_ok) પાછા;
        // Can't do it if delayed
        જો (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            પાછા;
        }
        જો (!process(nodep->rhsp())) પાછા;
        // If a simple variable assignment, update the binding
        AstVarRef* અચલ lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* અચલ valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        જો (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            પાછા;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    શૂન્ય visit(AstIf* nodep) override {
        જો (!m_ok) પાછા;
        જો (!process(nodep->condp())) પાછા;
        // If condition is constant, replce with the relevant branch
        જો (AstConst* અચલ condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            જો (Ast_ગાંઠ* અચલ bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    શૂન્ય visit(AstJumpGo* nodep) override {
        જો (!m_ok) પાછા;
        // Remove trailing dead code
        જો (nodep->nextp()) ધકેલો_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    શૂન્ય visit(AstLoop* nodep) override {
        જો (!m_ok) પાછા;
        m_bindings.checkpoint();
        std::pair<Ast_ગાંઠ*, તાર્કિક> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        જો (!pair.second) {
            m_bindings.revert();
            process(nodep);
            પાછા;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        જો (pair.first) {
            nodep->replaceWith(pair.first);
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

જાહેર:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    સ્થિર std::pair<Ast_ગાંઠ*, તાર્કિક> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        જો (visitor.m_ok) પાછા {visitor.m_stmtsp, સાચું};
        // Otherwise delete intermediate results
        જો (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_વૃક્ષ(), visitor.m_stmtsp);
        પાછા {nullptr, ખોટું};
    }
};

//######################################################################
// Unroll all AstLoop statements

વર્ગ UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    સ્થિર AstVarRef* unwrapToVarRef(Ast_ગાંઠ_પદાવલિ* nodep, પૂર્ણાંક minWidth) {
        જ્યારે (સાચું) {
            જો (AstVarRef* અચલ refp = VN_CAST(nodep, VarRef)) પાછા refp;
            જો (AstExtend* અચલ ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } નહીંતર જો (AstExtendS* અચલ ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } નહીંતર જો (AstSel* અચલ sp = VN_CAST(nodep, Sel)) {
                અચલ AstConst* અચલ lsbp = VN_CAST(sp->lsbp(), Const);
                જો (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) પાછા nullptr;
                nodep = sp->fromp();
            } નહીંતર {
                પાછા nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    તાર્કિક isVarPlus1(Ast_ગાંઠ* nodep, અચલ AstVarScope* vscp) {
        AstAdd* અચલ addp = VN_CAST(nodep, Add);
        જો (!addp || !addp->lhsp()->isOne()) પાછા ખોટું;
        અચલ AstVarRef* અચલ r = unwrapToVarRef(addp->rhsp(), addp->width());
        પાછા r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    સ્થિર Ast_ગાંઠ_પદાવલિ* resizeToWidth(Ast_ગાંઠ_પદાવલિ* exprp, અચલ AstVarRef* targetRefp) {
        અચલ પૂર્ણાંક width = targetRefp->width();
        જો (width == 32) પાછા exprp;
        File_લીટી* અચલ flp = exprp->fileline();
        જો (width < 32) પાછા નવું AstSel{flp, exprp, 0, width};
        AstExtend* અચલ extp = નવું AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        પાછા extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    સ્થિર તાર્કિક ascendingBound(Ast_ગાંઠ_પદાવલિ* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        જો (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) પાછા ખોટું;
        Ast_ગાંઠ_Biop* અચલ bp = VN_AS(condp, ગાંઠ_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        પાછા wp && idxRefp && !wp->અંક().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    સ્થિર તાર્કિક isInRangeGuard(Ast_ગાંઠ_પદાવલિ* condp, અચલ AstVarScope* idxVscp, uint32_t width,
                               પૂર્ણાંક addrBits) {
        જો (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) પાછા ખોટું;
        Ast_ગાંઠ_Biop* અચલ bp = VN_AS(condp, ગાંઠ_Biop);
        અચલ AstConst* અચલ cp = VN_CAST(bp->lhsp(), Const);
        જો (!cp || cp->અંક().isFourState() || cp->toUInt() < width - 1) પાછા ખોટું;
        અચલ AstVarRef* અચલ r = unwrapToVarRef(bp->rhsp(), addrBits);
        પાછા r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    તાર્કિક tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* અચલ testp = VN_CAST(loopp->stmtsp(), LoopTest);
        જો (!testp) પાછા ખોટું;
        AstIf* અચલ ifp = VN_CAST(testp->nextp(), If);
        જો (!ifp) પાછા ખોટું;
        AstAssign* અચલ incp = VN_CAST(ifp->nextp(), Assign);
        જો (!incp || incp->nextp()) પાછા ખોટું;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        જો (!ascendingBound(testp->condp(), wp, idxRefp)) પાછા ખોટું;
        AstVarScope* અચલ idxVscp = idxRefp->varScopep();
        અચલ uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        અચલ પૂર્ણાંક addrBits = width <= 1 ? 1 : V3_સંખ્યા::log2b(width - 1) + 1;
        અચલ AstConst* અચલ idxInitp = m_bindings.get(idxVscp);
        જો (!idxInitp || !idxInitp->isZero()) પાછા ખોટું;
        AstVarRef* અચલ incLhsp = VN_CAST(incp->lhsp(), VarRef);
        જો (!incLhsp || incLhsp->varScopep() != idxVscp) પાછા ખોટું;
        જો (!isVarPlus1(incp->rhsp(), idxVscp)) પાછા ખોટું;
        જો (ifp->elsesp()) પાછા ખોટું;
        AstAssign* અચલ thenp = VN_CAST(ifp->thensp(), Assign);
        જો (!thenp || thenp->nextp()) પાછા ખોટું;
        AstVarRef* અચલ targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        જો (!targetRefp) પાછા ખોટું;
        AstVarScope* અચલ targetVscp = targetRefp->varScopep();
        જો (targetVscp == idxVscp) પાછા ખોટું;
        અચલ તાર્કિક isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        અચલ તાર્કિક is_ગણતરી_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        જો (!isLeadingOne && !is_ગણતરી_Ones) પાછા ખોટું;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_ગાંઠ_પદાવલિ* condp = ifp->condp();
        જો (AstLogAnd* અચલ andp = VN_CAST(condp, LogAnd)) {
            જો (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } નહીંતર જો (AstCond* અચલ ternp = VN_CAST(condp, Cond)) {
            જો (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* અચલ selp = VN_CAST(condp, Sel);
        જો (!selp || selp->width() != 1) પાછા ખોટું;
        અચલ AstVarRef* અચલ fromp = VN_CAST(selp->fromp(), VarRef);
        જો (!fromp) પાછા ખોટું;
        અચલ AstVarScope* અચલ fromVscp = fromp->varScopep();
        જો (fromVscp == idxVscp || fromVscp == targetVscp) પાછા ખોટું;
        Ast_ગાંઠ_પદાવલિ* અચલ vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        જો (static_cast<પૂર્ણાંક>(width) != vecExprp->width()) પાછા ખોટું;
        અચલ AstVarRef* અચલ idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        જો (!idxInSel || idxInSel->varScopep() != idxVscp) પાછા ખોટું;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        અચલ AstConst* અચલ targetInitp = m_bindings.get(targetVscp);
        જો (!targetInitp || !targetInitp->isZero()) પાછા ખોટું;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_લીટી* અચલ flp = loopp->fileline();
        Ast_ગાંઠ_પદાવલિ* reducep;
        જો (isLeadingOne) {
            reducep = નવું AstMostSetBitP1{flp, vecExprp->clone_વૃક્ષ(ખોટું)};
        } નહીંતર {
            Ast_ગણતરી_Ones* અચલ conep = નવું Ast_ગણતરી_Ones{flp, vecExprp->clone_વૃક્ષ(ખોટું)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* અચલ newp = નવું AstAssign{flp, targetRefp->clone_વૃક્ષ(ખોટું), reducep};
        newp->addNext(નવું AstAssign{flp, incLhsp->clone_વૃક્ષ(ખોટું), wp->clone_વૃક્ષ(ખોટું)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(loopp), loopp);
        જો (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } નહીંતર {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_ગણતરી_Ones_Lowered;
        }
        પાછા સાચું;
    }

    // VISIT
    શૂન્ય visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        માટે (Ast_ગાંઠ *succp = nodep, *currp = nodep->backp(); સાચું;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            જો (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                જો (VN_IS(currp, JumpBlock)) ચાલુ;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                તોડો;
            }
            AstAssign* અચલ assignp = VN_CAST(currp, Assign);
            જો (!assignp) તોડો;
            AstConst* અચલ valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            જો (!valp) તોડો;
            AstVarRef* અચલ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            જો (!lhsp) તોડો;
            // Don't bind if volatile
            જો (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) ચાલુ;
            // Don't overwrite a later binding
            જો (m_bindings.get(lhsp->varScopep())) ચાલુ;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        જો (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) પાછા;

        // Attempt to unroll this loop
        અચલ std::pair<Ast_ગાંઠ*, તાર્કિક> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        જો (!pair.second) {
            iterateChildren(nodep);
            પાછા;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        જો (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર {
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

જાહેર:
    સ્થિર શૂન્ય apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

શૂન્ય V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_વૃક્ષ("unroll", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
