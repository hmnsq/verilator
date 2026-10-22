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
    साठी (DfgVertex& vtx : dfg.opVertices()) {
        जर (vtx.is<DfgLogic>()) workList.ढकला_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* स्थिरांक logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        स्थिरांक तार्किक used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* स्थिरांक uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* स्थिरांक vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            जर (vVtxp->hasSinks()) परत सत्य;
            जर (vVtxp->isObserved()) परत सत्य;
            परत असत्य;
        });
        // If some are used, the logic must stay in the Ast
        जर (used) परत;
        // If impure, it must stay in the Ast
        जर (!logicp->isPure()) परत;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* स्थिरांक varp = src.as<DfgVertexVar>();
            जर (!varp->srcp()) परत असत्य;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.ढकला_front(*driver.as<DfgLogic>());
                परत असत्य;
            });
            परत असत्य;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_गाठ* स्थिरांक nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    साठी (DfgVertexVar* स्थिरांक vVtxp : dfg.varVertices().unlinkable()) {
        जर (vVtxp->hasSinks()) सुरूठेवा;
        जर (vVtxp->isObserved()) सुरूठेवा;
        DfgVertex* स्थिरांक srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* स्थिरांक vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        स्थिरांक तार्किक delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        जर (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                परत असत्य;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        जर (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_वृक्ष(), vscp);
            ++ctx.m_varsDeleted;
        } अन्यथा {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    साठी (DfgVertex* स्थिरांक vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* स्थिरांक logicp = vtxp->cast<DfgLogic>();
        जर (!logicp) सुरूठेवा;
        जर (logicp->hasSinks()) सुरूठेवा;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        स्थिरांक VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            परत असत्य;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_गाठ* varp) {  //
            परत varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

शून्य V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    साठी (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* स्थिरांक selp = vtx.cast<DfgSel>();
        जर (!selp) सुरूठेवा;
        selps.ढकला_back(selp);
    }

    साठी (DfgSel* स्थिरांक selp : selps) {
        File_ओळ* स्थिरांक flp = selp->fileline();
        स्थिरांक DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        जर (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            सुरूठेवा;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* स्थिरांक varp = selp->fromp()->cast<DfgVarPacked>();
        जर (!varp || !varp->tmpForp()) सुरूठेवा;

        // Find the driver of this range
        स्थिरांक स्वतः pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* स्थिरांक driverp = pair.first;
        स्थिरांक uint32_t driverLsb = pair.second;
        जर (!driverp) सुरूठेवा;

        // If partial driver is the whole thing we are looking for, just replace with that
        जर (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            सुरूठेवा;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* स्थिरांक newSelp = नवीन DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

शून्य V3DfgPasses::inlineVars(DfgGraph& dfg) {
    साठी (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        जर (!vtx.hasSinks()) सुरूठेवा;
        // Nohting to inline
        DfgVertex* स्थिरांक srcp = vtx.srcp();
        जर (!srcp) सुरूठेवा;
        // Value can differ from driver
        जर (vtx.isVolatile()) सुरूठेवा;
        // Partial driver cannot be inlined
        जर (srcp->is<DfgVertexSplice>()) सुरूठेवा;
        जर (srcp->is<DfgUnitArray>()) सुरूठेवा;
        // Don't inline CReset
        जर (srcp->is<DfgCReset>()) सुरूठेवा;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

शून्य V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    साठी (DfgVertex& vtx : dfg.opVertices()) {
        जर (vtx.hasSinks()) सुरूठेवा;
        // This vertex is unused. Add to work list.
        workList.ढकला_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    साठी (DfgVertexVar& vtx : dfg.varVertices()) {
        जर (!vtx.tmpForp()) सुरूठेवा;
        जर (vtx.hasSinks()) सुरूठेवा;
        जर (vtx.hasDfgRefs()) सुरूठेवा;
        // This vertex is unused. Add to work list.
        workList.ढकला_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        जर (vtx.hasSinks()) परत;
        // If temporary used in another graph, we need to keep it
        जर (स्थिरांक DfgVertexVar* स्थिरांक varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            जर (varp->hasDfgRefs()) परत;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            जर (src.is<DfgConst>()) परत असत्य;
            स्थिरांक DfgVertexVar* स्थिरांक varp = src.cast<DfgVertexVar>();
            जर (varp && !varp->tmpForp()) परत असत्य;
            // Add source to workList
            workList.ढकला_front(src);
            परत असत्य;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    साठी (DfgVertexVar* स्थिरांक vtxp : dfg.varVertices().unlinkable()) {
        जर (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    साठी (DfgConst* स्थिरांक vtxp : dfg.constVertices().unlinkable()) {
        जर (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

शून्य V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    संरचना Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        तार्किक m_inv = असत्य;  // '!=', instead of '=='
        Term() = पूर्वनिर्धारित;
        Term(DfgVertex* vtxp, तार्किक inv)
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
    स्थिरांक स्वतः widthOk = [](स्थिरांक DfgVertex* vtxp) {
        स्थिरांक uint32_t width = vtxp->width();
        परत WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    स्थिरांक std::function<तार्किक(स्थिरांक DfgVertex*, तार्किक)> useOk
        = [&](स्थिरांक DfgVertex* vtxp, तार्किक inv) -> तार्किक {
        // Go past a single 'Not' sink, which is common
        जर (DfgVertex* स्थिरांक sinkp = vtxp->singleSink()) {
            जर (sinkp->is<DfgNot>()) परत useOk(sinkp, !inv);
        }
        स्थिरांक तार्किक cond_वृक्ष = vtxp->foreachSink([&](स्थिरांक DfgVertex& sink) {
            स्थिरांक DfgCond* स्थिरांक condp = sink.cast<DfgCond>();
            जर (!condp) परत असत्य;
            जर (condp->condp() != vtxp) परत असत्य;
            परत inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        परत !cond_वृक्ष;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    साठी (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        तार्किक inv = असत्य;
        जर (DfgEq* स्थिरांक eqp = vtx.cast<DfgEq>()) {
            DfgConst* स्थिरांक constp = eqp->lhsp()->cast<DfgConst>();
            जर (!constp || !widthOk(constp) || !useOk(eqp, असत्य)) सुरूठेवा;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = असत्य;
        } अन्यथा जर (DfgNeq* स्थिरांक neqp = vtx.cast<DfgNeq>()) {
            DfgConst* स्थिरांक constp = neqp->lhsp()->cast<DfgConst>();
            जर (!constp || !widthOk(constp) || !useOk(neqp, सत्य)) सुरूठेवा;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = सत्य;
        } अन्यथा जर (DfgRedAnd* स्थिरांक redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            जर (!widthOk(srcp) || !useOk(redAndp, असत्य)) सुरूठेवा;
            val = (1U << srcp->width()) - 1;
            inv = असत्य;
        } अन्यथा जर (DfgRedOr* स्थिरांक redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            जर (!widthOk(srcp) || !useOk(redOrp, सत्य)) सुरूठेवा;
            val = 0;
            inv = सत्य;
        } अन्यथा जर (DfgSel* स्थिरांक selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            जर (selp->width() != 1) सुरूठेवा;
            DfgShiftL* स्थिरांक shiftLp = selp->fromp()->cast<DfgShiftL>();
            जर (!shiftLp) सुरूठेवा;
            स्थिरांक DfgConst* स्थिरांक constp = shiftLp->lhsp()->cast<DfgConst>();
            जर (!constp || !useOk(selp, असत्य)) सुरूठेवा;
            जर (!constp->has_मूल्य(1)) सुरूठेवा;
            srcp = shiftLp->rhsp();
            जर (!widthOk(srcp)) सुरूठेवा;
            val = selp->lsb();
            inv = असत्य;
        } अन्यथा {
            // Not a comparison-like vertex
            सुरूठेवा;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        जर (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    जर (nTerms < TERM_LIMIT) परत;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    साठी (DfgVertex* स्थिरांक srcp : srcps) {
        स्थिरांक Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        जर (val2Terms.size() < TERM_LIMIT) सुरूठेवा;

        // Width of the decoded binary value
        स्थिरांक uint32_t width = srcp->width();
        // Number of bits in the input operand
        स्थिरांक uint32_t nBits = 1U << width;

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

        File_ओळ* स्थिरांक flp = srcp->fileline();
        AstScope* स्थिरांक scopep = srcp->scopep(scopeCache, सत्य);

        AstActive* स्थिरांक staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            साठी (Ast_गाठ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* स्थिरांक activep = VN_CAST(nodep, Active);
                जर (!activep) सुरूठेवा;
                जर (activep->sentreep()->hasStatic()) परत activep;
            }
            // If there isn't one, create a new one
            File_ओळ* स्थिरांक flp = scopep->fileline();
            Ast_Sen_वृक्ष* स्थिरांक stp = नवीन Ast_Sen_वृक्ष{flp, नवीन AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* स्थिरांक activep = नवीन AstActive{flp, "", stp};
            activep->sen_वृक्ष_Storep(stp);
            scopep->addBlocksp(activep);
            परत activep;
        }();
        AstActive* स्थिरांक combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            साठी (Ast_गाठ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* स्थिरांक activep = VN_CAST(nodep, Active);
                जर (!activep) सुरूठेवा;
                जर (activep->sentreep()->hasCombo()) परत activep;
            }
            // If there isn't one, create a new one
            File_ओळ* स्थिरांक flp = scopep->fileline();
            Ast_Sen_वृक्ष* स्थिरांक stp = नवीन Ast_Sen_वृक्ष{flp, नवीन AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* स्थिरांक activep = नवीन AstActive{flp, "", stp};
            activep->sen_वृक्ष_Storep(stp);
            scopep->addBlocksp(activep);
            परत activep;
        }();

        // Required data types
        स्थिरांक DfgDataType& idxDType = srcp->dtype();
        स्थिरांक DfgDataType& bitDType = DfgDataType::packed(1);
        स्थिरांक DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            जर (DfgVertexVar* स्थिरांक vp = srcp->getResultVar()) परत vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* स्थिरांक vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(सत्य);
            vtxp->srcp(srcp);
            परत vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* स्थिरांक idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* स्थिरांक preVscp = [&]() {
            DfgVertexVar* स्थिरांक vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* स्थिरांक vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(सत्य);
            vscp->varp()->noReset(सत्य);
            vscp->varp()->setIgnoreSchedWrite();
            परत vscp;
        }();
        // The table variable
        DfgVarArray* स्थिरांक tabVtxp = [&]() {
            DfgVertexVar* स्थिरांक varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(सत्य);
            varp->vscp()->varp()->noReset(सत्य);
            varp->setHasModWrRefs();
            परत varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* स्थिरांक initp = नवीन AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(नवीन AstAssign{
                flp,  //
                नवीन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, static_cast<पूर्णांक>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* स्थिरांक callp = नवीन AstCMethodHard{
                flp, नवीन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(नवीन AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* स्थिरांक logicp = नवीन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(नवीन AstAssign{
                flp,  //
                नवीन AstArraySel{flp, नवीन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                नवीन AstVarRef{flp, preVscp, VAccess::READ}},  //
                नवीन AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* स्थिरांक idxRefp = नवीन AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(नवीन AstAssign{
                flp,  //
                नवीन AstArraySel{flp, नवीन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                नवीन AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* स्थिरांक astRdp = नवीन DfgAstRd{dfg, idxRefp, असत्य, असत्य};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* स्थिरांक idxRefp = नवीन AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(नवीन AstAssign{flp,  //
                                            नवीन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* स्थिरांक astRdp = नवीन DfgAstRd{dfg, idxRefp, असत्य, असत्य};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        साठी (स्थिरांक स्वतः& pair : val2Terms) {
            स्थिरांक uint32_t val = pair.first;
            स्थिरांक std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_ओळ* स्थिरांक aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* स्थिरांक aselp = नवीन DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(नवीन DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            साठी (स्थिरांक Term& term : terms) {
                जर (term.m_inv) {
                    जर (!notp) {
                        notp = नवीन DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } अन्यथा {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
