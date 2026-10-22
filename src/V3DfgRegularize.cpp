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

वर्ग DfgRegularize final {
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
        साठी (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            जर (vtx.is<DfgVertexSplice>()) सुरूठेवा;
            जर (vtx.is<DfgUnitArray>()) सुरूठेवा;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* स्थिरांक varp = vtx.getResultVar();
            जर (!varp) सुरूठेवा;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    स्थिर तार्किक isUnused(स्थिरांक DfgVertex& vtx) {
        जर (vtx.hasSinks()) परत असत्य;
        जर (स्थिरांक DfgVertexVar* स्थिरांक varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            जर (varp->hasModWrRefs()) परत असत्य;
            जर (varp->hasExtRefs()) परत असत्य;
            जर (varp->hasPrev()) परत असत्य;
        }
        परत सत्य;
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
        जर (aVtx.is<DfgPrev>()) परत असत्य;

        जर (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            जर (!aVtx.isCheaperThanLoad()) परत सत्य;

            // Not adding temporary
            परत असत्य;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        जर (sink.is<DfgVertexVar>()) परत असत्य;

        // CReset always needs to be driving a variable
        जर (aVtx.is<DfgCReset>()) परत सत्य;

        // Do not inline expressions into a loop body
        जर (स्थिरांक DfgAstRd* स्थिरांक astRdp = sink.cast<DfgAstRd>()) { परत astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        जर (aVtx.is<DfgConcat>()) {
            जर (sink.is<DfgConcat>()) परत असत्य;  // Not root of tree
            परत VL_शब्द_I(static_cast<पूर्णांक>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        परत असत्य;
    }

    शून्य eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            जर (vtx.is<DfgVertexAst>()) परत;
            जर (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.ढकला_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        स्थिरांक स्वतः removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.ढकला_front(src);
                परत असत्य;
            });
            // Delete corresponsing Ast variable at the end
            जर (स्थिरांक DfgVertexVar* स्थिरांक varp = vtx.cast<DfgVertexVar>()) {
                जर (!varp->hasPrev()) m_ctx.m_deleteps.ढकला_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            जर (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                परत;
            }

            // Consider eliminating variables
            DfgVertexVar* स्थिरांक varp = vtx.cast<DfgVertexVar>();
            जर (!varp) परत;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* स्थिरांक srcp = varp->srcp();
            जर (!srcp) परत;

            // Can't eliminate if referenced external to the module - can't replace those refs
            जर (varp->hasExtRefs()) परत;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            जर (varp->hasModWrRefs()) परत;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            जर (varp->hasPrev()) परत;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            जर (varp->defaultp()) परत;
            जर (srcp->is<DfgVertexSplice>()) परत;
            जर (srcp->is<DfgUnitArray>()) परत;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            जर (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) परत;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            परत;
        });
    }

    शून्य घाला_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        साठी (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            जर (vtx.is<DfgVertexSplice>()) सुरूठेवा;
            जर (vtx.is<DfgUnitArray>()) सुरूठेवा;
            // Add to map
            स्थिरांक uint32_t fanout = vtx.fanout();
            जर (!fanout) सुरूठेवा;
            जर (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ढकला_back(&vtx);
        }
        जर (fanout2Vtxps.empty()) परत;

        // Ensure intermediate values used multiple times are written to variables
        साठी (माप_प्रकार fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            साठी (DfgVertex* स्थिरांक vtxp : fanout2Vtxps[fanout]) {
                जर (!needsTemporary(*vtxp, *vtxp)) सुरूठेवा;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_ओळ* स्थिरांक flp = vtxp->fileline();
                AstScope* स्थिरांक scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* स्थिरांक newp
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
        जर (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        जर (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        घाला_Temporaries();
        जर (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

शून्य V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
