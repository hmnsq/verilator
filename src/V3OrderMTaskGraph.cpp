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

uint32_t LogicMTask::s_اگلا_Id = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(ترتیب_MTask_گراف& گراف, ترتیب_Move_راس* mVtxp)
    : V3_گراف_راس{&گراف} {
    UASSERT(s_اگلا_Id < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    إذا (!mVtxp) أرجع;
    m_mVertices.link_پٹھ(mVtxp);
    إذا (ثابت ترتیب_Logic_راس* ثابت olvp = mVtxp->logicp()) {
        m_لاگت += V3_Instr_گنتی::گنتی(olvp->nodep(), سچ);
    }
}

//######################################################################
// OrderMTaskGraph

ترتیب_MTask_گراف::ترتیب_MTask_گراف(ترتیب_Move_گراف& move_گراف)
    : m_move_گراف{move_گراف}
    , m_entryp{جديد LogicMTask{*هذا, خالی_سوچک}}
    , m_exitp{جديد LogicMTask{*هذا, خالی_سوچک}}
    , m_slowAsserts{v3Global.opt.debug_حصے_کرو()} {}

منطقي ترتیب_MTask_گراف::رستہ_Exists_Impl(LogicMTask* fromp, LogicMTask* چوٹی,
                                     ثابت MTask_کنارا* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    إذا (fromp == چوٹی) أرجع سچ;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    إذا (fromp->cpExclusive<گراف_Way::الٹ_2>() < چوٹی->cpInclusive<گراف_Way::الٹ_2>()) {
        أرجع خطأ;
    }
    إذا (fromp->cpInclusive<گراف_Way::FORWARD>() > چوٹی->cpExclusive<گراف_Way::FORWARD>()) {
        أرجع خطأ;
    }

    // Recursively look for a path
    لكل (ثابت V3_گراف_کنارا& follow : fromp->باہرلا_کنارے()) {
        إذا (&follow == excludedEdgep) استمر;
        LogicMTask* ثابت nextp = static_cast<LogicMTask*>(follow.چوٹی());
        // Don't visit the same MTask twice in the same search.
        إذا (nextp->m_generation == m_currentGeneration) استمر;
        إذا (رستہ_Exists_Impl(nextp, چوٹی, خالی_سوچک)) أرجع سچ;
    }
    أرجع خطأ;
}

سانچا <گراف_Way::en N_Way>
فراغ ترتیب_MTask_گراف::propagate_ڈالو(LogicMTask* mtaskp) {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way inv{way.invert()};
    ثابت uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    لكل (V3_گراف_کنارا& گراف_کنارا : mtaskp->کنارے<way>()) {
        MTask_کنارا& کنارا = static_cast<MTask_کنارا&>(گراف_کنارا);

        LogicMTask* ثابت relativep = کنارا.furtherMTaskp<N_Way>();
        کنارا_ڈھیر::گرہ_2& کنارا_ڈھیر_گرہ = کنارا.m_کنارا_ڈھیر_گرہ[inv];
        إذا (inclusiveCp > کنارا_ڈھیر_گرہ.کلید().m_cp) {
            relativep->m_کنارا_ڈھیر[inv].increase_کلید(&کنارا_ڈھیر_گرہ, inclusiveCp);
        }

        ثابت uint64_t relativeCp = relativep->cpExclusive<way>();

        إذا (relativeCp >= inclusiveCp) استمر;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ثابت uint64_t increment = inclusiveCp - relativeCp;

        Propagate_Pending_ڈھیر::گرہ_2*& pendingNodepRef = relativep->m_propagate_ڈھیر_Nodep;
        إذا (Propagate_Pending_ڈھیر::گرہ_2* ثابت nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            إذا (increment > nodep->کلید().m_increment) {
                m_pending_ڈھیر.increase_کلید(nodep, increment);
            }
            استمر;
        }

        // Add to heap
        Propagate_Pending_ڈھیر::گرہ_2* ثابت nodep = m_pending_گرہ_Pool.alloc();
        pendingNodepRef = nodep;
        m_pending_ڈھیر.داخل_کرو(nodep, {increment, relativep->id(), relativep});
    }
}

سانچا <گراف_Way::en N_Way>
فراغ ترتیب_MTask_گراف::propagateResolve() {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    طالما (!m_pending_ڈھیر.خالی_تھاواں()) {
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
        m_pending_گرہ_Pool.حرر(maxp);
        mtaskp->m_propagate_ڈھیر_Nodep = خالی_سوچک;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ثابت uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        إذا (VL_UNLIKELY(m_slowAsserts)) {
            ثابت uint64_t کنارا_Cp = mtaskp->m_کنارا_ڈھیر[inv].max()->کلید().m_cp;
            UASSERT_OBJ(کنارا_Cp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_ڈالو<N_Way>(mtaskp);
    }
}

uint64_t ترتیب_MTask_گراف::کل_لاگت() ثابت {
    uint64_t لاگت = 0;
    لكل (ثابت V3_گراف_راس& vtx : vertices()) لاگت += static_cast<ثابت LogicMTask&>(vtx).لاگت();
    أرجع لاگت;
}

فراغ ترتیب_MTask_گراف::جوڑو_کنارا(LogicMTask* fromp, LogicMTask* چوٹی) {
    UASSERT_OBJ(fromp != چوٹی, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->has_کنارا_منزل(چوٹی), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    جديد MTask_کنارا{هذا, fromp, چوٹی};

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
    إذا (fromp->cpInclusive<گراف_Way::FORWARD>() > چوٹی->cpExclusive<گراف_Way::FORWARD>()) {
        propagate<گراف_Way::FORWARD>(fromp);
    }
    إذا (چوٹی->cpInclusive<گراف_Way::الٹ_2>() > fromp->cpExclusive<گراف_Way::الٹ_2>()) {
        propagate<گراف_Way::الٹ_2>(چوٹی);
    }
}

فراغ ترتیب_MTask_گراف::ملاؤ_MTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    طالما (MTask_کنارا* ثابت edgep = static_cast<MTask_کنارا*>(donorp->باہرلا_کنارے().frontp())) {
        LogicMTask* ثابت relativep = edgep->منزل_MTaskp();

        relativep->remove_Relative_کنارا<گراف_Way::الٹ_2>(edgep);

        إذا (relativep == recipientp || recipientp->has_کنارا_منزل(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } وإلا {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->جوڑو_Dependent(relativep);
            recipientp->steal_Relative_کنارا<گراف_Way::FORWARD>(edgep);
            relativep->جوڑو_Relative_کنارا<گراف_Way::الٹ_2>(edgep);
        }
    }

    // Process incoming edges of donor
    طالما (MTask_کنارا* ثابت edgep = static_cast<MTask_کنارا*>(donorp->وچ_کنارے().frontp())) {
        LogicMTask* ثابت relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->remove_Relative_کنارا<گراف_Way::FORWARD>(edgep);

        إذا (relativep == recipientp || relativep->has_کنارا_منزل(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } وإلا {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_چوٹی(recipientp);
            relativep->جوڑو_Dependent(recipientp);
            relativep->جوڑو_Relative_کنارا<گراف_Way::FORWARD>(edgep);
            recipientp->steal_Relative_کنارا<گراف_Way::الٹ_2>(edgep);
        }
    }

    // Move the contents of the donor into the recipient, update its cost
    recipientp->m_mVertices.splice(recipientp->m_mVertices.سرا(), donorp->m_mVertices);
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
    VL_DO_DANGLING(donorp->unlinkDelete(هذا), donorp);
}

فراغ ترتیب_MTask_گراف::remove_متعدی_کنارے() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    لكل (V3_گراف_راس& vtx : vertices()) {
        لكل (V3_گراف_کنارا* ثابت گراف_Edgep : vtx.باہرلا_کنارے().unlinkable()) {
            MTask_کنارا* ثابت edgep = static_cast<MTask_کنارا*>(گراف_Edgep);
            LogicMTask* ثابت fromp = edgep->fromMTaskp();
            LogicMTask* ثابت چوٹی = edgep->منزل_MTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            إذا (!رستہ_Exists(fromp, چوٹی, edgep)) استمر;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(چوٹی);
            fromp->remove_Relative_کنارا<گراف_Way::FORWARD>(edgep);
            چوٹی->remove_Relative_کنارا<گراف_Way::الٹ_2>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

فراغ ترتیب_MTask_گراف::remove_خالی_تھاواں_MTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    لكل (V3_گراف_راس* ثابت vtxp : vertices().unlinkable()) {
        LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        إذا (mtaskp == m_entryp || mtaskp == m_exitp) استمر;

        // Keep any MTask that holds logic
        منطقي خالی_تھاواں = سچ;
        لكل (ثابت ترتیب_Move_راس& mVtx : mtaskp->راس_List()) {
            إذا (mVtx.logicp()) {
                خالی_تھاواں = خطأ;
                اكسر;
            }
        }
        إذا (!خالی_تھاواں) استمر;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->لاگت(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        لكل (V3_گراف_کنارا& وچ_کنارا : mtaskp->وچ_کنارے()) {
            LogicMTask* ثابت fromp = static_cast<MTask_کنارا&>(وچ_کنارا).fromMTaskp();
            لكل (V3_گراف_کنارا& باہرلا_کنارا : mtaskp->باہرلا_کنارے()) {
                LogicMTask* ثابت چوٹی = static_cast<MTask_کنارا&>(باہرلا_کنارا).منزل_MTaskp();
                إذا (!fromp->has_کنارا_منزل(چوٹی)) جوڑو_کنارا(fromp, چوٹی);
            }
        }

        // Remove incoming edges of 'mtaskp'
        طالما (MTask_کنارا* ثابت edgep = static_cast<MTask_کنارا*>(mtaskp->وچ_کنارے().frontp())) {
            LogicMTask* ثابت relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->remove_Relative_کنارا<گراف_Way::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        طالما (MTask_کنارا* ثابت edgep = static_cast<MTask_کنارا*>(mtaskp->باہرلا_کنارے().frontp())) {
            LogicMTask* ثابت relativep = edgep->منزل_MTaskp();
            relativep->remove_Relative_کنارا<گراف_Way::الٹ_2>(edgep);
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
سانچا <گراف_Way::en N_Way>
فراغ ترتیب_MTask_گراف::validateWay() ثابت {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way inv = way.invert();
    لكل (ثابت V3_گراف_راس& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cp_لاگت = 0;
        std::unordered_سیٹ<ثابت V3_گراف_راس*> relatives;
        لكل (ثابت V3_گراف_کنارا& گراف_کنارا : mtask.کنارے<inv>()) {
            ثابت MTask_کنارا& کنارا = *گراف_کنارا.as<MTask_کنارا>();
            ثابت LogicMTask& relative = *(کنارا.furtherp<inv>()->سانچا as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(کنارا.بھار_2() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ثابت منطقي پہلا = relatives.داخل_کرو(&relative).دوجا;
            UASSERT_OBJ(پہلا, &mtask, "Should be no redundant edges in MTask graph");
            ثابت uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(کنارا.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(کنارا.cachedId(inv) == relative.id(), &mtask,
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

فراغ ترتیب_MTask_گراف::validate() ثابت {
    إذا (!m_slowAsserts) أرجع;

    validateWay<گراف_Way::FORWARD>();
    validateWay<گراف_Way::الٹ_2>();

    // Check the dependents set of each MTask agrees with its out-edges
    لكل (ثابت V3_گراف_راس& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        ناپ_t nDependents = 0;
        لكل (ثابت V3_گراف_کنارا& گراف_کنارا : mtask.باہرلا_کنارے()) {
            LogicMTask* ثابت چوٹی = گراف_کنارا.as<MTask_کنارا>()->منزل_MTaskp();
            UASSERT_OBJ(mtask.has_کنارا_منزل(چوٹی), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.ناپ() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

صنف ترتیب_MTask_گراف_Builder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ثابت VNUser1_وچ_Use m_user1_وچ_Use;

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
    ساكن منطقي bypassOk(ترتیب_Move_راس* mvtxp) {
        // Need to keep all logic vertices
        إذا (mvtxp->logicp()) أرجع خطأ;
        // Count fan-in, up to 3
        بنانشانی fan_وچ = 0;
        خودکار& وچ_کنارے = mvtxp->وچ_کنارے();
        لكل (خودکار it = وچ_کنارے.begin(); it != وچ_کنارے.سرا(); ++it) {
            إذا (++fan_وچ == 3) اكسر;
        }
        // If fanIn no more than one, bypass
        إذا (fan_وچ <= 1) أرجع سچ;
        // Count fan-out, up to 3
        بنانشانی fan_باہرلا = 0;
        خودکار& باہرلا_کنارے = mvtxp->باہرلا_کنارے();
        لكل (خودکار it = باہرلا_کنارے.begin(); it != باہرلا_کنارے.سرا(); ++it) {
            إذا (++fan_باہرلا == 3) اكسر;
        }
        // If fan-out no more than one, bypass
        إذا (fan_باہرلا <= 1) أرجع سچ;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        أرجع fan_وچ + fan_باہرلا == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    فراغ جوڑو_کنارا(LogicMTask* srcp, LogicMTask* dstp) {
        إذا (srcp->has_کنارا_منزل(dstp)) أرجع;  // Don't create redundant edges.
        m_mtask_گراف.جوڑو_کنارا(srcp, dstp);
    }

    // CONSTRUCTORS
    واضح ترتیب_MTask_گراف_Builder(ترتیب_MTask_گراف& mtask_گراف)
        : m_mtask_گراف{mtask_گراف} {

        // Create the LogicMTasks for each OrderMoveVertex
        لكل (V3_گراف_راس& vtx : mtask_گراف.move_گراف().vertices()) {
            ترتیب_Move_راس& mVtx = static_cast<ترتیب_Move_راس&>(vtx);
            إذا (bypassOk(&mVtx)) {
                mVtx.userp(خالی_سوچک);  // Set to nullptr to mark as bypassed
            } وإلا {
                mVtx.userp(جديد LogicMTask{mtask_گراف, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& اندراج = *mtask_گراف.entryp();
        LogicMTask& باہر_نکلو = *mtask_گراف.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        لكل (V3_گراف_راس& vtx : mtask_گراف.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            إذا (VL_UNLIKELY((&mtask == &اندراج) || (&mtask == &باہر_نکلو))) استمر;

            ترتیب_Move_راس::List& راس_List = mtask.راس_List();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(راس_List.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            ترتیب_Move_راس* ثابت mVtxp = راس_List.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            لكل (ثابت V3_گراف_کنارا& d_کنارا : mVtxp->باہرلا_کنارے()) {
                V3_گراف_راس* ثابت چوٹی = d_کنارا.چوٹی();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                إذا (LogicMTask* ثابت otherp = static_cast<LogicMTask*>(چوٹی->userp())) {
                    جوڑو_کنارا(&mtask, otherp);
                    استمر;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                لكل (ثابت V3_گراف_کنارا& t_کنارا : چوٹی->باہرلا_کنارے()) {
                    LogicMTask* ثابت transp = static_cast<LogicMTask*>(t_کنارا.چوٹی()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    جوڑو_کنارا(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        لكل (V3_گراف_راس& vtx : mtask_گراف.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            إذا (VL_UNLIKELY((&mtask == &اندراج) || (&mtask == &باہر_نکلو))) استمر;
            // Add the entry/exit edges if not otherwise connected
            إذا (mtask.وچ_خالی_تھاواں()) جوڑو_کنارا(&اندراج, &mtask);
            إذا (mtask.باہرلا_خالی_تھاواں()) جوڑو_کنارا(&mtask, &باہر_نکلو);
        }
    }
    ~ترتیب_MTask_گراف_Builder() = افتراضي;
    VL_UNCOPYABLE(ترتیب_MTask_گراف_Builder);
    VL_UNMOVABLE(ترتیب_MTask_گراف_Builder);

عام:
    ساكن فراغ apply(ترتیب_MTask_گراف& mtask_گراف) { ترتیب_MTask_گراف_Builder{mtask_گراف}; }
};

std::unique_ptr<ترتیب_MTask_گراف> ترتیب_MTask_گراف::build(ترتیب_Move_گراف& move_گراف) {
    std::unique_ptr<ترتیب_MTask_گراف> resp{جديد ترتیب_MTask_گراف{move_گراف}};
    ترتیب_MTask_گراف_Builder::apply(*resp);
    resp->validate();
    أرجع resp;
}
