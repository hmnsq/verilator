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

#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3GraphPathChecker.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3GraphStream.h"

//######################################################################
// GraphPCNode

ರಚನೆ ನಕ್ಷೆ_PCNode final {
    // User data for each node in GraphPathChecker.
    //
    // Like the LogicMTasks's, store before and after CPs for the nodes in
    // the GraphPathChecker graph.
    //
    // Unlike the LogicMTasks's, we have no cost info for the generic graph
    // accepted by GraphPathChecker, so assume each node has unit cost.
    std::array<uint32_t, ನಕ್ಷೆ_Way::ಅಂಕೆ_WAYS> m_cp = {};

    // Detect if we've seen this node before in a given recursive
    // operation. We'll use this in pathExistsInternal() to avoid checking
    // the same node twice, and again in updateHalfCriticalPath() to assert
    // there are no cycles.
    uint64_t m_ನೋಡಿದ_At_Generation = 0;

    // CONSTRUCTORS
    ನಕ್ಷೆ_PCNode() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~ನಕ್ಷೆ_PCNode() = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// GraphPathChecker implementation

ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
ಶೂನ್ಯ ನಕ್ಷೆ_ಮಾರ್ಗ_Checker::initHalfCriticalPaths(ತಾರ್ಕಿಕ checkOnly) {
    constexpr ನಕ್ಷೆ_Way way{N_Way};
    constexpr ನಕ್ಷೆ_Way rev = way.invert();
    ನಕ್ಷೆ_ಹರಿವು_Unordered ಕ್ರಮ(m_graphp, way);
    ಆಗ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = ಕ್ರಮ.nextp()) {
        ಚಿಹ್ನೆರಹಿತ crit_ಮಾರ್ಗ_ವೆಚ್ಚ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಅಂಚುಗಳು<rev>()) {
            ಆದರೆ (!m_ಅಂಚು_Funcp(&ಅಂಚು)) ಮುಂದುವರಿ;

            ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* wrelativep = ಅಂಚು.furtherp<rev>();
            ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_PCNode* ಸ್ಥಿರಾಂಕ wrelUserp = static_cast<ನಕ್ಷೆ_PCNode*>(wrelativep->userp());
            crit_ಮಾರ್ಗ_ವೆಚ್ಚ = std::max(crit_ಮಾರ್ಗ_ವೆಚ್ಚ, wrelUserp->m_cp[way] + 1);
        }

        ನಕ್ಷೆ_PCNode* ಸ್ಥಿರಾಂಕ ourUserp = static_cast<ನಕ್ಷೆ_PCNode*>(vertexp->userp());
        ಆದರೆ (checkOnly) {
            UASSERT_OBJ(ourUserp->m_cp[way] == crit_ಮಾರ್ಗ_ವೆಚ್ಚ, vertexp,
                        "Validation of critical paths failed");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ourUserp->m_cp[way] = crit_ಮಾರ್ಗ_ವೆಚ್ಚ;
        }
    }
}

ಮಾದರಿ ಶೂನ್ಯ ನಕ್ಷೆ_ಮಾರ್ಗ_Checker::initHalfCriticalPaths<ನಕ್ಷೆ_Way::FORWARD>(ತಾರ್ಕಿಕ);
ಮಾದರಿ ಶೂನ್ಯ ನಕ್ಷೆ_ಮಾರ್ಗ_Checker::initHalfCriticalPaths<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(ತಾರ್ಕಿಕ);

ನಕ್ಷೆ_ಮಾರ್ಗ_Checker::ನಕ್ಷೆ_ಮಾರ್ಗ_Checker(V3_ನಕ್ಷೆ* graphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp)
    : ನಕ್ಷೆ_Alg<V3_ನಕ್ಷೆ>{graphp, ಅಂಚು_Funcp} {
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
        // Setup tracking structure for each node.  If delete a vertex
        // there would be a leak, but ok as accept only const V3Graph*'s.
        vtx.userp(ಹೊಸ ನಕ್ಷೆ_PCNode);
    }
    // Init critical paths in userp() for each vertex
    initHalfCriticalPaths<ನಕ್ಷೆ_Way::FORWARD>(ಸುಳ್ಳು);
    initHalfCriticalPaths<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>(ಸುಳ್ಳು);
}

