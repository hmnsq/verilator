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
    Dfg_ನಕ್ಷೆ& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    ಶೂನ್ಯ uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            ಆದರೆ (vtx.is<Dfg_ಶೃಂಗ_Splice>()) ಮುಂದುವರಿ;
            ಆದರೆ (vtx.is<DfgUnitArray>()) ಮುಂದುವರಿ;

            // The prefered result variable is the canonical one if exists
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = vtx.get_ಫಲಿತಾಂಶ_Var();
            ಆದರೆ (!varp) ಮುಂದುವರಿ;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(ಶೂನ್ಯಸೂಚಕ);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_ಬಳಸದ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
        ಆದರೆ (vtx.hasSinks()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = vtx.cast<Dfg_ಶೃಂಗ_Var>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            ಆದರೆ (varp->hasModWrRefs()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (varp->hasExtRefs()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (varp->hasPrev()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    ತಾರ್ಕಿಕ needsTemporary(Dfg_ಶೃಂಗ& aVtx, Dfg_ಶೃಂಗ& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.is_Cheaper_Than_ತುಂಬು() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<Dfg_ಶೃಂಗ_Var>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        ಆದರೆ (aVtx.is<DfgPrev>()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಆದರೆ (bVtx.has_ಅಪವರ್ತ್ಯ_Sinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            ಆದರೆ (!aVtx.is_Cheaper_Than_ತುಂಬು()) ಹಿಂತಿರುಗಿ ನಿಜ;

            // Not adding temporary
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        Dfg_ಶೃಂಗ& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        ಆದರೆ (sink.is<Dfg_ಶೃಂಗ_Var>()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // CReset always needs to be driving a variable
        ಆದರೆ (aVtx.is<DfgCReset>()) ಹಿಂತಿರುಗಿ ನಿಜ;

        // Do not inline expressions into a loop body
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgAstRd* ಸ್ಥಿರಾಂಕ astRdp = sink.cast<DfgAstRd>()) { ಹಿಂತಿರುಗಿ astRdp->ಒಳ_Loop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        ಆದರೆ (aVtx.is<DfgConcat>()) {
            ಆದರೆ (sink.is<DfgConcat>()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Not root of tree
            ಹಿಂತಿರುಗಿ VL_ಪದಗಳು_I(static_cast<ಪೂರ್ಣಾಂಕ>(aVtx.ಅಗಲ())) > v3Global.opt.expand_ಎಲ್ಲೆ();
        }

        // No need for a temporary otherwise
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.for_Each_ಶೃಂಗ([&](Dfg_ಶೃಂಗ& vtx) {
            ಆದರೆ (vtx.is<Dfg_ಶೃಂಗ_Ast>()) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (vtx.is<Dfg_ಶೃಂಗ_Var>() || !vtx.hasSinks()) workList.ತಳ್ಳು_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ remove_ಶೃಂಗ = [&](Dfg_ಶೃಂಗ& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
                workList.ತಳ್ಳು_front(src);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            // Delete corresponsing Ast variable at the end
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = vtx.cast<Dfg_ಶೃಂಗ_Var>()) {
                ಆದರೆ (!varp->hasPrev()) m_ctx.m_deleteps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](Dfg_ಶೃಂಗ& vtx) {
            // Remove unused vertices
            ಆದರೆ (is_ಬಳಸದ(vtx)) {
                ++m_ctx.m_ಬಳಸದ_Removed;
                remove_ಶೃಂಗ(vtx);
                ಹಿಂತಿರುಗಿ;
            }

            // Consider eliminating variables
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = vtx.cast<Dfg_ಶೃಂಗ_Var>();
            ಆದರೆ (!varp) ಹಿಂತಿರುಗಿ;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = varp->srcp();
            ಆದರೆ (!srcp) ಹಿಂತಿರುಗಿ;

            // Can't eliminate if referenced external to the module - can't replace those refs
            ಆದರೆ (varp->hasExtRefs()) ಹಿಂತಿರುಗಿ;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            ಆದರೆ (varp->hasModWrRefs()) ಹಿಂತಿರುಗಿ;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            ಆದರೆ (varp->hasPrev()) ಹಿಂತಿರುಗಿ;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            ಆದರೆ (varp->defaultp()) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (srcp->is<Dfg_ಶೃಂಗ_Splice>()) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (srcp->is<DfgUnitArray>()) ಹಿಂತಿರುಗಿ;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            ಆದರೆ (!srcp->is<Dfg_ಶೃಂಗ_Var>() && needsTemporary(*srcp, *varp)) ಹಿಂತಿರುಗಿ;

            // Inline this variable into its single sink
            ++m_ctx.m_ಬಳಸಿದ_Vars_Inlined;
            varp->replaceWith(varp->srcp());
            remove_ಶೃಂಗ(*varp);
            ಹಿಂತಿರುಗಿ;
        });
    }

    ಶೂನ್ಯ ಸೇರಿಸು_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        Dfg_ಶೃಂಗ::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::ಸದಿಶ<std::ಸದಿಶ<Dfg_ಶೃಂಗ*>> fanout2Vtxps;
        ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            ಆದರೆ (vtx.is<Dfg_ಶೃಂಗ_Splice>()) ಮುಂದುವರಿ;
            ಆದರೆ (vtx.is<DfgUnitArray>()) ಮುಂದುವರಿ;
            // Add to map
            ಸ್ಥಿರಾಂಕ uint32_t fanout = vtx.fanout();
            ಆದರೆ (!fanout) ಮುಂದುವರಿ;
            ಆದರೆ (fanout >= fanout2Vtxps.ಗಾತ್ರ_2()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);
        }
        ಆದರೆ (fanout2Vtxps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

        // Ensure intermediate values used multiple times are written to variables
        ಗಾಗಿ (ಗಾತ್ರ_t fanout = fanout2Vtxps.ಗಾತ್ರ_2() - 1; fanout > 0; --fanout) {
            ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : fanout2Vtxps[fanout]) {
                ಆದರೆ (!needsTemporary(*vtxp, *vtxp)) ಮುಂದುವರಿ;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
                AstScope* ಸ್ಥಿರಾಂಕ scopep = vtxp->scopep(scopeCache);
                Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ newp
                    = m_dfg.makeNewVar(flp, "Regularize", vtxp->dtype(), scopep);
                // Replace vertex with the variable, make it drive the variable
                vtxp->replaceWith(newp);
                newp->srcp(vtxp);
            }
        }
    }

    // Insert intermediate variables for vertices with multiple sinks (or use an existing one)
    DfgRegularize(Dfg_ನಕ್ಷೆ& dfg, V3DfgRegularizeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        uninlineVariables();
        ಆದರೆ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        ಆದರೆ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        ಸೇರಿಸು_Temporaries();
        ಆದರೆ (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Dfg_ನಕ್ಷೆ& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

ಶೂನ್ಯ V3DfgPasses::regularize(Dfg_ನಕ್ಷೆ& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
