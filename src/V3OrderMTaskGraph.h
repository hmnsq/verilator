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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3ORDERMTASKGRAPH_H_
#وصف_ڪريو VERILATOR_V3ORDERMTASKGRAPH_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Graph.h"
#ضمن "V3OrderMoveGraph.h"
#ضمن "V3PairingHeap.h"
#ضمن "V3PoolAllocator.h"

#ضمن <array>
#ضمن <memory>
#ضمن <sstream>
#ضمن <unordered_set>

class LogicMTask;
class ترتيب_MTask_گراف;

//=============================================================================
// MTaskEdge graph edges are stored in a PairingHeap in each LogicMTask they
// connect to, sorted by critical path through that edge (and id for stability).

بنية ڪنارو_ڪنجي final {
    uint64_t m_cp;  // The inclusive critical path of the further MTask of the edge
    uint32_t m_id;  // The ID of the further MTask, for stable comparison
    فراغ increase(uint64_t cp) {
        UDEBUGONLY(UASSERT(cp >= m_cp, "Must increase"););
        m_cp = cp;
    }
    // Sort first by critical path, then by ID
    bool عامل<(ثابت ڪنارو_ڪنجي& other) ثابت {
        إذا (m_cp != other.m_cp) أرجع m_cp < other.m_cp;
        أرجع m_id < other.m_id;
    }
};

using ڪنارو_ڍير = Pairing_ڍير<ڪنارو_ڪنجي>;

//=============================================================================
// LogicMTasks are stored in a PairingHeap during critical path update propagation.

بنية Propagate_Pending_ڪنجي final {
    uint64_t m_increment;  // The amount the critical path of the MTask will grow by
    uint32_t m_id;  // The ID of the MTask, for stable comparison
    LogicMTask* m_mtaskp;  // The MTask the heap entry corresponds to
    فراغ increase(uint64_t increment) {
        UDEBUGONLY(UASSERT(increment >= m_increment, "Must increase"););
        m_increment = increment;
    }
    // Sort first by increment, then by ID
    bool عامل<(ثابت Propagate_Pending_ڪنجي& other) ثابت {
        إذا (m_increment != other.m_increment) أرجع m_increment < other.m_increment;
        أرجع m_id < other.m_id;
    }
};

using Propagate_Pending_ڍير = Pairing_ڍير<Propagate_Pending_ڪنجي>;

//=============================================================================
// GraphEdge for the MTask graph

class MTask_ڪنارو final : عام V3_گراف_ڪنارو {
    VL_RTTI_IMPL(MTask_ڪنارو, V3_گراف_ڪنارو)

    دوست class LogicMTask;
    دوست class ترتيب_MTask_گراف;

    // MEMBERS
    // This edge can be in 2 EdgeHeaps, one forward and one reverse. We allocate the heap nodes
    // directly within the edge as they are always required and this makes association cheap.
    std::array<ڪنارو_ڍير::ڳنڍ_2, گراف_Way::انگ_WAYS> m_ڪنارو_ڍير_ڳنڍ;

    // CONSTRUCTORS
    // Private, so edges can only be created via OrderMTaskGraph, which also updates the critical
    // paths on graph mutation.
    مضمن MTask_ڪنارو(ترتيب_MTask_گراف* graphp, LogicMTask* fromp, LogicMTask* چوٽي);

عام:
    VL_UNCOPYABLE(MTask_ڪنارو);
    VL_UNMOVABLE(MTask_ڪنارو);

    // METHODS
    سانچو <گراف_Way::en N_Way>
    مضمن LogicMTask* furtherMTaskp() ثابت;
    مضمن LogicMTask* fromMTaskp() ثابت;
    مضمن LogicMTask* منزل_MTaskp() ثابت;

