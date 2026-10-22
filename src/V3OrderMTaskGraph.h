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

#నిర్వచితం_కాకపోతే VERILATOR_V3ORDERMTASKGRAPH_H_
#నిర్వచించు VERILATOR_V3ORDERMTASKGRAPH_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3OrderMoveGraph.h"
#అన్తర్భావ "V3PairingHeap.h"
#అన్తర్భావ "V3PoolAllocator.h"

#అన్తర్భావ <array>
#అన్తర్భావ <memory>
#అన్తర్భావ <sstream>
#అన్తర్భావ <unordered_set>

class LogicMTask;
class క్రమం_MTask_లేఖాచిత్రం;

//=============================================================================
// MTaskEdge graph edges are stored in a PairingHeap in each LogicMTask they
// connect to, sorted by critical path through that edge (and id for stability).

నిర్మాణం అంచు_కీలకం final {
    uint64_t m_cp;  // The inclusive critical path of the further MTask of the edge
    uint32_t m_id;  // The ID of the further MTask, for stable comparison
    శూన్య increase(uint64_t cp) {
        UDEBUGONLY(UASSERT(cp >= m_cp, "Must increase"););
        m_cp = cp;
    }
    // Sort first by critical path, then by ID
    bool క్రియాకారకం<(స్థిరరాశి అంచు_కీలకం& other) స్థిరరాశి {
        ఒకవేళ (m_cp != other.m_cp) తిరిగి m_cp < other.m_cp;
        తిరిగి m_id < other.m_id;
    }
};

using అంచు_కుప్ప = Pairing_కుప్ప<అంచు_కీలకం>;

//=============================================================================
// LogicMTasks are stored in a PairingHeap during critical path update propagation.

నిర్మాణం Propagate_Pending_కీలకం final {
    uint64_t m_increment;  // The amount the critical path of the MTask will grow by
    uint32_t m_id;  // The ID of the MTask, for stable comparison
    LogicMTask* m_mtaskp;  // The MTask the heap entry corresponds to
    శూన్య increase(uint64_t increment) {
        UDEBUGONLY(UASSERT(increment >= m_increment, "Must increase"););
        m_increment = increment;
    }
    // Sort first by increment, then by ID
    bool క్రియాకారకం<(స్థిరరాశి Propagate_Pending_కీలకం& other) స్థిరరాశి {
        ఒకవేళ (m_increment != other.m_increment) తిరిగి m_increment < other.m_increment;
        తిరిగి m_id < other.m_id;
    }
};

using Propagate_Pending_కుప్ప = Pairing_కుప్ప<Propagate_Pending_కీలకం>;

//=============================================================================
// GraphEdge for the MTask graph

class MTask_అంచు final : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    VL_RTTI_IMPL(MTask_అంచు, V3_లేఖాచిత్రం_అంచు)

    మిత్ర class LogicMTask;
    మిత్ర class క్రమం_MTask_లేఖాచిత్రం;

    // MEMBERS
    // This edge can be in 2 EdgeHeaps, one forward and one reverse. We allocate the heap nodes
    // directly within the edge as they are always required and this makes association cheap.
    std::array<అంచు_కుప్ప::కణుపు_2, లేఖాచిత్రం_Way::అంకె_WAYS> m_అంచు_కుప్ప_కణుపు;

    // CONSTRUCTORS
    // Private, so edges can only be created via OrderMTaskGraph, which also updates the critical
    // paths on graph mutation.
    అంతఃవరుస MTask_అంచు(క్రమం_MTask_లేఖాచిత్రం* graphp, LogicMTask* fromp, LogicMTask* శిఖరం);

