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

خالی V3DfgPasses::removeUnobservable(Dfg_گراف& dfg, V3DfgContext& dfgCtx) {
    V3DfgRemoveUnobservableContext& ctx = dfgCtx.m_removeUnobservableContext;

    // Enqueue all DfgLogic vertices to work list
    DfgWorklist workList{dfg};
    برائے (Dfg_راس& vtx : dfg.opVertices()) {
        اگر (vtx.is<DfgLogic>()) workList.ڈالو_front(vtx);
    }

    // Remove all logic that only drives unobservable variables
    workList.foreach([&](Dfg_راس& vtx) {
        DfgLogic* ثابت logicp = vtx.as<DfgLogic>();
        // Check all variables driven by this logic are removable
        ثابت منطقی مستعمل = logicp->foreachSink([&](Dfg_راس& snk) {
            DfgUnresolved* ثابت uVtxp = snk.as<DfgUnresolved>();
            Dfg_راس_Var* ثابت vVtxp = uVtxp->firtsSinkp()->as<Dfg_راس_Var>();
            اگر (vVtxp->hasSinks()) واپس سچ;
            اگر (vVtxp->isObserved()) واپس سچ;
            واپس جھوٹ;
        });
        // If some are used, the logic must stay in the Ast
        اگر (مستعمل) واپس;
        // If impure, it must stay in the Ast
        اگر (!logicp->is_خالص()) واپس;
        // Enqueue logic driving the inputs of this logic we are about to delete
        logicp->foreach_ماخذ([&](Dfg_راس& src) {
            Dfg_راس_Var* ثابت varp = src.as<Dfg_راس_Var>();
            اگر (!varp->srcp()) واپس جھوٹ;
            varp->srcp()->as<DfgUnresolved>()->foreach_ماخذ([&](Dfg_راس& driver) {
                workList.ڈالو_front(*driver.as<DfgLogic>());
                واپس جھوٹ;
            });
            واپس جھوٹ;
        });
        // Delete this logic both from the Dfg and the Ast
        Ast_گرہ* ثابت nodep = logicp->nodep();
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        ++ctx.m_logicDeleted;
    });

    // Remove unobservable variables
    برائے (Dfg_راس_Var* ثابت vVtxp : dfg.varVertices().unlinkable()) {
        اگر (vVtxp->hasSinks()) جاری;
        اگر (vVtxp->isObserved()) جاری;
        Dfg_راس* ثابت srcp = vVtxp->srcp();  // Must be a DfgUnresolved or nullptr
        AstVarScope* ثابت vscp = vVtxp->vscp();
        // Can delete the Ast variable too if it has no other references
        ثابت منطقی delAst = (!srcp || !srcp->nInputs())  //
                            && !vVtxp->hasExtWrRefs()  //
                            && !vVtxp->hasModWrRefs();
        VL_DO_DANGLING(vVtxp->unlinkDelete(dfg), vVtxp);
        اگر (srcp) {
            srcp->foreach_ماخذ([&](Dfg_راس& src) {
                src.as<DfgLogic>()->سیٹ_Drives_غیر_مستعمل_Vars();
                واپس جھوٹ;
            });
            VL_DO_DANGLING(srcp->unlinkDelete(dfg), srcp);
        }
        اگر (delAst) {
            VL_DO_DANGLING(vscp->unlink_Fr_عقب()->delete_درخت(), vscp);
            ++ctx.m_varsDeleted;
        } ورنہ {
            ++ctx.m_varsRemoved;
        }
    }

    // Finally remove logic from the Dfg if it drives no variables in the graph.
    // These should only be those with side effects.
    برائے (Dfg_راس* ثابت vtxp : dfg.opVertices().unlinkable()) {
        DfgLogic* ثابت logicp = vtxp->cast<DfgLogic>();
        اگر (!logicp) جاری;
        اگر (logicp->hasSinks()) جاری;
        // Input variables will be read in Ast code, add Ast reference vertices
        // AstVar/AstVarScope::user2p() -> corresponding DfgVertexVar* in the graph
        ثابت VNUser2_اندر_Use m_user2_اندر_Use;
        logicp->foreach_ماخذ([](Dfg_راس& src) {
            src.as<Dfg_راس_Var>()->vscp()->user2p(&src);
            واپس جھوٹ;
        });
        V3DfgPasses::جوڑو_Ast_Refs(dfg, logicp->nodep(), [](Ast_گرہ* varp) {  //
            واپس varp->user2u().مقصد<Dfg_راس_Var*>();
        });
        VL_DO_DANGLING(logicp->unlinkDelete(dfg), logicp);
        ++ctx.m_logicRemoved;
    }
}

