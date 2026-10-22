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

#جے_بنا_تعریف VERILATOR_V3DFGPASSES_H_
#تعریف_کرو VERILATOR_V3DFGPASSES_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Dfg.h"
#ضمن "V3DfgContext.h"

class AstModule;
class Dfg_گراف;

namespace V3DfgPasses {
//===========================================================================
// Top level entry points
//===========================================================================

// Construct a DfGGraph representing the combinational logic in the netlist.
// The logic that is represented by the graph is removed from the netlist.
// Returns the constructed DfgGraph.
std::unique_ptr<Dfg_گراف> ast_منزل_Dfg(AstNetlist&, V3DfgContext&) VL_MT_DISABLED;
// Remove unobservable variabels and logic that drives only such variables
فراغ removeUnobservable(Dfg_گراف&, V3DfgContext&) VL_MT_DISABLED;
// Synthesize DfgLogic vertices into primitive operations.
// Removes all DfgLogic (even those that were not synthesized).
فراغ synthesize(Dfg_گراف&, V3DfgContext&) VL_MT_DISABLED;
// Remove redundant selects
فراغ removeSelects(Dfg_گراف& dfg, V3DfgRemoveSelectsContext& ctx) VL_MT_DISABLED;
// Make the given cyclic graph into an acyclic one, by tracing drivers, and
// if that is unsuccessful, by inserting Prev vertices. The given graph will
// always become acyclic after this pass.
فراغ break_چکراں(Dfg_گراف&, V3DfgContext&) VL_MT_DISABLED;
// Construct binary to oneHot decoders
فراغ bin_منزل_One_Hot(Dfg_گراف&, V3_Dfg_Bin_منزل_One_Hot_Context&) VL_MT_DISABLED;
// Common subexpression elimination
فراغ cse(Dfg_گراف&, V3DfgCseContext&) VL_MT_DISABLED;
// Inline fully driven variables
فراغ inlineVars(Dfg_گراف&) VL_MT_DISABLED;
// Push down selects through concatenations
فراغ ڈالو_تھلے_Sels(Dfg_گراف& dfg, V3_Dfg_ڈالو_تھلے_Sels_Context& ctx) VL_MT_DISABLED;
// Peephole optimizations
فراغ peephole(Dfg_گراف&, V3DfgPeepholeContext&) VL_MT_DISABLED;
// Regularize graph. This must be run before converting back to Ast.
فراغ regularize(Dfg_گراف&, V3DfgRegularizeContext&) VL_MT_DISABLED;
// Convert DfgGraph back into Ast, and insert converted graph back into the Ast.
فراغ dfg_منزل_Ast(Dfg_گراف&, V3DfgContext&) VL_MT_DISABLED;
// Dump the patterns in the given graphs
فراغ dumpPatterns(ثابت std::سمتیہ<std::unique_ptr<Dfg_گراف>>&,
                  ثابت std::string& suffix = "") VL_MT_DISABLED;

//===========================================================================
// Intermediate/internal operations
//===========================================================================

// Add DfgVertexAst to the given DfgGraph for all references in the given AstNode.
// The function 'getVarVertex' is used to get the DfgVertexVar for an AstVarScope.
// If it returns nullptr, the reference will be ignored.
فراغ جوڑو_Ast_Refs(Dfg_گراف& dfg, Ast_گرہ* nodep,
                std::function<Dfg_راس_Var*(AstVarScope*)> get_Var_راس) VL_MT_DISABLED;
// Populates the given DfgUserMap for all vertext to:
// - 0, if the vertex is not part of a non-trivial strongly connected component
//   and is not part of a self-loop. That is: the Vertex is not part of any cycle.
// - N, if the vertex is part of a non-trivial strongly conneced component or self-loop N.
//   That is: each set of vertices that are reachable from each other will have the same
//   non-zero value assigned.
// Returns the number of non-trivial SCCs (distinct cycles)
uint32_t colorStronglyConnectedComponents(ثابت Dfg_گراف&, DfgUserMap<uint64_t>&) VL_MT_DISABLED;
// Remove unused nodes
فراغ remove_بن_ورتیا(Dfg_گراف&) VL_MT_DISABLED;
// Check all types are consistent. This will not return if there is a type error.
فراغ typeCheck(ثابت Dfg_گراف&) VL_MT_DISABLED;

}  // namespace V3DfgPasses

#جے_انت
