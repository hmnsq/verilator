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

#جے_بنا_تعریف VERILATOR_V3ORDERMTASKGRAPH_H_
#تعریف_کرو VERILATOR_V3ORDERMTASKGRAPH_H_

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
class ترتیب_MTask_گراف;

//=============================================================================
// MTaskEdge graph edges are stored in a PairingHeap in each LogicMTask they
// connect to, sorted by critical path through that edge (and id for stability).

بنية کنارا_کلید final {
    uint64_t m_cp;  // The inclusive critical path of the further MTask of the edge
    uint32_t m_id;  // The ID of the further MTask, for stable comparison
    فراغ increase(uint64_t cp) {
        UDEBUGONLY(UASSERT(cp >= m_cp, "Must increase"););
        m_cp = cp;
    }
    // Sort first by critical path, then by ID
    bool عامل<(ثابت کنارا_کلید& other) ثابت {
        إذا (m_cp != other.m_cp) أرجع m_cp < other.m_cp;
        أرجع m_id < other.m_id;
    }
};

using کنارا_ڈھیر = Pairing_ڈھیر<کنارا_کلید>;

//=============================================================================
// LogicMTasks are stored in a PairingHeap during critical path update propagation.

بنية Propagate_Pending_کلید final {
    uint64_t m_increment;  // The amount the critical path of the MTask will grow by
    uint32_t m_id;  // The ID of the MTask, for stable comparison
    LogicMTask* m_mtaskp;  // The MTask the heap entry corresponds to
    فراغ increase(uint64_t increment) {
        UDEBUGONLY(UASSERT(increment >= m_increment, "Must increase"););
        m_increment = increment;
    }
    // Sort first by increment, then by ID
    bool عامل<(ثابت Propagate_Pending_کلید& other) ثابت {
        إذا (m_increment != other.m_increment) أرجع m_increment < other.m_increment;
        أرجع m_id < other.m_id;
    }
};

using Propagate_Pending_ڈھیر = Pairing_ڈھیر<Propagate_Pending_کلید>;

//=============================================================================
// GraphEdge for the MTask graph

class MTask_کنارا final : عام V3_گراف_کنارا {
    VL_RTTI_IMPL(MTask_کنارا, V3_گراف_کنارا)

    یار class LogicMTask;
    یار class ترتیب_MTask_گراف;

    // MEMBERS
    // This edge can be in 2 EdgeHeaps, one forward and one reverse. We allocate the heap nodes
    // directly within the edge as they are always required and this makes association cheap.
    std::array<کنارا_ڈھیر::گرہ_2, گراف_Way::عدد_WAYS> m_کنارا_ڈھیر_گرہ;

    // CONSTRUCTORS
    // Private, so edges can only be created via OrderMTaskGraph, which also updates the critical
    // paths on graph mutation.
    مضمن MTask_کنارا(ترتیب_MTask_گراف* graphp, LogicMTask* fromp, LogicMTask* چوٹی);

عام:
    VL_UNCOPYABLE(MTask_کنارا);
    VL_UNMOVABLE(MTask_کنارا);

    // METHODS
    سانچا <گراف_Way::en N_Way>
    مضمن LogicMTask* furtherMTaskp() ثابت;
    مضمن LogicMTask* fromMTaskp() ثابت;
    مضمن LogicMTask* منزل_MTaskp() ثابت;

    uint64_t cachedCp(گراف_Way way) ثابت { أرجع m_کنارا_ڈھیر_گرہ[way].کلید().m_cp; }
    uint32_t cachedId(گراف_Way way) ثابت { أرجع m_کنارا_ڈھیر_گرہ[way].کلید().m_id; }
    // Convert from the address of the m_edgeHeapNode[way] in an MTaskEdge back to the MTaskEdge
    ساكن ثابت MTask_کنارا* منزل_MTask_کنارا(گراف_Way way, ثابت کنارا_ڈھیر::گرہ_2* nodep) {
        ثابت ناپ_t وتھ = VL_OFFSETOF(MTask_کنارا, m_کنارا_ڈھیر_گرہ[way]);
        أرجع reinterpret_cast<ثابت MTask_کنارا*>(reinterpret_cast<uintptr_t>(nodep) - وتھ);
    }
};