బహిరంగ:
    VL_UNCOPYABLE(MTask_అంచు);
    VL_UNMOVABLE(MTask_అంచు);

    // METHODS
    మూస <లేఖాచిత్రం_Way::en N_Way>
    అంతఃవరుస LogicMTask* furtherMTaskp() స్థిరరాశి;
    అంతఃవరుస LogicMTask* fromMTaskp() స్థిరరాశి;
    అంతఃవరుస LogicMTask* గమ్యం_MTaskp() స్థిరరాశి;

    uint64_t cachedCp(లేఖాచిత్రం_Way way) స్థిరరాశి { తిరిగి m_అంచు_కుప్ప_కణుపు[way].కీలకం().m_cp; }
    uint32_t cachedId(లేఖాచిత్రం_Way way) స్థిరరాశి { తిరిగి m_అంచు_కుప్ప_కణుపు[way].కీలకం().m_id; }
    // Convert from the address of the m_edgeHeapNode[way] in an MTaskEdge back to the MTaskEdge
    స్థిర స్థిరరాశి MTask_అంచు* గమ్యం_MTask_అంచు(లేఖాచిత్రం_Way way, స్థిరరాశి అంచు_కుప్ప::కణుపు_2* nodep) {
        స్థిరరాశి కొలత_t స్థానభ్రంశం = VL_OFFSETOF(MTask_అంచు, m_అంచు_కుప్ప_కణుపు[way]);
        తిరిగి reinterpret_cast<స్థిరరాశి MTask_అంచు*>(reinterpret_cast<uintptr_t>(nodep) - స్థానభ్రంశం);
    }
};

//=============================================================================
// LogicMTask

class LogicMTask final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(LogicMTask, V3_లేఖాచిత్రం_శీర్షం)

    మిత్ర class MTask_అంచు;
    మిత్ర class క్రమం_MTask_లేఖాచిత్రం;

    // MEMBERS

    // List of OrderMoveVertex's assigned to this mtask. LogicMTask does not own the
    // OrderMoveVertex objects, we merely keep them in a list here.
    క్రమం_Move_శీర్షం::List m_mVertices;

    స్థిర uint32_t s_తదుపరి_Id;  // Next ID number to use
    స్థిరరాశి uint32_t m_id = s_తదుపరి_Id++;  // Unique LogicMTask ID number for stable comparison

    // Cost estimate for this LogicMTask, derived from V3InstrCount, in abstract time units.
    // Cost estimates and critical path lengths are bounded by number of AstNodes * constant,
    // will run out of host memory storing the Ast way before they can overflow.
    uint64_t m_ఖర్చు = 0;

    // Critical path in each direction: going FORWARD from graph-start to the start of this vertex,
    // and going REVERSE from graph-exit to the end of this vertex. Exclusive of the cost of this
    // vertex itself, see cpInclusive() for the value including it.
    std::array<uint64_t, లేఖాచిత్రం_Way::అంకె_WAYS> m_cpExclusive = {0, 0};

    // The MTasks this MTask has an out-edge to, so checking for an existing edge is O(1)
    std::unordered_సమితి<LogicMTask*> m_dependents;
    // Store the out/in edges in a heaps sorted by the critical path length through each edge
    std::array<అంచు_కుప్ప, లేఖాచిత్రం_Way::అంకె_WAYS> m_అంచు_కుప్ప;

    // Count "generations" which are just operations that scan through the
    // graph. We'll mark each node with the last generation that scanned
    // it. We can use this to avoid recursing through the same node twice
    // while searching for a path.
    uint64_t m_generation = 0;

    // Scratch pointer used only by the critical path propagation in OrderMTaskGraph: this MTask's
    // node in the pending heap, or nullptr if this MTask is not pending.
    Propagate_Pending_కుప్ప::కణుపు_2* m_propagate_కుప్ప_Nodep = nullptr;

