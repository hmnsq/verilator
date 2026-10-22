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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3InlineCFuncs.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

ವರ್ಗ Inline_CFuncs_Function_ಶೃಂಗ;
ವರ್ಗ Inline_CFuncs_Call_Site_ಶೃಂಗ;

ವರ್ಗ Inline_CFuncs_Call_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
ಸಾರ್ವಜನಿಕ:
    Inline_CFuncs_Call_ನಕ್ಷೆ()
        : V3_ನಕ್ಷೆ{} {}
    ~Inline_CFuncs_Call_ನಕ್ಷೆ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(Inline_CFuncs_Function_ಶೃಂಗ& from, Inline_CFuncs_Call_Site_ಶೃಂಗ& ತುದಿ);
    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(Inline_CFuncs_Call_Site_ಶೃಂಗ& from, Inline_CFuncs_Function_ಶೃಂಗ& ತುದಿ);
};

ವರ್ಗ Either_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Either_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
ಸಂರಕ್ಷಿತ:
    ಸ್ಪಷ್ಟ Either_ಶೃಂಗ(Inline_CFuncs_Call_ನಕ್ಷೆ& ನಕ್ಷೆ)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{&ನಕ್ಷೆ} {}
};

ವರ್ಗ Inline_CFuncs_Function_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Either_ಶೃಂಗ {
    VL_RTTI_IMPL(Inline_CFuncs_Function_ಶೃಂಗ, Either_ಶೃಂಗ)
    AstCFunc* ಸ್ಥಿರಾಂಕ m_cfuncp;  // The function
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_noInlineWyp = ಶೂನ್ಯಸೂಚಕ;  // First reason the function should not be inlined
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_keepWyp = ಶೂನ್ಯಸೂಚಕ;  // Why the function should not be removed
    ಗಾತ್ರ_t m_ಗಾತ್ರ = 0;  // The size of the function

ಸಾರ್ವಜನಿಕ:
    Inline_CFuncs_Function_ಶೃಂಗ(Inline_CFuncs_Call_ನಕ್ಷೆ& ನಕ್ಷೆ, AstCFunc* cfuncp)
        : Either_ಶೃಂಗ{ನಕ್ಷೆ}
        , m_cfuncp{cfuncp} {}
    ~Inline_CFuncs_Function_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    AstCFunc* cfuncp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cfuncp; }
    ಗಾತ್ರ_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಗಾತ್ರ; }
    ಶೂನ್ಯ ಗಾತ್ರ_Inc(ಗಾತ್ರ_t ಮೌಲ್ಯ = 1) { m_ಗಾತ್ರ += ಮೌಲ್ಯ; }
    ತಾರ್ಕಿಕ noInline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_noInlineWyp; }
    ಶೂನ್ಯ ಗಣ_No_Inline(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whyp) {
        ಆದರೆ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    ತಾರ್ಕಿಕ keep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_keepWyp; }
    ಶೂನ್ಯ ಗಣ_Keep(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whyp) {
        ಆದರೆ (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_cfuncp->fileline(); }
    std::string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "box"; }
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        std::string str = cvt_ಗಮ್ಯ_Hex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::ಗಮ್ಯ_string(m_ಗಾತ್ರ);
        ಆದರೆ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ಆದರೆ (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        ಹಿಂತಿರುಗಿ str;
    }
};

ವರ್ಗ Inline_CFuncs_Call_Site_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Either_ಶೃಂಗ {
    VL_RTTI_IMPL(Inline_CFuncs_Call_Site_ಶೃಂಗ, Either_ಶೃಂಗ)
    AstCCall* ಸ್ಥಿರಾಂಕ m_callp;  // The call site
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_noInlineWyp = ಶೂನ್ಯಸೂಚಕ;  // First reason the function should not be inlined

ಸಾರ್ವಜನಿಕ:
    Inline_CFuncs_Call_Site_ಶೃಂಗ(Inline_CFuncs_Call_ನಕ್ಷೆ& ನಕ್ಷೆ, AstCCall* callp)
        : Either_ಶೃಂಗ{ನಕ್ಷೆ}
        , m_callp{callp} {}
    ~Inline_CFuncs_Call_Site_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    AstCCall* callp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_callp; }
    ತಾರ್ಕಿಕ noInline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_noInlineWyp; }
    ಶೂನ್ಯ ಗಣ_No_Inline(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whyp) {
        ಆದರೆ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_callp->fileline(); }
    std::string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "ellipse"; }
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        std::string str = cvt_ಗಮ್ಯ_Hex(m_callp);
        ಆದರೆ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ಹಿಂತಿರುಗಿ str;
    }
};

