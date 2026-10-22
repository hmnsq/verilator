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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

صنف DfgRegularize final {
    // STATE
    Dfg_گراف& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    فراغ uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        لكل (Dfg_راس& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            إذا (vtx.is<Dfg_راس_Splice>()) استمر;
            إذا (vtx.is<DfgUnitArray>()) استمر;

            // The prefered result variable is the canonical one if exists
            Dfg_راس_Var* ثابت varp = vtx.get_نتيجو_Var();
            إذا (!varp) استمر;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(خالي_اشارو);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    ساكن منطقي is_اڻ_استعمال_ٿيل(ثابت Dfg_راس& vtx) {
        إذا (vtx.hasSinks()) أرجع خطأ;
        إذا (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            إذا (varp->hasModWrRefs()) أرجع خطأ;
            إذا (varp->hasExtRefs()) أرجع خطأ;
            إذا (varp->hasPrev()) أرجع خطأ;
        }
        أرجع سچ;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    منطقي needsTemporary(Dfg_راس& aVtx, Dfg_راس& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.is_Cheaper_Than_ڀريو() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<Dfg_راس_Var>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        إذا (aVtx.is<DfgPrev>()) أرجع خطأ;

        إذا (bVtx.has_گڻو_Sinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            إذا (!aVtx.is_Cheaper_Than_ڀريو()) أرجع سچ;

            // Not adding temporary
            أرجع خطأ;
        }

        Dfg_راس& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        إذا (sink.is<Dfg_راس_Var>()) أرجع خطأ;

        // CReset always needs to be driving a variable
        إذا (aVtx.is<DfgCReset>()) أرجع سچ;

        // Do not inline expressions into a loop body
        إذا (ثابت DfgAstRd* ثابت astRdp = sink.cast<DfgAstRd>()) { أرجع astRdp->اندريون_Loop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        إذا (aVtx.is<DfgConcat>()) {
            إذا (sink.is<DfgConcat>()) أرجع خطأ;  // Not root of tree
            أرجع VL_لفظ_I(static_cast<صحيح>(aVtx.ويڪر())) > v3Global.opt.expand_حد_بندي();
        }

        // No need for a temporary otherwise
        أرجع خطأ;
    }

    فراغ eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.for_Each_راس([&](Dfg_راس& vtx) {
            إذا (vtx.is<Dfg_راس_Ast>()) أرجع;
            إذا (vtx.is<Dfg_راس_Var>() || !vtx.hasSinks()) workList.ڌڪو_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ثابت خودڪار remove_راس = [&](Dfg_راس& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreach_ذريعو([&](Dfg_راس& src) {
                workList.ڌڪو_front(src);
                أرجع خطأ;
            });
            // Delete corresponsing Ast variable at the end
            إذا (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
                إذا (!varp->hasPrev()) m_ctx.m_deleteps.ڌڪو_پٺ(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](Dfg_راس& vtx) {
            // Remove unused vertices
            إذا (is_اڻ_استعمال_ٿيل(vtx)) {
                ++m_ctx.m_اڻ_استعمال_ٿيل_Removed;
                remove_راس(vtx);
                أرجع;
            }

            // Consider eliminating variables
            Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>();
            إذا (!varp) أرجع;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            Dfg_راس* ثابت srcp = varp->srcp();
            إذا (!srcp) أرجع;

            // Can't eliminate if referenced external to the module - can't replace those refs
            إذا (varp->hasExtRefs()) أرجع;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            إذا (varp->hasModWrRefs()) أرجع;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            إذا (varp->hasPrev()) أرجع;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            إذا (varp->defaultp()) أرجع;
            إذا (srcp->is<Dfg_راس_Splice>()) أرجع;
            إذا (srcp->is<DfgUnitArray>()) أرجع;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            إذا (!srcp->is<Dfg_راس_Var>() && needsTemporary(*srcp, *varp)) أرجع;

            // Inline this variable into its single sink
            ++m_ctx.m_استعمال_ٿيل_Vars_Inlined;
            varp->replaceWith(varp->srcp());
            remove_راس(*varp);
            أرجع;
        });
    }

    فراغ وجهو_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        Dfg_راس::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::ويڪٽر<std::ويڪٽر<Dfg_راس*>> fanout2Vtxps;
        لكل (Dfg_راس& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            إذا (vtx.is<Dfg_راس_Splice>()) استمر;
            إذا (vtx.is<DfgUnitArray>()) استمر;
            // Add to map
            ثابت uint32_t fanout = vtx.fanout();
            إذا (!fanout) استمر;
            إذا (fanout >= fanout2Vtxps.سائيز()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ڌڪو_پٺ(&vtx);
        }
        إذا (fanout2Vtxps.خالي_جايون()) أرجع;

        // Ensure intermediate values used multiple times are written to variables
        لكل (سائيز_t fanout = fanout2Vtxps.سائيز() - 1; fanout > 0; --fanout) {
            لكل (Dfg_راس* ثابت vtxp : fanout2Vtxps[fanout]) {
                إذا (!needsTemporary(*vtxp, *vtxp)) استمر;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_سٽ* ثابت flp = vtxp->fileline();
                AstScope* ثابت scopep = vtxp->scopep(scopeCache);
                Dfg_راس_Var* ثابت newp
                    = m_dfg.makeNewVar(flp, "Regularize", vtxp->dtype(), scopep);
                // Replace vertex with the variable, make it drive the variable
                vtxp->replaceWith(newp);
                newp->srcp(vtxp);
            }
        }
    }

    // Insert intermediate variables for vertices with multiple sinks (or use an existing one)
    DfgRegularize(Dfg_گراف& dfg, V3DfgRegularizeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        uninlineVariables();
        إذا (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        إذا (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        وجهو_Temporaries();
        إذا (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

فراغ V3DfgPasses::regularize(Dfg_گراف& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
