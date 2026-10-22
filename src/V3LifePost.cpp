// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: NBA shadow variable assignment elimination
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
// Given a pair of variables 'd' and 'q', where 'd' is the shadow variable
// created by V3Delayed using the ShadowVar scheme. Attemp to turn this code:
//
//      ... reads of 'q' ok here
//  X1:  d = q; // First and complete write of 'd' (the pre-scheduled NBA initial assignment)
//      ... reads of 'q' ok here
//       d = foo; // Second assignment to 'd'
//      ... no reads of 'q' here
//  X2:  q = d; // Only and complete write of 'q', only read of 'd' (the post-scheduled NBA commit)
//      ... reads of 'q' ok here
//
// into:
//
//  X1:  q = q;
//       ...
//       q = foo;
//       ...
//  X2:  q = q;
//
// by replacing 'd' with 'q'. This then allows deletion of 'd' and the two assignments.
//
// More formally, with the non-sequential mtasks graph, we must prove all of these:
//  1) No reads of 'd' anywhere except for the ASSIGNPOST itself
//  2) No write of 'q' anywhere except for the ASSIGNPOST itself
//  3) The first write of 'd' is complete (writes all bits)
//  4) Every read  of 'q' either falls before the second write of 'd', or after only read of 'd'
//
// Notes:
//
// While these rules could be applied to any variables, not just the NBA
// shadow variables. **Proving** that no reads of 'q' happen after the second
// assignment of 'd' is difficult due to the presence of loops (the whole
// eval_nba is inside a loop), virtual methods and other dynamic executions.
// For the NBA shadow variables, we can compute this safely as their use
// is understood as we schedule their first and last assignments specially.
//
// Constraint 2 could be relaxed to "no write of 'q' before the only read of
// 'd', however we only have one write of 'q', created in V3Delayed, so
// trying harder would just be a code coverage hole today.
//
// Constraint 3 should always hold with V3Delayed, will check assert it.
//
// If old-value reads prevent elimination, constant partial writes can still narrow both
// copies to the words containing those writes. Other shadow bits are never read. The
// assignments keep their original positions, so evaluation order and scheduling are unchanged.
//
//*************************************************************************

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LifePost.h"

#അന്തര്ഭാവ "V3ExecGraph.h"
#അന്തര്ഭാവ "V3GraphPathChecker.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LifePost delay elimination

