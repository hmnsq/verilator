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

صنف یوځای_Candidate;
صنف SiblingMC;
صنف څنډه_MC;

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
constexpr بینښه PART_SIBLING_څنډه_حد = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr بینښه PART_DEFAULT_MAX_MTASKS_فی_THREAD = 50;

//######################################################################
// MTask utility classes

بنية یوځای_Candidate_کلی final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    منطقي چلوونکی<(ثابت یوځای_Candidate_کلی& other) ثابت {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        أرجع m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
کارول یوځای_Candidate_ډېرۍ_غوټه = Pairing_ډېرۍ<یوځای_Candidate_کلی>::غوټه_2;

// Information associated with scoreboarding a merge candidate
صنف یوځای_Candidate VL_NOT_FINAL : عام یوځای_Candidate_ډېرۍ_غوټه {
    // Only the known subclasses can create or delete one of these
    ملګری صنف SiblingMC;
    ملګری صنف څنډه_MC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ساكن constexpr uint64_t IS_SIBLING_ماسک = 1ULL << 0;
    ساكن constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    منطقي isSiblingMC() ثابت { أرجع m_کلی.m_id & IS_SIBLING_ماسک; }

    // CONSTRUCTORS
    څرګند یوځای_Candidate(منطقي isSiblingMC) {
        ساكن uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_کلی.m_id = s_serial | (isSiblingMC * IS_SIBLING_ماسک);
    }
    ~یوځای_Candidate() = افتراضي;

عام:
    // METHODS
    SiblingMC* منزل_Sibling_MC();  // Instead of cast<>/as<>
    څنډه_MC* منزل_څنډه_MC();  // Instead of cast<>/as<>
    منطقي یوځای_Would_Create_دوره(ترتیب_MTask_ګراف& ګراف);

    // The current score of this candidate, which changes as the graph is contracted
    مضمن uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ثابت { أرجع m_کلی.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    فراغ updateScore() { m_کلی.m_score = currentScore(); }

    ساكن یوځای_Candidate* ډېرۍ_غوټه_منزل_Elem(یوځای_Candidate_ډېرۍ_غوټه* nodep) {
        أرجع static_cast<یوځای_Candidate*>(nodep);
    }
};

static_assert(حجم(یوځای_Candidate) == حجم(یوځای_Candidate_ډېرۍ_غوټه),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
صنف SiblingMC final : عام یوځای_Candidate {
    LogicMTask* ثابت m_ap;  // The higher ID MTask
    LogicMTask* ثابت m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { أرجع m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { أرجع m_bLinks; }

عام:
    // List type to store instances of this class
    کارول AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    کارول BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = افتراضي;

    // METHODS
    فراغ unlinkA();
    فراغ unlinkB();

    LogicMTask* ap() ثابت { أرجع m_ap; }
    LogicMTask* bp() ثابت { أرجع m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::ارزښت, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
صنف څنډه_MC final : عام یوځای_Candidate {
    MTask_څنډه* ثابت m_edgep;  // The associated edge

عام:
    // CONSTRUCTORS
    څرګند څنډه_MC(MTask_څنډه* edgep)
        : یوځای_Candidate{/* isSiblingMC: */ خطأ}
        , m_edgep{edgep} {}
    ~څنډه_MC() = افتراضي;

    // METHODS
    MTask_څنډه* edgep() ثابت { أرجع m_edgep; }
};

static_assert(!std::is_polymorphic<څنډه_MC>::ارزښت, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
بنية MTask_Contraction_معلومات final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_سېټ<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
ساكن MTask_Contraction_معلومات& mtask_معلومات(ثابت LogicMTask* mtaskp) {
    أرجع *static_cast<MTask_Contraction_معلومات*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
ساكن څنډه_MC* څنډه_MC_2(ثابت MTask_څنډه* edgep) { أرجع static_cast<څنډه_MC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* یوځای_Candidate::منزل_Sibling_MC() {
    أرجع isSiblingMC() ? static_cast<SiblingMC*>(هذا) : تش_اشاره;
}

څنډه_MC* یوځای_Candidate::منزل_څنډه_MC() { أرجع isSiblingMC() ? تش_اشاره : static_cast<څنډه_MC*>(هذا); }

منطقي یوځای_Candidate::یوځای_Would_Create_دوره(ترتیب_MTask_ګراف& ګراف) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    إذا (ثابت SiblingMC* ثابت sibp = منزل_Sibling_MC()) {
        أرجع ګراف.لار_Exists(sibp->ap(), sibp->bp(), تش_اشاره)
               || ګراف.لار_Exists(sibp->bp(), sibp->ap(), تش_اشاره);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTask_څنډه* ثابت edgep = منزل_څنډه_MC()->edgep();
    أرجع ګراف.لار_Exists(edgep->fromMTaskp(), edgep->منزل_MTaskp(), edgep);
}

uint64_t یوځای_Candidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    إذا (ثابت SiblingMC* ثابت sibp = منزل_Sibling_MC()) {
        ثابت LogicMTask* ثابت ap = sibp->ap();
        ثابت LogicMTask* ثابت bp = sibp->bp();
        ثابت uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<ګراف_Way::FORWARD>(), bp->cpExclusive<ګراف_Way::FORWARD>());
        ثابت uint64_t mergedCpRev
            = std::max(ap->cpExclusive<ګراف_Way::سرچپه_2>(), bp->cpExclusive<ګراف_Way::سرچپه_2>());
        أرجع mergedCpRev + mergedCpFwd + ap->لګښت() + bp->لګښت();
    }

    // Edge merge
    {
        MTask_څنډه* ثابت edgep = منزل_څنډه_MC()->edgep();
        ثابت LogicMTask* ثابت fromp = edgep->fromMTaskp();
        ثابت LogicMTask* ثابت سر = edgep->منزل_MTaskp();
        ثابت uint64_t mergedCpFwd = std::max(fromp->cpExclusive<ګراف_Way::FORWARD>(),
                                              سر->cpExclusiveWithout<ګراف_Way::FORWARD>(edgep));
        ثابت uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<ګراف_Way::سرچپه_2>(edgep),
                                              سر->cpExclusive<ګراف_Way::سرچپه_2>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        أرجع 1 + mergedCpRev + mergedCpFwd + fromp->لګښت() + سر->لګښت();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : یوځای_Candidate{/* isSiblingMC: */ ریښتیا}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtask_معلومات(ap).siblings.شمېر(bp), "Should be in sibling map"););
    mtask_معلومات(m_ap).aSiblingMCs.link_شا(هذا);
    mtask_معلومات(m_bp).bSiblingMCs.link_شا(هذا);
}

فراغ SiblingMC::unlinkA() {
    VL_ATTR_ناکارول_شوی ثابت اندازه_t removed = mtask_معلومات(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtask_معلومات(m_ap).aSiblingMCs.unlink(هذا);
}

فراغ SiblingMC::unlinkB() { mtask_معلومات(m_bp).bSiblingMCs.unlink(هذا); }

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
صنف یوځای_Candidate_Scoreboard final {
    // TYPES
    کارول ډېرۍ = Pairing_ډېرۍ<یوځای_Candidate_کلی>;

    // MEMBERS
    ډېرۍ m_ډېرۍ;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<څنډه_MC> m_څنډه_MCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    فراغ ورننباسه(یوځای_Candidate* nodep) {
        nodep->updateScore();
        m_ډېرۍ.ورننباسه(nodep);
    }

    // Remove a candidate from the scoreboard.
    فراغ remove(یوځای_Candidate* nodep) { m_ډېرۍ.remove(nodep); }

عام:
    // CONSTRUCTORS
    یوځای_Candidate_Scoreboard() = افتراضي;
    ~یوځای_Candidate_Scoreboard() = افتراضي;
    VL_UNCOPYABLE(یوځای_Candidate_Scoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    یوځای_Candidate* غوره() ثابت { أرجع یوځای_Candidate::ډېرۍ_غوټه_منزل_Elem(m_ډېرۍ.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    فراغ rescore(یوځای_Candidate* nodep) {
        remove(nodep);
        ورننباسه(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    فراغ جمع_کړه_څنډه_MC(MTask_څنډه* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        څنډه_MC* ثابت څنډه_MCp = m_څنډه_MCPool.alloc(edgep);
        edgep->userp(څنډه_MCp);
        ورننباسه(څنډه_MCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    فراغ remove_څنډه_MC(MTask_څنډه* edgep) {
        څنډه_MC* ثابت څنډه_MCp = څنډه_MC_2(edgep);
        UDEBUGONLY(UASSERT(څنډه_MCp, "Edge has no merge candidate"););
        edgep->userp(تش_اشاره);
        remove(څنډه_MCp);
        VL_DO_DANGLING(m_څنډه_MCPool.حرر(څنډه_MCp), څنډه_MCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    فراغ جمع_کړه_Sibling_MC(LogicMTask* ap, LogicMTask* bp) { ورننباسه(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    فراغ removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.حرر(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    فراغ removeMC(یوځای_Candidate* nodep) {
        إذا (SiblingMC* ثابت smcp = nodep->منزل_Sibling_MC()) {
            removeSiblingMC(smcp);
        } وإلا {
            remove_څنډه_MC(nodep->منزل_څنډه_MC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
صنف Contraction final {
    // MEMBERS
    ترتیب_MTask_ګراف& m_m_Task_ګراف;  // The Mtask graph
    uint64_t m_score_حد;  // Critical path limit for merges
    یوځای_Candidate_Scoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTask_Contraction_معلومات[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    فراغ جمع_کړه_څنډه_MCs(LogicMTask* mtaskp) {
        لكل (V3_ګراف_څنډه& e : mtaskp->بهر_څنډې()) m_sb.جمع_کړه_څنډه_MC(static_cast<MTask_څنډه*>(&e));
        لكل (V3_ګراف_څنډه& e : mtaskp->دننه_څنډې()) m_sb.جمع_کړه_څنډه_MC(static_cast<MTask_څنډه*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    فراغ remove_څنډه_MCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        لكل (V3_ګراف_څنډه& څنډه : mtaskp->بهر_څنډې()) {
            MTask_څنډه* ثابت edgep = static_cast<MTask_څنډه*>(&څنډه);
            إذا (څنډه_MC_2(edgep)) m_sb.remove_څنډه_MC(edgep);
        }
        لكل (V3_ګراف_څنډه& څنډه : mtaskp->دننه_څنډې()) {
            MTask_څنډه* ثابت edgep = static_cast<MTask_څنډه*>(&څنډه);
            إذا (څنډه_MC_2(edgep)) m_sb.remove_څنډه_MC(edgep);
        }
    }

    فراغ makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        إذا (ap->id() < bp->id()) std::بدلون(ap, bp);
        // The higher id vertex owns the association set
        ثابت منطقي لومړی = mtask_معلومات(ap).siblings.ورننباسه(bp).دوهم;
        إذا (لومړی) {
            m_sb.جمع_کړه_Sibling_MC(ap, bp);
            أرجع;
        }

        إذا (VL_UNLIKELY(m_m_Task_ګراف.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            منطقي وموندل = خطأ;
            لكل (ثابت SiblingMC& smc : mtask_معلومات(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                إذا (smc.bp() == bp) وموندل = ریښتیا;
            }
            UASSERT_OBJ(وموندل, ap, "Sibling not found");
        }
    }

    کينډۍ <ګراف_Way::en N_Way, منطقي N_Exhaustive>
    فراغ جمع_کړه_Sibling_MCs_From_Relatives(LogicMTask* mtaskp) {
        constexpr ګراف_Way way{N_Way};
        // Need at least 2 edges
        خپلکاری& څنډې = mtaskp->څنډې<way>();
        إذا (!څنډې.has_مضرب_Elements()) أرجع;

        std::array<LogicMTask*, PART_SIBLING_څنډه_حد> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        بنية alignas(16) ترتیبونه_ریکارډ final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_څنډه_حد <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            منطقي چلوونکی<(ثابت ترتیبونه_ریکارډ& that) ثابت {
                أرجع m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(حجم(ترتیبونه_ریکارډ) <= 16, "How could this be padded to more than 16?");

        std::array<ترتیبونه_ریکارډ, PART_SIBLING_څنډه_حد> ترتیبول_Recs;
        اندازه_t n = 0;

        // Populate the buffers
        لكل (V3_ګراف_څنډه& څنډه : mtaskp->څنډې<way>()) {
            LogicMTask* ثابت otherp = static_cast<LogicMTask*>(څنډه.furtherp<way>());
            neighbors[n] = otherp;
            ترتیبول_Recs[n].m_id = otherp->id();
            ترتیبول_Recs[n].m_cp = otherp->cpInclusive<way>();
            ترتیبول_Recs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            إذا (n >= PART_SIBLING_څنډه_حد) اكسر;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr اندازه_t MAX_NONEXHAUSTIVE_جوړې = 3;

        إذا (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_جوړې) {
            ثابت اندازه_t پای_ټکی = n & ~static_cast<اندازه_t>(1);  // Round down to even, (we want pairs)
            std::ترتیبول(ترتیبول_Recs.begin(), ترتیبول_Recs.begin() + n);
            لكل (اندازه_t i = 0; i < پای_ټکی; i += 2) {
                makeSiblingMC(neighbors[ترتیبول_Recs[i].m_idx], neighbors[ترتیبول_Recs[i + 1].m_idx]);
            }
        } وإلا {
            constexpr اندازه_t پای_ټکی = 2 * MAX_NONEXHAUSTIVE_جوړې;
            std::partial_ترتیبول(ترتیبول_Recs.begin(), ترتیبول_Recs.begin() + پای_ټکی, ترتیبول_Recs.begin() + n);
            لكل (اندازه_t i = 0; i < پای_ټکی; i += 2) {
                makeSiblingMC(neighbors[ترتیبول_Recs[i].m_idx], neighbors[ترتیبول_Recs[i + 1].m_idx]);
            }
        }
    }

    فراغ removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        طالما (SiblingMC* ثابت smcp = mtask_معلومات(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        طالما (SiblingMC* ثابت smcp = mtask_معلومات(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    فراغ contract(یوځای_Candidate* یوځای_Canp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* سر;
        إذا (ثابت څنډه_MC* ثابت څنډه_MCp = یوځای_Canp->منزل_څنډه_MC()) {
            fromp = څنډه_MCp->edgep()->fromMTaskp();
            سر = څنډه_MCp->edgep()->منزل_MTaskp();
        } وإلا {
            ثابت SiblingMC* ثابت sibMCp = یوځای_Canp->منزل_Sibling_MC();
            fromp = sibMCp->bp();
            سر = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        إذا (fromp->لګښت() > سر->لګښت()) {
            recipientp = fromp;
            donorp = سر;
        } وإلا {
            donorp = fromp;
            recipientp = سر;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(سر);  // Use donorp and recipientp now instead

        // Remove all SiblingMCs that include either MTask
        removeSiblingMCs(recipientp);
        removeSiblingMCs(donorp);

        // Remove the EdgeMCs of both MTasks
        remove_څنډه_MCs(recipientp);
        remove_څنډه_MCs(donorp);

        // Merge the MTasks. This redirects all edges, updates critical paths, and deletes donorp
        m_m_Task_ګراف.یوځای_MTasks(recipientp, donorp);
        VL_DANGLING(donorp);

        // Confirm we haven't botched the CP updates. This is a whole graph walk after every single
        // merge, so it is quadratic in the size of the graph, hence only under '--debug 9'.
        إذا (VL_UNLIKELY(debug() >= 9)) m_m_Task_ګراف.validate();

        // Add the EdgeMCs of the merged MTask
        جمع_کړه_څنډه_MCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        جمع_کړه_Sibling_MCs_From_Relatives<ګراف_Way::سرچپه_2, ریښتیا>(recipientp);
        جمع_کړه_Sibling_MCs_From_Relatives<ګراف_Way::FORWARD, ریښتیا>(recipientp);
        بینښه څنډې = 0;
        لكل (V3_ګراف_څنډه& څنډه : recipientp->بهر_څنډې()) {
            LogicMTask* ثابت postreqp = static_cast<LogicMTask*>(څنډه.سر());
            جمع_کړه_Sibling_MCs_From_Relatives<ګراف_Way::سرچپه_2, خطأ>(postreqp);
            ++څنډې;
            إذا (څنډې >= PART_SIBLING_څنډه_حد) اكسر;
        }
        څنډې = 0;
        لكل (V3_ګراف_څنډه& څنډه : recipientp->دننه_څنډې()) {
            LogicMTask* ثابت prereqp = static_cast<LogicMTask*>(څنډه.fromp());
            جمع_کړه_Sibling_MCs_From_Relatives<ګراف_Way::FORWARD, خطأ>(prereqp);
            ++څنډې;
            إذا (څنډې >= PART_SIBLING_څنډه_حد) اكسر;
        }
    }

    // CONSTRUCTORS
    Contraction(ترتیب_MTask_ګراف& m_Task_ګراف, uint64_t cp_حد)
        : m_m_Task_ګراف{m_Task_ګراف}
        , m_score_حد{cp_حد} {

        // Check the graph we were given is consistent.
        m_m_Task_ګراف.validate();

        // Figure out maximum number of MTasks
        ثابت uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ثابت صحيح given = v3Global.opt.تارونه_Max_MTasks();
            إذا (given > 0) أرجع given;
            // Unspecified so estimate
            أرجع PART_DEFAULT_MAX_MTASKS_فی_THREAD * v3Global.opt.تارونه();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ثابت اندازه_t nMTasks = m_m_Task_ګراف.vertices().اندازه();
            m_mtaskDatap.reset(جديد MTask_Contraction_معلومات[nMTasks]);
            اندازه_t i = 0;
            لكل (V3_ګراف_راس& vtx : m_m_Task_ګراف.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        لكل (V3_ګراف_راس& vtx : m_m_Task_ګراف.vertices()) {
            لكل (V3_ګراف_څنډه& څنډه : vtx.بهر_څنډې())
                m_sb.جمع_کړه_څنډه_MC(static_cast<MTask_څنډه*>(&څنډه));
            LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(&vtx);
            جمع_کړه_Sibling_MCs_From_Relatives<ګراف_Way::سرچپه_2, ریښتیا>(mtaskp);
            جمع_کړه_Sibling_MCs_From_Relatives<ګراف_Way::FORWARD, ریښتیا>(mtaskp);
        }

        طالما (ریښتیا) {
            // Pick the candidate yielding the lowest local critical path.
            یوځای_Candidate* ثابت یوځای_Canp = m_sb.غوره();
            إذا (!یوځای_Canp) اكسر;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ثابت uint64_t score = یوځای_Canp->currentScore();
            إذا (score != یوځای_Canp->score()) {
                m_sb.rescore(یوځای_Canp);
                استمر;
            }

            // Check if the critical path limit is reached.
            إذا (score > m_score_حد) {

                // If there are still too many MTasks, raise the limit and keep going
                ثابت بینښه mtask_شمېر = m_m_Task_ګراف.vertices().اندازه();
                إذا (mtask_شمېر > maxMTasks) {
                    m_score_حد = (m_score_حد * 120) / 100;
                    File_کرښه* ثابت flp = v3Global.rootp()->fileline();
                    إذا (!flp->warnIsOff(V3_Error_کوډ::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3_Error_کوډ::UNOPTTHREADS, ریښتیا);
                    }
                    استمر;
                }

                // MTasks limit and CP limit reached. Stop.
                اكسر;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            إذا (څنډه_MC* ثابت څنډه_MCp = یوځای_Canp->منزل_څنډه_MC()) {
                MTask_څنډه* ثابت edgep = څنډه_MCp->edgep();
                إذا (edgep->fromp() == m_m_Task_ګراف.entryp()
                    || edgep->سر() == m_m_Task_ګراف.exitp()) {
                    m_sb.remove_څنډه_MC(edgep);
                    استمر;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            إذا (یوځای_Canp->یوځای_Would_Create_دوره(m_m_Task_ګراف)) {
                m_sb.removeMC(یوځای_Canp);
                استمر;
            }

            // Merge this candidate
            contract(یوځای_Canp);
        }

        // Free all remaining merge candidates.
        طالما (یوځای_Candidate* ثابت یوځای_Canp = m_sb.غوره()) m_sb.removeMC(یوځای_Canp);
    }

عام:
    ساكن فراغ apply(ترتیب_MTask_ګراف& m_Task_ګراف, uint64_t score_حد) {
        Contraction{m_Task_ګراف, score_حد};
    }
};

//######################################################################
// OrderMTaskGraph entry point

فراغ ترتیب_MTask_ګراف::contract(ترتیب_MTask_ګراف& mtask_ګراف, uint64_t score_حد) {
    Contraction::apply(mtask_ګراف, score_حد);
}
