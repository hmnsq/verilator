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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ORDERMTASKGRAPH_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ORDERMTASKGRAPH_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderMoveGraph.h"
#ಅನ್ತರ್ಭಾವ "V3PairingHeap.h"
#ಅನ್ತರ್ಭಾವ "V3PoolAllocator.h"

#ಅನ್ತರ್ಭಾವ <array>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <sstream>
#ಅನ್ತರ್ಭಾವ <unordered_set>

class LogicMTask;
class ಕ್ರಮ_MTask_ನಕ್ಷೆ;

//=============================================================================
// MTaskEdge graph edges are stored in a PairingHeap in each LogicMTask they
// connect to, sorted by critical path through that edge (and id for stability).

ರಚನೆ ಅಂಚು_ಕೀಲಿ final {
    uint64_t m_cp;  // The inclusive critical path of the further MTask of the edge
    uint32_t m_id;  // The ID of the further MTask, for stable comparison
    ಶೂನ್ಯ increase(uint64_t cp) {
        UDEBUGONLY(UASSERT(cp >= m_cp, "Must increase"););
        m_cp = cp;
    }
    // Sort first by critical path, then by ID
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ ಅಂಚು_ಕೀಲಿ& other) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_cp != other.m_cp) ಹಿಂತಿರುಗಿ m_cp < other.m_cp;
        ಹಿಂತಿರುಗಿ m_id < other.m_id;
    }
};

using ಅಂಚು_ರಾಶಿ = Pairing_ರಾಶಿ<ಅಂಚು_ಕೀಲಿ>;

//=============================================================================
// LogicMTasks are stored in a PairingHeap during critical path update propagation.

ರಚನೆ Propagate_Pending_ಕೀಲಿ final {
    uint64_t m_increment;  // The amount the critical path of the MTask will grow by
    uint32_t m_id;  // The ID of the MTask, for stable comparison
    LogicMTask* m_mtaskp;  // The MTask the heap entry corresponds to
    ಶೂನ್ಯ increase(uint64_t increment) {
        UDEBUGONLY(UASSERT(increment >= m_increment, "Must increase"););
        m_increment = increment;
    }
    // Sort first by increment, then by ID
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ Propagate_Pending_ಕೀಲಿ& other) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_increment != other.m_increment) ಹಿಂತಿರುಗಿ m_increment < other.m_increment;
        ಹಿಂತಿರುಗಿ m_id < other.m_id;
    }
};

using Propagate_Pending_ರಾಶಿ = Pairing_ರಾಶಿ<Propagate_Pending_ಕೀಲಿ>;

//=============================================================================
// GraphEdge for the MTask graph

class MTask_ಅಂಚು final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    VL_RTTI_IMPL(MTask_ಅಂಚು, V3_ನಕ್ಷೆ_ಅಂಚು)

    ಮಿತ್ರ class LogicMTask;
    ಮಿತ್ರ class ಕ್ರಮ_MTask_ನಕ್ಷೆ;

    // MEMBERS
    // This edge can be in 2 EdgeHeaps, one forward and one reverse. We allocate the heap nodes
    // directly within the edge as they are always required and this makes association cheap.
    std::array<ಅಂಚು_ರಾಶಿ::ಗಂಟು_2, ನಕ್ಷೆ_Way::ಅಂಕೆ_WAYS> m_ಅಂಚು_ರಾಶಿ_ಗಂಟು;

    // CONSTRUCTORS
    // Private, so edges can only be created via OrderMTaskGraph, which also updates the critical
    // paths on graph mutation.
    ಸಾಲೊಳಗೆ MTask_ಅಂಚು(ಕ್ರಮ_MTask_ನಕ್ಷೆ* graphp, LogicMTask* fromp, LogicMTask* ತುದಿ);

