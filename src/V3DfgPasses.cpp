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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3DfgPasses.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

శూన్య V3DfgPasses::removeUnobservable(Dfg_లేఖాచిత్రం& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    కొరకు (Dfg_శీర్షం& vtx : dfg.opVertices()) {
        ఒకవేళ (vtx.is<DfgLogic>()) workList.నెట్టు_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](Dfg_శీర్షం& vtx) {
        DfgLogic* స్థిరరాశి logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        స్థిరరాశి తార్కిక వాడిన = logicp->foreachSink([&](Dfg_శీర్షం& snk) {
            DfgUnresolved* స్థిరరాశి uVtxp = snk.as<DfgUnresolved>();
            Dfg_శీర్షం_Var* స్థిరరాశి vVtxp = uVtxp->firtsSinkp()->as<Dfg_శీర్షం_Var>();
            ఒకవేళ (vVtxp->hasSinks()) తిరిగి నిజం;
            ఒకవేళ (vVtxp->isObserved()) తిరిగి నిజం;
            తిరిగి అబద్ధం;
        });
        // If some are used, the logic must stay in the Ast
        ఒకవేళ (వాడిన) తిరిగి;
        // If impure, it must stay in the Ast
        ఒకవేళ (!logicp->is_శుద్ధ()) తిరిగి;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreach_ఆధారం([&](Dfg_శీర్షం& src) {
            Dfg_శీర్షం_Var* స్థిరరాశి varp = src.as<Dfg_శీర్షం_Var>();
            ఒకవేళ (!varp->srcp()) తిరిగి అబద్ధం;
            varp->srcp()->as<DfgUnresolved>()->foreach_ఆధారం([&](Dfg_శీర్షం& driver) {
                workList.నెట్టు_front(*driver.as<DfgLogic>());
                తిరిగి అబద్ధం;
            });
            తిరిగి అబద్ధం;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_కణుపు* స్థిరరాశి nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    కొరకు (Dfg_శీర్షం_Var* స్థిరరాశి vVtxp : dfg.varVertices().unlinkable()) {
        ఒకవేళ (vVtxp->hasSinks()) కొనసాగు;
        ఒకవేళ (vVtxp->isObserved()) కొనసాగు;
        Dfg_శీర్షం* స్థిరరాశి srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* స్థిరరాశి vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        స్థిరరాశి తార్కిక delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        ఒకవేళ (srcp) {
            srcp->foreach_ఆధారం([&](Dfg_శీర్షం& src) {
                src.as<DfgLogic>()->సమితి_Drives_వాడని_Vars();
                తిరిగి అబద్ధం;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        ఒకవేళ (delAst) {
            VL_DO_DANGLING(vscp->unlink_Fr_వెనుక()->delete_వృక్షం(), vscp);
            ++ctx.m_varsDeleted;
        } లేకపోతే {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* స్థిరరాశి logicp = vtxp->cast<DfgLogic>();
        ఒకవేళ (!logicp) కొనసాగు;
        ఒకవేళ (logicp->hasSinks()) కొనసాగు;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        స్థిరరాశి VNUser2_లోపల_Use m_user2_లోపల_Use;
        logicp->foreach_ఆధారం([](Dfg_శీర్షం& src) {
            src.as<Dfg_శీర్షం_Var>()->vscp()->user2p(&src);
            తిరిగి అబద్ధం;
        });
        V3DfgPasses::కూడు_Ast_Refs(dfg, logicp->nodep(), [](Ast_కణుపు* varp) {  //
            తిరిగి varp->user2u().గమ్యం<Dfg_శీర్షం_Var*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

శూన్య V3DfgPasses::removeSelects(Dfg_లేఖాచిత్రం& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::సదిశ<DfgSel*> selps;
    కొరకు (Dfg_శీర్షం& vtx : dfg.opVertices()) {
        DfgSel* స్థిరరాశి selp = vtx.cast<DfgSel>();
        ఒకవేళ (!selp) కొనసాగు;
        selps.నెట్టు_వెనుక(selp);
    }

    కొరకు (DfgSel* స్థిరరాశి selp : selps) {
        File_పంక్తి* స్థిరరాశి flp = selp->fileline();
        స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = selp->dtype();

        // Remove full width selects
        ఒకవేళ (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removed_నిండిన_వెడల్పు;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            కొనసాగు;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* స్థిరరాశి varp = selp->fromp()->cast<DfgVarPacked>();
        ఒకవేళ (!varp || !varp->tmpForp()) కొనసాగు;

        // Find the driver of this range
        స్థిరరాశి స్వయంచాలక pair = varp->driver_యొక్క_Range(selp->lsb(), selp->వెడల్పు());
        Dfg_శీర్షం* స్థిరరాశి driverp = pair.మొదటి;
        స్థిరరాశి uint32_t driverLsb = pair.రెండవ;
        ఒకవేళ (!driverp) కొనసాగు;

        // If partial driver is the whole thing we are looking for, just replace with that
        ఒకవేళ (driverp->dtype() == dtype) {
            ++ctx.m_replaced_With_సంపూర్ణ_Driver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            కొనసాగు;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* స్థిరరాశి newSelp = కొత్త DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

శూన్య V3DfgPasses::inlineVars(Dfg_లేఖాచిత్రం& dfg) {
    కొరకు (Dfg_శీర్షం_Var& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        ఒకవేళ (!vtx.hasSinks()) కొనసాగు;
        // Nohting to inline
        Dfg_శీర్షం* స్థిరరాశి srcp = vtx.srcp();
        ఒకవేళ (!srcp) కొనసాగు;
        // Value can differ from driver
        ఒకవేళ (vtx.isVolatile()) కొనసాగు;
        // Partial driver cannot be inlined
        ఒకవేళ (srcp->is<Dfg_శీర్షం_Splice>()) కొనసాగు;
        ఒకవేళ (srcp->is<DfgUnitArray>()) కొనసాగు;
        // Don't inline CReset
        ఒకవేళ (srcp->is<DfgCReset>()) కొనసాగు;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

శూన్య V3DfgPasses::remove_వాడని(Dfg_లేఖాచిత్రం& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    కొరకు (Dfg_శీర్షం& vtx : dfg.opVertices()) {
        ఒకవేళ (vtx.hasSinks()) కొనసాగు;
        // This vertex is unused. Add to work list.
        workList.నెట్టు_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    కొరకు (Dfg_శీర్షం_Var& vtx : dfg.varVertices()) {
        ఒకవేళ (!vtx.tmpForp()) కొనసాగు;
        ఒకవేళ (vtx.hasSinks()) కొనసాగు;
        ఒకవేళ (vtx.hasDfgRefs()) కొనసాగు;
        // This vertex is unused. Add to work list.
        workList.నెట్టు_front(vtx);
    }

    // Process the work list
    workList.foreach([&](Dfg_శీర్షం& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        ఒకవేళ (vtx.hasSinks()) తిరిగి;
        // If temporary used in another graph, we need to keep it
        ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = vtx.cast<Dfg_శీర్షం_Var>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            ఒకవేళ (varp->hasDfgRefs()) తిరిగి;
        }
        // Add sources of unused vertex to work list
        vtx.foreach_ఆధారం([&](Dfg_శీర్షం& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            ఒకవేళ (src.is<DfgConst>()) తిరిగి అబద్ధం;
            స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = src.cast<Dfg_శీర్షం_Var>();
            ఒకవేళ (varp && !varp->tmpForp()) తిరిగి అబద్ధం;
            // Add source to workList
            workList.నెట్టు_front(src);
            తిరిగి అబద్ధం;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    కొరకు (Dfg_శీర్షం_Var* స్థిరరాశి vtxp : dfg.varVertices().unlinkable()) {
        ఒకవేళ (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    కొరకు (DfgConst* స్థిరరాశి vtxp : dfg.constVertices().unlinkable()) {
        ఒకవేళ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

శూన్య V3DfgPasses::bin_గమ్యం_One_Hot(Dfg_లేఖాచిత్రం& dfg, V3_Dfg_Bin_గమ్యం_One_Hot_Context& ctx) {
    // Structure to keep track of comparison details
    నిర్మాణం పదం_2 final {
        Dfg_శీర్షం* m_vtxp = శూన్యసూచిక;  // Vertex to replace
        తార్కిక m_inv = అబద్ధం;  // '!=', instead of '=='
        పదం_2() = అప్రమేయ;
        పదం_2(Dfg_శీర్షం* vtxp, తార్కిక inv)
            : m_vtxp{vtxp}
            , m_inv{inv} {}
    };

    // List of vertices that are used as sources
    std::సదిశ<Dfg_శీర్షం*> srcps;

    // Map from 'vertices' -> 'value beign compared' -> 'terms'
    ఉపయోగించి Val2Terms = std::map<uint32_t, std::సదిశ<పదం_2>>;
    DfgUserMap<Val2Terms> vtx2Val2Terms = dfg.makeUserMap<Val2Terms>();

    // Only consider input variables from a reasonable range:
    // - not too big to avoid huge tables, you are doomed anyway at that point..
    // - not too small, as it's probably not worth it
    constexpr uint32_t వెడల్పు_MIN = 7;
    constexpr uint32_t వెడల్పు_MAX = 20;
    స్థిరరాశి స్వయంచాలక వెడల్పు_Ok = [](స్థిరరాశి Dfg_శీర్షం* vtxp) {
        స్థిరరాశి uint32_t వెడల్పు = vtxp->వెడల్పు();
        తిరిగి వెడల్పు_MIN <= వెడల్పు && వెడల్పు <= వెడల్పు_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    స్థిరరాశి std::function<తార్కిక(స్థిరరాశి Dfg_శీర్షం*, తార్కిక)> useOk
        = [&](స్థిరరాశి Dfg_శీర్షం* vtxp, తార్కిక inv) -> తార్కిక {
        // Go past a single 'Not' sink, which is common
        ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి sinkp = vtxp->singleSink()) {
            ఒకవేళ (sinkp->is<DfgNot>()) తిరిగి useOk(sinkp, !inv);
        }
        స్థిరరాశి తార్కిక cond_వృక్షం = vtxp->foreachSink([&](స్థిరరాశి Dfg_శీర్షం& sink) {
            స్థిరరాశి DfgCond* స్థిరరాశి condp = sink.cast<DfgCond>();
            ఒకవేళ (!condp) తిరిగి అబద్ధం;
            ఒకవేళ (condp->condp() != vtxp) తిరిగి అబద్ధం;
            తిరిగి inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        తిరిగి !cond_వృక్షం;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    కొరకు (Dfg_శీర్షం& vtx : dfg.opVertices()) {
        Dfg_శీర్షం* srcp = శూన్యసూచిక;
        uint32_t val = 0;
        తార్కిక inv = అబద్ధం;
        ఒకవేళ (DfgEq* స్థిరరాశి eqp = vtx.cast<DfgEq>()) {
            DfgConst* స్థిరరాశి constp = eqp->lhsp()->cast<DfgConst>();
            ఒకవేళ (!constp || !వెడల్పు_Ok(constp) || !useOk(eqp, అబద్ధం)) కొనసాగు;
            srcp = eqp->rhsp();
            val = constp->గమ్యం_U32();
            inv = అబద్ధం;
        } లేకపోతే ఒకవేళ (DfgNeq* స్థిరరాశి neqp = vtx.cast<DfgNeq>()) {
            DfgConst* స్థిరరాశి constp = neqp->lhsp()->cast<DfgConst>();
            ఒకవేళ (!constp || !వెడల్పు_Ok(constp) || !useOk(neqp, నిజం)) కొనసాగు;
            srcp = neqp->rhsp();
            val = constp->గమ్యం_U32();
            inv = నిజం;
        } లేకపోతే ఒకవేళ (DfgRedAnd* స్థిరరాశి redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            ఒకవేళ (!వెడల్పు_Ok(srcp) || !useOk(redAndp, అబద్ధం)) కొనసాగు;
            val = (1U << srcp->వెడల్పు()) - 1;
            inv = అబద్ధం;
        } లేకపోతే ఒకవేళ (DfgRedOr* స్థిరరాశి redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            ఒకవేళ (!వెడల్పు_Ok(srcp) || !useOk(redOrp, నిజం)) కొనసాగు;
            val = 0;
            inv = నిజం;
        } లేకపోతే ఒకవేళ (DfgSel* స్థిరరాశి selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            ఒకవేళ (selp->వెడల్పు() != 1) కొనసాగు;
            Dfg_జరుపు_L* స్థిరరాశి జరుపు_Lp = selp->fromp()->cast<Dfg_జరుపు_L>();
            ఒకవేళ (!జరుపు_Lp) కొనసాగు;
            స్థిరరాశి DfgConst* స్థిరరాశి constp = జరుపు_Lp->lhsp()->cast<DfgConst>();
            ఒకవేళ (!constp || !useOk(selp, అబద్ధం)) కొనసాగు;
            ఒకవేళ (!constp->has_విలువ(1)) కొనసాగు;
            srcp = జరుపు_Lp->rhsp();
            ఒకవేళ (!వెడల్పు_Ok(srcp)) కొనసాగు;
            val = selp->lsb();
            inv = అబద్ధం;
        } లేకపోతే {
            // Not a comparison-like vertex
            కొనసాగు;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        ఒకవేళ (val2Terms.ఖాళీలు()) srcps.emplace_వెనుక(srcp);
        // Record term
        val2Terms[val].emplace_వెనుక(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t పదం_హద్దు = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << వెడల్పు_MIN) >= పదం_హద్దు, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    ఒకవేళ (nTerms < పదం_హద్దు) తిరిగి;

    Dfg_శీర్షం::ScopeCache scopeCache;

    // Create decoders for each srcp
    కొరకు (Dfg_శీర్షం* స్థిరరాశి srcp : srcps) {
        స్థిరరాశి Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        ఒకవేళ (val2Terms.కొలత() < పదం_హద్దు) కొనసాగు;

        // Width of the decoded binary value
        స్థిరరాశి uint32_t వెడల్పు = srcp->వెడల్పు();
        // Number of bits in the input operand
        స్థిరరాశి uint32_t n_బిట్లు = 1U << వెడల్పు;

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

        File_పంక్తి* స్థిరరాశి flp = srcp->fileline();
        AstScope* స్థిరరాశి scopep = srcp->scopep(scopeCache, నిజం);

        Ast_చురుకైన* స్థిరరాశి staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            కొరకు (Ast_కణుపు* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_చురుకైన* స్థిరరాశి activep = VN_CAST(nodep, చురుకైన);
                ఒకవేళ (!activep) కొనసాగు;
                ఒకవేళ (activep->sentreep()->hasStatic()) తిరిగి activep;
            }
            // If there isn't one, create a new one
            File_పంక్తి* స్థిరరాశి flp = scopep->fileline();
            Ast_Sen_వృక్షం* స్థిరరాశి stp = కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, Ast_Sen_అంశం::Static{}}};
            Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "", stp};
            activep->sen_వృక్షం_Storep(stp);
            scopep->కూడు_Blocksp(activep);
            తిరిగి activep;
        }();
        Ast_చురుకైన* స్థిరరాశి combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            కొరకు (Ast_కణుపు* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_చురుకైన* స్థిరరాశి activep = VN_CAST(nodep, చురుకైన);
                ఒకవేళ (!activep) కొనసాగు;
                ఒకవేళ (activep->sentreep()->hasCombo()) తిరిగి activep;
            }
            // If there isn't one, create a new one
            File_పంక్తి* స్థిరరాశి flp = scopep->fileline();
            Ast_Sen_వృక్షం* స్థిరరాశి stp = కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, Ast_Sen_అంశం::Combo{}}};
            Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "", stp};
            activep->sen_వృక్షం_Storep(stp);
            scopep->కూడు_Blocksp(activep);
            తిరిగి activep;
        }();

        // Required data types
        స్థిరరాశి Dfg_దత్తాంశం_Type& idxDType = srcp->dtype();
        స్థిరరాశి Dfg_దత్తాంశం_Type& బిట్టు_DType = Dfg_దత్తాంశం_Type::packed(1);
        స్థిరరాశి Dfg_దత్తాంశం_Type& tabDType = Dfg_దత్తాంశం_Type::array(బిట్టు_DType, n_బిట్లు);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            ఒకవేళ (Dfg_శీర్షం_Var* స్థిరరాశి vp = srcp->get_ఫలితం_Var()) తిరిగి vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            Dfg_శీర్షం_Var* స్థిరరాశి vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->is_అంతర్గత(నిజం);
            vtxp->srcp(srcp);
            తిరిగి vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* స్థిరరాశి idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* స్థిరరాశి preVscp = [&]() {
            Dfg_శీర్షం_Var* స్థిరరాశి vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* స్థిరరాశి vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->is_అంతర్గత(నిజం);
            vscp->varp()->noReset(నిజం);
            vscp->varp()->సమితి_Ignore_Sched_Write();
            తిరిగి vscp;
        }();
        // The table variable
        DfgVarArray* స్థిరరాశి tabVtxp = [&]() {
            Dfg_శీర్షం_Var* స్థిరరాశి varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->is_అంతర్గత(నిజం);
            varp->vscp()->varp()->noReset(నిజం);
            varp->సమితి_Has_Mod_Wr_Refs();
            తిరిగి varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* స్థిరరాశి initp = కొత్త AstInitialStatic{flp, శూన్యసూచిక};
        staticActivep->కూడు_Stmtsp(initp);
        {  // pre = 0
            initp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
                flp,  //
                కొత్త AstVarRef{flp, preVscp, VAccess::WRITE},  //
                కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, static_cast<పూర్ణాంకం>(వెడల్పు), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
                flp, కొత్త AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->కూడు_Pinsp(కొత్త AstConst{flp, AstConst::బిట్టు_False{}});
            callp->dtype_సమితి_Void();
            initp->కూడు_Stmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* స్థిరరాశి logicp = కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, శూన్యసూచిక, శూన్యసూచిక};
        combActivep->కూడు_Stmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
                flp,  //
                కొత్త AstArraySel{flp, కొత్త AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                కొత్త AstVarRef{flp, preVscp, VAccess::READ}},  //
                కొత్త AstConst{flp, AstConst::బిట్టు_False{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* స్థిరరాశి idxRefp = కొత్త AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
                flp,  //
                కొత్త AstArraySel{flp, కొత్త AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                కొత్త AstConst{flp, AstConst::బిట్టు_True{}}});
            DfgAstRd* స్థిరరాశి astRdp = కొత్త DfgAstRd{dfg, idxRefp, అబద్ధం, అబద్ధం};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* స్థిరరాశి idxRefp = కొత్త AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp,  //
                                            కొత్త AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* స్థిరరాశి astRdp = కొత్త DfgAstRd{dfg, idxRefp, అబద్ధం, అబద్ధం};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : val2Terms) {
            స్థిరరాశి uint32_t val = pair.మొదటి;
            స్థిరరాశి std::సదిశ<పదం_2>& terms = pair.రెండవ;
            // Create the ArraySel
            File_పంక్తి* స్థిరరాశి aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* స్థిరరాశి aselp = కొత్త DfgArraySel{dfg, aflp, బిట్టు_DType};
            aselp->fromp(tabVtxp);
            aselp->bitp(కొత్త DfgConst{dfg, aflp, వెడల్పు, val});
            // The inverted value, if needed
            DfgNot* notp = శూన్యసూచిక;
            // Repalce the terms
            కొరకు (స్థిరరాశి పదం_2& పదం : terms) {
                ఒకవేళ (పదం.m_inv) {
                    ఒకవేళ (!notp) {
                        notp = కొత్త DfgNot{dfg, aflp, బిట్టు_DType};
                        notp->srcp(aselp);
                    }
                    పదం.m_vtxp->replaceWith(notp);
                } లేకపోతే {
                    పదం.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(పదం.m_vtxp->unlinkDelete(dfg), పదం.m_vtxp);
            }
        }
    }
}
