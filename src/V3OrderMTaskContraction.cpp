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

صنف ادغام_Candidate;
صنف SiblingMC;
صنف یال_MC;

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
constexpr بیعلامت PART_SIBLING_یال_حد = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr بیعلامت PART_DEFAULT_MAX_MTASKS_در_هر_THREAD = 50;

//######################################################################
// MTask utility classes

بنية ادغام_Candidate_کلید final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    منطقي عملگر<(ثابت ادغام_Candidate_کلید& other) ثابت {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        أرجع m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
استفاده ادغام_Candidate_انباشته_گره = Pairing_انباشته<ادغام_Candidate_کلید>::گره_2;

// Information associated with scoreboarding a merge candidate
صنف ادغام_Candidate VL_NOT_FINAL : عام ادغام_Candidate_انباشته_گره {
    // Only the known subclasses can create or delete one of these
    دوست صنف SiblingMC;
    دوست صنف یال_MC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ساكن constexpr uint64_t IS_SIBLING_ماسک = 1ULL << 0;
    ساكن constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    منطقي isSiblingMC() ثابت { أرجع m_کلید.m_id & IS_SIBLING_ماسک; }

    // CONSTRUCTORS
    صریح ادغام_Candidate(منطقي isSiblingMC) {
        ساكن uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_کلید.m_id = s_serial | (isSiblingMC * IS_SIBLING_ماسک);
    }
    ~ادغام_Candidate() = افتراضي;

عام:
    // METHODS
    SiblingMC* مقصد_Sibling_MC();  // Instead of cast<>/as<>
    یال_MC* مقصد_یال_MC();  // Instead of cast<>/as<>
    منطقي ادغام_Would_Create_چرخه(ترتیب_MTask_گراف& گراف);

    // The current score of this candidate, which changes as the graph is contracted
    مضمن uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ثابت { أرجع m_کلید.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    فراغ updateScore() { m_کلید.m_score = currentScore(); }

    ساكن ادغام_Candidate* انباشته_گره_مقصد_Elem(ادغام_Candidate_انباشته_گره* nodep) {
        أرجع static_cast<ادغام_Candidate*>(nodep);
    }
};

static_assert(حجم(ادغام_Candidate) == حجم(ادغام_Candidate_انباشته_گره),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
صنف SiblingMC final : عام ادغام_Candidate {
    LogicMTask* ثابت m_ap;  // The higher ID MTask
    LogicMTask* ثابت m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { أرجع m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { أرجع m_bLinks; }

عام:
    // List type to store instances of this class
    استفاده AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    استفاده BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = افتراضي;

    // METHODS
    فراغ unlinkA();
    فراغ unlinkB();

    LogicMTask* ap() ثابت { أرجع m_ap; }
    LogicMTask* bp() ثابت { أرجع m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::مقدار, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
صنف یال_MC final : عام ادغام_Candidate {
    MTask_یال* ثابت m_edgep;  // The associated edge

عام:
    // CONSTRUCTORS
    صریح یال_MC(MTask_یال* edgep)
        : ادغام_Candidate{/* isSiblingMC: */ خطأ}
        , m_edgep{edgep} {}
    ~یال_MC() = افتراضي;

    // METHODS
    MTask_یال* edgep() ثابت { أرجع m_edgep; }
};

static_assert(!std::is_polymorphic<یال_MC>::مقدار, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
بنية MTask_Contraction_داده final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_مجموعه<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
ساكن MTask_Contraction_داده& mtask_داده(ثابت LogicMTask* mtaskp) {
    أرجع *static_cast<MTask_Contraction_داده*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
ساكن یال_MC* یال_MC_2(ثابت MTask_یال* edgep) { أرجع static_cast<یال_MC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* ادغام_Candidate::مقصد_Sibling_MC() {
    أرجع isSiblingMC() ? static_cast<SiblingMC*>(هذا) : اشاره_تهی;
}

یال_MC* ادغام_Candidate::مقصد_یال_MC() { أرجع isSiblingMC() ? اشاره_تهی : static_cast<یال_MC*>(هذا); }

منطقي ادغام_Candidate::ادغام_Would_Create_چرخه(ترتیب_MTask_گراف& گراف) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    إذا (ثابت SiblingMC* ثابت sibp = مقصد_Sibling_MC()) {
        أرجع گراف.مسیر_Exists(sibp->ap(), sibp->bp(), اشاره_تهی)
               || گراف.مسیر_Exists(sibp->bp(), sibp->ap(), اشاره_تهی);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTask_یال* ثابت edgep = مقصد_یال_MC()->edgep();
    أرجع گراف.مسیر_Exists(edgep->fromMTaskp(), edgep->مقصد_MTaskp(), edgep);
}

uint64_t ادغام_Candidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    إذا (ثابت SiblingMC* ثابت sibp = مقصد_Sibling_MC()) {
        ثابت LogicMTask* ثابت ap = sibp->ap();
        ثابت LogicMTask* ثابت bp = sibp->bp();
        ثابت uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<گراف_Way::FORWARD>(), bp->cpExclusive<گراف_Way::FORWARD>());
        ثابت uint64_t mergedCpRev
            = std::max(ap->cpExclusive<گراف_Way::معکوس_2>(), bp->cpExclusive<گراف_Way::معکوس_2>());
        أرجع mergedCpRev + mergedCpFwd + ap->هزینه() + bp->هزینه();
    }

    // Edge merge
    {
        MTask_یال* ثابت edgep = مقصد_یال_MC()->edgep();
        ثابت LogicMTask* ثابت fromp = edgep->fromMTaskp();
        ثابت LogicMTask* ثابت بالاترین = edgep->مقصد_MTaskp();
        ثابت uint64_t mergedCpFwd = std::max(fromp->cpExclusive<گراف_Way::FORWARD>(),
                                              بالاترین->cpExclusiveWithout<گراف_Way::FORWARD>(edgep));
        ثابت uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<گراف_Way::معکوس_2>(edgep),
                                              بالاترین->cpExclusive<گراف_Way::معکوس_2>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        أرجع 1 + mergedCpRev + mergedCpFwd + fromp->هزینه() + بالاترین->هزینه();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : ادغام_Candidate{/* isSiblingMC: */ درست}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtask_داده(ap).siblings.شمار(bp), "Should be in sibling map"););
    mtask_داده(m_ap).aSiblingMCs.link_پشت(هذا);
    mtask_داده(m_bp).bSiblingMCs.link_پشت(هذا);
}

فراغ SiblingMC::unlinkA() {
    VL_ATTR_استفادهنشده ثابت اندازه_t removed = mtask_داده(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtask_داده(m_ap).aSiblingMCs.unlink(هذا);
}

فراغ SiblingMC::unlinkB() { mtask_داده(m_bp).bSiblingMCs.unlink(هذا); }

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
صنف ادغام_Candidate_Scoreboard final {
    // TYPES
    استفاده انباشته = Pairing_انباشته<ادغام_Candidate_کلید>;

    // MEMBERS
    انباشته m_انباشته;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<یال_MC> m_یال_MCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    فراغ درج(ادغام_Candidate* nodep) {
        nodep->updateScore();
        m_انباشته.درج(nodep);
    }

    // Remove a candidate from the scoreboard.
    فراغ remove(ادغام_Candidate* nodep) { m_انباشته.remove(nodep); }

عام:
    // CONSTRUCTORS
    ادغام_Candidate_Scoreboard() = افتراضي;
    ~ادغام_Candidate_Scoreboard() = افتراضي;
    VL_UNCOPYABLE(ادغام_Candidate_Scoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    ادغام_Candidate* بهترین() ثابت { أرجع ادغام_Candidate::انباشته_گره_مقصد_Elem(m_انباشته.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    فراغ rescore(ادغام_Candidate* nodep) {
        remove(nodep);
        درج(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    فراغ جمع_کن_یال_MC(MTask_یال* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        یال_MC* ثابت یال_MCp = m_یال_MCPool.alloc(edgep);
        edgep->userp(یال_MCp);
        درج(یال_MCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    فراغ remove_یال_MC(MTask_یال* edgep) {
        یال_MC* ثابت یال_MCp = یال_MC_2(edgep);
        UDEBUGONLY(UASSERT(یال_MCp, "Edge has no merge candidate"););
        edgep->userp(اشاره_تهی);
        remove(یال_MCp);
        VL_DO_DANGLING(m_یال_MCPool.حرر(یال_MCp), یال_MCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    فراغ جمع_کن_Sibling_MC(LogicMTask* ap, LogicMTask* bp) { درج(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    فراغ removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.حرر(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    فراغ removeMC(ادغام_Candidate* nodep) {
        إذا (SiblingMC* ثابت smcp = nodep->مقصد_Sibling_MC()) {
            removeSiblingMC(smcp);
        } وإلا {
            remove_یال_MC(nodep->مقصد_یال_MC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
صنف Contraction final {
    // MEMBERS
    ترتیب_MTask_گراف& m_m_Task_گراف;  // The Mtask graph
    uint64_t m_score_حد;  // Critical path limit for merges
    ادغام_Candidate_Scoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTask_Contraction_داده[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    فراغ جمع_کن_یال_MCs(LogicMTask* mtaskp) {
        لكل (V3_گراف_یال& e : mtaskp->بیرون_یالها()) m_sb.جمع_کن_یال_MC(static_cast<MTask_یال*>(&e));
        لكل (V3_گراف_یال& e : mtaskp->درون_یالها()) m_sb.جمع_کن_یال_MC(static_cast<MTask_یال*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    فراغ remove_یال_MCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        لكل (V3_گراف_یال& یال : mtaskp->بیرون_یالها()) {
            MTask_یال* ثابت edgep = static_cast<MTask_یال*>(&یال);
            إذا (یال_MC_2(edgep)) m_sb.remove_یال_MC(edgep);
        }
        لكل (V3_گراف_یال& یال : mtaskp->درون_یالها()) {
            MTask_یال* ثابت edgep = static_cast<MTask_یال*>(&یال);
            إذا (یال_MC_2(edgep)) m_sb.remove_یال_MC(edgep);
        }
    }

    فراغ makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        إذا (ap->id() < bp->id()) std::جابجا(ap, bp);
        // The higher id vertex owns the association set
        ثابت منطقي اولی = mtask_داده(ap).siblings.درج(bp).دومی;
        إذا (اولی) {
            m_sb.جمع_کن_Sibling_MC(ap, bp);
            أرجع;
        }

        إذا (VL_UNLIKELY(m_m_Task_گراف.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            منطقي یافت = خطأ;
            لكل (ثابت SiblingMC& smc : mtask_داده(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                إذا (smc.bp() == bp) یافت = درست;
            }
            UASSERT_OBJ(یافت, ap, "Sibling not found");
        }
    }

    قالب <گراف_Way::en N_Way, منطقي N_Exhaustive>
    فراغ جمع_کن_Sibling_MCs_From_Relatives(LogicMTask* mtaskp) {
        constexpr گراف_Way way{N_Way};
        // Need at least 2 edges
        خودکار& یالها = mtaskp->یالها<way>();
        إذا (!یالها.has_مضرب_Elements()) أرجع;

        std::array<LogicMTask*, PART_SIBLING_یال_حد> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        بنية alignas(16) ترتیبدهی_رکورد final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_یال_حد <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            منطقي عملگر<(ثابت ترتیبدهی_رکورد& that) ثابت {
                أرجع m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(حجم(ترتیبدهی_رکورد) <= 16, "How could this be padded to more than 16?");

        std::array<ترتیبدهی_رکورد, PART_SIBLING_یال_حد> مرتب_سازی_Recs;
        اندازه_t n = 0;

        // Populate the buffers
        لكل (V3_گراف_یال& یال : mtaskp->یالها<way>()) {
            LogicMTask* ثابت otherp = static_cast<LogicMTask*>(یال.furtherp<way>());
            neighbors[n] = otherp;
            مرتب_سازی_Recs[n].m_id = otherp->id();
            مرتب_سازی_Recs[n].m_cp = otherp->cpInclusive<way>();
            مرتب_سازی_Recs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            إذا (n >= PART_SIBLING_یال_حد) اكسر;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr اندازه_t MAX_NONEXHAUSTIVE_جفتها = 3;

        إذا (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_جفتها) {
            ثابت اندازه_t انتها = n & ~static_cast<اندازه_t>(1);  // Round down to even, (we want pairs)
            std::مرتب_سازی(مرتب_سازی_Recs.begin(), مرتب_سازی_Recs.begin() + n);
            لكل (اندازه_t i = 0; i < انتها; i += 2) {
                makeSiblingMC(neighbors[مرتب_سازی_Recs[i].m_idx], neighbors[مرتب_سازی_Recs[i + 1].m_idx]);
            }
        } وإلا {
            constexpr اندازه_t انتها = 2 * MAX_NONEXHAUSTIVE_جفتها;
            std::partial_مرتب_سازی(مرتب_سازی_Recs.begin(), مرتب_سازی_Recs.begin() + انتها, مرتب_سازی_Recs.begin() + n);
            لكل (اندازه_t i = 0; i < انتها; i += 2) {
                makeSiblingMC(neighbors[مرتب_سازی_Recs[i].m_idx], neighbors[مرتب_سازی_Recs[i + 1].m_idx]);
            }
        }
    }

    فراغ removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        طالما (SiblingMC* ثابت smcp = mtask_داده(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        طالما (SiblingMC* ثابت smcp = mtask_داده(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    فراغ contract(ادغام_Candidate* ادغام_Canp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* بالاترین;
        إذا (ثابت یال_MC* ثابت یال_MCp = ادغام_Canp->مقصد_یال_MC()) {
            fromp = یال_MCp->edgep()->fromMTaskp();
            بالاترین = یال_MCp->edgep()->مقصد_MTaskp();
        } وإلا {
            ثابت SiblingMC* ثابت sibMCp = ادغام_Canp->مقصد_Sibling_MC();
            fromp = sibMCp->bp();
            بالاترین = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        إذا (fromp->هزینه() > بالاترین->هزینه()) {
            recipientp = fromp;
            donorp = بالاترین;
        } وإلا {
            donorp = fromp;
            recipientp = بالاترین;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(بالاترین);  // Use donorp and recipientp now instead

        // Remove all SiblingMCs that include either MTask
        removeSiblingMCs(recipientp);
        removeSiblingMCs(donorp);

        // Remove the EdgeMCs of both MTasks
        remove_یال_MCs(recipientp);
        remove_یال_MCs(donorp);

        // Merge the MTasks. This redirects all edges, updates critical paths, and deletes donorp
        m_m_Task_گراف.ادغام_MTasks(recipientp, donorp);
        VL_DANGLING(donorp);

        // Confirm we haven't botched the CP updates. This is a whole graph walk after every single
        // merge, so it is quadratic in the size of the graph, hence only under '--debug 9'.
        إذا (VL_UNLIKELY(debug() >= 9)) m_m_Task_گراف.validate();

        // Add the EdgeMCs of the merged MTask
        جمع_کن_یال_MCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        جمع_کن_Sibling_MCs_From_Relatives<گراف_Way::معکوس_2, درست>(recipientp);
        جمع_کن_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, درست>(recipientp);
        بیعلامت یالها = 0;
        لكل (V3_گراف_یال& یال : recipientp->بیرون_یالها()) {
            LogicMTask* ثابت postreqp = static_cast<LogicMTask*>(یال.بالاترین());
            جمع_کن_Sibling_MCs_From_Relatives<گراف_Way::معکوس_2, خطأ>(postreqp);
            ++یالها;
            إذا (یالها >= PART_SIBLING_یال_حد) اكسر;
        }
        یالها = 0;
        لكل (V3_گراف_یال& یال : recipientp->درون_یالها()) {
            LogicMTask* ثابت prereqp = static_cast<LogicMTask*>(یال.fromp());
            جمع_کن_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, خطأ>(prereqp);
            ++یالها;
            إذا (یالها >= PART_SIBLING_یال_حد) اكسر;
        }
    }

    // CONSTRUCTORS
    Contraction(ترتیب_MTask_گراف& m_Task_گراف, uint64_t cp_حد)
        : m_m_Task_گراف{m_Task_گراف}
        , m_score_حد{cp_حد} {

        // Check the graph we were given is consistent.
        m_m_Task_گراف.validate();

        // Figure out maximum number of MTasks
        ثابت uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ثابت صحيح given = v3Global.opt.نخها_Max_MTasks();
            إذا (given > 0) أرجع given;
            // Unspecified so estimate
            أرجع PART_DEFAULT_MAX_MTASKS_در_هر_THREAD * v3Global.opt.نخها();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ثابت اندازه_t nMTasks = m_m_Task_گراف.vertices().اندازه();
            m_mtaskDatap.reset(جديد MTask_Contraction_داده[nMTasks]);
            اندازه_t i = 0;
            لكل (V3_گراف_رأس& vtx : m_m_Task_گراف.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        لكل (V3_گراف_رأس& vtx : m_m_Task_گراف.vertices()) {
            لكل (V3_گراف_یال& یال : vtx.بیرون_یالها())
                m_sb.جمع_کن_یال_MC(static_cast<MTask_یال*>(&یال));
            LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(&vtx);
            جمع_کن_Sibling_MCs_From_Relatives<گراف_Way::معکوس_2, درست>(mtaskp);
            جمع_کن_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, درست>(mtaskp);
        }

        طالما (درست) {
            // Pick the candidate yielding the lowest local critical path.
            ادغام_Candidate* ثابت ادغام_Canp = m_sb.بهترین();
            إذا (!ادغام_Canp) اكسر;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ثابت uint64_t score = ادغام_Canp->currentScore();
            إذا (score != ادغام_Canp->score()) {
                m_sb.rescore(ادغام_Canp);
                استمر;
            }

            // Check if the critical path limit is reached.
            إذا (score > m_score_حد) {

                // If there are still too many MTasks, raise the limit and keep going
                ثابت بیعلامت mtask_شمار = m_m_Task_گراف.vertices().اندازه();
                إذا (mtask_شمار > maxMTasks) {
                    m_score_حد = (m_score_حد * 120) / 100;
                    File_سطر* ثابت flp = v3Global.rootp()->fileline();
                    إذا (!flp->warnIsOff(V3_Error_کد::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3_Error_کد::UNOPTTHREADS, درست);
                    }
                    استمر;
                }

                // MTasks limit and CP limit reached. Stop.
                اكسر;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            إذا (یال_MC* ثابت یال_MCp = ادغام_Canp->مقصد_یال_MC()) {
                MTask_یال* ثابت edgep = یال_MCp->edgep();
                إذا (edgep->fromp() == m_m_Task_گراف.entryp()
                    || edgep->بالاترین() == m_m_Task_گراف.exitp()) {
                    m_sb.remove_یال_MC(edgep);
                    استمر;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            إذا (ادغام_Canp->ادغام_Would_Create_چرخه(m_m_Task_گراف)) {
                m_sb.removeMC(ادغام_Canp);
                استمر;
            }

            // Merge this candidate
            contract(ادغام_Canp);
        }

        // Free all remaining merge candidates.
        طالما (ادغام_Candidate* ثابت ادغام_Canp = m_sb.بهترین()) m_sb.removeMC(ادغام_Canp);
    }

عام:
    ساكن فراغ apply(ترتیب_MTask_گراف& m_Task_گراف, uint64_t score_حد) {
        Contraction{m_Task_گراف, score_حد};
    }
};

//######################################################################
// OrderMTaskGraph entry point

فراغ ترتیب_MTask_گراف::contract(ترتیب_MTask_گراف& mtask_گراف, uint64_t score_حد) {
    Contraction::apply(mtask_گراف, score_حد);
}
