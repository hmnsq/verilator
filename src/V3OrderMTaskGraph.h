// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: MTask graph for multi-threaded ordering
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
//  LogicMTask and MTaskEdge are the vertex and edge of the mtask
//  graph built and coarsened by the multi-threaded partitioner (see
//  V3OrderParallel.cpp). They are independent of the partitioner's merge
//  candidate machinery: any auxiliary data the algorithms need is attached
//  externally via the vertex/edge user pointers.
//
//  OrderMTaskGraph maintains the critical paths of the MTasks, and the ones
//  cached in the edge heaps, as the graph is mutated via 'addEdge' and
//  'mergeMTasks'.
//
//*************************************************************************

#ifndef VERILATOR_V3ORDERMTASKGRAPH_H_
#define VERILATOR_V3ORDERMTASKGRAPH_H_

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3OrderMoveGraph.h"
#অন্তর্ভাব "V3PairingHeap.h"
#অন্তর্ভাব "V3PoolAllocator.h"

#অন্তর্ভাব <array>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <sstream>
#অন্তর্ভাব <unordered_set>

class LogicMTask;
class OrderMTaskGraph;

//=============================================================================
// MTaskEdge graph edges are stored in a PairingHeap in each LogicMTask they
// connect to, sorted by critical path through that edge (and id for stability).

কাঠামো Edge_চাবি final {
    uint64_t m_cp;  // The inclusive critical path of the further MTask of the edge
    uint32_t m_id;  // The ID of the further MTask, for stable comparison
    শূন্য increase(uint64_t cp) {
        UDEBUGONLY(UASSERT(cp >= m_cp, "Must increase"););
        m_cp = cp;
    }
    // Sort first by critical path, then by ID
    bool operator<(ধ্রুবক Edge_চাবি& other) ধ্রুবক {
        যদি (m_cp != other.m_cp) ফেরত m_cp < other.m_cp;
        ফেরত m_id < other.m_id;
    }
};

using EdgeHeap = PairingHeap<Edge_চাবি>;

//=============================================================================
// LogicMTasks are stored in a PairingHeap during critical path update propagation.

কাঠামো Propagate_Pending_চাবি final {
    uint64_t m_increment;  // The amount the critical path of the MTask will grow by
    uint32_t m_id;  // The ID of the MTask, for stable comparison
    LogicMTask* m_mtaskp;  // The MTask the heap entry corresponds to
    শূন্য increase(uint64_t increment) {
        UDEBUGONLY(UASSERT(increment >= m_increment, "Must increase"););
        m_increment = increment;
    }
    // Sort first by increment, then by ID
    bool operator<(ধ্রুবক Propagate_Pending_চাবি& other) ধ্রুবক {
        যদি (m_increment != other.m_increment) ফেরত m_increment < other.m_increment;
        ফেরত m_id < other.m_id;
    }
};

using PropagatePendingHeap = PairingHeap<Propagate_Pending_চাবি>;

//=============================================================================
// GraphEdge for the MTask graph

class MTaskEdge final : সর্বজনীন V3GraphEdge {
    VL_RTTI_IMPL(MTaskEdge, V3GraphEdge)

    friend class LogicMTask;
    friend class OrderMTaskGraph;

    // MEMBERS
    // This edge can be in 2 EdgeHeaps, one forward and one reverse. We allocate the heap nodes
    // directly within the edge as they are always required and this makes association cheap.
    std::array<EdgeHeap::গ্রন্থি_2, GraphWay::অঙ্ক_WAYS> m_edge_Heap_গ্রন্থি;

    // CONSTRUCTORS
    // Private, so edges can only be created via OrderMTaskGraph, which also updates the critical
    // paths on graph mutation.
    অন্তঃরেখ MTaskEdge(OrderMTaskGraph* graphp, LogicMTask* fromp, LogicMTask* শীর্ষ);

সর্বজনীন:
    VL_UNCOPYABLE(MTaskEdge);
    VL_UNMOVABLE(MTaskEdge);

