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

#यदि_अपरिभाषितम् VERILATOR_V3ORDERMTASKGRAPH_H_
#परिभाषय VERILATOR_V3ORDERMTASKGRAPH_H_

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
class क्रमः_MTask_Graph;

//=============================================================================
// MTaskEdge graph edges are stored in a PairingHeap in each LogicMTask they
// connect to, sorted by critical path through that edge (and id for stability).

संरचना Edge_कुञ्चिका final {
    uint64_t m_cp;  // The inclusive critical path of the further MTask of the edge
    uint32_t m_id;  // The ID of the further MTask, for stable comparison
    शून्य increase(uint64_t cp) {
        UDEBUGONLY(UASSERT(cp >= m_cp, "Must increase"););
        m_cp = cp;
    }
    // Sort first by critical path, then by ID
    bool संक्रियकः<(ध्रुव Edge_कुञ्चिका& other) ध्रुव {
        यदि (m_cp != other.m_cp) प्रत्यागम m_cp < other.m_cp;
        प्रत्यागम m_id < other.m_id;
    }
};

using EdgeHeap = PairingHeap<Edge_कुञ्चिका>;

//=============================================================================
// LogicMTasks are stored in a PairingHeap during critical path update propagation.

संरचना Propagate_Pending_कुञ्चिका final {
    uint64_t m_increment;  // The amount the critical path of the MTask will grow by
    uint32_t m_id;  // The ID of the MTask, for stable comparison
    LogicMTask* m_mtaskp;  // The MTask the heap entry corresponds to
    शून्य increase(uint64_t increment) {
        UDEBUGONLY(UASSERT(increment >= m_increment, "Must increase"););
        m_increment = increment;
    }
    // Sort first by increment, then by ID
    bool संक्रियकः<(ध्रुव Propagate_Pending_कुञ्चिका& other) ध्रुव {
        यदि (m_increment != other.m_increment) प्रत्यागम m_increment < other.m_increment;
        प्रत्यागम m_id < other.m_id;
    }
};

using PropagatePendingHeap = PairingHeap<Propagate_Pending_कुञ्चिका>;

//=============================================================================
// GraphEdge for the MTask graph

class MTaskEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(MTaskEdge, V3GraphEdge)

    मित्रम् class LogicMTask;
    मित्रम् class क्रमः_MTask_Graph;

    // MEMBERS
    // This edge can be in 2 EdgeHeaps, one forward and one reverse. We allocate the heap nodes
    // directly within the edge as they are always required and this makes association cheap.
    std::array<EdgeHeap::ग्रन्थिः_2, GraphWay::अङ्कः_WAYS> m_edge_Heap_ग्रन्थिः;

    // CONSTRUCTORS
    // Private, so edges can only be created via OrderMTaskGraph, which also updates the critical
    // paths on graph mutation.
    अन्तःस्थ MTaskEdge(क्रमः_MTask_Graph* graphp, LogicMTask* fromp, LogicMTask* शिखरम्);

सार्वजनिक:
    VL_UNCOPYABLE(MTaskEdge);
    VL_UNMOVABLE(MTaskEdge);

    // METHODS
    प्रतिरूपम् <GraphWay::en N_Way>
    अन्तःस्थ LogicMTask* furtherMTaskp() ध्रुव;
    अन्तःस्थ LogicMTask* fromMTaskp() ध्रुव;
    अन्तःस्थ LogicMTask* गन्तव्यम्_MTaskp() ध्रुव;

    uint64_t cachedCp(GraphWay way) ध्रुव { प्रत्यागम m_edge_Heap_ग्रन्थिः[way].कुञ्चिका().m_cp; }
    uint32_t cachedId(GraphWay way) ध्रुव { प्रत्यागम m_edge_Heap_ग्रन्थिः[way].कुञ्चिका().m_id; }
    // Convert from the address of the m_edgeHeapNode[way] in an MTaskEdge back to the MTaskEdge
    स्थिर ध्रुव MTaskEdge* गन्तव्यम्_MTask_Edge(GraphWay way, ध्रुव EdgeHeap::ग्रन्थिः_2* nodep) {
        ध्रुव आकार_t विस्थापनम् = VL_OFFSETOF(MTaskEdge, m_edge_Heap_ग्रन्थिः[way]);
        प्रत्यागम reinterpret_cast<ध्रुव MTaskEdge*>(reinterpret_cast<uintptr_t>(nodep) - विस्थापनम्);
    }
};