വർഗ്ഗം LifePostDlyVisitor final : പൊതു VNVisitorConst {
    // TYPES

    // Location of AstNode within the program
    template <typename T_കണ്ണി>
    വർഗ്ഗം Location final {
        template <typename U_കണ്ണി>
        friend വർഗ്ഗം Location;

        T_കണ്ണി* m_nodep;  // The AstNode being recorded
        സ്ഥിരം AstExecGraph* m_egraphp;  // AstExecTraph location is under, if any
        സ്ഥിരം ExecMTask* m_mtaskp;  // The ExecMTask location is under, if any
        uint32_t m_seq_അക്കം;  // Location counter

    പൊതു:
        Location(T_കണ്ണി* nodep, സ്ഥിരം AstExecGraph* egp, സ്ഥിരം ExecMTask* mtaskp, uint32_t seq_അക്കം)
            : m_nodep{nodep}
            , m_egraphp{egp}
            , m_mtaskp{mtaskp}
            , m_seq_അക്കം{seq_അക്കം} {}
        Location() = delete;

        T_കണ്ണി* സ്ഥിരം& nodep() സ്ഥിരം { തിരികെ m_nodep; }

        // "is before" - Note: Equality (concurrency) is possible iff they are independent mtasks!
        template <typename U_കണ്ണി>
        തർക്കം operator<(സ്ഥിരം Location<U_കണ്ണി>& that) സ്ഥിരം {
            // If they are in different mtasks under the same graph, check for a path in the graph
            എങ്കിൽ (m_egraphp && m_egraphp == that.m_egraphp && m_mtaskp != that.m_mtaskp) {
                GraphPathChecker* സ്ഥിരം checkerp = m_egraphp->user1u().to<GraphPathChecker*>();
                തിരികെ checkerp->pathExistsFrom(m_mtaskp, that.m_mtaskp);
            }
            // Otherwise the sequence numbers work (one/both outside graph, or both in same mtask)
            തിരികെ m_seq_അക്കം < that.m_seq_അക്കം;
        }
    };

    // NODE STATE
    // AstVarScope::user1()    -> bool: referenced outside _eval__nba
    // AstVarScope::user4p()   -> AstVarScope*: Replacement variable
    // AstExecGraph::user1p()  -> GraphPathChecker*: path checker for this AstExecGraph
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser4InUse m_inuser4;

    // STATE
    uint32_t m_sequence = 0;  // Sequence number of assigns/varrefs,
    സ്ഥിരം AstExecGraph* m_execGraphp = nullptr;  // Current AstExecGraph being processed (or null)
    സ്ഥിരം ExecMTask* m_execMTaskp = nullptr;  // Current ExecMTask being processed (or null)
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_stat_വാക്കുകൾ_Saved;  // Words removed from NBA shadow copies
    // Maps from Varscope to all their reads and writes
    using LocMap = std::unordered_map<സ്ഥിരം AstVarScope*, std::vector<Location<AstVarRef>>>;
    LocMap m_reads;  // VarScope read locations
    LocMap m_writes;  // VarScope write locations
    std::vector<Location<Ast_കണ്ണി_Assign>> m_assigns;  // Assignments considered for removal
    std::vector<std::unique_ptr<GraphPathChecker>> m_checkers;  // Storage for exec graph checkers
    സ്ഥിരം AstCFunc* സ്ഥിരം m_nbaFuncp;  // The 'nba' region entry point
    തർക്കം m_inEvalNba = അസത്യം;  // Traversing under the 'nba' region entry point

    // METHODS
    ശൂന്യം narrowCopies(Ast_കണ്ണി_Assign* postp, AstVarScope* dVscp,
                      സ്ഥിരം std::vector<Location<AstVarRef>>& writes) {
        എങ്കിൽ (!dVscp->isWide()) തിരികെ;
        Ast_കണ്ണി_Assign* സ്ഥിരം prep = VN_AS(writes[0].nodep()->backp(), കണ്ണി_Assign);
        UASSERT_OBJ(VN_AS(prep->lhsp(), VarRef)->varScopep()
                            == VN_AS(postp->rhsp(), VarRef)->varScopep()
                        && VN_AS(prep->rhsp(), VarRef)->varScopep()
                               == VN_AS(postp->lhsp(), VarRef)->varScopep(),
                    prep, "NBA shadow pre/post assignments are not reverse copies");

        പൂർണ്ണസംഖ്യ lsb = dVscp->width();
        പൂർണ്ണസംഖ്യ end = 0;
        വേണ്ടി (size_t i = 1; i < writes.size(); ++i) {
            സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(writes[i].nodep()->backp(), Sel);
            എങ്കിൽ (!selp || !VN_IS(selp->lsbp(), Const)) തിരികെ;
            സ്ഥിരം പൂർണ്ണസംഖ്യ start = selp->lsbConst();
            എങ്കിൽ (start > dVscp->width() - selp->width()) തിരികെ;
            lsb = std::min(lsb, start);
            end = std::max(end, start + selp->width());
        }
        // Copy one word-aligned range enclosing all writes, including any gaps.
        lsb = VL_BITWORD_E(lsb) * VL_EDATASIZE;
        end = std::min(VL_വാക്കുകൾ_I(end) * VL_EDATASIZE, dVscp->width());
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = end - lsb;
        എങ്കിൽ (width == dVscp->width()) തിരികെ;
        വേണ്ടി (Ast_കണ്ണി_Assign* സ്ഥിരം assignp : {prep, postp}) {
            File_വരി* സ്ഥിരം flp = assignp->fileline();
            assignp->lhsp(പുതിയ AstSel{flp, assignp->lhsp()->unlinkFrBack(), lsb, width});
            assignp->rhsp(പുതിയ AstSel{flp, assignp->rhsp()->unlinkFrBack(), lsb, width});
            assignp->dtypeFrom(assignp->lhsp());
        }
        m_stat_വാക്കുകൾ_Saved += 2 * (dVscp->width_വാക്കുകൾ() - VL_വാക്കുകൾ_I(width));
    }

    ശൂന്യം squashAssignposts() {
        വേണ്ടി (സ്ഥിരം Location<Ast_കണ്ണി_Assign>& assign : m_assigns) {
            AstVarScope* സ്ഥിരം dVscp = VN_AS(assign.nodep()->rhsp(), VarRef)->varScopep();
            AstVarScope* സ്ഥിരം qVscp = VN_AS(assign.nodep()->lhsp(), VarRef)->varScopep();

            // We are considering deleting 'y', don't do it if referenced external to _eval__nba
            എങ്കിൽ (dVscp->user1()) തുടരുക;

            സ്ഥിരം std::vector<Location<AstVarRef>>& dWrites = m_writes[dVscp];
            UASSERT_OBJ(!dWrites.empty(), dVscp, "NBA shadow variable read but never written");

            // *** See file header for requirements ***

            // Proof (1) - Only read is on the RHS of this assignment
            എങ്കിൽ (m_reads[dVscp].size() > 1) തുടരുക;

            // Proof (2) - Only write is on the LHS of this assignment
            എങ്കിൽ (m_writes[qVscp].size() > 1) തുടരുക;

            // Proof (3) - Should always hold
            UASSERT_OBJ(VN_IS(dWrites.at(0).nodep()->backp(), കണ്ണി_Assign), dVscp,
                        "Partial first write to NBA shadow variable");

            // Proof (4)
            എങ്കിൽ (dWrites.size() > 1) {
                // V3Order always serializes writes so they cannot be concurrent
                UASSERT_OBJ(dWrites[0] < dWrites[1], dVscp, "Concurrent writes");
                സ്ഥിരം തർക്കം qRdOK = [&]() {
                    വേണ്ടി (സ്ഥിരം Location<AstVarRef>& qRead : m_reads[qVscp]) {
                        എങ്കിൽ (assign < qRead) തുടരുക;
                        // Check from 2nd write of 'd'
                        വേണ്ടി (size_t i = 1; i < dWrites.size(); ++i) {
                            എങ്കിൽ (qRead < dWrites[i]) തുടരുക;
                            തിരികെ അസത്യം;
                        }
                    }
                    തിരികെ സത്യം;
                }();
                എങ്കിൽ (!qRdOK) {
                    narrowCopies(assign.nodep(), dVscp, dWrites);
                    തുടരുക;
                }
            }

            // Mark variable for replacement
            dVscp->user4p(qVscp);
            // Delete assignment
            UINFO(4, "    DELETE " << assign.nodep());
            VL_DO_DANGLING(assign.nodep()->unlinkFrBack()->delete_വൃക്ഷം(), assign.nodep());
            ++m_statAssnDel;
        }
    }

    // Trace code in the given function
    ശൂന്യം trace(AstCFunc* nodep) {
        VL_RESTORER(m_inEvalNba);
        എങ്കിൽ (nodep == m_nbaFuncp) m_inEvalNba = സത്യം;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        // First, build maps of every location (mtask and sequence
        // within the mtask) where each varscope is read, and written.
        iterateChildrenConst(nodep);

        // We need to be able to pick up the first write of each variable.
        // V3Order serializes all writes, and we trace AstExecGraph in
        // dependency order, so the first one we encounter during tracing should
        // always be the one. It's somewhat expensive to assert so only with debugCheck().
        എങ്കിൽ (v3Global.opt.debugCheck()) {
            വേണ്ടി (auto& pair : m_writes) {
                സ്ഥിരം std::vector<Location<AstVarRef>>& writes = pair.second;
                സ്ഥിരം Location<AstVarRef>& first = writes[0];
                വേണ്ടി (size_t i = 1; i < writes.size(); ++i) {
                    UASSERT_OBJ(first < writes[i], pair.first, "First write is not the first");
                }
            }
        }

        // Find all assignposts. Determine which ones can be
        // eliminated. Remove those, and mark their dly vars' user4 field
        // to indicate we should replace these dly vars with their original
        // variables.
        squashAssignposts();

        // Apply replacements
        nodep->foreach([](AstVarRef* nodep) {
            സ്ഥിരം AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
            AstVarScope* സ്ഥിരം replacementp = VN_AS(vscp->user4p(), VarScope);
            എങ്കിൽ (!replacementp) തിരികെ;
            UINFO(9, "  Replace " << nodep << " target " << vscp << " with " << replacementp);
            nodep->varScopep(replacementp);
            nodep->varp(replacementp->varp());
        });
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        // We only try to optimize NBA shadow variables
        എങ്കിൽ (!nodep->varScopep()->optimizeLifePost()) തിരികെ;

        // Mark variables referenced outside the 'nba' region
        എങ്കിൽ (!m_inEvalNba) {
            nodep->varScopep()->user1(സത്യം);
            തിരികെ;
        }

        // Consumption/generation of a variable,
        സ്ഥിരം AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");

        ++m_sequence;
        എങ്കിൽ (nodep->access().isWriteOrRW()) {
            m_writes[vscp].emplace_back(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
        എങ്കിൽ (nodep->access().isReadOrRW()) {
            m_reads[vscp].emplace_back(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        // Record RHS before assignment
        iterateConst(nodep->rhsp());
        // If a straight assignment between NBA variables, consider for removal
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                എങ്കിൽ (lhsp->varScopep()->optimizeLifePost()  //
                    && rhsp->varScopep()->optimizeLifePost()) {
                    m_assigns.emplace_back(nodep, m_execGraphp, m_execMTaskp, ++m_sequence);
                }
            }
        }
        // Record LHS after assignment
        iterateConst(nodep->lhsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // Entry points are roots of the trace, no need to do it here
        എങ്കിൽ (nodep->funcp()->entryPoint()) തിരികെ;
        // Trace cellee
        trace(nodep->funcp());
    }
    ശൂന്യം visit(AstExecGraph* nodep) override {
        UASSERT_OBJ(!m_execGraphp, nodep, "Nested AstExecGraph");
        VL_RESTORER(m_execGraphp);
        m_execGraphp = nodep;

        // Set up the path checker for this graph
        UASSERT_OBJ(!nodep->user1p(), nodep, "AstExecGraph visited twice");
        m_checkers.emplace_back(പുതിയ GraphPathChecker{nodep->depGraphp()});
        nodep->user1p(m_checkers.back().get());

        // Trace each mtask body. Note: the vertices are in topological order,
        // and we do not reset m_sequence, so a lower sequence number does
        // guarantee a node is not earlier than a higher sequence number, but
        // might still be concurrent.
        വേണ്ടി (V3GraphVertex& mtaskVtx : nodep->depGraphp()->vertices()) {
            സ്ഥിരം ExecMTask* സ്ഥിരം mtaskp = mtaskVtx.as<ExecMTask>();
            VL_RESTORER(m_execMTaskp);
            m_execMTaskp = mtaskp;
            trace(mtaskp->funcp());
        }
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        // Start a trace from each entry point
        എങ്കിൽ (nodep->entryPoint()) trace(nodep);
    }
    //-----
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit LifePostDlyVisitor(AstNetlist* netlistp)
        : m_nbaFuncp{netlistp->evalFuncp(VEval::NBA)} {
        iterateConst(netlistp);
    }
    ~LifePostDlyVisitor() override {
        V3Stats::addStat("Optimizations, Lifetime postassign deletions", m_statAssnDel);
        V3Stats::addStat("Optimizations, Lifetime NBA copy words removed", m_stat_വാക്കുകൾ_Saved);
    }
};

//######################################################################
// LifePost class functions

ശൂന്യം V3LifePost::lifepostAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LifePostDlyVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("life_post", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
