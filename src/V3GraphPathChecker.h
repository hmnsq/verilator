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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3GRAPHPATHCHECKER_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3GRAPHPATHCHECKER_H_

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3GraphAlg.h"

//######################################################################

/// Implement pathExistsFrom() with some caching to prune the search.
/// Far more aggressive caching/pruning is possible; for now the use cases
/// don't rely so heavily on this class that it's necessary.
///
/// The graph (or at least, the subset the algorithm sees through
/// edgeFuncp) must not change during the lifetime of the checker.
class ನಕ್ಷೆ_ಮಾರ್ಗ_Checker final : ನಕ್ಷೆ_Alg<V3_ನಕ್ಷೆ> {
    // Count "generations" which increases on operations that scan through
    // the graph. Each node is marked with the last generation that scanned
    // it, to enable asserting there are no cycles, and to avoid recursing
    // through the same node twice while searching for a path.
    uint64_t m_generation = 0;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ನಕ್ಷೆ_ಮಾರ್ಗ_Checker(V3_ನಕ್ಷೆ* graphp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp
                                               = V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue) VL_MT_DISABLED;
    ~ನಕ್ಷೆ_ಮಾರ್ಗ_Checker() VL_MT_DISABLED;

    // METHODS
    bool ಮಾರ್ಗ_Exists_From(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ) VL_MT_DISABLED;

    // If have edges A->B, B->C, and A->C then A->C is considered a
    // "transitive" edge (implied by A->B and B->C) and it could be safely
    // removed. Detect such an edge.
    bool is_ಸಂಕ್ರಾಮಿಕ_ಅಂಚು(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* edgep) VL_MT_DISABLED;

ಖಾಸಗಿ:
    bool ಮಾರ್ಗ_Exists_ಆಂತರಿಕ(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ap, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* bp,
                            ಚಿಹ್ನೆರಹಿತ* costp = nullptr) VL_MT_DISABLED;
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಶೂನ್ಯ initHalfCriticalPaths(bool checkOnly) VL_MT_DISABLED;
    ಶೂನ್ಯ incGeneration() { ++m_generation; }

    VL_UNCOPYABLE(ನಕ್ಷೆ_ಮಾರ್ಗ_Checker);
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
