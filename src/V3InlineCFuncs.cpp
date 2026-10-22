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

صنف Inline_CFuncs_Call_ګراف final : عام V3_ګراف {
عام:
    Inline_CFuncs_Call_ګراف()
        : V3_ګراف{} {}
    ~Inline_CFuncs_Call_ګراف() override = افتراضي;

    فراغ جمع_کړه_څنډه(Inline_CFuncs_Function_راس& from, Inline_CFuncs_Call_Site_راس& سر);
    فراغ جمع_کړه_څنډه(Inline_CFuncs_Call_Site_راس& from, Inline_CFuncs_Function_راس& سر);
};

صنف Either_راس VL_NOT_FINAL : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Either_راس, V3_ګراف_راس)
محمي:
    څرګند Either_راس(Inline_CFuncs_Call_ګراف& ګراف)
        : V3_ګراف_راس{&ګراف} {}
};

صنف Inline_CFuncs_Function_راس final : عام Either_راس {
    VL_RTTI_IMPL(Inline_CFuncs_Function_راس, Either_راس)
    AstCFunc* ثابت m_cfuncp;  // The function
    ثابت حرف* m_noInlineWyp = تش_اشاره;  // First reason the function should not be inlined
    ثابت حرف* m_keepWyp = تش_اشاره;  // Why the function should not be removed
    اندازه_t m_اندازه = 0;  // The size of the function

عام:
    Inline_CFuncs_Function_راس(Inline_CFuncs_Call_ګراف& ګراف, AstCFunc* cfuncp)
        : Either_راس{ګراف}
        , m_cfuncp{cfuncp} {}
    ~Inline_CFuncs_Function_راس() override = افتراضي;

    // ACCESSORS
    AstCFunc* cfuncp() ثابت { أرجع m_cfuncp; }
    اندازه_t اندازه() ثابت { أرجع m_اندازه; }
    فراغ اندازه_Inc(اندازه_t ارزښت = 1) { m_اندازه += ارزښت; }
    منطقي noInline() ثابت { أرجع m_noInlineWyp; }
    فراغ سېټ_No_Inline(ثابت حرف* whyp) {
        إذا (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    منطقي keep() ثابت { أرجع m_keepWyp; }
    فراغ سېټ_Keep(ثابت حرف* whyp) {
        إذا (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ثابت override {
        أرجع m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_کرښه* fileline() ثابت override { أرجع m_cfuncp->fileline(); }
    std::string dotShape() ثابت override { أرجع "box"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvt_منزل_Hex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::منزل_string(m_اندازه);
        إذا (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        إذا (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        أرجع str;
    }
};

صنف Inline_CFuncs_Call_Site_راس final : عام Either_راس {
    VL_RTTI_IMPL(Inline_CFuncs_Call_Site_راس, Either_راس)
    AstCCall* ثابت m_callp;  // The call site
    ثابت حرف* m_noInlineWyp = تش_اشاره;  // First reason the function should not be inlined

عام:
    Inline_CFuncs_Call_Site_راس(Inline_CFuncs_Call_ګراف& ګراف, AstCCall* callp)
        : Either_راس{ګراف}
        , m_callp{callp} {}
    ~Inline_CFuncs_Call_Site_راس() override = افتراضي;

    // ACCESSORS
    AstCCall* callp() ثابت { أرجع m_callp; }
    منطقي noInline() ثابت { أرجع m_noInlineWyp; }
    فراغ سېټ_No_Inline(ثابت حرف* whyp) {
        إذا (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_کرښه* fileline() ثابت override { أرجع m_callp->fileline(); }
    std::string dotColor() ثابت override { أرجع m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ثابت override { أرجع "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvt_منزل_Hex(m_callp);
        إذا (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        أرجع str;
    }
};

فراغ Inline_CFuncs_Call_ګراف::جمع_کړه_څنډه(Inline_CFuncs_Function_راس& caller,
                                    Inline_CFuncs_Call_Site_راس& callsite) {
    UASSERT_OBJ(callsite.دننه_تشه(), &callsite, "Call site should have at most one incoming edge");
    جديد V3_ګراف_څنډه{هذا, &caller, &callsite, 1, ریښتیا};  // Can cut caller -> callsite
}
فراغ Inline_CFuncs_Call_ګراف::جمع_کړه_څنډه(Inline_CFuncs_Call_Site_راس& callsite,
                                    Inline_CFuncs_Function_راس& callee) {
    UASSERT_OBJ(callsite.بهر_تشه(), &callsite, "Call site should have at most one outgoing edge");
    جديد V3_ګراف_څنډه{هذا, &callsite, &callee, 1, خطأ};
}

//######################################################################

صنف InlineCFuncsVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;

    // STATE
    Inline_CFuncs_Call_ګراف m_ګراف;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ثابت اندازه_t m_اندازه_پوله = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ثابت اندازه_t m_prod_پوله = v3Global.opt.inline_CFuncs_حاصل_ضرب();
    // Maximum size of caller to consider inlining into
    ثابت اندازه_t m_max_اندازه_CFunc = []() -> اندازه_t {
        صحيح maxCFunc = v3Global.opt.output_وېشل_CFuncs();
        صحيح maxFile = v3Global.opt.output_وېشل();
        إذا (maxCFunc <= 0) maxCFunc = std::numeric_limits<صحيح>::max();
        إذا (maxFile <= 0) maxFile = std::numeric_limits<صحيح>::max();
        أرجع std::min(maxCFunc, maxFile);
    }();
    ثابت اندازه_t m_max_اندازه_Trace = []() -> اندازه_t {
        صحيح maxTrace = v3Global.opt.output_وېشل_CTrace();
        صحيح maxFile = v3Global.opt.output_وېشل();
        إذا (maxTrace <= 0) maxTrace = std::numeric_limits<صحيح>::max();
        إذا (maxFile <= 0) maxFile = std::numeric_limits<صحيح>::max();
        أرجع std::min(maxTrace, maxFile);
    }();
    Inline_CFuncs_Function_راس* m_cfuncVtxp = تش_اشاره;  // Vertex of currently iterated function
    منطقي m_دننه_Exec_ګراف = خطأ;  // True while inside an AstExecGraph subtree

    // METHODS
    Inline_CFuncs_Function_راس* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        إذا (!cfuncp->user1p()) cfuncp->user1p(جديد Inline_CFuncs_Function_راس{m_ګراف, cfuncp});
        أرجع cfuncp->user1u().منزل<Inline_CFuncs_Function_راس*>();
    }

    Inline_CFuncs_Call_Site_راس* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        إذا (!callp->user1p()) callp->user1p(جديد Inline_CFuncs_Call_Site_راس{m_ګراف, callp});
        أرجع callp->user1u().منزل<Inline_CFuncs_Call_Site_راس*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ثابت callerp,  //
                               AstCCall* ثابت callp,  //
                               AstCFunc* ثابت calleep,  //
                               ثابت اندازه_t seq_عدد) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_غوټه_Stmt* ثابت callSitep = VN_AS(callp->backp(), Stmt_بیان);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        إذا (!calleep->stmtsp()) {
            VL_DO_DANGLING(ټېل_Deletep(callSitep->unlink_Fr_شا()), callSitep);
            أرجع تش_اشاره;
        }

        // Replace call site with a local scope
        File_کرښه* ثابت flp = callSitep->fileline();
        AstCLocalScope* ثابت lscopep = جديد AstCLocalScope{flp, تش_اشاره};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ټېل_Deletep(callSitep), callSitep);
        lscopep->جمع_کړه_Stmtsp(جديد AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ثابت std::string var_مختاړی
            = "__Vinline_" + std::منزل_string(seq_عدد) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ثابت VNUser2_دننه_Use user2_دننه_Use;

        // Clone local variables, add them to the local scope
        لكل (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ثابت newVarp = varp->clone_ونه(خطأ);
            newVarp->name(var_مختاړی + varp->name());
            lscopep->جمع_کړه_Stmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_غوټه* ثابت bodyp = calleep->stmtsp()->clone_ونه(ریښتیا);
        lscopep->جمع_کړه_Stmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreach_And_بل([&](Ast_غوټه* nodep) {
            إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                إذا (AstVar* ثابت varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_غوټه_DType* ثابت dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } وإلا إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                varp->name(var_مختاړی + varp->name());
            } وإلا إذا (AstCCall* ثابت callp = VN_CAST(nodep, CCall)) {
                callp->user1p(تش_اشاره);
            }
        });

        // Return the local scope
        أرجع lscopep;
    }

    فراغ doInlining() {
        // Need to gather vertices as we are changing the graph
        std::ویکټور<Inline_CFuncs_Function_راس*> m_fVtxps;
        لكل (V3_ګراف_راس& vtx : m_ګراف.vertices()) {
            إذا (Inline_CFuncs_Function_راس* ثابت fVtxp = vtx.cast<Inline_CFuncs_Function_راس>()) {
                m_fVtxps.emplace_شا(fVtxp);
            }
        }

        // Iterate functions leaf to root
        لكل (Inline_CFuncs_Function_راس* ثابت calleeVtxp : vlstd::سرچپه_view(m_fVtxps)) {
            // Should we inline this function?
            إذا (calleeVtxp->noInline()) استمر;  // Told not to

            // Check size heuristics
            ثابت منطقي doIt = [&]() {
                // Inline if small enough
                إذا (calleeVtxp->اندازه() <= m_اندازه_پوله) أرجع ریښتیا;
                // Inline if not too much bloat
                ثابت اندازه_t nCalls = calleeVtxp->دننه_څنډې().اندازه();
                إذا (nCalls * calleeVtxp->اندازه() <= m_prod_پوله) أرجع ریښتیا;
                // Otherwise don't inline
                أرجع خطأ;
            }();
            إذا (!doIt) استمر;

            // Ok, attempt to inline call sites
            اندازه_t nInlined = 0;
            لكل (ثابت V3_ګراف_څنډه* ثابت edgep : calleeVtxp->دننه_څنډې().unlinkable()) {
                Inline_CFuncs_Call_Site_راس* ثابت callVtxp
                    = edgep->fromp()->as<Inline_CFuncs_Call_Site_راس>();

                AstCFunc* ثابت calleep = calleeVtxp->cfuncp();
                AstCCall* ثابت callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                إذا (callVtxp->noInline()) استمر;  // Told not to
                إذا (callVtxp->دننه_تشه()) استمر;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->دننه_Size1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                Inline_CFuncs_Function_راس* ثابت callerVtxp
                    = callVtxp->دننه_څنډې().frontp()->fromp()->as<Inline_CFuncs_Function_راس>();
                AstCFunc* ثابت callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ثابت اندازه_t حد = callerp->isTrace() ? m_max_اندازه_Trace : m_max_اندازه_CFunc;
                إذا (callerVtxp->اندازه() + calleeVtxp->اندازه() > حد) استمر;

                // Can't do it if it's in a different scope, self pointers differ
                إذا (callerp->scopep() != calleep->scopep()) استمر;

                // Inline it
                إذا (!nInlined) ++m_statFuncsInlined;
                Ast_غوټه* ثابت inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_ګراف), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                إذا (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    فراغ remove_ناکارول_شوی_Funcs() {
        // Iterate root to leaves
        لكل (V3_ګراف_راس* ثابت vtxp : m_ګراف.vertices().unlinkable()) {
            Inline_CFuncs_Function_راس* ثابت fVtxp = vtxp->cast<Inline_CFuncs_Function_راس>();
            إذا (!fVtxp) استمر;
            // Keep if still called
            إذا (!fVtxp->دننه_تشه()) استمر;
            // Keep for other reasons
            إذا (fVtxp->keep()) استمر;

            AstCFunc* ثابت funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            لكل (ثابت V3_ګراف_څنډه* ثابت edgep : vtxp->بهر_څنډې().unlinkable()) {
                edgep->سر()->unlink_څنډې(&m_ګراف);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_ګراف);
            // Delete the function
            VL_DO_DANGLING(ټېل_Deletep(funcp->unlink_Fr_شا()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        لكل (V3_ګراف_راس* ثابت vtxp : m_ګراف.vertices().unlinkable()) {
            Inline_CFuncs_Call_Site_راس* ثابت cVtxp = vtxp->cast<Inline_CFuncs_Call_Site_راس>();
            إذا (!cVtxp) استمر;
            إذا (!cVtxp->دننه_تشه()) استمر;
            إذا (!cVtxp->بهر_تشه()) استمر;
            vtxp->unlinkDelete(&m_ګراف);
        }
    }

    // VISITORS
    فراغ لیدنه(AstCFunc* nodep) override {
        // Create the function vertex
        Inline_CFuncs_Function_راس* ثابت vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        إذا (nodep->rtnTypeVoid() != "void") vtxp->سېټ_No_Inline("Not void");
        إذا (nodep->dpiImportPrototype()) vtxp->سېټ_No_Inline("DPI import prototype");
        إذا (nodep->recursive()) vtxp->سېټ_No_Inline("Recursive");
        إذا (nodep->argsp()) vtxp->سېټ_No_Inline("Has arguments");
        إذا (nodep->isVirtual()) vtxp->سېټ_No_Inline("Virtual method");

        // Check if the function should not be removed
        إذا (nodep->ثبت_ټکی()) vtxp->سېټ_Keep("Entry point");
        إذا (nodep->dpiImportPrototype()) vtxp->سېټ_Keep("DPI import prototype");
        إذا (nodep->dpiExportDispatcher()) vtxp->سېټ_Keep("DPI export implementation");
        إذا (nodep->isVirtual()) vtxp->سېټ_Keep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    فراغ لیدنه(AstCCall* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->اندازه_Inc();
        AstCFunc* ثابت calleep = nodep->funcp();

        // Create the call site vertex
        Inline_CFuncs_Call_Site_راس* ثابت vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        إذا (!VN_IS(nodep->backp(), Stmt_بیان)) vtxp->سېټ_No_Inline("Not in statement position");
        إذا (m_دننه_Exec_ګراف) vtxp->سېټ_No_Inline("In ExecGraph");
        إذا (calleep->isVirtual()) vtxp->سېټ_No_Inline("Virtual method");

        // Add caller/callee edges
        إذا (m_cfuncVtxp) m_ګراف.جمع_کړه_څنډه(*m_cfuncVtxp, *vtxp);
        m_ګراف.جمع_کړه_څنډه(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->اندازه_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->سېټ_Keep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_Addr_د_CFunc* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->اندازه_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->سېټ_Keep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    فراغ لیدنه(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        إذا (m_cfuncVtxp) m_cfuncVtxp->سېټ_No_Inline("Contains TraceDecl");

        إذا (AstCCall* ثابت callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->سېټ_No_Inline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_Exec_ګراف* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        إذا (m_cfuncVtxp) m_cfuncVtxp->سېټ_No_Inline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        لكل (ثابت V3_ګراف_راس& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->سېټ_Keep("MTask function");
        }
        VL_RESTORER(m_دننه_Exec_ګراف);
        m_دننه_Exec_ګراف = ریښتیا;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstCStmt* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->سېټ_No_Inline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstCExpr* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->سېټ_No_Inline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstCStmtUser* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->سېټ_No_Inline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstCExprUser* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->سېټ_No_Inline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstCReturn* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->سېټ_No_Inline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->اندازه_Inc();
        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    څرګند InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_ګراف.acyclic(V3_ګراف_څنډه::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_ګراف.ترتیب();
        إذا (dump_ګراف_Level() >= 6) m_ګراف.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        إذا (dump_ګراف_Level() >= 6) m_ګراف.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        remove_ناکارول_شوی_Funcs();
        إذا (dump_ګراف_Level() >= 6) m_ګراف.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::جمع_کړه_Stat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::جمع_کړه_Stat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::جمع_کړه_Stat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

فراغ V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    إذا (v3Global.opt.profCFuncs()) أرجع;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("inlinecfuncs", 0, dump_ونه_Either_Level() >= 6);
}
