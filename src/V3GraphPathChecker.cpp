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

#تعریف_کن VL_MT_DISABLED_کد_UNIT 1

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
    std::array<uint32_t, گراف_Way::رقم_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_دیده_At_Generation = 0;

    // CONSTRUCTORS
    گراف_PCNode() = افتراضي;
    ~گراف_PCNode() = افتراضي;
};

//######################################################################
// GraphPathChecker implementation

قالب <گراف_Way::en N_Way>
فراغ گراف_مسیر_Checker::initHalfCriticalPaths(منطقي checkOnly) {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way rev = way.invert();
    گراف_جریان_Unordered ترتیب(m_graphp, way);
    طالما (ثابت V3_گراف_رأس* ثابت vertexp = ترتیب.nextp()) {
        بیعلامت crit_مسیر_هزینه = 0;
        لكل (ثابت V3_گراف_یال& یال : vertexp->یالها<rev>()) {
            إذا (!m_یال_Funcp(&یال)) استمر;

            ثابت V3_گراف_رأس* wrelativep = یال.furtherp<rev>();
            ثابت گراف_PCNode* ثابت wrelUserp = static_cast<گراف_PCNode*>(wrelativep->userp());
            crit_مسیر_هزینه = std::max(crit_مسیر_هزینه, wrelUserp->m_cp[way] + 1);
        }

        گراف_PCNode* ثابت ourUserp = static_cast<گراف_PCNode*>(vertexp->userp());
        إذا (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == crit_مسیر_هزینه, vertexp,
                        "Validation of critical paths failed");
        } وإلا {
            ourUserp->m_cp[way] = crit_مسیر_هزینه;
        }
    }
}

قالب فراغ گراف_مسیر_Checker::initHalfCriticalPaths<گراف_Way::FORWARD>(منطقي);
قالب فراغ گراف_مسیر_Checker::initHalfCriticalPaths<گراف_Way::معکوس_2>(منطقي);

گراف_مسیر_Checker::گراف_مسیر_Checker(V3_گراف* graphp, V3_یال_Func_P یال_Funcp)
    : گراف_Alg<V3_گراف>{graphp, یال_Funcp} {
    لكل (V3_گراف_رأس& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(جديد گراف_PCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<گراف_Way::FORWARD>(خطأ);
    initHalfCriticalPaths<گراف_Way::معکوس_2>(خطأ);
}

گراف_مسیر_Checker::~گراف_مسیر_Checker() {
    // Free every GraphPCNode
    لكل (V3_گراف_رأس& vtx : m_graphp->vertices()) {
        ثابت گراف_PCNode* ثابت nodep = static_cast<گراف_PCNode*>(vtx.userp());
        VL_DO_DANGLING(حذف nodep, nodep);
        vtx.userp(اشاره_تهی);
    }
}

منطقي گراف_مسیر_Checker::مسیر_Exists_داخلی(ثابت V3_گراف_رأس* ap, ثابت V3_گراف_رأس* bp,
                                          بیعلامت* costp) {
    گراف_PCNode* ثابت auserp = static_cast<گراف_PCNode*>(ap->userp());
    ثابت گراف_PCNode* ثابت buserp = static_cast<گراف_PCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    إذا (auserp->m_دیده_At_Generation == m_generation) {
        إذا (costp) *costp = 0;
        أرجع خطأ;
    }
    auserp->m_دیده_At_Generation = m_generation;

    إذا (costp) *costp = 1;  // count 'a' toward the search cost

    إذا (ap == bp) أرجع درست;

    // Rule out an a->b path based on their CPs
    إذا (auserp->m_cp[گراف_Way::معکوس_2] < buserp->m_cp[گراف_Way::معکوس_2] + 1) أرجع خطأ;
    إذا (buserp->m_cp[گراف_Way::FORWARD] < auserp->m_cp[گراف_Way::FORWARD] + 1) أرجع خطأ;

    // Slow path; visit some extended family
    منطقي یافت_مسیر = خطأ;
    لكل (ثابت V3_گراف_یال& یال : ap->بیرون_یالها()) {
        إذا (!m_یال_Funcp(&یال)) استمر;

        بیعلامت فرزند_هزینه;
        إذا (مسیر_Exists_داخلی(یال.بالاترین(), bp, &فرزند_هزینه)) یافت_مسیر = درست;
        إذا (costp) *costp += فرزند_هزینه;
    }

    أرجع یافت_مسیر;
}

منطقي گراف_مسیر_Checker::مسیر_Exists_From(ثابت V3_گراف_رأس* fromp, ثابت V3_گراف_رأس* بالاترین) {
    incGeneration();
    أرجع مسیر_Exists_داخلی(fromp, بالاترین);
}

منطقي گراف_مسیر_Checker::is_متعدی_یال(ثابت V3_گراف_یال* edgep) {
    ثابت V3_گراف_رأس* fromp = edgep->fromp();
    ثابت V3_گراف_رأس* بالاترین = edgep->بالاترین();
    incGeneration();
    لكل (ثابت V3_گراف_یال& from_بیرون : fromp->بیرون_یالها()) {
        إذا (&from_بیرون == edgep) استمر;
        إذا (مسیر_Exists_داخلی(from_بیرون.بالاترین(), بالاترین)) أرجع درست;
    }
    أرجع خطأ;
}
