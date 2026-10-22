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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ DfgRegularize final {
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
        প্রতি (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            যদি (vtx.is<DfgVertexSplice>()) অনুবর্ত;
            যদি (vtx.is<DfgUnitArray>()) অনুবর্ত;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* ধ্রুব varp = vtx.getResultVar();
            যদি (!varp) অনুবর্ত;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    স্থির তার্কিক isUnused(ধ্রুব DfgVertex& vtx) {
        যদি (vtx.hasSinks()) প্রত্যাগম অসত্য;
        যদি (ধ্রুব DfgVertexVar* ধ্রুব varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            যদি (varp->hasModWrRefs()) প্রত্যাগম অসত্য;
            যদি (varp->hasExtRefs()) প্রত্যাগম অসত্য;
            যদি (varp->hasPrev()) প্রত্যাগম অসত্য;
        }
        প্রত্যাগম সত্য;
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
        যদি (aVtx.is<DfgPrev>()) প্রত্যাগম অসত্য;

        যদি (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            যদি (!aVtx.isCheaperThanLoad()) প্রত্যাগম সত্য;

            // Not adding temporary
            প্রত্যাগম অসত্য;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        যদি (sink.is<DfgVertexVar>()) প্রত্যাগম অসত্য;

        // CReset always needs to be driving a variable
        যদি (aVtx.is<DfgCReset>()) প্রত্যাগম সত্য;

        // Do not inline expressions into a loop body
        যদি (ধ্রুব DfgAstRd* ধ্রুব astRdp = sink.cast<DfgAstRd>()) { প্রত্যাগম astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        যদি (aVtx.is<DfgConcat>()) {
            যদি (sink.is<DfgConcat>()) প্রত্যাগম অসত্য;  // Not root of tree
            প্রত্যাগম VL_শব্দগুলি_I(static_cast<পূর্ণাঙ্ক>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        প্রত্যাগম অসত্য;
    }

    শূন্য eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            যদি (vtx.is<DfgVertexAst>()) প্রত্যাগম;
            যদি (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.ঠেলো_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ধ্রুব auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.ঠেলো_front(src);
                প্রত্যাগম অসত্য;
            });
            // Delete corresponsing Ast variable at the end
            যদি (ধ্রুব DfgVertexVar* ধ্রুব varp = vtx.cast<DfgVertexVar>()) {
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
                প্রত্যাগম;
            }

            // Consider eliminating variables
            DfgVertexVar* ধ্রুব varp = vtx.cast<DfgVertexVar>();
            যদি (!varp) প্রত্যাগম;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* ধ্রুব srcp = varp->srcp();
            যদি (!srcp) প্রত্যাগম;

            // Can't eliminate if referenced external to the module - can't replace those refs
            যদি (varp->hasExtRefs()) প্রত্যাগম;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            যদি (varp->hasModWrRefs()) প্রত্যাগম;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            যদি (varp->hasPrev()) প্রত্যাগম;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            যদি (varp->defaultp()) প্রত্যাগম;
            যদি (srcp->is<DfgVertexSplice>()) প্রত্যাগম;
            যদি (srcp->is<DfgUnitArray>()) প্রত্যাগম;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            যদি (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) প্রত্যাগম;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            প্রত্যাগম;
        });
    }

    শূন্য ঢোকাও_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        প্রতি (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            যদি (vtx.is<DfgVertexSplice>()) অনুবর্ত;
            যদি (vtx.is<DfgUnitArray>()) অনুবর্ত;
            // Add to map
            ধ্রুব uint32_t fanout = vtx.fanout();
            যদি (!fanout) অনুবর্ত;
            যদি (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ঠেলো_back(&vtx);
        }
        যদি (fanout2Vtxps.empty()) প্রত্যাগম;

        // Ensure intermediate values used multiple times are written to variables
        প্রতি (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            প্রতি (DfgVertex* ধ্রুব vtxp : fanout2Vtxps[fanout]) {
                যদি (!needsTemporary(*vtxp, *vtxp)) অনুবর্ত;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_সারি* ধ্রুব flp = vtxp->fileline();
                AstScope* ধ্রুব scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* ধ্রুব newp
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

সার্বজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

শূন্য V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
