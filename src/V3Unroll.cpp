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
        माप_प्रकार m_मान = 0;  // Statistics value
        अचर वर्ण* अचर m_name;  // Name for stats file and UDEBUG

    सार्वजनिक:
        explicit Stat(अचर वर्ण* अचर name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_मान); }
        अचर वर्ण* name() अचर { फर्काऊ m_name; }
        Stat& चालक++() {
            ++m_मान;
            फर्काऊ *एतत्;
        }
        Stat& चालक+=(माप_प्रकार v) {
            m_मान += v;
            फर्काऊ *एतत्;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_गन्ती{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_गन्ती_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
वर्ग UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    अचर VNUser1InUse m_inuser1;

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
        Ast_गाँठो::user1_Clear_रूख();
    }

    शून्य checkpoint() { m_checkpoints.थप_back(m_curr); }

    शून्य revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.झिक_back();
    }

    शून्य commit() { m_checkpoints.झिक_back(); }

    शून्य set(AstVarScope* vscp, AstConst* valp) {
        यदि (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.थप_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        यदि (static_cast<माप_प्रकार>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* अचर valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        फर्काऊ valp;
    }
};

//######################################################################
// Unroll one AstLoop

वर्ग UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* अचर m_loopp;  // The loop we are trying to unroll
    Ast_गाँठो* m_stmtsp = nullptr;  // Resulting unrolled statement
    माप_प्रकार m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* अचर m_wrapp = नूतन AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, असत्य};
    अचर तार्किक m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    तार्किक m_ok = सत्य;  // Unrolling successful so far, gave up if false

    // METHODS
    शून्य cantUnroll(Ast_गाँठो* nodep, UnrollStats::Stat& stat) {
        m_ok = असत्य;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    तार्किक unrollOneIteration(Ast_गाँठो* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        तार्किक foundLoopTest = असत्य;
        // Process one body statement at a time
        लागि (Ast_गाँठो* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            यदि (AstLoopTest* अचर testp = VN_CAST(stmtp, LoopTest)) {
                Ast_गाँठो* अचर condp = V3Const::constifyEdit(testp->condp());
                यदि (condp->isZero()) {
                    // Loop terminates
                    फर्काऊ असत्य;
                } अन्यथा यदि (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    जारी;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_रूख(असत्य));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            यदि (!m_ok) फर्काऊ असत्य;

            // Add statements to unrolled body
            जबसम्म (Ast_गाँठो* अचर nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                यदि (!m_unrollFull) {
                    m_unrolledSize += nodep->गाँठो_गन्ती();
                    यदि (m_unrolledSize > static_cast<माप_प्रकार>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        फर्काऊ असत्य;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                यदि (AstLoopTest* अचर testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = सत्य;
                    // Will not actually need it, nor any subsequent
                    थप_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    यदि (testp->condp()->isNeqZero()) जारी;
                    // Won't need any of the trailing statements
                    यदि (m_wrapp->stmtsp()) थप_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    यदि (testp->condp()->isZero()) फर्काऊ असत्य;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    फर्काऊ असत्य;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_गाँठो::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                यदि (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    फर्काऊ असत्य;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        यदि (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            फर्काऊ असत्य;
        }
        // One iteration done, loop continues
        फर्काऊ सत्य;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    तार्किक process(Ast_गाँठो* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        यदि (!nodep) फर्काऊ सत्य;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_गाँठो* np) {
            // Failed earlier
            यदि (!m_ok) फर्काऊ;
            // Check for AstLoopTest
            यदि (AstLoopTest* अचर testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                यदि (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                फर्काऊ;
            }
            // Check for AstFork - can't unroll
            यदि (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                फर्काऊ;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            यदि (Ast_गाँठो_CCall* अचर callp = VN_CAST(np, गाँठो_CCall)) {
                यदि (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    फर्काऊ;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            यदि (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                फर्काऊ;
            }
            // Process variable references
            AstVarRef* अचर refp = VN_CAST(np, VarRef);
            यदि (!refp) फर्काऊ;
            // Ignore if the referenced variable has no binding
            AstConst* अचर valp = m_bindings.get(refp->varScopep());
            यदि (!valp) फर्काऊ;
            // If writen, remove the binding
            यदि (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                फर्काऊ;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.थप_back(refp);
        });
        // Give up if we have to
        यदि (!m_ok) फर्काऊ असत्य;
        // Actually substitute the variables that still have bindings
        लागि (AstVarRef* अचर refp : toSubstitute) {
            // Pick up bound value
            AstConst* अचर valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            यदि (!valp) जारी;
            // Substitute it
            refp->replaceWith(valp->clone_रूख(असत्य));
            VL_DO_DANGLING(थप_Deletep(refp), refp);
        }
        फर्काऊ सत्य;
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
            फर्काऊ;
        }
        // Attempt to unroll the loop
        अचर माप_प्रकार iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_गन्ती();
        माप_प्रकार iter_गन्ती = 0;
        गर {
            यदि (iter_गन्ती > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_गन्ती);
                यदि (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                फर्काऊ;
            }
            ++iter_गन्ती;
        } जबसम्म (unrollOneIteration(loopp->stmtsp()));

        यदि (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_गन्ती;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_रूख(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (!m_ok) फर्काऊ;
        // Generic body statement, just substitute
        process(nodep);
    }
    शून्य visit(AstLoopTest* nodep) override {
        यदि (!m_ok) फर्काऊ;
        // If the condition is a ExprStmt, move it before the LoopTest
        यदि (Ast_अभिव्यक्ति_Stmt* अचर exprp = VN_CAST(nodep->condp(), अभिव्यक्ति_Stmt)) {
            Ast_गाँठो* अचर stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(थप_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            फर्काऊ;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    शून्य visit(AstAssign* nodep) override {
        यदि (!m_ok) फर्काऊ;
        // Can't do it if delayed
        यदि (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            फर्काऊ;
        }
        यदि (!process(nodep->rhsp())) फर्काऊ;
        // If a simple variable assignment, update the binding
        AstVarRef* अचर lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* अचर valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        यदि (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            फर्काऊ;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    शून्य visit(AstIf* nodep) override {
        यदि (!m_ok) फर्काऊ;
        यदि (!process(nodep->condp())) फर्काऊ;
        // If condition is constant, replce with the relevant branch
        यदि (AstConst* अचर condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            यदि (Ast_गाँठो* अचर bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
            फर्काऊ;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    शून्य visit(AstJumpGo* nodep) override {
        यदि (!m_ok) फर्काऊ;
        // Remove trailing dead code
        यदि (nodep->nextp()) थप_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    शून्य visit(AstLoop* nodep) override {
        यदि (!m_ok) फर्काऊ;
        m_bindings.checkpoint();
        std::pair<Ast_गाँठो*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        यदि (!pair.second) {
            m_bindings.revert();
            process(nodep);
            फर्काऊ;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        यदि (pair.first) {
            nodep->replaceWith(pair.first);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

सार्वजनिक:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    स्थिर std::pair<Ast_गाँठो*, तार्किक> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        यदि (visitor.m_ok) फर्काऊ {visitor.m_stmtsp, सत्य};
        // Otherwise delete intermediate results
        यदि (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_रूख(), visitor.m_stmtsp);
        फर्काऊ {nullptr, असत्य};
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
    स्थिर AstVarRef* unwrapToVarRef(Ast_गाँठो_अभिव्यक्ति* nodep, पूर्णाङ्क minWidth) {
        जबसम्म (सत्य) {
            यदि (AstVarRef* अचर refp = VN_CAST(nodep, VarRef)) फर्काऊ refp;
            यदि (AstExtend* अचर ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } अन्यथा यदि (AstExtendS* अचर ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } अन्यथा यदि (AstSel* अचर sp = VN_CAST(nodep, Sel)) {
                अचर AstConst* अचर lsbp = VN_CAST(sp->lsbp(), Const);
                यदि (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) फर्काऊ nullptr;
                nodep = sp->fromp();
            } अन्यथा {
                फर्काऊ nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    तार्किक isVarPlus1(Ast_गाँठो* nodep, अचर AstVarScope* vscp) {
        AstAdd* अचर addp = VN_CAST(nodep, Add);
        यदि (!addp || !addp->lhsp()->isOne()) फर्काऊ असत्य;
        अचर AstVarRef* अचर r = unwrapToVarRef(addp->rhsp(), addp->width());
        फर्काऊ r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    स्थिर Ast_गाँठो_अभिव्यक्ति* resizeToWidth(Ast_गाँठो_अभिव्यक्ति* exprp, अचर AstVarRef* targetRefp) {
        अचर पूर्णाङ्क width = targetRefp->width();
        यदि (width == 32) फर्काऊ exprp;
        File_पङ्क्ति* अचर flp = exprp->fileline();
        यदि (width < 32) फर्काऊ नूतन AstSel{flp, exprp, 0, width};
        AstExtend* अचर extp = नूतन AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        फर्काऊ extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    स्थिर तार्किक ascendingBound(Ast_गाँठो_अभिव्यक्ति* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        यदि (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) फर्काऊ असत्य;
        Ast_गाँठो_Biop* अचर bp = VN_AS(condp, गाँठो_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        फर्काऊ wp && idxRefp && !wp->अङ्क().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    स्थिर तार्किक isInRangeGuard(Ast_गाँठो_अभिव्यक्ति* condp, अचर AstVarScope* idxVscp, uint32_t width,
                               पूर्णाङ्क addrBits) {
        यदि (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) फर्काऊ असत्य;
        Ast_गाँठो_Biop* अचर bp = VN_AS(condp, गाँठो_Biop);
        अचर AstConst* अचर cp = VN_CAST(bp->lhsp(), Const);
        यदि (!cp || cp->अङ्क().isFourState() || cp->toUInt() < width - 1) फर्काऊ असत्य;
        अचर AstVarRef* अचर r = unwrapToVarRef(bp->rhsp(), addrBits);
        फर्काऊ r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    तार्किक tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* अचर testp = VN_CAST(loopp->stmtsp(), LoopTest);
        यदि (!testp) फर्काऊ असत्य;
        AstIf* अचर ifp = VN_CAST(testp->nextp(), If);
        यदि (!ifp) फर्काऊ असत्य;
        AstAssign* अचर incp = VN_CAST(ifp->nextp(), Assign);
        यदि (!incp || incp->nextp()) फर्काऊ असत्य;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        यदि (!ascendingBound(testp->condp(), wp, idxRefp)) फर्काऊ असत्य;
        AstVarScope* अचर idxVscp = idxRefp->varScopep();
        अचर uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        अचर पूर्णाङ्क addrBits = width <= 1 ? 1 : V3_सङ्ख्या::log2b(width - 1) + 1;
        अचर AstConst* अचर idxInitp = m_bindings.get(idxVscp);
        यदि (!idxInitp || !idxInitp->isZero()) फर्काऊ असत्य;
        AstVarRef* अचर incLhsp = VN_CAST(incp->lhsp(), VarRef);
        यदि (!incLhsp || incLhsp->varScopep() != idxVscp) फर्काऊ असत्य;
        यदि (!isVarPlus1(incp->rhsp(), idxVscp)) फर्काऊ असत्य;
        यदि (ifp->elsesp()) फर्काऊ असत्य;
        AstAssign* अचर thenp = VN_CAST(ifp->thensp(), Assign);
        यदि (!thenp || thenp->nextp()) फर्काऊ असत्य;
        AstVarRef* अचर targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        यदि (!targetRefp) फर्काऊ असत्य;
        AstVarScope* अचर targetVscp = targetRefp->varScopep();
        यदि (targetVscp == idxVscp) फर्काऊ असत्य;
        अचर तार्किक isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        अचर तार्किक is_गन्ती_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        यदि (!isLeadingOne && !is_गन्ती_Ones) फर्काऊ असत्य;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_गाँठो_अभिव्यक्ति* condp = ifp->condp();
        यदि (AstLogAnd* अचर andp = VN_CAST(condp, LogAnd)) {
            यदि (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } अन्यथा यदि (AstCond* अचर ternp = VN_CAST(condp, Cond)) {
            यदि (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* अचर selp = VN_CAST(condp, Sel);
        यदि (!selp || selp->width() != 1) फर्काऊ असत्य;
        अचर AstVarRef* अचर fromp = VN_CAST(selp->fromp(), VarRef);
        यदि (!fromp) फर्काऊ असत्य;
        अचर AstVarScope* अचर fromVscp = fromp->varScopep();
        यदि (fromVscp == idxVscp || fromVscp == targetVscp) फर्काऊ असत्य;
        Ast_गाँठो_अभिव्यक्ति* अचर vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        यदि (static_cast<पूर्णाङ्क>(width) != vecExprp->width()) फर्काऊ असत्य;
        अचर AstVarRef* अचर idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        यदि (!idxInSel || idxInSel->varScopep() != idxVscp) फर्काऊ असत्य;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        अचर AstConst* अचर targetInitp = m_bindings.get(targetVscp);
        यदि (!targetInitp || !targetInitp->isZero()) फर्काऊ असत्य;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_पङ्क्ति* अचर flp = loopp->fileline();
        Ast_गाँठो_अभिव्यक्ति* reducep;
        यदि (isLeadingOne) {
            reducep = नूतन AstMostSetBitP1{flp, vecExprp->clone_रूख(असत्य)};
        } अन्यथा {
            Ast_गन्ती_Ones* अचर conep = नूतन Ast_गन्ती_Ones{flp, vecExprp->clone_रूख(असत्य)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* अचर newp = नूतन AstAssign{flp, targetRefp->clone_रूख(असत्य), reducep};
        newp->addNext(नूतन AstAssign{flp, incLhsp->clone_रूख(असत्य), wp->clone_रूख(असत्य)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(थप_Deletep(loopp), loopp);
        यदि (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } अन्यथा {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_गन्ती_Ones_Lowered;
        }
        फर्काऊ सत्य;
    }

    // VISIT
    शून्य visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        लागि (Ast_गाँठो *succp = nodep, *currp = nodep->backp(); सत्य;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            यदि (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                यदि (VN_IS(currp, JumpBlock)) जारी;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                रोक;
            }
            AstAssign* अचर assignp = VN_CAST(currp, Assign);
            यदि (!assignp) रोक;
            AstConst* अचर valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            यदि (!valp) रोक;
            AstVarRef* अचर lhsp = VN_CAST(assignp->lhsp(), VarRef);
            यदि (!lhsp) रोक;
            // Don't bind if volatile
            यदि (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) जारी;
            // Don't overwrite a later binding
            यदि (m_bindings.get(lhsp->varScopep())) जारी;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        यदि (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) फर्काऊ;

        // Attempt to unroll this loop
        अचर std::pair<Ast_गाँठो*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        यदि (!pair.second) {
            iterateChildren(nodep);
            फर्काऊ;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        यदि (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        } अन्यथा {
            VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_रूख("unroll", 0, dump_रूख_Either_Level() >= 3);
}
