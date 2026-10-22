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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderMTaskGraph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3InstrCount.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_ਅਗਲਾ_Id = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* mVtxp)
    : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{&ਗ੍ਰਾਫ} {
    UASSERT(s_ਅਗਲਾ_Id < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    ਜੇ (!mVtxp) ਵਾਪਸ;
    m_mVertices.link_ਪਿੱਠ(mVtxp);
    ਜੇ (ਅਚਲ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ olvp = mVtxp->logicp()) {
        m_ਲਾਗਤ += V3_Instr_ਗਿਣਤੀ::ਗਿਣਤੀ_2(olvp->nodep(), ਸੱਚ);
    }
}

//######################################################################
// OrderMTaskGraph

ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ)
    : m_move_ਗ੍ਰਾਫ{move_ਗ੍ਰਾਫ}
    , m_entryp{ਨਵਾਂ LogicMTask{*ਇਹ, ਖਾਲੀ_ਸੂਚਕ}}
    , m_exitp{ਨਵਾਂ LogicMTask{*ਇਹ, ਖਾਲੀ_ਸੂਚਕ}}
    , m_slowAsserts{v3Global.opt.debug_ਹਿੱਸੇ_ਕਰੋ()} {}

ਤਾਰਕਿਕ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::ਰਸਤਾ_Exists_Impl(LogicMTask* fromp, LogicMTask* ਸਿਖਰ,
                                     ਅਚਲ MTask_ਕਿਨਾਰਾ* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    ਜੇ (fromp == ਸਿਖਰ) ਵਾਪਸ ਸੱਚ;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    ਜੇ (fromp->cpExclusive<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>() < ਸਿਖਰ->cpInclusive<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>()) {
        ਵਾਪਸ ਝੂਠ;
    }
    ਜੇ (fromp->cpInclusive<ਗ੍ਰਾਫ_Way::FORWARD>() > ਸਿਖਰ->cpExclusive<ਗ੍ਰਾਫ_Way::FORWARD>()) {
        ਵਾਪਸ ਝੂਠ;
    }

    // Recursively look for a path
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& follow : fromp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        ਜੇ (&follow == excludedEdgep) ਜਾਰੀ;
        LogicMTask* ਅਚਲ nextp = static_cast<LogicMTask*>(follow.ਸਿਖਰ());
        // Don't visit the same MTask twice in the same search.
        ਜੇ (nextp->m_generation == m_currentGeneration) ਜਾਰੀ;
        ਜੇ (ਰਸਤਾ_Exists_Impl(nextp, ਸਿਖਰ, ਖਾਲੀ_ਸੂਚਕ)) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}

ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::propagate_ਧੱਕੋ(LogicMTask* mtaskp) {
    constexpr ਗ੍ਰਾਫ_Way way{N_Way};
    constexpr ਗ੍ਰਾਫ_Way inv{way.invert()};
    ਅਚਲ uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਗ੍ਰਾਫ_ਕਿਨਾਰਾ : mtaskp->ਕਿਨਾਰੇ<way>()) {
        MTask_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ = static_cast<MTask_ਕਿਨਾਰਾ&>(ਗ੍ਰਾਫ_ਕਿਨਾਰਾ);

        LogicMTask* ਅਚਲ relativep = ਕਿਨਾਰਾ.furtherMTaskp<N_Way>();
        ਕਿਨਾਰਾ_ਢੇਰ::ਗੰਢ_2& ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ = ਕਿਨਾਰਾ.m_ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ[inv];
        ਜੇ (inclusiveCp > ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ.ਕੁੰਜੀ().m_cp) {
            relativep->m_ਕਿਨਾਰਾ_ਢੇਰ[inv].increase_ਕੁੰਜੀ(&ਕਿਨਾਰਾ_ਢੇਰ_ਗੰਢ, inclusiveCp);
        }

        ਅਚਲ uint64_t relativeCp = relativep->cpExclusive<way>();

        ਜੇ (relativeCp >= inclusiveCp) ਜਾਰੀ;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ਅਚਲ uint64_t increment = inclusiveCp - relativeCp;

        Propagate_Pending_ਢੇਰ::ਗੰਢ_2*& pendingNodepRef = relativep->m_propagate_ਢੇਰ_Nodep;
        ਜੇ (Propagate_Pending_ਢੇਰ::ਗੰਢ_2* ਅਚਲ nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            ਜੇ (increment > nodep->ਕੁੰਜੀ().m_increment) {
                m_pending_ਢੇਰ.increase_ਕੁੰਜੀ(nodep, increment);
            }
            ਜਾਰੀ;
        }

        // Add to heap
        Propagate_Pending_ਢੇਰ::ਗੰਢ_2* ਅਚਲ nodep = m_pending_ਗੰਢ_Pool.alloc();
        pendingNodepRef = nodep;
        m_pending_ਢੇਰ.ਪਾਓ(nodep, {increment, relativep->id(), relativep});
    }
}

ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::propagateResolve() {
    constexpr ਗ੍ਰਾਫ_Way way{N_Way};
    constexpr ਗ੍ਰਾਫ_Way inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    ਜਦੋਂ (!m_pending_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        // Pop max element from heap
        Propagate_Pending_ਢੇਰ::ਗੰਢ_2* ਅਚਲ maxp = m_pending_ਢੇਰ.max();
        m_pending_ਢੇਰ.remove(maxp);
        // Pick up values
        LogicMTask* ਅਚਲ mtaskp = maxp->ਕੁੰਜੀ().m_mtaskp;
        ਅਚਲ uint64_t cpGrowBy = maxp->ਕੁੰਜੀ().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_ਗੰਢ_Pool.ਮੁਕਤ(maxp);
        mtaskp->m_propagate_ਢੇਰ_Nodep = ਖਾਲੀ_ਸੂਚਕ;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ਅਚਲ uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        ਜੇ (VL_UNLIKELY(m_slowAsserts)) {
            ਅਚਲ uint64_t ਕਿਨਾਰਾ_Cp = mtaskp->m_ਕਿਨਾਰਾ_ਢੇਰ[inv].max()->ਕੁੰਜੀ().m_cp;
            UASSERT_OBJ(ਕਿਨਾਰਾ_Cp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_ਧੱਕੋ<N_Way>(mtaskp);
    }
}

uint64_t ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::ਕੁੱਲ_ਲਾਗਤ() ਅਚਲ {
    uint64_t ਲਾਗਤ = 0;
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) ਲਾਗਤ += static_cast<ਅਚਲ LogicMTask&>(vtx).ਲਾਗਤ();
    ਵਾਪਸ ਲਾਗਤ;
}

ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::ਜੋੜੋ_ਕਿਨਾਰਾ(LogicMTask* fromp, LogicMTask* ਸਿਖਰ) {
    UASSERT_OBJ(fromp != ਸਿਖਰ, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->has_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ(ਸਿਖਰ), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    ਨਵਾਂ MTask_ਕਿਨਾਰਾ{ਇਹ, fromp, ਸਿਖਰ};

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
    ਜੇ (fromp->cpInclusive<ਗ੍ਰਾਫ_Way::FORWARD>() > ਸਿਖਰ->cpExclusive<ਗ੍ਰਾਫ_Way::FORWARD>()) {
        propagate<ਗ੍ਰਾਫ_Way::FORWARD>(fromp);
    }
    ਜੇ (ਸਿਖਰ->cpInclusive<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>() > fromp->cpExclusive<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>()) {
        propagate<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(ਸਿਖਰ);
    }
}

ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::ਮਿਲਾਓ_MTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    ਜਦੋਂ (MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = static_cast<MTask_ਕਿਨਾਰਾ*>(donorp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp())) {
        LogicMTask* ਅਚਲ relativep = edgep->ਮੰਜ਼ਿਲ_MTaskp();

        relativep->remove_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(edgep);

        ਜੇ (relativep == recipientp || recipientp->has_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ਨਹੀਂਤਾਂ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->ਜੋੜੋ_Dependent(relativep);
            recipientp->steal_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::FORWARD>(edgep);
            relativep->ਜੋੜੋ_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(edgep);
        }
    }

    // Process incoming edges of donor
    ਜਦੋਂ (MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = static_cast<MTask_ਕਿਨਾਰਾ*>(donorp->ਵਿੱਚ_ਕਿਨਾਰੇ().frontp())) {
        LogicMTask* ਅਚਲ relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->remove_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::FORWARD>(edgep);

        ਜੇ (relativep == recipientp || relativep->has_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ਨਹੀਂਤਾਂ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_ਸਿਖਰ(recipientp);
            relativep->ਜੋੜੋ_Dependent(recipientp);
            relativep->ਜੋੜੋ_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::FORWARD>(edgep);
            recipientp->steal_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(edgep);
        }
    }

    // Move the contents of the donor into the recipient, update its cost
    recipientp->m_mVertices.splice(recipientp->m_mVertices.ਸਿਰਾ(), donorp->m_mVertices);
    recipientp->m_ਲਾਗਤ += donorp->m_ਲਾਗਤ;

    // The recipient now holds all edges of the merged MTask, and the critical paths of all its
    // relatives are still up to date, so the critical paths implied by its edges are the critical
    // paths of the merged MTask.
    ਅਚਲ uint64_t newCpFwd = recipientp->cp_Exclusive_From_ਕਿਨਾਰੇ<ਗ੍ਰਾਫ_Way::FORWARD>();
    ਅਚਲ uint64_t newCpRev = recipientp->cp_Exclusive_From_ਕਿਨਾਰੇ<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<ਗ੍ਰਾਫ_Way::FORWARD>(newCpFwd);
    propagate<ਗ੍ਰਾਫ_Way::FORWARD>(recipientp);
    recipientp->cpExclusive<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(newCpRev);
    propagate<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(ਇਹ), donorp);
}

ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ ਗ੍ਰਾਫ_Edgep : vtx.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
            MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = static_cast<MTask_ਕਿਨਾਰਾ*>(ਗ੍ਰਾਫ_Edgep);
            LogicMTask* ਅਚਲ fromp = edgep->fromMTaskp();
            LogicMTask* ਅਚਲ ਸਿਖਰ = edgep->ਮੰਜ਼ਿਲ_MTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            ਜੇ (!ਰਸਤਾ_Exists(fromp, ਸਿਖਰ, edgep)) ਜਾਰੀ;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(ਸਿਖਰ);
            fromp->remove_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::FORWARD>(edgep);
            ਸਿਖਰ->remove_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::remove_ਖਾਲੀ_ਥਾਵਾਂ_MTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : vertices().unlinkable()) {
        LogicMTask* ਅਚਲ mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        ਜੇ (mtaskp == m_entryp || mtaskp == m_exitp) ਜਾਰੀ;

        // Keep any MTask that holds logic
        ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ = ਸੱਚ;
        ਲਈ (ਅਚਲ ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼& mVtx : mtaskp->ਸ਼ੀਰਸ਼_List()) {
            ਜੇ (mVtx.logicp()) {
                ਖਾਲੀ_ਥਾਵਾਂ = ਝੂਠ;
                ਰੋਕੋ;
            }
        }
        ਜੇ (!ਖਾਲੀ_ਥਾਵਾਂ) ਜਾਰੀ;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->ਲਾਗਤ(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਵਿੱਚ_ਕਿਨਾਰਾ : mtaskp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            LogicMTask* ਅਚਲ fromp = static_cast<MTask_ਕਿਨਾਰਾ&>(ਵਿੱਚ_ਕਿਨਾਰਾ).fromMTaskp();
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਬਾਹਰਲਾ_ਕਿਨਾਰਾ : mtaskp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                LogicMTask* ਅਚਲ ਸਿਖਰ = static_cast<MTask_ਕਿਨਾਰਾ&>(ਬਾਹਰਲਾ_ਕਿਨਾਰਾ).ਮੰਜ਼ਿਲ_MTaskp();
                ਜੇ (!fromp->has_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ(ਸਿਖਰ)) ਜੋੜੋ_ਕਿਨਾਰਾ(fromp, ਸਿਖਰ);
            }
        }

        // Remove incoming edges of 'mtaskp'
        ਜਦੋਂ (MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = static_cast<MTask_ਕਿਨਾਰਾ*>(mtaskp->ਵਿੱਚ_ਕਿਨਾਰੇ().frontp())) {
            LogicMTask* ਅਚਲ relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->remove_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        ਜਦੋਂ (MTask_ਕਿਨਾਰਾ* ਅਚਲ edgep = static_cast<MTask_ਕਿਨਾਰਾ*>(mtaskp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp())) {
            LogicMTask* ਅਚਲ relativep = edgep->ਮੰਜ਼ਿਲ_MTaskp();
            relativep->remove_Relative_ਕਿਨਾਰਾ<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(ਇਹ), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::validateWay() ਅਚਲ {
    constexpr ਗ੍ਰਾਫ_Way way{N_Way};
    constexpr ਗ੍ਰਾਫ_Way inv = way.invert();
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) {
        ਅਚਲ LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cp_ਲਾਗਤ = 0;
        std::unordered_ਸੈੱਟ<ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> relatives;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਗ੍ਰਾਫ_ਕਿਨਾਰਾ : mtask.ਕਿਨਾਰੇ<inv>()) {
            ਅਚਲ MTask_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ = *ਗ੍ਰਾਫ_ਕਿਨਾਰਾ.as<MTask_ਕਿਨਾਰਾ>();
            ਅਚਲ LogicMTask& relative = *(ਕਿਨਾਰਾ.furtherp<inv>()->ਸਾਂਚਾ as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(ਕਿਨਾਰਾ.ਭਾਰ_2() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ਅਚਲ ਤਾਰਕਿਕ ਪਹਿਲਾ = relatives.ਪਾਓ(&relative).ਦੂਜਾ;
            UASSERT_OBJ(ਪਹਿਲਾ, &mtask, "Should be no redundant edges in MTask graph");
            ਅਚਲ uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(ਕਿਨਾਰਾ.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(ਕਿਨਾਰਾ.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cp_ਲਾਗਤ = std::max(cp_ਲਾਗਤ, inclusiveCp);
        }
        ਅਚਲ uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cp_ਲਾਗਤ, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cp_Exclusive_From_ਕਿਨਾਰੇ<N_Way>() == cp_ਲਾਗਤ, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

ਖਾਲੀ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::validate() ਅਚਲ {
    ਜੇ (!m_slowAsserts) ਵਾਪਸ;

    validateWay<ਗ੍ਰਾਫ_Way::FORWARD>();
    validateWay<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>();

    // Check the dependents set of each MTask agrees with its out-edges
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : vertices()) {
        ਅਚਲ LogicMTask& mtask = *vtx.as<LogicMTask>();
        ਮਾਪ_t nDependents = 0;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਗ੍ਰਾਫ_ਕਿਨਾਰਾ : mtask.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            LogicMTask* ਅਚਲ ਸਿਖਰ = ਗ੍ਰਾਫ_ਕਿਨਾਰਾ.as<MTask_ਕਿਨਾਰਾ>()->ਮੰਜ਼ਿਲ_MTaskp();
            UASSERT_OBJ(mtask.has_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ(ਸਿਖਰ), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.ਮਾਪ() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

ਵਰਗ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ_Builder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // MEMBERS
    ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& m_mtask_ਗ੍ਰਾਫ;  // Output OrderMTaskGraph

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
    ਸਥਿਰ ਤਾਰਕਿਕ bypassOk(ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* mvtxp) {
        // Need to keep all logic vertices
        ਜੇ (mvtxp->logicp()) ਵਾਪਸ ਝੂਠ;
        // Count fan-in, up to 3
        ਬਿਨਾਨਿਸ਼ਾਨੀ fan_ਵਿੱਚ = 0;
        ਸਵੈਚਾਲਿਤ& ਵਿੱਚ_ਕਿਨਾਰੇ = mvtxp->ਵਿੱਚ_ਕਿਨਾਰੇ();
        ਲਈ (ਸਵੈਚਾਲਿਤ it = ਵਿੱਚ_ਕਿਨਾਰੇ.begin(); it != ਵਿੱਚ_ਕਿਨਾਰੇ.ਸਿਰਾ(); ++it) {
            ਜੇ (++fan_ਵਿੱਚ == 3) ਰੋਕੋ;
        }
        // If fanIn no more than one, bypass
        ਜੇ (fan_ਵਿੱਚ <= 1) ਵਾਪਸ ਸੱਚ;
        // Count fan-out, up to 3
        ਬਿਨਾਨਿਸ਼ਾਨੀ fan_ਬਾਹਰਲਾ = 0;
        ਸਵੈਚਾਲਿਤ& ਬਾਹਰਲਾ_ਕਿਨਾਰੇ = mvtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ();
        ਲਈ (ਸਵੈਚਾਲਿਤ it = ਬਾਹਰਲਾ_ਕਿਨਾਰੇ.begin(); it != ਬਾਹਰਲਾ_ਕਿਨਾਰੇ.ਸਿਰਾ(); ++it) {
            ਜੇ (++fan_ਬਾਹਰਲਾ == 3) ਰੋਕੋ;
        }
        // If fan-out no more than one, bypass
        ਜੇ (fan_ਬਾਹਰਲਾ <= 1) ਵਾਪਸ ਸੱਚ;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        ਵਾਪਸ fan_ਵਿੱਚ + fan_ਬਾਹਰਲਾ == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(LogicMTask* srcp, LogicMTask* dstp) {
        ਜੇ (srcp->has_ਕਿਨਾਰਾ_ਮੰਜ਼ਿਲ(dstp)) ਵਾਪਸ;  // Don't create redundant edges.
        m_mtask_ਗ੍ਰਾਫ.ਜੋੜੋ_ਕਿਨਾਰਾ(srcp, dstp);
    }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ_Builder(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& mtask_ਗ੍ਰਾਫ)
        : m_mtask_ਗ੍ਰਾਫ{mtask_ਗ੍ਰਾਫ} {

        // Create the LogicMTasks for each OrderMoveVertex
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : mtask_ਗ੍ਰਾਫ.move_ਗ੍ਰਾਫ().vertices()) {
            ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼& mVtx = static_cast<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼&>(vtx);
            ਜੇ (bypassOk(&mVtx)) {
                mVtx.userp(ਖਾਲੀ_ਸੂਚਕ);  // Set to nullptr to mark as bypassed
            } ਨਹੀਂਤਾਂ {
                mVtx.userp(ਨਵਾਂ LogicMTask{mtask_ਗ੍ਰਾਫ, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& ਇੰਦਰਾਜ = *mtask_ਗ੍ਰਾਫ.entryp();
        LogicMTask& ਬਾਹਰ_ਨਿਕਲੋ = *mtask_ਗ੍ਰਾਫ.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : mtask_ਗ੍ਰਾਫ.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            ਜੇ (VL_UNLIKELY((&mtask == &ਇੰਦਰਾਜ) || (&mtask == &ਬਾਹਰ_ਨਿਕਲੋ))) ਜਾਰੀ;

            ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼::List& ਸ਼ੀਰਸ਼_List = mtask.ਸ਼ੀਰਸ਼_List();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(ਸ਼ੀਰਸ਼_List.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = ਸ਼ੀਰਸ਼_List.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& d_ਕਿਨਾਰਾ : mVtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ ਸਿਖਰ = d_ਕਿਨਾਰਾ.ਸਿਖਰ();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                ਜੇ (LogicMTask* ਅਚਲ otherp = static_cast<LogicMTask*>(ਸਿਖਰ->userp())) {
                    ਜੋੜੋ_ਕਿਨਾਰਾ(&mtask, otherp);
                    ਜਾਰੀ;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& t_ਕਿਨਾਰਾ : ਸਿਖਰ->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                    LogicMTask* ਅਚਲ transp = static_cast<LogicMTask*>(t_ਕਿਨਾਰਾ.ਸਿਖਰ()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    ਜੋੜੋ_ਕਿਨਾਰਾ(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : mtask_ਗ੍ਰਾਫ.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            ਜੇ (VL_UNLIKELY((&mtask == &ਇੰਦਰਾਜ) || (&mtask == &ਬਾਹਰ_ਨਿਕਲੋ))) ਜਾਰੀ;
            // Add the entry/exit edges if not otherwise connected
            ਜੇ (mtask.ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) ਜੋੜੋ_ਕਿਨਾਰਾ(&ਇੰਦਰਾਜ, &mtask);
            ਜੇ (mtask.ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) ਜੋੜੋ_ਕਿਨਾਰਾ(&mtask, &ਬਾਹਰ_ਨਿਕਲੋ);
        }
    }
    ~ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ_Builder() = ਮੂਲ;
    VL_UNCOPYABLE(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ_Builder);
    VL_UNMOVABLE(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ_Builder);

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& mtask_ਗ੍ਰਾਫ) { ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ_Builder{mtask_ਗ੍ਰਾਫ}; }
};

std::unique_ptr<ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ> ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::build(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ) {
    std::unique_ptr<ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ> resp{ਨਵਾਂ ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ{move_ਗ੍ਰਾਫ}};
    ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ_Builder::apply(*resp);
    resp->validate();
    ਵਾਪਸ resp;
}
