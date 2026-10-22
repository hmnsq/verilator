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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3DfgPasses.h"

#include "V3Ast.h"
#include "V3Dfg.h"
#include "V3File.h"
#include "V3Global.h"
#include "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

void V3DfgPasses::removeUnobservable(DfgGraph& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    សម្រាប់ (DfgVertex& vtx : dfg.opVertices()) {
        ប្រសិនបើ (vtx.is<DfgLogic>()) workList.push_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* const logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        const bool used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* const uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* const vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            ប្រសិនបើ (vVtxp->hasSinks()) ត្រឡប់ true;
            ប្រសិនបើ (vVtxp->isObserved()) ត្រឡប់ true;
            ត្រឡប់ false;
        });
        // If some are used, the logic must stay in the Ast
        ប្រសិនបើ (used) ត្រឡប់;
        // If impure, it must stay in the Ast
        ប្រសិនបើ (!logicp->isPure()) ត្រឡប់;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* const varp = src.as<DfgVertexVar>();
            ប្រសិនបើ (!varp->srcp()) ត្រឡប់ false;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.push_front(*driver.as<DfgLogic>());
                ត្រឡប់ false;
            });
            ត្រឡប់ false;
        });
        // Delete this logic both from the Dfg and the Ast
        AstNode* const nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    សម្រាប់ (DfgVertexVar* const vVtxp : dfg.varVertices().unlinkable()) {
        ប្រសិនបើ (vVtxp->hasSinks()) continue;
        ប្រសិនបើ (vVtxp->isObserved()) continue;
        DfgVertex* const srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* const vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        const bool delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        ប្រសិនបើ (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                ត្រឡប់ false;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        ប្រសិនបើ (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->deleteTree(), vscp);
            ++ctx.m_varsDeleted;
        } បើមិនដូច្នេះ {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    សម្រាប់ (DfgVertex* const vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* const logicp = vtxp->cast<DfgLogic>();
        ប្រសិនបើ (!logicp) continue;
        ប្រសិនបើ (logicp->hasSinks()) continue;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        const VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            ត្រឡប់ false;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](AstNode* varp) {  //
            ត្រឡប់ varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

void V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    សម្រាប់ (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* const selp = vtx.cast<DfgSel>();
        ប្រសិនបើ (!selp) continue;
        selps.push_back(selp);
    }

    សម្រាប់ (DfgSel* const selp : selps) {
        FileLine* const flp = selp->fileline();
        const DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        ប្រសិនបើ (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            continue;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* const varp = selp->fromp()->cast<DfgVarPacked>();
        ប្រសិនបើ (!varp || !varp->tmpForp()) continue;

        // Find the driver of this range
        const auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* const driverp = pair.first;
        const uint32_t driverLsb = pair.second;
        ប្រសិនបើ (!driverp) continue;

        // If partial driver is the whole thing we are looking for, just replace with that
        ប្រសិនបើ (driverp->dtype() == dtype) {
            ++ctx.m_replacedWithWholeDriver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            continue;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* const newSelp = new DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

void V3DfgPasses::inlineVars(DfgGraph& dfg) {
    សម្រាប់ (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        ប្រសិនបើ (!vtx.hasSinks()) continue;
        // Nohting to inline
        DfgVertex* const srcp = vtx.srcp();
        ប្រសិនបើ (!srcp) continue;
        // Value can differ from driver
        ប្រសិនបើ (vtx.isVolatile()) continue;
        // Partial driver cannot be inlined
        ប្រសិនបើ (srcp->is<DfgVertexSplice>()) continue;
        ប្រសិនបើ (srcp->is<DfgUnitArray>()) continue;
        // Don't inline CReset
        ប្រសិនបើ (srcp->is<DfgCReset>()) continue;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

void V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    សម្រាប់ (DfgVertex& vtx : dfg.opVertices()) {
        ប្រសិនបើ (vtx.hasSinks()) continue;
        // This vertex is unused. Add to work list.
        workList.push_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    សម្រាប់ (DfgVertexVar& vtx : dfg.varVertices()) {
        ប្រសិនបើ (!vtx.tmpForp()) continue;
        ប្រសិនបើ (vtx.hasSinks()) continue;
        ប្រសិនបើ (vtx.hasDfgRefs()) continue;
        // This vertex is unused. Add to work list.
        workList.push_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        ប្រសិនបើ (vtx.hasSinks()) ត្រឡប់;
        // If temporary used in another graph, we need to keep it
        ប្រសិនបើ (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            ប្រសិនបើ (varp->hasDfgRefs()) ត្រឡប់;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            ប្រសិនបើ (src.is<DfgConst>()) ត្រឡប់ false;
            const DfgVertexVar* const varp = src.cast<DfgVertexVar>();
            ប្រសិនបើ (varp && !varp->tmpForp()) ត្រឡប់ false;
            // Add source to workList
            workList.push_front(src);
            ត្រឡប់ false;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    សម្រាប់ (DfgVertexVar* const vtxp : dfg.varVertices().unlinkable()) {
        ប្រសិនបើ (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    សម្រាប់ (DfgConst* const vtxp : dfg.constVertices().unlinkable()) {
        ប្រសិនបើ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

void V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    sa_nracanaa Term final {
        DfgVertex* m_vtxp = nullptr;  // Vertex to replace
        bool m_inv = false;  // '!=', instead of '=='
        Term() = default;
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
    const auto widthOk = [](const DfgVertex* vtxp) {
        const uint32_t width = vtxp->width();
        ត្រឡប់ WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    const std::function<bool(const DfgVertex*, bool)> useOk
        = [&](const DfgVertex* vtxp, bool inv) -> bool {
        // Go past a single 'Not' sink, which is common
        ប្រសិនបើ (DfgVertex* const sinkp = vtxp->singleSink()) {
            ប្រសិនបើ (sinkp->is<DfgNot>()) ត្រឡប់ useOk(sinkp, !inv);
        }
        const bool condTree = vtxp->foreachSink([&](const DfgVertex& sink) {
            const DfgCond* const condp = sink.cast<DfgCond>();
            ប្រសិនបើ (!condp) ត្រឡប់ false;
            ប្រសិនបើ (condp->condp() != vtxp) ត្រឡប់ false;
            ត្រឡប់ inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        ត្រឡប់ !condTree;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    សម្រាប់ (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        bool inv = false;
        ប្រសិនបើ (DfgEq* const eqp = vtx.cast<DfgEq>()) {
            DfgConst* const constp = eqp->lhsp()->cast<DfgConst>();
            ប្រសិនបើ (!constp || !widthOk(constp) || !useOk(eqp, false)) continue;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = false;
        } បើមិនដូច្នេះ ប្រសិនបើ (DfgNeq* const neqp = vtx.cast<DfgNeq>()) {
            DfgConst* const constp = neqp->lhsp()->cast<DfgConst>();
            ប្រសិនបើ (!constp || !widthOk(constp) || !useOk(neqp, true)) continue;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = true;
        } បើមិនដូច្នេះ ប្រសិនបើ (DfgRedAnd* const redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            ប្រសិនបើ (!widthOk(srcp) || !useOk(redAndp, false)) continue;
            val = (1U << srcp->width()) - 1;
            inv = false;
        } បើមិនដូច្នេះ ប្រសិនបើ (DfgRedOr* const redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            ប្រសិនបើ (!widthOk(srcp) || !useOk(redOrp, true)) continue;
            val = 0;
            inv = true;
        } បើមិនដូច្នេះ ប្រសិនបើ (DfgSel* const selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            ប្រសិនបើ (selp->width() != 1) continue;
            DfgShiftL* const shiftLp = selp->fromp()->cast<DfgShiftL>();
            ប្រសិនបើ (!shiftLp) continue;
            const DfgConst* const constp = shiftLp->lhsp()->cast<DfgConst>();
            ប្រសិនបើ (!constp || !useOk(selp, false)) continue;
            ប្រសិនបើ (!constp->hasValue(1)) continue;
            srcp = shiftLp->rhsp();
            ប្រសិនបើ (!widthOk(srcp)) continue;
            val = selp->lsb();
            inv = false;
        } បើមិនដូច្នេះ {
            // Not a comparison-like vertex
            continue;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        ប្រសិនបើ (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    ប្រសិនបើ (nTerms < TERM_LIMIT) ត្រឡប់;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    សម្រាប់ (DfgVertex* const srcp : srcps) {
        const Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        ប្រសិនបើ (val2Terms.size() < TERM_LIMIT) continue;

        // Width of the decoded binary value
        const uint32_t width = srcp->width();
        // Number of bits in the input operand
        const uint32_t nBits = 1U << width;

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

        FileLine* const flp = srcp->fileline();
        AstScope* const scopep = srcp->scopep(scopeCache, true);

        AstActive* const staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            សម្រាប់ (AstNode* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* const activep = VN_CAST(nodep, Active);
                ប្រសិនបើ (!activep) continue;
                ប្រសិនបើ (activep->sentreep()->hasStatic()) ត្រឡប់ activep;
            }
            // If there isn't one, create a new one
            FileLine* const flp = scopep->fileline();
            AstSenTree* const stp = new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* const activep = new AstActive{flp, "", stp};
            activep->senTreeStorep(stp);
            scopep->addBlocksp(activep);
            ត្រឡប់ activep;
        }();
        AstActive* const combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            សម្រាប់ (AstNode* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* const activep = VN_CAST(nodep, Active);
                ប្រសិនបើ (!activep) continue;
                ប្រសិនបើ (activep->sentreep()->hasCombo()) ត្រឡប់ activep;
            }
            // If there isn't one, create a new one
            FileLine* const flp = scopep->fileline();
            AstSenTree* const stp = new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* const activep = new AstActive{flp, "", stp};
            activep->senTreeStorep(stp);
            scopep->addBlocksp(activep);
            ត្រឡប់ activep;
        }();

        // Required data types
        const DfgDataType& idxDType = srcp->dtype();
        const DfgDataType& bitDType = DfgDataType::packed(1);
        const DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            ប្រសិនបើ (DfgVertexVar* const vp = srcp->getResultVar()) ត្រឡប់ vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* const vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(true);
            vtxp->srcp(srcp);
            ត្រឡប់ vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* const idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* const preVscp = [&]() {
            DfgVertexVar* const vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* const vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->isInternal(true);
            vscp->varp()->noReset(true);
            vscp->varp()->setIgnoreSchedWrite();
            ត្រឡប់ vscp;
        }();
        // The table variable
        DfgVarArray* const tabVtxp = [&]() {
            DfgVertexVar* const varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(true);
            varp->vscp()->varp()->noReset(true);
            varp->setHasModWrRefs();
            ត្រឡប់ varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* const initp = new AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(new AstAssign{
                flp,  //
                new AstVarRef{flp, preVscp, VAccess::WRITE},  //
                new AstConst{flp, AstConst::WidthedValue{}, static_cast<ចំនួនគត់>(width), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* const callp = new AstCMethodHard{
                flp, new AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->addPinsp(new AstConst{flp, AstConst::BitFalse{}});
            callp->dtypeSetVoid();
            initp->addStmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* const logicp = new AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, nullptr};
        combActivep->addStmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->addStmtsp(new AstAssign{
                flp,  //
                new AstArraySel{flp, new AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                new AstVarRef{flp, preVscp, VAccess::READ}},  //
                new AstConst{flp, AstConst::BitFalse{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* const idxRefp = new AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(new AstAssign{
                flp,  //
                new AstArraySel{flp, new AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                new AstConst{flp, AstConst::BitTrue{}}});
            DfgAstRd* const astRdp = new DfgAstRd{dfg, idxRefp, false, false};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* const idxRefp = new AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->addStmtsp(new AstAssign{flp,  //
                                            new AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* const astRdp = new DfgAstRd{dfg, idxRefp, false, false};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        សម្រាប់ (const auto& pair : val2Terms) {
            const uint32_t val = pair.first;
            const std::vector<Term>& terms = pair.second;
            // Create the ArraySel
            FileLine* const aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* const aselp = new DfgArraySel{dfg, aflp, bitDType};
            aselp->fromp(tabVtxp);
            aselp->bitp(new DfgConst{dfg, aflp, width, val});
            // The inverted value, if needed
            DfgNot* notp = nullptr;
            // Repalce the terms
            សម្រាប់ (const Term& term : terms) {
                ប្រសិនបើ (term.m_inv) {
                    ប្រសិនបើ (!notp) {
                        notp = new DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } បើមិនដូច្នេះ {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
