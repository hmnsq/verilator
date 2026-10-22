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
    জন্য (DfgVertex& vtx : dfg.opVertices()) {
        যদি (vtx.is<DfgLogic>()) workList.ঠেলো_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ধ্রুবক logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ধ্রুবক যৌক্তিক used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ধ্রুবক uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ধ্রুবক vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            যদি (vVtxp->hasSinks()) ফেরত সত্য;
            যদি (vVtxp->isObserved()) ফেরত সত্য;
            ফেরত মিথ্যা;
        });
        // If some are used, the logic must stay in the Ast
        যদি (used) ফেরত;
        // If impure, it must stay in the Ast
        যদি (!logicp->isPure()) ফেরত;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ধ্রুবক varp = src.as<DfgVertexVar>();
            যদি (!varp->srcp()) ফেরত মিথ্যা;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.ঠেলো_front(*driver.as<DfgLogic>());
                ফেরত মিথ্যা;
            });
            ফেরত মিথ্যা;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_গ্রন্থি* ধ্রুবক nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    জন্য (DfgVertexVar* ধ্রুবক vVtxp : dfg.varVertices().unlinkable()) {
        যদি (vVtxp->hasSinks()) চালু;
        যদি (vVtxp->isObserved()) চালু;
        DfgVertex* ধ্রুবক srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ধ্রুবক vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ধ্রুবক যৌক্তিক delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        যদি (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                ফেরত মিথ্যা;
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
    জন্য (DfgVertex* ধ্রুবক vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ধ্রুবক logicp = vtxp->cast<DfgLogic>();
        যদি (!logicp) চালু;
        যদি (logicp->hasSinks()) চালু;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ধ্রুবক VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            ফেরত মিথ্যা;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_গ্রন্থি* varp) {  //
            ফেরত varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

শূন্য V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    জন্য (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ধ্রুবক selp = vtx.cast<DfgSel>();
        যদি (!selp) চালু;
        selps.ঠেলো_back(selp);
    }

    জন্য (DfgSel* ধ্রুবক selp : selps) {
        File_সারি* ধ্রুবক flp = selp->fileline();
        ধ্রুবক Dfg_তথ্য_Type& dtype = selp->dtype();

        // Remove full width selects
        যদি (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            চালু;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ধ্রুবক varp = selp->fromp()->cast<DfgVarPacked>();
        যদি (!varp || !varp->tmpForp()) চালু;

        // Find the driver of this range
        ধ্রুবক auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* ধ্রুবক driverp = pair.first;
        ধ্রুবক uint32_t driverLsb = pair.second;
        যদি (!driverp) চালু;

        // If partial driver is the whole thing we are looking for, just replace with that
        যদি (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            চালু;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ধ্রুবক newSelp = নতুন DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

শূন্য V3DfgPasses::inlineVars(DfgGraph& dfg) {
    জন্য (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        যদি (!vtx.hasSinks()) চালু;
        // Nohting to inline
        DfgVertex* ধ্রুবক srcp = vtx.srcp();
        যদি (!srcp) চালু;
        // Value can differ from driver
        যদি (vtx.isVolatile()) চালু;
        // Partial driver cannot be inlined
        যদি (srcp->is<DfgVertexSplice>()) চালু;
        যদি (srcp->is<DfgUnitArray>()) চালু;
        // Don't inline CReset
        যদি (srcp->is<DfgCReset>()) চালু;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

শূন্য V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    জন্য (DfgVertex& vtx : dfg.opVertices()) {
        যদি (vtx.hasSinks()) চালু;
        // This vertex is unused. Add to work list.
        workList.ঠেলো_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    জন্য (DfgVertexVar& vtx : dfg.varVertices()) {
        যদি (!vtx.tmpForp()) চালু;
        যদি (vtx.hasSinks()) চালু;
        যদি (vtx.hasDfgRefs()) চালু;
        // This vertex is unused. Add to work list.
        workList.ঠেলো_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        যদি (vtx.hasSinks()) ফেরত;
        // If temporary used in another graph, we need to keep it
        যদি (ধ্রুবক DfgVertexVar* ধ্রুবক varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            যদি (varp->hasDfgRefs()) ফেরত;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            যদি (src.is<DfgConst>()) ফেরত মিথ্যা;
            ধ্রুবক DfgVertexVar* ধ্রুবক varp = src.cast<DfgVertexVar>();
            যদি (varp && !varp->tmpForp()) ফেরত মিথ্যা;
            // Add source to workList
            workList.ঠেলো_front(src);
            ফেরত মিথ্যা;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    জন্য (DfgVertexVar* ধ্রুবক vtxp : dfg.varVertices().unlinkable()) {
        যদি (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    জন্য (DfgConst* ধ্রুবক vtxp : dfg.constVertices().unlinkable()) {
        যদি (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

শূন্য V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    কাঠামো Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        যৌক্তিক m_inv = মিথ্যা;  // '!=', instead of '=='
        Term() = পূর্বনির্ধারিত;
        Term(DfgVertex* vtxp, যৌক্তিক inv)
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
    ধ্রুবক auto widthOk = [](ধ্রুবক DfgVertex* vtxp) {
        ধ্রুবক uint32_t width = vtxp->width();
        ফেরত WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ধ্রুবক std::function<যৌক্তিক(ধ্রুবক DfgVertex*, যৌক্তিক)> useOk
        = [&](ধ্রুবক DfgVertex* vtxp, যৌক্তিক inv) -> যৌক্তিক {
        // Go past a single 'Not' sink, which is common
        যদি (DfgVertex* ধ্রুবক sinkp = vtxp->singleSink()) {
            যদি (sinkp->is<DfgNot>()) ফেরত useOk(sinkp, !inv);
        }
        ধ্রুবক যৌক্তিক cond_বৃক্ষ = vtxp->foreachSink([&](ধ্রুবক DfgVertex& sink) {
            ধ্রুবক DfgCond* ধ্রুবক condp = sink.cast<DfgCond>();
            যদি (!condp) ফেরত মিথ্যা;
            যদি (condp->condp() != vtxp) ফেরত মিথ্যা;
            ফেরত inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        ফেরত !cond_বৃক্ষ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    জন্য (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        যৌক্তিক inv = মিথ্যা;
        যদি (DfgEq* ধ্রুবক eqp = vtx.cast<DfgEq>()) {
            DfgConst* ধ্রুবক constp = eqp->lhsp()->cast<DfgConst>();
            যদি (!constp || !widthOk(constp) || !useOk(eqp, মিথ্যা)) চালু;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = মিথ্যা;
        } অন্যথা যদি (DfgNeq* ধ্রুবক neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ধ্রুবক constp = neqp->lhsp()->cast<DfgConst>();
            যদি (!constp || !widthOk(constp) || !useOk(neqp, সত্য)) চালু;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = সত্য;
        } অন্যথা যদি (DfgRedAnd* ধ্রুবক redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            যদি (!widthOk(srcp) || !useOk(redAndp, মিথ্যা)) চালু;
            val = (1U << srcp->width()) - 1;
            inv = মিথ্যা;
        } অন্যথা যদি (DfgRedOr* ধ্রুবক redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            যদি (!widthOk(srcp) || !useOk(redOrp, সত্য)) চালু;
            val = 0;
            inv = সত্য;
        } অন্যথা যদি (DfgSel* ধ্রুবক selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            যদি (selp->width() != 1) চালু;
            DfgShiftL* ধ্রুবক shiftLp = selp->fromp()->cast<DfgShiftL>();
            যদি (!shiftLp) চালু;
            ধ্রুবক DfgConst* ধ্রুবক constp = shiftLp->lhsp()->cast<DfgConst>();
            যদি (!constp || !useOk(selp, মিথ্যা)) চালু;
            যদি (!constp->has_মান(1)) চালু;
            srcp = shiftLp->rhsp();
            যদি (!widthOk(srcp)) চালু;
            val = selp->lsb();
            inv = মিথ্যা;
        } অন্যথা {
            // Not a comparison-like vertex
            চালু;
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
    যদি (nTerms < TERM_LIMIT) ফেরত;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    জন্য (DfgVertex* ধ্রুবক srcp : srcps) {
        ধ্রুবক Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        যদি (val2Terms.আকার_2() < TERM_LIMIT) চালু;

        // Width of the decoded binary value
        ধ্রুবক uint32_t width = srcp->width();
        // Number of bits in the input operand
        ধ্রুবক uint32_t nBits = 1U << width;

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

        File_সারি* ধ্রুবক flp = srcp->fileline();
        AstScope* ধ্রুবক scopep = srcp->scopep(scopeCache, সত্য);

        AstActive* ধ্রুবক staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            জন্য (Ast_গ্রন্থি* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ধ্রুবক activep = VN_CAST(nodep, Active);
                যদি (!activep) চালু;
                যদি (activep->sentreep()->hasStatic()) ফেরত activep;
            }
            // If there isn't one, create a new one
            File_সারি* ধ্রুবক flp = scopep->fileline();
            Ast_Sen_বৃক্ষ* ধ্রুবক stp = নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* ধ্রুবক activep = নতুন AstActive{flp, "", stp};
            activep->sen_বৃক্ষ_Storep(stp);
            scopep->addBlocksp(activep);
            ফেরত activep;
        }();
        AstActive* ধ্রুবক combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            জন্য (Ast_গ্রন্থি* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ধ্রুবক activep = VN_CAST(nodep, Active);
                যদি (!activep) চালু;
                যদি (activep->sentreep()->hasCombo()) ফেরত activep;
            }
            // If there isn't one, create a new one
            File_সারি* ধ্রুবক flp = scopep->fileline();
            Ast_Sen_বৃক্ষ* ধ্রুবক stp = নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* ধ্রুবক activep = নতুন AstActive{flp, "", stp};
            activep->sen_বৃক্ষ_Storep(stp);
            scopep->addBlocksp(activep);
            ফেরত activep;
        }();

        // Required data types
        ধ্রুবক Dfg_তথ্য_Type& idxDType = srcp->dtype();
        ধ্রুবক Dfg_তথ্য_Type& bitDType = Dfg_তথ্য_Type::packed(1);
        ধ্রুবক Dfg_তথ্য_Type& tabDType = Dfg_তথ্য_Type::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            যদি (DfgVertexVar* ধ্রুবক vp = srcp->get_ফলাফল_Var()) ফেরত vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ধ্রুবক vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(সত্য);
            vtxp->srcp(srcp);
            ফেরত vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ধ্রুবক idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ধ্রুবক preVscp = [&]() {
            DfgVertexVar* ধ্রুবক vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ধ্রুবক vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(সত্য);
            vscp->varp()->noReset(সত্য);
            vscp->varp()->setIgnoreSchedWrite();
            ফেরত vscp;
        }();
        // The table variable
        DfgVarArray* ধ্রুবক tabVtxp = [&]() {
            DfgVertexVar* ধ্রুবক varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(সত্য);
            varp->vscp()->varp()->noReset(সত্য);
            varp->setHasModWrRefs();
            ফেরত varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ধ্রুবক initp = নতুন AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(নতুন AstAssign{
                flp,  //
                নতুন AstVarRef{flp, preVscp, VAccess::WRITE},  //
                নতুন AstConst{flp, AstConst::Widthed_মান{}, static_cast<পূর্ণসংখ্যা>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ধ্রুবক callp = নতুন AstCMethodHard{
                flp, নতুন AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(নতুন AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ধ্রুবক logicp = নতুন AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(নতুন AstAssign{
                flp,  //
                নতুন AstArraySel{flp, নতুন AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                নতুন AstVarRef{flp, preVscp, VAccess::READ}},  //
                নতুন AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ধ্রুবক idxRefp = নতুন AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(নতুন AstAssign{
                flp,  //
                নতুন AstArraySel{flp, নতুন AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                নতুন AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* ধ্রুবক astRdp = নতুন DfgAstRd{dfg, idxRefp, মিথ্যা, মিথ্যা};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ধ্রুবক idxRefp = নতুন AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(নতুন AstAssign{flp,  //
                                            নতুন AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ধ্রুবক astRdp = নতুন DfgAstRd{dfg, idxRefp, মিথ্যা, মিথ্যা};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        জন্য (ধ্রুবক auto& pair : val2Terms) {
            ধ্রুবক uint32_t val = pair.first;
            ধ্রুবক std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_সারি* ধ্রুবক aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ধ্রুবক aselp = নতুন DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(নতুন DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            জন্য (ধ্রুবক Term& term : terms) {
                যদি (term.m_inv) {
                    যদি (!notp) {
                        notp = নতুন DfgNot{dfg, aflp, bitDType};
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
