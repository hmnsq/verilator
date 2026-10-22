// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add temporaries, such as for task nodes
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
// V3Task's Transformations:
//
// Each module:
//      Look for TASKREF
//          Insert task's statements into the referrer
//      Look for TASKs
//          Remove them, they're inlined
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LanguageWords.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

ਵਰਗ Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼);
    Ast_ਗੰਢ* m_impurep = ਖਾਲੀ_ਸੂਚਕ;  // Node causing impure function w/ outside references
    ਤਾਰਕਿਕ m_noInline = ਝੂਠ;  // Marked with pragma
ਜਨਤਕ:
    ਸਪਸ਼ਟ Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp} {}
    ~Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    ਤਾਰਕਿਕ ਸ਼ੁੱਧ_3() ਅਚਲ { ਵਾਪਸ m_impurep == ਖਾਲੀ_ਸੂਚਕ; }
    Ast_ਗੰਢ* impure_ਗੰਢ() ਅਚਲ { ਵਾਪਸ m_impurep; }
    ਖਾਲੀ impure(Ast_ਗੰਢ* nodep) { m_impurep = nodep; }
    ਤਾਰਕਿਕ noInline() ਅਚਲ { ਵਾਪਸ m_noInline; }
    ਖਾਲੀ noInline(ਤਾਰਕਿਕ flag) { m_noInline = flag; }
};

ਵਰਗ Task_FTask_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Task_FTask_ਸ਼ੀਰਸ਼, Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_ਗੰਢ_FTask* ਅਚਲ m_nodep;
    AstCFunc* m_cFuncp = ਖਾਲੀ_ਸੂਚਕ;
    ਤਾਰਕਿਕ m_needsNonInlineCFunc = ਝੂਠ;  // Needs CFunc for selected non-inlined call sites

ਜਨਤਕ:
    Task_FTask_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ_FTask* nodep)
        : Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼{graphp}
        , m_nodep{nodep} {}
    ~Task_FTask_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    Ast_ਗੰਢ_FTask* nodep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_nodep; }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nodep()->name(); }
    string dotColor() ਅਚਲ override { ਵਾਪਸ ਸ਼ੁੱਧ_3() ? "black" : "red"; }
    AstCFunc* cFuncp() ਅਚਲ { ਵਾਪਸ m_cFuncp; }
    ਖਾਲੀ cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    ਤਾਰਕਿਕ needsNonInlineCFunc() ਅਚਲ { ਵਾਪਸ m_needsNonInlineCFunc; }
    ਖਾਲੀ needsNonInlineCFunc(ਤਾਰਕਿਕ flag) { m_needsNonInlineCFunc = flag; }
};

ਵਰਗ Task_ਕੋਡ_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Task_ਕੋਡ_ਸ਼ੀਰਸ਼, Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼);
    // Top vertex for all calls not under another task
ਜਨਤਕ:
    ਸਪਸ਼ਟ Task_ਕੋਡ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp)
        : Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼{graphp} {}
    ~Task_ਕੋਡ_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ "*CODE*"; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "green"; }
};

ਵਰਗ Task_ਕਿਨਾਰਾ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(Task_ਕਿਨਾਰਾ, V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ)
ਜਨਤਕ:
    Task_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼* fromp, Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼* ਸਿਖਰ)
        : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, 1, ਝੂਠ} {}
    ~Task_ਕਿਨਾਰਾ() override = ਮੂਲ;
    string dotLabel() ਅਚਲ override { ਵਾਪਸ "w" + cvt_ਮੰਜ਼ਿਲ_Str(ਭਾਰ_2()); }
};

//######################################################################

ਵਰਗ TaskStateVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;
    ਅਚਲ VNUser4_ਵਿੱਚ_Use m_inuser4;

    // TYPES
    ਵਰਤੋਂ Var_ਮੰਜ਼ਿਲ_Scope_Map = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    ਵਰਤੋਂ Func_ਮੰਜ਼ਿਲ_Class_Map = std::unordered_map<ਅਚਲ Ast_ਗੰਢ_FTask*, AstClass*>;
    // MEMBERS
    Var_ਮੰਜ਼ਿਲ_Scope_Map m_var_ਮੰਜ਼ਿਲ_Scope_Map;  // Map for Var -> VarScope mappings
    Func_ਮੰਜ਼ਿਲ_Class_Map m_func_ਮੰਜ਼ਿਲ_Class_Map;  // Map for ctor func -> class
    Ast_ਗੰਢ_FTask* m_ctorp = ਖਾਲੀ_ਸੂਚਕ;  // Class constructor
    AstClass* m_classp = ਖਾਲੀ_ਸੂਚਕ;  // Current class
    V3_ਗ੍ਰਾਫ m_call_ਗ੍ਰਾਫ;  // Task call graph
    Task_ਅਧਾਰ_ਸ਼ੀਰਸ਼* m_curVxp;  // Current vertex we're adding to
    std::ਸਦਿਸ਼<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    ਤਾਰਕਿਕ m_under_ਪੋਰਟ_Var = ਝੂਠ;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

