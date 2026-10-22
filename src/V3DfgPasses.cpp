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

ಶೂನ್ಯ V3DfgPasses::removeUnobservable(Dfg_ನಕ್ಷೆ& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : dfg.opVertices()) {
        ಆದರೆ (vtx.is<DfgLogic>()) workList.ತಳ್ಳು_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](Dfg_ಶೃಂಗ& vtx) {
        DfgLogic* ಸ್ಥಿರಾಂಕ logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಬಳಸಿದ = logicp->foreachSink([&](Dfg_ಶೃಂಗ& snk) {
            DfgUnresolved* ಸ್ಥಿರಾಂಕ uVtxp = snk.as<DfgUnresolved>();
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vVtxp = uVtxp->firtsSinkp()->as<Dfg_ಶೃಂಗ_Var>();
            ಆದರೆ (vVtxp->hasSinks()) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಆದರೆ (vVtxp->isObserved()) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        // If some are used, the logic must stay in the Ast
        ಆದರೆ (ಬಳಸಿದ) ಹಿಂತಿರುಗಿ;
        // If impure, it must stay in the Ast
        ಆದರೆ (!logicp->is_ಶುದ್ಧ()) ಹಿಂತಿರುಗಿ;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = src.as<Dfg_ಶೃಂಗ_Var>();
            ಆದರೆ (!varp->srcp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            varp->srcp()->as<DfgUnresolved>()->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& driver) {
                workList.ತಳ್ಳು_front(*driver.as<DfgLogic>());
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    ಗಾಗಿ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vVtxp : dfg.varVertices().unlinkable()) {
        ಆದರೆ (vVtxp->hasSinks()) ಮುಂದುವರಿ;
        ಆದರೆ (vVtxp->isObserved()) ಮುಂದುವರಿ;
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        ಆದರೆ (srcp) {
            srcp->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
                src.as<DfgLogic>()->ಗಣ_Drives_ಬಳಸದ_Vars();
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        ಆದರೆ (delAst) {
            VL_DO_DANGLING(vscp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), vscp);
            ++ctx.m_varsDeleted;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ಸ್ಥಿರಾಂಕ logicp = vtxp->cast<DfgLogic>();
        ಆದರೆ (!logicp) ಮುಂದುವರಿ;
        ಆದರೆ (logicp->hasSinks()) ಮುಂದುವರಿ;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;
        logicp->foreach_ಮೂಲ([](Dfg_ಶೃಂಗ& src) {
            src.as<Dfg_ಶೃಂಗ_Var>()->vscp()->user2p(&src);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        V3DfgPasses::ಕೂಡಿಸು_Ast_Refs(dfg, logicp->nodep(), [](Ast_ಗಂಟು* varp) {  //
            ಹಿಂತಿರುಗಿ varp->user2u().ಗಮ್ಯ<Dfg_ಶೃಂಗ_Var*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

ಶೂನ್ಯ V3DfgPasses::removeSelects(Dfg_ನಕ್ಷೆ& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::ಸದಿಶ<DfgSel*> selps;
    ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : dfg.opVertices()) {
        DfgSel* ಸ್ಥಿರಾಂಕ selp = vtx.cast<DfgSel>();
        ಆದರೆ (!selp) ಮುಂದುವರಿ;
        selps.ತಳ್ಳು_ಹಿಂಭಾಗ(selp);
    }

    ಗಾಗಿ (DfgSel* ಸ್ಥಿರಾಂಕ selp : selps) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = selp->fileline();
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = selp->dtype();

        // Remove full width selects
        ಆದರೆ (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removed_ತುಂಬಿದ_ಅಗಲ;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ಮುಂದುವರಿ;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ಸ್ಥಿರಾಂಕ varp = selp->fromp()->cast<DfgVarPacked>();
        ಆದರೆ (!varp || !varp->tmpForp()) ಮುಂದುವರಿ;

        // Find the driver of this range
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = varp->driver_ದ_Range(selp->lsb(), selp->ಅಗಲ());
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ driverp = pair.ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ uint32_t driverLsb = pair.ಎರಡನೇ;
        ಆದರೆ (!driverp) ಮುಂದುವರಿ;

        // If partial driver is the whole thing we are looking for, just replace with that
        ಆದರೆ (driverp->dtype() == dtype) {
            ++ctx.m_replaced_With_ಸಂಪೂರ್ಣ_Driver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            ಮುಂದುವರಿ;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ಸ್ಥಿರಾಂಕ newSelp = ಹೊಸ DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

ಶೂನ್ಯ V3DfgPasses::inlineVars(Dfg_ನಕ್ಷೆ& dfg) {
    ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        ಆದರೆ (!vtx.hasSinks()) ಮುಂದುವರಿ;
        // Nohting to inline
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtx.srcp();
        ಆದರೆ (!srcp) ಮುಂದುವರಿ;
        // Value can differ from driver
        ಆದರೆ (vtx.isVolatile()) ಮುಂದುವರಿ;
        // Partial driver cannot be inlined
        ಆದರೆ (srcp->is<Dfg_ಶೃಂಗ_Splice>()) ಮುಂದುವರಿ;
        ಆದರೆ (srcp->is<DfgUnitArray>()) ಮುಂದುವರಿ;
        // Don't inline CReset
        ಆದರೆ (srcp->is<DfgCReset>()) ಮುಂದುವರಿ;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

ಶೂನ್ಯ V3DfgPasses::remove_ಬಳಸದ(Dfg_ನಕ್ಷೆ& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : dfg.opVertices()) {
        ಆದರೆ (vtx.hasSinks()) ಮುಂದುವರಿ;
        // This vertex is unused. Add to work list.
        workList.ತಳ್ಳು_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : dfg.varVertices()) {
        ಆದರೆ (!vtx.tmpForp()) ಮುಂದುವರಿ;
        ಆದರೆ (vtx.hasSinks()) ಮುಂದುವರಿ;
        ಆದರೆ (vtx.hasDfgRefs()) ಮುಂದುವರಿ;
        // This vertex is unused. Add to work list.
        workList.ತಳ್ಳು_front(vtx);
    }

    // Process the work list
    workList.foreach([&](Dfg_ಶೃಂಗ& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        ಆದರೆ (vtx.hasSinks()) ಹಿಂತಿರುಗಿ;
        // If temporary used in another graph, we need to keep it
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = vtx.cast<Dfg_ಶೃಂಗ_Var>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            ಆದರೆ (varp->hasDfgRefs()) ಹಿಂತಿರುಗಿ;
        }
        // Add sources of unused vertex to work list
        vtx.foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            ಆದರೆ (src.is<DfgConst>()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = src.cast<Dfg_ಶೃಂಗ_Var>();
            ಆದರೆ (varp && !varp->tmpForp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Add source to workList
            workList.ತಳ್ಳು_front(src);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    ಗಾಗಿ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp : dfg.varVertices().unlinkable()) {
        ಆದರೆ (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    ಗಾಗಿ (DfgConst* ಸ್ಥಿರಾಂಕ vtxp : dfg.constVertices().unlinkable()) {
        ಆದರೆ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

ಶೂನ್ಯ V3DfgPasses::bin_ಗಮ್ಯ_One_Hot(Dfg_ನಕ್ಷೆ& dfg, V3_Dfg_Bin_ಗಮ್ಯ_One_Hot_Context& ctx) {
    // Structure to keep track of comparison details
    ರಚನೆ ಪದ_2 final {
        Dfg_ಶೃಂಗ* m_vtxp = ಶೂನ್ಯಸೂಚಕ;  // Vertex to replace
        ತಾರ್ಕಿಕ m_inv = ಸುಳ್ಳು;  // '!=', instead of '=='
        ಪದ_2() = ಪೂರ್ವನಿಯೋಜಿತ;
        ಪದ_2(Dfg_ಶೃಂಗ* vtxp, ತಾರ್ಕಿಕ inv)
            : m_vtxp{vtxp}
            , m_inv{inv} {}
    };

    // List of vertices that are used as sources
    std::ಸದಿಶ<Dfg_ಶೃಂಗ*> srcps;

    // Map from 'vertices' -> 'value beign compared' -> 'terms'
    ಬಳಸಿ Val2Terms = std::map<uint32_t, std::ಸದಿಶ<ಪದ_2>>;
    DfgUserMap<Val2Terms> vtx2Val2Terms = dfg.makeUserMap<Val2Terms>();

    // Only consider input variables from a reasonable range:
    // - not too big to avoid huge tables, you are doomed anyway at that point..
    // - not too small, as it's probably not worth it
    constexpr uint32_t ಅಗಲ_MIN = 7;
    constexpr uint32_t ಅಗಲ_MAX = 20;
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಅಗಲ_Ok = [](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* vtxp) {
        ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = vtxp->ಅಗಲ();
        ಹಿಂತಿರುಗಿ ಅಗಲ_MIN <= ಅಗಲ && ಅಗಲ <= ಅಗಲ_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ಸ್ಥಿರಾಂಕ std::function<ತಾರ್ಕಿಕ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*, ತಾರ್ಕಿಕ)> useOk
        = [&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* vtxp, ತಾರ್ಕಿಕ inv) -> ತಾರ್ಕಿಕ {
        // Go past a single 'Not' sink, which is common
        ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sinkp = vtxp->singleSink()) {
            ಆದರೆ (sinkp->is<DfgNot>()) ಹಿಂತಿರುಗಿ useOk(sinkp, !inv);
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ cond_ವೃಕ್ಷ = vtxp->foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& sink) {
            ಸ್ಥಿರಾಂಕ DfgCond* ಸ್ಥಿರಾಂಕ condp = sink.cast<DfgCond>();
            ಆದರೆ (!condp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (condp->condp() != vtxp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಹಿಂತಿರುಗಿ inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        ಹಿಂತಿರುಗಿ !cond_ವೃಕ್ಷ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : dfg.opVertices()) {
        Dfg_ಶೃಂಗ* srcp = ಶೂನ್ಯಸೂಚಕ;
        uint32_t val = 0;
        ತಾರ್ಕಿಕ inv = ಸುಳ್ಳು;
        ಆದರೆ (DfgEq* ಸ್ಥಿರಾಂಕ eqp = vtx.cast<DfgEq>()) {
            DfgConst* ಸ್ಥಿರಾಂಕ constp = eqp->lhsp()->cast<DfgConst>();
            ಆದರೆ (!constp || !ಅಗಲ_Ok(constp) || !useOk(eqp, ಸುಳ್ಳು)) ಮುಂದುವರಿ;
            srcp = eqp->rhsp();
            val = constp->ಗಮ್ಯ_U32();
            inv = ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (DfgNeq* ಸ್ಥಿರಾಂಕ neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ಸ್ಥಿರಾಂಕ constp = neqp->lhsp()->cast<DfgConst>();
            ಆದರೆ (!constp || !ಅಗಲ_Ok(constp) || !useOk(neqp, ನಿಜ)) ಮುಂದುವರಿ;
            srcp = neqp->rhsp();
            val = constp->ಗಮ್ಯ_U32();
            inv = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (DfgRedAnd* ಸ್ಥಿರಾಂಕ redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            ಆದರೆ (!ಅಗಲ_Ok(srcp) || !useOk(redAndp, ಸುಳ್ಳು)) ಮುಂದುವರಿ;
            val = (1U << srcp->ಅಗಲ()) - 1;
            inv = ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (DfgRedOr* ಸ್ಥಿರಾಂಕ redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            ಆದರೆ (!ಅಗಲ_Ok(srcp) || !useOk(redOrp, ನಿಜ)) ಮುಂದುವರಿ;
            val = 0;
            inv = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            ಆದರೆ (selp->ಅಗಲ() != 1) ಮುಂದುವರಿ;
            Dfg_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ ಸರಿಸು_Lp = selp->fromp()->cast<Dfg_ಸರಿಸು_L>();
            ಆದರೆ (!ಸರಿಸು_Lp) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ constp = ಸರಿಸು_Lp->lhsp()->cast<DfgConst>();
            ಆದರೆ (!constp || !useOk(selp, ಸುಳ್ಳು)) ಮುಂದುವರಿ;
            ಆದರೆ (!constp->has_ಮೌಲ್ಯ(1)) ಮುಂದುವರಿ;
            srcp = ಸರಿಸು_Lp->rhsp();
            ಆದರೆ (!ಅಗಲ_Ok(srcp)) ಮುಂದುವರಿ;
            val = selp->lsb();
            inv = ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Not a comparison-like vertex
            ಮುಂದುವರಿ;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        ಆದರೆ (val2Terms.ಖಾಲಿಗಳು()) srcps.emplace_ಹಿಂಭಾಗ(srcp);
        // Record term
        val2Terms[val].emplace_ಹಿಂಭಾಗ(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t ಪದ_ಎಲ್ಲೆ = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << ಅಗಲ_MIN) >= ಪದ_ಎಲ್ಲೆ, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    ಆದರೆ (nTerms < ಪದ_ಎಲ್ಲೆ) ಹಿಂತಿರುಗಿ;

    Dfg_ಶೃಂಗ::ScopeCache scopeCache;

    // Create decoders for each srcp
    ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp : srcps) {
        ಸ್ಥಿರಾಂಕ Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        ಆದರೆ (val2Terms.ಗಾತ್ರ_2() < ಪದ_ಎಲ್ಲೆ) ಮುಂದುವರಿ;

        // Width of the decoded binary value
        ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = srcp->ಅಗಲ();
        // Number of bits in the input operand
        ಸ್ಥಿರಾಂಕ uint32_t n_ಬಿಟ್ಟುಗಳು = 1U << ಅಗಲ;

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

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = srcp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = srcp->scopep(scopeCache, ನಿಜ);

        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            ಗಾಗಿ (Ast_ಗಂಟು* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = VN_CAST(nodep, ಸಕ್ರಿಯ);
                ಆದರೆ (!activep) ಮುಂದುವರಿ;
                ಆದರೆ (activep->sentreep()->hasStatic()) ಹಿಂತಿರುಗಿ activep;
            }
            // If there isn't one, create a new one
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = scopep->fileline();
            Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ stp = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, Ast_Sen_ಅಂಶ::Static{}}};
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "", stp};
            activep->sen_ವೃಕ್ಷ_Storep(stp);
            scopep->ಕೂಡಿಸು_Blocksp(activep);
            ಹಿಂತಿರುಗಿ activep;
        }();
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            ಗಾಗಿ (Ast_ಗಂಟು* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = VN_CAST(nodep, ಸಕ್ರಿಯ);
                ಆದರೆ (!activep) ಮುಂದುವರಿ;
                ಆದರೆ (activep->sentreep()->hasCombo()) ಹಿಂತಿರುಗಿ activep;
            }
            // If there isn't one, create a new one
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = scopep->fileline();
            Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ stp = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, Ast_Sen_ಅಂಶ::Combo{}}};
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "", stp};
            activep->sen_ವೃಕ್ಷ_Storep(stp);
            scopep->ಕೂಡಿಸು_Blocksp(activep);
            ಹಿಂತಿರುಗಿ activep;
        }();

        // Required data types
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& idxDType = srcp->dtype();
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಬಿಟ್_DType = Dfg_ದತ್ತಾಂಶ_Type::packed(1);
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& tabDType = Dfg_ದತ್ತಾಂಶ_Type::array(ಬಿಟ್_DType, n_ಬಿಟ್ಟುಗಳು);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            ಆದರೆ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vp = srcp->get_ಫಲಿತಾಂಶ_Var()) ಹಿಂತಿರುಗಿ vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->is_ಆಂತರಿಕ(ನಿಜ);
            vtxp->srcp(srcp);
            ಹಿಂತಿರುಗಿ vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ಸ್ಥಿರಾಂಕ idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ಸ್ಥಿರಾಂಕ preVscp = [&]() {
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->is_ಆಂತರಿಕ(ನಿಜ);
            vscp->varp()->noReset(ನಿಜ);
            vscp->varp()->ಗಣ_Ignore_Sched_Write();
            ಹಿಂತಿರುಗಿ vscp;
        }();
        // The table variable
        DfgVarArray* ಸ್ಥಿರಾಂಕ tabVtxp = [&]() {
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->is_ಆಂತರಿಕ(ನಿಜ);
            varp->vscp()->varp()->noReset(ನಿಜ);
            varp->ಗಣ_Has_Mod_Wr_Refs();
            ಹಿಂತಿರುಗಿ varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ಸ್ಥಿರಾಂಕ initp = ಹೊಸ AstInitialStatic{flp, ಶೂನ್ಯಸೂಚಕ};
        staticActivep->ಕೂಡಿಸು_Stmtsp(initp);
        {  // pre = 0
            initp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                flp,  //
                ಹೊಸ AstVarRef{flp, preVscp, VAccess::WRITE},  //
                ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, static_cast<ಪೂರ್ಣಾಂಕ>(ಅಗಲ), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
                flp, ಹೊಸ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}});
            callp->dtype_ಗಣ_Void();
            initp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ಸ್ಥಿರಾಂಕ logicp = ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        combActivep->ಕೂಡಿಸು_Stmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                flp,  //
                ಹೊಸ AstArraySel{flp, ಹೊಸ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                ಹೊಸ AstVarRef{flp, preVscp, VAccess::READ}},  //
                ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ಸ್ಥಿರಾಂಕ idxRefp = ಹೊಸ AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                flp,  //
                ಹೊಸ AstArraySel{flp, ಹೊಸ AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}});
            DfgAstRd* ಸ್ಥಿರಾಂಕ astRdp = ಹೊಸ DfgAstRd{dfg, idxRefp, ಸುಳ್ಳು, ಸುಳ್ಳು};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ಸ್ಥಿರಾಂಕ idxRefp = ಹೊಸ AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp,  //
                                            ಹೊಸ AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ಸ್ಥಿರಾಂಕ astRdp = ಹೊಸ DfgAstRd{dfg, idxRefp, ಸುಳ್ಳು, ಸುಳ್ಳು};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : val2Terms) {
            ಸ್ಥಿರಾಂಕ uint32_t val = pair.ಮೊದಲ;
            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಪದ_2>& terms = pair.ಎರಡನೇ;
            // Create the ArraySel
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ಸ್ಥಿರಾಂಕ aselp = ಹೊಸ DfgArraySel{dfg, aflp, ಬಿಟ್_DType};
            aselp->fromp(tabVtxp);
            aselp->bitp(ಹೊಸ DfgConst{dfg, aflp, ಅಗಲ, val});
            // The inverted value, if needed
            DfgNot* notp = ಶೂನ್ಯಸೂಚಕ;
            // Repalce the terms
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಪದ_2& ಪದ : terms) {
                ಆದರೆ (ಪದ.m_inv) {
                    ಆದರೆ (!notp) {
                        notp = ಹೊಸ DfgNot{dfg, aflp, ಬಿಟ್_DType};
                        notp->srcp(aselp);
                    }
                    ಪದ.m_vtxp->replaceWith(notp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಪದ.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(ಪದ.m_vtxp->unlinkDelete(dfg), ಪದ.m_vtxp);
            }
        }
    }
}
