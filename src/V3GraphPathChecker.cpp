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

#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3GraphPathChecker.h"

#شامل "V3Global.h"
#شامل "V3GraphStream.h"

//######################################################################
// GraphPCNode

ساخت گراف_PCNode final {
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
    uint64_t m_نظر_آیا_At_Generation = 0;

    // CONSTRUCTORS
    گراف_PCNode() = طےشدہ;
    ~گراف_PCNode() = طےشدہ;
};

//######################################################################
// GraphPathChecker implementation

سانچہ <گراف_Way::en N_Way>
خالی گراف_راستہ_Checker::initHalfCriticalPaths(منطقی checkOnly) {
    constexpr گراف_Way way{N_Way};
    constexpr گراف_Way rev = way.invert();
    گراف_دھارا_Unordered ترتیب(m_graphp, way);
    جب (ثابت V3_گراف_راس* ثابت vertexp = ترتیب.nextp()) {
        غیرعلامتی crit_راستہ_لاگت = 0;
        برائے (ثابت V3_گراف_کنارہ& کنارہ : vertexp->کنارے<rev>()) {
            اگر (!m_کنارہ_Funcp(&کنارہ)) جاری;

            ثابت V3_گراف_راس* wrelativep = کنارہ.furtherp<rev>();
            ثابت گراف_PCNode* ثابت wrelUserp = static_cast<گراف_PCNode*>(wrelativep->userp());
            crit_راستہ_لاگت = std::max(crit_راستہ_لاگت, wrelUserp->m_cp[way] + 1);
        }

        گراف_PCNode* ثابت ourUserp = static_cast<گراف_PCNode*>(vertexp->userp());
        اگر (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == crit_راستہ_لاگت, vertexp,
                        "Validation of critical paths failed");
        } ورنہ {
            ourUserp->m_cp[way] = crit_راستہ_لاگت;
        }
    }
}

سانچہ خالی گراف_راستہ_Checker::initHalfCriticalPaths<گراف_Way::FORWARD>(منطقی);
سانچہ خالی گراف_راستہ_Checker::initHalfCriticalPaths<گراف_Way::الٹ_2>(منطقی);

گراف_راستہ_Checker::گراف_راستہ_Checker(V3_گراف* graphp, V3_کنارہ_Func_P کنارہ_Funcp)
    : گراف_Alg<V3_گراف>{graphp, کنارہ_Funcp} {
    برائے (V3_گراف_راس& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(نیا گراف_PCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<گراف_Way::FORWARD>(جھوٹ);
    initHalfCriticalPaths<گراف_Way::الٹ_2>(جھوٹ);
}

گراف_راستہ_Checker::~گراف_راستہ_Checker() {
    // Free every GraphPCNode
    برائے (V3_گراف_راس& vtx : m_graphp->vertices()) {
        ثابت گراف_PCNode* ثابت nodep = static_cast<گراف_PCNode*>(vtx.userp());
        VL_DO_DANGLING(حذف nodep, nodep);
        vtx.userp(خالی_اشاریہ);
    }
}

منطقی گراف_راستہ_Checker::راستہ_Exists_اندرونی(ثابت V3_گراف_راس* ap, ثابت V3_گراف_راس* bp,
                                          غیرعلامتی* costp) {
    گراف_PCNode* ثابت auserp = static_cast<گراف_PCNode*>(ap->userp());
    ثابت گراف_PCNode* ثابت buserp = static_cast<گراف_PCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    اگر (auserp->m_نظر_آیا_At_Generation == m_generation) {
        اگر (costp) *costp = 0;
        واپس جھوٹ;
    }
    auserp->m_نظر_آیا_At_Generation = m_generation;

    اگر (costp) *costp = 1;  // count 'a' toward the search cost

    اگر (ap == bp) واپس سچ;

    // Rule out an a->b path based on their CPs
    اگر (auserp->m_cp[گراف_Way::الٹ_2] < buserp->m_cp[گراف_Way::الٹ_2] + 1) واپس جھوٹ;
    اگر (buserp->m_cp[گراف_Way::FORWARD] < auserp->m_cp[گراف_Way::FORWARD] + 1) واپس جھوٹ;

    // Slow path; visit some extended family
    منطقی ملا_راستہ = جھوٹ;
    برائے (ثابت V3_گراف_کنارہ& کنارہ : ap->برآمد_کنارے()) {
        اگر (!m_کنارہ_Funcp(&کنارہ)) جاری;

        غیرعلامتی بچہ_لاگت;
        اگر (راستہ_Exists_اندرونی(کنارہ.چوٹی(), bp, &بچہ_لاگت)) ملا_راستہ = سچ;
        اگر (costp) *costp += بچہ_لاگت;
    }

    واپس ملا_راستہ;
}

منطقی گراف_راستہ_Checker::راستہ_Exists_From(ثابت V3_گراف_راس* fromp, ثابت V3_گراف_راس* چوٹی) {
    incGeneration();
    واپس راستہ_Exists_اندرونی(fromp, چوٹی);
}

منطقی گراف_راستہ_Checker::is_متعدی_کنارہ(ثابت V3_گراف_کنارہ* edgep) {
    ثابت V3_گراف_راس* fromp = edgep->fromp();
    ثابت V3_گراف_راس* چوٹی = edgep->چوٹی();
    incGeneration();
    برائے (ثابت V3_گراف_کنارہ& from_برآمد : fromp->برآمد_کنارے()) {
        اگر (&from_برآمد == edgep) جاری;
        اگر (راستہ_Exists_اندرونی(from_برآمد.چوٹی(), چوٹی)) واپس سچ;
    }
    واپس جھوٹ;
}
