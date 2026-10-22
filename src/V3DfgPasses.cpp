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
        إذا (vtx.is<DfgLogic>()) workList.ڈالو_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](Dfg_راس& vtx) {
        DfgLogic* ثابت logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ثابت منطقي ورتیا = logicp->foreachSink([&](Dfg_راس& snk) {
            DfgUnresolved* ثابت uVtxp = snk.as<DfgUnresolved>();
            Dfg_راس_Var* ثابت vVtxp = uVtxp->firtsSinkp()->as<Dfg_راس_Var>();
            إذا (vVtxp->hasSinks()) أرجع سچ;
            إذا (vVtxp->isObserved()) أرجع سچ;
            أرجع خطأ;
        });
        // If some are used, the logic must stay in the Ast
        إذا (ورتیا) أرجع;
        // If impure, it must stay in the Ast
        إذا (!logicp->is_خالص()) أرجع;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreach_سروت([&](Dfg_راس& src) {
            Dfg_راس_Var* ثابت varp = src.as<Dfg_راس_Var>();
            إذا (!varp->srcp()) أرجع خطأ;
            varp->srcp()->as<DfgUnresolved>()->foreach_سروت([&](Dfg_راس& driver) {
                workList.ڈالو_front(*driver.as<DfgLogic>());
                أرجع خطأ;
            });
            أرجع خطأ;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_گرہ* ثابت nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
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
            srcp->foreach_سروت([&](Dfg_راس& src) {
                src.as<DfgLogic>()->سیٹ_Drives_بن_ورتیا_Vars();
                أرجع خطأ;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        إذا (delAst) {
            VL_DO_DANGLING(vscp->unlink_Fr_پٹھ()->delete_درخت(), vscp);
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
        ثابت VNUser2_وچ_Use m_user2_وچ_Use;
        logicp->foreach_سروت([](Dfg_راس& src) {
            src.as<Dfg_راس_Var>()->vscp()->user2p(&src);
            أرجع خطأ;
        });
        V3DfgPasses::جوڑو_Ast_Refs(dfg, logicp->nodep(), [](Ast_گرہ* varp) {  //
            أرجع varp->user2u().منزل<Dfg_راس_Var*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

فراغ V3DfgPasses::removeSelects(Dfg_گراف& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::سمتیہ<DfgSel*> selps;
    لكل (Dfg_راس& vtx : dfg.opVertices()) {
        DfgSel* ثابت selp = vtx.cast<DfgSel>();
        إذا (!selp) استمر;
        selps.ڈالو_پٹھ(selp);
    }

    لكل (DfgSel* ثابت selp : selps) {
        File_سطر* ثابت flp = selp->fileline();
        ثابت Dfg_انکڑے_Type& dtype = selp->dtype();

        // Remove full width selects
        إذا (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removed_بھریا_چوڑائی;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            استمر;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ثابت varp = selp->fromp()->cast<DfgVarPacked>();
        إذا (!varp || !varp->tmpForp()) استمر;

        // Find the driver of this range
        ثابت خودکار pair = varp->driver_دا_Range(selp->lsb(), selp->چوڑائی());
        Dfg_راس* ثابت driverp = pair.پہلا;
        ثابت uint32_t driverLsb = pair.دوجا;
        إذا (!driverp) استمر;

        // If partial driver is the whole thing we are looking for, just replace with that
        إذا (driverp->dtype() == dtype) {
            ++ctx.m_replaced_With_سارا_Driver;
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

فراغ V3DfgPasses::remove_بن_ورتیا(Dfg_گراف& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    لكل (Dfg_راس& vtx : dfg.opVertices()) {
        إذا (vtx.hasSinks()) استمر;
        // This vertex is unused. Add to work list.
        workList.ڈالو_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
        إذا (!vtx.tmpForp()) استمر;
        إذا (vtx.hasSinks()) استمر;
        إذا (vtx.hasDfgRefs()) استمر;
        // This vertex is unused. Add to work list.
        workList.ڈالو_front(vtx);
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
        vtx.foreach_سروت([&](Dfg_راس& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            إذا (src.is<DfgConst>()) أرجع خطأ;
            ثابت Dfg_راس_Var* ثابت varp = src.cast<Dfg_راس_Var>();
            إذا (varp && !varp->tmpForp()) أرجع خطأ;
            // Add source to workList
            workList.ڈالو_front(src);
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
    بنية رکن_2 final {
        Dfg_راس* m_vtxp = خالی_سوچک;  // Vertex to replace
        منطقي m_inv = خطأ;  // '!=', instead of '=='
        رکن_2() = افتراضي;
        رکن_2(Dfg_راس* vtxp, منطقي inv)
            : m_vtxp{vtxp}
            , m_inv{inv} {}
    };

    // List of vertices that are used as sources
    std::سمتیہ<Dfg_راس*> srcps;

    // Map from 'vertices' -> 'value beign compared' -> 'terms'
    ورتوں Val2Terms = std::map<uint32_t, std::سمتیہ<رکن_2>>;
    DfgUserMap<Val2Terms> vtx2Val2Terms = dfg.makeUserMap<Val2Terms>();

    // Only consider input variables from a reasonable range:
    // - not too big to avoid huge tables, you are doomed anyway at that point..
    // - not too small, as it's probably not worth it
    constexpr uint32_t چوڑائی_MIN = 7;
    constexpr uint32_t چوڑائی_MAX = 20;
    ثابت خودکار چوڑائی_Ok = [](ثابت Dfg_راس* vtxp) {
        ثابت uint32_t چوڑائی = vtxp->چوڑائی();
        أرجع چوڑائی_MIN <= چوڑائی && چوڑائی <= چوڑائی_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ثابت std::function<منطقي(ثابت Dfg_راس*, منطقي)> useOk
        = [&](ثابت Dfg_راس* vtxp, منطقي inv) -> منطقي {
        // Go past a single 'Not' sink, which is common
        إذا (Dfg_راس* ثابت sinkp = vtxp->singleSink()) {
            إذا (sinkp->is<DfgNot>()) أرجع useOk(sinkp, !inv);
        }
        ثابت منطقي cond_درخت = vtxp->foreachSink([&](ثابت Dfg_راس& sink) {
            ثابت DfgCond* ثابت condp = sink.cast<DfgCond>();
            إذا (!condp) أرجع خطأ;
            إذا (condp->condp() != vtxp) أرجع خطأ;
            أرجع inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        أرجع !cond_درخت;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    لكل (Dfg_راس& vtx : dfg.opVertices()) {
        Dfg_راس* srcp = خالی_سوچک;
        uint32_t val = 0;
        منطقي inv = خطأ;
        إذا (DfgEq* ثابت eqp = vtx.cast<DfgEq>()) {
            DfgConst* ثابت constp = eqp->lhsp()->cast<DfgConst>();
            إذا (!constp || !چوڑائی_Ok(constp) || !useOk(eqp, خطأ)) استمر;
            srcp = eqp->rhsp();
            val = constp->منزل_U32();
            inv = خطأ;
        } وإلا إذا (DfgNeq* ثابت neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ثابت constp = neqp->lhsp()->cast<DfgConst>();
            إذا (!constp || !چوڑائی_Ok(constp) || !useOk(neqp, سچ)) استمر;
            srcp = neqp->rhsp();
            val = constp->منزل_U32();
            inv = سچ;
        } وإلا إذا (DfgRedAnd* ثابت redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            إذا (!چوڑائی_Ok(srcp) || !useOk(redAndp, خطأ)) استمر;
            val = (1U << srcp->چوڑائی()) - 1;
            inv = خطأ;
        } وإلا إذا (DfgRedOr* ثابت redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            إذا (!چوڑائی_Ok(srcp) || !useOk(redOrp, سچ)) استمر;
            val = 0;
            inv = سچ;
        } وإلا إذا (DfgSel* ثابت selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            إذا (selp->چوڑائی() != 1) استمر;
            Dfg_کھسکاؤ_L* ثابت کھسکاؤ_Lp = selp->fromp()->cast<Dfg_کھسکاؤ_L>();
            إذا (!کھسکاؤ_Lp) استمر;
            ثابت DfgConst* ثابت constp = کھسکاؤ_Lp->lhsp()->cast<DfgConst>();
            إذا (!constp || !useOk(selp, خطأ)) استمر;
            إذا (!constp->has_قدر(1)) استمر;
            srcp = کھسکاؤ_Lp->rhsp();
            إذا (!چوڑائی_Ok(srcp)) استمر;
            val = selp->lsb();
            inv = خطأ;
        } وإلا {
            // Not a comparison-like vertex
            استمر;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        إذا (val2Terms.خالی_تھاواں()) srcps.emplace_پٹھ(srcp);
        // Record term
        val2Terms[val].emplace_پٹھ(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t رکن_حد_بندی = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << چوڑائی_MIN) >= رکن_حد_بندی, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    إذا (nTerms < رکن_حد_بندی) أرجع;

    Dfg_راس::ScopeCache scopeCache;

    // Create decoders for each srcp
    لكل (Dfg_راس* ثابت srcp : srcps) {
        ثابت Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        إذا (val2Terms.ناپ() < رکن_حد_بندی) استمر;

        // Width of the decoded binary value
        ثابت uint32_t چوڑائی = srcp->چوڑائی();
        // Number of bits in the input operand
        ثابت uint32_t n_بٹ = 1U << چوڑائی;

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
        AstScope* ثابت scopep = srcp->scopep(scopeCache, سچ);

        Ast_سرگرم* ثابت staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            لكل (Ast_گرہ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_سرگرم* ثابت activep = VN_CAST(nodep, سرگرم);
                إذا (!activep) استمر;
                إذا (activep->sentreep()->hasStatic()) أرجع activep;
            }
            // If there isn't one, create a new one
            File_سطر* ثابت flp = scopep->fileline();
            Ast_Sen_درخت* ثابت stp = جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, Ast_Sen_شے::Static{}}};
            Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "", stp};
            activep->sen_درخت_Storep(stp);
            scopep->جوڑو_Blocksp(activep);
            أرجع activep;
        }();
        Ast_سرگرم* ثابت combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            لكل (Ast_گرہ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_سرگرم* ثابت activep = VN_CAST(nodep, سرگرم);
                إذا (!activep) استمر;
                إذا (activep->sentreep()->hasCombo()) أرجع activep;
            }
            // If there isn't one, create a new one
            File_سطر* ثابت flp = scopep->fileline();
            Ast_Sen_درخت* ثابت stp = جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, Ast_Sen_شے::Combo{}}};
            Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "", stp};
            activep->sen_درخت_Storep(stp);
            scopep->جوڑو_Blocksp(activep);
            أرجع activep;
        }();

        // Required data types
        ثابت Dfg_انکڑے_Type& idxDType = srcp->dtype();
        ثابت Dfg_انکڑے_Type& ثنائی_ہندسہ_DType = Dfg_انکڑے_Type::packed(1);
        ثابت Dfg_انکڑے_Type& tabDType = Dfg_انکڑے_Type::array(ثنائی_ہندسہ_DType, n_بٹ);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            إذا (Dfg_راس_Var* ثابت vp = srcp->get_نتیجہ_Var()) أرجع vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            Dfg_راس_Var* ثابت vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->is_اندرونی(سچ);
            vtxp->srcp(srcp);
            أرجع vtxp->as<DfgVarPacked>();
        }();
        AstVarScope* ثابت idxVscp = idxVtxp->vscp();
        // The previous index variable - we don't need a vertex for this
        AstVarScope* ثابت preVscp = [&]() {
            Dfg_راس_Var* ثابت vtxp = dfg.makeNewVar(flp, "BinToOneHot_Pre", idxDType, scopep);
            AstVarScope* ثابت vscp = vtxp->vscp();
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
            vscp->varp()->is_اندرونی(سچ);
            vscp->varp()->noReset(سچ);
            vscp->varp()->سیٹ_Ignore_Sched_Write();
            أرجع vscp;
        }();
        // The table variable
        DfgVarArray* ثابت tabVtxp = [&]() {
            Dfg_راس_Var* ثابت varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->is_اندرونی(سچ);
            varp->vscp()->varp()->noReset(سچ);
            varp->سیٹ_Has_Mod_Wr_Refs();
            أرجع varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ثابت initp = جديد AstInitialStatic{flp, خالی_سوچک};
        staticActivep->جوڑو_Stmtsp(initp);
        {  // pre = 0
            initp->جوڑو_Stmtsp(جديد Ast_سونپو{
                flp,  //
                جديد AstVarRef{flp, preVscp, VAccess::WRITE},  //
                جديد AstConst{flp, AstConst::Widthed_قدر{}, static_cast<صحيح>(چوڑائی), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->جوڑو_Pinsp(جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
            callp->dtype_سیٹ_Void();
            initp->جوڑو_Stmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ثابت logicp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, خالی_سوچک, خالی_سوچک};
        combActivep->جوڑو_Stmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->جوڑو_Stmtsp(جديد Ast_سونپو{
                flp,  //
                جديد AstArraySel{flp, جديد AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                جديد AstVarRef{flp, preVscp, VAccess::READ}},  //
                جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ثابت idxRefp = جديد AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->جوڑو_Stmtsp(جديد Ast_سونپو{
                flp,  //
                جديد AstArraySel{flp, جديد AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}});
            DfgAstRd* ثابت astRdp = جديد DfgAstRd{dfg, idxRefp, خطأ, خطأ};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ثابت idxRefp = جديد AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->جوڑو_Stmtsp(جديد Ast_سونپو{flp,  //
                                            جديد AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ثابت astRdp = جديد DfgAstRd{dfg, idxRefp, خطأ, خطأ};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        لكل (ثابت خودکار& pair : val2Terms) {
            ثابت uint32_t val = pair.پہلا;
            ثابت std::سمتیہ<رکن_2>& terms = pair.دوجا;
            // Create the ArraySel
            File_سطر* ثابت aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ثابت aselp = جديد DfgArraySel{dfg, aflp, ثنائی_ہندسہ_DType};
            aselp->fromp(tabVtxp);
            aselp->bitp(جديد DfgConst{dfg, aflp, چوڑائی, val});
            // The inverted value, if needed
            DfgNot* notp = خالی_سوچک;
            // Repalce the terms
            لكل (ثابت رکن_2& رکن : terms) {
                إذا (رکن.m_inv) {
                    إذا (!notp) {
                        notp = جديد DfgNot{dfg, aflp, ثنائی_ہندسہ_DType};
                        notp->srcp(aselp);
                    }
                    رکن.m_vtxp->replaceWith(notp);
                } وإلا {
                    رکن.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(رکن.m_vtxp->unlinkDelete(dfg), رکن.m_vtxp);
            }
        }
    }
}