//=============================================================================
// LogicMTask

class LogicMTask final : عام V3_گراف_راس {
    VL_RTTI_IMPL(LogicMTask, V3_گراف_راس)

    یار class MTask_کنارا;
    یار class ترتیب_MTask_گراف;

    // MEMBERS

    // List of OrderMoveVertex's assigned to this mtask. LogicMTask does not own the
    // OrderMoveVertex objects, we merely keep them in a list here.
    ترتیب_Move_راس::List m_mVertices;

    ساكن uint32_t s_اگلا_Id;  // Next ID number to use
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
    std::array<کنارا_ڈھیر, گراف_Way::عدد_WAYS> m_کنارا_ڈھیر;

    // Count "generations" which are just operations that scan through the
    // graph. We'll mark each node with the last generation that scanned
    // it. We can use this to avoid recursing through the same node twice
    // while searching for a path.
    uint64_t m_generation = 0;

    // Scratch pointer used only by the critical path propagation in OrderMTaskGraph: this MTask's
    // node in the pending heap, or nullptr if this MTask is not pending.
    Propagate_Pending_ڈھیر::گرہ_2* m_propagate_ڈھیر_Nodep = nullptr;

عام:
    // CONSTRUCTORS
    LogicMTask(ترتیب_MTask_گراف& گراف, ترتیب_Move_راس* mVtxp) VL_MT_DISABLED;
    VL_UNCOPYABLE(LogicMTask);
    VL_UNMOVABLE(LogicMTask);

    // METHODS
    ترتیب_Move_راس::List& راس_List() { أرجع m_mVertices; }
    uint32_t id() ثابت { أرجع m_id; }
    bool عامل<(ثابت LogicMTask& rhs) ثابت { أرجع id() < rhs.id(); }

    uint64_t لاگت() ثابت VL_MT_محفوظ { أرجع m_لاگت; }
    سانچا <گراف_Way::en N_Way>
    uint64_t cpExclusive() ثابت {
        أرجع m_cpExclusive[N_Way];
    }
    سانچا <گراف_Way::en N_Way>
    uint64_t cpInclusive() ثابت {
        أرجع m_cpExclusive[N_Way] + m_لاگت;
    }
    // The critical path of this MTask without considering the given edge.
    سانچا <گراف_Way::en N_Way>
    uint64_t cpExclusiveWithout(ثابت V3_گراف_کنارا* edgep) ثابت {
        ثابت گراف_Way way{N_Way};
        ثابت گراف_Way inv = way.invert();
        UDEBUGONLY(UASSERT(edgep->furtherp<N_Way>() == this,
                           "In cpExclusiveWithout(), 'edgep' must further to 'this'"););
        // At most two edges need to be considered: the critical path, if that is not via 'edgep',
        // or the second-worst path, if the critical path is via 'edgep'.
        ثابت کنارا_ڈھیر& کنارا_ڈھیر_2 = m_کنارا_ڈھیر[inv];
        // Pick up the critical path edge
        ثابت کنارا_ڈھیر::گرہ_2* ثابت maxp = کنارا_ڈھیر_2.max();
        UDEBUGONLY(UASSERT(maxp, "Edge not in heap"););
        // If 'edgep' is not the critical path edge, return its critical path
        إذا (MTask_کنارا::منزل_MTask_کنارا(inv, maxp) != edgep) أرجع maxp->کلید().m_cp;
        // Otherwise return the second-worst path, if there is one
        ثابت کنارا_ڈھیر::گرہ_2* ثابت secp = کنارا_ڈھیر_2.دوجا_Max();
        إذا (!secp) أرجع 0;
        أرجع secp->کلید().m_cp;
    }

    bool has_کنارا_منزل(LogicMTask* dependentp) ثابت { أرجع m_dependents.گنتی(dependentp); }

    // For Graphviz dumps only
    std::string name() ثابت override VL_MT_STABLE {
        std::ostringstream باہرلا;
        باہرلا << "mt" << m_id  //
            << " | cpFwd " << m_cpExclusive[گراف_Way::FORWARD]  //
            << " | cost " << لاگت()  //
            << " | cpRev " << m_cpExclusive[گراف_Way::الٹ_2];
        أرجع باہرلا.str();
    }

خاص:
    // Following only used by OrderMTaskGraph, which maintains cached CPs and graph invariants.