బహిరంగ:
    // CONSTRUCTORS
    LogicMTask(క్రమం_MTask_లేఖాచిత్రం& లేఖాచిత్రం, క్రమం_Move_శీర్షం* mVtxp) VL_MT_DISABLED;
    VL_UNCOPYABLE(LogicMTask);
    VL_UNMOVABLE(LogicMTask);

    // METHODS
    క్రమం_Move_శీర్షం::List& శీర్షం_List() { తిరిగి m_mVertices; }
    uint32_t id() స్థిరరాశి { తిరిగి m_id; }
    bool క్రియాకారకం<(స్థిరరాశి LogicMTask& rhs) స్థిరరాశి { తిరిగి id() < rhs.id(); }

    uint64_t ఖర్చు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_ఖర్చు; }
    మూస <లేఖాచిత్రం_Way::en N_Way>
    uint64_t cpExclusive() స్థిరరాశి {
        తిరిగి m_cpExclusive[N_Way];
    }
    మూస <లేఖాచిత్రం_Way::en N_Way>
    uint64_t cpInclusive() స్థిరరాశి {
        తిరిగి m_cpExclusive[N_Way] + m_ఖర్చు;
    }
    // The critical path of this MTask without considering the given edge.
    మూస <లేఖాచిత్రం_Way::en N_Way>
    uint64_t cpExclusiveWithout(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* edgep) స్థిరరాశి {
        స్థిరరాశి లేఖాచిత్రం_Way way{N_Way};
        స్థిరరాశి లేఖాచిత్రం_Way inv = way.invert();
        UDEBUGONLY(UASSERT(edgep->furtherp<N_Way>() == this,
                           "In cpExclusiveWithout(), 'edgep' must further to 'this'"););
        // At most two edges need to be considered: the critical path, if that is not via 'edgep',
        // or the second-worst path, if the critical path is via 'edgep'.
        స్థిరరాశి అంచు_కుప్ప& అంచు_కుప్ప_2 = m_అంచు_కుప్ప[inv];
        // Pick up the critical path edge
        స్థిరరాశి అంచు_కుప్ప::కణుపు_2* స్థిరరాశి maxp = అంచు_కుప్ప_2.max();
        UDEBUGONLY(UASSERT(maxp, "Edge not in heap"););
        // If 'edgep' is not the critical path edge, return its critical path
        ఒకవేళ (MTask_అంచు::గమ్యం_MTask_అంచు(inv, maxp) != edgep) తిరిగి maxp->కీలకం().m_cp;
        // Otherwise return the second-worst path, if there is one
        స్థిరరాశి అంచు_కుప్ప::కణుపు_2* స్థిరరాశి secp = అంచు_కుప్ప_2.రెండవ_Max();
        ఒకవేళ (!secp) తిరిగి 0;
        తిరిగి secp->కీలకం().m_cp;
    }

    bool has_అంచు_గమ్యం(LogicMTask* dependentp) స్థిరరాశి { తిరిగి m_dependents.లెక్క(dependentp); }

    // For Graphviz dumps only
    std::string name() స్థిరరాశి override VL_MT_STABLE {
        std::ostringstream బహిర్గతం;
        బహిర్గతం << "mt" << m_id  //
            << " | cpFwd " << m_cpExclusive[లేఖాచిత్రం_Way::FORWARD]  //
            << " | cost " << ఖర్చు()  //
            << " | cpRev " << m_cpExclusive[లేఖాచిత్రం_Way::తిరగేయి_2];
        తిరిగి బహిర్గతం.str();
    }

అంతరంగ:
    // Following only used by OrderMTaskGraph, which maintains cached CPs and graph invariants.

    మూస <లేఖాచిత్రం_Way::en N_Way>
    uint64_t cp_Exclusive_From_అంచులు() స్థిరరాశి {
        constexpr లేఖాచిత్రం_Way inv = లేఖాచిత్రం_Way{N_Way}.invert();
        స్థిరరాశి అంచు_కుప్ప::కణుపు_2* స్థిరరాశి maxp = m_అంచు_కుప్ప[inv].max();
        తిరిగి maxp ? maxp->కీలకం().m_cp : 0;
    }
    మూస <లేఖాచిత్రం_Way::en N_Way>
    శూన్య cpExclusive(uint64_t cp) {
        m_cpExclusive[N_Way] = cp;
    }

    మూస <లేఖాచిత్రం_Way::en N_Way>
    శూన్య కూడు_Relative_అంచు(MTask_అంచు* edgep) {
        constexpr లేఖాచిత్రం_Way way{N_Way};
        constexpr లేఖాచిత్రం_Way inv = way.invert();
        // Add to the edge heap
        LogicMTask* స్థిరరాశి relativep = edgep->furtherMTaskp<N_Way>();
        // Value is the !way inclusive cp of the relative
        స్థిరరాశి uint64_t cp = relativep->cpInclusive<inv>();
        m_అంచు_కుప్ప[way].చొప్పించు(&edgep->m_అంచు_కుప్ప_కణుపు[way], {cp, relativep->id()});
    }
    మూస <లేఖాచిత్రం_Way::en N_Way>
    శూన్య steal_Relative_అంచు(MTask_అంచు* edgep) {
        constexpr లేఖాచిత్రం_Way way{N_Way};
        // Make heap node insertable, ruining the heap it is currently in.
        edgep->m_అంచు_కుప్ప_కణుపు[way].yank();
        // Add the edge as new
        కూడు_Relative_అంచు<N_Way>(edgep);
    }
    మూస <లేఖాచిత్రం_Way::en N_Way>
    శూన్య remove_Relative_అంచు(MTask_అంచు* edgep) {
        constexpr లేఖాచిత్రం_Way way{N_Way};
        // Remove from the edge heap
        m_అంచు_కుప్ప[way].remove(&edgep->m_అంచు_కుప్ప_కణుపు[way]);
    }

    శూన్య కూడు_Dependent(LogicMTask* dependentp) {
        స్థిరరాశి bool exists = !m_dependents.emplace(dependentp).రెండవ;
        UDEBUGONLY(UASSERT(!exists, "Adding existing dependent"););
    }
    శూన్య removeDependent(LogicMTask* dependentp) {
        స్థిరరాశి కొలత_t removed = m_dependents.erase(dependentp);
        UDEBUGONLY(UASSERT(removed, "Dependent should have been in set"););
    }
};

