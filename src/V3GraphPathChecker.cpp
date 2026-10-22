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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3GraphPathChecker.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3GraphStream.h"

//######################################################################
// GraphPCNode

ਬਣਤਰ ਗ੍ਰਾਫ_PCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, ਗ੍ਰਾਫ_Way::ਅੰਕ_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_ਵੇਖਿਆ_At_Generation = 0;

    // CONSTRUCTORS
    ਗ੍ਰਾਫ_PCNode() = ਮੂਲ;
    ~ਗ੍ਰਾਫ_PCNode() = ਮੂਲ;
};

//######################################################################
// GraphPathChecker implementation

ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
ਖਾਲੀ ਗ੍ਰਾਫ_ਰਸਤਾ_Checker::initHalfCriticalPaths(ਤਾਰਕਿਕ checkOnly) {
    constexpr ਗ੍ਰਾਫ_Way way{N_Way};
    constexpr ਗ੍ਰਾਫ_Way rev = way.invert();
    ਗ੍ਰਾਫ_ਧਾਰਾ_Unordered ਕ੍ਰਮ(m_graphp, way);
    ਜਦੋਂ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = ਕ੍ਰਮ.nextp()) {
        ਬਿਨਾਨਿਸ਼ਾਨੀ crit_ਰਸਤਾ_ਲਾਗਤ = 0;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਕਿਨਾਰੇ<rev>()) {
            ਜੇ (!m_ਕਿਨਾਰਾ_Funcp(&ਕਿਨਾਰਾ)) ਜਾਰੀ;

            ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* wrelativep = ਕਿਨਾਰਾ.furtherp<rev>();
            ਅਚਲ ਗ੍ਰਾਫ_PCNode* ਅਚਲ wrelUserp = static_cast<ਗ੍ਰਾਫ_PCNode*>(wrelativep->userp());
            crit_ਰਸਤਾ_ਲਾਗਤ = std::max(crit_ਰਸਤਾ_ਲਾਗਤ, wrelUserp->m_cp[way] + 1);
        }

        ਗ੍ਰਾਫ_PCNode* ਅਚਲ ourUserp = static_cast<ਗ੍ਰਾਫ_PCNode*>(vertexp->userp());
        ਜੇ (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == crit_ਰਸਤਾ_ਲਾਗਤ, vertexp,
                        "Validation of critical paths failed");
        } ਨਹੀਂਤਾਂ {
            ourUserp->m_cp[way] = crit_ਰਸਤਾ_ਲਾਗਤ;
        }
    }
}

ਸਾਂਚਾ ਖਾਲੀ ਗ੍ਰਾਫ_ਰਸਤਾ_Checker::initHalfCriticalPaths<ਗ੍ਰਾਫ_Way::FORWARD>(ਤਾਰਕਿਕ);
ਸਾਂਚਾ ਖਾਲੀ ਗ੍ਰਾਫ_ਰਸਤਾ_Checker::initHalfCriticalPaths<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(ਤਾਰਕਿਕ);

ਗ੍ਰਾਫ_ਰਸਤਾ_Checker::ਗ੍ਰਾਫ_ਰਸਤਾ_Checker(V3_ਗ੍ਰਾਫ* graphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp)
    : ਗ੍ਰਾਫ_Alg<V3_ਗ੍ਰਾਫ>{graphp, ਕਿਨਾਰਾ_Funcp} {
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(ਨਵਾਂ ਗ੍ਰਾਫ_PCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<ਗ੍ਰਾਫ_Way::FORWARD>(ਝੂਠ);
    initHalfCriticalPaths<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>(ਝੂਠ);
}

ਗ੍ਰਾਫ_ਰਸਤਾ_Checker::~ਗ੍ਰਾਫ_ਰਸਤਾ_Checker() {
    // Free every GraphPCNode
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_graphp->vertices()) {
        ਅਚਲ ਗ੍ਰਾਫ_PCNode* ਅਚਲ nodep = static_cast<ਗ੍ਰਾਫ_PCNode*>(vtx.userp());
        VL_DO_DANGLING(ਮਿਟਾਓ nodep, nodep);
        vtx.userp(ਖਾਲੀ_ਸੂਚਕ);
    }
}