    // METHODS
    template <GraphWay::en N_Way>
    অন্তঃরেখ LogicMTask* furtherMTaskp() ধ্রুবক;
    অন্তঃরেখ LogicMTask* fromMTaskp() ধ্রুবক;
    অন্তঃরেখ LogicMTask* toMTaskp() ধ্রুবক;

    uint64_t cachedCp(GraphWay way) ধ্রুবক { ফেরত m_edge_Heap_গ্রন্থি[way].চাবি().m_cp; }
    uint32_t cachedId(GraphWay way) ধ্রুবক { ফেরত m_edge_Heap_গ্রন্থি[way].চাবি().m_id; }
    // Convert from the address of the m_edgeHeapNode[way] in an MTaskEdge back to the MTaskEdge
    স্থির ধ্রুবক MTaskEdge* toMTaskEdge(GraphWay way, ধ্রুবক EdgeHeap::গ্রন্থি_2* nodep) {
        ধ্রুবক আকার_t offset = VL_OFFSETOF(MTaskEdge, m_edge_Heap_গ্রন্থি[way]);
        ফেরত reinterpret_cast<ধ্রুবক MTaskEdge*>(reinterpret_cast<uintptr_t>(nodep) - offset);
    }
};

//=============================================================================
// LogicMTask

class LogicMTask final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(LogicMTask, V3GraphVertex)

    friend class MTaskEdge;
    friend class OrderMTaskGraph;

    // MEMBERS

    // List of OrderMoveVertex's assigned to this mtask. LogicMTask does not own the
    // OrderMoveVertex objects, we merely keep them in a list here.
    OrderMoveVertex::List m_mVertices;

    স্থির uint32_t s_পরবর্তী_Id;  // Next ID number to use
    ধ্রুবক uint32_t m_id = s_পরবর্তী_Id++;  // Unique LogicMTask ID number for stable comparison

    // Cost estimate for this LogicMTask, derived from V3InstrCount, in abstract time units.
    // Cost estimates and critical path lengths are bounded by number of AstNodes * constant,
    // will run out of host memory storing the Ast way before they can overflow.
    uint64_t m_cost = 0;

    // Critical path in each direction: going FORWARD from graph-start to the start of this vertex,
    // and going REVERSE from graph-exit to the end of this vertex. Exclusive of the cost of this
    // vertex itself, see cpInclusive() for the value including it.
    std::array<uint64_t, GraphWay::অঙ্ক_WAYS> m_cpExclusive = {0, 0};

    // The MTasks this MTask has an out-edge to, so checking for an existing edge is O(1)
    std::unordered_set<LogicMTask*> m_dependents;
    // Store the out/in edges in a heaps sorted by the critical path length through each edge
    std::array<EdgeHeap, GraphWay::অঙ্ক_WAYS> m_edgeHeap;

    // Count "generations" which are just operations that scan through the
    // graph. We'll mark each node with the last generation that scanned
    // it. We can use this to avoid recursing through the same node twice
    // while searching for a path.
    uint64_t m_generation = 0;

    // Scratch pointer used only by the critical path propagation in OrderMTaskGraph: this MTask's
    // node in the pending heap, or nullptr if this MTask is not pending.
    PropagatePendingHeap::গ্রন্থি_2* m_propagateHeapNodep = nullptr;

