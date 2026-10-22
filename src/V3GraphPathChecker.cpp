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

#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

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
    std::array<uint32_t, گراف_Way::عدد_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_تکیا_At_Generation = 0;

    // CONSTRUCTORS
    گراف_PCNode() = افتراضي;
    ~گراف_PCNode() = افتراضي;
};

//######################################################################
// GraphPathChecker implementation

سانچا <گراف_Way::en N_Way>
فراغ گراف_رستہ_Checker::initHalfCriticalPaths(منطقي checkOnly) {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way rev = way.invert();
    گراف_دھارا_Unordered ترتیب(m_graphp, way);
    طالما (ثابت V3_گراف_راس* ثابت vertexp = ترتیب.nextp()) {
        بنانشانی crit_رستہ_لاگت = 0;
        لكل (ثابت V3_گراف_کنارا& کنارا : vertexp->کنارے<rev>()) {
            إذا (!m_کنارا_Funcp(&کنارا)) استمر;

            ثابت V3_گراف_راس* wrelativep = کنارا.furtherp<rev>();
            ثابت گراف_PCNode* ثابت wrelUserp = static_cast<گراف_PCNode*>(wrelativep->userp());
            crit_رستہ_لاگت = std::max(crit_رستہ_لاگت, wrelUserp->m_cp[way] + 1);
        }

        گراف_PCNode* ثابت ourUserp = static_cast<گراف_PCNode*>(vertexp->userp());
        إذا (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == crit_رستہ_لاگت, vertexp,
                        "Validation of critical paths failed");
        } وإلا {
            ourUserp->m_cp[way] = crit_رستہ_لاگت;
        }
    }
}

سانچا فراغ گراف_رستہ_Checker::initHalfCriticalPaths<گراف_Way::FORWARD>(منطقي);
سانچا فراغ گراف_رستہ_Checker::initHalfCriticalPaths<گراف_Way::الٹ_2>(منطقي);

گراف_رستہ_Checker::گراف_رستہ_Checker(V3_گراف* graphp, V3_کنارا_Func_P کنارا_Funcp)
    : گراف_Alg<V3_گراف>{graphp, کنارا_Funcp} {
    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(جديد گراف_PCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<گراف_Way::FORWARD>(خطأ);
    initHalfCriticalPaths<گراف_Way::الٹ_2>(خطأ);
}

گراف_رستہ_Checker::~گراف_رستہ_Checker() {
    // Free every GraphPCNode
    لكل (V3_گراف_راس& vtx : m_graphp->vertices()) {
        ثابت گراف_PCNode* ثابت nodep = static_cast<گراف_PCNode*>(vtx.userp());
        VL_DO_DANGLING(مٹاؤ nodep, nodep);
        vtx.userp(خالی_سوچک);
    }
}

منطقي گراف_رستہ_Checker::رستہ_Exists_اندرونی(ثابت V3_گراف_راس* ap, ثابت V3_گراف_راس* bp,
                                          بنانشانی* costp) {
    گراف_PCNode* ثابت auserp = static_cast<گراف_PCNode*>(ap->userp());
    ثابت گراف_PCNode* ثابت buserp = static_cast<گراف_PCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    إذا (auserp->m_تکیا_At_Generation == m_generation) {
        إذا (costp) *costp = 0;
        أرجع خطأ;
    }
    auserp->m_تکیا_At_Generation = m_generation;

    إذا (costp) *costp = 1;  // count 'a' toward the search cost

    إذا (ap == bp) أرجع سچ;

    // Rule out an a->b path based on their CPs
    إذا (auserp->m_cp[گراف_Way::الٹ_2] < buserp->m_cp[گراف_Way::الٹ_2] + 1) أرجع خطأ;
    إذا (buserp->m_cp[گراف_Way::FORWARD] < auserp->m_cp[گراف_Way::FORWARD] + 1) أرجع خطأ;

    // Slow path; visit some extended family
    منطقي لبھیا_رستہ = خطأ;
    لكل (ثابت V3_گراف_کنارا& کنارا : ap->باہرلا_کنارے()) {
        إذا (!m_کنارا_Funcp(&کنارا)) استمر;

        بنانشانی بچہ_لاگت;
        إذا (رستہ_Exists_اندرونی(کنارا.چوٹی(), bp, &بچہ_لاگت)) لبھیا_رستہ = سچ;
        إذا (costp) *costp += بچہ_لاگت;
    }

    أرجع لبھیا_رستہ;
}

منطقي گراف_رستہ_Checker::رستہ_Exists_From(ثابت V3_گراف_راس* fromp, ثابت V3_گراف_راس* چوٹی) {
    incGeneration();
    أرجع رستہ_Exists_اندرونی(fromp, چوٹی);
}

منطقي گراف_رستہ_Checker::is_متعدی_کنارا(ثابت V3_گراف_کنارا* edgep) {
    ثابت V3_گراف_راس* fromp = edgep->fromp();
    ثابت V3_گراف_راس* چوٹی = edgep->چوٹی();
    incGeneration();
    لكل (ثابت V3_گراف_کنارا& from_باہرلا : fromp->باہرلا_کنارے()) {
        إذا (&from_باہرلا == edgep) استمر;
        إذا (رستہ_Exists_اندرونی(from_باہرلا.چوٹی(), چوٹی)) أرجع سچ;
    }
    أرجع خطأ;
}
