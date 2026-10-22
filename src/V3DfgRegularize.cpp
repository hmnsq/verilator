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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

رده DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    تهی uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        برای (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            اگر (vtx.is<DfgVertexSplice>()) ادامه;
            اگر (vtx.is<DfgUnitArray>()) ادامه;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* ثابت varp = vtx.getResultVar();
            اگر (!varp) ادامه;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    ایستا منطقی isUnused(ثابت DfgVertex& vtx) {
        اگر (vtx.hasSinks()) بازگشت نادرست;
        اگر (ثابت DfgVertexVar* ثابت varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            اگر (varp->hasModWrRefs()) بازگشت نادرست;
            اگر (varp->hasExtRefs()) بازگشت نادرست;
            اگر (varp->hasPrev()) بازگشت نادرست;
        }
        بازگشت درست;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    منطقی needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        اگر (aVtx.is<DfgPrev>()) بازگشت نادرست;

        اگر (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            اگر (!aVtx.isCheaperThanLoad()) بازگشت درست;

            // Not adding temporary
            بازگشت نادرست;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        اگر (sink.is<DfgVertexVar>()) بازگشت نادرست;

        // CReset always needs to be driving a variable
        اگر (aVtx.is<DfgCReset>()) بازگشت درست;

        // Do not inline expressions into a loop body
        اگر (ثابت DfgAstRd* ثابت astRdp = sink.cast<DfgAstRd>()) { بازگشت astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        اگر (aVtx.is<DfgConcat>()) {
            اگر (sink.is<DfgConcat>()) بازگشت نادرست;  // Not root of tree
            بازگشت VL_واژگان_I(static_cast<صحیح>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        بازگشت نادرست;
    }

    تهی eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            اگر (vtx.is<DfgVertexAst>()) بازگشت;
            اگر (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.افزودن_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ثابت auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.افزودن_front(src);
                بازگشت نادرست;
            });
            // Delete corresponsing Ast variable at the end
            اگر (ثابت DfgVertexVar* ثابت varp = vtx.cast<DfgVertexVar>()) {
                اگر (!varp->hasPrev()) m_ctx.m_deleteps.افزودن_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            اگر (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                بازگشت;
            }

            // Consider eliminating variables
            DfgVertexVar* ثابت varp = vtx.cast<DfgVertexVar>();
            اگر (!varp) بازگشت;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* ثابت srcp = varp->srcp();
            اگر (!srcp) بازگشت;

            // Can't eliminate if referenced external to the module - can't replace those refs
            اگر (varp->hasExtRefs()) بازگشت;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            اگر (varp->hasModWrRefs()) بازگشت;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            اگر (varp->hasPrev()) بازگشت;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            اگر (varp->defaultp()) بازگشت;
            اگر (srcp->is<DfgVertexSplice>()) بازگشت;
            اگر (srcp->is<DfgUnitArray>()) بازگشت;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            اگر (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) بازگشت;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            بازگشت;
        });
    }

    تهی درج_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        برای (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            اگر (vtx.is<DfgVertexSplice>()) ادامه;
            اگر (vtx.is<DfgUnitArray>()) ادامه;
            // Add to map
            ثابت uint32_t fanout = vtx.fanout();
            اگر (!fanout) ادامه;
            اگر (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].افزودن_back(&vtx);
        }
        اگر (fanout2Vtxps.empty()) بازگشت;

        // Ensure intermediate values used multiple times are written to variables
        برای (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            برای (DfgVertex* ثابت vtxp : fanout2Vtxps[fanout]) {
                اگر (!needsTemporary(*vtxp, *vtxp)) ادامه;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_سطر* ثابت flp = vtxp->fileline();
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
        اگر (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        اگر (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        درج_Temporaries();
        اگر (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

عمومی:
    ایستا تهی apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

تهی V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
