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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderMTaskGraph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// DpiImportCallVisitor

// Scan node, indicate whether it contains a call to a DPI imported routine.
ਵਰਗ DpiImportCallVisitor final : ਜਨਤਕ VNVisitor {
    ਤਾਰਕਿਕ m_hasDpiHazard = ਝੂਠ;  // Found a DPI import call.
    ਤਾਰਕਿਕ m_tracingCall = ਝੂਠ;  // Iterating into a CCall to a CFunc
    // METHODS
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        ਜੇ (!m_tracingCall) ਵਾਪਸ;
        m_tracingCall = ਝੂਠ;
        ਜੇ (nodep->dpiImportWrapper()) {
            ਜੇ (nodep->dpi_ਸ਼ੁੱਧ() ? !v3Global.opt.ਧਾਗੇ_Dpi_ਸ਼ੁੱਧ()
                                 : !v3Global.opt.ਧਾਗੇ_Dpi_Unpure()) {
                // If hierarchical DPI wrapper cost is not found or is of a 0 cost,
                // we have a normal DPI which induces DPI hazard by default.
                m_hasDpiHazard = V3Control::get_Profile_ਅੰਕੜੇ(nodep->cname()) == 0;
                UINFO(9, "DPI wrapper '" << nodep->cname()
                                         << "' has dpi hazard = " << m_hasDpiHazard);
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        iterateChildren(nodep);
        // Enter the function and trace it
        m_tracingCall = ਸੱਚ;
        iterate(nodep->funcp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ DpiImportCallVisitor(Ast_ਗੰਢ* nodep) { iterate(nodep); }

ਜਨਤਕ:
    ਸਥਿਰ ਤਾਰਕਿਕ hasDpiHazard(Ast_ਗੰਢ* nodep) { ਵਾਪਸ DpiImportCallVisitor{nodep}.m_hasDpiHazard; }
};

//######################################################################
// FixDataHazards

ਵਰਗ Fix_ਅੰਕੜੇ_Hazards final {
    // TYPES

    // Access to one variable, with the MTask performing it
    ਬਣਤਰ Access final {
        ਅਚਲ AstVarScope* m_vscp;  // The variable accessed, its index is in 'user1'
        LogicMTask* m_mtaskp;  // The accessing MTask
        VAccess m_access;  // The kind of access
    };

    // NODE STATE
    //  AstVarScope::user1    -> int: Variable index for stable sorting
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // MEMBERS
    ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& m_m_Task_ਗ੍ਰਾਫ;  // The Mtask graph
    std::ਸਦਿਸ਼<LogicMTask*> m_scratch;  // Scratch MTask list, reused to avoid reallocation

    // METHODS

    // Assign each MTask a unique sequence number, held in 'user()', such that for every edge
    // 'from -> to' we have seq(from) < seq(to). See the note at top of file on why this matters.
    // This is a topological sort using Kahn's algorithm with MTasks enumerated in a globally
    // non-decreasing critical path order.
    ਖਾਲੀ ਸੌਂਪੋ_Sequence_Numbers() {
        ਬਣਤਰ MTaskCmp final {
            ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ LogicMTask* ap, ਅਚਲ LogicMTask* bp) ਅਚਲ {
                // Order by critical path
                ਅਚਲ uint64_t aCp = ap->cpExclusive<ਗ੍ਰਾਫ_Way::FORWARD>();
                ਅਚਲ uint64_t bCp = bp->cpExclusive<ਗ੍ਰਾਫ_Way::FORWARD>();
                ਜੇ (aCp != bCp) ਵਾਪਸ aCp < bCp;
                // Break ties by stable id
                ਵਾਪਸ *ap < *bp;
            }
        };
        // Set of ready vertices. Initialized to the entry MTask.
        std::ਸੈੱਟ<LogicMTask*, MTaskCmp> ready{m_m_Task_ਗ੍ਰਾਫ.entryp()};
        // 'user' also used to count remaining dependencies of each MTask. Initialize it.
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_m_Task_ਗ੍ਰਾਫ.vertices()) {
            vtx.user(static_cast<uint32_t>(vtx.ਵਿੱਚ_ਕਿਨਾਰੇ().ਮਾਪ()));
        }
        // Next sequence number to assign to each MTask.
        uint32_t seq = 0;
        // Process ready vertices in critical path order.
        ਜਦੋਂ (!ready.ਖਾਲੀ_ਥਾਵਾਂ()) {
            // Pick up and detach the ready MTask with the smallest critical path.
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = ready.begin();
            LogicMTask* ਅਚਲ mtaskp = *it;
            ready.erase(it);
            // Assign the next sequence number to the MTask
            mtaskp->user(++seq);
            // Decrement edge count of successors and add to ready set if no dependencies left
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtaskp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                LogicMTask* ਅਚਲ ਸਿਖਰ = static_cast<LogicMTask*>(ਕਿਨਾਰਾ.ਸਿਖਰ());
                ਅਚਲ uint32_t nDeps = ਸਿਖਰ->user() - 1;
                ਸਿਖਰ->user(nDeps);
                ਜੇ (!nDeps) ready.ਪਾਓ(ਸਿਖਰ);
            }
        }
        UASSERT(seq == m_m_Task_ਗ੍ਰਾਫ.vertices().ਮਾਪ(), "MTask graph is cyclic");
    }

    // Gather every variable access, resolved to the MTask performing it.
    std::ਸਦਿਸ਼<Access> gatherAccesses() {
        std::ਸਦਿਸ਼<Access> accesses;
        ਪੂਰਨ nVars = 0;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_m_Task_ਗ੍ਰਾਫ.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            ਲਈ (ਅਚਲ ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼& mVtx : mtask.ਸ਼ੀਰਸ਼_List()) {
                ਅਚਲ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp = mVtx.logicp();
                ਜੇ (!lVtxp) ਜਾਰੀ;
                ਲਈ (ਅਚਲ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼::VarAccess& acc : lVtxp->varAccesses()) {
                    AstVarScope* ਅਚਲ vscp = acc.m_vscp;
                    ਜੇ (!vscp->user1()) vscp->user1(++nVars);
                    accesses.ਧੱਕੋ_ਪਿੱਠ({vscp, &mtask, acc.m_access});
                }
            }
        }
        ਵਾਪਸ accesses;
    }

    // Add an edge ordering 'fromp' before 'top', unless they are already ordered
    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ_If_Needed(LogicMTask* fromp, LogicMTask* ਸਿਖਰ) {
        // Nothing to order within a single MTask
        ਜੇ (fromp == ਸਿਖਰ) ਵਾਪਸ;
        UASSERT_OBJ(fromp->user() < ਸਿਖਰ->user(), fromp,
                    "Edge must run in increasing sequence order");
        // Already directly ordered. This is an O(1) set lookup, and catches the common case of
        // a dependency the OrderGraph already provided.
        ਜੇ (fromp->has_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ(ਸਿਖਰ)) ਵਾਪਸ;
        // Otherwise check if already ordered
        ਜੇ (m_m_Task_ਗ੍ਰਾਫ.ਰਸਤਾ_Exists(fromp, ਸਿਖਰ, ਖਾਲੀ_ਸੂਚਕ)) ਵਾਪਸ;
        // Unordered. Add an edge between them.
        m_m_Task_ਗ੍ਰਾਫ.ਜੋੜੋ_ਕਿਨਾਰਾ(fromp, ਸਿਖਰ);
    }

    // Add the edges required to make the accesses of one variable race free. 'beginp'/'endp'
    // delimit the accesses of a single variable, in sequence number order.
    ਖਾਲੀ serializeVariable(ਅਚਲ Access* beginp, ਅਚਲ Access* endp) {
        // Nothing to order if at most one MTask accesses this variable. Each MTask appears at
        // most once in the range, see the assertion below, so this is just the range size.
        ਜੇ (endp - beginp < 2) ਵਾਪਸ;

        std::ਸਦਿਸ਼<LogicMTask*>& pendingReaders = m_scratch;
        pendingReaders.clear();
        LogicMTask* prevWriterp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ Access* accp = beginp; accp != endp; ++accp) {
            // The OrderLogicVertices record one access per variable, and at this point each MTask
            // holds exactly one logic block, so an MTask cannot appear twice here.
            UASSERT_OBJ(accp == beginp || accp[-1].m_mtaskp != accp->m_mtaskp, accp->m_mtaskp,
                        "Multiple accesses of a variable in an MTask");
            LogicMTask* ਅਚਲ mtaskp = accp->m_mtaskp;
            ਜੇ (accp->m_access.isWriteOrRW()) {
                // Reads since the previous write must complete before this write
                ਲਈ (LogicMTask* ਅਚਲ readerp : pendingReaders) ਜੋੜੋ_ਕਿਨਾਰਾ_If_Needed(readerp, mtaskp);
                // Consecutive writes must be ordered. If a read came between them, the edges to
                // and from that read imply it already.
                ਜੇ (prevWriterp && pendingReaders.ਖਾਲੀ_ਥਾਵਾਂ()) ਜੋੜੋ_ਕਿਨਾਰਾ_If_Needed(prevWriterp, mtaskp);
                pendingReaders.clear();
                prevWriterp = mtaskp;
            } ਨਹੀਂਤਾਂ {
                // This read must happen after the preceding write
                ਜੇ (prevWriterp) ਜੋੜੋ_ਕਿਨਾਰਾ_If_Needed(prevWriterp, mtaskp);
                pendingReaders.ਧੱਕੋ_ਪਿੱਠ(mtaskp);
            }
        }
    }

    // Serialize the given MTasks, in sequence number order
    ਖਾਲੀ serializeMTasks(std::ਸਦਿਸ਼<LogicMTask*>& mtaskps) {
        std::ਛਾਂਟੋ(mtaskps.begin(), mtaskps.ਸਿਰਾ(), [](ਅਚਲ LogicMTask* ap, ਅਚਲ LogicMTask* bp) {
            ਵਾਪਸ ap->user() < bp->user();
        });
        mtaskps.erase(std::unique(mtaskps.begin(), mtaskps.ਸਿਰਾ()), mtaskps.ਸਿਰਾ());
        ਲਈ (ਮਾਪ_t i = 1; i < mtaskps.ਮਾਪ(); ++i) ਜੋੜੋ_ਕਿਨਾਰਾ_If_Needed(mtaskps[i - 1], mtaskps[i]);
    }

    ਤਾਰਕਿਕ hasDpiHazard(LogicMTask* mtaskp) {
        ਲਈ (ਅਚਲ ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼& mVtx : mtaskp->ਸ਼ੀਰਸ਼_List()) {
            ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp = mVtx.logicp();
            ਜੇ (!lvtxp) ਜਾਰੀ;
            // NOTE: We don't handle DPI exports. If testbench code calls a DPI-exported function
            // at any time during eval() we may have a data hazard. (Likewise in non-threaded mode
            // if an export messes with an ordered variable we're broken.)

            // Find all calls to DPI-imported functions, we can put those into a serial order at
            // least. That should solve the most likely DPI-related data hazards.
            ਜੇ (DpiImportCallVisitor::hasDpiHazard(lvtxp->nodep())) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // CONSTRUCTOR
    Fix_ਅੰਕੜੇ_Hazards(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& m_Task_ਗ੍ਰਾਫ)
        : m_m_Task_ਗ੍ਰਾਫ{m_Task_ਗ੍ਰਾਫ} {
        // Give the MTasks a total order consistent with their dependencies
        ਸੌਂਪੋ_Sequence_Numbers();

        // Gather variable accesses made by each MTask
        std::ਸਦਿਸ਼<Access> accesses = gatherAccesses();
        // Sort by variable (to group by variable), then by sequence number of the accessing MTask
        std::ਛਾਂਟੋ(accesses.begin(), accesses.ਸਿਰਾ(), [](ਅਚਲ Access& a, ਅਚਲ Access& b) {
            ਜੇ (a.m_vscp != b.m_vscp) ਵਾਪਸ a.m_vscp->user1() < b.m_vscp->user1();
            ਵਾਪਸ a.m_mtaskp->user() < b.m_mtaskp->user();
        });

        // Serialize the accesses of each variable
        ਲਈ (ਮਾਪ_t i = 0; i < accesses.ਮਾਪ();) {
            ਮਾਪ_t ਸਿਰਾ = i + 1;
            ਜਦੋਂ (ਸਿਰਾ < accesses.ਮਾਪ() && accesses[ਸਿਰਾ].m_vscp == accesses[i].m_vscp) ++ਸਿਰਾ;
            serializeVariable(accesses.ਅੰਕੜੇ() + i, accesses.ਅੰਕੜੇ() + ਸਿਰਾ);
            i = ਸਿਰਾ;
        }

        // Serialize all writes to SystemC vars. Note the reads of an individual SC var are already
        // ordered against its writes by the per variable pass above, it is only the writes between
        // different SC vars that need this extra serialization. Only top level ports are SC vars,
        // so this should not hurt performance too much.
        {
            m_scratch.clear();
            ਲਈ (ਅਚਲ Access& access : accesses) {
                ਜੇ (!access.m_access.isWriteOrRW()) ਜਾਰੀ;
                ਜੇ (!access.m_vscp->varp()->isSc()) ਜਾਰੀ;
                m_scratch.ਧੱਕੋ_ਪਿੱਠ(access.m_mtaskp);
            }
            serializeMTasks(m_scratch);
        }

        // Serialize DPI calls unless user gave '--threads-dpi none'.
        ਜੇ (!v3Global.opt.ਧਾਗੇ_Dpi_ਸ਼ੁੱਧ() || !v3Global.opt.ਧਾਗੇ_Dpi_Unpure()) {
            m_scratch.clear();
            ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_m_Task_ਗ੍ਰਾਫ.vertices()) {
                LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
                ਜੇ (hasDpiHazard(&mtask)) m_scratch.ਧੱਕੋ_ਪਿੱਠ(&mtask);
            }
            serializeMTasks(m_scratch);
        }
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& m_Task_ਗ੍ਰਾਫ) { Fix_ਅੰਕੜੇ_Hazards{m_Task_ਗ੍ਰਾਫ}; }
};

ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::fix_ਅੰਕੜੇ_Hazards(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& mtask_ਗ੍ਰਾਫ) {
    Fix_ਅੰਕੜੇ_Hazards::apply(mtask_ਗ੍ਰਾਫ);
    // The critical paths are maintained as the graph is mutated, check them
    mtask_ਗ੍ਰਾਫ.validate();
}
