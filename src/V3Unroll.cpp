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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Unroll.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

संरचना UnrollStats final {
    वर्ग Stat final {
        माप_प्रकार m_मूल्य = 0;  // Statistics value
        ध्रुव वर्ण* ध्रुव m_name;  // Name for stats file and UDEBUG

    सार्वजनिक:
        explicit Stat(ध्रुव वर्ण* ध्रुव name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_मूल्य); }
        ध्रुव वर्ण* name() ध्रुव { प्रत्यागम m_name; }
        Stat& चालक++() {
            ++m_मूल्य;
            प्रत्यागम *एतत्;
        }
        Stat& चालक+=(माप_प्रकार v) {
            m_मूल्य += v;
            प्रत्यागम *एतत्;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_गिनती{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_गिनती_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
वर्ग UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ध्रुव VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

सार्वजनिक:
    // CONSTRUCTOR
    UnrolllBindings() = स्वभाव;
    ~UnrolllBindings() = स्वभाव;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    शून्य clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_गाँठ::user1_Clear_वृक्ष();
    }

    शून्य checkpoint() { m_checkpoints.धकेलो_back(m_curr); }

    शून्य revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.निकालो_back();
    }

    शून्य commit() { m_checkpoints.निकालो_back(); }

    शून्य set(AstVarScope* vscp, AstConst* valp) {
        यदि (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.धकेलो_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        यदि (static_cast<माप_प्रकार>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ध्रुव valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        प्रत्यागम valp;
    }
};

//######################################################################
// Unroll one AstLoop

वर्ग UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ध्रुव m_loopp;  // The loop we are trying to unroll
    Ast_गाँठ* m_stmtsp = nullptr;  // Resulting unrolled statement
    माप_प्रकार m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ध्रुव m_wrapp = नूतन AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, असत्य};
    ध्रुव तार्किक m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    तार्किक m_ok = सत्य;  // Unrolling successful so far, gave up if false

    // METHODS
    शून्य cantUnroll(Ast_गाँठ* nodep, UnrollStats::Stat& stat) {
        m_ok = असत्य;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    तार्किक unrollOneIteration(Ast_गाँठ* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        तार्किक foundLoopTest = असत्य;
        // Process one body statement at a time
        प्रति (Ast_गाँठ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            यदि (AstLoopTest* ध्रुव testp = VN_CAST(stmtp, LoopTest)) {
                Ast_गाँठ* ध्रुव condp = V3Const::constifyEdit(testp->condp());
                यदि (condp->isZero()) {
                    // Loop terminates
                    प्रत्यागम असत्य;
                } अन्यथा यदि (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    अनुवर्त;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_वृक्ष(असत्य));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            यदि (!m_ok) प्रत्यागम असत्य;

            // Add statements to unrolled body
            यावत् (Ast_गाँठ* ध्रुव nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                यदि (!m_unrollFull) {
                    m_unrolledSize += nodep->गाँठ_गिनती();
                    यदि (m_unrolledSize > static_cast<माप_प्रकार>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        प्रत्यागम असत्य;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                यदि (AstLoopTest* ध्रुव testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = सत्य;
                    // Will not actually need it, nor any subsequent
                    धकेलो_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    यदि (testp->condp()->isNeqZero()) अनुवर्त;
                    // Won't need any of the trailing statements
                    यदि (m_wrapp->stmtsp()) धकेलो_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    यदि (testp->condp()->isZero()) प्रत्यागम असत्य;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    प्रत्यागम असत्य;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_गाँठ::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                यदि (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    प्रत्यागम असत्य;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        यदि (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            प्रत्यागम असत्य;
        }
        // One iteration done, loop continues
        प्रत्यागम सत्य;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    तार्किक process(Ast_गाँठ* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        यदि (!nodep) प्रत्यागम सत्य;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_गाँठ* np) {
            // Failed earlier
            यदि (!m_ok) प्रत्यागम;
            // Check for AstLoopTest
            यदि (AstLoopTest* ध्रुव testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                यदि (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                प्रत्यागम;
            }
            // Check for AstFork - can't unroll
            यदि (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                प्रत्यागम;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            यदि (Ast_गाँठ_CCall* ध्रुव callp = VN_CAST(np, गाँठ_CCall)) {
                यदि (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    प्रत्यागम;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            यदि (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                प्रत्यागम;
            }
            // Process variable references
            AstVarRef* ध्रुव refp = VN_CAST(np, VarRef);
            यदि (!refp) प्रत्यागम;
            // Ignore if the referenced variable has no binding
            AstConst* ध्रुव valp = m_bindings.get(refp->varScopep());
            यदि (!valp) प्रत्यागम;
            // If writen, remove the binding
            यदि (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                प्रत्यागम;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.धकेलो_back(refp);
        });
        // Give up if we have to
        यदि (!m_ok) प्रत्यागम असत्य;
        // Actually substitute the variables that still have bindings
        प्रति (AstVarRef* ध्रुव refp : toSubstitute) {
            // Pick up bound value
            AstConst* ध्रुव valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            यदि (!valp) अनुवर्त;
            // Substitute it
            refp->replaceWith(valp->clone_वृक्ष(असत्य));
            VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
        }
        प्रत्यागम सत्य;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        यदि (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            प्रत्यागम;
        }
        // Attempt to unroll the loop
        ध्रुव माप_प्रकार iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_गिनती();
        माप_प्रकार iter_गिनती = 0;
        कुरु {
            यदि (iter_गिनती > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_गिनती);
                यदि (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                प्रत्यागम;
            }
            ++iter_गिनती;
        } यावत् (unrollOneIteration(loopp->stmtsp()));

        यदि (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_गिनती;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_वृक्ष(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        // Generic body statement, just substitute
        process(nodep);
    }
    शून्य visit(AstLoopTest* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        // If the condition is a ExprStmt, move it before the LoopTest
        यदि (Ast_व्यंजक_Stmt* ध्रुव exprp = VN_CAST(nodep->condp(), व्यंजक_Stmt)) {
            Ast_गाँठ* ध्रुव stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(धकेलो_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            प्रत्यागम;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    शून्य visit(AstAssign* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        // Can't do it if delayed
        यदि (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            प्रत्यागम;
        }
        यदि (!process(nodep->rhsp())) प्रत्यागम;
        // If a simple variable assignment, update the binding
        AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ध्रुव valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        यदि (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            प्रत्यागम;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    शून्य visit(AstIf* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        यदि (!process(nodep->condp())) प्रत्यागम;
        // If condition is constant, replce with the relevant branch
        यदि (AstConst* ध्रुव condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            यदि (Ast_गाँठ* ध्रुव bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            प्रत्यागम;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    शून्य visit(AstJumpGo* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        // Remove trailing dead code
        यदि (nodep->nextp()) धकेलो_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    शून्य visit(AstLoop* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        m_bindings.checkpoint();
        std::pair<Ast_गाँठ*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        यदि (!pair.second) {
            m_bindings.revert();
            process(nodep);
            प्रत्यागम;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        यदि (pair.first) {
            nodep->replaceWith(pair.first);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

सार्वजनिक:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    स्थिर std::pair<Ast_गाँठ*, तार्किक> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        यदि (visitor.m_ok) प्रत्यागम {visitor.m_stmtsp, सत्य};
        // Otherwise delete intermediate results
        यदि (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_वृक्ष(), visitor.m_stmtsp);
        प्रत्यागम {nullptr, असत्य};
    }
};

//######################################################################
// Unroll all AstLoop statements

वर्ग UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    स्थिर AstVarRef* unwrapToVarRef(Ast_गाँठ_व्यंजक* nodep, पूर्णाङ्क minWidth) {
        यावत् (सत्य) {
            यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) प्रत्यागम refp;
            यदि (AstExtend* ध्रुव ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } अन्यथा यदि (AstExtendS* ध्रुव ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } अन्यथा यदि (AstSel* ध्रुव sp = VN_CAST(nodep, Sel)) {
                ध्रुव AstConst* ध्रुव lsbp = VN_CAST(sp->lsbp(), Const);
                यदि (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) प्रत्यागम nullptr;
                nodep = sp->fromp();
            } अन्यथा {
                प्रत्यागम nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    तार्किक isVarPlus1(Ast_गाँठ* nodep, ध्रुव AstVarScope* vscp) {
        AstAdd* ध्रुव addp = VN_CAST(nodep, Add);
        यदि (!addp || !addp->lhsp()->isOne()) प्रत्यागम असत्य;
        ध्रुव AstVarRef* ध्रुव r = unwrapToVarRef(addp->rhsp(), addp->width());
        प्रत्यागम r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    स्थिर Ast_गाँठ_व्यंजक* resizeToWidth(Ast_गाँठ_व्यंजक* exprp, ध्रुव AstVarRef* targetRefp) {
        ध्रुव पूर्णाङ्क width = targetRefp->width();
        यदि (width == 32) प्रत्यागम exprp;
        File_पंक्ति* ध्रुव flp = exprp->fileline();
        यदि (width < 32) प्रत्यागम नूतन AstSel{flp, exprp, 0, width};
        AstExtend* ध्रुव extp = नूतन AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        प्रत्यागम extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    स्थिर तार्किक ascendingBound(Ast_गाँठ_व्यंजक* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        यदि (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) प्रत्यागम असत्य;
        Ast_गाँठ_Biop* ध्रुव bp = VN_AS(condp, गाँठ_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        प्रत्यागम wp && idxRefp && !wp->अंक().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    स्थिर तार्किक isInRangeGuard(Ast_गाँठ_व्यंजक* condp, ध्रुव AstVarScope* idxVscp, uint32_t width,
                               पूर्णाङ्क addrBits) {
        यदि (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) प्रत्यागम असत्य;
        Ast_गाँठ_Biop* ध्रुव bp = VN_AS(condp, गाँठ_Biop);
        ध्रुव AstConst* ध्रुव cp = VN_CAST(bp->lhsp(), Const);
        यदि (!cp || cp->अंक().isFourState() || cp->toUInt() < width - 1) प्रत्यागम असत्य;
        ध्रुव AstVarRef* ध्रुव r = unwrapToVarRef(bp->rhsp(), addrBits);
        प्रत्यागम r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    तार्किक tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* ध्रुव testp = VN_CAST(loopp->stmtsp(), LoopTest);
        यदि (!testp) प्रत्यागम असत्य;
        AstIf* ध्रुव ifp = VN_CAST(testp->nextp(), If);
        यदि (!ifp) प्रत्यागम असत्य;
        AstAssign* ध्रुव incp = VN_CAST(ifp->nextp(), Assign);
        यदि (!incp || incp->nextp()) प्रत्यागम असत्य;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        यदि (!ascendingBound(testp->condp(), wp, idxRefp)) प्रत्यागम असत्य;
        AstVarScope* ध्रुव idxVscp = idxRefp->varScopep();
        ध्रुव uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ध्रुव पूर्णाङ्क addrBits = width <= 1 ? 1 : V3_संख्या::log2b(width - 1) + 1;
        ध्रुव AstConst* ध्रुव idxInitp = m_bindings.get(idxVscp);
        यदि (!idxInitp || !idxInitp->isZero()) प्रत्यागम असत्य;
        AstVarRef* ध्रुव incLhsp = VN_CAST(incp->lhsp(), VarRef);
        यदि (!incLhsp || incLhsp->varScopep() != idxVscp) प्रत्यागम असत्य;
        यदि (!isVarPlus1(incp->rhsp(), idxVscp)) प्रत्यागम असत्य;
        यदि (ifp->elsesp()) प्रत्यागम असत्य;
        AstAssign* ध्रुव thenp = VN_CAST(ifp->thensp(), Assign);
        यदि (!thenp || thenp->nextp()) प्रत्यागम असत्य;
        AstVarRef* ध्रुव targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        यदि (!targetRefp) प्रत्यागम असत्य;
        AstVarScope* ध्रुव targetVscp = targetRefp->varScopep();
        यदि (targetVscp == idxVscp) प्रत्यागम असत्य;
        ध्रुव तार्किक isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ध्रुव तार्किक is_गिनती_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        यदि (!isLeadingOne && !is_गिनती_Ones) प्रत्यागम असत्य;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_गाँठ_व्यंजक* condp = ifp->condp();
        यदि (AstLogAnd* ध्रुव andp = VN_CAST(condp, LogAnd)) {
            यदि (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } अन्यथा यदि (AstCond* ध्रुव ternp = VN_CAST(condp, Cond)) {
            यदि (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ध्रुव selp = VN_CAST(condp, Sel);
        यदि (!selp || selp->width() != 1) प्रत्यागम असत्य;
        ध्रुव AstVarRef* ध्रुव fromp = VN_CAST(selp->fromp(), VarRef);
        यदि (!fromp) प्रत्यागम असत्य;
        ध्रुव AstVarScope* ध्रुव fromVscp = fromp->varScopep();
        यदि (fromVscp == idxVscp || fromVscp == targetVscp) प्रत्यागम असत्य;
        Ast_गाँठ_व्यंजक* ध्रुव vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        यदि (static_cast<पूर्णाङ्क>(width) != vecExprp->width()) प्रत्यागम असत्य;
        ध्रुव AstVarRef* ध्रुव idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        यदि (!idxInSel || idxInSel->varScopep() != idxVscp) प्रत्यागम असत्य;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ध्रुव AstConst* ध्रुव targetInitp = m_bindings.get(targetVscp);
        यदि (!targetInitp || !targetInitp->isZero()) प्रत्यागम असत्य;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_पंक्ति* ध्रुव flp = loopp->fileline();
        Ast_गाँठ_व्यंजक* reducep;
        यदि (isLeadingOne) {
            reducep = नूतन AstMostSetBitP1{flp, vecExprp->clone_वृक्ष(असत्य)};
        } अन्यथा {
            Ast_गिनती_Ones* ध्रुव conep = नूतन Ast_गिनती_Ones{flp, vecExprp->clone_वृक्ष(असत्य)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ध्रुव newp = नूतन AstAssign{flp, targetRefp->clone_वृक्ष(असत्य), reducep};
        newp->addNext(नूतन AstAssign{flp, incLhsp->clone_वृक्ष(असत्य), wp->clone_वृक्ष(असत्य)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(loopp), loopp);
        यदि (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } अन्यथा {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_गिनती_Ones_Lowered;
        }
        प्रत्यागम सत्य;
    }

    // VISIT
    शून्य visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        प्रति (Ast_गाँठ *succp = nodep, *currp = nodep->backp(); सत्य;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            यदि (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                यदि (VN_IS(currp, JumpBlock)) अनुवर्त;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                भङ्ग;
            }
            AstAssign* ध्रुव assignp = VN_CAST(currp, Assign);
            यदि (!assignp) भङ्ग;
            AstConst* ध्रुव valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            यदि (!valp) भङ्ग;
            AstVarRef* ध्रुव lhsp = VN_CAST(assignp->lhsp(), VarRef);
            यदि (!lhsp) भङ्ग;
            // Don't bind if volatile
            यदि (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) अनुवर्त;
            // Don't overwrite a later binding
            यदि (m_bindings.get(lhsp->varScopep())) अनुवर्त;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        यदि (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) प्रत्यागम;

        // Attempt to unroll this loop
        ध्रुव std::pair<Ast_गाँठ*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        यदि (!pair.second) {
            iterateChildren(nodep);
            प्रत्यागम;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        यदि (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा {
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

सार्वजनिक:
    स्थिर शून्य apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

शून्य V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_वृक्ष("unroll", 0, dump_वृक्ष_Either_Level() >= 3);
}
