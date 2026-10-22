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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3OrderMoveGraph.h"
#अन्तर्भाव "V3PairingHeap.h"
#अन्तर्भाव "V3PoolAllocator.h"

#अन्तर्भाव <array>
#अन्तर्भाव <memory>
#अन्तर्भाव <sstream>
#अन्तर्भाव <unordered_set>

class LogicMTask;
class OrderMTaskGraph;

//=============================================================================
// MTaskEdge graph edges are stored in a PairingHeap in each LogicMTask they
// connect to, sorted by critical path through that edge (and id for stability).

संरचना Edge_किल्ली final {
    uint64_t m_cp;  // The inclusive critical path of the further MTask of the edge
    uint32_t m_id;  // The ID of the further MTask, for stable comparison
    शून्य increase(uint64_t cp) {
        UDEBUGONLY(UASSERT(cp >= m_cp, "Must increase"););
        m_cp = cp;
    }
    // Sort first by critical path, then by ID
    bool चालक<(स्थिरांक Edge_किल्ली& other) स्थिरांक {
        जर (m_cp != other.m_cp) परत m_cp < other.m_cp;
        परत m_id < other.m_id;
    }
};

using EdgeHeap = PairingHeap<Edge_किल्ली>;

//=============================================================================
// LogicMTasks are stored in a PairingHeap during critical path update propagation.

संरचना Propagate_Pending_किल्ली final {
    uint64_t m_increment;  // The amount the critical path of the MTask will grow by
    uint32_t m_id;  // The ID of the MTask, for stable comparison
    LogicMTask* m_mtaskp;  // The MTask the heap entry corresponds to
    शून्य increase(uint64_t increment) {
        UDEBUGONLY(UASSERT(increment >= m_increment, "Must increase"););
        m_increment = increment;
    }
    // Sort first by increment, then by ID
    bool चालक<(स्थिरांक Propagate_Pending_किल्ली& other) स्थिरांक {
        जर (m_increment != other.m_increment) परत m_increment < other.m_increment;
        परत m_id < other.m_id;
    }
};

using PropagatePendingHeap = PairingHeap<Propagate_Pending_किल्ली>;

//=============================================================================
// GraphEdge for the MTask graph

class MTaskEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(MTaskEdge, V3GraphEdge)

    मित्र class LogicMTask;
    मित्र class OrderMTaskGraph;

    // MEMBERS
    // This edge can be in 2 EdgeHeaps, one forward and one reverse. We allocate the heap nodes
    // directly within the edge as they are always required and this makes association cheap.
    std::array<EdgeHeap::गाठ_2, GraphWay::अंक_WAYS> m_edge_Heap_गाठ;

    // CONSTRUCTORS
    // Private, so edges can only be created via OrderMTaskGraph, which also updates the critical
    // paths on graph mutation.
    ओळीत MTaskEdge(OrderMTaskGraph* graphp, LogicMTask* fromp, LogicMTask* शीर्ष);

सार्वजनिक:
    VL_UNCOPYABLE(MTaskEdge);
    VL_UNMOVABLE(MTaskEdge);

    // METHODS
    ढाँचा <GraphWay::en N_Way>
    ओळीत LogicMTask* furtherMTaskp() स्थिरांक;
    ओळीत LogicMTask* fromMTaskp() स्थिरांक;
    ओळीत LogicMTask* toMTaskp() स्थिरांक;

    uint64_t cachedCp(GraphWay way) स्थिरांक { परत m_edge_Heap_गाठ[way].किल्ली().m_cp; }
    uint32_t cachedId(GraphWay way) स्थिरांक { परत m_edge_Heap_गाठ[way].किल्ली().m_id; }
    // Convert from the address of the m_edgeHeapNode[way] in an MTaskEdge back to the MTaskEdge
    स्थिर स्थिरांक MTaskEdge* toMTaskEdge(GraphWay way, स्थिरांक EdgeHeap::गाठ_2* nodep) {
        स्थिरांक माप_प्रकार offset = VL_OFFSETOF(MTaskEdge, m_edge_Heap_गाठ[way]);
        परत reinterpret_cast<स्थिरांक MTaskEdge*>(reinterpret_cast<uintptr_t>(nodep) - offset);
    }
};

//=============================================================================
// LogicMTask

