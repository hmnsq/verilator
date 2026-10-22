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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ DfgRegularize final {
    // STATE
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    ਖਾਲੀ uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            ਜੇ (vtx.is<Dfg_ਸ਼ੀਰਸ਼_Splice>()) ਜਾਰੀ;
            ਜੇ (vtx.is<DfgUnitArray>()) ਜਾਰੀ;

            // The prefered result variable is the canonical one if exists
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = vtx.get_ਨਤੀਜਾ_Var();
            ਜੇ (!varp) ਜਾਰੀ;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(ਖਾਲੀ_ਸੂਚਕ);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ is_ਅਣਵਰਤਿਆ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
        ਜੇ (vtx.hasSinks()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = vtx.cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            ਜੇ (varp->hasModWrRefs()) ਵਾਪਸ ਝੂਠ;
            ਜੇ (varp->hasExtRefs()) ਵਾਪਸ ਝੂਠ;
            ਜੇ (varp->hasPrev()) ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    ਤਾਰਕਿਕ needsTemporary(Dfg_ਸ਼ੀਰਸ਼& aVtx, Dfg_ਸ਼ੀਰਸ਼& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.is_Cheaper_Than_ਲੱਦੋ() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<Dfg_ਸ਼ੀਰਸ਼_Var>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        ਜੇ (aVtx.is<DfgPrev>()) ਵਾਪਸ ਝੂਠ;

        ਜੇ (bVtx.has_ਗੁਣਜ_Sinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            ਜੇ (!aVtx.is_Cheaper_Than_ਲੱਦੋ()) ਵਾਪਸ ਸੱਚ;

            // Not adding temporary
            ਵਾਪਸ ਝੂਠ;
        }

        Dfg_ਸ਼ੀਰਸ਼& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        ਜੇ (sink.is<Dfg_ਸ਼ੀਰਸ਼_Var>()) ਵਾਪਸ ਝੂਠ;

        // CReset always needs to be driving a variable
        ਜੇ (aVtx.is<DfgCReset>()) ਵਾਪਸ ਸੱਚ;

        // Do not inline expressions into a loop body
        ਜੇ (ਅਚਲ DfgAstRd* ਅਚਲ astRdp = sink.cast<DfgAstRd>()) { ਵਾਪਸ astRdp->ਵਿੱਚ_Loop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        ਜੇ (aVtx.is<DfgConcat>()) {
            ਜੇ (sink.is<DfgConcat>()) ਵਾਪਸ ਝੂਠ;  // Not root of tree
            ਵਾਪਸ VL_ਸ਼ਬਦ_I(static_cast<ਪੂਰਨ>(aVtx.ਚੌੜਾਈ())) > v3Global.opt.expand_ਹੱਦਬੰਦੀ();
        }

        // No need for a temporary otherwise
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.for_Each_ਸ਼ੀਰਸ਼([&](Dfg_ਸ਼ੀਰਸ਼& vtx) {
            ਜੇ (vtx.is<Dfg_ਸ਼ੀਰਸ਼_Ast>()) ਵਾਪਸ;
            ਜੇ (vtx.is<Dfg_ਸ਼ੀਰਸ਼_Var>() || !vtx.hasSinks()) workList.ਧੱਕੋ_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ਅਚਲ ਸਵੈਚਾਲਿਤ remove_ਸ਼ੀਰਸ਼ = [&](Dfg_ਸ਼ੀਰਸ਼& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
                workList.ਧੱਕੋ_front(src);
                ਵਾਪਸ ਝੂਠ;
            });
            // Delete corresponsing Ast variable at the end
            ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = vtx.cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
                ਜੇ (!varp->hasPrev()) m_ctx.m_deleteps.ਧੱਕੋ_ਪਿੱਠ(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](Dfg_ਸ਼ੀਰਸ਼& vtx) {
            // Remove unused vertices
            ਜੇ (is_ਅਣਵਰਤਿਆ(vtx)) {
                ++m_ctx.m_ਅਣਵਰਤਿਆ_Removed;
                remove_ਸ਼ੀਰਸ਼(vtx);
                ਵਾਪਸ;
            }

            // Consider eliminating variables
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = vtx.cast<Dfg_ਸ਼ੀਰਸ਼_Var>();
            ਜੇ (!varp) ਵਾਪਸ;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = varp->srcp();
            ਜੇ (!srcp) ਵਾਪਸ;

            // Can't eliminate if referenced external to the module - can't replace those refs
            ਜੇ (varp->hasExtRefs()) ਵਾਪਸ;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            ਜੇ (varp->hasModWrRefs()) ਵਾਪਸ;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            ਜੇ (varp->hasPrev()) ਵਾਪਸ;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            ਜੇ (varp->defaultp()) ਵਾਪਸ;
            ਜੇ (srcp->is<Dfg_ਸ਼ੀਰਸ਼_Splice>()) ਵਾਪਸ;
            ਜੇ (srcp->is<DfgUnitArray>()) ਵਾਪਸ;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            ਜੇ (!srcp->is<Dfg_ਸ਼ੀਰਸ਼_Var>() && needsTemporary(*srcp, *varp)) ਵਾਪਸ;

            // Inline this variable into its single sink
            ++m_ctx.m_ਵਰਤਿਆ_Vars_Inlined;
            varp->replaceWith(varp->srcp());
            remove_ਸ਼ੀਰਸ਼(*varp);
            ਵਾਪਸ;
        });
    }

    ਖਾਲੀ ਪਾਓ_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        Dfg_ਸ਼ੀਰਸ਼::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*>> fanout2Vtxps;
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            ਜੇ (vtx.is<Dfg_ਸ਼ੀਰਸ਼_Splice>()) ਜਾਰੀ;
            ਜੇ (vtx.is<DfgUnitArray>()) ਜਾਰੀ;
            // Add to map
            ਅਚਲ uint32_t fanout = vtx.fanout();
            ਜੇ (!fanout) ਜਾਰੀ;
            ਜੇ (fanout >= fanout2Vtxps.ਮਾਪ()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ਧੱਕੋ_ਪਿੱਠ(&vtx);
        }
        ਜੇ (fanout2Vtxps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

        // Ensure intermediate values used multiple times are written to variables
        ਲਈ (ਮਾਪ_t fanout = fanout2Vtxps.ਮਾਪ() - 1; fanout > 0; --fanout) {
            ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : fanout2Vtxps[fanout]) {
                ਜੇ (!needsTemporary(*vtxp, *vtxp)) ਜਾਰੀ;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
                AstScope* ਅਚਲ scopep = vtxp->scopep(scopeCache);
                Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ newp
                    = m_dfg.makeNewVar(flp, "Regularize", vtxp->dtype(), scopep);
                // Replace vertex with the variable, make it drive the variable
                vtxp->replaceWith(newp);
                newp->srcp(vtxp);
            }
        }
    }

    // Insert intermediate variables for vertices with multiple sinks (or use an existing one)
    DfgRegularize(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgRegularizeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        uninlineVariables();
        ਜੇ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        ਜੇ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        ਪਾਓ_Temporaries();
        ਜੇ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

ਖਾਲੀ V3DfgPasses::regularize(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
