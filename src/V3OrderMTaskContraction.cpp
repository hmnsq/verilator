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

ವರ್ಗ ವಿಲೀನ_Candidate;
ವರ್ಗ SiblingMC;
ವರ್ಗ ಅಂಚು_MC;

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
constexpr ಚಿಹ್ನೆರಹಿತ PART_SIBLING_ಅಂಚು_ಎಲ್ಲೆ = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr ಚಿಹ್ನೆರಹಿತ PART_DEFAULT_MAX_MTASKS_ಪ್ರತಿ_THREAD = 50;

//######################################################################
// MTask utility classes

ರಚನೆ ವಿಲೀನ_Candidate_ಕೀಲಿ final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ ವಿಲೀನ_Candidate_ಕೀಲಿ& other) ಸ್ಥಿರಾಂಕ {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        ಹಿಂತಿರುಗಿ m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
ಬಳಸಿ ವಿಲೀನ_Candidate_ರಾಶಿ_ಗಂಟು = Pairing_ರಾಶಿ<ವಿಲೀನ_Candidate_ಕೀಲಿ>::ಗಂಟು_2;

// Information associated with scoreboarding a merge candidate
ವರ್ಗ ವಿಲೀನ_Candidate VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ ವಿಲೀನ_Candidate_ರಾಶಿ_ಗಂಟು {
    // Only the known subclasses can create or delete one of these
    ಮಿತ್ರ ವರ್ಗ SiblingMC;
    ಮಿತ್ರ ವರ್ಗ ಅಂಚು_MC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ಸ್ಥಿರ constexpr uint64_t IS_SIBLING_ಮುಖವಾಡ = 1ULL << 0;
    ಸ್ಥಿರ constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    ತಾರ್ಕಿಕ isSiblingMC() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿ.m_id & IS_SIBLING_ಮುಖವಾಡ; }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ವಿಲೀನ_Candidate(ತಾರ್ಕಿಕ isSiblingMC) {
        ಸ್ಥಿರ uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_ಕೀಲಿ.m_id = s_serial | (isSiblingMC * IS_SIBLING_ಮುಖವಾಡ);
    }
    ~ವಿಲೀನ_Candidate() = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    // METHODS
    SiblingMC* ಗಮ್ಯ_Sibling_MC();  // Instead of cast<>/as<>
    ಅಂಚು_MC* ಗಮ್ಯ_ಅಂಚು_MC();  // Instead of cast<>/as<>
    ತಾರ್ಕಿಕ ವಿಲೀನ_Would_Create_ಚಕ್ರ(ಕ್ರಮ_MTask_ನಕ್ಷೆ& ನಕ್ಷೆ);

    // The current score of this candidate, which changes as the graph is contracted
    ಸಾಲೊಳಗೆ uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿ.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    ಶೂನ್ಯ updateScore() { m_ಕೀಲಿ.m_score = currentScore(); }

    ಸ್ಥಿರ ವಿಲೀನ_Candidate* ರಾಶಿ_ಗಂಟು_ಗಮ್ಯ_Elem(ವಿಲೀನ_Candidate_ರಾಶಿ_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ static_cast<ವಿಲೀನ_Candidate*>(nodep);
    }
};

