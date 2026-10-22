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

uint32_t LogicMTask::s_ايندڙ_Id = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(ترتيب_MTask_گراف& گراف, ترتيب_Move_راس* mVtxp)
    : V3_گراف_راس{&گراف} {
    UASSERT(s_ايندڙ_Id < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    إذا (!mVtxp) أرجع;
    m_mVertices.link_پٺ(mVtxp);
    إذا (ثابت ترتيب_Logic_راس* ثابت olvp = mVtxp->logicp()) {
        m_لاڳت += V3_Instr_ڳڻپ::ڳڻپ(olvp->nodep(), سچ);
    }
}

//######################################################################
// OrderMTaskGraph

ترتيب_MTask_گراف::ترتيب_MTask_گراف(ترتيب_Move_گراف& move_گراف)
    : m_move_گراف{move_گراف}
    , m_entryp{جديد LogicMTask{*هذا, خالي_اشارو}}
    , m_exitp{جديد LogicMTask{*هذا, خالي_اشارو}}
    , m_slowAsserts{v3Global.opt.debug_حصا_ڪريو()} {}

منطقي ترتيب_MTask_گراف::رستو_Exists_Impl(LogicMTask* fromp, LogicMTask* چوٽي,
                                     ثابت MTask_ڪنارو* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    إذا (fromp == چوٽي) أرجع سچ;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    إذا (fromp->cpExclusive<گراف_Way::ابتو_2>() < چوٽي->cpInclusive<گراف_Way::ابتو_2>()) {
        أرجع خطأ;
    }
    إذا (fromp->cpInclusive<گراف_Way::FORWARD>() > چوٽي->cpExclusive<گراف_Way::FORWARD>()) {
        أرجع خطأ;
    }

    // Recursively look for a path
    لكل (ثابت V3_گراف_ڪنارو& follow : fromp->ٻاهريون_ڪنارا()) {
        إذا (&follow == excludedEdgep) استمر;
        LogicMTask* ثابت nextp = static_cast<LogicMTask*>(follow.چوٽي());
        // Don't visit the same MTask twice in the same search.
        إذا (nextp->m_generation == m_currentGeneration) استمر;
        إذا (رستو_Exists_Impl(nextp, چوٽي, خالي_اشارو)) أرجع سچ;
    }
    أرجع خطأ;
}

سانچو <گراف_Way::en N_Way>
فراغ ترتيب_MTask_گراف::propagate_ڌڪو(LogicMTask* mtaskp) {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way inv{way.invert()};
    ثابت uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    لكل (V3_گراف_ڪنارو& گراف_ڪنارو : mtaskp->ڪنارا<way>()) {
        MTask_ڪنارو& ڪنارو = static_cast<MTask_ڪنارو&>(گراف_ڪنارو);

        LogicMTask* ثابت relativep = ڪنارو.furtherMTaskp<N_Way>();
        ڪنارو_ڍير::ڳنڍ_2& ڪنارو_ڍير_ڳنڍ = ڪنارو.m_ڪنارو_ڍير_ڳنڍ[inv];
        إذا (inclusiveCp > ڪنارو_ڍير_ڳنڍ.ڪنجي().m_cp) {
            relativep->m_ڪنارو_ڍير[inv].increase_ڪنجي(&ڪنارو_ڍير_ڳنڍ, inclusiveCp);
        }

        ثابت uint64_t relativeCp = relativep->cpExclusive<way>();

        إذا (relativeCp >= inclusiveCp) استمر;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ثابت uint64_t increment = inclusiveCp - relativeCp;

        Propagate_Pending_ڍير::ڳنڍ_2*& pendingNodepRef = relativep->m_propagate_ڍير_Nodep;
        إذا (Propagate_Pending_ڍير::ڳنڍ_2* ثابت nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            إذا (increment > nodep->ڪنجي().m_increment) {
                m_pending_ڍير.increase_ڪنجي(nodep, increment);
            }
            استمر;
        }

        // Add to heap
        Propagate_Pending_ڍير::ڳنڍ_2* ثابت nodep = m_pending_ڳنڍ_Pool.alloc();
        pendingNodepRef = nodep;
        m_pending_ڍير.وجهو(nodep, {increment, relativep->id(), relativep});
    }
}

سانچو <گراف_Way::en N_Way>
فراغ ترتيب_MTask_گراف::propagateResolve() {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    طالما (!m_pending_ڍير.خالي_جايون()) {
        // Pop max element from heap
        Propagate_Pending_ڍير::ڳنڍ_2* ثابت maxp = m_pending_ڍير.max();
        m_pending_ڍير.remove(maxp);
        // Pick up values
        LogicMTask* ثابت mtaskp = maxp->ڪنجي().m_mtaskp;
        ثابت uint64_t cpGrowBy = maxp->ڪنجي().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_ڳنڍ_Pool.حرر(maxp);
        mtaskp->m_propagate_ڍير_Nodep = خالي_اشارو;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ثابت uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        إذا (VL_UNLIKELY(m_slowAsserts)) {
            ثابت uint64_t ڪنارو_Cp = mtaskp->m_ڪنارو_ڍير[inv].max()->ڪنجي().m_cp;
            UASSERT_OBJ(ڪنارو_Cp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_ڌڪو<N_Way>(mtaskp);
    }
}

uint64_t ترتيب_MTask_گراف::ڪل_لاڳت() ثابت {
    uint64_t لاڳت = 0;
    لكل (ثابت V3_گراف_راس& vtx : vertices()) لاڳت += static_cast<ثابت LogicMTask&>(vtx).لاڳت();
    أرجع لاڳت;
}

فراغ ترتيب_MTask_گراف::جوڙيو_ڪنارو(LogicMTask* fromp, LogicMTask* چوٽي) {
    UASSERT_OBJ(fromp != چوٽي, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->has_ڪنارو_منزل(چوٽي), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    جديد MTask_ڪنارو{هذا, fromp, چوٽي};

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
    إذا (fromp->cpInclusive<گراف_Way::FORWARD>() > چوٽي->cpExclusive<گراف_Way::FORWARD>()) {
        propagate<گراف_Way::FORWARD>(fromp);
    }
    إذا (چوٽي->cpInclusive<گراف_Way::ابتو_2>() > fromp->cpExclusive<گراف_Way::ابتو_2>()) {
        propagate<گراف_Way::ابتو_2>(چوٽي);
    }
}

فراغ ترتيب_MTask_گراف::ملايو_MTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    طالما (MTask_ڪنارو* ثابت edgep = static_cast<MTask_ڪنارو*>(donorp->ٻاهريون_ڪنارا().frontp())) {
        LogicMTask* ثابت relativep = edgep->منزل_MTaskp();

        relativep->remove_Relative_ڪنارو<گراف_Way::ابتو_2>(edgep);

        إذا (relativep == recipientp || recipientp->has_ڪنارو_منزل(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } وإلا {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->جوڙيو_Dependent(relativep);
            recipientp->steal_Relative_ڪنارو<گراف_Way::FORWARD>(edgep);
            relativep->جوڙيو_Relative_ڪنارو<گراف_Way::ابتو_2>(edgep);
        }
    }

    // Process incoming edges of donor
    طالما (MTask_ڪنارو* ثابت edgep = static_cast<MTask_ڪنارو*>(donorp->اندريون_ڪنارا().frontp())) {
        LogicMTask* ثابت relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->remove_Relative_ڪنارو<گراف_Way::FORWARD>(edgep);

        إذا (relativep == recipientp || relativep->has_ڪنارو_منزل(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } وإلا {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_چوٽي(recipientp);
            relativep->جوڙيو_Dependent(recipientp);
            relativep->جوڙيو_Relative_ڪنارو<گراف_Way::FORWARD>(edgep);
            recipientp->steal_Relative_ڪنارو<گراف_Way::ابتو_2>(edgep);
        }
    }

    // Move the contents of the donor into the recipient, update its cost
    recipientp->m_mVertices.splice(recipientp->m_mVertices.پڇاڙي(), donorp->m_mVertices);
    recipientp->m_لاڳت += donorp->m_لاڳت;

    // The recipient now holds all edges of the merged MTask, and the critical paths of all its
    // relatives are still up to date, so the critical paths implied by its edges are the critical
    // paths of the merged MTask.
    ثابت uint64_t newCpFwd = recipientp->cp_Exclusive_From_ڪنارا<گراف_Way::FORWARD>();
    ثابت uint64_t newCpRev = recipientp->cp_Exclusive_From_ڪنارا<گراف_Way::ابتو_2>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<گراف_Way::FORWARD>(newCpFwd);
    propagate<گراف_Way::FORWARD>(recipientp);
    recipientp->cpExclusive<گراف_Way::ابتو_2>(newCpRev);
    propagate<گراف_Way::ابتو_2>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(هذا), donorp);
}

فراغ ترتيب_MTask_گراف::remove_متعدي_ڪنارا() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    لكل (V3_گراف_راس& vtx : vertices()) {
        لكل (V3_گراف_ڪنارو* ثابت گراف_Edgep : vtx.ٻاهريون_ڪنارا().unlinkable()) {
            MTask_ڪنارو* ثابت edgep = static_cast<MTask_ڪنارو*>(گراف_Edgep);
            LogicMTask* ثابت fromp = edgep->fromMTaskp();
            LogicMTask* ثابت چوٽي = edgep->منزل_MTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            إذا (!رستو_Exists(fromp, چوٽي, edgep)) استمر;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(چوٽي);
            fromp->remove_Relative_ڪنارو<گراف_Way::FORWARD>(edgep);
            چوٽي->remove_Relative_ڪنارو<گراف_Way::ابتو_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

فراغ ترتيب_MTask_گراف::remove_خالي_جايون_MTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    لكل (V3_گراف_راس* ثابت vtxp : vertices().unlinkable()) {
        LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        إذا (mtaskp == m_entryp || mtaskp == m_exitp) استمر;

        // Keep any MTask that holds logic
        منطقي خالي_جايون = سچ;
        لكل (ثابت ترتيب_Move_راس& mVtx : mtaskp->راس_List()) {
            إذا (mVtx.logicp()) {
                خالي_جايون = خطأ;
                اكسر;
            }
        }
        إذا (!خالي_جايون) استمر;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->لاڳت(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        لكل (V3_گراف_ڪنارو& اندريون_ڪنارو : mtaskp->اندريون_ڪنارا()) {
            LogicMTask* ثابت fromp = static_cast<MTask_ڪنارو&>(اندريون_ڪنارو).fromMTaskp();
            لكل (V3_گراف_ڪنارو& ٻاهريون_ڪنارو : mtaskp->ٻاهريون_ڪنارا()) {
                LogicMTask* ثابت چوٽي = static_cast<MTask_ڪنارو&>(ٻاهريون_ڪنارو).منزل_MTaskp();
                إذا (!fromp->has_ڪنارو_منزل(چوٽي)) جوڙيو_ڪنارو(fromp, چوٽي);
            }
        }

        // Remove incoming edges of 'mtaskp'
        طالما (MTask_ڪنارو* ثابت edgep = static_cast<MTask_ڪنارو*>(mtaskp->اندريون_ڪنارا().frontp())) {
            LogicMTask* ثابت relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->remove_Relative_ڪنارو<گراف_Way::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        طالما (MTask_ڪنارو* ثابت edgep = static_cast<MTask_ڪنارو*>(mtaskp->ٻاهريون_ڪنارا().frontp())) {
            LogicMTask* ثابت relativep = edgep->منزل_MTaskp();
            relativep->remove_Relative_ڪنارو<گراف_Way::ابتو_2>(edgep);
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
سانچو <گراف_Way::en N_Way>
فراغ ترتيب_MTask_گراف::validateWay() ثابت {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way inv = way.invert();
    لكل (ثابت V3_گراف_راس& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cp_لاڳت = 0;
        std::unordered_سيٽ<ثابت V3_گراف_راس*> relatives;
        لكل (ثابت V3_گراف_ڪنارو& گراف_ڪنارو : mtask.ڪنارا<inv>()) {
            ثابت MTask_ڪنارو& ڪنارو = *گراف_ڪنارو.as<MTask_ڪنارو>();
            ثابت LogicMTask& relative = *(ڪنارو.furtherp<inv>()->سانچو as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(ڪنارو.وزن_2() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ثابت منطقي پهريون = relatives.وجهو(&relative).ٻيو;
            UASSERT_OBJ(پهريون, &mtask, "Should be no redundant edges in MTask graph");
            ثابت uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(ڪنارو.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(ڪنارو.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cp_لاڳت = std::max(cp_لاڳت, inclusiveCp);
        }
        ثابت uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cp_لاڳت, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cp_Exclusive_From_ڪنارا<N_Way>() == cp_لاڳت, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

فراغ ترتيب_MTask_گراف::validate() ثابت {
    إذا (!m_slowAsserts) أرجع;

    validateWay<گراف_Way::FORWARD>();
    validateWay<گراف_Way::ابتو_2>();

    // Check the dependents set of each MTask agrees with its out-edges
    لكل (ثابت V3_گراف_راس& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        سائيز_t nDependents = 0;
        لكل (ثابت V3_گراف_ڪنارو& گراف_ڪنارو : mtask.ٻاهريون_ڪنارا()) {
            LogicMTask* ثابت چوٽي = گراف_ڪنارو.as<MTask_ڪنارو>()->منزل_MTaskp();
            UASSERT_OBJ(mtask.has_ڪنارو_منزل(چوٽي), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.سائيز() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

صنف ترتيب_MTask_گراف_Builder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;

    // MEMBERS
    ترتيب_MTask_گراف& m_mtask_گراف;  // Output OrderMTaskGraph

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
    ساكن منطقي bypassOk(ترتيب_Move_راس* mvtxp) {
        // Need to keep all logic vertices
        إذا (mvtxp->logicp()) أرجع خطأ;
        // Count fan-in, up to 3
        بنانشان fan_اندريون = 0;
        خودڪار& اندريون_ڪنارا = mvtxp->اندريون_ڪنارا();
        لكل (خودڪار it = اندريون_ڪنارا.begin(); it != اندريون_ڪنارا.پڇاڙي(); ++it) {
            إذا (++fan_اندريون == 3) اكسر;
        }
        // If fanIn no more than one, bypass
        إذا (fan_اندريون <= 1) أرجع سچ;
        // Count fan-out, up to 3
        بنانشان fan_ٻاهريون = 0;
        خودڪار& ٻاهريون_ڪنارا = mvtxp->ٻاهريون_ڪنارا();
        لكل (خودڪار it = ٻاهريون_ڪنارا.begin(); it != ٻاهريون_ڪنارا.پڇاڙي(); ++it) {
            إذا (++fan_ٻاهريون == 3) اكسر;
        }
        // If fan-out no more than one, bypass
        إذا (fan_ٻاهريون <= 1) أرجع سچ;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        أرجع fan_اندريون + fan_ٻاهريون == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    فراغ جوڙيو_ڪنارو(LogicMTask* srcp, LogicMTask* dstp) {
        إذا (srcp->has_ڪنارو_منزل(dstp)) أرجع;  // Don't create redundant edges.
        m_mtask_گراف.جوڙيو_ڪنارو(srcp, dstp);
    }

    // CONSTRUCTORS
    واضح ترتيب_MTask_گراف_Builder(ترتيب_MTask_گراف& mtask_گراف)
        : m_mtask_گراف{mtask_گراف} {

        // Create the LogicMTasks for each OrderMoveVertex
        لكل (V3_گراف_راس& vtx : mtask_گراف.move_گراف().vertices()) {
            ترتيب_Move_راس& mVtx = static_cast<ترتيب_Move_راس&>(vtx);
            إذا (bypassOk(&mVtx)) {
                mVtx.userp(خالي_اشارو);  // Set to nullptr to mark as bypassed
            } وإلا {
                mVtx.userp(جديد LogicMTask{mtask_گراف, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& داخلا_2 = *mtask_گراف.entryp();
        LogicMTask& ٻاهر_نڪرو = *mtask_گراف.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        لكل (V3_گراف_راس& vtx : mtask_گراف.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            إذا (VL_UNLIKELY((&mtask == &داخلا_2) || (&mtask == &ٻاهر_نڪرو))) استمر;

            ترتيب_Move_راس::List& راس_List = mtask.راس_List();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(راس_List.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            ترتيب_Move_راس* ثابت mVtxp = راس_List.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            لكل (ثابت V3_گراف_ڪنارو& d_ڪنارو : mVtxp->ٻاهريون_ڪنارا()) {
                V3_گراف_راس* ثابت چوٽي = d_ڪنارو.چوٽي();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                إذا (LogicMTask* ثابت otherp = static_cast<LogicMTask*>(چوٽي->userp())) {
                    جوڙيو_ڪنارو(&mtask, otherp);
                    استمر;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                لكل (ثابت V3_گراف_ڪنارو& t_ڪنارو : چوٽي->ٻاهريون_ڪنارا()) {
                    LogicMTask* ثابت transp = static_cast<LogicMTask*>(t_ڪنارو.چوٽي()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    جوڙيو_ڪنارو(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        لكل (V3_گراف_راس& vtx : mtask_گراف.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            إذا (VL_UNLIKELY((&mtask == &داخلا_2) || (&mtask == &ٻاهر_نڪرو))) استمر;
            // Add the entry/exit edges if not otherwise connected
            إذا (mtask.اندريون_خالي_جايون()) جوڙيو_ڪنارو(&داخلا_2, &mtask);
            إذا (mtask.ٻاهريون_خالي_جايون()) جوڙيو_ڪنارو(&mtask, &ٻاهر_نڪرو);
        }
    }
    ~ترتيب_MTask_گراف_Builder() = افتراضي;
    VL_UNCOPYABLE(ترتيب_MTask_گراف_Builder);
    VL_UNMOVABLE(ترتيب_MTask_گراف_Builder);

عام:
    ساكن فراغ apply(ترتيب_MTask_گراف& mtask_گراف) { ترتيب_MTask_گراف_Builder{mtask_گراف}; }
};

std::unique_ptr<ترتيب_MTask_گراف> ترتيب_MTask_گراف::build(ترتيب_Move_گراف& move_گراف) {
    std::unique_ptr<ترتيب_MTask_گراف> resp{جديد ترتيب_MTask_گراف{move_گراف}};
    ترتيب_MTask_گراف_Builder::apply(*resp);
    resp->validate();
    أرجع resp;
}
