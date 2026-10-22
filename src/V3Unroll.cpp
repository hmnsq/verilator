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
        आकार_t m_मानम् = 0;  // Statistics value
        ध्रुव वर्ण* ध्रुव m_name;  // Name for stats file and UDEBUG

    सार्वजनिक:
        स्पष्ट Stat(ध्रुव वर्ण* ध्रुव name)
            : m_name{name} {}
        ~Stat() { V3Stats::सङ्कलय_Stat("Optimizations, Loop unrolling, "s + m_name, m_मानम्); }
        ध्रुव वर्ण* name() ध्रुव { प्रत्यागम m_name; }
        Stat& संक्रियकः++() {
            ++m_मानम्;
            प्रत्यागम *एतत्;
        }
        Stat& संक्रियकः+=(आकार_t v) {
            m_मानम् += v;
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
    Stat m_n_Fail_Unroll_गणनम्{"Failed - reached --unroll-count"};
    Stat m_nFailUnrollStmts{"Failed - reached --unroll-stmts"};
    Stat m_nPragmaDisabled{"Pragma unroll_disable"};
    Stat m_nUnrolledLoops{"Unrolled loops"};
    Stat m_nUnrolledIters{"Unrolled iterations"};
    Stat m_bit_अवलोकनम्_Lowered{"Lowered priority-encoder to mostsetbitp1"};
    Stat m_गणनम्_Ones_Lowered{"Lowered count-set-bits to countones"};
};

