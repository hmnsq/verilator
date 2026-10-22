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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Dfg.h"
#অন্তর্ভাৱ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

শ্ৰেণী DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    শূন্য uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        বাবে (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            যদি (vtx.is<DfgVertexSplice>()) অনুৱর্ত;
            যদি (vtx.is<DfgUnitArray>()) অনুৱর্ত;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* ধ্রুৱ varp = vtx.getResultVar();
            যদি (!varp) অনুৱর্ত;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    স্থির তার্কিক isUnused(ধ্রুৱ DfgVertex& vtx) {
        যদি (vtx.hasSinks()) ঘূৰাই মিছা;
        যদি (ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            যদি (varp->hasModWrRefs()) ঘূৰাই মিছা;
            যদি (varp->hasExtRefs()) ঘূৰাই মিছা;
            যদি (varp->hasPrev()) ঘূৰাই মিছা;
        }
        ঘূৰাই সঁচা;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    তার্কিক needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        যদি (aVtx.is<DfgPrev>()) ঘূৰাই মিছা;

        যদি (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            যদি (!aVtx.isCheaperThanLoad()) ঘূৰাই সঁচা;

            // Not adding temporary
            ঘূৰাই মিছা;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        যদি (sink.is<DfgVertexVar>()) ঘূৰাই মিছা;

        // CReset always needs to be driving a variable
        যদি (aVtx.is<DfgCReset>()) ঘূৰাই সঁচা;

        // Do not inline expressions into a loop body
        যদি (ধ্রুৱ DfgAstRd* ধ্রুৱ astRdp = sink.cast<DfgAstRd>()) { ঘূৰাই astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        যদি (aVtx.is<DfgConcat>()) {
            যদি (sink.is<DfgConcat>()) ঘূৰাই মিছা;  // Not root of tree
            ঘূৰাই VL_শব্দগুলি_I(static_cast<পূর্ণাঙ্ক>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        ঘূৰাই মিছা;
    }

    শূন্য eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            যদি (vtx.is<DfgVertexAst>()) ঘূৰাই;
            যদি (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.ঠেলো_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ধ্রুৱ auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.ঠেলো_front(src);
                ঘূৰাই মিছা;
            });
            // Delete corresponsing Ast variable at the end
            যদি (ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = vtx.cast<DfgVertexVar>()) {
                যদি (!varp->hasPrev()) m_ctx.m_deleteps.ঠেলো_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            যদি (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                ঘূৰাই;
            }

            // Consider eliminating variables
            DfgVertexVar* ধ্রুৱ varp = vtx.cast<DfgVertexVar>();
            যদি (!varp) ঘূৰাই;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* ধ্রুৱ srcp = varp->srcp();
            যদি (!srcp) ঘূৰাই;

            // Can't eliminate if referenced external to the module - can't replace those refs
            যদি (varp->hasExtRefs()) ঘূৰাই;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            যদি (varp->hasModWrRefs()) ঘূৰাই;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            যদি (varp->hasPrev()) ঘূৰাই;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            যদি (varp->defaultp()) ঘূৰাই;
            যদি (srcp->is<DfgVertexSplice>()) ঘূৰাই;
            যদি (srcp->is<DfgUnitArray>()) ঘূৰাই;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            যদি (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) ঘূৰাই;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            ঘূৰাই;
        });
    }

    শূন্য ঢোকাও_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        বাবে (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            যদি (vtx.is<DfgVertexSplice>()) অনুৱর্ত;
            যদি (vtx.is<DfgUnitArray>()) অনুৱর্ত;
            // Add to map
            ধ্রুৱ uint32_t fanout = vtx.fanout();
            যদি (!fanout) অনুৱর্ত;
            যদি (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ঠেলো_back(&vtx);
        }
        যদি (fanout2Vtxps.empty()) ঘূৰাই;

        // Ensure intermediate values used multiple times are written to variables
        বাবে (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            বাবে (DfgVertex* ধ্রুৱ vtxp : fanout2Vtxps[fanout]) {
                যদি (!needsTemporary(*vtxp, *vtxp)) অনুৱর্ত;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
                AstScope* ধ্রুৱ scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* ধ্রুৱ newp
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
        যদি (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        যদি (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        ঢোকাও_Temporaries();
        যদি (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

সার্ৱজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

শূন্য V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
