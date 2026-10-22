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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3OrderMTaskGraph.h"

#ضمن "V3Global.h"
#ضمن "V3InstrCount.h"

#ضمن <algorithm>
#ضمن <memory>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_بل_Id = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(ترتیب_MTask_ګراف& ګراف, ترتیب_Move_راس* mVtxp)
    : V3_ګراف_راس{&ګراف} {
    UASSERT(s_بل_Id < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    إذا (!mVtxp) أرجع;
    m_mVertices.link_شا(mVtxp);
    إذا (ثابت ترتیب_Logic_راس* ثابت olvp = mVtxp->logicp()) {
        m_لګښت += V3_Instr_شمېر::شمېر(olvp->nodep(), ریښتیا);
    }
}

//######################################################################
// OrderMTaskGraph

ترتیب_MTask_ګراف::ترتیب_MTask_ګراف(ترتیب_Move_ګراف& move_ګراف)
    : m_move_ګراف{move_ګراف}
    , m_entryp{جديد LogicMTask{*هذا, تش_اشاره}}
    , m_exitp{جديد LogicMTask{*هذا, تش_اشاره}}
    , m_slowAsserts{v3Global.opt.debug_برخه_کول()} {}

منطقي ترتیب_MTask_ګراف::لار_Exists_Impl(LogicMTask* fromp, LogicMTask* سر,
                                     ثابت MTask_څنډه* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    إذا (fromp == سر) أرجع ریښتیا;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    إذا (fromp->cpExclusive<ګراف_Way::سرچپه_2>() < سر->cpInclusive<ګراف_Way::سرچپه_2>()) {
        أرجع خطأ;
    }
    إذا (fromp->cpInclusive<ګراف_Way::FORWARD>() > سر->cpExclusive<ګراف_Way::FORWARD>()) {
        أرجع خطأ;
    }

    // Recursively look for a path
    لكل (ثابت V3_ګراف_څنډه& follow : fromp->بهر_څنډې()) {
        إذا (&follow == excludedEdgep) استمر;
        LogicMTask* ثابت nextp = static_cast<LogicMTask*>(follow.سر());
        // Don't visit the same MTask twice in the same search.
        إذا (nextp->m_generation == m_currentGeneration) استمر;
        إذا (لار_Exists_Impl(nextp, سر, تش_اشاره)) أرجع ریښتیا;
    }
    أرجع خطأ;
}

کينډۍ <ګراف_Way::en N_Way>
فراغ ترتیب_MTask_ګراف::propagate_ټېل(LogicMTask* mtaskp) {
    constexpr ګراف_Way way{N_Way};
    constexpr ګراف_Way inv{way.invert()};
    ثابت uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    لكل (V3_ګراف_څنډه& ګراف_څنډه : mtaskp->څنډې<way>()) {
        MTask_څنډه& څنډه = static_cast<MTask_څنډه&>(ګراف_څنډه);

        LogicMTask* ثابت relativep = څنډه.furtherMTaskp<N_Way>();
        څنډه_ډېرۍ::غوټه_2& څنډه_ډېرۍ_غوټه = څنډه.m_څنډه_ډېرۍ_غوټه[inv];
        إذا (inclusiveCp > څنډه_ډېرۍ_غوټه.کلی().m_cp) {
            relativep->m_څنډه_ډېرۍ[inv].increase_کلی(&څنډه_ډېرۍ_غوټه, inclusiveCp);
        }

        ثابت uint64_t relativeCp = relativep->cpExclusive<way>();

        إذا (relativeCp >= inclusiveCp) استمر;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ثابت uint64_t increment = inclusiveCp - relativeCp;

        Propagate_Pending_ډېرۍ::غوټه_2*& pendingNodepRef = relativep->m_propagate_ډېرۍ_Nodep;
        إذا (Propagate_Pending_ډېرۍ::غوټه_2* ثابت nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            إذا (increment > nodep->کلی().m_increment) {
                m_pending_ډېرۍ.increase_کلی(nodep, increment);
            }
            استمر;
        }

        // Add to heap
        Propagate_Pending_ډېرۍ::غوټه_2* ثابت nodep = m_pending_غوټه_Pool.alloc();
        pendingNodepRef = nodep;
        m_pending_ډېرۍ.ورننباسه(nodep, {increment, relativep->id(), relativep});
    }
}

کينډۍ <ګراف_Way::en N_Way>
فراغ ترتیب_MTask_ګراف::propagateResolve() {
    constexpr ګراف_Way way{N_Way};
    constexpr ګراف_Way inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    طالما (!m_pending_ډېرۍ.تشه()) {
        // Pop max element from heap
        Propagate_Pending_ډېرۍ::غوټه_2* ثابت maxp = m_pending_ډېرۍ.max();
        m_pending_ډېرۍ.remove(maxp);
        // Pick up values
        LogicMTask* ثابت mtaskp = maxp->کلی().m_mtaskp;
        ثابت uint64_t cpGrowBy = maxp->کلی().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_غوټه_Pool.حرر(maxp);
        mtaskp->m_propagate_ډېرۍ_Nodep = تش_اشاره;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ثابت uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        إذا (VL_UNLIKELY(m_slowAsserts)) {
            ثابت uint64_t څنډه_Cp = mtaskp->m_څنډه_ډېرۍ[inv].max()->کلی().m_cp;
            UASSERT_OBJ(څنډه_Cp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_ټېل<N_Way>(mtaskp);
    }
}

uint64_t ترتیب_MTask_ګراف::ټول_لګښت() ثابت {
    uint64_t لګښت = 0;
    لكل (ثابت V3_ګراف_راس& vtx : vertices()) لګښت += static_cast<ثابت LogicMTask&>(vtx).لګښت();
    أرجع لګښت;
}

فراغ ترتیب_MTask_ګراف::جمع_کړه_څنډه(LogicMTask* fromp, LogicMTask* سر) {
    UASSERT_OBJ(fromp != سر, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->has_څنډه_منزل(سر), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    جديد MTask_څنډه{هذا, fromp, سر};

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
    إذا (fromp->cpInclusive<ګراف_Way::FORWARD>() > سر->cpExclusive<ګراف_Way::FORWARD>()) {
        propagate<ګراف_Way::FORWARD>(fromp);
    }
    إذا (سر->cpInclusive<ګراف_Way::سرچپه_2>() > fromp->cpExclusive<ګراف_Way::سرچپه_2>()) {
        propagate<ګراف_Way::سرچپه_2>(سر);
    }
}

فراغ ترتیب_MTask_ګراف::یوځای_MTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    طالما (MTask_څنډه* ثابت edgep = static_cast<MTask_څنډه*>(donorp->بهر_څنډې().frontp())) {
        LogicMTask* ثابت relativep = edgep->منزل_MTaskp();

        relativep->remove_Relative_څنډه<ګراف_Way::سرچپه_2>(edgep);

        إذا (relativep == recipientp || recipientp->has_څنډه_منزل(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } وإلا {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->جمع_کړه_Dependent(relativep);
            recipientp->steal_Relative_څنډه<ګراف_Way::FORWARD>(edgep);
            relativep->جمع_کړه_Relative_څنډه<ګراف_Way::سرچپه_2>(edgep);
        }
    }

    // Process incoming edges of donor
    طالما (MTask_څنډه* ثابت edgep = static_cast<MTask_څنډه*>(donorp->دننه_څنډې().frontp())) {
        LogicMTask* ثابت relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->remove_Relative_څنډه<ګراف_Way::FORWARD>(edgep);

        إذا (relativep == recipientp || relativep->has_څنډه_منزل(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } وإلا {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_سر(recipientp);
            relativep->جمع_کړه_Dependent(recipientp);
            relativep->جمع_کړه_Relative_څنډه<ګراف_Way::FORWARD>(edgep);
            recipientp->steal_Relative_څنډه<ګراف_Way::سرچپه_2>(edgep);
        }
    }

    // Move the contents of the donor into the recipient, update its cost
    recipientp->m_mVertices.splice(recipientp->m_mVertices.پای_ټکی(), donorp->m_mVertices);
    recipientp->m_لګښت += donorp->m_لګښت;

    // The recipient now holds all edges of the merged MTask, and the critical paths of all its
    // relatives are still up to date, so the critical paths implied by its edges are the critical
    // paths of the merged MTask.
    ثابت uint64_t newCpFwd = recipientp->cp_Exclusive_From_څنډې<ګراف_Way::FORWARD>();
    ثابت uint64_t newCpRev = recipientp->cp_Exclusive_From_څنډې<ګراف_Way::سرچپه_2>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<ګراف_Way::FORWARD>(newCpFwd);
    propagate<ګراف_Way::FORWARD>(recipientp);
    recipientp->cpExclusive<ګراف_Way::سرچپه_2>(newCpRev);
    propagate<ګراف_Way::سرچپه_2>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(هذا), donorp);
}

فراغ ترتیب_MTask_ګراف::remove_متعدي_څنډې() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    لكل (V3_ګراف_راس& vtx : vertices()) {
        لكل (V3_ګراف_څنډه* ثابت ګراف_Edgep : vtx.بهر_څنډې().unlinkable()) {
            MTask_څنډه* ثابت edgep = static_cast<MTask_څنډه*>(ګراف_Edgep);
            LogicMTask* ثابت fromp = edgep->fromMTaskp();
            LogicMTask* ثابت سر = edgep->منزل_MTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            إذا (!لار_Exists(fromp, سر, edgep)) استمر;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(سر);
            fromp->remove_Relative_څنډه<ګراف_Way::FORWARD>(edgep);
            سر->remove_Relative_څنډه<ګراف_Way::سرچپه_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

فراغ ترتیب_MTask_ګراف::remove_تشه_MTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    لكل (V3_ګراف_راس* ثابت vtxp : vertices().unlinkable()) {
        LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        إذا (mtaskp == m_entryp || mtaskp == m_exitp) استمر;

        // Keep any MTask that holds logic
        منطقي تشه = ریښتیا;
        لكل (ثابت ترتیب_Move_راس& mVtx : mtaskp->راس_List()) {
            إذا (mVtx.logicp()) {
                تشه = خطأ;
                اكسر;
            }
        }
        إذا (!تشه) استمر;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->لګښت(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        لكل (V3_ګراف_څنډه& دننه_څنډه : mtaskp->دننه_څنډې()) {
            LogicMTask* ثابت fromp = static_cast<MTask_څنډه&>(دننه_څنډه).fromMTaskp();
            لكل (V3_ګراف_څنډه& بهر_څنډه : mtaskp->بهر_څنډې()) {
                LogicMTask* ثابت سر = static_cast<MTask_څنډه&>(بهر_څنډه).منزل_MTaskp();
                إذا (!fromp->has_څنډه_منزل(سر)) جمع_کړه_څنډه(fromp, سر);
            }
        }

        // Remove incoming edges of 'mtaskp'
        طالما (MTask_څنډه* ثابت edgep = static_cast<MTask_څنډه*>(mtaskp->دننه_څنډې().frontp())) {
            LogicMTask* ثابت relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->remove_Relative_څنډه<ګراف_Way::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        طالما (MTask_څنډه* ثابت edgep = static_cast<MTask_څنډه*>(mtaskp->بهر_څنډې().frontp())) {
            LogicMTask* ثابت relativep = edgep->منزل_MTaskp();
            relativep->remove_Relative_څنډه<ګراف_Way::سرچپه_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(هذا), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
کينډۍ <ګراف_Way::en N_Way>
فراغ ترتیب_MTask_ګراف::validateWay() ثابت {
    constexpr ګراف_Way way{N_Way};
    constexpr ګراف_Way inv = way.invert();
    لكل (ثابت V3_ګراف_راس& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cp_لګښت = 0;
        std::unordered_سېټ<ثابت V3_ګراف_راس*> relatives;
        لكل (ثابت V3_ګراف_څنډه& ګراف_څنډه : mtask.څنډې<inv>()) {
            ثابت MTask_څنډه& څنډه = *ګراف_څنډه.as<MTask_څنډه>();
            ثابت LogicMTask& relative = *(څنډه.furtherp<inv>()->کينډۍ as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(څنډه.وزن_2() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ثابت منطقي لومړی = relatives.ورننباسه(&relative).دوهم;
            UASSERT_OBJ(لومړی, &mtask, "Should be no redundant edges in MTask graph");
            ثابت uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(څنډه.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(څنډه.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cp_لګښت = std::max(cp_لګښت, inclusiveCp);
        }
        ثابت uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cp_لګښت, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cp_Exclusive_From_څنډې<N_Way>() == cp_لګښت, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

فراغ ترتیب_MTask_ګراف::validate() ثابت {
    إذا (!m_slowAsserts) أرجع;

    validateWay<ګراف_Way::FORWARD>();
    validateWay<ګراف_Way::سرچپه_2>();

    // Check the dependents set of each MTask agrees with its out-edges
    لكل (ثابت V3_ګراف_راس& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        اندازه_t nDependents = 0;
        لكل (ثابت V3_ګراف_څنډه& ګراف_څنډه : mtask.بهر_څنډې()) {
            LogicMTask* ثابت سر = ګراف_څنډه.as<MTask_څنډه>()->منزل_MTaskp();
            UASSERT_OBJ(mtask.has_څنډه_منزل(سر), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.اندازه() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

صنف ترتیب_MTask_ګراف_Builder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;

    // MEMBERS
    ترتیب_MTask_ګراف& m_mtask_ګراف;  // Output OrderMTaskGraph

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
    ساكن منطقي bypassOk(ترتیب_Move_راس* mvtxp) {
        // Need to keep all logic vertices
        إذا (mvtxp->logicp()) أرجع خطأ;
        // Count fan-in, up to 3
        بینښه fan_دننه = 0;
        خپلکاری& دننه_څنډې = mvtxp->دننه_څنډې();
        لكل (خپلکاری it = دننه_څنډې.begin(); it != دننه_څنډې.پای_ټکی(); ++it) {
            إذا (++fan_دننه == 3) اكسر;
        }
        // If fanIn no more than one, bypass
        إذا (fan_دننه <= 1) أرجع ریښتیا;
        // Count fan-out, up to 3
        بینښه fan_بهر = 0;
        خپلکاری& بهر_څنډې = mvtxp->بهر_څنډې();
        لكل (خپلکاری it = بهر_څنډې.begin(); it != بهر_څنډې.پای_ټکی(); ++it) {
            إذا (++fan_بهر == 3) اكسر;
        }
        // If fan-out no more than one, bypass
        إذا (fan_بهر <= 1) أرجع ریښتیا;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        أرجع fan_دننه + fan_بهر == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    فراغ جمع_کړه_څنډه(LogicMTask* srcp, LogicMTask* dstp) {
        إذا (srcp->has_څنډه_منزل(dstp)) أرجع;  // Don't create redundant edges.
        m_mtask_ګراف.جمع_کړه_څنډه(srcp, dstp);
    }

    // CONSTRUCTORS
    څرګند ترتیب_MTask_ګراف_Builder(ترتیب_MTask_ګراف& mtask_ګراف)
        : m_mtask_ګراف{mtask_ګراف} {

        // Create the LogicMTasks for each OrderMoveVertex
        لكل (V3_ګراف_راس& vtx : mtask_ګراف.move_ګراف().vertices()) {
            ترتیب_Move_راس& mVtx = static_cast<ترتیب_Move_راس&>(vtx);
            إذا (bypassOk(&mVtx)) {
                mVtx.userp(تش_اشاره);  // Set to nullptr to mark as bypassed
            } وإلا {
                mVtx.userp(جديد LogicMTask{mtask_ګراف, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& ثبت = *mtask_ګراف.entryp();
        LogicMTask& وتل = *mtask_ګراف.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        لكل (V3_ګراف_راس& vtx : mtask_ګراف.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            إذا (VL_UNLIKELY((&mtask == &ثبت) || (&mtask == &وتل))) استمر;

            ترتیب_Move_راس::List& راس_List = mtask.راس_List();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(راس_List.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            ترتیب_Move_راس* ثابت mVtxp = راس_List.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            لكل (ثابت V3_ګراف_څنډه& d_څنډه : mVtxp->بهر_څنډې()) {
                V3_ګراف_راس* ثابت سر = d_څنډه.سر();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                إذا (LogicMTask* ثابت otherp = static_cast<LogicMTask*>(سر->userp())) {
                    جمع_کړه_څنډه(&mtask, otherp);
                    استمر;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                لكل (ثابت V3_ګراف_څنډه& t_څنډه : سر->بهر_څنډې()) {
                    LogicMTask* ثابت transp = static_cast<LogicMTask*>(t_څنډه.سر()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    جمع_کړه_څنډه(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        لكل (V3_ګراف_راس& vtx : mtask_ګراف.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            إذا (VL_UNLIKELY((&mtask == &ثبت) || (&mtask == &وتل))) استمر;
            // Add the entry/exit edges if not otherwise connected
            إذا (mtask.دننه_تشه()) جمع_کړه_څنډه(&ثبت, &mtask);
            إذا (mtask.بهر_تشه()) جمع_کړه_څنډه(&mtask, &وتل);
        }
    }
    ~ترتیب_MTask_ګراف_Builder() = افتراضي;
    VL_UNCOPYABLE(ترتیب_MTask_ګراف_Builder);
    VL_UNMOVABLE(ترتیب_MTask_ګراف_Builder);

عام:
    ساكن فراغ apply(ترتیب_MTask_ګراف& mtask_ګراف) { ترتیب_MTask_ګراف_Builder{mtask_ګراف}; }
};

std::unique_ptr<ترتیب_MTask_ګراف> ترتیب_MTask_ګراف::build(ترتیب_Move_ګراف& move_ګراف) {
    std::unique_ptr<ترتیب_MTask_ګراف> resp{جديد ترتیب_MTask_ګراف{move_ګراف}};
    ترتیب_MTask_ګراف_Builder::apply(*resp);
    resp->validate();
    أرجع resp;
}
