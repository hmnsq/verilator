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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3OrderMTaskGraph.h"
#அந்தர்பாவ "V3PairingHeap.h"
#அந்தர்பாவ "V3PoolAllocator.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <array>
#அந்தர்பாவ <limits>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <type_traits>
#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு MergeCandidate;
வகுப்பு SiblingMC;
வகுப்பு EdgeMC;

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

கட்டமைப்பு Merge_Candidate_சாவி final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    தருக்க operator<(மாறிலி Merge_Candidate_சாவி& other) மாறிலி {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        திரும்பு m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
using Merge_Candidate_Heap_முனை = PairingHeap<Merge_Candidate_சாவி>::முனை_2;

// Information associated with scoreboarding a merge candidate
வகுப்பு MergeCandidate VL_NOT_FINAL : பொது Merge_Candidate_Heap_முனை {
    // Only the known subclasses can create or delete one of these
    friend வகுப்பு SiblingMC;
    friend வகுப்பு EdgeMC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    நிலையான constexpr uint64_t IS_SIBLING_MASK = 1ULL << 0;
    நிலையான constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    தருக்க isSiblingMC() மாறிலி { திரும்பு m_சாவி.m_id & IS_SIBLING_MASK; }

    // CONSTRUCTORS
    explicit MergeCandidate(தருக்க isSiblingMC) {
        நிலையான uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_சாவி.m_id = s_serial | (isSiblingMC * IS_SIBLING_MASK);
    }
    ~MergeCandidate() = இயல்பு;

பொது:
    // METHODS
    SiblingMC* toSiblingMC();  // Instead of cast<>/as<>
    EdgeMC* toEdgeMC();  // Instead of cast<>/as<>
    தருக்க mergeWouldCreateCycle(OrderMTaskGraph& graph);

    // The current score of this candidate, which changes as the graph is contracted
    உள்வரி uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() மாறிலி { திரும்பு m_சாவி.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    வெறுமை updateScore() { m_சாவி.m_score = currentScore(); }

    நிலையான MergeCandidate* heap_முனை_To_Elem(Merge_Candidate_Heap_முனை* nodep) {
        திரும்பு static_cast<MergeCandidate*>(nodep);
    }
};

static_assert(அளவு(MergeCandidate) == அளவு(Merge_Candidate_Heap_முனை),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
வகுப்பு SiblingMC final : பொது MergeCandidate {
    LogicMTask* மாறிலி m_ap;  // The higher ID MTask
    LogicMTask* மாறிலி m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { திரும்பு m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { திரும்பு m_bLinks; }

பொது:
    // List type to store instances of this class
    using AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    using BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = இயல்பு;

    // METHODS
    வெறுமை unlinkA();
    வெறுமை unlinkB();

    LogicMTask* ap() மாறிலி { திரும்பு m_ap; }
    LogicMTask* bp() மாறிலி { திரும்பு m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::மதிப்பு, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
வகுப்பு EdgeMC final : பொது MergeCandidate {
    MTaskEdge* மாறிலி m_edgep;  // The associated edge

பொது:
    // CONSTRUCTORS
    explicit EdgeMC(MTaskEdge* edgep)
        : MergeCandidate{/* isSiblingMC: */ பொய்}
        , m_edgep{edgep} {}
    ~EdgeMC() = இயல்பு;

    // METHODS
    MTaskEdge* edgep() மாறிலி { திரும்பு m_edgep; }
};

static_assert(!std::is_polymorphic<EdgeMC>::மதிப்பு, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
கட்டமைப்பு MTaskContractionData final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_set<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
நிலையான MTaskContractionData& mtaskData(மாறிலி LogicMTask* mtaskp) {
    திரும்பு *static_cast<MTaskContractionData*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
நிலையான EdgeMC* edgeMC(மாறிலி MTaskEdge* edgep) { திரும்பு static_cast<EdgeMC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* MergeCandidate::toSiblingMC() {
    திரும்பு isSiblingMC() ? static_cast<SiblingMC*>(இது) : nullptr;
}

EdgeMC* MergeCandidate::toEdgeMC() { திரும்பு isSiblingMC() ? nullptr : static_cast<EdgeMC*>(இது); }

தருக்க MergeCandidate::mergeWouldCreateCycle(OrderMTaskGraph& graph) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    எனில் (மாறிலி SiblingMC* மாறிலி sibp = toSiblingMC()) {
        திரும்பு graph.pathExists(sibp->ap(), sibp->bp(), nullptr)
               || graph.pathExists(sibp->bp(), sibp->ap(), nullptr);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTaskEdge* மாறிலி edgep = toEdgeMC()->edgep();
    திரும்பு graph.pathExists(edgep->fromMTaskp(), edgep->toMTaskp(), edgep);
}

uint64_t MergeCandidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    எனில் (மாறிலி SiblingMC* மாறிலி sibp = toSiblingMC()) {
        மாறிலி LogicMTask* மாறிலி ap = sibp->ap();
        மாறிலி LogicMTask* மாறிலி bp = sibp->bp();
        மாறிலி uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<GraphWay::FORWARD>(), bp->cpExclusive<GraphWay::FORWARD>());
        மாறிலி uint64_t mergedCpRev
            = std::max(ap->cpExclusive<GraphWay::REVERSE>(), bp->cpExclusive<GraphWay::REVERSE>());
        திரும்பு mergedCpRev + mergedCpFwd + ap->cost() + bp->cost();
    }

    // Edge merge
    {
        MTaskEdge* மாறிலி edgep = toEdgeMC()->edgep();
        மாறிலி LogicMTask* மாறிலி fromp = edgep->fromMTaskp();
        மாறிலி LogicMTask* மாறிலி உச்சி = edgep->toMTaskp();
        மாறிலி uint64_t mergedCpFwd = std::max(fromp->cpExclusive<GraphWay::FORWARD>(),
                                              உச்சி->cpExclusiveWithout<GraphWay::FORWARD>(edgep));
        மாறிலி uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<GraphWay::REVERSE>(edgep),
                                              உச்சி->cpExclusive<GraphWay::REVERSE>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        திரும்பு 1 + mergedCpRev + mergedCpFwd + fromp->cost() + உச்சி->cost();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : MergeCandidate{/* isSiblingMC: */ உண்மை}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtaskData(ap).siblings.மொத்தம்(bp), "Should be in sibling map"););
    mtaskData(m_ap).aSiblingMCs.linkBack(இது);
    mtaskData(m_bp).bSiblingMCs.linkBack(இது);
}

வெறுமை SiblingMC::unlinkA() {
    VL_ATTR_UNUSED மாறிலி size_t removed = mtaskData(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtaskData(m_ap).aSiblingMCs.unlink(இது);
}

வெறுமை SiblingMC::unlinkB() { mtaskData(m_bp).bSiblingMCs.unlink(இது); }

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
வகுப்பு MergeCandidateScoreboard final {
    // TYPES
    using Heap = PairingHeap<Merge_Candidate_சாவி>;

    // MEMBERS
    Heap m_heap;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<EdgeMC> m_edgeMCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    வெறுமை செருகு(MergeCandidate* nodep) {
        nodep->updateScore();
        m_heap.செருகு(nodep);
    }

    // Remove a candidate from the scoreboard.
    வெறுமை remove(MergeCandidate* nodep) { m_heap.remove(nodep); }

பொது:
    // CONSTRUCTORS
    MergeCandidateScoreboard() = இயல்பு;
    ~MergeCandidateScoreboard() = இயல்பு;
    VL_UNCOPYABLE(MergeCandidateScoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    MergeCandidate* best() மாறிலி { திரும்பு MergeCandidate::heap_முனை_To_Elem(m_heap.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    வெறுமை rescore(MergeCandidate* nodep) {
        remove(nodep);
        செருகு(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    வெறுமை addEdgeMC(MTaskEdge* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        EdgeMC* மாறிலி edgeMCp = m_edgeMCPool.alloc(edgep);
        edgep->userp(edgeMCp);
        செருகு(edgeMCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    வெறுமை removeEdgeMC(MTaskEdge* edgep) {
        EdgeMC* மாறிலி edgeMCp = edgeMC(edgep);
        UDEBUGONLY(UASSERT(edgeMCp, "Edge has no merge candidate"););
        edgep->userp(nullptr);
        remove(edgeMCp);
        VL_DO_DANGLING(m_edgeMCPool.விடுவி(edgeMCp), edgeMCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    வெறுமை addSiblingMC(LogicMTask* ap, LogicMTask* bp) { செருகு(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    வெறுமை removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.விடுவி(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    வெறுமை removeMC(MergeCandidate* nodep) {
        எனில் (SiblingMC* மாறிலி smcp = nodep->toSiblingMC()) {
            removeSiblingMC(smcp);
        } இல்லையெனில் {
            removeEdgeMC(nodep->toEdgeMC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
வகுப்பு Contraction final {
    // MEMBERS
    OrderMTaskGraph& m_mTaskGraph;  // The Mtask graph
    uint64_t m_scoreLimit;  // Critical path limit for merges
    MergeCandidateScoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTaskContractionData[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    வெறுமை addEdgeMCs(LogicMTask* mtaskp) {
        க்கு (V3GraphEdge& e : mtaskp->outEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
        க்கு (V3GraphEdge& e : mtaskp->inEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    வெறுமை removeEdgeMCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        க்கு (V3GraphEdge& edge : mtaskp->outEdges()) {
            MTaskEdge* மாறிலி edgep = static_cast<MTaskEdge*>(&edge);
            எனில் (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
        க்கு (V3GraphEdge& edge : mtaskp->inEdges()) {
            MTaskEdge* மாறிலி edgep = static_cast<MTaskEdge*>(&edge);
            எனில் (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
    }

    வெறுமை makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        எனில் (ap->id() < bp->id()) std::swap(ap, bp);
        // The higher id vertex owns the association set
        மாறிலி தருக்க first = mtaskData(ap).siblings.செருகு(bp).second;
        எனில் (first) {
            m_sb.addSiblingMC(ap, bp);
            திரும்பு;
        }

        எனில் (VL_UNLIKELY(m_mTaskGraph.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            தருக்க found = பொய்;
            க்கு (மாறிலி SiblingMC& smc : mtaskData(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                எனில் (smc.bp() == bp) found = உண்மை;
            }
            UASSERT_OBJ(found, ap, "Sibling not found");
        }
    }

    template <GraphWay::en N_Way, தருக்க N_Exhaustive>
    வெறுமை addSiblingMCsFromRelatives(LogicMTask* mtaskp) {
        constexpr GraphWay way{N_Way};
        // Need at least 2 edges
        auto& edges = mtaskp->edges<way>();
        எனில் (!edges.hasMultipleElements()) திரும்பு;

        std::array<LogicMTask*, PART_SIBLING_EDGE_LIMIT> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        கட்டமைப்பு alignas(16) SortingRecord final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_EDGE_LIMIT <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            தருக்க operator<(மாறிலி SortingRecord& that) மாறிலி {
                திரும்பு m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(அளவு(SortingRecord) <= 16, "How could this be padded to more than 16?");

        std::array<SortingRecord, PART_SIBLING_EDGE_LIMIT> sortRecs;
        size_t n = 0;

        // Populate the buffers
        க்கு (V3GraphEdge& edge : mtaskp->edges<way>()) {
            LogicMTask* மாறிலி otherp = static_cast<LogicMTask*>(edge.furtherp<way>());
            neighbors[n] = otherp;
            sortRecs[n].m_id = otherp->id();
            sortRecs[n].m_cp = otherp->cpInclusive<way>();
            sortRecs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            எனில் (n >= PART_SIBLING_EDGE_LIMIT) நிறுத்து;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr size_t MAX_NONEXHAUSTIVE_PAIRS = 3;

        எனில் (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_PAIRS) {
            மாறிலி size_t end = n & ~static_cast<size_t>(1);  // Round down to even, (we want pairs)
            std::sort(sortRecs.begin(), sortRecs.begin() + n);
            க்கு (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        } இல்லையெனில் {
            constexpr size_t end = 2 * MAX_NONEXHAUSTIVE_PAIRS;
            std::partial_sort(sortRecs.begin(), sortRecs.begin() + end, sortRecs.begin() + n);
            க்கு (size_t i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        }
    }

    வெறுமை removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        போது (SiblingMC* மாறிலி smcp = mtaskData(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        போது (SiblingMC* மாறிலி smcp = mtaskData(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    வெறுமை contract(MergeCandidate* mergeCanp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* உச்சி;
        எனில் (மாறிலி EdgeMC* மாறிலி edgeMCp = mergeCanp->toEdgeMC()) {
            fromp = edgeMCp->edgep()->fromMTaskp();
            உச்சி = edgeMCp->edgep()->toMTaskp();
        } இல்லையெனில் {
            மாறிலி SiblingMC* மாறிலி sibMCp = mergeCanp->toSiblingMC();
            fromp = sibMCp->bp();
            உச்சி = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        எனில் (fromp->cost() > உச்சி->cost()) {
            recipientp = fromp;
            donorp = உச்சி;
        } இல்லையெனில் {
            donorp = fromp;
            recipientp = உச்சி;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(உச்சி);  // Use donorp and recipientp now instead

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
        எனில் (VL_UNLIKELY(debug() >= 9)) m_mTaskGraph.validate();

        // Add the EdgeMCs of the merged MTask
        addEdgeMCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        addSiblingMCsFromRelatives<GraphWay::REVERSE, உண்மை>(recipientp);
        addSiblingMCsFromRelatives<GraphWay::FORWARD, உண்மை>(recipientp);
        unsigned edges = 0;
        க்கு (V3GraphEdge& edge : recipientp->outEdges()) {
            LogicMTask* மாறிலி postreqp = static_cast<LogicMTask*>(edge.உச்சி());
            addSiblingMCsFromRelatives<GraphWay::REVERSE, பொய்>(postreqp);
            ++edges;
            எனில் (edges >= PART_SIBLING_EDGE_LIMIT) நிறுத்து;
        }
        edges = 0;
        க்கு (V3GraphEdge& edge : recipientp->inEdges()) {
            LogicMTask* மாறிலி prereqp = static_cast<LogicMTask*>(edge.fromp());
            addSiblingMCsFromRelatives<GraphWay::FORWARD, பொய்>(prereqp);
            ++edges;
            எனில் (edges >= PART_SIBLING_EDGE_LIMIT) நிறுத்து;
        }
    }

    // CONSTRUCTORS
    Contraction(OrderMTaskGraph& mTaskGraph, uint64_t cpLimit)
        : m_mTaskGraph{mTaskGraph}
        , m_scoreLimit{cpLimit} {

        // Check the graph we were given is consistent.
        m_mTaskGraph.validate();

        // Figure out maximum number of MTasks
        மாறிலி uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            மாறிலி முழு given = v3Global.opt.threadsMaxMTasks();
            எனில் (given > 0) திரும்பு given;
            // Unspecified so estimate
            திரும்பு PART_DEFAULT_MAX_MTASKS_PER_THREAD * v3Global.opt.threads();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            மாறிலி size_t nMTasks = m_mTaskGraph.vertices().size();
            m_mtaskDatap.reset(புதிய MTaskContractionData[nMTasks]);
            size_t i = 0;
            க்கு (V3GraphVertex& vtx : m_mTaskGraph.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        க்கு (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
            க்கு (V3GraphEdge& edge : vtx.outEdges())
                m_sb.addEdgeMC(static_cast<MTaskEdge*>(&edge));
            LogicMTask* மாறிலி mtaskp = static_cast<LogicMTask*>(&vtx);
            addSiblingMCsFromRelatives<GraphWay::REVERSE, உண்மை>(mtaskp);
            addSiblingMCsFromRelatives<GraphWay::FORWARD, உண்மை>(mtaskp);
        }

        போது (உண்மை) {
            // Pick the candidate yielding the lowest local critical path.
            MergeCandidate* மாறிலி mergeCanp = m_sb.best();
            எனில் (!mergeCanp) நிறுத்து;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            மாறிலி uint64_t score = mergeCanp->currentScore();
            எனில் (score != mergeCanp->score()) {
                m_sb.rescore(mergeCanp);
                தொடர்;
            }

            // Check if the critical path limit is reached.
            எனில் (score > m_scoreLimit) {

                // If there are still too many MTasks, raise the limit and keep going
                மாறிலி unsigned mtask_மொத்தம் = m_mTaskGraph.vertices().size();
                எனில் (mtask_மொத்தம் > maxMTasks) {
                    m_scoreLimit = (m_scoreLimit * 120) / 100;
                    File_வரி* மாறிலி flp = v3Global.rootp()->fileline();
                    எனில் (!flp->warnIsOff(V3ErrorCode::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3ErrorCode::UNOPTTHREADS, உண்மை);
                    }
                    தொடர்;
                }

                // MTasks limit and CP limit reached. Stop.
                நிறுத்து;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            எனில் (EdgeMC* மாறிலி edgeMCp = mergeCanp->toEdgeMC()) {
                MTaskEdge* மாறிலி edgep = edgeMCp->edgep();
                எனில் (edgep->fromp() == m_mTaskGraph.entryp()
                    || edgep->உச்சி() == m_mTaskGraph.exitp()) {
                    m_sb.removeEdgeMC(edgep);
                    தொடர்;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            எனில் (mergeCanp->mergeWouldCreateCycle(m_mTaskGraph)) {
                m_sb.removeMC(mergeCanp);
                தொடர்;
            }

            // Merge this candidate
            contract(mergeCanp);
        }

        // Free all remaining merge candidates.
        போது (MergeCandidate* மாறிலி mergeCanp = m_sb.best()) m_sb.removeMC(mergeCanp);
    }

பொது:
    நிலையான வெறுமை apply(OrderMTaskGraph& mTaskGraph, uint64_t scoreLimit) {
        Contraction{mTaskGraph, scoreLimit};
    }
};

//######################################################################
// OrderMTaskGraph entry point

வெறுமை OrderMTaskGraph::contract(OrderMTaskGraph& mtaskGraph, uint64_t scoreLimit) {
    Contraction::apply(mtaskGraph, scoreLimit);
}
