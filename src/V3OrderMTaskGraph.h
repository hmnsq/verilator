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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ORDERMTASKGRAPH_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ORDERMTASKGRAPH_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderMoveGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PairingHeap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PoolAllocator.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <array>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

class LogicMTask;
class ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ;

//=============================================================================
// MTaskEdge graph edges are stored in a PairingHeap in each LogicMTask they
// connect to, sorted by critical path through that edge (and id for stability).

ਬਣਤਰ ਕਿਨਾਰਾ_ਕੁੰਜੀ final {
    uint64_t m_cp;  // The inclusive critical path of the further MTask of the edge
    uint32_t m_id;  // The ID of the further MTask, for stable comparison
    ਖਾਲੀ increase(uint64_t cp) {
        UDEBUGONLY(UASSERT(cp >= m_cp, "Must increase"););
        m_cp = cp;
    }
    // Sort first by critical path, then by ID
    bool ਸੰਚਾਲਕ<(ਅਚਲ ਕਿਨਾਰਾ_ਕੁੰਜੀ& other) ਅਚਲ {
        ਜੇ (m_cp != other.m_cp) ਵਾਪਸ m_cp < other.m_cp;
        ਵਾਪਸ m_id < other.m_id;
    }
};

using ਕਿਨਾਰਾ_ਢੇਰ = Pairing_ਢੇਰ<ਕਿਨਾਰਾ_ਕੁੰਜੀ>;

//=============================================================================
// LogicMTasks are stored in a PairingHeap during critical path update propagation.

ਬਣਤਰ Propagate_Pending_ਕੁੰਜੀ final {
    uint64_t m_increment;  // The amount the critical path of the MTask will grow by
    uint32_t m_id;  // The ID of the MTask, for stable comparison
    LogicMTask* m_mtaskp;  // The MTask the heap entry corresponds to
    ਖਾਲੀ increase(uint64_t increment) {
        UDEBUGONLY(UASSERT(increment >= m_increment, "Must increase"););
        m_increment = increment;
    }
    // Sort first by increment, then by ID
    bool ਸੰਚਾਲਕ<(ਅਚਲ Propagate_Pending_ਕੁੰਜੀ& other) ਅਚਲ {
        ਜੇ (m_increment != other.m_increment) ਵਾਪਸ m_increment < other.m_increment;
        ਵਾਪਸ m_id < other.m_id;
    }
};

using Propagate_Pending_ਢੇਰ = Pairing_ਢੇਰ<Propagate_Pending_ਕੁੰਜੀ>;

//=============================================================================
// GraphEdge for the MTask graph

class MTask_ਕਿਨਾਰਾ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(MTask_ਕਿਨਾਰਾ, V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ)

    ਮਿੱਤਰ class LogicMTask;
    ਮਿੱਤਰ class ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ;

    // MEMBERS
    // This edge can be in 2 EdgeHeaps, one forward and one reverse. We allocate the heap nodes
    // directly within the edge as they are always required and this makes association cheap.
    std::array<ਕਿਨਾਰਾ_ਢੇਰ::ਗੰਢ_2, ਗ੍ਰਾਫ_Way::ਅੰਕ_WAYS> m_ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ;

    // CONSTRUCTORS
    // Private, so edges can only be created via OrderMTaskGraph, which also updates the critical
    // paths on graph mutation.
    ਸਤਰਅੰਦਰ MTask_ਕਿਨਾਰਾ(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ* graphp, LogicMTask* fromp, LogicMTask* ਸਿਖਰ);

