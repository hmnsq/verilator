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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3OrderMTaskGraph.h"
#അന്തര്ഭാവ "V3PairingHeap.h"
#അന്തര്ഭാവ "V3PoolAllocator.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <array>
#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <type_traits>
#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം MergeCandidate;
വർഗ്ഗം SiblingMC;
വർഗ്ഗം EdgeMC;

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

ഘടന Merge_Candidate_താക്കോൽ final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    തർക്കം operator<(സ്ഥിരം Merge_Candidate_താക്കോൽ& other) സ്ഥിരം {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        തിരികെ m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
using Merge_Candidate_Heap_കണ്ണി = PairingHeap<Merge_Candidate_താക്കോൽ>::കണ്ണി_2;

// Information associated with scoreboarding a merge candidate
വർഗ്ഗം MergeCandidate VL_NOT_FINAL : പൊതു Merge_Candidate_Heap_കണ്ണി {
    // Only the known subclasses can create or delete one of these
    friend വർഗ്ഗം SiblingMC;
    friend വർഗ്ഗം EdgeMC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    സ്ഥിതം constexpr uint64_t IS_SIBLING_MASK = 1ULL << 0;
    സ്ഥിതം constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    തർക്കം isSiblingMC() സ്ഥിരം { തിരികെ m_താക്കോൽ.m_id & IS_SIBLING_MASK; }

    // CONSTRUCTORS
    explicit MergeCandidate(തർക്കം isSiblingMC) {
        സ്ഥിതം uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_താക്കോൽ.m_id = s_serial | (isSiblingMC * IS_SIBLING_MASK);
    }
    ~MergeCandidate() = സ്ഥിരസ്ഥിതി;

പൊതു:
    // METHODS
    SiblingMC* toSiblingMC();  // Instead of cast<>/as<>
    EdgeMC* toEdgeMC();  // Instead of cast<>/as<>
    തർക്കം mergeWouldCreateCycle(OrderMTaskGraph& graph);

    // The current score of this candidate, which changes as the graph is contracted
    വരിയിൽ uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() സ്ഥിരം { തിരികെ m_താക്കോൽ.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    ശൂന്യം updateScore() { m_താക്കോൽ.m_score = currentScore(); }

    സ്ഥിതം MergeCandidate* heap_കണ്ണി_To_Elem(Merge_Candidate_Heap_കണ്ണി* nodep) {
        തിരികെ static_cast<MergeCandidate*>(nodep);
    }
};

static_assert(വലുപ്പം(MergeCandidate) == വലുപ്പം(Merge_Candidate_Heap_കണ്ണി),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
വർഗ്ഗം SiblingMC final : പൊതു MergeCandidate {
    LogicMTask* സ്ഥിരം m_ap;  // The higher ID MTask
    LogicMTask* സ്ഥിരം m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { തിരികെ m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { തിരികെ m_bLinks; }

പൊതു:
    // List type to store instances of this class
    using AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    using BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = സ്ഥിരസ്ഥിതി;

    // METHODS
    ശൂന്യം unlinkA();
    ശൂന്യം unlinkB();

    LogicMTask* ap() സ്ഥിരം { തിരികെ m_ap; }
    LogicMTask* bp() സ്ഥിരം { തിരികെ m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::മൂല്യം, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
വർഗ്ഗം EdgeMC final : പൊതു MergeCandidate {
    MTaskEdge* സ്ഥിരം m_edgep;  // The associated edge

പൊതു:
    // CONSTRUCTORS
    explicit EdgeMC(MTaskEdge* edgep)
        : MergeCandidate{/* isSiblingMC: */ അസത്യം}
        , m_edgep{edgep} {}
    ~EdgeMC() = സ്ഥിരസ്ഥിതി;

    // METHODS
    MTaskEdge* edgep() സ്ഥിരം { തിരികെ m_edgep; }
};

static_assert(!std::is_polymorphic<EdgeMC>::മൂല്യം, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
ഘടന MTaskContractionData final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_set<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
സ്ഥിതം MTaskContractionData& mtaskData(സ്ഥിരം LogicMTask* mtaskp) {
    തിരികെ *static_cast<MTaskContractionData*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
സ്ഥിതം EdgeMC* edgeMC(സ്ഥിരം MTaskEdge* edgep) { തിരികെ static_cast<EdgeMC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* MergeCandidate::toSiblingMC() {
    തിരികെ isSiblingMC() ? static_cast<SiblingMC*>(ഇത്) : nullptr;
}

EdgeMC* MergeCandidate::toEdgeMC() { തിരികെ isSiblingMC() ? nullptr : static_cast<EdgeMC*>(ഇത്); }

തർക്കം MergeCandidate::mergeWouldCreateCycle(OrderMTaskGraph& graph) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    എങ്കിൽ (സ്ഥിരം SiblingMC* സ്ഥിരം sibp = toSiblingMC()) {
        തിരികെ graph.pathExists(sibp->ap(), sibp->bp(), nullptr)
               || graph.pathExists(sibp->bp(), sibp->ap(), nullptr);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTaskEdge* സ്ഥിരം edgep = toEdgeMC()->edgep();
    തിരികെ graph.pathExists(edgep->fromMTaskp(), edgep->toMTaskp(), edgep);
}

uint64_t MergeCandidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    എങ്കിൽ (സ്ഥിരം SiblingMC* സ്ഥിരം sibp = toSiblingMC()) {
        സ്ഥിരം LogicMTask* സ്ഥിരം ap = sibp->ap();
        സ്ഥിരം LogicMTask* സ്ഥിരം bp = sibp->bp();
        സ്ഥിരം uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<GraphWay::FORWARD>(), bp->cpExclusive<GraphWay::FORWARD>());
        സ്ഥിരം uint64_t mergedCpRev
            = std::max(ap->cpExclusive<GraphWay::REVERSE>(), bp->cpExclusive<GraphWay::REVERSE>());
        തിരികെ mergedCpRev + mergedCpFwd + ap->cost() + bp->cost();
    }

    // Edge merge
    {
        MTaskEdge* സ്ഥിരം edgep = toEdgeMC()->edgep();
        സ്ഥിരം LogicMTask* സ്ഥിരം fromp = edgep->fromMTaskp();
        സ്ഥിരം LogicMTask* സ്ഥിരം മുകൾ = edgep->toMTaskp();
        സ്ഥിരം uint64_t mergedCpFwd = std::max(fromp->cpExclusive<GraphWay::FORWARD>(),
                                              മുകൾ->cpExclusiveWithout<GraphWay::FORWARD>(edgep));
        സ്ഥിരം uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<GraphWay::REVERSE>(edgep),
                                              മുകൾ->cpExclusive<GraphWay::REVERSE>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        തിരികെ 1 + mergedCpRev + mergedCpFwd + fromp->cost() + മുകൾ->cost();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : MergeCandidate{/* isSiblingMC: */ സത്യം}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtaskData(ap).siblings.എണ്ണം(bp), "Should be in sibling map"););
    mtaskData(m_ap).aSiblingMCs.linkBack(ഇത്);
    mtaskData(m_bp).bSiblingMCs.linkBack(ഇത്);
}

ശൂന്യം SiblingMC::unlinkA() {
    VL_ATTR_UNUSED സ്ഥിരം size_t removed = mtaskData(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtaskData(m_ap).aSiblingMCs.unlink(ഇത്);
}

ശൂന്യം SiblingMC::unlinkB() { mtaskData(m_bp).bSiblingMCs.unlink(ഇത്); }

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
വർഗ്ഗം MergeCandidateScoreboard final {
    // TYPES
    using Heap = PairingHeap<Merge_Candidate_താക്കോൽ>;

    // MEMBERS
    Heap m_heap;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<EdgeMC> m_edgeMCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    ശൂന്യം തിരുകുക(MergeCandidate* nodep) {
        nodep->updateScore();
        m_heap.തിരുകുക(nodep);
    }

    // Remove a candidate from the scoreboard.
    ശൂന്യം remove(MergeCandidate* nodep) { m_heap.remove(nodep); }

പൊതു:
    // CONSTRUCTORS
    MergeCandidateScoreboard() = സ്ഥിരസ്ഥിതി;
    ~MergeCandidateScoreboard() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(MergeCandidateScoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    MergeCandidate* best() സ്ഥിരം { തിരികെ MergeCandidate::heap_കണ്ണി_To_Elem(m_heap.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    ശൂന്യം rescore(MergeCandidate* nodep) {
        remove(nodep);
        തിരുകുക(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    ശൂന്യം addEdgeMC(MTaskEdge* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        EdgeMC* സ്ഥിരം edgeMCp = m_edgeMCPool.alloc(edgep);
        edgep->userp(edgeMCp);
        തിരുകുക(edgeMCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    ശൂന്യം removeEdgeMC(MTaskEdge* edgep) {
        EdgeMC* സ്ഥിരം edgeMCp = edgeMC(edgep);
        UDEBUGONLY(UASSERT(edgeMCp, "Edge has no merge candidate"););
        edgep->userp(nullptr);
        remove(edgeMCp);
        VL_DO_DANGLING(m_edgeMCPool.സ്വതന്ത്രം(edgeMCp), edgeMCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    ശൂന്യം addSiblingMC(LogicMTask* ap, LogicMTask* bp) { തിരുകുക(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    ശൂന്യം removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.സ്വതന്ത്രം(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    ശൂന്യം removeMC(MergeCandidate* nodep) {
        എങ്കിൽ (SiblingMC* സ്ഥിരം smcp = nodep->toSiblingMC()) {
            removeSiblingMC(smcp);
        } അല്ലെങ്കിൽ {
            removeEdgeMC(nodep->toEdgeMC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
വർഗ്ഗം Contraction final {
    // MEMBERS
    OrderMTaskGraph& m_mTaskGraph;  // The Mtask graph
    uint64_t m_scoreLimit;  // Critical path limit for merges
    MergeCandidateScoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTaskContractionData[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    ശൂന്യം addEdgeMCs(LogicMTask* mtaskp) {
        വേണ്ടി (V3GraphEdge& e : mtaskp->outEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
        വേണ്ടി (V3GraphEdge& e : mtaskp->inEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    ശൂന്യം removeEdgeMCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        വേണ്ടി (V3GraphEdge& edge : mtaskp->outEdges()) {
            MTaskEdge* സ്ഥിരം edgep = static_cast<MTaskEdge*>(&edge);
            എങ്കിൽ (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
        വേണ്ടി (V3GraphEdge& edge : mtaskp->inEdges()) {
            MTaskEdge* സ്ഥിരം edgep = static_cast<MTaskEdge*>(&edge);
            എങ്കിൽ (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
    }

    ശൂന്യം makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        എങ്കിൽ (ap->id() < bp->id()) std::swap(ap, bp);
        // The higher id vertex owns the association set
        സ്ഥിരം തർക്കം first = mtaskData(ap).siblings.തിരുകുക(bp).second;
        എങ്കിൽ (first) {
            m_sb.addSiblingMC(ap, bp);
            തിരികെ;
        }

        എങ്കിൽ (VL_UNLIKELY(m_mTaskGraph.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            തർക്കം found = അസത്യം;
            വേണ്ടി (സ്ഥിരം SiblingMC& smc : mtaskData(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                എങ്കിൽ (smc.bp() == bp) found = സത്യം;
            }
            UASSERT_OBJ(found, ap, "Sibling not found");
        }
    }

    template <GraphWay::en N_Way, തർക്കം N_Exhaustive>
    ശൂന്യം addSiblingMCsFromRelatives(LogicMTask* mtaskp) {
        constexpr GraphWay way{N_Way};
        // Need at least 2 edges
        auto& edges = mtaskp->edges<way>();
        എങ്കിൽ (!edges.hasMultipleElements()) തിരികെ;

        std::array<LogicMTask*, PART_SIBLING_EDGE_LIMIT> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        ഘടന alignas(16) SortingRecord final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_EDGE_LIMIT <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            തർക്കം operator<(സ്ഥിരം SortingRecord& that) സ്ഥിരം {
                തിരികെ m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(വലുപ്പം(SortingRecord) <= 16, "How could this be padded to more than 16?");

        std::array<SortingRecord, PART_SIBLING_EDGE_LIMIT> sortRecs;
        size_t n = 0;

        // Populate the buffers
        വേണ്ടി (V3GraphEdge& edge : mtaskp->edges<way>()) {
            LogicMTask* സ്ഥിരം otherp = static_cast<LogicMTask*>(edge.furtherp<way>());
            neighbors[n] = otherp;
            sortRecs[n].m_id = otherp->id();
            sortRecs[n].m_cp = otherp->cpInclusive<way>();
            sortRecs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            എങ്കിൽ (n >= PART_SIBLING_EDGE_LIMIT) നിർത്തുക;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr size_t MAX_NONEXHAUSTIVE_PAIRS = 3;

        എങ്കിൽ (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_PAIRS) {
            സ്ഥിരം size_t end = n & ~static_cast<size_t>(1);  // Round down to even, (we want pairs)
            std::sort(sortRecs.begin(), sortRecs.begin() + n);
            വേണ്ടി (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        } അല്ലെങ്കിൽ {
            constexpr size_t end = 2 * MAX_NONEXHAUSTIVE_PAIRS;
            std::partial_sort(sortRecs.begin(), sortRecs.begin() + end, sortRecs.begin() + n);
            വേണ്ടി (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        }
    }

    ശൂന്യം removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        ആകുമ്പോൾ (SiblingMC* സ്ഥിരം smcp = mtaskData(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        ആകുമ്പോൾ (SiblingMC* സ്ഥിരം smcp = mtaskData(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    ശൂന്യം contract(MergeCandidate* mergeCanp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* മുകൾ;
        എങ്കിൽ (സ്ഥിരം EdgeMC* സ്ഥിരം edgeMCp = mergeCanp->toEdgeMC()) {
            fromp = edgeMCp->edgep()->fromMTaskp();
            മുകൾ = edgeMCp->edgep()->toMTaskp();
        } അല്ലെങ്കിൽ {
            സ്ഥിരം SiblingMC* സ്ഥിരം sibMCp = mergeCanp->toSiblingMC();
            fromp = sibMCp->bp();
            മുകൾ = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        എങ്കിൽ (fromp->cost() > മുകൾ->cost()) {
            recipientp = fromp;
            donorp = മുകൾ;
        } അല്ലെങ്കിൽ {
            donorp = fromp;
            recipientp = മുകൾ;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(മുകൾ);  // Use donorp and recipientp now instead

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
        എങ്കിൽ (VL_UNLIKELY(debug() >= 9)) m_mTaskGraph.validate();

        // Add the EdgeMCs of the merged MTask
        addEdgeMCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        addSiblingMCsFromRelatives<GraphWay::REVERSE, സത്യം>(recipientp);
        addSiblingMCsFromRelatives<GraphWay::FORWARD, സത്യം>(recipientp);
        unsigned edges = 0;
        വേണ്ടി (V3GraphEdge& edge : recipientp->outEdges()) {
            LogicMTask* സ്ഥിരം postreqp = static_cast<LogicMTask*>(edge.മുകൾ());
            addSiblingMCsFromRelatives<GraphWay::REVERSE, അസത്യം>(postreqp);
            ++edges;
            എങ്കിൽ (edges >= PART_SIBLING_EDGE_LIMIT) നിർത്തുക;
        }
        edges = 0;
        വേണ്ടി (V3GraphEdge& edge : recipientp->inEdges()) {
            LogicMTask* സ്ഥിരം prereqp = static_cast<LogicMTask*>(edge.fromp());
            addSiblingMCsFromRelatives<GraphWay::FORWARD, അസത്യം>(prereqp);
            ++edges;
            എങ്കിൽ (edges >= PART_SIBLING_EDGE_LIMIT) നിർത്തുക;
        }
    }

    // CONSTRUCTORS
    Contraction(OrderMTaskGraph& mTaskGraph, uint64_t cpLimit)
        : m_mTaskGraph{mTaskGraph}
        , m_scoreLimit{cpLimit} {

        // Check the graph we were given is consistent.
        m_mTaskGraph.validate();

        // Figure out maximum number of MTasks
        സ്ഥിരം uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            സ്ഥിരം പൂർണ്ണസംഖ്യ given = v3Global.opt.threadsMaxMTasks();
            എങ്കിൽ (given > 0) തിരികെ given;
            // Unspecified so estimate
            തിരികെ PART_DEFAULT_MAX_MTASKS_PER_THREAD * v3Global.opt.threads();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            സ്ഥിരം size_t nMTasks = m_mTaskGraph.vertices().size();
            m_mtaskDatap.reset(പുതിയ MTaskContractionData[nMTasks]);
            size_t i = 0;
            വേണ്ടി (V3GraphVertex& vtx : m_mTaskGraph.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        വേണ്ടി (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
            വേണ്ടി (V3GraphEdge& edge : vtx.outEdges())
                m_sb.addEdgeMC(static_cast<MTaskEdge*>(&edge));
            LogicMTask* സ്ഥിരം mtaskp = static_cast<LogicMTask*>(&vtx);
            addSiblingMCsFromRelatives<GraphWay::REVERSE, സത്യം>(mtaskp);
            addSiblingMCsFromRelatives<GraphWay::FORWARD, സത്യം>(mtaskp);
        }

        ആകുമ്പോൾ (സത്യം) {
            // Pick the candidate yielding the lowest local critical path.
            MergeCandidate* സ്ഥിരം mergeCanp = m_sb.best();
            എങ്കിൽ (!mergeCanp) നിർത്തുക;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            സ്ഥിരം uint64_t score = mergeCanp->currentScore();
            എങ്കിൽ (score != mergeCanp->score()) {
                m_sb.rescore(mergeCanp);
                തുടരുക;
            }

            // Check if the critical path limit is reached.
            എങ്കിൽ (score > m_scoreLimit) {

                // If there are still too many MTasks, raise the limit and keep going
                സ്ഥിരം unsigned mtask_എണ്ണം = m_mTaskGraph.vertices().size();
                എങ്കിൽ (mtask_എണ്ണം > maxMTasks) {
                    m_scoreLimit = (m_scoreLimit * 120) / 100;
                    File_വരി* സ്ഥിരം flp = v3Global.rootp()->fileline();
                    എങ്കിൽ (!flp->warnIsOff(V3ErrorCode::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3ErrorCode::UNOPTTHREADS, സത്യം);
                    }
                    തുടരുക;
                }

                // MTasks limit and CP limit reached. Stop.
                നിർത്തുക;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            എങ്കിൽ (EdgeMC* സ്ഥിരം edgeMCp = mergeCanp->toEdgeMC()) {
                MTaskEdge* സ്ഥിരം edgep = edgeMCp->edgep();
                എങ്കിൽ (edgep->fromp() == m_mTaskGraph.entryp()
                    || edgep->മുകൾ() == m_mTaskGraph.exitp()) {
                    m_sb.removeEdgeMC(edgep);
                    തുടരുക;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            എങ്കിൽ (mergeCanp->mergeWouldCreateCycle(m_mTaskGraph)) {
                m_sb.removeMC(mergeCanp);
                തുടരുക;
            }

            // Merge this candidate
            contract(mergeCanp);
        }

        // Free all remaining merge candidates.
        ആകുമ്പോൾ (MergeCandidate* സ്ഥിരം mergeCanp = m_sb.best()) m_sb.removeMC(mergeCanp);
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(OrderMTaskGraph& mTaskGraph, uint64_t scoreLimit) {
        Contraction{mTaskGraph, scoreLimit};
    }
};

//######################################################################
// OrderMTaskGraph entry point

ശൂന്യം OrderMTaskGraph::contract(OrderMTaskGraph& mtaskGraph, uint64_t scoreLimit) {
    Contraction::apply(mtaskGraph, scoreLimit);
}
