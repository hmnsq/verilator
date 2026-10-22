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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3InlineCFuncs.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3ExecGraph.h"
#شامل "V3Graph.h"
#شامل "V3Stats.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

جماعت Inline_CFuncs_Function_راس;
جماعت Inline_CFuncs_Call_Site_راس;

جماعت Inline_CFuncs_Call_گراف final : عوامی V3_گراف {
عوامی:
    Inline_CFuncs_Call_گراف()
        : V3_گراف{} {}
    ~Inline_CFuncs_Call_گراف() override = طےشدہ;

    خالی جوڑو_کنارہ(Inline_CFuncs_Function_راس& from, Inline_CFuncs_Call_Site_راس& چوٹی);
    خالی جوڑو_کنارہ(Inline_CFuncs_Call_Site_راس& from, Inline_CFuncs_Function_راس& چوٹی);
};

جماعت Either_راس VL_NOT_FINAL : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Either_راس, V3_گراف_راس)
محفوظ:
    صریح Either_راس(Inline_CFuncs_Call_گراف& گراف)
        : V3_گراف_راس{&گراف} {}
};

جماعت Inline_CFuncs_Function_راس final : عوامی Either_راس {
    VL_RTTI_IMPL(Inline_CFuncs_Function_راس, Either_راس)
    AstCFunc* ثابت m_cfuncp;  // The function
    ثابت حرف* m_noInlineWyp = خالی_اشاریہ;  // First reason the function should not be inlined
    ثابت حرف* m_keepWyp = خالی_اشاریہ;  // Why the function should not be removed
    سائز_t m_سائز = 0;  // The size of the function

عوامی:
    Inline_CFuncs_Function_راس(Inline_CFuncs_Call_گراف& گراف, AstCFunc* cfuncp)
        : Either_راس{گراف}
        , m_cfuncp{cfuncp} {}
    ~Inline_CFuncs_Function_راس() override = طےشدہ;

    // ACCESSORS
    AstCFunc* cfuncp() ثابت { واپس m_cfuncp; }
    سائز_t سائز() ثابت { واپس m_سائز; }
    خالی سائز_Inc(سائز_t قدر = 1) { m_سائز += قدر; }
    منطقی noInline() ثابت { واپس m_noInlineWyp; }
    خالی سیٹ_No_Inline(ثابت حرف* whyp) {
        اگر (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    منطقی keep() ثابت { واپس m_keepWyp; }
    خالی سیٹ_Keep(ثابت حرف* whyp) {
        اگر (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ثابت override {
        واپس m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_سطر* fileline() ثابت override { واپس m_cfuncp->fileline(); }
    std::string dotShape() ثابت override { واپس "box"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvt_مقصد_Hex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::مقصد_string(m_سائز);
        اگر (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        اگر (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        واپس str;
    }
};

جماعت Inline_CFuncs_Call_Site_راس final : عوامی Either_راس {
    VL_RTTI_IMPL(Inline_CFuncs_Call_Site_راس, Either_راس)
    AstCCall* ثابت m_callp;  // The call site
    ثابت حرف* m_noInlineWyp = خالی_اشاریہ;  // First reason the function should not be inlined

عوامی:
    Inline_CFuncs_Call_Site_راس(Inline_CFuncs_Call_گراف& گراف, AstCCall* callp)
        : Either_راس{گراف}
        , m_callp{callp} {}
    ~Inline_CFuncs_Call_Site_راس() override = طےشدہ;

    // ACCESSORS
    AstCCall* callp() ثابت { واپس m_callp; }
    منطقی noInline() ثابت { واپس m_noInlineWyp; }
    خالی سیٹ_No_Inline(ثابت حرف* whyp) {
        اگر (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_سطر* fileline() ثابت override { واپس m_callp->fileline(); }
    std::string dotColor() ثابت override { واپس m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ثابت override { واپس "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvt_مقصد_Hex(m_callp);
        اگر (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        واپس str;
    }
};

خالی Inline_CFuncs_Call_گراف::جوڑو_کنارہ(Inline_CFuncs_Function_راس& caller,
                                    Inline_CFuncs_Call_Site_راس& callsite) {
    UASSERT_OBJ(callsite.اندر_خالی_خانے(), &callsite, "Call site should have at most one incoming edge");
    نیا V3_گراف_کنارہ{یہ, &caller, &callsite, 1, سچ};  // Can cut caller -> callsite
}
خالی Inline_CFuncs_Call_گراف::جوڑو_کنارہ(Inline_CFuncs_Call_Site_راس& callsite,
                                    Inline_CFuncs_Function_راس& callee) {
    UASSERT_OBJ(callsite.برآمد_خالی_خانے(), &callsite, "Call site should have at most one outgoing edge");
    نیا V3_گراف_کنارہ{یہ, &callsite, &callee, 1, جھوٹ};
}

//######################################################################

جماعت InlineCFuncsVisitor final : عوامی VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // STATE
    Inline_CFuncs_Call_گراف m_گراف;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ثابت سائز_t m_سائز_حد = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ثابت سائز_t m_prod_حد = v3Global.opt.inline_CFuncs_حاصل_ضرب();
    // Maximum size of caller to consider inlining into
    ثابت سائز_t m_max_سائز_CFunc = []() -> سائز_t {
        صحیح maxCFunc = v3Global.opt.output_بانٹو_CFuncs();
        صحیح maxFile = v3Global.opt.output_بانٹو();
        اگر (maxCFunc <= 0) maxCFunc = std::numeric_limits<صحیح>::max();
        اگر (maxFile <= 0) maxFile = std::numeric_limits<صحیح>::max();
        واپس std::min(maxCFunc, maxFile);
    }();
    ثابت سائز_t m_max_سائز_Trace = []() -> سائز_t {
        صحیح maxTrace = v3Global.opt.output_بانٹو_CTrace();
        صحیح maxFile = v3Global.opt.output_بانٹو();
        اگر (maxTrace <= 0) maxTrace = std::numeric_limits<صحیح>::max();
        اگر (maxFile <= 0) maxFile = std::numeric_limits<صحیح>::max();
        واپس std::min(maxTrace, maxFile);
    }();
    Inline_CFuncs_Function_راس* m_cfuncVtxp = خالی_اشاریہ;  // Vertex of currently iterated function
    منطقی m_اندر_Exec_گراف = جھوٹ;  // True while inside an AstExecGraph subtree

    // METHODS
    Inline_CFuncs_Function_راس* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        اگر (!cfuncp->user1p()) cfuncp->user1p(نیا Inline_CFuncs_Function_راس{m_گراف, cfuncp});
        واپس cfuncp->user1u().مقصد<Inline_CFuncs_Function_راس*>();
    }

    Inline_CFuncs_Call_Site_راس* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        اگر (!callp->user1p()) callp->user1p(نیا Inline_CFuncs_Call_Site_راس{m_گراف, callp});
        واپس callp->user1u().مقصد<Inline_CFuncs_Call_Site_راس*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ثابت callerp,  //
                               AstCCall* ثابت callp,  //
                               AstCFunc* ثابت calleep,  //
                               ثابت سائز_t seq_عدد) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_گرہ_Stmt* ثابت callSitep = VN_AS(callp->backp(), Stmt_اظہاریہ);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        اگر (!calleep->stmtsp()) {
            VL_DO_DANGLING(ڈالو_Deletep(callSitep->unlink_Fr_عقب()), callSitep);
            واپس خالی_اشاریہ;
        }

        // Replace call site with a local scope
        File_سطر* ثابت flp = callSitep->fileline();
        AstCLocalScope* ثابت lscopep = نیا AstCLocalScope{flp, خالی_اشاریہ};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ڈالو_Deletep(callSitep), callSitep);
        lscopep->جوڑو_Stmtsp(نیا AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ثابت std::string var_سابقہ
            = "__Vinline_" + std::مقصد_string(seq_عدد) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ثابت VNUser2_اندر_Use user2_اندر_Use;

        // Clone local variables, add them to the local scope
        برائے (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ثابت newVarp = varp->clone_درخت(جھوٹ);
            newVarp->name(var_سابقہ + varp->name());
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
            اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                اگر (AstVar* ثابت varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_گرہ_DType* ثابت dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } ورنہ اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                varp->name(var_سابقہ + varp->name());
            } ورنہ اگر (AstCCall* ثابت callp = VN_CAST(nodep, CCall)) {
                callp->user1p(خالی_اشاریہ);
            }
        });

        // Return the local scope
        واپس lscopep;
    }

    خالی doInlining() {
        // Need to gather vertices as we are changing the graph
        std::سمتیہ<Inline_CFuncs_Function_راس*> m_fVtxps;
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            اگر (Inline_CFuncs_Function_راس* ثابت fVtxp = vtx.cast<Inline_CFuncs_Function_راس>()) {
                m_fVtxps.emplace_عقب(fVtxp);
            }
        }

        // Iterate functions leaf to root
        برائے (Inline_CFuncs_Function_راس* ثابت calleeVtxp : vlstd::الٹ_view(m_fVtxps)) {
            // Should we inline this function?
            اگر (calleeVtxp->noInline()) جاری;  // Told not to

            // Check size heuristics
            ثابت منطقی doIt = [&]() {
                // Inline if small enough
                اگر (calleeVtxp->سائز() <= m_سائز_حد) واپس سچ;
                // Inline if not too much bloat
                ثابت سائز_t nCalls = calleeVtxp->اندر_کنارے().سائز();
                اگر (nCalls * calleeVtxp->سائز() <= m_prod_حد) واپس سچ;
                // Otherwise don't inline
                واپس جھوٹ;
            }();
            اگر (!doIt) جاری;

            // Ok, attempt to inline call sites
            سائز_t nInlined = 0;
            برائے (ثابت V3_گراف_کنارہ* ثابت edgep : calleeVtxp->اندر_کنارے().unlinkable()) {
                Inline_CFuncs_Call_Site_راس* ثابت callVtxp
                    = edgep->fromp()->as<Inline_CFuncs_Call_Site_راس>();

                AstCFunc* ثابت calleep = calleeVtxp->cfuncp();
                AstCCall* ثابت callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                اگر (callVtxp->noInline()) جاری;  // Told not to
                اگر (callVtxp->اندر_خالی_خانے()) جاری;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->اندر_Size1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                Inline_CFuncs_Function_راس* ثابت callerVtxp
                    = callVtxp->اندر_کنارے().frontp()->fromp()->as<Inline_CFuncs_Function_راس>();
                AstCFunc* ثابت callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ثابت سائز_t حد_بندی = callerp->isTrace() ? m_max_سائز_Trace : m_max_سائز_CFunc;
                اگر (callerVtxp->سائز() + calleeVtxp->سائز() > حد_بندی) جاری;

                // Can't do it if it's in a different scope, self pointers differ
                اگر (callerp->scopep() != calleep->scopep()) جاری;

                // Inline it
                اگر (!nInlined) ++m_statFuncsInlined;
                Ast_گرہ* ثابت inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_گراف), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                اگر (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    خالی remove_غیر_مستعمل_Funcs() {
        // Iterate root to leaves
        برائے (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            Inline_CFuncs_Function_راس* ثابت fVtxp = vtxp->cast<Inline_CFuncs_Function_راس>();
            اگر (!fVtxp) جاری;
            // Keep if still called
            اگر (!fVtxp->اندر_خالی_خانے()) جاری;
            // Keep for other reasons
            اگر (fVtxp->keep()) جاری;

            AstCFunc* ثابت funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            برائے (ثابت V3_گراف_کنارہ* ثابت edgep : vtxp->برآمد_کنارے().unlinkable()) {
                edgep->چوٹی()->unlink_کنارے(&m_گراف);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_گراف);
            // Delete the function
            VL_DO_DANGLING(ڈالو_Deletep(funcp->unlink_Fr_عقب()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        برائے (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            Inline_CFuncs_Call_Site_راس* ثابت cVtxp = vtxp->cast<Inline_CFuncs_Call_Site_راس>();
            اگر (!cVtxp) جاری;
            اگر (!cVtxp->اندر_خالی_خانے()) جاری;
            اگر (!cVtxp->برآمد_خالی_خانے()) جاری;
            vtxp->unlinkDelete(&m_گراف);
        }
    }

    // VISITORS
    خالی دورہ(AstCFunc* nodep) override {
        // Create the function vertex
        Inline_CFuncs_Function_راس* ثابت vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        اگر (nodep->rtnTypeVoid() != "void") vtxp->سیٹ_No_Inline("Not void");
        اگر (nodep->dpiImportPrototype()) vtxp->سیٹ_No_Inline("DPI import prototype");
        اگر (nodep->recursive()) vtxp->سیٹ_No_Inline("Recursive");
        اگر (nodep->argsp()) vtxp->سیٹ_No_Inline("Has arguments");
        اگر (nodep->isVirtual()) vtxp->سیٹ_No_Inline("Virtual method");

        // Check if the function should not be removed
        اگر (nodep->اندراج_نقطہ()) vtxp->سیٹ_Keep("Entry point");
        اگر (nodep->dpiImportPrototype()) vtxp->سیٹ_Keep("DPI import prototype");
        اگر (nodep->dpiExportDispatcher()) vtxp->سیٹ_Keep("DPI export implementation");
        اگر (nodep->isVirtual()) vtxp->سیٹ_Keep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    خالی دورہ(AstCCall* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->سائز_Inc();
        AstCFunc* ثابت calleep = nodep->funcp();

        // Create the call site vertex
        Inline_CFuncs_Call_Site_راس* ثابت vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        اگر (!VN_IS(nodep->backp(), Stmt_اظہاریہ)) vtxp->سیٹ_No_Inline("Not in statement position");
        اگر (m_اندر_Exec_گراف) vtxp->سیٹ_No_Inline("In ExecGraph");
        اگر (calleep->isVirtual()) vtxp->سیٹ_No_Inline("Virtual method");

        // Add caller/callee edges
        اگر (m_cfuncVtxp) m_گراف.جوڑو_کنارہ(*m_cfuncVtxp, *vtxp);
        m_گراف.جوڑو_کنارہ(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->سائز_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->سیٹ_Keep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_Addr_کا_CFunc* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->سائز_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->سیٹ_Keep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    خالی دورہ(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        اگر (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains TraceDecl");

        اگر (AstCCall* ثابت callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->سیٹ_No_Inline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_Exec_گراف* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        اگر (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        برائے (ثابت V3_گراف_راس& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->سیٹ_Keep("MTask function");
        }
        VL_RESTORER(m_اندر_Exec_گراف);
        m_اندر_Exec_گراف = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCStmt* nodep) override {
        // Can reference anything in text
        اگر (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCExpr* nodep) override {
        // Can reference anything in text
        اگر (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCStmtUser* nodep) override {
        // Can reference anything in text
        اگر (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCExprUser* nodep) override {
        // Can reference anything in text
        اگر (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCReturn* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->سیٹ_No_Inline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->سائز_Inc();
        iterateChildrenConst(nodep);
    }

عوامی:
    // CONSTRUCTORS
    صریح InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_گراف.acyclic(V3_گراف_کنارہ::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_گراف.ترتیب();
        اگر (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        اگر (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        remove_غیر_مستعمل_Funcs();
        اگر (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::جوڑو_Stat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::جوڑو_Stat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

خالی V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    اگر (v3Global.opt.profCFuncs()) واپس;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("inlinecfuncs", 0, dump_درخت_Either_Level() >= 6);
}
