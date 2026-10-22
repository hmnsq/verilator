// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Passes over DfgGraph
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

#ifndef VERILATOR_V3DFGPASSES_H_
#define VERILATOR_V3DFGPASSES_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgContext.h"

class AstModule;
class DfgGraph;

namespace V3DfgPasses {
//===========================================================================
// Top level entry points
//===========================================================================

// Construct a DfGGraph representing the combinational logic in the netlist.
// The logic that is represented by the graph is removed from the netlist.
// Returns the constructed DfgGraph.
std::unique_ptr<DfgGraph> astToDfg(AstNetlist&, V3DfgContext&) VL_MT_DISABLED;
// Remove unobservable variabels and logic that drives only such variables
வெறுமை removeUnobservable(DfgGraph&, V3DfgContext&) VL_MT_DISABLED;
// Synthesize DfgLogic vertices into primitive operations.
// Removes all DfgLogic (even those that were not synthesized).
வெறுமை synthesize(DfgGraph&, V3DfgContext&) VL_MT_DISABLED;
// Remove redundant selects
வெறுமை removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) VL_MT_DISABLED;
// Make the given cyclic graph into an acyclic one, by tracing drivers, and
// if that is unsuccessful, by inserting Prev vertices. The given graph will
// always become acyclic after this pass.
வெறுமை breakCycles(DfgGraph&, V3DfgContext&) VL_MT_DISABLED;
// Construct binary to oneHot decoders
வெறுமை binToOneHot(DfgGraph&, V3DfgBinToOneHotContext&) VL_MT_DISABLED;
// Common subexpression elimination
வெறுமை cse(DfgGraph&, V3DfgCseContext&) VL_MT_DISABLED;
// Inline fully driven variables
வெறுமை inlineVars(DfgGraph&) VL_MT_DISABLED;
// Push down selects through concatenations
வெறுமை தள்ளு_Down_Sels(DfgGraph& dfg, V3_Dfg_தள்ளு_Down_Sels_Context& ctx) VL_MT_DISABLED;
// Peephole optimizations
வெறுமை peephole(DfgGraph&, V3DfgPeepholeContext&) VL_MT_DISABLED;
// Regularize graph. This must be run before converting back to Ast.
வெறுமை regularize(DfgGraph&, V3DfgRegularizeContext&) VL_MT_DISABLED;
// Convert DfgGraph back into Ast, and insert converted graph back into the Ast.
வெறுமை dfgToAst(DfgGraph&, V3DfgContext&) VL_MT_DISABLED;
// Dump the patterns in the given graphs
வெறுமை dumpPatterns(மாறிலி std::vector<std::unique_ptr<DfgGraph>>&,
                  மாறிலி std::string& suffix = "") VL_MT_DISABLED;

//===========================================================================
// Intermediate/internal operations
//===========================================================================

// Add DfgVertexAst to the given DfgGraph for all references in the given AstNode.
// The function 'getVarVertex' is used to get the DfgVertexVar for an AstVarScope.
// If it returns nullptr, the reference will be ignored.
வெறுமை addAstRefs(DfgGraph& dfg, Ast_முனை* nodep,
                std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) VL_MT_DISABLED;
// Populates the given DfgUserMap for all vertext to:
// - 0, if the vertex is not part of a non-trivial strongly connected component
//   and is not part of a self-loop. That is: the Vertex is not part of any cycle.
// - N, if the vertex is part of a non-trivial strongly conneced component or self-loop N.
//   That is: each set of vertices that are reachable from each other will have the same
//   non-zero value assigned.
// Returns the number of non-trivial SCCs (distinct cycles)
uint32_t colorStronglyConnectedComponents(மாறிலி DfgGraph&, DfgUserMap<uint64_t>&) VL_MT_DISABLED;
// Remove unused nodes
வெறுமை removeUnused(DfgGraph&) VL_MT_DISABLED;
// Check all types are consistent. This will not return if there is a type error.
வெறுமை typeCheck(மாறிலி DfgGraph&) VL_MT_DISABLED;

}  // namespace V3DfgPasses

#endif
