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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3DFGPASSES_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3DFGPASSES_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgContext.h"

class AstModule;
class Dfg_ਗ੍ਰਾਫ;

namespace V3DfgPasses {
//===========================================================================
// Top level entry points
//===========================================================================

// Construct a DfGGraph representing the combinational logic in the netlist.
// The logic that is represented by the graph is removed from the netlist.
// Returns the constructed DfgGraph.
std::unique_ptr<Dfg_ਗ੍ਰਾਫ> ast_ਮੰਜ਼ਿਲ_Dfg(AstNetlist&, V3DfgContext&) VL_MT_DISABLED;
// Remove unobservable variabels and logic that drives only such variables
ਖਾਲੀ removeUnobservable(Dfg_ਗ੍ਰਾਫ&, V3DfgContext&) VL_MT_DISABLED;
// Synthesize DfgLogic vertices into primitive operations.
// Removes all DfgLogic (even those that were not synthesized).
ਖਾਲੀ synthesize(Dfg_ਗ੍ਰਾਫ&, V3DfgContext&) VL_MT_DISABLED;
// Remove redundant selects
ਖਾਲੀ removeSelects(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgRemoveSelectsContext& ctx) VL_MT_DISABLED;
// Make the given cyclic graph into an acyclic one, by tracing drivers, and
// if that is unsuccessful, by inserting Prev vertices. The given graph will
// always become acyclic after this pass.
ਖਾਲੀ break_ਚੱਕਰਾਂ(Dfg_ਗ੍ਰਾਫ&, V3DfgContext&) VL_MT_DISABLED;
// Construct binary to oneHot decoders
ਖਾਲੀ bin_ਮੰਜ਼ਿਲ_One_Hot(Dfg_ਗ੍ਰਾਫ&, V3_Dfg_Bin_ਮੰਜ਼ਿਲ_One_Hot_Context&) VL_MT_DISABLED;
// Common subexpression elimination
ਖਾਲੀ cse(Dfg_ਗ੍ਰਾਫ&, V3DfgCseContext&) VL_MT_DISABLED;
// Inline fully driven variables
ਖਾਲੀ inlineVars(Dfg_ਗ੍ਰਾਫ&) VL_MT_DISABLED;
// Push down selects through concatenations
ਖਾਲੀ ਧੱਕੋ_ਥੱਲੇ_Sels(Dfg_ਗ੍ਰਾਫ& dfg, V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels_Context& ctx) VL_MT_DISABLED;
// Peephole optimizations
ਖਾਲੀ peephole(Dfg_ਗ੍ਰਾਫ&, V3DfgPeepholeContext&) VL_MT_DISABLED;
// Regularize graph. This must be run before converting back to Ast.
ਖਾਲੀ regularize(Dfg_ਗ੍ਰਾਫ&, V3DfgRegularizeContext&) VL_MT_DISABLED;
// Convert DfgGraph back into Ast, and insert converted graph back into the Ast.
ਖਾਲੀ dfg_ਮੰਜ਼ਿਲ_Ast(Dfg_ਗ੍ਰਾਫ&, V3DfgContext&) VL_MT_DISABLED;
// Dump the patterns in the given graphs
ਖਾਲੀ dumpPatterns(ਅਚਲ std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>>&,
                  ਅਚਲ std::string& suffix = "") VL_MT_DISABLED;

//===========================================================================
// Intermediate/internal operations
//===========================================================================

// Add DfgVertexAst to the given DfgGraph for all references in the given AstNode.
// The function 'getVarVertex' is used to get the DfgVertexVar for an AstVarScope.
// If it returns nullptr, the reference will be ignored.
ਖਾਲੀ ਜੋੜੋ_Ast_Refs(Dfg_ਗ੍ਰਾਫ& dfg, Ast_ਗੰਢ* nodep,
                std::function<Dfg_ਸ਼ੀਰਸ਼_Var*(AstVarScope*)> get_Var_ਸ਼ੀਰਸ਼) VL_MT_DISABLED;
// Populates the given DfgUserMap for all vertext to:
// - 0, if the vertex is not part of a non-trivial strongly connected component
//   and is not part of a self-loop. That is: the Vertex is not part of any cycle.
// - N, if the vertex is part of a non-trivial strongly conneced component or self-loop N.
//   That is: each set of vertices that are reachable from each other will have the same
//   non-zero value assigned.
// Returns the number of non-trivial SCCs (distinct cycles)
uint32_t colorStronglyConnectedComponents(ਅਚਲ Dfg_ਗ੍ਰਾਫ&, DfgUserMap<uint64_t>&) VL_MT_DISABLED;
// Remove unused nodes
ਖਾਲੀ remove_ਅਣਵਰਤਿਆ(Dfg_ਗ੍ਰਾਫ&) VL_MT_DISABLED;
// Check all types are consistent. This will not return if there is a type error.
ਖਾਲੀ typeCheck(ਅਚਲ Dfg_ਗ੍ਰਾਫ&) VL_MT_DISABLED;

}  // namespace V3DfgPasses

#ਜੇ_ਅੰਤ
