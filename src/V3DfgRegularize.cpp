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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    ଶୂନ୍ୟ uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        ପାଇଁ (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            ଯଦି (vtx.is<DfgVertexSplice>()) ଜାରି;
            ଯଦି (vtx.is<DfgUnitArray>()) ଜାରି;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* ଅଚଳ varp = vtx.getResultVar();
            ଯଦି (!varp) ଜାରି;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    ସ୍ଥିର ତାର୍କିକ isUnused(ଅଚଳ DfgVertex& vtx) {
        ଯଦି (vtx.hasSinks()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (ଅଚଳ DfgVertexVar* ଅଚଳ varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            ଯଦି (varp->hasModWrRefs()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (varp->hasExtRefs()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (varp->hasPrev()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    ତାର୍କିକ needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        ଯଦି (aVtx.is<DfgPrev>()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ଯଦି (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            ଯଦି (!aVtx.isCheaperThanLoad()) ଫେରନ୍ତୁ ସତ୍ୟ;

            // Not adding temporary
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        ଯଦି (sink.is<DfgVertexVar>()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // CReset always needs to be driving a variable
        ଯଦି (aVtx.is<DfgCReset>()) ଫେରନ୍ତୁ ସତ୍ୟ;

        // Do not inline expressions into a loop body
        ଯଦି (ଅଚଳ DfgAstRd* ଅଚଳ astRdp = sink.cast<DfgAstRd>()) { ଫେରନ୍ତୁ astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        ଯଦି (aVtx.is<DfgConcat>()) {
            ଯଦି (sink.is<DfgConcat>()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Not root of tree
            ଫେରନ୍ତୁ VL_ଶବ୍ଦଗୁଡ଼ିକ_I(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            ଯଦି (vtx.is<DfgVertexAst>()) ଫେରନ୍ତୁ;
            ଯଦି (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.ଠେଲ_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ଅଚଳ auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.ଠେଲ_front(src);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            // Delete corresponsing Ast variable at the end
            ଯଦି (ଅଚଳ DfgVertexVar* ଅଚଳ varp = vtx.cast<DfgVertexVar>()) {
                ଯଦି (!varp->hasPrev()) m_ctx.m_deleteps.ଠେଲ_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            ଯଦି (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                ଫେରନ୍ତୁ;
            }

            // Consider eliminating variables
            DfgVertexVar* ଅଚଳ varp = vtx.cast<DfgVertexVar>();
            ଯଦି (!varp) ଫେରନ୍ତୁ;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* ଅଚଳ srcp = varp->srcp();
            ଯଦି (!srcp) ଫେରନ୍ତୁ;

            // Can't eliminate if referenced external to the module - can't replace those refs
            ଯଦି (varp->hasExtRefs()) ଫେରନ୍ତୁ;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            ଯଦି (varp->hasModWrRefs()) ଫେରନ୍ତୁ;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            ଯଦି (varp->hasPrev()) ଫେରନ୍ତୁ;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            ଯଦି (varp->defaultp()) ଫେରନ୍ତୁ;
            ଯଦି (srcp->is<DfgVertexSplice>()) ଫେରନ୍ତୁ;
            ଯଦି (srcp->is<DfgUnitArray>()) ଫେରନ୍ତୁ;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            ଯଦି (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) ଫେରନ୍ତୁ;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            ଫେରନ୍ତୁ;
        });
    }

    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        ପାଇଁ (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            ଯଦି (vtx.is<DfgVertexSplice>()) ଜାରି;
            ଯଦି (vtx.is<DfgUnitArray>()) ଜାରି;
            // Add to map
            ଅଚଳ uint32_t fanout = vtx.fanout();
            ଯଦି (!fanout) ଜାରି;
            ଯଦି (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ଠେଲ_back(&vtx);
        }
        ଯଦି (fanout2Vtxps.empty()) ଫେରନ୍ତୁ;

        // Ensure intermediate values used multiple times are written to variables
        ପାଇଁ (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            ପାଇଁ (DfgVertex* ଅଚଳ vtxp : fanout2Vtxps[fanout]) {
                ଯଦି (!needsTemporary(*vtxp, *vtxp)) ଜାରି;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
                AstScope* ଅଚଳ scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* ଅଚଳ newp
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
        ଯଦି (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        ଯଦି (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        ଭର୍ତ୍ତି_Temporaries();
        ଯଦି (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

ଶୂନ୍ୟ V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
