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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderMTaskGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3PairingHeap.h"
#ଅନ୍ତର୍ଭାଵ "V3PoolAllocator.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <array>
#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <type_traits>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ MergeCandidate;
ଶ୍ରେଣୀ SiblingMC;
ଶ୍ରେଣୀ EdgeMC;

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

ସଂରଚନା Merge_Candidate_ଚାବି final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    ତାର୍କିକ operator<(ଅଚଳ Merge_Candidate_ଚାବି& other) ଅଚଳ {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        ଫେରନ୍ତୁ m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
using Merge_Candidate_Heap_ଗଣ୍ଠି = PairingHeap<Merge_Candidate_ଚାବି>::ଗଣ୍ଠି_2;

// Information associated with scoreboarding a merge candidate
ଶ୍ରେଣୀ MergeCandidate VL_NOT_FINAL : ସର୍ବସାଧାରଣ Merge_Candidate_Heap_ଗଣ୍ଠି {
    // Only the known subclasses can create or delete one of these
    friend ଶ୍ରେଣୀ SiblingMC;
    friend ଶ୍ରେଣୀ EdgeMC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ସ୍ଥିର constexpr uint64_t IS_SIBLING_MASK = 1ULL << 0;
    ସ୍ଥିର constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    ତାର୍କିକ isSiblingMC() ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବି.m_id & IS_SIBLING_MASK; }

    // CONSTRUCTORS
    explicit MergeCandidate(ତାର୍କିକ isSiblingMC) {
        ସ୍ଥିର uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_ଚାବି.m_id = s_serial | (isSiblingMC * IS_SIBLING_MASK);
    }
    ~MergeCandidate() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    // METHODS
    SiblingMC* toSiblingMC();  // Instead of cast<>/as<>
    EdgeMC* toEdgeMC();  // Instead of cast<>/as<>
    ତାର୍କିକ mergeWouldCreateCycle(OrderMTaskGraph& graph);

    // The current score of this candidate, which changes as the graph is contracted
    ଧାଡ଼ିଭିତରେ uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବି.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    ଶୂନ୍ୟ updateScore() { m_ଚାବି.m_score = currentScore(); }

    ସ୍ଥିର MergeCandidate* heap_ଗଣ୍ଠି_To_Elem(Merge_Candidate_Heap_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ static_cast<MergeCandidate*>(nodep);
    }
};

