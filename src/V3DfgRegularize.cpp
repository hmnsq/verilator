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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

वग्ग DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    शून्य uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        अत्थाय (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            सचे (vtx.is<DfgVertexSplice>()) अनुवर्त;
            सचे (vtx.is<DfgUnitArray>()) अनुवर्त;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* ध्रुव varp = vtx.getResultVar();
            सचे (!varp) अनुवर्त;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    स्थिर तार्किक isUnused(ध्रुव DfgVertex& vtx) {
        सचे (vtx.hasSinks()) पटिनिवत्त मिच्छा;
        सचे (ध्रुव DfgVertexVar* ध्रुव varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            सचे (varp->hasModWrRefs()) पटिनिवत्त मिच्छा;
            सचे (varp->hasExtRefs()) पटिनिवत्त मिच्छा;
            सचे (varp->hasPrev()) पटिनिवत्त मिच्छा;
        }
        पटिनिवत्त सच्च;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    तार्किक needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        सचे (aVtx.is<DfgPrev>()) पटिनिवत्त मिच्छा;

        सचे (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            सचे (!aVtx.isCheaperThanLoad()) पटिनिवत्त सच्च;

            // Not adding temporary
            पटिनिवत्त मिच्छा;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        सचे (sink.is<DfgVertexVar>()) पटिनिवत्त मिच्छा;

        // CReset always needs to be driving a variable
        सचे (aVtx.is<DfgCReset>()) पटिनिवत्त सच्च;

        // Do not inline expressions into a loop body
        सचे (ध्रुव DfgAstRd* ध्रुव astRdp = sink.cast<DfgAstRd>()) { पटिनिवत्त astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        सचे (aVtx.is<DfgConcat>()) {
            सचे (sink.is<DfgConcat>()) पटिनिवत्त मिच्छा;  // Not root of tree
            पटिनिवत्त VL_शब्दाः_I(static_cast<पूर्णाङ्क>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        पटिनिवत्त मिच्छा;
    }

    शून्य eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            सचे (vtx.is<DfgVertexAst>()) पटिनिवत्त;
            सचे (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.प्रक्षिप_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ध्रुव auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.प्रक्षिप_front(src);
                पटिनिवत्त मिच्छा;
            });
            // Delete corresponsing Ast variable at the end
            सचे (ध्रुव DfgVertexVar* ध्रुव varp = vtx.cast<DfgVertexVar>()) {
                सचे (!varp->hasPrev()) m_ctx.m_deleteps.प्रक्षिप_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            सचे (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                पटिनिवत्त;
            }

            // Consider eliminating variables
            DfgVertexVar* ध्रुव varp = vtx.cast<DfgVertexVar>();
            सचे (!varp) पटिनिवत्त;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* ध्रुव srcp = varp->srcp();
            सचे (!srcp) पटिनिवत्त;

            // Can't eliminate if referenced external to the module - can't replace those refs
            सचे (varp->hasExtRefs()) पटिनिवत्त;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            सचे (varp->hasModWrRefs()) पटिनिवत्त;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            सचे (varp->hasPrev()) पटिनिवत्त;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            सचे (varp->defaultp()) पटिनिवत्त;
            सचे (srcp->is<DfgVertexSplice>()) पटिनिवत्त;
            सचे (srcp->is<DfgUnitArray>()) पटिनिवत्त;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            सचे (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) पटिनिवत्त;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            पटिनिवत्त;
        });
    }

    शून्य निवेशय_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        अत्थाय (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            सचे (vtx.is<DfgVertexSplice>()) अनुवर्त;
            सचे (vtx.is<DfgUnitArray>()) अनुवर्त;
            // Add to map
            ध्रुव uint32_t fanout = vtx.fanout();
            सचे (!fanout) अनुवर्त;
            सचे (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].प्रक्षिप_back(&vtx);
        }
        सचे (fanout2Vtxps.empty()) पटिनिवत्त;

        // Ensure intermediate values used multiple times are written to variables
        अत्थाय (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            अत्थाय (DfgVertex* ध्रुव vtxp : fanout2Vtxps[fanout]) {
                सचे (!needsTemporary(*vtxp, *vtxp)) अनुवर्त;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_पङ्क्तिः* ध्रुव flp = vtxp->fileline();
                AstScope* ध्रुव scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* ध्रुव newp
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
        सचे (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        सचे (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        निवेशय_Temporaries();
        सचे (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

शून्य V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
