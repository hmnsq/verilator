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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    व्योम uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        क्रम (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            अगर (vtx.is<DfgVertexSplice>()) जारी;
            अगर (vtx.is<DfgUnitArray>()) जारी;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* स्थिर varp = vtx.get_परिणाम_Var();
            अगर (!varp) जारी;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    अटल bool isUnused(स्थिर DfgVertex& vtx) {
        अगर (vtx.hasSinks()) वापस false;
        अगर (स्थिर DfgVertexVar* स्थिर varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            अगर (varp->hasModWrRefs()) वापस false;
            अगर (varp->hasExtRefs()) वापस false;
            अगर (varp->has_पूर्व()) वापस false;
        }
        वापस true;
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
        अगर (aVtx.is<Dfg_पूर्व>()) वापस false;

        अगर (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            अगर (!aVtx.isCheaperThanLoad()) वापस true;

            // Not adding temporary
            वापस false;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        अगर (sink.is<DfgVertexVar>()) वापस false;

        // CReset always needs to be driving a variable
        अगर (aVtx.is<DfgCReset>()) वापस true;

        // Do not inline expressions into a loop body
        अगर (स्थिर DfgAstRd* स्थिर astRdp = sink.cast<DfgAstRd>()) { वापस astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        अगर (aVtx.is<DfgConcat>()) {
            अगर (sink.is<DfgConcat>()) वापस false;  // Not root of tree
            वापस VL_शब्दगण_I(static_cast<पूर्णांक>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        वापस false;
    }

    व्योम eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            अगर (vtx.is<DfgVertexAst>()) वापस;
            अगर (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.धकेलो_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        स्थिर स्वतः removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.धकेलो_front(src);
                वापस false;
            });
            // Delete corresponsing Ast variable at the end
            अगर (स्थिर DfgVertexVar* स्थिर varp = vtx.cast<DfgVertexVar>()) {
                अगर (!varp->has_पूर्व()) m_ctx.m_deleteps.धकेलो_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            अगर (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                वापस;
            }

            // Consider eliminating variables
            DfgVertexVar* स्थिर varp = vtx.cast<DfgVertexVar>();
            अगर (!varp) वापस;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* स्थिर srcp = varp->srcp();
            अगर (!srcp) वापस;

            // Can't eliminate if referenced external to the module - can't replace those refs
            अगर (varp->hasExtRefs()) वापस;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            अगर (varp->hasModWrRefs()) वापस;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            अगर (varp->has_पूर्व()) वापस;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            अगर (varp->defaultp()) वापस;
            अगर (srcp->is<DfgVertexSplice>()) वापस;
            अगर (srcp->is<DfgUnitArray>()) वापस;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            अगर (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) वापस;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            वापस;
        });
    }

    व्योम जोड़ो_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        क्रम (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            अगर (vtx.is<DfgVertexSplice>()) जारी;
            अगर (vtx.is<DfgUnitArray>()) जारी;
            // Add to map
            स्थिर uint32_t fanout = vtx.fanout();
            अगर (!fanout) जारी;
            अगर (fanout >= fanout2Vtxps.आकार()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].धकेलो_back(&vtx);
        }
        अगर (fanout2Vtxps.empty()) वापस;

        // Ensure intermediate values used multiple times are written to variables
        क्रम (माप_प्रकार fanout = fanout2Vtxps.आकार() - 1; fanout > 0; --fanout) {
            क्रम (DfgVertex* स्थिर vtxp : fanout2Vtxps[fanout]) {
                अगर (!needsTemporary(*vtxp, *vtxp)) जारी;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_पंक्ति* स्थिर flp = vtxp->fileline();
                AstScope* स्थिर scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* स्थिर newp
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
        अगर (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        अगर (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        जोड़ो_Temporaries();
        अगर (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

खुला:
    अटल व्योम apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

व्योम V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
