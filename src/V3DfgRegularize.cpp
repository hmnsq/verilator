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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    ಶೂನ್ಯ uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        ಪ್ರತಿ (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            ಯದಿ (vtx.is<DfgVertexSplice>()) ಅನುವರ್ತ;
            ಯದಿ (vtx.is<DfgUnitArray>()) ಅನುವರ್ತ;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* ಧ್ರುವ varp = vtx.getResultVar();
            ಯದಿ (!varp) ಅನುವರ್ತ;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isUnused(ಧ್ರುವ DfgVertex& vtx) {
        ಯದಿ (vtx.hasSinks()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            ಯದಿ (varp->hasModWrRefs()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (varp->hasExtRefs()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (varp->hasPrev()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    ತಾರ್ಕಿಕ needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        ಯದಿ (aVtx.is<DfgPrev>()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಯದಿ (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            ಯದಿ (!aVtx.isCheaperThanLoad()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

            // Not adding temporary
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        ಯದಿ (sink.is<DfgVertexVar>()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // CReset always needs to be driving a variable
        ಯದಿ (aVtx.is<DfgCReset>()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

        // Do not inline expressions into a loop body
        ಯದಿ (ಧ್ರುವ DfgAstRd* ಧ್ರುವ astRdp = sink.cast<DfgAstRd>()) { ಪ್ರತ್ಯಾಗಮ astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        ಯದಿ (aVtx.is<DfgConcat>()) {
            ಯದಿ (sink.is<DfgConcat>()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Not root of tree
            ಪ್ರತ್ಯಾಗಮ VL_ಪದಗಳು_I(static_cast<ಪೂರ್ಣಾಙ್ಕ>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            ಯದಿ (vtx.is<DfgVertexAst>()) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.ತಳ್ಳು_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ಧ್ರುವ auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.ತಳ್ಳು_front(src);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            // Delete corresponsing Ast variable at the end
            ಯದಿ (ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varp = vtx.cast<DfgVertexVar>()) {
                ಯದಿ (!varp->hasPrev()) m_ctx.m_deleteps.ತಳ್ಳು_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            ಯದಿ (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                ಪ್ರತ್ಯಾಗಮ;
            }

            // Consider eliminating variables
            DfgVertexVar* ಧ್ರುವ varp = vtx.cast<DfgVertexVar>();
            ಯದಿ (!varp) ಪ್ರತ್ಯಾಗಮ;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* ಧ್ರುವ srcp = varp->srcp();
            ಯದಿ (!srcp) ಪ್ರತ್ಯಾಗಮ;

            // Can't eliminate if referenced external to the module - can't replace those refs
            ಯದಿ (varp->hasExtRefs()) ಪ್ರತ್ಯಾಗಮ;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            ಯದಿ (varp->hasModWrRefs()) ಪ್ರತ್ಯಾಗಮ;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            ಯದಿ (varp->hasPrev()) ಪ್ರತ್ಯಾಗಮ;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            ಯದಿ (varp->defaultp()) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (srcp->is<DfgVertexSplice>()) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (srcp->is<DfgUnitArray>()) ಪ್ರತ್ಯಾಗಮ;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            ಯದಿ (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) ಪ್ರತ್ಯಾಗಮ;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            ಪ್ರತ್ಯಾಗಮ;
        });
    }

    ಶೂನ್ಯ ಸೇರಿಸು_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        ಪ್ರತಿ (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            ಯದಿ (vtx.is<DfgVertexSplice>()) ಅನುವರ್ತ;
            ಯದಿ (vtx.is<DfgUnitArray>()) ಅನುವರ್ತ;
            // Add to map
            ಧ್ರುವ uint32_t fanout = vtx.fanout();
            ಯದಿ (!fanout) ಅನುವರ್ತ;
            ಯದಿ (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ತಳ್ಳು_back(&vtx);
        }
        ಯದಿ (fanout2Vtxps.empty()) ಪ್ರತ್ಯಾಗಮ;

        // Ensure intermediate values used multiple times are written to variables
        ಪ್ರತಿ (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : fanout2Vtxps[fanout]) {
                ಯದಿ (!needsTemporary(*vtxp, *vtxp)) ಅನುವರ್ತ;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
                AstScope* ಧ್ರುವ scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* ಧ್ರುವ newp
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
        ಯದಿ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        ಯದಿ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        ಸೇರಿಸು_Temporaries();
        ಯದಿ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

ಶೂನ್ಯ V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