خالی V3DfgPasses::removeSelects(Dfg_گراف& dfg, V3DfgRemoveSelectsContext& ctx) {

    std::سمتیہ<DfgSel*> selps;
    برائے (Dfg_راس& vtx : dfg.opVertices()) {
        DfgSel* ثابت selp = vtx.cast<DfgSel>();
        اگر (!selp) جاری;
        selps.ڈالو_عقب(selp);
    }

    برائے (DfgSel* ثابت selp : selps) {
        File_سطر* ثابت flp = selp->fileline();
        ثابت Dfg_ڈیٹا_Type& dtype = selp->dtype();

        // Remove full width selects
        اگر (selp->fromp()->dtype() == dtype) {
            ++ctx.m_removed_بھرا_چوڑائی;
            selp->replaceWith(selp->fromp());
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            جاری;
        }

        // Push selects through synthesis temporaries only
        DfgVarPacked* ثابت varp = selp->fromp()->cast<DfgVarPacked>();
        اگر (!varp || !varp->tmpForp()) جاری;

        // Find the driver of this range
        ثابت خودکار pair = varp->driver_کا_Range(selp->lsb(), selp->چوڑائی());
        Dfg_راس* ثابت driverp = pair.پہلا;
        ثابت uint32_t driverLsb = pair.دوسرا;
        اگر (!driverp) جاری;

        // If partial driver is the whole thing we are looking for, just replace with that
        اگر (driverp->dtype() == dtype) {
            ++ctx.m_replaced_With_پورا_Driver;
            selp->replaceWith(driverp);
            VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
            جاری;
        }

        // Otherwise create a new select from the partial driver
        ++ctx.m_replacedWithSelFromDriver;
        DfgSel* ثابت newSelp = نیا DfgSel{dfg, flp, dtype};
        newSelp->lsb(driverLsb);
        newSelp->fromp(driverp);
        selp->replaceWith(newSelp);
        VL_DO_DANGLING(selp->unlinkDelete(dfg), selp);
    }
}

خالی V3DfgPasses::inlineVars(Dfg_گراف& dfg) {
    برائے (Dfg_راس_Var& vtx : dfg.varVertices()) {
        // Nothing to inline it into
        اگر (!vtx.hasSinks()) جاری;
        // Nohting to inline
        Dfg_راس* ثابت srcp = vtx.srcp();
        اگر (!srcp) جاری;
        // Value can differ from driver
        اگر (vtx.isVolatile()) جاری;
        // Partial driver cannot be inlined
        اگر (srcp->is<Dfg_راس_Splice>()) جاری;
        اگر (srcp->is<DfgUnitArray>()) جاری;
        // Don't inline CReset
        اگر (srcp->is<DfgCReset>()) جاری;
        // Okie dokie, here we go ...
        vtx.replaceWith(srcp);
    }
}

