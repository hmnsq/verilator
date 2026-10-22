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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3DfgPasses.h"

#समावेश "V3Ast.h"
#समावेश "V3Dfg.h"
#समावेश "V3File.h"
#समावेश "V3Global.h"
#समावेश "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

व्योम V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    क्रम (DfgVertex& vtx : dfg.opVertices()) {
        अगर (vtx.is<DfgLogic>()) workList.धकेलो_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* स्थिर logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        स्थिर bool used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* स्थिर uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* स्थिर vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            अगर (vVtxp->hasSinks()) वापस true;
            अगर (vVtxp->isObserved()) वापस true;
            वापस false;
        });
        // If some are used, the logic must stay in the Ast
        अगर (used) वापस;
        // If impure, it must stay in the Ast
        अगर (!logicp->isPure()) वापस;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* स्थिर varp = src.as<DfgVertexVar>();
            अगर (!varp->srcp()) वापस false;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.धकेलो_front(*driver.as<DfgLogic>());
                वापस false;
            });
            वापस false;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_गाँठ* स्थिर nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    क्रम (DfgVertexVar* स्थिर vVtxp : dfg.varVertices().unlinkable()) {
        अगर (vVtxp->hasSinks()) जारी;
        अगर (vVtxp->isObserved()) जारी;
        DfgVertex* स्थिर srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* स्थिर vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        स्थिर bool delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        अगर (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                वापस false;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        अगर (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_वृक्ष(), vscp);
            ++ctx.m_varsDeleted;
        } अन्यथा {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    क्रम (DfgVertex* स्थिर vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* स्थिर logicp = vtxp->cast<DfgLogic>();
        अगर (!logicp) जारी;
        अगर (logicp->hasSinks()) जारी;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        स्थिर VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            वापस false;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_गाँठ* varp) {  //
            वापस varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

व्योम V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    क्रम (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* स्थिर selp = vtx.cast<DfgSel>();
        अगर (!selp) जारी;
        selps.धकेलो_back(selp);
    }

    क्रम (DfgSel* स्थिर selp : selps) {
        File_पंक्ति* स्थिर flp = selp->fileline();
        स्थिर Dfg_तथ्य_Type& dtype = selp->dtype();

        // Remove full width selects
        अगर (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            जारी;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* स्थिर varp = selp->fromp()->cast<DfgVarPacked>();
        अगर (!varp || !varp->tmpForp()) जारी;

        // Find the driver of this range
        स्थिर स्वतः pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* स्थिर driverp = pair.first;
        स्थिर uint32_t driverLsb = pair.second;
        अगर (!driverp) जारी;

        // If partial driver is the whole thing we are looking for, just replace with that
        अगर (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            जारी;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* स्थिर newSelp = नया DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

व्योम V3DfgPasses::inlineVars(DfgGraph& dfg) {
    क्रम (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        अगर (!vtx.hasSinks()) जारी;
        // Nohting to inline
        DfgVertex* स्थिर srcp = vtx.srcp();
        अगर (!srcp) जारी;
        // Value can differ from driver
        अगर (vtx.isVolatile()) जारी;
        // Partial driver cannot be inlined
        अगर (srcp->is<DfgVertexSplice>()) जारी;
        अगर (srcp->is<DfgUnitArray>()) जारी;
        // Don't inline CReset
        अगर (srcp->is<DfgCReset>()) जारी;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

व्योम V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    क्रम (DfgVertex& vtx : dfg.opVertices()) {
        अगर (vtx.hasSinks()) जारी;
        // This vertex is unused. Add to work list.
        workList.धकेलो_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    क्रम (DfgVertexVar& vtx : dfg.varVertices()) {
        अगर (!vtx.tmpForp()) जारी;
        अगर (vtx.hasSinks()) जारी;
        अगर (vtx.hasDfgRefs()) जारी;
        // This vertex is unused. Add to work list.
        workList.धकेलो_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        अगर (vtx.hasSinks()) वापस;
        // If temporary used in another graph, we need to keep it
        अगर (स्थिर DfgVertexVar* स्थिर varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            अगर (varp->hasDfgRefs()) वापस;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            अगर (src.is<DfgConst>()) वापस false;
            स्थिर DfgVertexVar* स्थिर varp = src.cast<DfgVertexVar>();
            अगर (varp && !varp->tmpForp()) वापस false;
            // Add source to workList
            workList.धकेलो_front(src);
            वापस false;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    क्रम (DfgVertexVar* स्थिर vtxp : dfg.varVertices().unlinkable()) {
        अगर (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    क्रम (DfgConst* स्थिर vtxp : dfg.constVertices().unlinkable()) {
        अगर (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

व्योम V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    काष्ठा Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        bool m_inv = false;  // '!=', instead of '=='
        Term() = शेष;
        Term(DfgVertex* vtxp, bool inv)
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
    स्थिर स्वतः widthOk = [](स्थिर DfgVertex* vtxp) {
        स्थिर uint32_t width = vtxp->width();
        वापस WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    स्थिर std::function<bool(स्थिर DfgVertex*, bool)> useOk
        = [&](स्थिर DfgVertex* vtxp, bool inv) -> bool {
        // Go past a single 'Not' sink, which is common
        अगर (DfgVertex* स्थिर sinkp = vtxp->singleSink()) {
            अगर (sinkp->is<DfgNot>()) वापस useOk(sinkp, !inv);
        }
        स्थिर bool cond_वृक्ष = vtxp->foreachSink([&](स्थिर DfgVertex& sink) {
            स्थिर DfgCond* स्थिर condp = sink.cast<DfgCond>();
            अगर (!condp) वापस false;
            अगर (condp->condp() != vtxp) वापस false;
            वापस inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        वापस !cond_वृक्ष;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    क्रम (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        bool inv = false;
        अगर (DfgEq* स्थिर eqp = vtx.cast<DfgEq>()) {
            DfgConst* स्थिर constp = eqp->lhsp()->cast<DfgConst>();
            अगर (!constp || !widthOk(constp) || !useOk(eqp, false)) जारी;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = false;
        } अन्यथा अगर (DfgNeq* स्थिर neqp = vtx.cast<DfgNeq>()) {
            DfgConst* स्थिर constp = neqp->lhsp()->cast<DfgConst>();
            अगर (!constp || !widthOk(constp) || !useOk(neqp, true)) जारी;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = true;
        } अन्यथा अगर (DfgRedAnd* स्थिर redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            अगर (!widthOk(srcp) || !useOk(redAndp, false)) जारी;
            val = (1U << srcp->width()) - 1;
            inv = false;
        } अन्यथा अगर (DfgRedOr* स्थिर redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            अगर (!widthOk(srcp) || !useOk(redOrp, true)) जारी;
            val = 0;
            inv = true;
        } अन्यथा अगर (DfgSel* स्थिर selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            अगर (selp->width() != 1) जारी;
            DfgShiftL* स्थिर shiftLp = selp->fromp()->cast<DfgShiftL>();
            अगर (!shiftLp) जारी;
            स्थिर DfgConst* स्थिर constp = shiftLp->lhsp()->cast<DfgConst>();
            अगर (!constp || !useOk(selp, false)) जारी;
            अगर (!constp->has_मूल्य(1)) जारी;
            srcp = shiftLp->rhsp();
            अगर (!widthOk(srcp)) जारी;
            val = selp->lsb();
            inv = false;
        } अन्यथा {
            // Not a comparison-like vertex
            जारी;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        अगर (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    अगर (nTerms < TERM_LIMIT) वापस;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    क्रम (DfgVertex* स्थिर srcp : srcps) {
        स्थिर Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        अगर (val2Terms.आकार() < TERM_LIMIT) जारी;

        // Width of the decoded binary value
        स्थिर uint32_t width = srcp->width();
        // Number of bits in the input operand
        स्थिर uint32_t nBits = 1U << width;

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

        File_पंक्ति* स्थिर flp = srcp->fileline();
        AstScope* स्थिर scopep = srcp->scopep(scopeCache, true);

        AstActive* स्थिर staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            क्रम (Ast_गाँठ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* स्थिर activep = VN_CAST(nodep, Active);
                अगर (!activep) जारी;
                अगर (activep->sentreep()->hasStatic()) वापस activep;
            }
            // If there isn't one, create a new one
            File_पंक्ति* स्थिर flp = scopep->fileline();
            Ast_Sen_वृक्ष* स्थिर stp = नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* स्थिर activep = नया AstActive{flp, "", stp};
            activep->sen_वृक्ष_Storep(stp);
            scopep->addBlocksp(activep);
            वापस activep;
        }();
        AstActive* स्थिर combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            क्रम (Ast_गाँठ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* स्थिर activep = VN_CAST(nodep, Active);
                अगर (!activep) जारी;
                अगर (activep->sentreep()->hasCombo()) वापस activep;
            }
            // If there isn't one, create a new one
            File_पंक्ति* स्थिर flp = scopep->fileline();
            Ast_Sen_वृक्ष* स्थिर stp = नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* स्थिर activep = नया AstActive{flp, "", stp};
            activep->sen_वृक्ष_Storep(stp);
            scopep->addBlocksp(activep);
            वापस activep;
        }();

        // Required data types
        स्थिर Dfg_तथ्य_Type& idxDType = srcp->dtype();
        स्थिर Dfg_तथ्य_Type& bitDType = Dfg_तथ्य_Type::packed(1);
        स्थिर Dfg_तथ्य_Type& tabDType = Dfg_तथ्य_Type::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            अगर (DfgVertexVar* स्थिर vp = srcp->get_परिणाम_Var()) वापस vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* स्थिर vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(true);
            vtxp->srcp(srcp);
            वापस vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* स्थिर idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* स्थिर preVscp = [&]() {
            DfgVertexVar* स्थिर vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* स्थिर vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(true);
            vscp->varp()->noReset(true);
            vscp->varp()->setIgnoreSchedWrite();
            वापस vscp;
        }();
        // The table variable
        DfgVarArray* स्थिर tabVtxp = [&]() {
            DfgVertexVar* स्थिर varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(true);
            varp->vscp()->varp()->noReset(true);
            varp->setHasModWrRefs();
            वापस varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* स्थिर initp = नया AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(नया AstAssign{
                flp,  //
                नया AstVarRef{flp, preVscp, VAccess::WRITE},  //
                नया AstConst{flp, AstConst::Widthed_मूल्य{}, static_cast<पूर्णांक>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* स्थिर callp = नया AstCMethodHard{
                flp, नया AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(नया AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* स्थिर logicp = नया AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(नया AstAssign{
                flp,  //
                नया AstArraySel{flp, नया AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                नया AstVarRef{flp, preVscp, VAccess::READ}},  //
                नया AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* स्थिर idxRefp = नया AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(नया AstAssign{
                flp,  //
                नया AstArraySel{flp, नया AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                नया AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* स्थिर astRdp = नया DfgAstRd{dfg, idxRefp, false, false};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* स्थिर idxRefp = नया AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(नया AstAssign{flp,  //
                                            नया AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* स्थिर astRdp = नया DfgAstRd{dfg, idxRefp, false, false};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        क्रम (स्थिर स्वतः& pair : val2Terms) {
            स्थिर uint32_t val = pair.first;
            स्थिर std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_पंक्ति* स्थिर aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* स्थिर aselp = नया DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(नया DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            क्रम (स्थिर Term& term : terms) {
                अगर (term.m_inv) {
                    अगर (!notp) {
                        notp = नया DfgNot{dfg, aflp, bitDType};
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
