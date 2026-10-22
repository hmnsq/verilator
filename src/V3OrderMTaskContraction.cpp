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

صنف ملايو_Candidate;
صنف SiblingMC;
صنف ڪنارو_MC;

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
constexpr بنانشان PART_SIBLING_ڪنارو_حد_بندي = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr بنانشان PART_DEFAULT_MAX_MTASKS_في_THREAD = 50;

//######################################################################
// MTask utility classes

بنية ملايو_Candidate_ڪنجي final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    منطقي عامل<(ثابت ملايو_Candidate_ڪنجي& other) ثابت {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        أرجع m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
استعمال ملايو_Candidate_ڍير_ڳنڍ = Pairing_ڍير<ملايو_Candidate_ڪنجي>::ڳنڍ_2;

// Information associated with scoreboarding a merge candidate
صنف ملايو_Candidate VL_NOT_FINAL : عام ملايو_Candidate_ڍير_ڳنڍ {
    // Only the known subclasses can create or delete one of these
    دوست صنف SiblingMC;
    دوست صنف ڪنارو_MC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ساكن constexpr uint64_t IS_SIBLING_نقاب = 1ULL << 0;
    ساكن constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    منطقي isSiblingMC() ثابت { أرجع m_ڪنجي.m_id & IS_SIBLING_نقاب; }

    // CONSTRUCTORS
    واضح ملايو_Candidate(منطقي isSiblingMC) {
        ساكن uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_ڪنجي.m_id = s_serial | (isSiblingMC * IS_SIBLING_نقاب);
    }
    ~ملايو_Candidate() = افتراضي;

عام:
    // METHODS
    SiblingMC* منزل_Sibling_MC();  // Instead of cast<>/as<>
    ڪنارو_MC* منزل_ڪنارو_MC();  // Instead of cast<>/as<>
    منطقي ملايو_Would_Create_چڪر(ترتيب_MTask_گراف& گراف);

    // The current score of this candidate, which changes as the graph is contracted
    مضمن uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ثابت { أرجع m_ڪنجي.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    فراغ updateScore() { m_ڪنجي.m_score = currentScore(); }

    ساكن ملايو_Candidate* ڍير_ڳنڍ_منزل_Elem(ملايو_Candidate_ڍير_ڳنڍ* nodep) {
        أرجع static_cast<ملايو_Candidate*>(nodep);
    }
};

static_assert(حجم(ملايو_Candidate) == حجم(ملايو_Candidate_ڍير_ڳنڍ),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
صنف SiblingMC final : عام ملايو_Candidate {
    LogicMTask* ثابت m_ap;  // The higher ID MTask
    LogicMTask* ثابت m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { أرجع m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { أرجع m_bLinks; }

عام:
    // List type to store instances of this class
    استعمال AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    استعمال BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = افتراضي;

    // METHODS
    فراغ unlinkA();
    فراغ unlinkB();

    LogicMTask* ap() ثابت { أرجع m_ap; }
    LogicMTask* bp() ثابت { أرجع m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::قدر, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
صنف ڪنارو_MC final : عام ملايو_Candidate {
    MTask_ڪنارو* ثابت m_edgep;  // The associated edge

عام:
    // CONSTRUCTORS
    واضح ڪنارو_MC(MTask_ڪنارو* edgep)
        : ملايو_Candidate{/* isSiblingMC: */ خطأ}
        , m_edgep{edgep} {}
    ~ڪنارو_MC() = افتراضي;

    // METHODS
    MTask_ڪنارو* edgep() ثابت { أرجع m_edgep; }
};

static_assert(!std::is_polymorphic<ڪنارو_MC>::قدر, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
بنية MTask_Contraction_معلومات final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_سيٽ<LogicMTask*> siblings;
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
ساكن ڪنارو_MC* ڪنارو_MC_2(ثابت MTask_ڪنارو* edgep) { أرجع static_cast<ڪنارو_MC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* ملايو_Candidate::منزل_Sibling_MC() {
    أرجع isSiblingMC() ? static_cast<SiblingMC*>(هذا) : خالي_اشارو;
}

ڪنارو_MC* ملايو_Candidate::منزل_ڪنارو_MC() { أرجع isSiblingMC() ? خالي_اشارو : static_cast<ڪنارو_MC*>(هذا); }

منطقي ملايو_Candidate::ملايو_Would_Create_چڪر(ترتيب_MTask_گراف& گراف) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    إذا (ثابت SiblingMC* ثابت sibp = منزل_Sibling_MC()) {
        أرجع گراف.رستو_Exists(sibp->ap(), sibp->bp(), خالي_اشارو)
               || گراف.رستو_Exists(sibp->bp(), sibp->ap(), خالي_اشارو);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTask_ڪنارو* ثابت edgep = منزل_ڪنارو_MC()->edgep();
    أرجع گراف.رستو_Exists(edgep->fromMTaskp(), edgep->منزل_MTaskp(), edgep);
}

uint64_t ملايو_Candidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    إذا (ثابت SiblingMC* ثابت sibp = منزل_Sibling_MC()) {
        ثابت LogicMTask* ثابت ap = sibp->ap();
        ثابت LogicMTask* ثابت bp = sibp->bp();
        ثابت uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<گراف_Way::FORWARD>(), bp->cpExclusive<گراف_Way::FORWARD>());
        ثابت uint64_t mergedCpRev
            = std::max(ap->cpExclusive<گراف_Way::ابتو_2>(), bp->cpExclusive<گراف_Way::ابتو_2>());
        أرجع mergedCpRev + mergedCpFwd + ap->لاڳت() + bp->لاڳت();
    }

    // Edge merge
    {
        MTask_ڪنارو* ثابت edgep = منزل_ڪنارو_MC()->edgep();
        ثابت LogicMTask* ثابت fromp = edgep->fromMTaskp();
        ثابت LogicMTask* ثابت چوٽي = edgep->منزل_MTaskp();
        ثابت uint64_t mergedCpFwd = std::max(fromp->cpExclusive<گراف_Way::FORWARD>(),
                                              چوٽي->cpExclusiveWithout<گراف_Way::FORWARD>(edgep));
        ثابت uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<گراف_Way::ابتو_2>(edgep),
                                              چوٽي->cpExclusive<گراف_Way::ابتو_2>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        أرجع 1 + mergedCpRev + mergedCpFwd + fromp->لاڳت() + چوٽي->لاڳت();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : ملايو_Candidate{/* isSiblingMC: */ سچ}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtask_معلومات(ap).siblings.ڳڻپ(bp), "Should be in sibling map"););
    mtask_معلومات(m_ap).aSiblingMCs.link_پٺ(هذا);
    mtask_معلومات(m_bp).bSiblingMCs.link_پٺ(هذا);
}

فراغ SiblingMC::unlinkA() {
    VL_ATTR_اڻ_استعمال_ٿيل ثابت سائيز_t removed = mtask_معلومات(m_ap).siblings.erase(m_bp);
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
صنف ملايو_Candidate_Scoreboard final {
    // TYPES
    استعمال ڍير_2 = Pairing_ڍير<ملايو_Candidate_ڪنجي>;

    // MEMBERS
    ڍير_2 m_ڍير_2;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<ڪنارو_MC> m_ڪنارو_MCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    فراغ وجهو(ملايو_Candidate* nodep) {
        nodep->updateScore();
        m_ڍير_2.وجهو(nodep);
    }

    // Remove a candidate from the scoreboard.
    فراغ remove(ملايو_Candidate* nodep) { m_ڍير_2.remove(nodep); }

عام:
    // CONSTRUCTORS
    ملايو_Candidate_Scoreboard() = افتراضي;
    ~ملايو_Candidate_Scoreboard() = افتراضي;
    VL_UNCOPYABLE(ملايو_Candidate_Scoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    ملايو_Candidate* بهترين() ثابت { أرجع ملايو_Candidate::ڍير_ڳنڍ_منزل_Elem(m_ڍير_2.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    فراغ rescore(ملايو_Candidate* nodep) {
        remove(nodep);
        وجهو(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    فراغ جوڙيو_ڪنارو_MC(MTask_ڪنارو* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        ڪنارو_MC* ثابت ڪنارو_MCp = m_ڪنارو_MCPool.alloc(edgep);
        edgep->userp(ڪنارو_MCp);
        وجهو(ڪنارو_MCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    فراغ remove_ڪنارو_MC(MTask_ڪنارو* edgep) {
        ڪنارو_MC* ثابت ڪنارو_MCp = ڪنارو_MC_2(edgep);
        UDEBUGONLY(UASSERT(ڪنارو_MCp, "Edge has no merge candidate"););
        edgep->userp(خالي_اشارو);
        remove(ڪنارو_MCp);
        VL_DO_DANGLING(m_ڪنارو_MCPool.حرر(ڪنارو_MCp), ڪنارو_MCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    فراغ جوڙيو_Sibling_MC(LogicMTask* ap, LogicMTask* bp) { وجهو(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    فراغ removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.حرر(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    فراغ removeMC(ملايو_Candidate* nodep) {
        إذا (SiblingMC* ثابت smcp = nodep->منزل_Sibling_MC()) {
            removeSiblingMC(smcp);
        } وإلا {
            remove_ڪنارو_MC(nodep->منزل_ڪنارو_MC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
صنف Contraction final {
    // MEMBERS
    ترتيب_MTask_گراف& m_m_Task_گراف;  // The Mtask graph
    uint64_t m_score_حد_بندي;  // Critical path limit for merges
    ملايو_Candidate_Scoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTask_Contraction_معلومات[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    فراغ جوڙيو_ڪنارو_MCs(LogicMTask* mtaskp) {
        لكل (V3_گراف_ڪنارو& e : mtaskp->ٻاهريون_ڪنارا()) m_sb.جوڙيو_ڪنارو_MC(static_cast<MTask_ڪنارو*>(&e));
        لكل (V3_گراف_ڪنارو& e : mtaskp->اندريون_ڪنارا()) m_sb.جوڙيو_ڪنارو_MC(static_cast<MTask_ڪنارو*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    فراغ remove_ڪنارو_MCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        لكل (V3_گراف_ڪنارو& ڪنارو : mtaskp->ٻاهريون_ڪنارا()) {
            MTask_ڪنارو* ثابت edgep = static_cast<MTask_ڪنارو*>(&ڪنارو);
            إذا (ڪنارو_MC_2(edgep)) m_sb.remove_ڪنارو_MC(edgep);
        }
        لكل (V3_گراف_ڪنارو& ڪنارو : mtaskp->اندريون_ڪنارا()) {
            MTask_ڪنارو* ثابت edgep = static_cast<MTask_ڪنارو*>(&ڪنارو);
            إذا (ڪنارو_MC_2(edgep)) m_sb.remove_ڪنارو_MC(edgep);
        }
    }

    فراغ makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        إذا (ap->id() < bp->id()) std::مٽا_سٽا(ap, bp);
        // The higher id vertex owns the association set
        ثابت منطقي پهريون = mtask_معلومات(ap).siblings.وجهو(bp).ٻيو;
        إذا (پهريون) {
            m_sb.جوڙيو_Sibling_MC(ap, bp);
            أرجع;
        }

        إذا (VL_UNLIKELY(m_m_Task_گراف.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            منطقي مليو = خطأ;
            لكل (ثابت SiblingMC& smc : mtask_معلومات(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                إذا (smc.bp() == bp) مليو = سچ;
            }
            UASSERT_OBJ(مليو, ap, "Sibling not found");
        }
    }

    سانچو <گراف_Way::en N_Way, منطقي N_Exhaustive>
    فراغ جوڙيو_Sibling_MCs_From_Relatives(LogicMTask* mtaskp) {
        constexpr گراف_Way way{N_Way};
        // Need at least 2 edges
        خودڪار& ڪنارا = mtaskp->ڪنارا<way>();
        إذا (!ڪنارا.has_گڻو_Elements()) أرجع;

        std::array<LogicMTask*, PART_SIBLING_ڪنارو_حد_بندي> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        بنية alignas(16) ترتيب_ڏيڻ_رڪارڊ final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_ڪنارو_حد_بندي <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            منطقي عامل<(ثابت ترتيب_ڏيڻ_رڪارڊ& that) ثابت {
                أرجع m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(حجم(ترتيب_ڏيڻ_رڪارڊ) <= 16, "How could this be padded to more than 16?");

        std::array<ترتيب_ڏيڻ_رڪارڊ, PART_SIBLING_ڪنارو_حد_بندي> ترتيب_ڏيو_Recs;
        سائيز_t n = 0;

        // Populate the buffers
        لكل (V3_گراف_ڪنارو& ڪنارو : mtaskp->ڪنارا<way>()) {
            LogicMTask* ثابت otherp = static_cast<LogicMTask*>(ڪنارو.furtherp<way>());
            neighbors[n] = otherp;
            ترتيب_ڏيو_Recs[n].m_id = otherp->id();
            ترتيب_ڏيو_Recs[n].m_cp = otherp->cpInclusive<way>();
            ترتيب_ڏيو_Recs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            إذا (n >= PART_SIBLING_ڪنارو_حد_بندي) اكسر;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr سائيز_t MAX_NONEXHAUSTIVE_جوڙا = 3;

        إذا (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_جوڙا) {
            ثابت سائيز_t پڇاڙي = n & ~static_cast<سائيز_t>(1);  // Round down to even, (we want pairs)
            std::ترتيب_ڏيو(ترتيب_ڏيو_Recs.begin(), ترتيب_ڏيو_Recs.begin() + n);
            لكل (سائيز_t i = 0; i < پڇاڙي; i += 2) {
                makeSiblingMC(neighbors[ترتيب_ڏيو_Recs[i].m_idx], neighbors[ترتيب_ڏيو_Recs[i + 1].m_idx]);
            }
        } وإلا {
            constexpr سائيز_t پڇاڙي = 2 * MAX_NONEXHAUSTIVE_جوڙا;
            std::partial_ترتيب_ڏيو(ترتيب_ڏيو_Recs.begin(), ترتيب_ڏيو_Recs.begin() + پڇاڙي, ترتيب_ڏيو_Recs.begin() + n);
            لكل (سائيز_t i = 0; i < پڇاڙي; i += 2) {
                makeSiblingMC(neighbors[ترتيب_ڏيو_Recs[i].m_idx], neighbors[ترتيب_ڏيو_Recs[i + 1].m_idx]);
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
    فراغ contract(ملايو_Candidate* ملايو_Canp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* چوٽي;
        إذا (ثابت ڪنارو_MC* ثابت ڪنارو_MCp = ملايو_Canp->منزل_ڪنارو_MC()) {
            fromp = ڪنارو_MCp->edgep()->fromMTaskp();
            چوٽي = ڪنارو_MCp->edgep()->منزل_MTaskp();
        } وإلا {
            ثابت SiblingMC* ثابت sibMCp = ملايو_Canp->منزل_Sibling_MC();
            fromp = sibMCp->bp();
            چوٽي = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        إذا (fromp->لاڳت() > چوٽي->لاڳت()) {
            recipientp = fromp;
            donorp = چوٽي;
        } وإلا {
            donorp = fromp;
            recipientp = چوٽي;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(چوٽي);  // Use donorp and recipientp now instead

        // Remove all SiblingMCs that include either MTask
        removeSiblingMCs(recipientp);
        removeSiblingMCs(donorp);

        // Remove the EdgeMCs of both MTasks
        remove_ڪنارو_MCs(recipientp);
        remove_ڪنارو_MCs(donorp);

        // Merge the MTasks. This redirects all edges, updates critical paths, and deletes donorp
        m_m_Task_گراف.ملايو_MTasks(recipientp, donorp);
        VL_DANGLING(donorp);

        // Confirm we haven't botched the CP updates. This is a whole graph walk after every single
        // merge, so it is quadratic in the size of the graph, hence only under '--debug 9'.
        إذا (VL_UNLIKELY(debug() >= 9)) m_m_Task_گراف.validate();

        // Add the EdgeMCs of the merged MTask
        جوڙيو_ڪنارو_MCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        جوڙيو_Sibling_MCs_From_Relatives<گراف_Way::ابتو_2, سچ>(recipientp);
        جوڙيو_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, سچ>(recipientp);
        بنانشان ڪنارا = 0;
        لكل (V3_گراف_ڪنارو& ڪنارو : recipientp->ٻاهريون_ڪنارا()) {
            LogicMTask* ثابت postreqp = static_cast<LogicMTask*>(ڪنارو.چوٽي());
            جوڙيو_Sibling_MCs_From_Relatives<گراف_Way::ابتو_2, خطأ>(postreqp);
            ++ڪنارا;
            إذا (ڪنارا >= PART_SIBLING_ڪنارو_حد_بندي) اكسر;
        }
        ڪنارا = 0;
        لكل (V3_گراف_ڪنارو& ڪنارو : recipientp->اندريون_ڪنارا()) {
            LogicMTask* ثابت prereqp = static_cast<LogicMTask*>(ڪنارو.fromp());
            جوڙيو_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, خطأ>(prereqp);
            ++ڪنارا;
            إذا (ڪنارا >= PART_SIBLING_ڪنارو_حد_بندي) اكسر;
        }
    }

    // CONSTRUCTORS
    Contraction(ترتيب_MTask_گراف& m_Task_گراف, uint64_t cp_حد_بندي)
        : m_m_Task_گراف{m_Task_گراف}
        , m_score_حد_بندي{cp_حد_بندي} {

        // Check the graph we were given is consistent.
        m_m_Task_گراف.validate();

        // Figure out maximum number of MTasks
        ثابت uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ثابت صحيح given = v3Global.opt.ڌاڳا_Max_MTasks();
            إذا (given > 0) أرجع given;
            // Unspecified so estimate
            أرجع PART_DEFAULT_MAX_MTASKS_في_THREAD * v3Global.opt.ڌاڳا();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ثابت سائيز_t nMTasks = m_m_Task_گراف.vertices().سائيز();
            m_mtaskDatap.reset(جديد MTask_Contraction_معلومات[nMTasks]);
            سائيز_t i = 0;
            لكل (V3_گراف_راس& vtx : m_m_Task_گراف.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        لكل (V3_گراف_راس& vtx : m_m_Task_گراف.vertices()) {
            لكل (V3_گراف_ڪنارو& ڪنارو : vtx.ٻاهريون_ڪنارا())
                m_sb.جوڙيو_ڪنارو_MC(static_cast<MTask_ڪنارو*>(&ڪنارو));
            LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(&vtx);
            جوڙيو_Sibling_MCs_From_Relatives<گراف_Way::ابتو_2, سچ>(mtaskp);
            جوڙيو_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, سچ>(mtaskp);
        }

        طالما (سچ) {
            // Pick the candidate yielding the lowest local critical path.
            ملايو_Candidate* ثابت ملايو_Canp = m_sb.بهترين();
            إذا (!ملايو_Canp) اكسر;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ثابت uint64_t score = ملايو_Canp->currentScore();
            إذا (score != ملايو_Canp->score()) {
                m_sb.rescore(ملايو_Canp);
                استمر;
            }

            // Check if the critical path limit is reached.
            إذا (score > m_score_حد_بندي) {

                // If there are still too many MTasks, raise the limit and keep going
                ثابت بنانشان mtask_ڳڻپ = m_m_Task_گراف.vertices().سائيز();
                إذا (mtask_ڳڻپ > maxMTasks) {
                    m_score_حد_بندي = (m_score_حد_بندي * 120) / 100;
                    File_سٽ* ثابت flp = v3Global.rootp()->fileline();
                    إذا (!flp->warnIsOff(V3_Error_ڪوڊ::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3_Error_ڪوڊ::UNOPTTHREADS, سچ);
                    }
                    استمر;
                }

                // MTasks limit and CP limit reached. Stop.
                اكسر;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            إذا (ڪنارو_MC* ثابت ڪنارو_MCp = ملايو_Canp->منزل_ڪنارو_MC()) {
                MTask_ڪنارو* ثابت edgep = ڪنارو_MCp->edgep();
                إذا (edgep->fromp() == m_m_Task_گراف.entryp()
                    || edgep->چوٽي() == m_m_Task_گراف.exitp()) {
                    m_sb.remove_ڪنارو_MC(edgep);
                    استمر;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            إذا (ملايو_Canp->ملايو_Would_Create_چڪر(m_m_Task_گراف)) {
                m_sb.removeMC(ملايو_Canp);
                استمر;
            }

            // Merge this candidate
            contract(ملايو_Canp);
        }

        // Free all remaining merge candidates.
        طالما (ملايو_Candidate* ثابت ملايو_Canp = m_sb.بهترين()) m_sb.removeMC(ملايو_Canp);
    }

عام:
    ساكن فراغ apply(ترتيب_MTask_گراف& m_Task_گراف, uint64_t score_حد_بندي) {
        Contraction{m_Task_گراف, score_حد_بندي};
    }
};

//######################################################################
// OrderMTaskGraph entry point

فراغ ترتيب_MTask_گراف::contract(ترتيب_MTask_گراف& mtask_گراف, uint64_t score_حد_بندي) {
    Contraction::apply(mtask_گراف, score_حد_بندي);
}