সর্বজনীন:
    // CONSTRUCTORS
    LogicMTask(OrderMTaskGraph& graph, OrderMoveVertex* mVtxp) VL_MT_DISABLED;
    VL_UNCOPYABLE(LogicMTask);
    VL_UNMOVABLE(LogicMTask);

    // METHODS
    OrderMoveVertex::List& vertexList() { ফেরত m_mVertices; }
    uint32_t id() ধ্রুবক { ফেরত m_id; }
    bool operator<(ধ্রুবক LogicMTask& rhs) ধ্রুবক { ফেরত id() < rhs.id(); }

    uint64_t cost() ধ্রুবক VL_MT_SAFE { ফেরত m_cost; }
    template <GraphWay::en N_Way>
    uint64_t cpExclusive() ধ্রুবক {
        ফেরত m_cpExclusive[N_Way];
    }
    template <GraphWay::en N_Way>
    uint64_t cpInclusive() ধ্রুবক {
        ফেরত m_cpExclusive[N_Way] + m_cost;
    }
    // The critical path of this MTask without considering the given edge.
    template <GraphWay::en N_Way>
    uint64_t cpExclusiveWithout(ধ্রুবক V3GraphEdge* edgep) ধ্রুবক {
        ধ্রুবক GraphWay way{N_Way};
        ধ্রুবক GraphWay inv = way.invert();
        UDEBUGONLY(UASSERT(edgep->furtherp<N_Way>() == this,
                           "In cpExclusiveWithout(), 'edgep' must further to 'this'"););
        // At most two edges need to be considered: the critical path, if that is not via 'edgep',
        // or the second-worst path, if the critical path is via 'edgep'.
        ধ্রুবক EdgeHeap& edgeHeap = m_edgeHeap[inv];
        // Pick up the critical path edge
        ধ্রুবক EdgeHeap::গ্রন্থি_2* ধ্রুবক maxp = edgeHeap.max();
        UDEBUGONLY(UASSERT(maxp, "Edge not in heap"););
        // If 'edgep' is not the critical path edge, return its critical path
        যদি (MTaskEdge::toMTaskEdge(inv, maxp) != edgep) ফেরত maxp->চাবি().m_cp;
        // Otherwise return the second-worst path, if there is one
        ধ্রুবক EdgeHeap::গ্রন্থি_2* ধ্রুবক secp = edgeHeap.secondMax();
        যদি (!secp) ফেরত 0;
        ফেরত secp->চাবি().m_cp;
    }

    bool hasEdgeTo(LogicMTask* dependentp) ধ্রুবক { ফেরত m_dependents.গুনতি(dependentp); }

    // For Graphviz dumps only
    std::string name() ধ্রুবক override VL_MT_STABLE {
        std::ostringstream out;
        out << "mt" << m_id  //
            << " | cpFwd " << m_cpExclusive[GraphWay::FORWARD]  //
            << " | cost " << cost()  //
            << " | cpRev " << m_cpExclusive[GraphWay::REVERSE];
        ফেরত out.str();
    }

ব্যক্তিগত:
    // Following only used by OrderMTaskGraph, which maintains cached CPs and graph invariants.

    template <GraphWay::en N_Way>
    uint64_t cpExclusiveFromEdges() ধ্রুবক {
        constexpr GraphWay inv = GraphWay{N_Way}.invert();
        ধ্রুবক EdgeHeap::গ্রন্থি_2* ধ্রুবক maxp = m_edgeHeap[inv].max();
        ফেরত maxp ? maxp->চাবি().m_cp : 0;
    }
    template <GraphWay::en N_Way>
    শূন্য cpExclusive(uint64_t cp) {
        m_cpExclusive[N_Way] = cp;
    }

    template <GraphWay::en N_Way>
    শূন্য addRelativeEdge(MTaskEdge* edgep) {
        constexpr GraphWay way{N_Way};
        constexpr GraphWay inv = way.invert();
        // Add to the edge heap
        LogicMTask* ধ্রুবক relativep = edgep->furtherMTaskp<N_Way>();
        // Value is the !way inclusive cp of the relative
        ধ্রুবক uint64_t cp = relativep->cpInclusive<inv>();
        m_edgeHeap[way].ঢোকাও(&edgep->m_edge_Heap_গ্রন্থি[way], {cp, relativep->id()});
    }
    template <GraphWay::en N_Way>
    শূন্য stealRelativeEdge(MTaskEdge* edgep) {
        constexpr GraphWay way{N_Way};
        // Make heap node insertable, ruining the heap it is currently in.
        edgep->m_edge_Heap_গ্রন্থি[way].yank();
        // Add the edge as new
        addRelativeEdge<N_Way>(edgep);
    }
    template <GraphWay::en N_Way>
    শূন্য removeRelativeEdge(MTaskEdge* edgep) {
        constexpr GraphWay way{N_Way};
        // Remove from the edge heap
        m_edgeHeap[way].remove(&edgep->m_edge_Heap_গ্রন্থি[way]);
    }

    শূন্য addDependent(LogicMTask* dependentp) {
        ধ্রুবক bool exists = !m_dependents.emplace(dependentp).second;
        UDEBUGONLY(UASSERT(!exists, "Adding existing dependent"););
    }
    শূন্য removeDependent(LogicMTask* dependentp) {
        ধ্রুবক আকার_t removed = m_dependents.erase(dependentp);
        UDEBUGONLY(UASSERT(removed, "Dependent should have been in set"););
    }
};

