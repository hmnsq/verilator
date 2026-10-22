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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3DfgPasses.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

வெறுமை V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    க்கு (DfgVertex& vtx : dfg.opVertices()) {
        எனில் (vtx.is<DfgLogic>()) workList.தள்ளு_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* மாறிலி logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        மாறிலி தருக்க used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* மாறிலி uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* மாறிலி vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            எனில் (vVtxp->hasSinks()) திரும்பு உண்மை;
            எனில் (vVtxp->isObserved()) திரும்பு உண்மை;
            திரும்பு பொய்;
        });
        // If some are used, the logic must stay in the Ast
        எனில் (used) திரும்பு;
        // If impure, it must stay in the Ast
        எனில் (!logicp->isPure()) திரும்பு;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* மாறிலி varp = src.as<DfgVertexVar>();
            எனில் (!varp->srcp()) திரும்பு பொய்;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.தள்ளு_front(*driver.as<DfgLogic>());
                திரும்பு பொய்;
            });
            திரும்பு பொய்;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_முனை* மாறிலி nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    க்கு (DfgVertexVar* மாறிலி vVtxp : dfg.varVertices().unlinkable()) {
        எனில் (vVtxp->hasSinks()) தொடர்;
        எனில் (vVtxp->isObserved()) தொடர்;
        DfgVertex* மாறிலி srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* மாறிலி vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        மாறிலி தருக்க delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        எனில் (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                திரும்பு பொய்;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        எனில் (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_மரம்(), vscp);
            ++ctx.m_varsDeleted;
        } இல்லையெனில் {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    க்கு (DfgVertex* மாறிலி vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* மாறிலி logicp = vtxp->cast<DfgLogic>();
        எனில் (!logicp) தொடர்;
        எனில் (logicp->hasSinks()) தொடர்;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        மாறிலி VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            திரும்பு பொய்;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_முனை* varp) {  //
            திரும்பு varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

வெறுமை V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    க்கு (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* மாறிலி selp = vtx.cast<DfgSel>();
        எனில் (!selp) தொடர்;
        selps.தள்ளு_back(selp);
    }

    க்கு (DfgSel* மாறிலி selp : selps) {
        File_வரி* மாறிலி flp = selp->fileline();
        மாறிலி DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        எனில் (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            தொடர்;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* மாறிலி varp = selp->fromp()->cast<DfgVarPacked>();
        எனில் (!varp || !varp->tmpForp()) தொடர்;

        // Find the driver of this range
        மாறிலி auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* மாறிலி driverp = pair.first;
        மாறிலி uint32_t driverLsb = pair.second;
        எனில் (!driverp) தொடர்;

        // If partial driver is the whole thing we are looking for, just replace with that
        எனில் (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            தொடர்;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* மாறிலி newSelp = புதிய DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

வெறுமை V3DfgPasses::inlineVars(DfgGraph& dfg) {
    க்கு (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        எனில் (!vtx.hasSinks()) தொடர்;
        // Nohting to inline
        DfgVertex* மாறிலி srcp = vtx.srcp();
        எனில் (!srcp) தொடர்;
        // Value can differ from driver
        எனில் (vtx.isVolatile()) தொடர்;
        // Partial driver cannot be inlined
        எனில் (srcp->is<DfgVertexSplice>()) தொடர்;
        எனில் (srcp->is<DfgUnitArray>()) தொடர்;
        // Don't inline CReset
        எனில் (srcp->is<DfgCReset>()) தொடர்;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

வெறுமை V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    க்கு (DfgVertex& vtx : dfg.opVertices()) {
        எனில் (vtx.hasSinks()) தொடர்;
        // This vertex is unused. Add to work list.
        workList.தள்ளு_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    க்கு (DfgVertexVar& vtx : dfg.varVertices()) {
        எனில் (!vtx.tmpForp()) தொடர்;
        எனில் (vtx.hasSinks()) தொடர்;
        எனில் (vtx.hasDfgRefs()) தொடர்;
        // This vertex is unused. Add to work list.
        workList.தள்ளு_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        எனில் (vtx.hasSinks()) திரும்பு;
        // If temporary used in another graph, we need to keep it
        எனில் (மாறிலி DfgVertexVar* மாறிலி varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            எனில் (varp->hasDfgRefs()) திரும்பு;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            எனில் (src.is<DfgConst>()) திரும்பு பொய்;
            மாறிலி DfgVertexVar* மாறிலி varp = src.cast<DfgVertexVar>();
            எனில் (varp && !varp->tmpForp()) திரும்பு பொய்;
            // Add source to workList
            workList.தள்ளு_front(src);
            திரும்பு பொய்;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    க்கு (DfgVertexVar* மாறிலி vtxp : dfg.varVertices().unlinkable()) {
        எனில் (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    க்கு (DfgConst* மாறிலி vtxp : dfg.constVertices().unlinkable()) {
        எனில் (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

வெறுமை V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    கட்டமைப்பு Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        தருக்க m_inv = பொய்;  // '!=', instead of '=='
        Term() = இயல்பு;
        Term(DfgVertex* vtxp, தருக்க inv)
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
    மாறிலி auto widthOk = [](மாறிலி DfgVertex* vtxp) {
        மாறிலி uint32_t width = vtxp->width();
        திரும்பு WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    மாறிலி std::function<தருக்க(மாறிலி DfgVertex*, தருக்க)> useOk
        = [&](மாறிலி DfgVertex* vtxp, தருக்க inv) -> தருக்க {
        // Go past a single 'Not' sink, which is common
        எனில் (DfgVertex* மாறிலி sinkp = vtxp->singleSink()) {
            எனில் (sinkp->is<DfgNot>()) திரும்பு useOk(sinkp, !inv);
        }
        மாறிலி தருக்க cond_மரம் = vtxp->foreachSink([&](மாறிலி DfgVertex& sink) {
            மாறிலி DfgCond* மாறிலி condp = sink.cast<DfgCond>();
            எனில் (!condp) திரும்பு பொய்;
            எனில் (condp->condp() != vtxp) திரும்பு பொய்;
            திரும்பு inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        திரும்பு !cond_மரம்;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    க்கு (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        தருக்க inv = பொய்;
        எனில் (DfgEq* மாறிலி eqp = vtx.cast<DfgEq>()) {
            DfgConst* மாறிலி constp = eqp->lhsp()->cast<DfgConst>();
            எனில் (!constp || !widthOk(constp) || !useOk(eqp, பொய்)) தொடர்;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = பொய்;
        } இல்லையெனில் எனில் (DfgNeq* மாறிலி neqp = vtx.cast<DfgNeq>()) {
            DfgConst* மாறிலி constp = neqp->lhsp()->cast<DfgConst>();
            எனில் (!constp || !widthOk(constp) || !useOk(neqp, உண்மை)) தொடர்;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = உண்மை;
        } இல்லையெனில் எனில் (DfgRedAnd* மாறிலி redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            எனில் (!widthOk(srcp) || !useOk(redAndp, பொய்)) தொடர்;
            val = (1U << srcp->width()) - 1;
            inv = பொய்;
        } இல்லையெனில் எனில் (DfgRedOr* மாறிலி redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            எனில் (!widthOk(srcp) || !useOk(redOrp, உண்மை)) தொடர்;
            val = 0;
            inv = உண்மை;
        } இல்லையெனில் எனில் (DfgSel* மாறிலி selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            எனில் (selp->width() != 1) தொடர்;
            DfgShiftL* மாறிலி shiftLp = selp->fromp()->cast<DfgShiftL>();
            எனில் (!shiftLp) தொடர்;
            மாறிலி DfgConst* மாறிலி constp = shiftLp->lhsp()->cast<DfgConst>();
            எனில் (!constp || !useOk(selp, பொய்)) தொடர்;
            எனில் (!constp->has_மதிப்பு(1)) தொடர்;
            srcp = shiftLp->rhsp();
            எனில் (!widthOk(srcp)) தொடர்;
            val = selp->lsb();
            inv = பொய்;
        } இல்லையெனில் {
            // Not a comparison-like vertex
            தொடர்;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        எனில் (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    எனில் (nTerms < TERM_LIMIT) திரும்பு;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    க்கு (DfgVertex* மாறிலி srcp : srcps) {
        மாறிலி Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        எனில் (val2Terms.size() < TERM_LIMIT) தொடர்;

        // Width of the decoded binary value
        மாறிலி uint32_t width = srcp->width();
        // Number of bits in the input operand
        மாறிலி uint32_t nBits = 1U << width;

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

        File_வரி* மாறிலி flp = srcp->fileline();
        AstScope* மாறிலி scopep = srcp->scopep(scopeCache, உண்மை);

        AstActive* மாறிலி staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            க்கு (Ast_முனை* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* மாறிலி activep = VN_CAST(nodep, Active);
                எனில் (!activep) தொடர்;
                எனில் (activep->sentreep()->hasStatic()) திரும்பு activep;
            }
            // If there isn't one, create a new one
            File_வரி* மாறிலி flp = scopep->fileline();
            Ast_Sen_மரம்* மாறிலி stp = புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* மாறிலி activep = புதிய AstActive{flp, "", stp};
            activep->sen_மரம்_Storep(stp);
            scopep->addBlocksp(activep);
            திரும்பு activep;
        }();
        AstActive* மாறிலி combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            க்கு (Ast_முனை* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* மாறிலி activep = VN_CAST(nodep, Active);
                எனில் (!activep) தொடர்;
                எனில் (activep->sentreep()->hasCombo()) திரும்பு activep;
            }
            // If there isn't one, create a new one
            File_வரி* மாறிலி flp = scopep->fileline();
            Ast_Sen_மரம்* மாறிலி stp = புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* மாறிலி activep = புதிய AstActive{flp, "", stp};
            activep->sen_மரம்_Storep(stp);
            scopep->addBlocksp(activep);
            திரும்பு activep;
        }();

        // Required data types
        மாறிலி DfgDataType& idxDType = srcp->dtype();
        மாறிலி DfgDataType& bitDType = DfgDataType::packed(1);
        மாறிலி DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            எனில் (DfgVertexVar* மாறிலி vp = srcp->getResultVar()) திரும்பு vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* மாறிலி vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(உண்மை);
            vtxp->srcp(srcp);
            திரும்பு vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* மாறிலி idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* மாறிலி preVscp = [&]() {
            DfgVertexVar* மாறிலி vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* மாறிலி vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(உண்மை);
            vscp->varp()->noReset(உண்மை);
            vscp->varp()->setIgnoreSchedWrite();
            திரும்பு vscp;
        }();
        // The table variable
        DfgVarArray* மாறிலி tabVtxp = [&]() {
            DfgVertexVar* மாறிலி varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(உண்மை);
            varp->vscp()->varp()->noReset(உண்மை);
            varp->setHasModWrRefs();
            திரும்பு varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* மாறிலி initp = புதிய AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(புதிய AstAssign{
                flp,  //
                புதிய AstVarRef{flp, preVscp, VAccess::WRITE},  //
                புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, static_cast<முழு>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* மாறிலி callp = புதிய AstCMethodHard{
                flp, புதிய AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(புதிய AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* மாறிலி logicp = புதிய AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(புதிய AstAssign{
                flp,  //
                புதிய AstArraySel{flp, புதிய AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                புதிய AstVarRef{flp, preVscp, VAccess::READ}},  //
                புதிய AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* மாறிலி idxRefp = புதிய AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(புதிய AstAssign{
                flp,  //
                புதிய AstArraySel{flp, புதிய AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                புதிய AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* மாறிலி astRdp = புதிய DfgAstRd{dfg, idxRefp, பொய், பொய்};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* மாறிலி idxRefp = புதிய AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(புதிய AstAssign{flp,  //
                                            புதிய AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* மாறிலி astRdp = புதிய DfgAstRd{dfg, idxRefp, பொய், பொய்};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        க்கு (மாறிலி auto& pair : val2Terms) {
            மாறிலி uint32_t val = pair.first;
            மாறிலி std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_வரி* மாறிலி aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* மாறிலி aselp = புதிய DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(புதிய DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            க்கு (மாறிலி Term& term : terms) {
                எனில் (term.m_inv) {
                    எனில் (!notp) {
                        notp = புதிய DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } இல்லையெனில் {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