    سانچا <گراف_Way::en N_Way>
    uint64_t cp_Exclusive_From_کنارے() ثابت {
        constexpr گراف_Way inv = گراف_Way{N_Way}.invert();
        ثابت کنارا_ڈھیر::گرہ_2* ثابت maxp = m_کنارا_ڈھیر[inv].max();
        أرجع maxp ? maxp->کلید().m_cp : 0;
    }
    سانچا <گراف_Way::en N_Way>
    فراغ cpExclusive(uint64_t cp) {
        m_cpExclusive[N_Way] = cp;
    }

    سانچا <گراف_Way::en N_Way>
    فراغ جوڑو_Relative_کنارا(MTask_کنارا* edgep) {
        constexpr گراف_Way way{N_Way};
        constexpr گراف_Way inv = way.invert();
        // Add to the edge heap
        LogicMTask* ثابت relativep = edgep->furtherMTaskp<N_Way>();
        // Value is the !way inclusive cp of the relative
        ثابت uint64_t cp = relativep->cpInclusive<inv>();
        m_کنارا_ڈھیر[way].داخل_کرو(&edgep->m_کنارا_ڈھیر_گرہ[way], {cp, relativep->id()});
    }
    سانچا <گراف_Way::en N_Way>
    فراغ steal_Relative_کنارا(MTask_کنارا* edgep) {
        constexpr گراف_Way way{N_Way};
        // Make heap node insertable, ruining the heap it is currently in.
        edgep->m_کنارا_ڈھیر_گرہ[way].yank();
        // Add the edge as new
        جوڑو_Relative_کنارا<N_Way>(edgep);
    }
    سانچا <گراف_Way::en N_Way>
    فراغ remove_Relative_کنارا(MTask_کنارا* edgep) {
        constexpr گراف_Way way{N_Way};
        // Remove from the edge heap
        m_کنارا_ڈھیر[way].remove(&edgep->m_کنارا_ڈھیر_گرہ[way]);
    }

    فراغ جوڑو_Dependent(LogicMTask* dependentp) {
        ثابت bool exists = !m_dependents.emplace(dependentp).دوجا;
        UDEBUGONLY(UASSERT(!exists, "Adding existing dependent"););
    }
    فراغ removeDependent(LogicMTask* dependentp) {
        ثابت ناپ_t removed = m_dependents.erase(dependentp);
        UDEBUGONLY(UASSERT(removed, "Dependent should have been in set"););
    }
};

//=============================================================================
// OrderMTaskGraph

// The graph of LogicMTask vertices and MTaskEdge edges, used during multi-threaded scheduling.
class ترتیب_MTask_گراف final : عام V3_گراف {
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

    bool رستہ_Exists_Impl(LogicMTask* fromp, LogicMTask* چوٹی, ثابت MTask_کنارا* excludedEdgep);

    // Bring the critical paths of all MTasks wayward of 'mtaskp' in direction N_Way, and those
    // cached in the edge heaps on the way, up to date. Call after mutating the graph such that
    // only MTasks wayward of 'mtaskp' can have a stale critical path, and the critical path of
    // 'mtaskp' itself is already correct. 'mtaskp' is read, but never modified.
    //
    // Note critical paths can only ever grow: those cached in the edge heaps are heap keys, and a
    // heap key can be increased in place, but not decreased.
    سانچا <گراف_Way::en N_Way>
    فراغ propagate(LogicMTask* mtaskp) {
        ++m_currentGeneration;
        propagate_ڈالو<N_Way>(mtaskp);
        propagateResolve<N_Way>();
    }
    // Push the inclusive critical path of 'mtaskp' onto each of its wayward relatives, and add any
    // relative left with a stale critical path to the pending heap (out of line below)
    سانچا <گراف_Way::en N_Way>
    فراغ propagate_ڈالو(LogicMTask* mtaskp);
    // Resolve all pending critical path increases (out of line below)
    سانچا <گراف_Way::en N_Way>
    فراغ propagateResolve();
    // Part of 'validate'
    سانچا <گراف_Way::en N_Way>
    فراغ validateWay() ثابت;

عام:
    // ACCESSORS
    ترتیب_Move_گراف& move_گراف() ثابت { أرجع m_move_گراف; }
    LogicMTask* entryp() ثابت { أرجع m_entryp; }
    LogicMTask* exitp() ثابت { أرجع m_exitp; }
    bool slowAsserts() ثابت { أرجع m_slowAsserts; }

