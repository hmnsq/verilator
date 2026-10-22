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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ORDERMTASKGRAPH_H_
#تعریف_کرو VERILATOR_V3ORDERMTASKGRAPH_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Graph.h"
#شامل "V3OrderMoveGraph.h"
#شامل "V3PairingHeap.h"
#شامل "V3PoolAllocator.h"

#شامل <array>
#شامل <memory>
#شامل <sstream>
#شامل <unordered_set>

class LogicMTask;
class ترتیب_MTask_گراف;

//=============================================================================
// MTaskEdge graph edges are stored in a PairingHeap in each LogicMTask they
// connect to, sorted by critical path through that edge (and id for stability).

ساخت کنارہ_کلید final {
    uint64_t m_cp;  // The inclusive critical path of the further MTask of the edge
    uint32_t m_id;  // The ID of the further MTask, for stable comparison
    خالی increase(uint64_t cp) {
        UDEBUGONLY(UASSERT(cp >= m_cp, "Must increase"););
        m_cp = cp;
    }
    // Sort first by critical path, then by ID
    bool عامل<(ثابت کنارہ_کلید& other) ثابت {
        اگر (m_cp != other.m_cp) واپس m_cp < other.m_cp;
        واپس m_id < other.m_id;
    }
};

using کنارہ_ڈھیر = Pairing_ڈھیر<کنارہ_کلید>;

//=============================================================================
// LogicMTasks are stored in a PairingHeap during critical path update propagation.

ساخت Propagate_Pending_کلید final {
    uint64_t m_increment;  // The amount the critical path of the MTask will grow by
    uint32_t m_id;  // The ID of the MTask, for stable comparison
    LogicMTask* m_mtaskp;  // The MTask the heap entry corresponds to
    خالی increase(uint64_t increment) {
        UDEBUGONLY(UASSERT(increment >= m_increment, "Must increase"););
        m_increment = increment;
    }
    // Sort first by increment, then by ID
    bool عامل<(ثابت Propagate_Pending_کلید& other) ثابت {
        اگر (m_increment != other.m_increment) واپس m_increment < other.m_increment;
        واپس m_id < other.m_id;
    }
};

using Propagate_Pending_ڈھیر = Pairing_ڈھیر<Propagate_Pending_کلید>;

//=============================================================================
// GraphEdge for the MTask graph

class MTask_کنارہ final : عوامی V3_گراف_کنارہ {
    VL_RTTI_IMPL(MTask_کنارہ, V3_گراف_کنارہ)

    دوست class LogicMTask;
    دوست class ترتیب_MTask_گراف;

    // MEMBERS
    // This edge can be in 2 EdgeHeaps, one forward and one reverse. We allocate the heap nodes
    // directly within the edge as they are always required and this makes association cheap.
    std::array<کنارہ_ڈھیر::گرہ_2, گراف_Way::عدد_WAYS> m_کنارہ_ڈھیر_گرہ;

    // CONSTRUCTORS
    // Private, so edges can only be created via OrderMTaskGraph, which also updates the critical
    // paths on graph mutation.
    اندرونی MTask_کنارہ(ترتیب_MTask_گراف* graphp, LogicMTask* fromp, LogicMTask* چوٹی);

عوامی:
    VL_UNCOPYABLE(MTask_کنارہ);
    VL_UNMOVABLE(MTask_کنارہ);

    // METHODS
    سانچہ <گراف_Way::en N_Way>
    اندرونی LogicMTask* furtherMTaskp() ثابت;
    اندرونی LogicMTask* fromMTaskp() ثابت;
    اندرونی LogicMTask* مقصد_MTaskp() ثابت;

    uint64_t cachedCp(گراف_Way way) ثابت { واپس m_کنارہ_ڈھیر_گرہ[way].کلید().m_cp; }
    uint32_t cachedId(گراف_Way way) ثابت { واپس m_کنارہ_ڈھیر_گرہ[way].کلید().m_id; }
    // Convert from the address of the m_edgeHeapNode[way] in an MTaskEdge back to the MTaskEdge
    ساکن ثابت MTask_کنارہ* مقصد_MTask_کنارہ(گراف_Way way, ثابت کنارہ_ڈھیر::گرہ_2* nodep) {
        ثابت سائز_t انحراف = VL_OFFSETOF(MTask_کنارہ, m_کنارہ_ڈھیر_گرہ[way]);
        واپس reinterpret_cast<ثابت MTask_کنارہ*>(reinterpret_cast<uintptr_t>(nodep) - انحراف);
    }
};

//=============================================================================
// LogicMTask