ਜਨਤਕ:
    VL_UNCOPYABLE(MTask_ਕਿਨਾਰਾ);
    VL_UNMOVABLE(MTask_ਕਿਨਾਰਾ);

    // METHODS
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਸਤਰਅੰਦਰ LogicMTask* furtherMTaskp() ਅਚਲ;
    ਸਤਰਅੰਦਰ LogicMTask* fromMTaskp() ਅਚਲ;
    ਸਤਰਅੰਦਰ LogicMTask* ਮੰਜ਼ਿਲ_MTaskp() ਅਚਲ;

    uint64_t cachedCp(ਗ੍ਰਾਫ_Way way) ਅਚਲ { ਵਾਪਸ m_ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ[way].ਕੁੰਜੀ().m_cp; }
    uint32_t cachedId(ਗ੍ਰਾਫ_Way way) ਅਚਲ { ਵਾਪਸ m_ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ[way].ਕੁੰਜੀ().m_id; }
    // Convert from the address of the m_edgeHeapNode[way] in an MTaskEdge back to the MTaskEdge
    ਸਥਿਰ ਅਚਲ MTask_ਕਿਨਾਰਾ* ਮੰਜ਼ਿਲ_MTask_ਕਿਨਾਰਾ(ਗ੍ਰਾਫ_Way way, ਅਚਲ ਕਿਨਾਰਾ_ਢੇਰ::ਗੰਢ_2* nodep) {
        ਅਚਲ ਮਾਪ_t ਵਿੱਥ = VL_OFFSETOF(MTask_ਕਿਨਾਰਾ, m_ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ[way]);
        ਵਾਪਸ reinterpret_cast<ਅਚਲ MTask_ਕਿਨਾਰਾ*>(reinterpret_cast<uintptr_t>(nodep) - ਵਿੱਥ);
    }
};

//=============================================================================
// LogicMTask

class LogicMTask final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(LogicMTask, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)

    ਮਿੱਤਰ class MTask_ਕਿਨਾਰਾ;
    ਮਿੱਤਰ class ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ;

    // MEMBERS

    // List of OrderMoveVertex's assigned to this mtask. LogicMTask does not own the
    // OrderMoveVertex objects, we merely keep them in a list here.
    ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼::List m_mVertices;

    ਸਥਿਰ uint32_t s_ਅਗਲਾ_Id;  // Next ID number to use
    ਅਚਲ uint32_t m_id = s_ਅਗਲਾ_Id++;  // Unique LogicMTask ID number for stable comparison

    // Cost estimate for this LogicMTask, derived from V3InstrCount, in abstract time units.
    // Cost estimates and critical path lengths are bounded by number of AstNodes * constant,
    // will run out of host memory storing the Ast way before they can overflow.
    uint64_t m_ਲਾਗਤ = 0;

    // Critical path in each direction: going FORWARD from graph-start to the start of this vertex,
    // and going REVERSE from graph-exit to the end of this vertex. Exclusive of the cost of this
    // vertex itself, see cpInclusive() for the value including it.
    std::array<uint64_t, ਗ੍ਰਾਫ_Way::ਅੰਕ_WAYS> m_cpExclusive = {0, 0};

    // The MTasks this MTask has an out-edge to, so checking for an existing edge is O(1)
    std::unordered_ਸੈੱਟ<LogicMTask*> m_dependents;
    // Store the out/in edges in a heaps sorted by the critical path length through each edge
    std::array<ਕਿਨਾਰਾ_ਢੇਰ, ਗ੍ਰਾਫ_Way::ਅੰਕ_WAYS> m_ਕਿਨਾਰਾ_ਢੇਰ;

    // Count "generations" which are just operations that scan through the
    // graph. We'll mark each node with the last generation that scanned
    // it. We can use this to avoid recursing through the same node twice
    // while searching for a path.
    uint64_t m_generation = 0;

    // Scratch pointer used only by the critical path propagation in OrderMTaskGraph: this MTask's
    // node in the pending heap, or nullptr if this MTask is not pending.
    Propagate_Pending_ਢੇਰ::ਗੰਢ_2* m_propagate_ਢੇਰ_Nodep = nullptr;

