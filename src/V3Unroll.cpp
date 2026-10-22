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
    वग्ग Stat final {
        size_t m_मानम् = 0;  // Statistics value
        ध्रुव वर्ण* ध्रुव m_name;  // Name for stats file and UDEBUG

    सार्वजनिक:
        explicit Stat(ध्रुव वर्ण* ध्रुव name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_मानम्); }
        ध्रुव वर्ण* name() ध्रुव { पटिनिवत्त m_name; }
        Stat& operator++() {
            ++m_मानम्;
            पटिनिवत्त *एतत्;
        }
        Stat& operator+=(size_t v) {
            m_मानम् += v;
            पटिनिवत्त *एतत्;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_गणनम्{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_गणनम्_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
वग्ग UnrolllBindings final {
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
        Ast_ग्रन्थिः::user1_Clear_वृक्षः();
    }

    शून्य checkpoint() { m_checkpoints.प्रक्षिप_back(m_curr); }

    शून्य revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.निष्कासय_back();
    }

    शून्य commit() { m_checkpoints.निष्कासय_back(); }

    शून्य set(AstVarScope* vscp, AstConst* valp) {
        सचे (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.प्रक्षिप_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        सचे (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* ध्रुव valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        पटिनिवत्त valp;
    }
};

//######################################################################
// Unroll one AstLoop

वग्ग UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* ध्रुव m_loopp;  // The loop we are trying to unroll
    Ast_ग्रन्थिः* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ध्रुव m_wrapp = नूतन AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, मिच्छा};
    ध्रुव तार्किक m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    तार्किक m_ok = सच्च;  // Unrolling successful so far, gave up if false

    // METHODS
    शून्य cantUnroll(Ast_ग्रन्थिः* nodep, UnrollStats::Stat& stat) {
        m_ok = मिच्छा;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    तार्किक unrollOneIteration(Ast_ग्रन्थिः* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        तार्किक foundLoopTest = मिच्छा;
        // Process one body statement at a time
        अत्थाय (Ast_ग्रन्थिः* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            सचे (AstLoopTest* ध्रुव testp = VN_CAST(stmtp, LoopTest)) {
                Ast_ग्रन्थिः* ध्रुव condp = V3Const::constifyEdit(testp->condp());
                सचे (condp->isZero()) {
                    // Loop terminates
                    पटिनिवत्त मिच्छा;
                } अञ्ञथा सचे (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    अनुवर्त;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_वृक्षः(मिच्छा));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            सचे (!m_ok) पटिनिवत्त मिच्छा;

            // Add statements to unrolled body
            याव (Ast_ग्रन्थिः* ध्रुव nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                सचे (!m_unrollFull) {
                    m_unrolledSize += nodep->ग्रन्थिः_गणनम्();
                    सचे (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        पटिनिवत्त मिच्छा;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                सचे (AstLoopTest* ध्रुव testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = सच्च;
                    // Will not actually need it, nor any subsequent
                    प्रक्षिप_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    सचे (testp->condp()->isNeqZero()) अनुवर्त;
                    // Won't need any of the trailing statements
                    सचे (m_wrapp->stmtsp()) प्रक्षिप_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    सचे (testp->condp()->isZero()) पटिनिवत्त मिच्छा;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    पटिनिवत्त मिच्छा;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_ग्रन्थिः::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                सचे (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    पटिनिवत्त मिच्छा;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        सचे (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            पटिनिवत्त मिच्छा;
        }
        // One iteration done, loop continues
        पटिनिवत्त सच्च;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    तार्किक process(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        सचे (!nodep) पटिनिवत्त सच्च;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_ग्रन्थिः* np) {
            // Failed earlier
            सचे (!m_ok) पटिनिवत्त;
            // Check for AstLoopTest
            सचे (AstLoopTest* ध्रुव testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                सचे (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                पटिनिवत्त;
            }
            // Check for AstFork - can't unroll
            सचे (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                पटिनिवत्त;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            सचे (Ast_ग्रन्थिः_CCall* ध्रुव callp = VN_CAST(np, ग्रन्थिः_CCall)) {
                सचे (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    पटिनिवत्त;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            सचे (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                पटिनिवत्त;
            }
            // Process variable references
            AstVarRef* ध्रुव refp = VN_CAST(np, VarRef);
            सचे (!refp) पटिनिवत्त;
            // Ignore if the referenced variable has no binding
            AstConst* ध्रुव valp = m_bindings.get(refp->varScopep());
            सचे (!valp) पटिनिवत्त;
            // If writen, remove the binding
            सचे (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                पटिनिवत्त;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.प्रक्षिप_back(refp);
        });
        // Give up if we have to
        सचे (!m_ok) पटिनिवत्त मिच्छा;
        // Actually substitute the variables that still have bindings
        अत्थाय (AstVarRef* ध्रुव refp : toSubstitute) {
            // Pick up bound value
            AstConst* ध्रुव valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            सचे (!valp) अनुवर्त;
            // Substitute it
            refp->replaceWith(valp->clone_वृक्षः(मिच्छा));
            VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
        }
        पटिनिवत्त सच्च;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        सचे (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            पटिनिवत्त;
        }
        // Attempt to unroll the loop
        ध्रुव size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_गणनम्();
        size_t iter_गणनम् = 0;
        करोहि {
            सचे (iter_गणनम् > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_गणनम्);
                सचे (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                पटिनिवत्त;
            }
            ++iter_गणनम्;
        } याव (unrollOneIteration(loopp->stmtsp()));

        सचे (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_गणनम्;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_वृक्षः(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        सचे (!m_ok) पटिनिवत्त;
        // Generic body statement, just substitute
        process(nodep);
    }
    शून्य visit(AstLoopTest* nodep) override {
        सचे (!m_ok) पटिनिवत्त;
        // If the condition is a ExprStmt, move it before the LoopTest
        सचे (Ast_व्यञ्जकम्_Stmt* ध्रुव exprp = VN_CAST(nodep->condp(), व्यञ्जकम्_Stmt)) {
            Ast_ग्रन्थिः* ध्रुव stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(प्रक्षिप_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            पटिनिवत्त;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    शून्य visit(AstAssign* nodep) override {
        सचे (!m_ok) पटिनिवत्त;
        // Can't do it if delayed
        सचे (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            पटिनिवत्त;
        }
        सचे (!process(nodep->rhsp())) पटिनिवत्त;
        // If a simple variable assignment, update the binding
        AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* ध्रुव valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        सचे (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            पटिनिवत्त;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    शून्य visit(AstIf* nodep) override {
        सचे (!m_ok) पटिनिवत्त;
        सचे (!process(nodep->condp())) पटिनिवत्त;
        // If condition is constant, replce with the relevant branch
        सचे (AstConst* ध्रुव condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            सचे (Ast_ग्रन्थिः* ध्रुव bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            पटिनिवत्त;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    शून्य visit(AstJumpGo* nodep) override {
        सचे (!m_ok) पटिनिवत्त;
        // Remove trailing dead code
        सचे (nodep->nextp()) प्रक्षिप_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    शून्य visit(AstLoop* nodep) override {
        सचे (!m_ok) पटिनिवत्त;
        m_bindings.checkpoint();
        std::pair<Ast_ग्रन्थिः*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        सचे (!pair.second) {
            m_bindings.revert();
            process(nodep);
            पटिनिवत्त;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        सचे (pair.first) {
            nodep->replaceWith(pair.first);
        } अञ्ञथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

सार्वजनिक:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    स्थिर std::pair<Ast_ग्रन्थिः*, तार्किक> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        सचे (visitor.m_ok) पटिनिवत्त {visitor.m_stmtsp, सच्च};
        // Otherwise delete intermediate results
        सचे (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_वृक्षः(), visitor.m_stmtsp);
        पटिनिवत्त {nullptr, मिच्छा};
    }
};

//######################################################################
// Unroll all AstLoop statements

वग्ग UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    स्थिर AstVarRef* unwrapToVarRef(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क minWidth) {
        याव (सच्च) {
            सचे (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) पटिनिवत्त refp;
            सचे (AstExtend* ध्रुव ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } अञ्ञथा सचे (AstExtendS* ध्रुव ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } अञ्ञथा सचे (AstSel* ध्रुव sp = VN_CAST(nodep, Sel)) {
                ध्रुव AstConst* ध्रुव lsbp = VN_CAST(sp->lsbp(), Const);
                सचे (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) पटिनिवत्त nullptr;
                nodep = sp->fromp();
            } अञ्ञथा {
                पटिनिवत्त nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    तार्किक isVarPlus1(Ast_ग्रन्थिः* nodep, ध्रुव AstVarScope* vscp) {
        AstAdd* ध्रुव addp = VN_CAST(nodep, Add);
        सचे (!addp || !addp->lhsp()->isOne()) पटिनिवत्त मिच्छा;
        ध्रुव AstVarRef* ध्रुव r = unwrapToVarRef(addp->rhsp(), addp->width());
        पटिनिवत्त r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* resizeToWidth(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव AstVarRef* targetRefp) {
        ध्रुव पूर्णाङ्क width = targetRefp->width();
        सचे (width == 32) पटिनिवत्त exprp;
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        सचे (width < 32) पटिनिवत्त नूतन AstSel{flp, exprp, 0, width};
        AstExtend* ध्रुव extp = नूतन AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        पटिनिवत्त extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    स्थिर तार्किक ascendingBound(Ast_ग्रन्थिः_व्यञ्जकम्* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        सचे (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) पटिनिवत्त मिच्छा;
        Ast_ग्रन्थिः_Biop* ध्रुव bp = VN_AS(condp, ग्रन्थिः_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        पटिनिवत्त wp && idxRefp && !wp->अङ्कः().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    स्थिर तार्किक isInRangeGuard(Ast_ग्रन्थिः_व्यञ्जकम्* condp, ध्रुव AstVarScope* idxVscp, uint32_t width,
                               पूर्णाङ्क addrBits) {
        सचे (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) पटिनिवत्त मिच्छा;
        Ast_ग्रन्थिः_Biop* ध्रुव bp = VN_AS(condp, ग्रन्थिः_Biop);
        ध्रुव AstConst* ध्रुव cp = VN_CAST(bp->lhsp(), Const);
        सचे (!cp || cp->अङ्कः().isFourState() || cp->toUInt() < width - 1) पटिनिवत्त मिच्छा;
        ध्रुव AstVarRef* ध्रुव r = unwrapToVarRef(bp->rhsp(), addrBits);
        पटिनिवत्त r && r->varScopep() == idxVscp;
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
        सचे (!testp) पटिनिवत्त मिच्छा;
        AstIf* ध्रुव ifp = VN_CAST(testp->nextp(), If);
        सचे (!ifp) पटिनिवत्त मिच्छा;
        AstAssign* ध्रुव incp = VN_CAST(ifp->nextp(), Assign);
        सचे (!incp || incp->nextp()) पटिनिवत्त मिच्छा;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        सचे (!ascendingBound(testp->condp(), wp, idxRefp)) पटिनिवत्त मिच्छा;
        AstVarScope* ध्रुव idxVscp = idxRefp->varScopep();
        ध्रुव uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ध्रुव पूर्णाङ्क addrBits = width <= 1 ? 1 : V3_सङ्ख्या::log2b(width - 1) + 1;
        ध्रुव AstConst* ध्रुव idxInitp = m_bindings.get(idxVscp);
        सचे (!idxInitp || !idxInitp->isZero()) पटिनिवत्त मिच्छा;
        AstVarRef* ध्रुव incLhsp = VN_CAST(incp->lhsp(), VarRef);
        सचे (!incLhsp || incLhsp->varScopep() != idxVscp) पटिनिवत्त मिच्छा;
        सचे (!isVarPlus1(incp->rhsp(), idxVscp)) पटिनिवत्त मिच्छा;
        सचे (ifp->elsesp()) पटिनिवत्त मिच्छा;
        AstAssign* ध्रुव thenp = VN_CAST(ifp->thensp(), Assign);
        सचे (!thenp || thenp->nextp()) पटिनिवत्त मिच्छा;
        AstVarRef* ध्रुव targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        सचे (!targetRefp) पटिनिवत्त मिच्छा;
        AstVarScope* ध्रुव targetVscp = targetRefp->varScopep();
        सचे (targetVscp == idxVscp) पटिनिवत्त मिच्छा;
        ध्रुव तार्किक isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ध्रुव तार्किक is_गणनम्_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        सचे (!isLeadingOne && !is_गणनम्_Ones) पटिनिवत्त मिच्छा;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_ग्रन्थिः_व्यञ्जकम्* condp = ifp->condp();
        सचे (AstLogAnd* ध्रुव andp = VN_CAST(condp, LogAnd)) {
            सचे (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } अञ्ञथा सचे (AstCond* ध्रुव ternp = VN_CAST(condp, Cond)) {
            सचे (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* ध्रुव selp = VN_CAST(condp, Sel);
        सचे (!selp || selp->width() != 1) पटिनिवत्त मिच्छा;
        ध्रुव AstVarRef* ध्रुव fromp = VN_CAST(selp->fromp(), VarRef);
        सचे (!fromp) पटिनिवत्त मिच्छा;
        ध्रुव AstVarScope* ध्रुव fromVscp = fromp->varScopep();
        सचे (fromVscp == idxVscp || fromVscp == targetVscp) पटिनिवत्त मिच्छा;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        सचे (static_cast<पूर्णाङ्क>(width) != vecExprp->width()) पटिनिवत्त मिच्छा;
        ध्रुव AstVarRef* ध्रुव idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        सचे (!idxInSel || idxInSel->varScopep() != idxVscp) पटिनिवत्त मिच्छा;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ध्रुव AstConst* ध्रुव targetInitp = m_bindings.get(targetVscp);
        सचे (!targetInitp || !targetInitp->isZero()) पटिनिवत्त मिच्छा;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_पङ्क्तिः* ध्रुव flp = loopp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* reducep;
        सचे (isLeadingOne) {
            reducep = नूतन AstMostSetBitP1{flp, vecExprp->clone_वृक्षः(मिच्छा)};
        } अञ्ञथा {
            Ast_गणनम्_Ones* ध्रुव conep = नूतन Ast_गणनम्_Ones{flp, vecExprp->clone_वृक्षः(मिच्छा)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* ध्रुव newp = नूतन AstAssign{flp, targetRefp->clone_वृक्षः(मिच्छा), reducep};
        newp->addNext(नूतन AstAssign{flp, incLhsp->clone_वृक्षः(मिच्छा), wp->clone_वृक्षः(मिच्छा)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(loopp), loopp);
        सचे (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } अञ्ञथा {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_गणनम्_Ones_Lowered;
        }
        पटिनिवत्त सच्च;
    }

    // VISIT
    शून्य visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        अत्थाय (Ast_ग्रन्थिः *succp = nodep, *currp = nodep->backp(); सच्च;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            सचे (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                सचे (VN_IS(currp, JumpBlock)) अनुवर्त;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                भङ्ग;
            }
            AstAssign* ध्रुव assignp = VN_CAST(currp, Assign);
            सचे (!assignp) भङ्ग;
            AstConst* ध्रुव valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            सचे (!valp) भङ्ग;
            AstVarRef* ध्रुव lhsp = VN_CAST(assignp->lhsp(), VarRef);
            सचे (!lhsp) भङ्ग;
            // Don't bind if volatile
            सचे (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) अनुवर्त;
            // Don't overwrite a later binding
            सचे (m_bindings.get(lhsp->varScopep())) अनुवर्त;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        सचे (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) पटिनिवत्त;

        // Attempt to unroll this loop
        ध्रुव std::pair<Ast_ग्रन्थिः*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        सचे (!pair.second) {
            iterateChildren(nodep);
            पटिनिवत्त;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        सचे (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अञ्ञथा {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_वृक्षः("unroll", 0, dump_वृक्षः_Either_Level() >= 3);
}
