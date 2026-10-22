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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderMTaskGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PairingHeap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PoolAllocator.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <array>
#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ ਮਿਲਾਓ_Candidate;
ਵਰਗ SiblingMC;
ਵਰਗ ਕਿਨਾਰਾ_MC;

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
constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ PART_SIBLING_ਕਿਨਾਰਾ_ਹੱਦਬੰਦੀ = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ PART_DEFAULT_MAX_MTASKS_ਪ੍ਰਤੀ_THREAD = 50;

//######################################################################
// MTask utility classes

ਬਣਤਰ ਮਿਲਾਓ_Candidate_ਕੁੰਜੀ final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    ਤਾਰਕਿਕ ਸੰਚਾਲਕ<(ਅਚਲ ਮਿਲਾਓ_Candidate_ਕੁੰਜੀ& other) ਅਚਲ {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        ਵਾਪਸ m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
ਵਰਤੋਂ ਮਿਲਾਓ_Candidate_ਢੇਰ_ਗੰਢ = Pairing_ਢੇਰ<ਮਿਲਾਓ_Candidate_ਕੁੰਜੀ>::ਗੰਢ_2;

// Information associated with scoreboarding a merge candidate
ਵਰਗ ਮਿਲਾਓ_Candidate VL_NOT_FINAL : ਜਨਤਕ ਮਿਲਾਓ_Candidate_ਢੇਰ_ਗੰਢ {
    // Only the known subclasses can create or delete one of these
    ਮਿੱਤਰ ਵਰਗ SiblingMC;
    ਮਿੱਤਰ ਵਰਗ ਕਿਨਾਰਾ_MC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ਸਥਿਰ constexpr uint64_t IS_SIBLING_ਨਕਾਬ = 1ULL << 0;
    ਸਥਿਰ constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    ਤਾਰਕਿਕ isSiblingMC() ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀ.m_id & IS_SIBLING_ਨਕਾਬ; }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਮਿਲਾਓ_Candidate(ਤਾਰਕਿਕ isSiblingMC) {
        ਸਥਿਰ uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_ਕੁੰਜੀ.m_id = s_serial | (isSiblingMC * IS_SIBLING_ਨਕਾਬ);
    }
    ~ਮਿਲਾਓ_Candidate() = ਮੂਲ;

ਜਨਤਕ:
    // METHODS
    SiblingMC* ਮੰਜ਼ਿਲ_Sibling_MC();  // Instead of cast<>/as<>
    ਕਿਨਾਰਾ_MC* ਮੰਜ਼ਿਲ_ਕਿਨਾਰਾ_MC();  // Instead of cast<>/as<>
    ਤਾਰਕਿਕ ਮਿਲਾਓ_Would_Create_ਚੱਕਰ(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ);

    // The current score of this candidate, which changes as the graph is contracted
    ਸਤਰਅੰਦਰ uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀ.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    ਖਾਲੀ updateScore() { m_ਕੁੰਜੀ.m_score = currentScore(); }

    ਸਥਿਰ ਮਿਲਾਓ_Candidate* ਢੇਰ_ਗੰਢ_ਮੰਜ਼ਿਲ_Elem(ਮਿਲਾਓ_Candidate_ਢੇਰ_ਗੰਢ* nodep) {
        ਵਾਪਸ static_cast<ਮਿਲਾਓ_Candidate*>(nodep);
    }
};