    uint64_t cachedCp(گراف_Way way) ثابت { أرجع m_ڪنارو_ڍير_ڳنڍ[way].ڪنجي().m_cp; }
    uint32_t cachedId(گراف_Way way) ثابت { أرجع m_ڪنارو_ڍير_ڳنڍ[way].ڪنجي().m_id; }
    // Convert from the address of the m_edgeHeapNode[way] in an MTaskEdge back to the MTaskEdge
    ساكن ثابت MTask_ڪنارو* منزل_MTask_ڪنارو(گراف_Way way, ثابت ڪنارو_ڍير::ڳنڍ_2* nodep) {
        ثابت سائيز_t وٿي = VL_OFFSETOF(MTask_ڪنارو, m_ڪنارو_ڍير_ڳنڍ[way]);
        أرجع reinterpret_cast<ثابت MTask_ڪنارو*>(reinterpret_cast<uintptr_t>(nodep) - وٿي);
    }
};

//=============================================================================
// LogicMTask

class LogicMTask final : عام V3_گراف_راس {
    VL_RTTI_IMPL(LogicMTask, V3_گراف_راس)

    دوست class MTask_ڪنارو;
    دوست class ترتيب_MTask_گراف;

    // MEMBERS

    // List of OrderMoveVertex's assigned to this mtask. LogicMTask does not own the
    // OrderMoveVertex objects, we merely keep them in a list here.
    ترتيب_Move_راس::List m_mVertices;

    ساكن uint32_t s_ايندڙ_Id;  // Next ID number to use
    ثابت uint32_t m_id = s_ايندڙ_Id++;  // Unique LogicMTask ID number for stable comparison

    // Cost estimate for this LogicMTask, derived from V3InstrCount, in abstract time units.
    // Cost estimates and critical path lengths are bounded by number of AstNodes * constant,
    // will run out of host memory storing the Ast way before they can overflow.
    uint64_t m_لاڳت = 0;

    // Critical path in each direction: going FORWARD from graph-start to the start of this vertex,
    // and going REVERSE from graph-exit to the end of this vertex. Exclusive of the cost of this
    // vertex itself, see cpInclusive() for the value including it.
    std::array<uint64_t, گراف_Way::انگ_WAYS> m_cpExclusive = {0, 0};

    // The MTasks this MTask has an out-edge to, so checking for an existing edge is O(1)
    std::unordered_سيٽ<LogicMTask*> m_dependents;
    // Store the out/in edges in a heaps sorted by the critical path length through each edge
    std::array<ڪنارو_ڍير, گراف_Way::انگ_WAYS> m_ڪنارو_ڍير;

    // Count "generations" which are just operations that scan through the
    // graph. We'll mark each node with the last generation that scanned
    // it. We can use this to avoid recursing through the same node twice
    // while searching for a path.
    uint64_t m_generation = 0;

    // Scratch pointer used only by the critical path propagation in OrderMTaskGraph: this MTask's
    // node in the pending heap, or nullptr if this MTask is not pending.
    Propagate_Pending_ڍير::ڳنڍ_2* m_propagate_ڍير_Nodep = nullptr;

عام:
    // CONSTRUCTORS
    LogicMTask(ترتيب_MTask_گراف& گراف, ترتيب_Move_راس* mVtxp) VL_MT_DISABLED;
    VL_UNCOPYABLE(LogicMTask);
    VL_UNMOVABLE(LogicMTask);

    // METHODS
    ترتيب_Move_راس::List& راس_List() { أرجع m_mVertices; }
    uint32_t id() ثابت { أرجع m_id; }
    bool عامل<(ثابت LogicMTask& rhs) ثابت { أرجع id() < rhs.id(); }

