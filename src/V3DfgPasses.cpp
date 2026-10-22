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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3DfgPasses.h"

#ضمن "V3Ast.h"
#ضمن "V3Dfg.h"
#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

فراغ V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    لكل (DfgVertex& vtx : dfg.opVertices()) {
        إذا (vtx.is<DfgLogic>()) workList.ئىتتىرىش_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ثابت logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ثابت منطقي used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ثابت uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ثابت vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            إذا (vVtxp->hasSinks()) أرجع true;
            إذا (vVtxp->isObserved()) أرجع true;
            أرجع خطأ;
        });
        // If some are used, the logic must stay in the Ast
        إذا (used) أرجع;
        // If impure, it must stay in the Ast
        إذا (!logicp->isPure()) أرجع;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ثابت varp = src.as<DfgVertexVar>();
            إذا (!varp->srcp()) أرجع خطأ;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.ئىتتىرىش_front(*driver.as<DfgLogic>());
                أرجع خطأ;
            });
            أرجع خطأ;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_تۈگۈن* ثابت nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_دەرەخ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    لكل (DfgVertexVar* ثابت vVtxp : dfg.varVertices().unlinkable()) {
        إذا (vVtxp->hasSinks()) استمر;
        إذا (vVtxp->isObserved()) استمر;
        DfgVertex* ثابت srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ثابت vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ثابت منطقي delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        إذا (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                أرجع خطأ;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        إذا (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_دەرەخ(), vscp);
            ++ctx.m_varsDeleted;
        } وإلا {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    لكل (DfgVertex* ثابت vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
        إذا (!logicp) استمر;
        إذا (logicp->hasSinks()) استمر;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ثابت VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            أرجع خطأ;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_تۈگۈن* varp) {  //
            أرجع varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

فراغ V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    لكل (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ثابت selp = vtx.cast<DfgSel>();
        إذا (!selp) استمر;
        selps.ئىتتىرىش_back(selp);
    }

    لكل (DfgSel* ثابت selp : selps) {
        File_قۇر* ثابت flp = selp->fileline();
        ثابت DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        إذا (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            استمر;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ثابت varp = selp->fromp()->cast<DfgVarPacked>();
        إذا (!varp || !varp->tmpForp()) استمر;

        // Find the driver of this range
        ثابت auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* ثابت driverp = pair.first;
        ثابت uint32_t driverLsb = pair.second;
        إذا (!driverp) استمر;

        // If partial driver is the whole thing we are looking for, just replace with that
        إذا (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            استمر;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ثابت newSelp = جديد DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

فراغ V3DfgPasses::inlineVars(DfgGraph& dfg) {
    لكل (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        إذا (!vtx.hasSinks()) استمر;
        // Nohting to inline
        DfgVertex* ثابت srcp = vtx.srcp();
        إذا (!srcp) استمر;
        // Value can differ from driver
        إذا (vtx.isVolatile()) استمر;
        // Partial driver cannot be inlined
        إذا (srcp->is<DfgVertexSplice>()) استمر;
        إذا (srcp->is<DfgUnitArray>()) استمر;
        // Don't inline CReset
        إذا (srcp->is<DfgCReset>()) استمر;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

فراغ V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    لكل (DfgVertex& vtx : dfg.opVertices()) {
        إذا (vtx.hasSinks()) استمر;
        // This vertex is unused. Add to work list.
        workList.ئىتتىرىش_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    لكل (DfgVertexVar& vtx : dfg.varVertices()) {
        إذا (!vtx.tmpForp()) استمر;
        إذا (vtx.hasSinks()) استمر;
        إذا (vtx.hasDfgRefs()) استمر;
        // This vertex is unused. Add to work list.
        workList.ئىتتىرىش_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        إذا (vtx.hasSinks()) أرجع;
        // If temporary used in another graph, we need to keep it
        إذا (ثابت DfgVertexVar* ثابت varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            إذا (varp->hasDfgRefs()) أرجع;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            إذا (src.is<DfgConst>()) أرجع خطأ;
            ثابت DfgVertexVar* ثابت varp = src.cast<DfgVertexVar>();
            إذا (varp && !varp->tmpForp()) أرجع خطأ;
            // Add source to workList
            workList.ئىتتىرىش_front(src);
            أرجع خطأ;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    لكل (DfgVertexVar* ثابت vtxp : dfg.varVertices().unlinkable()) {
        إذا (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    لكل (DfgConst* ثابت vtxp : dfg.constVertices().unlinkable()) {
        إذا (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

فراغ V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    بنية Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        منطقي m_inv = خطأ;  // '!=', instead of '=='
        Term() = افتراضي;
        Term(DfgVertex* vtxp, منطقي inv)
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
    ثابت auto widthOk = [](ثابت DfgVertex* vtxp) {
        ثابت uint32_t width = vtxp->width();
        أرجع WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ثابت std::function<منطقي(ثابت DfgVertex*, منطقي)> useOk
        = [&](ثابت DfgVertex* vtxp, منطقي inv) -> منطقي {
        // Go past a single 'Not' sink, which is common
        إذا (DfgVertex* ثابت sinkp = vtxp->singleSink()) {
            إذا (sinkp->is<DfgNot>()) أرجع useOk(sinkp, !inv);
        }
        ثابت منطقي cond_دەرەخ = vtxp->foreachSink([&](ثابت DfgVertex& sink) {
            ثابت DfgCond* ثابت condp = sink.cast<DfgCond>();
            إذا (!condp) أرجع خطأ;
            إذا (condp->condp() != vtxp) أرجع خطأ;
            أرجع inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        أرجع !cond_دەرەخ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    لكل (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        منطقي inv = خطأ;
        إذا (DfgEq* ثابت eqp = vtx.cast<DfgEq>()) {
            DfgConst* ثابت constp = eqp->lhsp()->cast<DfgConst>();
            إذا (!constp || !widthOk(constp) || !useOk(eqp, خطأ)) استمر;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = خطأ;
        } وإلا إذا (DfgNeq* ثابت neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ثابت constp = neqp->lhsp()->cast<DfgConst>();
            إذا (!constp || !widthOk(constp) || !useOk(neqp, true)) استمر;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = true;
        } وإلا إذا (DfgRedAnd* ثابت redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            إذا (!widthOk(srcp) || !useOk(redAndp, خطأ)) استمر;
            val = (1U << srcp->width()) - 1;
            inv = خطأ;
        } وإلا إذا (DfgRedOr* ثابت redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            إذا (!widthOk(srcp) || !useOk(redOrp, true)) استمر;
            val = 0;
            inv = true;
        } وإلا إذا (DfgSel* ثابت selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            إذا (selp->width() != 1) استمر;
            DfgShiftL* ثابت shiftLp = selp->fromp()->cast<DfgShiftL>();
            إذا (!shiftLp) استمر;
            ثابت DfgConst* ثابت constp = shiftLp->lhsp()->cast<DfgConst>();
            إذا (!constp || !useOk(selp, خطأ)) استمر;
            إذا (!constp->has_قىممەت(1)) استمر;
            srcp = shiftLp->rhsp();
            إذا (!widthOk(srcp)) استمر;
            val = selp->lsb();
            inv = خطأ;
        } وإلا {
            // Not a comparison-like vertex
            استمر;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        إذا (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    إذا (nTerms < TERM_LIMIT) أرجع;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    لكل (DfgVertex* ثابت srcp : srcps) {
        ثابت Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        إذا (val2Terms.size() < TERM_LIMIT) استمر;

        // Width of the decoded binary value
        ثابت uint32_t width = srcp->width();
        // Number of bits in the input operand
        ثابت uint32_t nBits = 1U << width;

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

        File_قۇر* ثابت flp = srcp->fileline();
        AstScope* ثابت scopep = srcp->scopep(scopeCache, true);

        AstActive* ثابت staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            لكل (Ast_تۈگۈن* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ثابت activep = VN_CAST(nodep, Active);
                إذا (!activep) استمر;
                إذا (activep->sentreep()->hasStatic()) أرجع activep;
            }
            // If there isn't one, create a new one
            File_قۇر* ثابت flp = scopep->fileline();
            Ast_Sen_دەرەخ* ثابت stp = جديد Ast_Sen_دەرەخ{flp, جديد AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* ثابت activep = جديد AstActive{flp, "", stp};
            activep->sen_دەرەخ_Storep(stp);
            scopep->addBlocksp(activep);
            أرجع activep;
        }();
        AstActive* ثابت combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            لكل (Ast_تۈگۈن* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ثابت activep = VN_CAST(nodep, Active);
                إذا (!activep) استمر;
                إذا (activep->sentreep()->hasCombo()) أرجع activep;
            }
            // If there isn't one, create a new one
            File_قۇر* ثابت flp = scopep->fileline();
            Ast_Sen_دەرەخ* ثابت stp = جديد Ast_Sen_دەرەخ{flp, جديد AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* ثابت activep = جديد AstActive{flp, "", stp};
            activep->sen_دەرەخ_Storep(stp);
            scopep->addBlocksp(activep);
            أرجع activep;
        }();

        // Required data types
        ثابت DfgDataType& idxDType = srcp->dtype();
        ثابت DfgDataType& bitDType = DfgDataType::packed(1);
        ثابت DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            إذا (DfgVertexVar* ثابت vp = srcp->getResultVar()) أرجع vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ثابت vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(true);
            vtxp->srcp(srcp);
            أرجع vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ثابت idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ثابت preVscp = [&]() {
            DfgVertexVar* ثابت vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ثابت vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(true);
            vscp->varp()->noReset(true);
            vscp->varp()->setIgnoreSchedWrite();
            أرجع vscp;
        }();
        // The table variable
        DfgVarArray* ثابت tabVtxp = [&]() {
            DfgVertexVar* ثابت varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(true);
            varp->vscp()->varp()->noReset(true);
            varp->setHasModWrRefs();
            أرجع varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ثابت initp = جديد AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(جديد AstAssign{
                flp,  //
                جديد AstVarRef{flp, preVscp, VAccess::WRITE},  //
                جديد AstConst{flp, AstConst::Widthed_قىممەت{}, static_cast<صحيح>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(جديد AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ثابت logicp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(جديد AstAssign{
                flp,  //
                جديد AstArraySel{flp, جديد AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                جديد AstVarRef{flp, preVscp, VAccess::READ}},  //
                جديد AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ثابت idxRefp = جديد AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(جديد AstAssign{
                flp,  //
                جديد AstArraySel{flp, جديد AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                جديد AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* ثابت astRdp = جديد DfgAstRd{dfg, idxRefp, خطأ, خطأ};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ثابت idxRefp = جديد AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(جديد AstAssign{flp,  //
                                            جديد AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ثابت astRdp = جديد DfgAstRd{dfg, idxRefp, خطأ, خطأ};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        لكل (ثابت auto& pair : val2Terms) {
            ثابت uint32_t val = pair.first;
            ثابت std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_قۇر* ثابت aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ثابت aselp = جديد DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(جديد DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            لكل (ثابت Term& term : terms) {
                إذا (term.m_inv) {
                    إذا (!notp) {
                        notp = جديد DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } وإلا {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
