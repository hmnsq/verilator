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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3OrderMTaskGraph.h"
#అన్తర్భావ "V3PairingHeap.h"
#అన్తర్భావ "V3PoolAllocator.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <array>
#అన్తర్భావ <limits>
#అన్తర్భావ <memory>
#అన్తర్భావ <type_traits>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి విలీనం_Candidate;
తరగతి SiblingMC;
తరగతి అంచు_MC;

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
constexpr గుర్తులేని PART_SIBLING_అంచు_హద్దు = 26;

// If the user doesn't give one with '--threads-max-mtasks', we'll set the
// maximum # of MTasks to (# of threads * PART_DEFAULT_MAX_MTASKS_PER_THREAD)
constexpr గుర్తులేని PART_DEFAULT_MAX_MTASKS_చొప్పున_THREAD = 50;

//######################################################################
// MTask utility classes

నిర్మాణం విలీనం_Candidate_కీలకం final {
    // Note: Structure layout chosen to minimize padding in PairingHeap<*>::Node
    uint64_t m_id;  // Unique ID part of the key
    uint64_t m_score;  // Score part of the key
    తార్కిక క్రియాకారకం<(స్థిరరాశి విలీనం_Candidate_కీలకం& other) స్థిరరాశి {
        // First by Score then by ID, but notice that we want minimums using a max-heap, so reverse
        తిరిగి m_score > other.m_score || (m_score == other.m_score && m_id > other.m_id);
    }
};

// For efficiency, MergeCandidateScoreboard elements must derive from
// PairingHeap<MergeCandidateKey>::Node
ఉపయోగించి విలీనం_Candidate_కుప్ప_కణుపు = Pairing_కుప్ప<విలీనం_Candidate_కీలకం>::కణుపు_2;

// Information associated with scoreboarding a merge candidate
తరగతి విలీనం_Candidate VL_NOT_FINAL : బహిరంగ విలీనం_Candidate_కుప్ప_కణుపు {
    // Only the known subclasses can create or delete one of these
    మిత్ర తరగతి SiblingMC;
    మిత్ర తరగతి అంచు_MC;

    // This structure is extremely hot. To save 8 bytes by not having a virtual
    // function table, we implement the few polymorphic methods over the two
    // known subclasses explicitly, using a bit of the id to denote the actual
    // subtype.

    // By using the bottom bits for flags, we can still use < to compare IDs without masking.
    // <63:1> Serial number for ordering, <0> subtype (SiblingMC)
    స్థిర constexpr uint64_t IS_SIBLING_ముసుగు = 1ULL << 0;
    స్థిర constexpr uint64_t ID_INCREMENT = 1ULL << 1;

    తార్కిక isSiblingMC() స్థిరరాశి { తిరిగి m_కీలకం.m_id & IS_SIBLING_ముసుగు; }

    // CONSTRUCTORS
    స్పష్టమైన విలీనం_Candidate(తార్కిక isSiblingMC) {
        స్థిర uint64_t s_serial = 0;
        s_serial += ID_INCREMENT;  // +ID_INCREMENT so doesn't set the special bottom bits
        m_కీలకం.m_id = s_serial | (isSiblingMC * IS_SIBLING_ముసుగు);
    }
    ~విలీనం_Candidate() = అప్రమేయ;

బహిరంగ:
    // METHODS
    SiblingMC* గమ్యం_Sibling_MC();  // Instead of cast<>/as<>
    అంచు_MC* గమ్యం_అంచు_MC();  // Instead of cast<>/as<>
    తార్కిక విలీనం_Would_Create_చక్రం(క్రమం_MTask_లేఖాచిత్రం& లేఖాచిత్రం);

    // The current score of this candidate, which changes as the graph is contracted
    అంతఃవరుస uint64_t currentScore();
    // The score this candidate was last given, which is its key in the scoreboard heap
    uint64_t score() స్థిరరాశి { తిరిగి m_కీలకం.m_score; }
    // Set the score of this candidate to its current value. Only valid while it is not in a heap.
    శూన్య updateScore() { m_కీలకం.m_score = currentScore(); }

    స్థిర విలీనం_Candidate* కుప్ప_కణుపు_గమ్యం_Elem(విలీనం_Candidate_కుప్ప_కణుపు* nodep) {
        తిరిగి static_cast<విలీనం_Candidate*>(nodep);
    }
};

static_assert(పరిమాణం(విలీనం_Candidate) == పరిమాణం(విలీనం_Candidate_కుప్ప_కణుపు),
              "Should not have a vtable");

