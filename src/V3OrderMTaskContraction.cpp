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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Global.h"
#ضمن "V3Graph.h"
#ضمن "V3OrderMTaskGraph.h"
#ضمن "V3PairingHeap.h"
#ضمن "V3PoolAllocator.h"

#ضمن <algorithm>
#ضمن <array>
#ضمن <limits>
#ضمن <memory>
#ضمن <type_traits>
#ضمن <unordered_set>
#ضمن <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف MergeCandidate;
صنف SiblingMC;
صنف EdgeMC;

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

بنية Merge_Candidate_کلیل final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    منطقي operator<(ثابت Merge_Candidate_کلیل& other) ثابت {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        أرجع m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
using Merge_Candidate_Heap_گرێ = PairingHeap<Merge_Candidate_کلیل>::گرێ_2;

// Information associated with scoreboarding a merge candidate
صنف MergeCandidate VL_NOT_FINAL : عام Merge_Candidate_Heap_گرێ {
    // Only the known subclasses can create or delete one of these
    friend صنف SiblingMC;
    friend صنف EdgeMC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ساكن constexpr uint64_t IS_SIBLING_MASK = 1ULL << 0;
    ساكن constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    منطقي isSiblingMC() ثابت { أرجع m_کلیل.m_id & IS_SIBLING_MASK; }

    // CONSTRUCTORS
    explicit MergeCandidate(منطقي isSiblingMC) {
        ساكن uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_کلیل.m_id = s_serial | (isSiblingMC * IS_SIBLING_MASK);
    }
    ~MergeCandidate() = افتراضي;

عام:
    // METHODS
    SiblingMC* toSiblingMC();  // Instead of cast<>/as<>
    EdgeMC* toEdgeMC();  // Instead of cast<>/as<>
    منطقي mergeWouldCreateCycle(OrderMTaskGraph& graph);

    // The current score of this candidate, which changes as the graph is contracted
    مضمن uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ثابت { أرجع m_کلیل.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    فراغ updateScore() { m_کلیل.m_score = currentScore(); }

    ساكن MergeCandidate* heap_گرێ_To_Elem(Merge_Candidate_Heap_گرێ* nodep) {
        أرجع static_cast<MergeCandidate*>(nodep);
    }
};

static_assert(حجم(MergeCandidate) == حجم(Merge_Candidate_Heap_گرێ),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
صنف SiblingMC final : عام MergeCandidate {
    LogicMTask* ثابت m_ap;  // The higher ID MTask
    LogicMTask* ثابت m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { أرجع m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { أرجع m_bLinks; }

عام:
    // List type to store instances of this class
    using AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    using BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = افتراضي;

    // METHODS
    فراغ unlinkA();
    فراغ unlinkB();

    LogicMTask* ap() ثابت { أرجع m_ap; }
    LogicMTask* bp() ثابت { أرجع m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::بەها, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
صنف EdgeMC final : عام MergeCandidate {
    MTaskEdge* ثابت m_edgep;  // The associated edge

عام:
    // CONSTRUCTORS
    explicit EdgeMC(MTaskEdge* edgep)
        : MergeCandidate{/* isSiblingMC: */ خطأ}
        , m_edgep{edgep} {}
    ~EdgeMC() = افتراضي;

    // METHODS
    MTaskEdge* edgep() ثابت { أرجع m_edgep; }
};

static_assert(!std::is_polymorphic<EdgeMC>::بەها, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
بنية MTaskContractionData final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_set<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
ساكن MTaskContractionData& mtaskData(ثابت LogicMTask* mtaskp) {
    أرجع *static_cast<MTaskContractionData*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
ساكن EdgeMC* edgeMC(ثابت MTaskEdge* edgep) { أرجع static_cast<EdgeMC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* MergeCandidate::toSiblingMC() {
    أرجع isSiblingMC() ? static_cast<SiblingMC*>(هذا) : nullptr;
}

EdgeMC* MergeCandidate::toEdgeMC() { أرجع isSiblingMC() ? nullptr : static_cast<EdgeMC*>(هذا); }

منطقي MergeCandidate::mergeWouldCreateCycle(OrderMTaskGraph& graph) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    إذا (ثابت SiblingMC* ثابت sibp = toSiblingMC()) {
        أرجع graph.pathExists(sibp->ap(), sibp->bp(), nullptr)
               || graph.pathExists(sibp->bp(), sibp->ap(), nullptr);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTaskEdge* ثابت edgep = toEdgeMC()->edgep();
    أرجع graph.pathExists(edgep->fromMTaskp(), edgep->toMTaskp(), edgep);
}

uint64_t MergeCandidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    إذا (ثابت SiblingMC* ثابت sibp = toSiblingMC()) {
        ثابت LogicMTask* ثابت ap = sibp->ap();
        ثابت LogicMTask* ثابت bp = sibp->bp();
        ثابت uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<GraphWay::FORWARD>(), bp->cpExclusive<GraphWay::FORWARD>());
        ثابت uint64_t mergedCpRev
            = std::max(ap->cpExclusive<GraphWay::REVERSE>(), bp->cpExclusive<GraphWay::REVERSE>());
        أرجع mergedCpRev + mergedCpFwd + ap->cost() + bp->cost();
    }

    // Edge merge
    {
        MTaskEdge* ثابت edgep = toEdgeMC()->edgep();
        ثابت LogicMTask* ثابت fromp = edgep->fromMTaskp();
        ثابت LogicMTask* ثابت لووتکە = edgep->toMTaskp();
        ثابت uint64_t mergedCpFwd = std::max(fromp->cpExclusive<GraphWay::FORWARD>(),
                                              لووتکە->cpExclusiveWithout<GraphWay::FORWARD>(edgep));
        ثابت uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<GraphWay::REVERSE>(edgep),
                                              لووتکە->cpExclusive<GraphWay::REVERSE>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        أرجع 1 + mergedCpRev + mergedCpFwd + fromp->cost() + لووتکە->cost();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : MergeCandidate{/* isSiblingMC: */ true}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtaskData(ap).siblings.ژماردن(bp), "Should be in sibling map"););
    mtaskData(m_ap).aSiblingMCs.linkBack(هذا);
    mtaskData(m_bp).bSiblingMCs.linkBack(هذا);
}

فراغ SiblingMC::unlinkA() {
    VL_ATTR_UNUSED ثابت size_t removed = mtaskData(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtaskData(m_ap).aSiblingMCs.unlink(هذا);
}

فراغ SiblingMC::unlinkB() { mtaskData(m_bp).bSiblingMCs.unlink(هذا); }

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
صنف MergeCandidateScoreboard final {
    // TYPES
    using Heap = PairingHeap<Merge_Candidate_کلیل>;

    // MEMBERS
    Heap m_heap;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<EdgeMC> m_edgeMCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    فراغ زیادکردن(MergeCandidate* nodep) {
        nodep->updateScore();
        m_heap.زیادکردن(nodep);
    }

    // Remove a candidate from the scoreboard.
    فراغ remove(MergeCandidate* nodep) { m_heap.remove(nodep); }

عام:
    // CONSTRUCTORS
    MergeCandidateScoreboard() = افتراضي;
    ~MergeCandidateScoreboard() = افتراضي;
    VL_UNCOPYABLE(MergeCandidateScoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    MergeCandidate* best() ثابت { أرجع MergeCandidate::heap_گرێ_To_Elem(m_heap.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    فراغ rescore(MergeCandidate* nodep) {
        remove(nodep);
        زیادکردن(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    فراغ addEdgeMC(MTaskEdge* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        EdgeMC* ثابت edgeMCp = m_edgeMCPool.alloc(edgep);
        edgep->userp(edgeMCp);
        زیادکردن(edgeMCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    فراغ removeEdgeMC(MTaskEdge* edgep) {
        EdgeMC* ثابت edgeMCp = edgeMC(edgep);
        UDEBUGONLY(UASSERT(edgeMCp, "Edge has no merge candidate"););
        edgep->userp(nullptr);
        remove(edgeMCp);
        VL_DO_DANGLING(m_edgeMCPool.حرر(edgeMCp), edgeMCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    فراغ addSiblingMC(LogicMTask* ap, LogicMTask* bp) { زیادکردن(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    فراغ removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.حرر(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    فراغ removeMC(MergeCandidate* nodep) {
        إذا (SiblingMC* ثابت smcp = nodep->toSiblingMC()) {
            removeSiblingMC(smcp);
        } وإلا {
            removeEdgeMC(nodep->toEdgeMC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
صنف Contraction final {
    // MEMBERS
    OrderMTaskGraph& m_mTaskGraph;  // The Mtask graph
    uint64_t m_scoreLimit;  // Critical path limit for merges
    MergeCandidateScoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTaskContractionData[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    فراغ addEdgeMCs(LogicMTask* mtaskp) {
        لكل (V3GraphEdge& e : mtaskp->outEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
        لكل (V3GraphEdge& e : mtaskp->inEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    فراغ removeEdgeMCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        لكل (V3GraphEdge& edge : mtaskp->outEdges()) {
            MTaskEdge* ثابت edgep = static_cast<MTaskEdge*>(&edge);
            إذا (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
        لكل (V3GraphEdge& edge : mtaskp->inEdges()) {
            MTaskEdge* ثابت edgep = static_cast<MTaskEdge*>(&edge);
            إذا (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
    }

    فراغ makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        إذا (ap->id() < bp->id()) std::swap(ap, bp);
        // The higher id vertex owns the association set
        ثابت منطقي first = mtaskData(ap).siblings.زیادکردن(bp).second;
        إذا (first) {
            m_sb.addSiblingMC(ap, bp);
            أرجع;
        }

        إذا (VL_UNLIKELY(m_mTaskGraph.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            منطقي found = خطأ;
            لكل (ثابت SiblingMC& smc : mtaskData(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                إذا (smc.bp() == bp) found = true;
            }
            UASSERT_OBJ(found, ap, "Sibling not found");
        }
    }

    template <GraphWay::en N_Way, منطقي N_Exhaustive>
    فراغ addSiblingMCsFromRelatives(LogicMTask* mtaskp) {
        constexpr GraphWay way{N_Way};
        // Need at least 2 edges
        auto& edges = mtaskp->edges<way>();
        إذا (!edges.hasMultipleElements()) أرجع;

        std::array<LogicMTask*, PART_SIBLING_EDGE_LIMIT> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        بنية alignas(16) SortingRecord final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_EDGE_LIMIT <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            منطقي operator<(ثابت SortingRecord& that) ثابت {
                أرجع m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(حجم(SortingRecord) <= 16, "How could this be padded to more than 16?");

        std::array<SortingRecord, PART_SIBLING_EDGE_LIMIT> sortRecs;
        size_t n = 0;

        // Populate the buffers
        لكل (V3GraphEdge& edge : mtaskp->edges<way>()) {
            LogicMTask* ثابت otherp = static_cast<LogicMTask*>(edge.furtherp<way>());
            neighbors[n] = otherp;
            sortRecs[n].m_id = otherp->id();
            sortRecs[n].m_cp = otherp->cpInclusive<way>();
            sortRecs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            إذا (n >= PART_SIBLING_EDGE_LIMIT) اكسر;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr size_t MAX_NONEXHAUSTIVE_PAIRS = 3;

        إذا (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_PAIRS) {
            ثابت size_t end = n & ~static_cast<size_t>(1);  // Round down to even, (we want pairs)
            std::sort(sortRecs.begin(), sortRecs.begin() + n);
            لكل (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        } وإلا {
            constexpr size_t end = 2 * MAX_NONEXHAUSTIVE_PAIRS;
            std::partial_sort(sortRecs.begin(), sortRecs.begin() + end, sortRecs.begin() + n);
            لكل (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        }
    }

    فراغ removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        طالما (SiblingMC* ثابت smcp = mtaskData(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        طالما (SiblingMC* ثابت smcp = mtaskData(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    فراغ contract(MergeCandidate* mergeCanp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* لووتکە;
        إذا (ثابت EdgeMC* ثابت edgeMCp = mergeCanp->toEdgeMC()) {
            fromp = edgeMCp->edgep()->fromMTaskp();
            لووتکە = edgeMCp->edgep()->toMTaskp();
        } وإلا {
            ثابت SiblingMC* ثابت sibMCp = mergeCanp->toSiblingMC();
            fromp = sibMCp->bp();
            لووتکە = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        إذا (fromp->cost() > لووتکە->cost()) {
            recipientp = fromp;
            donorp = لووتکە;
        } وإلا {
            donorp = fromp;
            recipientp = لووتکە;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(لووتکە);  // Use donorp and recipientp now instead

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
        إذا (VL_UNLIKELY(debug() >= 9)) m_mTaskGraph.validate();

        // Add the EdgeMCs of the merged MTask
        addEdgeMCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        addSiblingMCsFromRelatives<GraphWay::REVERSE, true>(recipientp);
        addSiblingMCsFromRelatives<GraphWay::FORWARD, true>(recipientp);
        unsigned edges = 0;
        لكل (V3GraphEdge& edge : recipientp->outEdges()) {
            LogicMTask* ثابت postreqp = static_cast<LogicMTask*>(edge.لووتکە());
            addSiblingMCsFromRelatives<GraphWay::REVERSE, خطأ>(postreqp);
            ++edges;
            إذا (edges >= PART_SIBLING_EDGE_LIMIT) اكسر;
        }
        edges = 0;
        لكل (V3GraphEdge& edge : recipientp->inEdges()) {
            LogicMTask* ثابت prereqp = static_cast<LogicMTask*>(edge.fromp());
            addSiblingMCsFromRelatives<GraphWay::FORWARD, خطأ>(prereqp);
            ++edges;
            إذا (edges >= PART_SIBLING_EDGE_LIMIT) اكسر;
        }
    }

    // CONSTRUCTORS
    Contraction(OrderMTaskGraph& mTaskGraph, uint64_t cpLimit)
        : m_mTaskGraph{mTaskGraph}
        , m_scoreLimit{cpLimit} {

        // Check the graph we were given is consistent.
        m_mTaskGraph.validate();

        // Figure out maximum number of MTasks
        ثابت uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ثابت صحيح given = v3Global.opt.threadsMaxMTasks();
            إذا (given > 0) أرجع given;
            // Unspecified so estimate
            أرجع PART_DEFAULT_MAX_MTASKS_PER_THREAD * v3Global.opt.threads();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ثابت size_t nMTasks = m_mTaskGraph.vertices().size();
            m_mtaskDatap.reset(جديد MTaskContractionData[nMTasks]);
            size_t i = 0;
            لكل (V3GraphVertex& vtx : m_mTaskGraph.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        لكل (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
            لكل (V3GraphEdge& edge : vtx.outEdges())
                m_sb.addEdgeMC(static_cast<MTaskEdge*>(&edge));
            LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(&vtx);
            addSiblingMCsFromRelatives<GraphWay::REVERSE, true>(mtaskp);
            addSiblingMCsFromRelatives<GraphWay::FORWARD, true>(mtaskp);
        }

        طالما (true) {
            // Pick the candidate yielding the lowest local critical path.
            MergeCandidate* ثابت mergeCanp = m_sb.best();
            إذا (!mergeCanp) اكسر;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ثابت uint64_t score = mergeCanp->currentScore();
            إذا (score != mergeCanp->score()) {
                m_sb.rescore(mergeCanp);
                استمر;
            }

            // Check if the critical path limit is reached.
            إذا (score > m_scoreLimit) {

                // If there are still too many MTasks, raise the limit and keep going
                ثابت unsigned mtask_ژماردن = m_mTaskGraph.vertices().size();
                إذا (mtask_ژماردن > maxMTasks) {
                    m_scoreLimit = (m_scoreLimit * 120) / 100;
                    File_هێڵ* ثابت flp = v3Global.rootp()->fileline();
                    إذا (!flp->warnIsOff(V3ErrorCode::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3ErrorCode::UNOPTTHREADS, true);
                    }
                    استمر;
                }

                // MTasks limit and CP limit reached. Stop.
                اكسر;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            إذا (EdgeMC* ثابت edgeMCp = mergeCanp->toEdgeMC()) {
                MTaskEdge* ثابت edgep = edgeMCp->edgep();
                إذا (edgep->fromp() == m_mTaskGraph.entryp()
                    || edgep->لووتکە() == m_mTaskGraph.exitp()) {
                    m_sb.removeEdgeMC(edgep);
                    استمر;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            إذا (mergeCanp->mergeWouldCreateCycle(m_mTaskGraph)) {
                m_sb.removeMC(mergeCanp);
                استمر;
            }

            // Merge this candidate
            contract(mergeCanp);
        }

        // Free all remaining merge candidates.
        طالما (MergeCandidate* ثابت mergeCanp = m_sb.best()) m_sb.removeMC(mergeCanp);
    }

عام:
    ساكن فراغ apply(OrderMTaskGraph& mTaskGraph, uint64_t scoreLimit) {
        Contraction{mTaskGraph, scoreLimit};
    }
};

//######################################################################
// OrderMTaskGraph entry point

فراغ OrderMTaskGraph::contract(OrderMTaskGraph& mtaskGraph, uint64_t scoreLimit) {
    Contraction::apply(mtaskGraph, scoreLimit);
}