ಸಾರ್ವಜನಿಕ:
    VL_UNCOPYABLE(MTask_ಅಂಚು);
    VL_UNMOVABLE(MTask_ಅಂಚು);

    // METHODS
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಸಾಲೊಳಗೆ LogicMTask* furtherMTaskp() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ LogicMTask* fromMTaskp() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ LogicMTask* ಗಮ್ಯ_MTaskp() ಸ್ಥಿರಾಂಕ;

    uint64_t cachedCp(ನಕ್ಷೆ_Way way) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಂಚು_ರಾಶಿ_ಗಂಟು[way].ಕೀಲಿ().m_cp; }
    uint32_t cachedId(ನಕ್ಷೆ_Way way) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಂಚು_ರಾಶಿ_ಗಂಟು[way].ಕೀಲಿ().m_id; }
    // Convert from the address of the m_edgeHeapNode[way] in an MTaskEdge back to the MTaskEdge
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ MTask_ಅಂಚು* ಗಮ್ಯ_MTask_ಅಂಚು(ನಕ್ಷೆ_Way way, ಸ್ಥಿರಾಂಕ ಅಂಚು_ರಾಶಿ::ಗಂಟು_2* nodep) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಸ್ಥಾನಾಂತರ = VL_OFFSETOF(MTask_ಅಂಚು, m_ಅಂಚು_ರಾಶಿ_ಗಂಟು[way]);
        ಹಿಂತಿರುಗಿ reinterpret_cast<ಸ್ಥಿರಾಂಕ MTask_ಅಂಚು*>(reinterpret_cast<uintptr_t>(nodep) - ಸ್ಥಾನಾಂತರ);
    }
};

//=============================================================================
// LogicMTask

class LogicMTask final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(LogicMTask, V3_ನಕ್ಷೆ_ಶೃಂಗ)

    ಮಿತ್ರ class MTask_ಅಂಚು;
    ಮಿತ್ರ class ಕ್ರಮ_MTask_ನಕ್ಷೆ;

    // MEMBERS

    // List of OrderMoveVertex's assigned to this mtask. LogicMTask does not own the
    // OrderMoveVertex objects, we merely keep them in a list here.
    ಕ್ರಮ_Move_ಶೃಂಗ::List m_mVertices;

    ಸ್ಥಿರ uint32_t s_ಮುಂದಿನ_Id;  // Next ID number to use
    ಸ್ಥಿರಾಂಕ uint32_t m_id = s_ಮುಂದಿನ_Id++;  // Unique LogicMTask ID number for stable comparison

    // Cost estimate for this LogicMTask, derived from V3InstrCount, in abstract time units.
    // Cost estimates and critical path lengths are bounded by number of AstNodes * constant,
    // will run out of host memory storing the Ast way before they can overflow.
    uint64_t m_ವೆಚ್ಚ = 0;

    // Critical path in each direction: going FORWARD from graph-start to the start of this vertex,
    // and going REVERSE from graph-exit to the end of this vertex. Exclusive of the cost of this
    // vertex itself, see cpInclusive() for the value including it.
    std::array<uint64_t, ನಕ್ಷೆ_Way::ಅಂಕೆ_WAYS> m_cpExclusive = {0, 0};

    // The MTasks this MTask has an out-edge to, so checking for an existing edge is O(1)
    std::unordered_ಗಣ<LogicMTask*> m_dependents;
    // Store the out/in edges in a heaps sorted by the critical path length through each edge
    std::array<ಅಂಚು_ರಾಶಿ, ನಕ್ಷೆ_Way::ಅಂಕೆ_WAYS> m_ಅಂಚು_ರಾಶಿ;

    // Count "generations" which are just operations that scan through the
    // graph. We'll mark each node with the last generation that scanned
    // it. We can use this to avoid recursing through the same node twice
    // while searching for a path.
    uint64_t m_generation = 0;

    // Scratch pointer used only by the critical path propagation in OrderMTaskGraph: this MTask's
    // node in the pending heap, or nullptr if this MTask is not pending.
    Propagate_Pending_ರಾಶಿ::ಗಂಟು_2* m_propagate_ರಾಶಿ_Nodep = nullptr;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LogicMTask(ಕ್ರಮ_MTask_ನಕ್ಷೆ& ನಕ್ಷೆ, ಕ್ರಮ_Move_ಶೃಂಗ* mVtxp) VL_MT_DISABLED;
    VL_UNCOPYABLE(LogicMTask);
    VL_UNMOVABLE(LogicMTask);

    // METHODS
    ಕ್ರಮ_Move_ಶೃಂಗ::List& ಶೃಂಗ_List() { ಹಿಂತಿರುಗಿ m_mVertices; }
    uint32_t id() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_id; }
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ LogicMTask& rhs) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ id() < rhs.id(); }

    uint64_t ವೆಚ್ಚ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ವೆಚ್ಚ; }
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    uint64_t cpExclusive() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_cpExclusive[N_Way];
    }
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    uint64_t cpInclusive() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_cpExclusive[N_Way] + m_ವೆಚ್ಚ;
    }
    // The critical path of this MTask without considering the given edge.
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    uint64_t cpExclusiveWithout(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* edgep) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_Way way{N_Way};
        ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_Way inv = way.invert();
        UDEBUGONLY(UASSERT(edgep->furtherp<N_Way>() == this,
                           "In cpExclusiveWithout(), 'edgep' must further to 'this'"););
        // At most two edges need to be considered: the critical path, if that is not via 'edgep',
        // or the second-worst path, if the critical path is via 'edgep'.
        ಸ್ಥಿರಾಂಕ ಅಂಚು_ರಾಶಿ& ಅಂಚು_ರಾಶಿ_2 = m_ಅಂಚು_ರಾಶಿ[inv];
        // Pick up the critical path edge
        ಸ್ಥಿರಾಂಕ ಅಂಚು_ರಾಶಿ::ಗಂಟು_2* ಸ್ಥಿರಾಂಕ maxp = ಅಂಚು_ರಾಶಿ_2.max();
        UDEBUGONLY(UASSERT(maxp, "Edge not in heap"););
        // If 'edgep' is not the critical path edge, return its critical path
        ಆದರೆ (MTask_ಅಂಚು::ಗಮ್ಯ_MTask_ಅಂಚು(inv, maxp) != edgep) ಹಿಂತಿರುಗಿ maxp->ಕೀಲಿ().m_cp;
        // Otherwise return the second-worst path, if there is one
        ಸ್ಥಿರಾಂಕ ಅಂಚು_ರಾಶಿ::ಗಂಟು_2* ಸ್ಥಿರಾಂಕ secp = ಅಂಚು_ರಾಶಿ_2.ಎರಡನೇ_Max();
        ಆದರೆ (!secp) ಹಿಂತಿರುಗಿ 0;
        ಹಿಂತಿರುಗಿ secp->ಕೀಲಿ().m_cp;
    }

    bool has_ಅಂಚು_ಗಮ್ಯ(LogicMTask* dependentp) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dependents.ಎಣಿಕೆ_2(dependentp); }

    // For Graphviz dumps only
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        std::ostringstream ಹೊರಹೋಗು;
        ಹೊರಹೋಗು << "mt" << m_id  //
            << " | cpFwd " << m_cpExclusive[ನಕ್ಷೆ_Way::FORWARD]  //
            << " | cost " << ವೆಚ್ಚ()  //
            << " | cpRev " << m_cpExclusive[ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2];
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು.str();
    }

