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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3OrderMTaskGraph.h"

#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3InstrCount.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_తదుపరి_Id = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(క్రమం_MTask_లేఖాచిత్రం& లేఖాచిత్రం, క్రమం_Move_శీర్షం* mVtxp)
    : V3_లేఖాచిత్రం_శీర్షం{&లేఖాచిత్రం} {
    UASSERT(s_తదుపరి_Id < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    ఒకవేళ (!mVtxp) తిరిగి;
    m_mVertices.link_వెనుక(mVtxp);
    ఒకవేళ (స్థిరరాశి క్రమం_Logic_శీర్షం* స్థిరరాశి olvp = mVtxp->logicp()) {
        m_ఖర్చు += V3_Instr_లెక్క::లెక్క(olvp->nodep(), నిజం);
    }
}

//######################################################################
// OrderMTaskGraph

క్రమం_MTask_లేఖాచిత్రం::క్రమం_MTask_లేఖాచిత్రం(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం)
    : m_move_లేఖాచిత్రం{move_లేఖాచిత్రం}
    , m_entryp{కొత్త LogicMTask{*ఇది, శూన్యసూచిక}}
    , m_exitp{కొత్త LogicMTask{*ఇది, శూన్యసూచిక}}
    , m_slowAsserts{v3Global.opt.debug_విభజన()} {}

తార్కిక క్రమం_MTask_లేఖాచిత్రం::మార్గం_Exists_Impl(LogicMTask* fromp, LogicMTask* శిఖరం,
                                     స్థిరరాశి MTask_అంచు* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    ఒకవేళ (fromp == శిఖరం) తిరిగి నిజం;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    ఒకవేళ (fromp->cpExclusive<లేఖాచిత్రం_Way::తిరగేయి_2>() < శిఖరం->cpInclusive<లేఖాచిత్రం_Way::తిరగేయి_2>()) {
        తిరిగి అబద్ధం;
    }
    ఒకవేళ (fromp->cpInclusive<లేఖాచిత్రం_Way::FORWARD>() > శిఖరం->cpExclusive<లేఖాచిత్రం_Way::FORWARD>()) {
        తిరిగి అబద్ధం;
    }

    // Recursively look for a path
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& follow : fromp->బహిర్గతం_అంచులు()) {
        ఒకవేళ (&follow == excludedEdgep) కొనసాగు;
        LogicMTask* స్థిరరాశి nextp = static_cast<LogicMTask*>(follow.శిఖరం());
        // Don't visit the same MTask twice in the same search.
        ఒకవేళ (nextp->m_generation == m_currentGeneration) కొనసాగు;
        ఒకవేళ (మార్గం_Exists_Impl(nextp, శిఖరం, శూన్యసూచిక)) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}

మూస <లేఖాచిత్రం_Way::en N_Way>
శూన్య క్రమం_MTask_లేఖాచిత్రం::propagate_నెట్టు(LogicMTask* mtaskp) {
    constexpr లేఖాచిత్రం_Way way{N_Way};
    constexpr లేఖాచిత్రం_Way inv{way.invert()};
    స్థిరరాశి uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    కొరకు (V3_లేఖాచిత్రం_అంచు& లేఖాచిత్రం_అంచు : mtaskp->అంచులు<way>()) {
        MTask_అంచు& అంచు = static_cast<MTask_అంచు&>(లేఖాచిత్రం_అంచు);

        LogicMTask* స్థిరరాశి relativep = అంచు.furtherMTaskp<N_Way>();
        అంచు_కుప్ప::కణుపు_2& అంచు_కుప్ప_కణుపు = అంచు.m_అంచు_కుప్ప_కణుపు[inv];
        ఒకవేళ (inclusiveCp > అంచు_కుప్ప_కణుపు.కీలకం().m_cp) {
            relativep->m_అంచు_కుప్ప[inv].increase_కీలకం(&అంచు_కుప్ప_కణుపు, inclusiveCp);
        }

        స్థిరరాశి uint64_t relativeCp = relativep->cpExclusive<way>();

        ఒకవేళ (relativeCp >= inclusiveCp) కొనసాగు;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        స్థిరరాశి uint64_t increment = inclusiveCp - relativeCp;

        Propagate_Pending_కుప్ప::కణుపు_2*& pendingNodepRef = relativep->m_propagate_కుప్ప_Nodep;
        ఒకవేళ (Propagate_Pending_కుప్ప::కణుపు_2* స్థిరరాశి nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            ఒకవేళ (increment > nodep->కీలకం().m_increment) {
                m_pending_కుప్ప.increase_కీలకం(nodep, increment);
            }
            కొనసాగు;
        }

        // Add to heap
        Propagate_Pending_కుప్ప::కణుపు_2* స్థిరరాశి nodep = m_pending_కణుపు_Pool.alloc();
        pendingNodepRef = nodep;
        m_pending_కుప్ప.చొప్పించు(nodep, {increment, relativep->id(), relativep});
    }
}

మూస <లేఖాచిత్రం_Way::en N_Way>
శూన్య క్రమం_MTask_లేఖాచిత్రం::propagateResolve() {
    constexpr లేఖాచిత్రం_Way way{N_Way};
    constexpr లేఖాచిత్రం_Way inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    అయితే (!m_pending_కుప్ప.ఖాళీలు()) {
        // Pop max element from heap
        Propagate_Pending_కుప్ప::కణుపు_2* స్థిరరాశి maxp = m_pending_కుప్ప.max();
        m_pending_కుప్ప.remove(maxp);
        // Pick up values
        LogicMTask* స్థిరరాశి mtaskp = maxp->కీలకం().m_mtaskp;
        స్థిరరాశి uint64_t cpGrowBy = maxp->కీలకం().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_కణుపు_Pool.విడుదల(maxp);
        mtaskp->m_propagate_కుప్ప_Nodep = శూన్యసూచిక;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        స్థిరరాశి uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        ఒకవేళ (VL_UNLIKELY(m_slowAsserts)) {
            స్థిరరాశి uint64_t అంచు_Cp = mtaskp->m_అంచు_కుప్ప[inv].max()->కీలకం().m_cp;
            UASSERT_OBJ(అంచు_Cp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_నెట్టు<N_Way>(mtaskp);
    }
}

uint64_t క్రమం_MTask_లేఖాచిత్రం::మొత్తం_ఖర్చు() స్థిరరాశి {
    uint64_t ఖర్చు = 0;
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) ఖర్చు += static_cast<స్థిరరాశి LogicMTask&>(vtx).ఖర్చు();
    తిరిగి ఖర్చు;
}

శూన్య క్రమం_MTask_లేఖాచిత్రం::కూడు_అంచు(LogicMTask* fromp, LogicMTask* శిఖరం) {
    UASSERT_OBJ(fromp != శిఖరం, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->has_అంచు_గమ్యం(శిఖరం), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    కొత్త MTask_అంచు{ఇది, fromp, శిఖరం};

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
    ఒకవేళ (fromp->cpInclusive<లేఖాచిత్రం_Way::FORWARD>() > శిఖరం->cpExclusive<లేఖాచిత్రం_Way::FORWARD>()) {
        propagate<లేఖాచిత్రం_Way::FORWARD>(fromp);
    }
    ఒకవేళ (శిఖరం->cpInclusive<లేఖాచిత్రం_Way::తిరగేయి_2>() > fromp->cpExclusive<లేఖాచిత్రం_Way::తిరగేయి_2>()) {
        propagate<లేఖాచిత్రం_Way::తిరగేయి_2>(శిఖరం);
    }
}

శూన్య క్రమం_MTask_లేఖాచిత్రం::విలీనం_MTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    అయితే (MTask_అంచు* స్థిరరాశి edgep = static_cast<MTask_అంచు*>(donorp->బహిర్గతం_అంచులు().frontp())) {
        LogicMTask* స్థిరరాశి relativep = edgep->గమ్యం_MTaskp();

        relativep->remove_Relative_అంచు<లేఖాచిత్రం_Way::తిరగేయి_2>(edgep);

        ఒకవేళ (relativep == recipientp || recipientp->has_అంచు_గమ్యం(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } లేకపోతే {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->కూడు_Dependent(relativep);
            recipientp->steal_Relative_అంచు<లేఖాచిత్రం_Way::FORWARD>(edgep);
            relativep->కూడు_Relative_అంచు<లేఖాచిత్రం_Way::తిరగేయి_2>(edgep);
        }
    }

    // Process incoming edges of donor
    అయితే (MTask_అంచు* స్థిరరాశి edgep = static_cast<MTask_అంచు*>(donorp->లోపల_అంచులు().frontp())) {
        LogicMTask* స్థిరరాశి relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->remove_Relative_అంచు<లేఖాచిత్రం_Way::FORWARD>(edgep);

        ఒకవేళ (relativep == recipientp || relativep->has_అంచు_గమ్యం(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } లేకపోతే {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_శిఖరం(recipientp);
            relativep->కూడు_Dependent(recipientp);
            relativep->కూడు_Relative_అంచు<లేఖాచిత్రం_Way::FORWARD>(edgep);
            recipientp->steal_Relative_అంచు<లేఖాచిత్రం_Way::తిరగేయి_2>(edgep);
        }
    }

    // Move the contents of the donor into the recipient, update its cost
    recipientp->m_mVertices.splice(recipientp->m_mVertices.చివర(), donorp->m_mVertices);
    recipientp->m_ఖర్చు += donorp->m_ఖర్చు;

    // The recipient now holds all edges of the merged MTask, and the critical paths of all its
    // relatives are still up to date, so the critical paths implied by its edges are the critical
    // paths of the merged MTask.
    స్థిరరాశి uint64_t newCpFwd = recipientp->cp_Exclusive_From_అంచులు<లేఖాచిత్రం_Way::FORWARD>();
    స్థిరరాశి uint64_t newCpRev = recipientp->cp_Exclusive_From_అంచులు<లేఖాచిత్రం_Way::తిరగేయి_2>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<లేఖాచిత్రం_Way::FORWARD>(newCpFwd);
    propagate<లేఖాచిత్రం_Way::FORWARD>(recipientp);
    recipientp->cpExclusive<లేఖాచిత్రం_Way::తిరగేయి_2>(newCpRev);
    propagate<లేఖాచిత్రం_Way::తిరగేయి_2>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(ఇది), donorp);
}

శూన్య క్రమం_MTask_లేఖాచిత్రం::remove_సంక్రమణ_అంచులు() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) {
        కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి లేఖాచిత్రం_Edgep : vtx.బహిర్గతం_అంచులు().unlinkable()) {
            MTask_అంచు* స్థిరరాశి edgep = static_cast<MTask_అంచు*>(లేఖాచిత్రం_Edgep);
            LogicMTask* స్థిరరాశి fromp = edgep->fromMTaskp();
            LogicMTask* స్థిరరాశి శిఖరం = edgep->గమ్యం_MTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            ఒకవేళ (!మార్గం_Exists(fromp, శిఖరం, edgep)) కొనసాగు;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(శిఖరం);
            fromp->remove_Relative_అంచు<లేఖాచిత్రం_Way::FORWARD>(edgep);
            శిఖరం->remove_Relative_అంచు<లేఖాచిత్రం_Way::తిరగేయి_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

శూన్య క్రమం_MTask_లేఖాచిత్రం::remove_ఖాళీలు_MTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : vertices().unlinkable()) {
        LogicMTask* స్థిరరాశి mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        ఒకవేళ (mtaskp == m_entryp || mtaskp == m_exitp) కొనసాగు;

        // Keep any MTask that holds logic
        తార్కిక ఖాళీలు = నిజం;
        కొరకు (స్థిరరాశి క్రమం_Move_శీర్షం& mVtx : mtaskp->శీర్షం_List()) {
            ఒకవేళ (mVtx.logicp()) {
                ఖాళీలు = అబద్ధం;
                ఆపు;
            }
        }
        ఒకవేళ (!ఖాళీలు) కొనసాగు;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->ఖర్చు(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        కొరకు (V3_లేఖాచిత్రం_అంచు& లోపల_అంచు : mtaskp->లోపల_అంచులు()) {
            LogicMTask* స్థిరరాశి fromp = static_cast<MTask_అంచు&>(లోపల_అంచు).fromMTaskp();
            కొరకు (V3_లేఖాచిత్రం_అంచు& బహిర్గతం_అంచు : mtaskp->బహిర్గతం_అంచులు()) {
                LogicMTask* స్థిరరాశి శిఖరం = static_cast<MTask_అంచు&>(బహిర్గతం_అంచు).గమ్యం_MTaskp();
                ఒకవేళ (!fromp->has_అంచు_గమ్యం(శిఖరం)) కూడు_అంచు(fromp, శిఖరం);
            }
        }

        // Remove incoming edges of 'mtaskp'
        అయితే (MTask_అంచు* స్థిరరాశి edgep = static_cast<MTask_అంచు*>(mtaskp->లోపల_అంచులు().frontp())) {
            LogicMTask* స్థిరరాశి relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->remove_Relative_అంచు<లేఖాచిత్రం_Way::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        అయితే (MTask_అంచు* స్థిరరాశి edgep = static_cast<MTask_అంచు*>(mtaskp->బహిర్గతం_అంచులు().frontp())) {
            LogicMTask* స్థిరరాశి relativep = edgep->గమ్యం_MTaskp();
            relativep->remove_Relative_అంచు<లేఖాచిత్రం_Way::తిరగేయి_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(ఇది), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
మూస <లేఖాచిత్రం_Way::en N_Way>
శూన్య క్రమం_MTask_లేఖాచిత్రం::validateWay() స్థిరరాశి {
    constexpr లేఖాచిత్రం_Way way{N_Way};
    constexpr లేఖాచిత్రం_Way inv = way.invert();
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) {
        స్థిరరాశి LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cp_ఖర్చు = 0;
        std::unordered_సమితి<స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*> relatives;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& లేఖాచిత్రం_అంచు : mtask.అంచులు<inv>()) {
            స్థిరరాశి MTask_అంచు& అంచు = *లేఖాచిత్రం_అంచు.as<MTask_అంచు>();
            స్థిరరాశి LogicMTask& relative = *(అంచు.furtherp<inv>()->మూస as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(అంచు.బరువు_2() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            స్థిరరాశి తార్కిక మొదటి = relatives.చొప్పించు(&relative).రెండవ;
            UASSERT_OBJ(మొదటి, &mtask, "Should be no redundant edges in MTask graph");
            స్థిరరాశి uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(అంచు.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(అంచు.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cp_ఖర్చు = std::max(cp_ఖర్చు, inclusiveCp);
        }
        స్థిరరాశి uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cp_ఖర్చు, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cp_Exclusive_From_అంచులు<N_Way>() == cp_ఖర్చు, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

శూన్య క్రమం_MTask_లేఖాచిత్రం::validate() స్థిరరాశి {
    ఒకవేళ (!m_slowAsserts) తిరిగి;

    validateWay<లేఖాచిత్రం_Way::FORWARD>();
    validateWay<లేఖాచిత్రం_Way::తిరగేయి_2>();

    // Check the dependents set of each MTask agrees with its out-edges
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : vertices()) {
        స్థిరరాశి LogicMTask& mtask = *vtx.as<LogicMTask>();
        కొలత_t nDependents = 0;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& లేఖాచిత్రం_అంచు : mtask.బహిర్గతం_అంచులు()) {
            LogicMTask* స్థిరరాశి శిఖరం = లేఖాచిత్రం_అంచు.as<MTask_అంచు>()->గమ్యం_MTaskp();
            UASSERT_OBJ(mtask.has_అంచు_గమ్యం(శిఖరం), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.కొలత() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

తరగతి క్రమం_MTask_లేఖాచిత్రం_Builder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // MEMBERS
    క్రమం_MTask_లేఖాచిత్రం& m_mtask_లేఖాచిత్రం;  // Output OrderMTaskGraph

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
    స్థిర తార్కిక bypassOk(క్రమం_Move_శీర్షం* mvtxp) {
        // Need to keep all logic vertices
        ఒకవేళ (mvtxp->logicp()) తిరిగి అబద్ధం;
        // Count fan-in, up to 3
        గుర్తులేని fan_లోపల = 0;
        స్వయంచాలక& లోపల_అంచులు = mvtxp->లోపల_అంచులు();
        కొరకు (స్వయంచాలక it = లోపల_అంచులు.begin(); it != లోపల_అంచులు.చివర(); ++it) {
            ఒకవేళ (++fan_లోపల == 3) ఆపు;
        }
        // If fanIn no more than one, bypass
        ఒకవేళ (fan_లోపల <= 1) తిరిగి నిజం;
        // Count fan-out, up to 3
        గుర్తులేని fan_బహిర్గతం = 0;
        స్వయంచాలక& బహిర్గతం_అంచులు = mvtxp->బహిర్గతం_అంచులు();
        కొరకు (స్వయంచాలక it = బహిర్గతం_అంచులు.begin(); it != బహిర్గతం_అంచులు.చివర(); ++it) {
            ఒకవేళ (++fan_బహిర్గతం == 3) ఆపు;
        }
        // If fan-out no more than one, bypass
        ఒకవేళ (fan_బహిర్గతం <= 1) తిరిగి నిజం;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        తిరిగి fan_లోపల + fan_బహిర్గతం == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    శూన్య కూడు_అంచు(LogicMTask* srcp, LogicMTask* dstp) {
        ఒకవేళ (srcp->has_అంచు_గమ్యం(dstp)) తిరిగి;  // Don't create redundant edges.
        m_mtask_లేఖాచిత్రం.కూడు_అంచు(srcp, dstp);
    }

    // CONSTRUCTORS
    స్పష్టమైన క్రమం_MTask_లేఖాచిత్రం_Builder(క్రమం_MTask_లేఖాచిత్రం& mtask_లేఖాచిత్రం)
        : m_mtask_లేఖాచిత్రం{mtask_లేఖాచిత్రం} {

        // Create the LogicMTasks for each OrderMoveVertex
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : mtask_లేఖాచిత్రం.move_లేఖాచిత్రం().vertices()) {
            క్రమం_Move_శీర్షం& mVtx = static_cast<క్రమం_Move_శీర్షం&>(vtx);
            ఒకవేళ (bypassOk(&mVtx)) {
                mVtx.userp(శూన్యసూచిక);  // Set to nullptr to mark as bypassed
            } లేకపోతే {
                mVtx.userp(కొత్త LogicMTask{mtask_లేఖాచిత్రం, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& నమోదు_2 = *mtask_లేఖాచిత్రం.entryp();
        LogicMTask& నిష్క్రమించు = *mtask_లేఖాచిత్రం.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : mtask_లేఖాచిత్రం.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            ఒకవేళ (VL_UNLIKELY((&mtask == &నమోదు_2) || (&mtask == &నిష్క్రమించు))) కొనసాగు;

            క్రమం_Move_శీర్షం::List& శీర్షం_List = mtask.శీర్షం_List();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(శీర్షం_List.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            క్రమం_Move_శీర్షం* స్థిరరాశి mVtxp = శీర్షం_List.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& d_అంచు : mVtxp->బహిర్గతం_అంచులు()) {
                V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి శిఖరం = d_అంచు.శిఖరం();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                ఒకవేళ (LogicMTask* స్థిరరాశి otherp = static_cast<LogicMTask*>(శిఖరం->userp())) {
                    కూడు_అంచు(&mtask, otherp);
                    కొనసాగు;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& t_అంచు : శిఖరం->బహిర్గతం_అంచులు()) {
                    LogicMTask* స్థిరరాశి transp = static_cast<LogicMTask*>(t_అంచు.శిఖరం()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    కూడు_అంచు(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : mtask_లేఖాచిత్రం.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            ఒకవేళ (VL_UNLIKELY((&mtask == &నమోదు_2) || (&mtask == &నిష్క్రమించు))) కొనసాగు;
            // Add the entry/exit edges if not otherwise connected
            ఒకవేళ (mtask.లోపల_ఖాళీలు()) కూడు_అంచు(&నమోదు_2, &mtask);
            ఒకవేళ (mtask.బహిర్గతం_ఖాళీలు()) కూడు_అంచు(&mtask, &నిష్క్రమించు);
        }
    }
    ~క్రమం_MTask_లేఖాచిత్రం_Builder() = అప్రమేయ;
    VL_UNCOPYABLE(క్రమం_MTask_లేఖాచిత్రం_Builder);
    VL_UNMOVABLE(క్రమం_MTask_లేఖాచిత్రం_Builder);

బహిరంగ:
    స్థిర శూన్య apply(క్రమం_MTask_లేఖాచిత్రం& mtask_లేఖాచిత్రం) { క్రమం_MTask_లేఖాచిత్రం_Builder{mtask_లేఖాచిత్రం}; }
};

std::unique_ptr<క్రమం_MTask_లేఖాచిత్రం> క్రమం_MTask_లేఖాచిత్రం::build(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం) {
    std::unique_ptr<క్రమం_MTask_లేఖాచిత్రం> resp{కొత్త క్రమం_MTask_లేఖాచిత్రం{move_లేఖాచిత్రం}};
    క్రమం_MTask_లేఖాచిత్రం_Builder::apply(*resp);
    resp->validate();
    తిరిగి resp;
}