//=============================================================================
// LogicMTask

class LogicMTask final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(LogicMTask, V3GraphVertex)

    मित्रम् class MTaskEdge;
    मित्रम् class क्रमः_MTask_Graph;

    // MEMBERS

    // List of OrderMoveVertex's assigned to this mtask. LogicMTask does not own the
    // OrderMoveVertex objects, we merely keep them in a list here.
    क्रमः_Move_Vertex::List m_mVertices;

    स्थिर uint32_t s_अग्र_Id;  // Next ID number to use
    ध्रुव uint32_t m_id = s_अग्र_Id++;  // Unique LogicMTask ID number for stable comparison

    // Cost estimate for this LogicMTask, derived from V3InstrCount, in abstract time units.
    // Cost estimates and critical path lengths are bounded by number of AstNodes * constant,
    // will run out of host memory storing the Ast way before they can overflow.
    uint64_t m_cost = 0;

    // Critical path in each direction: going FORWARD from graph-start to the start of this vertex,
    // and going REVERSE from graph-exit to the end of this vertex. Exclusive of the cost of this
    // vertex itself, see cpInclusive() for the value including it.
    std::array<uint64_t, GraphWay::अङ्कः_WAYS> m_cpExclusive = {0, 0};

    // The MTasks this MTask has an out-edge to, so checking for an existing edge is O(1)
    std::unordered_समुच्चयः<LogicMTask*> m_dependents;
    // Store the out/in edges in a heaps sorted by the critical path length through each edge
    std::array<EdgeHeap, GraphWay::अङ्कः_WAYS> m_edgeHeap;

    // Count "generations" which are just operations that scan through the
    // graph. We'll mark each node with the last generation that scanned
    // it. We can use this to avoid recursing through the same node twice
    // while searching for a path.
    uint64_t m_generation = 0;

    // Scratch pointer used only by the critical path propagation in OrderMTaskGraph: this MTask's
    // node in the pending heap, or nullptr if this MTask is not pending.
    PropagatePendingHeap::ग्रन्थिः_2* m_propagateHeapNodep = nullptr;

