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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം DfgRegularize final {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    ശൂന്യം uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        വേണ്ടി (DfgVertex& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            എങ്കിൽ (vtx.is<DfgVertexSplice>()) തുടരുക;
            എങ്കിൽ (vtx.is<DfgUnitArray>()) തുടരുക;

            // The prefered result variable is the canonical one if exists
            DfgVertexVar* സ്ഥിരം varp = vtx.getResultVar();
            എങ്കിൽ (!varp) തുടരുക;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(nullptr);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    സ്ഥിതം തർക്കം isUnused(സ്ഥിരം DfgVertex& vtx) {
        എങ്കിൽ (vtx.hasSinks()) തിരികെ അസത്യം;
        എങ്കിൽ (സ്ഥിരം DfgVertexVar* സ്ഥിരം varp = vtx.cast<DfgVertexVar>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            എങ്കിൽ (varp->hasModWrRefs()) തിരികെ അസത്യം;
            എങ്കിൽ (varp->hasExtRefs()) തിരികെ അസത്യം;
            എങ്കിൽ (varp->hasPrev()) തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    തർക്കം needsTemporary(DfgVertex& aVtx, DfgVertex& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.isCheaperThanLoad() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<DfgVertexVar>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        എങ്കിൽ (aVtx.is<DfgPrev>()) തിരികെ അസത്യം;

        എങ്കിൽ (bVtx.hasMultipleSinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            എങ്കിൽ (!aVtx.isCheaperThanLoad()) തിരികെ സത്യം;

            // Not adding temporary
            തിരികെ അസത്യം;
        }

        DfgVertex& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        എങ്കിൽ (sink.is<DfgVertexVar>()) തിരികെ അസത്യം;

        // CReset always needs to be driving a variable
        എങ്കിൽ (aVtx.is<DfgCReset>()) തിരികെ സത്യം;

        // Do not inline expressions into a loop body
        എങ്കിൽ (സ്ഥിരം DfgAstRd* സ്ഥിരം astRdp = sink.cast<DfgAstRd>()) { തിരികെ astRdp->inLoop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        എങ്കിൽ (aVtx.is<DfgConcat>()) {
            എങ്കിൽ (sink.is<DfgConcat>()) തിരികെ അസത്യം;  // Not root of tree
            തിരികെ VL_വാക്കുകൾ_I(static_cast<പൂർണ്ണസംഖ്യ>(aVtx.width())) > v3Global.opt.expandLimit();
        }

        // No need for a temporary otherwise
        തിരികെ അസത്യം;
    }

    ശൂന്യം eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.forEachVertex([&](DfgVertex& vtx) {
            എങ്കിൽ (vtx.is<DfgVertexAst>()) തിരികെ;
            എങ്കിൽ (vtx.is<DfgVertexVar>() || !vtx.hasSinks()) workList.തള്ളുക_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        സ്ഥിരം auto removeVertex = [&](DfgVertex& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreachSource([&](DfgVertex& src) {
                workList.തള്ളുക_front(src);
                തിരികെ അസത്യം;
            });
            // Delete corresponsing Ast variable at the end
            എങ്കിൽ (സ്ഥിരം DfgVertexVar* സ്ഥിരം varp = vtx.cast<DfgVertexVar>()) {
                എങ്കിൽ (!varp->hasPrev()) m_ctx.m_deleteps.തള്ളുക_back(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](DfgVertex& vtx) {
            // Remove unused vertices
            എങ്കിൽ (isUnused(vtx)) {
                ++m_ctx.m_unusedRemoved;
                removeVertex(vtx);
                തിരികെ;
            }

            // Consider eliminating variables
            DfgVertexVar* സ്ഥിരം varp = vtx.cast<DfgVertexVar>();
            എങ്കിൽ (!varp) തിരികെ;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            DfgVertex* സ്ഥിരം srcp = varp->srcp();
            എങ്കിൽ (!srcp) തിരികെ;

            // Can't eliminate if referenced external to the module - can't replace those refs
            എങ്കിൽ (varp->hasExtRefs()) തിരികെ;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            എങ്കിൽ (varp->hasModWrRefs()) തിരികെ;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            എങ്കിൽ (varp->hasPrev()) തിരികെ;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            എങ്കിൽ (varp->defaultp()) തിരികെ;
            എങ്കിൽ (srcp->is<DfgVertexSplice>()) തിരികെ;
            എങ്കിൽ (srcp->is<DfgUnitArray>()) തിരികെ;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            എങ്കിൽ (!srcp->is<DfgVertexVar>() && needsTemporary(*srcp, *varp)) തിരികെ;

            // Inline this variable into its single sink
            ++m_ctx.m_usedVarsInlined;
            varp->replaceWith(varp->srcp());
            removeVertex(*varp);
            തിരികെ;
        });
    }

    ശൂന്യം തിരുകുക_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        DfgVertex::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::vector<std::vector<DfgVertex*>> fanout2Vtxps;
        വേണ്ടി (DfgVertex& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            എങ്കിൽ (vtx.is<DfgVertexSplice>()) തുടരുക;
            എങ്കിൽ (vtx.is<DfgUnitArray>()) തുടരുക;
            // Add to map
            സ്ഥിരം uint32_t fanout = vtx.fanout();
            എങ്കിൽ (!fanout) തുടരുക;
            എങ്കിൽ (fanout >= fanout2Vtxps.size()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].തള്ളുക_back(&vtx);
        }
        എങ്കിൽ (fanout2Vtxps.empty()) തിരികെ;

        // Ensure intermediate values used multiple times are written to variables
        വേണ്ടി (size_t fanout = fanout2Vtxps.size() - 1; fanout > 0; --fanout) {
            വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : fanout2Vtxps[fanout]) {
                എങ്കിൽ (!needsTemporary(*vtxp, *vtxp)) തുടരുക;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_വരി* സ്ഥിരം flp = vtxp->fileline();
                AstScope* സ്ഥിരം scopep = vtxp->scopep(scopeCache);
                DfgVertexVar* സ്ഥിരം newp
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
        എങ്കിൽ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        എങ്കിൽ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        തിരുകുക_Temporaries();
        എങ്കിൽ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(DfgGraph& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

ശൂന്യം V3DfgPasses::regularize(DfgGraph& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
