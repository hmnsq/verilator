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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3InlineCFuncs.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ExecGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

ਵਰਗ Inline_CFuncs_Function_ਸ਼ੀਰਸ਼;
ਵਰਗ Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼;

ਵਰਗ Inline_CFuncs_Call_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
ਜਨਤਕ:
    Inline_CFuncs_Call_ਗ੍ਰਾਫ()
        : V3_ਗ੍ਰਾਫ{} {}
    ~Inline_CFuncs_Call_ਗ੍ਰਾਫ() override = ਮੂਲ;

    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(Inline_CFuncs_Function_ਸ਼ੀਰਸ਼& from, Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼& ਸਿਖਰ);
    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼& from, Inline_CFuncs_Function_ਸ਼ੀਰਸ਼& ਸਿਖਰ);
};

ਵਰਗ Either_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Either_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
ਸੁਰੱਖਿਅਤ:
    ਸਪਸ਼ਟ Either_ਸ਼ੀਰਸ਼(Inline_CFuncs_Call_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{&ਗ੍ਰਾਫ} {}
};

ਵਰਗ Inline_CFuncs_Function_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Either_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Inline_CFuncs_Function_ਸ਼ੀਰਸ਼, Either_ਸ਼ੀਰਸ਼)
    AstCFunc* ਅਚਲ m_cfuncp;  // The function
    ਅਚਲ ਅੱਖਰ* m_noInlineWyp = ਖਾਲੀ_ਸੂਚਕ;  // First reason the function should not be inlined
    ਅਚਲ ਅੱਖਰ* m_keepWyp = ਖਾਲੀ_ਸੂਚਕ;  // Why the function should not be removed
    ਮਾਪ_t m_ਮਾਪ = 0;  // The size of the function

