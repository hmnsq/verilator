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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ಶೂನ್ಯ V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    ಪ್ರತಿ (DfgVertex& vtx : dfg.opVertices()) {
        ಯದಿ (vtx.is<DfgLogic>()) workList.ತಳ್ಳು_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* ಧ್ರುವ logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ಧ್ರುವ ತಾರ್ಕಿಕ used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* ಧ್ರುವ uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* ಧ್ರುವ vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            ಯದಿ (vVtxp->hasSinks()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಯದಿ (vVtxp->isObserved()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        // If some are used, the logic must stay in the Ast
        ಯದಿ (used) ಪ್ರತ್ಯಾಗಮ;
        // If impure, it must stay in the Ast
        ಯದಿ (!logicp->isPure()) ಪ್ರತ್ಯಾಗಮ;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* ಧ್ರುವ varp = src.as<DfgVertexVar>();
            ಯದಿ (!varp->srcp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.ತಳ್ಳು_front(*driver.as<DfgLogic>());
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_ಗಂಟು* ಧ್ರುವ nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    ಪ್ರತಿ (DfgVertexVar* ಧ್ರುವ vVtxp : dfg.varVertices().unlinkable()) {
        ಯದಿ (vVtxp->hasSinks()) ಅನುವರ್ತ;
        ಯದಿ (vVtxp->isObserved()) ಅನುವರ್ತ;
        DfgVertex* ಧ್ರುವ srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ಧ್ರುವ vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ಧ್ರುವ ತಾರ್ಕಿಕ delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        ಯದಿ (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        ಯದಿ (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->delete_ವೃಕ್ಷ(), vscp);
            ++ctx.m_varsDeleted;
        } ಅನ್ಯಥಾ {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ಧ್ರುವ logicp = vtxp->cast<DfgLogic>();
        ಯದಿ (!logicp) ಅನುವರ್ತ;
        ಯದಿ (logicp->hasSinks()) ಅನುವರ್ತ;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ಧ್ರುವ VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](Ast_ಗಂಟು* varp) {  //
            ಪ್ರತ್ಯಾಗಮ varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

ಶೂನ್ಯ V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    ಪ್ರತಿ (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* ಧ್ರುವ selp = vtx.cast<DfgSel>();
        ಯದಿ (!selp) ಅನುವರ್ತ;
        selps.ತಳ್ಳು_back(selp);
    }

    ಪ್ರತಿ (DfgSel* ಧ್ರುವ selp : selps) {
        File_ಸಾಲು* ಧ್ರುವ flp = selp->fileline();
        ಧ್ರುವ DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        ಯದಿ (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ಅನುವರ್ತ;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ಧ್ರುವ varp = selp->fromp()->cast<DfgVarPacked>();
        ಯದಿ (!varp || !varp->tmpForp()) ಅನುವರ್ತ;

        // Find the driver of this range
        ಧ್ರುವ auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* ಧ್ರುವ driverp = pair.first;
        ಧ್ರುವ uint32_t driverLsb = pair.second;
        ಯದಿ (!driverp) ಅನುವರ್ತ;

        // If partial driver is the whole thing we are looking for, just replace with that
        ಯದಿ (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ಅನುವರ್ತ;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ಧ್ರುವ newSelp = ನೂತನ DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

ಶೂನ್ಯ V3DfgPasses::inlineVars(DfgGraph& dfg) {
    ಪ್ರತಿ (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        ಯದಿ (!vtx.hasSinks()) ಅನುವರ್ತ;
        // Nohting to inline
        DfgVertex* ಧ್ರುವ srcp = vtx.srcp();
        ಯದಿ (!srcp) ಅನುವರ್ತ;
        // Value can differ from driver
        ಯದಿ (vtx.isVolatile()) ಅನುವರ್ತ;
        // Partial driver cannot be inlined
        ಯದಿ (srcp->is<DfgVertexSplice>()) ಅನುವರ್ತ;
        ಯದಿ (srcp->is<DfgUnitArray>()) ಅನುವರ್ತ;
        // Don't inline CReset
        ಯದಿ (srcp->is<DfgCReset>()) ಅನುವರ್ತ;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

ಶೂನ್ಯ V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    ಪ್ರತಿ (DfgVertex& vtx : dfg.opVertices()) {
        ಯದಿ (vtx.hasSinks()) ಅನುವರ್ತ;
        // This vertex is unused. Add to work list.
        workList.ತಳ್ಳು_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    ಪ್ರತಿ (DfgVertexVar& vtx : dfg.varVertices()) {
        ಯದಿ (!vtx.tmpForp()) ಅನುವರ್ತ;
        ಯದಿ (vtx.hasSinks()) ಅನುವರ್ತ;
        ಯದಿ (vtx.hasDfgRefs()) ಅನುವರ್ತ;
        // This vertex is unused. Add to work list.
        workList.ತಳ್ಳು_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        ಯದಿ (vtx.hasSinks()) ಪ್ರತ್ಯಾಗಮ;
        // If temporary used in another graph, we need to keep it
        ಯದಿ (ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            ಯದಿ (varp->hasDfgRefs()) ಪ್ರತ್ಯಾಗಮ;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            ಯದಿ (src.is<DfgConst>()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varp = src.cast<DfgVertexVar>();
            ಯದಿ (varp && !varp->tmpForp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Add source to workList
            workList.ತಳ್ಳು_front(src);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    ಪ್ರತಿ (DfgVertexVar* ಧ್ರುವ vtxp : dfg.varVertices().unlinkable()) {
        ಯದಿ (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    ಪ್ರತಿ (DfgConst* ಧ್ರುವ vtxp : dfg.constVertices().unlinkable()) {
        ಯದಿ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

ಶೂನ್ಯ V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    ಸಂರಚನಾ Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        ತಾರ್ಕಿಕ m_inv = ಅಸತ್ಯ;  // '!=', instead of '=='
        Term() = ಸ್ವಭಾವ;
        Term(DfgVertex* vtxp, ತಾರ್ಕಿಕ inv)
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
    ಧ್ರುವ auto widthOk = [](ಧ್ರುವ DfgVertex* vtxp) {
        ಧ್ರುವ uint32_t width = vtxp->width();
        ಪ್ರತ್ಯಾಗಮ WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ಧ್ರುವ std::function<ತಾರ್ಕಿಕ(ಧ್ರುವ DfgVertex*, ತಾರ್ಕಿಕ)> useOk
        = [&](ಧ್ರುವ DfgVertex* vtxp, ತಾರ್ಕಿಕ inv) -> ತಾರ್ಕಿಕ {
        // Go past a single 'Not' sink, which is common
        ಯದಿ (DfgVertex* ಧ್ರುವ sinkp = vtxp->singleSink()) {
            ಯದಿ (sinkp->is<DfgNot>()) ಪ್ರತ್ಯಾಗಮ useOk(sinkp, !inv);
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ cond_ವೃಕ್ಷ = vtxp->foreachSink([&](ಧ್ರುವ DfgVertex& sink) {
            ಧ್ರುವ DfgCond* ಧ್ರುವ condp = sink.cast<DfgCond>();
            ಯದಿ (!condp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (condp->condp() != vtxp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        ಪ್ರತ್ಯಾಗಮ !cond_ವೃಕ್ಷ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    ಪ್ರತಿ (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        ತಾರ್ಕಿಕ inv = ಅಸತ್ಯ;
        ಯದಿ (DfgEq* ಧ್ರುವ eqp = vtx.cast<DfgEq>()) {
            DfgConst* ಧ್ರುವ constp = eqp->lhsp()->cast<DfgConst>();
            ಯದಿ (!constp || !widthOk(constp) || !useOk(eqp, ಅಸತ್ಯ)) ಅನುವರ್ತ;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (DfgNeq* ಧ್ರುವ neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ಧ್ರುವ constp = neqp->lhsp()->cast<DfgConst>();
            ಯದಿ (!constp || !widthOk(constp) || !useOk(neqp, ಸತ್ಯ)) ಅನುವರ್ತ;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (DfgRedAnd* ಧ್ರುವ redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            ಯದಿ (!widthOk(srcp) || !useOk(redAndp, ಅಸತ್ಯ)) ಅನುವರ್ತ;
            val = (1U << srcp->width()) - 1;
            inv = ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (DfgRedOr* ಧ್ರುವ redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            ಯದಿ (!widthOk(srcp) || !useOk(redOrp, ಸತ್ಯ)) ಅನುವರ್ತ;
            val = 0;
            inv = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (DfgSel* ಧ್ರುವ selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            ಯದಿ (selp->width() != 1) ಅನುವರ್ತ;
            DfgShiftL* ಧ್ರುವ shiftLp = selp->fromp()->cast<DfgShiftL>();
            ಯದಿ (!shiftLp) ಅನುವರ್ತ;
            ಧ್ರುವ DfgConst* ಧ್ರುವ constp = shiftLp->lhsp()->cast<DfgConst>();
            ಯದಿ (!constp || !useOk(selp, ಅಸತ್ಯ)) ಅನುವರ್ತ;
            ಯದಿ (!constp->has_ಮೌಲ್ಯ(1)) ಅನುವರ್ತ;
            srcp = shiftLp->rhsp();
            ಯದಿ (!widthOk(srcp)) ಅನುವರ್ತ;
            val = selp->lsb();
            inv = ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            // Not a comparison-like vertex
            ಅನುವರ್ತ;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        ಯದಿ (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    ಯದಿ (nTerms < TERM_LIMIT) ಪ್ರತ್ಯಾಗಮ;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    ಪ್ರತಿ (DfgVertex* ಧ್ರುವ srcp : srcps) {
        ಧ್ರುವ Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        ಯದಿ (val2Terms.size() < TERM_LIMIT) ಅನುವರ್ತ;

        // Width of the decoded binary value
        ಧ್ರುವ uint32_t width = srcp->width();
        // Number of bits in the input operand
        ಧ್ರುವ uint32_t nBits = 1U << width;

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

        File_ಸಾಲು* ಧ್ರುವ flp = srcp->fileline();
        AstScope* ಧ್ರುವ scopep = srcp->scopep(scopeCache, ಸತ್ಯ);

        AstActive* ಧ್ರುವ staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            ಪ್ರತಿ (Ast_ಗಂಟು* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ಧ್ರುವ activep = VN_CAST(nodep, Active);
                ಯದಿ (!activep) ಅನುವರ್ತ;
                ಯದಿ (activep->sentreep()->hasStatic()) ಪ್ರತ್ಯಾಗಮ activep;
            }
            // If there isn't one, create a new one
            File_ಸಾಲು* ಧ್ರುವ flp = scopep->fileline();
            Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ stp = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "", stp};
            activep->sen_ವೃಕ್ಷ_Storep(stp);
            scopep->addBlocksp(activep);
            ಪ್ರತ್ಯಾಗಮ activep;
        }();
        AstActive* ಧ್ರುವ combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            ಪ್ರತಿ (Ast_ಗಂಟು* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* ಧ್ರುವ activep = VN_CAST(nodep, Active);
                ಯದಿ (!activep) ಅನುವರ್ತ;
                ಯದಿ (activep->sentreep()->hasCombo()) ಪ್ರತ್ಯಾಗಮ activep;
            }
            // If there isn't one, create a new one
            File_ಸಾಲು* ಧ್ರುವ flp = scopep->fileline();
            Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ stp = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "", stp};
            activep->sen_ವೃಕ್ಷ_Storep(stp);
            scopep->addBlocksp(activep);
            ಪ್ರತ್ಯಾಗಮ activep;
        }();

        // Required data types
        ಧ್ರುವ DfgDataType& idxDType = srcp->dtype();
        ಧ್ರುವ DfgDataType& bitDType = DfgDataType::packed(1);
        ಧ್ರುವ DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            ಯದಿ (DfgVertexVar* ಧ್ರುವ vp = srcp->getResultVar()) ಪ್ರತ್ಯಾಗಮ vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* ಧ್ರುವ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(ಸತ್ಯ);
            vtxp->srcp(srcp);
            ಪ್ರತ್ಯಾಗಮ vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ಧ್ರುವ idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ಧ್ರುವ preVscp = [&]() {
            DfgVertexVar* ಧ್ರುವ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ಧ್ರುವ vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(ಸತ್ಯ);
            vscp->varp()->noReset(ಸತ್ಯ);
            vscp->varp()->setIgnoreSchedWrite();
            ಪ್ರತ್ಯಾಗಮ vscp;
        }();
        // The table variable
        DfgVarArray* ಧ್ರುವ tabVtxp = [&]() {
            DfgVertexVar* ಧ್ರುವ varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(ಸತ್ಯ);
            varp->vscp()->varp()->noReset(ಸತ್ಯ);
            varp->setHasModWrRefs();
            ಪ್ರತ್ಯಾಗಮ varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ಧ್ರುವ initp = ನೂತನ AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(ನೂತನ AstAssign{
                flp,  //
                ನೂತನ AstVarRef{flp, preVscp, VAccess::WRITE},  //
                ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, static_cast<ಪೂರ್ಣಾಙ್ಕ>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
                flp, ನೂತನ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(ನೂತನ AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ಧ್ರುವ logicp = ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(ನೂತನ AstAssign{
                flp,  //
                ನೂತನ AstArraySel{flp, ನೂತನ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                ನೂತನ AstVarRef{flp, preVscp, VAccess::READ}},  //
                ನೂತನ AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ಧ್ರುವ idxRefp = ನೂತನ AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(ನೂತನ AstAssign{
                flp,  //
                ನೂತನ AstArraySel{flp, ನೂತನ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                ನೂತನ AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* ಧ್ರುವ astRdp = ನೂತನ DfgAstRd{dfg, idxRefp, ಅಸತ್ಯ, ಅಸತ್ಯ};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ಧ್ರುವ idxRefp = ನೂತನ AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(ನೂತನ AstAssign{flp,  //
                                            ನೂತನ AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ಧ್ರುವ astRdp = ನೂತನ DfgAstRd{dfg, idxRefp, ಅಸತ್ಯ, ಅಸತ್ಯ};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : val2Terms) {
            ಧ್ರುವ uint32_t val = pair.first;
            ಧ್ರುವ std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            File_ಸಾಲು* ಧ್ರುವ aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ಧ್ರುವ aselp = ನೂತನ DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(ನೂತನ DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            ಪ್ರತಿ (ಧ್ರುವ Term& term : terms) {
                ಯದಿ (term.m_inv) {
                    ಯದಿ (!notp) {
                        notp = ನೂತನ DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } ಅನ್ಯಥಾ {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