    uint64_t لاڳت() ثابت VL_MT_محفوظ { أرجع m_لاڳت; }
    سانچو <گراف_Way::en N_Way>
    uint64_t cpExclusive() ثابت {
        أرجع m_cpExclusive[N_Way];
    }
    سانچو <گراف_Way::en N_Way>
    uint64_t cpInclusive() ثابت {
        أرجع m_cpExclusive[N_Way] + m_لاڳت;
    }
    // The critical path of this MTask without considering the given edge.
    سانچو <گراف_Way::en N_Way>
    uint64_t cpExclusiveWithout(ثابت V3_گراف_ڪنارو* edgep) ثابت {
        ثابت گراف_Way way{N_Way};
        ثابت گراف_Way inv = way.invert();
        UDEBUGONLY(UASSERT(edgep->furtherp<N_Way>() == this,
                           "In cpExclusiveWithout(), 'edgep' must further to 'this'"););
        // At most two edges need to be considered: the critical path, if that is not via 'edgep',
        // or the second-worst path, if the critical path is via 'edgep'.
        ثابت ڪنارو_ڍير& ڪنارو_ڍير_2 = m_ڪنارو_ڍير[inv];
        // Pick up the critical path edge
        ثابت ڪنارو_ڍير::ڳنڍ_2* ثابت maxp = ڪنارو_ڍير_2.max();
        UDEBUGONLY(UASSERT(maxp, "Edge not in heap"););
        // If 'edgep' is not the critical path edge, return its critical path
        إذا (MTask_ڪنارو::منزل_MTask_ڪنارو(inv, maxp) != edgep) أرجع maxp->ڪنجي().m_cp;
        // Otherwise return the second-worst path, if there is one
        ثابت ڪنارو_ڍير::ڳنڍ_2* ثابت secp = ڪنارو_ڍير_2.ٻيو_Max();
        إذا (!secp) أرجع 0;
        أرجع secp->ڪنجي().m_cp;
    }

    bool has_ڪنارو_منزل(LogicMTask* dependentp) ثابت { أرجع m_dependents.ڳڻپ(dependentp); }

    // For Graphviz dumps only
    std::string name() ثابت override VL_MT_STABLE {
        std::ostringstream ٻاهريون;
        ٻاهريون << "mt" << m_id  //
            << " | cpFwd " << m_cpExclusive[گراف_Way::FORWARD]  //
            << " | cost " << لاڳت()  //
            << " | cpRev " << m_cpExclusive[گراف_Way::ابتو_2];
        أرجع ٻاهريون.str();
    }

خاص:
    // Following only used by OrderMTaskGraph, which maintains cached CPs and graph invariants.

    سانچو <گراف_Way::en N_Way>
    uint64_t cp_Exclusive_From_ڪنارا() ثابت {
        constexpr گراف_Way inv = گراف_Way{N_Way}.invert();
        ثابت ڪنارو_ڍير::ڳنڍ_2* ثابت maxp = m_ڪنارو_ڍير[inv].max();
        أرجع maxp ? maxp->ڪنجي().m_cp : 0;
    }
    سانچو <گراف_Way::en N_Way>
    فراغ cpExclusive(uint64_t cp) {
        m_cpExclusive[N_Way] = cp;
    }

    سانچو <گراف_Way::en N_Way>
    فراغ جوڙيو_Relative_ڪنارو(MTask_ڪنارو* edgep) {
        constexpr گراف_Way way{N_Way};
        constexpr گراف_Way inv = way.invert();
        // Add to the edge heap
        LogicMTask* ثابت relativep = edgep->furtherMTaskp<N_Way>();
        // Value is the !way inclusive cp of the relative
        ثابت uint64_t cp = relativep->cpInclusive<inv>();
        m_ڪنارو_ڍير[way].وجهو(&edgep->m_ڪنارو_ڍير_ڳنڍ[way], {cp, relativep->id()});
    }
    سانچو <گراف_Way::en N_Way>
    فراغ steal_Relative_ڪنارو(MTask_ڪنارو* edgep) {
        constexpr گراف_Way way{N_Way};
        // Make heap node insertable, ruining the heap it is currently in.
        edgep->m_ڪنارو_ڍير_ڳنڍ[way].yank();
        // Add the edge as new
        جوڙيو_Relative_ڪنارو<N_Way>(edgep);
    }
    سانچو <گراف_Way::en N_Way>
    فراغ remove_Relative_ڪنارو(MTask_ڪنارو* edgep) {
        constexpr گراف_Way way{N_Way};
        // Remove from the edge heap
        m_ڪنارو_ڍير[way].remove(&edgep->m_ڪنارو_ڍير_ڳنڍ[way]);
    }

