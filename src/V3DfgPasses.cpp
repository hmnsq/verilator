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
        यदि (vtx.is<DfgLogic>()) workList.धकेलो_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ध्रुव logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ध्रुव तार्किक used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ध्रुव uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ध्रुव vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            यदि (vVtxp->hasSinks()) प्रत्यागम सत्य;
            यदि (vVtxp->isObserved()) प्रत्यागम सत्य;
            प्रत्यागम असत्य;
        });
        // If some are used, the logic must stay in the Ast
        यदि (used) प्रत्यागम;
        // If impure, it must stay in the Ast
        यदि (!logicp->isPure()) प्रत्यागम;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ध्रुव varp = src.as<DfgVertexVar>();
            यदि (!varp->srcp()) प्रत्यागम असत्य;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.धकेलो_front(*driver.as<DfgLogic>());
                प्रत्यागम असत्य;
            });
            प्रत्यागम असत्य;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_गाँठ* ध्रुव nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
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
                src.as<DfgLogic>()->setDrivesUnusedVars();
                प्रत्यागम असत्य;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        यदि (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_वृक्ष(), vscp);
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
        ध्रुव VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            प्रत्यागम असत्य;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_गाँठ* varp) {  //
            प्रत्यागम varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

शून्य V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    प्रति (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ध्रुव selp = vtx.cast<DfgSel>();
        यदि (!selp) अनुवर्त;
        selps.धकेलो_back(selp);
    }

    प्रति (DfgSel* ध्रुव selp : selps) {
        File_पंक्ति* ध्रुव flp = selp->fileline();
        ध्रुव DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        यदि (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            अनुवर्त;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ध्रुव varp = selp->fromp()->cast<DfgVarPacked>();
        यदि (!varp || !varp->tmpForp()) अनुवर्त;

        // Find the driver of this range
        ध्रुव auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* ध्रुव driverp = pair.first;
        ध्रुव uint32_t driverLsb = pair.second;
        यदि (!driverp) अनुवर्त;

        // If partial driver is the whole thing we are looking for, just replace with that
        यदि (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
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

शून्य V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    प्रति (DfgVertex& vtx : dfg.opVertices()) {
        यदि (vtx.hasSinks()) अनुवर्त;
        // This vertex is unused. Add to work list.
        workList.धकेलो_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    प्रति (DfgVertexVar& vtx : dfg.varVertices()) {
        यदि (!vtx.tmpForp()) अनुवर्त;
        यदि (vtx.hasSinks()) अनुवर्त;
        यदि (vtx.hasDfgRefs()) अनुवर्त;
        // This vertex is unused. Add to work list.
        workList.धकेलो_front(vtx);
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
            workList.धकेलो_front(src);
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
    ध्रुव auto widthOk = [](ध्रुव DfgVertex* vtxp) {
        ध्रुव uint32_t width = vtxp->width();
        प्रत्यागम WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ध्रुव std::function<तार्किक(ध्रुव DfgVertex*, तार्किक)> useOk
        = [&](ध्रुव DfgVertex* vtxp, तार्किक inv) -> तार्किक {
        // Go past a single 'Not' sink, which is common
        यदि (DfgVertex* ध्रुव sinkp = vtxp->singleSink()) {
            यदि (sinkp->is<DfgNot>()) प्रत्यागम useOk(sinkp, !inv);
        }
        ध्रुव तार्किक cond_वृक्ष = vtxp->foreachSink([&](ध्रुव DfgVertex& sink) {
            ध्रुव DfgCond* ध्रुव condp = sink.cast<DfgCond>();
            यदि (!condp) प्रत्यागम असत्य;
            यदि (condp->condp() != vtxp) प्रत्यागम असत्य;
            प्रत्यागम inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        प्रत्यागम !cond_वृक्ष;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    प्रति (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        तार्किक inv = असत्य;
        यदि (DfgEq* ध्रुव eqp = vtx.cast<DfgEq>()) {
            DfgConst* ध्रुव constp = eqp->lhsp()->cast<DfgConst>();
            यदि (!constp || !widthOk(constp) || !useOk(eqp, असत्य)) अनुवर्त;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = असत्य;
        } अन्यथा यदि (DfgNeq* ध्रुव neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ध्रुव constp = neqp->lhsp()->cast<DfgConst>();
            यदि (!constp || !widthOk(constp) || !useOk(neqp, सत्य)) अनुवर्त;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = सत्य;
        } अन्यथा यदि (DfgRedAnd* ध्रुव redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            यदि (!widthOk(srcp) || !useOk(redAndp, असत्य)) अनुवर्त;
            val = (1U << srcp->width()) - 1;
            inv = असत्य;
        } अन्यथा यदि (DfgRedOr* ध्रुव redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            यदि (!widthOk(srcp) || !useOk(redOrp, सत्य)) अनुवर्त;
            val = 0;
            inv = सत्य;
        } अन्यथा यदि (DfgSel* ध्रुव selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            यदि (selp->width() != 1) अनुवर्त;
            DfgShiftL* ध्रुव shiftLp = selp->fromp()->cast<DfgShiftL>();
            यदि (!shiftLp) अनुवर्त;
            ध्रुव DfgConst* ध्रुव constp = shiftLp->lhsp()->cast<DfgConst>();
            यदि (!constp || !useOk(selp, असत्य)) अनुवर्त;
            यदि (!constp->has_मूल्य(1)) अनुवर्त;
            srcp = shiftLp->rhsp();
            यदि (!widthOk(srcp)) अनुवर्त;
            val = selp->lsb();
            inv = असत्य;
        } अन्यथा {
            // Not a comparison-like vertex
            अनुवर्त;
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
    यदि (nTerms < TERM_LIMIT) प्रत्यागम;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    प्रति (DfgVertex* ध्रुव srcp : srcps) {
        ध्रुव Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        यदि (val2Terms.size() < TERM_LIMIT) अनुवर्त;

        // Width of the decoded binary value
        ध्रुव uint32_t width = srcp->width();
        // Number of bits in the input operand
        ध्रुव uint32_t nBits = 1U << width;

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

        File_पंक्ति* ध्रुव flp = srcp->fileline();
        AstScope* ध्रुव scopep = srcp->scopep(scopeCache, सत्य);

        AstActive* ध्रुव staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            प्रति (Ast_गाँठ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ध्रुव activep = VN_CAST(nodep, Active);
                यदि (!activep) अनुवर्त;
                यदि (activep->sentreep()->hasStatic()) प्रत्यागम activep;
            }
            // If there isn't one, create a new one
            File_पंक्ति* ध्रुव flp = scopep->fileline();
            Ast_Sen_वृक्ष* ध्रुव stp = नूतन Ast_Sen_वृक्ष{flp, नूतन AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* ध्रुव activep = नूतन AstActive{flp, "", stp};
            activep->sen_वृक्ष_Storep(stp);
            scopep->addBlocksp(activep);
            प्रत्यागम activep;
        }();
        AstActive* ध्रुव combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            प्रति (Ast_गाँठ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ध्रुव activep = VN_CAST(nodep, Active);
                यदि (!activep) अनुवर्त;
                यदि (activep->sentreep()->hasCombo()) प्रत्यागम activep;
            }
            // If there isn't one, create a new one
            File_पंक्ति* ध्रुव flp = scopep->fileline();
            Ast_Sen_वृक्ष* ध्रुव stp = नूतन Ast_Sen_वृक्ष{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* ध्रुव activep = नूतन AstActive{flp, "", stp};
            activep->sen_वृक्ष_Storep(stp);
            scopep->addBlocksp(activep);
            प्रत्यागम activep;
        }();

        // Required data types
        ध्रुव DfgDataType& idxDType = srcp->dtype();
        ध्रुव DfgDataType& bitDType = DfgDataType::packed(1);
        ध्रुव DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            यदि (DfgVertexVar* ध्रुव vp = srcp->getResultVar()) प्रत्यागम vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ध्रुव vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(सत्य);
            vtxp->srcp(srcp);
            प्रत्यागम vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ध्रुव idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ध्रुव preVscp = [&]() {
            DfgVertexVar* ध्रुव vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ध्रुव vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(सत्य);
            vscp->varp()->noReset(सत्य);
            vscp->varp()->setIgnoreSchedWrite();
            प्रत्यागम vscp;
        }();
        // The table variable
        DfgVarArray* ध्रुव tabVtxp = [&]() {
            DfgVertexVar* ध्रुव varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(सत्य);
            varp->vscp()->varp()->noReset(सत्य);
            varp->setHasModWrRefs();
            प्रत्यागम varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ध्रुव initp = नूतन AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(नूतन AstAssign{
                flp,  //
                नूतन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                नूतन AstConst{flp, AstConst::Widthed_मूल्य{}, static_cast<पूर्णाङ्क>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                flp, नूतन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(नूतन AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ध्रुव logicp = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(नूतन AstAssign{
                flp,  //
                नूतन AstArraySel{flp, नूतन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                नूतन AstVarRef{flp, preVscp, VAccess::READ}},  //
                नूतन AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ध्रुव idxRefp = नूतन AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(नूतन AstAssign{
                flp,  //
                नूतन AstArraySel{flp, नूतन AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                नूतन AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* ध्रुव astRdp = नूतन DfgAstRd{dfg, idxRefp, असत्य, असत्य};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ध्रुव idxRefp = नूतन AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(नूतन AstAssign{flp,  //
                                            नूतन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ध्रुव astRdp = नूतन DfgAstRd{dfg, idxRefp, असत्य, असत्य};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        प्रति (ध्रुव auto& pair : val2Terms) {
            ध्रुव uint32_t val = pair.first;
            ध्रुव std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_पंक्ति* ध्रुव aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ध्रुव aselp = नूतन DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(नूतन DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            प्रति (ध्रुव Term& term : terms) {
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
