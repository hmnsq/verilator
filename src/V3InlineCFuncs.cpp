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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3InlineCFuncs.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3ExecGraph.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

صنف Inline_CFuncs_Function_راس;
صنف Inline_CFuncs_Call_Site_راس;

صنف Inline_CFuncs_Call_گراف final : عام V3_گراف {
عام:
    Inline_CFuncs_Call_گراف()
        : V3_گراف{} {}
    ~Inline_CFuncs_Call_گراف() override = افتراضي;

    فراغ جوڑو_کنارا(Inline_CFuncs_Function_راس& from, Inline_CFuncs_Call_Site_راس& چوٹی);
    فراغ جوڑو_کنارا(Inline_CFuncs_Call_Site_راس& from, Inline_CFuncs_Function_راس& چوٹی);
};

صنف Either_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(Either_راس, V3_گراف_راس)
محمي:
    واضح Either_راس(Inline_CFuncs_Call_گراف& گراف)
        : V3_گراف_راس{&گراف} {}
};

صنف Inline_CFuncs_Function_راس final : عام Either_راس {
    VL_RTTI_IMPL(Inline_CFuncs_Function_راس, Either_راس)
    AstCFunc* ثابت m_cfuncp;  // The function
    ثابت حرف* m_noInlineWyp = خالی_سوچک;  // First reason the function should not be inlined
    ثابت حرف* m_keepWyp = خالی_سوچک;  // Why the function should not be removed
    ناپ_t m_ناپ = 0;  // The size of the function

عام:
    Inline_CFuncs_Function_راس(Inline_CFuncs_Call_گراف& گراف, AstCFunc* cfuncp)
        : Either_راس{گراف}
        , m_cfuncp{cfuncp} {}
    ~Inline_CFuncs_Function_راس() override = افتراضي;

    // ACCESSORS
    AstCFunc* cfuncp() ثابت { أرجع m_cfuncp; }
    ناپ_t ناپ() ثابت { أرجع m_ناپ; }
    فراغ ناپ_Inc(ناپ_t قدر = 1) { m_ناپ += قدر; }
    منطقي noInline() ثابت { أرجع m_noInlineWyp; }
    فراغ سیٹ_No_Inline(ثابت حرف* whyp) {
        إذا (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    منطقي keep() ثابت { أرجع m_keepWyp; }
    فراغ سیٹ_Keep(ثابت حرف* whyp) {
        إذا (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ثابت override {
        أرجع m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_سطر* fileline() ثابت override { أرجع m_cfuncp->fileline(); }
    std::string dotShape() ثابت override { أرجع "box"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvt_منزل_Hex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::منزل_string(m_ناپ);
        إذا (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        إذا (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        أرجع str;
    }
};

صنف Inline_CFuncs_Call_Site_راس final : عام Either_راس {
    VL_RTTI_IMPL(Inline_CFuncs_Call_Site_راس, Either_راس)
    AstCCall* ثابت m_callp;  // The call site
    ثابت حرف* m_noInlineWyp = خالی_سوچک;  // First reason the function should not be inlined

عام:
    Inline_CFuncs_Call_Site_راس(Inline_CFuncs_Call_گراف& گراف, AstCCall* callp)
        : Either_راس{گراف}
        , m_callp{callp} {}
    ~Inline_CFuncs_Call_Site_راس() override = افتراضي;

    // ACCESSORS
    AstCCall* callp() ثابت { أرجع m_callp; }
    منطقي noInline() ثابت { أرجع m_noInlineWyp; }
    فراغ سیٹ_No_Inline(ثابت حرف* whyp) {
        إذا (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_سطر* fileline() ثابت override { أرجع m_callp->fileline(); }
    std::string dotColor() ثابت override { أرجع m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ثابت override { أرجع "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvt_منزل_Hex(m_callp);
        إذا (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        أرجع str;
    }
};

فراغ Inline_CFuncs_Call_گراف::جوڑو_کنارا(Inline_CFuncs_Function_راس& caller,
                                    Inline_CFuncs_Call_Site_راس& callsite) {
    UASSERT_OBJ(callsite.وچ_خالی_تھاواں(), &callsite, "Call site should have at most one incoming edge");
    جديد V3_گراف_کنارا{هذا, &caller, &callsite, 1, سچ};  // Can cut caller -> callsite
}
فراغ Inline_CFuncs_Call_گراف::جوڑو_کنارا(Inline_CFuncs_Call_Site_راس& callsite,
                                    Inline_CFuncs_Function_راس& callee) {
    UASSERT_OBJ(callsite.باہرلا_خالی_تھاواں(), &callsite, "Call site should have at most one outgoing edge");
    جديد V3_گراف_کنارا{هذا, &callsite, &callee, 1, خطأ};
}

//######################################################################

صنف InlineCFuncsVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ثابت VNUser1_وچ_Use m_user1_وچ_Use;

    // STATE
    Inline_CFuncs_Call_گراف m_گراف;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ثابت ناپ_t m_ناپ_حد = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ثابت ناپ_t m_prod_حد = v3Global.opt.inline_CFuncs_ضرب_حاصل();
    // Maximum size of caller to consider inlining into
    ثابت ناپ_t m_max_ناپ_CFunc = []() -> ناپ_t {
        صحيح maxCFunc = v3Global.opt.output_ونڈو_CFuncs();
        صحيح maxFile = v3Global.opt.output_ونڈو();
        إذا (maxCFunc <= 0) maxCFunc = std::numeric_limits<صحيح>::max();
        إذا (maxFile <= 0) maxFile = std::numeric_limits<صحيح>::max();
        أرجع std::min(maxCFunc, maxFile);
    }();
    ثابت ناپ_t m_max_ناپ_Trace = []() -> ناپ_t {
        صحيح maxTrace = v3Global.opt.output_ونڈو_CTrace();
        صحيح maxFile = v3Global.opt.output_ونڈو();
        إذا (maxTrace <= 0) maxTrace = std::numeric_limits<صحيح>::max();
        إذا (maxFile <= 0) maxFile = std::numeric_limits<صحيح>::max();
        أرجع std::min(maxTrace, maxFile);
    }();
    Inline_CFuncs_Function_راس* m_cfuncVtxp = خالی_سوچک;  // Vertex of currently iterated function
    منطقي m_وچ_Exec_گراف = خطأ;  // True while inside an AstExecGraph subtree

    // METHODS
    Inline_CFuncs_Function_راس* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        إذا (!cfuncp->user1p()) cfuncp->user1p(جديد Inline_CFuncs_Function_راس{m_گراف, cfuncp});
        أرجع cfuncp->user1u().منزل<Inline_CFuncs_Function_راس*>();
    }

    Inline_CFuncs_Call_Site_راس* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        إذا (!callp->user1p()) callp->user1p(جديد Inline_CFuncs_Call_Site_راس{m_گراف, callp});
        أرجع callp->user1u().منزل<Inline_CFuncs_Call_Site_راس*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ثابت callerp,  //
                               AstCCall* ثابت callp,  //
                               AstCFunc* ثابت calleep,  //
                               ثابت ناپ_t seq_عدد) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_گرہ_Stmt* ثابت callSitep = VN_AS(callp->backp(), Stmt_اظہاریہ);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        إذا (!calleep->stmtsp()) {
            VL_DO_DANGLING(ڈالو_Deletep(callSitep->unlink_Fr_پٹھ()), callSitep);
            أرجع خالی_سوچک;
        }

        // Replace call site with a local scope
        File_سطر* ثابت flp = callSitep->fileline();
        AstCLocalScope* ثابت lscopep = جديد AstCLocalScope{flp, خالی_سوچک};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ڈالو_Deletep(callSitep), callSitep);
        lscopep->جوڑو_Stmtsp(جديد AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ثابت std::string var_اگیتر
            = "__Vinline_" + std::منزل_string(seq_عدد) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ثابت VNUser2_وچ_Use user2_وچ_Use;

        // Clone local variables, add them to the local scope
        لكل (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ثابت newVarp = varp->clone_درخت(خطأ);
            newVarp->name(var_اگیتر + varp->name());
            lscopep->جوڑو_Stmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_گرہ* ثابت bodyp = calleep->stmtsp()->clone_درخت(سچ);
        lscopep->جوڑو_Stmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreach_And_اگلا([&](Ast_گرہ* nodep) {
            إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                إذا (AstVar* ثابت varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_گرہ_DType* ثابت dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } وإلا إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                varp->name(var_اگیتر + varp->name());
            } وإلا إذا (AstCCall* ثابت callp = VN_CAST(nodep, CCall)) {
                callp->user1p(خالی_سوچک);
            }
        });

        // Return the local scope
        أرجع lscopep;
    }

    فراغ doInlining() {
        // Need to gather vertices as we are changing the graph
        std::سمتیہ<Inline_CFuncs_Function_راس*> m_fVtxps;
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (Inline_CFuncs_Function_راس* ثابت fVtxp = vtx.cast<Inline_CFuncs_Function_راس>()) {
                m_fVtxps.emplace_پٹھ(fVtxp);
            }
        }

        // Iterate functions leaf to root
        لكل (Inline_CFuncs_Function_راس* ثابت calleeVtxp : vlstd::الٹ_view(m_fVtxps)) {
            // Should we inline this function?
            إذا (calleeVtxp->noInline()) استمر;  // Told not to

            // Check size heuristics
            ثابت منطقي doIt = [&]() {
                // Inline if small enough
                إذا (calleeVtxp->ناپ() <= m_ناپ_حد) أرجع سچ;
                // Inline if not too much bloat
                ثابت ناپ_t nCalls = calleeVtxp->وچ_کنارے().ناپ();
                إذا (nCalls * calleeVtxp->ناپ() <= m_prod_حد) أرجع سچ;
                // Otherwise don't inline
                أرجع خطأ;
            }();
            إذا (!doIt) استمر;

            // Ok, attempt to inline call sites
            ناپ_t nInlined = 0;
            لكل (ثابت V3_گراف_کنارا* ثابت edgep : calleeVtxp->وچ_کنارے().unlinkable()) {
                Inline_CFuncs_Call_Site_راس* ثابت callVtxp
                    = edgep->fromp()->as<Inline_CFuncs_Call_Site_راس>();

                AstCFunc* ثابت calleep = calleeVtxp->cfuncp();
                AstCCall* ثابت callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                إذا (callVtxp->noInline()) استمر;  // Told not to
                إذا (callVtxp->وچ_خالی_تھاواں()) استمر;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->وچ_Size1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                Inline_CFuncs_Function_راس* ثابت callerVtxp
                    = callVtxp->وچ_کنارے().frontp()->fromp()->as<Inline_CFuncs_Function_راس>();
                AstCFunc* ثابت callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ثابت ناپ_t حد_بندی = callerp->isTrace() ? m_max_ناپ_Trace : m_max_ناپ_CFunc;
                إذا (callerVtxp->ناپ() + calleeVtxp->ناپ() > حد_بندی) استمر;

                // Can't do it if it's in a different scope, self pointers differ
                إذا (callerp->scopep() != calleep->scopep()) استمر;

                // Inline it
                إذا (!nInlined) ++m_statFuncsInlined;
                Ast_گرہ* ثابت inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_گراف), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                إذا (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    فراغ remove_بن_ورتیا_Funcs() {
        // Iterate root to leaves
        لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            Inline_CFuncs_Function_راس* ثابت fVtxp = vtxp->cast<Inline_CFuncs_Function_راس>();
            إذا (!fVtxp) استمر;
            // Keep if still called
            إذا (!fVtxp->وچ_خالی_تھاواں()) استمر;
            // Keep for other reasons
            إذا (fVtxp->keep()) استمر;

            AstCFunc* ثابت funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            لكل (ثابت V3_گراف_کنارا* ثابت edgep : vtxp->باہرلا_کنارے().unlinkable()) {
                edgep->چوٹی()->unlink_کنارے(&m_گراف);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_گراف);
            // Delete the function
            VL_DO_DANGLING(ڈالو_Deletep(funcp->unlink_Fr_پٹھ()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            Inline_CFuncs_Call_Site_راس* ثابت cVtxp = vtxp->cast<Inline_CFuncs_Call_Site_راس>();
            إذا (!cVtxp) استمر;
            إذا (!cVtxp->وچ_خالی_تھاواں()) استمر;
            إذا (!cVtxp->باہرلا_خالی_تھاواں()) استمر;
            vtxp->unlinkDelete(&m_گراف);
        }
    }

    // VISITORS
    فراغ ویکھو(AstCFunc* nodep) override {
        // Create the function vertex
        Inline_CFuncs_Function_راس* ثابت vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        إذا (nodep->rtnTypeVoid() != "void") vtxp->سیٹ_No_Inline("Not void");
        إذا (nodep->dpiImportPrototype()) vtxp->سیٹ_No_Inline("DPI import prototype");
        إذا (nodep->recursive()) vtxp->سیٹ_No_Inline("Recursive");
        إذا (nodep->argsp()) vtxp->سیٹ_No_Inline("Has arguments");
        إذا (nodep->isVirtual()) vtxp->سیٹ_No_Inline("Virtual method");

        // Check if the function should not be removed
        إذا (nodep->اندراج_نقطہ()) vtxp->سیٹ_Keep("Entry point");
        إذا (nodep->dpiImportPrototype()) vtxp->سیٹ_Keep("DPI import prototype");
        إذا (nodep->dpiExportDispatcher()) vtxp->سیٹ_Keep("DPI export implementation");
        إذا (nodep->isVirtual()) vtxp->سیٹ_Keep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    فراغ ویکھو(AstCCall* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->ناپ_Inc();
        AstCFunc* ثابت calleep = nodep->funcp();

        // Create the call site vertex
        Inline_CFuncs_Call_Site_راس* ثابت vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        إذا (!VN_IS(nodep->backp(), Stmt_اظہاریہ)) vtxp->سیٹ_No_Inline("Not in statement position");
        إذا (m_وچ_Exec_گراف) vtxp->سیٹ_No_Inline("In ExecGraph");
        إذا (calleep->isVirtual()) vtxp->سیٹ_No_Inline("Virtual method");

        // Add caller/callee edges
        إذا (m_cfuncVtxp) m_گراف.جوڑو_کنارا(*m_cfuncVtxp, *vtxp);
        m_گراف.جوڑو_کنارا(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_گرہ_CCall* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->ناپ_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->سیٹ_Keep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_Addr_دا_CFunc* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->ناپ_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->سیٹ_Keep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    فراغ ویکھو(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        إذا (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains TraceDecl");

        إذا (AstCCall* ثابت callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->سیٹ_No_Inline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_Exec_گراف* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        إذا (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        لكل (ثابت V3_گراف_راس& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->سیٹ_Keep("MTask function");
        }
        VL_RESTORER(m_وچ_Exec_گراف);
        m_وچ_Exec_گراف = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstCStmt* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstCExpr* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstCStmtUser* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstCExprUser* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstCReturn* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    فراغ ویکھو(Ast_گرہ* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->ناپ_Inc();
        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_گراف.acyclic(V3_گراف_کنارا::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_گراف.ترتیب();
        إذا (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        إذا (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        remove_بن_ورتیا_Funcs();
        إذا (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::جوڑو_Stat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::جوڑو_Stat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

فراغ V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    إذا (v3Global.opt.profCFuncs()) أرجع;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("inlinecfuncs", 0, dump_درخت_Either_Level() >= 6);
}
