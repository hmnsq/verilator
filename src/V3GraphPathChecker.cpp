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

#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3GraphPathChecker.h"

#ضمن "V3Global.h"
#ضمن "V3GraphStream.h"

//######################################################################
// GraphPCNode

بنية ګراف_PCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, ګراف_Way::عدد_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_کتل_شوی_At_Generation = 0;

    // CONSTRUCTORS
    ګراف_PCNode() = افتراضي;
    ~ګراف_PCNode() = افتراضي;
};

//######################################################################
// GraphPathChecker implementation

کينډۍ <ګراف_Way::en N_Way>
فراغ ګراف_لار_Checker::initHalfCriticalPaths(منطقي checkOnly) {
    constexpr ګراف_Way way{N_Way};
    constexpr ګراف_Way rev = way.invert();
    ګراف_بهیر_Unordered ترتیب(m_graphp, way);
    طالما (ثابت V3_ګراف_راس* ثابت vertexp = ترتیب.nextp()) {
        بینښه crit_لار_لګښت = 0;
        لكل (ثابت V3_ګراف_څنډه& څنډه : vertexp->څنډې<rev>()) {
            إذا (!m_څنډه_Funcp(&څنډه)) استمر;

            ثابت V3_ګراف_راس* wrelativep = څنډه.furtherp<rev>();
            ثابت ګراف_PCNode* ثابت wrelUserp = static_cast<ګراف_PCNode*>(wrelativep->userp());
            crit_لار_لګښت = std::max(crit_لار_لګښت, wrelUserp->m_cp[way] + 1);
        }

        ګراف_PCNode* ثابت ourUserp = static_cast<ګراف_PCNode*>(vertexp->userp());
        إذا (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == crit_لار_لګښت, vertexp,
                        "Validation of critical paths failed");
        } وإلا {
            ourUserp->m_cp[way] = crit_لار_لګښت;
        }
    }
}

کينډۍ فراغ ګراف_لار_Checker::initHalfCriticalPaths<ګراف_Way::FORWARD>(منطقي);
کينډۍ فراغ ګراف_لار_Checker::initHalfCriticalPaths<ګراف_Way::سرچپه_2>(منطقي);

ګراف_لار_Checker::ګراف_لار_Checker(V3_ګراف* graphp, V3_څنډه_Func_P څنډه_Funcp)
    : ګراف_Alg<V3_ګراف>{graphp, څنډه_Funcp} {
    لكل (V3_ګراف_راس& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(جديد ګراف_PCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<ګراف_Way::FORWARD>(خطأ);
    initHalfCriticalPaths<ګراف_Way::سرچپه_2>(خطأ);
}

ګراف_لار_Checker::~ګراف_لار_Checker() {
    // Free every GraphPCNode
    لكل (V3_ګراف_راس& vtx : m_graphp->vertices()) {
        ثابت ګراف_PCNode* ثابت nodep = static_cast<ګراف_PCNode*>(vtx.userp());
        VL_DO_DANGLING(ړنګول nodep, nodep);
        vtx.userp(تش_اشاره);
    }
}

منطقي ګراف_لار_Checker::لار_Exists_دننی(ثابت V3_ګراف_راس* ap, ثابت V3_ګراف_راس* bp,
                                          بینښه* costp) {
    ګراف_PCNode* ثابت auserp = static_cast<ګراف_PCNode*>(ap->userp());
    ثابت ګراف_PCNode* ثابت buserp = static_cast<ګراف_PCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    إذا (auserp->m_کتل_شوی_At_Generation == m_generation) {
        إذا (costp) *costp = 0;
        أرجع خطأ;
    }
    auserp->m_کتل_شوی_At_Generation = m_generation;

    إذا (costp) *costp = 1;  // count 'a' toward the search cost

    إذا (ap == bp) أرجع ریښتیا;

    // Rule out an a->b path based on their CPs
    إذا (auserp->m_cp[ګراف_Way::سرچپه_2] < buserp->m_cp[ګراف_Way::سرچپه_2] + 1) أرجع خطأ;
    إذا (buserp->m_cp[ګراف_Way::FORWARD] < auserp->m_cp[ګراف_Way::FORWARD] + 1) أرجع خطأ;

    // Slow path; visit some extended family
    منطقي وموندل_لار = خطأ;
    لكل (ثابت V3_ګراف_څنډه& څنډه : ap->بهر_څنډې()) {
        إذا (!m_څنډه_Funcp(&څنډه)) استمر;

        بینښه ماشوم_لګښت;
        إذا (لار_Exists_دننی(څنډه.سر(), bp, &ماشوم_لګښت)) وموندل_لار = ریښتیا;
        إذا (costp) *costp += ماشوم_لګښت;
    }

    أرجع وموندل_لار;
}

منطقي ګراف_لار_Checker::لار_Exists_From(ثابت V3_ګراف_راس* fromp, ثابت V3_ګراف_راس* سر) {
    incGeneration();
    أرجع لار_Exists_دننی(fromp, سر);
}

منطقي ګراف_لار_Checker::is_متعدي_څنډه(ثابت V3_ګراف_څنډه* edgep) {
    ثابت V3_ګراف_راس* fromp = edgep->fromp();
    ثابت V3_ګراف_راس* سر = edgep->سر();
    incGeneration();
    لكل (ثابت V3_ګراف_څنډه& from_بهر : fromp->بهر_څنډې()) {
        إذا (&from_بهر == edgep) استمر;
        إذا (لار_Exists_دننی(from_بهر.سر(), سر)) أرجع ریښتیا;
    }
    أرجع خطأ;
}