static_assert(ಗಾತ್ರ(ವಿಲೀನ_Candidate) == ಗಾತ್ರ(ವಿಲೀನ_Candidate_ರಾಶಿ_ಗಂಟು),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
ವರ್ಗ SiblingMC final : ಸಾರ್ವಜನಿಕ ವಿಲೀನ_Candidate {
    LogicMTask* ಸ್ಥಿರಾಂಕ m_ap;  // The higher ID MTask
    LogicMTask* ಸ್ಥಿರಾಂಕ m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { ಹಿಂತಿರುಗಿ m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { ಹಿಂತಿರುಗಿ m_bLinks; }

ಸಾರ್ವಜನಿಕ:
    // List type to store instances of this class
    ಬಳಸಿ AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    ಬಳಸಿ BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    ಶೂನ್ಯ unlinkA();
    ಶೂನ್ಯ unlinkB();

    LogicMTask* ap() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ap; }
    LogicMTask* bp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::ಮೌಲ್ಯ, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
ವರ್ಗ ಅಂಚು_MC final : ಸಾರ್ವಜನಿಕ ವಿಲೀನ_Candidate {
    MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ m_edgep;  // The associated edge

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಅಂಚು_MC(MTask_ಅಂಚು* edgep)
        : ವಿಲೀನ_Candidate{/* isSiblingMC: */ ಸುಳ್ಳು}
        , m_edgep{edgep} {}
    ~ಅಂಚು_MC() = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    MTask_ಅಂಚು* edgep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_edgep; }
};

static_assert(!std::is_polymorphic<ಅಂಚು_MC>::ಮೌಲ್ಯ, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
ರಚನೆ MTask_Contraction_ದತ್ತಾಂಶ final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_ಗಣ<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
ಸ್ಥಿರ MTask_Contraction_ದತ್ತಾಂಶ& mtask_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ LogicMTask* mtaskp) {
    ಹಿಂತಿರುಗಿ *static_cast<MTask_Contraction_ದತ್ತಾಂಶ*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
ಸ್ಥಿರ ಅಂಚು_MC* ಅಂಚು_MC_2(ಸ್ಥಿರಾಂಕ MTask_ಅಂಚು* edgep) { ಹಿಂತಿರುಗಿ static_cast<ಅಂಚು_MC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* ವಿಲೀನ_Candidate::ಗಮ್ಯ_Sibling_MC() {
    ಹಿಂತಿರುಗಿ isSiblingMC() ? static_cast<SiblingMC*>(ಇದು) : ಶೂನ್ಯಸೂಚಕ;
}

ಅಂಚು_MC* ವಿಲೀನ_Candidate::ಗಮ್ಯ_ಅಂಚು_MC() { ಹಿಂತಿರುಗಿ isSiblingMC() ? ಶೂನ್ಯಸೂಚಕ : static_cast<ಅಂಚು_MC*>(ಇದು); }

ತಾರ್ಕಿಕ ವಿಲೀನ_Candidate::ವಿಲೀನ_Would_Create_ಚಕ್ರ(ಕ್ರಮ_MTask_ನಕ್ಷೆ& ನಕ್ಷೆ) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    ಆದರೆ (ಸ್ಥಿರಾಂಕ SiblingMC* ಸ್ಥಿರಾಂಕ sibp = ಗಮ್ಯ_Sibling_MC()) {
        ಹಿಂತಿರುಗಿ ನಕ್ಷೆ.ಮಾರ್ಗ_Exists(sibp->ap(), sibp->bp(), ಶೂನ್ಯಸೂಚಕ)
               || ನಕ್ಷೆ.ಮಾರ್ಗ_Exists(sibp->bp(), sibp->ap(), ಶೂನ್ಯಸೂಚಕ);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = ಗಮ್ಯ_ಅಂಚು_MC()->edgep();
    ಹಿಂತಿರುಗಿ ನಕ್ಷೆ.ಮಾರ್ಗ_Exists(edgep->fromMTaskp(), edgep->ಗಮ್ಯ_MTaskp(), edgep);
}

uint64_t ವಿಲೀನ_Candidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    ಆದರೆ (ಸ್ಥಿರಾಂಕ SiblingMC* ಸ್ಥಿರಾಂಕ sibp = ಗಮ್ಯ_Sibling_MC()) {
        ಸ್ಥಿರಾಂಕ LogicMTask* ಸ್ಥಿರಾಂಕ ap = sibp->ap();
        ಸ್ಥಿರಾಂಕ LogicMTask* ಸ್ಥಿರಾಂಕ bp = sibp->bp();
        ಸ್ಥಿರಾಂಕ uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<ನಕ್ಷೆ_Way::FORWARD>(), bp->cpExclusive<ನಕ್ಷೆ_Way::FORWARD>());
        ಸ್ಥಿರಾಂಕ uint64_t mergedCpRev
            = std::max(ap->cpExclusive<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(), bp->cpExclusive<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>());
        ಹಿಂತಿರುಗಿ mergedCpRev + mergedCpFwd + ap->ವೆಚ್ಚ() + bp->ವೆಚ್ಚ();
    }

    // Edge merge
    {
        MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = ಗಮ್ಯ_ಅಂಚು_MC()->edgep();
        ಸ್ಥಿರಾಂಕ LogicMTask* ಸ್ಥಿರಾಂಕ fromp = edgep->fromMTaskp();
        ಸ್ಥಿರಾಂಕ LogicMTask* ಸ್ಥಿರಾಂಕ ತುದಿ = edgep->ಗಮ್ಯ_MTaskp();
        ಸ್ಥಿರಾಂಕ uint64_t mergedCpFwd = std::max(fromp->cpExclusive<ನಕ್ಷೆ_Way::FORWARD>(),
                                              ತುದಿ->cpExclusiveWithout<ನಕ್ಷೆ_Way::FORWARD>(edgep));
        ಸ್ಥಿರಾಂಕ uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(edgep),
                                              ತುದಿ->cpExclusive<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        ಹಿಂತಿರುಗಿ 1 + mergedCpRev + mergedCpFwd + fromp->ವೆಚ್ಚ() + ತುದಿ->ವೆಚ್ಚ();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : ವಿಲೀನ_Candidate{/* isSiblingMC: */ ನಿಜ}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtask_ದತ್ತಾಂಶ(ap).siblings.ಎಣಿಕೆ_2(bp), "Should be in sibling map"););
    mtask_ದತ್ತಾಂಶ(m_ap).aSiblingMCs.link_ಹಿಂಭಾಗ(ಇದು);
    mtask_ದತ್ತಾಂಶ(m_bp).bSiblingMCs.link_ಹಿಂಭಾಗ(ಇದು);
}