ನಕ್ಷೆ_ಮಾರ್ಗ_Checker::~ನಕ್ಷೆ_ಮಾರ್ಗ_Checker() {
    // Free every GraphPCNode
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_graphp->vertices()) {
        ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_PCNode* ಸ್ಥಿರಾಂಕ nodep = static_cast<ನಕ್ಷೆ_PCNode*>(vtx.userp());
        VL_DO_DANGLING(ಅಳಿಸು nodep, nodep);
        vtx.userp(ಶೂನ್ಯಸೂಚಕ);
    }
}

ತಾರ್ಕಿಕ ನಕ್ಷೆ_ಮಾರ್ಗ_Checker::ಮಾರ್ಗ_Exists_ಆಂತರಿಕ(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ap, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* bp,
                                          ಚಿಹ್ನೆರಹಿತ* costp) {
    ನಕ್ಷೆ_PCNode* ಸ್ಥಿರಾಂಕ auserp = static_cast<ನಕ್ಷೆ_PCNode*>(ap->userp());
    ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_PCNode* ಸ್ಥಿರಾಂಕ buserp = static_cast<ನಕ್ಷೆ_PCNode*>(bp->userp());

    // If have already searched this node on the current search, don't
    // recurse through it again. Since we're still searching, we must not
    // have found a path on the first go either.
    ಆದರೆ (auserp->m_ನೋಡಿದ_At_Generation == m_generation) {
        ಆದರೆ (costp) *costp = 0;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    auserp->m_ನೋಡಿದ_At_Generation = m_generation;

    ಆದರೆ (costp) *costp = 1;  // count 'a' toward the search cost

    ಆದರೆ (ap == bp) ಹಿಂತಿರುಗಿ ನಿಜ;

    // Rule out an a->b path based on their CPs
    ಆದರೆ (auserp->m_cp[ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2] < buserp->m_cp[ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2] + 1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (buserp->m_cp[ನಕ್ಷೆ_Way::FORWARD] < auserp->m_cp[ನಕ್ಷೆ_Way::FORWARD] + 1) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

    // Slow path; visit some extended family
    ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು_ಮಾರ್ಗ = ಸುಳ್ಳು;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ap->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        ಆದರೆ (!m_ಅಂಚು_Funcp(&ಅಂಚು)) ಮುಂದುವರಿ;

        ಚಿಹ್ನೆರಹಿತ ಮಗು_ವೆಚ್ಚ;
        ಆದರೆ (ಮಾರ್ಗ_Exists_ಆಂತರಿಕ(ಅಂಚು.ತುದಿ(), bp, &ಮಗು_ವೆಚ್ಚ)) ಸಿಕ್ಕಿತು_ಮಾರ್ಗ = ನಿಜ;
        ಆದರೆ (costp) *costp += ಮಗು_ವೆಚ್ಚ;
    }

    ಹಿಂತಿರುಗಿ ಸಿಕ್ಕಿತು_ಮಾರ್ಗ;
}

ತಾರ್ಕಿಕ ನಕ್ಷೆ_ಮಾರ್ಗ_Checker::ಮಾರ್ಗ_Exists_From(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ) {
    incGeneration();
    ಹಿಂತಿರುಗಿ ಮಾರ್ಗ_Exists_ಆಂತರಿಕ(fromp, ತುದಿ);
}

ತಾರ್ಕಿಕ ನಕ್ಷೆ_ಮಾರ್ಗ_Checker::is_ಸಂಕ್ರಾಮಿಕ_ಅಂಚು(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* edgep) {
    ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp = edgep->fromp();
    ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ = edgep->ತುದಿ();
    incGeneration();
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& from_ಹೊರಹೋಗು : fromp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        ಆದರೆ (&from_ಹೊರಹೋಗು == edgep) ಮುಂದುವರಿ;
        ಆದರೆ (ಮಾರ್ಗ_Exists_ಆಂತರಿಕ(from_ಹೊರಹೋಗು.ತುದಿ(), ತುದಿ)) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}