ਜਨਤਕ:
    // METHODS
    ਸਥਿਰ ਤਾਰਕਿਕ is_Virtual_Iface_ਵਿਧੀ_Call(ਅਚਲ Ast_ਗੰਢ_FTask_Ref* refp) {
        ਅਚਲ Ast_ਵਿਧੀ_Call* ਅਚਲ mrefp = VN_CAST(refp, ਵਿਧੀ_Call);
        ਜੇ (!mrefp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = mrefp->fromp();
        ਜੇ (!fromp || !fromp->dtypep()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstIfaceRefDType* ਅਚਲ ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        ਵਾਪਸ ifrefp && ifrefp->isVirtual();
    }
    ਸਥਿਰ ਤਾਰਕਿਕ isIfaceFTaskScope(ਅਚਲ AstScope* scopep) {
        ਵਾਪਸ scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_ਗੰਢ_FTask* nodep) {
        AstScope* ਅਚਲ scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        ਵਾਪਸ scopep;
    }
    AstVarScope* ਲੱਭੋ_Var_Scope(AstScope* scopep, AstVar* nodep) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_var_ਮੰਜ਼ਿਲ_Scope_Map.ਲੱਭੋ(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_var_ਮੰਜ਼ਿਲ_Scope_Map.ਸਿਰਾ(), nodep, "No scope for var");
        ਵਾਪਸ iter->ਦੂਜਾ;
    }
    AstClass* getClassp(Ast_ਗੰਢ_FTask* nodep) {
        AstClass* ਅਚਲ classp = m_func_ਮੰਜ਼ਿਲ_Class_Map[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        ਵਾਪਸ classp;
    }
    ਖਾਲੀ remapFuncClassp(Ast_ਗੰਢ_FTask* nodep, Ast_ਗੰਢ_FTask* newp) {
        m_func_ਮੰਜ਼ਿਲ_Class_Map[newp] = getClassp(nodep);
    }
    ਤਾਰਕਿਕ ftaskNoInline(Ast_ਗੰਢ_FTask* nodep) { ਵਾਪਸ get_FTask_ਸ਼ੀਰਸ਼(nodep)->noInline(); }
    ਤਾਰਕਿਕ ftaskNeedsNonInlineCFunc(Ast_ਗੰਢ_FTask* nodep) {
        ਵਾਪਸ get_FTask_ਸ਼ੀਰਸ਼(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_ਗੰਢ_FTask* nodep) { ਵਾਪਸ get_FTask_ਸ਼ੀਰਸ਼(nodep)->cFuncp(); }
    ਖਾਲੀ ftaskCFuncp(Ast_ਗੰਢ_FTask* nodep, AstCFunc* cfuncp) {
        get_FTask_ਸ਼ੀਰਸ਼(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_ਗੰਢ* checkImpure(Ast_ਗੰਢ_FTask* nodep) { ਵਾਪਸ get_FTask_ਸ਼ੀਰਸ਼(nodep)->impure_ਗੰਢ(); }

ਨਿਜੀ:
    Task_FTask_ਸ਼ੀਰਸ਼* get_FTask_ਸ਼ੀਰਸ਼(Ast_ਗੰਢ_FTask* nodep) {
        ਜੇ (!nodep->user4p()) nodep->user4p(ਨਵਾਂ Task_FTask_ਸ਼ੀਰਸ਼{&m_call_ਗ੍ਰਾਫ, nodep});
        ਵਾਪਸ static_cast<Task_FTask_ਸ਼ੀਰਸ਼*>(nodep->user4u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼());
    }

    ਖਾਲੀ propagateImpureReason() {
        std::ਸਦਿਸ਼<Task_FTask_ਸ਼ੀਰਸ਼*> workList;
        // Gather all impure FTask vertices
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_call_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (Task_FTask_ਸ਼ੀਰਸ਼* ਅਚਲ ftVtxp = vtx.cast<Task_FTask_ਸ਼ੀਰਸ਼>()) {
                ਜੇ (!ftVtxp->ਸ਼ੁੱਧ_3()) workList.emplace_ਪਿੱਠ(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        ਜਦੋਂ (!workList.ਖਾਲੀ_ਥਾਵਾਂ()) {
            Task_FTask_ਸ਼ੀਰਸ਼* ਅਚਲ calleep = workList.ਪਿੱਠ();
            workList.ਕੱਢੋ_ਪਿੱਠ();
            UASSERT_OBJ(!calleep->ਸ਼ੁੱਧ_3(), calleep->nodep(), "Should not enqueue pure vertex");
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : calleep->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ਜੇ (Task_FTask_ਸ਼ੀਰਸ਼* ਅਚਲ callerp = ਕਿਨਾਰਾ.fromp()->cast<Task_FTask_ਸ਼ੀਰਸ਼>()) {
                    ਜੇ (!callerp->ਸ਼ੁੱਧ_3()) ਜਾਰੀ;  // Already processed or on workList
                    callerp->impure(calleep->impure_ਗੰਢ());
                    workList.emplace_ਪਿੱਠ(callerp);
                }
            }
        }
    }

    ਖਾਲੀ decideInlining() {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_call_ਗ੍ਰਾਫ.vertices()) {
            Task_FTask_ਸ਼ੀਰਸ਼* ਅਚਲ ftVtxp = vtx.cast<Task_FTask_ਸ਼ੀਰਸ਼>();
            ਜੇ (!ftVtxp) ਜਾਰੀ;
            // Move on if already decided not to inline
            ਜੇ (ftVtxp->noInline()) ਜਾਰੀ;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            ਜੇ (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(ਸੱਚ);
                ਜਾਰੀ;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            ਜੇ (v3Global.opt.fInlineFuncsEager()) ਜਾਰੀ;

            Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            ਜੇ (!ftaskp->verilogFunction()) ਜਾਰੀ;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            ਜੇ (ftaskp->task_ਜਨਤਕ()) ਜਾਰੀ;
            // If the function is pure, it's OK not to inline, so don't do it
            ਜੇ (ftVtxp->ਸ਼ੁੱਧ_3()) ftVtxp->noInline(ਸੱਚ);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVarScope* ਅਚਲ vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ਅਚਲ varp = vscp->varp();
                ਜੇ (varp->isFuncLocal() || varp->is_ਵਰਤਿਆ_Loop_Idx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_var_ਮੰਜ਼ਿਲ_Scope_Map.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ taskp = VN_CAST(stmtp, ਗੰਢ_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        Task_FTask_ਸ਼ੀਰਸ਼* ਅਚਲ taskVtxp = get_FTask_ਸ਼ੀਰਸ਼(nodep->taskp());
        ਨਵਾਂ Task_ਕਿਨਾਰਾ{&m_call_ਗ੍ਰਾਫ, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        ਜੇ (is_Virtual_Iface_ਵਿਧੀ_Call(nodep)) taskVtxp->needsNonInlineCFunc(ਸੱਚ);
        // Do we have to disable inlining the function?
        ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        ਜੇ (!taskVtxp->noInline()) {  // Else short-circuit below
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : tconnects) {
                ਅਚਲ AstVar* ਅਚਲ portp = itr.ਪਹਿਲਾ;
                ਅਚਲ AstArg* ਅਚਲ argp = itr.ਦੂਜਾ;
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinp = argp->exprp()) {
                    ਜੇ ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(ਸੱਚ);
                    }
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = get_FTask_ਸ਼ੀਰਸ਼(nodep);
        ਜੇ (nodep->dpiImport()) m_curVxp->noInline(ਸੱਚ);
        ਜੇ (nodep->class_ਵਿਧੀ()) m_curVxp->noInline(ਸੱਚ);  // Until V3Task supports it
        ਜੇ (nodep->recursive()) m_curVxp->noInline(ਸੱਚ);
        ਜੇ (nodep->isConstructor()) {
            m_curVxp->noInline(ਸੱਚ);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_func_ਮੰਜ਼ਿਲ_Class_Map[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPragma* nodep) override {
        ਜੇ (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(ਸੱਚ);
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        VL_RESTORER(m_under_ਪੋਰਟ_Var);
        ਜੇ (nodep->isIO()) m_under_ਪੋਰਟ_Var = ਸੱਚ;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ਜੇ (m_under_ਪੋਰਟ_Var) ਵਾਪਸ;
        AstVar* ਅਚਲ varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        ਜੇ (nodep->access().isReadOnly() && varp->const_Pool_ਇੰਦਰਾਜ()) ਵਾਪਸ;
        ਜੇ (varp->user4u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼() != m_curVxp) {
            ਜੇ (m_curVxp->ਸ਼ੁੱਧ_3() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        // Move initial statements into the constructor
        VL_RESTORER_CLEAR(m_initialps);
        VL_RESTORER(m_ctorp);
        VL_RESTORER(m_classp);
        m_ctorp = ਖਾਲੀ_ਸੂਚਕ;
        m_classp = nodep;
        {  // Find m_initialps, m_ctor
            iterateChildren(nodep);
        }
        UASSERT_OBJ(m_ctorp, nodep, "class constructor missing");  // LinkDot always makes it
        Ast_ਗੰਢ* insertp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (AstInitialAutomatic* initialp : m_initialps) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ movep = initialp->stmtsp()) {
                movep->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(insertp, movep);
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(initialp->unlink_Fr_ਪਿੱਠ()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomatic* nodep) override {
        m_initialps.ਧੱਕੋ_ਪਿੱਠ(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = ਨਵਾਂ Task_ਕੋਡ_ਸ਼ੀਰਸ਼{&m_call_ਗ੍ਰਾਫ};
        //
        iterate(nodep);
        //
        m_call_ਗ੍ਰਾਫ.remove_Redundant_ਕਿਨਾਰੇ_ਜੋੜ(&Task_ਕਿਨਾਰਾ::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        ਜੇ (dump_ਗ੍ਰਾਫ_Level()) m_call_ਗ੍ਰਾਫ.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = ਮੂਲ;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

ਬਣਤਰ TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    ਸਥਿਰ std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> unpackDimsAndStrides(AstVar* varp) {
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypep()->skipRefp();
        std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> dimStrides;
        AstUnpackArrayDType* ਅਚਲ unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ਜੇ (!unpackp) ਵਾਪਸ dimStrides;

        ਅਚਲ std::ਸਦਿਸ਼<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ਅਚਲ ਮਾਪ_t nDims = dims.ਮਾਪ();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].ਪਹਿਲਾ = dims.ਪਿੱਠ()->elementsConst();
        dimStrides[nDims - 1].ਦੂਜਾ = 1;
        // Rest are densly packed
        ਲਈ (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].ਪਹਿਲਾ = dims[i]->elementsConst();
            dimStrides[i].ਦੂਜਾ = dimStrides[i + 1].ਪਹਿਲਾ * dimStrides[i + 1].ਦੂਜਾ;
        }
        ਵਾਪਸ dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    ਸਥਿਰ std::pair<std::string, ਤਾਰਕਿਕ> dpi_ਮੰਜ਼ਿਲ_ਅੰਦਰੂਨੀ_Cvt_Stmt(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        ਅਚਲ AstBasicDType* ਅਚਲ basicp = dtypep->basicp();

        // DPI types using Primitive C types
        ਜੇ (basicp->keyword() == VBasicDTypeKwd::CHANDLE) ਵਾਪਸ {"VL_CVT_VP_Q(", ਝੂਠ};
        ਜੇ (basicp->keyword() == VBasicDTypeKwd::STRING) ਵਾਪਸ {"VL_CVT_N_CSTR(", ਝੂਠ};
        ਜੇ (basicp->isDpiPrimitive()) ਵਾਪਸ {"(", ਝੂਠ};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->is_Dpi_ਦੋਆਧਾਰੀ_ਅੰਕ_Vec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ਅਚਲ std::string vecType = basicp->is_Dpi_ਦੋਆਧਾਰੀ_ਅੰਕ_Vec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ਅਚਲ ਅੱਖਰ ਮਾਪ_Char = dtypep->ਚੌੜਾਈ() <= 8    ? 'C'
                              : dtypep->ਚੌੜਾਈ() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ਅਚਲ std::string& ਮਾਪ = std::ਮੰਜ਼ਿਲ_string(dtypep->ਚੌੜਾਈ());
        // cppcheck-suppress strPlusChar
        ਵਾਪਸ {"VL_SET_"s + ਮਾਪ_Char + "_" + vecType + "(" + ਮਾਪ + ", ", ਸੱਚ};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

ਵਰਗ TaskVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // TYPES
    ਵਰਤੋਂ DpiCFuncs = std::map<ਅਚਲ string, std::tuple<Ast_ਗੰਢ_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ਅਚਲ m_statep;  // Common state between visitors
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    Ast_ਸਿਖਰ_Scope* ਅਚਲ m_ਸਿਖਰ_Scopep = v3Global.rootp()->ਸਿਖਰ_Scopep();  // The AstTopScope
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope
    Ast_ਗੰਢ* m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // Where to insert statement
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Sensesp = ਝੂਠ;  // Are we under a senitem?
    ਤਾਰਕਿਕ m_ਵਿੱਚ_New = ਝੂਠ;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    ਪੂਰਨ m_uncon_Var_ਅੰਕ = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpi_ਨਾਂ;  // Map of all created DPI functions
    std::map<std::string, ਅਚਲ AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ਅਚਲ string& name, AstVar* examplep) {
        AstVar* ਅਚਲ newvarp = ਨਵਾਂ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(ਸੱਚ);
        funcp->ਜੋੜੋ_Varsp(newvarp);
        AstVarScope* ਅਚਲ newvscp = ਨਵਾਂ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->ਜੋੜੋ_Varsp(newvscp);
        ਵਾਪਸ newvscp;
    }
    AstVarScope* create_ਨਿਵੇਸ਼_Var(AstCFunc* funcp, ਅਚਲ string& name, VBasicDTypeKwd kwd) {
        AstVar* ਅਚਲ newvarp
            = ਨਵਾਂ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->ਲੱਭੋ_Basic_DType(kwd)};
        newvarp->funcLocal(ਸੱਚ);
        newvarp->direction(VDirection::ਨਿਵੇਸ਼);
        funcp->ਜੋੜੋ_Argsp(newvarp);
        AstVarScope* ਅਚਲ newvscp = ਨਵਾਂ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->ਜੋੜੋ_Varsp(newvscp);
        ਵਾਪਸ newvscp;
    }
    ਸਥਿਰ AstVarScope* const_Pool_ਸਾਰਣੀ(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ਅਚਲ refp = V3ConstPool::ਲੱਭੋ(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ਅਚਲ vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_ਰੁੱਖ(), refp);
        ਵਾਪਸ vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ਅਚਲ Ast_ਗੰਢ_FTask_Ref* refp, AstVar* invarp,
                                ਅਚਲ std::string& suffix) {
        ਜੇ (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            ਵਾਪਸ const_Pool_ਸਾਰਣੀ(invarp);
        } ਨਹੀਂਤਾਂ {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* ਅਚਲ newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* ਅਚਲ newvarp = newvscp->varp();
            newvarp->funcLocal(ਝੂਠ);
            newvarp->propagateAttrFrom(invarp);
            newvarp->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
            ਵਾਪਸ newvscp;
        }
    }

    // Replace varrefs with new var pointer
    ਖਾਲੀ relink(Ast_ਗੰਢ* nodep) {
        nodep->foreach_And_ਅਗਲਾ([](AstVarRef* refp) {
            ਜੇ (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ਅਚਲ newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    Ast_ਸੌਂਪੋ* connect_ਪੋਰਟ_Make_ਵਿੱਚ_ਸੌਂਪੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* pinp, AstVarScope* newvscp, ਤਾਰਕਿਕ ਸ਼ੁੱਧ_Check) {
        // Create input assignment to go in FRONT of function call
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਿੱਚ_Pinp = pinp;
        ਜੇ (AstResizeLValue* sinPinp = VN_CAST(ਵਿੱਚ_Pinp, ResizeLValue)) ਵਿੱਚ_Pinp = sinPinp->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਿੱਚ_Pin_Clonep
            = ਸ਼ੁੱਧ_Check ? ਵਿੱਚ_Pinp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ) : ਵਿੱਚ_Pinp->clone_ਰੁੱਖ(ਸੱਚ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        ਜੇ (arrayp && arrayp->is_ਉਲਟ_ਦਾ(ਵਿੱਚ_Pin_Clonep->dtypep())) {
            ਵਿੱਚ_Pin_Clonep
                = ਨਵਾਂ Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack{pinp->fileline(), ਵਿੱਚ_Pin_Clonep, newvscp->dtypep()};
        }
        Ast_ਸੌਂਪੋ* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            pinp->fileline(), ਨਵਾਂ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            ਵਿੱਚ_Pin_Clonep};
        assp->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::BLKSEQ, ਸੱਚ);  // Ok if in <= block
        ਵਾਪਸ assp;
    }
    Ast_ਸੌਂਪੋ* connect_ਪੋਰਟ_Make_ਬਾਹਰਲਾ_ਸੌਂਪੋ(AstVar* portp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* pinp, AstVarScope* newvscp,
                                        ਤਾਰਕਿਕ ਸ਼ੁੱਧ_Check) {
        // If needed, remap size of function to caller's output size
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਬਾਹਰਲਾ_Pinp = pinp;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* postRhsp = ਨਵਾਂ AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        ਜੇ (AstResizeLValue* soutPinp = VN_CAST(ਬਾਹਰਲਾ_Pinp, ResizeLValue)) {
            ਬਾਹਰਲਾ_Pinp = soutPinp->lhsp();
            ਜੇ (VN_IS(ਬਾਹਰਲਾ_Pinp, RToIRoundS) || VN_IS(ਬਾਹਰਲਾ_Pinp, RToIS)) {
                ਬਾਹਰਲਾ_Pinp = VN_AS(ਬਾਹਰਲਾ_Pinp, ਗੰਢ_Uniop)->lhsp();
                ਜੇ (postRhsp->isSigned()) {
                    postRhsp = ਨਵਾਂ AstISToRD{pinp->fileline(), postRhsp};
                } ਨਹੀਂਤਾਂ {
                    postRhsp = ਨਵਾਂ AstIToRD{pinp->fileline(), postRhsp};
                }
            } ਨਹੀਂਤਾਂ {
                ਜੇ (Ast_ਗੰਢ_Uniop* aoutPinp = VN_CAST(ਬਾਹਰਲਾ_Pinp, Extend)) {
                    ਬਾਹਰਲਾ_Pinp = aoutPinp->lhsp();
                } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_Uniop* aoutPinp = VN_CAST(ਬਾਹਰਲਾ_Pinp, ExtendS)) {
                    ਬਾਹਰਲਾ_Pinp = aoutPinp->lhsp();
                } ਨਹੀਂਤਾਂ ਜੇ (AstSel* aoutPinp = VN_CAST(ਬਾਹਰਲਾ_Pinp, Sel)) {
                    ਬਾਹਰਲਾ_Pinp = aoutPinp->fromp();
                } ਨਹੀਂਤਾਂ {
                    ਬਾਹਰਲਾ_Pinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                ਜੇ (ਬਾਹਰਲਾ_Pinp->ਚੌੜਾਈ() < portp->ਚੌੜਾਈ()) {
                    postRhsp = ਨਵਾਂ AstSel{pinp->fileline(), postRhsp, 0, pinp->ਚੌੜਾਈ()};
                } ਨਹੀਂਤਾਂ {  // pin width > port width
                    ਜੇ (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = ਨਵਾਂ AstExtendS{pinp->fileline(), postRhsp};
                    } ਨਹੀਂਤਾਂ {
                        postRhsp = ਨਵਾਂ AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(ਬਾਹਰਲਾ_Pinp);
        }
        ਜੇ (VN_IS(ਬਾਹਰਲਾ_Pinp, IToRD) || VN_IS(ਬਾਹਰਲਾ_Pinp, ISToRD)) {
            ਬਾਹਰਲਾ_Pinp = VN_AS(ਬਾਹਰਲਾ_Pinp, ਗੰਢ_Uniop)->lhsp();
            postRhsp = ਨਵਾਂ AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(ਬਾਹਰਲਾ_Pinp);
        }
        // Put output assignment AFTER function call
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਬਾਹਰਲਾ_Pin_Clonep
            = ਸ਼ੁੱਧ_Check ? ਬਾਹਰਲਾ_Pinp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ) : ਬਾਹਰਲਾ_Pinp->clone_ਰੁੱਖ(ਸੱਚ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp
            = VN_CAST(ਬਾਹਰਲਾ_Pinp->dtypep()->skipRefp(), UnpackArrayDType);
        ਜੇ (arrayp && arrayp->is_ਉਲਟ_ਦਾ(postRhsp->dtypep())) {
            postRhsp = ਨਵਾਂ Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack{pinp->fileline(), postRhsp, ਬਾਹਰਲਾ_Pinp->dtypep()};
        }
        Ast_ਸੌਂਪੋ* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ{pinp->fileline(), ਬਾਹਰਲਾ_Pin_Clonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::BLKSEQ, ਸੱਚ);  // Ok if in <= block
        ਵਾਪਸ assp;
    }

    ਖਾਲੀ changeAtWriteRecurse(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp) {
        // Change nested at methods to writable variant
        ਜੇ (AstCMethodHard* ਅਚਲ cMethodp = VN_CAST(exprp, CMethodHard)) {
            ਜੇ (cMethodp->ਵਿਧੀ() == VCMethod::ARRAY_AT) {
                cMethodp->ਵਿਧੀ(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    ਖਾਲੀ connect_ਪੋਰਟ(ਅਚਲ Ast_ਗੰਢ_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_ਗੰਢ* beginp,
                     ਤਾਰਕਿਕ inlineTask) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* pinp = argp->exprp();
        ਜੇ (inlineTask) {
            portp->unlink_Fr_ਪਿੱਠ();
            ਧੱਕੋ_Deletep(portp);  // Remove it from the clone (not original)
        }
        ਜੇ (!pinp) {
            // Too few arguments in function call
        } ਨਹੀਂਤਾਂ {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            ਜੇ (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::ਮੰਜ਼ਿਲ_string(m_uncon_Var_ਅੰਕ++),
                                                portp->dtypep()};
                m_modp->ਜੋੜੋ_Stmtsp(varp);
                AstVarScope* ਅਚਲ newvscp = ਨਵਾਂ AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->ਜੋੜੋ_Varsp(newvscp);
                AstVarRef* ਅਚਲ repp = ਨਵਾਂ AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(pinp), pinp);
                pinp = repp;
            }
            ਜੇ (inlineTask) {
                ਧੱਕੋ_Deletep(pinp->unlink_Fr_ਪਿੱਠ());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), argp);  // Args no longer needed
            }
            ਜੇ (portp->isRef() || portp->isConstRef()) {
                ਤਾਰਕਿਕ refArgOk = ਝੂਠ;
                ਜੇ (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (AstCMethodHard* ਅਚਲ cMethodp = VN_CAST(pinp, CMethodHard)) {
                    ਜੇ (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), ਕਤਾਰ_DType)) {
                        refArgOk = cMethodp->ਵਿਧੀ() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->ਵਿਧੀ() == VCMethod::DYN_AT_WRITE_APPEND_ਪਿੱਠ;
                    } ਨਹੀਂਤਾਂ {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->ਵਿਧੀ() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                ਜੇ (refArgOk) {
                    ਜੇ (AstVarRef* ਅਚਲ varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } ਨਹੀਂਤਾਂ {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                ਜੇ (inlineTask) {
                    ਜੇ (AstVarRef* ਅਚਲ varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* ਅਚਲ localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } ਨਹੀਂਤਾਂ {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } ਨਹੀਂਤਾਂ ਜੇ (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* ਅਚਲ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ਜੇ (!inlineTask) {
                    pinp->replaceWith(
                        ਨਵਾਂ AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    ਧੱਕੋ_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                Ast_ਸੌਂਪੋ* ਅਚਲ preassp = connect_ਪੋਰਟ_Make_ਵਿੱਚ_ਸੌਂਪੋ(pinp, newvscp, ਸੱਚ);
                beginp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(preassp);

                Ast_ਸੌਂਪੋ* ਅਚਲ postassp = connect_ਪੋਰਟ_Make_ਬਾਹਰਲਾ_ਸੌਂਪੋ(portp, pinp, newvscp, ਸੱਚ);
                beginp->ਜੋੜੋ_ਅਗਲਾ(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } ਨਹੀਂਤਾਂ ਜੇ (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ਅਚਲ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ਜੇ (!inlineTask) {
                    pinp->replaceWith(ਨਵਾਂ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    ਧੱਕੋ_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                Ast_ਸੌਂਪੋ* ਅਚਲ postassp = connect_ਪੋਰਟ_Make_ਬਾਹਰਲਾ_ਸੌਂਪੋ(portp, pinp, newvscp, ਝੂਠ);
                // Put assignment BEHIND of all other statements
                beginp->ਜੋੜੋ_ਅਗਲਾ(postassp);
            } ਨਹੀਂਤਾਂ ਜੇ (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ਅਚਲ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                Ast_ਸੌਂਪੋ* ਅਚਲ preassp = connect_ਪੋਰਟ_Make_ਵਿੱਚ_ਸੌਂਪੋ(pinp, newvscp, ਝੂਠ);
                // Put assignment in FRONT of all other statements
                beginp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(preassp);
            }
        }
    }

    ਤਾਰਕਿਕ has_Ref_ਦਲੀਲ(Ast_ਗੰਢ_FTask* nodep) {
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (varp->isRef() || varp->isConstRef()) ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    Ast_ਗੰਢ* createInlinedFTask(Ast_ਗੰਢ_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_ਗੰਢ* ਅਚਲ newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_ਰੁੱਖ(ਸੱਚ) : ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ* ਅਚਲ beginp
            = ਨਵਾਂ AstComment{refp->fileline(), "Function: "s + refp->name(), ਸੱਚ};
        ਜੇ (newbodysp) beginp->ਜੋੜੋ_ਅਗਲਾ(newbodysp);
        ਜੇ (debug() >= 9) beginp->dump_ਰੁੱਖ_And_ਅਗਲਾ(ਬਾਹਰ, "-  newbegi: ");
        //
        // Create input variables
        Ast_ਗੰਢ::user2_Clear_ਰੁੱਖ();
        {
            ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : tconnects) {
                AstVar* ਅਚਲ portp = itr.ਪਹਿਲਾ;
                AstArg* ਅਚਲ argp = itr.ਦੂਜਾ;
                connect_ਪੋਰਟ(refp, portp, argp, beginp, ਸੱਚ);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_ਗੰਢ* nextstmtp;
            ਲਈ (Ast_ਗੰਢ* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    ਜੇ (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* ਅਚਲ localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        ਜੇ (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_ਗੰਢ* ਅਚਲ crstp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                                portp->fileline(),
                                ਨਵਾਂ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                ਨਵਾਂ AstCReset{portp->fileline(), portp, ਝੂਠ}};
                            portp->replaceWith(crstp);
                        } ਨਹੀਂਤਾਂ {
                            portp->unlink_Fr_ਪਿੱਠ();
                        }
                        ਧੱਕੋ_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        ਜੇ (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        ਜੇ (debug() >= 9) beginp->dump_ਰੁੱਖ_And_ਅਗਲਾ(ਬਾਹਰ, "-  iotask: ");
        ਵਾਪਸ beginp;
    }

    Ast_ਗੰਢ* createNonInlinedFTask(Ast_ਗੰਢ_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ਅਚਲ cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_ਗੰਢ* ਅਚਲ beginp
            = ਨਵਾਂ AstComment{refp->fileline(), "Function: "s + refp->name(), ਸੱਚ};
        Ast_ਗੰਢ_CCall* ccallp;
        ਜੇ (VN_IS(refp, New)) {
            AstCNew* ਅਚਲ cnewp = ਨਵਾਂ AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਵਿਧੀ_Call* ਅਚਲ mrefp = VN_CAST(refp, ਵਿਧੀ_Call)) {
            ccallp = ਨਵਾਂ AstCMethodCall{refp->fileline(), mrefp->fromp()->unlink_Fr_ਪਿੱਠ(), cfuncp};
            ccallp->dtype_ਸੈੱਟ_Void();
            beginp->ਜੋੜੋ_ਅਗਲਾ(ccallp->makeStmt());
        } ਨਹੀਂਤਾਂ {
            ccallp = ਨਵਾਂ AstCCall{refp->fileline(), cfuncp};
            ccallp->dtype_ਸੈੱਟ_Void();
            beginp->ਜੋੜੋ_ਅਗਲਾ(ccallp->makeStmt());
        }

        ਜੇ (ਅਚਲ AstFuncRef* ਅਚਲ funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstTaskRef* ਅਚਲ taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp = VN_CAST(refp, ਵਿਧੀ_Call)) {
            ccallp->superReference(ਵਿਧੀ_Callp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : tconnects) {
                AstVar* ਅਚਲ portp = itr.ਪਹਿਲਾ;
                AstArg* ਅਚਲ argp = itr.ਦੂਜਾ;
                connect_ਪੋਰਟ(refp, portp, argp, beginp, ਝੂਠ);
            }
        }
        // First argument is symbol table, then output if a function
        ਅਚਲ ਤਾਰਕਿਕ needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        ਜੇ (needSyms) ccallp->argTypes("vlSymsp");

        ਜੇ (refp->taskp()->dpiContext()) {
            AstScopeName* ਅਚਲ snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlink_Fr_ਪਿੱਠ();
            File_ਸਤਰ* ਅਚਲ flp = refp->fileline();
            // __Vscopep
            ccallp->ਜੋੜੋ_Argsp(snp);
            // __Vfilenamep
            ccallp->ਜੋੜੋ_Argsp(
                ਨਵਾਂ AstCExpr{flp, AstCExpr::ਸ਼ੁੱਧ{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        ਲਈ (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->ਜੋੜੋ_Argsp(argp->exprp()->unlink_Fr_ਪਿੱਠ());
        }

        ਜੇ (outvscp) ccallp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        ਜੇ (debug() >= 9) beginp->dump_ਰੁੱਖ_And_ਅਗਲਾ(ਬਾਹਰ, "-  nitask: ");
        ਵਾਪਸ beginp;
    }

    string dpiSignature(Ast_ਗੰਢ_FTask* nodep, AstVar* rtnvarp) ਅਚਲ {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        ਜੇ (nodep->dpi_ਸ਼ੁੱਧ()) dpiproto += "pure ";
        ਜੇ (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(ਸੱਚ, ਸੱਚ) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string ਦਲੀਲਾਂ;
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                ਜੇ (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    ਜੇ (ਦਲੀਲਾਂ != "") {
                        ਦਲੀਲਾਂ += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ਅਚਲ std::string dpiType = portp->dpiArgType(ਝੂਠ, ਝੂਠ);
                    dpiproto += dpiType;
                    ਅਚਲ std::string vType = portp->dtypep()->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(ਝੂਠ);
                    ਜੇ (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    ਦਲੀਲਾਂ += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        ਵਾਪਸ dpiproto;
    }

    ਸਥਿਰ ਖਾਲੀ checkLegalCIdentifier(Ast_ਗੰਢ* nodep, ਅਚਲ string& name) {
        ਜੇ (name.ਸਿਰਾ() != std::ਲੱਭੋ_if(name.begin(), name.ਸਿਰਾ(), [](ਅੱਖਰ c) {
                ਵਾਪਸ !std::ਅੱਖਰ_ਜਾਂ_ਅੰਕ(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    ਸਥਿਰ Ast_ਗੰਢ* create_Dpi_ਆਰਜ਼ੀ(AstVar* portp, ਅਚਲ string& suffix) {
        ਅਚਲ string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        ਵਾਪਸ ਨਵਾਂ AstCStmt{portp->fileline(), stmt};
    }

    ਖਾਲੀ unlinkAndClone(Ast_ਗੰਢ_FTask* funcp, Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ with_ਅਗਲਾ) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        ਜੇ (funcp->recursive()) {
            VNRelinker relinkHandle;
            ਜੇ (with_ਅਗਲਾ) {
                nodep->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(&relinkHandle);
            } ਨਹੀਂਤਾਂ {
                nodep->unlink_Fr_ਪਿੱਠ(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_ਗੰਢ* ਅਚਲ newp = nodep->clone_ਰੁੱਖ(with_ਅਗਲਾ);
            relinkHandle.relink(newp);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (with_ਅਗਲਾ) {
                nodep->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            } ਨਹੀਂਤਾਂ {
                nodep->unlink_Fr_ਪਿੱਠ();
            }
        }
    }

    ਸਥਿਰ Ast_ਗੰਢ* create_ਸੌਂਪੋ_ਅੰਦਰੂਨੀ_ਮੰਜ਼ਿਲ_Dpi(AstVar* portp, ਤਾਰਕਿਕ isPtr, ਅਚਲ string& frSuffix,
                                              ਅਚਲ string& ਮੰਜ਼ਿਲ_Suffix) {
        ਅਚਲ string stmt = V3Task::ਸੌਂਪੋ_ਅੰਦਰੂਨੀ_ਮੰਜ਼ਿਲ_Dpi(portp, isPtr, frSuffix, ਮੰਜ਼ਿਲ_Suffix);
        ਵਾਪਸ ਨਵਾਂ AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    ਸਥਿਰ Ast_ਗੰਢ_Stmt* createDpiScalarCvt(Ast_ਗੰਢ_ਵਿਅੰਜਕ* internalp, ਅਚਲ std::string& dpi,
                                           ਅਚਲ Ast_ਗੰਢ_DType* dtypep, ਤਾਰਕਿਕ ਮੰਜ਼ਿਲ_Dpi) {
        File_ਸਤਰ* ਅਚਲ flp = internalp->fileline();
        ਅਚਲ AstBasicDType* ਅਚਲ basicp = dtypep->basicp();
        ਜੇ (ਮੰਜ਼ਿਲ_Dpi) {
            AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{flp};
            ਜੇ (basicp->isDpiPrimitive()) {
                cstmtp->ਜੋੜੋ(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->ਜੋੜੋ(internalp);
                cstmtp->ਜੋੜੋ(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } ਨਹੀਂਤਾਂ {
                cstmtp->ਜੋੜੋ((basicp->is_Dpi_ਦੋਆਧਾਰੀ_ਅੰਕ_Vec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::ਮੰਜ਼ਿਲ_string(dtypep->ਚੌੜਾਈ()) + ", " + dpi + ", ");
                cstmtp->ਜੋੜੋ(internalp);
                cstmtp->ਜੋੜੋ(");");
            }
            ਵਾਪਸ cstmtp;
        }
        std::string cvt;
        ਤਾਰਕਿਕ useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_ਮੰਜ਼ਿਲ_ਅੰਦਰੂਨੀ_Cvt_Stmt(dtypep);
        ਜੇ (useSvVec) {
            AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{flp};
            cstmtp->ਜੋੜੋ(cvt);
            cstmtp->ਜੋੜੋ(internalp);
            cstmtp->ਜੋੜੋ(", " + dpi + ");");
            ਵਾਪਸ cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ਅਚਲ ਪੂਰਨ cwidth = basicp->ਚੌੜਾਈ_ਸ਼ਬਦ() * VL_EDATASIZE;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstCExpr{flp, cvt + dpi + ")", cwidth};
        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, internalp, ਨਵਾਂ AstSel{flp, rhsp, 0, dtypep->ਚੌੜਾਈ()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    ਸਥਿਰ Ast_ਗੰਢ_Stmt* createDpiCvt(Ast_ਗੰਢ_ਵਿਅੰਜਕ* internalp, ਅਚਲ std::string& dpi,
                                     ਅਚਲ Ast_ਗੰਢ_DType* dtypep, ਤਾਰਕਿਕ ਮੰਜ਼ਿਲ_Dpi) {
        File_ਸਤਰ* ਅਚਲ flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_ਗੰਢ_Stmt* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            ਲਈ (ਪੂਰਨ i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elemp = ਨਵਾਂ AstArraySel{flp, internalp->clone_ਰੁੱਖ(ਝੂਠ), i};
                stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::ਮੰਜ਼ਿਲ_string(i) + "]",
                                                       arrayp->subDTypep(), ਮੰਜ਼ਿਲ_Dpi));
            }
        } ਨਹੀਂਤਾਂ ਜੇ (dtypep->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct()) {
            ਲਈ (ਅਚਲ AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ਅਚਲ selp
                    = ਨਵਾਂ AstStructSel{flp, internalp->clone_ਰੁੱਖ(ਝੂਠ), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), ਮੰਜ਼ਿਲ_Dpi));
            }
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ createDpiScalarCvt(internalp, dpi, dtypep, ਮੰਜ਼ਿਲ_Dpi);
        }
        VL_DO_DANGLING(internalp->delete_ਰੁੱਖ(), internalp);
        ਵਾਪਸ stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    ਸਥਿਰ Ast_ਗੰਢ_Stmt* createDpiArgCvt(AstVarScope* vscp, ਅਚਲ std::string& dpiName,
                                        ਤਾਰਕਿਕ ਮੰਜ਼ਿਲ_Dpi) {
        AstVar* ਅਚਲ varp = vscp->varp();
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        ਅਚਲ std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ਅਚਲ ਪੂਰਨ ਕੁੱਲ = strides.ਖਾਲੀ_ਥਾਵਾਂ() ? 1 : strides[0].ਪਹਿਲਾ * strides[0].ਦੂਜਾ;
        ਅਚਲ ਤਾਰਕਿਕ isStruct = varp->dtypep()->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct();
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_ਗੰਢ_Stmt* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਪੂਰਨ i = 0; i < ਕੁੱਲ; ++i) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* internalp
                = ਨਵਾਂ AstVarRef{flp, vscp, ਮੰਜ਼ਿਲ_Dpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& stride : strides) {
                internalp = ਨਵਾਂ AstArraySel(flp, internalp, (i / stride.ਦੂਜਾ) % stride.ਪਹਿਲਾ);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            ਜੇ (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::ਮੰਜ਼ਿਲ_string(i * varp->basicp()->ਚੌੜਾਈ_ਸ਼ਬਦ());
            } ਨਹੀਂਤਾਂ ਜੇ (isStruct || !strides.ਖਾਲੀ_ਥਾਵਾਂ()) {
                dpi += "[" + std::ਮੰਜ਼ਿਲ_string(i) + "]";
            }
            stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, ਮੰਜ਼ਿਲ_Dpi));
        }
        ਵਾਪਸ stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_ਗੰਢ_FTask* ਅਚਲ nodep, AstVar* ਅਚਲ rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਅਚਲ std::string cname = Ast_ਗੰਢ::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ਅਚਲ ਅੱਖਰ* ਅਚਲ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ਅਚਲ std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(ਸੱਚ, ਸੱਚ) : "";
        // The function we are building
        AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(ਸੱਚ);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(ਸੱਚ);
        funcp->ਇੰਦਰਾਜ_ਬਿੰਦੂ(ਸੱਚ);
        funcp->isStatic(ਸੱਚ);
        funcp->protect(ਝੂਠ);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_ਸਿਖਰ_Scopep->scopep()->ਜੋੜੋ_Blocksp(funcp);
        // Check the arguments before converting them
        ਜੇ (!make_ਪੋਰਟ_List(nodep, funcp)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        // Note this function may dispatch on a different class.
        // Thus we need to be careful not to assume a particular function layout.
        //
        // Func numbers must be the same for each function, even when there are
        // completely different models with the same function name.
        // Thus we can't just use a constant computed at Verilation time.
        // We could use 64-bits of a MD5/SHA hash rather than a string here,
        // but the compare is only done on first call then memoized, so
        // it's not worth optimizing.

        // Peramble - fetch the exproted function from the scope table
        AstCStmt* ਅਚਲ prep = ਨਵਾਂ AstCStmt{flp};
        funcp->ਜੋੜੋ_Stmtsp(prep);
        // Static doesn't need save-restore as if below will re-fill proper value
        prep->ਜੋੜੋ("static int __Vfuncnum = -1;\n");
        // First time init (faster than what the compiler does if we did a singleton
        prep->ਜੋੜੋ("if (VL_UNLIKELY(__Vfuncnum == -1)) {\n");
        prep->ਜੋੜੋ("__Vfuncnum = Verilated::exportFuncNum(\"" + nodep->cname() + "\");\n");
        prep->ਜੋੜੋ("}\n");
        // If the find fails, it will throw an error
        prep->ਜੋੜੋ("const VerilatedScope* const __Vscopep = Verilated::dpiScope();\n");
        // If 'dpiScope()' fails and '__Vscopep' is null; the exportFind function throws an error
        // If __Vcb is null the exportFind function throws and error
        ਅਚਲ std::string cbtype
            = VIdProtect::protect(v3Global.opt.ਅਗੇਤਰ() + "__Vcb_" + nodep->cname() + "_t");
        prep->ਜੋੜੋ(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ਅਚਲ callp = ਨਵਾਂ AstCStmt{flp};
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_Func_Arg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ਅਚਲ vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(ਝੂਠ);
            portp->protect(ਝੂਠ);
            // Add argument to call
            ਅਚਲ VAccess access = portp->direction().pinAccess();
            callp->ਜੋੜੋ(", ");
            callp->ਜੋੜੋ(ਨਵਾਂ AstVarRef{portp->fileline(), vscp, access});
            ਵਾਪਸ vscp;
        };
        // Call callback
        callp->ਜੋੜੋ("(*__Vcb)(");
        // First argument is the Syms
        callp->ਜੋੜੋ("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_ਗੰਢ* ਬਾਹਰਲਾ_Cvtsp = ਖਾਲੀ_ਸੂਚਕ;  // Conversions of output/inout arguments, after the call
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var);
            ਜੇ (!portp) ਜਾਰੀ;
            ਜੇ (!portp->isIO()) ਜਾਰੀ;
            ਜੇ (portp->isFuncReturn()) ਜਾਰੀ;
            ਜੇ (portp == rtnvarp) ਜਾਰੀ;  // Handled below
            // Add argument to call
            AstVarScope* ਅਚਲ outvscp = ਜੋੜੋ_Func_Arg(portp);
            ਅਚਲ ਤਾਰਕਿਕ isStruct = portp->dtypep()->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct();
            ਜੇ (portp->isWritable()) {
                ਬਾਹਰਲਾ_Cvtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
                    ਬਾਹਰਲਾ_Cvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), ਸੱਚ)
                                       : create_ਸੌਂਪੋ_ਅੰਦਰੂਨੀ_ਮੰਜ਼ਿਲ_Dpi(portp, ਸੱਚ, tmpSuffixp, ""));
            }
            ਜੇ (!portp->isNonOutput()) ਜਾਰੀ;
            // Convert input/inout arguments to dpi type
            ਅਚਲ std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(ਝੂਠ).ਦੂਜਾ == 0
                      ? "*"
                      : "";
            funcp->ਜੋੜੋ_Stmtsp(createDpiArgCvt(outvscp, deref + portp->name(), ਝੂਠ));
        }
        // Return value argument goes last
        ਜੇ (rtnvarp) ਜੋੜੋ_Func_Arg(rtnvarp);
        // Close statement
        callp->ਜੋੜੋ(");");
        // Call the user function
        funcp->ਜੋੜੋ_Stmtsp(callp);
        // Convert output/inout arguments back to internal type
        ਜੇ (ਬਾਹਰਲਾ_Cvtsp) funcp->ਜੋੜੋ_Stmtsp(ਬਾਹਰਲਾ_Cvtsp);
        // Convert return value
        ਜੇ (rtnvarp) {
            funcp->ਜੋੜੋ_Stmtsp(create_Dpi_ਆਰਜ਼ੀ(rtnvarp, ""));
            funcp->ਜੋੜੋ_Stmtsp(create_ਸੌਂਪੋ_ਅੰਦਰੂਨੀ_ਮੰਜ਼ਿਲ_Dpi(rtnvarp, ਝੂਠ, tmpSuffixp, ""));
            ਅਚਲ std::string ਸੂਚਕਾਂਕ = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{flp, "return " + rtnvarp->name() + ਸੂਚਕਾਂਕ + ";"});
        }
        ਵਾਪਸ funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_ਗੰਢ_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ਅਚਲ string name = Ast_ਗੰਢ::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ਅਚਲ string rtnType = rtnvarp            ? rtnvarp->dpiArgType(ਸੱਚ, ਸੱਚ)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(ਸੱਚ);
        funcp->dontCombine(ਸੱਚ);
        funcp->ਇੰਦਰਾਜ_ਬਿੰਦੂ(ਝੂਠ);
        funcp->is_ਵਿਧੀ(ਝੂਠ);
        funcp->protect(ਝੂਠ);
        funcp->dpi_ਸ਼ੁੱਧ(nodep->dpi_ਸ਼ੁੱਧ());

        ਅਚਲ ਪੂਰਨ ਲਾਗਤ = static_cast<ਪੂਰਨ>(V3Control::get_Profile_ਅੰਕੜੇ(funcp->name()));
        m_statHierDpisWithCosts += (ਲਾਗਤ != 0);
        funcp->ਲਾਗਤ(ਲਾਗਤ);

        // Add DPI Import to top, since it's a global function
        m_ਸਿਖਰ_Scopep->scopep()->ਜੋੜੋ_Blocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        ਜੇ (!make_ਪੋਰਟ_List(nodep, funcp)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ funcp;
    }

    AstCFunc* getDpiFunc(Ast_ਗੰਢ_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ਅਚਲ string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_dpi_ਨਾਂ.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, ਖਾਲੀ_ਸੂਚਕ));
        ਜੇ (pair.ਦੂਜਾ) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ਅਚਲ funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.ਪਹਿਲਾ->ਦੂਜਾ) = funcp;
            ਵਾਪਸ funcp;
        } ਨਹੀਂਤਾਂ {
            // Seen this cname import before. Check if it's the same prototype.
            ਅਚਲ Ast_ਗੰਢ_FTask* ਪਹਿਲਾ_Nodep;
            string ਪਹਿਲਾ_Signature;
            AstCFunc* ਪਹਿਲਾ_Funcp;
            std::tie(ਪਹਿਲਾ_Nodep, ਪਹਿਲਾ_Signature, ਪਹਿਲਾ_Funcp) = pair.ਪਹਿਲਾ->ਦੂਜਾ;
            ਜੇ (signature != ਪਹਿਲਾ_Signature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warn_ਹੋਰ()  //
                               << "... New signature:      " << signature << '\n'  //
                               << ਪਹਿਲਾ_Nodep->warnOther()
                               << "... Original signature: " << ਪਹਿਲਾ_Signature << '\n'  //
                               << ਪਹਿਲਾ_Nodep->warnContextSecondary());
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            } ਨਹੀਂਤਾਂ {
                // Same signature, return the previously created CFunc
                ਵਾਪਸ ਪਹਿਲਾ_Funcp;
            }
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ make_ਪੋਰਟ_List(Ast_ਗੰਢ_FTask* nodep, AstCFunc* dpip) {
        ਤਾਰਕਿਕ allOk = ਸੱਚ;
        // Copy nodep's list of function I/O to the new dpip c function
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                ਜੇ (portp->isIO()) {
                    // Move it to new function
                    AstVar* ਅਚਲ newPortp = portp->clone_ਰੁੱਖ(ਝੂਠ);
                    newPortp->funcLocal(ਸੱਚ);
                    dpip->ਜੋੜੋ_Argsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    ਜੇ (!portp->basicp() && !portp->dtypep()->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct()) {
                        allOk = ਝੂਠ;
                        portp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: DPI argument of type "
                                << portp->dtypep()->prettyTypeName() << '\n'
                                << portp->warn_ਹੋਰ()
                                << "... For best portability, use bit, byte, int, or longint");
                        // We don't warn on logic either, although the 4-stateness is lost.
                        // That's what other simulators do.
                    }
                }
            }
        }
        ਵਾਪਸ allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    ਸਥਿਰ ਤਾਰਕਿਕ isDpiCIdentifier(ਅਚਲ string& name) {
        ਵਾਪਸ !name.ਖਾਲੀ_ਥਾਵਾਂ() && VString::isIdentifier(name)
               && !std::ਅੰਕ_ਹੈ(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_ਸ਼ਬਦ::isKeyword(name).ਖਾਲੀ_ਥਾਵਾਂ();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    ਤਾਰਕਿਕ checkDpiStruct(ਅਚਲ AstVar* portp, ਅਚਲ AstStructDType* sdtypep, ਤਾਰਕਿਕ nested) {
        ਅਚਲ std::string name = sdtypep->dpiName();
        ਜੇ (!(nested && name.ਖਾਲੀ_ਥਾਵਾਂ()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(ਸੱਚ) << "\n"
                              << portp->warn_ਹੋਰ()
                              << "... Suggest use a typedef with a C identifier name");
            ਵਾਪਸ ਝੂਠ;
        }
        ਲਈ (ਅਚਲ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ਜੇ (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                ਵਾਪਸ ਝੂਠ;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ਅਚਲ Ast_ਗੰਢ_DType* dtypep = itemp->subDTypep()->skipRefp();
            ਜਦੋਂ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ subp = VN_CAST(dtypep, ਗੰਢ_UOr_Struct_DType);
            ਅਚਲ AstBasicDType* ਅਚਲ basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ਅਚਲ ਤਾਰਕਿਕ isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            ਜੇ (dtypep->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct()) {
                ਜੇ (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), ਸੱਚ)) ਵਾਪਸ ਝੂਠ;
            } ਨਹੀਂਤਾਂ ਜੇ (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(ਸੱਚ)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                ਵਾਪਸ ਝੂਠ;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        ਜੇ (name.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਸੱਚ;
        ਅਚਲ AstStructDType* ਅਚਲ firstp = m_dpiStructs.emplace(name, sdtypep).ਪਹਿਲਾ->ਦੂਜਾ;
        ਜੇ (firstp->dpiTypedef() == sdtypep->dpiTypedef()) ਵਾਪਸ ਸੱਚ;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        ਵਾਪਸ ਝੂਠ;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    ਤਾਰਕਿਕ check_Dpi_Struct_ਦਲੀਲਾਂ(ਅਚਲ Ast_ਗੰਢ_FTask* nodep) {
        ਤਾਰਕਿਕ allOk = ਸੱਚ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਅਚਲ AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var);
            ਜੇ (portp && portp->isIO() && portp->dtypep()->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       ਝੂਠ)
                        && allOk;
            }
        }
        ਵਾਪਸ allOk;
    }

    ਖਾਲੀ bodyDpiImportFunc(Ast_ਗੰਢ_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ਅਚਲ ਅੱਖਰ* ਅਚਲ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        ਜੇ (v3Global.opt.profExec()) {
            cfuncp->ਜੋੜੋ_Stmtsp(AstCStmt::prof_Exec_Section_ਧੱਕੋ(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string ਦਲੀਲਾਂ;
        ਲਈ (Ast_ਗੰਢ* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ਅਚਲ portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                ਜੇ (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    ਜੇ (ਦਲੀਲਾਂ != "") ਦਲੀਲਾਂ += ", ";

                    ਜੇ (portp->isDpiOpenArray()) {
                        Ast_ਗੰਢ_DType* ਅਚਲ dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, ਕਤਾਰ_DType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ਅਚਲ string propName = portp->name() + "__Vopenprops";
                        ਅਚਲ string prop_ਕੋਡ = portp->vlPropDecl(propName);
                        cfuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{portp->fileline(), prop_ਕੋਡ});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ਅਚਲ string name = portp->name() + "__Vopenarray";
                        ਅਚਲ string var_ਕੋਡ
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{portp->fileline(), var_ਕੋਡ});
                        ਦਲੀਲਾਂ += "&" + name;
                    } ਨਹੀਂਤਾਂ {
                        // Unpacked structures are passed by reference from a C array
                        ਅਚਲ ਤਾਰਕਿਕ isStruct = portp->dtypep()->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct();
                        ਜੇ (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            ਜੇ (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) ਦਲੀਲਾਂ += "&";
                        }

                        ਦਲੀਲਾਂ += portp->name() + tmpSuffixp;

                        cfuncp->ਜੋੜੋ_Stmtsp(create_Dpi_ਆਰਜ਼ੀ(portp, tmpSuffixp));
                        ਜੇ (portp->isNonOutput()) {
                            cfuncp->ਜੋੜੋ_Stmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ਸੱਚ)
                                    : create_ਸੌਂਪੋ_ਅੰਦਰੂਨੀ_ਮੰਜ਼ਿਲ_Dpi(portp, ਝੂਠ, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        ਜੇ (nodep->dpiContext()) {
            ਅਚਲ string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtype_ਸੈੱਟ_Void();
            callp->argTypes(ਦਲੀਲਾਂ);
            ਜੇ (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->ਜੋੜੋ_Stmtsp(create_Dpi_ਆਰਜ਼ੀ(rtnvscp->varp(), tmpSuffixp));
                ਅਚਲ std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{nodep->fileline()};
                cstmtp->ਜੋੜੋ(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->ਜੋੜੋ(" = ");
                cstmtp->ਜੋੜੋ(callp);  // RHS
                cstmtp->ਜੋੜੋ(";");
                cfuncp->ਜੋੜੋ_Stmtsp(cstmtp);
            } ਨਹੀਂਤਾਂ {
                // Othervise just call it
                cfuncp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        ਲਈ (Ast_ਗੰਢ* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                portp->protect(ਝੂਠ);  // No additional exposure - already part of shown proto
                ਜੇ (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ਅਚਲ portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->ਜੋੜੋ_Stmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ਝੂਠ));
                }
            }
        }

        ਜੇ (v3Global.opt.profExec()) {
            cfuncp->ਜੋੜੋ_Stmtsp(AstCStmt::prof_Exec_Section_ਕੱਢੋ(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        ਜੇ (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        ਵਾਪਸ dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_ਗੰਢ_FTask* nodep, ਤਾਰਕਿਕ ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_ਗੰਢ::user2_Clear_ਰੁੱਖ();
        AstVar* rtnvarp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->isFunction()) {
            AstVar* ਅਚਲ portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            ਜੇ (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ਅਚਲ bdtypep = portp->dtypep()->basicp();
                ਜੇ (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(ਸੱਚ)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                }
                ਜੇ (!bdtypep->isDpiPrimitive()) {
                    ਜੇ (bdtypep->is_Dpi_ਦੋਆਧਾਰੀ_ਅੰਕ_Vec() && portp->ਚੌੜਾਈ() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warn_ਹੋਰ()
                                       + "... Suggest make it an output argument instead?");
                    }
                    ਜੇ (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->task_ਜਨਤਕ()) {
                ਜੇ (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warn_ਹੋਰ()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            ਜੇ (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(ਝੂਠ);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, ਝੂਠ);
            rtnvarp = portp;
            rtnvarp->funcLocal(ਸੱਚ);
            rtnvarp->noCReset(ਸੱਚ);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            ਜੇ (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(ਝੂਠ);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpi_Open_ਮਾਪਾ() && nodep->dpi_Open_ਬੱਚਾ()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = check_Dpi_Struct_ਦਲੀਲਾਂ(nodep) ? getDpiFunc(nodep, rtnvarp) : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!dpiFuncp || (nodep->dpiImport() && nodep->dpi_Open_ਮਾਪਾ())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                ਜੇ (rtnvarp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
        }

        AstVarScope* rtnvscp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (rtnvarp) {
            rtnvscp = ਨਵਾਂ AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->ਜੋੜੋ_Varsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string ਅਗੇਤਰ;
        ਜੇ (nodep->dpiImport()) {
            ਅਗੇਤਰ = "__Vdpiimwrap_";
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->dpiExport()) {
            ਅਗੇਤਰ = "__Vdpiexp_";
        } ਨਹੀਂਤਾਂ ਜੇ (ftaskNoInline) {
            ਅਗੇਤਰ = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        ਜੇ (!nodep->task_ਜਨਤਕ() && !nodep->class_ਵਿਧੀ()) suffix = "_" + m_scopep->nameDotless();
        ਅਚਲ string name = ((nodep->name() == "new") ? "new" : ਅਗੇਤਰ + nodep->name() + suffix);
        AstCFunc* ਅਚਲ cfuncp = ਨਵਾਂ AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->task_ਜਨਤਕ() && rtnvarp) ? rtnvarp->cPubArgType(ਸੱਚ, ਸੱਚ) : "")};
        // It's ok to combine imports because this is just a wrapper;
        // duplicate wrappers can get merged.
        cfuncp->dontCombine(!nodep->dpiImport());
        cfuncp->ਇੰਦਰਾਜ_ਬਿੰਦੂ(!nodep->dpiImport());
        cfuncp->func_ਜਨਤਕ(nodep->task_ਜਨਤਕ());
        cfuncp->dpiContext(nodep->dpiContext());
        cfuncp->dpiExportImpl(nodep->dpiExport());
        cfuncp->dpiImportWrapper(nodep->dpiImport());
        cfuncp->recursive(nodep->recursive());
        // Hardcoded based on UVM usage; TODO make a verilated_std.vlt control for these
        cfuncp->unlikely(nodep->name() == "uvm_report_error" || nodep->name() == "uvm_report_info"
                         || nodep->name() == "uvm_report_fatal"
                         || nodep->name() == "uvm_report_warning");
        ਜੇ (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(ਸੱਚ);
            cfuncp->isLoose(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {
            cfuncp->isStatic(ਝੂਠ);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpi_ਸ਼ੁੱਧ(nodep->dpi_ਸ਼ੁੱਧ());
        ਜੇ (nodep->name() == "new") cfuncp->isConstructor(ਸੱਚ);
        ਜੇ (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(ਸੱਚ);
        ਜੇ (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        ਜੇ (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ਅਚਲ ਤਾਰਕਿਕ needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->task_ਜਨਤਕ()) || v3Global.opt.profExec());
        ਜੇ (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        ਜੇ (!nodep->dpiImport() && !nodep->task_ਜਨਤਕ()) {
            // Need symbol table
            ਜੇ (cfuncp->isConstructor()) {
                ਤਾਰਕਿਕ isInterfaceClass = ਝੂਠ;
                ਜੇ (ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                ਜੇ (!isInterfaceClass) {
                    ਅਚਲ string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->ਜੋੜੋ_Stmtsp(
                        ਨਵਾਂ AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        ਜੇ (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_ਨਿਵੇਸ਼_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_ਨਿਵੇਸ਼_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_ਨਿਵੇਸ਼_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        ਜੇ (nodep->dpiExport()) {
            AstScopeName* ਅਚਲ snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(ਸੱਚ);
            snp->unlink_Fr_ਪਿੱਠ();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        ਲਈ (Ast_ਗੰਢ *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                ਜੇ (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlink_Fr_ਪਿੱਠ();
                    ਧੱਕੋ_Deletep(portp);
                    portp->user2p(const_Pool_ਸਾਰਣੀ(portp));
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, ਝੂਠ);
                        portp->funcLocal(ਸੱਚ);
                        ਜੇ (portp->valuep()) ਧੱਕੋ_Deletep(portp->valuep()->unlink_Fr_ਪਿੱਠ());
                        cfuncp->ਜੋੜੋ_Argsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        ਜੇ (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = portp->dtypep()->skipRefp();
                            ਜੇ (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } ਨਹੀਂਤਾਂ {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(ਸੱਚ);
                    }
                    AstVarScope* ਅਚਲ newvscp
                        = ਨਵਾਂ AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->ਜੋੜੋ_Varsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        ਜੇ (rtnvarp) cfuncp->ਜੋੜੋ_Argsp(rtnvarp);

        // Move body
        Ast_ਗੰਢ* bodysp = nodep->stmtsp();
        ਜੇ (bodysp) {
            unlinkAndClone(nodep, bodysp, ਸੱਚ);
            AstBegin* ਅਚਲ tempp
                = ਨਵਾਂ AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, ਝੂਠ};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            ਲਈ (Ast_ਗੰਢ *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                    ਜੇ (portp->isIO()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(portp->unlink_Fr_ਪਿੱਠ()), portp);
                }
            }
            ਜੇ (tempp->stmtsp()) cfuncp->ਜੋੜੋ_Stmtsp(tempp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            VL_DO_DANGLING(tempp->delete_ਰੁੱਖ(), tempp);
        }
        ਜੇ (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        ਜੇ (rtnvscp && nodep->task_ਜਨਤਕ()) {
            cfuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCReturn{
                rtnvscp->fileline(), ਨਵਾਂ AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        ਜੇ (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            ਤਾਰਕਿਕ writesNonLocals = ਝੂਠ;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* ਅਚਲ varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                ਜੇ (varp->isFuncLocal()) ਵਾਪਸ;
                // Check if written
                ਜੇ (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->ਸੈੱਟ_Written_By_Dpi();
                    // Remember we had some
                    writesNonLocals = ਸੱਚ;
                }
                // Check if read
                ਜੇ (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->ਸੈੱਟ_Read_By_Dpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            ਜੇ (writesNonLocals) {
                AstVarScope* ਅਚਲ dpiExportTriggerp = getDpiExporTrigger();
                File_ਸਤਰ* ਅਚਲ flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                Ast_ਸੌਂਪੋ* ਅਚਲ assignp
                    = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                ਜੇ (v3Global.opt.systemC()) {
                    assignp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        ਜੇ (nodep->needProcess()) cfuncp->ਸੈੱਟ_Need_Process();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        ਵਾਪਸ cfuncp;
    }

    ਖਾਲੀ iterateIntoFTask(Ast_ਗੰਢ_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    ਖਾਲੀ ਪਾਓ_Before_Stmt(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
    }
    ਖਾਲੀ process_ਦਲੀਲਾਂ(Ast_ਗੰਢ_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        ਲਈ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ਅਚਲ arrayp = VN_CAST(argp->exprp(), ConsArray);
            ਜੇ (!arrayp) ਜਾਰੀ;

            File_ਸਤਰ* ਅਚਲ flp = arrayp->fileline();
            AstVarScope* ਅਚਲ substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);

            Ast_ਸੌਂਪੋ* ਅਚਲ assignp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlink_Fr_ਪਿੱਠ()};

            Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ exprstmtp = ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{
                flp, assignp, ਨਵਾਂ AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    ਤਾਰਕਿਕ isIfaceLocalImpure(Ast_ਗੰਢ_FTask* nodep, Ast_ਗੰਢ* impurep) {
        ਅਚਲ AstScope* ਅਚਲ scopep = m_statep->getScope(nodep);
        ਜੇ (!TaskStateVisitor::isIfaceFTaskScope(scopep)) ਵਾਪਸ ਝੂਠ;

        ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(impurep, VarRef);
        ਜੇ (!refp || !refp->varScopep()) ਵਾਪਸ ਝੂਠ;

        ਵਾਪਸ refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) {
            // Make sure that the return expression is converted only once
            ਵਾਪਸ;
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ withExprp = VN_AS(nodep->exprp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
        nodep->ਜੋੜੋ_Exprp(ਨਵਾਂ AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCNew* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_New);
        m_ਵਿੱਚ_New = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (m_ਵਿੱਚ_Sensesp && !nodep->is_ਸ਼ੁੱਧ()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(ਖਾਲੀ_ਸੂਚਕ);  // So V3Broken doesn't complain
            ਵਾਪਸ;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        ਜੇ (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_ਗਿਣਤੀ(), nodep, "No non-inline task, but no error issued");
            ਜੇ (VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            } ਨਹੀਂਤਾਂ {
                nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
            ਵਾਪਸ;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->taskp()->isFunction()) {
            AstVar* ਅਚਲ fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            ਜੇ (Ast_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(nodep->backp(), ਸੌਂਪੋ)) {
                ਜੇ (AstVarRef* ਅਚਲ lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ਅਚਲ vscp = lhsp->varScopep();
                    ਜੇ (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_ਰੁੱਖ(
                            fvarp->dtypep()->skipRefp())
                        && !has_Ref_ਦਲੀਲ(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            ਜੇ (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_ਗੰਢ* beginp;
        AstCNew* cnewp = ਖਾਲੀ_ਸੂਚਕ;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ਅਚਲ ਤਾਰਕਿਕ virtualIfaceCall
            = TaskStateVisitor::is_Virtual_Iface_ਵਿਧੀ_Call(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        ਜੇ (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            process_ਦਲੀਲਾਂ(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } ਨਹੀਂਤਾਂ {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        ਜੇ (VN_IS(nodep, New)) {  // New not legal as while() condition
            ਪਾਓ_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->backp(), ਗੰਢ_ਸੌਂਪੋ)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            ਪਾਓ_Before_Stmt(nodep, beginp);
            AstVarRef* ਅਚਲ outrefp = ਨਵਾਂ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ਅਚਲ outrefp = ਨਵਾਂ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_ਵਿਅੰਜਕ_Stmt* lambdap = ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{nodep->fileline(), beginp, outrefp};

            ਜੇ (m_ਵਿੱਚ_New) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_ਰੁੱਖ(ਝੂਠ);
                newvarp->funcLocal(ਸੱਚ);

                // Create a new VarScope for the new variable
                AstVarScope* ਅਚਲ newvscp
                    = ਨਵਾਂ AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->ਜੋੜੋ_Varsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreach_And_ਅਗਲਾ([outvscp, newvscp](AstVarRef* refp) {
                    ਜੇ (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->has_ਨਤੀਜਾ(ਝੂਠ);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ਅਚਲ exprp = ਨਵਾਂ AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtype_ਸੈੱਟ_String();
                lambdap->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            VIsCached::clear_Cache_ਰੁੱਖ();
        } ਨਹੀਂਤਾਂ {  // VN_IS(nodep->backp(), StmtExpr)
            ਪਾਓ_Before_Stmt(nodep, beginp);
            ਜੇ (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_ਕੋਡ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* ਅਚਲ nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        ਜੇ (!nodep->user1_ਸੈੱਟ_Once()) {  // Just one creation needed per function
            // Expand functions in it
            ਪੂਰਨ modes = 0;
            ਜੇ (nodep->dpiImport()) modes++;
            ਜੇ (nodep->dpiExport()) modes++;
            ਜੇ (nodep->task_ਜਨਤਕ()) modes++;
            ਜੇ (nodep->class_ਵਿਧੀ()) modes++;
            ਜੇ (v3Global.opt.protectIds() && nodep->task_ਜਨਤਕ()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            ਜੇ (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            ਅਚਲ ਤਾਰਕਿਕ noInline = m_statep->ftaskNoInline(nodep);
            ਅਚਲ ਤਾਰਕਿਕ needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            ਜੇ (noInline && !nodep->class_ਵਿਧੀ() && !nodep->recursive()) {
                ਜੇ (Ast_ਗੰਢ* ਅਚਲ impurep = m_statep->checkImpure(nodep)) {
                    ਜੇ (!isIfaceLocalImpure(nodep, impurep)) {
                        nodep->v3warn(IMPURE, "Unsupported: External variable referenced by "
                                              "non-inlined function/task: "
                                                  << nodep->prettyNameQ() << '\n'
                                                  << nodep->warnContextPrimary() << '\n'
                                                  << impurep->warnOther()
                                                  << "... Location of the external reference: "
                                                  << impurep->prettyNameQ() << '\n'
                                                  << impurep->warnContextSecondary());
                    }
                }
            }

            ਜੇ (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->task_ਜਨਤਕ()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_ਗੰਢ_FTask* ਅਚਲ clonedFuncp = nodep->clone_ਰੁੱਖ(ਝੂਠ);
                ਜੇ (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ਅਚਲ ਤਾਰਕਿਕ makeNonInlineFunc = noInline || needsNonInlineCFunc;
                ਜੇ (AstCFunc* ਅਚਲ cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(cfuncp);
                    ਜੇ (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            ਜੇ (nodep->isFunction()) {
                ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* ਅਚਲ vscp = m_statep->ਲੱਭੋ_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(vscp->unlink_Fr_ਪਿੱਠ()), vscp);
                }
            }
            ਲਈ (Ast_ਗੰਢ *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ਅਚਲ vscp = m_statep->ਲੱਭੋ_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(vscp->unlink_Fr_ਪਿੱਠ()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Force* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_ਗੰਢ_FTask_Ref* refp) { refs.ਧੱਕੋ_ਪਿੱਠ(refp); });
        ਲਈ (Ast_ਗੰਢ_FTask_Ref* ਅਚਲ refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ਅਚਲ interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
            Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{
                nodep->fileline(), ਨਵਾਂ Ast_Sen_ਵਸਤੂ{nodep->fileline(), Ast_Sen_ਵਸਤੂ::Combo{}}};
            Ast_ਸਰਗਰਮ* ਅਚਲ activep
                = ਨਵਾਂ Ast_ਸਰਗਰਮ{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_ਰੁੱਖ_Storep(activep->sentreep());
            activep->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, ਖਾਲੀ_ਸੂਚਕ, assignp});
            m_scopep->ਜੋੜੋ_Blocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(ਨਵਾਂ AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_ਰੁੱਖ(), refp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        ਜੇ (!nodep->exprp()) VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        UASSERT_OBJ(!m_ਵਿੱਚ_Sensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_ਵਿੱਚ_Sensesp);
        m_ਵਿੱਚ_Sensesp = ਸੱਚ;
        iterateChildren(nodep);
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    TaskVisitor(AstNetlist* nodep, TaskStateVisitor* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~TaskVisitor() {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Functions inlined", m_statInlines);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Hierarchical DPI wrappers with costs",
                         m_statHierDpisWithCosts);
    }
};

//######################################################################
// Mark interface members under timing controls of interface CFuncs as interface-sensed

ਵਰਗ TaskIfaceSensVisitor final : ਜਨਤਕ VNVisitorConst {
    // STATE
    ਤਾਰਕਿਕ m_underIfaceFunc = ਝੂਠ;  // Under a CFunc owned by an interface scope
    ਤਾਰਕਿਕ m_underSenses = ਝੂਠ;  // Under a sensitivity expression of such a CFunc

    // METHODS
    ਖਾਲੀ markSensesAndIterate(Ast_ਗੰਢ* nodep) {
        ਜੇ (!m_underIfaceFunc) ਵਾਪਸ;
        VL_RESTORER(m_underSenses);
        m_underSenses = ਸੱਚ;
        iterate_And_ਅਗਲਾ_Const_Null(nodep);
    }
    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਰੁੱਖ* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    ਖਾਲੀ ਦੇਖੋ(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (!m_underSenses) ਵਾਪਸ;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        ਜੇ (nodep->varp()->isFuncLocal()) ਵਾਪਸ;
        ਜੇ (AstIface* ਅਚਲ ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = ਮੂਲ;
};

//######################################################################
// Task class functions

ਅਚਲ ਅੱਖਰ* ਅਚਲ V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_ਗੰਢ_FTask_Ref* nodep, Ast_ਗੰਢ* taskStmtsp,
                                    V3TaskConnectState* statep, ਤਾਰਕਿਕ makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ਅਚਲ std::string, ਪੂਰਨ> name_ਮੰਜ਼ਿਲ_ਸੂਚਕਾਂਕ;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    ਪੂਰਨ tpinnum = 0;
    AstVar* sformatp = ਖਾਲੀ_ਸੂਚਕ;
    ਲਈ (Ast_ਗੰਢ* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
            ਜੇ (portp->isIO()) {
                tconnects.emplace_ਪਿੱਠ(portp, static_cast<AstArg*>(ਖਾਲੀ_ਸੂਚਕ));
                name_ਮੰਜ਼ਿਲ_ਸੂਚਕਾਂਕ.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                ਜੇ (portp->attrSFormat()) {
                    sformatp = portp;
                } ਨਹੀਂਤਾਂ ਜੇ (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    ਪੂਰਨ ppinnum = 0;
    ਤਾਰਕਿਕ reorganize = ਝੂਠ;
    ਲਈ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        ਜੇ (!argp->name().ਖਾਲੀ_ਥਾਵਾਂ()) {
            // By name
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = name_ਮੰਜ਼ਿਲ_ਸੂਚਕਾਂਕ.ਲੱਭੋ(argp->name());
            ਜੇ (it == name_ਮੰਜ਼ਿਲ_ਸੂਚਕਾਂਕ.ਸਿਰਾ()) {
                ਜੇ (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warn_ਹੋਰ() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), argp);
                }
            } ਨਹੀਂਤਾਂ {
                ਜੇ (tconnects[it->ਦੂਜਾ].ਦੂਜਾ && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->ਦੂਜਾ].ਦੂਜਾ->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                }
                tconnects[it->ਦੂਜਾ].ਦੂਜਾ = argp;
                reorganize = ਸੱਚ;
            }
        } ਨਹੀਂਤਾਂ {  // By pin number
            ਜੇ (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } ਨਹੀਂਤਾਂ ਜੇ (ppinnum >= tpinnum) {
                ਜੇ (sformatp) {
                    tconnects.emplace_ਪਿੱਠ(sformatp, static_cast<AstArg*>(ਖਾਲੀ_ਸੂਚਕ));
                    tconnects[ppinnum].ਦੂਜਾ = argp;
                    ++tpinnum;
                } ਨਹੀਂਤਾਂ ਜੇ (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warn_ਹੋਰ() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), argp);
                }
            } ਨਹੀਂਤਾਂ {
                tconnects[ppinnum].ਦੂਜਾ = argp;
            }
        }
        ++ppinnum;
    }

    ਜੇ (!makeChanges) ਵਾਪਸ tconnects;

    // Connect missing ones
    VInsertion_ਸੈੱਟ<ਅਚਲ AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    ਲਈ (ਪੂਰਨ i = 0; i < tpinnum; ++i) {
        AstVar* ਅਚਲ portp = tconnects[i].ਪਹਿਲਾ;
        ਜੇ (!tconnects[i].ਦੂਜਾ || !tconnects[i].ਦੂਜਾ->exprp()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newvaluep = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } ਨਹੀਂਤਾਂ ਜੇ (AstFuncRef* ਅਚਲ funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ funcp = funcRefp->taskp();
                ਜੇ (funcp->class_ਵਿਧੀ() && funcp->isStatic()) newvaluep = funcRefp;
            } ਨਹੀਂਤਾਂ ਜੇ (AstConst* ਅਚਲ constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            ਜੇ (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), ਗੰਢ_ਵਿਅੰਜਕ));
                ਜੇ (!VN_IS(newvaluep, Const)) {
                    ਜੇ (statep) {
                        portp->pin_ਅੰਕ(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.ਪਾਓ(portp);
                    } ਨਹੀਂਤਾਂ {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_ਰੁੱਖ(ਸੱਚ) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ਅਚਲ newp = ਨਵਾਂ AstArg{nodep->fileline(), portp->name(), newvaluep};
            ਜੇ (tconnects[i].ਦੂਜਾ) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].ਦੂਜਾ->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(),
                            tconnects[i].ਦੂਜਾ = ਖਾਲੀ_ਸੂਚਕ);
            }
            tconnects[i].ਦੂਜਾ = newp;
            reorganize = ਸੱਚ;
        }
        ਜੇ (tconnects[i].ਦੂਜਾ) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].ਦੂਜਾ);
        } ਨਹੀਂਤਾਂ {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
        AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
        argp->name("");  // Can forget name as will add back in pin order
    }

    ਜੇ (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        ਜਦੋਂ (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlink_Fr_ਪਿੱਠ();
        }
        ਲਈ (ਪੂਰਨ i = 0; i < tpinnum; ++i) {
            AstArg* ਅਚਲ argp = tconnects[i].ਦੂਜਾ;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->ਜੋੜੋ_Argsp(argp);
        }
    }

    ਜੇ (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_ਰੁੱਖ("-  ftref-out: ");
        ਲਈ (ਪੂਰਨ i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvt_ਮੰਜ਼ਿਲ_Hex(tconnects[i].ਪਹਿਲਾ)
                               << "  conn=" << cvt_ਮੰਜ਼ਿਲ_Hex(tconnects[i].ਦੂਜਾ));
        }
    }  // LCOV_EXCL_STOP

    ਜੇ (!argWrap.ਖਾਲੀ_ਥਾਵਾਂ()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        ਵਾਪਸ taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            ਖਾਲੀ_ਸੂਚਕ);
    }
    ਵਾਪਸ tconnects;
}

ਖਾਲੀ V3Task::taskConnectWrap(Ast_ਗੰਢ_FTask_Ref* nodep, ਅਚਲ V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ਅਚਲ VInsertion_ਸੈੱਟ<ਅਚਲ AstVar*>& argWrap) {
    statep->ਸੈੱਟ_Did_Wrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    ਲਈ (ਅਚਲ AstVar* varp : argWrap) newname += "_" + cvt_ਮੰਜ਼ਿਲ_Str(varp->pin_ਅੰਕ());
    ਅਚਲ ਸਵੈਚਾਲਿਤ pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(ਖਾਲੀ_ਸੂਚਕ));
    ਜੇ (pair.ਦੂਜਾ) {
        pair.ਪਹਿਲਾ->ਦੂਜਾ = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_ਗੰਢ_FTask* ਅਚਲ newTaskp = pair.ਪਹਿਲਾ->ਦੂਜਾ;

    // Remove the defaulted arguments from original outside call
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
        ਅਚਲ AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
        AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
        ਜੇ (argWrap.exists(portp)) {  // Removed arg
            statep->ਧੱਕੋ_Deletep(argp->unlink_Fr_ਪਿੱਠ());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_ਗੰਢ_FTask* V3Task::taskConnectWrapNew(Ast_ਗੰਢ_FTask* taskp, ਅਚਲ string& newname,
                                         ਅਚਲ V3TaskConnects& tconnects,
                                         ਅਚਲ VInsertion_ਸੈੱਟ<ਅਚਲ AstVar*>& argWrap) {
    std::map<ਅਚਲ AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_ਗੰਢ_FTask* ਅਚਲ newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newTaskp);

    Ast_ਗੰਢ_FTask_Ref* newCallp = ਖਾਲੀ_ਸੂਚਕ;
    Ast_ਗੰਢ* newCallInsertp = ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (VN_IS(taskp, Func)) {
        AstVar* ਅਚਲ fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ਅਚਲ newFVarp = fvarp->clone_ਰੁੱਖ(ਸੱਚ);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = ਨਵਾਂ AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ{taskp->fileline(),
                            ਨਵਾਂ AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(taskp, Task)) {
        newCallp = ਨਵਾਂ AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{taskp->fileline(), newCallp};
    } ਨਹੀਂਤਾਂ {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
        AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
        AstVar* newPortp;
        ਜੇ (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = ਨਵਾਂ AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(ਸੱਚ);
            ਜੇ (newPortp->valuep()) newPortp->valuep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            newTaskp->ਜੋੜੋ_Stmtsp(newPortp);
        } ਨਹੀਂਤਾਂ {  // Defaulting arg
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = VN_AS(portp->valuep(), ਗੰਢ_ਵਿਅੰਜਕ);
            ਜੇ ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ਅਚਲ VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ਅਚਲ refp = VN_AS(valuep->clone_ਰੁੱਖ(ਝੂਠ), VarRef);
                refp->access(refAccess);
                AstArg* ਅਚਲ newArgp = ਨਵਾਂ AstArg{portp->fileline(), portp->name(), refp};
                newCallp->ਜੋੜੋ_Argsp(newArgp);
                ਜਾਰੀ;
            }
            // Create local temporary
            newPortp = ਨਵਾਂ AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(ਸੱਚ);
            newTaskp->ਜੋੜੋ_Stmtsp(newPortp);
            // Runtime-assign it to the default
            ਜੇ (!VN_IS(valuep, ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ)) {
                Ast_ਸੌਂਪੋ* ਅਚਲ newAssignp
                    = ਨਵਾਂ Ast_ਸੌਂਪੋ{valuep->fileline(),
                                    ਨਵਾਂ AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_ਰੁੱਖ(ਸੱਚ)};
                newTaskp->ਜੋੜੋ_Stmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ਅਚਲ newArgp = ਨਵਾਂ AstArg{
            portp->fileline(), portp->name(),
            ਨਵਾਂ AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->ਜੋੜੋ_Argsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->ਜੋੜੋ_Stmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = oldNewVars.ਲੱਭੋ(refp->varp());
        ਜੇ (it != oldNewVars.ਸਿਰਾ()) refp->varp(it->ਦੂਜਾ);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    ਵਾਪਸ newTaskp;
}

string V3Task::ਸੌਂਪੋ_ਅੰਦਰੂਨੀ_ਮੰਜ਼ਿਲ_Dpi(AstVar* portp, ਤਾਰਕਿਕ isPtr, ਅਚਲ string& frSuffix,
                                   ਅਚਲ string& ਮੰਜ਼ਿਲ_Suffix, ਅਚਲ string& fr_ਅਗੇਤਰ) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ਅਚਲ string frName = fr_ਅਗੇਤਰ + portp->name() + frSuffix;
    ਅਚਲ string ਮੰਜ਼ਿਲ_Name = portp->name() + ਮੰਜ਼ਿਲ_Suffix;
    ਅਚਲ string idx = portp->name() + "__Vidx";
    ਮਾਪ_t unpack_ਮਾਪ = 1;  // non-unpacked array is treated as size 1
    ਪੂਰਨ unpackDim = 0;
    ਜੇ (AstUnpackArrayDType* ਅਚਲ unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpack_ਮਾਪ = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(ਝੂਠ).ਦੂਜਾ;
        ਜੇ (unpackDim > 0) UASSERT_OBJ(unpack_ਮਾਪ > 0, portp, "size must be greater than 0");
    }
    ਜੇ (portp->basicp()->is_Dpi_ਦੋਆਧਾਰੀ_ਅੰਕ_Vec() || portp->basicp()->isDpiLogicVec()) {
        ਅਚਲ ਤਾਰਕਿਕ is_ਦੋਆਧਾਰੀ_ਅੰਕ = portp->basicp()->is_Dpi_ਦੋਆਧਾਰੀ_ਅੰਕ_Vec();
        ਅਚਲ ਤਾਰਕਿਕ needsFor = unpack_ਮਾਪ > 1;
        ਜੇ (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvt_ਮੰਜ਼ਿਲ_Str(unpack_ਮਾਪ) + "; ++"
                   + idx + ") ";
        }

        stmt += (is_ਦੋਆਧਾਰੀ_ਅੰਕ ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvt_ਮੰਜ਼ਿਲ_Str(portp->ਚੌੜਾਈ()) + ", ";
        stmt += ਮੰਜ਼ਿਲ_Name;
        ਜੇ (needsFor) {
            stmt += " + " + cvt_ਮੰਜ਼ਿਲ_Str(portp->dtypep()->skipRefp()->ਚੌੜਾਈ_ਸ਼ਬਦ()) + " * " + idx;
        }
        stmt += ", ";
        ਜੇ (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            ਲਈ (ਪੂਰਨ i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } ਨਹੀਂਤਾਂ {
            stmt += frName + ")";
        }
    } ਨਹੀਂਤਾਂ {
        ਅਚਲ ਤਾਰਕਿਕ isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        ਅਚਲ ਤਾਰਕਿਕ isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        ਅਚਲ string unpackAt = unpack_ਮਾਪ > 1 ? "[" + idx + "]" : "[0]";
        ਜੇ (unpackDim > 0) {
            ਅਚਲ string forStmt = unpack_ਮਾਪ > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvt_ਮੰਜ਼ਿਲ_Str(unpack_ਮਾਪ) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + ਮੰਜ਼ਿਲ_Name + unpackAt;
        } ਨਹੀਂਤਾਂ {
            ਜੇ (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += ਮੰਜ਼ਿਲ_Name;
        }
        stmt += " = ";
        ਜੇ (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        ਜੇ (unpackDim > 0) {
            stmt += "(&" + frName;
            ਲਈ (ਪੂਰਨ i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } ਨਹੀਂਤਾਂ {
            stmt += frName;
        }
        ਜੇ (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    ਵਾਪਸ stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::ਸੌਂਪੋ_Dpi_ਮੰਜ਼ਿਲ_ਅੰਦਰੂਨੀ(ਅਚਲ std::string& lhsName, AstVar* varp) {
    std::string cvt;
    ਤਾਰਕਿਕ useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_ਮੰਜ਼ਿਲ_ਅੰਦਰੂਨੀ_Cvt_Stmt(varp->dtypep());
    ਅਚਲ std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ਅਚਲ ਪੂਰਨ ਕੁੱਲ = strides.ਖਾਲੀ_ਥਾਵਾਂ() ? 1 : strides[0].ਪਹਿਲਾ * strides[0].ਦੂਜਾ;
    ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_ਸ਼ਬਦ = varp->basicp()->ਚੌੜਾਈ_ਸ਼ਬਦ();
    string statements;
    ਲਈ (ਪੂਰਨ i = 0; i < ਕੁੱਲ; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& stride : strides) {
            lhs += "[" + std::ਮੰਜ਼ਿਲ_string((i / stride.ਦੂਜਾ) % stride.ਪਹਿਲਾ) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        ਜੇ (useSvVec) {
            ਅਚਲ std::string ਵਿੱਥ = std::ਮੰਜ਼ਿਲ_string(i * ਚੌੜਾਈ_ਸ਼ਬਦ);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + ਵਿੱਥ + ");\n";
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ std::string elem = strides.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : "[" + std::ਮੰਜ਼ਿਲ_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    ਵਾਪਸ statements;
}

ਖਾਲੀ V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ਅਚਲ TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("task", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