class LogicMTask final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(LogicMTask, V3_گراف_راس)

    دوست class MTask_کنارہ;
    دوست class ترتیب_MTask_گراف;

    // MEMBERS

    // List of OrderMoveVertex's assigned to this mtask. LogicMTask does not own the
    // OrderMoveVertex objects, we merely keep them in a list here.
    ترتیب_Move_راس::List m_mVertices;

    ساکن uint32_t s_اگلا_Id;  // Next ID number to use
    ثابت uint32_t m_id = s_اگلا_Id++;  // Unique LogicMTask ID number for stable comparison

    // Cost estimate for this LogicMTask, derived from V3InstrCount, in abstract time units.
    // Cost estimates and critical path lengths are bounded by number of AstNodes * constant,
    // will run out of host memory storing the Ast way before they can overflow.
    uint64_t m_لاگت = 0;

    // Critical path in each direction: going FORWARD from graph-start to the start of this vertex,
    // and going REVERSE from graph-exit to the end of this vertex. Exclusive of the cost of this
    // vertex itself, see cpInclusive() for the value including it.
    std::array<uint64_t, گراف_Way::عدد_WAYS> m_cpExclusive = {0, 0};

    // The MTasks this MTask has an out-edge to, so checking for an existing edge is O(1)
    std::unordered_سیٹ<LogicMTask*> m_dependents;
    // Store the out/in edges in a heaps sorted by the critical path length through each edge
    std::array<کنارہ_ڈھیر, گراف_Way::عدد_WAYS> m_کنارہ_ڈھیر;

    // Count "generations" which are just operations that scan through the
    // graph. We'll mark each node with the last generation that scanned
    // it. We can use this to avoid recursing through the same node twice
    // while searching for a path.
    uint64_t m_generation = 0;

    // Scratch pointer used only by the critical path propagation in OrderMTaskGraph: this MTask's
    // node in the pending heap, or nullptr if this MTask is not pending.
    Propagate_Pending_ڈھیر::گرہ_2* m_propagate_ڈھیر_Nodep = nullptr;

عوامی:
    // CONSTRUCTORS
    LogicMTask(ترتیب_MTask_گراف& گراف, ترتیب_Move_راس* mVtxp) VL_MT_DISABLED;
    VL_UNCOPYABLE(LogicMTask);
    VL_UNMOVABLE(LogicMTask);

    // METHODS
    ترتیب_Move_راس::List& راس_List() { واپس m_mVertices; }
    uint32_t id() ثابت { واپس m_id; }
    bool عامل<(ثابت LogicMTask& rhs) ثابت { واپس id() < rhs.id(); }

    uint64_t لاگت() ثابت VL_MT_محفوظ { واپس m_لاگت; }
    سانچہ <گراف_Way::en N_Way>
    uint64_t cpExclusive() ثابت {
        واپس m_cpExclusive[N_Way];
    }
    سانچہ <گراف_Way::en N_Way>
    uint64_t cpInclusive() ثابت {
        واپس m_cpExclusive[N_Way] + m_لاگت;
    }
    // The critical path of this MTask without considering the given edge.
    سانچہ <گراف_Way::en N_Way>
    uint64_t cpExclusiveWithout(ثابت V3_گراف_کنارہ* edgep) ثابت {
        ثابت گراف_Way way{N_Way};
        ثابت گراف_Way inv = way.invert();
        UDEBUGONLY(UASSERT(edgep->furtherp<N_Way>() == this,
                           "In cpExclusiveWithout(), 'edgep' must further to 'this'"););
        // At most two edges need to be considered: the critical path, if that is not via 'edgep',
        // or the second-worst path, if the critical path is via 'edgep'.
        ثابت کنارہ_ڈھیر& کنارہ_ڈھیر_2 = m_کنارہ_ڈھیر[inv];
        // Pick up the critical path edge
        ثابت کنارہ_ڈھیر::گرہ_2* ثابت maxp = کنارہ_ڈھیر_2.max();
        UDEBUGONLY(UASSERT(maxp, "Edge not in heap"););
        // If 'edgep' is not the critical path edge, return its critical path
        اگر (MTask_کنارہ::مقصد_MTask_کنارہ(inv, maxp) != edgep) واپس maxp->کلید().m_cp;
        // Otherwise return the second-worst path, if there is one
        ثابت کنارہ_ڈھیر::گرہ_2* ثابت secp = کنارہ_ڈھیر_2.دوسرا_Max();
        اگر (!secp) واپس 0;
        واپس secp->کلید().m_cp;
    }

    bool has_کنارہ_مقصد(LogicMTask* dependentp) ثابت { واپس m_dependents.گنتی(dependentp); }

    // For Graphviz dumps only
    std::string name() ثابت override VL_MT_STABLE {
        std::ostringstream برآمد;
        برآمد << "mt" << m_id  //
            << " | cpFwd " << m_cpExclusive[گراف_Way::FORWARD]  //
            << " | cost " << لاگت()  //
            << " | cpRev " << m_cpExclusive[گراف_Way::الٹ_2];
        واپس برآمد.str();
    }