    // METHODS
    uint64_t کل_لاگت() ثابت;  // O(V), called once

    // True if there's a path from 'fromp' to 'top' excluding 'excludedEdgep', false otherwise.
    // 'excludedEdgep' may be nullptr in which case no edge is excluded. If 'excludedEdgep' is
    // non-nullptr it must connect fromp and top.
    bool رستہ_Exists(LogicMTask* fromp, LogicMTask* چوٹی, ثابت MTask_کنارا* excludedEdgep) {
        ++m_currentGeneration;
        أرجع رستہ_Exists_Impl(fromp, چوٹی, excludedEdgep);
    }

    // Add an edge to the graph, update impacted critical paths
    فراغ جوڑو_کنارا(LogicMTask* fromp, LogicMTask* چوٹی);

    // Merge 'donorp' into 'recipientp': move the contents and all edges of 'donorp' onto
    // 'recipientp', update impacted critical paths, then delete 'donorp'. The edge connecting the
    // two (if any) becomes internal to the merged MTask and is deleted, as is one of each pair of
    // edges the two have to a common relative. Note this deletes edges, so the caller must have
    // released any auxiliary data it attached to them via their user pointer.
    فراغ ملاؤ_MTasks(LogicMTask* recipientp, LogicMTask* donorp);

    // Remove all transitive edges. (This deliberately hides V3Graph::removeTransitiveEdges,
    // which would leave the auxiliary data structures stale.)
    // cppcheck-suppress duplInheritedMember
    فراغ remove_متعدی_کنارے();

    // Remove all MTasks holding no logic (except for entry and exit, which are kept even if
    // empty), connecting their predecessors directly to their successors.
    فراغ remove_خالی_تھاواں_MTasks();

    // Do an expensive check that the maintained critical paths, including the ones cached in the
    // edge heaps, match those implied by the current edges of the graph, and that auxiliary data
    // structures are consistent.
    فراغ validate() ثابت;

    // STATIC METHODS
    // Build an MTask graph from 'moveGraph'
    ساكن std::unique_ptr<ترتیب_MTask_گراف> build(ترتیب_Move_گراف& move_گراف) VL_MT_DISABLED;
    // Fix data hazards in the MTask graph
    ساكن فراغ fix_انکڑے_Hazards(ترتیب_MTask_گراف& mtask_گراف) VL_MT_DISABLED;
    // Coarsen the MTask graph by merging MTasks until the given critical-path limit is reached
    ساكن فراغ contract(ترتیب_MTask_گراف& mtask_گراف, uint64_t score_حد_بندی) VL_MT_DISABLED;
};

//=============================================================================
// MTaskEdge method definitions (need the full definition of LogicMTask)

MTask_کنارا::MTask_کنارا(ترتیب_MTask_گراف* graphp, LogicMTask* fromp, LogicMTask* چوٹی)
    : V3_گراف_کنارا{graphp, fromp, چوٹی, 1} {
    fromp->جوڑو_Dependent(چوٹی);
    fromp->جوڑو_Relative_کنارا<گراف_Way::FORWARD>(this);
    چوٹی->جوڑو_Relative_کنارا<گراف_Way::الٹ_2>(this);
}

سانچا <گراف_Way::en N_Way>
LogicMTask* MTask_کنارا::furtherMTaskp() ثابت {
    أرجع static_cast<LogicMTask*>(this->furtherp<N_Way>());
}
LogicMTask* MTask_کنارا::fromMTaskp() ثابت { أرجع static_cast<LogicMTask*>(fromp()); }
LogicMTask* MTask_کنارا::منزل_MTaskp() ثابت { أرجع static_cast<LogicMTask*>(چوٹی()); }

#جے_انت  // Guard