//######################################################################
// Variable bindings
वर्ग UnrolllBindings final {
    // NODE STATE
    // AstVarScope::user1()    int: index of the binding in m_bindings
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE
    // Map from AstVarScope::user1() to current variable value, nullptr if not bound - idx 0 unused
    std::सदिशम्<AstConst*> m_curr{शून्यसूचकः};
    // Stack of binding checkpoints
    std::सदिशम्<std::सदिशम्<AstConst*>> m_checkpoints;

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

    शून्य checkpoint() { m_checkpoints.प्रक्षिप_पृष्ठभागः(m_curr); }

    शून्य revert() {
        m_curr = std::move(m_checkpoints.पृष्ठभागः());
        m_checkpoints.निष्कासय_पृष्ठभागः();
    }

    शून्य commit() { m_checkpoints.निष्कासय_पृष्ठभागः(); }

    शून्य समुच्चयः(AstVarScope* vscp, AstConst* valp) {
        यदि (!vscp->user1()) {
            vscp->user1(m_curr.आकार());
            m_curr.प्रक्षिप_पृष्ठभागः(शून्यसूचकः);
        }
        UINFO(6, "Binding SET " << vscp->name() << " / " << vscp->user1() << " := " << valp);
        m_curr[vscp->user1()] = valp;
    }

    AstConst* get(AstVarScope* vscp) {
        // It's possible after a revert that user1 is set, but the vector is shorter, pad up
        यदि (static_cast<आकार_t>(vscp->user1()) >= m_curr.आकार()) {
            m_curr.resize(vscp->user1() + 1, शून्यसूचकः);
        }
        AstConst* ध्रुव valp = vscp->user1() ? m_curr[vscp->user1()] : शून्यसूचकः;
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
    Ast_ग्रन्थिः* m_stmtsp = शून्यसूचकः;  // Resulting unrolled statement
    आकार_t m_unrolled_आकार = 0;  // Number of nodes in unrolled loop
    // Temporary block needed for iteration of cloned statements
    AstBegin* ध्रुव m_wrapp = नूतन AstBegin{m_loopp->fileline(), "[EditWrapper]", शून्यसूचकः, असत्य};
    ध्रुव तार्किक m_unroll_पूर्णम् = m_loopp->unroll().is_समुच्चयः_True();  // Completely unroll the loop?
    तार्किक m_ok = सत्य;  // Unrolling successful so far, gave up if false

    // METHODS
    शून्य cantUnroll(Ast_ग्रन्थिः* nodep, UnrollStats::Stat& stat) {
        m_ok = असत्य;
        ++stat;
        UINFO(4, "   Can't Unroll: " << stat.name() << " :" << nodep);
    }

    // Returns false if the loop terminated (or we gave up)
    तार्किक unrollOneIteration(Ast_ग्रन्थिः* stmtsp) {
        // True if the loop contains at least one dependent AstLoopTest
        तार्किक प्राप्तम्_Loop_Test = असत्य;
        // Process one body statement at a time
        प्रति (Ast_ग्रन्थिः* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            // Check if this is a loop test - before substitution
            यदि (AstLoopTest* ध्रुव testp = VN_CAST(stmtp, LoopTest)) {
                Ast_ग्रन्थिः* ध्रुव condp = V3Const::constifyEdit(testp->condp());
                यदि (condp->isZero()) {
                    // Loop terminates
                    प्रत्यागम असत्य;
                } अन्यथा यदि (condp->isNeqZero()) {
                    // Loop test is unconditionally true, ignore
                    अनुवर्त;
                }
            }

            // Clone and iterate one body statement
            m_wrapp->सङ्कलय_Stmtsp(stmtp->clone_वृक्षः(असत्य));
            iterate_And_अग्र_Null(m_wrapp->stmtsp());
            // Give up if failed
            यदि (!m_ok) प्रत्यागम असत्य;

            // Add statements to unrolled body
            यावत् (Ast_ग्रन्थिः* ध्रुव nodep = m_wrapp->stmtsp()) {
                // Check if we reached the size limit, unless full unrolling is requested
                यदि (!m_unroll_पूर्णम्) {
                    m_unrolled_आकार += nodep->ग्रन्थिः_गणनम्();
                    यदि (m_unrolled_आकार > static_cast<आकार_t>(v3Global.opt.unrollStmts())) {
                        cantUnroll(m_loopp, m_stats.m_nFailUnrollStmts);
                        प्रत्यागम असत्य;
                    }
                }

                // Will be adding to results (or deleting)
                nodep->unlink_Fr_पृष्ठभागः();

                // If a LoopTest, check how it resolved
                यदि (AstLoopTest* ध्रुव testp = VN_CAST(nodep, LoopTest)) {
                    प्राप्तम्_Loop_Test = सत्य;
                    // Will not actually need it, nor any subsequent
                    प्रक्षिप_Deletep(testp);  // TODO testp used past here
                    // Loop continues - add rest of statements
                    यदि (testp->condp()->isNeqZero()) अनुवर्त;
                    // Won't need any of the trailing statements
                    यदि (m_wrapp->stmtsp()) प्रक्षिप_Deletep(m_wrapp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                    // Loop terminates
                    यदि (testp->condp()->isZero()) प्रत्यागम असत्य;
                    // Loop condition unknown - cannot unroll
                    cantUnroll(testp->condp(), m_stats.m_nFailCondition);
                    प्रत्यागम असत्य;
                }

                // Add this statement to the result list
                m_stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_stmtsp, nodep);

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
        यदि (!प्राप्तम्_Loop_Test) {
            cantUnroll(m_loopp, m_stats.m_nFailInfinite);
            प्रत्यागम असत्य;
        }
        // One iteration done, loop continues
        प्रत्यागम सत्य;
    }

    // Substitute all reads of bound variables with their value. If a write is
    // encountered, remove the binding and don't substitute that variable.
    // Returns false if we can't unroll
    तार्किक process(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(m_ok, nodep, "Should not call 'substituteCondVscp' if we gave up");
        यदि (!nodep) प्रत्यागम सत्य;
        // Variable references we should try to substitute
        std::सदिशम्<AstVarRef*> गन्तव्यम्_Substitute;
        // Iterate subtree
        nodep->foreach([&](Ast_ग्रन्थिः* np) {
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
            यदि (Ast_ग्रन्थिः_CCall* ध्रुव callp = VN_CAST(np, ग्रन्थिः_CCall)) {
                यदि (!callp->is_विशुद्धम्()) {
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
                m_bindings.समुच्चयः(refp->varScopep(), शून्यसूचकः);
                प्रत्यागम;
            }
            // Otherwise add it to the list of variables to substitute
            गन्तव्यम्_Substitute.प्रक्षिप_पृष्ठभागः(refp);
        });
        // Give up if we have to
        यदि (!m_ok) प्रत्यागम असत्य;
        // Actually substitute the variables that still have bindings
        प्रति (AstVarRef* ध्रुव refp : गन्तव्यम्_Substitute) {
            // Pick up bound value
            AstConst* ध्रुव valp = m_bindings.get(refp->varScopep());
            // Binding might have been removed after adding to 'toSubstitute'
            यदि (!valp) अनुवर्त;
            // Substitute it
            refp->replaceWith(valp->clone_वृक्षः(असत्य));
            VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
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
        यदि (loopp->unroll().is_समुच्चयः_False()) {
            cantUnroll(loopp, m_stats.m_nPragmaDisabled);
            प्रत्यागम;
        }
        // Attempt to unroll the loop
        ध्रुव आकार_t iter_सीमा = m_unroll_पूर्णम् ? v3Global.opt.unroll_सीमा()  //
                                              : v3Global.opt.unroll_गणनम्();
        आकार_t iter_गणनम् = 0;
        कुरु {
            यदि (iter_गणनम् > iter_सीमा) {
                cantUnroll(m_loopp, m_stats.m_n_Fail_Unroll_गणनम्);
                यदि (m_unroll_पूर्णम्) {
                    loopp->v3error("Unrolling procedural loop with '/* verilator unroll_full */' "
                                   "took too long; probably this is an infinite loop, otherwise "
                                   "set '--unroll-limit' above "
                                   << iter_सीमा);
                }
                प्रत्यागम;
            }
            ++iter_गणनम्;
        } यावत् (unrollOneIteration(loopp->stmtsp()));

        यदि (m_ok) {
            ++m_stats.m_nUnrolledLoops;
            m_stats.m_nUnrolledIters += iter_गणनम्;
        }
    }
    ~UnrollOneVisitor() { VL_DO_DANGLING(m_wrapp->delete_वृक्षः(), m_wrapp); }

    // VISIT - these are called for the statements directly in the loop body
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        // Generic body statement, just substitute
        process(nodep);
    }
    शून्य दर्शय(AstLoopTest* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        // If the condition is a ExprStmt, move it before the LoopTest
        यदि (Ast_व्यञ्जकम्_Stmt* ध्रुव exprp = VN_CAST(nodep->condp(), व्यञ्जकम्_Stmt)) {
            Ast_ग्रन्थिः* ध्रुव stmtsp = exprp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र();
            exprp->replaceWith(exprp->resultp()->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(exprp), exprp);
            VNRelinker relinker;
            nodep->unlink_Fr_पृष्ठभागः(&relinker);
            stmtsp->सङ्कलय_अग्र(nodep);
            relinker.relink(stmtsp);
            प्रत्यागम;
        }
        // Substitute the condition only, this is not a nested AstLoopTest
        process(nodep->condp());
        // Also simplify it, it will be checked later
        V3Const::constifyEdit(nodep->condp());
    }
    शून्य दर्शय(Ast_नियोजय* nodep) override {
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
            m_bindings.समुच्चयः(lhsp->varScopep(), valp);
            प्रत्यागम;
        }
        // Otherwise just like a generic statement
        process(nodep->lhsp());
    }
    शून्य दर्शय(AstIf* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        यदि (!process(nodep->condp())) प्रत्यागम;
        // If condition is constant, replce with the relevant branch
        यदि (AstConst* ध्रुव condp = VN_CAST(V3Const::constifyEdit(nodep->condp()), Const)) {
            यदि (Ast_ग्रन्थिः* ध्रुव bodyp = condp->isNeqZero() ? nodep->thensp() : nodep->elsesp()) {
                nodep->सङ्कलय_अग्र_अत्र(bodyp->unlink_Fr_पृष्ठभागः_With_अग्र());  // This will be iterated next
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        // Otherwise just like a generic statement
        process(nodep);
    }
    शून्य दर्शय(AstJumpGo* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        // Remove trailing dead code
        यदि (nodep->nextp()) प्रक्षिप_Deletep(nodep->nextp()->unlink_Fr_पृष्ठभागः_With_अग्र());
    }
    शून्य दर्शय(AstLoop* nodep) override {
        यदि (!m_ok) प्रत्यागम;
        m_bindings.checkpoint();
        std::pair<Ast_ग्रन्थिः*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

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
            nodep->unlink_Fr_पृष्ठभागः();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

सार्वजनिक:
    // Unroll the given loop. Returns the resulting statements and a flag indicating success
    स्थिर std::pair<Ast_ग्रन्थिः*, तार्किक> apply(UnrollStats& stats, UnrolllBindings& bindings,
                                           AstLoop* loopp) {
        UnrollOneVisitor visitor{stats, bindings, loopp};
        // If successfully unrolled, return the resulting list of statements - might be empty
        यदि (visitor.m_ok) प्रत्यागम {visitor.m_stmtsp, सत्य};
        // Otherwise delete intermediate results
        यदि (visitor.m_stmtsp) VL_DO_DANGLING(visitor.m_stmtsp->delete_वृक्षः(), visitor.m_stmtsp);
        प्रत्यागम {शून्यसूचकः, असत्य};
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
    स्थिर AstVarRef* unwrap_गन्तव्यम्_Var_Ref(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क min_विस्तारः) {
        यावत् (सत्य) {
            यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) प्रत्यागम refp;
            यदि (AstExtend* ध्रुव ep = VN_CAST(nodep, Extend)) {
                nodep = ep->lhsp();
            } अन्यथा यदि (AstExtendS* ध्रुव ep = VN_CAST(nodep, ExtendS)) {
                nodep = ep->lhsp();
            } अन्यथा यदि (AstSel* ध्रुव sp = VN_CAST(nodep, Sel)) {
                ध्रुव AstConst* ध्रुव lsbp = VN_CAST(sp->lsbp(), Const);
                यदि (!lsbp || lsbp->गन्तव्यम्_UInt() != 0 || sp->विस्तारः() < min_विस्तारः) प्रत्यागम शून्यसूचकः;
                nodep = sp->fromp();
            } अन्यथा {
                प्रत्यागम शून्यसूचकः;
            }
        }
    }
    // True if 'nodep' is exactly '1 + var' for 'vscp' (V3Const puts the constant on the LHS).
    // Passing the add's width as minWidth rejects a lossy increment like 32'(i[1:0]) + 1.
    तार्किक isVarPlus1(Ast_ग्रन्थिः* nodep, ध्रुव AstVarScope* vscp) {
        Ast_सङ्कलय* ध्रुव addp = VN_CAST(nodep, सङ्कलय_2);
        यदि (!addp || !addp->lhsp()->isOne()) प्रत्यागम असत्य;
        ध्रुव AstVarRef* ध्रुव r = unwrap_गन्तव्यम्_Var_Ref(addp->rhsp(), addp->विस्तारः());
        प्रत्यागम r && r->varScopep() == vscp;
    }
    // Resize the 32-bit reduction to the accumulator width; truncating the low bits matches
    // the original counted loop's wrap-around.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* resize_गन्तव्यम्_विस्तारः(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव AstVarRef* targetRefp) {
        ध्रुव पूर्णाङ्क विस्तारः = targetRefp->विस्तारः();
        यदि (विस्तारः == 32) प्रत्यागम exprp;
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        यदि (विस्तारः < 32) प्रत्यागम नूतन AstSel{flp, exprp, 0, विस्तारः};
        AstExtend* ध्रुव extp = नूतन AstExtend{flp, exprp};
        extp->dtypeFrom(targetRefp);
        प्रत्यागम extp;
    }
    // Match a strict ascending loop bound 'idx < W'.  V3Const canonicalizes this to the
    // 'W > idx' form (Gt unsigned, GtS signed), so only that form is matched.
    स्थिर तार्किक ascendingBound(Ast_ग्रन्थिः_व्यञ्जकम्* condp, AstConst*& wp, AstVarRef*& idxRefp) {
        यदि (!VN_IS(condp, Gt) && !VN_IS(condp, GtS)) प्रत्यागम असत्य;
        Ast_ग्रन्थिः_Biop* ध्रुव bp = VN_AS(condp, ग्रन्थिः_Biop);
        wp = VN_CAST(bp->lhsp(), Const);
        idxRefp = VN_CAST(bp->rhsp(), VarRef);
        प्रत्यागम wp && idxRefp && !wp->अङ्कः().isFourState();
    }
    // Recognize the redundant in-range guard Verilator auto-inserts for a select into a
    // non-power-of-two vector.  V3Const canonicalizes 'idx <= C' to '(C >= idx)' (Gte/GteS,
    // const on the LHS), so only that form occurs; with C >= W-1 it is always true for idx
    // in 0..W-1.
    स्थिर तार्किक is_अन्तर्_Range_Guard(Ast_ग्रन्थिः_व्यञ्जकम्* condp, ध्रुव AstVarScope* idxVscp, uint32_t विस्तारः,
                               पूर्णाङ्क addr_द्वयङ्काः) {
        यदि (!VN_IS(condp, Gte) && !VN_IS(condp, GteS)) प्रत्यागम असत्य;
        Ast_ग्रन्थिः_Biop* ध्रुव bp = VN_AS(condp, ग्रन्थिः_Biop);
        ध्रुव AstConst* ध्रुव cp = VN_CAST(bp->lhsp(), Const);
        यदि (!cp || cp->अङ्कः().isFourState() || cp->गन्तव्यम्_UInt() < विस्तारः - 1) प्रत्यागम असत्य;
        ध्रुव AstVarRef* ध्रुव r = unwrap_गन्तव्यम्_Var_Ref(bp->rhsp(), addr_द्वयङ्काः);
        प्रत्यागम r && r->varScopep() == idxVscp;
    }
    // Recognize a single-bit scan loop over all W bits of 'vec' (idx 0..W-1, target
    // pre-zeroed) and lower it to a bit-reduction primitive.  Two idioms are matched:
    //   target = 0; idx = 0;
    //   loop { looptest(W > idx); if (...vec[idx]...) target = <e>; idx = idx + 1; }
    // where, when W == width(vec):
    //   <e> = idx + 1     => target = $mostsetbitp1(vec)  (leading-one / bit-width)
    //   <e> = target + 1  => target = $countones(vec)     (population count)
    तार्किक try_अधस्तनम्_Bit_अवलोकनम्_Loop(AstLoop* loopp) {
        AstLoopTest* ध्रुव testp = VN_CAST(loopp->stmtsp(), LoopTest);
        यदि (!testp) प्रत्यागम असत्य;
        AstIf* ध्रुव ifp = VN_CAST(testp->nextp(), If);
        यदि (!ifp) प्रत्यागम असत्य;
        Ast_नियोजय* ध्रुव incp = VN_CAST(ifp->nextp(), नियोजय);
        यदि (!incp || incp->nextp()) प्रत्यागम असत्य;
        AstConst* wp = शून्यसूचकः;
        AstVarRef* idxRefp = शून्यसूचकः;
        यदि (!ascendingBound(testp->condp(), wp, idxRefp)) प्रत्यागम असत्य;
        AstVarScope* ध्रुव idxVscp = idxRefp->varScopep();
        ध्रुव uint32_t विस्तारः = wp->गन्तव्यम्_UInt();
        // Bits needed to address all W bits of 'vec' (clog2(W)); a narrower index is lossy.
        ध्रुव पूर्णाङ्क addr_द्वयङ्काः = विस्तारः <= 1 ? 1 : V3_सङ्ख्या::log2b(विस्तारः - 1) + 1;
        ध्रुव AstConst* ध्रुव idxInitp = m_bindings.get(idxVscp);
        यदि (!idxInitp || !idxInitp->isZero()) प्रत्यागम असत्य;
        AstVarRef* ध्रुव incLhsp = VN_CAST(incp->lhsp(), VarRef);
        यदि (!incLhsp || incLhsp->varScopep() != idxVscp) प्रत्यागम असत्य;
        यदि (!isVarPlus1(incp->rhsp(), idxVscp)) प्रत्यागम असत्य;
        यदि (ifp->elsesp()) प्रत्यागम असत्य;
        Ast_नियोजय* ध्रुव thenp = VN_CAST(ifp->thensp(), नियोजय);
        यदि (!thenp || thenp->nextp()) प्रत्यागम असत्य;
        AstVarRef* ध्रुव targetRefp = VN_CAST(thenp->lhsp(), VarRef);
        यदि (!targetRefp) प्रत्यागम असत्य;
        AstVarScope* ध्रुव targetVscp = targetRefp->varScopep();
        यदि (targetVscp == idxVscp) प्रत्यागम असत्य;
        ध्रुव तार्किक isLeadingOne = isVarPlus1(thenp->rhsp(), idxVscp);
        ध्रुव तार्किक is_गणनम्_Ones = !isLeadingOne && isVarPlus1(thenp->rhsp(), targetVscp);
        यदि (!isLeadingOne && !is_गणनम्_Ones) प्रत्यागम असत्य;
        // If-cond is the 1-bit select 'vec[idx]', possibly wrapped in the redundant in-range
        // guard Verilator auto-inserts (as 'guard && sel') for a non-power-of-two vector:
        //   '(idx <= W-1) && vec[idx]'        (default / --x-assign 0; a LogAnd), or
        //   '(idx <= W-1) ? vec[idx] : <x>'   (--x-assign unique; a Cond).
        // The guard is always true for idx in 0..W-1, so peel it to reach the select.  Any
        // other compound condition (e.g. 'vec[idx] && en') leaves a non-select, rejected below.
        Ast_ग्रन्थिः_व्यञ्जकम्* condp = ifp->condp();
        यदि (AstLogAnd* ध्रुव andp = VN_CAST(condp, LogAnd)) {
            यदि (is_अन्तर्_Range_Guard(andp->lhsp(), idxVscp, विस्तारः, addr_द्वयङ्काः)) condp = andp->rhsp();
        } अन्यथा यदि (AstCond* ध्रुव ternp = VN_CAST(condp, Cond)) {
            यदि (is_अन्तर्_Range_Guard(ternp->condp(), idxVscp, विस्तारः, addr_द्वयङ्काः)) condp = ternp->thenp();
        }
        AstSel* ध्रुव selp = VN_CAST(condp, Sel);
        यदि (!selp || selp->विस्तारः() != 1) प्रत्यागम असत्य;
        ध्रुव AstVarRef* ध्रुव fromp = VN_CAST(selp->fromp(), VarRef);
        यदि (!fromp) प्रत्यागम असत्य;
        ध्रुव AstVarScope* ध्रुव fromVscp = fromp->varScopep();
        यदि (fromVscp == idxVscp || fromVscp == targetVscp) प्रत्यागम असत्य;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव vecExprp = selp->fromp();
        // Must scan all W bits of 'vec', indexed by exactly 'idx' (address kept >= clog2(W),
        // so a lossy narrowing like vec[idx[2:0]] is rejected).
        यदि (static_cast<पूर्णाङ्क>(विस्तारः) != vecExprp->विस्तारः()) प्रत्यागम असत्य;
        ध्रुव AstVarRef* ध्रुव idx_अन्तर्_Sel = unwrap_गन्तव्यम्_Var_Ref(selp->lsbp(), addr_द्वयङ्काः);
        यदि (!idx_अन्तर्_Sel || idx_अन्तर्_Sel->varScopep() != idxVscp) प्रत्यागम असत्य;
        // 'target' must be const-0 immediately before the loop (collected in m_bindings),
        // so that an all-zero 'vec' yields 0, matching $mostsetbitp1's definition.
        ध्रुव AstConst* ध्रुव targetInitp = m_bindings.get(targetVscp);
        यदि (!targetInitp || !targetInitp->isZero()) प्रत्यागम असत्य;
        // Rewrite to 'target = <reduction>(vec); idx = W'.  The 'idx = W' store preserves the
        // loop's exit value, so this is sound even if idx is read afterwards (else DCE drops it).
        File_पङ्क्तिः* ध्रुव flp = loopp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* reducep;
        यदि (isLeadingOne) {
            reducep = नूतन Ast_Most_समुच्चयः_Bit_P1{flp, vecExprp->clone_वृक्षः(असत्य)};
        } अन्यथा {
            Ast_गणनम्_Ones* ध्रुव conep = नूतन Ast_गणनम्_Ones{flp, vecExprp->clone_वृक्षः(असत्य)};
            reducep = conep;
        }
        reducep = resize_गन्तव्यम्_विस्तारः(reducep, targetRefp);
        Ast_नियोजय* ध्रुव newp = नूतन Ast_नियोजय{flp, targetRefp->clone_वृक्षः(असत्य), reducep};
        newp->सङ्कलय_अग्र(नूतन Ast_नियोजय{flp, incLhsp->clone_वृक्षः(असत्य), wp->clone_वृक्षः(असत्य)});
        loopp->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(loopp), loopp);
        यदि (isLeadingOne) {
            UINFO(4, "Lowered priority-encoder loop to $mostsetbitp1: " << newp);
            ++m_stats.m_bit_अवलोकनम्_Lowered;
        } अन्यथा {
            UINFO(4, "Lowered count-set-bits loop to $countones: " << newp);
            ++m_stats.m_गणनम्_Ones_Lowered;
        }
        प्रत्यागम सत्य;
    }

    //दर्शय
    शून्य दर्शय(AstLoop* nodep) override {
        // Gather variable bindings from the preceding statements
        m_bindings.clear();
        प्रति (Ast_ग्रन्थिः *succp = nodep, *currp = nodep->backp(); सत्य;
             succp = currp, currp = currp->backp()) {
            // If the current statement is in higher list, proceed carefully
            यदि (currp->nextp() != succp) {
                // Jump block is OK, there is only one way to get here
                यदि (VN_IS(currp, Jump_खण्डम्)) अनुवर्त;
                // TODO: if?
                // Otherwise we dont' know the control flow, give up
                भङ्ग;
            }
            Ast_नियोजय* ध्रुव assignp = VN_CAST(currp, नियोजय);
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
            m_bindings.समुच्चयः(lhsp->varScopep(), valp);
        }

        // Recognize a bit counting loop and lower it to a builtin
        यदि (v3Global.opt.f_Bit_अवलोकनम्_Loops() && try_अधस्तनम्_Bit_अवलोकनम्_Loop(nodep)) प्रत्यागम;

        // Attempt to unroll this loop
        ध्रुव std::pair<Ast_ग्रन्थिः*, तार्किक> pair = UnrollOneVisitor::apply(m_stats, m_bindings, nodep);

        // If failed, carry on with nested loop
        यदि (!pair.second) {
            iterateChildren(nodep);
            प्रत्यागम;
        }

        // Otherwise replace the loop with the unrolled code - might be empty
        यदि (pair.first) {
            nodep->replaceWith(pair.first);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अन्यथा {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        }
        // Iteration continues with the unrolled body
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTOR
    स्पष्ट UnrollAllVisitor(AstNetlist* netlistp) { iterate(netlistp); }

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