class LogicMTask final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(LogicMTask, V3GraphVertex)

    मित्र class MTaskEdge;
    मित्र class OrderMTaskGraph;

    // MEMBERS

    // List of OrderMoveVertex's assigned to this mtask. LogicMTask does not own the
    // OrderMoveVertex objects, we merely keep them in a list here.
    OrderMoveVertex::List m_mVertices;

    स्थिर uint32_t s_nextId;  // Next ID number to use
    स्थिरांक uint32_t m_id = s_nextId++;  // Unique LogicMTask ID number for stable comparison

    // Cost estimate for this LogicMTask, derived from V3InstrCount, in abstract time units.
    // Cost estimates and critical path lengths are bounded by number of AstNodes * constant,
    // will run out of host memory storing the Ast way before they can overflow.
    uint64_t m_cost = 0;

    // Critical path in each direction: going FORWARD from graph-start to the start of this vertex,
    // and going REVERSE from graph-exit to the end of this vertex. Exclusive of the cost of this
    // vertex itself, see cpInclusive() for the value including it.
    std::array<uint64_t, GraphWay::अंक_WAYS> m_cpExclusive = {0, 0};

    // The MTasks this MTask has an out-edge to, so checking for an existing edge is O(1)
    std::unordered_set<LogicMTask*> m_dependents;
    // Store the out/in edges in a heaps sorted by the critical path length through each edge
    std::array<EdgeHeap, GraphWay::अंक_WAYS> m_edgeHeap;

    // Count "generations" which are just operations that scan through the
    // graph. We'll mark each node with the last generation that scanned
    // it. We can use this to avoid recursing through the same node twice
    // while searching for a path.
    uint64_t m_generation = 0;

    // Scratch pointer used only by the critical path propagation in OrderMTaskGraph: this MTask's
    // node in the pending heap, or nullptr if this MTask is not pending.
    PropagatePendingHeap::गाठ_2* m_propagateHeapNodep = nullptr;

सार्वजनिक:
    // CONSTRUCTORS
    LogicMTask(OrderMTaskGraph& graph, OrderMoveVertex* mVtxp) VL_MT_DISABLED;
    VL_UNCOPYABLE(LogicMTask);
    VL_UNMOVABLE(LogicMTask);

    // METHODS
    OrderMoveVertex::List& vertexList() { परत m_mVertices; }
    uint32_t id() स्थिरांक { परत m_id; }
    bool चालक<(स्थिरांक LogicMTask& rhs) स्थिरांक { परत id() < rhs.id(); }

    uint64_t cost() स्थिरांक VL_MT_SAFE { परत m_cost; }
    ढाँचा <GraphWay::en N_Way>
    uint64_t cpExclusive() स्थिरांक {
        परत m_cpExclusive[N_Way];
    }
    ढाँचा <GraphWay::en N_Way>
    uint64_t cpInclusive() स्थिरांक {
        परत m_cpExclusive[N_Way] + m_cost;
    }
    // The critical path of this MTask without considering the given edge.
    ढाँचा <GraphWay::en N_Way>
    uint64_t cpExclusiveWithout(स्थिरांक V3GraphEdge* edgep) स्थिरांक {
        स्थिरांक GraphWay way{N_Way};
        स्थिरांक GraphWay inv = way.invert();
        UDEBUGONLY(UASSERT(edgep->furtherp<N_Way>() == this,
                           "In cpExclusiveWithout(), 'edgep' must further to 'this'"););
        // At most two edges need to be considered: the critical path, if that is not via 'edgep',
        // or the second-worst path, if the critical path is via 'edgep'.
        स्थिरांक EdgeHeap& edgeHeap = m_edgeHeap[inv];
        // Pick up the critical path edge
        स्थिरांक EdgeHeap::गाठ_2* स्थिरांक maxp = edgeHeap.max();
        UDEBUGONLY(UASSERT(maxp, "Edge not in heap"););
        // If 'edgep' is not the critical path edge, return its critical path
        जर (MTaskEdge::toMTaskEdge(inv, maxp) != edgep) परत maxp->किल्ली().m_cp;
        // Otherwise return the second-worst path, if there is one
        स्थिरांक EdgeHeap::गाठ_2* स्थिरांक secp = edgeHeap.secondMax();
        जर (!secp) परत 0;
        परत secp->किल्ली().m_cp;
    }

    bool hasEdgeTo(LogicMTask* dependentp) स्थिरांक { परत m_dependents.मोजणी(dependentp); }

    // For Graphviz dumps only
    std::string name() स्थिरांक override VL_MT_STABLE {
        std::ostringstream out;
        out << "mt" << m_id  //
            << " | cpFwd " << m_cpExclusive[GraphWay::FORWARD]  //
            << " | cost " << cost()  //
            << " | cpRev " << m_cpExclusive[GraphWay::REVERSE];
        परत out.str();
    }

