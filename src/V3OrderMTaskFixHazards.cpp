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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3OrderGraph.h"
#అన్తర్భావ "V3OrderMTaskGraph.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <set>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// DpiImportCallVisitor

// Scan node, indicate whether it contains a call to a DPI imported routine.
తరగతి DpiImportCallVisitor final : బహిరంగ VNVisitor {
    తార్కిక m_hasDpiHazard = అబద్ధం;  // Found a DPI import call.
    తార్కిక m_tracingCall = అబద్ధం;  // Iterating into a CCall to a CFunc
    // METHODS
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        ఒకవేళ (!m_tracingCall) తిరిగి;
        m_tracingCall = అబద్ధం;
        ఒకవేళ (nodep->dpiImportWrapper()) {
            ఒకవేళ (nodep->dpi_శుద్ధ() ? !v3Global.opt.దారాలు_Dpi_శుద్ధ()
                                 : !v3Global.opt.దారాలు_Dpi_Unpure()) {
                // If hierarchical DPI wrapper cost is not found or is of a 0 cost,
                // we have a normal DPI which induces DPI hazard by default.
                m_hasDpiHazard = V3Control::get_Profile_దత్తాంశం(nodep->cname()) == 0;
                UINFO(9, "DPI wrapper '" << nodep->cname()
                                         << "' has dpi hazard = " << m_hasDpiHazard);
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        iterateChildren(nodep);
        // Enter the function and trace it
        m_tracingCall = నిజం;
        iterate(nodep->funcp());
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    స్పష్టమైన DpiImportCallVisitor(Ast_కణుపు* nodep) { iterate(nodep); }

బహిరంగ:
    స్థిర తార్కిక hasDpiHazard(Ast_కణుపు* nodep) { తిరిగి DpiImportCallVisitor{nodep}.m_hasDpiHazard; }
};

//######################################################################
// FixDataHazards

తరగతి Fix_దత్తాంశం_Hazards final {
    // TYPES

    // Access to one variable, with the MTask performing it
    నిర్మాణం Access final {
        స్థిరరాశి AstVarScope* m_vscp;  // The variable accessed, its index is in 'user1'
        LogicMTask* m_mtaskp;  // The accessing MTask
        VAccess m_access;  // The kind of access
    };

    // NODE STATE
    //  AstVarScope::user1    -> int: Variable index for stable sorting
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // MEMBERS
    క్రమం_MTask_లేఖాచిత్రం& m_m_Task_లేఖాచిత్రం;  // The Mtask graph
    std::సదిశ<LogicMTask*> m_scratch;  // Scratch MTask list, reused to avoid reallocation

    // METHODS

    // Assign each MTask a unique sequence number, held in 'user()', such that for every edge
    // 'from -> to' we have seq(from) < seq(to). See the note at top of file on why this matters.
    // This is a topological sort using Kahn's algorithm with MTasks enumerated in a globally
    // non-decreasing critical path order.
    శూన్య కేటాయించు_Sequence_Numbers() {
        నిర్మాణం MTaskCmp final {
            తార్కిక క్రియాకారకం()(స్థిరరాశి LogicMTask* ap, స్థిరరాశి LogicMTask* bp) స్థిరరాశి {
                // Order by critical path
                స్థిరరాశి uint64_t aCp = ap->cpExclusive<లేఖాచిత్రం_Way::FORWARD>();
                స్థిరరాశి uint64_t bCp = bp->cpExclusive<లేఖాచిత్రం_Way::FORWARD>();
                ఒకవేళ (aCp != bCp) తిరిగి aCp < bCp;
                // Break ties by stable id
                తిరిగి *ap < *bp;
            }
        };
        // Set of ready vertices. Initialized to the entry MTask.
        std::సమితి<LogicMTask*, MTaskCmp> ready{m_m_Task_లేఖాచిత్రం.entryp()};
        // 'user' also used to count remaining dependencies of each MTask. Initialize it.
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_m_Task_లేఖాచిత్రం.vertices()) {
            vtx.user(static_cast<uint32_t>(vtx.లోపల_అంచులు().కొలత()));
        }
        // Next sequence number to assign to each MTask.
        uint32_t seq = 0;
        // Process ready vertices in critical path order.
        అయితే (!ready.ఖాళీలు()) {
            // Pick up and detach the ready MTask with the smallest critical path.
            స్థిరరాశి స్వయంచాలక it = ready.begin();
            LogicMTask* స్థిరరాశి mtaskp = *it;
            ready.erase(it);
            // Assign the next sequence number to the MTask
            mtaskp->user(++seq);
            // Decrement edge count of successors and add to ready set if no dependencies left
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : mtaskp->బహిర్గతం_అంచులు()) {
                LogicMTask* స్థిరరాశి శిఖరం = static_cast<LogicMTask*>(అంచు.శిఖరం());
                స్థిరరాశి uint32_t nDeps = శిఖరం->user() - 1;
                శిఖరం->user(nDeps);
                ఒకవేళ (!nDeps) ready.చొప్పించు(శిఖరం);
            }
        }
        UASSERT(seq == m_m_Task_లేఖాచిత్రం.vertices().కొలత(), "MTask graph is cyclic");
    }

    // Gather every variable access, resolved to the MTask performing it.
    std::సదిశ<Access> gatherAccesses() {
        std::సదిశ<Access> accesses;
        పూర్ణాంకం nVars = 0;
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_m_Task_లేఖాచిత్రం.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            కొరకు (స్థిరరాశి క్రమం_Move_శీర్షం& mVtx : mtask.శీర్షం_List()) {
                స్థిరరాశి క్రమం_Logic_శీర్షం* స్థిరరాశి lVtxp = mVtx.logicp();
                ఒకవేళ (!lVtxp) కొనసాగు;
                కొరకు (స్థిరరాశి క్రమం_Logic_శీర్షం::VarAccess& acc : lVtxp->varAccesses()) {
                    AstVarScope* స్థిరరాశి vscp = acc.m_vscp;
                    ఒకవేళ (!vscp->user1()) vscp->user1(++nVars);
                    accesses.నెట్టు_వెనుక({vscp, &mtask, acc.m_access});
                }
            }
        }
        తిరిగి accesses;
    }

    // Add an edge ordering 'fromp' before 'top', unless they are already ordered
    శూన్య కూడు_అంచు_If_Needed(LogicMTask* fromp, LogicMTask* శిఖరం) {
        // Nothing to order within a single MTask
        ఒకవేళ (fromp == శిఖరం) తిరిగి;
        UASSERT_OBJ(fromp->user() < శిఖరం->user(), fromp,
                    "Edge must run in increasing sequence order");
        // Already directly ordered. This is an O(1) set lookup, and catches the common case of
        // a dependency the OrderGraph already provided.
        ఒకవేళ (fromp->has_అంచు_గమ్యం(శిఖరం)) తిరిగి;
        // Otherwise check if already ordered
        ఒకవేళ (m_m_Task_లేఖాచిత్రం.మార్గం_Exists(fromp, శిఖరం, శూన్యసూచిక)) తిరిగి;
        // Unordered. Add an edge between them.
        m_m_Task_లేఖాచిత్రం.కూడు_అంచు(fromp, శిఖరం);
    }

    // Add the edges required to make the accesses of one variable race free. 'beginp'/'endp'
    // delimit the accesses of a single variable, in sequence number order.
    శూన్య serializeVariable(స్థిరరాశి Access* beginp, స్థిరరాశి Access* endp) {
        // Nothing to order if at most one MTask accesses this variable. Each MTask appears at
        // most once in the range, see the assertion below, so this is just the range size.
        ఒకవేళ (endp - beginp < 2) తిరిగి;

        std::సదిశ<LogicMTask*>& pendingReaders = m_scratch;
        pendingReaders.clear();
        LogicMTask* prevWriterp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి Access* accp = beginp; accp != endp; ++accp) {
            // The OrderLogicVertices record one access per variable, and at this point each MTask
            // holds exactly one logic block, so an MTask cannot appear twice here.
            UASSERT_OBJ(accp == beginp || accp[-1].m_mtaskp != accp->m_mtaskp, accp->m_mtaskp,
                        "Multiple accesses of a variable in an MTask");
            LogicMTask* స్థిరరాశి mtaskp = accp->m_mtaskp;
            ఒకవేళ (accp->m_access.isWriteOrRW()) {
                // Reads since the previous write must complete before this write
                కొరకు (LogicMTask* స్థిరరాశి readerp : pendingReaders) కూడు_అంచు_If_Needed(readerp, mtaskp);
                // Consecutive writes must be ordered. If a read came between them, the edges to
                // and from that read imply it already.
                ఒకవేళ (prevWriterp && pendingReaders.ఖాళీలు()) కూడు_అంచు_If_Needed(prevWriterp, mtaskp);
                pendingReaders.clear();
                prevWriterp = mtaskp;
            } లేకపోతే {
                // This read must happen after the preceding write
                ఒకవేళ (prevWriterp) కూడు_అంచు_If_Needed(prevWriterp, mtaskp);
                pendingReaders.నెట్టు_వెనుక(mtaskp);
            }
        }
    }

    // Serialize the given MTasks, in sequence number order
    శూన్య serializeMTasks(std::సదిశ<LogicMTask*>& mtaskps) {
        std::క్రమీకరించు(mtaskps.begin(), mtaskps.చివర(), [](స్థిరరాశి LogicMTask* ap, స్థిరరాశి LogicMTask* bp) {
            తిరిగి ap->user() < bp->user();
        });
        mtaskps.erase(std::unique(mtaskps.begin(), mtaskps.చివర()), mtaskps.చివర());
        కొరకు (కొలత_t i = 1; i < mtaskps.కొలత(); ++i) కూడు_అంచు_If_Needed(mtaskps[i - 1], mtaskps[i]);
    }

    తార్కిక hasDpiHazard(LogicMTask* mtaskp) {
        కొరకు (స్థిరరాశి క్రమం_Move_శీర్షం& mVtx : mtaskp->శీర్షం_List()) {
            క్రమం_Logic_శీర్షం* స్థిరరాశి lvtxp = mVtx.logicp();
            ఒకవేళ (!lvtxp) కొనసాగు;
            // NOTE: We don't handle DPI exports. If testbench code calls a DPI-exported function
            // at any time during eval() we may have a data hazard. (Likewise in non-threaded mode
            // if an export messes with an ordered variable we're broken.)

            // Find all calls to DPI-imported functions, we can put those into a serial order at
            // least. That should solve the most likely DPI-related data hazards.
            ఒకవేళ (DpiImportCallVisitor::hasDpiHazard(lvtxp->nodep())) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // CONSTRUCTOR
    Fix_దత్తాంశం_Hazards(క్రమం_MTask_లేఖాచిత్రం& m_Task_లేఖాచిత్రం)
        : m_m_Task_లేఖాచిత్రం{m_Task_లేఖాచిత్రం} {
        // Give the MTasks a total order consistent with their dependencies
        కేటాయించు_Sequence_Numbers();

        // Gather variable accesses made by each MTask
        std::సదిశ<Access> accesses = gatherAccesses();
        // Sort by variable (to group by variable), then by sequence number of the accessing MTask
        std::క్రమీకరించు(accesses.begin(), accesses.చివర(), [](స్థిరరాశి Access& a, స్థిరరాశి Access& b) {
            ఒకవేళ (a.m_vscp != b.m_vscp) తిరిగి a.m_vscp->user1() < b.m_vscp->user1();
            తిరిగి a.m_mtaskp->user() < b.m_mtaskp->user();
        });

        // Serialize the accesses of each variable
        కొరకు (కొలత_t i = 0; i < accesses.కొలత();) {
            కొలత_t చివర = i + 1;
            అయితే (చివర < accesses.కొలత() && accesses[చివర].m_vscp == accesses[i].m_vscp) ++చివర;
            serializeVariable(accesses.దత్తాంశం() + i, accesses.దత్తాంశం() + చివర);
            i = చివర;
        }

        // Serialize all writes to SystemC vars. Note the reads of an individual SC var are already
        // ordered against its writes by the per variable pass above, it is only the writes between
        // different SC vars that need this extra serialization. Only top level ports are SC vars,
        // so this should not hurt performance too much.
        {
            m_scratch.clear();
            కొరకు (స్థిరరాశి Access& access : accesses) {
                ఒకవేళ (!access.m_access.isWriteOrRW()) కొనసాగు;
                ఒకవేళ (!access.m_vscp->varp()->isSc()) కొనసాగు;
                m_scratch.నెట్టు_వెనుక(access.m_mtaskp);
            }
            serializeMTasks(m_scratch);
        }

        // Serialize DPI calls unless user gave '--threads-dpi none'.
        ఒకవేళ (!v3Global.opt.దారాలు_Dpi_శుద్ధ() || !v3Global.opt.దారాలు_Dpi_Unpure()) {
            m_scratch.clear();
            కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_m_Task_లేఖాచిత్రం.vertices()) {
                LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
                ఒకవేళ (hasDpiHazard(&mtask)) m_scratch.నెట్టు_వెనుక(&mtask);
            }
            serializeMTasks(m_scratch);
        }
    }

బహిరంగ:
    స్థిర శూన్య apply(క్రమం_MTask_లేఖాచిత్రం& m_Task_లేఖాచిత్రం) { Fix_దత్తాంశం_Hazards{m_Task_లేఖాచిత్రం}; }
};

శూన్య క్రమం_MTask_లేఖాచిత్రం::fix_దత్తాంశం_Hazards(క్రమం_MTask_లేఖాచిత్రం& mtask_లేఖాచిత్రం) {
    Fix_దత్తాంశం_Hazards::apply(mtask_లేఖాచిత్రం);
    // The critical paths are maintained as the graph is mutated, check them
    mtask_లేఖాచిత్రం.validate();
}
