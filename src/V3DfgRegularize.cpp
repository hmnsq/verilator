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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت DfgRegularize final {
    // STATE
    Dfg_گراف& m_dfg;  // The graph being processed
    V3DfgRegularizeContext& m_ctx;  // The optimization context for stats
    VNDeleter m_deleter;  // Deletes replacement nodes at the end

    // METHODS

    // For all operation vetices, if they drive multiple variables, pick
    // a "canonical" one and uninline the logic through that variable.
    خالی uninlineVariables() {
        // Variable vertices, would have been inlined if equivalent,
        // so no need to process them here, they are where they must be.
        برائے (Dfg_راس& vtx : m_dfg.opVertices()) {
            // Don't process LValue operations
            اگر (vtx.is<Dfg_راس_Splice>()) جاری;
            اگر (vtx.is<DfgUnitArray>()) جاری;

            // The prefered result variable is the canonical one if exists
            Dfg_راس_Var* ثابت varp = vtx.get_نتیجہ_Var();
            اگر (!varp) جاری;

            // Relink all other sinks reading this vertex to read 'varp'
            varp->srcp(خالی_اشاریہ);
            vtx.replaceWith(varp);
            varp->srcp(&vtx);
        }
    }

    ساکن منطقی is_غیر_مستعمل(ثابت Dfg_راس& vtx) {
        اگر (vtx.hasSinks()) واپس جھوٹ;
        اگر (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");
            اگر (varp->hasModWrRefs()) واپس جھوٹ;
            اگر (varp->hasExtRefs()) واپس جھوٹ;
            اگر (varp->has_پچھلا()) واپس جھوٹ;
        }
        واپس سچ;
    }

    // Predicate to determine if a temporary should be inserted or if a variable
    // should be preserved. The given vertices are either the same, or aVtxp is
    // the sole driver of bVtx, or aVtxp is cheaper to recompute and might have
    // multiple sinks. In either case, bVtx can be used to check sinks, and aVtx
    // can be used to check the operation.
    منطقی needsTemporary(Dfg_راس& aVtx, Dfg_راس& bVtx) {
        UASSERT_OBJ(&aVtx == &bVtx || aVtx.is_Cheaper_Than_بھرو() || aVtx.singleSink() == &bVtx,
                    &aVtx, "Mismatched vertices");
        UASSERT_OBJ(!aVtx.is<Dfg_راس_Var>(), &aVtx, "Should be an operation vertex");

        // Prev is just a variable reference
        اگر (aVtx.is<Dfg_پچھلا>()) واپس جھوٹ;

        اگر (bVtx.has_مضرب_Sinks()) {
            // Add a temporary if it's cheaper to store and load from memory than recompute
            اگر (!aVtx.is_Cheaper_Than_بھرو()) واپس سچ;

            // Not adding temporary
            واپس جھوٹ;
        }

        Dfg_راس& sink = *bVtx.singleSink();

        // No need to add a temporary if the single sink is a variable already
        اگر (sink.is<Dfg_راس_Var>()) واپس جھوٹ;

        // CReset always needs to be driving a variable
        اگر (aVtx.is<DfgCReset>()) واپس سچ;

        // Do not inline expressions into a loop body
        اگر (ثابت DfgAstRd* ثابت astRdp = sink.cast<DfgAstRd>()) { واپس astRdp->اندر_Loop(); }

        // Make sure roots of wide concatenation trees are written to variables,
        // this enables V3FuncOpt to split them which can be a big speed gain
        // without expanding them.
        اگر (aVtx.is<DfgConcat>()) {
            اگر (sink.is<DfgConcat>()) واپس جھوٹ;  // Not root of tree
            واپس VL_الفاظ_I(static_cast<صحیح>(aVtx.چوڑائی())) > v3Global.opt.expand_حد_بندی();
        }

        // No need for a temporary otherwise
        واپس جھوٹ;
    }

    خالی eliminateVars() {
        // Worklist based algoritm
        DfgWorklist workList{m_dfg};

        // Add all variables and all vertices with no sinks to the worklist
        m_dfg.for_Each_راس([&](Dfg_راس& vtx) {
            اگر (vtx.is<Dfg_راس_Ast>()) واپس;
            اگر (vtx.is<Dfg_راس_Var>() || !vtx.hasSinks()) workList.ڈالو_front(vtx);
        });

        // Remove vertex, enqueue it's sources
        ثابت خودکار remove_راس = [&](Dfg_راس& vtx) {
            // Add sources of removed vertex to work list
            vtx.foreach_ماخذ([&](Dfg_راس& src) {
                workList.ڈالو_front(src);
                واپس جھوٹ;
            });
            // Delete corresponsing Ast variable at the end
            اگر (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
                اگر (!varp->has_پچھلا()) m_ctx.m_deleteps.ڈالو_عقب(varp->vscp());
            }
            // Remove the unused vertex
            vtx.unlinkDelete(m_dfg);
        };

        // Process the work list
        workList.foreach([&](Dfg_راس& vtx) {
            // Remove unused vertices
            اگر (is_غیر_مستعمل(vtx)) {
                ++m_ctx.m_غیر_مستعمل_Removed;
                remove_راس(vtx);
                واپس;
            }

            // Consider eliminating variables
            Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>();
            اگر (!varp) واپس;

            // If it has no driver (in this Dfg), there is nothing further we can optimize
            Dfg_راس* ثابت srcp = varp->srcp();
            اگر (!srcp) واپس;

            // Can't eliminate if referenced external to the module - can't replace those refs
            اگر (varp->hasExtRefs()) واپس;
            // Can't eliminate if written in the module - the write needs to go somewhere, and
            // we need to observe the write in this graph if the variable has sinks
            اگر (varp->hasModWrRefs()) واپس;
            // There is only one Dfg when running this pass
            UASSERT_OBJ(!varp->hasDfgRefs(), varp, "Should not have refs in other DfgGraph");

            // Do not eliminate circular variables - need to preserve UNOPTFLAT traces
            اگر (varp->has_پچھلا()) واپس;

            // Do not inline if partially driven (the partial driver network can't be fed into
            // arbitrary logic. TODO: we should peeophole these away entirely)
            اگر (varp->defaultp()) واپس;
            اگر (srcp->is<Dfg_راس_Splice>()) واپس;
            اگر (srcp->is<DfgUnitArray>()) واپس;

            // Do not eliminate variables that are driven from a vertex that needs a temporary
            اگر (!srcp->is<Dfg_راس_Var>() && needsTemporary(*srcp, *varp)) واپس;

            // Inline this variable into its single sink
            ++m_ctx.m_مستعمل_Vars_Inlined;
            varp->replaceWith(varp->srcp());
            remove_راس(*varp);
            واپس;
        });
    }

    خالی داخل_کرو_Temporaries() {
        // Insert a temporary variable for all vertices that have multiple non-variable sinks

        // Scope cache for below
        Dfg_راس::ScopeCache scopeCache;

        // Build map from fanout to list of vertices with that fanout
        std::سمتیہ<std::سمتیہ<Dfg_راس*>> fanout2Vtxps;
        برائے (Dfg_راس& vtx : m_dfg.opVertices()) {
            // LValue vertices feed into variables eventually and need no temporaries
            اگر (vtx.is<Dfg_راس_Splice>()) جاری;
            اگر (vtx.is<DfgUnitArray>()) جاری;
            // Add to map
            ثابت uint32_t fanout = vtx.fanout();
            اگر (!fanout) جاری;
            اگر (fanout >= fanout2Vtxps.سائز()) fanout2Vtxps.resize(2 * fanout);
            fanout2Vtxps[fanout].ڈالو_عقب(&vtx);
        }
        اگر (fanout2Vtxps.خالی_خانے()) واپس;

        // Ensure intermediate values used multiple times are written to variables
        برائے (سائز_t fanout = fanout2Vtxps.سائز() - 1; fanout > 0; --fanout) {
            برائے (Dfg_راس* ثابت vtxp : fanout2Vtxps[fanout]) {
                اگر (!needsTemporary(*vtxp, *vtxp)) جاری;
                // Need to create an intermediate variable
                ++m_ctx.m_temporariesIntroduced;
                File_سطر* ثابت flp = vtxp->fileline();
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
        اگر (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-uninlined");

        eliminateVars();
        اگر (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-eliminate");

        داخل_کرو_Temporaries();
        اگر (dumpDfgLevel() >= 9) dfg.dumpDotFilePrefixed("regularize-inserttmp");
    }

عوامی:
    ساکن خالی apply(Dfg_گراف& dfg, V3DfgRegularizeContext& ctx) { DfgRegularize{dfg, ctx}; }
};

خالی V3DfgPasses::regularize(Dfg_گراف& dfg, V3DfgRegularizeContext& ctx) {
    DfgRegularize::apply(dfg, ctx);
}