खाजगी:
    // Following only used by OrderMTaskGraph, which maintains cached CPs and graph invariants.

    ढाँचा <GraphWay::en N_Way>
    uint64_t cpExclusiveFromEdges() स्थिरांक {
        constexpr GraphWay inv = GraphWay{N_Way}.invert();
        स्थिरांक EdgeHeap::गाठ_2* स्थिरांक maxp = m_edgeHeap[inv].max();
        परत maxp ? maxp->किल्ली().m_cp : 0;
    }
    ढाँचा <GraphWay::en N_Way>
    शून्य cpExclusive(uint64_t cp) {
        m_cpExclusive[N_Way] = cp;
    }

    ढाँचा <GraphWay::en N_Way>
    शून्य addRelativeEdge(MTaskEdge* edgep) {
        constexpr GraphWay way{N_Way};
        constexpr GraphWay inv = way.invert();
        // Add to the edge heap
        LogicMTask* स्थिरांक relativep = edgep->furtherMTaskp<N_Way>();
        // Value is the !way inclusive cp of the relative
        स्थिरांक uint64_t cp = relativep->cpInclusive<inv>();
        m_edgeHeap[way].घाला(&edgep->m_edge_Heap_गाठ[way], {cp, relativep->id()});
    }
    ढाँचा <GraphWay::en N_Way>
    शून्य stealRelativeEdge(MTaskEdge* edgep) {
        constexpr GraphWay way{N_Way};
        // Make heap node insertable, ruining the heap it is currently in.
        edgep->m_edge_Heap_गाठ[way].yank();
        // Add the edge as new
        addRelativeEdge<N_Way>(edgep);
    }
    ढाँचा <GraphWay::en N_Way>
    शून्य removeRelativeEdge(MTaskEdge* edgep) {
        constexpr GraphWay way{N_Way};
        // Remove from the edge heap
        m_edgeHeap[way].हटाओ(&edgep->m_edge_Heap_गाठ[way]);
    }

    शून्य addDependent(LogicMTask* dependentp) {
        स्थिरांक bool exists = !m_dependents.emplace(dependentp).second;
        UDEBUGONLY(UASSERT(!exists, "Adding existing dependent"););
    }
    शून्य removeDependent(LogicMTask* dependentp) {
        स्थिरांक माप_प्रकार removed = m_dependents.erase(dependentp);
        UDEBUGONLY(UASSERT(removed, "Dependent should have been in set"););
    }
};

//=============================================================================
// OrderMTaskGraph

// The graph of LogicMTask vertices and MTaskEdge edges, used during multi-threaded scheduling.
class OrderMTaskGraph final : सार्वजनिक V3Graph {
    // MEMBERS
    OrderMoveGraph& m_moveGraph;  // The OrderMoveGraph this graph is built from
    LogicMTask* स्थिरांक m_entryp;  // The singular entry point vertex
    LogicMTask* स्थिरांक m_exitp;  // The singular exit point vertex

    स्थिरांक bool m_slowAsserts;  // Take extra time to validate the graph ('--debug-partition')

    // Critical path propagation state. Scratch only: the heap is empty, and no MTask is pending,
    // between calls to 'propagate'. The node pool persists to recycle the heap nodes.
    PropagatePendingHeap m_pendingHeap;  // Heap of MTasks pending a critical path update
    PoolAllocator<PropagatePendingHeap::गाठ_2> m_pending_गाठ_Pool;  // Allocator for the heap nodes

    // Generation counter, e.g. for marking the MTasks visited by algorithms
    uint64_t m_currentGeneration = 0;

    // CONSTRUCTOR
    explicit OrderMTaskGraph(OrderMoveGraph& moveGraph);  // Used by build(), hence private
    VL_UNCOPYABLE(OrderMTaskGraph);
    VL_UNMOVABLE(OrderMTaskGraph);

    // METHODS

    bool pathExistsImpl(LogicMTask* fromp, LogicMTask* शीर्ष, स्थिरांक MTaskEdge* excludedEdgep);