//=============================================================================
// OrderMTaskGraph

// The graph of LogicMTask vertices and MTaskEdge edges, used during multi-threaded scheduling.
class క్రమం_MTask_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
    // MEMBERS
    క్రమం_Move_లేఖాచిత్రం& m_move_లేఖాచిత్రం;  // The OrderMoveGraph this graph is built from
    LogicMTask* స్థిరరాశి m_entryp;  // The singular entry point vertex
    LogicMTask* స్థిరరాశి m_exitp;  // The singular exit point vertex

    స్థిరరాశి bool m_slowAsserts;  // Take extra time to validate the graph ('--debug-partition')

    // Critical path propagation state. Scratch only: the heap is empty, and no MTask is pending,
    // between calls to 'propagate'. The node pool persists to recycle the heap nodes.
    Propagate_Pending_కుప్ప m_pending_కుప్ప;  // Heap of MTasks pending a critical path update
    PoolAllocator<Propagate_Pending_కుప్ప::కణుపు_2> m_pending_కణుపు_Pool;  // Allocator for the heap nodes

    // Generation counter, e.g. for marking the MTasks visited by algorithms
    uint64_t m_currentGeneration = 0;

    // CONSTRUCTOR
    explicit క్రమం_MTask_లేఖాచిత్రం(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం);  // Used by build(), hence private
    VL_UNCOPYABLE(క్రమం_MTask_లేఖాచిత్రం);
    VL_UNMOVABLE(క్రమం_MTask_లేఖాచిత్రం);

    // METHODS

    bool మార్గం_Exists_Impl(LogicMTask* fromp, LogicMTask* శిఖరం, స్థిరరాశి MTask_అంచు* excludedEdgep);

    // Bring the critical paths of all MTasks wayward of 'mtaskp' in direction N_Way, and those
    // cached in the edge heaps on the way, up to date. Call after mutating the graph such that
    // only MTasks wayward of 'mtaskp' can have a stale critical path, and the critical path of
    // 'mtaskp' itself is already correct. 'mtaskp' is read, but never modified.
    //
    // Note critical paths can only ever grow: those cached in the edge heaps are heap keys, and a
    // heap key can be increased in place, but not decreased.
    మూస <లేఖాచిత్రం_Way::en N_Way>
    శూన్య propagate(LogicMTask* mtaskp) {
        ++m_currentGeneration;
        propagate_నెట్టు<N_Way>(mtaskp);
        propagateResolve<N_Way>();
    }
    // Push the inclusive critical path of 'mtaskp' onto each of its wayward relatives, and add any
    // relative left with a stale critical path to the pending heap (out of line below)
    మూస <లేఖాచిత్రం_Way::en N_Way>
    శూన్య propagate_నెట్టు(LogicMTask* mtaskp);
    // Resolve all pending critical path increases (out of line below)
    మూస <లేఖాచిత్రం_Way::en N_Way>
    శూన్య propagateResolve();
    // Part of 'validate'
    మూస <లేఖాచిత్రం_Way::en N_Way>
    శూన్య validateWay() స్థిరరాశి;

