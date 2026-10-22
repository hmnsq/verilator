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

#यदि_अपरिभाषितम् VERILATOR_V3DFGPASSES_H_
#परिभाषय VERILATOR_V3DFGPASSES_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgContext.h"

class AstModule;
class DfgGraph;

namespace V3DfgPasses {
//===========================================================================
// Top level entry points
//===========================================================================

// Construct a DfGGraph representing the combinational logic in the netlist.
// The logic that is represented by the graph is removed from the netlist.
// Returns the constructed DfgGraph.
std::unique_ptr<DfgGraph> ast_गन्तव्यम्_Dfg(AstNetlist&, V3DfgContext&) VL_MT_DISABLED;
// Remove unobservable variabels and logic that drives only such variables
शून्य removeUnobservable(DfgGraph&, V3DfgContext&) VL_MT_DISABLED;
// Synthesize DfgLogic vertices into primitive operations.
// Removes all DfgLogic (even those that were not synthesized).
शून्य synthesize(DfgGraph&, V3DfgContext&) VL_MT_DISABLED;
// Remove redundant selects
शून्य removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) VL_MT_DISABLED;
// Make the given cyclic graph into an acyclic one, by tracing drivers, and
// if that is unsuccessful, by inserting Prev vertices. The given graph will
// always become acyclic after this pass.
शून्य break_आवृत्तयः(DfgGraph&, V3DfgContext&) VL_MT_DISABLED;
// Construct binary to oneHot decoders
शून्य bin_गन्तव्यम्_One_Hot(DfgGraph&, V3_Dfg_Bin_गन्तव्यम्_One_Hot_Context&) VL_MT_DISABLED;
// Common subexpression elimination
शून्य cse(DfgGraph&, V3DfgCseContext&) VL_MT_DISABLED;
// Inline fully driven variables
शून्य inlineVars(DfgGraph&) VL_MT_DISABLED;
// Push down selects through concatenations
शून्य प्रक्षिप_अधस्तात्_Sels(DfgGraph& dfg, V3_Dfg_प्रक्षिप_अधस्तात्_Sels_Context& ctx) VL_MT_DISABLED;
// Peephole optimizations
शून्य peephole(DfgGraph&, V3DfgPeepholeContext&) VL_MT_DISABLED;
// Regularize graph. This must be run before converting back to Ast.
शून्य regularize(DfgGraph&, V3DfgRegularizeContext&) VL_MT_DISABLED;
// Convert DfgGraph back into Ast, and insert converted graph back into the Ast.
शून्य dfg_गन्तव्यम्_Ast(DfgGraph&, V3DfgContext&) VL_MT_DISABLED;
// Dump the patterns in the given graphs
शून्य dumpPatterns(ध्रुव std::सदिशम्<std::unique_ptr<DfgGraph>>&,
                  ध्रुव std::string& suffix = "") VL_MT_DISABLED;

//===========================================================================
// Intermediate/internal operations
//===========================================================================

// Add DfgVertexAst to the given DfgGraph for all references in the given AstNode.
// The function 'getVarVertex' is used to get the DfgVertexVar for an AstVarScope.
// If it returns nullptr, the reference will be ignored.
शून्य सङ्कलय_Ast_Refs(DfgGraph& dfg, Ast_ग्रन्थिः* nodep,
                std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) VL_MT_DISABLED;
// Populates the given DfgUserMap for all vertext to:
// - 0, if the vertex is not part of a non-trivial strongly connected component
//   and is not part of a self-loop. That is: the Vertex is not part of any cycle.
// - N, if the vertex is part of a non-trivial strongly conneced component or self-loop N.
//   That is: each set of vertices that are reachable from each other will have the same
//   non-zero value assigned.
// Returns the number of non-trivial SCCs (distinct cycles)
uint32_t colorStronglyConnectedComponents(ध्रुव DfgGraph&, DfgUserMap<uint64_t>&) VL_MT_DISABLED;
// Remove unused nodes
शून्य remove_अप्रयुक्तम्(DfgGraph&) VL_MT_DISABLED;
// Check all types are consistent. This will not return if there is a type error.
शून्य typeCheck(ध्रुव DfgGraph&) VL_MT_DISABLED;

}  // namespace V3DfgPasses

#यदि_अन्तः
