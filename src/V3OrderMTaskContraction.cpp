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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Global.h"
#समावेश "V3Graph.h"
#समावेश "V3OrderMTaskGraph.h"
#समावेश "V3PairingHeap.h"
#समावेश "V3PoolAllocator.h"

#समावेश <algorithm>
#समावेश <array>
#समावेश <limits>
#समावेश <memory>
#समावेश <type_traits>
#समावेश <unordered_set>
#समावेश <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी MergeCandidate;
श्रेणी SiblingMC;
श्रेणी EdgeMC;

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
constexpr अचिन्हित PART_SIBLING_EDGE_LIMIT = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr अचिन्हित PART_DEFAULT_MAX_MTASKS_PER_THREAD = 50;

//######################################################################
// MTask utility classes

काष्ठा Merge_Candidate_कुंजी final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    bool चालक<(स्थिर Merge_Candidate_कुंजी& other) स्थिर {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        वापस m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
using Merge_Candidate_Heap_गाँठ = PairingHeap<Merge_Candidate_कुंजी>::गाँठ_2;

// Information associated with scoreboarding a merge candidate
श्रेणी MergeCandidate VL_NOT_FINAL : खुला Merge_Candidate_Heap_गाँठ {
    // Only the known subclasses can create or delete one of these
    मित्र श्रेणी SiblingMC;
    मित्र श्रेणी EdgeMC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    अटल constexpr uint64_t IS_SIBLING_MASK = 1ULL << 0;
    अटल constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    bool isSiblingMC() स्थिर { वापस m_कुंजी.m_id & IS_SIBLING_MASK; }

    // CONSTRUCTORS
    explicit MergeCandidate(bool isSiblingMC) {
        अटल uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_कुंजी.m_id = s_serial | (isSiblingMC * IS_SIBLING_MASK);
    }
    ~MergeCandidate() = शेष;

खुला:
    // METHODS
    SiblingMC* toSiblingMC();  // Instead of cast<>/as<>
    EdgeMC* toEdgeMC();  // Instead of cast<>/as<>
    bool mergeWouldCreateCycle(OrderMTaskGraph& graph);

    // The current score of this candidate, which changes as the graph is contracted
    अंतरभूत uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() स्थिर { वापस m_कुंजी.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    व्योम updateScore() { m_कुंजी.m_score = currentScore(); }

    अटल MergeCandidate* heap_गाँठ_To_Elem(Merge_Candidate_Heap_गाँठ* nodep) {
        वापस static_cast<MergeCandidate*>(nodep);
    }
};

static_assert(माप(MergeCandidate) == माप(Merge_Candidate_Heap_गाँठ),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
श्रेणी SiblingMC final : खुला MergeCandidate {
    LogicMTask* स्थिर m_ap;  // The higher ID MTask
    LogicMTask* स्थिर m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { वापस m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { वापस m_bLinks; }

खुला:
    // List type to store instances of this class
    using AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    using BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = शेष;

    // METHODS
    व्योम unlinkA();
    व्योम unlinkB();

    LogicMTask* ap() स्थिर { वापस m_ap; }
    LogicMTask* bp() स्थिर { वापस m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::मूल्य, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
श्रेणी EdgeMC final : खुला MergeCandidate {
    MTaskEdge* स्थिर m_edgep;  // The associated edge

खुला:
    // CONSTRUCTORS
    explicit EdgeMC(MTaskEdge* edgep)
        : MergeCandidate{/* isSiblingMC: */ false}
        , m_edgep{edgep} {}
    ~EdgeMC() = शेष;

    // METHODS
    MTaskEdge* edgep() स्थिर { वापस m_edgep; }
};

static_assert(!std::is_polymorphic<EdgeMC>::मूल्य, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
काष्ठा MTask_Contraction_तथ्य final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_set<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
अटल MTask_Contraction_तथ्य& mtask_तथ्य(स्थिर LogicMTask* mtaskp) {
    वापस *static_cast<MTask_Contraction_तथ्य*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
अटल EdgeMC* edgeMC(स्थिर MTaskEdge* edgep) { वापस static_cast<EdgeMC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* MergeCandidate::toSiblingMC() {
    वापस isSiblingMC() ? static_cast<SiblingMC*>(यह) : nullptr;
}

EdgeMC* MergeCandidate::toEdgeMC() { वापस isSiblingMC() ? nullptr : static_cast<EdgeMC*>(यह); }

bool MergeCandidate::mergeWouldCreateCycle(OrderMTaskGraph& graph) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    अगर (स्थिर SiblingMC* स्थिर sibp = toSiblingMC()) {
        वापस graph.pathExists(sibp->ap(), sibp->bp(), nullptr)
               || graph.pathExists(sibp->bp(), sibp->ap(), nullptr);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTaskEdge* स्थिर edgep = toEdgeMC()->edgep();
    वापस graph.pathExists(edgep->fromMTaskp(), edgep->toMTaskp(), edgep);
}

uint64_t MergeCandidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    अगर (स्थिर SiblingMC* स्थिर sibp = toSiblingMC()) {
        स्थिर LogicMTask* स्थिर ap = sibp->ap();
        स्थिर LogicMTask* स्थिर bp = sibp->bp();
        स्थिर uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<GraphWay::FORWARD>(), bp->cpExclusive<GraphWay::FORWARD>());
        स्थिर uint64_t mergedCpRev
            = std::max(ap->cpExclusive<GraphWay::REVERSE>(), bp->cpExclusive<GraphWay::REVERSE>());
        वापस mergedCpRev + mergedCpFwd + ap->cost() + bp->cost();
    }

    // Edge merge
    {
        MTaskEdge* स्थिर edgep = toEdgeMC()->edgep();
        स्थिर LogicMTask* स्थिर fromp = edgep->fromMTaskp();
        स्थिर LogicMTask* स्थिर शीर्ष = edgep->toMTaskp();
        स्थिर uint64_t mergedCpFwd = std::max(fromp->cpExclusive<GraphWay::FORWARD>(),
                                              शीर्ष->cpExclusiveWithout<GraphWay::FORWARD>(edgep));
        स्थिर uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<GraphWay::REVERSE>(edgep),
                                              शीर्ष->cpExclusive<GraphWay::REVERSE>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        वापस 1 + mergedCpRev + mergedCpFwd + fromp->cost() + शीर्ष->cost();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : MergeCandidate{/* isSiblingMC: */ true}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtask_तथ्य(ap).siblings.गिनती(bp), "Should be in sibling map"););
    mtask_तथ्य(m_ap).aSiblingMCs.linkBack(यह);
    mtask_तथ्य(m_bp).bSiblingMCs.linkBack(यह);
}

व्योम SiblingMC::unlinkA() {
    VL_ATTR_UNUSED स्थिर माप_प्रकार removed = mtask_तथ्य(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtask_तथ्य(m_ap).aSiblingMCs.unlink(यह);
}

व्योम SiblingMC::unlinkB() { mtask_तथ्य(m_bp).bSiblingMCs.unlink(यह); }

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
श्रेणी MergeCandidateScoreboard final {
    // TYPES
    using Heap = PairingHeap<Merge_Candidate_कुंजी>;

    // MEMBERS
    Heap m_heap;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<EdgeMC> m_edgeMCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    व्योम जोड़ो(MergeCandidate* nodep) {
        nodep->updateScore();
        m_heap.जोड़ो(nodep);
    }

    // Remove a candidate from the scoreboard.
    व्योम हटाओ(MergeCandidate* nodep) { m_heap.हटाओ(nodep); }

खुला:
    // CONSTRUCTORS
    MergeCandidateScoreboard() = शेष;
    ~MergeCandidateScoreboard() = शेष;
    VL_UNCOPYABLE(MergeCandidateScoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    MergeCandidate* best() स्थिर { वापस MergeCandidate::heap_गाँठ_To_Elem(m_heap.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    व्योम rescore(MergeCandidate* nodep) {
        हटाओ(nodep);
        जोड़ो(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    व्योम addEdgeMC(MTaskEdge* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        EdgeMC* स्थिर edgeMCp = m_edgeMCPool.alloc(edgep);
        edgep->userp(edgeMCp);
        जोड़ो(edgeMCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    व्योम removeEdgeMC(MTaskEdge* edgep) {
        EdgeMC* स्थिर edgeMCp = edgeMC(edgep);
        UDEBUGONLY(UASSERT(edgeMCp, "Edge has no merge candidate"););
        edgep->userp(nullptr);
        हटाओ(edgeMCp);
        VL_DO_DANGLING(m_edgeMCPool.मुक्त(edgeMCp), edgeMCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    व्योम addSiblingMC(LogicMTask* ap, LogicMTask* bp) { जोड़ो(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    व्योम removeSiblingMC(SiblingMC* smcp) {
        हटाओ(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.मुक्त(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    व्योम removeMC(MergeCandidate* nodep) {
        अगर (SiblingMC* स्थिर smcp = nodep->toSiblingMC()) {
            removeSiblingMC(smcp);
        } अन्यथा {
            removeEdgeMC(nodep->toEdgeMC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
श्रेणी Contraction final {
    // MEMBERS
    OrderMTaskGraph& m_mTaskGraph;  // The Mtask graph
    uint64_t m_scoreLimit;  // Critical path limit for merges
    MergeCandidateScoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTask_Contraction_तथ्य[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    व्योम addEdgeMCs(LogicMTask* mtaskp) {
        क्रम (V3GraphEdge& e : mtaskp->outEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
        क्रम (V3GraphEdge& e : mtaskp->inEdges()) m_sb.addEdgeMC(static_cast<MTaskEdge*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    व्योम removeEdgeMCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        क्रम (V3GraphEdge& edge : mtaskp->outEdges()) {
            MTaskEdge* स्थिर edgep = static_cast<MTaskEdge*>(&edge);
            अगर (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
        क्रम (V3GraphEdge& edge : mtaskp->inEdges()) {
            MTaskEdge* स्थिर edgep = static_cast<MTaskEdge*>(&edge);
            अगर (edgeMC(edgep)) m_sb.removeEdgeMC(edgep);
        }
    }

    व्योम makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        अगर (ap->id() < bp->id()) std::swap(ap, bp);
        // The higher id vertex owns the association set
        स्थिर bool first = mtask_तथ्य(ap).siblings.जोड़ो(bp).second;
        अगर (first) {
            m_sb.addSiblingMC(ap, bp);
            वापस;
        }

        अगर (VL_UNLIKELY(m_mTaskGraph.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            bool found = false;
            क्रम (स्थिर SiblingMC& smc : mtask_तथ्य(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                अगर (smc.bp() == bp) found = true;
            }
            UASSERT_OBJ(found, ap, "Sibling not found");
        }
    }

    ढाँचा <GraphWay::en N_Way, bool N_Exhaustive>
    व्योम addSiblingMCsFromRelatives(LogicMTask* mtaskp) {
        constexpr GraphWay way{N_Way};
        // Need at least 2 edges
        स्वतः& edges = mtaskp->edges<way>();
        अगर (!edges.hasMultipleElements()) वापस;

        std::array<LogicMTask*, PART_SIBLING_EDGE_LIMIT> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        काष्ठा alignas(16) SortingRecord final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_EDGE_LIMIT <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            bool चालक<(स्थिर SortingRecord& that) स्थिर {
                वापस m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(माप(SortingRecord) <= 16, "How could this be padded to more than 16?");

        std::array<SortingRecord, PART_SIBLING_EDGE_LIMIT> sortRecs;
        माप_प्रकार n = 0;

        // Populate the buffers
        क्रम (V3GraphEdge& edge : mtaskp->edges<way>()) {
            LogicMTask* स्थिर otherp = static_cast<LogicMTask*>(edge.furtherp<way>());
            neighbors[n] = otherp;
            sortRecs[n].m_id = otherp->id();
            sortRecs[n].m_cp = otherp->cpInclusive<way>();
            sortRecs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            अगर (n >= PART_SIBLING_EDGE_LIMIT) अवरोध;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr माप_प्रकार MAX_NONEXHAUSTIVE_PAIRS = 3;

        अगर (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_PAIRS) {
            स्थिर माप_प्रकार end = n & ~static_cast<माप_प्रकार>(1);  // Round down to even, (we want pairs)
            std::sort(sortRecs.begin(), sortRecs.begin() + n);
            क्रम (माप_प्रकार i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        } अन्यथा {
            constexpr माप_प्रकार end = 2 * MAX_NONEXHAUSTIVE_PAIRS;
            std::partial_sort(sortRecs.begin(), sortRecs.begin() + end, sortRecs.begin() + n);
            क्रम (माप_प्रकार i = 0; i < end; i += 2) {
                makeSiblingMC(neighbors[sortRecs[i].m_idx], neighbors[sortRecs[i + 1].m_idx]);
            }
        }
    }

    व्योम removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        जबतक (SiblingMC* स्थिर smcp = mtask_तथ्य(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        जबतक (SiblingMC* स्थिर smcp = mtask_तथ्य(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    व्योम contract(MergeCandidate* mergeCanp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* शीर्ष;
        अगर (स्थिर EdgeMC* स्थिर edgeMCp = mergeCanp->toEdgeMC()) {
            fromp = edgeMCp->edgep()->fromMTaskp();
            शीर्ष = edgeMCp->edgep()->toMTaskp();
        } अन्यथा {
            स्थिर SiblingMC* स्थिर sibMCp = mergeCanp->toSiblingMC();
            fromp = sibMCp->bp();
            शीर्ष = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        अगर (fromp->cost() > शीर्ष->cost()) {
            recipientp = fromp;
            donorp = शीर्ष;
        } अन्यथा {
            donorp = fromp;
            recipientp = शीर्ष;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(शीर्ष);  // Use donorp and recipientp now instead

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
        अगर (VL_UNLIKELY(debug() >= 9)) m_mTaskGraph.validate();

        // Add the EdgeMCs of the merged MTask
        addEdgeMCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        addSiblingMCsFromRelatives<GraphWay::REVERSE, true>(recipientp);
        addSiblingMCsFromRelatives<GraphWay::FORWARD, true>(recipientp);
        अचिन्हित edges = 0;
        क्रम (V3GraphEdge& edge : recipientp->outEdges()) {
            LogicMTask* स्थिर postreqp = static_cast<LogicMTask*>(edge.शीर्ष());
            addSiblingMCsFromRelatives<GraphWay::REVERSE, false>(postreqp);
            ++edges;
            अगर (edges >= PART_SIBLING_EDGE_LIMIT) अवरोध;
        }
        edges = 0;
        क्रम (V3GraphEdge& edge : recipientp->inEdges()) {
            LogicMTask* स्थिर prereqp = static_cast<LogicMTask*>(edge.fromp());
            addSiblingMCsFromRelatives<GraphWay::FORWARD, false>(prereqp);
            ++edges;
            अगर (edges >= PART_SIBLING_EDGE_LIMIT) अवरोध;
        }
    }

    // CONSTRUCTORS
    Contraction(OrderMTaskGraph& mTaskGraph, uint64_t cpLimit)
        : m_mTaskGraph{mTaskGraph}
        , m_scoreLimit{cpLimit} {

        // Check the graph we were given is consistent.
        m_mTaskGraph.validate();

        // Figure out maximum number of MTasks
        स्थिर uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            स्थिर पूर्णांक given = v3Global.opt.threadsMaxMTasks();
            अगर (given > 0) वापस given;
            // Unspecified so estimate
            वापस PART_DEFAULT_MAX_MTASKS_PER_THREAD * v3Global.opt.threads();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            स्थिर माप_प्रकार nMTasks = m_mTaskGraph.vertices().आकार();
            m_mtaskDatap.reset(नया MTask_Contraction_तथ्य[nMTasks]);
            माप_प्रकार i = 0;
            क्रम (V3GraphVertex& vtx : m_mTaskGraph.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        क्रम (V3GraphVertex& vtx : m_mTaskGraph.vertices()) {
            क्रम (V3GraphEdge& edge : vtx.outEdges())
                m_sb.addEdgeMC(static_cast<MTaskEdge*>(&edge));
            LogicMTask* स्थिर mtaskp = static_cast<LogicMTask*>(&vtx);
            addSiblingMCsFromRelatives<GraphWay::REVERSE, true>(mtaskp);
            addSiblingMCsFromRelatives<GraphWay::FORWARD, true>(mtaskp);
        }

        जबतक (true) {
            // Pick the candidate yielding the lowest local critical path.
            MergeCandidate* स्थिर mergeCanp = m_sb.best();
            अगर (!mergeCanp) अवरोध;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            स्थिर uint64_t score = mergeCanp->currentScore();
            अगर (score != mergeCanp->score()) {
                m_sb.rescore(mergeCanp);
                जारी;
            }

            // Check if the critical path limit is reached.
            अगर (score > m_scoreLimit) {

                // If there are still too many MTasks, raise the limit and keep going
                स्थिर अचिन्हित mtask_गिनती = m_mTaskGraph.vertices().आकार();
                अगर (mtask_गिनती > maxMTasks) {
                    m_scoreLimit = (m_scoreLimit * 120) / 100;
                    File_पंक्ति* स्थिर flp = v3Global.rootp()->fileline();
                    अगर (!flp->warnIsOff(V3ErrorCode::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3ErrorCode::UNOPTTHREADS, true);
                    }
                    जारी;
                }

                // MTasks limit and CP limit reached. Stop.
                अवरोध;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            अगर (EdgeMC* स्थिर edgeMCp = mergeCanp->toEdgeMC()) {
                MTaskEdge* स्थिर edgep = edgeMCp->edgep();
                अगर (edgep->fromp() == m_mTaskGraph.entryp()
                    || edgep->शीर्ष() == m_mTaskGraph.exitp()) {
                    m_sb.removeEdgeMC(edgep);
                    जारी;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            अगर (mergeCanp->mergeWouldCreateCycle(m_mTaskGraph)) {
                m_sb.removeMC(mergeCanp);
                जारी;
            }

            // Merge this candidate
            contract(mergeCanp);
        }

        // Free all remaining merge candidates.
        जबतक (MergeCandidate* स्थिर mergeCanp = m_sb.best()) m_sb.removeMC(mergeCanp);
    }

खुला:
    अटल व्योम apply(OrderMTaskGraph& mTaskGraph, uint64_t scoreLimit) {
        Contraction{mTaskGraph, scoreLimit};
    }
};

//######################################################################
// OrderMTaskGraph entry point

व्योम OrderMTaskGraph::contract(OrderMTaskGraph& mtaskGraph, uint64_t scoreLimit) {
    Contraction::apply(mtaskGraph, scoreLimit);
}