static_assert(ਆਕਾਰ(ਮਿਲਾਓ_Candidate) == ਆਕਾਰ(ਮਿਲਾਓ_Candidate_ਢੇਰ_ਗੰਢ),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
ਵਰਗ SiblingMC final : ਜਨਤਕ ਮਿਲਾਓ_Candidate {
    LogicMTask* ਅਚਲ m_ap;  // The higher ID MTask
    LogicMTask* ਅਚਲ m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { ਵਾਪਸ m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { ਵਾਪਸ m_bLinks; }

ਜਨਤਕ:
    // List type to store instances of this class
    ਵਰਤੋਂ AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    ਵਰਤੋਂ BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = ਮੂਲ;

    // METHODS
    ਖਾਲੀ unlinkA();
    ਖਾਲੀ unlinkB();

    LogicMTask* ap() ਅਚਲ { ਵਾਪਸ m_ap; }
    LogicMTask* bp() ਅਚਲ { ਵਾਪਸ m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::ਮੁੱਲ, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
ਵਰਗ ਕਿਨਾਰਾ_MC final : ਜਨਤਕ ਮਿਲਾਓ_Candidate {
    MTask_ਕਿਨਾਰਾ* ਅਚਲ m_edgep;  // The associated edge

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਕਿਨਾਰਾ_MC(MTask_ਕਿਨਾਰਾ* edgep)
        : ਮਿਲਾਓ_Candidate{/* isSiblingMC: */ ਝੂਠ}
        , m_edgep{edgep} {}
    ~ਕਿਨਾਰਾ_MC() = ਮੂਲ;

    // METHODS
    MTask_ਕਿਨਾਰਾ* edgep() ਅਚਲ { ਵਾਪਸ m_edgep; }
};

static_assert(!std::is_polymorphic<ਕਿਨਾਰਾ_MC>::ਮੁੱਲ, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
ਬਣਤਰ MTask_Contraction_ਅੰਕੜੇ final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_ਸੈੱਟ<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
ਸਥਿਰ MTask_Contraction_ਅੰਕੜੇ& mtask_ਅੰਕੜੇ(ਅਚਲ LogicMTask* mtaskp) {
    ਵਾਪਸ *static_cast<MTask_Contraction_ਅੰਕੜੇ*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
ਸਥਿਰ ਕਿਨਾਰਾ_MC* ਕਿਨਾਰਾ_MC_2(ਅਚਲ MTask_ਕਿਨਾਰਾ* edgep) { ਵਾਪਸ static_cast<ਕਿਨਾਰਾ_MC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* ਮਿਲਾਓ_Candidate::ਮੰਜ਼ਿਲ_Sibling_MC() {
    ਵਾਪਸ isSiblingMC() ? static_cast<SiblingMC*>(ਇਹ) : ਖਾਲੀ_ਸੂਚਕ;
}

ਕਿਨਾਰਾ_MC* ਮਿਲਾਓ_Candidate::ਮੰਜ਼ਿਲ_ਕਿਨਾਰਾ_MC() { ਵਾਪਸ isSiblingMC() ? ਖਾਲੀ_ਸੂਚਕ : static_cast<ਕਿਨਾਰਾ_MC*>(ਇਹ); }

ਤਾਰਕਿਕ ਮਿਲਾਓ_Candidate::ਮਿਲਾਓ_Would_Create_ਚੱਕਰ(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    ਜੇ (ਅਚਲ SiblingMC* ਅਚਲ sibp = ਮੰਜ਼ਿਲ_Sibling_MC()) {
        ਵਾਪਸ ਗ੍ਰਾਫ.ਰਸਤਾ_Exists(sibp->ap(), sibp->bp(), ਖਾਲੀ_ਸੂਚਕ)
               || ਗ੍ਰਾਫ.ਰਸਤਾ_Exists(sibp->bp(), sibp->ap(), ਖਾਲੀ_ਸੂਚਕ);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = ਮੰਜ਼ਿਲ_ਕਿਨਾਰਾ_MC()->edgep();
    ਵਾਪਸ ਗ੍ਰਾਫ.ਰਸਤਾ_Exists(edgep->fromMTaskp(), edgep->ਮੰਜ਼ਿਲ_MTaskp(), edgep);
}

uint64_t ਮਿਲਾਓ_Candidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    ਜੇ (ਅਚਲ SiblingMC* ਅਚਲ sibp = ਮੰਜ਼ਿਲ_Sibling_MC()) {
        ਅਚਲ LogicMTask* ਅਚਲ ap = sibp->ap();
        ਅਚਲ LogicMTask* ਅਚਲ bp = sibp->bp();
        ਅਚਲ uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<ਗ੍ਰਾਫ_Way::FORWARD>(), bp->cpExclusive<ਗ੍ਰਾਫ_Way::FORWARD>());
        ਅਚਲ uint64_t mergedCpRev
            = std::max(ap->cpExclusive<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(), bp->cpExclusive<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>());
        ਵਾਪਸ mergedCpRev + mergedCpFwd + ap->ਲਾਗਤ() + bp->ਲਾਗਤ();
    }

    // Edge merge
    {
        MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = ਮੰਜ਼ਿਲ_ਕਿਨਾਰਾ_MC()->edgep();
        ਅਚਲ LogicMTask* ਅਚਲ fromp = edgep->fromMTaskp();
        ਅਚਲ LogicMTask* ਅਚਲ ਸਿਖਰ = edgep->ਮੰਜ਼ਿਲ_MTaskp();
        ਅਚਲ uint64_t mergedCpFwd = std::max(fromp->cpExclusive<ਗ੍ਰਾਫ_Way::FORWARD>(),
                                              ਸਿਖਰ->cpExclusiveWithout<ਗ੍ਰਾਫ_Way::FORWARD>(edgep));
        ਅਚਲ uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(edgep),
                                              ਸਿਖਰ->cpExclusive<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        ਵਾਪਸ 1 + mergedCpRev + mergedCpFwd + fromp->ਲਾਗਤ() + ਸਿਖਰ->ਲਾਗਤ();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : ਮਿਲਾਓ_Candidate{/* isSiblingMC: */ ਸੱਚ}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtask_ਅੰਕੜੇ(ap).siblings.ਗਿਣਤੀ_2(bp), "Should be in sibling map"););
    mtask_ਅੰਕੜੇ(m_ap).aSiblingMCs.link_ਪਿੱਠ(ਇਹ);
    mtask_ਅੰਕੜੇ(m_bp).bSiblingMCs.link_ਪਿੱਠ(ਇਹ);
}

ਖਾਲੀ SiblingMC::unlinkA() {
    VL_ATTR_ਅਣਵਰਤਿਆ ਅਚਲ ਮਾਪ_t removed = mtask_ਅੰਕੜੇ(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtask_ਅੰਕੜੇ(m_ap).aSiblingMCs.unlink(ਇਹ);
}

ਖਾਲੀ SiblingMC::unlinkB() { mtask_ਅੰਕੜੇ(m_bp).bSiblingMCs.unlink(ਇਹ); }

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
ਵਰਗ ਮਿਲਾਓ_Candidate_Scoreboard final {
    // TYPES
    ਵਰਤੋਂ ਢੇਰ_2 = Pairing_ਢੇਰ<ਮਿਲਾਓ_Candidate_ਕੁੰਜੀ>;

    // MEMBERS
    ਢੇਰ_2 m_ਢੇਰ_2;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<ਕਿਨਾਰਾ_MC> m_ਕਿਨਾਰਾ_MCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    ਖਾਲੀ ਪਾਓ(ਮਿਲਾਓ_Candidate* nodep) {
        nodep->updateScore();
        m_ਢੇਰ_2.ਪਾਓ(nodep);
    }

    // Remove a candidate from the scoreboard.
    ਖਾਲੀ remove(ਮਿਲਾਓ_Candidate* nodep) { m_ਢੇਰ_2.remove(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਮਿਲਾਓ_Candidate_Scoreboard() = ਮੂਲ;
    ~ਮਿਲਾਓ_Candidate_Scoreboard() = ਮੂਲ;
    VL_UNCOPYABLE(ਮਿਲਾਓ_Candidate_Scoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    ਮਿਲਾਓ_Candidate* ਉੱਤਮ() ਅਚਲ { ਵਾਪਸ ਮਿਲਾਓ_Candidate::ਢੇਰ_ਗੰਢ_ਮੰਜ਼ਿਲ_Elem(m_ਢੇਰ_2.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    ਖਾਲੀ rescore(ਮਿਲਾਓ_Candidate* nodep) {
        remove(nodep);
        ਪਾਓ(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ_MC(MTask_ਕਿਨਾਰਾ* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        ਕਿਨਾਰਾ_MC* ਅਚਲ ਕਿਨਾਰਾ_MCp = m_ਕਿਨਾਰਾ_MCPool.alloc(edgep);
        edgep->userp(ਕਿਨਾਰਾ_MCp);
        ਪਾਓ(ਕਿਨਾਰਾ_MCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    ਖਾਲੀ remove_ਕਿਨਾਰਾ_MC(MTask_ਕਿਨਾਰਾ* edgep) {
        ਕਿਨਾਰਾ_MC* ਅਚਲ ਕਿਨਾਰਾ_MCp = ਕਿਨਾਰਾ_MC_2(edgep);
        UDEBUGONLY(UASSERT(ਕਿਨਾਰਾ_MCp, "Edge has no merge candidate"););
        edgep->userp(ਖਾਲੀ_ਸੂਚਕ);
        remove(ਕਿਨਾਰਾ_MCp);
        VL_DO_DANGLING(m_ਕਿਨਾਰਾ_MCPool.ਮੁਕਤ(ਕਿਨਾਰਾ_MCp), ਕਿਨਾਰਾ_MCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    ਖਾਲੀ ਜੋੜੋ_Sibling_MC(LogicMTask* ap, LogicMTask* bp) { ਪਾਓ(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    ਖਾਲੀ removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.ਮੁਕਤ(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    ਖਾਲੀ removeMC(ਮਿਲਾਓ_Candidate* nodep) {
        ਜੇ (SiblingMC* ਅਚਲ smcp = nodep->ਮੰਜ਼ਿਲ_Sibling_MC()) {
            removeSiblingMC(smcp);
        } ਨਹੀਂਤਾਂ {
            remove_ਕਿਨਾਰਾ_MC(nodep->ਮੰਜ਼ਿਲ_ਕਿਨਾਰਾ_MC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
ਵਰਗ Contraction final {
    // MEMBERS
    ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& m_m_Task_ਗ੍ਰਾਫ;  // The Mtask graph
    uint64_t m_score_ਹੱਦਬੰਦੀ;  // Critical path limit for merges
    ਮਿਲਾਓ_Candidate_Scoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTask_Contraction_ਅੰਕੜੇ[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ_MCs(LogicMTask* mtaskp) {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& e : mtaskp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) m_sb.ਜੋੜੋ_ਕਿਨਾਰਾ_MC(static_cast<MTask_ਕਿਨਾਰਾ*>(&e));
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& e : mtaskp->ਵਿੱਚ_ਕਿਨਾਰੇ()) m_sb.ਜੋੜੋ_ਕਿਨਾਰਾ_MC(static_cast<MTask_ਕਿਨਾਰਾ*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    ਖਾਲੀ remove_ਕਿਨਾਰਾ_MCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtaskp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = static_cast<MTask_ਕਿਨਾਰਾ*>(&ਕਿਨਾਰਾ);
            ਜੇ (ਕਿਨਾਰਾ_MC_2(edgep)) m_sb.remove_ਕਿਨਾਰਾ_MC(edgep);
        }
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtaskp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = static_cast<MTask_ਕਿਨਾਰਾ*>(&ਕਿਨਾਰਾ);
            ਜੇ (ਕਿਨਾਰਾ_MC_2(edgep)) m_sb.remove_ਕਿਨਾਰਾ_MC(edgep);
        }
    }

    ਖਾਲੀ makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        ਜੇ (ap->id() < bp->id()) std::ਅਦਲਾ_ਬਦਲੀ(ap, bp);
        // The higher id vertex owns the association set
        ਅਚਲ ਤਾਰਕਿਕ ਪਹਿਲਾ = mtask_ਅੰਕੜੇ(ap).siblings.ਪਾਓ(bp).ਦੂਜਾ;
        ਜੇ (ਪਹਿਲਾ) {
            m_sb.ਜੋੜੋ_Sibling_MC(ap, bp);
            ਵਾਪਸ;
        }

        ਜੇ (VL_UNLIKELY(m_m_Task_ਗ੍ਰਾਫ.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            ਤਾਰਕਿਕ ਮਿਲਿਆ = ਝੂਠ;
            ਲਈ (ਅਚਲ SiblingMC& smc : mtask_ਅੰਕੜੇ(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                ਜੇ (smc.bp() == bp) ਮਿਲਿਆ = ਸੱਚ;
            }
            UASSERT_OBJ(ਮਿਲਿਆ, ap, "Sibling not found");
        }
    }

    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way, ਤਾਰਕਿਕ N_Exhaustive>
    ਖਾਲੀ ਜੋੜੋ_Sibling_MCs_From_Relatives(LogicMTask* mtaskp) {
        constexpr ਗ੍ਰਾਫ_Way way{N_Way};
        // Need at least 2 edges
        ਸਵੈਚਾਲਿਤ& ਕਿਨਾਰੇ = mtaskp->ਕਿਨਾਰੇ<way>();
        ਜੇ (!ਕਿਨਾਰੇ.has_ਗੁਣਜ_Elements()) ਵਾਪਸ;

        std::array<LogicMTask*, PART_SIBLING_ਕਿਨਾਰਾ_ਹੱਦਬੰਦੀ> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        ਬਣਤਰ alignas(16) ਛਾਂਟੀ_ਰਿਕਾਰਡ final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_ਕਿਨਾਰਾ_ਹੱਦਬੰਦੀ <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            ਤਾਰਕਿਕ ਸੰਚਾਲਕ<(ਅਚਲ ਛਾਂਟੀ_ਰਿਕਾਰਡ& that) ਅਚਲ {
                ਵਾਪਸ m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(ਆਕਾਰ(ਛਾਂਟੀ_ਰਿਕਾਰਡ) <= 16, "How could this be padded to more than 16?");

        std::array<ਛਾਂਟੀ_ਰਿਕਾਰਡ, PART_SIBLING_ਕਿਨਾਰਾ_ਹੱਦਬੰਦੀ> ਛਾਂਟੋ_Recs;
        ਮਾਪ_t n = 0;

        // Populate the buffers
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mtaskp->ਕਿਨਾਰੇ<way>()) {
            LogicMTask* ਅਚਲ otherp = static_cast<LogicMTask*>(ਕਿਨਾਰਾ.furtherp<way>());
            neighbors[n] = otherp;
            ਛਾਂਟੋ_Recs[n].m_id = otherp->id();
            ਛਾਂਟੋ_Recs[n].m_cp = otherp->cpInclusive<way>();
            ਛਾਂਟੋ_Recs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            ਜੇ (n >= PART_SIBLING_ਕਿਨਾਰਾ_ਹੱਦਬੰਦੀ) ਰੋਕੋ;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr ਮਾਪ_t MAX_NONEXHAUSTIVE_ਜੋੜੇ = 3;

        ਜੇ (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_ਜੋੜੇ) {
            ਅਚਲ ਮਾਪ_t ਸਿਰਾ = n & ~static_cast<ਮਾਪ_t>(1);  // Round down to even, (we want pairs)
            std::ਛਾਂਟੋ(ਛਾਂਟੋ_Recs.begin(), ਛਾਂਟੋ_Recs.begin() + n);
            ਲਈ (ਮਾਪ_t i = 0; i < ਸਿਰਾ; i += 2) {
                makeSiblingMC(neighbors[ਛਾਂਟੋ_Recs[i].m_idx], neighbors[ਛਾਂਟੋ_Recs[i + 1].m_idx]);
            }
        } ਨਹੀਂਤਾਂ {
            constexpr ਮਾਪ_t ਸਿਰਾ = 2 * MAX_NONEXHAUSTIVE_ਜੋੜੇ;
            std::partial_ਛਾਂਟੋ(ਛਾਂਟੋ_Recs.begin(), ਛਾਂਟੋ_Recs.begin() + ਸਿਰਾ, ਛਾਂਟੋ_Recs.begin() + n);
            ਲਈ (ਮਾਪ_t i = 0; i < ਸਿਰਾ; i += 2) {
                makeSiblingMC(neighbors[ਛਾਂਟੋ_Recs[i].m_idx], neighbors[ਛਾਂਟੋ_Recs[i + 1].m_idx]);
            }
        }
    }

    ਖਾਲੀ removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        ਜਦੋਂ (SiblingMC* ਅਚਲ smcp = mtask_ਅੰਕੜੇ(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        ਜਦੋਂ (SiblingMC* ਅਚਲ smcp = mtask_ਅੰਕੜੇ(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    ਖਾਲੀ contract(ਮਿਲਾਓ_Candidate* ਮਿਲਾਓ_Canp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* ਸਿਖਰ;
        ਜੇ (ਅਚਲ ਕਿਨਾਰਾ_MC* ਅਚਲ ਕਿਨਾਰਾ_MCp = ਮਿਲਾਓ_Canp->ਮੰਜ਼ਿਲ_ਕਿਨਾਰਾ_MC()) {
            fromp = ਕਿਨਾਰਾ_MCp->edgep()->fromMTaskp();
            ਸਿਖਰ = ਕਿਨਾਰਾ_MCp->edgep()->ਮੰਜ਼ਿਲ_MTaskp();
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ SiblingMC* ਅਚਲ sibMCp = ਮਿਲਾਓ_Canp->ਮੰਜ਼ਿਲ_Sibling_MC();
            fromp = sibMCp->bp();
            ਸਿਖਰ = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        ਜੇ (fromp->ਲਾਗਤ() > ਸਿਖਰ->ਲਾਗਤ()) {
            recipientp = fromp;
            donorp = ਸਿਖਰ;
        } ਨਹੀਂਤਾਂ {
            donorp = fromp;
            recipientp = ਸਿਖਰ;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(ਸਿਖਰ);  // Use donorp and recipientp now instead

        // Remove all SiblingMCs that include either MTask
        removeSiblingMCs(recipientp);
        removeSiblingMCs(donorp);

        // Remove the EdgeMCs of both MTasks
        remove_ਕਿਨਾਰਾ_MCs(recipientp);
        remove_ਕਿਨਾਰਾ_MCs(donorp);

        // Merge the MTasks. This redirects all edges, updates critical paths, and deletes donorp
        m_m_Task_ਗ੍ਰਾਫ.ਮਿਲਾਓ_MTasks(recipientp, donorp);
        VL_DANGLING(donorp);

        // Confirm we haven't botched the CP updates. This is a whole graph walk after every single
        // merge, so it is quadratic in the size of the graph, hence only under '--debug 9'.
        ਜੇ (VL_UNLIKELY(debug() >= 9)) m_m_Task_ਗ੍ਰਾਫ.validate();

        // Add the EdgeMCs of the merged MTask
        ਜੋੜੋ_ਕਿਨਾਰਾ_MCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        ਜੋੜੋ_Sibling_MCs_From_Relatives<ਗ੍ਰਾਫ_Way::ਉਲਟ_2, ਸੱਚ>(recipientp);
        ਜੋੜੋ_Sibling_MCs_From_Relatives<ਗ੍ਰਾਫ_Way::FORWARD, ਸੱਚ>(recipientp);
        ਬਿਨਾਨਿਸ਼ਾਨੀ ਕਿਨਾਰੇ = 0;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : recipientp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            LogicMTask* ਅਚਲ postreqp = static_cast<LogicMTask*>(ਕਿਨਾਰਾ.ਸਿਖਰ());
            ਜੋੜੋ_Sibling_MCs_From_Relatives<ਗ੍ਰਾਫ_Way::ਉਲਟ_2, ਝੂਠ>(postreqp);
            ++ਕਿਨਾਰੇ;
            ਜੇ (ਕਿਨਾਰੇ >= PART_SIBLING_ਕਿਨਾਰਾ_ਹੱਦਬੰਦੀ) ਰੋਕੋ;
        }
        ਕਿਨਾਰੇ = 0;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : recipientp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            LogicMTask* ਅਚਲ prereqp = static_cast<LogicMTask*>(ਕਿਨਾਰਾ.fromp());
            ਜੋੜੋ_Sibling_MCs_From_Relatives<ਗ੍ਰਾਫ_Way::FORWARD, ਝੂਠ>(prereqp);
            ++ਕਿਨਾਰੇ;
            ਜੇ (ਕਿਨਾਰੇ >= PART_SIBLING_ਕਿਨਾਰਾ_ਹੱਦਬੰਦੀ) ਰੋਕੋ;
        }
    }

    // CONSTRUCTORS
    Contraction(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& m_Task_ਗ੍ਰਾਫ, uint64_t cp_ਹੱਦਬੰਦੀ)
        : m_m_Task_ਗ੍ਰਾਫ{m_Task_ਗ੍ਰਾਫ}
        , m_score_ਹੱਦਬੰਦੀ{cp_ਹੱਦਬੰਦੀ} {

        // Check the graph we were given is consistent.
        m_m_Task_ਗ੍ਰਾਫ.validate();

        // Figure out maximum number of MTasks
        ਅਚਲ uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ਅਚਲ ਪੂਰਨ given = v3Global.opt.ਧਾਗੇ_Max_MTasks();
            ਜੇ (given > 0) ਵਾਪਸ given;
            // Unspecified so estimate
            ਵਾਪਸ PART_DEFAULT_MAX_MTASKS_ਪ੍ਰਤੀ_THREAD * v3Global.opt.ਧਾਗੇ();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ਅਚਲ ਮਾਪ_t nMTasks = m_m_Task_ਗ੍ਰਾਫ.vertices().ਮਾਪ();
            m_mtaskDatap.reset(ਨਵਾਂ MTask_Contraction_ਅੰਕੜੇ[nMTasks]);
            ਮਾਪ_t i = 0;
            ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_m_Task_ਗ੍ਰਾਫ.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_m_Task_ਗ੍ਰਾਫ.vertices()) {
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtx.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ())
                m_sb.ਜੋੜੋ_ਕਿਨਾਰਾ_MC(static_cast<MTask_ਕਿਨਾਰਾ*>(&ਕਿਨਾਰਾ));
            LogicMTask* ਅਚਲ mtaskp = static_cast<LogicMTask*>(&vtx);
            ਜੋੜੋ_Sibling_MCs_From_Relatives<ਗ੍ਰਾਫ_Way::ਉਲਟ_2, ਸੱਚ>(mtaskp);
            ਜੋੜੋ_Sibling_MCs_From_Relatives<ਗ੍ਰਾਫ_Way::FORWARD, ਸੱਚ>(mtaskp);
        }

        ਜਦੋਂ (ਸੱਚ) {
            // Pick the candidate yielding the lowest local critical path.
            ਮਿਲਾਓ_Candidate* ਅਚਲ ਮਿਲਾਓ_Canp = m_sb.ਉੱਤਮ();
            ਜੇ (!ਮਿਲਾਓ_Canp) ਰੋਕੋ;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ਅਚਲ uint64_t score = ਮਿਲਾਓ_Canp->currentScore();
            ਜੇ (score != ਮਿਲਾਓ_Canp->score()) {
                m_sb.rescore(ਮਿਲਾਓ_Canp);
                ਜਾਰੀ;
            }

            // Check if the critical path limit is reached.
            ਜੇ (score > m_score_ਹੱਦਬੰਦੀ) {

                // If there are still too many MTasks, raise the limit and keep going
                ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ mtask_ਗਿਣਤੀ = m_m_Task_ਗ੍ਰਾਫ.vertices().ਮਾਪ();
                ਜੇ (mtask_ਗਿਣਤੀ > maxMTasks) {
                    m_score_ਹੱਦਬੰਦੀ = (m_score_ਹੱਦਬੰਦੀ * 120) / 100;
                    File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->fileline();
                    ਜੇ (!flp->warnIsOff(V3_Error_ਕੋਡ::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3_Error_ਕੋਡ::UNOPTTHREADS, ਸੱਚ);
                    }
                    ਜਾਰੀ;
                }

                // MTasks limit and CP limit reached. Stop.
                ਰੋਕੋ;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            ਜੇ (ਕਿਨਾਰਾ_MC* ਅਚਲ ਕਿਨਾਰਾ_MCp = ਮਿਲਾਓ_Canp->ਮੰਜ਼ਿਲ_ਕਿਨਾਰਾ_MC()) {
                MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = ਕਿਨਾਰਾ_MCp->edgep();
                ਜੇ (edgep->fromp() == m_m_Task_ਗ੍ਰਾਫ.entryp()
                    || edgep->ਸਿਖਰ() == m_m_Task_ਗ੍ਰਾਫ.exitp()) {
                    m_sb.remove_ਕਿਨਾਰਾ_MC(edgep);
                    ਜਾਰੀ;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            ਜੇ (ਮਿਲਾਓ_Canp->ਮਿਲਾਓ_Would_Create_ਚੱਕਰ(m_m_Task_ਗ੍ਰਾਫ)) {
                m_sb.removeMC(ਮਿਲਾਓ_Canp);
                ਜਾਰੀ;
            }

            // Merge this candidate
            contract(ਮਿਲਾਓ_Canp);
        }

        // Free all remaining merge candidates.
        ਜਦੋਂ (ਮਿਲਾਓ_Candidate* ਅਚਲ ਮਿਲਾਓ_Canp = m_sb.ਉੱਤਮ()) m_sb.removeMC(ਮਿਲਾਓ_Canp);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& m_Task_ਗ੍ਰਾਫ, uint64_t score_ਹੱਦਬੰਦੀ) {
        Contraction{m_Task_ਗ੍ਰਾਫ, score_ਹੱਦਬੰਦੀ};
    }
};

//######################################################################
// OrderMTaskGraph entry point

ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::contract(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& mtask_ਗ੍ਰਾਫ, uint64_t score_ਹੱਦਬੰਦੀ) {
    Contraction::apply(mtask_ਗ੍ਰਾਫ, score_ਹੱਦਬੰਦੀ);
}
