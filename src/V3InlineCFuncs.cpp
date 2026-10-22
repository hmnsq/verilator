// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Inline small CFuncs into their callers
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
// V3InlineCFuncs's Transformations:
//
// Build a bipartite call graph containing function and call site vertices,
// then iterate functions leaf to root, inlining if size heuristics are met.
// Finally, remove unused functions.
//
// Two tunables control inlining:
//   --inline-cfuncs <n>         : Inline if size <= n
//   --inline-cfuncs-product <n> : Also inline if size * call_count <= n
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3InlineCFuncs.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3ExecGraph.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

తరగతి Inline_CFuncs_Function_శీర్షం;
తరగతి Inline_CFuncs_Call_Site_శీర్షం;

తరగతి Inline_CFuncs_Call_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
బహిరంగ:
    Inline_CFuncs_Call_లేఖాచిత్రం()
        : V3_లేఖాచిత్రం{} {}
    ~Inline_CFuncs_Call_లేఖాచిత్రం() override = అప్రమేయ;

    శూన్య కూడు_అంచు(Inline_CFuncs_Function_శీర్షం& from, Inline_CFuncs_Call_Site_శీర్షం& శిఖరం);
    శూన్య కూడు_అంచు(Inline_CFuncs_Call_Site_శీర్షం& from, Inline_CFuncs_Function_శీర్షం& శిఖరం);
};

తరగతి Either_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Either_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
రక్షిత:
    స్పష్టమైన Either_శీర్షం(Inline_CFuncs_Call_లేఖాచిత్రం& లేఖాచిత్రం)
        : V3_లేఖాచిత్రం_శీర్షం{&లేఖాచిత్రం} {}
};

