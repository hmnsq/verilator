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
    위해 (DfgVertex& vtx : dfg.opVertices()) {
        만약 (vtx.is<DfgLogic>()) workList.push_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](DfgVertex& vtx) {
        DfgLogic* const logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        const bool used = logicp->foreachSink([&](DfgVertex& snk) {
            DfgUnresolved* const uVtxp = snk.as<DfgUnresolved>();
            DfgVertexVar* const vVtxp = uVtxp->firtsSinkp()->as<DfgVertexVar>();
            만약 (vVtxp->hasSinks()) 반환 true;
            만약 (vVtxp->isObserved()) 반환 true;
            반환 false;
        });
        // If some are used, the logic must stay in the Ast
        만약 (used) 반환;
        // If impure, it must stay in the Ast
        만약 (!logicp->isPure()) 반환;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreachSource([&](DfgVertex& src) {
            DfgVertexVar* const varp = src.as<DfgVertexVar>();
            만약 (!varp->srcp()) 반환 false;
            varp->srcp()->as<DfgUnresolved>()->foreachSource([&](DfgVertex& driver) {
                workList.push_front(*driver.as<DfgLogic>());
                반환 false;
            });
            반환 false;
        });
        // Delete this logic both from the Dfg and the Ast
        AstNode* const nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    위해 (DfgVertexVar* const vVtxp : dfg.varVertices().unlinkable()) {
        만약 (vVtxp->hasSinks()) continue;
        만약 (vVtxp->isObserved()) continue;
        DfgVertex* const srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* const vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        const bool delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        만약 (srcp) {
            srcp->foreachSource([&](DfgVertex& src) {
                src.as<DfgLogic>()->setDrivesUnusedVars();
                반환 false;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        만약 (delAst) {
            VL_DO_DANGLING(vscp->unlinkFrBack()->deleteTree(), vscp);
            ++ctx.m_varsDeleted;
        } 아니면 {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    위해 (DfgVertex* const vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* const logicp = vtxp->cast<DfgLogic>();
        만약 (!logicp) continue;
        만약 (logicp->hasSinks()) continue;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        const VNUser2InUse m_user2InUse;
        logicp->foreachSource([](DfgVertex& src) {
            src.as<DfgVertexVar>()->vscp()->user2p(&src);
            반환 false;
        });
        V3DfgPasses::addAstRefs(dfg, logicp->nodep(), [](AstNode* varp) {  //
            반환 varp->user2u().to<DfgVertexVar*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

void V3DfgPasses::removeSelects(DfgGraph& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::vector<DfgSel*> selps;
    위해 (DfgVertex& vtx : dfg.opVertices()) {
        DfgSel* const selp = vtx.cast<DfgSel>();
        만약 (!selp) continue;
        selps.push_back(selp);
    }

    위해 (DfgSel* const selp : selps) {
        FileLine* const flp = selp->fileline();
        const DfgDataType& dtype = selp->dtype();

        // Remove full width selects
        만약 (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removedFullWidth;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            continue;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* const varp = selp->fromp()->cast<DfgVarPacked>();
        만약 (!varp || !varp->tmpForp()) continue;

        // Find the driver of this range
        const auto pair = varp->driverOfRange(selp->lsb(), selp->width());
        DfgVertex* const driverp = pair.first;
        const uint32_t driverLsb = pair.second;
        만약 (!driverp) continue;

        // If partial driver is the whole thing we are looking for, just replace with that
        만약 (driverp->dtype() == dtype) {
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
    위해 (DfgVertexVar& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        만약 (!vtx.hasSinks()) continue;
        // Nohting to inline
        DfgVertex* const srcp = vtx.srcp();
        만약 (!srcp) continue;
        // Value can differ from driver
        만약 (vtx.isVolatile()) continue;
        // Partial driver cannot be inlined
        만약 (srcp->is<DfgVertexSplice>()) continue;
        만약 (srcp->is<DfgUnitArray>()) continue;
        // Don't inline CReset
        만약 (srcp->is<DfgCReset>()) continue;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

void V3DfgPasses::removeUnused(DfgGraph& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    위해 (DfgVertex& vtx : dfg.opVertices()) {
        만약 (vtx.hasSinks()) continue;
        // This vertex is unused. Add to work list.
        workList.push_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    위해 (DfgVertexVar& vtx : dfg.varVertices()) {
        만약 (!vtx.tmpForp()) continue;
        만약 (vtx.hasSinks()) continue;
        만약 (vtx.hasDfgRefs()) continue;
        // This vertex is unused. Add to work list.
        workList.push_front(vtx);
    }

    // Process the work list
    workList.foreach([&](DfgVertex& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        만약 (vtx.hasSinks()) 반환;
        // If temporary used in another graph, we need to keep it
        만약 (const DfgVertexVar* const varp = vtx.cast<DfgVertexVar>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            만약 (varp->hasDfgRefs()) 반환;
        }
        // Add sources of unused vertex to work list
        vtx.foreachSource([&](DfgVertex& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            만약 (src.is<DfgConst>()) 반환 false;
            const DfgVertexVar* const varp = src.cast<DfgVertexVar>();
            만약 (varp && !varp->tmpForp()) 반환 false;
            // Add source to workList
            workList.push_front(src);
            반환 false;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    위해 (DfgVertexVar* const vtxp : dfg.varVertices().unlinkable()) {
        만약 (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    위해 (DfgConst* const vtxp : dfg.constVertices().unlinkable()) {
        만약 (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

void V3DfgPasses::binToOneHot(DfgGraph& dfg, V3DfgBinToOneHotContext& ctx) {
    // Structure to keep track of comparison details
    struct Term final {
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
        반환 WIDTH_MIN <= width && width <= WIDTH_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    const std::function<bool(const DfgVertex*, bool)> useOk
        = [&](const DfgVertex* vtxp, bool inv) -> bool {
        // Go past a single 'Not' sink, which is common
        만약 (DfgVertex* const sinkp = vtxp->singleSink()) {
            만약 (sinkp->is<DfgNot>()) 반환 useOk(sinkp, !inv);
        }
        const bool condTree = vtxp->foreachSink([&](const DfgVertex& sink) {
            const DfgCond* const condp = sink.cast<DfgCond>();
            만약 (!condp) 반환 false;
            만약 (condp->condp() != vtxp) 반환 false;
            반환 inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        반환 !condTree;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    위해 (DfgVertex& vtx : dfg.opVertices()) {
        DfgVertex* srcp = nullptr;
        uint32_t val = 0;
        bool inv = false;
        만약 (DfgEq* const eqp = vtx.cast<DfgEq>()) {
            DfgConst* const constp = eqp->lhsp()->cast<DfgConst>();
            만약 (!constp || !widthOk(constp) || !useOk(eqp, false)) continue;
            srcp = eqp->rhsp();
            val = constp->toU32();
            inv = false;
        } 아니면 만약 (DfgNeq* const neqp = vtx.cast<DfgNeq>()) {
            DfgConst* const constp = neqp->lhsp()->cast<DfgConst>();
            만약 (!constp || !widthOk(constp) || !useOk(neqp, true)) continue;
            srcp = neqp->rhsp();
            val = constp->toU32();
            inv = true;
        } 아니면 만약 (DfgRedAnd* const redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            만약 (!widthOk(srcp) || !useOk(redAndp, false)) continue;
            val = (1U << srcp->width()) - 1;
            inv = false;
        } 아니면 만약 (DfgRedOr* const redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            만약 (!widthOk(srcp) || !useOk(redOrp, true)) continue;
            val = 0;
            inv = true;
        } 아니면 만약 (DfgSel* const selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            만약 (selp->width() != 1) continue;
            DfgShiftL* const shiftLp = selp->fromp()->cast<DfgShiftL>();
            만약 (!shiftLp) continue;
            const DfgConst* const constp = shiftLp->lhsp()->cast<DfgConst>();
            만약 (!constp || !useOk(selp, false)) continue;
            만약 (!constp->hasValue(1)) continue;
            srcp = shiftLp->rhsp();
            만약 (!widthOk(srcp)) continue;
            val = selp->lsb();
            inv = false;
        } 아니면 {
            // Not a comparison-like vertex
            continue;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        만약 (val2Terms.empty()) srcps.emplace_back(srcp);
        // Record term
        val2Terms[val].emplace_back(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t TERM_LIMIT = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << WIDTH_MIN) >= TERM_LIMIT, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    만약 (nTerms < TERM_LIMIT) 반환;

    DfgVertex::ScopeCache scopeCache;

    // Create decoders for each srcp
    위해 (DfgVertex* const srcp : srcps) {
        const Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        만약 (val2Terms.size() < TERM_LIMIT) continue;

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
            위해 (AstNode* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* const activep = VN_CAST(nodep, Active);
                만약 (!activep) continue;
                만약 (activep->sentreep()->hasStatic()) 반환 activep;
            }
            // If there isn't one, create a new one
            FileLine* const flp = scopep->fileline();
            AstSenTree* const stp = new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Static{}}};
            AstActive* const activep = new AstActive{flp, "", stp};
            activep->senTreeStorep(stp);
            scopep->addBlocksp(activep);
            반환 activep;
        }();
        AstActive* const combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            위해 (AstNode* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                AstActive* const activep = VN_CAST(nodep, Active);
                만약 (!activep) continue;
                만약 (activep->sentreep()->hasCombo()) 반환 activep;
            }
            // If there isn't one, create a new one
            FileLine* const flp = scopep->fileline();
            AstSenTree* const stp = new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Combo{}}};
            AstActive* const activep = new AstActive{flp, "", stp};
            activep->senTreeStorep(stp);
            scopep->addBlocksp(activep);
            반환 activep;
        }();

        // Required data types
        const DfgDataType& idxDType = srcp->dtype();
        const DfgDataType& bitDType = DfgDataType::packed(1);
        const DfgDataType& tabDType = DfgDataType::array(bitDType, nBits);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            만약 (DfgVertexVar* const vp = srcp->getResultVar()) 반환 vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            DfgVertexVar* const vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->isInternal(true);
            vtxp->srcp(srcp);
            반환 vtxp->as<DfgVarPacked>();
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
            반환 vscp;
        }();
        // The table variable
        DfgVarArray* const tabVtxp = [&]() {
            DfgVertexVar* const varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->isInternal(true);
            varp->vscp()->varp()->noReset(true);
            varp->setHasModWrRefs();
            반환 varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* const initp = new AstInitialStatic{flp, nullptr};
        staticActivep->addStmtsp(initp);
        {  // pre = 0
            initp->addStmtsp(new AstAssign{
                flp,  //
                new AstVarRef{flp, preVscp, VAccess::WRITE},  //
                new AstConst{flp, AstConst::WidthedValue{}, static_cast<정수>(width), 0}});
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
        위해 (const auto& pair : val2Terms) {
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
            위해 (const Term& term : terms) {
                만약 (term.m_inv) {
                    만약 (!notp) {
                        notp = new DfgNot{dfg, aflp, bitDType};
                        notp->srcp(aselp);
                    }
                    term.m_vtxp->replaceWith(notp);
                } 아니면 {
                    term.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(term.m_vtxp->unlinkDelete(dfg), term.m_vtxp);
            }
        }
    }
}
