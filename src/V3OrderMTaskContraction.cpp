// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Multi-threaded MTask graph contraction (coarsening)
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
//  Coarsens the fine-grained MTask graph by repeatedly contracting MTasks,
//  merging along an edge, or merging two "sibling" MTasks until a
//  critical-path limit is reached.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderMTaskGraph.h"
#ಅನ್ತರ್ಭಾವ "V3PairingHeap.h"
#ಅನ್ತರ್ಭಾವ "V3PoolAllocator.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <array>
#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ MergeCandidate;
ವರ್ಗ SiblingMC;
ವರ್ಗ EdgeMC;

//######################################################################
// Tunable settings

// Arbitrarily limit the number of edges on a single vertex that will be
// considered when enumerating siblings, to the given value.  This protects
// the runtime in the presence of huge vertices.
//
// The sibling-merge is less important than the edge merge. Sibling merges
// can be disabled and result in halfway decent coarsening. Edge merges
// cannot be disabled, those are fundamental to the process. So, skipping
// the enumeration of some siblings on a few vertices does not have a large
// impact on the result of the partitioner.
//
// If vertices are small, the limit (at 26) approaches a no-op. Hence
// there's basically no cost to applying this limit even when no huge
// vertices are expected.
//
// If runtime is not a concern and the most precise result is desired,
// set the limit very high.
constexpr unsigned PART_SIBLING_EDGE_LIMIT = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr unsigned PART_DEFAULT_MAX_MTASKS_PER_THREAD = 50;

//######################################################################
// MTask utility classes

ಸಂರಚನಾ Merge_Candidate_ಕೀಲಿ final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    ತಾರ್ಕಿಕ operator<(ಧ್ರುವ Merge_Candidate_ಕೀಲಿ& other) ಧ್ರುವ {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        ಪ್ರತ್ಯಾಗಮ m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
using Merge_Candidate_Heap_ಗಂಟು = PairingHeap<Merge_Candidate_ಕೀಲಿ>::ಗಂಟು_2;

// Information associated with scoreboarding a merge candidate
ವರ್ಗ MergeCandidate VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Merge_Candidate_Heap_ಗಂಟು {
    // Only the known subclasses can create or delete one of these
    friend ವರ್ಗ SiblingMC;
    friend ವರ್ಗ EdgeMC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ಸ್ಥಿರ constexpr uint64_t IS_SIBLING_MASK = 1ULL << 0;
    ಸ್ಥಿರ constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    ತಾರ್ಕಿಕ isSiblingMC() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿ.m_id & IS_SIBLING_MASK; }

    // CONSTRUCTORS
    explicit MergeCandidate(ತಾರ್ಕಿಕ isSiblingMC) {
        ಸ್ಥಿರ uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_ಕೀಲಿ.m_id = s_serial | (isSiblingMC * IS_SIBLING_MASK);
    }
    ~MergeCandidate() = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    // METHODS
    SiblingMC* toSiblingMC();  // Instead of cast<>/as<>
    EdgeMC* toEdgeMC();  // Instead of cast<>/as<>
    ತಾರ್ಕಿಕ mergeWouldCreateCycle(OrderMTaskGraph& graph);

    // The current score of this candidate, which changes as the graph is contracted
    ಅನ್ತಃಸ್ಥ uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿ.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    ಶೂನ್ಯ updateScore() { m_ಕೀಲಿ.m_score = currentScore(); }

    ಸ್ಥಿರ MergeCandidate* heap_ಗಂಟು_To_Elem(Merge_Candidate_Heap_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ static_cast<MergeCandidate*>(nodep);
    }
};