ਜਨਤਕ:
    // CONSTRUCTORS
    LogicMTask(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* mVtxp) VL_MT_DISABLED;
    VL_UNCOPYABLE(LogicMTask);
    VL_UNMOVABLE(LogicMTask);

    // METHODS
    ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼::List& ਸ਼ੀਰਸ਼_List() { ਵਾਪਸ m_mVertices; }
    uint32_t id() ਅਚਲ { ਵਾਪਸ m_id; }
    bool ਸੰਚਾਲਕ<(ਅਚਲ LogicMTask& rhs) ਅਚਲ { ਵਾਪਸ id() < rhs.id(); }

    uint64_t ਲਾਗਤ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਲਾਗਤ; }
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    uint64_t cpExclusive() ਅਚਲ {
        ਵਾਪਸ m_cpExclusive[N_Way];
    }
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    uint64_t cpInclusive() ਅਚਲ {
        ਵਾਪਸ m_cpExclusive[N_Way] + m_ਲਾਗਤ;
    }
    // The critical path of this MTask without considering the given edge.
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    uint64_t cpExclusiveWithout(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep) ਅਚਲ {
        ਅਚਲ ਗ੍ਰਾਫ_Way way{N_Way};
        ਅਚਲ ਗ੍ਰਾਫ_Way inv = way.invert();
        UDEBUGONLY(UASSERT(edgep->furtherp<N_Way>() == this,
                           "In cpExclusiveWithout(), 'edgep' must further to 'this'"););
        // At most two edges need to be considered: the critical path, if that is not via 'edgep',
        // or the second-worst path, if the critical path is via 'edgep'.
        ਅਚਲ ਕਿਨਾਰਾ_ਢੇਰ& ਕਿਨਾਰਾ_ਢੇਰ_2 = m_ਕਿਨਾਰਾ_ਢੇਰ[inv];
        // Pick up the critical path edge
        ਅਚਲ ਕਿਨਾਰਾ_ਢੇਰ::ਗੰਢ_2* ਅਚਲ maxp = ਕਿਨਾਰਾ_ਢੇਰ_2.max();
        UDEBUGONLY(UASSERT(maxp, "Edge not in heap"););
        // If 'edgep' is not the critical path edge, return its critical path
        ਜੇ (MTask_ਕਿਨਾਰਾ::ਮੰਜ਼ਿਲ_MTask_ਕਿਨਾਰਾ(inv, maxp) != edgep) ਵਾਪਸ maxp->ਕੁੰਜੀ().m_cp;
        // Otherwise return the second-worst path, if there is one
        ਅਚਲ ਕਿਨਾਰਾ_ਢੇਰ::ਗੰਢ_2* ਅਚਲ secp = ਕਿਨਾਰਾ_ਢੇਰ_2.ਦੂਜਾ_Max();
        ਜੇ (!secp) ਵਾਪਸ 0;
        ਵਾਪਸ secp->ਕੁੰਜੀ().m_cp;
    }

    bool has_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ(LogicMTask* dependentp) ਅਚਲ { ਵਾਪਸ m_dependents.ਗਿਣਤੀ_2(dependentp); }

    // For Graphviz dumps only
    std::string name() ਅਚਲ override VL_MT_STABLE {
        std::ostringstream ਬਾਹਰਲਾ;
        ਬਾਹਰਲਾ << "mt" << m_id  //
            << " | cpFwd " << m_cpExclusive[ਗ੍ਰਾਫ_Way::FORWARD]  //
            << " | cost " << ਲਾਗਤ()  //
            << " | cpRev " << m_cpExclusive[ਗ੍ਰਾਫ_Way::ਉਲਟ_2];
        ਵਾਪਸ ਬਾਹਰਲਾ.str();
    }

ਨਿਜੀ:
    // Following only used by OrderMTaskGraph, which maintains cached CPs and graph invariants.

    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    uint64_t cp_Exclusive_From_ਕਿਨਾਰੇ() ਅਚਲ {
        constexpr ਗ੍ਰਾਫ_Way inv = ਗ੍ਰਾਫ_Way{N_Way}.invert();
        ਅਚਲ ਕਿਨਾਰਾ_ਢੇਰ::ਗੰਢ_2* ਅਚਲ maxp = m_ਕਿਨਾਰਾ_ਢੇਰ[inv].max();
        ਵਾਪਸ maxp ? maxp->ਕੁੰਜੀ().m_cp : 0;
    }
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਖਾਲੀ cpExclusive(uint64_t cp) {
        m_cpExclusive[N_Way] = cp;
    }

    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਖਾਲੀ ਜੋੜੋ_Relative_ਕਿਨਾਰਾ(MTask_ਕਿਨਾਰਾ* edgep) {
        constexpr ਗ੍ਰਾਫ_Way way{N_Way};
        constexpr ਗ੍ਰਾਫ_Way inv = way.invert();
        // Add to the edge heap
        LogicMTask* ਅਚਲ relativep = edgep->furtherMTaskp<N_Way>();
        // Value is the !way inclusive cp of the relative
        ਅਚਲ uint64_t cp = relativep->cpInclusive<inv>();
        m_ਕਿਨਾਰਾ_ਢੇਰ[way].ਪਾਓ(&edgep->m_ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ[way], {cp, relativep->id()});
    }
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਖਾਲੀ steal_Relative_ਕਿਨਾਰਾ(MTask_ਕਿਨਾਰਾ* edgep) {
        constexpr ਗ੍ਰਾਫ_Way way{N_Way};
        // Make heap node insertable, ruining the heap it is currently in.
        edgep->m_ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ[way].yank();
        // Add the edge as new
        ਜੋੜੋ_Relative_ਕਿਨਾਰਾ<N_Way>(edgep);
    }
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਖਾਲੀ remove_Relative_ਕਿਨਾਰਾ(MTask_ਕਿਨਾਰਾ* edgep) {
        constexpr ਗ੍ਰਾਫ_Way way{N_Way};
        // Remove from the edge heap
        m_ਕਿਨਾਰਾ_ਢੇਰ[way].remove(&edgep->m_ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ[way]);
    }

    ਖਾਲੀ ਜੋੜੋ_Dependent(LogicMTask* dependentp) {
        ਅਚਲ bool exists = !m_dependents.emplace(dependentp).ਦੂਜਾ;
        UDEBUGONLY(UASSERT(!exists, "Adding existing dependent"););
    }
    ਖਾਲੀ removeDependent(LogicMTask* dependentp) {
        ਅਚਲ ਮਾਪ_t removed = m_dependents.erase(dependentp);
        UDEBUGONLY(UASSERT(removed, "Dependent should have been in set"););
    }
};