ਤਾਰਕਿਕ ਗ੍ਰਾਫ_ਰਸਤਾ_Checker::ਰਸਤਾ_Exists_ਅੰਦਰੂਨੀ(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ap, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* bp,
                                          ਬਿਨਾਨਿਸ਼ਾਨੀ* costp) {
    ਗ੍ਰਾਫ_PCNode* ਅਚਲ auserp = static_cast<ਗ੍ਰਾਫ_PCNode*>(ap->userp());
    ਅਚਲ ਗ੍ਰਾਫ_PCNode* ਅਚਲ buserp = static_cast<ਗ੍ਰਾਫ_PCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    ਜੇ (auserp->m_ਵੇਖਿਆ_At_Generation == m_generation) {
        ਜੇ (costp) *costp = 0;
        ਵਾਪਸ ਝੂਠ;
    }
    auserp->m_ਵੇਖਿਆ_At_Generation = m_generation;

    ਜੇ (costp) *costp = 1;  // count 'a' toward the search cost

    ਜੇ (ap == bp) ਵਾਪਸ ਸੱਚ;

    // Rule out an a->b path based on their CPs
    ਜੇ (auserp->m_cp[ਗ੍ਰਾਫ_Way::ਉਲਟ_2] < buserp->m_cp[ਗ੍ਰਾਫ_Way::ਉਲਟ_2] + 1) ਵਾਪਸ ਝੂਠ;
    ਜੇ (buserp->m_cp[ਗ੍ਰਾਫ_Way::FORWARD] < auserp->m_cp[ਗ੍ਰਾਫ_Way::FORWARD] + 1) ਵਾਪਸ ਝੂਠ;

    // Slow path; visit some extended family
    ਤਾਰਕਿਕ ਮਿਲਿਆ_ਰਸਤਾ = ਝੂਠ;
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ap->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        ਜੇ (!m_ਕਿਨਾਰਾ_Funcp(&ਕਿਨਾਰਾ)) ਜਾਰੀ;

        ਬਿਨਾਨਿਸ਼ਾਨੀ ਬੱਚਾ_ਲਾਗਤ;
        ਜੇ (ਰਸਤਾ_Exists_ਅੰਦਰੂਨੀ(ਕਿਨਾਰਾ.ਸਿਖਰ(), bp, &ਬੱਚਾ_ਲਾਗਤ)) ਮਿਲਿਆ_ਰਸਤਾ = ਸੱਚ;
        ਜੇ (costp) *costp += ਬੱਚਾ_ਲਾਗਤ;
    }

    ਵਾਪਸ ਮਿਲਿਆ_ਰਸਤਾ;
}

ਤਾਰਕਿਕ ਗ੍ਰਾਫ_ਰਸਤਾ_Checker::ਰਸਤਾ_Exists_From(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ) {
    incGeneration();
    ਵਾਪਸ ਰਸਤਾ_Exists_ਅੰਦਰੂਨੀ(fromp, ਸਿਖਰ);
}

ਤਾਰਕਿਕ ਗ੍ਰਾਫ_ਰਸਤਾ_Checker::is_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰਾ(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep) {
    ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp = edgep->fromp();
    ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ = edgep->ਸਿਖਰ();
    incGeneration();
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& from_ਬਾਹਰਲਾ : fromp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        ਜੇ (&from_ਬਾਹਰਲਾ == edgep) ਜਾਰੀ;
        ਜੇ (ਰਸਤਾ_Exists_ਅੰਦਰੂਨੀ(from_ਬਾਹਰਲਾ.ਸਿਖਰ(), ਸਿਖਰ)) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}