// A pair of associated LogicMTask's that are merge candidates for sibling
// contraction
తరగతి SiblingMC final : బహిరంగ విలీనం_Candidate {
    LogicMTask* స్థిరరాశి m_ap;  // The higher ID MTask
    LogicMTask* స్థిరరాశి m_bp;  // The lower ID MTask

    V3ListLinks<SiblingMC> m_aLinks;  // List links to store instances of this class
    V3ListLinks<SiblingMC> m_bLinks;  // List links to store instances of this class

    V3ListLinks<SiblingMC>& aLinks() { తిరిగి m_aLinks; }
    V3ListLinks<SiblingMC>& bLinks() { తిరిగి m_bLinks; }

బహిరంగ:
    // List type to store instances of this class
    ఉపయోగించి AList = V3List<SiblingMC, &SiblingMC::aLinks>;
    ఉపయోగించి BList = V3List<SiblingMC, &SiblingMC::bLinks>;

    // CONSTRUCTORS
    SiblingMC(LogicMTask* ap, LogicMTask* bp);
    ~SiblingMC() = అప్రమేయ;

    // METHODS
    శూన్య unlinkA();
    శూన్య unlinkB();

    LogicMTask* ap() స్థిరరాశి { తిరిగి m_ap; }
    LogicMTask* bp() స్థిరరాశి { తిరిగి m_bp; }
};

static_assert(!std::is_polymorphic<SiblingMC>::విలువ, "Should not have a vtable");

// A merge candidate associated with an MTaskEdge (edge contraction candidate)
తరగతి అంచు_MC final : బహిరంగ విలీనం_Candidate {
    MTask_అంచు* స్థిరరాశి m_edgep;  // The associated edge

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన అంచు_MC(MTask_అంచు* edgep)
        : విలీనం_Candidate{/* isSiblingMC: */ అబద్ధం}
        , m_edgep{edgep} {}
    ~అంచు_MC() = అప్రమేయ;

    // METHODS
    MTask_అంచు* edgep() స్థిరరాశి { తిరిగి m_edgep; }
};

static_assert(!std::is_polymorphic<అంచు_MC>::విలువ, "Should not have a vtable");

// Auxiliary data associated with each LogicMTask during Contraction, attached via
// LogicMTask::userp(). Kept out of LogicMTask itself so that LogicMTask does not depend on the
// MergeCandidate hierarchy (which the SiblingMC lists reference).
నిర్మాణం MTask_Contraction_దత్తాంశం final {
    // MTasks for which a SiblingMC exists with the owning MTask as the higher ID MTask (m_ap)
    std::unordered_సమితి<LogicMTask*> siblings;
    // SiblingMCs for which the owning MTask is the higher ID MTask (m_ap in SiblingMC)
    SiblingMC::AList aSiblingMCs;
    // SiblingMCs for which the owning MTask is the lower ID MTask (m_bp in SiblingMC)
    SiblingMC::BList bSiblingMCs;
};

// The MTaskContractionData attached to 'mtaskp' (see LogicMTask::userp)
స్థిర MTask_Contraction_దత్తాంశం& mtask_దత్తాంశం(స్థిరరాశి LogicMTask* mtaskp) {
    తిరిగి *static_cast<MTask_Contraction_దత్తాంశం*>(mtaskp->userp());
}

// The EdgeMC associated with 'edgep' while it is on the scoreboard, or nullptr otherwise. Held in
// the edge's user pointer (see MTaskEdge), kept here so MTaskEdge does not depend on EdgeMC.
స్థిర అంచు_MC* అంచు_MC_2(స్థిరరాశి MTask_అంచు* edgep) { తిరిగి static_cast<అంచు_MC*>(edgep->userp()); }

// Instead of dynamic cast
SiblingMC* విలీనం_Candidate::గమ్యం_Sibling_MC() {
    తిరిగి isSiblingMC() ? static_cast<SiblingMC*>(ఇది) : శూన్యసూచిక;
}

అంచు_MC* విలీనం_Candidate::గమ్యం_అంచు_MC() { తిరిగి isSiblingMC() ? శూన్యసూచిక : static_cast<అంచు_MC*>(ఇది); }

