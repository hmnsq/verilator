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

#اگر_تعریف_نشده VERILATOR_V3GRAPHPATHCHECKER_H_
#تعریف_کن VERILATOR_V3GRAPHPATHCHECKER_H_

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
class گراف_مسیر_Checker final : گراف_Alg<V3_گراف> {
    // Count "generations" which increases on operations that scan through
    // the graph. Each node is marked with the last generation that scanned
    // it, to enable asserting there are no cycles, and to avoid recursing
    // through the same node twice while searching for a path.
    uint64_t m_generation = 0;

عام:
    // CONSTRUCTORS
    explicit گراف_مسیر_Checker(V3_گراف* graphp, V3_یال_Func_P یال_Funcp
                                               = V3_گراف_یال::followAlwaysTrue) VL_MT_DISABLED;
    ~گراف_مسیر_Checker() VL_MT_DISABLED;

    // METHODS
    bool مسیر_Exists_From(ثابت V3_گراف_رأس* fromp, ثابت V3_گراف_رأس* بالاترین) VL_MT_DISABLED;

    // If have edges A->B, B->C, and A->C then A->C is considered a
    // "transitive" edge (implied by A->B and B->C) and it could be safely
    // removed. Detect such an edge.
    bool is_متعدی_یال(ثابت V3_گراف_یال* edgep) VL_MT_DISABLED;

خاص:
    bool مسیر_Exists_داخلی(ثابت V3_گراف_رأس* ap, ثابت V3_گراف_رأس* bp,
                            بیعلامت* costp = nullptr) VL_MT_DISABLED;
    قالب <گراف_Way::en N_Way>
    فراغ initHalfCriticalPaths(bool checkOnly) VL_MT_DISABLED;
    فراغ incGeneration() { ++m_generation; }

    VL_UNCOPYABLE(گراف_مسیر_Checker);
};

#پایان_اگر  // Guard