static_assert(ଆକାର(MergeCandidate) == ଆକାର(Merge_Candidate_Heap_ଗଣ୍ଠି),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
ଶ୍ରେଣୀ SiblingMC final : ସର୍ବସାଧାରଣ MergeCandidate {
    LogicMTask* ଅଚଳ m_ap;  // The higher ID MTask
    LogicMTask* ଅଚଳ m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { ଫେରନ୍ତୁ m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { ଫେରନ୍ତୁ m_bLinks; }

ସର୍ବସାଧାରଣ:
    // List type to store instances of this class
    using AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    using BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    ଶୂନ୍ୟ unlinkA();
    ଶୂନ୍ୟ unlinkB();

    LogicMTask* ap() ଅଚଳ { ଫେରନ୍ତୁ m_ap; }
    LogicMTask* bp() ଅଚଳ { ଫେରନ୍ତୁ m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::ମୂଲ୍ୟ, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
ଶ୍ରେଣୀ EdgeMC final : ସର୍ବସାଧାରଣ MergeCandidate {
    MTaskEdge* ଅଚଳ m_edgep;  // The associated edge

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit EdgeMC(MTaskEdge* edgep)
        : MergeCandidate{/* isSiblingMC: */ ମିଥ୍ୟା}
        , m_edgep{edgep} {}
    ~EdgeMC() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    MTaskEdge* edgep() ଅଚଳ { ଫେରନ୍ତୁ m_edgep; }
};

static_assert(!std::is_polymorphic<EdgeMC>::ମୂଲ୍ୟ, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
ସଂରଚନା MTaskContractionData final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_set<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
ସ୍ଥିର MTaskContractionData& mtaskData(ଅଚଳ LogicMTask* mtaskp) {
    ଫେରନ୍ତୁ *static_cast<MTaskContractionData*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
ସ୍ଥିର EdgeMC* edgeMC(ଅଚଳ MTaskEdge* edgep) { ଫେରନ୍ତୁ static_cast<EdgeMC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* MergeCandidate::toSiblingMC() {
    ଫେରନ୍ତୁ isSiblingMC() ? static_cast<SiblingMC*>(ଏହି) : nullptr;
}

EdgeMC* MergeCandidate::toEdgeMC() { ଫେରନ୍ତୁ isSiblingMC() ? nullptr : static_cast<EdgeMC*>(ଏହି); }

ତାର୍କିକ MergeCandidate::mergeWouldCreateCycle(OrderMTaskGraph& graph) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    ଯଦି (ଅଚଳ SiblingMC* ଅଚଳ sibp = toSiblingMC()) {
        ଫେରନ୍ତୁ graph.pathExists(sibp->ap(), sibp->bp(), nullptr)
               || graph.pathExists(sibp->bp(), sibp->ap(), nullptr);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTaskEdge* ଅଚଳ edgep = toEdgeMC()->edgep();
    ଫେରନ୍ତୁ graph.pathExists(edgep->fromMTaskp(), edgep->toMTaskp(), edgep);
}

uint64_t MergeCandidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    ଯଦି (ଅଚଳ SiblingMC* ଅଚଳ sibp = toSiblingMC()) {
        ଅଚଳ LogicMTask* ଅଚଳ ap = sibp->ap();
        ଅଚଳ LogicMTask* ଅଚଳ bp = sibp->bp();
        ଅଚଳ uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<GraphWay::FORWARD>(), bp->cpExclusive<GraphWay::FORWARD>());
        ଅଚଳ uint64_t mergedCpRev
            = std::max(ap->cpExclusive<GraphWay::REVERSE>(), bp->cpExclusive<GraphWay::REVERSE>());
        ଫେରନ୍ତୁ mergedCpRev + mergedCpFwd + ap->cost() + bp->cost();
    }

    // Edge merge
    {
        MTaskEdge* ଅଚଳ edgep = toEdgeMC()->edgep();
        ଅଚଳ LogicMTask* ଅଚଳ fromp = edgep->fromMTaskp();
        ଅଚଳ LogicMTask* ଅଚଳ ଶୀର୍ଷ = edgep->toMTaskp();
        ଅଚଳ uint64_t mergedCpFwd = std::max(fromp->cpExclusive<GraphWay::FORWARD>(),
                                              ଶୀର୍ଷ->cpExclusiveWithout<GraphWay::FORWARD>(edgep));
        ଅଚଳ uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<GraphWay::REVERSE>(edgep),
                                              ଶୀର୍ଷ->cpExclusive<GraphWay::REVERSE>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        ଫେରନ୍ତୁ 1 + mergedCpRev + mergedCpFwd + fromp->cost() + ଶୀର୍ଷ->cost();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : MergeCandidate{/* isSiblingMC: */ ସତ୍ୟ}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtaskData(ap).siblings.ଗଣନା_2(bp), "Should be in sibling map"););
    mtaskData(m_ap).aSiblingMCs.linkBack(ଏହି);
    mtaskData(m_bp).bSiblingMCs.linkBack(ଏହି);
}

ଶୂନ୍ୟ SiblingMC::unlinkA() {
    VL_ATTR_UNUSED ଅଚଳ size_t removed = mtaskData(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtaskData(m_ap).aSiblingMCs.unlink(ଏହି);
}

ଶୂନ୍ୟ SiblingMC::unlinkB() { mtaskData(m_bp).bSiblingMCs.unlink(ଏହି); }

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
ଶ୍ରେଣୀ MergeCandidateScoreboard final {
    // TYPES
    using Heap = PairingHeap<Merge_Candidate_ଚାବି>;

    // MEMBERS
    Heap m_heap;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<EdgeMC> m_edgeMCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    ଶୂନ୍ୟ ଭର୍ତ୍ତି(MergeCandidate* nodep) {
        nodep->updateScore();
        m_heap.ଭର୍ତ୍ତି(nodep);
    }

    // Remove a candidate from the scoreboard.
    ଶୂନ୍ୟ remove(MergeCandidate* nodep) { m_heap.remove(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    MergeCandidateScoreboard() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~MergeCandidateScoreboard() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(MergeCandidateScoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    MergeCandidate* best() ଅଚଳ { ଫେରନ୍ତୁ MergeCandidate::heap_ଗଣ୍ଠି_To_Elem(m_heap.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    ଶୂନ୍ୟ rescore(MergeCandidate* nodep) {
        remove(nodep);
        ଭର୍ତ୍ତି(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    ଶୂନ୍ୟ addEdgeMC(MTaskEdge* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        EdgeMC* ଅଚଳ edgeMCp = m_edgeMCPool.alloc(edgep);
        edgep->userp(edgeMCp);
        ଭର୍ତ୍ତି(edgeMCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    ଶୂନ୍ୟ removeEdgeMC(MTaskEdge* edgep) {
        EdgeMC* ଅଚଳ edgeMCp = edgeMC(edgep);
        UDEBUGONLY(UASSERT(edgeMCp, "Edge has no merge candidate"););
        edgep->userp(nullptr);
        remove(edgeMCp);
        VL_DO_DANGLING(m_edgeMCPool.ମୁକ୍ତ(edgeMCp), edgeMCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    ଶୂନ୍ୟ addSiblingMC(LogicMTask* ap, LogicMTask* bp) { ଭର୍ତ୍ତି(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    ଶୂନ୍ୟ removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.ମୁକ୍ତ(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    ଶୂନ୍ୟ removeMC(MergeCandidate* nodep) {
        ଯଦି (SiblingMC* ଅଚଳ smcp = nodep->toSiblingMC()) {
            removeSiblingMC(smcp);
        } ଅନ୍ୟଥା {
            removeEdgeMC(nodep->toEdgeMC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
ଶ୍ରେଣୀ Contraction final {
    // MEMBERS
    OrderMTaskGraph& m_mTaskGraph;  // The Mtask graph
    uint64_t m_scoreLimit;  // Critical path limit for merges
    MergeCandidateScoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTaskContractionData[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    ଶୂନ୍ୟ addEdgeMCs(LogicMTask* mtaskp) {
        ପାଇଁ (V3GraphEdge& e : mtaskp->outEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
        ପାଇଁ (V3GraphEdge& e : mtaskp->inEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    ଶୂନ୍ୟ removeEdgeMCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        ପାଇଁ (V3GraphEdge& edge : mtaskp->outEdges()) {
            MTaskEdge* ଅଚଳ edgep = static_cast<MTaskEdge*>(&edge);
            ଯଦି (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
        ପାଇଁ (V3GraphEdge& edge : mtaskp->inEdges()) {
            MTaskEdge* ଅଚଳ edgep = static_cast<MTaskEdge*>(&edge);
            ଯଦି (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
    }

    ଶୂନ୍ୟ makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        ଯଦି (ap->id() < bp->id()) std::swap(ap, bp);
        // The higher id vertex owns the association set
        ଅଚଳ ତାର୍କିକ first = mtaskData(ap).siblings.ଭର୍ତ୍ତି(bp).second;
        ଯଦି (first) {
            m_sb.addSiblingMC(ap, bp);
            ଫେରନ୍ତୁ;
        }

        ଯଦି (VL_UNLIKELY(m_mTaskGraph.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            ତାର୍କିକ found = ମିଥ୍ୟା;
            ପାଇଁ (ଅଚଳ SiblingMC& smc : mtaskData(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                ଯଦି (smc.bp() == bp) found = ସତ୍ୟ;
            }
            UASSERT_OBJ(found, ap, "Sibling not found");
        }
    }

    template <GraphWay::en N_Way, ତାର୍କିକ N_Exhaustive>
    ଶୂନ୍ୟ addSiblingMCsFromRelatives(LogicMTask* mtaskp) {
        constexpr GraphWay way{N_Way};
        // Need at least 2 edges
        auto& edges = mtaskp->edges<way>();
        ଯଦି (!edges.hasMultipleElements()) ଫେରନ୍ତୁ;

        std::array<LogicMTask*, PART_SIBLING_EDGE_LIMIT> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        ସଂରଚନା alignas(16) SortingRecord final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_EDGE_LIMIT <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            ତାର୍କିକ operator<(ଅଚଳ SortingRecord& that) ଅଚଳ {
                ଫେରନ୍ତୁ m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(ଆକାର(SortingRecord) <= 16, "How could this be padded to more than 16?");

        std::array<SortingRecord, PART_SIBLING_EDGE_LIMIT> sortRecs;
        size_t n = 0;

        // Populate the buffers
        ପାଇଁ (V3GraphEdge& edge : mtaskp->edges<way>()) {
            LogicMTask* ଅଚଳ otherp = static_cast<LogicMTask*>(edge.furtherp<way>());
            neighbors[n] = otherp;
            sortRecs[n].m_id = otherp->id();
            sortRecs[n].m_cp = otherp->cpInclusive<way>();
            sortRecs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            ଯଦି (n >= PART_SIBLING_EDGE_LIMIT) ଭାଙ୍ଗ;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr size_t MAX_NONEXHAUSTIVE_PAIRS = 3;

        ଯଦି (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_PAIRS) {
            ଅଚଳ size_t end = n & ~static_cast<size_t>(1);  // Round down to even, (we want pairs)
            std::sort(sortRecs.begin(), sortRecs.begin() + n);
            ପାଇଁ (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        } ଅନ୍ୟଥା {
            constexpr size_t end = 2 * MAX_NONEXHAUSTIVE_PAIRS;
            std::partial_sort(sortRecs.begin(), sortRecs.begin() + end, sortRecs.begin() + n);
            ପାଇଁ (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        }
    }

    ଶୂନ୍ୟ removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        ଯେତେବେଳେ (SiblingMC* ଅଚଳ smcp = mtaskData(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        ଯେତେବେଳେ (SiblingMC* ଅଚଳ smcp = mtaskData(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    ଶୂନ୍ୟ contract(MergeCandidate* mergeCanp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* ଶୀର୍ଷ;
        ଯଦି (ଅଚଳ EdgeMC* ଅଚଳ edgeMCp = mergeCanp->toEdgeMC()) {
            fromp = edgeMCp->edgep()->fromMTaskp();
            ଶୀର୍ଷ = edgeMCp->edgep()->toMTaskp();
        } ଅନ୍ୟଥା {
            ଅଚଳ SiblingMC* ଅଚଳ sibMCp = mergeCanp->toSiblingMC();
            fromp = sibMCp->bp();
            ଶୀର୍ଷ = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        ଯଦି (fromp->cost() > ଶୀର୍ଷ->cost()) {
            recipientp = fromp;
            donorp = ଶୀର୍ଷ;
        } ଅନ୍ୟଥା {
            donorp = fromp;
            recipientp = ଶୀର୍ଷ;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(ଶୀର୍ଷ);  // Use donorp and recipientp now instead

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
        ଯଦି (VL_UNLIKELY(debug() >= 9)) m_mTaskGraph.validate();

        // Add the EdgeMCs of the merged MTask
        addEdgeMCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        addSiblingMCsFromRelatives<GraphWay::REVERSE, ସତ୍ୟ>(recipientp);
        addSiblingMCsFromRelatives<GraphWay::FORWARD, ସତ୍ୟ>(recipientp);
        unsigned edges = 0;
        ପାଇଁ (V3GraphEdge& edge : recipientp->outEdges()) {
            LogicMTask* ଅଚଳ postreqp = static_cast<LogicMTask*>(edge.ଶୀର୍ଷ());
            addSiblingMCsFromRelatives<GraphWay::REVERSE, ମିଥ୍ୟା>(postreqp);
            ++edges;
            ଯଦି (edges >= PART_SIBLING_EDGE_LIMIT) ଭାଙ୍ଗ;
        }
        edges = 0;
        ପାଇଁ (V3GraphEdge& edge : recipientp->inEdges()) {
            LogicMTask* ଅଚଳ prereqp = static_cast<LogicMTask*>(edge.fromp());
            addSiblingMCsFromRelatives<GraphWay::FORWARD, ମିଥ୍ୟା>(prereqp);
            ++edges;
            ଯଦି (edges >= PART_SIBLING_EDGE_LIMIT) ଭାଙ୍ଗ;
        }
    }

    // CONSTRUCTORS
    Contraction(OrderMTaskGraph& mTaskGraph, uint64_t cpLimit)
        : m_mTaskGraph{mTaskGraph}
        , m_scoreLimit{cpLimit} {

        // Check the graph we were given is consistent.
        m_mTaskGraph.validate();

        // Figure out maximum number of MTasks
        ଅଚଳ uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା given = v3Global.opt.threadsMaxMTasks();
            ଯଦି (given > 0) ଫେରନ୍ତୁ given;
            // Unspecified so estimate
            ଫେରନ୍ତୁ PART_DEFAULT_MAX_MTASKS_PER_THREAD * v3Global.opt.threads();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ଅଚଳ size_t nMTasks = m_mTaskGraph.vertices().size();
            m_mtaskDatap.reset(ନୂତନ MTaskContractionData[nMTasks]);
            size_t i = 0;
            ପାଇଁ (V3GraphVertex& vtx : m_mTaskGraph.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        ପାଇଁ (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
            ପାଇଁ (V3GraphEdge& edge : vtx.outEdges())
                m_sb.addEdgeMC(static_cast<MTaskEdge*>(&edge));
            LogicMTask* ଅଚଳ mtaskp = static_cast<LogicMTask*>(&vtx);
            addSiblingMCsFromRelatives<GraphWay::REVERSE, ସତ୍ୟ>(mtaskp);
            addSiblingMCsFromRelatives<GraphWay::FORWARD, ସତ୍ୟ>(mtaskp);
        }

        ଯେତେବେଳେ (ସତ୍ୟ) {
            // Pick the candidate yielding the lowest local critical path.
            MergeCandidate* ଅଚଳ mergeCanp = m_sb.best();
            ଯଦି (!mergeCanp) ଭାଙ୍ଗ;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ଅଚଳ uint64_t score = mergeCanp->currentScore();
            ଯଦି (score != mergeCanp->score()) {
                m_sb.rescore(mergeCanp);
                ଜାରି;
            }

            // Check if the critical path limit is reached.
            ଯଦି (score > m_scoreLimit) {

                // If there are still too many MTasks, raise the limit and keep going
                ଅଚଳ unsigned mtask_ଗଣନା = m_mTaskGraph.vertices().size();
                ଯଦି (mtask_ଗଣନା > maxMTasks) {
                    m_scoreLimit = (m_scoreLimit * 120) / 100;
                    File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->fileline();
                    ଯଦି (!flp->warnIsOff(V3ErrorCode::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3ErrorCode::UNOPTTHREADS, ସତ୍ୟ);
                    }
                    ଜାରି;
                }

                // MTasks limit and CP limit reached. Stop.
                ଭାଙ୍ଗ;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            ଯଦି (EdgeMC* ଅଚଳ edgeMCp = mergeCanp->toEdgeMC()) {
                MTaskEdge* ଅଚଳ edgep = edgeMCp->edgep();
                ଯଦି (edgep->fromp() == m_mTaskGraph.entryp()
                    || edgep->ଶୀର୍ଷ() == m_mTaskGraph.exitp()) {
                    m_sb.removeEdgeMC(edgep);
                    ଜାରି;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            ଯଦି (mergeCanp->mergeWouldCreateCycle(m_mTaskGraph)) {
                m_sb.removeMC(mergeCanp);
                ଜାରି;
            }

            // Merge this candidate
            contract(mergeCanp);
        }

        // Free all remaining merge candidates.
        ଯେତେବେଳେ (MergeCandidate* ଅଚଳ mergeCanp = m_sb.best()) m_sb.removeMC(mergeCanp);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(OrderMTaskGraph& mTaskGraph, uint64_t scoreLimit) {
        Contraction{mTaskGraph, scoreLimit};
    }
};

//######################################################################
// OrderMTaskGraph entry point

ଶୂନ୍ୟ OrderMTaskGraph::contract(OrderMTaskGraph& mtaskGraph, uint64_t scoreLimit) {
    Contraction::apply(mtaskGraph, scoreLimit);
}
