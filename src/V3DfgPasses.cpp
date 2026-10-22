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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3DfgPasses.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Dfg.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

শূন্য V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    বাবে (DfgVertex& vtx : dfg.opVertices()) {
        যদি (vtx.is<DfgLogic>()) workList.ঠেলো_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ধ্রুৱ logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ধ্রুৱ তার্কিক used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ধ্রুৱ uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ধ্রুৱ vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            যদি (vVtxp->hasSinks()) ঘূৰাই সঁচা;
            যদি (vVtxp->isObserved()) ঘূৰাই সঁচা;
            ঘূৰাই মিছা;
        });
        // If some are used, the logic must stay in the Ast
        যদি (used) ঘূৰাই;
        // If impure, it must stay in the Ast
        যদি (!logicp->isPure()) ঘূৰাই;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ধ্রুৱ varp = src.as<DfgVertexVar>();
            যদি (!varp->srcp()) ঘূৰাই মিছা;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.ঠেলো_front(*driver.as<DfgLogic>());
                ঘূৰাই মিছা;
            });
            ঘূৰাই মিছা;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_গ্ৰন্থি* ধ্রুৱ nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    বাবে (DfgVertexVar* ধ্রুৱ vVtxp : dfg.varVertices().unlinkable()) {
        যদি (vVtxp->hasSinks()) অনুৱর্ত;
        যদি (vVtxp->isObserved()) অনুৱর্ত;
        DfgVertex* ধ্রুৱ srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ধ্রুৱ vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ধ্রুৱ তার্কিক delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        যদি (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                ঘূৰাই মিছা;
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
    বাবে (DfgVertex* ধ্রুৱ vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ধ্রুৱ logicp = vtxp->cast<DfgLogic>();
        যদি (!logicp) অনুৱর্ত;
        যদি (logicp->hasSinks()) অনুৱর্ত;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ধ্রুৱ VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            ঘূৰাই মিছা;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_গ্ৰন্থি* varp) {  //
            ঘূৰাই varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

শূন্য V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    বাবে (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ধ্রুৱ selp = vtx.cast<DfgSel>();
        যদি (!selp) অনুৱর্ত;
        selps.ঠেলো_back(selp);
    }

    বাবে (DfgSel* ধ্রুৱ selp : selps) {
        File_সাৰি* ধ্রুৱ flp = selp->fileline();
        ধ্রুৱ DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        যদি (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            অনুৱর্ত;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ধ্রুৱ varp = selp->fromp()->cast<DfgVarPacked>();
        যদি (!varp || !varp->tmpForp()) অনুৱর্ত;

        // Find the driver of this range
        ধ্রুৱ auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* ধ্রুৱ driverp = pair.first;
        ধ্রুৱ uint32_t driverLsb = pair.second;
        যদি (!driverp) অনুৱর্ত;

        // If partial driver is the whole thing we are looking for, just replace with that
        যদি (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            অনুৱর্ত;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ধ্রুৱ newSelp = নূতন DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

শূন্য V3DfgPasses::inlineVars(DfgGraph& dfg) {
    বাবে (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        যদি (!vtx.hasSinks()) অনুৱর্ত;
        // Nohting to inline
        DfgVertex* ধ্রুৱ srcp = vtx.srcp();
        যদি (!srcp) অনুৱর্ত;
        // Value can differ from driver
        যদি (vtx.isVolatile()) অনুৱর্ত;
        // Partial driver cannot be inlined
        যদি (srcp->is<DfgVertexSplice>()) অনুৱর্ত;
        যদি (srcp->is<DfgUnitArray>()) অনুৱর্ত;
        // Don't inline CReset
        যদি (srcp->is<DfgCReset>()) অনুৱর্ত;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

শূন্য V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    বাবে (DfgVertex& vtx : dfg.opVertices()) {
        যদি (vtx.hasSinks()) অনুৱর্ত;
        // This vertex is unused. Add to work list.
        workList.ঠেলো_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    বাবে (DfgVertexVar& vtx : dfg.varVertices()) {
        যদি (!vtx.tmpForp()) অনুৱর্ত;
        যদি (vtx.hasSinks()) অনুৱর্ত;
        যদি (vtx.hasDfgRefs()) অনুৱর্ত;
        // This vertex is unused. Add to work list.
        workList.ঠেলো_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        যদি (vtx.hasSinks()) ঘূৰাই;
        // If temporary used in another graph, we need to keep it
        যদি (ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            যদি (varp->hasDfgRefs()) ঘূৰাই;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            যদি (src.is<DfgConst>()) ঘূৰাই মিছা;
            ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = src.cast<DfgVertexVar>();
            যদি (varp && !varp->tmpForp()) ঘূৰাই মিছা;
            // Add source to workList
            workList.ঠেলো_front(src);
            ঘূৰাই মিছা;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    বাবে (DfgVertexVar* ধ্রুৱ vtxp : dfg.varVertices().unlinkable()) {
        যদি (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    বাবে (DfgConst* ধ্রুৱ vtxp : dfg.constVertices().unlinkable()) {
        যদি (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

শূন্য V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    সংরচনা Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        তার্কিক m_inv = মিছা;  // '!=', instead of '=='
        Term() = স্ৱভাৱ;
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
    ধ্রুৱ auto widthOk = [](ধ্রুৱ DfgVertex* vtxp) {
        ধ্রুৱ uint32_t width = vtxp->width();
        ঘূৰাই WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ধ্রুৱ std::function<তার্কিক(ধ্রুৱ DfgVertex*, তার্কিক)> useOk
        = [&](ধ্রুৱ DfgVertex* vtxp, তার্কিক inv) -> তার্কিক {
        // Go past a single 'Not' sink, which is common
        যদি (DfgVertex* ধ্রুৱ sinkp = vtxp->singleSink()) {
            যদি (sinkp->is<DfgNot>()) ঘূৰাই useOk(sinkp, !inv);
        }
        ধ্রুৱ তার্কিক cond_বৃক্ষ = vtxp->foreachSink([&](ধ্রুৱ DfgVertex& sink) {
            ধ্রুৱ DfgCond* ধ্রুৱ condp = sink.cast<DfgCond>();
            যদি (!condp) ঘূৰাই মিছা;
            যদি (condp->condp() != vtxp) ঘূৰাই মিছা;
            ঘূৰাই inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        ঘূৰাই !cond_বৃক্ষ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    বাবে (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        তার্কিক inv = মিছা;
        যদি (DfgEq* ধ্রুৱ eqp = vtx.cast<DfgEq>()) {
            DfgConst* ধ্রুৱ constp = eqp->lhsp()->cast<DfgConst>();
            যদি (!constp || !widthOk(constp) || !useOk(eqp, মিছা)) অনুৱর্ত;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = মিছা;
        } অন্যথা যদি (DfgNeq* ধ্রুৱ neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ধ্রুৱ constp = neqp->lhsp()->cast<DfgConst>();
            যদি (!constp || !widthOk(constp) || !useOk(neqp, সঁচা)) অনুৱর্ত;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = সঁচা;
        } অন্যথা যদি (DfgRedAnd* ধ্রুৱ redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            যদি (!widthOk(srcp) || !useOk(redAndp, মিছা)) অনুৱর্ত;
            val = (1U << srcp->width()) - 1;
            inv = মিছা;
        } অন্যথা যদি (DfgRedOr* ধ্রুৱ redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            যদি (!widthOk(srcp) || !useOk(redOrp, সঁচা)) অনুৱর্ত;
            val = 0;
            inv = সঁচা;
        } অন্যথা যদি (DfgSel* ধ্রুৱ selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            যদি (selp->width() != 1) অনুৱর্ত;
            DfgShiftL* ধ্রুৱ shiftLp = selp->fromp()->cast<DfgShiftL>();
            যদি (!shiftLp) অনুৱর্ত;
            ধ্রুৱ DfgConst* ধ্রুৱ constp = shiftLp->lhsp()->cast<DfgConst>();
            যদি (!constp || !useOk(selp, মিছা)) অনুৱর্ত;
            যদি (!constp->has_মান(1)) অনুৱর্ত;
            srcp = shiftLp->rhsp();
            যদি (!widthOk(srcp)) অনুৱর্ত;
            val = selp->lsb();
            inv = মিছা;
        } অন্যথা {
            // Not a comparison-like vertex
            অনুৱর্ত;
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
    যদি (nTerms < TERM_LIMIT) ঘূৰাই;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    বাবে (DfgVertex* ধ্রুৱ srcp : srcps) {
        ধ্রুৱ Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        যদি (val2Terms.size() < TERM_LIMIT) অনুৱর্ত;

        // Width of the decoded binary value
        ধ্রুৱ uint32_t width = srcp->width();
        // Number of bits in the input operand
        ধ্রুৱ uint32_t nBits = 1U << width;

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

        File_সাৰি* ধ্রুৱ flp = srcp->fileline();
        AstScope* ধ্রুৱ scopep = srcp->scopep(scopeCache, সঁচা);

        AstActive* ধ্রুৱ staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            বাবে (Ast_গ্ৰন্থি* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ধ্রুৱ activep = VN_CAST(nodep, Active);
                যদি (!activep) অনুৱর্ত;
                যদি (activep->sentreep()->hasStatic()) ঘূৰাই activep;
            }
            // If there isn't one, create a new one
            File_সাৰি* ধ্রুৱ flp = scopep->fileline();
            Ast_Sen_বৃক্ষ* ধ্রুৱ stp = নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* ধ্রুৱ activep = নূতন AstActive{flp, "", stp};
            activep->sen_বৃক্ষ_Storep(stp);
            scopep->addBlocksp(activep);
            ঘূৰাই activep;
        }();
        AstActive* ধ্রুৱ combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            বাবে (Ast_গ্ৰন্থি* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ধ্রুৱ activep = VN_CAST(nodep, Active);
                যদি (!activep) অনুৱর্ত;
                যদি (activep->sentreep()->hasCombo()) ঘূৰাই activep;
            }
            // If there isn't one, create a new one
            File_সাৰি* ধ্রুৱ flp = scopep->fileline();
            Ast_Sen_বৃক্ষ* ধ্রুৱ stp = নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* ধ্রুৱ activep = নূতন AstActive{flp, "", stp};
            activep->sen_বৃক্ষ_Storep(stp);
            scopep->addBlocksp(activep);
            ঘূৰাই activep;
        }();

        // Required data types
        ধ্রুৱ DfgDataType& idxDType = srcp->dtype();
        ধ্রুৱ DfgDataType& bitDType = DfgDataType::packed(1);
        ধ্রুৱ DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            যদি (DfgVertexVar* ধ্রুৱ vp = srcp->getResultVar()) ঘূৰাই vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ধ্রুৱ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(সঁচা);
            vtxp->srcp(srcp);
            ঘূৰাই vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ধ্রুৱ idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ধ্রুৱ preVscp = [&]() {
            DfgVertexVar* ধ্রুৱ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ধ্রুৱ vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(সঁচা);
            vscp->varp()->noReset(সঁচা);
            vscp->varp()->setIgnoreSchedWrite();
            ঘূৰাই vscp;
        }();
        // The table variable
        DfgVarArray* ধ্রুৱ tabVtxp = [&]() {
            DfgVertexVar* ধ্রুৱ varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(সঁচা);
            varp->vscp()->varp()->noReset(সঁচা);
            varp->setHasModWrRefs();
            ঘূৰাই varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ধ্রুৱ initp = নূতন AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(নূতন AstAssign{
                flp,  //
                নূতন AstVarRef{flp, preVscp, VAccess::WRITE},  //
                নূতন AstConst{flp, AstConst::Widthed_মান{}, static_cast<পূর্ণাঙ্ক>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ধ্রুৱ callp = নূতন AstCMethodHard{
                flp, নূতন AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(নূতন AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ধ্রুৱ logicp = নূতন AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(নূতন AstAssign{
                flp,  //
                নূতন AstArraySel{flp, নূতন AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                নূতন AstVarRef{flp, preVscp, VAccess::READ}},  //
                নূতন AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ধ্রুৱ idxRefp = নূতন AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(নূতন AstAssign{
                flp,  //
                নূতন AstArraySel{flp, নূতন AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                নূতন AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* ধ্রুৱ astRdp = নূতন DfgAstRd{dfg, idxRefp, মিছা, মিছা};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ধ্রুৱ idxRefp = নূতন AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(নূতন AstAssign{flp,  //
                                            নূতন AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ধ্রুৱ astRdp = নূতন DfgAstRd{dfg, idxRefp, মিছা, মিছা};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        বাবে (ধ্রুৱ auto& pair : val2Terms) {
            ধ্রুৱ uint32_t val = pair.first;
            ধ্রুৱ std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_সাৰি* ধ্রুৱ aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ধ্রুৱ aselp = নূতন DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(নূতন DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            বাবে (ধ্রুৱ Term& term : terms) {
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