//=============================================================================
// OrderMTaskGraph

// The graph of LogicMTask vertices and MTaskEdge edges, used during multi-threaded scheduling.
class OrderMTaskGraph final : সর্বজনীন V3Graph {
    // MEMBERS
    OrderMoveGraph& m_moveGraph;  // The OrderMoveGraph this graph is built from
    LogicMTask* ধ্রুবক m_entryp;  // The singular entry point vertex
    LogicMTask* ধ্রুবক m_exitp;  // The singular exit point vertex

    ধ্রুবক bool m_slowAsserts;  // Take extra time to validate the graph ('--debug-partition')

    // Critical path propagation state. Scratch only: the heap is empty, and no MTask is pending,
    // between calls to 'propagate'. The node pool persists to recycle the heap nodes.
    PropagatePendingHeap m_pendingHeap;  // Heap of MTasks pending a critical path update
    PoolAllocator<PropagatePendingHeap::গ্রন্থি_2> m_pending_গ্রন্থি_Pool;  // Allocator for the heap nodes

    // Generation counter, e.g. for marking the MTasks visited by algorithms
    uint64_t m_currentGeneration = 0;

    // CONSTRUCTOR
    explicit OrderMTaskGraph(OrderMoveGraph& moveGraph);  // Used by build(), hence private
    VL_UNCOPYABLE(OrderMTaskGraph);
    VL_UNMOVABLE(OrderMTaskGraph);

    // METHODS

    bool pathExistsImpl(LogicMTask* fromp, LogicMTask* শীর্ষ, ধ্রুবক MTaskEdge* excludedEdgep);

    // Bring the critical paths of all MTasks wayward of 'mtaskp' in direction N_Way, and those
    // cached in the edge heaps on the way, up to date. Call after mutating the graph such that
    // only MTasks wayward of 'mtaskp' can have a stale critical path, and the critical path of
    // 'mtaskp' itself is already correct. 'mtaskp' is read, but never modified.
    //
    // Note critical paths can only ever grow: those cached in the edge heaps are heap keys, and a
    // heap key can be increased in place, but not decreased.
    template <GraphWay::en N_Way>
    শূন্য propagate(LogicMTask* mtaskp) {
        ++m_currentGeneration;
        propagate_ঠেলো<N_Way>(mtaskp);
        propagateResolve<N_Way>();
    }
    // Push the inclusive critical path of 'mtaskp' onto each of its wayward relatives, and add any
    // relative left with a stale critical path to the pending heap (out of line below)
    template <GraphWay::en N_Way>
    শূন্য propagate_ঠেলো(LogicMTask* mtaskp);
    // Resolve all pending critical path increases (out of line below)
    template <GraphWay::en N_Way>
    শূন্য propagateResolve();
    // Part of 'validate'
    template <GraphWay::en N_Way>
    শূন্য validateWay() ধ্রুবক;

সর্বজনীন:
    // ACCESSORS
    OrderMoveGraph& moveGraph() ধ্রুবক { ফেরত m_moveGraph; }
    LogicMTask* entryp() ধ্রুবক { ফেরত m_entryp; }
    LogicMTask* exitp() ধ্রুবক { ফেরত m_exitp; }
    bool slowAsserts() ধ্রুবক { ফেরত m_slowAsserts; }