    فراغ جوڙيو_Dependent(LogicMTask* dependentp) {
        ثابت bool exists = !m_dependents.emplace(dependentp).ٻيو;
        UDEBUGONLY(UASSERT(!exists, "Adding existing dependent"););
    }
    فراغ removeDependent(LogicMTask* dependentp) {
        ثابت سائيز_t removed = m_dependents.erase(dependentp);
        UDEBUGONLY(UASSERT(removed, "Dependent should have been in set"););
    }
};

//=============================================================================
// OrderMTaskGraph

// The graph of LogicMTask vertices and MTaskEdge edges, used during multi-threaded scheduling.
class ترتيب_MTask_گراف final : عام V3_گراف {
    // MEMBERS
    ترتيب_Move_گراف& m_move_گراف;  // The OrderMoveGraph this graph is built from
    LogicMTask* ثابت m_entryp;  // The singular entry point vertex
    LogicMTask* ثابت m_exitp;  // The singular exit point vertex

    ثابت bool m_slowAsserts;  // Take extra time to validate the graph ('--debug-partition')

    // Critical path propagation state. Scratch only: the heap is empty, and no MTask is pending,
    // between calls to 'propagate'. The node pool persists to recycle the heap nodes.
    Propagate_Pending_ڍير m_pending_ڍير;  // Heap of MTasks pending a critical path update
    PoolAllocator<Propagate_Pending_ڍير::ڳنڍ_2> m_pending_ڳنڍ_Pool;  // Allocator for the heap nodes

    // Generation counter, e.g. for marking the MTasks visited by algorithms
    uint64_t m_currentGeneration = 0;

    // CONSTRUCTOR
    explicit ترتيب_MTask_گراف(ترتيب_Move_گراف& move_گراف);  // Used by build(), hence private
    VL_UNCOPYABLE(ترتيب_MTask_گراف);
    VL_UNMOVABLE(ترتيب_MTask_گراف);

    // METHODS

    bool رستو_Exists_Impl(LogicMTask* fromp, LogicMTask* چوٽي, ثابت MTask_ڪنارو* excludedEdgep);

    // Bring the critical paths of all MTasks wayward of 'mtaskp' in direction N_Way, and those
    // cached in the edge heaps on the way, up to date. Call after mutating the graph such that
    // only MTasks wayward of 'mtaskp' can have a stale critical path, and the critical path of
    // 'mtaskp' itself is already correct. 'mtaskp' is read, but never modified.
    //
    // Note critical paths can only ever grow: those cached in the edge heaps are heap keys, and a
    // heap key can be increased in place, but not decreased.
    سانچو <گراف_Way::en N_Way>
    فراغ propagate(LogicMTask* mtaskp) {
        ++m_currentGeneration;
        propagate_ڌڪو<N_Way>(mtaskp);
        propagateResolve<N_Way>();
    }
    // Push the inclusive critical path of 'mtaskp' onto each of its wayward relatives, and add any
    // relative left with a stale critical path to the pending heap (out of line below)
    سانچو <گراف_Way::en N_Way>
    فراغ propagate_ڌڪو(LogicMTask* mtaskp);
    // Resolve all pending critical path increases (out of line below)
    سانچو <گراف_Way::en N_Way>
    فراغ propagateResolve();
    // Part of 'validate'
    سانچو <گراف_Way::en N_Way>
    فراغ validateWay() ثابت;

عام:
    // ACCESSORS
    ترتيب_Move_گراف& move_گراف() ثابت { أرجع m_move_گراف; }
    LogicMTask* entryp() ثابت { أرجع m_entryp; }
    LogicMTask* exitp() ثابت { أرجع m_exitp; }
    bool slowAsserts() ثابت { أرجع m_slowAsserts; }