//=============================================================================
// OrderMTaskGraph

// The graph of LogicMTask vertices and MTaskEdge edges, used during multi-threaded scheduling.
class ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    // MEMBERS
    ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& m_move_ਗ੍ਰਾਫ;  // The OrderMoveGraph this graph is built from
    LogicMTask* ਅਚਲ m_entryp;  // The singular entry point vertex
    LogicMTask* ਅਚਲ m_exitp;  // The singular exit point vertex

    ਅਚਲ bool m_slowAsserts;  // Take extra time to validate the graph ('--debug-partition')

    // Critical path propagation state. Scratch only: the heap is empty, and no MTask is pending,
    // between calls to 'propagate'. The node pool persists to recycle the heap nodes.
    Propagate_Pending_ਢੇਰ m_pending_ਢੇਰ;  // Heap of MTasks pending a critical path update
    PoolAllocator<Propagate_Pending_ਢੇਰ::ਗੰਢ_2> m_pending_ਗੰਢ_Pool;  // Allocator for the heap nodes

    // Generation counter, e.g. for marking the MTasks visited by algorithms
    uint64_t m_currentGeneration = 0;

    // CONSTRUCTOR
    explicit ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ);  // Used by build(), hence private
    VL_UNCOPYABLE(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ);
    VL_UNMOVABLE(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ);

    // METHODS

    bool ਰਸਤਾ_Exists_Impl(LogicMTask* fromp, LogicMTask* ਸਿਖਰ, ਅਚਲ MTask_ਕਿਨਾਰਾ* excludedEdgep);

    // Bring the critical paths of all MTasks wayward of 'mtaskp' in direction N_Way, and those
    // cached in the edge heaps on the way, up to date. Call after mutating the graph such that
    // only MTasks wayward of 'mtaskp' can have a stale critical path, and the critical path of
    // 'mtaskp' itself is already correct. 'mtaskp' is read, but never modified.
    //
    // Note critical paths can only ever grow: those cached in the edge heaps are heap keys, and a
    // heap key can be increased in place, but not decreased.
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਖਾਲੀ propagate(LogicMTask* mtaskp) {
        ++m_currentGeneration;
        propagate_ਧੱਕੋ<N_Way>(mtaskp);
        propagateResolve<N_Way>();
    }
    // Push the inclusive critical path of 'mtaskp' onto each of its wayward relatives, and add any
    // relative left with a stale critical path to the pending heap (out of line below)
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਖਾਲੀ propagate_ਧੱਕੋ(LogicMTask* mtaskp);
    // Resolve all pending critical path increases (out of line below)
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਖਾਲੀ propagateResolve();
    // Part of 'validate'
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਖਾਲੀ validateWay() ਅਚਲ;

