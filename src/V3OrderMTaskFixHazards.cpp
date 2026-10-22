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
رده DpiImportCallVisitor final : عمومی VNVisitor {
    منطقی m_hasDpiHazard = نادرست;  // Found a DPI import call.
    منطقی m_tracingCall = نادرست;  // Iterating into a CCall to a CFunc
    // METHODS
    تهی visit(AstCFunc* nodep) override {
        اگر (!m_tracingCall) بازگشت;
        m_tracingCall = نادرست;
        اگر (nodep->dpiImportWrapper()) {
            اگر (nodep->dpiPure() ? !v3Global.opt.threadsDpiPure()
                                 : !v3Global.opt.threadsDpiUnpure()) {
                // If hierarchical DPI wrapper cost is not found or is of a 0 cost,
                // we have a normal DPI which induces DPI hazard by default.
                m_hasDpiHazard = V3Control::getProfileData(nodep->cname()) == 0;
                UINFO(9, "DPI wrapper '" << nodep->cname()
                                         << "' has dpi hazard = " << m_hasDpiHazard);
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_CCall* nodep) override {
        iterateChildren(nodep);
        // Enter the function and trace it
        m_tracingCall = درست;
        iterate(nodep->funcp());
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

    // CONSTRUCTORS
    explicit DpiImportCallVisitor(Ast_گره* nodep) { iterate(nodep); }

عمومی:
    ایستا منطقی hasDpiHazard(Ast_گره* nodep) { بازگشت DpiImportCallVisitor{nodep}.m_hasDpiHazard; }
};

//######################################################################
// FixDataHazards

رده FixDataHazards final {
    // TYPES

    // Access to one variable, with the MTask performing it
    ساختار Access final {
        ثابت AstVarScope* m_vscp;  // The variable accessed, its index is in 'user1'
        LogicMTask* m_mtaskp;  // The accessing MTask
        VAccess m_access;  // The kind of access
    };

    // NODE STATE
    //  AstVarScope::user1    -> int: Variable index for stable sorting
    ثابت VNUser1InUse m_user1InUse;

    // MEMBERS
    OrderMTaskGraph& m_mTaskGraph;  // The Mtask graph
    std::vector<LogicMTask*> m_scratch;  // Scratch MTask list, reused to avoid reallocation

    // METHODS

    // Assign each MTask a unique sequence number, held in 'user()', such that for every edge
    // 'from -> to' we have seq(from) < seq(to). See the note at top of file on why this matters.
    // This is a topological sort using Kahn's algorithm with MTasks enumerated in a globally
    // non-decreasing critical path order.
    تهی assignSequenceNumbers() {
        ساختار MTaskCmp final {
            منطقی operator()(ثابت LogicMTask* ap, ثابت LogicMTask* bp) ثابت {
                // Order by critical path
                ثابت uint64_t aCp = ap->cpExclusive<GraphWay::FORWARD>();
                ثابت uint64_t bCp = bp->cpExclusive<GraphWay::FORWARD>();
                اگر (aCp != bCp) بازگشت aCp < bCp;
                // Break ties by stable id
                بازگشت *ap < *bp;
            }
        };
        // Set of ready vertices. Initialized to the entry MTask.
        std::set<LogicMTask*, MTaskCmp> ready{m_mTaskGraph.entryp()};
        // 'user' also used to count remaining dependencies of each MTask. Initialize it.
        برای (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
            vtx.user(static_cast<uint32_t>(vtx.inEdges().size()));
        }
        // Next sequence number to assign to each MTask.
        uint32_t seq = 0;
        // Process ready vertices in critical path order.
        وقتی (!ready.empty()) {
            // Pick up and detach the ready MTask with the smallest critical path.
            ثابت auto it = ready.begin();
            LogicMTask* ثابت mtaskp = *it;
            ready.erase(it);
            // Assign the next sequence number to the MTask
            mtaskp->user(++seq);
            // Decrement edge count of successors and add to ready set if no dependencies left
            برای (V3GraphEdge& edge : mtaskp->outEdges()) {
                LogicMTask* ثابت بالاترین = static_cast<LogicMTask*>(edge.بالاترین());
                ثابت uint32_t nDeps = بالاترین->user() - 1;
                بالاترین->user(nDeps);
                اگر (!nDeps) ready.درج(بالاترین);
            }
        }
        UASSERT(seq == m_mTaskGraph.vertices().size(), "MTask graph is cyclic");
    }

    // Gather every variable access, resolved to the MTask performing it.
    std::vector<Access> gatherAccesses() {
        std::vector<Access> accesses;
        صحیح nVars = 0;
        برای (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            برای (ثابت OrderMoveVertex& mVtx : mtask.vertexList()) {
                ثابت OrderLogicVertex* ثابت lVtxp = mVtx.logicp();
                اگر (!lVtxp) ادامه;
                برای (ثابت OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                    AstVarScope* ثابت vscp = acc.m_vscp;
                    اگر (!vscp->user1()) vscp->user1(++nVars);
                    accesses.افزودن_back({vscp, &mtask, acc.m_access});
                }
            }
        }
        بازگشت accesses;
    }

    // Add an edge ordering 'fromp' before 'top', unless they are already ordered
    تهی addEdgeIfNeeded(LogicMTask* fromp, LogicMTask* بالاترین) {
        // Nothing to order within a single MTask
        اگر (fromp == بالاترین) بازگشت;
        UASSERT_OBJ(fromp->user() < بالاترین->user(), fromp,
                    "Edge must run in increasing sequence order");
        // Already directly ordered. This is an O(1) set lookup, and catches the common case of
        // a dependency the OrderGraph already provided.
        اگر (fromp->hasEdgeTo(بالاترین)) بازگشت;
        // Otherwise check if already ordered
        اگر (m_mTaskGraph.pathExists(fromp, بالاترین, nullptr)) بازگشت;
        // Unordered. Add an edge between them.
        m_mTaskGraph.addEdge(fromp, بالاترین);
    }

    // Add the edges required to make the accesses of one variable race free. 'beginp'/'endp'
    // delimit the accesses of a single variable, in sequence number order.
    تهی serializeVariable(ثابت Access* beginp, ثابت Access* endp) {
        // Nothing to order if at most one MTask accesses this variable. Each MTask appears at
        // most once in the range, see the assertion below, so this is just the range size.
        اگر (endp - beginp < 2) بازگشت;

        std::vector<LogicMTask*>& pendingReaders = m_scratch;
        pendingReaders.clear();
        LogicMTask* prevWriterp = nullptr;
        برای (ثابت Access* accp = beginp; accp != endp; ++accp) {
            // The OrderLogicVertices record one access per variable, and at this point each MTask
            // holds exactly one logic block, so an MTask cannot appear twice here.
            UASSERT_OBJ(accp == beginp || accp[-1].m_mtaskp != accp->m_mtaskp, accp->m_mtaskp,
                        "Multiple accesses of a variable in an MTask");
            LogicMTask* ثابت mtaskp = accp->m_mtaskp;
            اگر (accp->m_access.isWriteOrRW()) {
                // Reads since the previous write must complete before this write
                برای (LogicMTask* ثابت readerp : pendingReaders) addEdgeIfNeeded(readerp, mtaskp);
                // Consecutive writes must be ordered. If a read came between them, the edges to
                // and from that read imply it already.
                اگر (prevWriterp && pendingReaders.empty()) addEdgeIfNeeded(prevWriterp, mtaskp);
                pendingReaders.clear();
                prevWriterp = mtaskp;
            } وگرنه {
                // This read must happen after the preceding write
                اگر (prevWriterp) addEdgeIfNeeded(prevWriterp, mtaskp);
                pendingReaders.افزودن_back(mtaskp);
            }
        }
    }

    // Serialize the given MTasks, in sequence number order
    تهی serializeMTasks(std::vector<LogicMTask*>& mtaskps) {
        std::sort(mtaskps.begin(), mtaskps.end(), [](ثابت LogicMTask* ap, ثابت LogicMTask* bp) {
            بازگشت ap->user() < bp->user();
        });
        mtaskps.erase(std::unique(mtaskps.begin(), mtaskps.end()), mtaskps.end());
        برای (size_t i = 1; i < mtaskps.size(); ++i) addEdgeIfNeeded(mtaskps[i - 1], mtaskps[i]);
    }

    منطقی hasDpiHazard(LogicMTask* mtaskp) {
        برای (ثابت OrderMoveVertex& mVtx : mtaskp->vertexList()) {
            OrderLogicVertex* ثابت lvtxp = mVtx.logicp();
            اگر (!lvtxp) ادامه;
            // NOTE: We don't handle DPI exports. If testbench code calls a DPI-exported function
            // at any time during eval() we may have a data hazard. (Likewise in non-threaded mode
            // if an export messes with an ordered variable we're broken.)

            // Find all calls to DPI-imported functions, we can put those into a serial order at
            // least. That should solve the most likely DPI-related data hazards.
            اگر (DpiImportCallVisitor::hasDpiHazard(lvtxp->nodep())) بازگشت درست;
        }
        بازگشت نادرست;
    }

    // CONSTRUCTOR
    FixDataHazards(OrderMTaskGraph& mTaskGraph)
        : m_mTaskGraph{mTaskGraph} {
        // Give the MTasks a total order consistent with their dependencies
        assignSequenceNumbers();

        // Gather variable accesses made by each MTask
        std::vector<Access> accesses = gatherAccesses();
        // Sort by variable (to group by variable), then by sequence number of the accessing MTask
        std::sort(accesses.begin(), accesses.end(), [](ثابت Access& a, ثابت Access& b) {
            اگر (a.m_vscp != b.m_vscp) بازگشت a.m_vscp->user1() < b.m_vscp->user1();
            بازگشت a.m_mtaskp->user() < b.m_mtaskp->user();
        });

        // Serialize the accesses of each variable
        برای (size_t i = 0; i < accesses.size();) {
            size_t end = i + 1;
            وقتی (end < accesses.size() && accesses[end].m_vscp == accesses[i].m_vscp) ++end;
            serializeVariable(accesses.data() + i, accesses.data() + end);
            i = end;
        }

        // Serialize all writes to SystemC vars. Note the reads of an individual SC var are already
        // ordered against its writes by the per variable pass above, it is only the writes between
        // different SC vars that need this extra serialization. Only top level ports are SC vars,
        // so this should not hurt performance too much.
        {
            m_scratch.clear();
            برای (ثابت Access& access : accesses) {
                اگر (!access.m_access.isWriteOrRW()) ادامه;
                اگر (!access.m_vscp->varp()->isSc()) ادامه;
                m_scratch.افزودن_back(access.m_mtaskp);
            }
            serializeMTasks(m_scratch);
        }

        // Serialize DPI calls unless user gave '--threads-dpi none'.
        اگر (!v3Global.opt.threadsDpiPure() || !v3Global.opt.threadsDpiUnpure()) {
            m_scratch.clear();
            برای (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
                LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
                اگر (hasDpiHazard(&mtask)) m_scratch.افزودن_back(&mtask);
            }
            serializeMTasks(m_scratch);
        }
    }

عمومی:
    ایستا تهی apply(OrderMTaskGraph& mTaskGraph) { FixDataHazards{mTaskGraph}; }
};

تهی OrderMTaskGraph::fixDataHazards(OrderMTaskGraph& mtaskGraph) {
    FixDataHazards::apply(mtaskGraph);
    // The critical paths are maintained as the graph is mutated, check them
    mtaskGraph.validate();
}
