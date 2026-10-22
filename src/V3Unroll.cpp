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
        size_t m_درج = 0;  // Statistics value
        ثابت حرف* ثابت m_name;  // Name for stats file and UDEBUG

    عام:
        explicit Stat(ثابت حرف* ثابت name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_درج); }
        ثابت حرف* name() ثابت { أرجع m_name; }
        Stat& operator++() {
            ++m_درج;
            أرجع *هذا;
        }
        Stat& operator+=(size_t v) {
            m_درج += v;
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
    Stat m_n_Fail_Unroll_كدي{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_كدي_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
صنف UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ثابت VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

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
        Ast_كلي::user1_Clear_بشيا();
    }

    فراغ checkpoint() { m_checkpoints.تور_back(m_curr); }

    فراغ revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.چرو_back();
    }

    فراغ commit() { m_checkpoints.چرو_back(); }

    فراغ set(AstVarScope* vscp, AstConst* valp) {
        إذا (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.تور_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        إذا (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ثابت valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
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
    Ast_كلي* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ثابت m_wrapp = جديد AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, خطأ};
    ثابت منطقي m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    منطقي m_ok = true;  // Unrolling successful so far, gave up if false

    // METHODS
    فراغ cantUnroll(Ast_كلي* nodep, UnrollStats::Stat& stat) {
        m_ok = خطأ;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    منطقي unrollOneIteration(Ast_كلي* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        منطقي foundLoopTest = خطأ;
        // Process one body statement at a time
        لكل (Ast_كلي* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            إذا (AstLoopTest* ثابت testp = VN_CAST(stmtp, LoopTest)) {
                Ast_كلي* ثابت condp = V3Const::constifyEdit(testp->condp());
                إذا (condp->isZero()) {
                    // Loop terminates
                    أرجع خطأ;
                } وإلا إذا (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    استمر;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_بشيا(خطأ));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            إذا (!m_ok) أرجع خطأ;

            // Add statements to unrolled body
            طالما (Ast_كلي* ثابت nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                إذا (!m_unrollFull) {
                    m_unrolledSize += nodep->كلي_كدي();
                    إذا (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        أرجع خطأ;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                إذا (AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = true;
                    // Will not actually need it, nor any subsequent
                    تور_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    إذا (testp->condp()->isNeqZero()) استمر;
                    // Won't need any of the trailing statements
                    إذا (m_wrapp->stmtsp()) تور_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    إذا (testp->condp()->isZero()) أرجع خطأ;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    أرجع خطأ;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_كلي::addNext(m_stmtsp, nodep);

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
        إذا (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            أرجع خطأ;
        }
        // One iteration done, loop continues
        أرجع true;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    منطقي process(Ast_كلي* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        إذا (!nodep) أرجع true;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_كلي* np) {
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
            إذا (Ast_كلي_CCall* ثابت callp = VN_CAST(np, كلي_CCall)) {
                إذا (!callp->isPure()) {
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
                m_bindings.set(refp->varScopep(), nullptr);
                أرجع;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.تور_back(refp);
        });
        // Give up if we have to
        إذا (!m_ok) أرجع خطأ;
        // Actually substitute the variables that still have bindings
        لكل (AstVarRef* ثابت refp : toSubstitute) {
            // Pick up bound value
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            إذا (!valp) استمر;
            // Substitute it
            refp->replaceWith(valp->clone_بشيا(خطأ));
            VL_DO_DANGLING(تور_Deletep(refp), refp);
        }
        أرجع true;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        إذا (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            أرجع;
        }
        // Attempt to unroll the loop
        ثابت size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_كدي();
        size_t iter_كدي = 0;
        افعل {
            إذا (iter_كدي > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_كدي);
                إذا (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                أرجع;
            }
            ++iter_كدي;
        } طالما (unrollOneIteration(loopp->stmtsp()));

        إذا (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_كدي;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_بشيا(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    فراغ visit(Ast_كلي* nodep) override {
        إذا (!m_ok) أرجع;
        // Generic body statement, just substitute
        process(nodep);
    }
    فراغ visit(AstLoopTest* nodep) override {
        إذا (!m_ok) أرجع;
        // If the condition is a ExprStmt, move it before the LoopTest
        إذا (Ast_فرچ_Stmt* ثابت exprp = VN_CAST(nodep->condp(), فرچ_Stmt)) {
            Ast_كلي* ثابت stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(تور_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            أرجع;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    فراغ visit(AstAssign* nodep) override {
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
            m_bindings.set(lhsp->varScopep(), valp);
            أرجع;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    فراغ visit(AstIf* nodep) override {
        إذا (!m_ok) أرجع;
        إذا (!process(nodep->condp())) أرجع;
        // If condition is constant, replce with the relevant branch
        إذا (AstConst* ثابت condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            إذا (Ast_كلي* ثابت bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(تور_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    فراغ visit(AstJumpGo* nodep) override {
        إذا (!m_ok) أرجع;
        // Remove trailing dead code
        إذا (nodep->nextp()) تور_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    فراغ visit(AstLoop* nodep) override {
        إذا (!m_ok) أرجع;
        m_bindings.checkpoint();
        std::pair<Ast_كلي*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        إذا (!pair.second) {
            m_bindings.revert();
            process(nodep);
            أرجع;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        إذا (pair.first) {
            nodep->replaceWith(pair.first);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(تور_Deletep(nodep), nodep);
    }

عام:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ساكن std::pair<Ast_كلي*, منطقي> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        إذا (visitor.m_ok) أرجع {visitor.m_stmtsp, true};
        // Otherwise delete intermediate results
        إذا (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_بشيا(), visitor.m_stmtsp);
        أرجع {nullptr, خطأ};
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
    ساكن AstVarRef* unwrapToVarRef(Ast_كلي_فرچ* nodep, صحيح minWidth) {
        طالما (true) {
            إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) أرجع refp;
            إذا (AstExtend* ثابت ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } وإلا إذا (AstExtendS* ثابت ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } وإلا إذا (AstSel* ثابت sp = VN_CAST(nodep, Sel)) {
                ثابت AstConst* ثابت lsbp = VN_CAST(sp->lsbp(), Const);
                إذا (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) أرجع nullptr;
                nodep = sp->fromp();
            } وإلا {
                أرجع nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    منطقي isVarPlus1(Ast_كلي* nodep, ثابت AstVarScope* vscp) {
        AstAdd* ثابت addp = VN_CAST(nodep, Add);
        إذا (!addp || !addp->lhsp()->isOne()) أرجع خطأ;
        ثابت AstVarRef* ثابت r = unwrapToVarRef(addp->rhsp(), addp->width());
        أرجع r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ساكن Ast_كلي_فرچ* resizeToWidth(Ast_كلي_فرچ* exprp, ثابت AstVarRef* targetRefp) {
        ثابت صحيح width = targetRefp->width();
        إذا (width == 32) أرجع exprp;
        File_لي* ثابت flp = exprp->fileline();
        إذا (width < 32) أرجع جديد AstSel{flp, exprp, 0, width};
        AstExtend* ثابت extp = جديد AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        أرجع extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ساكن منطقي ascendingBound(Ast_كلي_فرچ* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        إذا (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) أرجع خطأ;
        Ast_كلي_Biop* ثابت bp = VN_AS(condp, كلي_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        أرجع wp && idxRefp && !wp->لمب().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ساكن منطقي isInRangeGuard(Ast_كلي_فرچ* condp, ثابت AstVarScope* idxVscp, uint32_t width,
                               صحيح addrBits) {
        إذا (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) أرجع خطأ;
        Ast_كلي_Biop* ثابت bp = VN_AS(condp, كلي_Biop);
        ثابت AstConst* ثابت cp = VN_CAST(bp->lhsp(), Const);
        إذا (!cp || cp->لمب().isFourState() || cp->toUInt() < width - 1) أرجع خطأ;
        ثابت AstVarRef* ثابت r = unwrapToVarRef(bp->rhsp(), addrBits);
        أرجع r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    منطقي tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* ثابت testp = VN_CAST(loopp->stmtsp(), LoopTest);
        إذا (!testp) أرجع خطأ;
        AstIf* ثابت ifp = VN_CAST(testp->nextp(), If);
        إذا (!ifp) أرجع خطأ;
        AstAssign* ثابت incp = VN_CAST(ifp->nextp(), Assign);
        إذا (!incp || incp->nextp()) أرجع خطأ;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        إذا (!ascendingBound(testp->condp(), wp, idxRefp)) أرجع خطأ;
        AstVarScope* ثابت idxVscp = idxRefp->varScopep();
        ثابت uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ثابت صحيح addrBits = width <= 1 ? 1 : V3_لمب::log2b(width - 1) + 1;
        ثابت AstConst* ثابت idxInitp = m_bindings.get(idxVscp);
        إذا (!idxInitp || !idxInitp->isZero()) أرجع خطأ;
        AstVarRef* ثابت incLhsp = VN_CAST(incp->lhsp(), VarRef);
        إذا (!incLhsp || incLhsp->varScopep() != idxVscp) أرجع خطأ;
        إذا (!isVarPlus1(incp->rhsp(), idxVscp)) أرجع خطأ;
        إذا (ifp->elsesp()) أرجع خطأ;
        AstAssign* ثابت thenp = VN_CAST(ifp->thensp(), Assign);
        إذا (!thenp || thenp->nextp()) أرجع خطأ;
        AstVarRef* ثابت targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        إذا (!targetRefp) أرجع خطأ;
        AstVarScope* ثابت targetVscp = targetRefp->varScopep();
        إذا (targetVscp == idxVscp) أرجع خطأ;
        ثابت منطقي isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ثابت منطقي is_كدي_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        إذا (!isLeadingOne && !is_كدي_Ones) أرجع خطأ;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_كلي_فرچ* condp = ifp->condp();
        إذا (AstLogAnd* ثابت andp = VN_CAST(condp, LogAnd)) {
            إذا (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } وإلا إذا (AstCond* ثابت ternp = VN_CAST(condp, Cond)) {
            إذا (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ثابت selp = VN_CAST(condp, Sel);
        إذا (!selp || selp->width() != 1) أرجع خطأ;
        ثابت AstVarRef* ثابت fromp = VN_CAST(selp->fromp(), VarRef);
        إذا (!fromp) أرجع خطأ;
        ثابت AstVarScope* ثابت fromVscp = fromp->varScopep();
        إذا (fromVscp == idxVscp || fromVscp == targetVscp) أرجع خطأ;
        Ast_كلي_فرچ* ثابت vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        إذا (static_cast<صحيح>(width) != vecExprp->width()) أرجع خطأ;
        ثابت AstVarRef* ثابت idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        إذا (!idxInSel || idxInSel->varScopep() != idxVscp) أرجع خطأ;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ثابت AstConst* ثابت targetInitp = m_bindings.get(targetVscp);
        إذا (!targetInitp || !targetInitp->isZero()) أرجع خطأ;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_لي* ثابت flp = loopp->fileline();
        Ast_كلي_فرچ* reducep;
        إذا (isLeadingOne) {
            reducep = جديد AstMostSetBitP1{flp, vecExprp->clone_بشيا(خطأ)};
        } وإلا {
            Ast_كدي_Ones* ثابت conep = جديد Ast_كدي_Ones{flp, vecExprp->clone_بشيا(خطأ)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ثابت newp = جديد AstAssign{flp, targetRefp->clone_بشيا(خطأ), reducep};
        newp->addNext(جديد AstAssign{flp, incLhsp->clone_بشيا(خطأ), wp->clone_بشيا(خطأ)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(تور_Deletep(loopp), loopp);
        إذا (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } وإلا {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_كدي_Ones_Lowered;
        }
        أرجع true;
    }

    // VISIT
    فراغ visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        لكل (Ast_كلي *succp = nodep, *currp = nodep->backp(); true;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            إذا (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                إذا (VN_IS(currp, JumpBlock)) استمر;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                اكسر;
            }
            AstAssign* ثابت assignp = VN_CAST(currp, Assign);
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
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        إذا (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) أرجع;

        // Attempt to unroll this loop
        ثابت std::pair<Ast_كلي*, منطقي> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        إذا (!pair.second) {
            iterateChildren(nodep);
            أرجع;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        إذا (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(تور_Deletep(nodep), nodep);
        } وإلا {
            VL_DO_DANGLING(تور_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    فراغ visit(Ast_كلي* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

عام:
    ساكن فراغ apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

فراغ V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_بشيا("unroll", 0, dump_بشيا_Either_Level() >= 3);
}