    // METHODS
    uint64_t যোগ_Cost() ধ্রুবক;  // O(V), called once

    // True if there's a path from 'fromp' to 'top' excluding 'excludedEdgep', false otherwise.
    // 'excludedEdgep' may be nullptr in which case no edge is excluded. If 'excludedEdgep' is
    // non-nullptr it must connect fromp and top.
    bool pathExists(LogicMTask* fromp, LogicMTask* শীর্ষ, ধ্রুবক MTaskEdge* excludedEdgep) {
        ++m_currentGeneration;
        ফেরত pathExistsImpl(fromp, শীর্ষ, excludedEdgep);
    }

    // Add an edge to the graph, update impacted critical paths
    শূন্য addEdge(LogicMTask* fromp, LogicMTask* শীর্ষ);

    // Merge 'donorp' into 'recipientp': move the contents and all edges of 'donorp' onto
    // 'recipientp', update impacted critical paths, then delete 'donorp'. The edge connecting the
    // two (if any) becomes internal to the merged MTask and is deleted, as is one of each pair of
    // edges the two have to a common relative. Note this deletes edges, so the caller must have
    // released any auxiliary data it attached to them via their user pointer.
    শূন্য mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp);

    // Remove all transitive edges. (This deliberately hides V3Graph::removeTransitiveEdges,
    // which would leave the auxiliary data structures stale.)
    // cppcheck-suppress duplInheritedMember
    শূন্য removeTransitiveEdges();

    // Remove all MTasks holding no logic (except for entry and exit, which are kept even if
    // empty), connecting their predecessors directly to their successors.
    শূন্য removeEmptyMTasks();

    // Do an expensive check that the maintained critical paths, including the ones cached in the
    // edge heaps, match those implied by the current edges of the graph, and that auxiliary data
    // structures are consistent.
    শূন্য validate() ধ্রুবক;

    // STATIC METHODS
    // Build an MTask graph from 'moveGraph'
    স্থির std::unique_ptr<OrderMTaskGraph> build(OrderMoveGraph& moveGraph) VL_MT_DISABLED;
    // Fix data hazards in the MTask graph
    স্থির শূন্য fix_তথ্য_Hazards(OrderMTaskGraph& mtaskGraph) VL_MT_DISABLED;
    // Coarsen the MTask graph by merging MTasks until the given critical-path limit is reached
    স্থির শূন্য contract(OrderMTaskGraph& mtaskGraph, uint64_t scoreLimit) VL_MT_DISABLED;
};

//=============================================================================
// MTaskEdge method definitions (need the full definition of LogicMTask)

MTaskEdge::MTaskEdge(OrderMTaskGraph* graphp, LogicMTask* fromp, LogicMTask* শীর্ষ)
    : V3GraphEdge{graphp, fromp, শীর্ষ, 1} {
    fromp->addDependent(শীর্ষ);
    fromp->addRelativeEdge<GraphWay::FORWARD>(this);
    শীর্ষ->addRelativeEdge<GraphWay::REVERSE>(this);
}

template <GraphWay::en N_Way>
LogicMTask* MTaskEdge::furtherMTaskp() ধ্রুবক {
    ফেরত static_cast<LogicMTask*>(this->furtherp<N_Way>());
}
LogicMTask* MTaskEdge::fromMTaskp() ধ্রুবক { ফেরত static_cast<LogicMTask*>(fromp()); }
LogicMTask* MTaskEdge::toMTaskp() ধ্রুবক { ফেরত static_cast<LogicMTask*>(শীর্ষ()); }

#endif  // Guard