ਜਨਤਕ:
    Inline_CFuncs_Function_ਸ਼ੀਰਸ਼(Inline_CFuncs_Call_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, AstCFunc* cfuncp)
        : Either_ਸ਼ੀਰਸ਼{ਗ੍ਰਾਫ}
        , m_cfuncp{cfuncp} {}
    ~Inline_CFuncs_Function_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // ACCESSORS
    AstCFunc* cfuncp() ਅਚਲ { ਵਾਪਸ m_cfuncp; }
    ਮਾਪ_t ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_ਮਾਪ; }
    ਖਾਲੀ ਮਾਪ_Inc(ਮਾਪ_t ਮੁੱਲ = 1) { m_ਮਾਪ += ਮੁੱਲ; }
    ਤਾਰਕਿਕ noInline() ਅਚਲ { ਵਾਪਸ m_noInlineWyp; }
    ਖਾਲੀ ਸੈੱਟ_No_Inline(ਅਚਲ ਅੱਖਰ* whyp) {
        ਜੇ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    ਤਾਰਕਿਕ keep() ਅਚਲ { ਵਾਪਸ m_keepWyp; }
    ਖਾਲੀ ਸੈੱਟ_Keep(ਅਚਲ ਅੱਖਰ* whyp) {
        ਜੇ (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ਅਚਲ override {
        ਵਾਪਸ m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ m_cfuncp->fileline(); }
    std::string dotShape() ਅਚਲ override { ਵਾਪਸ "box"; }
    std::string name() ਅਚਲ override VL_MT_STABLE {
        std::string str = cvt_ਮੰਜ਼ਿਲ_Hex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::ਮੰਜ਼ਿਲ_string(m_ਮਾਪ);
        ਜੇ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ਜੇ (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        ਵਾਪਸ str;
    }
};

ਵਰਗ Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Either_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼, Either_ਸ਼ੀਰਸ਼)
    AstCCall* ਅਚਲ m_callp;  // The call site
    ਅਚਲ ਅੱਖਰ* m_noInlineWyp = ਖਾਲੀ_ਸੂਚਕ;  // First reason the function should not be inlined

ਜਨਤਕ:
    Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼(Inline_CFuncs_Call_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, AstCCall* callp)
        : Either_ਸ਼ੀਰਸ਼{ਗ੍ਰਾਫ}
        , m_callp{callp} {}
    ~Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // ACCESSORS
    AstCCall* callp() ਅਚਲ { ਵਾਪਸ m_callp; }
    ਤਾਰਕਿਕ noInline() ਅਚਲ { ਵਾਪਸ m_noInlineWyp; }
    ਖਾਲੀ ਸੈੱਟ_No_Inline(ਅਚਲ ਅੱਖਰ* whyp) {
        ਜੇ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ m_callp->fileline(); }
    std::string dotColor() ਅਚਲ override { ਵਾਪਸ m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ਅਚਲ override { ਵਾਪਸ "ellipse"; }
    std::string name() ਅਚਲ override VL_MT_STABLE {
        std::string str = cvt_ਮੰਜ਼ਿਲ_Hex(m_callp);
        ਜੇ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ਵਾਪਸ str;
    }
};

ਖਾਲੀ Inline_CFuncs_Call_ਗ੍ਰਾਫ::ਜੋੜੋ_ਕਿਨਾਰਾ(Inline_CFuncs_Function_ਸ਼ੀਰਸ਼& caller,
                                    Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼& callsite) {
    UASSERT_OBJ(callsite.ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ(), &callsite, "Call site should have at most one incoming edge");
    ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{ਇਹ, &caller, &callsite, 1, ਸੱਚ};  // Can cut caller -> callsite
}
ਖਾਲੀ Inline_CFuncs_Call_ਗ੍ਰਾਫ::ਜੋੜੋ_ਕਿਨਾਰਾ(Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼& callsite,
                                    Inline_CFuncs_Function_ਸ਼ੀਰਸ਼& callee) {
    UASSERT_OBJ(callsite.ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ(), &callsite, "Call site should have at most one outgoing edge");
    ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{ਇਹ, &callsite, &callee, 1, ਝੂਠ};
}

//######################################################################

ਵਰਗ InlineCFuncsVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // STATE
    Inline_CFuncs_Call_ਗ੍ਰਾਫ m_ਗ੍ਰਾਫ;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ਅਚਲ ਮਾਪ_t m_ਮਾਪ_ਹੱਦ = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ਅਚਲ ਮਾਪ_t m_prod_ਹੱਦ = v3Global.opt.inline_CFuncs_ਗੁਣਨਫਲ();
    // Maximum size of caller to consider inlining into
    ਅਚਲ ਮਾਪ_t m_max_ਮਾਪ_CFunc = []() -> ਮਾਪ_t {
        ਪੂਰਨ maxCFunc = v3Global.opt.output_ਵੰਡੋ_CFuncs();
        ਪੂਰਨ maxFile = v3Global.opt.output_ਵੰਡੋ();
        ਜੇ (maxCFunc <= 0) maxCFunc = std::numeric_limits<ਪੂਰਨ>::max();
        ਜੇ (maxFile <= 0) maxFile = std::numeric_limits<ਪੂਰਨ>::max();
        ਵਾਪਸ std::min(maxCFunc, maxFile);
    }();
    ਅਚਲ ਮਾਪ_t m_max_ਮਾਪ_Trace = []() -> ਮਾਪ_t {
        ਪੂਰਨ maxTrace = v3Global.opt.output_ਵੰਡੋ_CTrace();
        ਪੂਰਨ maxFile = v3Global.opt.output_ਵੰਡੋ();
        ਜੇ (maxTrace <= 0) maxTrace = std::numeric_limits<ਪੂਰਨ>::max();
        ਜੇ (maxFile <= 0) maxFile = std::numeric_limits<ਪੂਰਨ>::max();
        ਵਾਪਸ std::min(maxTrace, maxFile);
    }();
    Inline_CFuncs_Function_ਸ਼ੀਰਸ਼* m_cfuncVtxp = ਖਾਲੀ_ਸੂਚਕ;  // Vertex of currently iterated function
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Exec_ਗ੍ਰਾਫ = ਝੂਠ;  // True while inside an AstExecGraph subtree

    // METHODS
    Inline_CFuncs_Function_ਸ਼ੀਰਸ਼* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        ਜੇ (!cfuncp->user1p()) cfuncp->user1p(ਨਵਾਂ Inline_CFuncs_Function_ਸ਼ੀਰਸ਼{m_ਗ੍ਰਾਫ, cfuncp});
        ਵਾਪਸ cfuncp->user1u().ਮੰਜ਼ਿਲ<Inline_CFuncs_Function_ਸ਼ੀਰਸ਼*>();
    }

    Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        ਜੇ (!callp->user1p()) callp->user1p(ਨਵਾਂ Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼{m_ਗ੍ਰਾਫ, callp});
        ਵਾਪਸ callp->user1u().ਮੰਜ਼ਿਲ<Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ਅਚਲ callerp,  //
                               AstCCall* ਅਚਲ callp,  //
                               AstCFunc* ਅਚਲ calleep,  //
                               ਅਚਲ ਮਾਪ_t seq_ਅੰਕ) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_ਗੰਢ_Stmt* ਅਚਲ callSitep = VN_AS(callp->backp(), Stmt_ਵਿਅੰਜਕ);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        ਜੇ (!calleep->stmtsp()) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(callSitep->unlink_Fr_ਪਿੱਠ()), callSitep);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        // Replace call site with a local scope
        File_ਸਤਰ* ਅਚਲ flp = callSitep->fileline();
        AstCLocalScope* ਅਚਲ lscopep = ਨਵਾਂ AstCLocalScope{flp, ਖਾਲੀ_ਸੂਚਕ};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(callSitep), callSitep);
        lscopep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ਅਚਲ std::string var_ਅਗੇਤਰ
            = "__Vinline_" + std::ਮੰਜ਼ਿਲ_string(seq_ਅੰਕ) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;

        // Clone local variables, add them to the local scope
        ਲਈ (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ਅਚਲ newVarp = varp->clone_ਰੁੱਖ(ਝੂਠ);
            newVarp->name(var_ਅਗੇਤਰ + varp->name());
            lscopep->ਜੋੜੋ_Stmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_ਗੰਢ* ਅਚਲ bodyp = calleep->stmtsp()->clone_ਰੁੱਖ(ਸੱਚ);
        lscopep->ਜੋੜੋ_Stmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreach_And_ਅਗਲਾ([&](Ast_ਗੰਢ* nodep) {
            ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
                ਜੇ (AstVar* ਅਚਲ varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                varp->name(var_ਅਗੇਤਰ + varp->name());
            } ਨਹੀਂਤਾਂ ਜੇ (AstCCall* ਅਚਲ callp = VN_CAST(nodep, CCall)) {
                callp->user1p(ਖਾਲੀ_ਸੂਚਕ);
            }
        });

        // Return the local scope
        ਵਾਪਸ lscopep;
    }

    ਖਾਲੀ doInlining() {
        // Need to gather vertices as we are changing the graph
        std::ਸਦਿਸ਼<Inline_CFuncs_Function_ਸ਼ੀਰਸ਼*> m_fVtxps;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (Inline_CFuncs_Function_ਸ਼ੀਰਸ਼* ਅਚਲ fVtxp = vtx.cast<Inline_CFuncs_Function_ਸ਼ੀਰਸ਼>()) {
                m_fVtxps.emplace_ਪਿੱਠ(fVtxp);
            }
        }

        // Iterate functions leaf to root
        ਲਈ (Inline_CFuncs_Function_ਸ਼ੀਰਸ਼* ਅਚਲ calleeVtxp : vlstd::ਉਲਟ_view(m_fVtxps)) {
            // Should we inline this function?
            ਜੇ (calleeVtxp->noInline()) ਜਾਰੀ;  // Told not to

            // Check size heuristics
            ਅਚਲ ਤਾਰਕਿਕ doIt = [&]() {
                // Inline if small enough
                ਜੇ (calleeVtxp->ਮਾਪ() <= m_ਮਾਪ_ਹੱਦ) ਵਾਪਸ ਸੱਚ;
                // Inline if not too much bloat
                ਅਚਲ ਮਾਪ_t nCalls = calleeVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ().ਮਾਪ();
                ਜੇ (nCalls * calleeVtxp->ਮਾਪ() <= m_prod_ਹੱਦ) ਵਾਪਸ ਸੱਚ;
                // Otherwise don't inline
                ਵਾਪਸ ਝੂਠ;
            }();
            ਜੇ (!doIt) ਜਾਰੀ;

            // Ok, attempt to inline call sites
            ਮਾਪ_t nInlined = 0;
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : calleeVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ().unlinkable()) {
                Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼* ਅਚਲ callVtxp
                    = edgep->fromp()->as<Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼>();

                AstCFunc* ਅਚਲ calleep = calleeVtxp->cfuncp();
                AstCCall* ਅਚਲ callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                ਜੇ (callVtxp->noInline()) ਜਾਰੀ;  // Told not to
                ਜੇ (callVtxp->ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->ਵਿੱਚ_Size1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                Inline_CFuncs_Function_ਸ਼ੀਰਸ਼* ਅਚਲ callerVtxp
                    = callVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ().frontp()->fromp()->as<Inline_CFuncs_Function_ਸ਼ੀਰਸ਼>();
                AstCFunc* ਅਚਲ callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ਅਚਲ ਮਾਪ_t ਹੱਦਬੰਦੀ = callerp->isTrace() ? m_max_ਮਾਪ_Trace : m_max_ਮਾਪ_CFunc;
                ਜੇ (callerVtxp->ਮਾਪ() + calleeVtxp->ਮਾਪ() > ਹੱਦਬੰਦੀ) ਜਾਰੀ;

                // Can't do it if it's in a different scope, self pointers differ
                ਜੇ (callerp->scopep() != calleep->scopep()) ਜਾਰੀ;

                // Inline it
                ਜੇ (!nInlined) ++m_statFuncsInlined;
                Ast_ਗੰਢ* ਅਚਲ inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_ਗ੍ਰਾਫ), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                ਜੇ (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    ਖਾਲੀ remove_ਅਣਵਰਤਿਆ_Funcs() {
        // Iterate root to leaves
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_ਗ੍ਰਾਫ.vertices().unlinkable()) {
            Inline_CFuncs_Function_ਸ਼ੀਰਸ਼* ਅਚਲ fVtxp = vtxp->cast<Inline_CFuncs_Function_ਸ਼ੀਰਸ਼>();
            ਜੇ (!fVtxp) ਜਾਰੀ;
            // Keep if still called
            ਜੇ (!fVtxp->ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            // Keep for other reasons
            ਜੇ (fVtxp->keep()) ਜਾਰੀ;

            AstCFunc* ਅਚਲ funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
                edgep->ਸਿਖਰ()->unlink_ਕਿਨਾਰੇ(&m_ਗ੍ਰਾਫ);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_ਗ੍ਰਾਫ);
            // Delete the function
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(funcp->unlink_Fr_ਪਿੱਠ()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_ਗ੍ਰਾਫ.vertices().unlinkable()) {
            Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼* ਅਚਲ cVtxp = vtxp->cast<Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼>();
            ਜੇ (!cVtxp) ਜਾਰੀ;
            ਜੇ (!cVtxp->ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            ਜੇ (!cVtxp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            vtxp->unlinkDelete(&m_ਗ੍ਰਾਫ);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        // Create the function vertex
        Inline_CFuncs_Function_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        ਜੇ (nodep->rtnTypeVoid() != "void") vtxp->ਸੈੱਟ_No_Inline("Not void");
        ਜੇ (nodep->dpiImportPrototype()) vtxp->ਸੈੱਟ_No_Inline("DPI import prototype");
        ਜੇ (nodep->recursive()) vtxp->ਸੈੱਟ_No_Inline("Recursive");
        ਜੇ (nodep->argsp()) vtxp->ਸੈੱਟ_No_Inline("Has arguments");
        ਜੇ (nodep->isVirtual()) vtxp->ਸੈੱਟ_No_Inline("Virtual method");

        // Check if the function should not be removed
        ਜੇ (nodep->ਇੰਦਰਾਜ_ਬਿੰਦੂ()) vtxp->ਸੈੱਟ_Keep("Entry point");
        ਜੇ (nodep->dpiImportPrototype()) vtxp->ਸੈੱਟ_Keep("DPI import prototype");
        ਜੇ (nodep->dpiExportDispatcher()) vtxp->ਸੈੱਟ_Keep("DPI export implementation");
        ਜੇ (nodep->isVirtual()) vtxp->ਸੈੱਟ_Keep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    ਖਾਲੀ ਦੇਖੋ(AstCCall* nodep) override {
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਮਾਪ_Inc();
        AstCFunc* ਅਚਲ calleep = nodep->funcp();

        // Create the call site vertex
        Inline_CFuncs_Call_Site_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        ਜੇ (!VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)) vtxp->ਸੈੱਟ_No_Inline("Not in statement position");
        ਜੇ (m_ਵਿੱਚ_Exec_ਗ੍ਰਾਫ) vtxp->ਸੈੱਟ_No_Inline("In ExecGraph");
        ਜੇ (calleep->isVirtual()) vtxp->ਸੈੱਟ_No_Inline("Virtual method");

        // Add caller/callee edges
        ਜੇ (m_cfuncVtxp) m_ਗ੍ਰਾਫ.ਜੋੜੋ_ਕਿਨਾਰਾ(*m_cfuncVtxp, *vtxp);
        m_ਗ੍ਰਾਫ.ਜੋੜੋ_ਕਿਨਾਰਾ(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਮਾਪ_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->ਸੈੱਟ_Keep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Addr_ਦਾ_CFunc* nodep) override {
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਮਾਪ_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->ਸੈੱਟ_Keep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    ਖਾਲੀ ਦੇਖੋ(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਸੈੱਟ_No_Inline("Contains TraceDecl");

        ਜੇ (AstCCall* ਅਚਲ callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->ਸੈੱਟ_No_Inline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Exec_ਗ੍ਰਾਫ* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਸੈੱਟ_No_Inline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->ਸੈੱਟ_Keep("MTask function");
        }
        VL_RESTORER(m_ਵਿੱਚ_Exec_ਗ੍ਰਾਫ);
        m_ਵਿੱਚ_Exec_ਗ੍ਰਾਫ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmt* nodep) override {
        // Can reference anything in text
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਸੈੱਟ_No_Inline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExpr* nodep) override {
        // Can reference anything in text
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਸੈੱਟ_No_Inline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmtUser* nodep) override {
        // Can reference anything in text
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਸੈੱਟ_No_Inline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override {
        // Can reference anything in text
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਸੈੱਟ_No_Inline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCReturn* nodep) override {
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਸੈੱਟ_No_Inline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (m_cfuncVtxp) m_cfuncVtxp->ਮਾਪ_Inc();
        iterateChildrenConst(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_ਗ੍ਰਾਫ.acyclic(V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_ਗ੍ਰਾਫ.ਕ੍ਰਮ();
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        remove_ਅਣਵਰਤਿਆ_Funcs();
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

ਖਾਲੀ V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    ਜੇ (v3Global.opt.profCFuncs()) ਵਾਪਸ;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("inlinecfuncs", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
