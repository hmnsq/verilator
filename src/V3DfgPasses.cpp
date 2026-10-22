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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ଶୂନ୍ୟ V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    ପାଇଁ (DfgVertex& vtx : dfg.opVertices()) {
        ଯଦି (vtx.is<DfgLogic>()) workList.ଠେଲ_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ଅଚଳ logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ଅଚଳ ତାର୍କିକ used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ଅଚଳ uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ଅଚଳ vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            ଯଦି (vVtxp->hasSinks()) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଯଦି (vVtxp->isObserved()) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        // If some are used, the logic must stay in the Ast
        ଯଦି (used) ଫେରନ୍ତୁ;
        // If impure, it must stay in the Ast
        ଯଦି (!logicp->isPure()) ଫେରନ୍ତୁ;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ଅଚଳ varp = src.as<DfgVertexVar>();
            ଯଦି (!varp->srcp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.ଠେଲ_front(*driver.as<DfgLogic>());
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_ଗଣ୍ଠି* ଅଚଳ nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    ପାଇଁ (DfgVertexVar* ଅଚଳ vVtxp : dfg.varVertices().unlinkable()) {
        ଯଦି (vVtxp->hasSinks()) ଜାରି;
        ଯଦି (vVtxp->isObserved()) ଜାରି;
        DfgVertex* ଅଚଳ srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ଅଚଳ vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ଅଚଳ ତାର୍କିକ delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        ଯଦି (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        ଯଦି (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_ବୃକ୍ଷ(), vscp);
            ++ctx.m_varsDeleted;
        } ଅନ୍ୟଥା {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    ପାଇଁ (DfgVertex* ଅଚଳ vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ଅଚଳ logicp = vtxp->cast<DfgLogic>();
        ଯଦି (!logicp) ଜାରି;
        ଯଦି (logicp->hasSinks()) ଜାରି;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ଅଚଳ VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_ଗଣ୍ଠି* varp) {  //
            ଫେରନ୍ତୁ varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

ଶୂନ୍ୟ V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    ପାଇଁ (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ଅଚଳ selp = vtx.cast<DfgSel>();
        ଯଦି (!selp) ଜାରି;
        selps.ଠେଲ_back(selp);
    }

    ପାଇଁ (DfgSel* ଅଚଳ selp : selps) {
        File_ଧାଡ଼ି* ଅଚଳ flp = selp->fileline();
        ଅଚଳ DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        ଯଦି (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ଜାରି;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ଅଚଳ varp = selp->fromp()->cast<DfgVarPacked>();
        ଯଦି (!varp || !varp->tmpForp()) ଜାରି;

        // Find the driver of this range
        ଅଚଳ auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* ଅଚଳ driverp = pair.first;
        ଅଚଳ uint32_t driverLsb = pair.second;
        ଯଦି (!driverp) ଜାରି;

        // If partial driver is the whole thing we are looking for, just replace with that
        ଯଦି (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ଜାରି;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ଅଚଳ newSelp = ନୂତନ DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

ଶୂନ୍ୟ V3DfgPasses::inlineVars(DfgGraph& dfg) {
    ପାଇଁ (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        ଯଦି (!vtx.hasSinks()) ଜାରି;
        // Nohting to inline
        DfgVertex* ଅଚଳ srcp = vtx.srcp();
        ଯଦି (!srcp) ଜାରି;
        // Value can differ from driver
        ଯଦି (vtx.isVolatile()) ଜାରି;
        // Partial driver cannot be inlined
        ଯଦି (srcp->is<DfgVertexSplice>()) ଜାରି;
        ଯଦି (srcp->is<DfgUnitArray>()) ଜାରି;
        // Don't inline CReset
        ଯଦି (srcp->is<DfgCReset>()) ଜାରି;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

ଶୂନ୍ୟ V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    ପାଇଁ (DfgVertex& vtx : dfg.opVertices()) {
        ଯଦି (vtx.hasSinks()) ଜାରି;
        // This vertex is unused. Add to work list.
        workList.ଠେଲ_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    ପାଇଁ (DfgVertexVar& vtx : dfg.varVertices()) {
        ଯଦି (!vtx.tmpForp()) ଜାରି;
        ଯଦି (vtx.hasSinks()) ଜାରି;
        ଯଦି (vtx.hasDfgRefs()) ଜାରି;
        // This vertex is unused. Add to work list.
        workList.ଠେଲ_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        ଯଦି (vtx.hasSinks()) ଫେରନ୍ତୁ;
        // If temporary used in another graph, we need to keep it
        ଯଦି (ଅଚଳ DfgVertexVar* ଅଚଳ varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            ଯଦି (varp->hasDfgRefs()) ଫେରନ୍ତୁ;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            ଯଦି (src.is<DfgConst>()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଅଚଳ DfgVertexVar* ଅଚଳ varp = src.cast<DfgVertexVar>();
            ଯଦି (varp && !varp->tmpForp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Add source to workList
            workList.ଠେଲ_front(src);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    ପାଇଁ (DfgVertexVar* ଅଚଳ vtxp : dfg.varVertices().unlinkable()) {
        ଯଦି (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    ପାଇଁ (DfgConst* ଅଚଳ vtxp : dfg.constVertices().unlinkable()) {
        ଯଦି (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

ଶୂନ୍ୟ V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    ସଂରଚନା Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        ତାର୍କିକ m_inv = ମିଥ୍ୟା;  // '!=', instead of '=='
        Term() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        Term(DfgVertex* vtxp, ତାର୍କିକ inv)
            : m_vtxp{vtxp}
            , m_inv{inv} {}
    };

    // List of vertices that are used as sources
    std::vector<DfgVertex*> srcps;

    // Map from 'vertices' -> 'value beign compared' -> 'terms'
    using Val2Terms = std::map<uint32_t, std::vector<Term>>;
    DfgUserMap<Val2Terms> vtx2Val2Terms = dfg.makeUserMap<Val2Terms>();

    // Only consider input variables from a reasonable range:
    // - not too big to avoid huge tables, you are doomed anyway at that point..
    // - not too small, as it's probably not worth it
    constexpr uint32_t WIDTH_MIN = 7;
    constexpr uint32_t WIDTH_MAX = 20;
    ଅଚଳ auto widthOk = [](ଅଚଳ DfgVertex* vtxp) {
        ଅଚଳ uint32_t width = vtxp->width();
        ଫେରନ୍ତୁ WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ଅଚଳ std::function<ତାର୍କିକ(ଅଚଳ DfgVertex*, ତାର୍କିକ)> useOk
        = [&](ଅଚଳ DfgVertex* vtxp, ତାର୍କିକ inv) -> ତାର୍କିକ {
        // Go past a single 'Not' sink, which is common
        ଯଦି (DfgVertex* ଅଚଳ sinkp = vtxp->singleSink()) {
            ଯଦି (sinkp->is<DfgNot>()) ଫେରନ୍ତୁ useOk(sinkp, !inv);
        }
        ଅଚଳ ତାର୍କିକ cond_ବୃକ୍ଷ = vtxp->foreachSink([&](ଅଚଳ DfgVertex& sink) {
            ଅଚଳ DfgCond* ଅଚଳ condp = sink.cast<DfgCond>();
            ଯଦି (!condp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (condp->condp() != vtxp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଫେରନ୍ତୁ inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        ଫେରନ୍ତୁ !cond_ବୃକ୍ଷ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    ପାଇଁ (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        ତାର୍କିକ inv = ମିଥ୍ୟା;
        ଯଦି (DfgEq* ଅଚଳ eqp = vtx.cast<DfgEq>()) {
            DfgConst* ଅଚଳ constp = eqp->lhsp()->cast<DfgConst>();
            ଯଦି (!constp || !widthOk(constp) || !useOk(eqp, ମିଥ୍ୟା)) ଜାରି;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = ମିଥ୍ୟା;
        } ଅନ୍ୟଥା ଯଦି (DfgNeq* ଅଚଳ neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ଅଚଳ constp = neqp->lhsp()->cast<DfgConst>();
            ଯଦି (!constp || !widthOk(constp) || !useOk(neqp, ସତ୍ୟ)) ଜାରି;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (DfgRedAnd* ଅଚଳ redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            ଯଦି (!widthOk(srcp) || !useOk(redAndp, ମିଥ୍ୟା)) ଜାରି;
            val = (1U << srcp->width()) - 1;
            inv = ମିଥ୍ୟା;
        } ଅନ୍ୟଥା ଯଦି (DfgRedOr* ଅଚଳ redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            ଯଦି (!widthOk(srcp) || !useOk(redOrp, ସତ୍ୟ)) ଜାରି;
            val = 0;
            inv = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (DfgSel* ଅଚଳ selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            ଯଦି (selp->width() != 1) ଜାରି;
            DfgShiftL* ଅଚଳ shiftLp = selp->fromp()->cast<DfgShiftL>();
            ଯଦି (!shiftLp) ଜାରି;
            ଅଚଳ DfgConst* ଅଚଳ constp = shiftLp->lhsp()->cast<DfgConst>();
            ଯଦି (!constp || !useOk(selp, ମିଥ୍ୟା)) ଜାରି;
            ଯଦି (!constp->has_ମୂଲ୍ୟ(1)) ଜାରି;
            srcp = shiftLp->rhsp();
            ଯଦି (!widthOk(srcp)) ଜାରି;
            val = selp->lsb();
            inv = ମିଥ୍ୟା;
        } ଅନ୍ୟଥା {
            // Not a comparison-like vertex
            ଜାରି;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        ଯଦି (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    ଯଦି (nTerms < TERM_LIMIT) ଫେରନ୍ତୁ;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    ପାଇଁ (DfgVertex* ଅଚଳ srcp : srcps) {
        ଅଚଳ Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        ଯଦି (val2Terms.size() < TERM_LIMIT) ଜାରି;

        // Width of the decoded binary value
        ଅଚଳ uint32_t width = srcp->width();
        // Number of bits in the input operand
        ଅଚଳ uint32_t nBits = 1U << width;

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

        File_ଧାଡ଼ି* ଅଚଳ flp = srcp->fileline();
        AstScope* ଅଚଳ scopep = srcp->scopep(scopeCache, ସତ୍ୟ);

        AstActive* ଅଚଳ staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ଅଚଳ activep = VN_CAST(nodep, Active);
                ଯଦି (!activep) ଜାରି;
                ଯଦି (activep->sentreep()->hasStatic()) ଫେରନ୍ତୁ activep;
            }
            // If there isn't one, create a new one
            File_ଧାଡ଼ି* ଅଚଳ flp = scopep->fileline();
            Ast_Sen_ବୃକ୍ଷ* ଅଚଳ stp = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "", stp};
            activep->sen_ବୃକ୍ଷ_Storep(stp);
            scopep->addBlocksp(activep);
            ଫେରନ୍ତୁ activep;
        }();
        AstActive* ଅଚଳ combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ଅଚଳ activep = VN_CAST(nodep, Active);
                ଯଦି (!activep) ଜାରି;
                ଯଦି (activep->sentreep()->hasCombo()) ଫେରନ୍ତୁ activep;
            }
            // If there isn't one, create a new one
            File_ଧାଡ଼ି* ଅଚଳ flp = scopep->fileline();
            Ast_Sen_ବୃକ୍ଷ* ଅଚଳ stp = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "", stp};
            activep->sen_ବୃକ୍ଷ_Storep(stp);
            scopep->addBlocksp(activep);
            ଫେରନ୍ତୁ activep;
        }();

        // Required data types
        ଅଚଳ DfgDataType& idxDType = srcp->dtype();
        ଅଚଳ DfgDataType& bitDType = DfgDataType::packed(1);
        ଅଚଳ DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            ଯଦି (DfgVertexVar* ଅଚଳ vp = srcp->getResultVar()) ଫେରନ୍ତୁ vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ଅଚଳ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(ସତ୍ୟ);
            vtxp->srcp(srcp);
            ଫେରନ୍ତୁ vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ଅଚଳ idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ଅଚଳ preVscp = [&]() {
            DfgVertexVar* ଅଚଳ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ଅଚଳ vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(ସତ୍ୟ);
            vscp->varp()->noReset(ସତ୍ୟ);
            vscp->varp()->setIgnoreSchedWrite();
            ଫେରନ୍ତୁ vscp;
        }();
        // The table variable
        DfgVarArray* ଅଚଳ tabVtxp = [&]() {
            DfgVertexVar* ଅଚଳ varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(ସତ୍ୟ);
            varp->vscp()->varp()->noReset(ସତ୍ୟ);
            varp->setHasModWrRefs();
            ଫେରନ୍ତୁ varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ଅଚଳ initp = ନୂତନ AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(ନୂତନ AstAssign{
                flp,  //
                ନୂତନ AstVarRef{flp, preVscp, VAccess::WRITE},  //
                ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
                flp, ନୂତନ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(ନୂତନ AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ଅଚଳ logicp = ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(ନୂତନ AstAssign{
                flp,  //
                ନୂତନ AstArraySel{flp, ନୂତନ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                ନୂତନ AstVarRef{flp, preVscp, VAccess::READ}},  //
                ନୂତନ AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ଅଚଳ idxRefp = ନୂତନ AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(ନୂତନ AstAssign{
                flp,  //
                ନୂତନ AstArraySel{flp, ନୂତନ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                ନୂତନ AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* ଅଚଳ astRdp = ନୂତନ DfgAstRd{dfg, idxRefp, ମିଥ୍ୟା, ମିଥ୍ୟା};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ଅଚଳ idxRefp = ନୂତନ AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(ନୂତନ AstAssign{flp,  //
                                            ନୂତନ AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ଅଚଳ astRdp = ନୂତନ DfgAstRd{dfg, idxRefp, ମିଥ୍ୟା, ମିଥ୍ୟା};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        ପାଇଁ (ଅଚଳ auto& pair : val2Terms) {
            ଅଚଳ uint32_t val = pair.first;
            ଅଚଳ std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_ଧାଡ଼ି* ଅଚଳ aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ଅଚଳ aselp = ନୂତନ DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(ନୂତନ DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            ପାଇଁ (ଅଚଳ Term& term : terms) {
                ଯଦି (term.m_inv) {
                    ଯଦି (!notp) {
                        notp = ନୂତନ DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } ଅନ୍ୟଥା {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
