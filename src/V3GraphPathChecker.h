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

#اگر_غیر_تعریف_شدہ VERILATOR_V3GRAPHPATHCHECKER_H_
#تعریف_کرو VERILATOR_V3GRAPHPATHCHECKER_H_

#شامل "V3Error.h"
#شامل "V3Graph.h"
#شامل "V3GraphAlg.h"

//######################################################################

/// Implement pathExistsFrom() with some caching to prune the search.
/// Far more aggressive caching/pruning is possible; for now the use cases
/// don't rely so heavily on this class that it's necessary.
///
/// The graph (or at least, the subset the algorithm sees through
/// edgeFuncp) must not change during the lifetime of the checker.
class گراف_راستہ_Checker final : گراف_Alg<V3_گراف> {
    // Count "generations" which increases on operations that scan through
    // the graph. Each node is marked with the last generation that scanned
    // it, to enable asserting there are no cycles, and to avoid recursing
    // through the same node twice while searching for a path.
    uint64_t m_generation = 0;

عوامی:
    // CONSTRUCTORS
    explicit گراف_راستہ_Checker(V3_گراف* graphp, V3_کنارہ_Func_P کنارہ_Funcp
                                               = V3_گراف_کنارہ::followAlwaysTrue) VL_MT_DISABLED;
    ~گراف_راستہ_Checker() VL_MT_DISABLED;

    // METHODS
    bool راستہ_Exists_From(ثابت V3_گراف_راس* fromp, ثابت V3_گراف_راس* چوٹی) VL_MT_DISABLED;

    // If have edges A->B, B->C, and A->C then A->C is considered a
    // "transitive" edge (implied by A->B and B->C) and it could be safely
    // removed. Detect such an edge.
    bool is_متعدی_کنارہ(ثابت V3_گراف_کنارہ* edgep) VL_MT_DISABLED;

نجی:
    bool راستہ_Exists_اندرونی(ثابت V3_گراف_راس* ap, ثابت V3_گراف_راس* bp,
                            غیرعلامتی* costp = nullptr) VL_MT_DISABLED;
    سانچہ <گراف_Way::en N_Way>
    خالی initHalfCriticalPaths(bool checkOnly) VL_MT_DISABLED;
    خالی incGeneration() { ++m_generation; }

    VL_UNCOPYABLE(گراف_راستہ_Checker);
};

#اگر_ختم  // Guard
