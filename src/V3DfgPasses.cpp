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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3DfgPasses.h"

#ضمن "V3Ast.h"
#ضمن "V3Dfg.h"
#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3String.h"

VL_DEFINE_DEBUG_FUNCTIONS;

فراغ V3DfgPasses::removeUnobservable(Dfg_گراف& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    لكل (Dfg_راس& vtx : dfg.opVertices()) {
        إذا (vtx.is<DfgLogic>()) workList.ڌڪو_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](Dfg_راس& vtx) {
        DfgLogic* ثابت logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ثابت منطقي استعمال_ٿيل = logicp->foreachSink([&](Dfg_راس& snk) {
            DfgUnresolved* ثابت uVtxp = snk.as<DfgUnresolved>();
            Dfg_راس_Var* ثابت vVtxp = uVtxp->firtsSinkp()->as<Dfg_راس_Var>();
            إذا (vVtxp->hasSinks()) أرجع سچ;
            إذا (vVtxp->isObserved()) أرجع سچ;
            أرجع خطأ;
        });
        // If some are used, the logic must stay in the Ast
        إذا (استعمال_ٿيل) أرجع;
        // If impure, it must stay in the Ast
        إذا (!logicp->is_خالص()) أرجع;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreach_ذريعو([&](Dfg_راس& src) {
            Dfg_راس_Var* ثابت varp = src.as<Dfg_راس_Var>();
            إذا (!varp->srcp()) أرجع خطأ;
            varp->srcp()->as<DfgUnresolved>()->foreach_ذريعو([&](Dfg_راس& driver) {
                workList.ڌڪو_front(*driver.as<DfgLogic>());
                أرجع خطأ;
            });
            أرجع خطأ;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_ڳنڍ* ثابت nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    لكل (Dfg_راس_Var* ثابت vVtxp : dfg.varVertices().unlinkable()) {
        إذا (vVtxp->hasSinks()) استمر;
        إذا (vVtxp->isObserved()) استمر;
        Dfg_راس* ثابت srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ثابت vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ثابت منطقي delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        إذا (srcp) {
            srcp->foreach_ذريعو([&](Dfg_راس& src) {
                src.as<DfgLogic>()->سيٽ_Drives_اڻ_استعمال_ٿيل_Vars();
                أرجع خطأ;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        إذا (delAst) {
            VL_DO_DANGLING(vscp->unlink_Fr_پٺ()->delete_وڻ(), vscp);
            ++ctx.m_varsDeleted;
        } وإلا {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    لكل (Dfg_راس* ثابت vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
        إذا (!logicp) استمر;
        إذا (logicp->hasSinks()) استمر;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ثابت VNUser2_اندريون_Use m_user2_اندريون_Use;
        logicp->foreach_ذريعو([](Dfg_راس& src) {
            src.as<Dfg_راس_Var>()->vscp()->user2p(&src);
            أرجع خطأ;
        });
        V3DfgPasses::جوڙيو_Ast_Refs(dfg, logicp->nodep(), [](Ast_ڳنڍ* varp) {  //
            أرجع varp->user2u().منزل<Dfg_راس_Var*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

فراغ V3DfgPasses::removeSelects(Dfg_گراف& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::ويڪٽر<DfgSel*> selps;
    لكل (Dfg_راس& vtx : dfg.opVertices()) {
        DfgSel* ثابت selp = vtx.cast<DfgSel>();
        إذا (!selp) استمر;
        selps.ڌڪو_پٺ(selp);
    }

    لكل (DfgSel* ثابت selp : selps) {
        File_سٽ* ثابت flp = selp->fileline();
        ثابت Dfg_معلومات_Type& dtype = selp->dtype();

        // Remove full width selects
        إذا (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removed_ڀريل_ويڪر;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            استمر;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ثابت varp = selp->fromp()->cast<DfgVarPacked>();
        إذا (!varp || !varp->tmpForp()) استمر;

        // Find the driver of this range
        ثابت خودڪار pair = varp->driver_جو_Range(selp->lsb(), selp->ويڪر());
        Dfg_راس* ثابت driverp = pair.پهريون;
        ثابت uint32_t driverLsb = pair.ٻيو;
        إذا (!driverp) استمر;

        // If partial driver is the whole thing we are looking for, just replace with that
        إذا (driverp->dtype() == dtype) {
            ++ctx.m_replaced_With_سڄو_Driver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            استمر;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ثابت newSelp = جديد DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

فراغ V3DfgPasses::inlineVars(Dfg_گراف& dfg) {
    لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        إذا (!vtx.hasSinks()) استمر;
        // Nohting to inline
        Dfg_راس* ثابت srcp = vtx.srcp();
        إذا (!srcp) استمر;
        // Value can differ from driver
        إذا (vtx.isVolatile()) استمر;
        // Partial driver cannot be inlined
        إذا (srcp->is<Dfg_راس_Splice>()) استمر;
        إذا (srcp->is<DfgUnitArray>()) استمر;
        // Don't inline CReset
        إذا (srcp->is<DfgCReset>()) استمر;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

فراغ V3DfgPasses::remove_اڻ_استعمال_ٿيل(Dfg_گراف& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    لكل (Dfg_راس& vtx : dfg.opVertices()) {
        إذا (vtx.hasSinks()) استمر;
        // This vertex is unused. Add to work list.
        workList.ڌڪو_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
        إذا (!vtx.tmpForp()) استمر;
        إذا (vtx.hasSinks()) استمر;
        إذا (vtx.hasDfgRefs()) استمر;
        // This vertex is unused. Add to work list.
        workList.ڌڪو_front(vtx);
    }

    // Process the work list
    workList.foreach([&](Dfg_راس& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        إذا (vtx.hasSinks()) أرجع;
        // If temporary used in another graph, we need to keep it
        إذا (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            إذا (varp->hasDfgRefs()) أرجع;
        }
        // Add sources of unused vertex to work list
        vtx.foreach_ذريعو([&](Dfg_راس& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            إذا (src.is<DfgConst>()) أرجع خطأ;
            ثابت Dfg_راس_Var* ثابت varp = src.cast<Dfg_راس_Var>();
            إذا (varp && !varp->tmpForp()) أرجع خطأ;
            // Add source to workList
            workList.ڌڪو_front(src);
            أرجع خطأ;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    لكل (Dfg_راس_Var* ثابت vtxp : dfg.varVertices().unlinkable()) {
        إذا (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    لكل (DfgConst* ثابت vtxp : dfg.constVertices().unlinkable()) {
        إذا (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

فراغ V3DfgPasses::bin_منزل_One_Hot(Dfg_گراف& dfg, V3_Dfg_Bin_منزل_One_Hot_Context& ctx) {
    // Structure to keep track of comparison details
    بنية رڪن_2 final {
        Dfg_راس* m_vtxp = خالي_اشارو;  // Vertex to replace
        منطقي m_inv = خطأ;  // '!=', instead of '=='
        رڪن_2() = افتراضي;
        رڪن_2(Dfg_راس* vtxp, منطقي inv)
            : m_vtxp{vtxp}
            , m_inv{inv} {}
    };

    // List of vertices that are used as sources
    std::ويڪٽر<Dfg_راس*> srcps;

    // Map from 'vertices' -> 'value beign compared' -> 'terms'
    استعمال Val2Terms = std::map<uint32_t, std::ويڪٽر<رڪن_2>>;
    DfgUserMap<Val2Terms> vtx2Val2Terms = dfg.makeUserMap<Val2Terms>();

    // Only consider input variables from a reasonable range:
    // - not too big to avoid huge tables, you are doomed anyway at that point..
    // - not too small, as it's probably not worth it
    constexpr uint32_t ويڪر_MIN = 7;
    constexpr uint32_t ويڪر_MAX = 20;
    ثابت خودڪار ويڪر_Ok = [](ثابت Dfg_راس* vtxp) {
        ثابت uint32_t ويڪر = vtxp->ويڪر();
        أرجع ويڪر_MIN <= ويڪر && ويڪر <= ويڪر_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ثابت std::function<منطقي(ثابت Dfg_راس*, منطقي)> useOk
        = [&](ثابت Dfg_راس* vtxp, منطقي inv) -> منطقي {
        // Go past a single 'Not' sink, which is common
        إذا (Dfg_راس* ثابت sinkp = vtxp->singleSink()) {
            إذا (sinkp->is<DfgNot>()) أرجع useOk(sinkp, !inv);
        }
        ثابت منطقي cond_وڻ = vtxp->foreachSink([&](ثابت Dfg_راس& sink) {
            ثابت DfgCond* ثابت condp = sink.cast<DfgCond>();
            إذا (!condp) أرجع خطأ;
            إذا (condp->condp() != vtxp) أرجع خطأ;
            أرجع inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        أرجع !cond_وڻ;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    لكل (Dfg_راس& vtx : dfg.opVertices()) {
        Dfg_راس* srcp = خالي_اشارو;
        uint32_t val = 0;
        منطقي inv = خطأ;
        إذا (DfgEq* ثابت eqp = vtx.cast<DfgEq>()) {
            DfgConst* ثابت constp = eqp->lhsp()->cast<DfgConst>();
            إذا (!constp || !ويڪر_Ok(constp) || !useOk(eqp, خطأ)) استمر;
            srcp = eqp->rhsp();
            val = constp->منزل_U32();
            inv = خطأ;
        } وإلا إذا (DfgNeq* ثابت neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ثابت constp = neqp->lhsp()->cast<DfgConst>();
            إذا (!constp || !ويڪر_Ok(constp) || !useOk(neqp, سچ)) استمر;
            srcp = neqp->rhsp();
            val = constp->منزل_U32();
            inv = سچ;
        } وإلا إذا (DfgRedAnd* ثابت redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            إذا (!ويڪر_Ok(srcp) || !useOk(redAndp, خطأ)) استمر;
            val = (1U << srcp->ويڪر()) - 1;
            inv = خطأ;
        } وإلا إذا (DfgRedOr* ثابت redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            إذا (!ويڪر_Ok(srcp) || !useOk(redOrp, سچ)) استمر;
            val = 0;
            inv = سچ;
        } وإلا إذا (DfgSel* ثابت selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            إذا (selp->ويڪر() != 1) استمر;
            Dfg_سرڪايو_L* ثابت سرڪايو_Lp = selp->fromp()->cast<Dfg_سرڪايو_L>();
            إذا (!سرڪايو_Lp) استمر;
            ثابت DfgConst* ثابت constp = سرڪايو_Lp->lhsp()->cast<DfgConst>();
            إذا (!constp || !useOk(selp, خطأ)) استمر;
            إذا (!constp->has_قدر(1)) استمر;
            srcp = سرڪايو_Lp->rhsp();
            إذا (!ويڪر_Ok(srcp)) استمر;
            val = selp->lsb();
            inv = خطأ;
        } وإلا {
            // Not a comparison-like vertex
            استمر;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        إذا (val2Terms.خالي_جايون()) srcps.emplace_پٺ(srcp);
        // Record term
        val2Terms[val].emplace_پٺ(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t رڪن_حد_بندي = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << ويڪر_MIN) >= رڪن_حد_بندي, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    إذا (nTerms < رڪن_حد_بندي) أرجع;

    Dfg_راس::ScopeCache scopeCache;

    // Create decoders for each srcp
    لكل (Dfg_راس* ثابت srcp : srcps) {
        ثابت Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        إذا (val2Terms.سائيز() < رڪن_حد_بندي) استمر;

        // Width of the decoded binary value
        ثابت uint32_t ويڪر = srcp->ويڪر();
        // Number of bits in the input operand
        ثابت uint32_t n_بٽ = 1U << ويڪر;

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

        File_سٽ* ثابت flp = srcp->fileline();
        AstScope* ثابت scopep = srcp->scopep(scopeCache, سچ);

        Ast_سرگرم* ثابت staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            لكل (Ast_ڳنڍ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_سرگرم* ثابت activep = VN_CAST(nodep, سرگرم);
                إذا (!activep) استمر;
                إذا (activep->sentreep()->hasStatic()) أرجع activep;
            }
            // If there isn't one, create a new one
            File_سٽ* ثابت flp = scopep->fileline();
            Ast_Sen_وڻ* ثابت stp = جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, Ast_Sen_شي::Static{}}};
            Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "", stp};
            activep->sen_وڻ_Storep(stp);
            scopep->جوڙيو_Blocksp(activep);
            أرجع activep;
        }();
        Ast_سرگرم* ثابت combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            لكل (Ast_ڳنڍ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_سرگرم* ثابت activep = VN_CAST(nodep, سرگرم);
                إذا (!activep) استمر;
                إذا (activep->sentreep()->hasCombo()) أرجع activep;
            }
            // If there isn't one, create a new one
            File_سٽ* ثابت flp = scopep->fileline();
            Ast_Sen_وڻ* ثابت stp = جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, Ast_Sen_شي::Combo{}}};
            Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "", stp};
            activep->sen_وڻ_Storep(stp);
            scopep->جوڙيو_Blocksp(activep);
            أرجع activep;
        }();

        // Required data types
        ثابت Dfg_معلومات_Type& idxDType = srcp->dtype();
        ثابت Dfg_معلومات_Type& ٻٽو_انگ_DType = Dfg_معلومات_Type::packed(1);
        ثابت Dfg_معلومات_Type& tabDType = Dfg_معلومات_Type::array(ٻٽو_انگ_DType, n_بٽ);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            إذا (Dfg_راس_Var* ثابت vp = srcp->get_نتيجو_Var()) أرجع vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            Dfg_راس_Var* ثابت vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->is_اندروني(سچ);
            vtxp->srcp(srcp);
            أرجع vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ثابت idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ثابت preVscp = [&]() {
            Dfg_راس_Var* ثابت vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ثابت vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->is_اندروني(سچ);
            vscp->varp()->noReset(سچ);
            vscp->varp()->سيٽ_Ignore_Sched_Write();
            أرجع vscp;
        }();
        // The table variable
        DfgVarArray* ثابت tabVtxp = [&]() {
            Dfg_راس_Var* ثابت varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->is_اندروني(سچ);
            varp->vscp()->varp()->noReset(سچ);
            varp->سيٽ_Has_Mod_Wr_Refs();
            أرجع varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ثابت initp = جديد AstInitialStatic{flp, خالي_اشارو};
        staticActivep->جوڙيو_Stmtsp(initp);
        {  // pre = 0
            initp->جوڙيو_Stmtsp(جديد Ast_سونپيو{
                flp,  //
                جديد AstVarRef{flp, preVscp, VAccess::WRITE},  //
                جديد AstConst{flp, AstConst::Widthed_قدر{}, static_cast<صحيح>(ويڪر), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->جوڙيو_Pinsp(جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}});
            callp->dtype_سيٽ_Void();
            initp->جوڙيو_Stmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ثابت logicp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, خالي_اشارو, خالي_اشارو};
        combActivep->جوڙيو_Stmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->جوڙيو_Stmtsp(جديد Ast_سونپيو{
                flp,  //
                جديد AstArraySel{flp, جديد AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                جديد AstVarRef{flp, preVscp, VAccess::READ}},  //
                جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ثابت idxRefp = جديد AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->جوڙيو_Stmtsp(جديد Ast_سونپيو{
                flp,  //
                جديد AstArraySel{flp, جديد AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}});
            DfgAstRd* ثابت astRdp = جديد DfgAstRd{dfg, idxRefp, خطأ, خطأ};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ثابت idxRefp = جديد AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp,  //
                                            جديد AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ثابت astRdp = جديد DfgAstRd{dfg, idxRefp, خطأ, خطأ};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        لكل (ثابت خودڪار& pair : val2Terms) {
            ثابت uint32_t val = pair.پهريون;
            ثابت std::ويڪٽر<رڪن_2>& terms = pair.ٻيو;
            // Create the ArraySel
            File_سٽ* ثابت aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ثابت aselp = جديد DfgArraySel{dfg, aflp, ٻٽو_انگ_DType};
            aselp->fromp(tabVtxp);
            aselp->bitp(جديد DfgConst{dfg, aflp, ويڪر, val});
            // The inverted value, if needed
            DfgNot* notp = خالي_اشارو;
            // Repalce the terms
            لكل (ثابت رڪن_2& رڪن : terms) {
                إذا (رڪن.m_inv) {
                    إذا (!notp) {
                        notp = جديد DfgNot{dfg, aflp, ٻٽو_انگ_DType};
                        notp->srcp(aselp);
                    }
                    رڪن.m_vtxp->replaceWith(notp);
                } وإلا {
                    رڪن.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(رڪن.m_vtxp->unlinkDelete(dfg), رڪن.m_vtxp);
            }
        }
    }
}