బహిరంగ:
    // ACCESSORS
    క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం() స్థిరరాశి { తిరిగి m_move_లేఖాచిత్రం; }
    LogicMTask* entryp() స్థిరరాశి { తిరిగి m_entryp; }
    LogicMTask* exitp() స్థిరరాశి { తిరిగి m_exitp; }
    bool slowAsserts() స్థిరరాశి { తిరిగి m_slowAsserts; }

    // METHODS
    uint64_t మొత్తం_ఖర్చు() స్థిరరాశి;  // O(V), called once

    // True if there's a path from 'fromp' to 'top' excluding 'excludedEdgep', false otherwise.
    // 'excludedEdgep' may be nullptr in which case no edge is excluded. If 'excludedEdgep' is
    // non-nullptr it must connect fromp and top.
    bool మార్గం_Exists(LogicMTask* fromp, LogicMTask* శిఖరం, స్థిరరాశి MTask_అంచు* excludedEdgep) {
        ++m_currentGeneration;
        తిరిగి మార్గం_Exists_Impl(fromp, శిఖరం, excludedEdgep);
    }

    // Add an edge to the graph, update impacted critical paths
    శూన్య కూడు_అంచు(LogicMTask* fromp, LogicMTask* శిఖరం);

    // Merge 'donorp' into 'recipientp': move the contents and all edges of 'donorp' onto
    // 'recipientp', update impacted critical paths, then delete 'donorp'. The edge connecting the
    // two (if any) becomes internal to the merged MTask and is deleted, as is one of each pair of
    // edges the two have to a common relative. Note this deletes edges, so the caller must have
    // released any auxiliary data it attached to them via their user pointer.
    శూన్య విలీనం_MTasks(LogicMTask* recipientp, LogicMTask* donorp);

    // Remove all transitive edges. (This deliberately hides V3Graph::removeTransitiveEdges,
    // which would leave the auxiliary data structures stale.)
    // cppcheck-suppress duplInheritedMember
    శూన్య remove_సంక్రమణ_అంచులు();

    // Remove all MTasks holding no logic (except for entry and exit, which are kept even if
    // empty), connecting their predecessors directly to their successors.
    శూన్య remove_ఖాళీలు_MTasks();

    // Do an expensive check that the maintained critical paths, including the ones cached in the
    // edge heaps, match those implied by the current edges of the graph, and that auxiliary data
    // structures are consistent.
    శూన్య validate() స్థిరరాశి;

    // STATIC METHODS
    // Build an MTask graph from 'moveGraph'
    స్థిర std::unique_ptr<క్రమం_MTask_లేఖాచిత్రం> build(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం) VL_MT_DISABLED;
    // Fix data hazards in the MTask graph
    స్థిర శూన్య fix_దత్తాంశం_Hazards(క్రమం_MTask_లేఖాచిత్రం& mtask_లేఖాచిత్రం) VL_MT_DISABLED;
    // Coarsen the MTask graph by merging MTasks until the given critical-path limit is reached
    స్థిర శూన్య contract(క్రమం_MTask_లేఖాచిత్రం& mtask_లేఖాచిత్రం, uint64_t score_హద్దు) VL_MT_DISABLED;
};

//=============================================================================
// MTaskEdge method definitions (need the full definition of LogicMTask)

MTask_అంచు::MTask_అంచు(క్రమం_MTask_లేఖాచిత్రం* graphp, LogicMTask* fromp, LogicMTask* శిఖరం)
    : V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, 1} {
    fromp->కూడు_Dependent(శిఖరం);
    fromp->కూడు_Relative_అంచు<లేఖాచిత్రం_Way::FORWARD>(this);
    శిఖరం->కూడు_Relative_అంచు<లేఖాచిత్రం_Way::తిరగేయి_2>(this);
}

మూస <లేఖాచిత్రం_Way::en N_Way>
LogicMTask* MTask_అంచు::furtherMTaskp() స్థిరరాశి {
    తిరిగి static_cast<LogicMTask*>(this->furtherp<N_Way>());
}
LogicMTask* MTask_అంచు::fromMTaskp() స్థిరరాశి { తిరిగి static_cast<LogicMTask*>(fromp()); }
LogicMTask* MTask_అంచు::గమ్యం_MTaskp() స్థిరరాశి { తిరిగి static_cast<LogicMTask*>(శిఖరం()); }

#అయితే_ముగింపు  // Guard