सार्वजनिक:
    // CONSTRUCTORS
    LogicMTask(क्रमः_MTask_Graph& graph, क्रमः_Move_Vertex* mVtxp) VL_MT_DISABLED;
    VL_UNCOPYABLE(LogicMTask);
    VL_UNMOVABLE(LogicMTask);

    // METHODS
    क्रमः_Move_Vertex::List& vertexList() { प्रत्यागम m_mVertices; }
    uint32_t id() ध्रुव { प्रत्यागम m_id; }
    bool संक्रियकः<(ध्रुव LogicMTask& rhs) ध्रुव { प्रत्यागम id() < rhs.id(); }

    uint64_t cost() ध्रुव VL_MT_SAFE { प्रत्यागम m_cost; }
    प्रतिरूपम् <GraphWay::en N_Way>
    uint64_t cpExclusive() ध्रुव {
        प्रत्यागम m_cpExclusive[N_Way];
    }
    प्रतिरूपम् <GraphWay::en N_Way>
    uint64_t cpInclusive() ध्रुव {
        प्रत्यागम m_cpExclusive[N_Way] + m_cost;
    }
    // The critical path of this MTask without considering the given edge.
    प्रतिरूपम् <GraphWay::en N_Way>
    uint64_t cpExclusiveWithout(ध्रुव V3GraphEdge* edgep) ध्रुव {
        ध्रुव GraphWay way{N_Way};
        ध्रुव GraphWay inv = way.invert();
        UDEBUGONLY(UASSERT(edgep->furtherp<N_Way>() == this,
                           "In cpExclusiveWithout(), 'edgep' must further to 'this'"););
        // At most two edges need to be considered: the critical path, if that is not via 'edgep',
        // or the second-worst path, if the critical path is via 'edgep'.
        ध्रुव EdgeHeap& edgeHeap = m_edgeHeap[inv];
        // Pick up the critical path edge
        ध्रुव EdgeHeap::ग्रन्थिः_2* ध्रुव maxp = edgeHeap.max();
        UDEBUGONLY(UASSERT(maxp, "Edge not in heap"););
        // If 'edgep' is not the critical path edge, return its critical path
        यदि (MTaskEdge::गन्तव्यम्_MTask_Edge(inv, maxp) != edgep) प्रत्यागम maxp->कुञ्चिका().m_cp;
        // Otherwise return the second-worst path, if there is one
        ध्रुव EdgeHeap::ग्रन्थिः_2* ध्रुव secp = edgeHeap.secondMax();
        यदि (!secp) प्रत्यागम 0;
        प्रत्यागम secp->कुञ्चिका().m_cp;
    }

    bool has_Edge_गन्तव्यम्(LogicMTask* dependentp) ध्रुव { प्रत्यागम m_dependents.गणनम्(dependentp); }

    // For Graphviz dumps only
    std::string name() ध्रुव override VL_MT_STABLE {
        std::ostringstream बहिर्गमनम्;
        बहिर्गमनम् << "mt" << m_id  //
            << " | cpFwd " << m_cpExclusive[GraphWay::FORWARD]  //
            << " | cost " << cost()  //
            << " | cpRev " << m_cpExclusive[GraphWay::REVERSE];
        प्रत्यागम बहिर्गमनम्.str();
    }

गुप्त:
    // Following only used by OrderMTaskGraph, which maintains cached CPs and graph invariants.

    प्रतिरूपम् <GraphWay::en N_Way>
    uint64_t cp_Exclusive_From_धाराः() ध्रुव {
        constexpr GraphWay inv = GraphWay{N_Way}.invert();
        ध्रुव EdgeHeap::ग्रन्थिः_2* ध्रुव maxp = m_edgeHeap[inv].max();
        प्रत्यागम maxp ? maxp->कुञ्चिका().m_cp : 0;
    }
    प्रतिरूपम् <GraphWay::en N_Way>
    शून्य cpExclusive(uint64_t cp) {
        m_cpExclusive[N_Way] = cp;
    }

    प्रतिरूपम् <GraphWay::en N_Way>
    शून्य सङ्कलय_Relative_Edge(MTaskEdge* edgep) {
        constexpr GraphWay way{N_Way};
        constexpr GraphWay inv = way.invert();
        // Add to the edge heap
        LogicMTask* ध्रुव relativep = edgep->furtherMTaskp<N_Way>();
        // Value is the !way inclusive cp of the relative
        ध्रुव uint64_t cp = relativep->cpInclusive<inv>();
        m_edgeHeap[way].निवेशय(&edgep->m_edge_Heap_ग्रन्थिः[way], {cp, relativep->id()});
    }
    प्रतिरूपम् <GraphWay::en N_Way>
    शून्य stealRelativeEdge(MTaskEdge* edgep) {
        constexpr GraphWay way{N_Way};
        // Make heap node insertable, ruining the heap it is currently in.
        edgep->m_edge_Heap_ग्रन्थिः[way].yank();
        // Add the edge as new
        सङ्कलय_Relative_Edge<N_Way>(edgep);
    }
    प्रतिरूपम् <GraphWay::en N_Way>
    शून्य removeRelativeEdge(MTaskEdge* edgep) {
        constexpr GraphWay way{N_Way};
        // Remove from the edge heap
        m_edgeHeap[way].remove(&edgep->m_edge_Heap_ग्रन्थिः[way]);
    }

    शून्य सङ्कलय_Dependent(LogicMTask* dependentp) {
        ध्रुव bool exists = !m_dependents.emplace(dependentp).second;
        UDEBUGONLY(UASSERT(!exists, "Adding existing dependent"););
    }
    शून्य removeDependent(LogicMTask* dependentp) {
        ध्रुव आकार_t removed = m_dependents.erase(dependentp);
        UDEBUGONLY(UASSERT(removed, "Dependent should have been in set"););
    }
};

