// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: DAG Path Checking
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

#وصف_ڪريو VL_MT_DISABLED_ڪوڊ_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3GraphPathChecker.h"

#ضمن "V3Global.h"
#ضمن "V3GraphStream.h"

//######################################################################
// GraphPCNode

بنية گراف_PCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, گراف_Way::انگ_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_ڏٺل_At_Generation = 0;

    // CONSTRUCTORS
    گراف_PCNode() = افتراضي;
    ~گراف_PCNode() = افتراضي;
};

//######################################################################
// GraphPathChecker implementation

سانچو <گراف_Way::en N_Way>
فراغ گراف_رستو_Checker::initHalfCriticalPaths(منطقي checkOnly) {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way rev = way.invert();
    گراف_وهڪرو_Unordered ترتيب(m_graphp, way);
    طالما (ثابت V3_گراف_راس* ثابت vertexp = ترتيب.nextp()) {
        بنانشان crit_رستو_لاڳت = 0;
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vertexp->ڪنارا<rev>()) {
            إذا (!m_ڪنارو_Funcp(&ڪنارو)) استمر;

            ثابت V3_گراف_راس* wrelativep = ڪنارو.furtherp<rev>();
            ثابت گراف_PCNode* ثابت wrelUserp = static_cast<گراف_PCNode*>(wrelativep->userp());
            crit_رستو_لاڳت = std::max(crit_رستو_لاڳت, wrelUserp->m_cp[way] + 1);
        }

        گراف_PCNode* ثابت ourUserp = static_cast<گراف_PCNode*>(vertexp->userp());
        إذا (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == crit_رستو_لاڳت, vertexp,
                        "Validation of critical paths failed");
        } وإلا {
            ourUserp->m_cp[way] = crit_رستو_لاڳت;
        }
    }
}

سانچو فراغ گراف_رستو_Checker::initHalfCriticalPaths<گراف_Way::FORWARD>(منطقي);
سانچو فراغ گراف_رستو_Checker::initHalfCriticalPaths<گراف_Way::ابتو_2>(منطقي);

گراف_رستو_Checker::گراف_رستو_Checker(V3_گراف* graphp, V3_ڪنارو_Func_P ڪنارو_Funcp)
    : گراف_Alg<V3_گراف>{graphp, ڪنارو_Funcp} {
    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(جديد گراف_PCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<گراف_Way::FORWARD>(خطأ);
    initHalfCriticalPaths<گراف_Way::ابتو_2>(خطأ);
}

گراف_رستو_Checker::~گراف_رستو_Checker() {
    // Free every GraphPCNode
    لكل (V3_گراف_راس& vtx : m_graphp->vertices()) {
        ثابت گراف_PCNode* ثابت nodep = static_cast<گراف_PCNode*>(vtx.userp());
        VL_DO_DANGLING(ميٽيو nodep, nodep);
        vtx.userp(خالي_اشارو);
    }
}

منطقي گراف_رستو_Checker::رستو_Exists_اندروني(ثابت V3_گراف_راس* ap, ثابت V3_گراف_راس* bp,
                                          بنانشان* costp) {
    گراف_PCNode* ثابت auserp = static_cast<گراف_PCNode*>(ap->userp());
    ثابت گراف_PCNode* ثابت buserp = static_cast<گراف_PCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    إذا (auserp->m_ڏٺل_At_Generation == m_generation) {
        إذا (costp) *costp = 0;
        أرجع خطأ;
    }
    auserp->m_ڏٺل_At_Generation = m_generation;

    إذا (costp) *costp = 1;  // count 'a' toward the search cost

    إذا (ap == bp) أرجع سچ;

    // Rule out an a->b path based on their CPs
    إذا (auserp->m_cp[گراف_Way::ابتو_2] < buserp->m_cp[گراف_Way::ابتو_2] + 1) أرجع خطأ;
    إذا (buserp->m_cp[گراف_Way::FORWARD] < auserp->m_cp[گراف_Way::FORWARD] + 1) أرجع خطأ;

    // Slow path; visit some extended family
    منطقي مليو_رستو = خطأ;
    لكل (ثابت V3_گراف_ڪنارو& ڪنارو : ap->ٻاهريون_ڪنارا()) {
        إذا (!m_ڪنارو_Funcp(&ڪنارو)) استمر;

        بنانشان ٻار_لاڳت;
        إذا (رستو_Exists_اندروني(ڪنارو.چوٽي(), bp, &ٻار_لاڳت)) مليو_رستو = سچ;
        إذا (costp) *costp += ٻار_لاڳت;
    }

    أرجع مليو_رستو;
}

منطقي گراف_رستو_Checker::رستو_Exists_From(ثابت V3_گراف_راس* fromp, ثابت V3_گراف_راس* چوٽي) {
    incGeneration();
    أرجع رستو_Exists_اندروني(fromp, چوٽي);
}

منطقي گراف_رستو_Checker::is_متعدي_ڪنارو(ثابت V3_گراف_ڪنارو* edgep) {
    ثابت V3_گراف_راس* fromp = edgep->fromp();
    ثابت V3_گراف_راس* چوٽي = edgep->چوٽي();
    incGeneration();
    لكل (ثابت V3_گراف_ڪنارو& from_ٻاهريون : fromp->ٻاهريون_ڪنارا()) {
        إذا (&from_ٻاهريون == edgep) استمر;
        إذا (رستو_Exists_اندروني(from_ٻاهريون.چوٽي(), چوٽي)) أرجع سچ;
    }
    أرجع خطأ;
}
