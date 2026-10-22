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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3DFGPASSES_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3DFGPASSES_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgContext.h"

class AstModule;
class Dfg_ನಕ್ಷೆ;

namespace V3DfgPasses {
//===========================================================================
// Top level entry points
//===========================================================================

// Construct a DfGGraph representing the combinational logic in the netlist.
// The logic that is represented by the graph is removed from the netlist.
// Returns the constructed DfgGraph.
std::unique_ptr<Dfg_ನಕ್ಷೆ> ast_ಗಮ್ಯ_Dfg(AstNetlist&, V3DfgContext&) VL_MT_DISABLED;
// Remove unobservable variabels and logic that drives only such variables
ಶೂನ್ಯ removeUnobservable(Dfg_ನಕ್ಷೆ&, V3DfgContext&) VL_MT_DISABLED;
// Synthesize DfgLogic vertices into primitive operations.
// Removes all DfgLogic (even those that were not synthesized).
ಶೂನ್ಯ synthesize(Dfg_ನಕ್ಷೆ&, V3DfgContext&) VL_MT_DISABLED;
// Remove redundant selects
ಶೂನ್ಯ removeSelects(Dfg_ನಕ್ಷೆ& dfg, V3DfgRemoveSelectsContext& ctx) VL_MT_DISABLED;
// Make the given cyclic graph into an acyclic one, by tracing drivers, and
// if that is unsuccessful, by inserting Prev vertices. The given graph will
// always become acyclic after this pass.
ಶೂನ್ಯ break_ಚಕ್ರಗಳು(Dfg_ನಕ್ಷೆ&, V3DfgContext&) VL_MT_DISABLED;
// Construct binary to oneHot decoders
ಶೂನ್ಯ bin_ಗಮ್ಯ_One_Hot(Dfg_ನಕ್ಷೆ&, V3_Dfg_Bin_ಗಮ್ಯ_One_Hot_Context&) VL_MT_DISABLED;
// Common subexpression elimination
ಶೂನ್ಯ cse(Dfg_ನಕ್ಷೆ&, V3DfgCseContext&) VL_MT_DISABLED;
// Inline fully driven variables
ಶೂನ್ಯ inlineVars(Dfg_ನಕ್ಷೆ&) VL_MT_DISABLED;
// Push down selects through concatenations
ಶೂನ್ಯ ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels(Dfg_ನಕ್ಷೆ& dfg, V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context& ctx) VL_MT_DISABLED;
// Peephole optimizations
ಶೂನ್ಯ peephole(Dfg_ನಕ್ಷೆ&, V3DfgPeepholeContext&) VL_MT_DISABLED;
// Regularize graph. This must be run before converting back to Ast.
ಶೂನ್ಯ regularize(Dfg_ನಕ್ಷೆ&, V3DfgRegularizeContext&) VL_MT_DISABLED;
// Convert DfgGraph back into Ast, and insert converted graph back into the Ast.
ಶೂನ್ಯ dfg_ಗಮ್ಯ_Ast(Dfg_ನಕ್ಷೆ&, V3DfgContext&) VL_MT_DISABLED;
// Dump the patterns in the given graphs
ಶೂನ್ಯ dumpPatterns(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>>&,
                  ಸ್ಥಿರಾಂಕ std::string& suffix = "") VL_MT_DISABLED;

//===========================================================================
// Intermediate/internal operations
//===========================================================================

// Add DfgVertexAst to the given DfgGraph for all references in the given AstNode.
// The function 'getVarVertex' is used to get the DfgVertexVar for an AstVarScope.
// If it returns nullptr, the reference will be ignored.
ಶೂನ್ಯ ಕೂಡಿಸು_Ast_Refs(Dfg_ನಕ್ಷೆ& dfg, Ast_ಗಂಟು* nodep,
                std::function<Dfg_ಶೃಂಗ_Var*(AstVarScope*)> get_Var_ಶೃಂಗ) VL_MT_DISABLED;
// Populates the given DfgUserMap for all vertext to:
// - 0, if the vertex is not part of a non-trivial strongly connected component
//   and is not part of a self-loop. That is: the Vertex is not part of any cycle.
// - N, if the vertex is part of a non-trivial strongly conneced component or self-loop N.
//   That is: each set of vertices that are reachable from each other will have the same
//   non-zero value assigned.
// Returns the number of non-trivial SCCs (distinct cycles)
uint32_t colorStronglyConnectedComponents(ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ&, DfgUserMap<uint64_t>&) VL_MT_DISABLED;
// Remove unused nodes
ಶೂನ್ಯ remove_ಬಳಸದ(Dfg_ನಕ್ಷೆ&) VL_MT_DISABLED;
// Check all types are consistent. This will not return if there is a type error.
ಶೂನ್ಯ typeCheck(ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ&) VL_MT_DISABLED;

}  // namespace V3DfgPasses

#ಆದರೆ_ಅಂತ್ಯ