//=============================================================================
// OrderMTaskGraph

// The graph of LogicMTask vertices and MTaskEdge edges, used during multi-threaded scheduling.
class क्रमः_MTask_Graph final : सार्वजनिक V3Graph {
    // MEMBERS
    क्रमः_Move_Graph& m_moveGraph;  // The OrderMoveGraph this graph is built from
    LogicMTask* ध्रुव m_entryp;  // The singular entry point vertex
    LogicMTask* ध्रुव m_exitp;  // The singular exit point vertex

    ध्रुव bool m_slowAsserts;  // Take extra time to validate the graph ('--debug-partition')

    // Critical path propagation state. Scratch only: the heap is empty, and no MTask is pending,
    // between calls to 'propagate'. The node pool persists to recycle the heap nodes.
    PropagatePendingHeap m_pendingHeap;  // Heap of MTasks pending a critical path update
    PoolAllocator<PropagatePendingHeap::ग्रन्थिः_2> m_pending_ग्रन्थिः_Pool;  // Allocator for the heap nodes

    // Generation counter, e.g. for marking the MTasks visited by algorithms
    uint64_t m_currentGeneration = 0;

    // CONSTRUCTOR
    explicit क्रमः_MTask_Graph(क्रमः_Move_Graph& moveGraph);  // Used by build(), hence private
    VL_UNCOPYABLE(क्रमः_MTask_Graph);
    VL_UNMOVABLE(क्रमः_MTask_Graph);

    // METHODS

    bool मार्गः_Exists_Impl(LogicMTask* fromp, LogicMTask* शिखरम्, ध्रुव MTaskEdge* excludedEdgep);

    // Bring the critical paths of all MTasks wayward of 'mtaskp' in direction N_Way, and those
    // cached in the edge heaps on the way, up to date. Call after mutating the graph such that
    // only MTasks wayward of 'mtaskp' can have a stale critical path, and the critical path of
    // 'mtaskp' itself is already correct. 'mtaskp' is read, but never modified.
    //
    // Note critical paths can only ever grow: those cached in the edge heaps are heap keys, and a
    // heap key can be increased in place, but not decreased.
    प्रतिरूपम् <GraphWay::en N_Way>
    शून्य propagate(LogicMTask* mtaskp) {
        ++m_currentGeneration;
        propagate_प्रक्षिप<N_Way>(mtaskp);
        propagateResolve<N_Way>();
    }
    // Push the inclusive critical path of 'mtaskp' onto each of its wayward relatives, and add any
    // relative left with a stale critical path to the pending heap (out of line below)
    प्रतिरूपम् <GraphWay::en N_Way>
    शून्य propagate_प्रक्षिप(LogicMTask* mtaskp);
    // Resolve all pending critical path increases (out of line below)
    प्रतिरूपम् <GraphWay::en N_Way>
    शून्य propagateResolve();
    // Part of 'validate'
    प्रतिरूपम् <GraphWay::en N_Way>
    शून्य validateWay() ध्रुव;

सार्वजनिक:
    // ACCESSORS
    क्रमः_Move_Graph& moveGraph() ध्रुव { प्रत्यागम m_moveGraph; }
    LogicMTask* entryp() ध्रुव { प्रत्यागम m_entryp; }
    LogicMTask* exitp() ध्रुव { प्रत्यागम m_exitp; }
    bool slowAsserts() ध्रुव { प्रत्यागम m_slowAsserts; }

