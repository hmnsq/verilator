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

ساختار UnrollStats final {
    رده Stat final {
        size_t m_مقدار = 0;  // Statistics value
        ثابت نویسه* ثابت m_name;  // Name for stats file and UDEBUG

    عمومی:
        explicit Stat(ثابت نویسه* ثابت name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_مقدار); }
        ثابت نویسه* name() ثابت { بازگشت m_name; }
        Stat& operator++() {
            ++m_مقدار;
            بازگشت *این;
        }
        Stat& operator+=(size_t v) {
            m_مقدار += v;
            بازگشت *این;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_شمار{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_شمار_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
رده UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ثابت VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

عمومی:
    // CONSTRUCTOR
    UnrolllBindings() = پیشفرض;
    ~UnrolllBindings() = پیشفرض;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    تهی clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_گره::user1_Clear_درخت();
    }

    تهی checkpoint() { m_checkpoints.افزودن_back(m_curr); }

    تهی revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.برداشت_back();
    }

    تهی commit() { m_checkpoints.برداشت_back(); }

    تهی set(AstVarScope* vscp, AstConst* valp) {
        اگر (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.افزودن_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        اگر (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ثابت valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        بازگشت valp;
    }
};

//######################################################################
// Unroll one AstLoop

رده UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ثابت m_loopp;  // The loop we are trying to unroll
    Ast_گره* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ثابت m_wrapp = جدید AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, نادرست};
    ثابت منطقی m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    منطقی m_ok = درست;  // Unrolling successful so far, gave up if false

    // METHODS
    تهی cantUnroll(Ast_گره* nodep, UnrollStats::Stat& stat) {
        m_ok = نادرست;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    منطقی unrollOneIteration(Ast_گره* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        منطقی foundLoopTest = نادرست;
        // Process one body statement at a time
        برای (Ast_گره* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            اگر (AstLoopTest* ثابت testp = VN_CAST(stmtp, LoopTest)) {
                Ast_گره* ثابت condp = V3Const::constifyEdit(testp->condp());
                اگر (condp->isZero()) {
                    // Loop terminates
                    بازگشت نادرست;
                } وگرنه اگر (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    ادامه;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_درخت(نادرست));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            اگر (!m_ok) بازگشت نادرست;

            // Add statements to unrolled body
            وقتی (Ast_گره* ثابت nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                اگر (!m_unrollFull) {
                    m_unrolledSize += nodep->گره_شمار();
                    اگر (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        بازگشت نادرست;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                اگر (AstLoopTest* ثابت testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = درست;
                    // Will not actually need it, nor any subsequent
                    افزودن_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    اگر (testp->condp()->isNeqZero()) ادامه;
                    // Won't need any of the trailing statements
                    اگر (m_wrapp->stmtsp()) افزودن_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    اگر (testp->condp()->isZero()) بازگشت نادرست;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    بازگشت نادرست;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_گره::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                اگر (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    بازگشت نادرست;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        اگر (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            بازگشت نادرست;
        }
        // One iteration done, loop continues
        بازگشت درست;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    منطقی process(Ast_گره* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        اگر (!nodep) بازگشت درست;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_گره* np) {
            // Failed earlier
            اگر (!m_ok) بازگشت;
            // Check for AstLoopTest
            اگر (AstLoopTest* ثابت testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                اگر (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                بازگشت;
            }
            // Check for AstFork - can't unroll
            اگر (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                بازگشت;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            اگر (Ast_گره_CCall* ثابت callp = VN_CAST(np, گره_CCall)) {
                اگر (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    بازگشت;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            اگر (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                بازگشت;
            }
            // Process variable references
            AstVarRef* ثابت refp = VN_CAST(np, VarRef);
            اگر (!refp) بازگشت;
            // Ignore if the referenced variable has no binding
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            اگر (!valp) بازگشت;
            // If writen, remove the binding
            اگر (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                بازگشت;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.افزودن_back(refp);
        });
        // Give up if we have to
        اگر (!m_ok) بازگشت نادرست;
        // Actually substitute the variables that still have bindings
        برای (AstVarRef* ثابت refp : toSubstitute) {
            // Pick up bound value
            AstConst* ثابت valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            اگر (!valp) ادامه;
            // Substitute it
            refp->replaceWith(valp->clone_درخت(نادرست));
            VL_DO_DANGLING(افزودن_Deletep(refp), refp);
        }
        بازگشت درست;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        اگر (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            بازگشت;
        }
        // Attempt to unroll the loop
        ثابت size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_شمار();
        size_t iter_شمار = 0;
        انجامبده {
            اگر (iter_شمار > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_شمار);
                اگر (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                بازگشت;
            }
            ++iter_شمار;
        } وقتی (unrollOneIteration(loopp->stmtsp()));

        اگر (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_شمار;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_درخت(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    تهی visit(Ast_گره* nodep) override {
        اگر (!m_ok) بازگشت;
        // Generic body statement, just substitute
        process(nodep);
    }
    تهی visit(AstLoopTest* nodep) override {
        اگر (!m_ok) بازگشت;
        // If the condition is a ExprStmt, move it before the LoopTest
        اگر (Ast_عبارت_Stmt* ثابت exprp = VN_CAST(nodep->condp(), عبارت_Stmt)) {
            Ast_گره* ثابت stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            بازگشت;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    تهی visit(AstAssign* nodep) override {
        اگر (!m_ok) بازگشت;
        // Can't do it if delayed
        اگر (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            بازگشت;
        }
        اگر (!process(nodep->rhsp())) بازگشت;
        // If a simple variable assignment, update the binding
        AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        اگر (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            بازگشت;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    تهی visit(AstIf* nodep) override {
        اگر (!m_ok) بازگشت;
        اگر (!process(nodep->condp())) بازگشت;
        // If condition is constant, replce with the relevant branch
        اگر (AstConst* ثابت condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            اگر (Ast_گره* ثابت bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    تهی visit(AstJumpGo* nodep) override {
        اگر (!m_ok) بازگشت;
        // Remove trailing dead code
        اگر (nodep->nextp()) افزودن_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    تهی visit(AstLoop* nodep) override {
        اگر (!m_ok) بازگشت;
        m_bindings.checkpoint();
        std::pair<Ast_گره*, منطقی> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        اگر (!pair.second) {
            m_bindings.revert();
            process(nodep);
            بازگشت;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        اگر (pair.first) {
            nodep->replaceWith(pair.first);
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

عمومی:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    ایستا std::pair<Ast_گره*, منطقی> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        اگر (visitor.m_ok) بازگشت {visitor.m_stmtsp, درست};
        // Otherwise delete intermediate results
        اگر (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_درخت(), visitor.m_stmtsp);
        بازگشت {nullptr, نادرست};
    }
};

//######################################################################
// Unroll all AstLoop statements

رده UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    ایستا AstVarRef* unwrapToVarRef(Ast_گره_عبارت* nodep, صحیح minWidth) {
        وقتی (درست) {
            اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) بازگشت refp;
            اگر (AstExtend* ثابت ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } وگرنه اگر (AstExtendS* ثابت ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } وگرنه اگر (AstSel* ثابت sp = VN_CAST(nodep, Sel)) {
                ثابت AstConst* ثابت lsbp = VN_CAST(sp->lsbp(), Const);
                اگر (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) بازگشت nullptr;
                nodep = sp->fromp();
            } وگرنه {
                بازگشت nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    منطقی isVarPlus1(Ast_گره* nodep, ثابت AstVarScope* vscp) {
        AstAdd* ثابت addp = VN_CAST(nodep, Add);
        اگر (!addp || !addp->lhsp()->isOne()) بازگشت نادرست;
        ثابت AstVarRef* ثابت r = unwrapToVarRef(addp->rhsp(), addp->width());
        بازگشت r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    ایستا Ast_گره_عبارت* resizeToWidth(Ast_گره_عبارت* exprp, ثابت AstVarRef* targetRefp) {
        ثابت صحیح width = targetRefp->width();
        اگر (width == 32) بازگشت exprp;
        File_سطر* ثابت flp = exprp->fileline();
        اگر (width < 32) بازگشت جدید AstSel{flp, exprp, 0, width};
        AstExtend* ثابت extp = جدید AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        بازگشت extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    ایستا منطقی ascendingBound(Ast_گره_عبارت* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        اگر (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) بازگشت نادرست;
        Ast_گره_Biop* ثابت bp = VN_AS(condp, گره_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        بازگشت wp && idxRefp && !wp->رقم().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    ایستا منطقی isInRangeGuard(Ast_گره_عبارت* condp, ثابت AstVarScope* idxVscp, uint32_t width,
                               صحیح addrBits) {
        اگر (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) بازگشت نادرست;
        Ast_گره_Biop* ثابت bp = VN_AS(condp, گره_Biop);
        ثابت AstConst* ثابت cp = VN_CAST(bp->lhsp(), Const);
        اگر (!cp || cp->رقم().isFourState() || cp->toUInt() < width - 1) بازگشت نادرست;
        ثابت AstVarRef* ثابت r = unwrapToVarRef(bp->rhsp(), addrBits);
        بازگشت r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    منطقی tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* ثابت testp = VN_CAST(loopp->stmtsp(), LoopTest);
        اگر (!testp) بازگشت نادرست;
        AstIf* ثابت ifp = VN_CAST(testp->nextp(), If);
        اگر (!ifp) بازگشت نادرست;
        AstAssign* ثابت incp = VN_CAST(ifp->nextp(), Assign);
        اگر (!incp || incp->nextp()) بازگشت نادرست;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        اگر (!ascendingBound(testp->condp(), wp, idxRefp)) بازگشت نادرست;
        AstVarScope* ثابت idxVscp = idxRefp->varScopep();
        ثابت uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ثابت صحیح addrBits = width <= 1 ? 1 : V3_عدد::log2b(width - 1) + 1;
        ثابت AstConst* ثابت idxInitp = m_bindings.get(idxVscp);
        اگر (!idxInitp || !idxInitp->isZero()) بازگشت نادرست;
        AstVarRef* ثابت incLhsp = VN_CAST(incp->lhsp(), VarRef);
        اگر (!incLhsp || incLhsp->varScopep() != idxVscp) بازگشت نادرست;
        اگر (!isVarPlus1(incp->rhsp(), idxVscp)) بازگشت نادرست;
        اگر (ifp->elsesp()) بازگشت نادرست;
        AstAssign* ثابت thenp = VN_CAST(ifp->thensp(), Assign);
        اگر (!thenp || thenp->nextp()) بازگشت نادرست;
        AstVarRef* ثابت targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        اگر (!targetRefp) بازگشت نادرست;
        AstVarScope* ثابت targetVscp = targetRefp->varScopep();
        اگر (targetVscp == idxVscp) بازگشت نادرست;
        ثابت منطقی isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ثابت منطقی is_شمار_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        اگر (!isLeadingOne && !is_شمار_Ones) بازگشت نادرست;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_گره_عبارت* condp = ifp->condp();
        اگر (AstLogAnd* ثابت andp = VN_CAST(condp, LogAnd)) {
            اگر (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } وگرنه اگر (AstCond* ثابت ternp = VN_CAST(condp, Cond)) {
            اگر (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ثابت selp = VN_CAST(condp, Sel);
        اگر (!selp || selp->width() != 1) بازگشت نادرست;
        ثابت AstVarRef* ثابت fromp = VN_CAST(selp->fromp(), VarRef);
        اگر (!fromp) بازگشت نادرست;
        ثابت AstVarScope* ثابت fromVscp = fromp->varScopep();
        اگر (fromVscp == idxVscp || fromVscp == targetVscp) بازگشت نادرست;
        Ast_گره_عبارت* ثابت vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        اگر (static_cast<صحیح>(width) != vecExprp->width()) بازگشت نادرست;
        ثابت AstVarRef* ثابت idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        اگر (!idxInSel || idxInSel->varScopep() != idxVscp) بازگشت نادرست;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ثابت AstConst* ثابت targetInitp = m_bindings.get(targetVscp);
        اگر (!targetInitp || !targetInitp->isZero()) بازگشت نادرست;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_سطر* ثابت flp = loopp->fileline();
        Ast_گره_عبارت* reducep;
        اگر (isLeadingOne) {
            reducep = جدید AstMostSetBitP1{flp, vecExprp->clone_درخت(نادرست)};
        } وگرنه {
            Ast_شمار_Ones* ثابت conep = جدید Ast_شمار_Ones{flp, vecExprp->clone_درخت(نادرست)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ثابت newp = جدید AstAssign{flp, targetRefp->clone_درخت(نادرست), reducep};
        newp->addNext(جدید AstAssign{flp, incLhsp->clone_درخت(نادرست), wp->clone_درخت(نادرست)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(loopp), loopp);
        اگر (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } وگرنه {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_شمار_Ones_Lowered;
        }
        بازگشت درست;
    }

    // VISIT
    تهی visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        برای (Ast_گره *succp = nodep, *currp = nodep->backp(); درست;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            اگر (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                اگر (VN_IS(currp, JumpBlock)) ادامه;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                بشکن;
            }
            AstAssign* ثابت assignp = VN_CAST(currp, Assign);
            اگر (!assignp) بشکن;
            AstConst* ثابت valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            اگر (!valp) بشکن;
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            اگر (!lhsp) بشکن;
            // Don't bind if volatile
            اگر (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) ادامه;
            // Don't overwrite a later binding
            اگر (m_bindings.get(lhsp->varScopep())) ادامه;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        اگر (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) بازگشت;

        // Attempt to unroll this loop
        ثابت std::pair<Ast_گره*, منطقی> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        اگر (!pair.second) {
            iterateChildren(nodep);
            بازگشت;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        اگر (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه {
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

عمومی:
    ایستا تهی apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

تهی V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_درخت("unroll", 0, dump_درخت_Either_Level() >= 3);
}