نجی:
    // Following only used by OrderMTaskGraph, which maintains cached CPs and graph invariants.

    سانچہ <گراف_Way::en N_Way>
    uint64_t cp_Exclusive_From_کنارے() ثابت {
        constexpr گراف_Way inv = گراف_Way{N_Way}.invert();
        ثابت کنارہ_ڈھیر::گرہ_2* ثابت maxp = m_کنارہ_ڈھیر[inv].max();
        واپس maxp ? maxp->کلید().m_cp : 0;
    }
    سانچہ <گراف_Way::en N_Way>
    خالی cpExclusive(uint64_t cp) {
        m_cpExclusive[N_Way] = cp;
    }

    سانچہ <گراف_Way::en N_Way>
    خالی جوڑو_Relative_کنارہ(MTask_کنارہ* edgep) {
        constexpr گراف_Way way{N_Way};
        constexpr گراف_Way inv = way.invert();
        // Add to the edge heap
        LogicMTask* ثابت relativep = edgep->furtherMTaskp<N_Way>();
        // Value is the !way inclusive cp of the relative
        ثابت uint64_t cp = relativep->cpInclusive<inv>();
        m_کنارہ_ڈھیر[way].داخل_کرو(&edgep->m_کنارہ_ڈھیر_گرہ[way], {cp, relativep->id()});
    }
    سانچہ <گراف_Way::en N_Way>
    خالی steal_Relative_کنارہ(MTask_کنارہ* edgep) {
        constexpr گراف_Way way{N_Way};
        // Make heap node insertable, ruining the heap it is currently in.
        edgep->m_کنارہ_ڈھیر_گرہ[way].yank();
        // Add the edge as new
        جوڑو_Relative_کنارہ<N_Way>(edgep);
    }
    سانچہ <گراف_Way::en N_Way>
    خالی remove_Relative_کنارہ(MTask_کنارہ* edgep) {
        constexpr گراف_Way way{N_Way};
        // Remove from the edge heap
        m_کنارہ_ڈھیر[way].remove(&edgep->m_کنارہ_ڈھیر_گرہ[way]);
    }

    خالی جوڑو_Dependent(LogicMTask* dependentp) {
        ثابت bool exists = !m_dependents.emplace(dependentp).دوسرا;
        UDEBUGONLY(UASSERT(!exists, "Adding existing dependent"););
    }
    خالی removeDependent(LogicMTask* dependentp) {
        ثابت سائز_t removed = m_dependents.erase(dependentp);
        UDEBUGONLY(UASSERT(removed, "Dependent should have been in set"););
    }
};

//=============================================================================
// OrderMTaskGraph

// The graph of LogicMTask vertices and MTaskEdge edges, used during multi-threaded scheduling.
class ترتیب_MTask_گراف final : عوامی V3_گراف {
    // MEMBERS
    ترتیب_Move_گراف& m_move_گراف;  // The OrderMoveGraph this graph is built from
    LogicMTask* ثابت m_entryp;  // The singular entry point vertex
    LogicMTask* ثابت m_exitp;  // The singular exit point vertex

    ثابت bool m_slowAsserts;  // Take extra time to validate the graph ('--debug-partition')

    // Critical path propagation state. Scratch only: the heap is empty, and no MTask is pending,
    // between calls to 'propagate'. The node pool persists to recycle the heap nodes.
    Propagate_Pending_ڈھیر m_pending_ڈھیر;  // Heap of MTasks pending a critical path update
    PoolAllocator<Propagate_Pending_ڈھیر::گرہ_2> m_pending_گرہ_Pool;  // Allocator for the heap nodes

    // Generation counter, e.g. for marking the MTasks visited by algorithms
    uint64_t m_currentGeneration = 0;

    // CONSTRUCTOR
    explicit ترتیب_MTask_گراف(ترتیب_Move_گراف& move_گراف);  // Used by build(), hence private
    VL_UNCOPYABLE(ترتیب_MTask_گراف);
    VL_UNMOVABLE(ترتیب_MTask_گراف);

    // METHODS

    bool راستہ_Exists_Impl(LogicMTask* fromp, LogicMTask* چوٹی, ثابت MTask_کنارہ* excludedEdgep);

