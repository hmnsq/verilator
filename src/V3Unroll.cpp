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
        स्थिरांक अक्षर* स्थिरांक m_name;  // Name for stats file and UDEBUG

    सार्वजनिक:
        explicit Stat(स्थिरांक अक्षर* स्थिरांक name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_मूल्य); }
        स्थिरांक अक्षर* name() स्थिरांक { परत m_name; }
        Stat& चालक++() {
            ++m_मूल्य;
            परत *हे;
        }
        Stat& चालक+=(माप_प्रकार v) {
            m_मूल्य += v;
            परत *हे;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_मोजणी{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_मोजणी_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
वर्ग UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    स्थिरांक VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

सार्वजनिक:
    // CONSTRUCTOR
    UnrolllBindings() = पूर्वनिर्धारित;
    ~UnrolllBindings() = पूर्वनिर्धारित;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    शून्य clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_गाठ::user1_Clear_वृक्ष();
    }

    शून्य checkpoint() { m_checkpoints.ढकला_back(m_curr); }

    शून्य revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.काढा_back();
    }

    शून्य commit() { m_checkpoints.काढा_back(); }

    शून्य set(AstVarScope* vscp, AstConst* valp) {
        जर (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.ढकला_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        जर (static_cast<माप_प्रकार>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* स्थिरांक valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        परत valp;
    }
};

//######################################################################
// Unroll one AstLoop

वर्ग UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* स्थिरांक m_loopp;  // The loop we are trying to unroll
    Ast_गाठ* m_stmtsp = nullptr;  // Resulting unrolled statement
    माप_प्रकार m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* स्थिरांक m_wrapp = नवीन AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, असत्य};
    स्थिरांक तार्किक m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    तार्किक m_ok = सत्य;  // Unrolling successful so far, gave up if false

    // METHODS
    शून्य cantUnroll(Ast_गाठ* nodep, UnrollStats::Stat& stat) {
        m_ok = असत्य;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    तार्किक unrollOneIteration(Ast_गाठ* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        तार्किक foundLoopTest = असत्य;
        // Process one body statement at a time
        साठी (Ast_गाठ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            जर (AstLoopTest* स्थिरांक testp = VN_CAST(stmtp, LoopTest)) {
                Ast_गाठ* स्थिरांक condp = V3Const::constifyEdit(testp->condp());
                जर (condp->isZero()) {
                    // Loop terminates
                    परत असत्य;
                } अन्यथा जर (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    सुरूठेवा;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_वृक्ष(असत्य));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            जर (!m_ok) परत असत्य;

            // Add statements to unrolled body
            पर्यंत (Ast_गाठ* स्थिरांक nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                जर (!m_unrollFull) {
                    m_unrolledSize += nodep->गाठ_मोजणी();
                    जर (m_unrolledSize > static_cast<माप_प्रकार>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        परत असत्य;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                जर (AstLoopTest* स्थिरांक testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = सत्य;
                    // Will not actually need it, nor any subsequent
                    ढकला_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    जर (testp->condp()->isNeqZero()) सुरूठेवा;
                    // Won't need any of the trailing statements
                    जर (m_wrapp->stmtsp()) ढकला_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    जर (testp->condp()->isZero()) परत असत्य;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    परत असत्य;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_गाठ::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                जर (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    परत असत्य;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        जर (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            परत असत्य;
        }
        // One iteration done, loop continues
        परत सत्य;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    तार्किक process(Ast_गाठ* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        जर (!nodep) परत सत्य;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_गाठ* np) {
            // Failed earlier
            जर (!m_ok) परत;
            // Check for AstLoopTest
            जर (AstLoopTest* स्थिरांक testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                जर (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                परत;
            }
            // Check for AstFork - can't unroll
            जर (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                परत;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            जर (Ast_गाठ_CCall* स्थिरांक callp = VN_CAST(np, गाठ_CCall)) {
                जर (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    परत;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            जर (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                परत;
            }
            // Process variable references
            AstVarRef* स्थिरांक refp = VN_CAST(np, VarRef);
            जर (!refp) परत;
            // Ignore if the referenced variable has no binding
            AstConst* स्थिरांक valp = m_bindings.get(refp->varScopep());
            जर (!valp) परत;
            // If writen, remove the binding
            जर (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                परत;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.ढकला_back(refp);
        });
        // Give up if we have to
        जर (!m_ok) परत असत्य;
        // Actually substitute the variables that still have bindings
        साठी (AstVarRef* स्थिरांक refp : toSubstitute) {
            // Pick up bound value
            AstConst* स्थिरांक valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            जर (!valp) सुरूठेवा;
            // Substitute it
            refp->replaceWith(valp->clone_वृक्ष(असत्य));
            VL_DO_DANGLING(ढकला_Deletep(refp), refp);
        }
        परत सत्य;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        जर (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            परत;
        }
        // Attempt to unroll the loop
        स्थिरांक माप_प्रकार iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_मोजणी();
        माप_प्रकार iter_मोजणी = 0;
        करा {
            जर (iter_मोजणी > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_मोजणी);
                जर (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                परत;
            }
            ++iter_मोजणी;
        } पर्यंत (unrollOneIteration(loopp->stmtsp()));

        जर (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_मोजणी;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_वृक्ष(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    शून्य visit(Ast_गाठ* nodep) override {
        जर (!m_ok) परत;
        // Generic body statement, just substitute
        process(nodep);
    }
    शून्य visit(AstLoopTest* nodep) override {
        जर (!m_ok) परत;
        // If the condition is a ExprStmt, move it before the LoopTest
        जर (Ast_अभिव्यक्ती_Stmt* स्थिरांक exprp = VN_CAST(nodep->condp(), अभिव्यक्ती_Stmt)) {
            Ast_गाठ* स्थिरांक stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            परत;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    शून्य visit(AstAssign* nodep) override {
        जर (!m_ok) परत;
        // Can't do it if delayed
        जर (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            परत;
        }
        जर (!process(nodep->rhsp())) परत;
        // If a simple variable assignment, update the binding
        AstVarRef* स्थिरांक lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* स्थिरांक valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        जर (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            परत;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    शून्य visit(AstIf* nodep) override {
        जर (!m_ok) परत;
        जर (!process(nodep->condp())) परत;
        // If condition is constant, replce with the relevant branch
        जर (AstConst* स्थिरांक condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            जर (Ast_गाठ* स्थिरांक bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    शून्य visit(AstJumpGo* nodep) override {
        जर (!m_ok) परत;
        // Remove trailing dead code
        जर (nodep->nextp()) ढकला_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    शून्य visit(AstLoop* nodep) override {
        जर (!m_ok) परत;
        m_bindings.checkpoint();
        std::pair<Ast_गाठ*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        जर (!pair.second) {
            m_bindings.revert();
            process(nodep);
            परत;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        जर (pair.first) {
            nodep->replaceWith(pair.first);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

सार्वजनिक:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    स्थिर std::pair<Ast_गाठ*, तार्किक> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        जर (visitor.m_ok) परत {visitor.m_stmtsp, सत्य};
        // Otherwise delete intermediate results
        जर (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_वृक्ष(), visitor.m_stmtsp);
        परत {nullptr, असत्य};
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
    स्थिर AstVarRef* unwrapToVarRef(Ast_गाठ_अभिव्यक्ती* nodep, पूर्णांक minWidth) {
        पर्यंत (सत्य) {
            जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef)) परत refp;
            जर (AstExtend* स्थिरांक ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } अन्यथा जर (AstExtendS* स्थिरांक ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } अन्यथा जर (AstSel* स्थिरांक sp = VN_CAST(nodep, Sel)) {
                स्थिरांक AstConst* स्थिरांक lsbp = VN_CAST(sp->lsbp(), Const);
                जर (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) परत nullptr;
                nodep = sp->fromp();
            } अन्यथा {
                परत nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    तार्किक isVarPlus1(Ast_गाठ* nodep, स्थिरांक AstVarScope* vscp) {
        AstAdd* स्थिरांक addp = VN_CAST(nodep, Add);
        जर (!addp || !addp->lhsp()->isOne()) परत असत्य;
        स्थिरांक AstVarRef* स्थिरांक r = unwrapToVarRef(addp->rhsp(), addp->width());
        परत r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    स्थिर Ast_गाठ_अभिव्यक्ती* resizeToWidth(Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक AstVarRef* targetRefp) {
        स्थिरांक पूर्णांक width = targetRefp->width();
        जर (width == 32) परत exprp;
        File_ओळ* स्थिरांक flp = exprp->fileline();
        जर (width < 32) परत नवीन AstSel{flp, exprp, 0, width};
        AstExtend* स्थिरांक extp = नवीन AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        परत extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    स्थिर तार्किक ascendingBound(Ast_गाठ_अभिव्यक्ती* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        जर (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) परत असत्य;
        Ast_गाठ_Biop* स्थिरांक bp = VN_AS(condp, गाठ_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        परत wp && idxRefp && !wp->अंक().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    स्थिर तार्किक isInRangeGuard(Ast_गाठ_अभिव्यक्ती* condp, स्थिरांक AstVarScope* idxVscp, uint32_t width,
                               पूर्णांक addrBits) {
        जर (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) परत असत्य;
        Ast_गाठ_Biop* स्थिरांक bp = VN_AS(condp, गाठ_Biop);
        स्थिरांक AstConst* स्थिरांक cp = VN_CAST(bp->lhsp(), Const);
        जर (!cp || cp->अंक().isFourState() || cp->toUInt() < width - 1) परत असत्य;
        स्थिरांक AstVarRef* स्थिरांक r = unwrapToVarRef(bp->rhsp(), addrBits);
        परत r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    तार्किक tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* स्थिरांक testp = VN_CAST(loopp->stmtsp(), LoopTest);
        जर (!testp) परत असत्य;
        AstIf* स्थिरांक ifp = VN_CAST(testp->nextp(), If);
        जर (!ifp) परत असत्य;
        AstAssign* स्थिरांक incp = VN_CAST(ifp->nextp(), Assign);
        जर (!incp || incp->nextp()) परत असत्य;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        जर (!ascendingBound(testp->condp(), wp, idxRefp)) परत असत्य;
        AstVarScope* स्थिरांक idxVscp = idxRefp->varScopep();
        स्थिरांक uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        स्थिरांक पूर्णांक addrBits = width <= 1 ? 1 : V3_संख्या::log2b(width - 1) + 1;
        स्थिरांक AstConst* स्थिरांक idxInitp = m_bindings.get(idxVscp);
        जर (!idxInitp || !idxInitp->isZero()) परत असत्य;
        AstVarRef* स्थिरांक incLhsp = VN_CAST(incp->lhsp(), VarRef);
        जर (!incLhsp || incLhsp->varScopep() != idxVscp) परत असत्य;
        जर (!isVarPlus1(incp->rhsp(), idxVscp)) परत असत्य;
        जर (ifp->elsesp()) परत असत्य;
        AstAssign* स्थिरांक thenp = VN_CAST(ifp->thensp(), Assign);
        जर (!thenp || thenp->nextp()) परत असत्य;
        AstVarRef* स्थिरांक targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        जर (!targetRefp) परत असत्य;
        AstVarScope* स्थिरांक targetVscp = targetRefp->varScopep();
        जर (targetVscp == idxVscp) परत असत्य;
        स्थिरांक तार्किक isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        स्थिरांक तार्किक is_मोजणी_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        जर (!isLeadingOne && !is_मोजणी_Ones) परत असत्य;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_गाठ_अभिव्यक्ती* condp = ifp->condp();
        जर (AstLogAnd* स्थिरांक andp = VN_CAST(condp, LogAnd)) {
            जर (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } अन्यथा जर (AstCond* स्थिरांक ternp = VN_CAST(condp, Cond)) {
            जर (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* स्थिरांक selp = VN_CAST(condp, Sel);
        जर (!selp || selp->width() != 1) परत असत्य;
        स्थिरांक AstVarRef* स्थिरांक fromp = VN_CAST(selp->fromp(), VarRef);
        जर (!fromp) परत असत्य;
        स्थिरांक AstVarScope* स्थिरांक fromVscp = fromp->varScopep();
        जर (fromVscp == idxVscp || fromVscp == targetVscp) परत असत्य;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        जर (static_cast<पूर्णांक>(width) != vecExprp->width()) परत असत्य;
        स्थिरांक AstVarRef* स्थिरांक idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        जर (!idxInSel || idxInSel->varScopep() != idxVscp) परत असत्य;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        स्थिरांक AstConst* स्थिरांक targetInitp = m_bindings.get(targetVscp);
        जर (!targetInitp || !targetInitp->isZero()) परत असत्य;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_ओळ* स्थिरांक flp = loopp->fileline();
        Ast_गाठ_अभिव्यक्ती* reducep;
        जर (isLeadingOne) {
            reducep = नवीन AstMostSetBitP1{flp, vecExprp->clone_वृक्ष(असत्य)};
        } अन्यथा {
            Ast_मोजणी_Ones* स्थिरांक conep = नवीन Ast_मोजणी_Ones{flp, vecExprp->clone_वृक्ष(असत्य)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* स्थिरांक newp = नवीन AstAssign{flp, targetRefp->clone_वृक्ष(असत्य), reducep};
        newp->addNext(नवीन AstAssign{flp, incLhsp->clone_वृक्ष(असत्य), wp->clone_वृक्ष(असत्य)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(loopp), loopp);
        जर (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } अन्यथा {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_मोजणी_Ones_Lowered;
        }
        परत सत्य;
    }

    // VISIT
    शून्य visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        साठी (Ast_गाठ *succp = nodep, *currp = nodep->backp(); सत्य;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            जर (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                जर (VN_IS(currp, JumpBlock)) सुरूठेवा;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                थांबा;
            }
            AstAssign* स्थिरांक assignp = VN_CAST(currp, Assign);
            जर (!assignp) थांबा;
            AstConst* स्थिरांक valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            जर (!valp) थांबा;
            AstVarRef* स्थिरांक lhsp = VN_CAST(assignp->lhsp(), VarRef);
            जर (!lhsp) थांबा;
            // Don't bind if volatile
            जर (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) सुरूठेवा;
            // Don't overwrite a later binding
            जर (m_bindings.get(lhsp->varScopep())) सुरूठेवा;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        जर (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) परत;

        // Attempt to unroll this loop
        स्थिरांक std::pair<Ast_गाठ*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        जर (!pair.second) {
            iterateChildren(nodep);
            परत;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        जर (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा {
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

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