    // Bring the critical paths of all MTasks wayward of 'mtaskp' in direction N_Way, and those
    // cached in the edge heaps on the way, up to date. Call after mutating the graph such that
    // only MTasks wayward of 'mtaskp' can have a stale critical path, and the critical path of
    // 'mtaskp' itself is already correct. 'mtaskp' is read, but never modified.
    //
    // Note critical paths can only ever grow: those cached in the edge heaps are heap keys, and a
    // heap key can be increased in place, but not decreased.
    ढाँचा <GraphWay::en N_Way>
    शून्य propagate(LogicMTask* mtaskp) {
        ++m_currentGeneration;
        propagate_ढकला<N_Way>(mtaskp);
        propagateResolve<N_Way>();
    }
    // Push the inclusive critical path of 'mtaskp' onto each of its wayward relatives, and add any
    // relative left with a stale critical path to the pending heap (out of line below)
    ढाँचा <GraphWay::en N_Way>
    शून्य propagate_ढकला(LogicMTask* mtaskp);
    // Resolve all pending critical path increases (out of line below)
    ढाँचा <GraphWay::en N_Way>
    शून्य propagateResolve();
    // Part of 'validate'
    ढाँचा <GraphWay::en N_Way>
    शून्य validateWay() स्थिरांक;

सार्वजनिक:
    // ACCESSORS
    OrderMoveGraph& moveGraph() स्थिरांक { परत m_moveGraph; }
    LogicMTask* entryp() स्थिरांक { परत m_entryp; }
    LogicMTask* exitp() स्थिरांक { परत m_exitp; }
    bool slowAsserts() स्थिरांक { परत m_slowAsserts; }

    // METHODS
    uint64_t totalCost() स्थिरांक;  // O(V), called once

    // True if there's a path from 'fromp' to 'top' excluding 'excludedEdgep', false otherwise.
    // 'excludedEdgep' may be nullptr in which case no edge is excluded. If 'excludedEdgep' is
    // non-nullptr it must connect fromp and top.
    bool pathExists(LogicMTask* fromp, LogicMTask* शीर्ष, स्थिरांक MTaskEdge* excludedEdgep) {
        ++m_currentGeneration;
        परत pathExistsImpl(fromp, शीर्ष, excludedEdgep);
    }

    // Add an edge to the graph, update impacted critical paths
    शून्य addEdge(LogicMTask* fromp, LogicMTask* शीर्ष);

    // Merge 'donorp' into 'recipientp': move the contents and all edges of 'donorp' onto
    // 'recipientp', update impacted critical paths, then delete 'donorp'. The edge connecting the
    // two (if any) becomes internal to the merged MTask and is deleted, as is one of each pair of
    // edges the two have to a common relative. Note this deletes edges, so the caller must have
    // released any auxiliary data it attached to them via their user pointer.
    शून्य mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp);

    // Remove all transitive edges. (This deliberately hides V3Graph::removeTransitiveEdges,
    // which would leave the auxiliary data structures stale.)
    // cppcheck-suppress duplInheritedMember
    शून्य removeTransitiveEdges();

    // Remove all MTasks holding no logic (except for entry and exit, which are kept even if
    // empty), connecting their predecessors directly to their successors.
    शून्य removeEmptyMTasks();

    // Do an expensive check that the maintained critical paths, including the ones cached in the
    // edge heaps, match those implied by the current edges of the graph, and that auxiliary data
    // structures are consistent.
    शून्य validate() स्थिरांक;

    // STATIC METHODS
    // Build an MTask graph from 'moveGraph'
    स्थिर std::unique_ptr<OrderMTaskGraph> build(OrderMoveGraph& moveGraph) VL_MT_DISABLED;
    // Fix data hazards in the MTask graph
    स्थिर शून्य fixDataHazards(OrderMTaskGraph& mtaskGraph) VL_MT_DISABLED;
    // Coarsen the MTask graph by merging MTasks until the given critical-path limit is reached
    स्थिर शून्य contract(OrderMTaskGraph& mtaskGraph, uint64_t scoreLimit) VL_MT_DISABLED;
};

//=============================================================================
// MTaskEdge method definitions (need the full definition of LogicMTask)

MTaskEdge::MTaskEdge(OrderMTaskGraph* graphp, LogicMTask* fromp, LogicMTask* शीर्ष)
    : V3GraphEdge{graphp, fromp, शीर्ष, 1} {
    fromp->addDependent(शीर्ष);
    fromp->addRelativeEdge<GraphWay::FORWARD>(this);
    शीर्ष->addRelativeEdge<GraphWay::REVERSE>(this);
}

ढाँचा <GraphWay::en N_Way>
LogicMTask* MTaskEdge::furtherMTaskp() स्थिरांक {
    परत static_cast<LogicMTask*>(this->furtherp<N_Way>());
}
LogicMTask* MTaskEdge::fromMTaskp() स्थिरांक { परत static_cast<LogicMTask*>(fromp()); }
LogicMTask* MTaskEdge::toMTaskp() स्थिरांक { परत static_cast<LogicMTask*>(शीर्ष()); }

#endif  // Guard
