// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Multi-threaded MTask graph data hazard fixing
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
// The fine-grained graph from V3Order may contain data hazards which are not a problem for
// serial mode, but which would be a problem in parallel mode. There are two classes:
// unordered pairs of writes, and unordered write-read pairs. This transform adds edges here
// until no such unordered pair remains.
//
// ABOUT UNORDERED WRITE-WRITE PAIRS
//
//   The V3Order dependency graph treats these as unordered events:
//
//    a)  sig[15:8] = stuff;
//          ...
//    b)  sig[7:0]  = other_stuff;
//
//   They are writes to disjoint bits of the same signal. They can run in
//   either order, in serial mode, and the result will be the same.
//
//   The resulting C code for each of this isn't a pure write, it's actually an R-M-W
//   sequence:
//
//    a)  sig = (sig & 0xff)   | (0xff00 & (stuff << 8));
//          ...
//    b)  sig = (sig & 0xff00) | (0xff & other_stuff);
//
//   In serial mode, order doesn't matter as these run serially. In parallel mode,
//   they must be serialized to avoid a race.
//
//   This pass does not check if each write would involve an R-M-W, it just assumes that
//   it does. If this routine ever causes a drastic increase in critical path, it could be
//   optimized to make a better prediction (with all the risk that word implies!) about
//   whether a given write is likely to turn into an R-M-W.
//
// ABOUT UNORDERED WRITE-READ PAIRS
//
//   If we don't put unordered write-read pairs into some order at Verilation time, we risk
//   a runtime race.
//
//   These arise because the OrderGraph deliberately does not model every access. A read of
//   a variable that is in the reading block's own hybrid sensitivity list gets no edge, as
//   does a read ignored due to a force/release, or an access to a variable marked
//   'ignoreSchedWrite' and friends. For serial mode that is fine: whatever order the logic
//   ends up in, it runs one block at a time. In parallel mode two such blocks can run
//   concurrently, and if one of them writes what the other reads, that is an observable
//   data race.
//
// HOW TO FIX THEM
//
//   An arbitrary ordering is prescribed by adding edges between MTasks. The new edges
//   must not create a cycle, and should be added in a way that increases critical paths
//   as little as possible.
//
//   Every MTask is given a unique sequence number, in a topological order of the graph, so
//   that for every edge 'from -> to' we have seq(from) < seq(to). Adding a new edge that
//   runs in increasing sequence order therefore cannot create a cycle, and the property is
//   maintained by induction as more edges are added.
//
//   With that in hand, for each variable we take its accessors in sequence order, chain the
//   writers together, and bracket each reader between the writers either side of it.
//   Readers need no ordering with respect to one another.
//
//   The sequence numbers are assigned by a topological sort that emits the ready MTask with
//   the smallest forward critical path first. As the forward critical path of an MTask is at
//   least that of each of its predecessors, the smallest among the ready MTasks is also the
//   smallest among all not yet emitted ones, so MTasks come out in globally non-decreasing
//   forward critical path order. Chaining them in that order is what tends to grow the
//   critical path least: the longest path through a chain starts at the head of its first
//   MTask, so putting those with the shortest path to them first keeps that sum down.
//
//   The critical paths are also what makes finding the edges that are actually needed cheap.
//   An edge is only added between a pair that is not ordered already, and a pair can be
//   ruled ordered, or not, in constant time whenever their critical paths are inconsistent
//   with a path between them, which avoids searching the graph for most pairs.
//
//   SystemC variables are handled similarly, except that all SystemC variables are treated as
//   a single entity. Reasoning: writing a systemC var actually turns into a call to a var.write()
//   method, which under the hood is accessing some data structure that's shared by many SC vars.
//   It's not thread safe.
//
//   DPI calls are serialized similarly (they are assumed not thread safe), unless directed by
//   options.
//
//*************************************************************************

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Control.h"
#شامل "V3Global.h"
#شامل "V3Graph.h"
#شامل "V3OrderGraph.h"
#شامل "V3OrderMTaskGraph.h"

#شامل <algorithm>
#شامل <set>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// DpiImportCallVisitor

