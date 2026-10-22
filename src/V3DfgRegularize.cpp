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
        အတွက် (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            အကယ်၍ (vtx.is<DfgVertexSplice>()) continue;
            အကယ်၍ (vtx.is<DfgUnitArray>()) continue;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* const varp = vtx.getResultVar();
            အကယ်၍ (!varp) continue;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    static bool isUnused(const DfgVertex& vtx) {
        အကယ်၍ (vtx.hasSinks()) ပြန်ပေး false;
        အကယ်၍ (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            အကယ်၍ (varp->hasModWrRefs()) ပြန်ပေး false;
            အကယ်၍ (varp->hasExtRefs()) ပြန်ပေး false;
            အကယ်၍ (varp->hasPrev()) ပြန်ပေး false;
        }
        ပြန်ပေး true;
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
        အကယ်၍ (aVtx.is<DfgPrev>()) ပြန်ပေး false;

        အကယ်၍ (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            အကယ်၍ (!aVtx.isCheaperThanLoad()) ပြန်ပေး true;

            // Not adding temporary
            ပြန်ပေး false;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        အကယ်၍ (sink.is<DfgVertexVar>()) ပြန်ပေး false;

        // CReset always needs to be driving a variable
        အကယ်၍ (aVtx.is<DfgCReset>()) ပြန်ပေး true;

        // Do not inline expressions into a loop body
        အကယ်၍ (const DfgAstRd* const astRdp = sink.cast<DfgAstRd>()) { ပြန်ပေး astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        အကယ်၍ (aVtx.is<DfgConcat>()) {
            အကယ်၍ (sink.is<DfgConcat>()) ပြန်ပေး false;  // Not root of tree
            ပြန်ပေး VL_WORDS_I(static_cast<ကိန်းပြည့်>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        ပြန်ပေး false;
    }

    void eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            အကယ်၍ (vtx.is<DfgVertexAst>()) ပြန်ပေး;
            အကယ်၍ (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.push_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        const auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.push_front(src);
                ပြန်ပေး false;
            });
            // Delete corresponsing Ast variable at the end
            အကယ်၍ (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
                အကယ်၍ (!varp->hasPrev()) m_ctx.m_deleteps.push_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            အကယ်၍ (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                ပြန်ပေး;
            }

            // Consider eliminating variables
            DfgVertexVar* const varp = vtx.cast<DfgVertexVar>();
            အကယ်၍ (!varp) ပြန်ပေး;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* const srcp = varp->srcp();
            အကယ်၍ (!srcp) ပြန်ပေး;

            // Can't eliminate if referenced external to the module - can't replace those refs
            အကယ်၍ (varp->hasExtRefs()) ပြန်ပေး;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            အကယ်၍ (varp->hasModWrRefs()) ပြန်ပေး;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            အကယ်၍ (varp->hasPrev()) ပြန်ပေး;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            အကယ်၍ (varp->defaultp()) ပြန်ပေး;
            အကယ်၍ (srcp->is<DfgVertexSplice>()) ပြန်ပေး;
            အကယ်၍ (srcp->is<DfgUnitArray>()) ပြန်ပေး;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            အကယ်၍ (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) ပြန်ပေး;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            ပြန်ပေး;
        });
    }

    void insertTemporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        အတွက် (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            အကယ်၍ (vtx.is<DfgVertexSplice>()) continue;
            အကယ်၍ (vtx.is<DfgUnitArray>()) continue;
            // Add to map
            const uint32_t fanout = vtx.fanout();
            အကယ်၍ (!fanout) continue;
            အကယ်၍ (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].push_back(&vtx);
        }
        အကယ်၍ (fanout2Vtxps.empty()) ပြန်ပေး;

        // Ensure intermediate values used multiple times are written to variables
        အတွက် (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            အတွက် (DfgVertex* const vtxp : fanout2Vtxps[fanout]) {
                အကယ်၍ (!needsTemporary(*vtxp, *vtxp)) continue;
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
        အကယ်၍ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        အကယ်၍ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        insertTemporaries();
        အကယ်၍ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

public:
    static void apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

void V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
