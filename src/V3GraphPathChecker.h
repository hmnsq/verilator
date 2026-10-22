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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3GRAPHPATHCHECKER_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3GRAPHPATHCHECKER_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3GraphAlg.h"

//######################################################################

/// Implement pathExistsFrom() with some caching to prune the search.
/// Far more aggressive caching/pruning is possible; for now the use cases
/// don't rely so heavily on this class that it's necessary.
///
/// The graph (or at least, the subset the algorithm sees through
/// edgeFuncp) must not change during the lifetime of the checker.
class ਗ੍ਰਾਫ_ਰਸਤਾ_Checker final : ਗ੍ਰਾਫ_Alg<V3_ਗ੍ਰਾਫ> {
    // Count "generations" which increases on operations that scan through
    // the graph. Each node is marked with the last generation that scanned
    // it, to enable asserting there are no cycles, and to avoid recursing
    // through the same node twice while searching for a path.
    uint64_t m_generation = 0;

ਜਨਤਕ:
    // CONSTRUCTORS
    explicit ਗ੍ਰਾਫ_ਰਸਤਾ_Checker(V3_ਗ੍ਰਾਫ* graphp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp
                                               = V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue) VL_MT_DISABLED;
    ~ਗ੍ਰਾਫ_ਰਸਤਾ_Checker() VL_MT_DISABLED;

    // METHODS
    bool ਰਸਤਾ_Exists_From(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ) VL_MT_DISABLED;

    // If have edges A->B, B->C, and A->C then A->C is considered a
    // "transitive" edge (implied by A->B and B->C) and it could be safely
    // removed. Detect such an edge.
    bool is_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰਾ(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep) VL_MT_DISABLED;

ਨਿਜੀ:
    bool ਰਸਤਾ_Exists_ਅੰਦਰੂਨੀ(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ap, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* bp,
                            ਬਿਨਾਨਿਸ਼ਾਨੀ* costp = nullptr) VL_MT_DISABLED;
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਖਾਲੀ initHalfCriticalPaths(bool checkOnly) VL_MT_DISABLED;
    ਖਾਲੀ incGeneration() { ++m_generation; }

    VL_UNCOPYABLE(ਗ੍ਰਾਫ_ਰਸਤਾ_Checker);
};

#ਜੇ_ਅੰਤ  // Guard
