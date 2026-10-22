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

#నిర్వచితం_కాకపోతే VERILATOR_V3GRAPHPATHCHECKER_H_
#నిర్వచించు VERILATOR_V3GRAPHPATHCHECKER_H_

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3GraphAlg.h"

//######################################################################

/// Implement pathExistsFrom() with some caching to prune the search.
/// Far more aggressive caching/pruning is possible; for now the use cases
/// don't rely so heavily on this class that it's necessary.
///
/// The graph (or at least, the subset the algorithm sees through
/// edgeFuncp) must not change during the lifetime of the checker.
class లేఖాచిత్రం_మార్గం_Checker final : లేఖాచిత్రం_Alg<V3_లేఖాచిత్రం> {
    // Count "generations" which increases on operations that scan through
    // the graph. Each node is marked with the last generation that scanned
    // it, to enable asserting there are no cycles, and to avoid recursing
    // through the same node twice while searching for a path.
    uint64_t m_generation = 0;

బహిరంగ:
    // CONSTRUCTORS
    explicit లేఖాచిత్రం_మార్గం_Checker(V3_లేఖాచిత్రం* graphp, V3_అంచు_Func_P అంచు_Funcp
                                               = V3_లేఖాచిత్రం_అంచు::followAlwaysTrue) VL_MT_DISABLED;
    ~లేఖాచిత్రం_మార్గం_Checker() VL_MT_DISABLED;

    // METHODS
    bool మార్గం_Exists_From(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* fromp, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* శిఖరం) VL_MT_DISABLED;

    // If have edges A->B, B->C, and A->C then A->C is considered a
    // "transitive" edge (implied by A->B and B->C) and it could be safely
    // removed. Detect such an edge.
    bool is_సంక్రమణ_అంచు(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* edgep) VL_MT_DISABLED;

అంతరంగ:
    bool మార్గం_Exists_అంతర్గత(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* ap, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* bp,
                            గుర్తులేని* costp = nullptr) VL_MT_DISABLED;
    మూస <లేఖాచిత్రం_Way::en N_Way>
    శూన్య initHalfCriticalPaths(bool checkOnly) VL_MT_DISABLED;
    శూన్య incGeneration() { ++m_generation; }

    VL_UNCOPYABLE(లేఖాచిత్రం_మార్గం_Checker);
};

#అయితే_ముగింపు  // Guard
