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
        สำหรับ (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            ถ้า (vtx.is<DfgVertexSplice>()) continue;
            ถ้า (vtx.is<DfgUnitArray>()) continue;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* const varp = vtx.getResultVar();
            ถ้า (!varp) continue;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    static bool isUnused(const DfgVertex& vtx) {
        ถ้า (vtx.hasSinks()) คืน false;
        ถ้า (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            ถ้า (varp->hasModWrRefs()) คืน false;
            ถ้า (varp->hasExtRefs()) คืน false;
            ถ้า (varp->hasPrev()) คืน false;
        }
        คืน true;
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
        ถ้า (aVtx.is<DfgPrev>()) คืน false;

        ถ้า (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            ถ้า (!aVtx.isCheaperThanLoad()) คืน true;

            // Not adding temporary
            คืน false;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        ถ้า (sink.is<DfgVertexVar>()) คืน false;

        // CReset always needs to be driving a variable
        ถ้า (aVtx.is<DfgCReset>()) คืน true;

        // Do not inline expressions into a loop body
        ถ้า (const DfgAstRd* const astRdp = sink.cast<DfgAstRd>()) { คืน astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        ถ้า (aVtx.is<DfgConcat>()) {
            ถ้า (sink.is<DfgConcat>()) คืน false;  // Not root of tree
            คืน VL_WORDS_I(static_cast<จำนวนเต็ม>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        คืน false;
    }

    void eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            ถ้า (vtx.is<DfgVertexAst>()) คืน;
            ถ้า (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.push_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        const auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.push_front(src);
                คืน false;
            });
            // Delete corresponsing Ast variable at the end
            ถ้า (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
                ถ้า (!varp->hasPrev()) m_ctx.m_deleteps.push_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            ถ้า (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                คืน;
            }

            // Consider eliminating variables
            DfgVertexVar* const varp = vtx.cast<DfgVertexVar>();
            ถ้า (!varp) คืน;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* const srcp = varp->srcp();
            ถ้า (!srcp) คืน;

            // Can't eliminate if referenced external to the module - can't replace those refs
            ถ้า (varp->hasExtRefs()) คืน;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            ถ้า (varp->hasModWrRefs()) คืน;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            ถ้า (varp->hasPrev()) คืน;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            ถ้า (varp->defaultp()) คืน;
            ถ้า (srcp->is<DfgVertexSplice>()) คืน;
            ถ้า (srcp->is<DfgUnitArray>()) คืน;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            ถ้า (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) คืน;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            คืน;
        });
    }

    void insertTemporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        สำหรับ (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            ถ้า (vtx.is<DfgVertexSplice>()) continue;
            ถ้า (vtx.is<DfgUnitArray>()) continue;
            // Add to map
            const uint32_t fanout = vtx.fanout();
            ถ้า (!fanout) continue;
            ถ้า (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].push_back(&vtx);
        }
        ถ้า (fanout2Vtxps.empty()) คืน;

        // Ensure intermediate values used multiple times are written to variables
        สำหรับ (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            สำหรับ (DfgVertex* const vtxp : fanout2Vtxps[fanout]) {
                ถ้า (!needsTemporary(*vtxp, *vtxp)) continue;
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
        ถ้า (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        ถ้า (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        insertTemporaries();
        ถ้า (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

public:
    static void apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

void V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
