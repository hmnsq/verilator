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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3DfgPasses.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

শূন্য V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    প্রতি (DfgVertex& vtx : dfg.opVertices()) {
        যদি (vtx.is<DfgLogic>()) workList.ঠেলো_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ধ্রুব logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ধ্রুব তার্কিক used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ধ্রুব uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ধ্রুব vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            যদি (vVtxp->hasSinks()) প্রত্যাগম সত্য;
            যদি (vVtxp->isObserved()) প্রত্যাগম সত্য;
            প্রত্যাগম অসত্য;
        });
        // If some are used, the logic must stay in the Ast
        যদি (used) প্রত্যাগম;
        // If impure, it must stay in the Ast
        যদি (!logicp->isPure()) প্রত্যাগম;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ধ্রুব varp = src.as<DfgVertexVar>();
            যদি (!varp->srcp()) প্রত্যাগম অসত্য;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.ঠেলো_front(*driver.as<DfgLogic>());
                প্রত্যাগম অসত্য;
            });
            প্রত্যাগম অসত্য;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_গ্রন্থি* ধ্রুব nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    প্রতি (DfgVertexVar* ধ্রুব vVtxp : dfg.varVertices().unlinkable()) {
        যদি (vVtxp->hasSinks()) অনুবর্ত;
        যদি (vVtxp->isObserved()) অনুবর্ত;
        DfgVertex* ধ্রুব srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ধ্রুব vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ধ্রুব তার্কিক delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        যদি (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                প্রত্যাগম অসত্য;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        যদি (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_বৃক্ষ(), vscp);
            ++ctx.m_varsDeleted;
        } অন্যথা {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    প্রতি (DfgVertex* ধ্রুব vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ধ্রুব logicp = vtxp->cast<DfgLogic>();
        যদি (!logicp) অনুবর্ত;
        যদি (logicp->hasSinks()) অনুবর্ত;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ধ্রুব VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            প্রত্যাগম অসত্য;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_গ্রন্থি* varp) {  //
            প্রত্যাগম varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

শূন্য V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    প্রতি (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ধ্রুব selp = vtx.cast<DfgSel>();
        যদি (!selp) অনুবর্ত;
        selps.ঠেলো_back(selp);
    }

    প্রতি (DfgSel* ধ্রুব selp : selps) {
        File_সারি* ধ্রুব flp = selp->fileline();
        ধ্রুব DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        যদি (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            অনুবর্ত;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ধ্রুব varp = selp->fromp()->cast<DfgVarPacked>();
        যদি (!varp || !varp->tmpForp()) অনুবর্ত;

        // Find the driver of this range
        ধ্রুব auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* ধ্রুব driverp = pair.first;
        ধ্রুব uint32_t driverLsb = pair.second;
        যদি (!driverp) অনুবর্ত;

        // If partial driver is the whole thing we are looking for, just replace with that
        যদি (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            অনুবর্ত;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ধ্রুব newSelp = নূতন DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

শূন্য V3DfgPasses::inlineVars(DfgGraph& dfg) {
    প্রতি (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        যদি (!vtx.hasSinks()) অনুবর্ত;
        // Nohting to inline
        DfgVertex* ধ্রুব srcp = vtx.srcp();
        যদি (!srcp) অনুবর্ত;
        // Value can differ from driver
        যদি (vtx.isVolatile()) অনুবর্ত;
        // Partial driver cannot be inlined
        যদি (srcp->is<DfgVertexSplice>()) অনুবর্ত;
        যদি (srcp->is<DfgUnitArray>()) অনুবর্ত;
        // Don't inline CReset
        যদি (srcp->is<DfgCReset>()) অনুবর্ত;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

শূন্য V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    প্রতি (DfgVertex& vtx : dfg.opVertices()) {
        যদি (vtx.hasSinks()) অনুবর্ত;
        // This vertex is unused. Add to work list.
        workList.ঠেলো_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    প্রতি (DfgVertexVar& vtx : dfg.varVertices()) {
        যদি (!vtx.tmpForp()) অনুবর্ত;
        যদি (vtx.hasSinks()) অনুবর্ত;
        যদি (vtx.hasDfgRefs()) অনুবর্ত;
        // This vertex is unused. Add to work list.
        workList.ঠেলো_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        যদি (vtx.hasSinks()) প্রত্যাগম;
        // If temporary used in another graph, we need to keep it
        যদি (ধ্রুব DfgVertexVar* ধ্রুব varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            যদি (varp->hasDfgRefs()) প্রত্যাগম;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            যদি (src.is<DfgConst>()) প্রত্যাগম অসত্য;
            ধ্রুব DfgVertexVar* ধ্রুব varp = src.cast<DfgVertexVar>();
            যদি (varp && !varp->tmpForp()) প্রত্যাগম অসত্য;
            // Add source to workList
            workList.ঠেলো_front(src);
            প্রত্যাগম অসত্য;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    প্রতি (DfgVertexVar* ধ্রুব vtxp : dfg.varVertices().unlinkable()) {
        যদি (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    প্রতি (DfgConst* ধ্রুব vtxp : dfg.constVertices().unlinkable()) {
        যদি (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

শূন্য V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    সংরচনা Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        তার্কিক m_inv = অসত্য;  // '!=', instead of '=='
        Term() = স্বভাব;
        Term(DfgVertex* vtxp, তার্কিক inv)
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
    ধ্রুব auto widthOk = [](ধ্রুব DfgVertex* vtxp) {
        ধ্রুব uint32_t width = vtxp->width();
        প্রত্যাগম WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ধ্রুব std::function<তার্কিক(ধ্রুব DfgVertex*, তার্কিক)> useOk
        = [&](ধ্রুব DfgVertex* vtxp, তার্কিক inv) -> তার্কিক {
        // Go past a single 'Not' sink, which is common
        যদি (DfgVertex* ধ্রুব sinkp = vtxp->singleSink()) {
            যদি (sinkp->is<DfgNot>()) প্রত্যাগম useOk(sinkp, !inv);
        }
        ধ্রুব তার্কিক cond_বৃক্ষ = vtxp->foreachSink([&](ধ্রুব DfgVertex& sink) {
            ধ্রুব DfgCond* ধ্রুব condp = sink.cast<DfgCond>();
            যদি (!condp) প্রত্যাগম অসত্য;
            যদি (condp->condp() != vtxp) প্রত্যাগম অসত্য;
            প্রত্যাগম inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        প্রত্যাগম !cond_বৃক্ষ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    প্রতি (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        তার্কিক inv = অসত্য;
        যদি (DfgEq* ধ্রুব eqp = vtx.cast<DfgEq>()) {
            DfgConst* ধ্রুব constp = eqp->lhsp()->cast<DfgConst>();
            যদি (!constp || !widthOk(constp) || !useOk(eqp, অসত্য)) অনুবর্ত;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = অসত্য;
        } অন্যথা যদি (DfgNeq* ধ্রুব neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ধ্রুব constp = neqp->lhsp()->cast<DfgConst>();
            যদি (!constp || !widthOk(constp) || !useOk(neqp, সত্য)) অনুবর্ত;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = সত্য;
        } অন্যথা যদি (DfgRedAnd* ধ্রুব redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            যদি (!widthOk(srcp) || !useOk(redAndp, অসত্য)) অনুবর্ত;
            val = (1U << srcp->width()) - 1;
            inv = অসত্য;
        } অন্যথা যদি (DfgRedOr* ধ্রুব redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            যদি (!widthOk(srcp) || !useOk(redOrp, সত্য)) অনুবর্ত;
            val = 0;
            inv = সত্য;
        } অন্যথা যদি (DfgSel* ধ্রুব selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            যদি (selp->width() != 1) অনুবর্ত;
            DfgShiftL* ধ্রুব shiftLp = selp->fromp()->cast<DfgShiftL>();
            যদি (!shiftLp) অনুবর্ত;
            ধ্রুব DfgConst* ধ্রুব constp = shiftLp->lhsp()->cast<DfgConst>();
            যদি (!constp || !useOk(selp, অসত্য)) অনুবর্ত;
            যদি (!constp->has_মান(1)) অনুবর্ত;
            srcp = shiftLp->rhsp();
            যদি (!widthOk(srcp)) অনুবর্ত;
            val = selp->lsb();
            inv = অসত্য;
        } অন্যথা {
            // Not a comparison-like vertex
            অনুবর্ত;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        যদি (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    যদি (nTerms < TERM_LIMIT) প্রত্যাগম;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    প্রতি (DfgVertex* ধ্রুব srcp : srcps) {
        ধ্রুব Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        যদি (val2Terms.size() < TERM_LIMIT) অনুবর্ত;

        // Width of the decoded binary value
        ধ্রুব uint32_t width = srcp->width();
        // Number of bits in the input operand
        ধ্রুব uint32_t nBits = 1U << width;

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

        File_সারি* ধ্রুব flp = srcp->fileline();
        AstScope* ধ্রুব scopep = srcp->scopep(scopeCache, সত্য);

        AstActive* ধ্রুব staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            প্রতি (Ast_গ্রন্থি* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ধ্রুব activep = VN_CAST(nodep, Active);
                যদি (!activep) অনুবর্ত;
                যদি (activep->sentreep()->hasStatic()) প্রত্যাগম activep;
            }
            // If there isn't one, create a new one
            File_সারি* ধ্রুব flp = scopep->fileline();
            Ast_Sen_বৃক্ষ* ধ্রুব stp = নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* ধ্রুব activep = নূতন AstActive{flp, "", stp};
            activep->sen_বৃক্ষ_Storep(stp);
            scopep->addBlocksp(activep);
            প্রত্যাগম activep;
        }();
        AstActive* ধ্রুব combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            প্রতি (Ast_গ্রন্থি* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ধ্রুব activep = VN_CAST(nodep, Active);
                যদি (!activep) অনুবর্ত;
                যদি (activep->sentreep()->hasCombo()) প্রত্যাগম activep;
            }
            // If there isn't one, create a new one
            File_সারি* ধ্রুব flp = scopep->fileline();
            Ast_Sen_বৃক্ষ* ধ্রুব stp = নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* ধ্রুব activep = নূতন AstActive{flp, "", stp};
            activep->sen_বৃক্ষ_Storep(stp);
            scopep->addBlocksp(activep);
            প্রত্যাগম activep;
        }();

        // Required data types
        ধ্রুব DfgDataType& idxDType = srcp->dtype();
        ধ্রুব DfgDataType& bitDType = DfgDataType::packed(1);
        ধ্রুব DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            যদি (DfgVertexVar* ধ্রুব vp = srcp->getResultVar()) প্রত্যাগম vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ধ্রুব vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(সত্য);
            vtxp->srcp(srcp);
            প্রত্যাগম vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ধ্রুব idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ধ্রুব preVscp = [&]() {
            DfgVertexVar* ধ্রুব vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ধ্রুব vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(সত্য);
            vscp->varp()->noReset(সত্য);
            vscp->varp()->setIgnoreSchedWrite();
            প্রত্যাগম vscp;
        }();
        // The table variable
        DfgVarArray* ধ্রুব tabVtxp = [&]() {
            DfgVertexVar* ধ্রুব varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(সত্য);
            varp->vscp()->varp()->noReset(সত্য);
            varp->setHasModWrRefs();
            প্রত্যাগম varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ধ্রুব initp = নূতন AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(নূতন AstAssign{
                flp,  //
                নূতন AstVarRef{flp, preVscp, VAccess::WRITE},  //
                নূতন AstConst{flp, AstConst::Widthed_মান{}, static_cast<পূর্ণাঙ্ক>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ধ্রুব callp = নূতন AstCMethodHard{
                flp, নূতন AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(নূতন AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ধ্রুব logicp = নূতন AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(নূতন AstAssign{
                flp,  //
                নূতন AstArraySel{flp, নূতন AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                নূতন AstVarRef{flp, preVscp, VAccess::READ}},  //
                নূতন AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ধ্রুব idxRefp = নূতন AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(নূতন AstAssign{
                flp,  //
                নূতন AstArraySel{flp, নূতন AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                নূতন AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* ধ্রুব astRdp = নূতন DfgAstRd{dfg, idxRefp, অসত্য, অসত্য};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ধ্রুব idxRefp = নূতন AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(নূতন AstAssign{flp,  //
                                            নূতন AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ধ্রুব astRdp = নূতন DfgAstRd{dfg, idxRefp, অসত্য, অসত্য};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        প্রতি (ধ্রুব auto& pair : val2Terms) {
            ধ্রুব uint32_t val = pair.first;
            ধ্রুব std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_সারি* ধ্রুব aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ধ্রুব aselp = নূতন DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(নূতন DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            প্রতি (ধ্রুব Term& term : terms) {
                যদি (term.m_inv) {
                    যদি (!notp) {
                        notp = নূতন DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } অন্যথা {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