ಶೂನ್ಯ Inline_CFuncs_Call_ನಕ್ಷೆ::ಕೂಡಿಸು_ಅಂಚು(Inline_CFuncs_Function_ಶೃಂಗ& caller,
                                    Inline_CFuncs_Call_Site_ಶೃಂಗ& callsite) {
    UASSERT_OBJ(callsite.ಒಳ_ಖಾಲಿಗಳು(), &callsite, "Call site should have at most one incoming edge");
    ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{ಇದು, &caller, &callsite, 1, ನಿಜ};  // Can cut caller -> callsite
}
ಶೂನ್ಯ Inline_CFuncs_Call_ನಕ್ಷೆ::ಕೂಡಿಸು_ಅಂಚು(Inline_CFuncs_Call_Site_ಶೃಂಗ& callsite,
                                    Inline_CFuncs_Function_ಶೃಂಗ& callee) {
    UASSERT_OBJ(callsite.ಹೊರಹೋಗು_ಖಾಲಿಗಳು(), &callsite, "Call site should have at most one outgoing edge");
    ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{ಇದು, &callsite, &callee, 1, ಸುಳ್ಳು};
}

//######################################################################

ವರ್ಗ InlineCFuncsVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // STATE
    Inline_CFuncs_Call_ನಕ್ಷೆ m_ನಕ್ಷೆ;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_ಗಾತ್ರ_ಮಿತಿ = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_prod_ಮಿತಿ = v3Global.opt.inline_CFuncs_ಗುಣಲಬ್ಧ();
    // Maximum size of caller to consider inlining into
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_max_ಗಾತ್ರ_CFunc = []() -> ಗಾತ್ರ_t {
        ಪೂರ್ಣಾಂಕ maxCFunc = v3Global.opt.output_ವಿಭಜಿಸು_CFuncs();
        ಪೂರ್ಣಾಂಕ maxFile = v3Global.opt.output_ವಿಭಜಿಸು();
        ಆದರೆ (maxCFunc <= 0) maxCFunc = std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();
        ಆದರೆ (maxFile <= 0) maxFile = std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();
        ಹಿಂತಿರುಗಿ std::min(maxCFunc, maxFile);
    }();
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_max_ಗಾತ್ರ_Trace = []() -> ಗಾತ್ರ_t {
        ಪೂರ್ಣಾಂಕ maxTrace = v3Global.opt.output_ವಿಭಜಿಸು_CTrace();
        ಪೂರ್ಣಾಂಕ maxFile = v3Global.opt.output_ವಿಭಜಿಸು();
        ಆದರೆ (maxTrace <= 0) maxTrace = std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();
        ಆದರೆ (maxFile <= 0) maxFile = std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();
        ಹಿಂತಿರುಗಿ std::min(maxTrace, maxFile);
    }();
    Inline_CFuncs_Function_ಶೃಂಗ* m_cfuncVtxp = ಶೂನ್ಯಸೂಚಕ;  // Vertex of currently iterated function
    ತಾರ್ಕಿಕ m_ಒಳ_Exec_ನಕ್ಷೆ = ಸುಳ್ಳು;  // True while inside an AstExecGraph subtree

    // METHODS
    Inline_CFuncs_Function_ಶೃಂಗ* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        ಆದರೆ (!cfuncp->user1p()) cfuncp->user1p(ಹೊಸ Inline_CFuncs_Function_ಶೃಂಗ{m_ನಕ್ಷೆ, cfuncp});
        ಹಿಂತಿರುಗಿ cfuncp->user1u().ಗಮ್ಯ<Inline_CFuncs_Function_ಶೃಂಗ*>();
    }

    Inline_CFuncs_Call_Site_ಶೃಂಗ* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        ಆದರೆ (!callp->user1p()) callp->user1p(ಹೊಸ Inline_CFuncs_Call_Site_ಶೃಂಗ{m_ನಕ್ಷೆ, callp});
        ಹಿಂತಿರುಗಿ callp->user1u().ಗಮ್ಯ<Inline_CFuncs_Call_Site_ಶೃಂಗ*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ಸ್ಥಿರಾಂಕ callerp,  //
                               AstCCall* ಸ್ಥಿರಾಂಕ callp,  //
                               AstCFunc* ಸ್ಥಿರಾಂಕ calleep,  //
                               ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t seq_ಅಂಕೆ) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ callSitep = VN_AS(callp->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        ಆದರೆ (!calleep->stmtsp()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(callSitep->unlink_Fr_ಹಿಂಭಾಗ()), callSitep);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        // Replace call site with a local scope
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = callSitep->fileline();
        AstCLocalScope* ಸ್ಥಿರಾಂಕ lscopep = ಹೊಸ AstCLocalScope{flp, ಶೂನ್ಯಸೂಚಕ};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(callSitep), callSitep);
        lscopep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ಸ್ಥಿರಾಂಕ std::string var_ಪೂರ್ವಪ್ರತ್ಯಯ
            = "__Vinline_" + std::ಗಮ್ಯ_string(seq_ಅಂಕೆ) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;

        // Clone local variables, add them to the local scope
        ಗಾಗಿ (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ಸ್ಥಿರಾಂಕ newVarp = varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            newVarp->name(var_ಪೂರ್ವಪ್ರತ್ಯಯ + varp->name());
            lscopep->ಕೂಡಿಸು_Stmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodyp = calleep->stmtsp()->clone_ವೃಕ್ಷ(ನಿಜ);
        lscopep->ಕೂಡಿಸು_Stmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreach_And_ಮುಂದಿನ([&](Ast_ಗಂಟು* nodep) {
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                varp->name(var_ಪೂರ್ವಪ್ರತ್ಯಯ + varp->name());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCCall* ಸ್ಥಿರಾಂಕ callp = VN_CAST(nodep, CCall)) {
                callp->user1p(ಶೂನ್ಯಸೂಚಕ);
            }
        });

        // Return the local scope
        ಹಿಂತಿರುಗಿ lscopep;
    }

    ಶೂನ್ಯ doInlining() {
        // Need to gather vertices as we are changing the graph
        std::ಸದಿಶ<Inline_CFuncs_Function_ಶೃಂಗ*> m_fVtxps;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (Inline_CFuncs_Function_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fVtxp = vtx.cast<Inline_CFuncs_Function_ಶೃಂಗ>()) {
                m_fVtxps.emplace_ಹಿಂಭಾಗ(fVtxp);
            }
        }

        // Iterate functions leaf to root
        ಗಾಗಿ (Inline_CFuncs_Function_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ calleeVtxp : vlstd::ಹಿಮ್ಮುಖ_view(m_fVtxps)) {
            // Should we inline this function?
            ಆದರೆ (calleeVtxp->noInline()) ಮುಂದುವರಿ;  // Told not to

            // Check size heuristics
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ doIt = [&]() {
                // Inline if small enough
                ಆದರೆ (calleeVtxp->ಗಾತ್ರ_2() <= m_ಗಾತ್ರ_ಮಿತಿ) ಹಿಂತಿರುಗಿ ನಿಜ;
                // Inline if not too much bloat
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nCalls = calleeVtxp->ಒಳ_ಅಂಚುಗಳು().ಗಾತ್ರ_2();
                ಆದರೆ (nCalls * calleeVtxp->ಗಾತ್ರ_2() <= m_prod_ಮಿತಿ) ಹಿಂತಿರುಗಿ ನಿಜ;
                // Otherwise don't inline
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }();
            ಆದರೆ (!doIt) ಮುಂದುವರಿ;

            // Ok, attempt to inline call sites
            ಗಾತ್ರ_t nInlined = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : calleeVtxp->ಒಳ_ಅಂಚುಗಳು().unlinkable()) {
                Inline_CFuncs_Call_Site_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ callVtxp
                    = edgep->fromp()->as<Inline_CFuncs_Call_Site_ಶೃಂಗ>();

                AstCFunc* ಸ್ಥಿರಾಂಕ calleep = calleeVtxp->cfuncp();
                AstCCall* ಸ್ಥಿರಾಂಕ callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                ಆದರೆ (callVtxp->noInline()) ಮುಂದುವರಿ;  // Told not to
                ಆದರೆ (callVtxp->ಒಳ_ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->ಒಳ_Size1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                Inline_CFuncs_Function_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ callerVtxp
                    = callVtxp->ಒಳ_ಅಂಚುಗಳು().frontp()->fromp()->as<Inline_CFuncs_Function_ಶೃಂಗ>();
                AstCFunc* ಸ್ಥಿರಾಂಕ callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಎಲ್ಲೆ = callerp->isTrace() ? m_max_ಗಾತ್ರ_Trace : m_max_ಗಾತ್ರ_CFunc;
                ಆದರೆ (callerVtxp->ಗಾತ್ರ_2() + calleeVtxp->ಗಾತ್ರ_2() > ಎಲ್ಲೆ) ಮುಂದುವರಿ;

                // Can't do it if it's in a different scope, self pointers differ
                ಆದರೆ (callerp->scopep() != calleep->scopep()) ಮುಂದುವರಿ;

                // Inline it
                ಆದರೆ (!nInlined) ++m_statFuncsInlined;
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_ನಕ್ಷೆ), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                ಆದರೆ (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    ಶೂನ್ಯ remove_ಬಳಸದ_Funcs() {
        // Iterate root to leaves
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_ನಕ್ಷೆ.vertices().unlinkable()) {
            Inline_CFuncs_Function_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fVtxp = vtxp->cast<Inline_CFuncs_Function_ಶೃಂಗ>();
            ಆದರೆ (!fVtxp) ಮುಂದುವರಿ;
            // Keep if still called
            ಆದರೆ (!fVtxp->ಒಳ_ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            // Keep for other reasons
            ಆದರೆ (fVtxp->keep()) ಮುಂದುವರಿ;

            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
                edgep->ತುದಿ()->unlink_ಅಂಚುಗಳು(&m_ನಕ್ಷೆ);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_ನಕ್ಷೆ);
            // Delete the function
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcp->unlink_Fr_ಹಿಂಭಾಗ()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_ನಕ್ಷೆ.vertices().unlinkable()) {
            Inline_CFuncs_Call_Site_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cVtxp = vtxp->cast<Inline_CFuncs_Call_Site_ಶೃಂಗ>();
            ಆದರೆ (!cVtxp) ಮುಂದುವರಿ;
            ಆದರೆ (!cVtxp->ಒಳ_ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            ಆದರೆ (!cVtxp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            vtxp->unlinkDelete(&m_ನಕ್ಷೆ);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        // Create the function vertex
        Inline_CFuncs_Function_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        ಆದರೆ (nodep->rtnTypeVoid() != "void") vtxp->ಗಣ_No_Inline("Not void");
        ಆದರೆ (nodep->dpiImportPrototype()) vtxp->ಗಣ_No_Inline("DPI import prototype");
        ಆದರೆ (nodep->recursive()) vtxp->ಗಣ_No_Inline("Recursive");
        ಆದರೆ (nodep->argsp()) vtxp->ಗಣ_No_Inline("Has arguments");
        ಆದರೆ (nodep->isVirtual()) vtxp->ಗಣ_No_Inline("Virtual method");

        // Check if the function should not be removed
        ಆದರೆ (nodep->ನಮೂದು_ಬಿಂದು()) vtxp->ಗಣ_Keep("Entry point");
        ಆದರೆ (nodep->dpiImportPrototype()) vtxp->ಗಣ_Keep("DPI import prototype");
        ಆದರೆ (nodep->dpiExportDispatcher()) vtxp->ಗಣ_Keep("DPI export implementation");
        ಆದರೆ (nodep->isVirtual()) vtxp->ಗಣ_Keep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    ಶೂನ್ಯ ಭೇಟಿ(AstCCall* nodep) override {
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಾತ್ರ_Inc();
        AstCFunc* ಸ್ಥಿರಾಂಕ calleep = nodep->funcp();

        // Create the call site vertex
        Inline_CFuncs_Call_Site_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        ಆದರೆ (!VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) vtxp->ಗಣ_No_Inline("Not in statement position");
        ಆದರೆ (m_ಒಳ_Exec_ನಕ್ಷೆ) vtxp->ಗಣ_No_Inline("In ExecGraph");
        ಆದರೆ (calleep->isVirtual()) vtxp->ಗಣ_No_Inline("Virtual method");

        // Add caller/callee edges
        ಆದರೆ (m_cfuncVtxp) m_ನಕ್ಷೆ.ಕೂಡಿಸು_ಅಂಚು(*m_cfuncVtxp, *vtxp);
        m_ನಕ್ಷೆ.ಕೂಡಿಸು_ಅಂಚು(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಾತ್ರ_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->ಗಣ_Keep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Addr_ದ_CFunc* nodep) override {
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಾತ್ರ_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->ಗಣ_Keep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಣ_No_Inline("Contains TraceDecl");

        ಆದರೆ (AstCCall* ಸ್ಥಿರಾಂಕ callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->ಗಣ_No_Inline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Exec_ನಕ್ಷೆ* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಣ_No_Inline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->ಗಣ_Keep("MTask function");
        }
        VL_RESTORER(m_ಒಳ_Exec_ನಕ್ಷೆ);
        m_ಒಳ_Exec_ನಕ್ಷೆ = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmt* nodep) override {
        // Can reference anything in text
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಣ_No_Inline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExpr* nodep) override {
        // Can reference anything in text
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಣ_No_Inline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmtUser* nodep) override {
        // Can reference anything in text
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಣ_No_Inline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override {
        // Can reference anything in text
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಣ_No_Inline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCReturn* nodep) override {
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಣ_No_Inline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (m_cfuncVtxp) m_cfuncVtxp->ಗಾತ್ರ_Inc();
        iterateChildrenConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_ನಕ್ಷೆ.acyclic(V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_ನಕ್ಷೆ.ಕ್ರಮ();
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_ನಕ್ಷೆ.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_ನಕ್ಷೆ.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        remove_ಬಳಸದ_Funcs();
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_ನಕ್ಷೆ.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

ಶೂನ್ಯ V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    ಆದರೆ (v3Global.opt.profCFuncs()) ಹಿಂತಿರುಗಿ;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("inlinecfuncs", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
