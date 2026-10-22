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

صنف ملاؤ_Candidate;
صنف SiblingMC;
صنف کنارا_MC;

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
constexpr بنانشانی PART_SIBLING_کنارا_حد_بندی = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr بنانشانی PART_DEFAULT_MAX_MTASKS_فی_THREAD = 50;

//######################################################################
// MTask utility classes

بنية ملاؤ_Candidate_کلید final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    منطقي عامل<(ثابت ملاؤ_Candidate_کلید& other) ثابت {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        أرجع m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
ورتوں ملاؤ_Candidate_ڈھیر_گرہ = Pairing_ڈھیر<ملاؤ_Candidate_کلید>::گرہ_2;

// Information associated with scoreboarding a merge candidate
صنف ملاؤ_Candidate VL_NOT_FINAL : عام ملاؤ_Candidate_ڈھیر_گرہ {
    // Only the known subclasses can create or delete one of these
    یار صنف SiblingMC;
    یار صنف کنارا_MC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ساكن constexpr uint64_t IS_SIBLING_نقاب = 1ULL << 0;
    ساكن constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    منطقي isSiblingMC() ثابت { أرجع m_کلید.m_id & IS_SIBLING_نقاب; }

    // CONSTRUCTORS
    واضح ملاؤ_Candidate(منطقي isSiblingMC) {
        ساكن uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_کلید.m_id = s_serial | (isSiblingMC * IS_SIBLING_نقاب);
    }
    ~ملاؤ_Candidate() = افتراضي;

عام:
    // METHODS
    SiblingMC* منزل_Sibling_MC();  // Instead of cast<>/as<>
    کنارا_MC* منزل_کنارا_MC();  // Instead of cast<>/as<>
    منطقي ملاؤ_Would_Create_چکر(ترتیب_MTask_گراف& گراف);

    // The current score of this candidate, which changes as the graph is contracted
    مضمن uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ثابت { أرجع m_کلید.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    فراغ updateScore() { m_کلید.m_score = currentScore(); }

    ساكن ملاؤ_Candidate* ڈھیر_گرہ_منزل_Elem(ملاؤ_Candidate_ڈھیر_گرہ* nodep) {
        أرجع static_cast<ملاؤ_Candidate*>(nodep);
    }
};

static_assert(حجم(ملاؤ_Candidate) == حجم(ملاؤ_Candidate_ڈھیر_گرہ),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
صنف SiblingMC final : عام ملاؤ_Candidate {
    LogicMTask* ثابت m_ap;  // The higher ID MTask
    LogicMTask* ثابت m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { أرجع m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { أرجع m_bLinks; }

عام:
    // List type to store instances of this class
    ورتوں AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    ورتوں BList = V3List<SiblingMC, &SiblingMC::bLinks>;

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
صنف کنارا_MC final : عام ملاؤ_Candidate {
    MTask_کنارا* ثابت m_edgep;  // The associated edge

عام:
    // CONSTRUCTORS
    واضح کنارا_MC(MTask_کنارا* edgep)
        : ملاؤ_Candidate{/* isSiblingMC: */ خطأ}
        , m_edgep{edgep} {}
    ~کنارا_MC() = افتراضي;

    // METHODS
    MTask_کنارا* edgep() ثابت { أرجع m_edgep; }
};

static_assert(!std::is_polymorphic<کنارا_MC>::قدر, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
بنية MTask_Contraction_انکڑے final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_سیٹ<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
ساكن MTask_Contraction_انکڑے& mtask_انکڑے(ثابت LogicMTask* mtaskp) {
    أرجع *static_cast<MTask_Contraction_انکڑے*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
ساكن کنارا_MC* کنارا_MC_2(ثابت MTask_کنارا* edgep) { أرجع static_cast<کنارا_MC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* ملاؤ_Candidate::منزل_Sibling_MC() {
    أرجع isSiblingMC() ? static_cast<SiblingMC*>(هذا) : خالی_سوچک;
}

کنارا_MC* ملاؤ_Candidate::منزل_کنارا_MC() { أرجع isSiblingMC() ? خالی_سوچک : static_cast<کنارا_MC*>(هذا); }

منطقي ملاؤ_Candidate::ملاؤ_Would_Create_چکر(ترتیب_MTask_گراف& گراف) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    إذا (ثابت SiblingMC* ثابت sibp = منزل_Sibling_MC()) {
        أرجع گراف.رستہ_Exists(sibp->ap(), sibp->bp(), خالی_سوچک)
               || گراف.رستہ_Exists(sibp->bp(), sibp->ap(), خالی_سوچک);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTask_کنارا* ثابت edgep = منزل_کنارا_MC()->edgep();
    أرجع گراف.رستہ_Exists(edgep->fromMTaskp(), edgep->منزل_MTaskp(), edgep);
}

uint64_t ملاؤ_Candidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    إذا (ثابت SiblingMC* ثابت sibp = منزل_Sibling_MC()) {
        ثابت LogicMTask* ثابت ap = sibp->ap();
        ثابت LogicMTask* ثابت bp = sibp->bp();
        ثابت uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<گراف_Way::FORWARD>(), bp->cpExclusive<گراف_Way::FORWARD>());
        ثابت uint64_t mergedCpRev
            = std::max(ap->cpExclusive<گراف_Way::الٹ_2>(), bp->cpExclusive<گراف_Way::الٹ_2>());
        أرجع mergedCpRev + mergedCpFwd + ap->لاگت() + bp->لاگت();
    }

    // Edge merge
    {
        MTask_کنارا* ثابت edgep = منزل_کنارا_MC()->edgep();
        ثابت LogicMTask* ثابت fromp = edgep->fromMTaskp();
        ثابت LogicMTask* ثابت چوٹی = edgep->منزل_MTaskp();
        ثابت uint64_t mergedCpFwd = std::max(fromp->cpExclusive<گراف_Way::FORWARD>(),
                                              چوٹی->cpExclusiveWithout<گراف_Way::FORWARD>(edgep));
        ثابت uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<گراف_Way::الٹ_2>(edgep),
                                              چوٹی->cpExclusive<گراف_Way::الٹ_2>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        أرجع 1 + mergedCpRev + mergedCpFwd + fromp->لاگت() + چوٹی->لاگت();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : ملاؤ_Candidate{/* isSiblingMC: */ سچ}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtask_انکڑے(ap).siblings.گنتی(bp), "Should be in sibling map"););
    mtask_انکڑے(m_ap).aSiblingMCs.link_پٹھ(هذا);
    mtask_انکڑے(m_bp).bSiblingMCs.link_پٹھ(هذا);
}

فراغ SiblingMC::unlinkA() {
    VL_ATTR_بن_ورتیا ثابت ناپ_t removed = mtask_انکڑے(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtask_انکڑے(m_ap).aSiblingMCs.unlink(هذا);
}

فراغ SiblingMC::unlinkB() { mtask_انکڑے(m_bp).bSiblingMCs.unlink(هذا); }

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
صنف ملاؤ_Candidate_Scoreboard final {
    // TYPES
    ورتوں ڈھیر_2 = Pairing_ڈھیر<ملاؤ_Candidate_کلید>;

    // MEMBERS
    ڈھیر_2 m_ڈھیر_2;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<کنارا_MC> m_کنارا_MCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    فراغ داخل_کرو(ملاؤ_Candidate* nodep) {
        nodep->updateScore();
        m_ڈھیر_2.داخل_کرو(nodep);
    }

    // Remove a candidate from the scoreboard.
    فراغ remove(ملاؤ_Candidate* nodep) { m_ڈھیر_2.remove(nodep); }

عام:
    // CONSTRUCTORS
    ملاؤ_Candidate_Scoreboard() = افتراضي;
    ~ملاؤ_Candidate_Scoreboard() = افتراضي;
    VL_UNCOPYABLE(ملاؤ_Candidate_Scoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    ملاؤ_Candidate* بہترین() ثابت { أرجع ملاؤ_Candidate::ڈھیر_گرہ_منزل_Elem(m_ڈھیر_2.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    فراغ rescore(ملاؤ_Candidate* nodep) {
        remove(nodep);
        داخل_کرو(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    فراغ جوڑو_کنارا_MC(MTask_کنارا* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        کنارا_MC* ثابت کنارا_MCp = m_کنارا_MCPool.alloc(edgep);
        edgep->userp(کنارا_MCp);
        داخل_کرو(کنارا_MCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    فراغ remove_کنارا_MC(MTask_کنارا* edgep) {
        کنارا_MC* ثابت کنارا_MCp = کنارا_MC_2(edgep);
        UDEBUGONLY(UASSERT(کنارا_MCp, "Edge has no merge candidate"););
        edgep->userp(خالی_سوچک);
        remove(کنارا_MCp);
        VL_DO_DANGLING(m_کنارا_MCPool.حرر(کنارا_MCp), کنارا_MCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    فراغ جوڑو_Sibling_MC(LogicMTask* ap, LogicMTask* bp) { داخل_کرو(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    فراغ removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.حرر(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    فراغ removeMC(ملاؤ_Candidate* nodep) {
        إذا (SiblingMC* ثابت smcp = nodep->منزل_Sibling_MC()) {
            removeSiblingMC(smcp);
        } وإلا {
            remove_کنارا_MC(nodep->منزل_کنارا_MC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
صنف Contraction final {
    // MEMBERS
    ترتیب_MTask_گراف& m_m_Task_گراف;  // The Mtask graph
    uint64_t m_score_حد_بندی;  // Critical path limit for merges
    ملاؤ_Candidate_Scoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTask_Contraction_انکڑے[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    فراغ جوڑو_کنارا_MCs(LogicMTask* mtaskp) {
        لكل (V3_گراف_کنارا& e : mtaskp->باہرلا_کنارے()) m_sb.جوڑو_کنارا_MC(static_cast<MTask_کنارا*>(&e));
        لكل (V3_گراف_کنارا& e : mtaskp->وچ_کنارے()) m_sb.جوڑو_کنارا_MC(static_cast<MTask_کنارا*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    فراغ remove_کنارا_MCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        لكل (V3_گراف_کنارا& کنارا : mtaskp->باہرلا_کنارے()) {
            MTask_کنارا* ثابت edgep = static_cast<MTask_کنارا*>(&کنارا);
            إذا (کنارا_MC_2(edgep)) m_sb.remove_کنارا_MC(edgep);
        }
        لكل (V3_گراف_کنارا& کنارا : mtaskp->وچ_کنارے()) {
            MTask_کنارا* ثابت edgep = static_cast<MTask_کنارا*>(&کنارا);
            إذا (کنارا_MC_2(edgep)) m_sb.remove_کنارا_MC(edgep);
        }
    }

    فراغ makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        إذا (ap->id() < bp->id()) std::وٹاندرا(ap, bp);
        // The higher id vertex owns the association set
        ثابت منطقي پہلا = mtask_انکڑے(ap).siblings.داخل_کرو(bp).دوجا;
        إذا (پہلا) {
            m_sb.جوڑو_Sibling_MC(ap, bp);
            أرجع;
        }

        إذا (VL_UNLIKELY(m_m_Task_گراف.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            منطقي لبھیا = خطأ;
            لكل (ثابت SiblingMC& smc : mtask_انکڑے(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                إذا (smc.bp() == bp) لبھیا = سچ;
            }
            UASSERT_OBJ(لبھیا, ap, "Sibling not found");
        }
    }

    سانچا <گراف_Way::en N_Way, منطقي N_Exhaustive>
    فراغ جوڑو_Sibling_MCs_From_Relatives(LogicMTask* mtaskp) {
        constexpr گراف_Way way{N_Way};
        // Need at least 2 edges
        خودکار& کنارے = mtaskp->کنارے<way>();
        إذا (!کنارے.has_مضرب_Elements()) أرجع;

        std::array<LogicMTask*, PART_SIBLING_کنارا_حد_بندی> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        بنية alignas(16) چھانٹی_ریکارڈ final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_کنارا_حد_بندی <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            منطقي عامل<(ثابت چھانٹی_ریکارڈ& that) ثابت {
                أرجع m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(حجم(چھانٹی_ریکارڈ) <= 16, "How could this be padded to more than 16?");

        std::array<چھانٹی_ریکارڈ, PART_SIBLING_کنارا_حد_بندی> ترتیب_دیو_Recs;
        ناپ_t n = 0;

        // Populate the buffers
        لكل (V3_گراف_کنارا& کنارا : mtaskp->کنارے<way>()) {
            LogicMTask* ثابت otherp = static_cast<LogicMTask*>(کنارا.furtherp<way>());
            neighbors[n] = otherp;
            ترتیب_دیو_Recs[n].m_id = otherp->id();
            ترتیب_دیو_Recs[n].m_cp = otherp->cpInclusive<way>();
            ترتیب_دیو_Recs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            إذا (n >= PART_SIBLING_کنارا_حد_بندی) اكسر;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr ناپ_t MAX_NONEXHAUSTIVE_جوڑے = 3;

        إذا (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_جوڑے) {
            ثابت ناپ_t سرا = n & ~static_cast<ناپ_t>(1);  // Round down to even, (we want pairs)
            std::ترتیب_دیو(ترتیب_دیو_Recs.begin(), ترتیب_دیو_Recs.begin() + n);
            لكل (ناپ_t i = 0; i < سرا; i += 2) {
                makeSiblingMC(neighbors[ترتیب_دیو_Recs[i].m_idx], neighbors[ترتیب_دیو_Recs[i + 1].m_idx]);
            }
        } وإلا {
            constexpr ناپ_t سرا = 2 * MAX_NONEXHAUSTIVE_جوڑے;
            std::partial_ترتیب_دیو(ترتیب_دیو_Recs.begin(), ترتیب_دیو_Recs.begin() + سرا, ترتیب_دیو_Recs.begin() + n);
            لكل (ناپ_t i = 0; i < سرا; i += 2) {
                makeSiblingMC(neighbors[ترتیب_دیو_Recs[i].m_idx], neighbors[ترتیب_دیو_Recs[i + 1].m_idx]);
            }
        }
    }

    فراغ removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        طالما (SiblingMC* ثابت smcp = mtask_انکڑے(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        طالما (SiblingMC* ثابت smcp = mtask_انکڑے(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    فراغ contract(ملاؤ_Candidate* ملاؤ_Canp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* چوٹی;
        إذا (ثابت کنارا_MC* ثابت کنارا_MCp = ملاؤ_Canp->منزل_کنارا_MC()) {
            fromp = کنارا_MCp->edgep()->fromMTaskp();
            چوٹی = کنارا_MCp->edgep()->منزل_MTaskp();
        } وإلا {
            ثابت SiblingMC* ثابت sibMCp = ملاؤ_Canp->منزل_Sibling_MC();
            fromp = sibMCp->bp();
            چوٹی = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        إذا (fromp->لاگت() > چوٹی->لاگت()) {
            recipientp = fromp;
            donorp = چوٹی;
        } وإلا {
            donorp = fromp;
            recipientp = چوٹی;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(چوٹی);  // Use donorp and recipientp now instead

        // Remove all SiblingMCs that include either MTask
        removeSiblingMCs(recipientp);
        removeSiblingMCs(donorp);

        // Remove the EdgeMCs of both MTasks
        remove_کنارا_MCs(recipientp);
        remove_کنارا_MCs(donorp);

        // Merge the MTasks. This redirects all edges, updates critical paths, and deletes donorp
        m_m_Task_گراف.ملاؤ_MTasks(recipientp, donorp);
        VL_DANGLING(donorp);

        // Confirm we haven't botched the CP updates. This is a whole graph walk after every single
        // merge, so it is quadratic in the size of the graph, hence only under '--debug 9'.
        إذا (VL_UNLIKELY(debug() >= 9)) m_m_Task_گراف.validate();

        // Add the EdgeMCs of the merged MTask
        جوڑو_کنارا_MCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        جوڑو_Sibling_MCs_From_Relatives<گراف_Way::الٹ_2, سچ>(recipientp);
        جوڑو_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, سچ>(recipientp);
        بنانشانی کنارے = 0;
        لكل (V3_گراف_کنارا& کنارا : recipientp->باہرلا_کنارے()) {
            LogicMTask* ثابت postreqp = static_cast<LogicMTask*>(کنارا.چوٹی());
            جوڑو_Sibling_MCs_From_Relatives<گراف_Way::الٹ_2, خطأ>(postreqp);
            ++کنارے;
            إذا (کنارے >= PART_SIBLING_کنارا_حد_بندی) اكسر;
        }
        کنارے = 0;
        لكل (V3_گراف_کنارا& کنارا : recipientp->وچ_کنارے()) {
            LogicMTask* ثابت prereqp = static_cast<LogicMTask*>(کنارا.fromp());
            جوڑو_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, خطأ>(prereqp);
            ++کنارے;
            إذا (کنارے >= PART_SIBLING_کنارا_حد_بندی) اكسر;
        }
    }

    // CONSTRUCTORS
    Contraction(ترتیب_MTask_گراف& m_Task_گراف, uint64_t cp_حد_بندی)
        : m_m_Task_گراف{m_Task_گراف}
        , m_score_حد_بندی{cp_حد_بندی} {

        // Check the graph we were given is consistent.
        m_m_Task_گراف.validate();

        // Figure out maximum number of MTasks
        ثابت uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            ثابت صحيح given = v3Global.opt.دھاگے_Max_MTasks();
            إذا (given > 0) أرجع given;
            // Unspecified so estimate
            أرجع PART_DEFAULT_MAX_MTASKS_فی_THREAD * v3Global.opt.دھاگے();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ثابت ناپ_t nMTasks = m_m_Task_گراف.vertices().ناپ();
            m_mtaskDatap.reset(جديد MTask_Contraction_انکڑے[nMTasks]);
            ناپ_t i = 0;
            لكل (V3_گراف_راس& vtx : m_m_Task_گراف.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        لكل (V3_گراف_راس& vtx : m_m_Task_گراف.vertices()) {
            لكل (V3_گراف_کنارا& کنارا : vtx.باہرلا_کنارے())
                m_sb.جوڑو_کنارا_MC(static_cast<MTask_کنارا*>(&کنارا));
            LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(&vtx);
            جوڑو_Sibling_MCs_From_Relatives<گراف_Way::الٹ_2, سچ>(mtaskp);
            جوڑو_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, سچ>(mtaskp);
        }

        طالما (سچ) {
            // Pick the candidate yielding the lowest local critical path.
            ملاؤ_Candidate* ثابت ملاؤ_Canp = m_sb.بہترین();
            إذا (!ملاؤ_Canp) اكسر;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ثابت uint64_t score = ملاؤ_Canp->currentScore();
            إذا (score != ملاؤ_Canp->score()) {
                m_sb.rescore(ملاؤ_Canp);
                استمر;
            }

            // Check if the critical path limit is reached.
            إذا (score > m_score_حد_بندی) {

                // If there are still too many MTasks, raise the limit and keep going
                ثابت بنانشانی mtask_گنتی = m_m_Task_گراف.vertices().ناپ();
                إذا (mtask_گنتی > maxMTasks) {
                    m_score_حد_بندی = (m_score_حد_بندی * 120) / 100;
                    File_سطر* ثابت flp = v3Global.rootp()->fileline();
                    إذا (!flp->warnIsOff(V3_Error_کوڈ::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3_Error_کوڈ::UNOPTTHREADS, سچ);
                    }
                    استمر;
                }

                // MTasks limit and CP limit reached. Stop.
                اكسر;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            إذا (کنارا_MC* ثابت کنارا_MCp = ملاؤ_Canp->منزل_کنارا_MC()) {
                MTask_کنارا* ثابت edgep = کنارا_MCp->edgep();
                إذا (edgep->fromp() == m_m_Task_گراف.entryp()
                    || edgep->چوٹی() == m_m_Task_گراف.exitp()) {
                    m_sb.remove_کنارا_MC(edgep);
                    استمر;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            إذا (ملاؤ_Canp->ملاؤ_Would_Create_چکر(m_m_Task_گراف)) {
                m_sb.removeMC(ملاؤ_Canp);
                استمر;
            }

            // Merge this candidate
            contract(ملاؤ_Canp);
        }

        // Free all remaining merge candidates.
        طالما (ملاؤ_Candidate* ثابت ملاؤ_Canp = m_sb.بہترین()) m_sb.removeMC(ملاؤ_Canp);
    }

عام:
    ساكن فراغ apply(ترتیب_MTask_گراف& m_Task_گراف, uint64_t score_حد_بندی) {
        Contraction{m_Task_گراف, score_حد_بندی};
    }
};

//######################################################################
// OrderMTaskGraph entry point

فراغ ترتیب_MTask_گراف::contract(ترتیب_MTask_گراف& mtask_گراف, uint64_t score_حد_بندی) {
    Contraction::apply(mtask_گراف, score_حد_بندی);
}