static_assert(ಪರಿಮಾಣ(MergeCandidate) == ಪರಿಮಾಣ(Merge_Candidate_Heap_ಗಂಟು),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
ವರ್ಗ SiblingMC final : ಸಾರ್ವಜನಿಕ MergeCandidate {
    LogicMTask* ಧ್ರುವ m_ap;  // The higher ID MTask
    LogicMTask* ಧ್ರುವ m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { ಪ್ರತ್ಯಾಗಮ m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { ಪ್ರತ್ಯಾಗಮ m_bLinks; }

ಸಾರ್ವಜನಿಕ:
    // List type to store instances of this class
    using AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    using BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = ಸ್ವಭಾವ;

    // METHODS
    ಶೂನ್ಯ unlinkA();
    ಶೂನ್ಯ unlinkB();

    LogicMTask* ap() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ap; }
    LogicMTask* bp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::ಮೌಲ್ಯ, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
ವರ್ಗ EdgeMC final : ಸಾರ್ವಜನಿಕ MergeCandidate {
    MTaskEdge* ಧ್ರುವ m_edgep;  // The associated edge

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit EdgeMC(MTaskEdge* edgep)
        : MergeCandidate{/* isSiblingMC: */ ಅಸತ್ಯ}
        , m_edgep{edgep} {}
    ~EdgeMC() = ಸ್ವಭಾವ;

    // METHODS
    MTaskEdge* edgep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_edgep; }
};

static_assert(!std::is_polymorphic<EdgeMC>::ಮೌಲ್ಯ, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
ಸಂರಚನಾ MTaskContractionData final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_set<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
ಸ್ಥಿರ MTaskContractionData& mtaskData(ಧ್ರುವ LogicMTask* mtaskp) {
    ಪ್ರತ್ಯಾಗಮ *static_cast<MTaskContractionData*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
ಸ್ಥಿರ EdgeMC* edgeMC(ಧ್ರುವ MTaskEdge* edgep) { ಪ್ರತ್ಯಾಗಮ static_cast<EdgeMC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* MergeCandidate::toSiblingMC() {
    ಪ್ರತ್ಯಾಗಮ isSiblingMC() ? static_cast<SiblingMC*>(ಏತತ್) : nullptr;
}

EdgeMC* MergeCandidate::toEdgeMC() { ಪ್ರತ್ಯಾಗಮ isSiblingMC() ? nullptr : static_cast<EdgeMC*>(ಏತತ್); }

ತಾರ್ಕಿಕ MergeCandidate::mergeWouldCreateCycle(OrderMTaskGraph& graph) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    ಯದಿ (ಧ್ರುವ SiblingMC* ಧ್ರುವ sibp = toSiblingMC()) {
        ಪ್ರತ್ಯಾಗಮ graph.pathExists(sibp->ap(), sibp->bp(), nullptr)
               || graph.pathExists(sibp->bp(), sibp->ap(), nullptr);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTaskEdge* ಧ್ರುವ edgep = toEdgeMC()->edgep();
    ಪ್ರತ್ಯಾಗಮ graph.pathExists(edgep->fromMTaskp(), edgep->toMTaskp(), edgep);
}

uint64_t MergeCandidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    ಯದಿ (ಧ್ರುವ SiblingMC* ಧ್ರುವ sibp = toSiblingMC()) {
        ಧ್ರುವ LogicMTask* ಧ್ರುವ ap = sibp->ap();
        ಧ್ರುವ LogicMTask* ಧ್ರುವ bp = sibp->bp();
        ಧ್ರುವ uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<GraphWay::FORWARD>(), bp->cpExclusive<GraphWay::FORWARD>());
        ಧ್ರುವ uint64_t mergedCpRev
            = std::max(ap->cpExclusive<GraphWay::REVERSE>(), bp->cpExclusive<GraphWay::REVERSE>());
        ಪ್ರತ್ಯಾಗಮ mergedCpRev + mergedCpFwd + ap->cost() + bp->cost();
    }

    // Edge merge
    {
        MTaskEdge* ಧ್ರುವ edgep = toEdgeMC()->edgep();
        ಧ್ರುವ LogicMTask* ಧ್ರುವ fromp = edgep->fromMTaskp();
        ಧ್ರುವ LogicMTask* ಧ್ರುವ ತುದಿ = edgep->toMTaskp();
        ಧ್ರುವ uint64_t mergedCpFwd = std::max(fromp->cpExclusive<GraphWay::FORWARD>(),
                                              ತುದಿ->cpExclusiveWithout<GraphWay::FORWARD>(edgep));
        ಧ್ರುವ uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<GraphWay::REVERSE>(edgep),
                                              ತುದಿ->cpExclusive<GraphWay::REVERSE>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        ಪ್ರತ್ಯಾಗಮ 1 + mergedCpRev + mergedCpFwd + fromp->cost() + ತುದಿ->cost();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : MergeCandidate{/* isSiblingMC: */ ಸತ್ಯ}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtaskData(ap).siblings.ಎಣಿಕೆ(bp), "Should be in sibling map"););
    mtaskData(m_ap).aSiblingMCs.linkBack(ಏತತ್);
    mtaskData(m_bp).bSiblingMCs.linkBack(ಏತತ್);
}

ಶೂನ್ಯ SiblingMC::unlinkA() {
    VL_ATTR_UNUSED ಧ್ರುವ size_t removed = mtaskData(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtaskData(m_ap).aSiblingMCs.unlink(ಏತತ್);
}

ಶೂನ್ಯ SiblingMC::unlinkB() { mtaskData(m_bp).bSiblingMCs.unlink(ಏತತ್); }

// Scoreboard of MTask merge candidates.
//
// This is a heap, sorted by the local critical path that would result from merging the candidate,
// that is: the longest critical path running through the merged MTask. Merges proceed by picking
// the candidate yielding the lowest such critical path, which is the merge that does the least
// damage: a merge can only ever lengthen paths, and one whose local critical path is no longer
// than the current global critical path cannot lengthen that at all.
//
// A candidate's score changes as the graph is contracted, so the score a candidate is in the heap
// with is only the score it was last given, which is a lower bound on its current score. This
// makes the top of the heap a lower bound on the score of every candidate, so the caller can pick
// the best candidate by checking whether the top still has the score it was given, and
// rescoring it if not (see the contraction loop).
//
// The scoreboard owns the lifetime of the merge candidate objects: callers add/remove candidates
// via the methods below and never allocate or free them directly. For edges this maintains the
// invariant that an MTaskEdge has an associated EdgeMC (held in its userp()), if and only if it is
// currently on the scoreboard.
ವರ್ಗ MergeCandidateScoreboard final {
    // TYPES
    using Heap = PairingHeap<Merge_Candidate_ಕೀಲಿ>;

    // MEMBERS
    Heap m_heap;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<EdgeMC> m_edgeMCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    ಶೂನ್ಯ ಸೇರಿಸು(MergeCandidate* nodep) {
        nodep->updateScore();
        m_heap.ಸೇರಿಸು(nodep);
    }

    // Remove a candidate from the scoreboard.
    ಶೂನ್ಯ remove(MergeCandidate* nodep) { m_heap.remove(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    MergeCandidateScoreboard() = ಸ್ವಭಾವ;
    ~MergeCandidateScoreboard() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(MergeCandidateScoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    MergeCandidate* best() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ MergeCandidate::heap_ಗಂಟು_To_Elem(m_heap.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    ಶೂನ್ಯ rescore(MergeCandidate* nodep) {
        remove(nodep);
        ಸೇರಿಸು(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    ಶೂನ್ಯ addEdgeMC(MTaskEdge* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        EdgeMC* ಧ್ರುವ edgeMCp = m_edgeMCPool.alloc(edgep);
        edgep->userp(edgeMCp);
        ಸೇರಿಸು(edgeMCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    ಶೂನ್ಯ removeEdgeMC(MTaskEdge* edgep) {
        EdgeMC* ಧ್ರುವ edgeMCp = edgeMC(edgep);
        UDEBUGONLY(UASSERT(edgeMCp, "Edge has no merge candidate"););
        edgep->userp(nullptr);
        remove(edgeMCp);
        VL_DO_DANGLING(m_edgeMCPool.ಮುಕ್ತ(edgeMCp), edgeMCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    ಶೂನ್ಯ addSiblingMC(LogicMTask* ap, LogicMTask* bp) { ಸೇರಿಸು(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    ಶೂನ್ಯ removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.ಮುಕ್ತ(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    ಶೂನ್ಯ removeMC(MergeCandidate* nodep) {
        ಯದಿ (SiblingMC* ಧ್ರುವ smcp = nodep->toSiblingMC()) {
            removeSiblingMC(smcp);
        } ಅನ್ಯಥಾ {
            removeEdgeMC(nodep->toEdgeMC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
ವರ್ಗ Contraction final {
    // MEMBERS
    OrderMTaskGraph& m_mTaskGraph;  // The Mtask graph
    uint64_t m_scoreLimit;  // Critical path limit for merges
    MergeCandidateScoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTaskContractionData[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    ಶೂನ್ಯ addEdgeMCs(LogicMTask* mtaskp) {
        ಪ್ರತಿ (V3GraphEdge& e : mtaskp->outEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
        ಪ್ರತಿ (V3GraphEdge& e : mtaskp->inEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    ಶೂನ್ಯ removeEdgeMCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        ಪ್ರತಿ (V3GraphEdge& edge : mtaskp->outEdges()) {
            MTaskEdge* ಧ್ರುವ edgep = static_cast<MTaskEdge*>(&edge);
            ಯದಿ (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
        ಪ್ರತಿ (V3GraphEdge& edge : mtaskp->inEdges()) {
            MTaskEdge* ಧ್ರುವ edgep = static_cast<MTaskEdge*>(&edge);
            ಯದಿ (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
    }

    ಶೂನ್ಯ makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        ಯದಿ (ap->id() < bp->id()) std::swap(ap, bp);
        // The higher id vertex owns the association set
        ಧ್ರುವ ತಾರ್ಕಿಕ first = mtaskData(ap).siblings.ಸೇರಿಸು(bp).second;
        ಯದಿ (first) {
            m_sb.addSiblingMC(ap, bp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (VL_UNLIKELY(m_mTaskGraph.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            ತಾರ್ಕಿಕ found = ಅಸತ್ಯ;
            ಪ್ರತಿ (ಧ್ರುವ SiblingMC& smc : mtaskData(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                ಯದಿ (smc.bp() == bp) found = ಸತ್ಯ;
            }
            UASSERT_OBJ(found, ap, "Sibling not found");
        }
    }

    template <GraphWay::en N_Way, ತಾರ್ಕಿಕ N_Exhaustive>
    ಶೂನ್ಯ addSiblingMCsFromRelatives(LogicMTask* mtaskp) {
        constexpr GraphWay way{N_Way};
        // Need at least 2 edges
        auto& edges = mtaskp->edges<way>();
        ಯದಿ (!edges.hasMultipleElements()) ಪ್ರತ್ಯಾಗಮ;

        std::array<LogicMTask*, PART_SIBLING_EDGE_LIMIT> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        ಸಂರಚನಾ alignas(16) SortingRecord final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_EDGE_LIMIT <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            ತಾರ್ಕಿಕ operator<(ಧ್ರುವ SortingRecord& that) ಧ್ರುವ {
                ಪ್ರತ್ಯಾಗಮ m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(ಪರಿಮಾಣ(SortingRecord) <= 16, "How could this be padded to more than 16?");

        std::array<SortingRecord, PART_SIBLING_EDGE_LIMIT> sortRecs;
        size_t n = 0;

        // Populate the buffers
        ಪ್ರತಿ (V3GraphEdge& edge : mtaskp->edges<way>()) {
            LogicMTask* ಧ್ರುವ otherp = static_cast<LogicMTask*>(edge.furtherp<way>());
            neighbors[n] = otherp;
            sortRecs[n].m_id = otherp->id();
            sortRecs[n].m_cp = otherp->cpInclusive<way>();
            sortRecs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            ಯದಿ (n >= PART_SIBLING_EDGE_LIMIT) ಭಙ್ಗ;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr size_t MAX_NONEXHAUSTIVE_PAIRS = 3;

        ಯದಿ (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_PAIRS) {
            ಧ್ರುವ size_t end = n & ~static_cast<size_t>(1);  // Round down to even, (we want pairs)
            std::sort(sortRecs.begin(), sortRecs.begin() + n);
            ಪ್ರತಿ (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        } ಅನ್ಯಥಾ {
            constexpr size_t end = 2 * MAX_NONEXHAUSTIVE_PAIRS;
            std::partial_sort(sortRecs.begin(), sortRecs.begin() + end, sortRecs.begin() + n);
            ಪ್ರತಿ (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        }
    }

    ಶೂನ್ಯ removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        ಯಾವತ್ (SiblingMC* ಧ್ರುವ smcp = mtaskData(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        ಯಾವತ್ (SiblingMC* ಧ್ರುವ smcp = mtaskData(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    ಶೂನ್ಯ contract(MergeCandidate* mergeCanp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* ತುದಿ;
        ಯದಿ (ಧ್ರುವ EdgeMC* ಧ್ರುವ edgeMCp = mergeCanp->toEdgeMC()) {
            fromp = edgeMCp->edgep()->fromMTaskp();
            ತುದಿ = edgeMCp->edgep()->toMTaskp();
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ SiblingMC* ಧ್ರುವ sibMCp = mergeCanp->toSiblingMC();
            fromp = sibMCp->bp();
            ತುದಿ = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        ಯದಿ (fromp->cost() > ತುದಿ->cost()) {
            recipientp = fromp;
            donorp = ತುದಿ;
        } ಅನ್ಯಥಾ {
            donorp = fromp;
            recipientp = ತುದಿ;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(ತುದಿ);  // Use donorp and recipientp now instead

        // Remove all SiblingMCs that include either MTask
        removeSiblingMCs(recipientp);
        removeSiblingMCs(donorp);

        // Remove the EdgeMCs of both MTasks
        removeEdgeMCs(recipientp);
        removeEdgeMCs(donorp);

        // Merge the MTasks. This redirects all edges, updates critical paths, and deletes donorp
        m_mTaskGraph.mergeMTasks(recipientp, donorp);
        VL_DANGLING(donorp);

        // Confirm we haven't botched the CP updates. This is a whole graph walk after every single
        // merge, so it is quadratic in the size of the graph, hence only under '--debug 9'.
        ಯದಿ (VL_UNLIKELY(debug() >= 9)) m_mTaskGraph.validate();

        // Add the EdgeMCs of the merged MTask
        addEdgeMCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        addSiblingMCsFromRelatives<GraphWay::REVERSE, ಸತ್ಯ>(recipientp);
        addSiblingMCsFromRelatives<GraphWay::FORWARD, ಸತ್ಯ>(recipientp);
        unsigned edges = 0;
        ಪ್ರತಿ (V3GraphEdge& edge : recipientp->outEdges()) {
            LogicMTask* ಧ್ರುವ postreqp = static_cast<LogicMTask*>(edge.ತುದಿ());
            addSiblingMCsFromRelatives<GraphWay::REVERSE, ಅಸತ್ಯ>(postreqp);
            ++edges;
            ಯದಿ (edges >= PART_SIBLING_EDGE_LIMIT) ಭಙ್ಗ;
        }
        edges = 0;
        ಪ್ರತಿ (V3GraphEdge& edge : recipientp->inEdges()) {
            LogicMTask* ಧ್ರುವ prereqp = static_cast<LogicMTask*>(edge.fromp());
            addSiblingMCsFromRelatives<GraphWay::FORWARD, ಅಸತ್ಯ>(prereqp);
            ++edges;
            ಯದಿ (edges >= PART_SIBLING_EDGE_LIMIT) ಭಙ್ಗ;
        }
    }

    // CONSTRUCTORS
    Contraction(OrderMTaskGraph& mTaskGraph, uint64_t cpLimit)
        : m_mTaskGraph{mTaskGraph}
        , m_scoreLimit{cpLimit} {

        // Check the graph we were given is consistent.
        m_mTaskGraph.validate();

        // Figure out maximum number of MTasks
        ಧ್ರುವ uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ given = v3Global.opt.threadsMaxMTasks();
            ಯದಿ (given > 0) ಪ್ರತ್ಯಾಗಮ given;
            // Unspecified so estimate
            ಪ್ರತ್ಯಾಗಮ PART_DEFAULT_MAX_MTASKS_PER_THREAD * v3Global.opt.threads();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ಧ್ರುವ size_t nMTasks = m_mTaskGraph.vertices().size();
            m_mtaskDatap.reset(ನೂತನ MTaskContractionData[nMTasks]);
            size_t i = 0;
            ಪ್ರತಿ (V3GraphVertex& vtx : m_mTaskGraph.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        ಪ್ರತಿ (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
            ಪ್ರತಿ (V3GraphEdge& edge : vtx.outEdges())
                m_sb.addEdgeMC(static_cast<MTaskEdge*>(&edge));
            LogicMTask* ಧ್ರುವ mtaskp = static_cast<LogicMTask*>(&vtx);
            addSiblingMCsFromRelatives<GraphWay::REVERSE, ಸತ್ಯ>(mtaskp);
            addSiblingMCsFromRelatives<GraphWay::FORWARD, ಸತ್ಯ>(mtaskp);
        }

        ಯಾವತ್ (ಸತ್ಯ) {
            // Pick the candidate yielding the lowest local critical path.
            MergeCandidate* ಧ್ರುವ mergeCanp = m_sb.best();
            ಯದಿ (!mergeCanp) ಭಙ್ಗ;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ಧ್ರುವ uint64_t score = mergeCanp->currentScore();
            ಯದಿ (score != mergeCanp->score()) {
                m_sb.rescore(mergeCanp);
                ಅನುವರ್ತ;
            }

            // Check if the critical path limit is reached.
            ಯದಿ (score > m_scoreLimit) {

                // If there are still too many MTasks, raise the limit and keep going
                ಧ್ರುವ unsigned mtask_ಎಣಿಕೆ = m_mTaskGraph.vertices().size();
                ಯದಿ (mtask_ಎಣಿಕೆ > maxMTasks) {
                    m_scoreLimit = (m_scoreLimit * 120) / 100;
                    File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->fileline();
                    ಯದಿ (!flp->warnIsOff(V3ErrorCode::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3ErrorCode::UNOPTTHREADS, ಸತ್ಯ);
                    }
                    ಅನುವರ್ತ;
                }

                // MTasks limit and CP limit reached. Stop.
                ಭಙ್ಗ;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            ಯದಿ (EdgeMC* ಧ್ರುವ edgeMCp = mergeCanp->toEdgeMC()) {
                MTaskEdge* ಧ್ರುವ edgep = edgeMCp->edgep();
                ಯದಿ (edgep->fromp() == m_mTaskGraph.entryp()
                    || edgep->ತುದಿ() == m_mTaskGraph.exitp()) {
                    m_sb.removeEdgeMC(edgep);
                    ಅನುವರ್ತ;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            ಯದಿ (mergeCanp->mergeWouldCreateCycle(m_mTaskGraph)) {
                m_sb.removeMC(mergeCanp);
                ಅನುವರ್ತ;
            }

            // Merge this candidate
            contract(mergeCanp);
        }

        // Free all remaining merge candidates.
        ಯಾವತ್ (MergeCandidate* ಧ್ರುವ mergeCanp = m_sb.best()) m_sb.removeMC(mergeCanp);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(OrderMTaskGraph& mTaskGraph, uint64_t scoreLimit) {
        Contraction{mTaskGraph, scoreLimit};
    }
};

//######################################################################
// OrderMTaskGraph entry point

ಶೂನ್ಯ OrderMTaskGraph::contract(OrderMTaskGraph& mtaskGraph, uint64_t scoreLimit) {
    Contraction::apply(mtaskGraph, scoreLimit);
}
