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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Unroll.h"

#समावेश "V3Const.h"
#समावेश "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

काष्ठा UnrollStats final {
    श्रेणी Stat final {
        माप_प्रकार m_मूल्य = 0;  // Statistics value
        स्थिर अक्षर* स्थिर m_name;  // Name for stats file and UDEBUG

    खुला:
        explicit Stat(स्थिर अक्षर* स्थिर name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_मूल्य); }
        स्थिर अक्षर* name() स्थिर { वापस m_name; }
        Stat& चालक++() {
            ++m_मूल्य;
            वापस *यह;
        }
        Stat& चालक+=(माप_प्रकार v) {
            m_मूल्य += v;
            वापस *यह;
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
श्रेणी UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    स्थिर VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

खुला:
    // CONSTRUCTOR
    UnrolllBindings() = शेष;
    ~UnrolllBindings() = शेष;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    व्योम सफा() {
        m_curr.resize(1);
        m_checkpoints.सफा();
        Ast_गाँठ::user1_Clear_वृक्ष();
    }

    व्योम checkpoint() { m_checkpoints.धकेलो_back(m_curr); }

    व्योम revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.निकालो_back();
    }

    व्योम commit() { m_checkpoints.निकालो_back(); }

    व्योम set(AstVarScope* vscp, AstConst* valp) {
        अगर (!vscp->user1()) {
            vscp->user1(m_curr.आकार());
            m_curr.धकेलो_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* लो(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        अगर (static_cast<माप_प्रकार>(vscp->user1()) >= m_curr.आकार()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* स्थिर valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        वापस valp;
    }
};

//######################################################################
// Unroll one AstLoop

श्रेणी UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* स्थिर m_loopp;  // The loop we are trying to unroll
    Ast_गाँठ* m_stmtsp = nullptr;  // Resulting unrolled statement
    माप_प्रकार m_unrolled_आकार = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* स्थिर m_wrapp = नया AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, false};
    स्थिर bool m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    bool m_ok = true;  // Unrolling successful so far, gave up if false

    // METHODS
    व्योम cantUnroll(Ast_गाँठ* nodep, UnrollStats::Stat& stat) {
        m_ok = false;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    bool unrollOneIteration(Ast_गाँठ* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        bool foundLoopTest = false;
        // Process one body statement at a time
        क्रम (Ast_गाँठ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            अगर (AstLoopTest* स्थिर testp = VN_CAST(stmtp, LoopTest)) {
                Ast_गाँठ* स्थिर condp = V3Const::constifyEdit(testp->condp());
                अगर (condp->isZero()) {
                    // Loop terminates
                    वापस false;
                } अन्यथा अगर (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    जारी;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_वृक्ष(false));
            iterate_And_अग्र_Null(m_wrapp->stmtsp());
            // Give up if failed
            अगर (!m_ok) वापस false;

            // Add statements to unrolled body
            जबतक (Ast_गाँठ* स्थिर nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                अगर (!m_unrollFull) {
                    m_unrolled_आकार += nodep->गाँठ_गिनती();
                    अगर (m_unrolled_आकार > static_cast<माप_प्रकार>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        वापस false;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                अगर (AstLoopTest* स्थिर testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = true;
                    // Will not actually need it, nor any subsequent
                    धकेलो_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    अगर (testp->condp()->isNeqZero()) जारी;
                    // Won't need any of the trailing statements
                    अगर (m_wrapp->stmtsp()) धकेलो_Deletep(m_wrapp->stmtsp()->unlink_Fr_Back_With_अग्र());
                    // Loop terminates
                    अगर (testp->condp()->isZero()) वापस false;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    वापस false;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_गाँठ::add_अग्र(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                अगर (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    वापस false;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        अगर (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            वापस false;
        }
        // One iteration done, loop continues
        वापस true;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    bool process(Ast_गाँठ* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        अगर (!nodep) वापस true;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_गाँठ* np) {
            // Failed earlier
            अगर (!m_ok) वापस;
            // Check for AstLoopTest
            अगर (AstLoopTest* स्थिर testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                अगर (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                वापस;
            }
            // Check for AstFork - can't unroll
            अगर (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                वापस;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            अगर (Ast_गाँठ_CCall* स्थिर callp = VN_CAST(np, गाँठ_CCall)) {
                अगर (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    वापस;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            अगर (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                वापस;
            }
            // Process variable references
            AstVarRef* स्थिर refp = VN_CAST(np, VarRef);
            अगर (!refp) वापस;
            // Ignore if the referenced variable has no binding
            AstConst* स्थिर valp = m_bindings.लो(refp->varScopep());
            अगर (!valp) वापस;
            // If writen, remove the binding
            अगर (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                वापस;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.धकेलो_back(refp);
        });
        // Give up if we have to
        अगर (!m_ok) वापस false;
        // Actually substitute the variables that still have bindings
        क्रम (AstVarRef* स्थिर refp : toSubstitute) {
            // Pick up bound value
            AstConst* स्थिर valp = m_bindings.लो(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            अगर (!valp) जारी;
            // Substitute it
            refp->replaceWith(valp->clone_वृक्ष(false));
            VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
        }
        वापस true;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        अगर (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            वापस;
        }
        // Attempt to unroll the loop
        स्थिर माप_प्रकार iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_गिनती();
        माप_प्रकार iter_गिनती = 0;
        करो {
            अगर (iter_गिनती > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_गिनती);
                अगर (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                वापस;
            }
            ++iter_गिनती;
        } जबतक (unrollOneIteration(loopp->stmtsp()));

        अगर (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_गिनती;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_वृक्ष(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (!m_ok) वापस;
        // Generic body statement, just substitute
        process(nodep);
    }
    व्योम visit(AstLoopTest* nodep) override {
        अगर (!m_ok) वापस;
        // If the condition is a ExprStmt, move it before the LoopTest
        अगर (Ast_व्यंजक_Stmt* स्थिर exprp = VN_CAST(nodep->condp(), व्यंजक_Stmt)) {
            Ast_गाँठ* स्थिर stmtsp = exprp->stmtsp()->unlink_Fr_Back_With_अग्र();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(धकेलो_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->add_अग्र(nodep);
            relinker.relink(stmtsp);
            वापस;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    व्योम visit(AstAssign* nodep) override {
        अगर (!m_ok) वापस;
        // Can't do it if delayed
        अगर (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            वापस;
        }
        अगर (!process(nodep->rhsp())) वापस;
        // If a simple variable assignment, update the binding
        AstVarRef* स्थिर lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* स्थिर valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        अगर (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            वापस;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    व्योम visit(AstIf* nodep) override {
        अगर (!m_ok) वापस;
        अगर (!process(nodep->condp())) वापस;
        // If condition is constant, replce with the relevant branch
        अगर (AstConst* स्थिर condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            अगर (Ast_गाँठ* स्थिर bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->add_अग्र_Here(bodyp->unlink_Fr_Back_With_अग्र());  // This will be iterated next
            }
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    व्योम visit(AstJumpGo* nodep) override {
        अगर (!m_ok) वापस;
        // Remove trailing dead code
        अगर (nodep->nextp()) धकेलो_Deletep(nodep->nextp()->unlink_Fr_Back_With_अग्र());
    }
    व्योम visit(AstLoop* nodep) override {
        अगर (!m_ok) वापस;
        m_bindings.checkpoint();
        std::pair<Ast_गाँठ*, bool> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        अगर (!pair.second) {
            m_bindings.revert();
            process(nodep);
            वापस;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        अगर (pair.first) {
            nodep->replaceWith(pair.first);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

खुला:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    अटल std::pair<Ast_गाँठ*, bool> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        अगर (visitor.m_ok) वापस {visitor.m_stmtsp, true};
        // Otherwise delete intermediate results
        अगर (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_वृक्ष(), visitor.m_stmtsp);
        वापस {nullptr, false};
    }
};

//######################################################################
// Unroll all AstLoop statements

श्रेणी UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    अटल AstVarRef* unwrapToVarRef(Ast_गाँठ_व्यंजक* nodep, पूर्णांक minWidth) {
        जबतक (true) {
            अगर (AstVarRef* स्थिर refp = VN_CAST(nodep, VarRef)) वापस refp;
            अगर (AstExtend* स्थिर ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } अन्यथा अगर (AstExtendS* स्थिर ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } अन्यथा अगर (AstSel* स्थिर sp = VN_CAST(nodep, Sel)) {
                स्थिर AstConst* स्थिर lsbp = VN_CAST(sp->lsbp(), Const);
                अगर (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) वापस nullptr;
                nodep = sp->fromp();
            } अन्यथा {
                वापस nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    bool isVarPlus1(Ast_गाँठ* nodep, स्थिर AstVarScope* vscp) {
        AstAdd* स्थिर addp = VN_CAST(nodep, Add);
        अगर (!addp || !addp->lhsp()->isOne()) वापस false;
        स्थिर AstVarRef* स्थिर r = unwrapToVarRef(addp->rhsp(), addp->width());
        वापस r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    अटल Ast_गाँठ_व्यंजक* resizeToWidth(Ast_गाँठ_व्यंजक* exprp, स्थिर AstVarRef* targetRefp) {
        स्थिर पूर्णांक width = targetRefp->width();
        अगर (width == 32) वापस exprp;
        File_पंक्ति* स्थिर flp = exprp->fileline();
        अगर (width < 32) वापस नया AstSel{flp, exprp, 0, width};
        AstExtend* स्थिर extp = नया AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        वापस extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    अटल bool ascendingBound(Ast_गाँठ_व्यंजक* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        अगर (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) वापस false;
        Ast_गाँठ_Biop* स्थिर bp = VN_AS(condp, गाँठ_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        वापस wp && idxRefp && !wp->अंक().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    अटल bool isInRangeGuard(Ast_गाँठ_व्यंजक* condp, स्थिर AstVarScope* idxVscp, uint32_t width,
                               पूर्णांक addrBits) {
        अगर (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) वापस false;
        Ast_गाँठ_Biop* स्थिर bp = VN_AS(condp, गाँठ_Biop);
        स्थिर AstConst* स्थिर cp = VN_CAST(bp->lhsp(), Const);
        अगर (!cp || cp->अंक().isFourState() || cp->toUInt() < width - 1) वापस false;
        स्थिर AstVarRef* स्थिर r = unwrapToVarRef(bp->rhsp(), addrBits);
        वापस r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    bool tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* स्थिर testp = VN_CAST(loopp->stmtsp(), LoopTest);
        अगर (!testp) वापस false;
        AstIf* स्थिर ifp = VN_CAST(testp->nextp(), If);
        अगर (!ifp) वापस false;
        AstAssign* स्थिर incp = VN_CAST(ifp->nextp(), Assign);
        अगर (!incp || incp->nextp()) वापस false;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        अगर (!ascendingBound(testp->condp(), wp, idxRefp)) वापस false;
        AstVarScope* स्थिर idxVscp = idxRefp->varScopep();
        स्थिर uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        स्थिर पूर्णांक addrBits = width <= 1 ? 1 : V3_संख्या::log2b(width - 1) + 1;
        स्थिर AstConst* स्थिर idxInitp = m_bindings.लो(idxVscp);
        अगर (!idxInitp || !idxInitp->isZero()) वापस false;
        AstVarRef* स्थिर incLhsp = VN_CAST(incp->lhsp(), VarRef);
        अगर (!incLhsp || incLhsp->varScopep() != idxVscp) वापस false;
        अगर (!isVarPlus1(incp->rhsp(), idxVscp)) वापस false;
        अगर (ifp->elsesp()) वापस false;
        AstAssign* स्थिर thenp = VN_CAST(ifp->thensp(), Assign);
        अगर (!thenp || thenp->nextp()) वापस false;
        AstVarRef* स्थिर targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        अगर (!targetRefp) वापस false;
        AstVarScope* स्थिर targetVscp = targetRefp->varScopep();
        अगर (targetVscp == idxVscp) वापस false;
        स्थिर bool isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        स्थिर bool is_गिनती_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        अगर (!isLeadingOne && !is_गिनती_Ones) वापस false;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_गाँठ_व्यंजक* condp = ifp->condp();
        अगर (AstLogAnd* स्थिर andp = VN_CAST(condp, LogAnd)) {
            अगर (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } अन्यथा अगर (AstCond* स्थिर ternp = VN_CAST(condp, Cond)) {
            अगर (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* स्थिर selp = VN_CAST(condp, Sel);
        अगर (!selp || selp->width() != 1) वापस false;
        स्थिर AstVarRef* स्थिर fromp = VN_CAST(selp->fromp(), VarRef);
        अगर (!fromp) वापस false;
        स्थिर AstVarScope* स्थिर fromVscp = fromp->varScopep();
        अगर (fromVscp == idxVscp || fromVscp == targetVscp) वापस false;
        Ast_गाँठ_व्यंजक* स्थिर vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        अगर (static_cast<पूर्णांक>(width) != vecExprp->width()) वापस false;
        स्थिर AstVarRef* स्थिर idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        अगर (!idxInSel || idxInSel->varScopep() != idxVscp) वापस false;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        स्थिर AstConst* स्थिर targetInitp = m_bindings.लो(targetVscp);
        अगर (!targetInitp || !targetInitp->isZero()) वापस false;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_पंक्ति* स्थिर flp = loopp->fileline();
        Ast_गाँठ_व्यंजक* reducep;
        अगर (isLeadingOne) {
            reducep = नया AstMostSetBitP1{flp, vecExprp->clone_वृक्ष(false)};
        } अन्यथा {
            Ast_गिनती_Ones* स्थिर conep = नया Ast_गिनती_Ones{flp, vecExprp->clone_वृक्ष(false)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* स्थिर newp = नया AstAssign{flp, targetRefp->clone_वृक्ष(false), reducep};
        newp->add_अग्र(नया AstAssign{flp, incLhsp->clone_वृक्ष(false), wp->clone_वृक्ष(false)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(loopp), loopp);
        अगर (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } अन्यथा {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_गिनती_Ones_Lowered;
        }
        वापस true;
    }

    // VISIT
    व्योम visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.सफा();
        क्रम (Ast_गाँठ *succp = nodep, *currp = nodep->backp(); true;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            अगर (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                अगर (VN_IS(currp, JumpBlock)) जारी;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                अवरोध;
            }
            AstAssign* स्थिर assignp = VN_CAST(currp, Assign);
            अगर (!assignp) अवरोध;
            AstConst* स्थिर valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            अगर (!valp) अवरोध;
            AstVarRef* स्थिर lhsp = VN_CAST(assignp->lhsp(), VarRef);
            अगर (!lhsp) अवरोध;
            // Don't bind if volatile
            अगर (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) जारी;
            // Don't overwrite a later binding
            अगर (m_bindings.लो(lhsp->varScopep())) जारी;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        अगर (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) वापस;

        // Attempt to unroll this loop
        स्थिर std::pair<Ast_गाँठ*, bool> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        अगर (!pair.second) {
            iterateChildren(nodep);
            वापस;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        अगर (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा {
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

खुला:
    अटल व्योम apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

व्योम V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_वृक्ष("unroll", 0, dump_वृक्ष_Either_Level() >= 3);
}
