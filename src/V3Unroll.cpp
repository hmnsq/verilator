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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Unroll.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statistics tracking

ഘടന UnrollStats final {
    വർഗ്ഗം Stat final {
        size_t m_മൂല്യം = 0;  // Statistics value
        സ്ഥിരം അക്ഷരം* സ്ഥിരം m_name;  // Name for stats file and UDEBUG

    പൊതു:
        explicit Stat(സ്ഥിരം അക്ഷരം* സ്ഥിരം name)
            : m_name{name} {}
        ~Stat() { V3Stats::addStat("Optimizations, Loop unrolling, "s + m_name, m_മൂല്യം); }
        സ്ഥിരം അക്ഷരം* name() സ്ഥിരം { തിരികെ m_name; }
        Stat& operator++() {
            ++m_മൂല്യം;
            തിരികെ *ഇത്;
        }
        Stat& operator+=(size_t v) {
            m_മൂല്യം += v;
            തിരികെ *ഇത്;
        }
    };

    // STATE - statistics
    Stat m_nFailCall{"Failed - non-inlined call"};
    Stat m_nFailCondition{"Failed - unknown loop condition"};
    Stat m_nFailFork{"Failed - contains fork"};
    Stat m_nFailInfinite{"Failed - infinite loop"};
    Stat m_nFailNestedLoopTest{"Failed - loop test in sub-statement"};
    Stat m_nFailTimingControl{"Failed - contains timing control"};
    Stat m_n_Fail_Unroll_എണ്ണം{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bitScanLowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_എണ്ണം_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
വർഗ്ഗം UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::vector<AstConst*> m_curr{nullptr};
    // Stack of binding checkpoints
    std::vector<std::vector<AstConst*>> m_checkpoints;

പൊതു:
    // CONSTRUCTOR
    UnrolllBindings() = സ്ഥിരസ്ഥിതി;
    ~UnrolllBindings() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(UnrolllBindings);
    VL_UNMOVABLE(UnrolllBindings);

    // METHODS
    ശൂന്യം clear() {
        m_curr.resize(1);
        m_checkpoints.clear();
        Ast_കണ്ണി::user1_Clear_വൃക്ഷം();
    }

    ശൂന്യം checkpoint() { m_checkpoints.തള്ളുക_back(m_curr); }

    ശൂന്യം revert() {
        m_curr = std::move(m_checkpoints.back());
        m_checkpoints.എടുക്കുക_back();
    }

    ശൂന്യം commit() { m_checkpoints.എടുക്കുക_back(); }

    ശൂന്യം set(AstVarScope* vscp, AstConst* valp) {
        എങ്കിൽ (!vscp->user1()) {
            vscp->user1(m_curr.size());
            m_curr.തള്ളുക_back(nullptr);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        എങ്കിൽ (static_cast<size_t>(vscp->user1()) >= m_curr.size()) {
            m_curr.resize(vscp->user1() + 1, nullptr);
        }
        AstConst* സ്ഥിരം valp = vscp->user1() ? m_curr[vscp->user1()] : nullptr;
        UINFO(6, "Binding GET " << vscp->name() << " / " << vscp->user1() << " == " << valp);
        തിരികെ valp;
    }
};

//######################################################################
// Unroll one AstLoop

വർഗ്ഗം UnrollOneVisitor final : VNVisitor {
    // STATE
    UnrollStats& m_stats;  // Statistics tracking
    UnrolllBindings& m_bindings;  // Variable bindings
    AstLoop* സ്ഥിരം m_loopp;  // The loop we are trying to unroll
    Ast_കണ്ണി* m_stmtsp = nullptr;  // Resulting unrolled statement
    size_t m_unrolledSize = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* സ്ഥിരം m_wrapp = പുതിയ AstBegin{m_loopp->fileline(), "[EditWrapper]", nullptr, അസത്യം};
    സ്ഥിരം തർക്കം m_unrollFull = m_loopp->unroll().isSetTrue();  // Completely unroll the loop?
    തർക്കം m_ok = സത്യം;  // Unrolling successful so far, gave up if false

    // METHODS
    ശൂന്യം cantUnroll(Ast_കണ്ണി* nodep, UnrollStats::Stat& stat) {
        m_ok = അസത്യം;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    തർക്കം unrollOneIteration(Ast_കണ്ണി* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        തർക്കം foundLoopTest = അസത്യം;
        // Process one body statement at a time
        വേണ്ടി (Ast_കണ്ണി* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            എങ്കിൽ (AstLoopTest* സ്ഥിരം testp = VN_CAST(stmtp, LoopTest)) {
                Ast_കണ്ണി* സ്ഥിരം condp = V3Const::constifyEdit(testp->condp());
                എങ്കിൽ (condp->isZero()) {
                    // Loop terminates
                    തിരികെ അസത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    തുടരുക;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->addStmtsp(stmtp->clone_വൃക്ഷം(അസത്യം));
            iterateAndNextNull(m_wrapp->stmtsp());
            // Give up if failed
            എങ്കിൽ (!m_ok) തിരികെ അസത്യം;

            // Add statements to unrolled body
            ആകുമ്പോൾ (Ast_കണ്ണി* സ്ഥിരം nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                എങ്കിൽ (!m_unrollFull) {
                    m_unrolledSize += nodep->കണ്ണി_എണ്ണം();
                    എങ്കിൽ (m_unrolledSize > static_cast<size_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        തിരികെ അസത്യം;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlinkFrBack();

                // If a LoopTest, check how it resolved
                എങ്കിൽ (AstLoopTest* സ്ഥിരം testp = VN_CAST(nodep, LoopTest)) {
                    foundLoopTest = സത്യം;
                    // Will not actually need it, nor any subsequent
                    തള്ളുക_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    എങ്കിൽ (testp->condp()->isNeqZero()) തുടരുക;
                    // Won't need any of the trailing statements
                    എങ്കിൽ (m_wrapp->stmtsp()) തള്ളുക_Deletep(m_wrapp->stmtsp()->unlinkFrBackWithNext());
                    // Loop terminates
                    എങ്കിൽ (testp->condp()->isZero()) തിരികെ അസത്യം;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    തിരികെ അസത്യം;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_കണ്ണി::addNext(m_stmtsp, nodep);

                // Check if terminated via JumpGo
                എങ്കിൽ (VN_IS(nodep, JumpGo)) {
                    UASSERT_OBJ(!m_wrapp->stmtsp(), nodep, "Statements after JumpGo");
                    // This JumpGo is going directly into the body of the unrolled loop.
                    // A JumpGo always redirects to the end of an enclosing JumpBlock,
                    // so this JumpGo must go outside the loop. The loop terminates.
                    തിരികെ അസത്യം;
                }
            }
        }
        // If there is no loop test in the body, give up, it's an infinite loop
        എങ്കിൽ (!foundLoopTest) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            തിരികെ അസത്യം;
        }
        // One iteration done, loop continues
        തിരികെ സത്യം;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    തർക്കം process(Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        എങ്കിൽ (!nodep) തിരികെ സത്യം;
        // Variable references we should try to substitute
        std::vector<AstVarRef*> toSubstitute;
        // Iterate subtree
        nodep->foreach([&](Ast_കണ്ണി* np) {
            // Failed earlier
            എങ്കിൽ (!m_ok) തിരികെ;
            // Check for AstLoopTest
            എങ്കിൽ (AstLoopTest* സ്ഥിരം testp = VN_CAST(np, LoopTest)) {
                // Nested loop is OK, bail only if the nested LoopTest is for the current loop
                എങ്കിൽ (testp->loopp() == m_loopp) cantUnroll(np, m_stats.m_nFailNestedLoopTest);
                തിരികെ;
            }
            // Check for AstFork - can't unroll
            എങ്കിൽ (VN_IS(np, Fork)) {
                cantUnroll(np, m_stats.m_nFailFork);
                തിരികെ;
            }
            // Check for calls - can't unroll if not pure might modify bindings
            എങ്കിൽ (Ast_കണ്ണി_CCall* സ്ഥിരം callp = VN_CAST(np, കണ്ണി_CCall)) {
                എങ്കിൽ (!callp->isPure()) {
                    cantUnroll(np, m_stats.m_nFailCall);
                    തിരികെ;
                }
            }
            // Check for timing control - can't unroll, might modify bindings
            എങ്കിൽ (np->isTimingControl()) {
                cantUnroll(np, m_stats.m_nFailTimingControl);
                തിരികെ;
            }
            // Process variable references
            AstVarRef* സ്ഥിരം refp = VN_CAST(np, VarRef);
            എങ്കിൽ (!refp) തിരികെ;
            // Ignore if the referenced variable has no binding
            AstConst* സ്ഥിരം valp = m_bindings.get(refp->varScopep());
            എങ്കിൽ (!valp) തിരികെ;
            // If writen, remove the binding
            എങ്കിൽ (refp->access().isWriteOrRW()) {
                m_bindings.set(refp->varScopep(), nullptr);
                തിരികെ;
            }
            // Otherwise add it to the list of variables to substitute
            toSubstitute.തള്ളുക_back(refp);
        });
        // Give up if we have to
        എങ്കിൽ (!m_ok) തിരികെ അസത്യം;
        // Actually substitute the variables that still have bindings
        വേണ്ടി (AstVarRef* സ്ഥിരം refp : toSubstitute) {
            // Pick up bound value
            AstConst* സ്ഥിരം valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            എങ്കിൽ (!valp) തുടരുക;
            // Substitute it
            refp->replaceWith(valp->clone_വൃക്ഷം(അസത്യം));
            VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
        }
        തിരികെ സത്യം;
    }

    // CONSTRUCTOR
    UnrollOneVisitor(UnrollStats& stats, UnrolllBindings& bindings, AstLoop* loopp)
        : m_stats{stats}
        , m_bindings{bindings}
        , m_loopp{loopp} {
        UASSERT_OBJ(!loopp->contsp(), loopp, "'contsp' only used before LinkJump");
        // Do not unroll if we are told not to
        എങ്കിൽ (loopp->unroll().isSetFalse()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            തിരികെ;
        }
        // Attempt to unroll the loop
        സ്ഥിരം size_t iterLimit = m_unrollFull ? v3Global.opt.unrollLimit()  //
                                              : v3Global.opt.unroll_എണ്ണം();
        size_t iter_എണ്ണം = 0;
        ചെയ്യുക {
            എങ്കിൽ (iter_എണ്ണം > iterLimit) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_എണ്ണം);
                എങ്കിൽ (m_unrollFull) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iterLimit);
                }
                തിരികെ;
            }
            ++iter_എണ്ണം;
        } ആകുമ്പോൾ (unrollOneIteration(loopp->stmtsp()));

        എങ്കിൽ (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_എണ്ണം;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_വൃക്ഷം(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (!m_ok) തിരികെ;
        // Generic body statement, just substitute
        process(nodep);
    }
    ശൂന്യം visit(AstLoopTest* nodep) override {
        എങ്കിൽ (!m_ok) തിരികെ;
        // If the condition is a ExprStmt, move it before the LoopTest
        എങ്കിൽ (Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം exprp = VN_CAST(nodep->condp(), വ്യഞ്ജകം_Stmt)) {
            Ast_കണ്ണി* സ്ഥിരം stmtsp = exprp->stmtsp()->unlinkFrBackWithNext();
            exprp->replaceWith(exprp->resultp()->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            stmtsp->addNext(nodep);
            relinker.relink(stmtsp);
            തിരികെ;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    ശൂന്യം visit(AstAssign* nodep) override {
        എങ്കിൽ (!m_ok) തിരികെ;
        // Can't do it if delayed
        എങ്കിൽ (nodep->timingControlp()) {
            cantUnroll(nodep, m_stats.m_nFailTimingControl);
            തിരികെ;
        }
        എങ്കിൽ (!process(nodep->rhsp())) തിരികെ;
        // If a simple variable assignment, update the binding
        AstVarRef* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp(), VarRef);
        AstConst* സ്ഥിരം valp = VN_CAST(V3Const::constifyEdit(nodep->rhsp()), Const);
        എങ്കിൽ (lhsp && valp && !lhsp->varp()->isForced() && !lhsp->varp()->isSigUserRWPublic()) {
            m_bindings.set(lhsp->varScopep(), valp);
            തിരികെ;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    ശൂന്യം visit(AstIf* nodep) override {
        എങ്കിൽ (!m_ok) തിരികെ;
        എങ്കിൽ (!process(nodep->condp())) തിരികെ;
        // If condition is constant, replce with the relevant branch
        എങ്കിൽ (AstConst* സ്ഥിരം condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->addNextHere(bodyp->unlinkFrBackWithNext());  // This will be iterated next
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    ശൂന്യം visit(AstJumpGo* nodep) override {
        എങ്കിൽ (!m_ok) തിരികെ;
        // Remove trailing dead code
        എങ്കിൽ (nodep->nextp()) തള്ളുക_Deletep(nodep->nextp()->unlinkFrBackWithNext());
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        എങ്കിൽ (!m_ok) തിരികെ;
        m_bindings.checkpoint();
        std::pair<Ast_കണ്ണി*, തർക്കം> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, revert the bindings and process the loop as a generic statement
        എങ്കിൽ (!pair.second) {
            m_bindings.revert();
            process(nodep);
            തിരികെ;
        }

        // Commit the bindings and replace the loop with the unrolled code
        m_bindings.commit();
        എങ്കിൽ (pair.first) {
            nodep->replaceWith(pair.first);
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

പൊതു:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    സ്ഥിതം std::pair<Ast_കണ്ണി*, തർക്കം> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        എങ്കിൽ (visitor.m_ok) തിരികെ {visitor.m_stmtsp, സത്യം};
        // Otherwise delete intermediate results
        എങ്കിൽ (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_വൃക്ഷം(), visitor.m_stmtsp);
        തിരികെ {nullptr, അസത്യം};
    }
};

//######################################################################
// Unroll all AstLoop statements

വർഗ്ഗം UnrollAllVisitor final : VNVisitor {
    // STATE
    UnrollStats m_stats;  // Statistic tracking
    UnrolllBindings m_bindings;  // Variable bindings

    // METHODS
    // Peel value-preserving width casts (Extend/ExtendS, or a low-bits Sel with lsb 0) to the
    // underlying VarRef.  A Sel kept narrower than 'minWidth' is a lossy narrowing (idx[1:0])
    // and is rejected.
    സ്ഥിതം AstVarRef* unwrapToVarRef(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, പൂർണ്ണസംഖ്യ minWidth) {
        ആകുമ്പോൾ (സത്യം) {
            എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) തിരികെ refp;
            എങ്കിൽ (AstExtend* സ്ഥിരം ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } അല്ലെങ്കിൽ എങ്കിൽ (AstExtendS* സ്ഥിരം ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } അല്ലെങ്കിൽ എങ്കിൽ (AstSel* സ്ഥിരം sp = VN_CAST(nodep, Sel)) {
                സ്ഥിരം AstConst* സ്ഥിരം lsbp = VN_CAST(sp->lsbp(), Const);
                എങ്കിൽ (!lsbp || lsbp->toUInt() != 0 || sp->width() < minWidth) തിരികെ nullptr;
                nodep = sp->fromp();
            } അല്ലെങ്കിൽ {
                തിരികെ nullptr;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    തർക്കം isVarPlus1(Ast_കണ്ണി* nodep, സ്ഥിരം AstVarScope* vscp) {
        AstAdd* സ്ഥിരം addp = VN_CAST(nodep, Add);
        എങ്കിൽ (!addp || !addp->lhsp()->isOne()) തിരികെ അസത്യം;
        സ്ഥിരം AstVarRef* സ്ഥിരം r = unwrapToVarRef(addp->rhsp(), addp->width());
        തിരികെ r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* resizeToWidth(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം AstVarRef* targetRefp) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = targetRefp->width();
        എങ്കിൽ (width == 32) തിരികെ exprp;
        File_വരി* സ്ഥിരം flp = exprp->fileline();
        എങ്കിൽ (width < 32) തിരികെ പുതിയ AstSel{flp, exprp, 0, width};
        AstExtend* സ്ഥിരം extp = പുതിയ AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        തിരികെ extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    സ്ഥിതം തർക്കം ascendingBound(Ast_കണ്ണി_വ്യഞ്ജകം* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        എങ്കിൽ (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) തിരികെ അസത്യം;
        Ast_കണ്ണി_Biop* സ്ഥിരം bp = VN_AS(condp, കണ്ണി_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        തിരികെ wp && idxRefp && !wp->അക്കം().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    സ്ഥിതം തർക്കം isInRangeGuard(Ast_കണ്ണി_വ്യഞ്ജകം* condp, സ്ഥിരം AstVarScope* idxVscp, uint32_t width,
                               പൂർണ്ണസംഖ്യ addrBits) {
        എങ്കിൽ (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) തിരികെ അസത്യം;
        Ast_കണ്ണി_Biop* സ്ഥിരം bp = VN_AS(condp, കണ്ണി_Biop);
        സ്ഥിരം AstConst* സ്ഥിരം cp = VN_CAST(bp->lhsp(), Const);
        എങ്കിൽ (!cp || cp->അക്കം().isFourState() || cp->toUInt() < width - 1) തിരികെ അസത്യം;
        സ്ഥിരം AstVarRef* സ്ഥിരം r = unwrapToVarRef(bp->rhsp(), addrBits);
        തിരികെ r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    തർക്കം tryLowerBitScanLoop(AstLoop* loopp) {
        AstLoopTest* സ്ഥിരം testp = VN_CAST(loopp->stmtsp(), LoopTest);
        എങ്കിൽ (!testp) തിരികെ അസത്യം;
        AstIf* സ്ഥിരം ifp = VN_CAST(testp->nextp(), If);
        എങ്കിൽ (!ifp) തിരികെ അസത്യം;
        AstAssign* സ്ഥിരം incp = VN_CAST(ifp->nextp(), Assign);
        എങ്കിൽ (!incp || incp->nextp()) തിരികെ അസത്യം;
        AstConst* wp = nullptr;
        AstVarRef* idxRefp = nullptr;
        എങ്കിൽ (!ascendingBound(testp->condp(), wp, idxRefp)) തിരികെ അസത്യം;
        AstVarScope* സ്ഥിരം idxVscp = idxRefp->varScopep();
        സ്ഥിരം uint32_t width = wp->toUInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        സ്ഥിരം പൂർണ്ണസംഖ്യ addrBits = width <= 1 ? 1 : V3_സംഖ്യ::log2b(width - 1) + 1;
        സ്ഥിരം AstConst* സ്ഥിരം idxInitp = m_bindings.get(idxVscp);
        എങ്കിൽ (!idxInitp || !idxInitp->isZero()) തിരികെ അസത്യം;
        AstVarRef* സ്ഥിരം incLhsp = VN_CAST(incp->lhsp(), VarRef);
        എങ്കിൽ (!incLhsp || incLhsp->varScopep() != idxVscp) തിരികെ അസത്യം;
        എങ്കിൽ (!isVarPlus1(incp->rhsp(), idxVscp)) തിരികെ അസത്യം;
        എങ്കിൽ (ifp->elsesp()) തിരികെ അസത്യം;
        AstAssign* സ്ഥിരം thenp = VN_CAST(ifp->thensp(), Assign);
        എങ്കിൽ (!thenp || thenp->nextp()) തിരികെ അസത്യം;
        AstVarRef* സ്ഥിരം targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        എങ്കിൽ (!targetRefp) തിരികെ അസത്യം;
        AstVarScope* സ്ഥിരം targetVscp = targetRefp->varScopep();
        എങ്കിൽ (targetVscp == idxVscp) തിരികെ അസത്യം;
        സ്ഥിരം തർക്കം isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        സ്ഥിരം തർക്കം is_എണ്ണം_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        എങ്കിൽ (!isLeadingOne && !is_എണ്ണം_Ones) തിരികെ അസത്യം;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_കണ്ണി_വ്യഞ്ജകം* condp = ifp->condp();
        എങ്കിൽ (AstLogAnd* സ്ഥിരം andp = VN_CAST(condp, LogAnd)) {
            എങ്കിൽ (isInRangeGuard(andp->lhsp(), idxVscp, width, addrBits)) condp = andp->rhsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (AstCond* സ്ഥിരം ternp = VN_CAST(condp, Cond)) {
            എങ്കിൽ (isInRangeGuard(ternp->condp(), idxVscp, width, addrBits)) condp = ternp->thenp();
        }
        AstSel* സ്ഥിരം selp = VN_CAST(condp, Sel);
        എങ്കിൽ (!selp || selp->width() != 1) തിരികെ അസത്യം;
        സ്ഥിരം AstVarRef* സ്ഥിരം fromp = VN_CAST(selp->fromp(), VarRef);
        എങ്കിൽ (!fromp) തിരികെ അസത്യം;
        സ്ഥിരം AstVarScope* സ്ഥിരം fromVscp = fromp->varScopep();
        എങ്കിൽ (fromVscp == idxVscp || fromVscp == targetVscp) തിരികെ അസത്യം;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        എങ്കിൽ (static_cast<പൂർണ്ണസംഖ്യ>(width) != vecExprp->width()) തിരികെ അസത്യം;
        സ്ഥിരം AstVarRef* സ്ഥിരം idxInSel = unwrapToVarRef(selp->lsbp(), addrBits);
        എങ്കിൽ (!idxInSel || idxInSel->varScopep() != idxVscp) തിരികെ അസത്യം;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        സ്ഥിരം AstConst* സ്ഥിരം targetInitp = m_bindings.get(targetVscp);
        എങ്കിൽ (!targetInitp || !targetInitp->isZero()) തിരികെ അസത്യം;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_വരി* സ്ഥിരം flp = loopp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* reducep;
        എങ്കിൽ (isLeadingOne) {
            reducep = പുതിയ AstMostSetBitP1{flp, vecExprp->clone_വൃക്ഷം(അസത്യം)};
        } അല്ലെങ്കിൽ {
            Ast_എണ്ണം_Ones* സ്ഥിരം conep = പുതിയ Ast_എണ്ണം_Ones{flp, vecExprp->clone_വൃക്ഷം(അസത്യം)};
            reducep = conep;
        }
        reducep = resizeToWidth(reducep, targetRefp);
        AstAssign* സ്ഥിരം newp = പുതിയ AstAssign{flp, targetRefp->clone_വൃക്ഷം(അസത്യം), reducep};
        newp->addNext(പുതിയ AstAssign{flp, incLhsp->clone_വൃക്ഷം(അസത്യം), wp->clone_വൃക്ഷം(അസത്യം)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(loopp), loopp);
        എങ്കിൽ (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bitScanLowered;
        } അല്ലെങ്കിൽ {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_എണ്ണം_Ones_Lowered;
        }
        തിരികെ സത്യം;
    }

    // VISIT
    ശൂന്യം visit(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        വേണ്ടി (Ast_കണ്ണി *succp = nodep, *currp = nodep->backp(); സത്യം;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            എങ്കിൽ (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                എങ്കിൽ (VN_IS(currp, JumpBlock)) തുടരുക;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                നിർത്തുക;
            }
            AstAssign* സ്ഥിരം assignp = VN_CAST(currp, Assign);
            എങ്കിൽ (!assignp) നിർത്തുക;
            AstConst* സ്ഥിരം valp = VN_CAST(V3Const::constifyEdit(assignp->rhsp()), Const);
            എങ്കിൽ (!valp) നിർത്തുക;
            AstVarRef* സ്ഥിരം lhsp = VN_CAST(assignp->lhsp(), VarRef);
            എങ്കിൽ (!lhsp) നിർത്തുക;
            // Don't bind if volatile
            എങ്കിൽ (lhsp->varp()->isForced() || lhsp->varp()->isSigUserRWPublic()) തുടരുക;
            // Don't overwrite a later binding
            എങ്കിൽ (m_bindings.get(lhsp->varScopep())) തുടരുക;
            // Set up the binding
            m_bindings.set(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        എങ്കിൽ (v3Global.opt.fBitScanLoops() && tryLowerBitScanLoop(nodep)) തിരികെ;

        // Attempt to unroll this loop
        സ്ഥിരം std::pair<Ast_കണ്ണി*, തർക്കം> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        എങ്കിൽ (!pair.second) {
            iterateChildren(nodep);
            തിരികെ;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        എങ്കിൽ (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ {
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    explicit UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(AstNetlist* netlistp) { UnrollAllVisitor{netlistp}; }
};

//######################################################################
// V3Unroll class functions

ശൂന്യം V3Unroll::unrollAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    UnrollAllVisitor::apply(nodep);
    V3Global::dump_Check_Global_വൃക്ഷം("unroll", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