ಶೂನ್ಯ SiblingMC::unlinkA() {
    VL_ATTR_ಬಳಸದ ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t removed = mtask_ದತ್ತಾಂಶ(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtask_ದತ್ತಾಂಶ(m_ap).aSiblingMCs.unlink(ಇದು);
}

ಶೂನ್ಯ SiblingMC::unlinkB() { mtask_ದತ್ತಾಂಶ(m_bp).bSiblingMCs.unlink(ಇದು); }

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
ವರ್ಗ ವಿಲೀನ_Candidate_Scoreboard final {
    // TYPES
    ಬಳಸಿ ರಾಶಿ_2 = Pairing_ರಾಶಿ<ವಿಲೀನ_Candidate_ಕೀಲಿ>;

    // MEMBERS
    ರಾಶಿ_2 m_ರಾಶಿ_2;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<ಅಂಚು_MC> m_ಅಂಚು_MCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    ಶೂನ್ಯ ಸೇರಿಸು(ವಿಲೀನ_Candidate* nodep) {
        nodep->updateScore();
        m_ರಾಶಿ_2.ಸೇರಿಸು(nodep);
    }

    // Remove a candidate from the scoreboard.
    ಶೂನ್ಯ remove(ವಿಲೀನ_Candidate* nodep) { m_ರಾಶಿ_2.remove(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ವಿಲೀನ_Candidate_Scoreboard() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~ವಿಲೀನ_Candidate_Scoreboard() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(ವಿಲೀನ_Candidate_Scoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    ವಿಲೀನ_Candidate* ಉತ್ತಮ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ವಿಲೀನ_Candidate::ರಾಶಿ_ಗಂಟು_ಗಮ್ಯ_Elem(m_ರಾಶಿ_2.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    ಶೂನ್ಯ rescore(ವಿಲೀನ_Candidate* nodep) {
        remove(nodep);
        ಸೇರಿಸು(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು_MC(MTask_ಅಂಚು* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        ಅಂಚು_MC* ಸ್ಥಿರಾಂಕ ಅಂಚು_MCp = m_ಅಂಚು_MCPool.alloc(edgep);
        edgep->userp(ಅಂಚು_MCp);
        ಸೇರಿಸು(ಅಂಚು_MCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    ಶೂನ್ಯ remove_ಅಂಚು_MC(MTask_ಅಂಚು* edgep) {
        ಅಂಚು_MC* ಸ್ಥಿರಾಂಕ ಅಂಚು_MCp = ಅಂಚು_MC_2(edgep);
        UDEBUGONLY(UASSERT(ಅಂಚು_MCp, "Edge has no merge candidate"););
        edgep->userp(ಶೂನ್ಯಸೂಚಕ);
        remove(ಅಂಚು_MCp);
        VL_DO_DANGLING(m_ಅಂಚು_MCPool.ಬಿಡುಗಡೆ(ಅಂಚು_MCp), ಅಂಚು_MCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    ಶೂನ್ಯ ಕೂಡಿಸು_Sibling_MC(LogicMTask* ap, LogicMTask* bp) { ಸೇರಿಸು(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    ಶೂನ್ಯ removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.ಬಿಡುಗಡೆ(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    ಶೂನ್ಯ removeMC(ವಿಲೀನ_Candidate* nodep) {
        ಆದರೆ (SiblingMC* ಸ್ಥಿರಾಂಕ smcp = nodep->ಗಮ್ಯ_Sibling_MC()) {
            removeSiblingMC(smcp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            remove_ಅಂಚು_MC(nodep->ಗಮ್ಯ_ಅಂಚು_MC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
ವರ್ಗ Contraction final {
    // MEMBERS
    ಕ್ರಮ_MTask_ನಕ್ಷೆ& m_m_Task_ನಕ್ಷೆ;  // The Mtask graph
    uint64_t m_score_ಎಲ್ಲೆ;  // Critical path limit for merges
    ವಿಲೀನ_Candidate_Scoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTask_Contraction_ದತ್ತಾಂಶ[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು_MCs(LogicMTask* mtaskp) {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& e : mtaskp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) m_sb.ಕೂಡಿಸು_ಅಂಚು_MC(static_cast<MTask_ಅಂಚು*>(&e));
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& e : mtaskp->ಒಳ_ಅಂಚುಗಳು()) m_sb.ಕೂಡಿಸು_ಅಂಚು_MC(static_cast<MTask_ಅಂಚು*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    ಶೂನ್ಯ remove_ಅಂಚು_MCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtaskp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = static_cast<MTask_ಅಂಚು*>(&ಅಂಚು);
            ಆದರೆ (ಅಂಚು_MC_2(edgep)) m_sb.remove_ಅಂಚು_MC(edgep);
        }
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtaskp->ಒಳ_ಅಂಚುಗಳು()) {
            MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = static_cast<MTask_ಅಂಚು*>(&ಅಂಚು);
            ಆದರೆ (ಅಂಚು_MC_2(edgep)) m_sb.remove_ಅಂಚು_MC(edgep);
        }
    }

    ಶೂನ್ಯ makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        ಆದರೆ (ap->id() < bp->id()) std::ಅದಲುಬದಲು(ap, bp);
        // The higher id vertex owns the association set
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮೊದಲ = mtask_ದತ್ತಾಂಶ(ap).siblings.ಸೇರಿಸು(bp).ಎರಡನೇ;
        ಆದರೆ (ಮೊದಲ) {
            m_sb.ಕೂಡಿಸು_Sibling_MC(ap, bp);
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (VL_UNLIKELY(m_m_Task_ನಕ್ಷೆ.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು = ಸುಳ್ಳು;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ SiblingMC& smc : mtask_ದತ್ತಾಂಶ(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                ಆದರೆ (smc.bp() == bp) ಸಿಕ್ಕಿತು = ನಿಜ;
            }
            UASSERT_OBJ(ಸಿಕ್ಕಿತು, ap, "Sibling not found");
        }
    }

    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way, ತಾರ್ಕಿಕ N_Exhaustive>
    ಶೂನ್ಯ ಕೂಡಿಸು_Sibling_MCs_From_Relatives(LogicMTask* mtaskp) {
        constexpr ನಕ್ಷೆ_Way way{N_Way};
        // Need at least 2 edges
        ಸ್ವಯಂಚಾಲಿತ& ಅಂಚುಗಳು = mtaskp->ಅಂಚುಗಳು<way>();
        ಆದರೆ (!ಅಂಚುಗಳು.has_ಅಪವರ್ತ್ಯ_Elements()) ಹಿಂತಿರುಗಿ;

        std::array<LogicMTask*, PART_SIBLING_ಅಂಚು_ಎಲ್ಲೆ> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        ರಚನೆ alignas(16) ವಿಂಗಡಣೆ_ದಾಖಲೆ final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_ಅಂಚು_ಎಲ್ಲೆ <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ ವಿಂಗಡಣೆ_ದಾಖಲೆ& that) ಸ್ಥಿರಾಂಕ {
                ಹಿಂತಿರುಗಿ m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(ಗಾತ್ರ(ವಿಂಗಡಣೆ_ದಾಖಲೆ) <= 16, "How could this be padded to more than 16?");

        std::array<ವಿಂಗಡಣೆ_ದಾಖಲೆ, PART_SIBLING_ಅಂಚು_ಎಲ್ಲೆ> ವಿಂಗಡಿಸು_Recs;
        ಗಾತ್ರ_t n = 0;

        // Populate the buffers
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mtaskp->ಅಂಚುಗಳು<way>()) {
            LogicMTask* ಸ್ಥಿರಾಂಕ otherp = static_cast<LogicMTask*>(ಅಂಚು.furtherp<way>());
            neighbors[n] = otherp;
            ವಿಂಗಡಿಸು_Recs[n].m_id = otherp->id();
            ವಿಂಗಡಿಸು_Recs[n].m_cp = otherp->cpInclusive<way>();
            ವಿಂಗಡಿಸು_Recs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            ಆದರೆ (n >= PART_SIBLING_ಅಂಚು_ಎಲ್ಲೆ) ನಿಲ್ಲಿಸು;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr ಗಾತ್ರ_t MAX_NONEXHAUSTIVE_ಜೋಡಿಗಳು = 3;

        ಆದರೆ (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_ಜೋಡಿಗಳು) {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಕೊನೆ = n & ~static_cast<ಗಾತ್ರ_t>(1);  // Round down to even, (we want pairs)
            std::ವಿಂಗಡಿಸು(ವಿಂಗಡಿಸು_Recs.begin(), ವಿಂಗಡಿಸು_Recs.begin() + n);
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಕೊನೆ; i += 2) {
                makeSiblingMC(neighbors[ವಿಂಗಡಿಸು_Recs[i].m_idx], neighbors[ವಿಂಗಡಿಸು_Recs[i + 1].m_idx]);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            constexpr ಗಾತ್ರ_t ಕೊನೆ = 2 * MAX_NONEXHAUSTIVE_ಜೋಡಿಗಳು;
            std::partial_ವಿಂಗಡಿಸು(ವಿಂಗಡಿಸು_Recs.begin(), ವಿಂಗಡಿಸು_Recs.begin() + ಕೊನೆ, ವಿಂಗಡಿಸು_Recs.begin() + n);
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಕೊನೆ; i += 2) {
                makeSiblingMC(neighbors[ವಿಂಗಡಿಸು_Recs[i].m_idx], neighbors[ವಿಂಗಡಿಸು_Recs[i + 1].m_idx]);
            }
        }
    }

    ಶೂನ್ಯ removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        ಆಗ (SiblingMC* ಸ್ಥಿರಾಂಕ smcp = mtask_ದತ್ತಾಂಶ(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        ಆಗ (SiblingMC* ಸ್ಥಿರಾಂಕ smcp = mtask_ದತ್ತಾಂಶ(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    ಶೂನ್ಯ contract(ವಿಲೀನ_Candidate* ವಿಲೀನ_Canp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* ತುದಿ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಅಂಚು_MC* ಸ್ಥಿರಾಂಕ ಅಂಚು_MCp = ವಿಲೀನ_Canp->ಗಮ್ಯ_ಅಂಚು_MC()) {
            fromp = ಅಂಚು_MCp->edgep()->fromMTaskp();
            ತುದಿ = ಅಂಚು_MCp->edgep()->ಗಮ್ಯ_MTaskp();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ SiblingMC* ಸ್ಥಿರಾಂಕ sibMCp = ವಿಲೀನ_Canp->ಗಮ್ಯ_Sibling_MC();
            fromp = sibMCp->bp();
            ತುದಿ = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        ಆದರೆ (fromp->ವೆಚ್ಚ() > ತುದಿ->ವೆಚ್ಚ()) {
            recipientp = fromp;
            donorp = ತುದಿ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            donorp = fromp;
            recipientp = ತುದಿ;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(ತುದಿ);  // Use donorp and recipientp now instead

        // Remove all SiblingMCs that include either MTask
        removeSiblingMCs(recipientp);
        removeSiblingMCs(donorp);

        // Remove the EdgeMCs of both MTasks
        remove_ಅಂಚು_MCs(recipientp);
        remove_ಅಂಚು_MCs(donorp);

        // Merge the MTasks. This redirects all edges, updates critical paths, and deletes donorp
        m_m_Task_ನಕ್ಷೆ.ವಿಲೀನ_MTasks(recipientp, donorp);
        VL_DANGLING(donorp);

        // Confirm we haven't botched the CP updates. This is a whole graph walk after every single
        // merge, so it is quadratic in the size of the graph, hence only under '--debug 9'.
        ಆದರೆ (VL_UNLIKELY(debug() >= 9)) m_m_Task_ನಕ್ಷೆ.validate();

        // Add the EdgeMCs of the merged MTask
        ಕೂಡಿಸು_ಅಂಚು_MCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        ಕೂಡಿಸು_Sibling_MCs_From_Relatives<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2, ನಿಜ>(recipientp);
        ಕೂಡಿಸು_Sibling_MCs_From_Relatives<ನಕ್ಷೆ_Way::FORWARD, ನಿಜ>(recipientp);
        ಚಿಹ್ನೆರಹಿತ ಅಂಚುಗಳು = 0;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : recipientp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            LogicMTask* ಸ್ಥಿರಾಂಕ postreqp = static_cast<LogicMTask*>(ಅಂಚು.ತುದಿ());
            ಕೂಡಿಸು_Sibling_MCs_From_Relatives<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2, ಸುಳ್ಳು>(postreqp);
            ++ಅಂಚುಗಳು;
            ಆದರೆ (ಅಂಚುಗಳು >= PART_SIBLING_ಅಂಚು_ಎಲ್ಲೆ) ನಿಲ್ಲಿಸು;
        }
        ಅಂಚುಗಳು = 0;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : recipientp->ಒಳ_ಅಂಚುಗಳು()) {
            LogicMTask* ಸ್ಥಿರಾಂಕ prereqp = static_cast<LogicMTask*>(ಅಂಚು.fromp());
            ಕೂಡಿಸು_Sibling_MCs_From_Relatives<ನಕ್ಷೆ_Way::FORWARD, ಸುಳ್ಳು>(prereqp);
            ++ಅಂಚುಗಳು;
            ಆದರೆ (ಅಂಚುಗಳು >= PART_SIBLING_ಅಂಚು_ಎಲ್ಲೆ) ನಿಲ್ಲಿಸು;
        }
    }

    // CONSTRUCTORS
    Contraction(ಕ್ರಮ_MTask_ನಕ್ಷೆ& m_Task_ನಕ್ಷೆ, uint64_t cp_ಎಲ್ಲೆ)
        : m_m_Task_ನಕ್ಷೆ{m_Task_ನಕ್ಷೆ}
        , m_score_ಎಲ್ಲೆ{cp_ಎಲ್ಲೆ} {

        // Check the graph we were given is consistent.
        m_m_Task_ನಕ್ಷೆ.validate();

        // Figure out maximum number of MTasks
        ಸ್ಥಿರಾಂಕ uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ given = v3Global.opt.ಎಳೆಗಳು_Max_MTasks();
            ಆದರೆ (given > 0) ಹಿಂತಿರುಗಿ given;
            // Unspecified so estimate
            ಹಿಂತಿರುಗಿ PART_DEFAULT_MAX_MTASKS_ಪ್ರತಿ_THREAD * v3Global.opt.ಎಳೆಗಳು();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nMTasks = m_m_Task_ನಕ್ಷೆ.vertices().ಗಾತ್ರ_2();
            m_mtaskDatap.reset(ಹೊಸ MTask_Contraction_ದತ್ತಾಂಶ[nMTasks]);
            ಗಾತ್ರ_t i = 0;
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_m_Task_ನಕ್ಷೆ.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_m_Task_ನಕ್ಷೆ.vertices()) {
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtx.ಹೊರಹೋಗು_ಅಂಚುಗಳು())
                m_sb.ಕೂಡಿಸು_ಅಂಚು_MC(static_cast<MTask_ಅಂಚು*>(&ಅಂಚು));
            LogicMTask* ಸ್ಥಿರಾಂಕ mtaskp = static_cast<LogicMTask*>(&vtx);
            ಕೂಡಿಸು_Sibling_MCs_From_Relatives<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2, ನಿಜ>(mtaskp);
            ಕೂಡಿಸು_Sibling_MCs_From_Relatives<ನಕ್ಷೆ_Way::FORWARD, ನಿಜ>(mtaskp);
        }

        ಆಗ (ನಿಜ) {
            // Pick the candidate yielding the lowest local critical path.
            ವಿಲೀನ_Candidate* ಸ್ಥಿರಾಂಕ ವಿಲೀನ_Canp = m_sb.ಉತ್ತಮ();
            ಆದರೆ (!ವಿಲೀನ_Canp) ನಿಲ್ಲಿಸು;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ಸ್ಥಿರಾಂಕ uint64_t score = ವಿಲೀನ_Canp->currentScore();
            ಆದರೆ (score != ವಿಲೀನ_Canp->score()) {
                m_sb.rescore(ವಿಲೀನ_Canp);
                ಮುಂದುವರಿ;
            }

            // Check if the critical path limit is reached.
            ಆದರೆ (score > m_score_ಎಲ್ಲೆ) {

                // If there are still too many MTasks, raise the limit and keep going
                ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ mtask_ಎಣಿಕೆ = m_m_Task_ನಕ್ಷೆ.vertices().ಗಾತ್ರ_2();
                ಆದರೆ (mtask_ಎಣಿಕೆ > maxMTasks) {
                    m_score_ಎಲ್ಲೆ = (m_score_ಎಲ್ಲೆ * 120) / 100;
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->fileline();
                    ಆದರೆ (!flp->warnIsOff(V3_Error_ಸಂಕೇತ::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNOPTTHREADS, ನಿಜ);
                    }
                    ಮುಂದುವರಿ;
                }

                // MTasks limit and CP limit reached. Stop.
                ನಿಲ್ಲಿಸು;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            ಆದರೆ (ಅಂಚು_MC* ಸ್ಥಿರಾಂಕ ಅಂಚು_MCp = ವಿಲೀನ_Canp->ಗಮ್ಯ_ಅಂಚು_MC()) {
                MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = ಅಂಚು_MCp->edgep();
                ಆದರೆ (edgep->fromp() == m_m_Task_ನಕ್ಷೆ.entryp()
                    || edgep->ತುದಿ() == m_m_Task_ನಕ್ಷೆ.exitp()) {
                    m_sb.remove_ಅಂಚು_MC(edgep);
                    ಮುಂದುವರಿ;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            ಆದರೆ (ವಿಲೀನ_Canp->ವಿಲೀನ_Would_Create_ಚಕ್ರ(m_m_Task_ನಕ್ಷೆ)) {
                m_sb.removeMC(ವಿಲೀನ_Canp);
                ಮುಂದುವರಿ;
            }

            // Merge this candidate
            contract(ವಿಲೀನ_Canp);
        }

        // Free all remaining merge candidates.
        ಆಗ (ವಿಲೀನ_Candidate* ಸ್ಥಿರಾಂಕ ವಿಲೀನ_Canp = m_sb.ಉತ್ತಮ()) m_sb.removeMC(ವಿಲೀನ_Canp);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(ಕ್ರಮ_MTask_ನಕ್ಷೆ& m_Task_ನಕ್ಷೆ, uint64_t score_ಎಲ್ಲೆ) {
        Contraction{m_Task_ನಕ್ಷೆ, score_ಎಲ್ಲೆ};
    }
};

//######################################################################
// OrderMTaskGraph entry point

ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::contract(ಕ್ರಮ_MTask_ನಕ್ಷೆ& mtask_ನಕ್ಷೆ, uint64_t score_ಎಲ್ಲೆ) {
    Contraction::apply(mtask_ನಕ್ಷೆ, score_ಎಲ್ಲೆ);
}