తరగతి Inline_CFuncs_Function_శీర్షం final : బహిరంగ Either_శీర్షం {
    VL_RTTI_IMPL(Inline_CFuncs_Function_శీర్షం, Either_శీర్షం)
    AstCFunc* స్థిరరాశి m_cfuncp;  // The function
    స్థిరరాశి అక్షరం* m_noInlineWyp = శూన్యసూచిక;  // First reason the function should not be inlined
    స్థిరరాశి అక్షరం* m_keepWyp = శూన్యసూచిక;  // Why the function should not be removed
    కొలత_t m_కొలత = 0;  // The size of the function

బహిరంగ:
    Inline_CFuncs_Function_శీర్షం(Inline_CFuncs_Call_లేఖాచిత్రం& లేఖాచిత్రం, AstCFunc* cfuncp)
        : Either_శీర్షం{లేఖాచిత్రం}
        , m_cfuncp{cfuncp} {}
    ~Inline_CFuncs_Function_శీర్షం() override = అప్రమేయ;

    // ACCESSORS
    AstCFunc* cfuncp() స్థిరరాశి { తిరిగి m_cfuncp; }
    కొలత_t కొలత() స్థిరరాశి { తిరిగి m_కొలత; }
    శూన్య కొలత_Inc(కొలత_t విలువ = 1) { m_కొలత += విలువ; }
    తార్కిక noInline() స్థిరరాశి { తిరిగి m_noInlineWyp; }
    శూన్య సమితి_No_Inline(స్థిరరాశి అక్షరం* whyp) {
        ఒకవేళ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    తార్కిక keep() స్థిరరాశి { తిరిగి m_keepWyp; }
    శూన్య సమితి_Keep(స్థిరరాశి అక్షరం* whyp) {
        ఒకవేళ (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() స్థిరరాశి override {
        తిరిగి m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి m_cfuncp->fileline(); }
    std::string dotShape() స్థిరరాశి override { తిరిగి "box"; }
    std::string name() స్థిరరాశి override VL_MT_STABLE {
        std::string str = cvt_గమ్యం_Hex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::గమ్యం_string(m_కొలత);
        ఒకవేళ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ఒకవేళ (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        తిరిగి str;
    }
};

తరగతి Inline_CFuncs_Call_Site_శీర్షం final : బహిరంగ Either_శీర్షం {
    VL_RTTI_IMPL(Inline_CFuncs_Call_Site_శీర్షం, Either_శీర్షం)
    AstCCall* స్థిరరాశి m_callp;  // The call site
    స్థిరరాశి అక్షరం* m_noInlineWyp = శూన్యసూచిక;  // First reason the function should not be inlined

బహిరంగ:
    Inline_CFuncs_Call_Site_శీర్షం(Inline_CFuncs_Call_లేఖాచిత్రం& లేఖాచిత్రం, AstCCall* callp)
        : Either_శీర్షం{లేఖాచిత్రం}
        , m_callp{callp} {}
    ~Inline_CFuncs_Call_Site_శీర్షం() override = అప్రమేయ;

    // ACCESSORS
    AstCCall* callp() స్థిరరాశి { తిరిగి m_callp; }
    తార్కిక noInline() స్థిరరాశి { తిరిగి m_noInlineWyp; }
    శూన్య సమితి_No_Inline(స్థిరరాశి అక్షరం* whyp) {
        ఒకవేళ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి m_callp->fileline(); }
    std::string dotColor() స్థిరరాశి override { తిరిగి m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() స్థిరరాశి override { తిరిగి "ellipse"; }
    std::string name() స్థిరరాశి override VL_MT_STABLE {
        std::string str = cvt_గమ్యం_Hex(m_callp);
        ఒకవేళ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        తిరిగి str;
    }
};

శూన్య Inline_CFuncs_Call_లేఖాచిత్రం::కూడు_అంచు(Inline_CFuncs_Function_శీర్షం& caller,
                                    Inline_CFuncs_Call_Site_శీర్షం& callsite) {
    UASSERT_OBJ(callsite.లోపల_ఖాళీలు(), &callsite, "Call site should have at most one incoming edge");
    కొత్త V3_లేఖాచిత్రం_అంచు{ఇది, &caller, &callsite, 1, నిజం};  // Can cut caller -> callsite
}
శూన్య Inline_CFuncs_Call_లేఖాచిత్రం::కూడు_అంచు(Inline_CFuncs_Call_Site_శీర్షం& callsite,
                                    Inline_CFuncs_Function_శీర్షం& callee) {
    UASSERT_OBJ(callsite.బహిర్గతం_ఖాళీలు(), &callsite, "Call site should have at most one outgoing edge");
    కొత్త V3_లేఖాచిత్రం_అంచు{ఇది, &callsite, &callee, 1, అబద్ధం};
}

//######################################################################

తరగతి InlineCFuncsVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // STATE
    Inline_CFuncs_Call_లేఖాచిత్రం m_లేఖాచిత్రం;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    స్థిరరాశి కొలత_t m_కొలత_పరిమితి = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    స్థిరరాశి కొలత_t m_prod_పరిమితి = v3Global.opt.inline_CFuncs_లబ్ధం();
    // Maximum size of caller to consider inlining into
    స్థిరరాశి కొలత_t m_max_కొలత_CFunc = []() -> కొలత_t {
        పూర్ణాంకం maxCFunc = v3Global.opt.output_విభజించు_CFuncs();
        పూర్ణాంకం maxFile = v3Global.opt.output_విభజించు();
        ఒకవేళ (maxCFunc <= 0) maxCFunc = std::numeric_limits<పూర్ణాంకం>::max();
        ఒకవేళ (maxFile <= 0) maxFile = std::numeric_limits<పూర్ణాంకం>::max();
        తిరిగి std::min(maxCFunc, maxFile);
    }();
    స్థిరరాశి కొలత_t m_max_కొలత_Trace = []() -> కొలత_t {
        పూర్ణాంకం maxTrace = v3Global.opt.output_విభజించు_CTrace();
        పూర్ణాంకం maxFile = v3Global.opt.output_విభజించు();
        ఒకవేళ (maxTrace <= 0) maxTrace = std::numeric_limits<పూర్ణాంకం>::max();
        ఒకవేళ (maxFile <= 0) maxFile = std::numeric_limits<పూర్ణాంకం>::max();
        తిరిగి std::min(maxTrace, maxFile);
    }();
    Inline_CFuncs_Function_శీర్షం* m_cfuncVtxp = శూన్యసూచిక;  // Vertex of currently iterated function
    తార్కిక m_లోపల_Exec_లేఖాచిత్రం = అబద్ధం;  // True while inside an AstExecGraph subtree

    // METHODS
    Inline_CFuncs_Function_శీర్షం* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        ఒకవేళ (!cfuncp->user1p()) cfuncp->user1p(కొత్త Inline_CFuncs_Function_శీర్షం{m_లేఖాచిత్రం, cfuncp});
        తిరిగి cfuncp->user1u().గమ్యం<Inline_CFuncs_Function_శీర్షం*>();
    }

    Inline_CFuncs_Call_Site_శీర్షం* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        ఒకవేళ (!callp->user1p()) callp->user1p(కొత్త Inline_CFuncs_Call_Site_శీర్షం{m_లేఖాచిత్రం, callp});
        తిరిగి callp->user1u().గమ్యం<Inline_CFuncs_Call_Site_శీర్షం*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* స్థిరరాశి callerp,  //
                               AstCCall* స్థిరరాశి callp,  //
                               AstCFunc* స్థిరరాశి calleep,  //
                               స్థిరరాశి కొలత_t seq_అంకె) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_కణుపు_Stmt* స్థిరరాశి callSitep = VN_AS(callp->backp(), Stmt_సమాసం);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        ఒకవేళ (!calleep->stmtsp()) {
            VL_DO_DANGLING(నెట్టు_Deletep(callSitep->unlink_Fr_వెనుక()), callSitep);
            తిరిగి శూన్యసూచిక;
        }

        // Replace call site with a local scope
        File_పంక్తి* స్థిరరాశి flp = callSitep->fileline();
        AstCLocalScope* స్థిరరాశి lscopep = కొత్త AstCLocalScope{flp, శూన్యసూచిక};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(నెట్టు_Deletep(callSitep), callSitep);
        lscopep->కూడు_Stmtsp(కొత్త AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        స్థిరరాశి std::string var_ఉపసర్గ
            = "__Vinline_" + std::గమ్యం_string(seq_అంకె) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;

        // Clone local variables, add them to the local scope
        కొరకు (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* స్థిరరాశి newVarp = varp->clone_వృక్షం(అబద్ధం);
            newVarp->name(var_ఉపసర్గ + varp->name());
            lscopep->కూడు_Stmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_కణుపు* స్థిరరాశి bodyp = calleep->stmtsp()->clone_వృక్షం(నిజం);
        lscopep->కూడు_Stmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreach_And_తదుపరి([&](Ast_కణుపు* nodep) {
            ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
                ఒకవేళ (AstVar* స్థిరరాశి varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_కణుపు_DType* స్థిరరాశి dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } లేకపోతే ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                varp->name(var_ఉపసర్గ + varp->name());
            } లేకపోతే ఒకవేళ (AstCCall* స్థిరరాశి callp = VN_CAST(nodep, CCall)) {
                callp->user1p(శూన్యసూచిక);
            }
        });

        // Return the local scope
        తిరిగి lscopep;
    }

    శూన్య doInlining() {
        // Need to gather vertices as we are changing the graph
        std::సదిశ<Inline_CFuncs_Function_శీర్షం*> m_fVtxps;
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (Inline_CFuncs_Function_శీర్షం* స్థిరరాశి fVtxp = vtx.cast<Inline_CFuncs_Function_శీర్షం>()) {
                m_fVtxps.emplace_వెనుక(fVtxp);
            }
        }

        // Iterate functions leaf to root
        కొరకు (Inline_CFuncs_Function_శీర్షం* స్థిరరాశి calleeVtxp : vlstd::తిరగేయి_view(m_fVtxps)) {
            // Should we inline this function?
            ఒకవేళ (calleeVtxp->noInline()) కొనసాగు;  // Told not to

            // Check size heuristics
            స్థిరరాశి తార్కిక doIt = [&]() {
                // Inline if small enough
                ఒకవేళ (calleeVtxp->కొలత() <= m_కొలత_పరిమితి) తిరిగి నిజం;
                // Inline if not too much bloat
                స్థిరరాశి కొలత_t nCalls = calleeVtxp->లోపల_అంచులు().కొలత();
                ఒకవేళ (nCalls * calleeVtxp->కొలత() <= m_prod_పరిమితి) తిరిగి నిజం;
                // Otherwise don't inline
                తిరిగి అబద్ధం;
            }();
            ఒకవేళ (!doIt) కొనసాగు;

            // Ok, attempt to inline call sites
            కొలత_t nInlined = 0;
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : calleeVtxp->లోపల_అంచులు().unlinkable()) {
                Inline_CFuncs_Call_Site_శీర్షం* స్థిరరాశి callVtxp
                    = edgep->fromp()->as<Inline_CFuncs_Call_Site_శీర్షం>();

                AstCFunc* స్థిరరాశి calleep = calleeVtxp->cfuncp();
                AstCCall* స్థిరరాశి callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                ఒకవేళ (callVtxp->noInline()) కొనసాగు;  // Told not to
                ఒకవేళ (callVtxp->లోపల_ఖాళీలు()) కొనసాగు;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->లోపల_Size1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                Inline_CFuncs_Function_శీర్షం* స్థిరరాశి callerVtxp
                    = callVtxp->లోపల_అంచులు().frontp()->fromp()->as<Inline_CFuncs_Function_శీర్షం>();
                AstCFunc* స్థిరరాశి callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                స్థిరరాశి కొలత_t హద్దు = callerp->isTrace() ? m_max_కొలత_Trace : m_max_కొలత_CFunc;
                ఒకవేళ (callerVtxp->కొలత() + calleeVtxp->కొలత() > హద్దు) కొనసాగు;

                // Can't do it if it's in a different scope, self pointers differ
                ఒకవేళ (callerp->scopep() != calleep->scopep()) కొనసాగు;

                // Inline it
                ఒకవేళ (!nInlined) ++m_statFuncsInlined;
                Ast_కణుపు* స్థిరరాశి inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_లేఖాచిత్రం), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                ఒకవేళ (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    శూన్య remove_వాడని_Funcs() {
        // Iterate root to leaves
        కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_లేఖాచిత్రం.vertices().unlinkable()) {
            Inline_CFuncs_Function_శీర్షం* స్థిరరాశి fVtxp = vtxp->cast<Inline_CFuncs_Function_శీర్షం>();
            ఒకవేళ (!fVtxp) కొనసాగు;
            // Keep if still called
            ఒకవేళ (!fVtxp->లోపల_ఖాళీలు()) కొనసాగు;
            // Keep for other reasons
            ఒకవేళ (fVtxp->keep()) కొనసాగు;

            AstCFunc* స్థిరరాశి funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : vtxp->బహిర్గతం_అంచులు().unlinkable()) {
                edgep->శిఖరం()->unlink_అంచులు(&m_లేఖాచిత్రం);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_లేఖాచిత్రం);
            // Delete the function
            VL_DO_DANGLING(నెట్టు_Deletep(funcp->unlink_Fr_వెనుక()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_లేఖాచిత్రం.vertices().unlinkable()) {
            Inline_CFuncs_Call_Site_శీర్షం* స్థిరరాశి cVtxp = vtxp->cast<Inline_CFuncs_Call_Site_శీర్షం>();
            ఒకవేళ (!cVtxp) కొనసాగు;
            ఒకవేళ (!cVtxp->లోపల_ఖాళీలు()) కొనసాగు;
            ఒకవేళ (!cVtxp->బహిర్గతం_ఖాళీలు()) కొనసాగు;
            vtxp->unlinkDelete(&m_లేఖాచిత్రం);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        // Create the function vertex
        Inline_CFuncs_Function_శీర్షం* స్థిరరాశి vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        ఒకవేళ (nodep->rtnTypeVoid() != "void") vtxp->సమితి_No_Inline("Not void");
        ఒకవేళ (nodep->dpiImportPrototype()) vtxp->సమితి_No_Inline("DPI import prototype");
        ఒకవేళ (nodep->recursive()) vtxp->సమితి_No_Inline("Recursive");
        ఒకవేళ (nodep->argsp()) vtxp->సమితి_No_Inline("Has arguments");
        ఒకవేళ (nodep->isVirtual()) vtxp->సమితి_No_Inline("Virtual method");

        // Check if the function should not be removed
        ఒకవేళ (nodep->నమోదు_బిందువు()) vtxp->సమితి_Keep("Entry point");
        ఒకవేళ (nodep->dpiImportPrototype()) vtxp->సమితి_Keep("DPI import prototype");
        ఒకవేళ (nodep->dpiExportDispatcher()) vtxp->సమితి_Keep("DPI export implementation");
        ఒకవేళ (nodep->isVirtual()) vtxp->సమితి_Keep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    శూన్య సందర్శించు(AstCCall* nodep) override {
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->కొలత_Inc();
        AstCFunc* స్థిరరాశి calleep = nodep->funcp();

        // Create the call site vertex
        Inline_CFuncs_Call_Site_శీర్షం* స్థిరరాశి vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        ఒకవేళ (!VN_IS(nodep->backp(), Stmt_సమాసం)) vtxp->సమితి_No_Inline("Not in statement position");
        ఒకవేళ (m_లోపల_Exec_లేఖాచిత్రం) vtxp->సమితి_No_Inline("In ExecGraph");
        ఒకవేళ (calleep->isVirtual()) vtxp->సమితి_No_Inline("Virtual method");

        // Add caller/callee edges
        ఒకవేళ (m_cfuncVtxp) m_లేఖాచిత్రం.కూడు_అంచు(*m_cfuncVtxp, *vtxp);
        m_లేఖాచిత్రం.కూడు_అంచు(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->కొలత_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->సమితి_Keep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_Addr_యొక్క_CFunc* nodep) override {
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->కొలత_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->సమితి_Keep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    శూన్య సందర్శించు(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->సమితి_No_Inline("Contains TraceDecl");

        ఒకవేళ (AstCCall* స్థిరరాశి callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->సమితి_No_Inline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_Exec_లేఖాచిత్రం* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->సమితి_No_Inline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->సమితి_Keep("MTask function");
        }
        VL_RESTORER(m_లోపల_Exec_లేఖాచిత్రం);
        m_లోపల_Exec_లేఖాచిత్రం = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCStmt* nodep) override {
        // Can reference anything in text
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->సమితి_No_Inline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCExpr* nodep) override {
        // Can reference anything in text
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->సమితి_No_Inline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCStmtUser* nodep) override {
        // Can reference anything in text
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->సమితి_No_Inline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCExprUser* nodep) override {
        // Can reference anything in text
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->సమితి_No_Inline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCReturn* nodep) override {
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->సమితి_No_Inline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (m_cfuncVtxp) m_cfuncVtxp->కొలత_Inc();
        iterateChildrenConst(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_లేఖాచిత్రం.acyclic(V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_లేఖాచిత్రం.క్రమం();
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_లేఖాచిత్రం.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_లేఖాచిత్రం.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        remove_వాడని_Funcs();
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_లేఖాచిత్రం.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::కూడు_Stat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::కూడు_Stat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::కూడు_Stat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

శూన్య V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    ఒకవేళ (v3Global.opt.profCFuncs()) తిరిగి;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("inlinecfuncs", 0, dump_వృక్షం_Either_Level() >= 6);
}
