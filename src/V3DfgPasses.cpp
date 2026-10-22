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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3DfgPasses.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

શૂન્ય V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    માટે (DfgVertex& vtx : dfg.opVertices()) {
        જો (vtx.is<DfgLogic>()) workList.ધકેલો_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* અચલ logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        અચલ તાર્કિક used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* અચલ uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* અચલ vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            જો (vVtxp->hasSinks()) પાછા સાચું;
            જો (vVtxp->isObserved()) પાછા સાચું;
            પાછા ખોટું;
        });
        // If some are used, the logic must stay in the Ast
        જો (used) પાછા;
        // If impure, it must stay in the Ast
        જો (!logicp->isPure()) પાછા;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* અચલ varp = src.as<DfgVertexVar>();
            જો (!varp->srcp()) પાછા ખોટું;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.ધકેલો_front(*driver.as<DfgLogic>());
                પાછા ખોટું;
            });
            પાછા ખોટું;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_ગાંઠ* અચલ nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    માટે (DfgVertexVar* અચલ vVtxp : dfg.varVertices().unlinkable()) {
        જો (vVtxp->hasSinks()) ચાલુ;
        જો (vVtxp->isObserved()) ચાલુ;
        DfgVertex* અચલ srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* અચલ vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        અચલ તાર્કિક delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        જો (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                પાછા ખોટું;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        જો (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_વૃક્ષ(), vscp);
            ++ctx.m_varsDeleted;
        } નહીંતર {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    માટે (DfgVertex* અચલ vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* અચલ logicp = vtxp->cast<DfgLogic>();
        જો (!logicp) ચાલુ;
        જો (logicp->hasSinks()) ચાલુ;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        અચલ VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            પાછા ખોટું;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_ગાંઠ* varp) {  //
            પાછા varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

શૂન્ય V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    માટે (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* અચલ selp = vtx.cast<DfgSel>();
        જો (!selp) ચાલુ;
        selps.ધકેલો_back(selp);
    }

    માટે (DfgSel* અચલ selp : selps) {
        File_લીટી* અચલ flp = selp->fileline();
        અચલ DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        જો (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ચાલુ;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* અચલ varp = selp->fromp()->cast<DfgVarPacked>();
        જો (!varp || !varp->tmpForp()) ચાલુ;

        // Find the driver of this range
        અચલ auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* અચલ driverp = pair.first;
        અચલ uint32_t driverLsb = pair.second;
        જો (!driverp) ચાલુ;

        // If partial driver is the whole thing we are looking for, just replace with that
        જો (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ચાલુ;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* અચલ newSelp = નવું DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

શૂન્ય V3DfgPasses::inlineVars(DfgGraph& dfg) {
    માટે (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        જો (!vtx.hasSinks()) ચાલુ;
        // Nohting to inline
        DfgVertex* અચલ srcp = vtx.srcp();
        જો (!srcp) ચાલુ;
        // Value can differ from driver
        જો (vtx.isVolatile()) ચાલુ;
        // Partial driver cannot be inlined
        જો (srcp->is<DfgVertexSplice>()) ચાલુ;
        જો (srcp->is<DfgUnitArray>()) ચાલુ;
        // Don't inline CReset
        જો (srcp->is<DfgCReset>()) ચાલુ;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

શૂન્ય V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    માટે (DfgVertex& vtx : dfg.opVertices()) {
        જો (vtx.hasSinks()) ચાલુ;
        // This vertex is unused. Add to work list.
        workList.ધકેલો_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    માટે (DfgVertexVar& vtx : dfg.varVertices()) {
        જો (!vtx.tmpForp()) ચાલુ;
        જો (vtx.hasSinks()) ચાલુ;
        જો (vtx.hasDfgRefs()) ચાલુ;
        // This vertex is unused. Add to work list.
        workList.ધકેલો_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        જો (vtx.hasSinks()) પાછા;
        // If temporary used in another graph, we need to keep it
        જો (અચલ DfgVertexVar* અચલ varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            જો (varp->hasDfgRefs()) પાછા;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            જો (src.is<DfgConst>()) પાછા ખોટું;
            અચલ DfgVertexVar* અચલ varp = src.cast<DfgVertexVar>();
            જો (varp && !varp->tmpForp()) પાછા ખોટું;
            // Add source to workList
            workList.ધકેલો_front(src);
            પાછા ખોટું;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    માટે (DfgVertexVar* અચલ vtxp : dfg.varVertices().unlinkable()) {
        જો (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    માટે (DfgConst* અચલ vtxp : dfg.constVertices().unlinkable()) {
        જો (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

શૂન્ય V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    સંરચના Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        તાર્કિક m_inv = ખોટું;  // '!=', instead of '=='
        Term() = મૂળભૂત;
        Term(DfgVertex* vtxp, તાર્કિક inv)
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
    અચલ auto widthOk = [](અચલ DfgVertex* vtxp) {
        અચલ uint32_t width = vtxp->width();
        પાછા WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    અચલ std::function<તાર્કિક(અચલ DfgVertex*, તાર્કિક)> useOk
        = [&](અચલ DfgVertex* vtxp, તાર્કિક inv) -> તાર્કિક {
        // Go past a single 'Not' sink, which is common
        જો (DfgVertex* અચલ sinkp = vtxp->singleSink()) {
            જો (sinkp->is<DfgNot>()) પાછા useOk(sinkp, !inv);
        }
        અચલ તાર્કિક cond_વૃક્ષ = vtxp->foreachSink([&](અચલ DfgVertex& sink) {
            અચલ DfgCond* અચલ condp = sink.cast<DfgCond>();
            જો (!condp) પાછા ખોટું;
            જો (condp->condp() != vtxp) પાછા ખોટું;
            પાછા inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        પાછા !cond_વૃક્ષ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    માટે (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        તાર્કિક inv = ખોટું;
        જો (DfgEq* અચલ eqp = vtx.cast<DfgEq>()) {
            DfgConst* અચલ constp = eqp->lhsp()->cast<DfgConst>();
            જો (!constp || !widthOk(constp) || !useOk(eqp, ખોટું)) ચાલુ;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = ખોટું;
        } નહીંતર જો (DfgNeq* અચલ neqp = vtx.cast<DfgNeq>()) {
            DfgConst* અચલ constp = neqp->lhsp()->cast<DfgConst>();
            જો (!constp || !widthOk(constp) || !useOk(neqp, સાચું)) ચાલુ;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = સાચું;
        } નહીંતર જો (DfgRedAnd* અચલ redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            જો (!widthOk(srcp) || !useOk(redAndp, ખોટું)) ચાલુ;
            val = (1U << srcp->width()) - 1;
            inv = ખોટું;
        } નહીંતર જો (DfgRedOr* અચલ redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            જો (!widthOk(srcp) || !useOk(redOrp, સાચું)) ચાલુ;
            val = 0;
            inv = સાચું;
        } નહીંતર જો (DfgSel* અચલ selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            જો (selp->width() != 1) ચાલુ;
            DfgShiftL* અચલ shiftLp = selp->fromp()->cast<DfgShiftL>();
            જો (!shiftLp) ચાલુ;
            અચલ DfgConst* અચલ constp = shiftLp->lhsp()->cast<DfgConst>();
            જો (!constp || !useOk(selp, ખોટું)) ચાલુ;
            જો (!constp->has_કિંમત(1)) ચાલુ;
            srcp = shiftLp->rhsp();
            જો (!widthOk(srcp)) ચાલુ;
            val = selp->lsb();
            inv = ખોટું;
        } નહીંતર {
            // Not a comparison-like vertex
            ચાલુ;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        જો (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    જો (nTerms < TERM_LIMIT) પાછા;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    માટે (DfgVertex* અચલ srcp : srcps) {
        અચલ Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        જો (val2Terms.size() < TERM_LIMIT) ચાલુ;

        // Width of the decoded binary value
        અચલ uint32_t width = srcp->width();
        // Number of bits in the input operand
        અચલ uint32_t nBits = 1U << width;

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

        File_લીટી* અચલ flp = srcp->fileline();
        AstScope* અચલ scopep = srcp->scopep(scopeCache, સાચું);

        AstActive* અચલ staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            માટે (Ast_ગાંઠ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* અચલ activep = VN_CAST(nodep, Active);
                જો (!activep) ચાલુ;
                જો (activep->sentreep()->hasStatic()) પાછા activep;
            }
            // If there isn't one, create a new one
            File_લીટી* અચલ flp = scopep->fileline();
            Ast_Sen_વૃક્ષ* અચલ stp = નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* અચલ activep = નવું AstActive{flp, "", stp};
            activep->sen_વૃક્ષ_Storep(stp);
            scopep->addBlocksp(activep);
            પાછા activep;
        }();
        AstActive* અચલ combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            માટે (Ast_ગાંઠ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* અચલ activep = VN_CAST(nodep, Active);
                જો (!activep) ચાલુ;
                જો (activep->sentreep()->hasCombo()) પાછા activep;
            }
            // If there isn't one, create a new one
            File_લીટી* અચલ flp = scopep->fileline();
            Ast_Sen_વૃક્ષ* અચલ stp = નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* અચલ activep = નવું AstActive{flp, "", stp};
            activep->sen_વૃક્ષ_Storep(stp);
            scopep->addBlocksp(activep);
            પાછા activep;
        }();

        // Required data types
        અચલ DfgDataType& idxDType = srcp->dtype();
        અચલ DfgDataType& bitDType = DfgDataType::packed(1);
        અચલ DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            જો (DfgVertexVar* અચલ vp = srcp->getResultVar()) પાછા vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* અચલ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(સાચું);
            vtxp->srcp(srcp);
            પાછા vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* અચલ idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* અચલ preVscp = [&]() {
            DfgVertexVar* અચલ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* અચલ vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(સાચું);
            vscp->varp()->noReset(સાચું);
            vscp->varp()->setIgnoreSchedWrite();
            પાછા vscp;
        }();
        // The table variable
        DfgVarArray* અચલ tabVtxp = [&]() {
            DfgVertexVar* અચલ varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(સાચું);
            varp->vscp()->varp()->noReset(સાચું);
            varp->setHasModWrRefs();
            પાછા varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* અચલ initp = નવું AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(નવું AstAssign{
                flp,  //
                નવું AstVarRef{flp, preVscp, VAccess::WRITE},  //
                નવું AstConst{flp, AstConst::Widthed_કિંમત{}, static_cast<પૂર્ણાંક>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* અચલ callp = નવું AstCMethodHard{
                flp, નવું AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(નવું AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* અચલ logicp = નવું AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(નવું AstAssign{
                flp,  //
                નવું AstArraySel{flp, નવું AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                નવું AstVarRef{flp, preVscp, VAccess::READ}},  //
                નવું AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* અચલ idxRefp = નવું AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(નવું AstAssign{
                flp,  //
                નવું AstArraySel{flp, નવું AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                નવું AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* અચલ astRdp = નવું DfgAstRd{dfg, idxRefp, ખોટું, ખોટું};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* અચલ idxRefp = નવું AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(નવું AstAssign{flp,  //
                                            નવું AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* અચલ astRdp = નવું DfgAstRd{dfg, idxRefp, ખોટું, ખોટું};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        માટે (અચલ auto& pair : val2Terms) {
            અચલ uint32_t val = pair.first;
            અચલ std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_લીટી* અચલ aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* અચલ aselp = નવું DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(નવું DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            માટે (અચલ Term& term : terms) {
                જો (term.m_inv) {
                    જો (!notp) {
                        notp = નવું DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } નહીંતર {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