// Scan node, indicate whether it contains a call to a DPI imported routine.
جماعت DpiImportCallVisitor final : عوامی VNVisitor {
    منطقی m_hasDpiHazard = جھوٹ;  // Found a DPI import call.
    منطقی m_tracingCall = جھوٹ;  // Iterating into a CCall to a CFunc
    // METHODS
    خالی دورہ(AstCFunc* nodep) override {
        اگر (!m_tracingCall) واپس;
        m_tracingCall = جھوٹ;
        اگر (nodep->dpiImportWrapper()) {
            اگر (nodep->dpi_خالص() ? !v3Global.opt.دھاگے_Dpi_خالص()
                                 : !v3Global.opt.دھاگے_Dpi_Unpure()) {
                // If hierarchical DPI wrapper cost is not found or is of a 0 cost,
                // we have a normal DPI which induces DPI hazard by default.
                m_hasDpiHazard = V3Control::get_Profile_ڈیٹا(nodep->cname()) == 0;
                UINFO(9, "DPI wrapper '" << nodep->cname()
                                         << "' has dpi hazard = " << m_hasDpiHazard);
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        iterateChildren(nodep);
        // Enter the function and trace it
        m_tracingCall = سچ;
        iterate(nodep->funcp());
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    صریح DpiImportCallVisitor(Ast_گرہ* nodep) { iterate(nodep); }

عوامی:
    ساکن منطقی hasDpiHazard(Ast_گرہ* nodep) { واپس DpiImportCallVisitor{nodep}.m_hasDpiHazard; }
};

//######################################################################
// FixDataHazards

جماعت Fix_ڈیٹا_Hazards final {
    // TYPES

    // Access to one variable, with the MTask performing it
    ساخت Access final {
        ثابت AstVarScope* m_vscp;  // The variable accessed, its index is in 'user1'
        LogicMTask* m_mtaskp;  // The accessing MTask
        VAccess m_access;  // The kind of access
    };

    // NODE STATE
    //  AstVarScope::user1    -> int: Variable index for stable sorting
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // MEMBERS
    ترتیب_MTask_گراف& m_m_Task_گراف;  // The Mtask graph
    std::سمتیہ<LogicMTask*> m_scratch;  // Scratch MTask list, reused to avoid reallocation

    // METHODS

    // Assign each MTask a unique sequence number, held in 'user()', such that for every edge
    // 'from -> to' we have seq(from) < seq(to). See the note at top of file on why this matters.
    // This is a topological sort using Kahn's algorithm with MTasks enumerated in a globally
    // non-decreasing critical path order.
    خالی تفویض_Sequence_Numbers() {
        ساخت MTaskCmp final {
            منطقی عامل()(ثابت LogicMTask* ap, ثابت LogicMTask* bp) ثابت {
                // Order by critical path
                ثابت uint64_t aCp = ap->cpExclusive<گراف_Way::FORWARD>();
                ثابت uint64_t bCp = bp->cpExclusive<گراف_Way::FORWARD>();
                اگر (aCp != bCp) واپس aCp < bCp;
                // Break ties by stable id
                واپس *ap < *bp;
            }
        };
        // Set of ready vertices. Initialized to the entry MTask.
        std::سیٹ<LogicMTask*, MTaskCmp> ready{m_m_Task_گراف.entryp()};
        // 'user' also used to count remaining dependencies of each MTask. Initialize it.
        برائے (V3_گراف_راس& vtx : m_m_Task_گراف.vertices()) {
            vtx.user(static_cast<uint32_t>(vtx.اندر_کنارے().سائز()));
        }
        // Next sequence number to assign to each MTask.
        uint32_t seq = 0;
        // Process ready vertices in critical path order.
        جب (!ready.خالی_خانے()) {
            // Pick up and detach the ready MTask with the smallest critical path.
            ثابت خودکار it = ready.begin();
            LogicMTask* ثابت mtaskp = *it;
            ready.erase(it);
            // Assign the next sequence number to the MTask
            mtaskp->user(++seq);
            // Decrement edge count of successors and add to ready set if no dependencies left
            برائے (V3_گراف_کنارہ& کنارہ : mtaskp->برآمد_کنارے()) {
                LogicMTask* ثابت چوٹی = static_cast<LogicMTask*>(کنارہ.چوٹی());
                ثابت uint32_t nDeps = چوٹی->user() - 1;
                چوٹی->user(nDeps);
                اگر (!nDeps) ready.داخل_کرو(چوٹی);
            }
        }
        UASSERT(seq == m_m_Task_گراف.vertices().سائز(), "MTask graph is cyclic");
    }

    // Gather every variable access, resolved to the MTask performing it.
    std::سمتیہ<Access> gatherAccesses() {
        std::سمتیہ<Access> accesses;
        صحیح nVars = 0;
        برائے (V3_گراف_راس& vtx : m_m_Task_گراف.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            برائے (ثابت ترتیب_Move_راس& mVtx : mtask.راس_List()) {
                ثابت ترتیب_Logic_راس* ثابت lVtxp = mVtx.logicp();
                اگر (!lVtxp) جاری;
                برائے (ثابت ترتیب_Logic_راس::VarAccess& acc : lVtxp->varAccesses()) {
                    AstVarScope* ثابت vscp = acc.m_vscp;
                    اگر (!vscp->user1()) vscp->user1(++nVars);
                    accesses.ڈالو_عقب({vscp, &mtask, acc.m_access});
                }
            }
        }
        واپس accesses;
    }

    // Add an edge ordering 'fromp' before 'top', unless they are already ordered
    خالی جوڑو_کنارہ_If_Needed(LogicMTask* fromp, LogicMTask* چوٹی) {
        // Nothing to order within a single MTask
        اگر (fromp == چوٹی) واپس;
        UASSERT_OBJ(fromp->user() < چوٹی->user(), fromp,
                    "Edge must run in increasing sequence order");
        // Already directly ordered. This is an O(1) set lookup, and catches the common case of
        // a dependency the OrderGraph already provided.
        اگر (fromp->has_کنارہ_مقصد(چوٹی)) واپس;
        // Otherwise check if already ordered
        اگر (m_m_Task_گراف.راستہ_Exists(fromp, چوٹی, خالی_اشاریہ)) واپس;
        // Unordered. Add an edge between them.
        m_m_Task_گراف.جوڑو_کنارہ(fromp, چوٹی);
    }

    // Add the edges required to make the accesses of one variable race free. 'beginp'/'endp'
    // delimit the accesses of a single variable, in sequence number order.
    خالی serializeVariable(ثابت Access* beginp, ثابت Access* endp) {
        // Nothing to order if at most one MTask accesses this variable. Each MTask appears at
        // most once in the range, see the assertion below, so this is just the range size.
        اگر (endp - beginp < 2) واپس;

        std::سمتیہ<LogicMTask*>& pendingReaders = m_scratch;
        pendingReaders.clear();
        LogicMTask* پچھلا_Writerp = خالی_اشاریہ;
        برائے (ثابت Access* accp = beginp; accp != endp; ++accp) {
            // The OrderLogicVertices record one access per variable, and at this point each MTask
            // holds exactly one logic block, so an MTask cannot appear twice here.
            UASSERT_OBJ(accp == beginp || accp[-1].m_mtaskp != accp->m_mtaskp, accp->m_mtaskp,
                        "Multiple accesses of a variable in an MTask");
            LogicMTask* ثابت mtaskp = accp->m_mtaskp;
            اگر (accp->m_access.isWriteOrRW()) {
                // Reads since the previous write must complete before this write
                برائے (LogicMTask* ثابت readerp : pendingReaders) جوڑو_کنارہ_If_Needed(readerp, mtaskp);
                // Consecutive writes must be ordered. If a read came between them, the edges to
                // and from that read imply it already.
                اگر (پچھلا_Writerp && pendingReaders.خالی_خانے()) جوڑو_کنارہ_If_Needed(پچھلا_Writerp, mtaskp);
                pendingReaders.clear();
                پچھلا_Writerp = mtaskp;
            } ورنہ {
                // This read must happen after the preceding write
                اگر (پچھلا_Writerp) جوڑو_کنارہ_If_Needed(پچھلا_Writerp, mtaskp);
                pendingReaders.ڈالو_عقب(mtaskp);
            }
        }
    }

    // Serialize the given MTasks, in sequence number order
    خالی serializeMTasks(std::سمتیہ<LogicMTask*>& mtaskps) {
        std::ترتیب_دو(mtaskps.begin(), mtaskps.انتہا(), [](ثابت LogicMTask* ap, ثابت LogicMTask* bp) {
            واپس ap->user() < bp->user();
        });
        mtaskps.erase(std::unique(mtaskps.begin(), mtaskps.انتہا()), mtaskps.انتہا());
        برائے (سائز_t i = 1; i < mtaskps.سائز(); ++i) جوڑو_کنارہ_If_Needed(mtaskps[i - 1], mtaskps[i]);
    }

    منطقی hasDpiHazard(LogicMTask* mtaskp) {
        برائے (ثابت ترتیب_Move_راس& mVtx : mtaskp->راس_List()) {
            ترتیب_Logic_راس* ثابت lvtxp = mVtx.logicp();
            اگر (!lvtxp) جاری;
            // NOTE: We don't handle DPI exports. If testbench code calls a DPI-exported function
            // at any time during eval() we may have a data hazard. (Likewise in non-threaded mode
            // if an export messes with an ordered variable we're broken.)

            // Find all calls to DPI-imported functions, we can put those into a serial order at
            // least. That should solve the most likely DPI-related data hazards.
            اگر (DpiImportCallVisitor::hasDpiHazard(lvtxp->nodep())) واپس سچ;
        }
        واپس جھوٹ;
    }

    // CONSTRUCTOR
    Fix_ڈیٹا_Hazards(ترتیب_MTask_گراف& m_Task_گراف)
        : m_m_Task_گراف{m_Task_گراف} {
        // Give the MTasks a total order consistent with their dependencies
        تفویض_Sequence_Numbers();

        // Gather variable accesses made by each MTask
        std::سمتیہ<Access> accesses = gatherAccesses();
        // Sort by variable (to group by variable), then by sequence number of the accessing MTask
        std::ترتیب_دو(accesses.begin(), accesses.انتہا(), [](ثابت Access& a, ثابت Access& b) {
            اگر (a.m_vscp != b.m_vscp) واپس a.m_vscp->user1() < b.m_vscp->user1();
            واپس a.m_mtaskp->user() < b.m_mtaskp->user();
        });

        // Serialize the accesses of each variable
        برائے (سائز_t i = 0; i < accesses.سائز();) {
            سائز_t انتہا = i + 1;
            جب (انتہا < accesses.سائز() && accesses[انتہا].m_vscp == accesses[i].m_vscp) ++انتہا;
            serializeVariable(accesses.ڈیٹا() + i, accesses.ڈیٹا() + انتہا);
            i = انتہا;
        }

        // Serialize all writes to SystemC vars. Note the reads of an individual SC var are already
        // ordered against its writes by the per variable pass above, it is only the writes between
        // different SC vars that need this extra serialization. Only top level ports are SC vars,
        // so this should not hurt performance too much.
        {
            m_scratch.clear();
            برائے (ثابت Access& access : accesses) {
                اگر (!access.m_access.isWriteOrRW()) جاری;
                اگر (!access.m_vscp->varp()->isSc()) جاری;
                m_scratch.ڈالو_عقب(access.m_mtaskp);
            }
            serializeMTasks(m_scratch);
        }

        // Serialize DPI calls unless user gave '--threads-dpi none'.
        اگر (!v3Global.opt.دھاگے_Dpi_خالص() || !v3Global.opt.دھاگے_Dpi_Unpure()) {
            m_scratch.clear();
            برائے (V3_گراف_راس& vtx : m_m_Task_گراف.vertices()) {
                LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
                اگر (hasDpiHazard(&mtask)) m_scratch.ڈالو_عقب(&mtask);
            }
            serializeMTasks(m_scratch);
        }
    }

عوامی:
    ساکن خالی apply(ترتیب_MTask_گراف& m_Task_گراف) { Fix_ڈیٹا_Hazards{m_Task_گراف}; }
};

خالی ترتیب_MTask_گراف::fix_ڈیٹا_Hazards(ترتیب_MTask_گراف& mtask_گراف) {
    Fix_ڈیٹا_Hazards::apply(mtask_گراف);
    // The critical paths are maintained as the graph is mutated, check them
    mtask_گراف.validate();
}
