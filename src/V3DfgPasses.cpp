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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3DfgPasses.h"

#شامل "V3Ast.h"
#شامل "V3Dfg.h"
#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

تهی V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    برای (DfgVertex& vtx : dfg.opVertices()) {
        اگر (vtx.is<DfgLogic>()) workList.افزودن_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ثابت logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ثابت منطقی used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ثابت uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ثابت vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            اگر (vVtxp->hasSinks()) بازگشت درست;
            اگر (vVtxp->isObserved()) بازگشت درست;
            بازگشت نادرست;
        });
        // If some are used, the logic must stay in the Ast
        اگر (used) بازگشت;
        // If impure, it must stay in the Ast
        اگر (!logicp->isPure()) بازگشت;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ثابت varp = src.as<DfgVertexVar>();
            اگر (!varp->srcp()) بازگشت نادرست;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.افزودن_front(*driver.as<DfgLogic>());
                بازگشت نادرست;
            });
            بازگشت نادرست;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_گره* ثابت nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    برای (DfgVertexVar* ثابت vVtxp : dfg.varVertices().unlinkable()) {
        اگر (vVtxp->hasSinks()) ادامه;
        اگر (vVtxp->isObserved()) ادامه;
        DfgVertex* ثابت srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ثابت vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ثابت منطقی delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        اگر (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                بازگشت نادرست;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        اگر (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_درخت(), vscp);
            ++ctx.m_varsDeleted;
        } وگرنه {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    برای (DfgVertex* ثابت vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
        اگر (!logicp) ادامه;
        اگر (logicp->hasSinks()) ادامه;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ثابت VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            بازگشت نادرست;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_گره* varp) {  //
            بازگشت varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

تهی V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    برای (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ثابت selp = vtx.cast<DfgSel>();
        اگر (!selp) ادامه;
        selps.افزودن_back(selp);
    }

    برای (DfgSel* ثابت selp : selps) {
        File_سطر* ثابت flp = selp->fileline();
        ثابت DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        اگر (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ادامه;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ثابت varp = selp->fromp()->cast<DfgVarPacked>();
        اگر (!varp || !varp->tmpForp()) ادامه;

        // Find the driver of this range
        ثابت auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* ثابت driverp = pair.first;
        ثابت uint32_t driverLsb = pair.second;
        اگر (!driverp) ادامه;

        // If partial driver is the whole thing we are looking for, just replace with that
        اگر (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ادامه;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ثابت newSelp = جدید DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

تهی V3DfgPasses::inlineVars(DfgGraph& dfg) {
    برای (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        اگر (!vtx.hasSinks()) ادامه;
        // Nohting to inline
        DfgVertex* ثابت srcp = vtx.srcp();
        اگر (!srcp) ادامه;
        // Value can differ from driver
        اگر (vtx.isVolatile()) ادامه;
        // Partial driver cannot be inlined
        اگر (srcp->is<DfgVertexSplice>()) ادامه;
        اگر (srcp->is<DfgUnitArray>()) ادامه;
        // Don't inline CReset
        اگر (srcp->is<DfgCReset>()) ادامه;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

تهی V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    برای (DfgVertex& vtx : dfg.opVertices()) {
        اگر (vtx.hasSinks()) ادامه;
        // This vertex is unused. Add to work list.
        workList.افزودن_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    برای (DfgVertexVar& vtx : dfg.varVertices()) {
        اگر (!vtx.tmpForp()) ادامه;
        اگر (vtx.hasSinks()) ادامه;
        اگر (vtx.hasDfgRefs()) ادامه;
        // This vertex is unused. Add to work list.
        workList.افزودن_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        اگر (vtx.hasSinks()) بازگشت;
        // If temporary used in another graph, we need to keep it
        اگر (ثابت DfgVertexVar* ثابت varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            اگر (varp->hasDfgRefs()) بازگشت;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            اگر (src.is<DfgConst>()) بازگشت نادرست;
            ثابت DfgVertexVar* ثابت varp = src.cast<DfgVertexVar>();
            اگر (varp && !varp->tmpForp()) بازگشت نادرست;
            // Add source to workList
            workList.افزودن_front(src);
            بازگشت نادرست;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    برای (DfgVertexVar* ثابت vtxp : dfg.varVertices().unlinkable()) {
        اگر (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    برای (DfgConst* ثابت vtxp : dfg.constVertices().unlinkable()) {
        اگر (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

تهی V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    ساختار Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        منطقی m_inv = نادرست;  // '!=', instead of '=='
        Term() = پیشفرض;
        Term(DfgVertex* vtxp, منطقی inv)
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
        بازگشت WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ثابت std::function<منطقی(ثابت DfgVertex*, منطقی)> useOk
        = [&](ثابت DfgVertex* vtxp, منطقی inv) -> منطقی {
        // Go past a single 'Not' sink, which is common
        اگر (DfgVertex* ثابت sinkp = vtxp->singleSink()) {
            اگر (sinkp->is<DfgNot>()) بازگشت useOk(sinkp, !inv);
        }
        ثابت منطقی cond_درخت = vtxp->foreachSink([&](ثابت DfgVertex& sink) {
            ثابت DfgCond* ثابت condp = sink.cast<DfgCond>();
            اگر (!condp) بازگشت نادرست;
            اگر (condp->condp() != vtxp) بازگشت نادرست;
            بازگشت inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        بازگشت !cond_درخت;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    برای (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        منطقی inv = نادرست;
        اگر (DfgEq* ثابت eqp = vtx.cast<DfgEq>()) {
            DfgConst* ثابت constp = eqp->lhsp()->cast<DfgConst>();
            اگر (!constp || !widthOk(constp) || !useOk(eqp, نادرست)) ادامه;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = نادرست;
        } وگرنه اگر (DfgNeq* ثابت neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ثابت constp = neqp->lhsp()->cast<DfgConst>();
            اگر (!constp || !widthOk(constp) || !useOk(neqp, درست)) ادامه;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = درست;
        } وگرنه اگر (DfgRedAnd* ثابت redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            اگر (!widthOk(srcp) || !useOk(redAndp, نادرست)) ادامه;
            val = (1U << srcp->width()) - 1;
            inv = نادرست;
        } وگرنه اگر (DfgRedOr* ثابت redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            اگر (!widthOk(srcp) || !useOk(redOrp, درست)) ادامه;
            val = 0;
            inv = درست;
        } وگرنه اگر (DfgSel* ثابت selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            اگر (selp->width() != 1) ادامه;
            DfgShiftL* ثابت shiftLp = selp->fromp()->cast<DfgShiftL>();
            اگر (!shiftLp) ادامه;
            ثابت DfgConst* ثابت constp = shiftLp->lhsp()->cast<DfgConst>();
            اگر (!constp || !useOk(selp, نادرست)) ادامه;
            اگر (!constp->has_مقدار(1)) ادامه;
            srcp = shiftLp->rhsp();
            اگر (!widthOk(srcp)) ادامه;
            val = selp->lsb();
            inv = نادرست;
        } وگرنه {
            // Not a comparison-like vertex
            ادامه;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        اگر (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    اگر (nTerms < TERM_LIMIT) بازگشت;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    برای (DfgVertex* ثابت srcp : srcps) {
        ثابت Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        اگر (val2Terms.size() < TERM_LIMIT) ادامه;

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

        File_سطر* ثابت flp = srcp->fileline();
        AstScope* ثابت scopep = srcp->scopep(scopeCache, درست);

        AstActive* ثابت staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            برای (Ast_گره* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ثابت activep = VN_CAST(nodep, Active);
                اگر (!activep) ادامه;
                اگر (activep->sentreep()->hasStatic()) بازگشت activep;
            }
            // If there isn't one, create a new one
            File_سطر* ثابت flp = scopep->fileline();
            Ast_Sen_درخت* ثابت stp = جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* ثابت activep = جدید AstActive{flp, "", stp};
            activep->sen_درخت_Storep(stp);
            scopep->addBlocksp(activep);
            بازگشت activep;
        }();
        AstActive* ثابت combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            برای (Ast_گره* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ثابت activep = VN_CAST(nodep, Active);
                اگر (!activep) ادامه;
                اگر (activep->sentreep()->hasCombo()) بازگشت activep;
            }
            // If there isn't one, create a new one
            File_سطر* ثابت flp = scopep->fileline();
            Ast_Sen_درخت* ثابت stp = جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* ثابت activep = جدید AstActive{flp, "", stp};
            activep->sen_درخت_Storep(stp);
            scopep->addBlocksp(activep);
            بازگشت activep;
        }();

        // Required data types
        ثابت DfgDataType& idxDType = srcp->dtype();
        ثابت DfgDataType& bitDType = DfgDataType::packed(1);
        ثابت DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            اگر (DfgVertexVar* ثابت vp = srcp->getResultVar()) بازگشت vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ثابت vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(درست);
            vtxp->srcp(srcp);
            بازگشت vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ثابت idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ثابت preVscp = [&]() {
            DfgVertexVar* ثابت vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ثابت vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(درست);
            vscp->varp()->noReset(درست);
            vscp->varp()->setIgnoreSchedWrite();
            بازگشت vscp;
        }();
        // The table variable
        DfgVarArray* ثابت tabVtxp = [&]() {
            DfgVertexVar* ثابت varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(درست);
            varp->vscp()->varp()->noReset(درست);
            varp->setHasModWrRefs();
            بازگشت varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ثابت initp = جدید AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(جدید AstAssign{
                flp,  //
                جدید AstVarRef{flp, preVscp, VAccess::WRITE},  //
                جدید AstConst{flp, AstConst::Widthed_مقدار{}, static_cast<صحیح>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ثابت callp = جدید AstCMethodHard{
                flp, جدید AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(جدید AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ثابت logicp = جدید AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(جدید AstAssign{
                flp,  //
                جدید AstArraySel{flp, جدید AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                جدید AstVarRef{flp, preVscp, VAccess::READ}},  //
                جدید AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ثابت idxRefp = جدید AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(جدید AstAssign{
                flp,  //
                جدید AstArraySel{flp, جدید AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                جدید AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* ثابت astRdp = جدید DfgAstRd{dfg, idxRefp, نادرست, نادرست};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ثابت idxRefp = جدید AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(جدید AstAssign{flp,  //
                                            جدید AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ثابت astRdp = جدید DfgAstRd{dfg, idxRefp, نادرست, نادرست};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        برای (ثابت auto& pair : val2Terms) {
            ثابت uint32_t val = pair.first;
            ثابت std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_سطر* ثابت aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ثابت aselp = جدید DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(جدید DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            برای (ثابت Term& term : terms) {
                اگر (term.m_inv) {
                    اگر (!notp) {
                        notp = جدید DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } وگرنه {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