ಖಾಸಗಿ:
    // Following only used by OrderMTaskGraph, which maintains cached CPs and graph invariants.

    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    uint64_t cp_Exclusive_From_ಅಂಚುಗಳು() ಸ್ಥಿರಾಂಕ {
        constexpr ನಕ್ಷೆ_Way inv = ನಕ್ಷೆ_Way{N_Way}.invert();
        ಸ್ಥಿರಾಂಕ ಅಂಚು_ರಾಶಿ::ಗಂಟು_2* ಸ್ಥಿರಾಂಕ maxp = m_ಅಂಚು_ರಾಶಿ[inv].max();
        ಹಿಂತಿರುಗಿ maxp ? maxp->ಕೀಲಿ().m_cp : 0;
    }
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಶೂನ್ಯ cpExclusive(uint64_t cp) {
        m_cpExclusive[N_Way] = cp;
    }

    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಶೂನ್ಯ ಕೂಡಿಸು_Relative_ಅಂಚು(MTask_ಅಂಚು* edgep) {
        constexpr ನಕ್ಷೆ_Way way{N_Way};
        constexpr ನಕ್ಷೆ_Way inv = way.invert();
        // Add to the edge heap
        LogicMTask* ಸ್ಥಿರಾಂಕ relativep = edgep->furtherMTaskp<N_Way>();
        // Value is the !way inclusive cp of the relative
        ಸ್ಥಿರಾಂಕ uint64_t cp = relativep->cpInclusive<inv>();
        m_ಅಂಚು_ರಾಶಿ[way].ಸೇರಿಸು(&edgep->m_ಅಂಚು_ರಾಶಿ_ಗಂಟು[way], {cp, relativep->id()});
    }
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಶೂನ್ಯ steal_Relative_ಅಂಚು(MTask_ಅಂಚು* edgep) {
        constexpr ನಕ್ಷೆ_Way way{N_Way};
        // Make heap node insertable, ruining the heap it is currently in.
        edgep->m_ಅಂಚು_ರಾಶಿ_ಗಂಟು[way].yank();
        // Add the edge as new
        ಕೂಡಿಸು_Relative_ಅಂಚು<N_Way>(edgep);
    }
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಶೂನ್ಯ remove_Relative_ಅಂಚು(MTask_ಅಂಚು* edgep) {
        constexpr ನಕ್ಷೆ_Way way{N_Way};
        // Remove from the edge heap
        m_ಅಂಚು_ರಾಶಿ[way].remove(&edgep->m_ಅಂಚು_ರಾಶಿ_ಗಂಟು[way]);
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Dependent(LogicMTask* dependentp) {
        ಸ್ಥಿರಾಂಕ bool exists = !m_dependents.emplace(dependentp).ಎರಡನೇ;
        UDEBUGONLY(UASSERT(!exists, "Adding existing dependent"););
    }
    ಶೂನ್ಯ removeDependent(LogicMTask* dependentp) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t removed = m_dependents.erase(dependentp);
        UDEBUGONLY(UASSERT(removed, "Dependent should have been in set"););
    }
};

