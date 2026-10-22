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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Dfg.h"
#include "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

class DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    void uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        for (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            ᬬᬾᬦ᭄ (vtx.is<DfgVertexSplice>()) continue;
            ᬬᬾᬦ᭄ (vtx.is<DfgUnitArray>()) continue;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* const varp = vtx.getResultVar();
            ᬬᬾᬦ᭄ (!varp) continue;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    static bool isUnused(const DfgVertex& vtx) {
        ᬬᬾᬦ᭄ (vtx.hasSinks()) ᬯᬮᬶ false;
        ᬬᬾᬦ᭄ (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            ᬬᬾᬦ᭄ (varp->hasModWrRefs()) ᬯᬮᬶ false;
            ᬬᬾᬦ᭄ (varp->hasExtRefs()) ᬯᬮᬶ false;
            ᬬᬾᬦ᭄ (varp->hasPrev()) ᬯᬮᬶ false;
        }
        ᬯᬮᬶ true;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    bool needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        ᬬᬾᬦ᭄ (aVtx.is<DfgPrev>()) ᬯᬮᬶ false;

        ᬬᬾᬦ᭄ (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            ᬬᬾᬦ᭄ (!aVtx.isCheaperThanLoad()) ᬯᬮᬶ true;

            // Not adding temporary
            ᬯᬮᬶ false;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        ᬬᬾᬦ᭄ (sink.is<DfgVertexVar>()) ᬯᬮᬶ false;

        // CReset always needs to be driving a variable
        ᬬᬾᬦ᭄ (aVtx.is<DfgCReset>()) ᬯᬮᬶ true;

        // Do not inline expressions into a loop body
        ᬬᬾᬦ᭄ (const DfgAstRd* const astRdp = sink.cast<DfgAstRd>()) { ᬯᬮᬶ astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        ᬬᬾᬦ᭄ (aVtx.is<DfgConcat>()) {
            ᬬᬾᬦ᭄ (sink.is<DfgConcat>()) ᬯᬮᬶ false;  // Not root of tree
            ᬯᬮᬶ VL_WORDS_I(static_cast<puur_naa_nka>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        ᬯᬮᬶ false;
    }

    void eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            ᬬᬾᬦ᭄ (vtx.is<DfgVertexAst>()) ᬯᬮᬶ;
            ᬬᬾᬦ᭄ (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.push_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        const auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.push_front(src);
                ᬯᬮᬶ false;
            });
            // Delete corresponsing Ast variable at the end
            ᬬᬾᬦ᭄ (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
                ᬬᬾᬦ᭄ (!varp->hasPrev()) m_ctx.m_deleteps.push_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            ᬬᬾᬦ᭄ (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                ᬯᬮᬶ;
            }

            // Consider eliminating variables
            DfgVertexVar* const varp = vtx.cast<DfgVertexVar>();
            ᬬᬾᬦ᭄ (!varp) ᬯᬮᬶ;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* const srcp = varp->srcp();
            ᬬᬾᬦ᭄ (!srcp) ᬯᬮᬶ;

            // Can't eliminate if referenced external to the module - can't replace those refs
            ᬬᬾᬦ᭄ (varp->hasExtRefs()) ᬯᬮᬶ;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            ᬬᬾᬦ᭄ (varp->hasModWrRefs()) ᬯᬮᬶ;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            ᬬᬾᬦ᭄ (varp->hasPrev()) ᬯᬮᬶ;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            ᬬᬾᬦ᭄ (varp->defaultp()) ᬯᬮᬶ;
            ᬬᬾᬦ᭄ (srcp->is<DfgVertexSplice>()) ᬯᬮᬶ;
            ᬬᬾᬦ᭄ (srcp->is<DfgUnitArray>()) ᬯᬮᬶ;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            ᬬᬾᬦ᭄ (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) ᬯᬮᬶ;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            ᬯᬮᬶ;
        });
    }

    void insertTemporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        for (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            ᬬᬾᬦ᭄ (vtx.is<DfgVertexSplice>()) continue;
            ᬬᬾᬦ᭄ (vtx.is<DfgUnitArray>()) continue;
            // Add to map
            const uint32_t fanout = vtx.fanout();
            ᬬᬾᬦ᭄ (!fanout) continue;
            ᬬᬾᬦ᭄ (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].push_back(&vtx);
        }
        ᬬᬾᬦ᭄ (fanout2Vtxps.empty()) ᬯᬮᬶ;

        // Ensure intermediate values used multiple times are written to variables
        for (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            for (DfgVertex* const vtxp : fanout2Vtxps[fanout]) {
                ᬬᬾᬦ᭄ (!needsTemporary(*vtxp, *vtxp)) continue;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                FileLine* const flp = vtxp->fileline();
                AstScope* const scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* const newp
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
        ᬬᬾᬦ᭄ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        ᬬᬾᬦ᭄ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        insertTemporaries();
        ᬬᬾᬦ᭄ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

public:
    static void apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

void V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