తార్కిక విలీనం_Candidate::విలీనం_Would_Create_చక్రం(క్రమం_MTask_లేఖాచిత్రం& లేఖాచిత్రం) {
    // Sibling merge: merging creates a cycle if either sibling is reachable from the other
    ఒకవేళ (స్థిరరాశి SiblingMC* స్థిరరాశి sibp = గమ్యం_Sibling_MC()) {
        తిరిగి లేఖాచిత్రం.మార్గం_Exists(sibp->ap(), sibp->bp(), శూన్యసూచిక)
               || లేఖాచిత్రం.మార్గం_Exists(sibp->bp(), sibp->ap(), శూన్యసూచిక);
    }

    // Edge merge: merging creates a cycle if there is another path between the two MTasks
    MTask_అంచు* స్థిరరాశి edgep = గమ్యం_అంచు_MC()->edgep();
    తిరిగి లేఖాచిత్రం.మార్గం_Exists(edgep->fromMTaskp(), edgep->గమ్యం_MTaskp(), edgep);
}

uint64_t విలీనం_Candidate::currentScore() {
    // Score this candidate. The score is the new local CP length if we merge this candidate.
    // ("Local" means the longest critical path running through the merged node.)

    // Sibling merge
    ఒకవేళ (స్థిరరాశి SiblingMC* స్థిరరాశి sibp = గమ్యం_Sibling_MC()) {
        స్థిరరాశి LogicMTask* స్థిరరాశి ap = sibp->ap();
        స్థిరరాశి LogicMTask* స్థిరరాశి bp = sibp->bp();
        స్థిరరాశి uint64_t mergedCpFwd
            = std::max(ap->cpExclusive<లేఖాచిత్రం_Way::FORWARD>(), bp->cpExclusive<లేఖాచిత్రం_Way::FORWARD>());
        స్థిరరాశి uint64_t mergedCpRev
            = std::max(ap->cpExclusive<లేఖాచిత్రం_Way::తిరగేయి_2>(), bp->cpExclusive<లేఖాచిత్రం_Way::తిరగేయి_2>());
        తిరిగి mergedCpRev + mergedCpFwd + ap->ఖర్చు() + bp->ఖర్చు();
    }

    // Edge merge
    {
        MTask_అంచు* స్థిరరాశి edgep = గమ్యం_అంచు_MC()->edgep();
        స్థిరరాశి LogicMTask* స్థిరరాశి fromp = edgep->fromMTaskp();
        స్థిరరాశి LogicMTask* స్థిరరాశి శిఖరం = edgep->గమ్యం_MTaskp();
        స్థిరరాశి uint64_t mergedCpFwd = std::max(fromp->cpExclusive<లేఖాచిత్రం_Way::FORWARD>(),
                                              శిఖరం->cpExclusiveWithout<లేఖాచిత్రం_Way::FORWARD>(edgep));
        స్థిరరాశి uint64_t mergedCpRev = std::max(fromp->cpExclusiveWithout<లేఖాచిత్రం_Way::తిరగేయి_2>(edgep),
                                              శిఖరం->cpExclusive<లేఖాచిత్రం_Way::తిరగేయి_2>());
        // Give a slight preference to sibling merges by increasing the cost of edge merges.
        // This biases towards sibling merges in case they are equal score with edge merges.
        // This avoid a central node growing while many leaves remain due to edge merges.
        తిరిగి 1 + mergedCpRev + mergedCpFwd + fromp->ఖర్చు() + శిఖరం->ఖర్చు();
    }
}

SiblingMC::SiblingMC(LogicMTask* ap, LogicMTask* bp)
    : విలీనం_Candidate{/* isSiblingMC: */ నిజం}
    , m_ap{ap}
    , m_bp{bp} {
    // Storage management depends on this
    UASSERT(ap->id() > bp->id(), "Should be ordered");
    UDEBUGONLY(UASSERT(mtask_దత్తాంశం(ap).siblings.లెక్క(bp), "Should be in sibling map"););
    mtask_దత్తాంశం(m_ap).aSiblingMCs.link_వెనుక(ఇది);
    mtask_దత్తాంశం(m_bp).bSiblingMCs.link_వెనుక(ఇది);
}

శూన్య SiblingMC::unlinkA() {
    VL_ATTR_వాడని స్థిరరాశి కొలత_t removed = mtask_దత్తాంశం(m_ap).siblings.erase(m_bp);
    UDEBUGONLY(UASSERT(removed == 1, "Should have been in sibling set"););
    mtask_దత్తాంశం(m_ap).aSiblingMCs.unlink(ఇది);
}

