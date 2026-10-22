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
    अत्थाय (DfgVertex& vtx : dfg.opVertices()) {
        सचे (vtx.is<DfgLogic>()) workList.प्रक्षिप_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ध्रुव logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ध्रुव तार्किक used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ध्रुव uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ध्रुव vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            सचे (vVtxp->hasSinks()) पटिनिवत्त सच्च;
            सचे (vVtxp->isObserved()) पटिनिवत्त सच्च;
            पटिनिवत्त मिच्छा;
        });
        // If some are used, the logic must stay in the Ast
        सचे (used) पटिनिवत्त;
        // If impure, it must stay in the Ast
        सचे (!logicp->isPure()) पटिनिवत्त;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ध्रुव varp = src.as<DfgVertexVar>();
            सचे (!varp->srcp()) पटिनिवत्त मिच्छा;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.प्रक्षिप_front(*driver.as<DfgLogic>());
                पटिनिवत्त मिच्छा;
            });
            पटिनिवत्त मिच्छा;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_ग्रन्थिः* ध्रुव nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    अत्थाय (DfgVertexVar* ध्रुव vVtxp : dfg.varVertices().unlinkable()) {
        सचे (vVtxp->hasSinks()) अनुवर्त;
        सचे (vVtxp->isObserved()) अनुवर्त;
        DfgVertex* ध्रुव srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ध्रुव vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ध्रुव तार्किक delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        सचे (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                पटिनिवत्त मिच्छा;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        सचे (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_वृक्षः(), vscp);
            ++ctx.m_varsDeleted;
        } अञ्ञथा {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    अत्थाय (DfgVertex* ध्रुव vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ध्रुव logicp = vtxp->cast<DfgLogic>();
        सचे (!logicp) अनुवर्त;
        सचे (logicp->hasSinks()) अनुवर्त;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ध्रुव VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            पटिनिवत्त मिच्छा;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_ग्रन्थिः* varp) {  //
            पटिनिवत्त varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

शून्य V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    अत्थाय (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ध्रुव selp = vtx.cast<DfgSel>();
        सचे (!selp) अनुवर्त;
        selps.प्रक्षिप_back(selp);
    }

    अत्थाय (DfgSel* ध्रुव selp : selps) {
        File_पङ्क्तिः* ध्रुव flp = selp->fileline();
        ध्रुव DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        सचे (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            अनुवर्त;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ध्रुव varp = selp->fromp()->cast<DfgVarPacked>();
        सचे (!varp || !varp->tmpForp()) अनुवर्त;

        // Find the driver of this range
        ध्रुव auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* ध्रुव driverp = pair.first;
        ध्रुव uint32_t driverLsb = pair.second;
        सचे (!driverp) अनुवर्त;

        // If partial driver is the whole thing we are looking for, just replace with that
        सचे (driverp->dtype() == dtype) {
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
    अत्थाय (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        सचे (!vtx.hasSinks()) अनुवर्त;
        // Nohting to inline
        DfgVertex* ध्रुव srcp = vtx.srcp();
        सचे (!srcp) अनुवर्त;
        // Value can differ from driver
        सचे (vtx.isVolatile()) अनुवर्त;
        // Partial driver cannot be inlined
        सचे (srcp->is<DfgVertexSplice>()) अनुवर्त;
        सचे (srcp->is<DfgUnitArray>()) अनुवर्त;
        // Don't inline CReset
        सचे (srcp->is<DfgCReset>()) अनुवर्त;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

शून्य V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    अत्थाय (DfgVertex& vtx : dfg.opVertices()) {
        सचे (vtx.hasSinks()) अनुवर्त;
        // This vertex is unused. Add to work list.
        workList.प्रक्षिप_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    अत्थाय (DfgVertexVar& vtx : dfg.varVertices()) {
        सचे (!vtx.tmpForp()) अनुवर्त;
        सचे (vtx.hasSinks()) अनुवर्त;
        सचे (vtx.hasDfgRefs()) अनुवर्त;
        // This vertex is unused. Add to work list.
        workList.प्रक्षिप_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        सचे (vtx.hasSinks()) पटिनिवत्त;
        // If temporary used in another graph, we need to keep it
        सचे (ध्रुव DfgVertexVar* ध्रुव varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            सचे (varp->hasDfgRefs()) पटिनिवत्त;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            सचे (src.is<DfgConst>()) पटिनिवत्त मिच्छा;
            ध्रुव DfgVertexVar* ध्रुव varp = src.cast<DfgVertexVar>();
            सचे (varp && !varp->tmpForp()) पटिनिवत्त मिच्छा;
            // Add source to workList
            workList.प्रक्षिप_front(src);
            पटिनिवत्त मिच्छा;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    अत्थाय (DfgVertexVar* ध्रुव vtxp : dfg.varVertices().unlinkable()) {
        सचे (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    अत्थाय (DfgConst* ध्रुव vtxp : dfg.constVertices().unlinkable()) {
        सचे (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

शून्य V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    संरचना Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        तार्किक m_inv = मिच्छा;  // '!=', instead of '=='
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
        पटिनिवत्त WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ध्रुव std::function<तार्किक(ध्रुव DfgVertex*, तार्किक)> useOk
        = [&](ध्रुव DfgVertex* vtxp, तार्किक inv) -> तार्किक {
        // Go past a single 'Not' sink, which is common
        सचे (DfgVertex* ध्रुव sinkp = vtxp->singleSink()) {
            सचे (sinkp->is<DfgNot>()) पटिनिवत्त useOk(sinkp, !inv);
        }
        ध्रुव तार्किक cond_वृक्षः = vtxp->foreachSink([&](ध्रुव DfgVertex& sink) {
            ध्रुव DfgCond* ध्रुव condp = sink.cast<DfgCond>();
            सचे (!condp) पटिनिवत्त मिच्छा;
            सचे (condp->condp() != vtxp) पटिनिवत्त मिच्छा;
            पटिनिवत्त inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        पटिनिवत्त !cond_वृक्षः;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    अत्थाय (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        तार्किक inv = मिच्छा;
        सचे (DfgEq* ध्रुव eqp = vtx.cast<DfgEq>()) {
            DfgConst* ध्रुव constp = eqp->lhsp()->cast<DfgConst>();
            सचे (!constp || !widthOk(constp) || !useOk(eqp, मिच्छा)) अनुवर्त;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = मिच्छा;
        } अञ्ञथा सचे (DfgNeq* ध्रुव neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ध्रुव constp = neqp->lhsp()->cast<DfgConst>();
            सचे (!constp || !widthOk(constp) || !useOk(neqp, सच्च)) अनुवर्त;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = सच्च;
        } अञ्ञथा सचे (DfgRedAnd* ध्रुव redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            सचे (!widthOk(srcp) || !useOk(redAndp, मिच्छा)) अनुवर्त;
            val = (1U << srcp->width()) - 1;
            inv = मिच्छा;
        } अञ्ञथा सचे (DfgRedOr* ध्रुव redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            सचे (!widthOk(srcp) || !useOk(redOrp, सच्च)) अनुवर्त;
            val = 0;
            inv = सच्च;
        } अञ्ञथा सचे (DfgSel* ध्रुव selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            सचे (selp->width() != 1) अनुवर्त;
            DfgShiftL* ध्रुव shiftLp = selp->fromp()->cast<DfgShiftL>();
            सचे (!shiftLp) अनुवर्त;
            ध्रुव DfgConst* ध्रुव constp = shiftLp->lhsp()->cast<DfgConst>();
            सचे (!constp || !useOk(selp, मिच्छा)) अनुवर्त;
            सचे (!constp->has_मानम्(1)) अनुवर्त;
            srcp = shiftLp->rhsp();
            सचे (!widthOk(srcp)) अनुवर्त;
            val = selp->lsb();
            inv = मिच्छा;
        } अञ्ञथा {
            // Not a comparison-like vertex
            अनुवर्त;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        सचे (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    सचे (nTerms < TERM_LIMIT) पटिनिवत्त;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    अत्थाय (DfgVertex* ध्रुव srcp : srcps) {
        ध्रुव Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        सचे (val2Terms.size() < TERM_LIMIT) अनुवर्त;

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

        File_पङ्क्तिः* ध्रुव flp = srcp->fileline();
        AstScope* ध्रुव scopep = srcp->scopep(scopeCache, सच्च);

        AstActive* ध्रुव staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            अत्थाय (Ast_ग्रन्थिः* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ध्रुव activep = VN_CAST(nodep, Active);
                सचे (!activep) अनुवर्त;
                सचे (activep->sentreep()->hasStatic()) पटिनिवत्त activep;
            }
            // If there isn't one, create a new one
            File_पङ्क्तिः* ध्रुव flp = scopep->fileline();
            Ast_Sen_वृक्षः* ध्रुव stp = नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* ध्रुव activep = नूतन AstActive{flp, "", stp};
            activep->sen_वृक्षः_Storep(stp);
            scopep->addBlocksp(activep);
            पटिनिवत्त activep;
        }();
        AstActive* ध्रुव combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            अत्थाय (Ast_ग्रन्थिः* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ध्रुव activep = VN_CAST(nodep, Active);
                सचे (!activep) अनुवर्त;
                सचे (activep->sentreep()->hasCombo()) पटिनिवत्त activep;
            }
            // If there isn't one, create a new one
            File_पङ्क्तिः* ध्रुव flp = scopep->fileline();
            Ast_Sen_वृक्षः* ध्रुव stp = नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* ध्रुव activep = नूतन AstActive{flp, "", stp};
            activep->sen_वृक्षः_Storep(stp);
            scopep->addBlocksp(activep);
            पटिनिवत्त activep;
        }();

        // Required data types
        ध्रुव DfgDataType& idxDType = srcp->dtype();
        ध्रुव DfgDataType& bitDType = DfgDataType::packed(1);
        ध्रुव DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            सचे (DfgVertexVar* ध्रुव vp = srcp->getResultVar()) पटिनिवत्त vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ध्रुव vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(सच्च);
            vtxp->srcp(srcp);
            पटिनिवत्त vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ध्रुव idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ध्रुव preVscp = [&]() {
            DfgVertexVar* ध्रुव vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ध्रुव vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(सच्च);
            vscp->varp()->noReset(सच्च);
            vscp->varp()->setIgnoreSchedWrite();
            पटिनिवत्त vscp;
        }();
        // The table variable
        DfgVarArray* ध्रुव tabVtxp = [&]() {
            DfgVertexVar* ध्रुव varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(सच्च);
            varp->vscp()->varp()->noReset(सच्च);
            varp->setHasModWrRefs();
            पटिनिवत्त varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ध्रुव initp = नूतन AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(नूतन AstAssign{
                flp,  //
                नूतन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                नूतन AstConst{flp, AstConst::Widthed_मानम्{}, static_cast<पूर्णाङ्क>(width), 0}});
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
            DfgAstRd* ध्रुव astRdp = नूतन DfgAstRd{dfg, idxRefp, मिच्छा, मिच्छा};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ध्रुव idxRefp = नूतन AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(नूतन AstAssign{flp,  //
                                            नूतन AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ध्रुव astRdp = नूतन DfgAstRd{dfg, idxRefp, मिच्छा, मिच्छा};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        अत्थाय (ध्रुव auto& pair : val2Terms) {
            ध्रुव uint32_t val = pair.first;
            ध्रुव std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_पङ्क्तिः* ध्रुव aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ध्रुव aselp = नूतन DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(नूतन DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            अत्थाय (ध्रुव Term& term : terms) {
                सचे (term.m_inv) {
                    सचे (!notp) {
                        notp = नूतन DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } अञ्ञथा {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
