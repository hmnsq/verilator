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
            ᱡᱩᱫᱤ (vtx.is<DfgVertexSplice>()) continue;
            ᱡᱩᱫᱤ (vtx.is<DfgUnitArray>()) continue;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* const varp = vtx.getResultVar();
            ᱡᱩᱫᱤ (!varp) continue;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    static bool isUnused(const DfgVertex& vtx) {
        ᱡᱩᱫᱤ (vtx.hasSinks()) ᱨᱩᱣᱟᱹᱲ false;
        ᱡᱩᱫᱤ (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            ᱡᱩᱫᱤ (varp->hasModWrRefs()) ᱨᱩᱣᱟᱹᱲ false;
            ᱡᱩᱫᱤ (varp->hasExtRefs()) ᱨᱩᱣᱟᱹᱲ false;
            ᱡᱩᱫᱤ (varp->hasPrev()) ᱨᱩᱣᱟᱹᱲ false;
        }
        ᱨᱩᱣᱟᱹᱲ true;
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
        ᱡᱩᱫᱤ (aVtx.is<DfgPrev>()) ᱨᱩᱣᱟᱹᱲ false;

        ᱡᱩᱫᱤ (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            ᱡᱩᱫᱤ (!aVtx.isCheaperThanLoad()) ᱨᱩᱣᱟᱹᱲ true;

            // Not adding temporary
            ᱨᱩᱣᱟᱹᱲ false;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        ᱡᱩᱫᱤ (sink.is<DfgVertexVar>()) ᱨᱩᱣᱟᱹᱲ false;

        // CReset always needs to be driving a variable
        ᱡᱩᱫᱤ (aVtx.is<DfgCReset>()) ᱨᱩᱣᱟᱹᱲ true;

        // Do not inline expressions into a loop body
        ᱡᱩᱫᱤ (const DfgAstRd* const astRdp = sink.cast<DfgAstRd>()) { ᱨᱩᱣᱟᱹᱲ astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        ᱡᱩᱫᱤ (aVtx.is<DfgConcat>()) {
            ᱡᱩᱫᱤ (sink.is<DfgConcat>()) ᱨᱩᱣᱟᱹᱲ false;  // Not root of tree
            ᱨᱩᱣᱟᱹᱲ VL_WORDS_I(static_cast<puur_naa_nka>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        ᱨᱩᱣᱟᱹᱲ false;
    }

    void eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            ᱡᱩᱫᱤ (vtx.is<DfgVertexAst>()) ᱨᱩᱣᱟᱹᱲ;
            ᱡᱩᱫᱤ (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.push_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        const auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.push_front(src);
                ᱨᱩᱣᱟᱹᱲ false;
            });
            // Delete corresponsing Ast variable at the end
            ᱡᱩᱫᱤ (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
                ᱡᱩᱫᱤ (!varp->hasPrev()) m_ctx.m_deleteps.push_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            ᱡᱩᱫᱤ (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                ᱨᱩᱣᱟᱹᱲ;
            }

            // Consider eliminating variables
            DfgVertexVar* const varp = vtx.cast<DfgVertexVar>();
            ᱡᱩᱫᱤ (!varp) ᱨᱩᱣᱟᱹᱲ;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* const srcp = varp->srcp();
            ᱡᱩᱫᱤ (!srcp) ᱨᱩᱣᱟᱹᱲ;

            // Can't eliminate if referenced external to the module - can't replace those refs
            ᱡᱩᱫᱤ (varp->hasExtRefs()) ᱨᱩᱣᱟᱹᱲ;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            ᱡᱩᱫᱤ (varp->hasModWrRefs()) ᱨᱩᱣᱟᱹᱲ;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            ᱡᱩᱫᱤ (varp->hasPrev()) ᱨᱩᱣᱟᱹᱲ;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            ᱡᱩᱫᱤ (varp->defaultp()) ᱨᱩᱣᱟᱹᱲ;
            ᱡᱩᱫᱤ (srcp->is<DfgVertexSplice>()) ᱨᱩᱣᱟᱹᱲ;
            ᱡᱩᱫᱤ (srcp->is<DfgUnitArray>()) ᱨᱩᱣᱟᱹᱲ;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            ᱡᱩᱫᱤ (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) ᱨᱩᱣᱟᱹᱲ;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            ᱨᱩᱣᱟᱹᱲ;
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
            ᱡᱩᱫᱤ (vtx.is<DfgVertexSplice>()) continue;
            ᱡᱩᱫᱤ (vtx.is<DfgUnitArray>()) continue;
            // Add to map
            const uint32_t fanout = vtx.fanout();
            ᱡᱩᱫᱤ (!fanout) continue;
            ᱡᱩᱫᱤ (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].push_back(&vtx);
        }
        ᱡᱩᱫᱤ (fanout2Vtxps.empty()) ᱨᱩᱣᱟᱹᱲ;

        // Ensure intermediate values used multiple times are written to variables
        for (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            for (DfgVertex* const vtxp : fanout2Vtxps[fanout]) {
                ᱡᱩᱫᱤ (!needsTemporary(*vtxp, *vtxp)) continue;
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
        ᱡᱩᱫᱤ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        ᱡᱩᱫᱤ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        insertTemporaries();
        ᱡᱩᱫᱤ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

public:
    static void apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

void V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
