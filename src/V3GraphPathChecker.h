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

#که_نه_تعریف_شوی VERILATOR_V3GRAPHPATHCHECKER_H_
#تعریف_کړه VERILATOR_V3GRAPHPATHCHECKER_H_

#ضمن "V3Error.h"
#ضمن "V3Graph.h"
#ضمن "V3GraphAlg.h"

//######################################################################

/// Implement pathExistsFrom() with some caching to prune the search.
/// Far more aggressive caching/pruning is possible; for now the use cases
/// don't rely so heavily on this class that it's necessary.
///
/// The graph (or at least, the subset the algorithm sees through
/// edgeFuncp) must not change during the lifetime of the checker.
class ګراف_لار_Checker final : ګراف_Alg<V3_ګراف> {
    // Count "generations" which increases on operations that scan through
    // the graph. Each node is marked with the last generation that scanned
    // it, to enable asserting there are no cycles, and to avoid recursing
    // through the same node twice while searching for a path.
    uint64_t m_generation = 0;

عام:
    // CONSTRUCTORS
    explicit ګراف_لار_Checker(V3_ګراف* graphp, V3_څنډه_Func_P څنډه_Funcp
                                               = V3_ګراف_څنډه::followAlwaysTrue) VL_MT_DISABLED;
    ~ګراف_لار_Checker() VL_MT_DISABLED;

    // METHODS
    bool لار_Exists_From(ثابت V3_ګراف_راس* fromp, ثابت V3_ګراف_راس* سر) VL_MT_DISABLED;

    // If have edges A->B, B->C, and A->C then A->C is considered a
    // "transitive" edge (implied by A->B and B->C) and it could be safely
    // removed. Detect such an edge.
    bool is_متعدي_څنډه(ثابت V3_ګراف_څنډه* edgep) VL_MT_DISABLED;

خاص:
    bool لار_Exists_دننی(ثابت V3_ګراف_راس* ap, ثابت V3_ګراف_راس* bp,
                            بینښه* costp = nullptr) VL_MT_DISABLED;
    کينډۍ <ګراف_Way::en N_Way>
    فراغ initHalfCriticalPaths(bool checkOnly) VL_MT_DISABLED;
    فراغ incGeneration() { ++m_generation; }

    VL_UNCOPYABLE(ګراف_لار_Checker);
};

#د_که_پای  // Guard