خالی V3DfgPasses::remove_غیر_مستعمل(Dfg_گراف& dfg) {
    // Worklist based algoritm
    DfgWorklist workList{dfg};

    // Add all unused operation vertices to the work list
    برائے (Dfg_راس& vtx : dfg.opVertices()) {
        اگر (vtx.hasSinks()) جاری;
        // This vertex is unused. Add to work list.
        workList.ڈالو_front(vtx);
    }

    // Also add all unused temporaries created during synthesis
    برائے (Dfg_راس_Var& vtx : dfg.varVertices()) {
        اگر (!vtx.tmpForp()) جاری;
        اگر (vtx.hasSinks()) جاری;
        اگر (vtx.hasDfgRefs()) جاری;
        // This vertex is unused. Add to work list.
        workList.ڈالو_front(vtx);
    }

    // Process the work list
    workList.foreach([&](Dfg_راس& vtx) {
        // DfgLogic should have been synthesized or removed
        UASSERT_OBJ(!vtx.is<DfgLogic>(), &vtx, "Should not be DfgLogic");
        // If used, then nothing to do, so move on
        اگر (vtx.hasSinks()) واپس;
        // If temporary used in another graph, we need to keep it
        اگر (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
            UASSERT_OBJ(varp->tmpForp(), varp, "Non-temporary variable should not be visited");
            اگر (varp->hasDfgRefs()) واپس;
        }
        // Add sources of unused vertex to work list
        vtx.foreach_ماخذ([&](Dfg_راس& src) {
            // We only remove actual operation vertices and synthesis temporaries in this loop
            اگر (src.is<DfgConst>()) واپس جھوٹ;
            ثابت Dfg_راس_Var* ثابت varp = src.cast<Dfg_راس_Var>();
            اگر (varp && !varp->tmpForp()) واپس جھوٹ;
            // Add source to workList
            workList.ڈالو_front(src);
            واپس جھوٹ;
        });
        // Remove the unused vertex
        vtx.unlinkDelete(dfg);
    });

    // Remove unused and undriven variable vertices
    برائے (Dfg_راس_Var* ثابت vtxp : dfg.varVertices().unlinkable()) {
        اگر (!vtxp->hasSinks() && !vtxp->srcp()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Finally remove unused constants
    برائے (DfgConst* ثابت vtxp : dfg.constVertices().unlinkable()) {
        اگر (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }
}

خالی V3DfgPasses::bin_مقصد_One_Hot(Dfg_گراف& dfg, V3_Dfg_Bin_مقصد_One_Hot_Context& ctx) {
    // Structure to keep track of comparison details
    ساخت رکن_2 final {
        Dfg_راس* m_vtxp = خالی_اشاریہ;  // Vertex to replace
        منطقی m_inv = جھوٹ;  // '!=', instead of '=='
        رکن_2() = طےشدہ;
        رکن_2(Dfg_راس* vtxp, منطقی inv)
            : m_vtxp{vtxp}
            , m_inv{inv} {}
    };

    // List of vertices that are used as sources
    std::سمتیہ<Dfg_راس*> srcps;

    // Map from 'vertices' -> 'value beign compared' -> 'terms'
    استعمال Val2Terms = std::map<uint32_t, std::سمتیہ<رکن_2>>;
    DfgUserMap<Val2Terms> vtx2Val2Terms = dfg.makeUserMap<Val2Terms>();

    // Only consider input variables from a reasonable range:
    // - not too big to avoid huge tables, you are doomed anyway at that point..
    // - not too small, as it's probably not worth it
    constexpr uint32_t چوڑائی_MIN = 7;
    constexpr uint32_t چوڑائی_MAX = 20;
    ثابت خودکار چوڑائی_Ok = [](ثابت Dfg_راس* vtxp) {
        ثابت uint32_t چوڑائی = vtxp->چوڑائی();
        واپس چوڑائی_MIN <= چوڑائی && چوڑائی <= چوڑائی_MAX;
    };

    // Do not convert terms that look like they are in a Cond tree
    // the C++ compiler can generate jump tables for these
    ثابت std::function<منطقی(ثابت Dfg_راس*, منطقی)> useOk
        = [&](ثابت Dfg_راس* vtxp, منطقی inv) -> منطقی {
        // Go past a single 'Not' sink, which is common
        اگر (Dfg_راس* ثابت sinkp = vtxp->singleSink()) {
            اگر (sinkp->is<DfgNot>()) واپس useOk(sinkp, !inv);
        }
        ثابت منطقی cond_درخت = vtxp->foreachSink([&](ثابت Dfg_راس& sink) {
            ثابت DfgCond* ثابت condp = sink.cast<DfgCond>();
            اگر (!condp) واپس جھوٹ;
            اگر (condp->condp() != vtxp) واپس جھوٹ;
            واپس inv ? condp->thenp()->is<DfgCond>() : condp->elsep()->is<DfgCond>();
        });
        واپس !cond_درخت;
    };

    // Look at all comparison nodes and build the 'Val2Terms' map for each source vertex
    uint32_t nTerms = 0;
    برائے (Dfg_راس& vtx : dfg.opVertices()) {
        Dfg_راس* srcp = خالی_اشاریہ;
        uint32_t val = 0;
        منطقی inv = جھوٹ;
        اگر (DfgEq* ثابت eqp = vtx.cast<DfgEq>()) {
            DfgConst* ثابت constp = eqp->lhsp()->cast<DfgConst>();
            اگر (!constp || !چوڑائی_Ok(constp) || !useOk(eqp, جھوٹ)) جاری;
            srcp = eqp->rhsp();
            val = constp->مقصد_U32();
            inv = جھوٹ;
        } ورنہ اگر (DfgNeq* ثابت neqp = vtx.cast<DfgNeq>()) {
            DfgConst* ثابت constp = neqp->lhsp()->cast<DfgConst>();
            اگر (!constp || !چوڑائی_Ok(constp) || !useOk(neqp, سچ)) جاری;
            srcp = neqp->rhsp();
            val = constp->مقصد_U32();
            inv = سچ;
        } ورنہ اگر (DfgRedAnd* ثابت redAndp = vtx.cast<DfgRedAnd>()) {
            srcp = redAndp->srcp();
            اگر (!چوڑائی_Ok(srcp) || !useOk(redAndp, جھوٹ)) جاری;
            val = (1U << srcp->چوڑائی()) - 1;
            inv = جھوٹ;
        } ورنہ اگر (DfgRedOr* ثابت redOrp = vtx.cast<DfgRedOr>()) {
            srcp = redOrp->srcp();
            اگر (!چوڑائی_Ok(srcp) || !useOk(redOrp, سچ)) جاری;
            val = 0;
            inv = سچ;
        } ورنہ اگر (DfgSel* ثابت selp = vtx.cast<DfgSel>()) {
            // Special case: Sel(ShiftL(1, src),lsb,width=1)
            اگر (selp->چوڑائی() != 1) جاری;
            Dfg_سرکاؤ_L* ثابت سرکاؤ_Lp = selp->fromp()->cast<Dfg_سرکاؤ_L>();
            اگر (!سرکاؤ_Lp) جاری;
            ثابت DfgConst* ثابت constp = سرکاؤ_Lp->lhsp()->cast<DfgConst>();
            اگر (!constp || !useOk(selp, جھوٹ)) جاری;
            اگر (!constp->has_قدر(1)) جاری;
            srcp = سرکاؤ_Lp->rhsp();
            اگر (!چوڑائی_Ok(srcp)) جاری;
            val = selp->lsb();
            inv = جھوٹ;
        } ورنہ {
            // Not a comparison-like vertex
            جاری;
        }
        // Grab Val2Terms for this vertex
        Val2Terms& val2Terms = vtx2Val2Terms[srcp];
        // Remeber and on first encounter
        اگر (val2Terms.خالی_خانے()) srcps.emplace_عقب(srcp);
        // Record term
        val2Terms[val].emplace_عقب(&vtx, inv);
        ++nTerms;
    }

    // Somewhat arbitrarily, only apply if more than 64 unique comparisons are required
    constexpr uint32_t رکن_حد_بندی = 65;
    // This should hold, otherwise we do redundant work gathering terms that will never be used
    static_assert((1U << چوڑائی_MIN) >= رکن_حد_بندی, "TERM_LIMIT too big relative to 2**WIDTH_MIN");

    // Fast path exit if we surely don't need to convet anything
    اگر (nTerms < رکن_حد_بندی) واپس;

    Dfg_راس::ScopeCache scopeCache;

    // Create decoders for each srcp
    برائے (Dfg_راس* ثابت srcp : srcps) {
        ثابت Val2Terms& val2Terms = vtx2Val2Terms[srcp];

        // If not enough terms in this vertex, ignore
        اگر (val2Terms.سائز() < رکن_حد_بندی) جاری;

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

        Ast_فعال* ثابت staticActivep = [scopep]() {
            // Try to find the existing static AstActive
            برائے (Ast_گرہ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_فعال* ثابت activep = VN_CAST(nodep, فعال);
                اگر (!activep) جاری;
                اگر (activep->sentreep()->hasStatic()) واپس activep;
            }
            // If there isn't one, create a new one
            File_سطر* ثابت flp = scopep->fileline();
            Ast_Sen_درخت* ثابت stp = نیا Ast_Sen_درخت{flp, نیا Ast_Sen_شے{flp, Ast_Sen_شے::Static{}}};
            Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "", stp};
            activep->sen_درخت_Storep(stp);
            scopep->جوڑو_Blocksp(activep);
            واپس activep;
        }();
        Ast_فعال* ثابت combActivep = [scopep]() {
            // Try to find the existing combinational AstActive
            برائے (Ast_گرہ* nodep = scopep->blocksp(); nodep; nodep = nodep->nextp()) {
                Ast_فعال* ثابت activep = VN_CAST(nodep, فعال);
                اگر (!activep) جاری;
                اگر (activep->sentreep()->hasCombo()) واپس activep;
            }
            // If there isn't one, create a new one
            File_سطر* ثابت flp = scopep->fileline();
            Ast_Sen_درخت* ثابت stp = نیا Ast_Sen_درخت{flp, نیا Ast_Sen_شے{flp, Ast_Sen_شے::Combo{}}};
            Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "", stp};
            activep->sen_درخت_Storep(stp);
            scopep->جوڑو_Blocksp(activep);
            واپس activep;
        }();

        // Required data types
        ثابت Dfg_ڈیٹا_Type& idxDType = srcp->dtype();
        ثابت Dfg_ڈیٹا_Type& ثنائی_ہندسہ_DType = Dfg_ڈیٹا_Type::packed(1);
        ثابت Dfg_ڈیٹا_Type& tabDType = Dfg_ڈیٹا_Type::array(ثنائی_ہندسہ_DType, n_بٹ);

        // The index variable
        DfgVarPacked* idxVtxp = [&]() {
            // If there is an existing result variable, use that
            اگر (Dfg_راس_Var* ثابت vp = srcp->get_نتیجہ_Var()) واپس vp->as<DfgVarPacked>();
            // Otherwise create a new variable
            Dfg_راس_Var* ثابت vtxp = dfg.makeNewVar(flp, "BinToOneHot_Idx", idxDType, scopep);
            vtxp->vscp()->varp()->is_اندرونی(سچ);
            vtxp->srcp(srcp);
            واپس vtxp->as<DfgVarPacked>();
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
            واپس vscp;
        }();
        // The table variable
        DfgVarArray* ثابت tabVtxp = [&]() {
            Dfg_راس_Var* ثابت varp = dfg.makeNewVar(flp, "BinToOneHot_Tab", tabDType, scopep);
            varp->vscp()->varp()->is_اندرونی(سچ);
            varp->vscp()->varp()->noReset(سچ);
            varp->سیٹ_Has_Mod_Wr_Refs();
            واپس varp->as<DfgVarArray>();
        }();

        ++ctx.m_decodersCreated;

        // Initialize 'tab' and 'pre' variables statically
        AstInitialStatic* ثابت initp = نیا AstInitialStatic{flp, خالی_اشاریہ};
        staticActivep->جوڑو_Stmtsp(initp);
        {  // pre = 0
            initp->جوڑو_Stmtsp(نیا Ast_تفویض{
                flp,  //
                نیا AstVarRef{flp, preVscp, VAccess::WRITE},  //
                نیا AstConst{flp, AstConst::Widthed_قدر{}, static_cast<صحیح>(چوڑائی), 0}});
        }
        {  // tab.fill(0)
            AstCMethodHard* ثابت callp = نیا AstCMethodHard{
                flp, نیا AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE}, VCMethod::UNPACKED_FILL};
            callp->جوڑو_Pinsp(نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
            callp->dtype_سیٹ_Void();
            initp->جوڑو_Stmtsp(callp->makeStmt());
        }

        // Build the decoder logic
        AstAlways* ثابت logicp = نیا AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, خالی_اشاریہ, خالی_اشاریہ};
        combActivep->جوڑو_Stmtsp(logicp);
        {  // tab[pre] = 0;
            logicp->جوڑو_Stmtsp(نیا Ast_تفویض{
                flp,  //
                نیا AstArraySel{flp, نیا AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                نیا AstVarRef{flp, preVscp, VAccess::READ}},  //
                نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}});
        }
        {  // tab[idx] = 1
            AstVarRef* ثابت idxRefp = نیا AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->جوڑو_Stmtsp(نیا Ast_تفویض{
                flp,  //
                نیا AstArraySel{flp, نیا AstVarRef{flp, tabVtxp->vscp(), VAccess::WRITE},
                                idxRefp},  //
                نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}});
            DfgAstRd* ثابت astRdp = نیا DfgAstRd{dfg, idxRefp, جھوٹ, جھوٹ};
            astRdp->srcp(idxVtxp);
        }
        {  // pre = idx
            AstVarRef* ثابت idxRefp = نیا AstVarRef{flp, idxVscp, VAccess::READ};
            logicp->جوڑو_Stmtsp(نیا Ast_تفویض{flp,  //
                                            نیا AstVarRef{flp, preVscp, VAccess::WRITE},  //
                                            idxRefp});
            DfgAstRd* ثابت astRdp = نیا DfgAstRd{dfg, idxRefp, جھوٹ, جھوٹ};
            astRdp->srcp(idxVtxp);
        }

        // Replace terms with ArraySels
        برائے (ثابت خودکار& pair : val2Terms) {
            ثابت uint32_t val = pair.پہلا;
            ثابت std::سمتیہ<رکن_2>& terms = pair.دوسرا;
            // Create the ArraySel
            File_سطر* ثابت aflp = terms.front().m_vtxp->fileline();
            DfgArraySel* ثابت aselp = نیا DfgArraySel{dfg, aflp, ثنائی_ہندسہ_DType};
            aselp->fromp(tabVtxp);
            aselp->bitp(نیا DfgConst{dfg, aflp, چوڑائی, val});
            // The inverted value, if needed
            DfgNot* notp = خالی_اشاریہ;
            // Repalce the terms
            برائے (ثابت رکن_2& رکن : terms) {
                اگر (رکن.m_inv) {
                    اگر (!notp) {
                        notp = نیا DfgNot{dfg, aflp, ثنائی_ہندسہ_DType};
                        notp->srcp(aselp);
                    }
                    رکن.m_vtxp->replaceWith(notp);
                } ورنہ {
                    رکن.m_vtxp->replaceWith(aselp);
                }
                VL_DO_DANGLING(رکن.m_vtxp->unlinkDelete(dfg), رکن.m_vtxp);
            }
        }
    }
}