    // METHODS
    uint64_t ڪل_لاڳت() ثابت;  // O(V), called once

    // True if there's a path from 'fromp' to 'top' excluding 'excludedEdgep', false otherwise.
    // 'excludedEdgep' may be nullptr in which case no edge is excluded. If 'excludedEdgep' is
    // non-nullptr it must connect fromp and top.
    bool رستو_Exists(LogicMTask* fromp, LogicMTask* چوٽي, ثابت MTask_ڪنارو* excludedEdgep) {
        ++m_currentGeneration;
        أرجع رستو_Exists_Impl(fromp, چوٽي, excludedEdgep);
    }

    // Add an edge to the graph, update impacted critical paths
    فراغ جوڙيو_ڪنارو(LogicMTask* fromp, LogicMTask* چوٽي);

    // Merge 'donorp' into 'recipientp': move the contents and all edges of 'donorp' onto
    // 'recipientp', update impacted critical paths, then delete 'donorp'. The edge connecting the
    // two (if any) becomes internal to the merged MTask and is deleted, as is one of each pair of
    // edges the two have to a common relative. Note this deletes edges, so the caller must have
    // released any auxiliary data it attached to them via their user pointer.
    فراغ ملايو_MTasks(LogicMTask* recipientp, LogicMTask* donorp);

    // Remove all transitive edges. (This deliberately hides V3Graph::removeTransitiveEdges,
    // which would leave the auxiliary data structures stale.)
    // cppcheck-suppress duplInheritedMember
    فراغ remove_متعدي_ڪنارا();

    // Remove all MTasks holding no logic (except for entry and exit, which are kept even if
    // empty), connecting their predecessors directly to their successors.
    فراغ remove_خالي_جايون_MTasks();

    // Do an expensive check that the maintained critical paths, including the ones cached in the
    // edge heaps, match those implied by the current edges of the graph, and that auxiliary data
    // structures are consistent.
    فراغ validate() ثابت;

    // STATIC METHODS
    // Build an MTask graph from 'moveGraph'
    ساكن std::unique_ptr<ترتيب_MTask_گراف> build(ترتيب_Move_گراف& move_گراف) VL_MT_DISABLED;
    // Fix data hazards in the MTask graph
    ساكن فراغ fix_معلومات_Hazards(ترتيب_MTask_گراف& mtask_گراف) VL_MT_DISABLED;
    // Coarsen the MTask graph by merging MTasks until the given critical-path limit is reached
    ساكن فراغ contract(ترتيب_MTask_گراف& mtask_گراف, uint64_t score_حد_بندي) VL_MT_DISABLED;
};

//=============================================================================
// MTaskEdge method definitions (need the full definition of LogicMTask)

MTask_ڪنارو::MTask_ڪنارو(ترتيب_MTask_گراف* graphp, LogicMTask* fromp, LogicMTask* چوٽي)
    : V3_گراف_ڪنارو{graphp, fromp, چوٽي, 1} {
    fromp->جوڙيو_Dependent(چوٽي);
    fromp->جوڙيو_Relative_ڪنارو<گراف_Way::FORWARD>(this);
    چوٽي->جوڙيو_Relative_ڪنارو<گراف_Way::ابتو_2>(this);
}

سانچو <گراف_Way::en N_Way>
LogicMTask* MTask_ڪنارو::furtherMTaskp() ثابت {
    أرجع static_cast<LogicMTask*>(this->furtherp<N_Way>());
}
LogicMTask* MTask_ڪنارو::fromMTaskp() ثابت { أرجع static_cast<LogicMTask*>(fromp()); }
LogicMTask* MTask_ڪنارو::منزل_MTaskp() ثابت { أرجع static_cast<LogicMTask*>(چوٽي()); }

#جيڪڏهن_پورو  // Guard
