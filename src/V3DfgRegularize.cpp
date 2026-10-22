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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    வெறுமை uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        க்கு (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            எனில் (vtx.is<DfgVertexSplice>()) தொடர்;
            எனில் (vtx.is<DfgUnitArray>()) தொடர்;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* மாறிலி varp = vtx.getResultVar();
            எனில் (!varp) தொடர்;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    நிலையான தருக்க isUnused(மாறிலி DfgVertex& vtx) {
        எனில் (vtx.hasSinks()) திரும்பு பொய்;
        எனில் (மாறிலி DfgVertexVar* மாறிலி varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            எனில் (varp->hasModWrRefs()) திரும்பு பொய்;
            எனில் (varp->hasExtRefs()) திரும்பு பொய்;
            எனில் (varp->hasPrev()) திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    தருக்க needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        எனில் (aVtx.is<DfgPrev>()) திரும்பு பொய்;

        எனில் (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            எனில் (!aVtx.isCheaperThanLoad()) திரும்பு உண்மை;

            // Not adding temporary
            திரும்பு பொய்;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        எனில் (sink.is<DfgVertexVar>()) திரும்பு பொய்;

        // CReset always needs to be driving a variable
        எனில் (aVtx.is<DfgCReset>()) திரும்பு உண்மை;

        // Do not inline expressions into a loop body
        எனில் (மாறிலி DfgAstRd* மாறிலி astRdp = sink.cast<DfgAstRd>()) { திரும்பு astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        எனில் (aVtx.is<DfgConcat>()) {
            எனில் (sink.is<DfgConcat>()) திரும்பு பொய்;  // Not root of tree
            திரும்பு VL_சொற்கள்_I(static_cast<முழு>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        திரும்பு பொய்;
    }

    வெறுமை eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            எனில் (vtx.is<DfgVertexAst>()) திரும்பு;
            எனில் (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.தள்ளு_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        மாறிலி auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.தள்ளு_front(src);
                திரும்பு பொய்;
            });
            // Delete corresponsing Ast variable at the end
            எனில் (மாறிலி DfgVertexVar* மாறிலி varp = vtx.cast<DfgVertexVar>()) {
                எனில் (!varp->hasPrev()) m_ctx.m_deleteps.தள்ளு_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            எனில் (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                திரும்பு;
            }

            // Consider eliminating variables
            DfgVertexVar* மாறிலி varp = vtx.cast<DfgVertexVar>();
            எனில் (!varp) திரும்பு;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* மாறிலி srcp = varp->srcp();
            எனில் (!srcp) திரும்பு;

            // Can't eliminate if referenced external to the module - can't replace those refs
            எனில் (varp->hasExtRefs()) திரும்பு;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            எனில் (varp->hasModWrRefs()) திரும்பு;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            எனில் (varp->hasPrev()) திரும்பு;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            எனில் (varp->defaultp()) திரும்பு;
            எனில் (srcp->is<DfgVertexSplice>()) திரும்பு;
            எனில் (srcp->is<DfgUnitArray>()) திரும்பு;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            எனில் (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) திரும்பு;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            திரும்பு;
        });
    }

    வெறுமை செருகு_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        க்கு (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            எனில் (vtx.is<DfgVertexSplice>()) தொடர்;
            எனில் (vtx.is<DfgUnitArray>()) தொடர்;
            // Add to map
            மாறிலி uint32_t fanout = vtx.fanout();
            எனில் (!fanout) தொடர்;
            எனில் (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].தள்ளு_back(&vtx);
        }
        எனில் (fanout2Vtxps.empty()) திரும்பு;

        // Ensure intermediate values used multiple times are written to variables
        க்கு (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            க்கு (DfgVertex* மாறிலி vtxp : fanout2Vtxps[fanout]) {
                எனில் (!needsTemporary(*vtxp, *vtxp)) தொடர்;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_வரி* மாறிலி flp = vtxp->fileline();
                AstScope* மாறிலி scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* மாறிலி newp
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
        எனில் (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        எனில் (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        செருகு_Temporaries();
        எனில் (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

பொது:
    நிலையான வெறுமை apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

வெறுமை V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