    // Bring the critical paths of all MTasks wayward of 'mtaskp' in direction N_Way, and those
    // cached in the edge heaps on the way, up to date. Call after mutating the graph such that
    // only MTasks wayward of 'mtaskp' can have a stale critical path, and the critical path of
    // 'mtaskp' itself is already correct. 'mtaskp' is read, but never modified.
    //
    // Note critical paths can only ever grow: those cached in the edge heaps are heap keys, and a
    // heap key can be increased in place, but not decreased.
    سانچہ <گراف_Way::en N_Way>
    خالی propagate(LogicMTask* mtaskp) {
        ++m_currentGeneration;
        propagate_ڈالو<N_Way>(mtaskp);
        propagateResolve<N_Way>();
    }
    // Push the inclusive critical path of 'mtaskp' onto each of its wayward relatives, and add any
    // relative left with a stale critical path to the pending heap (out of line below)
    سانچہ <گراف_Way::en N_Way>
    خالی propagate_ڈالو(LogicMTask* mtaskp);
    // Resolve all pending critical path increases (out of line below)
    سانچہ <گراف_Way::en N_Way>
    خالی propagateResolve();
    // Part of 'validate'
    سانچہ <گراف_Way::en N_Way>
    خالی validateWay() ثابت;

عوامی:
    // ACCESSORS
    ترتیب_Move_گراف& move_گراف() ثابت { واپس m_move_گراف; }
    LogicMTask* entryp() ثابت { واپس m_entryp; }
    LogicMTask* exitp() ثابت { واپس m_exitp; }
    bool slowAsserts() ثابت { واپس m_slowAsserts; }

    // METHODS
    uint64_t کل_لاگت() ثابت;  // O(V), called once

    // True if there's a path from 'fromp' to 'top' excluding 'excludedEdgep', false otherwise.
    // 'excludedEdgep' may be nullptr in which case no edge is excluded. If 'excludedEdgep' is
    // non-nullptr it must connect fromp and top.
    bool راستہ_Exists(LogicMTask* fromp, LogicMTask* چوٹی, ثابت MTask_کنارہ* excludedEdgep) {
        ++m_currentGeneration;
        واپس راستہ_Exists_Impl(fromp, چوٹی, excludedEdgep);
    }

    // Add an edge to the graph, update impacted critical paths
    خالی جوڑو_کنارہ(LogicMTask* fromp, LogicMTask* چوٹی);

    // Merge 'donorp' into 'recipientp': move the contents and all edges of 'donorp' onto
    // 'recipientp', update impacted critical paths, then delete 'donorp'. The edge connecting the
    // two (if any) becomes internal to the merged MTask and is deleted, as is one of each pair of
    // edges the two have to a common relative. Note this deletes edges, so the caller must have
    // released any auxiliary data it attached to them via their user pointer.
    خالی ملاؤ_MTasks(LogicMTask* recipientp, LogicMTask* donorp);

    // Remove all transitive edges. (This deliberately hides V3Graph::removeTransitiveEdges,
    // which would leave the auxiliary data structures stale.)
    // cppcheck-suppress duplInheritedMember
    خالی remove_متعدی_کنارے();

    // Remove all MTasks holding no logic (except for entry and exit, which are kept even if
    // empty), connecting their predecessors directly to their successors.
    خالی remove_خالی_خانے_MTasks();

    // Do an expensive check that the maintained critical paths, including the ones cached in the
    // edge heaps, match those implied by the current edges of the graph, and that auxiliary data
    // structures are consistent.
    خالی validate() ثابت;

    // STATIC METHODS
    // Build an MTask graph from 'moveGraph'
    ساکن std::unique_ptr<ترتیب_MTask_گراف> build(ترتیب_Move_گراف& move_گراف) VL_MT_DISABLED;
    // Fix data hazards in the MTask graph
    ساکن خالی fix_ڈیٹا_Hazards(ترتیب_MTask_گراف& mtask_گراف) VL_MT_DISABLED;
    // Coarsen the MTask graph by merging MTasks until the given critical-path limit is reached
    ساکن خالی contract(ترتیب_MTask_گراف& mtask_گراف, uint64_t score_حد_بندی) VL_MT_DISABLED;
};

//=============================================================================
// MTaskEdge method definitions (need the full definition of LogicMTask)

MTask_کنارہ::MTask_کنارہ(ترتیب_MTask_گراف* graphp, LogicMTask* fromp, LogicMTask* چوٹی)
    : V3_گراف_کنارہ{graphp, fromp, چوٹی, 1} {
    fromp->جوڑو_Dependent(چوٹی);
    fromp->جوڑو_Relative_کنارہ<گراف_Way::FORWARD>(this);
    چوٹی->جوڑو_Relative_کنارہ<گراف_Way::الٹ_2>(this);
}

سانچہ <گراف_Way::en N_Way>
LogicMTask* MTask_کنارہ::furtherMTaskp() ثابت {
    واپس static_cast<LogicMTask*>(this->furtherp<N_Way>());
}
LogicMTask* MTask_کنارہ::fromMTaskp() ثابت { واپس static_cast<LogicMTask*>(fromp()); }
LogicMTask* MTask_کنارہ::مقصد_MTaskp() ثابت { واپس static_cast<LogicMTask*>(چوٹی()); }

#اگر_ختم  // Guard
