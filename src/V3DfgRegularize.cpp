// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert DfgGraph to AstModule
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
//
// - Ensures intermediate values (other than simple memory references or
//   constants) with multiple uses are assigned to variables
//
//*************************************************************************

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    શૂન્ય uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        માટે (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            જો (vtx.is<DfgVertexSplice>()) ચાલુ;
            જો (vtx.is<DfgUnitArray>()) ચાલુ;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* અચલ varp = vtx.getResultVar();
            જો (!varp) ચાલુ;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    સ્થિર તાર્કિક isUnused(અચલ DfgVertex& vtx) {
        જો (vtx.hasSinks()) પાછા ખોટું;
        જો (અચલ DfgVertexVar* અચલ varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            જો (varp->hasModWrRefs()) પાછા ખોટું;
            જો (varp->hasExtRefs()) પાછા ખોટું;
            જો (varp->hasPrev()) પાછા ખોટું;
        }
        પાછા સાચું;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    તાર્કિક needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        જો (aVtx.is<DfgPrev>()) પાછા ખોટું;

        જો (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            જો (!aVtx.isCheaperThanLoad()) પાછા સાચું;

            // Not adding temporary
            પાછા ખોટું;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        જો (sink.is<DfgVertexVar>()) પાછા ખોટું;

        // CReset always needs to be driving a variable
        જો (aVtx.is<DfgCReset>()) પાછા સાચું;

        // Do not inline expressions into a loop body
        જો (અચલ DfgAstRd* અચલ astRdp = sink.cast<DfgAstRd>()) { પાછા astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        જો (aVtx.is<DfgConcat>()) {
            જો (sink.is<DfgConcat>()) પાછા ખોટું;  // Not root of tree
            પાછા VL_શબ્દો_I(static_cast<પૂર્ણાંક>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        પાછા ખોટું;
    }

    શૂન્ય eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            જો (vtx.is<DfgVertexAst>()) પાછા;
            જો (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.ધકેલો_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        અચલ auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.ધકેલો_front(src);
                પાછા ખોટું;
            });
            // Delete corresponsing Ast variable at the end
            જો (અચલ DfgVertexVar* અચલ varp = vtx.cast<DfgVertexVar>()) {
                જો (!varp->hasPrev()) m_ctx.m_deleteps.ધકેલો_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            જો (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                પાછા;
            }

            // Consider eliminating variables
            DfgVertexVar* અચલ varp = vtx.cast<DfgVertexVar>();
            જો (!varp) પાછા;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* અચલ srcp = varp->srcp();
            જો (!srcp) પાછા;

            // Can't eliminate if referenced external to the module - can't replace those refs
            જો (varp->hasExtRefs()) પાછા;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            જો (varp->hasModWrRefs()) પાછા;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            જો (varp->hasPrev()) પાછા;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            જો (varp->defaultp()) પાછા;
            જો (srcp->is<DfgVertexSplice>()) પાછા;
            જો (srcp->is<DfgUnitArray>()) પાછા;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            જો (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) પાછા;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            પાછા;
        });
    }

    શૂન્ય ઉમેરો_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        માટે (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            જો (vtx.is<DfgVertexSplice>()) ચાલુ;
            જો (vtx.is<DfgUnitArray>()) ચાલુ;
            // Add to map
            અચલ uint32_t fanout = vtx.fanout();
            જો (!fanout) ચાલુ;
            જો (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ધકેલો_back(&vtx);
        }
        જો (fanout2Vtxps.empty()) પાછા;

        // Ensure intermediate values used multiple times are written to variables
        માટે (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            માટે (DfgVertex* અચલ vtxp : fanout2Vtxps[fanout]) {
                જો (!needsTemporary(*vtxp, *vtxp)) ચાલુ;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_લીટી* અચલ flp = vtxp->fileline();
                AstScope* અચલ scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* અચલ newp
                    = m_dfg.makeNewVar(flp, "Regularize", vtxp->dtype(), scopep);
                // Replace vertex with the variable, make it drive the variable
                vtxp->replaceWith(newp);
                newp->srcp(vtxp);
            }
        }
    }

    // Insert intermediate variables for vertices with multiple sinks (or use an existing one)
    DfgRegularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        uninlineVariables();
        જો (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        જો (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        ઉમેરો_Temporaries();
        જો (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

જાહેર:
    સ્થિર શૂન્ય apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

શૂન્ય V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
