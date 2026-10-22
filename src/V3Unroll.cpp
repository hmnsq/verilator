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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Unroll.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

கட்டமைப்பு UnrollStats final {
    வகுப்பு Stat final {
        size_t m_மதிப்பு = 0;  // Statistics value
        மாறிலி எழுத்து* மாறிலி m_name;  // Name for stats file and UDEBUG

    பொது:
        explicit Stat(மாறிலி எழுத்து* மாறிலி name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_மதிப்பு); }
        மாறிலி எழுத்து* name() மாறிலி { திரும்பு m_name; }
        Stat& operator++() {
            ++m_மதிப்பு;
            திரும்பு *இது;
        }
        Stat& operator+=(size_t v) {
            m_மதிப்பு += v;
            திரும்பு *இது;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_மொத்தம்{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_மொத்தம்_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
வகுப்பு UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    மாறிலி VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

பொது:
    // CONSTRUCTOR
    UnrolllBindings() = இயல்பு;
    ~UnrolllBindings() = இயல்பு;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    வெறுமை clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_முனை::user1_Clear_மரம்();
    }

    வெறுமை checkpoint() { m_checkpoints.தள்ளு_back(m_curr); }

    வெறுமை revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.எடு_back();
    }

    வெறுமை commit() { m_checkpoints.எடு_back(); }

    வெறுமை set(AstVarScope* vscp, AstConst* valp) {
        எனில் (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.தள்ளு_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        எனில் (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* மாறிலி valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        திரும்பு valp;
    }
};

//######################################################################
// Unroll one AstLoop

வகுப்பு UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* மாறிலி m_loopp;  // The loop we are trying to unroll
    Ast_முனை* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* மாறிலி m_wrapp = புதிய AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, பொய்};
    மாறிலி தருக்க m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    தருக்க m_ok = உண்மை;  // Unrolling successful so far, gave up if false

    // METHODS
    வெறுமை cantUnroll(Ast_முனை* nodep, UnrollStats::Stat& stat) {
        m_ok = பொய்;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    தருக்க unrollOneIteration(Ast_முனை* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        தருக்க foundLoopTest = பொய்;
        // Process one body statement at a time
        க்கு (Ast_முனை* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            எனில் (AstLoopTest* மாறிலி testp = VN_CAST(stmtp, LoopTest)) {
                Ast_முனை* மாறிலி condp = V3Const::constifyEdit(testp->condp());
                எனில் (condp->isZero()) {
                    // Loop terminates
                    திரும்பு பொய்;
                } இல்லையெனில் எனில் (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    தொடர்;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_மரம்(பொய்));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            எனில் (!m_ok) திரும்பு பொய்;

            // Add statements to unrolled body
            போது (Ast_முனை* மாறிலி nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                எனில் (!m_unrollFull) {
                    m_unrolledSize += nodep->முனை_மொத்தம்();
                    எனில் (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        திரும்பு பொய்;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                எனில் (AstLoopTest* மாறிலி testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = உண்மை;
                    // Will not actually need it, nor any subsequent
                    தள்ளு_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    எனில் (testp->condp()->isNeqZero()) தொடர்;
                    // Won't need any of the trailing statements
                    எனில் (m_wrapp->stmtsp()) தள்ளு_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    எனில் (testp->condp()->isZero()) திரும்பு பொய்;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    திரும்பு பொய்;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_முனை::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                எனில் (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    திரும்பு பொய்;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        எனில் (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            திரும்பு பொய்;
        }
        // One iteration done, loop continues
        திரும்பு உண்மை;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    தருக்க process(Ast_முனை* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        எனில் (!nodep) திரும்பு உண்மை;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_முனை* np) {
            // Failed earlier
            எனில் (!m_ok) திரும்பு;
            // Check for AstLoopTest
            எனில் (AstLoopTest* மாறிலி testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                எனில் (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                திரும்பு;
            }
            // Check for AstFork - can't unroll
            எனில் (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                திரும்பு;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            எனில் (Ast_முனை_CCall* மாறிலி callp = VN_CAST(np, முனை_CCall)) {
                எனில் (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    திரும்பு;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            எனில் (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                திரும்பு;
            }
            // Process variable references
            AstVarRef* மாறிலி refp = VN_CAST(np, VarRef);
            எனில் (!refp) திரும்பு;
            // Ignore if the referenced variable has no binding
            AstConst* மாறிலி valp = m_bindings.get(refp->varScopep());
            எனில் (!valp) திரும்பு;
            // If writen, remove the binding
            எனில் (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                திரும்பு;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.தள்ளு_back(refp);
        });
        // Give up if we have to
        எனில் (!m_ok) திரும்பு பொய்;
        // Actually substitute the variables that still have bindings
        க்கு (AstVarRef* மாறிலி refp : toSubstitute) {
            // Pick up bound value
            AstConst* மாறிலி valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            எனில் (!valp) தொடர்;
            // Substitute it
            refp->replaceWith(valp->clone_மரம்(பொய்));
            VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
        }
        திரும்பு உண்மை;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        எனில் (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            திரும்பு;
        }
        // Attempt to unroll the loop
        மாறிலி size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_மொத்தம்();
        size_t iter_மொத்தம் = 0;
        செய் {
            எனில் (iter_மொத்தம் > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_மொத்தம்);
                எனில் (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                திரும்பு;
            }
            ++iter_மொத்தம்;
        } போது (unrollOneIteration(loopp->stmtsp()));

        எனில் (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_மொத்தம்;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_மரம்(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (!m_ok) திரும்பு;
        // Generic body statement, just substitute
        process(nodep);
    }
    வெறுமை visit(AstLoopTest* nodep) override {
        எனில் (!m_ok) திரும்பு;
        // If the condition is a ExprStmt, move it before the LoopTest
        எனில் (Ast_கோவை_Stmt* மாறிலி exprp = VN_CAST(nodep->condp(), கோவை_Stmt)) {
            Ast_முனை* மாறிலி stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            திரும்பு;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    வெறுமை visit(AstAssign* nodep) override {
        எனில் (!m_ok) திரும்பு;
        // Can't do it if delayed
        எனில் (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            திரும்பு;
        }
        எனில் (!process(nodep->rhsp())) திரும்பு;
        // If a simple variable assignment, update the binding
        AstVarRef* மாறிலி lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* மாறிலி valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        எனில் (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            திரும்பு;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    வெறுமை visit(AstIf* nodep) override {
        எனில் (!m_ok) திரும்பு;
        எனில் (!process(nodep->condp())) திரும்பு;
        // If condition is constant, replce with the relevant branch
        எனில் (AstConst* மாறிலி condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            எனில் (Ast_முனை* மாறிலி bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    வெறுமை visit(AstJumpGo* nodep) override {
        எனில் (!m_ok) திரும்பு;
        // Remove trailing dead code
        எனில் (nodep->nextp()) தள்ளு_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    வெறுமை visit(AstLoop* nodep) override {
        எனில் (!m_ok) திரும்பு;
        m_bindings.checkpoint();
        std::pair<Ast_முனை*, தருக்க> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        எனில் (!pair.second) {
            m_bindings.revert();
            process(nodep);
            திரும்பு;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        எனில் (pair.first) {
            nodep->replaceWith(pair.first);
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

பொது:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    நிலையான std::pair<Ast_முனை*, தருக்க> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        எனில் (visitor.m_ok) திரும்பு {visitor.m_stmtsp, உண்மை};
        // Otherwise delete intermediate results
        எனில் (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_மரம்(), visitor.m_stmtsp);
        திரும்பு {nullptr, பொய்};
    }
};

//######################################################################
// Unroll all AstLoop statements

வகுப்பு UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    நிலையான AstVarRef* unwrapToVarRef(Ast_முனை_கோவை* nodep, முழு minWidth) {
        போது (உண்மை) {
            எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef)) திரும்பு refp;
            எனில் (AstExtend* மாறிலி ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } இல்லையெனில் எனில் (AstExtendS* மாறிலி ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } இல்லையெனில் எனில் (AstSel* மாறிலி sp = VN_CAST(nodep, Sel)) {
                மாறிலி AstConst* மாறிலி lsbp = VN_CAST(sp->lsbp(), Const);
                எனில் (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) திரும்பு nullptr;
                nodep = sp->fromp();
            } இல்லையெனில் {
                திரும்பு nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    தருக்க isVarPlus1(Ast_முனை* nodep, மாறிலி AstVarScope* vscp) {
        AstAdd* மாறிலி addp = VN_CAST(nodep, Add);
        எனில் (!addp || !addp->lhsp()->isOne()) திரும்பு பொய்;
        மாறிலி AstVarRef* மாறிலி r = unwrapToVarRef(addp->rhsp(), addp->width());
        திரும்பு r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    நிலையான Ast_முனை_கோவை* resizeToWidth(Ast_முனை_கோவை* exprp, மாறிலி AstVarRef* targetRefp) {
        மாறிலி முழு width = targetRefp->width();
        எனில் (width == 32) திரும்பு exprp;
        File_வரி* மாறிலி flp = exprp->fileline();
        எனில் (width < 32) திரும்பு புதிய AstSel{flp, exprp, 0, width};
        AstExtend* மாறிலி extp = புதிய AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        திரும்பு extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    நிலையான தருக்க ascendingBound(Ast_முனை_கோவை* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        எனில் (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) திரும்பு பொய்;
        Ast_முனை_Biop* மாறிலி bp = VN_AS(condp, முனை_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        திரும்பு wp && idxRefp && !wp->இலக்கம்().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    நிலையான தருக்க isInRangeGuard(Ast_முனை_கோவை* condp, மாறிலி AstVarScope* idxVscp, uint32_t width,
                               முழு addrBits) {
        எனில் (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) திரும்பு பொய்;
        Ast_முனை_Biop* மாறிலி bp = VN_AS(condp, முனை_Biop);
        மாறிலி AstConst* மாறிலி cp = VN_CAST(bp->lhsp(), Const);
        எனில் (!cp || cp->இலக்கம்().isFourState() || cp->toUInt() < width - 1) திரும்பு பொய்;
        மாறிலி AstVarRef* மாறிலி r = unwrapToVarRef(bp->rhsp(), addrBits);
        திரும்பு r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    தருக்க tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* மாறிலி testp = VN_CAST(loopp->stmtsp(), LoopTest);
        எனில் (!testp) திரும்பு பொய்;
        AstIf* மாறிலி ifp = VN_CAST(testp->nextp(), If);
        எனில் (!ifp) திரும்பு பொய்;
        AstAssign* மாறிலி incp = VN_CAST(ifp->nextp(), Assign);
        எனில் (!incp || incp->nextp()) திரும்பு பொய்;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        எனில் (!ascendingBound(testp->condp(), wp, idxRefp)) திரும்பு பொய்;
        AstVarScope* மாறிலி idxVscp = idxRefp->varScopep();
        மாறிலி uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        மாறிலி முழு addrBits = width <= 1 ? 1 : V3_எண்::log2b(width - 1) + 1;
        மாறிலி AstConst* மாறிலி idxInitp = m_bindings.get(idxVscp);
        எனில் (!idxInitp || !idxInitp->isZero()) திரும்பு பொய்;
        AstVarRef* மாறிலி incLhsp = VN_CAST(incp->lhsp(), VarRef);
        எனில் (!incLhsp || incLhsp->varScopep() != idxVscp) திரும்பு பொய்;
        எனில் (!isVarPlus1(incp->rhsp(), idxVscp)) திரும்பு பொய்;
        எனில் (ifp->elsesp()) திரும்பு பொய்;
        AstAssign* மாறிலி thenp = VN_CAST(ifp->thensp(), Assign);
        எனில் (!thenp || thenp->nextp()) திரும்பு பொய்;
        AstVarRef* மாறிலி targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        எனில் (!targetRefp) திரும்பு பொய்;
        AstVarScope* மாறிலி targetVscp = targetRefp->varScopep();
        எனில் (targetVscp == idxVscp) திரும்பு பொய்;
        மாறிலி தருக்க isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        மாறிலி தருக்க is_மொத்தம்_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        எனில் (!isLeadingOne && !is_மொத்தம்_Ones) திரும்பு பொய்;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_முனை_கோவை* condp = ifp->condp();
        எனில் (AstLogAnd* மாறிலி andp = VN_CAST(condp, LogAnd)) {
            எனில் (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } இல்லையெனில் எனில் (AstCond* மாறிலி ternp = VN_CAST(condp, Cond)) {
            எனில் (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* மாறிலி selp = VN_CAST(condp, Sel);
        எனில் (!selp || selp->width() != 1) திரும்பு பொய்;
        மாறிலி AstVarRef* மாறிலி fromp = VN_CAST(selp->fromp(), VarRef);
        எனில் (!fromp) திரும்பு பொய்;
        மாறிலி AstVarScope* மாறிலி fromVscp = fromp->varScopep();
        எனில் (fromVscp == idxVscp || fromVscp == targetVscp) திரும்பு பொய்;
        Ast_முனை_கோவை* மாறிலி vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        எனில் (static_cast<முழு>(width) != vecExprp->width()) திரும்பு பொய்;
        மாறிலி AstVarRef* மாறிலி idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        எனில் (!idxInSel || idxInSel->varScopep() != idxVscp) திரும்பு பொய்;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        மாறிலி AstConst* மாறிலி targetInitp = m_bindings.get(targetVscp);
        எனில் (!targetInitp || !targetInitp->isZero()) திரும்பு பொய்;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_வரி* மாறிலி flp = loopp->fileline();
        Ast_முனை_கோவை* reducep;
        எனில் (isLeadingOne) {
            reducep = புதிய AstMostSetBitP1{flp, vecExprp->clone_மரம்(பொய்)};
        } இல்லையெனில் {
            Ast_மொத்தம்_Ones* மாறிலி conep = புதிய Ast_மொத்தம்_Ones{flp, vecExprp->clone_மரம்(பொய்)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* மாறிலி newp = புதிய AstAssign{flp, targetRefp->clone_மரம்(பொய்), reducep};
        newp->addNext(புதிய AstAssign{flp, incLhsp->clone_மரம்(பொய்), wp->clone_மரம்(பொய்)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(loopp), loopp);
        எனில் (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } இல்லையெனில் {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_மொத்தம்_Ones_Lowered;
        }
        திரும்பு உண்மை;
    }

    // VISIT
    வெறுமை visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        க்கு (Ast_முனை *succp = nodep, *currp = nodep->backp(); உண்மை;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            எனில் (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                எனில் (VN_IS(currp, JumpBlock)) தொடர்;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                நிறுத்து;
            }
            AstAssign* மாறிலி assignp = VN_CAST(currp, Assign);
            எனில் (!assignp) நிறுத்து;
            AstConst* மாறிலி valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            எனில் (!valp) நிறுத்து;
            AstVarRef* மாறிலி lhsp = VN_CAST(assignp->lhsp(), VarRef);
            எனில் (!lhsp) நிறுத்து;
            // Don't bind if volatile
            எனில் (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) தொடர்;
            // Don't overwrite a later binding
            எனில் (m_bindings.get(lhsp->varScopep())) தொடர்;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        எனில் (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) திரும்பு;

        // Attempt to unroll this loop
        மாறிலி std::pair<Ast_முனை*, தருக்க> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        எனில் (!pair.second) {
            iterateChildren(nodep);
            திரும்பு;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        எனில் (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் {
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

பொது:
    நிலையான வெறுமை apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

வெறுமை V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_மரம்("unroll", 0, dump_மரம்_Either_Level() >= 3);
}
