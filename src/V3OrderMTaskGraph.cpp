// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: OrderMTask graph construction
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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3OrderMTaskGraph.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3InstrCount.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_ಮುಂದಿನ_Id = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(ಕ್ರಮ_MTask_ನಕ್ಷೆ& ನಕ್ಷೆ, ಕ್ರಮ_Move_ಶೃಂಗ* mVtxp)
    : V3_ನಕ್ಷೆ_ಶೃಂಗ{&ನಕ್ಷೆ} {
    UASSERT(s_ಮುಂದಿನ_Id < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    ಆದರೆ (!mVtxp) ಹಿಂತಿರುಗಿ;
    m_mVertices.link_ಹಿಂಭಾಗ(mVtxp);
    ಆದರೆ (ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ olvp = mVtxp->logicp()) {
        m_ವೆಚ್ಚ += V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ_2(olvp->nodep(), ನಿಜ);
    }
}

//######################################################################
// OrderMTaskGraph

ಕ್ರಮ_MTask_ನಕ್ಷೆ::ಕ್ರಮ_MTask_ನಕ್ಷೆ(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ)
    : m_move_ನಕ್ಷೆ{move_ನಕ್ಷೆ}
    , m_entryp{ಹೊಸ LogicMTask{*ಇದು, ಶೂನ್ಯಸೂಚಕ}}
    , m_exitp{ಹೊಸ LogicMTask{*ಇದು, ಶೂನ್ಯಸೂಚಕ}}
    , m_slowAsserts{v3Global.opt.debug_ವಿಭಜನೆ()} {}

ತಾರ್ಕಿಕ ಕ್ರಮ_MTask_ನಕ್ಷೆ::ಮಾರ್ಗ_Exists_Impl(LogicMTask* fromp, LogicMTask* ತುದಿ,
                                     ಸ್ಥಿರಾಂಕ MTask_ಅಂಚು* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    ಆದರೆ (fromp == ತುದಿ) ಹಿಂತಿರುಗಿ ನಿಜ;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    ಆದರೆ (fromp->cpExclusive<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>() < ತುದಿ->cpInclusive<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>()) {
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಆದರೆ (fromp->cpInclusive<ನಕ್ಷೆ_Way::FORWARD>() > ತುದಿ->cpExclusive<ನಕ್ಷೆ_Way::FORWARD>()) {
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Recursively look for a path
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& follow : fromp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        ಆದರೆ (&follow == excludedEdgep) ಮುಂದುವರಿ;
        LogicMTask* ಸ್ಥಿರಾಂಕ nextp = static_cast<LogicMTask*>(follow.ತುದಿ());
        // Don't visit the same MTask twice in the same search.
        ಆದರೆ (nextp->m_generation == m_currentGeneration) ಮುಂದುವರಿ;
        ಆದರೆ (ಮಾರ್ಗ_Exists_Impl(nextp, ತುದಿ, ಶೂನ್ಯಸೂಚಕ)) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::propagate_ತಳ್ಳು(LogicMTask* mtaskp) {
    constexpr ನಕ್ಷೆ_Way way{N_Way};
    constexpr ನಕ್ಷೆ_Way inv{way.invert()};
    ಸ್ಥಿರಾಂಕ uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ನಕ್ಷೆ_ಅಂಚು : mtaskp->ಅಂಚುಗಳು<way>()) {
        MTask_ಅಂಚು& ಅಂಚು = static_cast<MTask_ಅಂಚು&>(ನಕ್ಷೆ_ಅಂಚು);

        LogicMTask* ಸ್ಥಿರಾಂಕ relativep = ಅಂಚು.furtherMTaskp<N_Way>();
        ಅಂಚು_ರಾಶಿ::ಗಂಟು_2& ಅಂಚು_ರಾಶಿ_ಗಂಟು = ಅಂಚು.m_ಅಂಚು_ರಾಶಿ_ಗಂಟು[inv];
        ಆದರೆ (inclusiveCp > ಅಂಚು_ರಾಶಿ_ಗಂಟು.ಕೀಲಿ().m_cp) {
            relativep->m_ಅಂಚು_ರಾಶಿ[inv].increase_ಕೀಲಿ(&ಅಂಚು_ರಾಶಿ_ಗಂಟು, inclusiveCp);
        }

        ಸ್ಥಿರಾಂಕ uint64_t relativeCp = relativep->cpExclusive<way>();

        ಆದರೆ (relativeCp >= inclusiveCp) ಮುಂದುವರಿ;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ಸ್ಥಿರಾಂಕ uint64_t increment = inclusiveCp - relativeCp;

        Propagate_Pending_ರಾಶಿ::ಗಂಟು_2*& pendingNodepRef = relativep->m_propagate_ರಾಶಿ_Nodep;
        ಆದರೆ (Propagate_Pending_ರಾಶಿ::ಗಂಟು_2* ಸ್ಥಿರಾಂಕ nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            ಆದರೆ (increment > nodep->ಕೀಲಿ().m_increment) {
                m_pending_ರಾಶಿ.increase_ಕೀಲಿ(nodep, increment);
            }
            ಮುಂದುವರಿ;
        }

        // Add to heap
        Propagate_Pending_ರಾಶಿ::ಗಂಟು_2* ಸ್ಥಿರಾಂಕ nodep = m_pending_ಗಂಟು_Pool.alloc();
        pendingNodepRef = nodep;
        m_pending_ರಾಶಿ.ಸೇರಿಸು(nodep, {increment, relativep->id(), relativep});
    }
}

ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::propagateResolve() {
    constexpr ನಕ್ಷೆ_Way way{N_Way};
    constexpr ನಕ್ಷೆ_Way inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    ಆಗ (!m_pending_ರಾಶಿ.ಖಾಲಿಗಳು()) {
        // Pop max element from heap
        Propagate_Pending_ರಾಶಿ::ಗಂಟು_2* ಸ್ಥಿರಾಂಕ maxp = m_pending_ರಾಶಿ.max();
        m_pending_ರಾಶಿ.remove(maxp);
        // Pick up values
        LogicMTask* ಸ್ಥಿರಾಂಕ mtaskp = maxp->ಕೀಲಿ().m_mtaskp;
        ಸ್ಥಿರಾಂಕ uint64_t cpGrowBy = maxp->ಕೀಲಿ().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_ಗಂಟು_Pool.ಬಿಡುಗಡೆ(maxp);
        mtaskp->m_propagate_ರಾಶಿ_Nodep = ಶೂನ್ಯಸೂಚಕ;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ಸ್ಥಿರಾಂಕ uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        ಆದರೆ (VL_UNLIKELY(m_slowAsserts)) {
            ಸ್ಥಿರಾಂಕ uint64_t ಅಂಚು_Cp = mtaskp->m_ಅಂಚು_ರಾಶಿ[inv].max()->ಕೀಲಿ().m_cp;
            UASSERT_OBJ(ಅಂಚು_Cp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_ತಳ್ಳು<N_Way>(mtaskp);
    }
}

uint64_t ಕ್ರಮ_MTask_ನಕ್ಷೆ::ಒಟ್ಟು_ವೆಚ್ಚ() ಸ್ಥಿರಾಂಕ {
    uint64_t ವೆಚ್ಚ = 0;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) ವೆಚ್ಚ += static_cast<ಸ್ಥಿರಾಂಕ LogicMTask&>(vtx).ವೆಚ್ಚ();
    ಹಿಂತಿರುಗಿ ವೆಚ್ಚ;
}

ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::ಕೂಡಿಸು_ಅಂಚು(LogicMTask* fromp, LogicMTask* ತುದಿ) {
    UASSERT_OBJ(fromp != ತುದಿ, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->has_ಅಂಚು_ಗಮ್ಯ(ತುದಿ), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    ಹೊಸ MTask_ಅಂಚು{ಇದು, fromp, ತುದಿ};

    // The path through the new edge might be longer than the current critical path of its
    // endpoints, in which case the critical paths need updating. Note each endpoint is the seed of
    // one propagation, and is updated by the other: the inclusive critical paths of the endpoints
    // themselves did not change (a new out-edge cannot lengthen a path into 'fromp', nor a new
    // in-edge a path out of 'top'), so it is the new relative of each seed whose critical path
    // might need to grow. That is, 'top' is updated wayward of 'fromp' below, and vice versa,
    // together with the relatives of each, transitively.
    //
    // The guards below are an asymptotic optimization. The graph is consistent apart from the new
    // edge, so the new relative is the only relative of either seed that can have a stale critical
    // path, and if it does not need updating the propagation does nothing. It would however still
    // walk all edges of the seed to discover that, which is expensive for a high degree seed.
    ಆದರೆ (fromp->cpInclusive<ನಕ್ಷೆ_Way::FORWARD>() > ತುದಿ->cpExclusive<ನಕ್ಷೆ_Way::FORWARD>()) {
        propagate<ನಕ್ಷೆ_Way::FORWARD>(fromp);
    }
    ಆದರೆ (ತುದಿ->cpInclusive<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>() > fromp->cpExclusive<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>()) {
        propagate<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(ತುದಿ);
    }
}

ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::ವಿಲೀನ_MTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    ಆಗ (MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = static_cast<MTask_ಅಂಚು*>(donorp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp())) {
        LogicMTask* ಸ್ಥಿರಾಂಕ relativep = edgep->ಗಮ್ಯ_MTaskp();

        relativep->remove_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(edgep);

        ಆದರೆ (relativep == recipientp || recipientp->has_ಅಂಚು_ಗಮ್ಯ(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->ಕೂಡಿಸು_Dependent(relativep);
            recipientp->steal_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::FORWARD>(edgep);
            relativep->ಕೂಡಿಸು_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(edgep);
        }
    }

    // Process incoming edges of donor
    ಆಗ (MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = static_cast<MTask_ಅಂಚು*>(donorp->ಒಳ_ಅಂಚುಗಳು().frontp())) {
        LogicMTask* ಸ್ಥಿರಾಂಕ relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->remove_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::FORWARD>(edgep);

        ಆದರೆ (relativep == recipientp || relativep->has_ಅಂಚು_ಗಮ್ಯ(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_ತುದಿ(recipientp);
            relativep->ಕೂಡಿಸು_Dependent(recipientp);
            relativep->ಕೂಡಿಸು_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::FORWARD>(edgep);
            recipientp->steal_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(edgep);
        }
    }

    // Move the contents of the donor into the recipient, update its cost
    recipientp->m_mVertices.splice(recipientp->m_mVertices.ಕೊನೆ(), donorp->m_mVertices);
    recipientp->m_ವೆಚ್ಚ += donorp->m_ವೆಚ್ಚ;

    // The recipient now holds all edges of the merged MTask, and the critical paths of all its
    // relatives are still up to date, so the critical paths implied by its edges are the critical
    // paths of the merged MTask.
    ಸ್ಥಿರಾಂಕ uint64_t newCpFwd = recipientp->cp_Exclusive_From_ಅಂಚುಗಳು<ನಕ್ಷೆ_Way::FORWARD>();
    ಸ್ಥಿರಾಂಕ uint64_t newCpRev = recipientp->cp_Exclusive_From_ಅಂಚುಗಳು<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<ನಕ್ಷೆ_Way::FORWARD>(newCpFwd);
    propagate<ನಕ್ಷೆ_Way::FORWARD>(recipientp);
    recipientp->cpExclusive<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(newCpRev);
    propagate<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(ಇದು), donorp);
}

ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_Edgep : vtx.ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
            MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = static_cast<MTask_ಅಂಚು*>(ನಕ್ಷೆ_Edgep);
            LogicMTask* ಸ್ಥಿರಾಂಕ fromp = edgep->fromMTaskp();
            LogicMTask* ಸ್ಥಿರಾಂಕ ತುದಿ = edgep->ಗಮ್ಯ_MTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            ಆದರೆ (!ಮಾರ್ಗ_Exists(fromp, ತುದಿ, edgep)) ಮುಂದುವರಿ;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(ತುದಿ);
            fromp->remove_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::FORWARD>(edgep);
            ತುದಿ->remove_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::remove_ಖಾಲಿಗಳು_MTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : vertices().unlinkable()) {
        LogicMTask* ಸ್ಥಿರಾಂಕ mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        ಆದರೆ (mtaskp == m_entryp || mtaskp == m_exitp) ಮುಂದುವರಿ;

        // Keep any MTask that holds logic
        ತಾರ್ಕಿಕ ಖಾಲಿಗಳು = ನಿಜ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Move_ಶೃಂಗ& mVtx : mtaskp->ಶೃಂಗ_List()) {
            ಆದರೆ (mVtx.logicp()) {
                ಖಾಲಿಗಳು = ಸುಳ್ಳು;
                ನಿಲ್ಲಿಸು;
            }
        }
        ಆದರೆ (!ಖಾಲಿಗಳು) ಮುಂದುವರಿ;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->ವೆಚ್ಚ(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಒಳ_ಅಂಚು : mtaskp->ಒಳ_ಅಂಚುಗಳು()) {
            LogicMTask* ಸ್ಥಿರಾಂಕ fromp = static_cast<MTask_ಅಂಚು&>(ಒಳ_ಅಂಚು).fromMTaskp();
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಹೊರಹೋಗು_ಅಂಚು : mtaskp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                LogicMTask* ಸ್ಥಿರಾಂಕ ತುದಿ = static_cast<MTask_ಅಂಚು&>(ಹೊರಹೋಗು_ಅಂಚು).ಗಮ್ಯ_MTaskp();
                ಆದರೆ (!fromp->has_ಅಂಚು_ಗಮ್ಯ(ತುದಿ)) ಕೂಡಿಸು_ಅಂಚು(fromp, ತುದಿ);
            }
        }

        // Remove incoming edges of 'mtaskp'
        ಆಗ (MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = static_cast<MTask_ಅಂಚು*>(mtaskp->ಒಳ_ಅಂಚುಗಳು().frontp())) {
            LogicMTask* ಸ್ಥಿರಾಂಕ relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->remove_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        ಆಗ (MTask_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = static_cast<MTask_ಅಂಚು*>(mtaskp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp())) {
            LogicMTask* ಸ್ಥಿರಾಂಕ relativep = edgep->ಗಮ್ಯ_MTaskp();
            relativep->remove_Relative_ಅಂಚು<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(ಇದು), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::validateWay() ಸ್ಥಿರಾಂಕ {
    constexpr ನಕ್ಷೆ_Way way{N_Way};
    constexpr ನಕ್ಷೆ_Way inv = way.invert();
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) {
        ಸ್ಥಿರಾಂಕ LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cp_ವೆಚ್ಚ = 0;
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*> relatives;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ನಕ್ಷೆ_ಅಂಚು : mtask.ಅಂಚುಗಳು<inv>()) {
            ಸ್ಥಿರಾಂಕ MTask_ಅಂಚು& ಅಂಚು = *ನಕ್ಷೆ_ಅಂಚು.as<MTask_ಅಂಚು>();
            ಸ್ಥಿರಾಂಕ LogicMTask& relative = *(ಅಂಚು.furtherp<inv>()->ಮಾದರಿ as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(ಅಂಚು.ತೂಕ_2() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮೊದಲ = relatives.ಸೇರಿಸು(&relative).ಎರಡನೇ;
            UASSERT_OBJ(ಮೊದಲ, &mtask, "Should be no redundant edges in MTask graph");
            ಸ್ಥಿರಾಂಕ uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(ಅಂಚು.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(ಅಂಚು.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cp_ವೆಚ್ಚ = std::max(cp_ವೆಚ್ಚ, inclusiveCp);
        }
        ಸ್ಥಿರಾಂಕ uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cp_ವೆಚ್ಚ, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cp_Exclusive_From_ಅಂಚುಗಳು<N_Way>() == cp_ವೆಚ್ಚ, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

ಶೂನ್ಯ ಕ್ರಮ_MTask_ನಕ್ಷೆ::validate() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!m_slowAsserts) ಹಿಂತಿರುಗಿ;

    validateWay<ನಕ್ಷೆ_Way::FORWARD>();
    validateWay<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>();

    // Check the dependents set of each MTask agrees with its out-edges
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : vertices()) {
        ಸ್ಥಿರಾಂಕ LogicMTask& mtask = *vtx.as<LogicMTask>();
        ಗಾತ್ರ_t nDependents = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ನಕ್ಷೆ_ಅಂಚು : mtask.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            LogicMTask* ಸ್ಥಿರಾಂಕ ತುದಿ = ನಕ್ಷೆ_ಅಂಚು.as<MTask_ಅಂಚು>()->ಗಮ್ಯ_MTaskp();
            UASSERT_OBJ(mtask.has_ಅಂಚು_ಗಮ್ಯ(ತುದಿ), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.ಗಾತ್ರ_2() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

ವರ್ಗ ಕ್ರಮ_MTask_ನಕ್ಷೆ_Builder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // MEMBERS
    ಕ್ರಮ_MTask_ನಕ್ಷೆ& m_mtask_ನಕ್ಷೆ;  // Output OrderMTaskGraph

    // METHODS

    // Predicate function to determine what OrderMoveVertex to bypass when constructing the MTask
    // graph. The OrderMoveGraph is a bipartite graph of:
    // - 1. OrderMoveVertex instances containing logic via OrderLogicVertex
    //      (OrderMoveVertex::logicp() != nullptr)
    // - 2. OrderMoveVertex instances containing an (OrderVarVertex, domain) pair
    // The goal is to order the logic vertices. The second type of variable/domain vertices only
    // carry dependencies and are eventually discarded. In order to reduce the working set size,
    // we 'bypass' and not create LogicMTask vertices for some variable vertices, and instead add
    // the transitive dependencies directly, but only if adding the transitive edges directly does
    // not require more dependency edges than keeping the intermediate vertex. That is, we bypass a
    // variable vertex if fanIn * fanOut <= fanIn + fanOut. This is true if fanIn or fanOut are 1,
    // or if they are both 2. This can significantly reduce the initial size of OrderMTaskGraph.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ bypassOk(ಕ್ರಮ_Move_ಶೃಂಗ* mvtxp) {
        // Need to keep all logic vertices
        ಆದರೆ (mvtxp->logicp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Count fan-in, up to 3
        ಚಿಹ್ನೆರಹಿತ fan_ಒಳ = 0;
        ಸ್ವಯಂಚಾಲಿತ& ಒಳ_ಅಂಚುಗಳು = mvtxp->ಒಳ_ಅಂಚುಗಳು();
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = ಒಳ_ಅಂಚುಗಳು.begin(); it != ಒಳ_ಅಂಚುಗಳು.ಕೊನೆ(); ++it) {
            ಆದರೆ (++fan_ಒಳ == 3) ನಿಲ್ಲಿಸು;
        }
        // If fanIn no more than one, bypass
        ಆದರೆ (fan_ಒಳ <= 1) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Count fan-out, up to 3
        ಚಿಹ್ನೆರಹಿತ fan_ಹೊರಹೋಗು = 0;
        ಸ್ವಯಂಚಾಲಿತ& ಹೊರಹೋಗು_ಅಂಚುಗಳು = mvtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು();
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = ಹೊರಹೋಗು_ಅಂಚುಗಳು.begin(); it != ಹೊರಹೋಗು_ಅಂಚುಗಳು.ಕೊನೆ(); ++it) {
            ಆದರೆ (++fan_ಹೊರಹೋಗು == 3) ನಿಲ್ಲಿಸು;
        }
        // If fan-out no more than one, bypass
        ಆದರೆ (fan_ಹೊರಹೋಗು <= 1) ಹಿಂತಿರುಗಿ ನಿಜ;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        ಹಿಂತಿರುಗಿ fan_ಒಳ + fan_ಹೊರಹೋಗು == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(LogicMTask* srcp, LogicMTask* dstp) {
        ಆದರೆ (srcp->has_ಅಂಚು_ಗಮ್ಯ(dstp)) ಹಿಂತಿರುಗಿ;  // Don't create redundant edges.
        m_mtask_ನಕ್ಷೆ.ಕೂಡಿಸು_ಅಂಚು(srcp, dstp);
    }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಕ್ರಮ_MTask_ನಕ್ಷೆ_Builder(ಕ್ರಮ_MTask_ನಕ್ಷೆ& mtask_ನಕ್ಷೆ)
        : m_mtask_ನಕ್ಷೆ{mtask_ನಕ್ಷೆ} {

        // Create the LogicMTasks for each OrderMoveVertex
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : mtask_ನಕ್ಷೆ.move_ನಕ್ಷೆ().vertices()) {
            ಕ್ರಮ_Move_ಶೃಂಗ& mVtx = static_cast<ಕ್ರಮ_Move_ಶೃಂಗ&>(vtx);
            ಆದರೆ (bypassOk(&mVtx)) {
                mVtx.userp(ಶೂನ್ಯಸೂಚಕ);  // Set to nullptr to mark as bypassed
            } ಇಲ್ಲದಿದ್ದರೆ {
                mVtx.userp(ಹೊಸ LogicMTask{mtask_ನಕ್ಷೆ, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& ನಮೂದು = *mtask_ನಕ್ಷೆ.entryp();
        LogicMTask& ನಿರ್ಗಮಿಸು = *mtask_ನಕ್ಷೆ.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : mtask_ನಕ್ಷೆ.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            ಆದರೆ (VL_UNLIKELY((&mtask == &ನಮೂದು) || (&mtask == &ನಿರ್ಗಮಿಸು))) ಮುಂದುವರಿ;

            ಕ್ರಮ_Move_ಶೃಂಗ::List& ಶೃಂಗ_List = mtask.ಶೃಂಗ_List();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(ಶೃಂಗ_List.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = ಶೃಂಗ_List.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& d_ಅಂಚು : mVtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ತುದಿ = d_ಅಂಚು.ತುದಿ();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                ಆದರೆ (LogicMTask* ಸ್ಥಿರಾಂಕ otherp = static_cast<LogicMTask*>(ತುದಿ->userp())) {
                    ಕೂಡಿಸು_ಅಂಚು(&mtask, otherp);
                    ಮುಂದುವರಿ;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& t_ಅಂಚು : ತುದಿ->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                    LogicMTask* ಸ್ಥಿರಾಂಕ transp = static_cast<LogicMTask*>(t_ಅಂಚು.ತುದಿ()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    ಕೂಡಿಸು_ಅಂಚು(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : mtask_ನಕ್ಷೆ.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            ಆದರೆ (VL_UNLIKELY((&mtask == &ನಮೂದು) || (&mtask == &ನಿರ್ಗಮಿಸು))) ಮುಂದುವರಿ;
            // Add the entry/exit edges if not otherwise connected
            ಆದರೆ (mtask.ಒಳ_ಖಾಲಿಗಳು()) ಕೂಡಿಸು_ಅಂಚು(&ನಮೂದು, &mtask);
            ಆದರೆ (mtask.ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) ಕೂಡಿಸು_ಅಂಚು(&mtask, &ನಿರ್ಗಮಿಸು);
        }
    }
    ~ಕ್ರಮ_MTask_ನಕ್ಷೆ_Builder() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(ಕ್ರಮ_MTask_ನಕ್ಷೆ_Builder);
    VL_UNMOVABLE(ಕ್ರಮ_MTask_ನಕ್ಷೆ_Builder);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(ಕ್ರಮ_MTask_ನಕ್ಷೆ& mtask_ನಕ್ಷೆ) { ಕ್ರಮ_MTask_ನಕ್ಷೆ_Builder{mtask_ನಕ್ಷೆ}; }
};

std::unique_ptr<ಕ್ರಮ_MTask_ನಕ್ಷೆ> ಕ್ರಮ_MTask_ನಕ್ಷೆ::build(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ) {
    std::unique_ptr<ಕ್ರಮ_MTask_ನಕ್ಷೆ> resp{ಹೊಸ ಕ್ರಮ_MTask_ನಕ್ಷೆ{move_ನಕ್ಷೆ}};
    ಕ್ರಮ_MTask_ನಕ್ಷೆ_Builder::apply(*resp);
    resp->validate();
    ಹಿಂತಿರುಗಿ resp;
}
