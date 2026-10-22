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
    लागि (DfgVertex& vtx : dfg.opVertices()) {
        यदि (vtx.is<DfgLogic>()) workList.थप_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* अचर logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        अचर तार्किक used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* अचर uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* अचर vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            यदि (vVtxp->hasSinks()) फर्काऊ सत्य;
            यदि (vVtxp->isObserved()) फर्काऊ सत्य;
            फर्काऊ असत्य;
        });
        // If some are used, the logic must stay in the Ast
        यदि (used) फर्काऊ;
        // If impure, it must stay in the Ast
        यदि (!logicp->isPure()) फर्काऊ;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* अचर varp = src.as<DfgVertexVar>();
            यदि (!varp->srcp()) फर्काऊ असत्य;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.थप_front(*driver.as<DfgLogic>());
                फर्काऊ असत्य;
            });
            फर्काऊ असत्य;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_गाँठो* अचर nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    लागि (DfgVertexVar* अचर vVtxp : dfg.varVertices().unlinkable()) {
        यदि (vVtxp->hasSinks()) जारी;
        यदि (vVtxp->isObserved()) जारी;
        DfgVertex* अचर srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* अचर vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        अचर तार्किक delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        यदि (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                फर्काऊ असत्य;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        यदि (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_रूख(), vscp);
            ++ctx.m_varsDeleted;
        } अन्यथा {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    लागि (DfgVertex* अचर vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* अचर logicp = vtxp->cast<DfgLogic>();
        यदि (!logicp) जारी;
        यदि (logicp->hasSinks()) जारी;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        अचर VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            फर्काऊ असत्य;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_गाँठो* varp) {  //
            फर्काऊ varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

शून्य V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    लागि (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* अचर selp = vtx.cast<DfgSel>();
        यदि (!selp) जारी;
        selps.थप_back(selp);
    }

    लागि (DfgSel* अचर selp : selps) {
        File_पङ्क्ति* अचर flp = selp->fileline();
        अचर DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        यदि (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            जारी;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* अचर varp = selp->fromp()->cast<DfgVarPacked>();
        यदि (!varp || !varp->tmpForp()) जारी;

        // Find the driver of this range
        अचर स्वतः pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* अचर driverp = pair.first;
        अचर uint32_t driverLsb = pair.second;
        यदि (!driverp) जारी;

        // If partial driver is the whole thing we are looking for, just replace with that
        यदि (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            जारी;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* अचर newSelp = नूतन DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

शून्य V3DfgPasses::inlineVars(DfgGraph& dfg) {
    लागि (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        यदि (!vtx.hasSinks()) जारी;
        // Nohting to inline
        DfgVertex* अचर srcp = vtx.srcp();
        यदि (!srcp) जारी;
        // Value can differ from driver
        यदि (vtx.isVolatile()) जारी;
        // Partial driver cannot be inlined
        यदि (srcp->is<DfgVertexSplice>()) जारी;
        यदि (srcp->is<DfgUnitArray>()) जारी;
        // Don't inline CReset
        यदि (srcp->is<DfgCReset>()) जारी;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

शून्य V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    लागि (DfgVertex& vtx : dfg.opVertices()) {
        यदि (vtx.hasSinks()) जारी;
        // This vertex is unused. Add to work list.
        workList.थप_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    लागि (DfgVertexVar& vtx : dfg.varVertices()) {
        यदि (!vtx.tmpForp()) जारी;
        यदि (vtx.hasSinks()) जारी;
        यदि (vtx.hasDfgRefs()) जारी;
        // This vertex is unused. Add to work list.
        workList.थप_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        यदि (vtx.hasSinks()) फर्काऊ;
        // If temporary used in another graph, we need to keep it
        यदि (अचर DfgVertexVar* अचर varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            यदि (varp->hasDfgRefs()) फर्काऊ;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            यदि (src.is<DfgConst>()) फर्काऊ असत्य;
            अचर DfgVertexVar* अचर varp = src.cast<DfgVertexVar>();
            यदि (varp && !varp->tmpForp()) फर्काऊ असत्य;
            // Add source to workList
            workList.थप_front(src);
            फर्काऊ असत्य;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    लागि (DfgVertexVar* अचर vtxp : dfg.varVertices().unlinkable()) {
        यदि (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    लागि (DfgConst* अचर vtxp : dfg.constVertices().unlinkable()) {
        यदि (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

शून्य V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    संरचना Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        तार्किक m_inv = असत्य;  // '!=', instead of '=='
        Term() = स्वभाव;
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
    अचर स्वतः widthOk = [](अचर DfgVertex* vtxp) {
        अचर uint32_t width = vtxp->width();
        फर्काऊ WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    अचर std::function<तार्किक(अचर DfgVertex*, तार्किक)> useOk
        = [&](अचर DfgVertex* vtxp, तार्किक inv) -> तार्किक {
        // Go past a single 'Not' sink, which is common
        यदि (DfgVertex* अचर sinkp = vtxp->singleSink()) {
            यदि (sinkp->is<DfgNot>()) फर्काऊ useOk(sinkp, !inv);
        }
        अचर तार्किक cond_रूख = vtxp->foreachSink([&](अचर DfgVertex& sink) {
            अचर DfgCond* अचर condp = sink.cast<DfgCond>();
            यदि (!condp) फर्काऊ असत्य;
            यदि (condp->condp() != vtxp) फर्काऊ असत्य;
            फर्काऊ inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        फर्काऊ !cond_रूख;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    लागि (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        तार्किक inv = असत्य;
        यदि (DfgEq* अचर eqp = vtx.cast<DfgEq>()) {
            DfgConst* अचर constp = eqp->lhsp()->cast<DfgConst>();
            यदि (!constp || !widthOk(constp) || !useOk(eqp, असत्य)) जारी;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = असत्य;
        } अन्यथा यदि (DfgNeq* अचर neqp = vtx.cast<DfgNeq>()) {
            DfgConst* अचर constp = neqp->lhsp()->cast<DfgConst>();
            यदि (!constp || !widthOk(constp) || !useOk(neqp, सत्य)) जारी;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = सत्य;
        } अन्यथा यदि (DfgRedAnd* अचर redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            यदि (!widthOk(srcp) || !useOk(redAndp, असत्य)) जारी;
            val = (1U << srcp->width()) - 1;
            inv = असत्य;
        } अन्यथा यदि (DfgRedOr* अचर redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            यदि (!widthOk(srcp) || !useOk(redOrp, सत्य)) जारी;
            val = 0;
            inv = सत्य;
        } अन्यथा यदि (DfgSel* अचर selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            यदि (selp->width() != 1) जारी;
            DfgShiftL* अचर shiftLp = selp->fromp()->cast<DfgShiftL>();
            यदि (!shiftLp) जारी;
            अचर DfgConst* अचर constp = shiftLp->lhsp()->cast<DfgConst>();
            यदि (!constp || !useOk(selp, असत्य)) जारी;
            यदि (!constp->has_मान(1)) जारी;
            srcp = shiftLp->rhsp();
            यदि (!widthOk(srcp)) जारी;
            val = selp->lsb();
            inv = असत्य;
        } अन्यथा {
            // Not a comparison-like vertex
            जारी;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        यदि (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    यदि (nTerms < TERM_LIMIT) फर्काऊ;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    लागि (DfgVertex* अचर srcp : srcps) {
        अचर Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        यदि (val2Terms.size() < TERM_LIMIT) जारी;

        // Width of the decoded binary value
        अचर uint32_t width = srcp->width();
        // Number of bits in the input operand
        अचर uint32_t nBits = 1U << width;

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

        File_पङ्क्ति* अचर flp = srcp->fileline();
        AstScope* अचर scopep = srcp->scopep(scopeCache, सत्य);

        AstActive* अचर staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            लागि (Ast_गाँठो* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* अचर activep = VN_CAST(nodep, Active);
                यदि (!activep) जारी;
                यदि (activep->sentreep()->hasStatic()) फर्काऊ activep;
            }
            // If there isn't one, create a new one
            File_पङ्क्ति* अचर flp = scopep->fileline();
            Ast_Sen_रूख* अचर stp = नूतन Ast_Sen_रूख{flp, नूतन AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* अचर activep = नूतन AstActive{flp, "", stp};
            activep->sen_रूख_Storep(stp);
            scopep->addBlocksp(activep);
            फर्काऊ activep;
        }();
        AstActive* अचर combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            लागि (Ast_गाँठो* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* अचर activep = VN_CAST(nodep, Active);
                यदि (!activep) जारी;
                यदि (activep->sentreep()->hasCombo()) फर्काऊ activep;
            }
            // If there isn't one, create a new one
            File_पङ्क्ति* अचर flp = scopep->fileline();
            Ast_Sen_रूख* अचर stp = नूतन Ast_Sen_रूख{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* अचर activep = नूतन AstActive{flp, "", stp};
            activep->sen_रूख_Storep(stp);
            scopep->addBlocksp(activep);
            फर्काऊ activep;
        }();

        // Required data types
        अचर DfgDataType& idxDType = srcp->dtype();
        अचर DfgDataType& bitDType = DfgDataType::packed(1);
        अचर DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            यदि (DfgVertexVar* अचर vp = srcp->getResultVar()) फर्काऊ vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* अचर vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(सत्य);
            vtxp->srcp(srcp);
            फर्काऊ vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* अचर idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* अचर preVscp = [&]() {
            DfgVertexVar* अचर vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* अचर vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(सत्य);
            vscp->varp()->noReset(सत्य);
            vscp->varp()->setIgnoreSchedWrite();
            फर्काऊ vscp;
        }();
        // The table variable
        DfgVarArray* अचर tabVtxp = [&]() {
            DfgVertexVar* अचर varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(सत्य);
            varp->vscp()->varp()->noReset(सत्य);
            varp->setHasModWrRefs();
            फर्काऊ varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* अचर initp = नूतन AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(नूतन AstAssign{
                flp,  //
                नूतन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                नूतन AstConst{flp, AstConst::Widthed_मान{}, static_cast<पूर्णाङ्क>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* अचर callp = नूतन AstCMethodHard{
                flp, नूतन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(नूतन AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* अचर logicp = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(नूतन AstAssign{
                flp,  //
                नूतन AstArraySel{flp, नूतन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                नूतन AstVarRef{flp, preVscp, VAccess::READ}},  //
                नूतन AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* अचर idxRefp = नूतन AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(नूतन AstAssign{
                flp,  //
                नूतन AstArraySel{flp, नूतन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                नूतन AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* अचर astRdp = नूतन DfgAstRd{dfg, idxRefp, असत्य, असत्य};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* अचर idxRefp = नूतन AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(नूतन AstAssign{flp,  //
                                            नूतन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* अचर astRdp = नूतन DfgAstRd{dfg, idxRefp, असत्य, असत्य};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        लागि (अचर स्वतः& pair : val2Terms) {
            अचर uint32_t val = pair.first;
            अचर std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_पङ्क्ति* अचर aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* अचर aselp = नूतन DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(नूतन DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            लागि (अचर Term& term : terms) {
                यदि (term.m_inv) {
                    यदि (!notp) {
                        notp = नूतन DfgNot{dfg, aflp, bitDType};
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
