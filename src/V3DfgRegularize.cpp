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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

صنف DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    فراغ uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        لكل (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            ܐܢ (vtx.is<DfgVertexSplice>()) استمر;
            ܐܢ (vtx.is<DfgUnitArray>()) استمر;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* ثابت varp = vtx.getResultVar();
            ܐܢ (!varp) استمر;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    ساكن منطقي isUnused(ثابت DfgVertex& vtx) {
        ܐܢ (vtx.hasSinks()) ܦܢܐ ܕܓܠܐ;
        ܐܢ (ثابت DfgVertexVar* ثابت varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            ܐܢ (varp->hasModWrRefs()) ܦܢܐ ܕܓܠܐ;
            ܐܢ (varp->hasExtRefs()) ܦܢܐ ܕܓܠܐ;
            ܐܢ (varp->hasPrev()) ܦܢܐ ܕܓܠܐ;
        }
        ܦܢܐ ܫܪܪܐ;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    منطقي needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        ܐܢ (aVtx.is<DfgPrev>()) ܦܢܐ ܕܓܠܐ;

        ܐܢ (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            ܐܢ (!aVtx.isCheaperThanLoad()) ܦܢܐ ܫܪܪܐ;

            // Not adding temporary
            ܦܢܐ ܕܓܠܐ;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        ܐܢ (sink.is<DfgVertexVar>()) ܦܢܐ ܕܓܠܐ;

        // CReset always needs to be driving a variable
        ܐܢ (aVtx.is<DfgCReset>()) ܦܢܐ ܫܪܪܐ;

        // Do not inline expressions into a loop body
        ܐܢ (ثابت DfgAstRd* ثابت astRdp = sink.cast<DfgAstRd>()) { ܦܢܐ astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        ܐܢ (aVtx.is<DfgConcat>()) {
            ܐܢ (sink.is<DfgConcat>()) ܦܢܐ ܕܓܠܐ;  // Not root of tree
            ܦܢܐ VL_WORDS_I(static_cast<صحيح>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        ܦܢܐ ܕܓܠܐ;
    }

    فراغ eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            ܐܢ (vtx.is<DfgVertexAst>()) ܦܢܐ;
            ܐܢ (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.push_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ثابت auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.push_front(src);
                ܦܢܐ ܕܓܠܐ;
            });
            // Delete corresponsing Ast variable at the end
            ܐܢ (ثابت DfgVertexVar* ثابت varp = vtx.cast<DfgVertexVar>()) {
                ܐܢ (!varp->hasPrev()) m_ctx.m_deleteps.push_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            ܐܢ (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                ܦܢܐ;
            }

            // Consider eliminating variables
            DfgVertexVar* ثابت varp = vtx.cast<DfgVertexVar>();
            ܐܢ (!varp) ܦܢܐ;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* ثابت srcp = varp->srcp();
            ܐܢ (!srcp) ܦܢܐ;

            // Can't eliminate if referenced external to the module - can't replace those refs
            ܐܢ (varp->hasExtRefs()) ܦܢܐ;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            ܐܢ (varp->hasModWrRefs()) ܦܢܐ;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            ܐܢ (varp->hasPrev()) ܦܢܐ;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            ܐܢ (varp->defaultp()) ܦܢܐ;
            ܐܢ (srcp->is<DfgVertexSplice>()) ܦܢܐ;
            ܐܢ (srcp->is<DfgUnitArray>()) ܦܢܐ;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            ܐܢ (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) ܦܢܐ;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            ܦܢܐ;
        });
    }

    فراغ insertTemporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        لكل (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            ܐܢ (vtx.is<DfgVertexSplice>()) استمر;
            ܐܢ (vtx.is<DfgUnitArray>()) استمر;
            // Add to map
            ثابت uint32_t fanout = vtx.fanout();
            ܐܢ (!fanout) استمر;
            ܐܢ (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].push_back(&vtx);
        }
        ܐܢ (fanout2Vtxps.empty()) ܦܢܐ;

        // Ensure intermediate values used multiple times are written to variables
        لكل (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            لكل (DfgVertex* ثابت vtxp : fanout2Vtxps[fanout]) {
                ܐܢ (!needsTemporary(*vtxp, *vtxp)) استمر;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                FileLine* ثابت flp = vtxp->fileline();
                AstScope* ثابت scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* ثابت newp
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
        ܐܢ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        ܐܢ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        insertTemporaries();
        ܐܢ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

عام:
    ساكن فراغ apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

فراغ V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
