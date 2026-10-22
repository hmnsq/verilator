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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3DfgPasses.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

शून्य V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    प्रति (DfgVertex& vtx : dfg.opVertices()) {
        यदि (vtx.is<DfgLogic>()) workList.प्रक्षिप_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ध्रुव logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ध्रुव तार्किक प्रयुक्तम् = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ध्रुव uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ध्रुव vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            यदि (vVtxp->hasSinks()) प्रत्यागम सत्य;
            यदि (vVtxp->isObserved()) प्रत्यागम सत्य;
            प्रत्यागम असत्य;
        });
        // If some are used, the logic must stay in the Ast
        यदि (प्रयुक्तम्) प्रत्यागम;
        // If impure, it must stay in the Ast
        यदि (!logicp->is_विशुद्धम्()) प्रत्यागम;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ध्रुव varp = src.as<DfgVertexVar>();
            यदि (!varp->srcp()) प्रत्यागम असत्य;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.प्रक्षिप_front(*driver.as<DfgLogic>());
                प्रत्यागम असत्य;
            });
            प्रत्यागम असत्य;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_ग्रन्थिः* ध्रुव nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    प्रति (DfgVertexVar* ध्रुव vVtxp : dfg.varVertices().unlinkable()) {
        यदि (vVtxp->hasSinks()) अनुवर्त;
        यदि (vVtxp->isObserved()) अनुवर्त;
        DfgVertex* ध्रुव srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ध्रुव vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ध्रुव तार्किक delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        यदि (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->समुच्चयः_Drives_अप्रयुक्तम्_Vars();
                प्रत्यागम असत्य;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        यदि (delAst) {
            VL_DO_DANGLING(vscp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), vscp);
            ++ctx.m_varsDeleted;
        } अन्यथा {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    प्रति (DfgVertex* ध्रुव vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ध्रुव logicp = vtxp->cast<DfgLogic>();
        यदि (!logicp) अनुवर्त;
        यदि (logicp->hasSinks()) अनुवर्त;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ध्रुव VNUser2_अन्तर्_Use m_user2_अन्तर्_Use;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            प्रत्यागम असत्य;
        });
        V3DfgPasses::सङ्कलय_Ast_Refs(dfg, logicp->nodep(), [](Ast_ग्रन्थिः* varp) {  //
            प्रत्यागम varp->user2u().गन्तव्यम्<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

शून्य V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::सदिशम्<DfgSel*> selps;
    प्रति (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ध्रुव selp = vtx.cast<DfgSel>();
        यदि (!selp) अनुवर्त;
        selps.प्रक्षिप_पृष्ठभागः(selp);
    }

    प्रति (DfgSel* ध्रुव selp : selps) {
        File_पङ्क्तिः* ध्रुव flp = selp->fileline();
        ध्रुव Dfg_दत्त_Type& dtype = selp->dtype();

        // Remove full width selects
        यदि (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removed_पूर्णम्_विस्तारः;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            अनुवर्त;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ध्रुव varp = selp->fromp()->cast<DfgVarPacked>();
        यदि (!varp || !varp->tmpForp()) अनुवर्त;

        // Find the driver of this range
        ध्रुव स्वचालित pair = varp->driver_सम्बन्धि_Range(selp->lsb(), selp->विस्तारः());
        DfgVertex* ध्रुव driverp = pair.first;
        ध्रुव uint32_t driverLsb = pair.second;
        यदि (!driverp) अनुवर्त;

        // If partial driver is the whole thing we are looking for, just replace with that
        यदि (driverp->dtype() == dtype) {
            ++ctx.m_replaced_With_सम्पूर्णम्_Driver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            अनुवर्त;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ध्रुव newSelp = नूतन DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

शून्य V3DfgPasses::inlineVars(DfgGraph& dfg) {
    प्रति (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        यदि (!vtx.hasSinks()) अनुवर्त;
        // Nohting to inline
        DfgVertex* ध्रुव srcp = vtx.srcp();
        यदि (!srcp) अनुवर्त;
        // Value can differ from driver
        यदि (vtx.isVolatile()) अनुवर्त;
        // Partial driver cannot be inlined
        यदि (srcp->is<DfgVertexSplice>()) अनुवर्त;
        यदि (srcp->is<DfgUnitArray>()) अनुवर्त;
        // Don't inline CReset
        यदि (srcp->is<DfgCReset>()) अनुवर्त;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

शून्य V3DfgPasses::remove_अप्रयुक्तम्(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    प्रति (DfgVertex& vtx : dfg.opVertices()) {
        यदि (vtx.hasSinks()) अनुवर्त;
        // This vertex is unused. Add to work list.
        workList.प्रक्षिप_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    प्रति (DfgVertexVar& vtx : dfg.varVertices()) {
        यदि (!vtx.tmpForp()) अनुवर्त;
        यदि (vtx.hasSinks()) अनुवर्त;
        यदि (vtx.hasDfgRefs()) अनुवर्त;
        // This vertex is unused. Add to work list.
        workList.प्रक्षिप_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        यदि (vtx.hasSinks()) प्रत्यागम;
        // If temporary used in another graph, we need to keep it
        यदि (ध्रुव DfgVertexVar* ध्रुव varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            यदि (varp->hasDfgRefs()) प्रत्यागम;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            यदि (src.is<DfgConst>()) प्रत्यागम असत्य;
            ध्रुव DfgVertexVar* ध्रुव varp = src.cast<DfgVertexVar>();
            यदि (varp && !varp->tmpForp()) प्रत्यागम असत्य;
            // Add source to workList
            workList.प्रक्षिप_front(src);
            प्रत्यागम असत्य;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    प्रति (DfgVertexVar* ध्रुव vtxp : dfg.varVertices().unlinkable()) {
        यदि (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    प्रति (DfgConst* ध्रुव vtxp : dfg.constVertices().unlinkable()) {
        यदि (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

शून्य V3DfgPasses::bin_गन्तव्यम्_One_Hot(DfgGraph& dfg, V3_Dfg_Bin_गन्तव्यम्_One_Hot_Context& ctx) {
    // Structure to keep track of comparison details
    संरचना पदांशः_2 final {
        DfgVertex* m_vtxp = शून्यसूचकः;  // Vertex to replace
        तार्किक m_inv = असत्य;  // '!=', instead of '=='
        पदांशः_2() = स्वभाव;
        पदांशः_2(DfgVertex* vtxp, तार्किक inv)
            : m_vtxp{vtxp}
            , m_inv{inv} {}
    };

    // List of vertices that are used as sources
    std::सदिशम्<DfgVertex*> srcps;

    // Map from 'vertices' -> 'value beign compared' -> 'terms'
    उपयुज्य Val2Terms = std::map<uint32_t, std::सदिशम्<पदांशः_2>>;
    DfgUserMap<Val2Terms> vtx2Val2Terms = dfg.makeUserMap<Val2Terms>();

    // Only consider input variables from a reasonable range:
    // - not too big to avoid huge tables, you are doomed anyway at that point..
    // - not too small, as it's probably not worth it
    constexpr uint32_t विस्तारः_MIN = 7;
    constexpr uint32_t विस्तारः_MAX = 20;
    ध्रुव स्वचालित विस्तारः_Ok = [](ध्रुव DfgVertex* vtxp) {
        ध्रुव uint32_t विस्तारः = vtxp->विस्तारः();
        प्रत्यागम विस्तारः_MIN <= विस्तारः && विस्तारः <= विस्तारः_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ध्रुव std::function<तार्किक(ध्रुव DfgVertex*, तार्किक)> useOk
        = [&](ध्रुव DfgVertex* vtxp, तार्किक inv) -> तार्किक {
        // Go past a single 'Not' sink, which is common
        यदि (DfgVertex* ध्रुव sinkp = vtxp->singleSink()) {
            यदि (sinkp->is<DfgNot>()) प्रत्यागम useOk(sinkp, !inv);
        }
        ध्रुव तार्किक cond_वृक्षः = vtxp->foreachSink([&](ध्रुव DfgVertex& sink) {
            ध्रुव DfgCond* ध्रुव condp = sink.cast<DfgCond>();
            यदि (!condp) प्रत्यागम असत्य;
            यदि (condp->condp() != vtxp) प्रत्यागम असत्य;
            प्रत्यागम inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        प्रत्यागम !cond_वृक्षः;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    प्रति (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = शून्यसूचकः;
        uint32_t val = 0;
        तार्किक inv = असत्य;
        यदि (DfgEq* ध्रुव eqp = vtx.cast<DfgEq>()) {
            DfgConst* ध्रुव constp = eqp->lhsp()->cast<DfgConst>();
            यदि (!constp || !विस्तारः_Ok(constp) || !useOk(eqp, असत्य)) अनुवर्त;
            srcp = eqp->rhsp();
            val = constp->गन्तव्यम्_U32();
            inv = असत्य;
        } अन्यथा यदि (DfgNeq* ध्रुव neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ध्रुव constp = neqp->lhsp()->cast<DfgConst>();
            यदि (!constp || !विस्तारः_Ok(constp) || !useOk(neqp, सत्य)) अनुवर्त;
            srcp = neqp->rhsp();
            val = constp->गन्तव्यम्_U32();
            inv = सत्य;
        } अन्यथा यदि (DfgRedAnd* ध्रुव redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            यदि (!विस्तारः_Ok(srcp) || !useOk(redAndp, असत्य)) अनुवर्त;
            val = (1U << srcp->विस्तारः()) - 1;
            inv = असत्य;
        } अन्यथा यदि (DfgRedOr* ध्रुव redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            यदि (!विस्तारः_Ok(srcp) || !useOk(redOrp, सत्य)) अनुवर्त;
            val = 0;
            inv = सत्य;
        } अन्यथा यदि (DfgSel* ध्रुव selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            यदि (selp->विस्तारः() != 1) अनुवर्त;
            Dfg_सरणम्_L* ध्रुव सरणम्_Lp = selp->fromp()->cast<Dfg_सरणम्_L>();
            यदि (!सरणम्_Lp) अनुवर्त;
            ध्रुव DfgConst* ध्रुव constp = सरणम्_Lp->lhsp()->cast<DfgConst>();
            यदि (!constp || !useOk(selp, असत्य)) अनुवर्त;
            यदि (!constp->has_मानम्(1)) अनुवर्त;
            srcp = सरणम्_Lp->rhsp();
            यदि (!विस्तारः_Ok(srcp)) अनुवर्त;
            val = selp->lsb();
            inv = असत्य;
        } अन्यथा {
            // Not a comparison-like vertex
            अनुवर्त;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        यदि (val2Terms.रिक्तानि()) srcps.emplace_पृष्ठभागः(srcp);
        // Record term
        val2Terms[val].emplace_पृष्ठभागः(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t पदांशः_सीमा = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << विस्तारः_MIN) >= पदांशः_सीमा, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    यदि (nTerms < पदांशः_सीमा) प्रत्यागम;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    प्रति (DfgVertex* ध्रुव srcp : srcps) {
        ध्रुव Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        यदि (val2Terms.आकार() < पदांशः_सीमा) अनुवर्त;

        // Width of the decoded binary value
        ध्रुव uint32_t विस्तारः = srcp->विस्तारः();
        // Number of bits in the input operand
        ध्रुव uint32_t n_द्वयङ्काः = 1U << विस्तारः;

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

        File_पङ्क्तिः* ध्रुव flp = srcp->fileline();
        AstScope* ध्रुव scopep = srcp->scopep(scopeCache, सत्य);

        Ast_सक्रिय* ध्रुव staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            प्रति (Ast_ग्रन्थिः* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_सक्रिय* ध्रुव activep = VN_CAST(nodep, सक्रिय);
                यदि (!activep) अनुवर्त;
                यदि (activep->sentreep()->hasStatic()) प्रत्यागम activep;
            }
            // If there isn't one, create a new one
            File_पङ्क्तिः* ध्रुव flp = scopep->fileline();
            Ast_Sen_वृक्षः* ध्रुव stp = नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Static{}}};
            Ast_सक्रिय* ध्रुव activep = नूतन Ast_सक्रिय{flp, "", stp};
            activep->sen_वृक्षः_Storep(stp);
            scopep->सङ्कलय_Blocksp(activep);
            प्रत्यागम activep;
        }();
        Ast_सक्रिय* ध्रुव combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            प्रति (Ast_ग्रन्थिः* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_सक्रिय* ध्रुव activep = VN_CAST(nodep, सक्रिय);
                यदि (!activep) अनुवर्त;
                यदि (activep->sentreep()->hasCombo()) प्रत्यागम activep;
            }
            // If there isn't one, create a new one
            File_पङ्क्तिः* ध्रुव flp = scopep->fileline();
            Ast_Sen_वृक्षः* ध्रुव stp = नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
            Ast_सक्रिय* ध्रुव activep = नूतन Ast_सक्रिय{flp, "", stp};
            activep->sen_वृक्षः_Storep(stp);
            scopep->सङ्कलय_Blocksp(activep);
            प्रत्यागम activep;
        }();

        // Required data types
        ध्रुव Dfg_दत्त_Type& idxDType = srcp->dtype();
        ध्रुव Dfg_दत्त_Type& bitDType = Dfg_दत्त_Type::packed(1);
        ध्रुव Dfg_दत्त_Type& tabDType = Dfg_दत्त_Type::array(bitDType, n_द्वयङ्काः);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            यदि (DfgVertexVar* ध्रुव vp = srcp->get_परिणाम_Var()) प्रत्यागम vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ध्रुव vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->is_आन्तरिकम्(सत्य);
            vtxp->srcp(srcp);
            प्रत्यागम vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ध्रुव idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ध्रुव preVscp = [&]() {
            DfgVertexVar* ध्रुव vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ध्रुव vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->is_आन्तरिकम्(सत्य);
            vscp->varp()->noReset(सत्य);
            vscp->varp()->समुच्चयः_Ignore_Sched_Write();
            प्रत्यागम vscp;
        }();
        // The table variable
        DfgVarArray* ध्रुव tabVtxp = [&]() {
            DfgVertexVar* ध्रुव varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->is_आन्तरिकम्(सत्य);
            varp->vscp()->varp()->noReset(सत्य);
            varp->समुच्चयः_Has_Mod_Wr_Refs();
            प्रत्यागम varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ध्रुव initp = नूतन AstInitialStatic{flp, शून्यसूचकः};
        staticActivep->सङ्कलय_Stmtsp(initp);
        {  // pre = 0
            initp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
                flp,  //
                नूतन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                नूतन AstConst{flp, AstConst::Widthed_मानम्{}, static_cast<पूर्णाङ्क>(विस्तारः), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                flp, नूतन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->सङ्कलय_Pinsp(नूतन AstConst{flp, AstConst::BitFalse{}});
            callp->dtype_समुच्चयः_Void();
            initp->सङ्कलय_Stmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ध्रुव logicp = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, शून्यसूचकः, शून्यसूचकः};
        combActivep->सङ्कलय_Stmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
                flp,  //
                नूतन AstArraySel{flp, नूतन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                नूतन AstVarRef{flp, preVscp, VAccess::READ}},  //
                नूतन AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ध्रुव idxRefp = नूतन AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
                flp,  //
                नूतन AstArraySel{flp, नूतन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                नूतन AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* ध्रुव astRdp = नूतन DfgAstRd{dfg, idxRefp, असत्य, असत्य};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ध्रुव idxRefp = नूतन AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp,  //
                                            नूतन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ध्रुव astRdp = नूतन DfgAstRd{dfg, idxRefp, असत्य, असत्य};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        प्रति (ध्रुव स्वचालित& pair : val2Terms) {
            ध्रुव uint32_t val = pair.first;
            ध्रुव std::सदिशम्<पदांशः_2>& terms = pair.second;
            // Create the ArraySel
            File_पङ्क्तिः* ध्रुव aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ध्रुव aselp = नूतन DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(नूतन DfgConst{dfg, aflp, विस्तारः, val});
            // The inverted value, if needed
            DfgNot* notp = शून्यसूचकः;
            // Repalce the terms
            प्रति (ध्रुव पदांशः_2& पदांशः : terms) {
                यदि (पदांशः.m_inv) {
                    यदि (!notp) {
                        notp = नूतन DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    पदांशः.m_vtxp->replaceWith(notp);
                } अन्यथा {
                    पदांशः.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(पदांशः.m_vtxp->unlinkDelete(dfg), पदांशः.m_vtxp);
            }
        }
    }
}