శూన్య SiblingMC::unlinkB() { mtask_దత్తాంశం(m_bp).bSiblingMCs.unlink(ఇది); }

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
తరగతి విలీనం_Candidate_Scoreboard final {
    // TYPES
    ఉపయోగించి కుప్ప = Pairing_కుప్ప<విలీనం_Candidate_కీలకం>;

    // MEMBERS
    కుప్ప m_కుప్ప;  // The heap of candidates, keyed on their score (see class comment above)
    PoolAllocator<అంచు_MC> m_అంచు_MCPool;  // Allocator for the edge merge candidates
    PoolAllocator<SiblingMC> m_siblingMCPool;  // Allocator for the sibling merge candidates

    // METHODS
    // Set the score of a candidate and add it to the heap. The score is computed from the critical
    // paths of the MTasks, which are up to date while the graph is being contracted.
    శూన్య చొప్పించు(విలీనం_Candidate* nodep) {
        nodep->updateScore();
        m_కుప్ప.చొప్పించు(nodep);
    }

    // Remove a candidate from the scoreboard.
    శూన్య remove(విలీనం_Candidate* nodep) { m_కుప్ప.remove(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    విలీనం_Candidate_Scoreboard() = అప్రమేయ;
    ~విలీనం_Candidate_Scoreboard() = అప్రమేయ;
    VL_UNCOPYABLE(విలీనం_Candidate_Scoreboard);

    // The candidate with the lowest score it was given, or nullptr if the scoreboard is empty.
    // Note this is only a lower bound on the best current score, see the class comment above.
    విలీనం_Candidate* ఉత్తమ() స్థిరరాశి { తిరిగి విలీనం_Candidate::కుప్ప_కణుపు_గమ్యం_Elem(m_కుప్ప.max()); }

    // Update the score of a candidate to its current value, and reposition it in the heap
    శూన్య rescore(విలీనం_Candidate* nodep) {
        remove(nodep);
        చొప్పించు(nodep);
    }

    // Create a merge candidate for 'edgep' and add it to the scoreboard
    శూన్య కూడు_అంచు_MC(MTask_అంచు* edgep) {
        UDEBUGONLY(UASSERT(!edgep->userp(), "Edge already has a merge candidate"););
        అంచు_MC* స్థిరరాశి అంచు_MCp = m_అంచు_MCPool.alloc(edgep);
        edgep->userp(అంచు_MCp);
        చొప్పించు(అంచు_MCp);
    }
    // Remove 'edgep's merge candidate from the scoreboard and release it
    శూన్య remove_అంచు_MC(MTask_అంచు* edgep) {
        అంచు_MC* స్థిరరాశి అంచు_MCp = అంచు_MC_2(edgep);
        UDEBUGONLY(UASSERT(అంచు_MCp, "Edge has no merge candidate"););
        edgep->userp(శూన్యసూచిక);
        remove(అంచు_MCp);
        VL_DO_DANGLING(m_అంచు_MCPool.విడుదల(అంచు_MCp), అంచు_MCp);
    }

    // Create a sibling merge candidate for 'ap' and 'bp' and add it to the scoreboard
    శూన్య కూడు_Sibling_MC(LogicMTask* ap, LogicMTask* bp) { చొప్పించు(m_siblingMCPool.alloc(ap, bp)); }
    // Remove sibling merge candidate 'smcp' from the scoreboard and release it
    శూన్య removeSiblingMC(SiblingMC* smcp) {
        remove(smcp);
        smcp->unlinkA();
        smcp->unlinkB();
        VL_DO_DANGLING(m_siblingMCPool.విడుదల(smcp), smcp);
    }

    // Remove the given merge candidate, whichever kind it is, from the scoreboard and release it
    శూన్య removeMC(విలీనం_Candidate* nodep) {
        ఒకవేళ (SiblingMC* స్థిరరాశి smcp = nodep->గమ్యం_Sibling_MC()) {
            removeSiblingMC(smcp);
        } లేకపోతే {
            remove_అంచు_MC(nodep->గమ్యం_అంచు_MC()->edgep());
        }
    }
};

//######################################################################
// Contraction - Perform greedy edge or sibling merges on the MTask graph
తరగతి Contraction final {
    // MEMBERS
    క్రమం_MTask_లేఖాచిత్రం& m_m_Task_లేఖాచిత్రం;  // The Mtask graph
    uint64_t m_score_హద్దు;  // Critical path limit for merges
    విలీనం_Candidate_Scoreboard m_sb;  // Scoreboard
    // Auxiliary per-MTask data (the SiblingMC lists) attached to each MTask via its user pointer.
    // Owned here for the lifetime of this Contraction. A single array, as the number of MTasks
    // can only decrease during contraction.
    std::unique_ptr<MTask_Contraction_దత్తాంశం[]> m_mtaskDatap;

    // Add merge candidates for all edges of 'mtaskp' to the scoreboard
    శూన్య కూడు_అంచు_MCs(LogicMTask* mtaskp) {
        కొరకు (V3_లేఖాచిత్రం_అంచు& e : mtaskp->బహిర్గతం_అంచులు()) m_sb.కూడు_అంచు_MC(static_cast<MTask_అంచు*>(&e));
        కొరకు (V3_లేఖాచిత్రం_అంచు& e : mtaskp->లోపల_అంచులు()) m_sb.కూడు_అంచు_MC(static_cast<MTask_అంచు*>(&e));
    }

    // Remove the merge candidates of all edges of 'mtaskp' from the scoreboard.
    శూన్య remove_అంచు_MCs(LogicMTask* mtaskp) {
        // Note not all edges have a merge candidate:
        // those rejected by the main contraction loop had theirs removed there.
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : mtaskp->బహిర్గతం_అంచులు()) {
            MTask_అంచు* స్థిరరాశి edgep = static_cast<MTask_అంచు*>(&అంచు);
            ఒకవేళ (అంచు_MC_2(edgep)) m_sb.remove_అంచు_MC(edgep);
        }
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : mtaskp->లోపల_అంచులు()) {
            MTask_అంచు* స్థిరరాశి edgep = static_cast<MTask_అంచు*>(&అంచు);
            ఒకవేళ (అంచు_MC_2(edgep)) m_sb.remove_అంచు_MC(edgep);
        }
    }

    శూన్య makeSiblingMC(LogicMTask* ap, LogicMTask* bp) {
        ఒకవేళ (ap->id() < bp->id()) std::మార్పిడి(ap, bp);
        // The higher id vertex owns the association set
        స్థిరరాశి తార్కిక మొదటి = mtask_దత్తాంశం(ap).siblings.చొప్పించు(bp).రెండవ;
        ఒకవేళ (మొదటి) {
            m_sb.కూడు_Sibling_MC(ap, bp);
            తిరిగి;
        }

        ఒకవేళ (VL_UNLIKELY(m_m_Task_లేఖాచిత్రం.slowAsserts())) {
            // It's fine if we already have this SiblingMC, we may have
            // created it earlier. Just confirm that we have associated data.
            తార్కిక దొరికింది = అబద్ధం;
            కొరకు (స్థిరరాశి SiblingMC& smc : mtask_దత్తాంశం(ap).aSiblingMCs) {
                UASSERT_OBJ(smc.ap() == ap, ap, "Inconsistent SiblingMC");
                ఒకవేళ (smc.bp() == bp) దొరికింది = నిజం;
            }
            UASSERT_OBJ(దొరికింది, ap, "Sibling not found");
        }
    }

    మూస <లేఖాచిత్రం_Way::en N_Way, తార్కిక N_Exhaustive>
    శూన్య కూడు_Sibling_MCs_From_Relatives(LogicMTask* mtaskp) {
        constexpr లేఖాచిత్రం_Way way{N_Way};
        // Need at least 2 edges
        స్వయంచాలక& అంచులు = mtaskp->అంచులు<way>();
        ఒకవేళ (!అంచులు.has_గుణిజం_Elements()) తిరిగి;

        std::array<LogicMTask*, PART_SIBLING_అంచు_హద్దు> neighbors;

        // This is a hot method, so we want so sort as efficiently as possible. We pre-load
        // all data (critical path cost and id) required for determining ordering into an aligned
        // structure. There is not enough space next to these to keep a whole pointer within 16
        // bytes, so we store an index into the neighbors buffer instead. We can then compare
        // and swap these sorting records very efficiently. With this the standard library sorting
        // functions are efficient enough and using more optimized methods (e.g.: sorting networks)
        // has no measurable benefit.
        నిర్మాణం alignas(16) క్రమీకరణ_రికార్డు final {
            uint64_t m_cp;  // Critical path cost of the neighbor (primary sort key)
            uint32_t m_id;  // Id of the neighbor (tiebreaker sort key)
            uint8_t m_idx;  // Index of the neighbor within 'neighbors'
            static_assert(PART_SIBLING_అంచు_హద్దు <= std::numeric_limits<uint8_t>::max(),
                          "m_idx must fit all indices into 'neighbors'");
            తార్కిక క్రియాకారకం<(స్థిరరాశి క్రమీకరణ_రికార్డు& that) స్థిరరాశి {
                తిరిగి m_cp < that.m_cp || (m_cp == that.m_cp && m_id < that.m_id);
            }
        };
        static_assert(పరిమాణం(క్రమీకరణ_రికార్డు) <= 16, "How could this be padded to more than 16?");

        std::array<క్రమీకరణ_రికార్డు, PART_SIBLING_అంచు_హద్దు> క్రమీకరించు_Recs;
        కొలత_t n = 0;

        // Populate the buffers
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : mtaskp->అంచులు<way>()) {
            LogicMTask* స్థిరరాశి otherp = static_cast<LogicMTask*>(అంచు.furtherp<way>());
            neighbors[n] = otherp;
            క్రమీకరించు_Recs[n].m_id = otherp->id();
            క్రమీకరించు_Recs[n].m_cp = otherp->cpInclusive<way>();
            క్రమీకరించు_Recs[n].m_idx = n;
            ++n;
            // Prevent nodes with huge numbers of edges from massively slowing down us down
            ఒకవేళ (n >= PART_SIBLING_అంచు_హద్దు) ఆపు;
        }

        // Don't make all possible pairs of siblings when not requested (non-exhaustive).
        // Just make a few pairs.
        constexpr కొలత_t MAX_NONEXHAUSTIVE_జతలు = 3;

        ఒకవేళ (N_Exhaustive || n <= 2 * MAX_NONEXHAUSTIVE_జతలు) {
            స్థిరరాశి కొలత_t చివర = n & ~static_cast<కొలత_t>(1);  // Round down to even, (we want pairs)
            std::క్రమీకరించు(క్రమీకరించు_Recs.begin(), క్రమీకరించు_Recs.begin() + n);
            కొరకు (కొలత_t i = 0; i < చివర; i += 2) {
                makeSiblingMC(neighbors[క్రమీకరించు_Recs[i].m_idx], neighbors[క్రమీకరించు_Recs[i + 1].m_idx]);
            }
        } లేకపోతే {
            constexpr కొలత_t చివర = 2 * MAX_NONEXHAUSTIVE_జతలు;
            std::partial_క్రమీకరించు(క్రమీకరించు_Recs.begin(), క్రమీకరించు_Recs.begin() + చివర, క్రమీకరించు_Recs.begin() + n);
            కొరకు (కొలత_t i = 0; i < చివర; i += 2) {
                makeSiblingMC(neighbors[క్రమీకరించు_Recs[i].m_idx], neighbors[క్రమీకరించు_Recs[i + 1].m_idx]);
            }
        }
    }

    శూన్య removeSiblingMCs(LogicMTask* mtaskp) {
        // Note: 'removeSiblingMC' unlinks the candidate from both of its MTasks' lists, so taking
        // the front element repeatedly does terminate. It also erases the candidate from the
        // owning (higher id) MTask's sibling set as it goes, so both the sets and the lists are
        // left consistent, whichever side of the candidate 'mtaskp' happens to be on.
        అయితే (SiblingMC* స్థిరరాశి smcp = mtask_దత్తాంశం(mtaskp).aSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
        అయితే (SiblingMC* స్థిరరాశి smcp = mtask_దత్తాంశం(mtaskp).bSiblingMCs.frontp()) {
            m_sb.removeSiblingMC(smcp);
        }
    }

    // Merge the two MTasks of 'mergeCanp'
    శూన్య contract(విలీనం_Candidate* విలీనం_Canp) {
        // The two MTasks to merge. Note the order of the two decides which of them becomes the
        // recipient below when their costs are equal, so keep it stable.
        LogicMTask* fromp;
        LogicMTask* శిఖరం;
        ఒకవేళ (స్థిరరాశి అంచు_MC* స్థిరరాశి అంచు_MCp = విలీనం_Canp->గమ్యం_అంచు_MC()) {
            fromp = అంచు_MCp->edgep()->fromMTaskp();
            శిఖరం = అంచు_MCp->edgep()->గమ్యం_MTaskp();
        } లేకపోతే {
            స్థిరరాశి SiblingMC* స్థిరరాశి sibMCp = విలీనం_Canp->గమ్యం_Sibling_MC();
            fromp = sibMCp->bp();
            శిఖరం = sibMCp->ap();
        }

        // Merge the smaller mtask into the larger mtask.
        LogicMTask* recipientp;
        LogicMTask* donorp;
        ఒకవేళ (fromp->ఖర్చు() > శిఖరం->ఖర్చు()) {
            recipientp = fromp;
            donorp = శిఖరం;
        } లేకపోతే {
            donorp = fromp;
            recipientp = శిఖరం;
        }
        VL_DANGLING(fromp);
        VL_DANGLING(శిఖరం);  // Use donorp and recipientp now instead

        // Remove all SiblingMCs that include either MTask
        removeSiblingMCs(recipientp);
        removeSiblingMCs(donorp);

        // Remove the EdgeMCs of both MTasks
        remove_అంచు_MCs(recipientp);
        remove_అంచు_MCs(donorp);

        // Merge the MTasks. This redirects all edges, updates critical paths, and deletes donorp
        m_m_Task_లేఖాచిత్రం.విలీనం_MTasks(recipientp, donorp);
        VL_DANGLING(donorp);

        // Confirm we haven't botched the CP updates. This is a whole graph walk after every single
        // merge, so it is quadratic in the size of the graph, hence only under '--debug 9'.
        ఒకవేళ (VL_UNLIKELY(debug() >= 9)) m_m_Task_లేఖాచిత్రం.validate();

        // Add the EdgeMCs of the merged MTask
        కూడు_అంచు_MCs(recipientp);

        // Finally, make new sibling pairs as needed:
        //  - prereqs and postreqs of recipientp
        //  - prereqs of recipientp's postreqs
        //  - postreqs of recipientp's prereqs
        // Note that this depends on the updated critical paths (above).
        కూడు_Sibling_MCs_From_Relatives<లేఖాచిత్రం_Way::తిరగేయి_2, నిజం>(recipientp);
        కూడు_Sibling_MCs_From_Relatives<లేఖాచిత్రం_Way::FORWARD, నిజం>(recipientp);
        గుర్తులేని అంచులు = 0;
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : recipientp->బహిర్గతం_అంచులు()) {
            LogicMTask* స్థిరరాశి postreqp = static_cast<LogicMTask*>(అంచు.శిఖరం());
            కూడు_Sibling_MCs_From_Relatives<లేఖాచిత్రం_Way::తిరగేయి_2, అబద్ధం>(postreqp);
            ++అంచులు;
            ఒకవేళ (అంచులు >= PART_SIBLING_అంచు_హద్దు) ఆపు;
        }
        అంచులు = 0;
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : recipientp->లోపల_అంచులు()) {
            LogicMTask* స్థిరరాశి prereqp = static_cast<LogicMTask*>(అంచు.fromp());
            కూడు_Sibling_MCs_From_Relatives<లేఖాచిత్రం_Way::FORWARD, అబద్ధం>(prereqp);
            ++అంచులు;
            ఒకవేళ (అంచులు >= PART_SIBLING_అంచు_హద్దు) ఆపు;
        }
    }

    // CONSTRUCTORS
    Contraction(క్రమం_MTask_లేఖాచిత్రం& m_Task_లేఖాచిత్రం, uint64_t cp_హద్దు)
        : m_m_Task_లేఖాచిత్రం{m_Task_లేఖాచిత్రం}
        , m_score_హద్దు{cp_హద్దు} {

        // Check the graph we were given is consistent.
        m_m_Task_లేఖాచిత్రం.validate();

        // Figure out maximum number of MTasks
        స్థిరరాశి uint32_t maxMTasks = []() -> uint32_t {
            // If specified, use the given value
            స్థిరరాశి పూర్ణాంకం given = v3Global.opt.దారాలు_Max_MTasks();
            ఒకవేళ (given > 0) తిరిగి given;
            // Unspecified so estimate
            తిరిగి PART_DEFAULT_MAX_MTASKS_చొప్పున_THREAD * v3Global.opt.దారాలు();
        }();

        // Allocate and assign the auxiliary data for every LogicMTask.
        {
            స్థిరరాశి కొలత_t nMTasks = m_m_Task_లేఖాచిత్రం.vertices().కొలత();
            m_mtaskDatap.reset(కొత్త MTask_Contraction_దత్తాంశం[nMTasks]);
            కొలత_t i = 0;
            కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_m_Task_లేఖాచిత్రం.vertices()) vtx.userp(&m_mtaskDatap[i++]);
            UASSERT(i == nMTasks, "Inconsistent MTask count");
        }

        // Add initial candidates
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_m_Task_లేఖాచిత్రం.vertices()) {
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtx.బహిర్గతం_అంచులు())
                m_sb.కూడు_అంచు_MC(static_cast<MTask_అంచు*>(&అంచు));
            LogicMTask* స్థిరరాశి mtaskp = static_cast<LogicMTask*>(&vtx);
            కూడు_Sibling_MCs_From_Relatives<లేఖాచిత్రం_Way::తిరగేయి_2, నిజం>(mtaskp);
            కూడు_Sibling_MCs_From_Relatives<లేఖాచిత్రం_Way::FORWARD, నిజం>(mtaskp);
        }

        అయితే (నిజం) {
            // Pick the candidate yielding the lowest local critical path.
            విలీనం_Candidate* స్థిరరాశి విలీనం_Canp = m_sb.ఉత్తమ();
            ఒకవేళ (!విలీనం_Canp) ఆపు;  // No more candidates

            // If the score has changed since it was inserted into the scoreboard, rescore it and
            // pick again. (The real scores can differ from the one used to insert it into the
            // scoreboard, due to merges between insertion and retrieval.)
            స్థిరరాశి uint64_t score = విలీనం_Canp->currentScore();
            ఒకవేళ (score != విలీనం_Canp->score()) {
                m_sb.rescore(విలీనం_Canp);
                కొనసాగు;
            }

            // Check if the critical path limit is reached.
            ఒకవేళ (score > m_score_హద్దు) {

                // If there are still too many MTasks, raise the limit and keep going
                స్థిరరాశి గుర్తులేని mtask_లెక్క = m_m_Task_లేఖాచిత్రం.vertices().కొలత();
                ఒకవేళ (mtask_లెక్క > maxMTasks) {
                    m_score_హద్దు = (m_score_హద్దు * 120) / 100;
                    File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->fileline();
                    ఒకవేళ (!flp->warnIsOff(V3_Error_సంకేతం::UNOPTTHREADS)) {
                        flp->v3warn(UNOPTTHREADS,
                                    "Thread scheduler is unable to provide requested "
                                    "parallelism; suggest asking for fewer threads.");
                        flp->modifyWarnOff(V3_Error_సంకేతం::UNOPTTHREADS, నిజం);
                    }
                    కొనసాగు;
                }

                // MTasks limit and CP limit reached. Stop.
                ఆపు;
            }

            // Avoid merging the entry/exit nodes. This would create serialization, by forcing the
            // merged MTask to run before/after everything else. Empirically this helps performance
            // in a modest way by allowing other MTasks to start earlier.
            ఒకవేళ (అంచు_MC* స్థిరరాశి అంచు_MCp = విలీనం_Canp->గమ్యం_అంచు_MC()) {
                MTask_అంచు* స్థిరరాశి edgep = అంచు_MCp->edgep();
                ఒకవేళ (edgep->fromp() == m_m_Task_లేఖాచిత్రం.entryp()
                    || edgep->శిఖరం() == m_m_Task_లేఖాచిత్రం.exitp()) {
                    m_sb.remove_అంచు_MC(edgep);
                    కొనసాగు;
                }
            }

            // Avoid merging any edge that would create a cycle. For example suppose we begin with
            // vertices A, B, C and edges A->B, B->C, A->C. Merging A->C would create a cycle.
            ఒకవేళ (విలీనం_Canp->విలీనం_Would_Create_చక్రం(m_m_Task_లేఖాచిత్రం)) {
                m_sb.removeMC(విలీనం_Canp);
                కొనసాగు;
            }

            // Merge this candidate
            contract(విలీనం_Canp);
        }

        // Free all remaining merge candidates.
        అయితే (విలీనం_Candidate* స్థిరరాశి విలీనం_Canp = m_sb.ఉత్తమ()) m_sb.removeMC(విలీనం_Canp);
    }

బహిరంగ:
    స్థిర శూన్య apply(క్రమం_MTask_లేఖాచిత్రం& m_Task_లేఖాచిత్రం, uint64_t score_హద్దు) {
        Contraction{m_Task_లేఖాచిత్రం, score_హద్దు};
    }
};

//######################################################################
// OrderMTaskGraph entry point

శూన్య క్రమం_MTask_లేఖాచిత్రం::contract(క్రమం_MTask_లేఖాచిత్రం& mtask_లేఖాచిత్రం, uint64_t score_హద్దు) {
    Contraction::apply(mtask_లేఖాచిత్రం, score_హద్దు);
}
