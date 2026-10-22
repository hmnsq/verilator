// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Implementations of simple passes over DfgGraph
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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਖਾਲੀ V3DfgPasses::removeUnobservable(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : dfg.opVertices()) {
        ਜੇ (vtx.is<DfgLogic>()) workList.ਧੱਕੋ_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](Dfg_ਸ਼ੀਰਸ਼& vtx) {
        DfgLogic* ਅਚਲ logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ਅਚਲ ਤਾਰਕਿਕ ਵਰਤਿਆ = logicp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& snk) {
            DfgUnresolved* ਅਚਲ uVtxp = snk.as<DfgUnresolved>();
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vVtxp = uVtxp->firtsSinkp()->as<Dfg_ਸ਼ੀਰਸ਼_Var>();
            ਜੇ (vVtxp->hasSinks()) ਵਾਪਸ ਸੱਚ;
            ਜੇ (vVtxp->isObserved()) ਵਾਪਸ ਸੱਚ;
            ਵਾਪਸ ਝੂਠ;
        });
        // If some are used, the logic must stay in the Ast
        ਜੇ (ਵਰਤਿਆ) ਵਾਪਸ;
        // If impure, it must stay in the Ast
        ਜੇ (!logicp->is_ਸ਼ੁੱਧ()) ਵਾਪਸ;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = src.as<Dfg_ਸ਼ੀਰਸ਼_Var>();
            ਜੇ (!varp->srcp()) ਵਾਪਸ ਝੂਠ;
            varp->srcp()->as<DfgUnresolved>()->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& driver) {
                workList.ਧੱਕੋ_front(*driver.as<DfgLogic>());
                ਵਾਪਸ ਝੂਠ;
            });
            ਵਾਪਸ ਝੂਠ;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_ਗੰਢ* ਅਚਲ nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vVtxp : dfg.varVertices().unlinkable()) {
        ਜੇ (vVtxp->hasSinks()) ਜਾਰੀ;
        ਜੇ (vVtxp->isObserved()) ਜਾਰੀ;
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ਅਚਲ vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ਅਚਲ ਤਾਰਕਿਕ delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        ਜੇ (srcp) {
            srcp->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
                src.as<DfgLogic>()->ਸੈੱਟ_Drives_ਅਣਵਰਤਿਆ_Vars();
                ਵਾਪਸ ਝੂਠ;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        ਜੇ (delAst) {
            VL_DO_DANGLING(vscp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), vscp);
            ++ctx.m_varsDeleted;
        } ਨਹੀਂਤਾਂ {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ਅਚਲ logicp = vtxp->cast<DfgLogic>();
        ਜੇ (!logicp) ਜਾਰੀ;
        ਜੇ (logicp->hasSinks()) ਜਾਰੀ;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;
        logicp->foreach_ਸਰੋਤ([](Dfg_ਸ਼ੀਰਸ਼& src) {
            src.as<Dfg_ਸ਼ੀਰਸ਼_Var>()->vscp()->user2p(&src);
            ਵਾਪਸ ਝੂਠ;
        });
        V3DfgPasses::ਜੋੜੋ_Ast_Refs(dfg, logicp->nodep(), [](Ast_ਗੰਢ* varp) {  //
            ਵਾਪਸ varp->user2u().ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼_Var*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

ਖਾਲੀ V3DfgPasses::removeSelects(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::ਸਦਿਸ਼<DfgSel*> selps;
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : dfg.opVertices()) {
        DfgSel* ਅਚਲ selp = vtx.cast<DfgSel>();
        ਜੇ (!selp) ਜਾਰੀ;
        selps.ਧੱਕੋ_ਪਿੱਠ(selp);
    }

    ਲਈ (DfgSel* ਅਚਲ selp : selps) {
        File_ਸਤਰ* ਅਚਲ flp = selp->fileline();
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = selp->dtype();

        // Remove full width selects
        ਜੇ (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removed_ਭਰਿਆ_ਚੌੜਾਈ;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ਜਾਰੀ;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ਅਚਲ varp = selp->fromp()->cast<DfgVarPacked>();
        ਜੇ (!varp || !varp->tmpForp()) ਜਾਰੀ;

        // Find the driver of this range
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = varp->driver_ਦਾ_Range(selp->lsb(), selp->ਚੌੜਾਈ());
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ driverp = pair.ਪਹਿਲਾ;
        ਅਚਲ uint32_t driverLsb = pair.ਦੂਜਾ;
        ਜੇ (!driverp) ਜਾਰੀ;

        // If partial driver is the whole thing we are looking for, just replace with that
        ਜੇ (driverp->dtype() == dtype) {
            ++ctx.m_replaced_With_ਸਾਰਾ_Driver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ਜਾਰੀ;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ਅਚਲ newSelp = ਨਵਾਂ DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

ਖਾਲੀ V3DfgPasses::inlineVars(Dfg_ਗ੍ਰਾਫ& dfg) {
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        ਜੇ (!vtx.hasSinks()) ਜਾਰੀ;
        // Nohting to inline
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtx.srcp();
        ਜੇ (!srcp) ਜਾਰੀ;
        // Value can differ from driver
        ਜੇ (vtx.isVolatile()) ਜਾਰੀ;
        // Partial driver cannot be inlined
        ਜੇ (srcp->is<Dfg_ਸ਼ੀਰਸ਼_Splice>()) ਜਾਰੀ;
        ਜੇ (srcp->is<DfgUnitArray>()) ਜਾਰੀ;
        // Don't inline CReset
        ਜੇ (srcp->is<DfgCReset>()) ਜਾਰੀ;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

ਖਾਲੀ V3DfgPasses::remove_ਅਣਵਰਤਿਆ(Dfg_ਗ੍ਰਾਫ& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : dfg.opVertices()) {
        ਜੇ (vtx.hasSinks()) ਜਾਰੀ;
        // This vertex is unused. Add to work list.
        workList.ਧੱਕੋ_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : dfg.varVertices()) {
        ਜੇ (!vtx.tmpForp()) ਜਾਰੀ;
        ਜੇ (vtx.hasSinks()) ਜਾਰੀ;
        ਜੇ (vtx.hasDfgRefs()) ਜਾਰੀ;
        // This vertex is unused. Add to work list.
        workList.ਧੱਕੋ_front(vtx);
    }

    // Process the work list
    workList.foreach([&](Dfg_ਸ਼ੀਰਸ਼& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        ਜੇ (vtx.hasSinks()) ਵਾਪਸ;
        // If temporary used in another graph, we need to keep it
        ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = vtx.cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            ਜੇ (varp->hasDfgRefs()) ਵਾਪਸ;
        }
        // Add sources of unused vertex to work list
        vtx.foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            ਜੇ (src.is<DfgConst>()) ਵਾਪਸ ਝੂਠ;
            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = src.cast<Dfg_ਸ਼ੀਰਸ਼_Var>();
            ਜੇ (varp && !varp->tmpForp()) ਵਾਪਸ ਝੂਠ;
            // Add source to workList
            workList.ਧੱਕੋ_front(src);
            ਵਾਪਸ ਝੂਠ;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp : dfg.varVertices().unlinkable()) {
        ਜੇ (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    ਲਈ (DfgConst* ਅਚਲ vtxp : dfg.constVertices().unlinkable()) {
        ਜੇ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

ਖਾਲੀ V3DfgPasses::bin_ਮੰਜ਼ਿਲ_One_Hot(Dfg_ਗ੍ਰਾਫ& dfg, V3_Dfg_Bin_ਮੰਜ਼ਿਲ_One_Hot_Context& ctx) {
    // Structure to keep track of comparison details
    ਬਣਤਰ ਪਦ_2 final {
        Dfg_ਸ਼ੀਰਸ਼* m_vtxp = ਖਾਲੀ_ਸੂਚਕ;  // Vertex to replace
        ਤਾਰਕਿਕ m_inv = ਝੂਠ;  // '!=', instead of '=='
        ਪਦ_2() = ਮੂਲ;
        ਪਦ_2(Dfg_ਸ਼ੀਰਸ਼* vtxp, ਤਾਰਕਿਕ inv)
            : m_vtxp{vtxp}
            , m_inv{inv} {}
    };

    // List of vertices that are used as sources
    std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> srcps;

    // Map from 'vertices' -> 'value beign compared' -> 'terms'
    ਵਰਤੋਂ Val2Terms = std::map<uint32_t, std::ਸਦਿਸ਼<ਪਦ_2>>;
    DfgUserMap<Val2Terms> vtx2Val2Terms = dfg.makeUserMap<Val2Terms>();

    // Only consider input variables from a reasonable range:
    // - not too big to avoid huge tables, you are doomed anyway at that point..
    // - not too small, as it's probably not worth it
    constexpr uint32_t ਚੌੜਾਈ_MIN = 7;
    constexpr uint32_t ਚੌੜਾਈ_MAX = 20;
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਚੌੜਾਈ_Ok = [](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਅਚਲ uint32_t ਚੌੜਾਈ = vtxp->ਚੌੜਾਈ();
        ਵਾਪਸ ਚੌੜਾਈ_MIN <= ਚੌੜਾਈ && ਚੌੜਾਈ <= ਚੌੜਾਈ_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ਅਚਲ std::function<ਤਾਰਕਿਕ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*, ਤਾਰਕਿਕ)> useOk
        = [&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* vtxp, ਤਾਰਕਿਕ inv) -> ਤਾਰਕਿਕ {
        // Go past a single 'Not' sink, which is common
        ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ sinkp = vtxp->singleSink()) {
            ਜੇ (sinkp->is<DfgNot>()) ਵਾਪਸ useOk(sinkp, !inv);
        }
        ਅਚਲ ਤਾਰਕਿਕ cond_ਰੁੱਖ = vtxp->foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& sink) {
            ਅਚਲ DfgCond* ਅਚਲ condp = sink.cast<DfgCond>();
            ਜੇ (!condp) ਵਾਪਸ ਝੂਠ;
            ਜੇ (condp->condp() != vtxp) ਵਾਪਸ ਝੂਠ;
            ਵਾਪਸ inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        ਵਾਪਸ !cond_ਰੁੱਖ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : dfg.opVertices()) {
        Dfg_ਸ਼ੀਰਸ਼* srcp = ਖਾਲੀ_ਸੂਚਕ;
        uint32_t val = 0;
        ਤਾਰਕਿਕ inv = ਝੂਠ;
        ਜੇ (DfgEq* ਅਚਲ eqp = vtx.cast<DfgEq>()) {
            DfgConst* ਅਚਲ constp = eqp->lhsp()->cast<DfgConst>();
            ਜੇ (!constp || !ਚੌੜਾਈ_Ok(constp) || !useOk(eqp, ਝੂਠ)) ਜਾਰੀ;
            srcp = eqp->rhsp();
            val = constp->ਮੰਜ਼ਿਲ_U32();
            inv = ਝੂਠ;
        } ਨਹੀਂਤਾਂ ਜੇ (DfgNeq* ਅਚਲ neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ਅਚਲ constp = neqp->lhsp()->cast<DfgConst>();
            ਜੇ (!constp || !ਚੌੜਾਈ_Ok(constp) || !useOk(neqp, ਸੱਚ)) ਜਾਰੀ;
            srcp = neqp->rhsp();
            val = constp->ਮੰਜ਼ਿਲ_U32();
            inv = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (DfgRedAnd* ਅਚਲ redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            ਜੇ (!ਚੌੜਾਈ_Ok(srcp) || !useOk(redAndp, ਝੂਠ)) ਜਾਰੀ;
            val = (1U << srcp->ਚੌੜਾਈ()) - 1;
            inv = ਝੂਠ;
        } ਨਹੀਂਤਾਂ ਜੇ (DfgRedOr* ਅਚਲ redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            ਜੇ (!ਚੌੜਾਈ_Ok(srcp) || !useOk(redOrp, ਸੱਚ)) ਜਾਰੀ;
            val = 0;
            inv = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (DfgSel* ਅਚਲ selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            ਜੇ (selp->ਚੌੜਾਈ() != 1) ਜਾਰੀ;
            Dfg_ਖਿਸਕਾਓ_L* ਅਚਲ ਖਿਸਕਾਓ_Lp = selp->fromp()->cast<Dfg_ਖਿਸਕਾਓ_L>();
            ਜੇ (!ਖਿਸਕਾਓ_Lp) ਜਾਰੀ;
            ਅਚਲ DfgConst* ਅਚਲ constp = ਖਿਸਕਾਓ_Lp->lhsp()->cast<DfgConst>();
            ਜੇ (!constp || !useOk(selp, ਝੂਠ)) ਜਾਰੀ;
            ਜੇ (!constp->has_ਮੁੱਲ(1)) ਜਾਰੀ;
            srcp = ਖਿਸਕਾਓ_Lp->rhsp();
            ਜੇ (!ਚੌੜਾਈ_Ok(srcp)) ਜਾਰੀ;
            val = selp->lsb();
            inv = ਝੂਠ;
        } ਨਹੀਂਤਾਂ {
            // Not a comparison-like vertex
            ਜਾਰੀ;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        ਜੇ (val2Terms.ਖਾਲੀ_ਥਾਵਾਂ()) srcps.emplace_ਪਿੱਠ(srcp);
        // Record term
        val2Terms[val].emplace_ਪਿੱਠ(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t ਪਦ_ਹੱਦਬੰਦੀ = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << ਚੌੜਾਈ_MIN) >= ਪਦ_ਹੱਦਬੰਦੀ, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    ਜੇ (nTerms < ਪਦ_ਹੱਦਬੰਦੀ) ਵਾਪਸ;

    Dfg_ਸ਼ੀਰਸ਼::ScopeCache scopeCache;

    // Create decoders for each srcp
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp : srcps) {
        ਅਚਲ Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        ਜੇ (val2Terms.ਮਾਪ() < ਪਦ_ਹੱਦਬੰਦੀ) ਜਾਰੀ;

        // Width of the decoded binary value
        ਅਚਲ uint32_t ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
        // Number of bits in the input operand
        ਅਚਲ uint32_t n_ਬਿੱਟ = 1U << ਚੌੜਾਈ;

        // Construct the decoder by converting many "const == vtx" by:
        // - Adding a single decoder block, where 'tab' is zero initialized:
        //     always_comb begin
        //        tab[pre] = 0;
        //        tab[vtx] = 1;
        //        pre = vtx;
        //     end
        //   We mark 'pre' so the write is ignored during scheduling, so this
        //   won't cause a combinational cycle.
        //   Note that albeit this looks like partial udpates to 'tab', the
        //   actual result is that only one value in 'tab' is ever one, while
        //   all the others are always zero.
        // - and replace the comparisons with 'tab[const]'

        File_ਸਤਰ* ਅਚਲ flp = srcp->fileline();
        AstScope* ਅਚਲ scopep = srcp->scopep(scopeCache, ਸੱਚ);

        Ast_ਸਰਗਰਮ* ਅਚਲ staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            ਲਈ (Ast_ਗੰਢ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_ਸਰਗਰਮ* ਅਚਲ activep = VN_CAST(nodep, ਸਰਗਰਮ);
                ਜੇ (!activep) ਜਾਰੀ;
                ਜੇ (activep->sentreep()->hasStatic()) ਵਾਪਸ activep;
            }
            // If there isn't one, create a new one
            File_ਸਤਰ* ਅਚਲ flp = scopep->fileline();
            Ast_Sen_ਰੁੱਖ* ਅਚਲ stp = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, Ast_Sen_ਵਸਤੂ::Static{}}};
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "", stp};
            activep->sen_ਰੁੱਖ_Storep(stp);
            scopep->ਜੋੜੋ_Blocksp(activep);
            ਵਾਪਸ activep;
        }();
        Ast_ਸਰਗਰਮ* ਅਚਲ combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            ਲਈ (Ast_ਗੰਢ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_ਸਰਗਰਮ* ਅਚਲ activep = VN_CAST(nodep, ਸਰਗਰਮ);
                ਜੇ (!activep) ਜਾਰੀ;
                ਜੇ (activep->sentreep()->hasCombo()) ਵਾਪਸ activep;
            }
            // If there isn't one, create a new one
            File_ਸਤਰ* ਅਚਲ flp = scopep->fileline();
            Ast_Sen_ਰੁੱਖ* ਅਚਲ stp = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, Ast_Sen_ਵਸਤੂ::Combo{}}};
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "", stp};
            activep->sen_ਰੁੱਖ_Storep(stp);
            scopep->ਜੋੜੋ_Blocksp(activep);
            ਵਾਪਸ activep;
        }();

        // Required data types
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& idxDType = srcp->dtype();
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਦੋਆਧਾਰੀ_ਅੰਕ_DType = Dfg_ਅੰਕੜੇ_Type::packed(1);
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& tabDType = Dfg_ਅੰਕੜੇ_Type::array(ਦੋਆਧਾਰੀ_ਅੰਕ_DType, n_ਬਿੱਟ);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            ਜੇ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vp = srcp->get_ਨਤੀਜਾ_Var()) ਵਾਪਸ vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
            vtxp->srcp(srcp);
            ਵਾਪਸ vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ਅਚਲ idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ਅਚਲ preVscp = [&]() {
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ਅਚਲ vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
            vscp->varp()->noReset(ਸੱਚ);
            vscp->varp()->ਸੈੱਟ_Ignore_Sched_Write();
            ਵਾਪਸ vscp;
        }();
        // The table variable
        DfgVarArray* ਅਚਲ tabVtxp = [&]() {
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
            varp->vscp()->varp()->noReset(ਸੱਚ);
            varp->ਸੈੱਟ_Has_Mod_Wr_Refs();
            ਵਾਪਸ varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ਅਚਲ initp = ਨਵਾਂ AstInitialStatic{flp, ਖਾਲੀ_ਸੂਚਕ};
        staticActivep->ਜੋੜੋ_Stmtsp(initp);
        {  // pre = 0
            initp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                flp,  //
                ਨਵਾਂ AstVarRef{flp, preVscp, VAccess::WRITE},  //
                ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, static_cast<ਪੂਰਨ>(ਚੌੜਾਈ), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
                flp, ਨਵਾਂ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            callp->dtype_ਸੈੱਟ_Void();
            initp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ਅਚਲ logicp = ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        combActivep->ਜੋੜੋ_Stmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                flp,  //
                ਨਵਾਂ AstArraySel{flp, ਨਵਾਂ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                ਨਵਾਂ AstVarRef{flp, preVscp, VAccess::READ}},  //
                ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ਅਚਲ idxRefp = ਨਵਾਂ AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                flp,  //
                ਨਵਾਂ AstArraySel{flp, ਨਵਾਂ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}});
            DfgAstRd* ਅਚਲ astRdp = ਨਵਾਂ DfgAstRd{dfg, idxRefp, ਝੂਠ, ਝੂਠ};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ਅਚਲ idxRefp = ਨਵਾਂ AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp,  //
                                            ਨਵਾਂ AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ਅਚਲ astRdp = ਨਵਾਂ DfgAstRd{dfg, idxRefp, ਝੂਠ, ਝੂਠ};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : val2Terms) {
            ਅਚਲ uint32_t val = pair.ਪਹਿਲਾ;
            ਅਚਲ std::ਸਦਿਸ਼<ਪਦ_2>& terms = pair.ਦੂਜਾ;
            // Create the ArraySel
            File_ਸਤਰ* ਅਚਲ aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ਅਚਲ aselp = ਨਵਾਂ DfgArraySel{dfg, aflp, ਦੋਆਧਾਰੀ_ਅੰਕ_DType};
            aselp->fromp(tabVtxp);
            aselp->bitp(ਨਵਾਂ DfgConst{dfg, aflp, ਚੌੜਾਈ, val});
            // The inverted value, if needed
            DfgNot* notp = ਖਾਲੀ_ਸੂਚਕ;
            // Repalce the terms
            ਲਈ (ਅਚਲ ਪਦ_2& ਪਦ : terms) {
                ਜੇ (ਪਦ.m_inv) {
                    ਜੇ (!notp) {
                        notp = ਨਵਾਂ DfgNot{dfg, aflp, ਦੋਆਧਾਰੀ_ਅੰਕ_DType};
                        notp->srcp(aselp);
                    }
                    ਪਦ.m_vtxp->replaceWith(notp);
                } ਨਹੀਂਤਾਂ {
                    ਪਦ.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(ਪਦ.m_vtxp->unlinkDelete(dfg), ਪਦ.m_vtxp);
            }
        }
    }
}
