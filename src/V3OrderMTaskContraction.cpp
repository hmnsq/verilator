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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Global.h"
#شامل "V3Graph.h"
#شامل "V3OrderMTaskGraph.h"
#شامل "V3PairingHeap.h"
#شامل "V3PoolAllocator.h"

#شامل <algorithm>
#شامل <array>
#شامل <limits>
#شامل <memory>
#شامل <type_traits>
#شامل <unordered_set>
#شامل <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت ملاؤ_Candidate;
جماعت SiblingMC;
جماعت کنارہ_MC;

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
constexpr غیرعلامتی PART_SIBLING_کنارہ_حد_بندی = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr غیرعلامتی PART_DEFAULT_MAX_MTASKS_فی_THREAD = 50;

//######################################################################
// MTask utility classes

ساخت ملاؤ_Candidate_کلید final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    منطقی عامل<(ثابت ملاؤ_Candidate_کلید& other) ثابت {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        واپس m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
استعمال ملاؤ_Candidate_ڈھیر_گرہ = Pairing_ڈھیر<ملاؤ_Candidate_کلید>::گرہ_2;

// Information associated with scoreboarding a merge candidate
جماعت ملاؤ_Candidate VL_NOT_FINAL : عوامی ملاؤ_Candidate_ڈھیر_گرہ {
    // Only the known subclasses can create or delete one of these
    دوست جماعت SiblingMC;
    دوست جماعت کنارہ_MC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    ساکن constexpr uint64_t IS_SIBLING_نقاب = 1ULL << 0;
    ساکن constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    منطقی isSiblingMC() ثابت { واپس m_کلید.m_id & IS_SIBLING_نقاب; }

    // CONSTRUCTORS
    صریح ملاؤ_Candidate(منطقی isSiblingMC) {
        ساکن uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_کلید.m_id = s_serial | (isSiblingMC * IS_SIBLING_نقاب);
    }
    ~ملاؤ_Candidate() = طےشدہ;

عوامی:
    // METHODS
    SiblingMC* مقصد_Sibling_MC();  // Instead of cast<>/as<>
    کنارہ_MC* مقصد_کنارہ_MC();  // Instead of cast<>/as<>
    منطقی ملاؤ_Would_Create_چکر(ترتیب_MTask_گراف& گراف);

    // The current score of this candidate, which changes as the graph is contracted
    اندرونی uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() ثابت { واپس m_کلید.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    خالی updateScore() { m_کلید.m_score = currentScore(); }

    ساکن ملاؤ_Candidate* ڈھیر_گرہ_مقصد_Elem(ملاؤ_Candidate_ڈھیر_گرہ* nodep) {
        واپس static_cast<ملاؤ_Candidate*>(nodep);
    }
};

static_assert(حجم(ملاؤ_Candidate) == حجم(ملاؤ_Candidate_ڈھیر_گرہ),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
جماعت SiblingMC final : عوامی ملاؤ_Candidate {
    LogicMTask* ثابت m_ap;  // The higher ID MTask
    LogicMTask* ثابت m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { واپس m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { واپس m_bLinks; }

عوامی:
    // List type to store instances of this class
    استعمال AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    استعمال BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = طےشدہ;

    // METHODS
    خالی unlinkA();
    خالی unlinkB();

    LogicMTask* ap() ثابت { واپس m_ap; }
    LogicMTask* bp() ثابت { واپس m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::قدر, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
جماعت کنارہ_MC final : عوامی ملاؤ_Candidate {
    MTask_کنارہ* ثابت m_edgep;  // The associated edge

عوامی:
    // CONSTRUCTORS
    صریح کنارہ_MC(MTask_کنارہ* edgep)
        : ملاؤ_Candidate{/* isSiblingMC: */ جھوٹ}
        , m_edgep{edgep} {}
    ~کنارہ_MC() = طےشدہ;

    // METHODS
    MTask_کنارہ* edgep() ثابت { واپس m_edgep; }
};

static_assert(!std::is_polymorphic<کنارہ_MC>::قدر, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
ساخت MTask_Contraction_ڈیٹا final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_سیٹ<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
ساکن MTask_Contraction_ڈیٹا& mtask_ڈیٹا(ثابت LogicMTask* mtaskp) {
    واپس *static_cast<MTask_Contraction_ڈیٹا*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
ساکن کنارہ_MC* کنارہ_MC_2(ثابت MTask_کنارہ* edgep) { واپس static_cast<کنارہ_MC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* ملاؤ_Candidate::مقصد_Sibling_MC() {
    واپس isSiblingMC() ? static_cast<SiblingMC*>(یہ) : خالی_اشاریہ;
}

کنارہ_MC* ملاؤ_Candidate::مقصد_کنارہ_MC() { واپس isSiblingMC() ? خالی_اشاریہ : static_cast<کنارہ_MC*>(یہ); }

منطقی ملاؤ_Candidate::ملاؤ_Would_Create_چکر(ترتیب_MTask_گراف& گراف) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    اگر (ثابت SiblingMC* ثابت sibp = مقصد_Sibling_MC()) {
        واپس گراف.راستہ_Exists(sibp->ap(), sibp->bp(), خالی_اشاریہ)
               || گراف.راستہ_Exists(sibp->bp(), sibp->ap(), خالی_اشاریہ);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTask_کنارہ* ثابت edgep = مقصد_کنارہ_MC()->edgep();
    واپس گراف.راستہ_Exists(edgep->fromMTaskp(), edgep->مقصد_MTaskp(), edgep);
}

uint64_t ملاؤ_Candidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    اگر (ثابت SiblingMC* ثابت sibp = مقصد_Sibling_MC()) {
        ثابت LogicMTask* ثابت ap = sibp->ap();
        ثابت LogicMTask* ثابت bp = sibp->bp();
        ثابت uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<گراف_Way::FORWARD>(), bp->cpExclusive<گراف_Way::FORWARD>());
        ثابت uint64_t mergedCpRev
            = std::max(ap->cpExclusive<گراف_Way::الٹ_2>(), bp->cpExclusive<گراف_Way::الٹ_2>());
        واپس mergedCpRev + mergedCpFwd + ap->لاگت() + bp->لاگت();
    }

    // Edge merge
    {
        MTask_کنارہ* ثابت edgep = مقصد_کنارہ_MC()->edgep();
        ثابت LogicMTask* ثابت fromp = edgep->fromMTaskp();
        ثابت LogicMTask* ثابت چوٹی = edgep->مقصد_MTaskp();
        ثابت uint64_t mergedCpFwd = std::max(fromp->cpExclusive<گراف_Way::FORWARD>(),
                                              چوٹی->cpExclusiveWithout<گراف_Way::FORWARD>(edgep));
        ثابت uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<گراف_Way::الٹ_2>(edgep),
                                              چوٹی->cpExclusive<گراف_Way::الٹ_2>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        واپس 1 + mergedCpRev + mergedCpFwd + fromp->لاگت() + چوٹی->لاگت();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : ملاؤ_Candidate{/* isSiblingMC: */ سچ}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtask_ڈیٹا(ap).siblings.گنتی(bp), "Should be in sibling map"););
    mtask_ڈیٹا(m_ap).aSiblingMCs.link_عقب(یہ);
    mtask_ڈیٹا(m_bp).bSiblingMCs.link_عقب(یہ);
}

خالی SiblingMC::unlinkA() {
    VL_ATTR_غیر_مستعمل ثابت سائز_t removed = mtask_ڈیٹا(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtask_ڈیٹا(m_ap).aSiblingMCs.unlink(یہ);
}

خالی SiblingMC::unlinkB() { mtask_ڈیٹا(m_bp).bSiblingMCs.unlink(یہ); }

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
جماعت ملاؤ_Candidate_Scoreboard final {
    // TYPES
    استعمال ڈھیر_2 = Pairing_ڈھیر<ملاؤ_Candidate_کلید>;

    // MEMBERS
    ڈھیر_2 m_ڈھیر_2;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<کنارہ_MC> m_کنارہ_MCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    خالی داخل_کرو(ملاؤ_Candidate* nodep) {
        nodep->updateScore();
        m_ڈھیر_2.داخل_کرو(nodep);
    }

    // Remove a candidate from the scoreboard.
    خالی remove(ملاؤ_Candidate* nodep) { m_ڈھیر_2.remove(nodep); }

عوامی:
    // CONSTRUCTORS
    ملاؤ_Candidate_Scoreboard() = طےشدہ;
    ~ملاؤ_Candidate_Scoreboard() = طےشدہ;
    VL_UNCOPYABLE(ملاؤ_Candidate_Scoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    ملاؤ_Candidate* بہترین() ثابت { واپس ملاؤ_Candidate::ڈھیر_گرہ_مقصد_Elem(m_ڈھیر_2.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    خالی rescore(ملاؤ_Candidate* nodep) {
        remove(nodep);
        داخل_کرو(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    خالی جوڑو_کنارہ_MC(MTask_کنارہ* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        کنارہ_MC* ثابت کنارہ_MCp = m_کنارہ_MCPool.alloc(edgep);
        edgep->userp(کنارہ_MCp);
        داخل_کرو(کنارہ_MCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    خالی remove_کنارہ_MC(MTask_کنارہ* edgep) {
        کنارہ_MC* ثابت کنارہ_MCp = کنارہ_MC_2(edgep);
        UDEBUGONLY(UASSERT(کنارہ_MCp, "Edge has no merge candidate"););
        edgep->userp(خالی_اشاریہ);
        remove(کنارہ_MCp);
        VL_DO_DANGLING(m_کنارہ_MCPool.آزاد(کنارہ_MCp), کنارہ_MCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    خالی جوڑو_Sibling_MC(LogicMTask* ap, LogicMTask* bp) { داخل_کرو(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    خالی removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.آزاد(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    خالی removeMC(ملاؤ_Candidate* nodep) {
        اگر (SiblingMC* ثابت smcp = nodep->مقصد_Sibling_MC()) {
            removeSiblingMC(smcp);
        } ورنہ {
            remove_کنارہ_MC(nodep->مقصد_کنارہ_MC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
جماعت Contraction final {
    // MEMBERS
    ترتیب_MTask_گراف& m_m_Task_گراف;  // The Mtask graph
    uint64_t m_score_حد_بندی;  // Critical path limit for merges
    ملاؤ_Candidate_Scoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTask_Contraction_ڈیٹا[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    خالی جوڑو_کنارہ_MCs(LogicMTask* mtaskp) {
        برائے (V3_گراف_کنارہ& e : mtaskp->برآمد_کنارے()) m_sb.جوڑو_کنارہ_MC(static_cast<MTask_کنارہ*>(&e));
        برائے (V3_گراف_کنارہ& e : mtaskp->اندر_کنارے()) m_sb.جوڑو_کنارہ_MC(static_cast<MTask_کنارہ*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    خالی remove_کنارہ_MCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        برائے (V3_گراف_کنارہ& کنارہ : mtaskp->برآمد_کنارے()) {
            MTask_کنارہ* ثابت edgep = static_cast<MTask_کنارہ*>(&کنارہ);
            اگر (کنارہ_MC_2(edgep)) m_sb.remove_کنارہ_MC(edgep);
        }
        برائے (V3_گراف_کنارہ& کنارہ : mtaskp->اندر_کنارے()) {
            MTask_کنارہ* ثابت edgep = static_cast<MTask_کنارہ*>(&کنارہ);
            اگر (کنارہ_MC_2(edgep)) m_sb.remove_کنارہ_MC(edgep);
        }
    }

    خالی makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        اگر (ap->id() < bp->id()) std::تبادلہ(ap, bp);
        // The higher id vertex owns the association set
        ثابت منطقی پہلا = mtask_ڈیٹا(ap).siblings.داخل_کرو(bp).دوسرا;
        اگر (پہلا) {
            m_sb.جوڑو_Sibling_MC(ap, bp);
            واپس;
        }

        اگر (VL_UNLIKELY(m_m_Task_گراف.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            منطقی ملا = جھوٹ;
            برائے (ثابت SiblingMC& smc : mtask_ڈیٹا(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                اگر (smc.bp() == bp) ملا = سچ;
            }
            UASSERT_OBJ(ملا, ap, "Sibling not found");
        }
    }

    سانچہ <گراف_Way::en N_Way, منطقی N_Exhaustive>
    خالی جوڑو_Sibling_MCs_From_Relatives(LogicMTask* mtaskp) {
        constexpr گراف_Way way{N_Way};
        // Need at least 2 edges
        خودکار& کنارے = mtaskp->کنارے<way>();
        اگر (!کنارے.has_مضرب_Elements()) واپس;

        std::array<LogicMTask*, PART_SIBLING_کنارہ_حد_بندی> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        ساخت alignas(16) ترتیب_کاری_ریکارڈ final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_کنارہ_حد_بندی <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            منطقی عامل<(ثابت ترتیب_کاری_ریکارڈ& that) ثابت {
                واپس m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(حجم(ترتیب_کاری_ریکارڈ) <= 16, "How could this be padded to more than 16?");

        std::array<ترتیب_کاری_ریکارڈ, PART_SIBLING_کنارہ_حد_بندی> ترتیب_دو_Recs;
        سائز_t n = 0;

        // Populate the buffers
        برائے (V3_گراف_کنارہ& کنارہ : mtaskp->کنارے<way>()) {
            LogicMTask* ثابت otherp = static_cast<LogicMTask*>(کنارہ.furtherp<way>());
            neighbors[n] = otherp;
            ترتیب_دو_Recs[n].m_id = otherp->id();
            ترتیب_دو_Recs[n].m_cp = otherp->cpInclusive<way>();
            ترتیب_دو_Recs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            اگر (n >= PART_SIBLING_کنارہ_حد_بندی) توڑو;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr سائز_t MAX_NONEXHAUSTIVE_جوڑے = 3;

        اگر (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_جوڑے) {
            ثابت سائز_t انتہا = n & ~static_cast<سائز_t>(1);  // Round down to even, (we want pairs)
            std::ترتیب_دو(ترتیب_دو_Recs.begin(), ترتیب_دو_Recs.begin() + n);
            برائے (سائز_t i = 0; i < انتہا; i += 2) {
                makeSiblingMC(neighbors[ترتیب_دو_Recs[i].m_idx], neighbors[ترتیب_دو_Recs[i + 1].m_idx]);
            }
        } ورنہ {
            constexpr سائز_t انتہا = 2 * MAX_NONEXHAUSTIVE_جوڑے;
            std::partial_ترتیب_دو(ترتیب_دو_Recs.begin(), ترتیب_دو_Recs.begin() + انتہا, ترتیب_دو_Recs.begin() + n);
            برائے (سائز_t i = 0; i < انتہا; i += 2) {
                makeSiblingMC(neighbors[ترتیب_دو_Recs[i].m_idx], neighbors[ترتیب_دو_Recs[i + 1].m_idx]);
            }
        }
    }

    خالی removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        جب (SiblingMC* ثابت smcp = mtask_ڈیٹا(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        جب (SiblingMC* ثابت smcp = mtask_ڈیٹا(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    خالی contract(ملاؤ_Candidate* ملاؤ_Canp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* چوٹی;
        اگر (ثابت کنارہ_MC* ثابت کنارہ_MCp = ملاؤ_Canp->مقصد_کنارہ_MC()) {
            fromp = کنارہ_MCp->edgep()->fromMTaskp();
            چوٹی = کنارہ_MCp->edgep()->مقصد_MTaskp();
        } ورنہ {
            ثابت SiblingMC* ثابت sibMCp = ملاؤ_Canp->مقصد_Sibling_MC();
            fromp = sibMCp->bp();
            چوٹی = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        اگر (fromp->لاگت() > چوٹی->لاگت()) {
            recipientp = fromp;
            donorp = چوٹی;
        } ورنہ {
            donorp = fromp;
            recipientp = چوٹی;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(چوٹی);  // Use donorp and recipientp now instead

        // Remove all SiblingMCs that include either MTask
        removeSiblingMCs(recipientp);
        removeSiblingMCs(donorp);

        // Remove the EdgeMCs of both MTasks
        remove_کنارہ_MCs(recipientp);
        remove_کنارہ_MCs(donorp);

        // Merge the MTasks. This redirects all edges, updates critical paths, and deletes donorp
        m_m_Task_گراف.ملاؤ_MTasks(recipientp, donorp);
        VL_DANGLING(donorp);

        // Confirm we haven't botched the CP updates. This is a whole graph walk after every single
        // merge, so it is quadratic in the size of the graph, hence only under '--debug 9'.
        اگر (VL_UNLIKELY(debug() >= 9)) m_m_Task_گراف.validate();

        // Add the EdgeMCs of the merged MTask
        جوڑو_کنارہ_MCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        جوڑو_Sibling_MCs_From_Relatives<گراف_Way::الٹ_2, سچ>(recipientp);
        جوڑو_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, سچ>(recipientp);
        غیرعلامتی کنارے = 0;
        برائے (V3_گراف_کنارہ& کنارہ : recipientp->برآمد_کنارے()) {
            LogicMTask* ثابت postreqp = static_cast<LogicMTask*>(کنارہ.چوٹی());
            جوڑو_Sibling_MCs_From_Relatives<گراف_Way::الٹ_2, جھوٹ>(postreqp);
            ++کنارے;
            اگر (کنارے >= PART_SIBLING_کنارہ_حد_بندی) توڑو;
        }
        کنارے = 0;
        برائے (V3_گراف_کنارہ& کنارہ : recipientp->اندر_کنارے()) {
            LogicMTask* ثابت prereqp = static_cast<LogicMTask*>(کنارہ.fromp());
            جوڑو_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, جھوٹ>(prereqp);
            ++کنارے;
            اگر (کنارے >= PART_SIBLING_کنارہ_حد_بندی) توڑو;
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
            ثابت صحیح given = v3Global.opt.دھاگے_Max_MTasks();
            اگر (given > 0) واپس given;
            // Unspecified so estimate
            واپس PART_DEFAULT_MAX_MTASKS_فی_THREAD * v3Global.opt.دھاگے();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            ثابت سائز_t nMTasks = m_m_Task_گراف.vertices().سائز();
            m_mtaskDatap.reset(نیا MTask_Contraction_ڈیٹا[nMTasks]);
            سائز_t i = 0;
            برائے (V3_گراف_راس& vtx : m_m_Task_گراف.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        برائے (V3_گراف_راس& vtx : m_m_Task_گراف.vertices()) {
            برائے (V3_گراف_کنارہ& کنارہ : vtx.برآمد_کنارے())
                m_sb.جوڑو_کنارہ_MC(static_cast<MTask_کنارہ*>(&کنارہ));
            LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(&vtx);
            جوڑو_Sibling_MCs_From_Relatives<گراف_Way::الٹ_2, سچ>(mtaskp);
            جوڑو_Sibling_MCs_From_Relatives<گراف_Way::FORWARD, سچ>(mtaskp);
        }

        جب (سچ) {
            // Pick the candidate yielding the lowest local critical path.
            ملاؤ_Candidate* ثابت ملاؤ_Canp = m_sb.بہترین();
            اگر (!ملاؤ_Canp) توڑو;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            ثابت uint64_t score = ملاؤ_Canp->currentScore();
            اگر (score != ملاؤ_Canp->score()) {
                m_sb.rescore(ملاؤ_Canp);
                جاری;
            }

            // Check if the critical path limit is reached.
            اگر (score > m_score_حد_بندی) {

                // If there are still too many MTasks, raise the limit and keep going
                ثابت غیرعلامتی mtask_گنتی = m_m_Task_گراف.vertices().سائز();
                اگر (mtask_گنتی > maxMTasks) {
                    m_score_حد_بندی = (m_score_حد_بندی * 120) / 100;
                    File_سطر* ثابت flp = v3Global.rootp()->fileline();
                    اگر (!flp->warnIsOff(V3_Error_رمز::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3_Error_رمز::UNOPTTHREADS, سچ);
                    }
                    جاری;
                }

                // MTasks limit and CP limit reached. Stop.
                توڑو;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            اگر (کنارہ_MC* ثابت کنارہ_MCp = ملاؤ_Canp->مقصد_کنارہ_MC()) {
                MTask_کنارہ* ثابت edgep = کنارہ_MCp->edgep();
                اگر (edgep->fromp() == m_m_Task_گراف.entryp()
                    || edgep->چوٹی() == m_m_Task_گراف.exitp()) {
                    m_sb.remove_کنارہ_MC(edgep);
                    جاری;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            اگر (ملاؤ_Canp->ملاؤ_Would_Create_چکر(m_m_Task_گراف)) {
                m_sb.removeMC(ملاؤ_Canp);
                جاری;
            }

            // Merge this candidate
            contract(ملاؤ_Canp);
        }

        // Free all remaining merge candidates.
        جب (ملاؤ_Candidate* ثابت ملاؤ_Canp = m_sb.بہترین()) m_sb.removeMC(ملاؤ_Canp);
    }

عوامی:
    ساکن خالی apply(ترتیب_MTask_گراف& m_Task_گراف, uint64_t score_حد_بندی) {
        Contraction{m_Task_گراف, score_حد_بندی};
    }
};

//######################################################################
// OrderMTaskGraph entry point

خالی ترتیب_MTask_گراف::contract(ترتیب_MTask_گراف& mtask_گراف, uint64_t score_حد_بندی) {
    Contraction::apply(mtask_گراف, score_حد_بندی);
}
