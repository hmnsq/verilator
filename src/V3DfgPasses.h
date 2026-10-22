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

#నిర్వచితం_కాకపోతే VERILATOR_V3DFGPASSES_H_
#నిర్వచించు VERILATOR_V3DFGPASSES_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgContext.h"

class AstModule;
class Dfg_లేఖాచిత్రం;

namespace V3DfgPasses {
//===========================================================================
// Top level entry points
//===========================================================================

// Construct a DfGGraph representing the combinational logic in the netlist.
// The logic that is represented by the graph is removed from the netlist.
// Returns the constructed DfgGraph.
std::unique_ptr<Dfg_లేఖాచిత్రం> ast_గమ్యం_Dfg(AstNetlist&, V3DfgContext&) VL_MT_DISABLED;
// Remove unobservable variabels and logic that drives only such variables
శూన్య removeUnobservable(Dfg_లేఖాచిత్రం&, V3DfgContext&) VL_MT_DISABLED;
// Synthesize DfgLogic vertices into primitive operations.
// Removes all DfgLogic (even those that were not synthesized).
శూన్య synthesize(Dfg_లేఖాచిత్రం&, V3DfgContext&) VL_MT_DISABLED;
// Remove redundant selects
శూన్య removeSelects(Dfg_లేఖాచిత్రం& dfg, V3DfgRemoveSelectsContext& ctx) VL_MT_DISABLED;
// Make the given cyclic graph into an acyclic one, by tracing drivers, and
// if that is unsuccessful, by inserting Prev vertices. The given graph will
// always become acyclic after this pass.
శూన్య break_చక్రాలు(Dfg_లేఖాచిత్రం&, V3DfgContext&) VL_MT_DISABLED;
// Construct binary to oneHot decoders
శూన్య bin_గమ్యం_One_Hot(Dfg_లేఖాచిత్రం&, V3_Dfg_Bin_గమ్యం_One_Hot_Context&) VL_MT_DISABLED;
// Common subexpression elimination
శూన్య cse(Dfg_లేఖాచిత్రం&, V3DfgCseContext&) VL_MT_DISABLED;
// Inline fully driven variables
శూన్య inlineVars(Dfg_లేఖాచిత్రం&) VL_MT_DISABLED;
// Push down selects through concatenations
శూన్య నెట్టు_కిందికి_Sels(Dfg_లేఖాచిత్రం& dfg, V3_Dfg_నెట్టు_కిందికి_Sels_Context& ctx) VL_MT_DISABLED;
// Peephole optimizations
శూన్య peephole(Dfg_లేఖాచిత్రం&, V3DfgPeepholeContext&) VL_MT_DISABLED;
// Regularize graph. This must be run before converting back to Ast.
శూన్య regularize(Dfg_లేఖాచిత్రం&, V3DfgRegularizeContext&) VL_MT_DISABLED;
// Convert DfgGraph back into Ast, and insert converted graph back into the Ast.
శూన్య dfg_గమ్యం_Ast(Dfg_లేఖాచిత్రం&, V3DfgContext&) VL_MT_DISABLED;
// Dump the patterns in the given graphs
శూన్య dumpPatterns(స్థిరరాశి std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>>&,
                  స్థిరరాశి std::string& suffix = "") VL_MT_DISABLED;

//===========================================================================
// Intermediate/internal operations
//===========================================================================

// Add DfgVertexAst to the given DfgGraph for all references in the given AstNode.
// The function 'getVarVertex' is used to get the DfgVertexVar for an AstVarScope.
// If it returns nullptr, the reference will be ignored.
శూన్య కూడు_Ast_Refs(Dfg_లేఖాచిత్రం& dfg, Ast_కణుపు* nodep,
                std::function<Dfg_శీర్షం_Var*(AstVarScope*)> get_Var_శీర్షం) VL_MT_DISABLED;
// Populates the given DfgUserMap for all vertext to:
// - 0, if the vertex is not part of a non-trivial strongly connected component
//   and is not part of a self-loop. That is: the Vertex is not part of any cycle.
// - N, if the vertex is part of a non-trivial strongly conneced component or self-loop N.
//   That is: each set of vertices that are reachable from each other will have the same
//   non-zero value assigned.
// Returns the number of non-trivial SCCs (distinct cycles)
uint32_t colorStronglyConnectedComponents(స్థిరరాశి Dfg_లేఖాచిత్రం&, DfgUserMap<uint64_t>&) VL_MT_DISABLED;
// Remove unused nodes
శూన్య remove_వాడని(Dfg_లేఖాచిత్రం&) VL_MT_DISABLED;
// Check all types are consistent. This will not return if there is a type error.
శూన్య typeCheck(స్థిరరాశి Dfg_లేఖాచిత్రం&) VL_MT_DISABLED;

}  // namespace V3DfgPasses

#అయితే_ముగింపు