ਜਨਤਕ:
    // ACCESSORS
    ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ() ਅਚਲ { ਵਾਪਸ m_move_ਗ੍ਰਾਫ; }
    LogicMTask* entryp() ਅਚਲ { ਵਾਪਸ m_entryp; }
    LogicMTask* exitp() ਅਚਲ { ਵਾਪਸ m_exitp; }
    bool slowAsserts() ਅਚਲ { ਵਾਪਸ m_slowAsserts; }

    // METHODS
    uint64_t ਕੁੱਲ_ਲਾਗਤ() ਅਚਲ;  // O(V), called once

    // True if there's a path from 'fromp' to 'top' excluding 'excludedEdgep', false otherwise.
    // 'excludedEdgep' may be nullptr in which case no edge is excluded. If 'excludedEdgep' is
    // non-nullptr it must connect fromp and top.
    bool ਰਸਤਾ_Exists(LogicMTask* fromp, LogicMTask* ਸਿਖਰ, ਅਚਲ MTask_ਕਿਨਾਰਾ* excludedEdgep) {
        ++m_currentGeneration;
        ਵਾਪਸ ਰਸਤਾ_Exists_Impl(fromp, ਸਿਖਰ, excludedEdgep);
    }

    // Add an edge to the graph, update impacted critical paths
    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(LogicMTask* fromp, LogicMTask* ਸਿਖਰ);

    // Merge 'donorp' into 'recipientp': move the contents and all edges of 'donorp' onto
    // 'recipientp', update impacted critical paths, then delete 'donorp'. The edge connecting the
    // two (if any) becomes internal to the merged MTask and is deleted, as is one of each pair of
    // edges the two have to a common relative. Note this deletes edges, so the caller must have
    // released any auxiliary data it attached to them via their user pointer.
    ਖਾਲੀ ਮਿਲਾਓ_MTasks(LogicMTask* recipientp, LogicMTask* donorp);

    // Remove all transitive edges. (This deliberately hides V3Graph::removeTransitiveEdges,
    // which would leave the auxiliary data structures stale.)
    // cppcheck-suppress duplInheritedMember
    ਖਾਲੀ remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ();

    // Remove all MTasks holding no logic (except for entry and exit, which are kept even if
    // empty), connecting their predecessors directly to their successors.
    ਖਾਲੀ remove_ਖਾਲੀ_ਥਾਵਾਂ_MTasks();

    // Do an expensive check that the maintained critical paths, including the ones cached in the
    // edge heaps, match those implied by the current edges of the graph, and that auxiliary data
    // structures are consistent.
    ਖਾਲੀ validate() ਅਚਲ;

    // STATIC METHODS
    // Build an MTask graph from 'moveGraph'
    ਸਥਿਰ std::unique_ptr<ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ> build(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ) VL_MT_DISABLED;
    // Fix data hazards in the MTask graph
    ਸਥਿਰ ਖਾਲੀ fix_ਅੰਕੜੇ_Hazards(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& mtask_ਗ੍ਰਾਫ) VL_MT_DISABLED;
    // Coarsen the MTask graph by merging MTasks until the given critical-path limit is reached
    ਸਥਿਰ ਖਾਲੀ contract(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& mtask_ਗ੍ਰਾਫ, uint64_t score_ਹੱਦਬੰਦੀ) VL_MT_DISABLED;
};

//=============================================================================
// MTaskEdge method definitions (need the full definition of LogicMTask)

MTask_ਕਿਨਾਰਾ::MTask_ਕਿਨਾਰਾ(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ* graphp, LogicMTask* fromp, LogicMTask* ਸਿਖਰ)
    : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, 1} {
    fromp->ਜੋੜੋ_Dependent(ਸਿਖਰ);
    fromp->ਜੋੜੋ_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::FORWARD>(this);
    ਸਿਖਰ->ਜੋੜੋ_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(this);
}

ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
LogicMTask* MTask_ਕਿਨਾਰਾ::furtherMTaskp() ਅਚਲ {
    ਵਾਪਸ static_cast<LogicMTask*>(this->furtherp<N_Way>());
}
LogicMTask* MTask_ਕਿਨਾਰਾ::fromMTaskp() ਅਚਲ { ਵਾਪਸ static_cast<LogicMTask*>(fromp()); }
LogicMTask* MTask_ਕਿਨਾਰਾ::ਮੰਜ਼ਿਲ_MTaskp() ਅਚਲ { ਵਾਪਸ static_cast<LogicMTask*>(ਸਿਖਰ()); }

#ਜੇ_ਅੰਤ  // Guard
