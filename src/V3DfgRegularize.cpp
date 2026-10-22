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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి DfgRegularize final {
    // STATE
    Dfg_లేఖాచిత్రం& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    శూన్య uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        కొరకు (Dfg_శీర్షం& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            ఒకవేళ (vtx.is<Dfg_శీర్షం_Splice>()) కొనసాగు;
            ఒకవేళ (vtx.is<DfgUnitArray>()) కొనసాగు;

            // The prefered result variable is the canonical one if exists
            Dfg_శీర్షం_Var* స్థిరరాశి varp = vtx.get_ఫలితం_Var();
            ఒకవేళ (!varp) కొనసాగు;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(శూన్యసూచిక);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    స్థిర తార్కిక is_వాడని(స్థిరరాశి Dfg_శీర్షం& vtx) {
        ఒకవేళ (vtx.hasSinks()) తిరిగి అబద్ధం;
        ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = vtx.cast<Dfg_శీర్షం_Var>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            ఒకవేళ (varp->hasModWrRefs()) తిరిగి అబద్ధం;
            ఒకవేళ (varp->hasExtRefs()) తిరిగి అబద్ధం;
            ఒకవేళ (varp->hasPrev()) తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    తార్కిక needsTemporary(Dfg_శీర్షం& aVtx, Dfg_శీర్షం& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.is_Cheaper_Than_ఎక్కించు() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<Dfg_శీర్షం_Var>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        ఒకవేళ (aVtx.is<DfgPrev>()) తిరిగి అబద్ధం;

        ఒకవేళ (bVtx.has_గుణిజం_Sinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            ఒకవేళ (!aVtx.is_Cheaper_Than_ఎక్కించు()) తిరిగి నిజం;

            // Not adding temporary
            తిరిగి అబద్ధం;
        }

        Dfg_శీర్షం& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        ఒకవేళ (sink.is<Dfg_శీర్షం_Var>()) తిరిగి అబద్ధం;

        // CReset always needs to be driving a variable
        ఒకవేళ (aVtx.is<DfgCReset>()) తిరిగి నిజం;

        // Do not inline expressions into a loop body
        ఒకవేళ (స్థిరరాశి DfgAstRd* స్థిరరాశి astRdp = sink.cast<DfgAstRd>()) { తిరిగి astRdp->లోపల_Loop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        ఒకవేళ (aVtx.is<DfgConcat>()) {
            ఒకవేళ (sink.is<DfgConcat>()) తిరిగి అబద్ధం;  // Not root of tree
            తిరిగి VL_పదాలు_I(static_cast<పూర్ణాంకం>(aVtx.వెడల్పు())) > v3Global.opt.expand_హద్దు();
        }

        // No need for a temporary otherwise
        తిరిగి అబద్ధం;
    }

    శూన్య eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.for_Each_శీర్షం([&](Dfg_శీర్షం& vtx) {
            ఒకవేళ (vtx.is<Dfg_శీర్షం_Ast>()) తిరిగి;
            ఒకవేళ (vtx.is<Dfg_శీర్షం_Var>() || !vtx.hasSinks()) workList.నెట్టు_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        స్థిరరాశి స్వయంచాలక remove_శీర్షం = [&](Dfg_శీర్షం& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreach_ఆధారం([&](Dfg_శీర్షం& src) {
                workList.నెట్టు_front(src);
                తిరిగి అబద్ధం;
            });
            // Delete corresponsing Ast variable at the end
            ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = vtx.cast<Dfg_శీర్షం_Var>()) {
                ఒకవేళ (!varp->hasPrev()) m_ctx.m_deleteps.నెట్టు_వెనుక(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](Dfg_శీర్షం& vtx) {
            // Remove unused vertices
            ఒకవేళ (is_వాడని(vtx)) {
                ++m_ctx.m_వాడని_Removed;
                remove_శీర్షం(vtx);
                తిరిగి;
            }

            // Consider eliminating variables
            Dfg_శీర్షం_Var* స్థిరరాశి varp = vtx.cast<Dfg_శీర్షం_Var>();
            ఒకవేళ (!varp) తిరిగి;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            Dfg_శీర్షం* స్థిరరాశి srcp = varp->srcp();
            ఒకవేళ (!srcp) తిరిగి;

            // Can't eliminate if referenced external to the module - can't replace those refs
            ఒకవేళ (varp->hasExtRefs()) తిరిగి;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            ఒకవేళ (varp->hasModWrRefs()) తిరిగి;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            ఒకవేళ (varp->hasPrev()) తిరిగి;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            ఒకవేళ (varp->defaultp()) తిరిగి;
            ఒకవేళ (srcp->is<Dfg_శీర్షం_Splice>()) తిరిగి;
            ఒకవేళ (srcp->is<DfgUnitArray>()) తిరిగి;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            ఒకవేళ (!srcp->is<Dfg_శీర్షం_Var>() && needsTemporary(*srcp, *varp)) తిరిగి;

            // Inline this variable into its single sink
            ++m_ctx.m_వాడిన_Vars_Inlined;
            varp->replaceWith(varp->srcp());
            remove_శీర్షం(*varp);
            తిరిగి;
        });
    }

    శూన్య చొప్పించు_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        Dfg_శీర్షం::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::సదిశ<std::సదిశ<Dfg_శీర్షం*>> fanout2Vtxps;
        కొరకు (Dfg_శీర్షం& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            ఒకవేళ (vtx.is<Dfg_శీర్షం_Splice>()) కొనసాగు;
            ఒకవేళ (vtx.is<DfgUnitArray>()) కొనసాగు;
            // Add to map
            స్థిరరాశి uint32_t fanout = vtx.fanout();
            ఒకవేళ (!fanout) కొనసాగు;
            ఒకవేళ (fanout >= fanout2Vtxps.కొలత()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].నెట్టు_వెనుక(&vtx);
        }
        ఒకవేళ (fanout2Vtxps.ఖాళీలు()) తిరిగి;

        // Ensure intermediate values used multiple times are written to variables
        కొరకు (కొలత_t fanout = fanout2Vtxps.కొలత() - 1; fanout > 0; --fanout) {
            కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : fanout2Vtxps[fanout]) {
                ఒకవేళ (!needsTemporary(*vtxp, *vtxp)) కొనసాగు;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
                AstScope* స్థిరరాశి scopep = vtxp->scopep(scopeCache);
                Dfg_శీర్షం_Var* స్థిరరాశి newp
                    = m_dfg.makeNewVar(flp, "Regularize", vtxp->dtype(), scopep);
                // Replace vertex with the variable, make it drive the variable
                vtxp->replaceWith(newp);
                newp->srcp(vtxp);
            }
        }
    }

    // Insert intermediate variables for vertices with multiple sinks (or use an existing one)
    DfgRegularize(Dfg_లేఖాచిత్రం& dfg, V3DfgRegularizeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        uninlineVariables();
        ఒకవేళ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        ఒకవేళ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        చొప్పించు_Temporaries();
        ఒకవేళ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

బహిరంగ:
    స్థిర శూన్య apply(Dfg_లేఖాచిత్రం& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

శూన్య V3DfgPasses::regularize(Dfg_లేఖాచిత్రం& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