    // METHODS
    uint64_t योग_Cost() ध्रुव;  // O(V), called once

    // True if there's a path from 'fromp' to 'top' excluding 'excludedEdgep', false otherwise.
    // 'excludedEdgep' may be nullptr in which case no edge is excluded. If 'excludedEdgep' is
    // non-nullptr it must connect fromp and top.
    bool मार्गः_Exists(LogicMTask* fromp, LogicMTask* शिखरम्, ध्रुव MTaskEdge* excludedEdgep) {
        ++m_currentGeneration;
        प्रत्यागम मार्गः_Exists_Impl(fromp, शिखरम्, excludedEdgep);
    }

    // Add an edge to the graph, update impacted critical paths
    शून्य सङ्कलय_Edge(LogicMTask* fromp, LogicMTask* शिखरम्);

    // Merge 'donorp' into 'recipientp': move the contents and all edges of 'donorp' onto
    // 'recipientp', update impacted critical paths, then delete 'donorp'. The edge connecting the
    // two (if any) becomes internal to the merged MTask and is deleted, as is one of each pair of
    // edges the two have to a common relative. Note this deletes edges, so the caller must have
    // released any auxiliary data it attached to them via their user pointer.
    शून्य mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp);

    // Remove all transitive edges. (This deliberately hides V3Graph::removeTransitiveEdges,
    // which would leave the auxiliary data structures stale.)
    // cppcheck-suppress duplInheritedMember
    शून्य remove_संक्रामकम्_धाराः();

    // Remove all MTasks holding no logic (except for entry and exit, which are kept even if
    // empty), connecting their predecessors directly to their successors.
    शून्य remove_रिक्तानि_MTasks();

    // Do an expensive check that the maintained critical paths, including the ones cached in the
    // edge heaps, match those implied by the current edges of the graph, and that auxiliary data
    // structures are consistent.
    शून्य validate() ध्रुव;

    // STATIC METHODS
    // Build an MTask graph from 'moveGraph'
    स्थिर std::unique_ptr<क्रमः_MTask_Graph> build(क्रमः_Move_Graph& moveGraph) VL_MT_DISABLED;
    // Fix data hazards in the MTask graph
    स्थिर शून्य fix_दत्त_Hazards(क्रमः_MTask_Graph& mtaskGraph) VL_MT_DISABLED;
    // Coarsen the MTask graph by merging MTasks until the given critical-path limit is reached
    स्थिर शून्य contract(क्रमः_MTask_Graph& mtaskGraph, uint64_t score_सीमा) VL_MT_DISABLED;
};

//=============================================================================
// MTaskEdge method definitions (need the full definition of LogicMTask)

MTaskEdge::MTaskEdge(क्रमः_MTask_Graph* graphp, LogicMTask* fromp, LogicMTask* शिखरम्)
    : V3GraphEdge{graphp, fromp, शिखरम्, 1} {
    fromp->सङ्कलय_Dependent(शिखरम्);
    fromp->सङ्कलय_Relative_Edge<GraphWay::FORWARD>(this);
    शिखरम्->सङ्कलय_Relative_Edge<GraphWay::REVERSE>(this);
}

प्रतिरूपम् <GraphWay::en N_Way>
LogicMTask* MTaskEdge::furtherMTaskp() ध्रुव {
    प्रत्यागम static_cast<LogicMTask*>(this->furtherp<N_Way>());
}
LogicMTask* MTaskEdge::fromMTaskp() ध्रुव { प्रत्यागम static_cast<LogicMTask*>(fromp()); }
LogicMTask* MTaskEdge::गन्तव्यम्_MTaskp() ध्रुव { प्रत्यागम static_cast<LogicMTask*>(शिखरम्()); }

#यदि_अन्तः  // Guard
