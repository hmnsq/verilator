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

#यदि_अपरिभाषितम् VERILATOR_V3GRAPHPATHCHECKER_H_
#परिभाषय VERILATOR_V3GRAPHPATHCHECKER_H_

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3GraphAlg.h"

//######################################################################

/// Implement pathExistsFrom() with some caching to prune the search.
/// Far more aggressive caching/pruning is possible; for now the use cases
/// don't rely so heavily on this class that it's necessary.
///
/// The graph (or at least, the subset the algorithm sees through
/// edgeFuncp) must not change during the lifetime of the checker.
class Graph_मार्गः_Checker final : GraphAlg<V3Graph> {
    // Count "generations" which increases on operations that scan through
    // the graph. Each node is marked with the last generation that scanned
    // it, to enable asserting there are no cycles, and to avoid recursing
    // through the same node twice while searching for a path.
    uint64_t m_generation = 0;

सार्वजनिक:
    // CONSTRUCTORS
    explicit Graph_मार्गः_Checker(V3Graph* graphp, V3EdgeFuncP edgeFuncp
                                               = V3GraphEdge::followAlwaysTrue) VL_MT_DISABLED;
    ~Graph_मार्गः_Checker() VL_MT_DISABLED;

    // METHODS
    bool मार्गः_Exists_From(ध्रुव V3GraphVertex* fromp, ध्रुव V3GraphVertex* शिखरम्) VL_MT_DISABLED;

    // If have edges A->B, B->C, and A->C then A->C is considered a
    // "transitive" edge (implied by A->B and B->C) and it could be safely
    // removed. Detect such an edge.
    bool is_संक्रामकम्_Edge(ध्रुव V3GraphEdge* edgep) VL_MT_DISABLED;

गुप्त:
    bool मार्गः_Exists_आन्तरिकम्(ध्रुव V3GraphVertex* ap, ध्रुव V3GraphVertex* bp,
                            अचिह्नित* costp = nullptr) VL_MT_DISABLED;
    प्रतिरूपम् <GraphWay::en N_Way>
    शून्य initHalfCriticalPaths(bool checkOnly) VL_MT_DISABLED;
    शून्य incGeneration() { ++m_generation; }

    VL_UNCOPYABLE(Graph_मार्गः_Checker);
};

#यदि_अन्तः  // Guard
