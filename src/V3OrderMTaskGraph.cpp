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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3OrderMTaskGraph.h"

#شامل "V3Global.h"
#شامل "V3InstrCount.h"

#شامل <algorithm>
#شامل <memory>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_اگلا_Id = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(ترتیب_MTask_گراف& گراف, ترتیب_Move_راس* mVtxp)
    : V3_گراف_راس{&گراف} {
    UASSERT(s_اگلا_Id < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    اگر (!mVtxp) واپس;
    m_mVertices.link_عقب(mVtxp);
    اگر (ثابت ترتیب_Logic_راس* ثابت olvp = mVtxp->logicp()) {
        m_لاگت += V3_Instr_گنتی::گنتی(olvp->nodep(), سچ);
    }
}

//######################################################################
// OrderMTaskGraph

ترتیب_MTask_گراف::ترتیب_MTask_گراف(ترتیب_Move_گراف& move_گراف)
    : m_move_گراف{move_گراف}
    , m_entryp{نیا LogicMTask{*یہ, خالی_اشاریہ}}
    , m_exitp{نیا LogicMTask{*یہ, خالی_اشاریہ}}
    , m_slowAsserts{v3Global.opt.debug_تقسیم()} {}

منطقی ترتیب_MTask_گراف::راستہ_Exists_Impl(LogicMTask* fromp, LogicMTask* چوٹی,
                                     ثابت MTask_کنارہ* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    اگر (fromp == چوٹی) واپس سچ;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    اگر (fromp->cpExclusive<گراف_Way::الٹ_2>() < چوٹی->cpInclusive<گراف_Way::الٹ_2>()) {
        واپس جھوٹ;
    }
    اگر (fromp->cpInclusive<گراف_Way::FORWARD>() > چوٹی->cpExclusive<گراف_Way::FORWARD>()) {
        واپس جھوٹ;
    }

    // Recursively look for a path
    برائے (ثابت V3_گراف_کنارہ& follow : fromp->برآمد_کنارے()) {
        اگر (&follow == excludedEdgep) جاری;
        LogicMTask* ثابت nextp = static_cast<LogicMTask*>(follow.چوٹی());
        // Don't visit the same MTask twice in the same search.
        اگر (nextp->m_generation == m_currentGeneration) جاری;
        اگر (راستہ_Exists_Impl(nextp, چوٹی, خالی_اشاریہ)) واپس سچ;
    }
    واپس جھوٹ;
}

سانچہ <گراف_Way::en N_Way>
خالی ترتیب_MTask_گراف::propagate_ڈالو(LogicMTask* mtaskp) {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way inv{way.invert()};
    ثابت uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    برائے (V3_گراف_کنارہ& گراف_کنارہ : mtaskp->کنارے<way>()) {
        MTask_کنارہ& کنارہ = static_cast<MTask_کنارہ&>(گراف_کنارہ);

        LogicMTask* ثابت relativep = کنارہ.furtherMTaskp<N_Way>();
        کنارہ_ڈھیر::گرہ_2& کنارہ_ڈھیر_گرہ = کنارہ.m_کنارہ_ڈھیر_گرہ[inv];
        اگر (inclusiveCp > کنارہ_ڈھیر_گرہ.کلید().m_cp) {
            relativep->m_کنارہ_ڈھیر[inv].increase_کلید(&کنارہ_ڈھیر_گرہ, inclusiveCp);
        }

        ثابت uint64_t relativeCp = relativep->cpExclusive<way>();

        اگر (relativeCp >= inclusiveCp) جاری;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ثابت uint64_t increment = inclusiveCp - relativeCp;

        Propagate_Pending_ڈھیر::گرہ_2*& pendingNodepRef = relativep->m_propagate_ڈھیر_Nodep;
        اگر (Propagate_Pending_ڈھیر::گرہ_2* ثابت nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            اگر (increment > nodep->کلید().m_increment) {
                m_pending_ڈھیر.increase_کلید(nodep, increment);
            }
            جاری;
        }

        // Add to heap
        Propagate_Pending_ڈھیر::گرہ_2* ثابت nodep = m_pending_گرہ_Pool.alloc();
        pendingNodepRef = nodep;
        m_pending_ڈھیر.داخل_کرو(nodep, {increment, relativep->id(), relativep});
    }
}

سانچہ <گراف_Way::en N_Way>
خالی ترتیب_MTask_گراف::propagateResolve() {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    جب (!m_pending_ڈھیر.خالی_خانے()) {
        // Pop max element from heap
        Propagate_Pending_ڈھیر::گرہ_2* ثابت maxp = m_pending_ڈھیر.max();
        m_pending_ڈھیر.remove(maxp);
        // Pick up values
        LogicMTask* ثابت mtaskp = maxp->کلید().m_mtaskp;
        ثابت uint64_t cpGrowBy = maxp->کلید().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_گرہ_Pool.آزاد(maxp);
        mtaskp->m_propagate_ڈھیر_Nodep = خالی_اشاریہ;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ثابت uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        اگر (VL_UNLIKELY(m_slowAsserts)) {
            ثابت uint64_t کنارہ_Cp = mtaskp->m_کنارہ_ڈھیر[inv].max()->کلید().m_cp;
            UASSERT_OBJ(کنارہ_Cp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_ڈالو<N_Way>(mtaskp);
    }
}

uint64_t ترتیب_MTask_گراف::کل_لاگت() ثابت {
    uint64_t لاگت = 0;
    برائے (ثابت V3_گراف_راس& vtx : vertices()) لاگت += static_cast<ثابت LogicMTask&>(vtx).لاگت();
    واپس لاگت;
}

خالی ترتیب_MTask_گراف::جوڑو_کنارہ(LogicMTask* fromp, LogicMTask* چوٹی) {
    UASSERT_OBJ(fromp != چوٹی, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->has_کنارہ_مقصد(چوٹی), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    نیا MTask_کنارہ{یہ, fromp, چوٹی};

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
    اگر (fromp->cpInclusive<گراف_Way::FORWARD>() > چوٹی->cpExclusive<گراف_Way::FORWARD>()) {
        propagate<گراف_Way::FORWARD>(fromp);
    }
    اگر (چوٹی->cpInclusive<گراف_Way::الٹ_2>() > fromp->cpExclusive<گراف_Way::الٹ_2>()) {
        propagate<گراف_Way::الٹ_2>(چوٹی);
    }
}

خالی ترتیب_MTask_گراف::ملاؤ_MTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    جب (MTask_کنارہ* ثابت edgep = static_cast<MTask_کنارہ*>(donorp->برآمد_کنارے().frontp())) {
        LogicMTask* ثابت relativep = edgep->مقصد_MTaskp();

        relativep->remove_Relative_کنارہ<گراف_Way::الٹ_2>(edgep);

        اگر (relativep == recipientp || recipientp->has_کنارہ_مقصد(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ورنہ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->جوڑو_Dependent(relativep);
            recipientp->steal_Relative_کنارہ<گراف_Way::FORWARD>(edgep);
            relativep->جوڑو_Relative_کنارہ<گراف_Way::الٹ_2>(edgep);
        }
    }

    // Process incoming edges of donor
    جب (MTask_کنارہ* ثابت edgep = static_cast<MTask_کنارہ*>(donorp->اندر_کنارے().frontp())) {
        LogicMTask* ثابت relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->remove_Relative_کنارہ<گراف_Way::FORWARD>(edgep);

        اگر (relativep == recipientp || relativep->has_کنارہ_مقصد(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ورنہ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_چوٹی(recipientp);
            relativep->جوڑو_Dependent(recipientp);
            relativep->جوڑو_Relative_کنارہ<گراف_Way::FORWARD>(edgep);
            recipientp->steal_Relative_کنارہ<گراف_Way::الٹ_2>(edgep);
        }
    }

    // Move the contents of the donor into the recipient, update its cost
    recipientp->m_mVertices.splice(recipientp->m_mVertices.انتہا(), donorp->m_mVertices);
    recipientp->m_لاگت += donorp->m_لاگت;

    // The recipient now holds all edges of the merged MTask, and the critical paths of all its
    // relatives are still up to date, so the critical paths implied by its edges are the critical
    // paths of the merged MTask.
    ثابت uint64_t newCpFwd = recipientp->cp_Exclusive_From_کنارے<گراف_Way::FORWARD>();
    ثابت uint64_t newCpRev = recipientp->cp_Exclusive_From_کنارے<گراف_Way::الٹ_2>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<گراف_Way::FORWARD>(newCpFwd);
    propagate<گراف_Way::FORWARD>(recipientp);
    recipientp->cpExclusive<گراف_Way::الٹ_2>(newCpRev);
    propagate<گراف_Way::الٹ_2>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(یہ), donorp);
}

خالی ترتیب_MTask_گراف::remove_متعدی_کنارے() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    برائے (V3_گراف_راس& vtx : vertices()) {
        برائے (V3_گراف_کنارہ* ثابت گراف_Edgep : vtx.برآمد_کنارے().unlinkable()) {
            MTask_کنارہ* ثابت edgep = static_cast<MTask_کنارہ*>(گراف_Edgep);
            LogicMTask* ثابت fromp = edgep->fromMTaskp();
            LogicMTask* ثابت چوٹی = edgep->مقصد_MTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            اگر (!راستہ_Exists(fromp, چوٹی, edgep)) جاری;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(چوٹی);
            fromp->remove_Relative_کنارہ<گراف_Way::FORWARD>(edgep);
            چوٹی->remove_Relative_کنارہ<گراف_Way::الٹ_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

خالی ترتیب_MTask_گراف::remove_خالی_خانے_MTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    برائے (V3_گراف_راس* ثابت vtxp : vertices().unlinkable()) {
        LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        اگر (mtaskp == m_entryp || mtaskp == m_exitp) جاری;

        // Keep any MTask that holds logic
        منطقی خالی_خانے = سچ;
        برائے (ثابت ترتیب_Move_راس& mVtx : mtaskp->راس_List()) {
            اگر (mVtx.logicp()) {
                خالی_خانے = جھوٹ;
                توڑو;
            }
        }
        اگر (!خالی_خانے) جاری;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->لاگت(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        برائے (V3_گراف_کنارہ& اندر_کنارہ : mtaskp->اندر_کنارے()) {
            LogicMTask* ثابت fromp = static_cast<MTask_کنارہ&>(اندر_کنارہ).fromMTaskp();
            برائے (V3_گراف_کنارہ& برآمد_کنارہ : mtaskp->برآمد_کنارے()) {
                LogicMTask* ثابت چوٹی = static_cast<MTask_کنارہ&>(برآمد_کنارہ).مقصد_MTaskp();
                اگر (!fromp->has_کنارہ_مقصد(چوٹی)) جوڑو_کنارہ(fromp, چوٹی);
            }
        }

        // Remove incoming edges of 'mtaskp'
        جب (MTask_کنارہ* ثابت edgep = static_cast<MTask_کنارہ*>(mtaskp->اندر_کنارے().frontp())) {
            LogicMTask* ثابت relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->remove_Relative_کنارہ<گراف_Way::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        جب (MTask_کنارہ* ثابت edgep = static_cast<MTask_کنارہ*>(mtaskp->برآمد_کنارے().frontp())) {
            LogicMTask* ثابت relativep = edgep->مقصد_MTaskp();
            relativep->remove_Relative_کنارہ<گراف_Way::الٹ_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(یہ), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
سانچہ <گراف_Way::en N_Way>
خالی ترتیب_MTask_گراف::validateWay() ثابت {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way inv = way.invert();
    برائے (ثابت V3_گراف_راس& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cp_لاگت = 0;
        std::unordered_سیٹ<ثابت V3_گراف_راس*> relatives;
        برائے (ثابت V3_گراف_کنارہ& گراف_کنارہ : mtask.کنارے<inv>()) {
            ثابت MTask_کنارہ& کنارہ = *گراف_کنارہ.as<MTask_کنارہ>();
            ثابت LogicMTask& relative = *(کنارہ.furtherp<inv>()->سانچہ as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(کنارہ.وزن_2() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ثابت منطقی پہلا = relatives.داخل_کرو(&relative).دوسرا;
            UASSERT_OBJ(پہلا, &mtask, "Should be no redundant edges in MTask graph");
            ثابت uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(کنارہ.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(کنارہ.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cp_لاگت = std::max(cp_لاگت, inclusiveCp);
        }
        ثابت uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cp_لاگت, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cp_Exclusive_From_کنارے<N_Way>() == cp_لاگت, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

خالی ترتیب_MTask_گراف::validate() ثابت {
    اگر (!m_slowAsserts) واپس;

    validateWay<گراف_Way::FORWARD>();
    validateWay<گراف_Way::الٹ_2>();

    // Check the dependents set of each MTask agrees with its out-edges
    برائے (ثابت V3_گراف_راس& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        سائز_t nDependents = 0;
        برائے (ثابت V3_گراف_کنارہ& گراف_کنارہ : mtask.برآمد_کنارے()) {
            LogicMTask* ثابت چوٹی = گراف_کنارہ.as<MTask_کنارہ>()->مقصد_MTaskp();
            UASSERT_OBJ(mtask.has_کنارہ_مقصد(چوٹی), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.سائز() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

جماعت ترتیب_MTask_گراف_Builder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // MEMBERS
    ترتیب_MTask_گراف& m_mtask_گراف;  // Output OrderMTaskGraph

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
    ساکن منطقی bypassOk(ترتیب_Move_راس* mvtxp) {
        // Need to keep all logic vertices
        اگر (mvtxp->logicp()) واپس جھوٹ;
        // Count fan-in, up to 3
        غیرعلامتی fan_اندر = 0;
        خودکار& اندر_کنارے = mvtxp->اندر_کنارے();
        برائے (خودکار it = اندر_کنارے.begin(); it != اندر_کنارے.انتہا(); ++it) {
            اگر (++fan_اندر == 3) توڑو;
        }
        // If fanIn no more than one, bypass
        اگر (fan_اندر <= 1) واپس سچ;
        // Count fan-out, up to 3
        غیرعلامتی fan_برآمد = 0;
        خودکار& برآمد_کنارے = mvtxp->برآمد_کنارے();
        برائے (خودکار it = برآمد_کنارے.begin(); it != برآمد_کنارے.انتہا(); ++it) {
            اگر (++fan_برآمد == 3) توڑو;
        }
        // If fan-out no more than one, bypass
        اگر (fan_برآمد <= 1) واپس سچ;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        واپس fan_اندر + fan_برآمد == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    خالی جوڑو_کنارہ(LogicMTask* srcp, LogicMTask* dstp) {
        اگر (srcp->has_کنارہ_مقصد(dstp)) واپس;  // Don't create redundant edges.
        m_mtask_گراف.جوڑو_کنارہ(srcp, dstp);
    }

    // CONSTRUCTORS
    صریح ترتیب_MTask_گراف_Builder(ترتیب_MTask_گراف& mtask_گراف)
        : m_mtask_گراف{mtask_گراف} {

        // Create the LogicMTasks for each OrderMoveVertex
        برائے (V3_گراف_راس& vtx : mtask_گراف.move_گراف().vertices()) {
            ترتیب_Move_راس& mVtx = static_cast<ترتیب_Move_راس&>(vtx);
            اگر (bypassOk(&mVtx)) {
                mVtx.userp(خالی_اشاریہ);  // Set to nullptr to mark as bypassed
            } ورنہ {
                mVtx.userp(نیا LogicMTask{mtask_گراف, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& اندراج_2 = *mtask_گراف.entryp();
        LogicMTask& باہر_نکلو = *mtask_گراف.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        برائے (V3_گراف_راس& vtx : mtask_گراف.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            اگر (VL_UNLIKELY((&mtask == &اندراج_2) || (&mtask == &باہر_نکلو))) جاری;

            ترتیب_Move_راس::List& راس_List = mtask.راس_List();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(راس_List.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            ترتیب_Move_راس* ثابت mVtxp = راس_List.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            برائے (ثابت V3_گراف_کنارہ& d_کنارہ : mVtxp->برآمد_کنارے()) {
                V3_گراف_راس* ثابت چوٹی = d_کنارہ.چوٹی();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                اگر (LogicMTask* ثابت otherp = static_cast<LogicMTask*>(چوٹی->userp())) {
                    جوڑو_کنارہ(&mtask, otherp);
                    جاری;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                برائے (ثابت V3_گراف_کنارہ& t_کنارہ : چوٹی->برآمد_کنارے()) {
                    LogicMTask* ثابت transp = static_cast<LogicMTask*>(t_کنارہ.چوٹی()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    جوڑو_کنارہ(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        برائے (V3_گراف_راس& vtx : mtask_گراف.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            اگر (VL_UNLIKELY((&mtask == &اندراج_2) || (&mtask == &باہر_نکلو))) جاری;
            // Add the entry/exit edges if not otherwise connected
            اگر (mtask.اندر_خالی_خانے()) جوڑو_کنارہ(&اندراج_2, &mtask);
            اگر (mtask.برآمد_خالی_خانے()) جوڑو_کنارہ(&mtask, &باہر_نکلو);
        }
    }
    ~ترتیب_MTask_گراف_Builder() = طےشدہ;
    VL_UNCOPYABLE(ترتیب_MTask_گراف_Builder);
    VL_UNMOVABLE(ترتیب_MTask_گراف_Builder);

عوامی:
    ساکن خالی apply(ترتیب_MTask_گراف& mtask_گراف) { ترتیب_MTask_گراف_Builder{mtask_گراف}; }
};

std::unique_ptr<ترتیب_MTask_گراف> ترتیب_MTask_گراف::build(ترتیب_Move_گراف& move_گراف) {
    std::unique_ptr<ترتیب_MTask_گراف> resp{نیا ترتیب_MTask_گراف{move_گراف}};
    ترتیب_MTask_گراف_Builder::apply(*resp);
    resp->validate();
    واپس resp;
}
