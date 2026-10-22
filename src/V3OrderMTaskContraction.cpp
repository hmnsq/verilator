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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3OrderMTaskGraph.h"
#অন্তর্ভাৱ "V3PairingHeap.h"
#অন্তর্ভাৱ "V3PoolAllocator.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <array>
#অন্তর্ভাৱ <limits>
#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <type_traits>
#অন্তর্ভাৱ <unordered_set>
#অন্তর্ভাৱ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

শ্ৰেণী MergeCandidate;
শ্ৰেণী SiblingMC;
শ্ৰেণী EdgeMC;

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

সংরচনা Merge_Candidate_চাবি final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    তার্কিক operator<(ধ্রুৱ Merge_Candidate_চাবি& other) ধ্রুৱ {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        ঘূৰাই m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
using Merge_Candidate_Heap_গ্ৰন্থি = PairingHeap<Merge_Candidate_চাবি>::গ্ৰন্থি_2;

// Information associated with scoreboarding a merge candidate
শ্ৰেণী MergeCandidate VL_NOT_FINAL : সার্ৱজনিক Merge_Candidate_Heap_গ্ৰন্থি {
    // Only the known subclasses can create or delete one of these
    friend শ্ৰেণী SiblingMC;
    friend শ্ৰেণী EdgeMC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    স্থির constexpr uint64_t IS_SIBLING_MASK = 1ULL << 0;
    স্থির constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    তার্কিক isSiblingMC() ধ্রুৱ { ঘূৰাই m_চাবি.m_id & IS_SIBLING_MASK; }

    // CONSTRUCTORS
    explicit MergeCandidate(তার্কিক isSiblingMC) {
        স্থির uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_চাবি.m_id = s_serial | (isSiblingMC * IS_SIBLING_MASK);
    }
    ~MergeCandidate() = স্ৱভাৱ;

সার্ৱজনিক:
    // METHODS
    SiblingMC* toSiblingMC();  // Instead of cast<>/as<>
    EdgeMC* toEdgeMC();  // Instead of cast<>/as<>
    তার্কিক mergeWouldCreateCycle(OrderMTaskGraph& graph);

    // The current score of this candidate, which changes as the graph is contracted
    অন্তঃস্থ uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ধ্রুৱ { ঘূৰাই m_চাবি.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    শূন্য updateScore() { m_চাবি.m_score = currentScore(); }

    স্থির MergeCandidate* heap_গ্ৰন্থি_To_Elem(Merge_Candidate_Heap_গ্ৰন্থি* nodep) {
        ঘূৰাই static_cast<MergeCandidate*>(nodep);
    }
};

static_assert(পরিমাণ(MergeCandidate) == পরিমাণ(Merge_Candidate_Heap_গ্ৰন্থি),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
শ্ৰেণী SiblingMC final : সার্ৱজনিক MergeCandidate {
    LogicMTask* ধ্রুৱ m_ap;  // The higher ID MTask
    LogicMTask* ধ্রুৱ m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { ঘূৰাই m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { ঘূৰাই m_bLinks; }

সার্ৱজনিক:
    // List type to store instances of this class
    using AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    using BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = স্ৱভাৱ;

    // METHODS
    শূন্য unlinkA();
    শূন্য unlinkB();

    LogicMTask* ap() ধ্রুৱ { ঘূৰাই m_ap; }
    LogicMTask* bp() ধ্রুৱ { ঘূৰাই m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::মান, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
শ্ৰেণী EdgeMC final : সার্ৱজনিক MergeCandidate {
    MTaskEdge* ধ্রুৱ m_edgep;  // The associated edge

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit EdgeMC(MTaskEdge* edgep)
        : MergeCandidate{/* isSiblingMC: */ মিছা}
        , m_edgep{edgep} {}
    ~EdgeMC() = স্ৱভাৱ;

    // METHODS
    MTaskEdge* edgep() ধ্রুৱ { ঘূৰাই m_edgep; }
};

static_assert(!std::is_polymorphic<EdgeMC>::মান, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
সংরচনা MTaskContractionData final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_set<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
স্থির MTaskContractionData& mtaskData(ধ্রুৱ LogicMTask* mtaskp) {
    ঘূৰাই *static_cast<MTaskContractionData*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
স্থির EdgeMC* edgeMC(ধ্রুৱ MTaskEdge* edgep) { ঘূৰাই static_cast<EdgeMC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* MergeCandidate::toSiblingMC() {
    ঘূৰাই isSiblingMC() ? static_cast<SiblingMC*>(এতত্) : nullptr;
}

EdgeMC* MergeCandidate::toEdgeMC() { ঘূৰাই isSiblingMC() ? nullptr : static_cast<EdgeMC*>(এতত্); }

তার্কিক MergeCandidate::mergeWouldCreateCycle(OrderMTaskGraph& graph) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    যদি (ধ্রুৱ SiblingMC* ধ্রুৱ sibp = toSiblingMC()) {
        ঘূৰাই graph.pathExists(sibp->ap(), sibp->bp(), nullptr)
               || graph.pathExists(sibp->bp(), sibp->ap(), nullptr);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTaskEdge* ধ্রুৱ edgep = toEdgeMC()->edgep();
    ঘূৰাই graph.pathExists(edgep->fromMTaskp(), edgep->toMTaskp(), edgep);
}

uint64_t MergeCandidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    যদি (ধ্রুৱ SiblingMC* ধ্রুৱ sibp = toSiblingMC()) {
        ধ্রুৱ LogicMTask* ধ্রুৱ ap = sibp->ap();
        ধ্রুৱ LogicMTask* ধ্রুৱ bp = sibp->bp();
        ধ্রুৱ uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<GraphWay::FORWARD>(), bp->cpExclusive<GraphWay::FORWARD>());
        ধ্রুৱ uint64_t mergedCpRev
            = std::max(ap->cpExclusive<GraphWay::REVERSE>(), bp->cpExclusive<GraphWay::REVERSE>());
        ঘূৰাই mergedCpRev + mergedCpFwd + ap->cost() + bp->cost();
    }

    // Edge merge
    {
        MTaskEdge* ধ্রুৱ edgep = toEdgeMC()->edgep();
        ধ্রুৱ LogicMTask* ধ্রুৱ fromp = edgep->fromMTaskp();
        ধ্রুৱ LogicMTask* ধ্রুৱ শীৰ্ষ = edgep->toMTaskp();
        ধ্রুৱ uint64_t mergedCpFwd = std::max(fromp->cpExclusive<GraphWay::FORWARD>(),
                                              শীৰ্ষ->cpExclusiveWithout<GraphWay::FORWARD>(edgep));
        ধ্রুৱ uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<GraphWay::REVERSE>(edgep),
                                              শীৰ্ষ->cpExclusive<GraphWay::REVERSE>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        ঘূৰাই 1 + mergedCpRev + mergedCpFwd + fromp->cost() + শীৰ্ষ->cost();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : MergeCandidate{/* isSiblingMC: */ সঁচা}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtaskData(ap).siblings.গুনতি(bp), "Should be in sibling map"););
    mtaskData(m_ap).aSiblingMCs.linkBack(এতত্);
    mtaskData(m_bp).bSiblingMCs.linkBack(এতত্);
}

শূন্য SiblingMC::unlinkA() {
    VL_ATTR_UNUSED ধ্রুৱ size_t removed = mtaskData(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtaskData(m_ap).aSiblingMCs.unlink(এতত্);
}

শূন্য SiblingMC::unlinkB() { mtaskData(m_bp).bSiblingMCs.unlink(এতত্); }

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
শ্ৰেণী MergeCandidateScoreboard final {
    // TYPES
    using Heap = PairingHeap<Merge_Candidate_চাবি>;

    // MEMBERS
    Heap m_heap;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<EdgeMC> m_edgeMCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    শূন্য ঢোকাও(MergeCandidate* nodep) {
        nodep->updateScore();
        m_heap.ঢোকাও(nodep);
    }

    // Remove a candidate from the scoreboard.
    শূন্য remove(MergeCandidate* nodep) { m_heap.remove(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    MergeCandidateScoreboard() = স্ৱভাৱ;
    ~MergeCandidateScoreboard() = স্ৱভাৱ;
    VL_UNCOPYABLE(MergeCandidateScoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    MergeCandidate* best() ধ্রুৱ { ঘূৰাই MergeCandidate::heap_গ্ৰন্থি_To_Elem(m_heap.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    শূন্য rescore(MergeCandidate* nodep) {
        remove(nodep);
        ঢোকাও(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    শূন্য addEdgeMC(MTaskEdge* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        EdgeMC* ধ্রুৱ edgeMCp = m_edgeMCPool.alloc(edgep);
        edgep->userp(edgeMCp);
        ঢোকাও(edgeMCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    শূন্য removeEdgeMC(MTaskEdge* edgep) {
        EdgeMC* ধ্রুৱ edgeMCp = edgeMC(edgep);
        UDEBUGONLY(UASSERT(edgeMCp, "Edge has no merge candidate"););
        edgep->userp(nullptr);
        remove(edgeMCp);
        VL_DO_DANGLING(m_edgeMCPool.মুক্ত(edgeMCp), edgeMCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    শূন্য addSiblingMC(LogicMTask* ap, LogicMTask* bp) { ঢোকাও(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    শূন্য removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.মুক্ত(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    শূন্য removeMC(MergeCandidate* nodep) {
        যদি (SiblingMC* ধ্রুৱ smcp = nodep->toSiblingMC()) {
            removeSiblingMC(smcp);
        } অন্যথা {
            removeEdgeMC(nodep->toEdgeMC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
শ্ৰেণী Contraction final {
    // MEMBERS
    OrderMTaskGraph& m_mTaskGraph;  // The Mtask graph
    uint64_t m_scoreLimit;  // Critical path limit for merges
    MergeCandidateScoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTaskContractionData[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    শূন্য addEdgeMCs(LogicMTask* mtaskp) {
        বাবে (V3GraphEdge& e : mtaskp->outEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
        বাবে (V3GraphEdge& e : mtaskp->inEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    শূন্য removeEdgeMCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        বাবে (V3GraphEdge& edge : mtaskp->outEdges()) {
            MTaskEdge* ধ্রুৱ edgep = static_cast<MTaskEdge*>(&edge);
            যদি (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
        বাবে (V3GraphEdge& edge : mtaskp->inEdges()) {
            MTaskEdge* ধ্রুৱ edgep = static_cast<MTaskEdge*>(&edge);
            যদি (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
    }

    শূন্য makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        যদি (ap->id() < bp->id()) std::swap(ap, bp);
        // The higher id vertex owns the association set
        ধ্রুৱ তার্কিক first = mtaskData(ap).siblings.ঢোকাও(bp).second;
        যদি (first) {
            m_sb.addSiblingMC(ap, bp);
            ঘূৰাই;
        }

        যদি (VL_UNLIKELY(m_mTaskGraph.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            তার্কিক found = মিছা;
            বাবে (ধ্রুৱ SiblingMC& smc : mtaskData(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                যদি (smc.bp() == bp) found = সঁচা;
            }
            UASSERT_OBJ(found, ap, "Sibling not found");
        }
    }

    template <GraphWay::en N_Way, তার্কিক N_Exhaustive>
    শূন্য addSiblingMCsFromRelatives(LogicMTask* mtaskp) {
        constexpr GraphWay way{N_Way};
        // Need at least 2 edges
        auto& edges = mtaskp->edges<way>();
        যদি (!edges.hasMultipleElements()) ঘূৰাই;

        std::array<LogicMTask*, PART_SIBLING_EDGE_LIMIT> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        সংরচনা alignas(16) SortingRecord final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_EDGE_LIMIT <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            তার্কিক operator<(ধ্রুৱ SortingRecord& that) ধ্রুৱ {
                ঘূৰাই m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(পরিমাণ(SortingRecord) <= 16, "How could this be padded to more than 16?");

        std::array<SortingRecord, PART_SIBLING_EDGE_LIMIT> sortRecs;
        size_t n = 0;

        // Populate the buffers
        বাবে (V3GraphEdge& edge : mtaskp->edges<way>()) {
            LogicMTask* ধ্রুৱ otherp = static_cast<LogicMTask*>(edge.furtherp<way>());
            neighbors[n] = otherp;
            sortRecs[n].m_id = otherp->id();
            sortRecs[n].m_cp = otherp->cpInclusive<way>();
            sortRecs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            যদি (n >= PART_SIBLING_EDGE_LIMIT) ভঙ্গ;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr size_t MAX_NONEXHAUSTIVE_PAIRS = 3;

        যদি (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_PAIRS) {
            ধ্রুৱ size_t end = n & ~static_cast<size_t>(1);  // Round down to even, (we want pairs)
            std::sort(sortRecs.begin(), sortRecs.begin() + n);
            বাবে (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        } অন্যথা {
            constexpr size_t end = 2 * MAX_NONEXHAUSTIVE_PAIRS;
            std::partial_sort(sortRecs.begin(), sortRecs.begin() + end, sortRecs.begin() + n);
            বাবে (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        }
    }

    শূন্য removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        যেতিয়ালৈ (SiblingMC* ধ্রুৱ smcp = mtaskData(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        যেতিয়ালৈ (SiblingMC* ধ্রুৱ smcp = mtaskData(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    শূন্য contract(MergeCandidate* mergeCanp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* শীৰ্ষ;
        যদি (ধ্রুৱ EdgeMC* ধ্রুৱ edgeMCp = mergeCanp->toEdgeMC()) {
            fromp = edgeMCp->edgep()->fromMTaskp();
            শীৰ্ষ = edgeMCp->edgep()->toMTaskp();
        } অন্যথা {
            ধ্রুৱ SiblingMC* ধ্রুৱ sibMCp = mergeCanp->toSiblingMC();
            fromp = sibMCp->bp();
            শীৰ্ষ = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        যদি (fromp->cost() > শীৰ্ষ->cost()) {
            recipientp = fromp;
            donorp = শীৰ্ষ;
        } অন্যথা {
            donorp = fromp;
            recipientp = শীৰ্ষ;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(শীৰ্ষ);  // Use donorp and recipientp now instead

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
        যদি (VL_UNLIKELY(debug() >= 9)) m_mTaskGraph.validate();

        // Add the EdgeMCs of the merged MTask
        addEdgeMCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        addSiblingMCsFromRelatives<GraphWay::REVERSE, সঁচা>(recipientp);
        addSiblingMCsFromRelatives<GraphWay::FORWARD, সঁচা>(recipientp);
        unsigned edges = 0;
        বাবে (V3GraphEdge& edge : recipientp->outEdges()) {
            LogicMTask* ধ্রুৱ postreqp = static_cast<LogicMTask*>(edge.শীৰ্ষ());
            addSiblingMCsFromRelatives<GraphWay::REVERSE, মিছা>(postreqp);
            ++edges;
            যদি (edges >= PART_SIBLING_EDGE_LIMIT) ভঙ্গ;
        }
        edges = 0;
        বাবে (V3GraphEdge& edge : recipientp->inEdges()) {
            LogicMTask* ধ্রুৱ prereqp = static_cast<LogicMTask*>(edge.fromp());
            addSiblingMCsFromRelatives<GraphWay::FORWARD, মিছা>(prereqp);
            ++edges;
            যদি (edges >= PART_SIBLING_EDGE_LIMIT) ভঙ্গ;
        }
    }

    // CONSTRUCTORS
    Contraction(OrderMTaskGraph& mTaskGraph, uint64_t cpLimit)
        : m_mTaskGraph{mTaskGraph}
        , m_scoreLimit{cpLimit} {

        // Check the graph we were given is consistent.
        m_mTaskGraph.validate();

        // Figure out maximum number of MTasks
        ধ্রুৱ uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ধ্রুৱ পূর্ণাঙ্ক given = v3Global.opt.threadsMaxMTasks();
            যদি (given > 0) ঘূৰাই given;
            // Unspecified so estimate
            ঘূৰাই PART_DEFAULT_MAX_MTASKS_PER_THREAD * v3Global.opt.threads();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ধ্রুৱ size_t nMTasks = m_mTaskGraph.vertices().size();
            m_mtaskDatap.reset(নূতন MTaskContractionData[nMTasks]);
            size_t i = 0;
            বাবে (V3GraphVertex& vtx : m_mTaskGraph.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        বাবে (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
            বাবে (V3GraphEdge& edge : vtx.outEdges())
                m_sb.addEdgeMC(static_cast<MTaskEdge*>(&edge));
            LogicMTask* ধ্রুৱ mtaskp = static_cast<LogicMTask*>(&vtx);
            addSiblingMCsFromRelatives<GraphWay::REVERSE, সঁচা>(mtaskp);
            addSiblingMCsFromRelatives<GraphWay::FORWARD, সঁচা>(mtaskp);
        }

        যেতিয়ালৈ (সঁচা) {
            // Pick the candidate yielding the lowest local critical path.
            MergeCandidate* ধ্রুৱ mergeCanp = m_sb.best();
            যদি (!mergeCanp) ভঙ্গ;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ধ্রুৱ uint64_t score = mergeCanp->currentScore();
            যদি (score != mergeCanp->score()) {
                m_sb.rescore(mergeCanp);
                অনুৱর্ত;
            }

            // Check if the critical path limit is reached.
            যদি (score > m_scoreLimit) {

                // If there are still too many MTasks, raise the limit and keep going
                ধ্রুৱ unsigned mtask_গুনতি = m_mTaskGraph.vertices().size();
                যদি (mtask_গুনতি > maxMTasks) {
                    m_scoreLimit = (m_scoreLimit * 120) / 100;
                    File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->fileline();
                    যদি (!flp->warnIsOff(V3ErrorCode::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3ErrorCode::UNOPTTHREADS, সঁচা);
                    }
                    অনুৱর্ত;
                }

                // MTasks limit and CP limit reached. Stop.
                ভঙ্গ;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            যদি (EdgeMC* ধ্রুৱ edgeMCp = mergeCanp->toEdgeMC()) {
                MTaskEdge* ধ্রুৱ edgep = edgeMCp->edgep();
                যদি (edgep->fromp() == m_mTaskGraph.entryp()
                    || edgep->শীৰ্ষ() == m_mTaskGraph.exitp()) {
                    m_sb.removeEdgeMC(edgep);
                    অনুৱর্ত;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            যদি (mergeCanp->mergeWouldCreateCycle(m_mTaskGraph)) {
                m_sb.removeMC(mergeCanp);
                অনুৱর্ত;
            }

            // Merge this candidate
            contract(mergeCanp);
        }

        // Free all remaining merge candidates.
        যেতিয়ালৈ (MergeCandidate* ধ্রুৱ mergeCanp = m_sb.best()) m_sb.removeMC(mergeCanp);
    }

সার্ৱজনিক:
    স্থির শূন্য apply(OrderMTaskGraph& mTaskGraph, uint64_t scoreLimit) {
        Contraction{mTaskGraph, scoreLimit};
    }
};

//######################################################################
// OrderMTaskGraph entry point

শূন্য OrderMTaskGraph::contract(OrderMTaskGraph& mtaskGraph, uint64_t scoreLimit) {
    Contraction::apply(mtaskGraph, scoreLimit);
}