//=============================================================================
// OrderMTaskGraph

// The graph of LogicMTask vertices and MTaskEdge edges, used during multi-threaded scheduling.
class ಕ್ರಮ_MTask_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    // MEMBERS
    ಕ್ರಮ_Move_ನಕ್ಷೆ& m_move_ನಕ್ಷೆ;  // The OrderMoveGraph this graph is built from
    LogicMTask* ಸ್ಥಿರಾಂಕ m_entryp;  // The singular entry point vertex
    LogicMTask* ಸ್ಥಿರಾಂಕ m_exitp;  // The singular exit point vertex

    ಸ್ಥಿರಾಂಕ bool m_slowAsserts;  // Take extra time to validate the graph ('--debug-partition')

    // Critical path propagation state. Scratch only: the heap is empty, and no MTask is pending,
    // between calls to 'propagate'. The node pool persists to recycle the heap nodes.
    Propagate_Pending_ರಾಶಿ m_pending_ರಾಶಿ;  // Heap of MTasks pending a critical path update
    PoolAllocator<Propagate_Pending_ರಾಶಿ::ಗಂಟು_2> m_pending_ಗಂಟು_Pool;  // Allocator for the heap nodes

    // Generation counter, e.g. for marking the MTasks visited by algorithms
    uint64_t m_currentGeneration = 0;

    // CONSTRUCTOR
    explicit ಕ್ರಮ_MTask_ನಕ್ಷೆ(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ);  // Used by build(), hence private
    VL_UNCOPYABLE(ಕ್ರಮ_MTask_ನಕ್ಷೆ);
    VL_UNMOVABLE(ಕ್ರಮ_MTask_ನಕ್ಷೆ);

    // METHODS

    bool ಮಾರ್ಗ_Exists_Impl(LogicMTask* fromp, LogicMTask* ತುದಿ, ಸ್ಥಿರಾಂಕ MTask_ಅಂಚು* excludedEdgep);

    // Bring the critical paths of all MTasks wayward of 'mtaskp' in direction N_Way, and those
    // cached in the edge heaps on the way, up to date. Call after mutating the graph such that
    // only MTasks wayward of 'mtaskp' can have a stale critical path, and the critical path of
    // 'mtaskp' itself is already correct. 'mtaskp' is read, but never modified.
    //
    // Note critical paths can only ever grow: those cached in the edge heaps are heap keys, and a
    // heap key can be increased in place, but not decreased.
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಶೂನ್ಯ propagate(LogicMTask* mtaskp) {
        ++m_currentGeneration;
        propagate_ತಳ್ಳು<N_Way>(mtaskp);
        propagateResolve<N_Way>();
    }
    // Push the inclusive critical path of 'mtaskp' onto each of its wayward relatives, and add any
    // relative left with a stale critical path to the pending heap (out of line below)
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಶೂನ್ಯ propagate_ತಳ್ಳು(LogicMTask* mtaskp);
    // Resolve all pending critical path increases (out of line below)
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಶೂನ್ಯ propagateResolve();
    // Part of 'validate'
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಶೂನ್ಯ validateWay() ಸ್ಥಿರಾಂಕ;

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_move_ನಕ್ಷೆ; }
    LogicMTask* entryp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_entryp; }
    LogicMTask* exitp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_exitp; }
    bool slowAsserts() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_slowAsserts; }

    // METHODS
    uint64_t ಒಟ್ಟು_ವೆಚ್ಚ() ಸ್ಥಿರಾಂಕ;  // O(V), called once

    // True if there's a path from 'fromp' to 'top' excluding 'excludedEdgep', false otherwise.
    // 'excludedEdgep' may be nullptr in which case no edge is excluded. If 'excludedEdgep' is
    // non-nullptr it must connect fromp and top.
    bool ಮಾರ್ಗ_Exists(LogicMTask* fromp, LogicMTask* ತುದಿ, ಸ್ಥಿರಾಂಕ MTask_ಅಂಚು* excludedEdgep) {
        ++m_currentGeneration;
        ಹಿಂತಿರುಗಿ ಮಾರ್ಗ_Exists_Impl(fromp, ತುದಿ, excludedEdgep);
    }

    // Add an edge to the graph, update impacted critical paths
    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(LogicMTask* fromp, LogicMTask* ತುದಿ);

    // Merge 'donorp' into 'recipientp': move the contents and all edges of 'donorp' onto
    // 'recipientp', update impacted critical paths, then delete 'donorp'. The edge connecting the
    // two (if any) becomes internal to the merged MTask and is deleted, as is one of each pair of
    // edges the two have to a common relative. Note this deletes edges, so the caller must have
    // released any auxiliary data it attached to them via their user pointer.
    ಶೂನ್ಯ ವಿಲೀನ_MTasks(LogicMTask* recipientp, LogicMTask* donorp);

    // Remove all transitive edges. (This deliberately hides V3Graph::removeTransitiveEdges,
    // which would leave the auxiliary data structures stale.)
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು();

    // Remove all MTasks holding no logic (except for entry and exit, which are kept even if
    // empty), connecting their predecessors directly to their successors.
    ಶೂನ್ಯ remove_ಖಾಲಿಗಳು_MTasks();

    // Do an expensive check that the maintained critical paths, including the ones cached in the
    // edge heaps, match those implied by the current edges of the graph, and that auxiliary data
    // structures are consistent.
    ಶೂನ್ಯ validate() ಸ್ಥಿರಾಂಕ;

    // STATIC METHODS
    // Build an MTask graph from 'moveGraph'
    ಸ್ಥಿರ std::unique_ptr<ಕ್ರಮ_MTask_ನಕ್ಷೆ> build(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ) VL_MT_DISABLED;
    // Fix data hazards in the MTask graph
    ಸ್ಥಿರ ಶೂನ್ಯ fix_ದತ್ತಾಂಶ_Hazards(ಕ್ರಮ_MTask_ನಕ್ಷೆ& mtask_ನಕ್ಷೆ) VL_MT_DISABLED;
    // Coarsen the MTask graph by merging MTasks until the given critical-path limit is reached
    ಸ್ಥಿರ ಶೂನ್ಯ contract(ಕ್ರಮ_MTask_ನಕ್ಷೆ& mtask_ನಕ್ಷೆ, uint64_t score_ಎಲ್ಲೆ) VL_MT_DISABLED;
};

//=============================================================================
// MTaskEdge method definitions (need the full definition of LogicMTask)

MTask_ಅಂಚು::MTask_ಅಂಚು(ಕ್ರಮ_MTask_ನಕ್ಷೆ* graphp, LogicMTask* fromp, LogicMTask* ತುದಿ)
    : V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, 1} {
    fromp->ಕೂಡಿಸು_Dependent(ತುದಿ);
    fromp->ಕೂಡಿಸು_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::FORWARD>(this);
    ತುದಿ->ಕೂಡಿಸು_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(this);
}

ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
LogicMTask* MTask_ಅಂಚು::furtherMTaskp() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ static_cast<LogicMTask*>(this->furtherp<N_Way>());
}
LogicMTask* MTask_ಅಂಚು::fromMTaskp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<LogicMTask*>(fromp()); }
LogicMTask* MTask_ಅಂಚು::ಗಮ್ಯ_MTaskp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<LogicMTask*>(ತುದಿ()); }

#ಆದರೆ_ಅಂತ್ಯ  // Guard
