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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Task.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3LanguageWords.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

ವರ್ಗ Task_ತಳ_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Task_ತಳ_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ);
    Ast_ಗಂಟು* m_impurep = ಶೂನ್ಯಸೂಚಕ;  // Node causing impure function w/ outside references
    ತಾರ್ಕಿಕ m_noInline = ಸುಳ್ಳು;  // Marked with pragma
ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Task_ತಳ_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp} {}
    ~Task_ತಳ_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ತಾರ್ಕಿಕ ಶುದ್ಧ_3() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_impurep == ಶೂನ್ಯಸೂಚಕ; }
    Ast_ಗಂಟು* impure_ಗಂಟು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_impurep; }
    ಶೂನ್ಯ impure(Ast_ಗಂಟು* nodep) { m_impurep = nodep; }
    ತಾರ್ಕಿಕ noInline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_noInline; }
    ಶೂನ್ಯ noInline(ತಾರ್ಕಿಕ flag) { m_noInline = flag; }
};

ವರ್ಗ Task_FTask_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Task_ತಳ_ಶೃಂಗ {
    VL_RTTI_IMPL(Task_FTask_ಶೃಂಗ, Task_ತಳ_ಶೃಂಗ);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ m_nodep;
    AstCFunc* m_cFuncp = ಶೂನ್ಯಸೂಚಕ;
    ತಾರ್ಕಿಕ m_needsNonInlineCFunc = ಸುಳ್ಳು;  // Needs CFunc for selected non-inlined call sites

ಸಾರ್ವಜನಿಕ:
    Task_FTask_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು_FTask* nodep)
        : Task_ತಳ_ಶೃಂಗ{graphp}
        , m_nodep{nodep} {}
    ~Task_FTask_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    Ast_ಗಂಟು_FTask* nodep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_nodep; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nodep()->name(); }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಶುದ್ಧ_3() ? "black" : "red"; }
    AstCFunc* cFuncp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cFuncp; }
    ಶೂನ್ಯ cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    ತಾರ್ಕಿಕ needsNonInlineCFunc() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_needsNonInlineCFunc; }
    ಶೂನ್ಯ needsNonInlineCFunc(ತಾರ್ಕಿಕ flag) { m_needsNonInlineCFunc = flag; }
};

ವರ್ಗ Task_ಸಂಕೇತ_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Task_ತಳ_ಶೃಂಗ {
    VL_RTTI_IMPL(Task_ಸಂಕೇತ_ಶೃಂಗ, Task_ತಳ_ಶೃಂಗ);
    // Top vertex for all calls not under another task
ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Task_ಸಂಕೇತ_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp)
        : Task_ತಳ_ಶೃಂಗ{graphp} {}
    ~Task_ಸಂಕೇತ_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ "*CODE*"; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "green"; }
};

ವರ್ಗ Task_ಅಂಚು final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    VL_RTTI_IMPL(Task_ಅಂಚು, V3_ನಕ್ಷೆ_ಅಂಚು)
ಸಾರ್ವಜನಿಕ:
    Task_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, Task_ತಳ_ಶೃಂಗ* fromp, Task_ತಳ_ಶೃಂಗ* ತುದಿ)
        : V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, 1, ಸುಳ್ಳು} {}
    ~Task_ಅಂಚು() override = ಪೂರ್ವನಿಯೋಜಿತ;
    string dotLabel() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "w" + cvt_ಗಮ್ಯ_Str(ತೂಕ_2()); }
};

//######################################################################

ವರ್ಗ TaskStateVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_inuser4;

    // TYPES
    ಬಳಸಿ Var_ಗಮ್ಯ_Scope_Map = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    ಬಳಸಿ Func_ಗಮ್ಯ_Class_Map = std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask*, AstClass*>;
    // MEMBERS
    Var_ಗಮ್ಯ_Scope_Map m_var_ಗಮ್ಯ_Scope_Map;  // Map for Var -> VarScope mappings
    Func_ಗಮ್ಯ_Class_Map m_func_ಗಮ್ಯ_Class_Map;  // Map for ctor func -> class
    Ast_ಗಂಟು_FTask* m_ctorp = ಶೂನ್ಯಸೂಚಕ;  // Class constructor
    AstClass* m_classp = ಶೂನ್ಯಸೂಚಕ;  // Current class
    V3_ನಕ್ಷೆ m_call_ನಕ್ಷೆ;  // Task call graph
    Task_ತಳ_ಶೃಂಗ* m_curVxp;  // Current vertex we're adding to
    std::ಸದಿಶ<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    ತಾರ್ಕಿಕ m_under_ಪೋರ್ಟ್_Var = ಸುಳ್ಳು;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Virtual_Iface_ವಿಧಾನ_Call(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask_Ref* refp) {
        ಸ್ಥಿರಾಂಕ Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ mrefp = VN_CAST(refp, ವಿಧಾನ_Call);
        ಆದರೆ (!mrefp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = mrefp->fromp();
        ಆದರೆ (!fromp || !fromp->dtypep()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        ಹಿಂತಿರುಗಿ ifrefp && ifrefp->isVirtual();
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isIfaceFTaskScope(ಸ್ಥಿರಾಂಕ AstScope* scopep) {
        ಹಿಂತಿರುಗಿ scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_ಗಂಟು_FTask* nodep) {
        AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        ಹಿಂತಿರುಗಿ scopep;
    }
    AstVarScope* ಕಂಡುಹಿಡಿ_Var_Scope(AstScope* scopep, AstVar* nodep) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_var_ಗಮ್ಯ_Scope_Map.ಕಂಡುಹಿಡಿ(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_var_ಗಮ್ಯ_Scope_Map.ಕೊನೆ(), nodep, "No scope for var");
        ಹಿಂತಿರುಗಿ iter->ಎರಡನೇ;
    }
    AstClass* getClassp(Ast_ಗಂಟು_FTask* nodep) {
        AstClass* ಸ್ಥಿರಾಂಕ classp = m_func_ಗಮ್ಯ_Class_Map[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        ಹಿಂತಿರುಗಿ classp;
    }
    ಶೂನ್ಯ remapFuncClassp(Ast_ಗಂಟು_FTask* nodep, Ast_ಗಂಟು_FTask* newp) {
        m_func_ಗಮ್ಯ_Class_Map[newp] = getClassp(nodep);
    }
    ತಾರ್ಕಿಕ ftaskNoInline(Ast_ಗಂಟು_FTask* nodep) { ಹಿಂತಿರುಗಿ get_FTask_ಶೃಂಗ(nodep)->noInline(); }
    ತಾರ್ಕಿಕ ftaskNeedsNonInlineCFunc(Ast_ಗಂಟು_FTask* nodep) {
        ಹಿಂತಿರುಗಿ get_FTask_ಶೃಂಗ(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_ಗಂಟು_FTask* nodep) { ಹಿಂತಿರುಗಿ get_FTask_ಶೃಂಗ(nodep)->cFuncp(); }
    ಶೂನ್ಯ ftaskCFuncp(Ast_ಗಂಟು_FTask* nodep, AstCFunc* cfuncp) {
        get_FTask_ಶೃಂಗ(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_ಗಂಟು* checkImpure(Ast_ಗಂಟು_FTask* nodep) { ಹಿಂತಿರುಗಿ get_FTask_ಶೃಂಗ(nodep)->impure_ಗಂಟು(); }

ಖಾಸಗಿ:
    Task_FTask_ಶೃಂಗ* get_FTask_ಶೃಂಗ(Ast_ಗಂಟು_FTask* nodep) {
        ಆದರೆ (!nodep->user4p()) nodep->user4p(ಹೊಸ Task_FTask_ಶೃಂಗ{&m_call_ನಕ್ಷೆ, nodep});
        ಹಿಂತಿರುಗಿ static_cast<Task_FTask_ಶೃಂಗ*>(nodep->user4u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ());
    }

    ಶೂನ್ಯ propagateImpureReason() {
        std::ಸದಿಶ<Task_FTask_ಶೃಂಗ*> workList;
        // Gather all impure FTask vertices
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_call_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (Task_FTask_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ftVtxp = vtx.cast<Task_FTask_ಶೃಂಗ>()) {
                ಆದರೆ (!ftVtxp->ಶುದ್ಧ_3()) workList.emplace_ಹಿಂಭಾಗ(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        ಆಗ (!workList.ಖಾಲಿಗಳು()) {
            Task_FTask_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ calleep = workList.ಹಿಂಭಾಗ();
            workList.ತೆಗೆ_ಹಿಂಭಾಗ();
            UASSERT_OBJ(!calleep->ಶುದ್ಧ_3(), calleep->nodep(), "Should not enqueue pure vertex");
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : calleep->ಒಳ_ಅಂಚುಗಳು()) {
                ಆದರೆ (Task_FTask_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ callerp = ಅಂಚು.fromp()->cast<Task_FTask_ಶೃಂಗ>()) {
                    ಆದರೆ (!callerp->ಶುದ್ಧ_3()) ಮುಂದುವರಿ;  // Already processed or on workList
                    callerp->impure(calleep->impure_ಗಂಟು());
                    workList.emplace_ಹಿಂಭಾಗ(callerp);
                }
            }
        }
    }

    ಶೂನ್ಯ decideInlining() {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_call_ನಕ್ಷೆ.vertices()) {
            Task_FTask_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ftVtxp = vtx.cast<Task_FTask_ಶೃಂಗ>();
            ಆದರೆ (!ftVtxp) ಮುಂದುವರಿ;
            // Move on if already decided not to inline
            ಆದರೆ (ftVtxp->noInline()) ಮುಂದುವರಿ;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            ಆದರೆ (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(ನಿಜ);
                ಮುಂದುವರಿ;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            ಆದರೆ (v3Global.opt.fInlineFuncsEager()) ಮುಂದುವರಿ;

            Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            ಆದರೆ (!ftaskp->verilogFunction()) ಮುಂದುವರಿ;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            ಆದರೆ (ftaskp->task_ಬಹಿರಂಗ()) ಮುಂದುವರಿ;
            // If the function is pure, it's OK not to inline, so don't do it
            ಆದರೆ (ftVtxp->ಶುದ್ಧ_3()) ftVtxp->noInline(ನಿಜ);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = vscp->varp();
                ಆದರೆ (varp->isFuncLocal() || varp->is_ಬಳಸಿದ_Loop_Idx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_var_ಗಮ್ಯ_Scope_Map.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ taskp = VN_CAST(stmtp, ಗಂಟು_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        Task_FTask_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ taskVtxp = get_FTask_ಶೃಂಗ(nodep->taskp());
        ಹೊಸ Task_ಅಂಚು{&m_call_ನಕ್ಷೆ, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        ಆದರೆ (is_Virtual_Iface_ವಿಧಾನ_Call(nodep)) taskVtxp->needsNonInlineCFunc(ನಿಜ);
        // Do we have to disable inlining the function?
        ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        ಆದರೆ (!taskVtxp->noInline()) {  // Else short-circuit below
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : tconnects) {
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = itr.ಮೊದಲ;
                ಸ್ಥಿರಾಂಕ AstArg* ಸ್ಥಿರಾಂಕ argp = itr.ಎರಡನೇ;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ pinp = argp->exprp()) {
                    ಆದರೆ ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(ನಿಜ);
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = get_FTask_ಶೃಂಗ(nodep);
        ಆದರೆ (nodep->dpiImport()) m_curVxp->noInline(ನಿಜ);
        ಆದರೆ (nodep->class_ವಿಧಾನ()) m_curVxp->noInline(ನಿಜ);  // Until V3Task supports it
        ಆದರೆ (nodep->recursive()) m_curVxp->noInline(ನಿಜ);
        ಆದರೆ (nodep->isConstructor()) {
            m_curVxp->noInline(ನಿಜ);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_func_ಗಮ್ಯ_Class_Map[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPragma* nodep) override {
        ಆದರೆ (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(ನಿಜ);
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        VL_RESTORER(m_under_ಪೋರ್ಟ್_Var);
        ಆದರೆ (nodep->isIO()) m_under_ಪೋರ್ಟ್_Var = ನಿಜ;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (m_under_ಪೋರ್ಟ್_Var) ಹಿಂತಿರುಗಿ;
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        ಆದರೆ (nodep->access().isReadOnly() && varp->const_Pool_ನಮೂದು()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (varp->user4u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ() != m_curVxp) {
            ಆದರೆ (m_curVxp->ಶುದ್ಧ_3() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        // Move initial statements into the constructor
        VL_RESTORER_CLEAR(m_initialps);
        VL_RESTORER(m_ctorp);
        VL_RESTORER(m_classp);
        m_ctorp = ಶೂನ್ಯಸೂಚಕ;
        m_classp = nodep;
        {  // Find m_initialps, m_ctor
            iterateChildren(nodep);
        }
        UASSERT_OBJ(m_ctorp, nodep, "class constructor missing");  // LinkDot always makes it
        Ast_ಗಂಟು* insertp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (AstInitialAutomatic* initialp : m_initialps) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ movep = initialp->stmtsp()) {
                movep->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(insertp, movep);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(initialp->unlink_Fr_ಹಿಂಭಾಗ()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomatic* nodep) override {
        m_initialps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = ಹೊಸ Task_ಸಂಕೇತ_ಶೃಂಗ{&m_call_ನಕ್ಷೆ};
        //
        iterate(nodep);
        //
        m_call_ನಕ್ಷೆ.remove_Redundant_ಅಂಚುಗಳು_ಮೊತ್ತ(&Task_ಅಂಚು::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        ಆದರೆ (dump_ನಕ್ಷೆ_Level()) m_call_ನಕ್ಷೆ.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

ರಚನೆ TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    ಸ್ಥಿರ std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> unpackDimsAndStrides(AstVar* varp) {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep()->skipRefp();
        std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> dimStrides;
        AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ಆದರೆ (!unpackp) ಹಿಂತಿರುಗಿ dimStrides;

        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nDims = dims.ಗಾತ್ರ_2();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].ಮೊದಲ = dims.ಹಿಂಭಾಗ()->elementsConst();
        dimStrides[nDims - 1].ಎರಡನೇ = 1;
        // Rest are densly packed
        ಗಾಗಿ (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].ಮೊದಲ = dims[i]->elementsConst();
            dimStrides[i].ಎರಡನೇ = dimStrides[i + 1].ಮೊದಲ * dimStrides[i + 1].ಎರಡನೇ;
        }
        ಹಿಂತಿರುಗಿ dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    ಸ್ಥಿರ std::pair<std::string, ತಾರ್ಕಿಕ> dpi_ಗಮ್ಯ_ಆಂತರಿಕ_Cvt_Stmt(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = dtypep->basicp();

        // DPI types using Primitive C types
        ಆದರೆ (basicp->keyword() == VBasicDTypeKwd::CHANDLE) ಹಿಂತಿರುಗಿ {"VL_CVT_VP_Q(", ಸುಳ್ಳು};
        ಆದರೆ (basicp->keyword() == VBasicDTypeKwd::STRING) ಹಿಂತಿರುಗಿ {"VL_CVT_N_CSTR(", ಸುಳ್ಳು};
        ಆದರೆ (basicp->isDpiPrimitive()) ಹಿಂತಿರುಗಿ {"(", ಸುಳ್ಳು};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->is_Dpi_ಬಿಟ್_Vec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ಸ್ಥಿರಾಂಕ std::string vecType = basicp->is_Dpi_ಬಿಟ್_Vec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ ಗಾತ್ರ_Char = dtypep->ಅಗಲ() <= 8    ? 'C'
                              : dtypep->ಅಗಲ() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ಸ್ಥಿರಾಂಕ std::string& ಗಾತ್ರ_2 = std::ಗಮ್ಯ_string(dtypep->ಅಗಲ());
        // cppcheck-suppress strPlusChar
        ಹಿಂತಿರುಗಿ {"VL_SET_"s + ಗಾತ್ರ_Char + "_" + vecType + "(" + ಗಾತ್ರ_2 + ", ", ನಿಜ};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

ವರ್ಗ TaskVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // TYPES
    ಬಳಸಿ DpiCFuncs = std::map<ಸ್ಥಿರಾಂಕ string, std::tuple<Ast_ಗಂಟು_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ಸ್ಥಿರಾಂಕ m_statep;  // Common state between visitors
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    Ast_ತುದಿ_Scope* ಸ್ಥಿರಾಂಕ m_ತುದಿ_Scopep = v3Global.rootp()->ತುದಿ_Scopep();  // The AstTopScope
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope
    Ast_ಗಂಟು* m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // Where to insert statement
    ತಾರ್ಕಿಕ m_ಒಳ_Sensesp = ಸುಳ್ಳು;  // Are we under a senitem?
    ತಾರ್ಕಿಕ m_ಒಳ_New = ಸುಳ್ಳು;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    ಪೂರ್ಣಾಂಕ m_uncon_Var_ಅಂಕೆ = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpi_ಹೆಸರುಗಳು;  // Map of all created DPI functions
    std::map<std::string, ಸ್ಥಿರಾಂಕ AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ಸ್ಥಿರಾಂಕ string& name, AstVar* examplep) {
        AstVar* ಸ್ಥಿರಾಂಕ newvarp = ಹೊಸ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(ನಿಜ);
        funcp->ಕೂಡಿಸು_Varsp(newvarp);
        AstVarScope* ಸ್ಥಿರಾಂಕ newvscp = ಹೊಸ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->ಕೂಡಿಸು_Varsp(newvscp);
        ಹಿಂತಿರುಗಿ newvscp;
    }
    AstVarScope* create_ಒಳಹರಿವು_Var(AstCFunc* funcp, ಸ್ಥಿರಾಂಕ string& name, VBasicDTypeKwd kwd) {
        AstVar* ಸ್ಥಿರಾಂಕ newvarp
            = ಹೊಸ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->ಕಂಡುಹಿಡಿ_Basic_DType(kwd)};
        newvarp->funcLocal(ನಿಜ);
        newvarp->direction(VDirection::ಒಳಹರಿವು);
        funcp->ಕೂಡಿಸು_Argsp(newvarp);
        AstVarScope* ಸ್ಥಿರಾಂಕ newvscp = ಹೊಸ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->ಕೂಡಿಸು_Varsp(newvscp);
        ಹಿಂತಿರುಗಿ newvscp;
    }
    ಸ್ಥಿರ AstVarScope* const_Pool_ಕೋಷ್ಟಕ(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ಸ್ಥಿರಾಂಕ refp = V3ConstPool::ಕಂಡುಹಿಡಿ(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
        ಹಿಂತಿರುಗಿ vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask_Ref* refp, AstVar* invarp,
                                ಸ್ಥಿರಾಂಕ std::string& suffix) {
        ಆದರೆ (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            ಹಿಂತಿರುಗಿ const_Pool_ಕೋಷ್ಟಕ(invarp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* ಸ್ಥಿರಾಂಕ newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* ಸ್ಥಿರಾಂಕ newvarp = newvscp->varp();
            newvarp->funcLocal(ಸುಳ್ಳು);
            newvarp->propagateAttrFrom(invarp);
            newvarp->is_ಆಂತರಿಕ(ನಿಜ);
            ಹಿಂತಿರುಗಿ newvscp;
        }
    }

    // Replace varrefs with new var pointer
    ಶೂನ್ಯ relink(Ast_ಗಂಟು* nodep) {
        nodep->foreach_And_ಮುಂದಿನ([](AstVarRef* refp) {
            ಆದರೆ (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ಸ್ಥಿರಾಂಕ newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    Ast_ನಿಯೋಜಿಸು* connect_ಪೋರ್ಟ್_Make_ಒಳ_ನಿಯೋಜಿಸು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp, AstVarScope* newvscp, ತಾರ್ಕಿಕ ಶುದ್ಧ_Check) {
        // Create input assignment to go in FRONT of function call
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಒಳ_Pinp = pinp;
        ಆದರೆ (AstResizeLValue* sinPinp = VN_CAST(ಒಳ_Pinp, ResizeLValue)) ಒಳ_Pinp = sinPinp->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಒಳ_Pin_Clonep
            = ಶುದ್ಧ_Check ? ಒಳ_Pinp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ) : ಒಳ_Pinp->clone_ವೃಕ್ಷ(ನಿಜ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        ಆದರೆ (arrayp && arrayp->is_ಹಿಮ್ಮುಖ_ದ(ಒಳ_Pin_Clonep->dtypep())) {
            ಒಳ_Pin_Clonep
                = ಹೊಸ Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack{pinp->fileline(), ಒಳ_Pin_Clonep, newvscp->dtypep()};
        }
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            pinp->fileline(), ಹೊಸ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            ಒಳ_Pin_Clonep};
        assp->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::BLKSEQ, ನಿಜ);  // Ok if in <= block
        ಹಿಂತಿರುಗಿ assp;
    }
    Ast_ನಿಯೋಜಿಸು* connect_ಪೋರ್ಟ್_Make_ಹೊರಹೋಗು_ನಿಯೋಜಿಸು(AstVar* portp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp, AstVarScope* newvscp,
                                        ತಾರ್ಕಿಕ ಶುದ್ಧ_Check) {
        // If needed, remap size of function to caller's output size
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊರಹೋಗು_Pinp = pinp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* postRhsp = ಹೊಸ AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        ಆದರೆ (AstResizeLValue* soutPinp = VN_CAST(ಹೊರಹೋಗು_Pinp, ResizeLValue)) {
            ಹೊರಹೋಗು_Pinp = soutPinp->lhsp();
            ಆದರೆ (VN_IS(ಹೊರಹೋಗು_Pinp, RToIRoundS) || VN_IS(ಹೊರಹೋಗು_Pinp, RToIS)) {
                ಹೊರಹೋಗು_Pinp = VN_AS(ಹೊರಹೋಗು_Pinp, ಗಂಟು_Uniop)->lhsp();
                ಆದರೆ (postRhsp->isSigned()) {
                    postRhsp = ಹೊಸ AstISToRD{pinp->fileline(), postRhsp};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    postRhsp = ಹೊಸ AstIToRD{pinp->fileline(), postRhsp};
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (Ast_ಗಂಟು_Uniop* aoutPinp = VN_CAST(ಹೊರಹೋಗು_Pinp, Extend)) {
                    ಹೊರಹೋಗು_Pinp = aoutPinp->lhsp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_Uniop* aoutPinp = VN_CAST(ಹೊರಹೋಗು_Pinp, ExtendS)) {
                    ಹೊರಹೋಗು_Pinp = aoutPinp->lhsp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstSel* aoutPinp = VN_CAST(ಹೊರಹೋಗು_Pinp, Sel)) {
                    ಹೊರಹೋಗು_Pinp = aoutPinp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಹೊರಹೋಗು_Pinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                ಆದರೆ (ಹೊರಹೋಗು_Pinp->ಅಗಲ() < portp->ಅಗಲ()) {
                    postRhsp = ಹೊಸ AstSel{pinp->fileline(), postRhsp, 0, pinp->ಅಗಲ()};
                } ಇಲ್ಲದಿದ್ದರೆ {  // pin width > port width
                    ಆದರೆ (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = ಹೊಸ AstExtendS{pinp->fileline(), postRhsp};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        postRhsp = ಹೊಸ AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(ಹೊರಹೋಗು_Pinp);
        }
        ಆದರೆ (VN_IS(ಹೊರಹೋಗು_Pinp, IToRD) || VN_IS(ಹೊರಹೋಗು_Pinp, ISToRD)) {
            ಹೊರಹೋಗು_Pinp = VN_AS(ಹೊರಹೋಗು_Pinp, ಗಂಟು_Uniop)->lhsp();
            postRhsp = ಹೊಸ AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(ಹೊರಹೋಗು_Pinp);
        }
        // Put output assignment AFTER function call
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊರಹೋಗು_Pin_Clonep
            = ಶುದ್ಧ_Check ? ಹೊರಹೋಗು_Pinp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ) : ಹೊರಹೋಗು_Pinp->clone_ವೃಕ್ಷ(ನಿಜ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp
            = VN_CAST(ಹೊರಹೋಗು_Pinp->dtypep()->skipRefp(), UnpackArrayDType);
        ಆದರೆ (arrayp && arrayp->is_ಹಿಮ್ಮುಖ_ದ(postRhsp->dtypep())) {
            postRhsp = ಹೊಸ Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack{pinp->fileline(), postRhsp, ಹೊರಹೋಗು_Pinp->dtypep()};
        }
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{pinp->fileline(), ಹೊರಹೋಗು_Pin_Clonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::BLKSEQ, ನಿಜ);  // Ok if in <= block
        ಹಿಂತಿರುಗಿ assp;
    }

    ಶೂನ್ಯ changeAtWriteRecurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp) {
        // Change nested at methods to writable variant
        ಆದರೆ (AstCMethodHard* ಸ್ಥಿರಾಂಕ cMethodp = VN_CAST(exprp, CMethodHard)) {
            ಆದರೆ (cMethodp->ವಿಧಾನ() == VCMethod::ARRAY_AT) {
                cMethodp->ವಿಧಾನ(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    ಶೂನ್ಯ connect_ಪೋರ್ಟ್(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_ಗಂಟು* beginp,
                     ತಾರ್ಕಿಕ inlineTask) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp = argp->exprp();
        ಆದರೆ (inlineTask) {
            portp->unlink_Fr_ಹಿಂಭಾಗ();
            ತಳ್ಳು_Deletep(portp);  // Remove it from the clone (not original)
        }
        ಆದರೆ (!pinp) {
            // Too few arguments in function call
        } ಇಲ್ಲದಿದ್ದರೆ {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            ಆದರೆ (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::ಗಮ್ಯ_string(m_uncon_Var_ಅಂಕೆ++),
                                                portp->dtypep()};
                m_modp->ಕೂಡಿಸು_Stmtsp(varp);
                AstVarScope* ಸ್ಥಿರಾಂಕ newvscp = ಹೊಸ AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->ಕೂಡಿಸು_Varsp(newvscp);
                AstVarRef* ಸ್ಥಿರಾಂಕ repp = ಹೊಸ AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(pinp), pinp);
                pinp = repp;
            }
            ಆದರೆ (inlineTask) {
                ತಳ್ಳು_Deletep(pinp->unlink_Fr_ಹಿಂಭಾಗ());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), argp);  // Args no longer needed
            }
            ಆದರೆ (portp->isRef() || portp->isConstRef()) {
                ತಾರ್ಕಿಕ refArgOk = ಸುಳ್ಳು;
                ಆದರೆ (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCMethodHard* ಸ್ಥಿರಾಂಕ cMethodp = VN_CAST(pinp, CMethodHard)) {
                    ಆದರೆ (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), ಸರತಿ_DType)) {
                        refArgOk = cMethodp->ವಿಧಾನ() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->ವಿಧಾನ() == VCMethod::DYN_AT_WRITE_APPEND_ಹಿಂಭಾಗ;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->ವಿಧಾನ() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                ಆದರೆ (refArgOk) {
                    ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                ಆದರೆ (inlineTask) {
                    ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* ಸ್ಥಿರಾಂಕ localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* ಸ್ಥಿರಾಂಕ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ಆದರೆ (!inlineTask) {
                    pinp->replaceWith(
                        ಹೊಸ AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    ತಳ್ಳು_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ preassp = connect_ಪೋರ್ಟ್_Make_ಒಳ_ನಿಯೋಜಿಸು(pinp, newvscp, ನಿಜ);
                beginp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(preassp);

                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ postassp = connect_ಪೋರ್ಟ್_Make_ಹೊರಹೋಗು_ನಿಯೋಜಿಸು(portp, pinp, newvscp, ನಿಜ);
                beginp->ಕೂಡಿಸು_ಮುಂದಿನ(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ಸ್ಥಿರಾಂಕ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ಆದರೆ (!inlineTask) {
                    pinp->replaceWith(ಹೊಸ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    ತಳ್ಳು_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ postassp = connect_ಪೋರ್ಟ್_Make_ಹೊರಹೋಗು_ನಿಯೋಜಿಸು(portp, pinp, newvscp, ಸುಳ್ಳು);
                // Put assignment BEHIND of all other statements
                beginp->ಕೂಡಿಸು_ಮುಂದಿನ(postassp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ಸ್ಥಿರಾಂಕ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ preassp = connect_ಪೋರ್ಟ್_Make_ಒಳ_ನಿಯೋಜಿಸು(pinp, newvscp, ಸುಳ್ಳು);
                // Put assignment in FRONT of all other statements
                beginp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(preassp);
            }
        }
    }

    ತಾರ್ಕಿಕ has_Ref_ವಾದ(Ast_ಗಂಟು_FTask* nodep) {
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (varp->isRef() || varp->isConstRef()) ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    Ast_ಗಂಟು* createInlinedFTask(Ast_ಗಂಟು_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_ವೃಕ್ಷ(ನಿಜ) : ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ beginp
            = ಹೊಸ AstComment{refp->fileline(), "Function: "s + refp->name(), ನಿಜ};
        ಆದರೆ (newbodysp) beginp->ಕೂಡಿಸು_ಮುಂದಿನ(newbodysp);
        ಆದರೆ (debug() >= 9) beginp->dump_ವೃಕ್ಷ_And_ಮುಂದಿನ(ಹೊರಗೆ, "-  newbegi: ");
        //
        // Create input variables
        Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
        {
            ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : tconnects) {
                AstVar* ಸ್ಥಿರಾಂಕ portp = itr.ಮೊದಲ;
                AstArg* ಸ್ಥಿರಾಂಕ argp = itr.ಎರಡನೇ;
                connect_ಪೋರ್ಟ್(refp, portp, argp, beginp, ನಿಜ);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_ಗಂಟು* nextstmtp;
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    ಆದರೆ (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* ಸ್ಥಿರಾಂಕ localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        ಆದರೆ (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ crstp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                                portp->fileline(),
                                ಹೊಸ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                ಹೊಸ AstCReset{portp->fileline(), portp, ಸುಳ್ಳು}};
                            portp->replaceWith(crstp);
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            portp->unlink_Fr_ಹಿಂಭಾಗ();
                        }
                        ತಳ್ಳು_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        ಆದರೆ (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        ಆದರೆ (debug() >= 9) beginp->dump_ವೃಕ್ಷ_And_ಮುಂದಿನ(ಹೊರಗೆ, "-  iotask: ");
        ಹಿಂತಿರುಗಿ beginp;
    }

    Ast_ಗಂಟು* createNonInlinedFTask(Ast_ಗಂಟು_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ beginp
            = ಹೊಸ AstComment{refp->fileline(), "Function: "s + refp->name(), ನಿಜ};
        Ast_ಗಂಟು_CCall* ccallp;
        ಆದರೆ (VN_IS(refp, New)) {
            AstCNew* ಸ್ಥಿರಾಂಕ cnewp = ಹೊಸ AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ mrefp = VN_CAST(refp, ವಿಧಾನ_Call)) {
            ccallp = ಹೊಸ AstCMethodCall{refp->fileline(), mrefp->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), cfuncp};
            ccallp->dtype_ಗಣ_Void();
            beginp->ಕೂಡಿಸು_ಮುಂದಿನ(ccallp->makeStmt());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ccallp = ಹೊಸ AstCCall{refp->fileline(), cfuncp};
            ccallp->dtype_ಗಣ_Void();
            beginp->ಕೂಡಿಸು_ಮುಂದಿನ(ccallp->makeStmt());
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstFuncRef* ಸ್ಥಿರಾಂಕ funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstTaskRef* ಸ್ಥಿರಾಂಕ taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp = VN_CAST(refp, ವಿಧಾನ_Call)) {
            ccallp->superReference(ವಿಧಾನ_Callp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : tconnects) {
                AstVar* ಸ್ಥಿರಾಂಕ portp = itr.ಮೊದಲ;
                AstArg* ಸ್ಥಿರಾಂಕ argp = itr.ಎರಡನೇ;
                connect_ಪೋರ್ಟ್(refp, portp, argp, beginp, ಸುಳ್ಳು);
            }
        }
        // First argument is symbol table, then output if a function
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        ಆದರೆ (needSyms) ccallp->argTypes("vlSymsp");

        ಆದರೆ (refp->taskp()->dpiContext()) {
            AstScopeName* ಸ್ಥಿರಾಂಕ snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlink_Fr_ಹಿಂಭಾಗ();
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = refp->fileline();
            // __Vscopep
            ccallp->ಕೂಡಿಸು_Argsp(snp);
            // __Vfilenamep
            ccallp->ಕೂಡಿಸು_Argsp(
                ಹೊಸ AstCExpr{flp, AstCExpr::ಶುದ್ಧ{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        ಗಾಗಿ (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->ಕೂಡಿಸು_Argsp(argp->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
        }

        ಆದರೆ (outvscp) ccallp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        ಆದರೆ (debug() >= 9) beginp->dump_ವೃಕ್ಷ_And_ಮುಂದಿನ(ಹೊರಗೆ, "-  nitask: ");
        ಹಿಂತಿರುಗಿ beginp;
    }

    string dpiSignature(Ast_ಗಂಟು_FTask* nodep, AstVar* rtnvarp) ಸ್ಥಿರಾಂಕ {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        ಆದರೆ (nodep->dpi_ಶುದ್ಧ()) dpiproto += "pure ";
        ಆದರೆ (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(ನಿಜ, ನಿಜ) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string ವಾದಗಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    ಆದರೆ (ವಾದಗಳು != "") {
                        ವಾದಗಳು += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ಸ್ಥಿರಾಂಕ std::string dpiType = portp->dpiArgType(ಸುಳ್ಳು, ಸುಳ್ಳು);
                    dpiproto += dpiType;
                    ಸ್ಥಿರಾಂಕ std::string vType = portp->dtypep()->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(ಸುಳ್ಳು);
                    ಆದರೆ (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    ವಾದಗಳು += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        ಹಿಂತಿರುಗಿ dpiproto;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ checkLegalCIdentifier(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& name) {
        ಆದರೆ (name.ಕೊನೆ() != std::ಕಂಡುಹಿಡಿ_if(name.begin(), name.ಕೊನೆ(), [](ಅಕ್ಷರ c) {
                ಹಿಂತಿರುಗಿ !std::ಅಕ್ಷರವೇ_ಅಂಕಿಯೇ(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* create_Dpi_ತಾತ್ಕಾಲಿಕ(AstVar* portp, ಸ್ಥಿರಾಂಕ string& suffix) {
        ಸ್ಥಿರಾಂಕ string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        ಹಿಂತಿರುಗಿ ಹೊಸ AstCStmt{portp->fileline(), stmt};
    }

    ಶೂನ್ಯ unlinkAndClone(Ast_ಗಂಟು_FTask* funcp, Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ with_ಮುಂದಿನ) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        ಆದರೆ (funcp->recursive()) {
            VNRelinker relinkHandle;
            ಆದರೆ (with_ಮುಂದಿನ) {
                nodep->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ(&relinkHandle);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = nodep->clone_ವೃಕ್ಷ(with_ಮುಂದಿನ);
            relinkHandle.relink(newp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (with_ಮುಂದಿನ) {
                nodep->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->unlink_Fr_ಹಿಂಭಾಗ();
            }
        }
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* create_ನಿಯೋಜಿಸು_ಆಂತರಿಕ_ಗಮ್ಯ_Dpi(AstVar* portp, ತಾರ್ಕಿಕ isPtr, ಸ್ಥಿರಾಂಕ string& frSuffix,
                                              ಸ್ಥಿರಾಂಕ string& ಗಮ್ಯ_Suffix) {
        ಸ್ಥಿರಾಂಕ string stmt = V3Task::ನಿಯೋಜಿಸು_ಆಂತರಿಕ_ಗಮ್ಯ_Dpi(portp, isPtr, frSuffix, ಗಮ್ಯ_Suffix);
        ಹಿಂತಿರುಗಿ ಹೊಸ AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* createDpiScalarCvt(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* internalp, ಸ್ಥಿರಾಂಕ std::string& dpi,
                                           ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep, ತಾರ್ಕಿಕ ಗಮ್ಯ_Dpi) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = internalp->fileline();
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = dtypep->basicp();
        ಆದರೆ (ಗಮ್ಯ_Dpi) {
            AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{flp};
            ಆದರೆ (basicp->isDpiPrimitive()) {
                cstmtp->ಕೂಡಿಸು(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->ಕೂಡಿಸು(internalp);
                cstmtp->ಕೂಡಿಸು(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } ಇಲ್ಲದಿದ್ದರೆ {
                cstmtp->ಕೂಡಿಸು((basicp->is_Dpi_ಬಿಟ್_Vec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::ಗಮ್ಯ_string(dtypep->ಅಗಲ()) + ", " + dpi + ", ");
                cstmtp->ಕೂಡಿಸು(internalp);
                cstmtp->ಕೂಡಿಸು(");");
            }
            ಹಿಂತಿರುಗಿ cstmtp;
        }
        std::string cvt;
        ತಾರ್ಕಿಕ useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_ಗಮ್ಯ_ಆಂತರಿಕ_Cvt_Stmt(dtypep);
        ಆದರೆ (useSvVec) {
            AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{flp};
            cstmtp->ಕೂಡಿಸು(cvt);
            cstmtp->ಕೂಡಿಸು(internalp);
            cstmtp->ಕೂಡಿಸು(", " + dpi + ");");
            ಹಿಂತಿರುಗಿ cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ cwidth = basicp->ಅಗಲ_ಪದಗಳು() * VL_EDATASIZE;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstCExpr{flp, cvt + dpi + ")", cwidth};
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, internalp, ಹೊಸ AstSel{flp, rhsp, 0, dtypep->ಅಗಲ()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* createDpiCvt(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* internalp, ಸ್ಥಿರಾಂಕ std::string& dpi,
                                     ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep, ತಾರ್ಕಿಕ ಗಮ್ಯ_Dpi) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_ಗಂಟು_Stmt* stmtsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elemp = ಹೊಸ AstArraySel{flp, internalp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), i};
                stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::ಗಮ್ಯ_string(i) + "]",
                                                       arrayp->subDTypep(), ಗಮ್ಯ_Dpi));
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dtypep->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct()) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ಸ್ಥಿರಾಂಕ selp
                    = ಹೊಸ AstStructSel{flp, internalp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), ಗಮ್ಯ_Dpi));
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ createDpiScalarCvt(internalp, dpi, dtypep, ಗಮ್ಯ_Dpi);
        }
        VL_DO_DANGLING(internalp->delete_ವೃಕ್ಷ(), internalp);
        ಹಿಂತಿರುಗಿ stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* createDpiArgCvt(AstVarScope* vscp, ಸ್ಥಿರಾಂಕ std::string& dpiName,
                                        ತಾರ್ಕಿಕ ಗಮ್ಯ_Dpi) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = vscp->varp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಒಟ್ಟು = strides.ಖಾಲಿಗಳು() ? 1 : strides[0].ಮೊದಲ * strides[0].ಎರಡನೇ;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isStruct = varp->dtypep()->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct();
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_ಗಂಟು_Stmt* stmtsp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಒಟ್ಟು; ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* internalp
                = ಹೊಸ AstVarRef{flp, vscp, ಗಮ್ಯ_Dpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& stride : strides) {
                internalp = ಹೊಸ AstArraySel(flp, internalp, (i / stride.ಎರಡನೇ) % stride.ಮೊದಲ);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            ಆದರೆ (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::ಗಮ್ಯ_string(i * varp->basicp()->ಅಗಲ_ಪದಗಳು());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isStruct || !strides.ಖಾಲಿಗಳು()) {
                dpi += "[" + std::ಗಮ್ಯ_string(i) + "]";
            }
            stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, ಗಮ್ಯ_Dpi));
        }
        ಹಿಂತಿರುಗಿ stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ nodep, AstVar* ಸ್ಥಿರಾಂಕ rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಸ್ಥಿರಾಂಕ std::string cname = Ast_ಗಂಟು::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ಸ್ಥಿರಾಂಕ std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(ನಿಜ, ನಿಜ) : "";
        // The function we are building
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(ನಿಜ);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(ನಿಜ);
        funcp->ನಮೂದು_ಬಿಂದು(ನಿಜ);
        funcp->isStatic(ನಿಜ);
        funcp->protect(ಸುಳ್ಳು);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_ತುದಿ_Scopep->scopep()->ಕೂಡಿಸು_Blocksp(funcp);
        // Check the arguments before converting them
        ಆದರೆ (!make_ಪೋರ್ಟ್_List(nodep, funcp)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

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
        AstCStmt* ಸ್ಥಿರಾಂಕ prep = ಹೊಸ AstCStmt{flp};
        funcp->ಕೂಡಿಸು_Stmtsp(prep);
        // Static doesn't need save-restore as if below will re-fill proper value
        prep->ಕೂಡಿಸು("static int __Vfuncnum = -1;\n");
        // First time init (faster than what the compiler does if we did a singleton
        prep->ಕೂಡಿಸು("if (VL_UNLIKELY(__Vfuncnum == -1)) {\n");
        prep->ಕೂಡಿಸು("__Vfuncnum = Verilated::exportFuncNum(\"" + nodep->cname() + "\");\n");
        prep->ಕೂಡಿಸು("}\n");
        // If the find fails, it will throw an error
        prep->ಕೂಡಿಸು("const VerilatedScope* const __Vscopep = Verilated::dpiScope();\n");
        // If 'dpiScope()' fails and '__Vscopep' is null; the exportFind function throws an error
        // If __Vcb is null the exportFind function throws and error
        ಸ್ಥಿರಾಂಕ std::string cbtype
            = VIdProtect::protect(v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "__Vcb_" + nodep->cname() + "_t");
        prep->ಕೂಡಿಸು(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCStmt{flp};
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_Func_Arg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(ಸುಳ್ಳು);
            portp->protect(ಸುಳ್ಳು);
            // Add argument to call
            ಸ್ಥಿರಾಂಕ VAccess access = portp->direction().pinAccess();
            callp->ಕೂಡಿಸು(", ");
            callp->ಕೂಡಿಸು(ಹೊಸ AstVarRef{portp->fileline(), vscp, access});
            ಹಿಂತಿರುಗಿ vscp;
        };
        // Call callback
        callp->ಕೂಡಿಸು("(*__Vcb)(");
        // First argument is the Syms
        callp->ಕೂಡಿಸು("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_ಗಂಟು* ಹೊರಹೋಗು_Cvtsp = ಶೂನ್ಯಸೂಚಕ;  // Conversions of output/inout arguments, after the call
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var);
            ಆದರೆ (!portp) ಮುಂದುವರಿ;
            ಆದರೆ (!portp->isIO()) ಮುಂದುವರಿ;
            ಆದರೆ (portp->isFuncReturn()) ಮುಂದುವರಿ;
            ಆದರೆ (portp == rtnvarp) ಮುಂದುವರಿ;  // Handled below
            // Add argument to call
            AstVarScope* ಸ್ಥಿರಾಂಕ outvscp = ಕೂಡಿಸು_Func_Arg(portp);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isStruct = portp->dtypep()->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct();
            ಆದರೆ (portp->isWritable()) {
                ಹೊರಹೋಗು_Cvtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
                    ಹೊರಹೋಗು_Cvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), ನಿಜ)
                                       : create_ನಿಯೋಜಿಸು_ಆಂತರಿಕ_ಗಮ್ಯ_Dpi(portp, ನಿಜ, tmpSuffixp, ""));
            }
            ಆದರೆ (!portp->isNonOutput()) ಮುಂದುವರಿ;
            // Convert input/inout arguments to dpi type
            ಸ್ಥಿರಾಂಕ std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(ಸುಳ್ಳು).ಎರಡನೇ == 0
                      ? "*"
                      : "";
            funcp->ಕೂಡಿಸು_Stmtsp(createDpiArgCvt(outvscp, deref + portp->name(), ಸುಳ್ಳು));
        }
        // Return value argument goes last
        ಆದರೆ (rtnvarp) ಕೂಡಿಸು_Func_Arg(rtnvarp);
        // Close statement
        callp->ಕೂಡಿಸು(");");
        // Call the user function
        funcp->ಕೂಡಿಸು_Stmtsp(callp);
        // Convert output/inout arguments back to internal type
        ಆದರೆ (ಹೊರಹೋಗು_Cvtsp) funcp->ಕೂಡಿಸು_Stmtsp(ಹೊರಹೋಗು_Cvtsp);
        // Convert return value
        ಆದರೆ (rtnvarp) {
            funcp->ಕೂಡಿಸು_Stmtsp(create_Dpi_ತಾತ್ಕಾಲಿಕ(rtnvarp, ""));
            funcp->ಕೂಡಿಸು_Stmtsp(create_ನಿಯೋಜಿಸು_ಆಂತರಿಕ_ಗಮ್ಯ_Dpi(rtnvarp, ಸುಳ್ಳು, tmpSuffixp, ""));
            ಸ್ಥಿರಾಂಕ std::string ಸೂಚ್ಯಂಕ = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{flp, "return " + rtnvarp->name() + ಸೂಚ್ಯಂಕ + ";"});
        }
        ಹಿಂತಿರುಗಿ funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_ಗಂಟು_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ಸ್ಥಿರಾಂಕ string name = Ast_ಗಂಟು::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ಸ್ಥಿರಾಂಕ string rtnType = rtnvarp            ? rtnvarp->dpiArgType(ನಿಜ, ನಿಜ)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(ನಿಜ);
        funcp->dontCombine(ನಿಜ);
        funcp->ನಮೂದು_ಬಿಂದು(ಸುಳ್ಳು);
        funcp->is_ವಿಧಾನ(ಸುಳ್ಳು);
        funcp->protect(ಸುಳ್ಳು);
        funcp->dpi_ಶುದ್ಧ(nodep->dpi_ಶುದ್ಧ());

        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ವೆಚ್ಚ = static_cast<ಪೂರ್ಣಾಂಕ>(V3Control::get_Profile_ದತ್ತಾಂಶ(funcp->name()));
        m_statHierDpisWithCosts += (ವೆಚ್ಚ != 0);
        funcp->ವೆಚ್ಚ(ವೆಚ್ಚ);

        // Add DPI Import to top, since it's a global function
        m_ತುದಿ_Scopep->scopep()->ಕೂಡಿಸು_Blocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        ಆದರೆ (!make_ಪೋರ್ಟ್_List(nodep, funcp)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ funcp;
    }

    AstCFunc* getDpiFunc(Ast_ಗಂಟು_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ಸ್ಥಿರಾಂಕ string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_dpi_ಹೆಸರುಗಳು.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, ಶೂನ್ಯಸೂಚಕ));
        ಆದರೆ (pair.ಎರಡನೇ) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.ಮೊದಲ->ಎರಡನೇ) = funcp;
            ಹಿಂತಿರುಗಿ funcp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Seen this cname import before. Check if it's the same prototype.
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಮೊದಲ_Nodep;
            string ಮೊದಲ_Signature;
            AstCFunc* ಮೊದಲ_Funcp;
            std::tie(ಮೊದಲ_Nodep, ಮೊದಲ_Signature, ಮೊದಲ_Funcp) = pair.ಮೊದಲ->ಎರಡನೇ;
            ಆದರೆ (signature != ಮೊದಲ_Signature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warn_ಇನ್ನಷ್ಟು()  //
                               << "... New signature:      " << signature << '\n'  //
                               << ಮೊದಲ_Nodep->warnOther()
                               << "... Original signature: " << ಮೊದಲ_Signature << '\n'  //
                               << ಮೊದಲ_Nodep->warnContextSecondary());
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Same signature, return the previously created CFunc
                ಹಿಂತಿರುಗಿ ಮೊದಲ_Funcp;
            }
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ make_ಪೋರ್ಟ್_List(Ast_ಗಂಟು_FTask* nodep, AstCFunc* dpip) {
        ತಾರ್ಕಿಕ allOk = ನಿಜ;
        // Copy nodep's list of function I/O to the new dpip c function
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (portp->isIO()) {
                    // Move it to new function
                    AstVar* ಸ್ಥಿರಾಂಕ newPortp = portp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    newPortp->funcLocal(ನಿಜ);
                    dpip->ಕೂಡಿಸು_Argsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    ಆದರೆ (!portp->basicp() && !portp->dtypep()->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct()) {
                        allOk = ಸುಳ್ಳು;
                        portp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: DPI argument of type "
                                << portp->dtypep()->prettyTypeName() << '\n'
                                << portp->warn_ಇನ್ನಷ್ಟು()
                                << "... For best portability, use bit, byte, int, or longint");
                        // We don't warn on logic either, although the 4-stateness is lost.
                        // That's what other simulators do.
                    }
                }
            }
        }
        ಹಿಂತಿರುಗಿ allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isDpiCIdentifier(ಸ್ಥಿರಾಂಕ string& name) {
        ಹಿಂತಿರುಗಿ !name.ಖಾಲಿಗಳು() && VString::isIdentifier(name)
               && !std::ಅಂಕಿಯೇ(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_ಪದಗಳು::isKeyword(name).ಖಾಲಿಗಳು();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    ತಾರ್ಕಿಕ checkDpiStruct(ಸ್ಥಿರಾಂಕ AstVar* portp, ಸ್ಥಿರಾಂಕ AstStructDType* sdtypep, ತಾರ್ಕಿಕ nested) {
        ಸ್ಥಿರಾಂಕ std::string name = sdtypep->dpiName();
        ಆದರೆ (!(nested && name.ಖಾಲಿಗಳು()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(ನಿಜ) << "\n"
                              << portp->warn_ಇನ್ನಷ್ಟು()
                              << "... Suggest use a typedef with a C identifier name");
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಆದರೆ (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep = itemp->subDTypep()->skipRefp();
            ಆಗ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ subp = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType);
            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            ಆದರೆ (dtypep->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct()) {
                ಆದರೆ (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), ನಿಜ)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(ನಿಜ)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        ಆದರೆ (name.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಸ್ಥಿರಾಂಕ AstStructDType* ಸ್ಥಿರಾಂಕ firstp = m_dpiStructs.emplace(name, sdtypep).ಮೊದಲ->ಎರಡನೇ;
        ಆದರೆ (firstp->dpiTypedef() == sdtypep->dpiTypedef()) ಹಿಂತಿರುಗಿ ನಿಜ;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    ತಾರ್ಕಿಕ check_Dpi_Struct_ವಾದಗಳು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* nodep) {
        ತಾರ್ಕಿಕ allOk = ನಿಜ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var);
            ಆದರೆ (portp && portp->isIO() && portp->dtypep()->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       ಸುಳ್ಳು)
                        && allOk;
            }
        }
        ಹಿಂತಿರುಗಿ allOk;
    }

    ಶೂನ್ಯ bodyDpiImportFunc(Ast_ಗಂಟು_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        ಆದರೆ (v3Global.opt.profExec()) {
            cfuncp->ಕೂಡಿಸು_Stmtsp(AstCStmt::prof_Exec_Section_ತಳ್ಳು(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string ವಾದಗಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ಸ್ಥಿರಾಂಕ portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                ಆದರೆ (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    ಆದರೆ (ವಾದಗಳು != "") ವಾದಗಳು += ", ";

                    ಆದರೆ (portp->isDpiOpenArray()) {
                        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, ಸರತಿ_DType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ಸ್ಥಿರಾಂಕ string propName = portp->name() + "__Vopenprops";
                        ಸ್ಥಿರಾಂಕ string prop_ಸಂಕೇತ = portp->vlPropDecl(propName);
                        cfuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{portp->fileline(), prop_ಸಂಕೇತ});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ಸ್ಥಿರಾಂಕ string name = portp->name() + "__Vopenarray";
                        ಸ್ಥಿರಾಂಕ string var_ಸಂಕೇತ
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{portp->fileline(), var_ಸಂಕೇತ});
                        ವಾದಗಳು += "&" + name;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // Unpacked structures are passed by reference from a C array
                        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isStruct = portp->dtypep()->ಒಳಗೊಂಡಿದೆ_Unpacked_Struct();
                        ಆದರೆ (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            ಆದರೆ (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) ವಾದಗಳು += "&";
                        }

                        ವಾದಗಳು += portp->name() + tmpSuffixp;

                        cfuncp->ಕೂಡಿಸು_Stmtsp(create_Dpi_ತಾತ್ಕಾಲಿಕ(portp, tmpSuffixp));
                        ಆದರೆ (portp->isNonOutput()) {
                            cfuncp->ಕೂಡಿಸು_Stmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ನಿಜ)
                                    : create_ನಿಯೋಜಿಸು_ಆಂತರಿಕ_ಗಮ್ಯ_Dpi(portp, ಸುಳ್ಳು, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        ಆದರೆ (nodep->dpiContext()) {
            ಸ್ಥಿರಾಂಕ string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtype_ಗಣ_Void();
            callp->argTypes(ವಾದಗಳು);
            ಆದರೆ (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->ಕೂಡಿಸು_Stmtsp(create_Dpi_ತಾತ್ಕಾಲಿಕ(rtnvscp->varp(), tmpSuffixp));
                ಸ್ಥಿರಾಂಕ std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{nodep->fileline()};
                cstmtp->ಕೂಡಿಸು(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->ಕೂಡಿಸು(" = ");
                cstmtp->ಕೂಡಿಸು(callp);  // RHS
                cstmtp->ಕೂಡಿಸು(";");
                cfuncp->ಕೂಡಿಸು_Stmtsp(cstmtp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Othervise just call it
                cfuncp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                portp->protect(ಸುಳ್ಳು);  // No additional exposure - already part of shown proto
                ಆದರೆ (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ಸ್ಥಿರಾಂಕ portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->ಕೂಡಿಸು_Stmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ಸುಳ್ಳು));
                }
            }
        }

        ಆದರೆ (v3Global.opt.profExec()) {
            cfuncp->ಕೂಡಿಸು_Stmtsp(AstCStmt::prof_Exec_Section_ತೆಗೆ(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        ಆದರೆ (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        ಹಿಂತಿರುಗಿ dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_ಗಂಟು_FTask* nodep, ತಾರ್ಕಿಕ ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
        AstVar* rtnvarp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->isFunction()) {
            AstVar* ಸ್ಥಿರಾಂಕ portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            ಆದರೆ (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = portp->dtypep()->basicp();
                ಆದರೆ (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(ನಿಜ)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                }
                ಆದರೆ (!bdtypep->isDpiPrimitive()) {
                    ಆದರೆ (bdtypep->is_Dpi_ಬಿಟ್_Vec() && portp->ಅಗಲ() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warn_ಇನ್ನಷ್ಟು()
                                       + "... Suggest make it an output argument instead?");
                    }
                    ಆದರೆ (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->task_ಬಹಿರಂಗ()) {
                ಆದರೆ (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warn_ಇನ್ನಷ್ಟು()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            ಆದರೆ (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(ಸುಳ್ಳು);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, ಸುಳ್ಳು);
            rtnvarp = portp;
            rtnvarp->funcLocal(ನಿಜ);
            rtnvarp->noCReset(ನಿಜ);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            ಆದರೆ (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(ಸುಳ್ಳು);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpi_Open_ಪೋಷಕ() && nodep->dpi_Open_ಮಗು()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = check_Dpi_Struct_ವಾದಗಳು(nodep) ? getDpiFunc(nodep, rtnvarp) : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!dpiFuncp || (nodep->dpiImport() && nodep->dpi_Open_ಪೋಷಕ())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                ಆದರೆ (rtnvarp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
        }

        AstVarScope* rtnvscp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (rtnvarp) {
            rtnvscp = ಹೊಸ AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->ಕೂಡಿಸು_Varsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string ಪೂರ್ವಪ್ರತ್ಯಯ;
        ಆದರೆ (nodep->dpiImport()) {
            ಪೂರ್ವಪ್ರತ್ಯಯ = "__Vdpiimwrap_";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->dpiExport()) {
            ಪೂರ್ವಪ್ರತ್ಯಯ = "__Vdpiexp_";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ftaskNoInline) {
            ಪೂರ್ವಪ್ರತ್ಯಯ = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        ಆದರೆ (!nodep->task_ಬಹಿರಂಗ() && !nodep->class_ವಿಧಾನ()) suffix = "_" + m_scopep->nameDotless();
        ಸ್ಥಿರಾಂಕ string name = ((nodep->name() == "new") ? "new" : ಪೂರ್ವಪ್ರತ್ಯಯ + nodep->name() + suffix);
        AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp = ಹೊಸ AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->task_ಬಹಿರಂಗ() && rtnvarp) ? rtnvarp->cPubArgType(ನಿಜ, ನಿಜ) : "")};
        // It's ok to combine imports because this is just a wrapper;
        // duplicate wrappers can get merged.
        cfuncp->dontCombine(!nodep->dpiImport());
        cfuncp->ನಮೂದು_ಬಿಂದು(!nodep->dpiImport());
        cfuncp->func_ಬಹಿರಂಗ(nodep->task_ಬಹಿರಂಗ());
        cfuncp->dpiContext(nodep->dpiContext());
        cfuncp->dpiExportImpl(nodep->dpiExport());
        cfuncp->dpiImportWrapper(nodep->dpiImport());
        cfuncp->recursive(nodep->recursive());
        // Hardcoded based on UVM usage; TODO make a verilated_std.vlt control for these
        cfuncp->unlikely(nodep->name() == "uvm_report_error" || nodep->name() == "uvm_report_info"
                         || nodep->name() == "uvm_report_fatal"
                         || nodep->name() == "uvm_report_warning");
        ಆದರೆ (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(ನಿಜ);
            cfuncp->isLoose(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            cfuncp->isStatic(ಸುಳ್ಳು);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpi_ಶುದ್ಧ(nodep->dpi_ಶುದ್ಧ());
        ಆದರೆ (nodep->name() == "new") cfuncp->isConstructor(ನಿಜ);
        ಆದರೆ (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(ನಿಜ);
        ಆದರೆ (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        ಆದರೆ (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->task_ಬಹಿರಂಗ()) || v3Global.opt.profExec());
        ಆದರೆ (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        ಆದರೆ (!nodep->dpiImport() && !nodep->task_ಬಹಿರಂಗ()) {
            // Need symbol table
            ಆದರೆ (cfuncp->isConstructor()) {
                ತಾರ್ಕಿಕ isInterfaceClass = ಸುಳ್ಳು;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                ಆದರೆ (!isInterfaceClass) {
                    ಸ್ಥಿರಾಂಕ string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->ಕೂಡಿಸು_Stmtsp(
                        ಹೊಸ AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        ಆದರೆ (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_ಒಳಹರಿವು_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_ಒಳಹರಿವು_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_ಒಳಹರಿವು_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        ಆದರೆ (nodep->dpiExport()) {
            AstScopeName* ಸ್ಥಿರಾಂಕ snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(ನಿಜ);
            snp->unlink_Fr_ಹಿಂಭಾಗ();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        ಗಾಗಿ (Ast_ಗಂಟು *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlink_Fr_ಹಿಂಭಾಗ();
                    ತಳ್ಳು_Deletep(portp);
                    portp->user2p(const_Pool_ಕೋಷ್ಟಕ(portp));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, ಸುಳ್ಳು);
                        portp->funcLocal(ನಿಜ);
                        ಆದರೆ (portp->valuep()) ತಳ್ಳು_Deletep(portp->valuep()->unlink_Fr_ಹಿಂಭಾಗ());
                        cfuncp->ಕೂಡಿಸು_Argsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        ಆದರೆ (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = portp->dtypep()->skipRefp();
                            ಆದರೆ (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(ನಿಜ);
                    }
                    AstVarScope* ಸ್ಥಿರಾಂಕ newvscp
                        = ಹೊಸ AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->ಕೂಡಿಸು_Varsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        ಆದರೆ (rtnvarp) cfuncp->ಕೂಡಿಸು_Argsp(rtnvarp);

        // Move body
        Ast_ಗಂಟು* bodysp = nodep->stmtsp();
        ಆದರೆ (bodysp) {
            unlinkAndClone(nodep, bodysp, ನಿಜ);
            AstBegin* ಸ್ಥಿರಾಂಕ tempp
                = ಹೊಸ AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, ಸುಳ್ಳು};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            ಗಾಗಿ (Ast_ಗಂಟು *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                    ಆದರೆ (portp->isIO()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(portp->unlink_Fr_ಹಿಂಭಾಗ()), portp);
                }
            }
            ಆದರೆ (tempp->stmtsp()) cfuncp->ಕೂಡಿಸು_Stmtsp(tempp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            VL_DO_DANGLING(tempp->delete_ವೃಕ್ಷ(), tempp);
        }
        ಆದರೆ (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        ಆದರೆ (rtnvscp && nodep->task_ಬಹಿರಂಗ()) {
            cfuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCReturn{
                rtnvscp->fileline(), ಹೊಸ AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        ಆದರೆ (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            ತಾರ್ಕಿಕ writesNonLocals = ಸುಳ್ಳು;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                ಆದರೆ (varp->isFuncLocal()) ಹಿಂತಿರುಗಿ;
                // Check if written
                ಆದರೆ (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->ಗಣ_Written_By_Dpi();
                    // Remember we had some
                    writesNonLocals = ನಿಜ;
                }
                // Check if read
                ಆದರೆ (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->ಗಣ_Read_By_Dpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            ಆದರೆ (writesNonLocals) {
                AstVarScope* ಸ್ಥಿರಾಂಕ dpiExportTriggerp = getDpiExporTrigger();
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp
                    = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                ಆದರೆ (v3Global.opt.systemC()) {
                    assignp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        ಆದರೆ (nodep->needProcess()) cfuncp->ಗಣ_Need_Process();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        ಹಿಂತಿರುಗಿ cfuncp;
    }

    ಶೂನ್ಯ iterateIntoFTask(Ast_ಗಂಟು_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    ಶೂನ್ಯ ಸೇರಿಸು_Before_Stmt(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
    }
    ಶೂನ್ಯ process_ವಾದಗಳು(Ast_ಗಂಟು_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        ಗಾಗಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(argp->exprp(), ConsArray);
            ಆದರೆ (!arrayp) ಮುಂದುವರಿ;

            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = arrayp->fileline();
            AstVarScope* ಸ್ಥಿರಾಂಕ substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->is_ಆಂತರಿಕ(ನಿಜ);

            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlink_Fr_ಹಿಂಭಾಗ()};

            Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ exprstmtp = ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                flp, assignp, ಹೊಸ AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    ತಾರ್ಕಿಕ isIfaceLocalImpure(Ast_ಗಂಟು_FTask* nodep, Ast_ಗಂಟು* impurep) {
        ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = m_statep->getScope(nodep);
        ಆದರೆ (!TaskStateVisitor::isIfaceFTaskScope(scopep)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(impurep, VarRef);
        ಆದರೆ (!refp || !refp->varScopep()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಹಿಂತಿರುಗಿ refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) {
            // Make sure that the return expression is converted only once
            ಹಿಂತಿರುಗಿ;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ withExprp = VN_AS(nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        nodep->ಕೂಡಿಸು_Exprp(ಹೊಸ AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCNew* nodep) override {
        VL_RESTORER(m_ಒಳ_New);
        m_ಒಳ_New = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (m_ಒಳ_Sensesp && !nodep->is_ಶುದ್ಧ()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(ಶೂನ್ಯಸೂಚಕ);  // So V3Broken doesn't complain
            ಹಿಂತಿರುಗಿ;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        ಆದರೆ (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_ಎಣಿಕೆ(), nodep, "No non-inline task, but no error issued");
            ಆದರೆ (VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
            ಹಿಂತಿರುಗಿ;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->taskp()->isFunction()) {
            AstVar* ಸ್ಥಿರಾಂಕ fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            ಆದರೆ (Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(nodep->backp(), ನಿಯೋಜಿಸು)) {
                ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ಸ್ಥಿರಾಂಕ vscp = lhsp->varScopep();
                    ಆದರೆ (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_ವೃಕ್ಷ(
                            fvarp->dtypep()->skipRefp())
                        && !has_Ref_ವಾದ(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            ಆದರೆ (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_ಗಂಟು* beginp;
        AstCNew* cnewp = ಶೂನ್ಯಸೂಚಕ;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ virtualIfaceCall
            = TaskStateVisitor::is_Virtual_Iface_ವಿಧಾನ_Call(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        ಆದರೆ (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            process_ವಾದಗಳು(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } ಇಲ್ಲದಿದ್ದರೆ {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        ಆದರೆ (VN_IS(nodep, New)) {  // New not legal as while() condition
            ಸೇರಿಸು_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->backp(), ಗಂಟು_ನಿಯೋಜಿಸು)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            ಸೇರಿಸು_Before_Stmt(nodep, beginp);
            AstVarRef* ಸ್ಥಿರಾಂಕ outrefp = ಹೊಸ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ಸ್ಥಿರಾಂಕ outrefp = ಹೊಸ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* lambdap = ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{nodep->fileline(), beginp, outrefp};

            ಆದರೆ (m_ಒಳ_New) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                newvarp->funcLocal(ನಿಜ);

                // Create a new VarScope for the new variable
                AstVarScope* ಸ್ಥಿರಾಂಕ newvscp
                    = ಹೊಸ AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->ಕೂಡಿಸು_Varsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreach_And_ಮುಂದಿನ([outvscp, newvscp](AstVarRef* refp) {
                    ಆದರೆ (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->has_ಫಲಿತಾಂಶ(ಸುಳ್ಳು);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ಸ್ಥಿರಾಂಕ exprp = ಹೊಸ AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtype_ಗಣ_String();
                lambdap->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            VIsCached::clear_Cache_ವೃಕ್ಷ();
        } ಇಲ್ಲದಿದ್ದರೆ {  // VN_IS(nodep->backp(), StmtExpr)
            ಸೇರಿಸು_Before_Stmt(nodep, beginp);
            ಆದರೆ (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        ಆದರೆ (!nodep->user1_ಗಣ_Once()) {  // Just one creation needed per function
            // Expand functions in it
            ಪೂರ್ಣಾಂಕ modes = 0;
            ಆದರೆ (nodep->dpiImport()) modes++;
            ಆದರೆ (nodep->dpiExport()) modes++;
            ಆದರೆ (nodep->task_ಬಹಿರಂಗ()) modes++;
            ಆದರೆ (nodep->class_ವಿಧಾನ()) modes++;
            ಆದರೆ (v3Global.opt.protectIds() && nodep->task_ಬಹಿರಂಗ()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            ಆದರೆ (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ noInline = m_statep->ftaskNoInline(nodep);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            ಆದರೆ (noInline && !nodep->class_ವಿಧಾನ() && !nodep->recursive()) {
                ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ impurep = m_statep->checkImpure(nodep)) {
                    ಆದರೆ (!isIfaceLocalImpure(nodep, impurep)) {
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

            ಆದರೆ (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->task_ಬಹಿರಂಗ()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ clonedFuncp = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                ಆದರೆ (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ makeNonInlineFunc = noInline || needsNonInlineCFunc;
                ಆದರೆ (AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(cfuncp);
                    ಆದರೆ (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            ಆದರೆ (nodep->isFunction()) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* ಸ್ಥಿರಾಂಕ vscp = m_statep->ಕಂಡುಹಿಡಿ_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(vscp->unlink_Fr_ಹಿಂಭಾಗ()), vscp);
                }
            }
            ಗಾಗಿ (Ast_ಗಂಟು *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ಸ್ಥಿರಾಂಕ vscp = m_statep->ಕಂಡುಹಿಡಿ_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(vscp->unlink_Fr_ಹಿಂಭಾಗ()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Force* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::ಸದಿಶ<Ast_ಗಂಟು_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_ಗಂಟು_FTask_Ref* refp) { refs.ತಳ್ಳು_ಹಿಂಭಾಗ(refp); });
        ಗಾಗಿ (Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ಸ್ಥಿರಾಂಕ interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
            Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{
                nodep->fileline(), ಹೊಸ Ast_Sen_ಅಂಶ{nodep->fileline(), Ast_Sen_ಅಂಶ::Combo{}}};
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep
                = ಹೊಸ Ast_ಸಕ್ರಿಯ{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
            activep->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, ಶೂನ್ಯಸೂಚಕ, assignp});
            m_scopep->ಕೂಡಿಸು_Blocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(ಹೊಸ AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        ಆದರೆ (!nodep->exprp()) VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        UASSERT_OBJ(!m_ಒಳ_Sensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_ಒಳ_Sensesp);
        m_ಒಳ_Sensesp = ನಿಜ;
        iterateChildren(nodep);
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    TaskVisitor(AstNetlist* nodep, TaskStateVisitor* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~TaskVisitor() {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Functions inlined", m_statInlines);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Hierarchical DPI wrappers with costs",
                         m_statHierDpisWithCosts);
    }
};

//######################################################################
// Mark interface members under timing controls of interface CFuncs as interface-sensed

ವರ್ಗ TaskIfaceSensVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    ತಾರ್ಕಿಕ m_underIfaceFunc = ಸುಳ್ಳು;  // Under a CFunc owned by an interface scope
    ತಾರ್ಕಿಕ m_underSenses = ಸುಳ್ಳು;  // Under a sensitivity expression of such a CFunc

    // METHODS
    ಶೂನ್ಯ markSensesAndIterate(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!m_underIfaceFunc) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_underSenses);
        m_underSenses = ನಿಜ;
        iterate_And_ಮುಂದಿನ_Const_Null(nodep);
    }
    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ವೃಕ್ಷ* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (!m_underSenses) ಹಿಂತಿರುಗಿ;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        ಆದರೆ (nodep->varp()->isFuncLocal()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (AstIface* ಸ್ಥಿರಾಂಕ ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Task class functions

ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_ಗಂಟು_FTask_Ref* nodep, Ast_ಗಂಟು* taskStmtsp,
                                    V3TaskConnectState* statep, ತಾರ್ಕಿಕ makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ಸ್ಥಿರಾಂಕ std::string, ಪೂರ್ಣಾಂಕ> name_ಗಮ್ಯ_ಸೂಚ್ಯಂಕ;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    ಪೂರ್ಣಾಂಕ tpinnum = 0;
    AstVar* sformatp = ಶೂನ್ಯಸೂಚಕ;
    ಗಾಗಿ (Ast_ಗಂಟು* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
            ಆದರೆ (portp->isIO()) {
                tconnects.emplace_ಹಿಂಭಾಗ(portp, static_cast<AstArg*>(ಶೂನ್ಯಸೂಚಕ));
                name_ಗಮ್ಯ_ಸೂಚ್ಯಂಕ.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                ಆದರೆ (portp->attrSFormat()) {
                    sformatp = portp;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    ಪೂರ್ಣಾಂಕ ppinnum = 0;
    ತಾರ್ಕಿಕ reorganize = ಸುಳ್ಳು;
    ಗಾಗಿ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        ಆದರೆ (!argp->name().ಖಾಲಿಗಳು()) {
            // By name
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = name_ಗಮ್ಯ_ಸೂಚ್ಯಂಕ.ಕಂಡುಹಿಡಿ(argp->name());
            ಆದರೆ (it == name_ಗಮ್ಯ_ಸೂಚ್ಯಂಕ.ಕೊನೆ()) {
                ಆದರೆ (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warn_ಇನ್ನಷ್ಟು() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), argp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (tconnects[it->ಎರಡನೇ].ಎರಡನೇ && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->ಎರಡನೇ].ಎರಡನೇ->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                }
                tconnects[it->ಎರಡನೇ].ಎರಡನೇ = argp;
                reorganize = ನಿಜ;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {  // By pin number
            ಆದರೆ (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ppinnum >= tpinnum) {
                ಆದರೆ (sformatp) {
                    tconnects.emplace_ಹಿಂಭಾಗ(sformatp, static_cast<AstArg*>(ಶೂನ್ಯಸೂಚಕ));
                    tconnects[ppinnum].ಎರಡನೇ = argp;
                    ++tpinnum;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warn_ಇನ್ನಷ್ಟು() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), argp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                tconnects[ppinnum].ಎರಡನೇ = argp;
            }
        }
        ++ppinnum;
    }

    ಆದರೆ (!makeChanges) ಹಿಂತಿರುಗಿ tconnects;

    // Connect missing ones
    VInsertion_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < tpinnum; ++i) {
        AstVar* ಸ್ಥಿರಾಂಕ portp = tconnects[i].ಮೊದಲ;
        ಆದರೆ (!tconnects[i].ಎರಡನೇ || !tconnects[i].ಎರಡನೇ->exprp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newvaluep = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = ಹೊಸ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstFuncRef* ಸ್ಥಿರಾಂಕ funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ funcp = funcRefp->taskp();
                ಆದರೆ (funcp->class_ವಿಧಾನ() && funcp->isStatic()) newvaluep = funcRefp;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            ಆದರೆ (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
                ಆದರೆ (!VN_IS(newvaluep, Const)) {
                    ಆದರೆ (statep) {
                        portp->pin_ಅಂಕೆ(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.ಸೇರಿಸು(portp);
                    } ಇಲ್ಲದಿದ್ದರೆ {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = ಹೊಸ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_ವೃಕ್ಷ(ನಿಜ) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstArg{nodep->fileline(), portp->name(), newvaluep};
            ಆದರೆ (tconnects[i].ಎರಡನೇ) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].ಎರಡನೇ->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(),
                            tconnects[i].ಎರಡನೇ = ಶೂನ್ಯಸೂಚಕ);
            }
            tconnects[i].ಎರಡನೇ = newp;
            reorganize = ನಿಜ;
        }
        ಆದರೆ (tconnects[i].ಎರಡನೇ) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].ಎರಡನೇ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
        AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
        argp->name("");  // Can forget name as will add back in pin order
    }

    ಆದರೆ (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        ಆಗ (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlink_Fr_ಹಿಂಭಾಗ();
        }
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < tpinnum; ++i) {
            AstArg* ಸ್ಥಿರಾಂಕ argp = tconnects[i].ಎರಡನೇ;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->ಕೂಡಿಸು_Argsp(argp);
        }
    }

    ಆದರೆ (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_ವೃಕ್ಷ("-  ftref-out: ");
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvt_ಗಮ್ಯ_Hex(tconnects[i].ಮೊದಲ)
                               << "  conn=" << cvt_ಗಮ್ಯ_Hex(tconnects[i].ಎರಡನೇ));
        }
    }  // LCOV_EXCL_STOP

    ಆದರೆ (!argWrap.ಖಾಲಿಗಳು()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        ಹಿಂತಿರುಗಿ taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            ಶೂನ್ಯಸೂಚಕ);
    }
    ಹಿಂತಿರುಗಿ tconnects;
}

ಶೂನ್ಯ V3Task::taskConnectWrap(Ast_ಗಂಟು_FTask_Ref* nodep, ಸ್ಥಿರಾಂಕ V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ಸ್ಥಿರಾಂಕ VInsertion_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& argWrap) {
    statep->ಗಣ_Did_Wrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVar* varp : argWrap) newname += "_" + cvt_ಗಮ್ಯ_Str(varp->pin_ಅಂಕೆ());
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(ಶೂನ್ಯಸೂಚಕ));
    ಆದರೆ (pair.ಎರಡನೇ) {
        pair.ಮೊದಲ->ಎರಡನೇ = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ newTaskp = pair.ಮೊದಲ->ಎರಡನೇ;

    // Remove the defaulted arguments from original outside call
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
        AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
        ಆದರೆ (argWrap.exists(portp)) {  // Removed arg
            statep->ತಳ್ಳು_Deletep(argp->unlink_Fr_ಹಿಂಭಾಗ());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_ಗಂಟು_FTask* V3Task::taskConnectWrapNew(Ast_ಗಂಟು_FTask* taskp, ಸ್ಥಿರಾಂಕ string& newname,
                                         ಸ್ಥಿರಾಂಕ V3TaskConnects& tconnects,
                                         ಸ್ಥಿರಾಂಕ VInsertion_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*>& argWrap) {
    std::map<ಸ್ಥಿರಾಂಕ AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newTaskp);

    Ast_ಗಂಟು_FTask_Ref* newCallp = ಶೂನ್ಯಸೂಚಕ;
    Ast_ಗಂಟು* newCallInsertp = ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (VN_IS(taskp, Func)) {
        AstVar* ಸ್ಥಿರಾಂಕ fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ಸ್ಥಿರಾಂಕ newFVarp = fvarp->clone_ವೃಕ್ಷ(ನಿಜ);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = ಹೊಸ AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು{taskp->fileline(),
                            ಹೊಸ AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(taskp, Task)) {
        newCallp = ಹೊಸ AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{taskp->fileline(), newCallp};
    } ಇಲ್ಲದಿದ್ದರೆ {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
        AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
        AstVar* newPortp;
        ಆದರೆ (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = ಹೊಸ AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(ನಿಜ);
            ಆದರೆ (newPortp->valuep()) newPortp->valuep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            newTaskp->ಕೂಡಿಸು_Stmtsp(newPortp);
        } ಇಲ್ಲದಿದ್ದರೆ {  // Defaulting arg
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = VN_AS(portp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಆದರೆ ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ಸ್ಥಿರಾಂಕ VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_AS(valuep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), VarRef);
                refp->access(refAccess);
                AstArg* ಸ್ಥಿರಾಂಕ newArgp = ಹೊಸ AstArg{portp->fileline(), portp->name(), refp};
                newCallp->ಕೂಡಿಸು_Argsp(newArgp);
                ಮುಂದುವರಿ;
            }
            // Create local temporary
            newPortp = ಹೊಸ AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(ನಿಜ);
            newTaskp->ಕೂಡಿಸು_Stmtsp(newPortp);
            // Runtime-assign it to the default
            ಆದರೆ (!VN_IS(valuep, ಖಾಲಿಗಳು_ಸರತಿ)) {
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ newAssignp
                    = ಹೊಸ Ast_ನಿಯೋಜಿಸು{valuep->fileline(),
                                    ಹೊಸ AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_ವೃಕ್ಷ(ನಿಜ)};
                newTaskp->ಕೂಡಿಸು_Stmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ಸ್ಥಿರಾಂಕ newArgp = ಹೊಸ AstArg{
            portp->fileline(), portp->name(),
            ಹೊಸ AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->ಕೂಡಿಸು_Argsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->ಕೂಡಿಸು_Stmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = oldNewVars.ಕಂಡುಹಿಡಿ(refp->varp());
        ಆದರೆ (it != oldNewVars.ಕೊನೆ()) refp->varp(it->ಎರಡನೇ);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    ಹಿಂತಿರುಗಿ newTaskp;
}

string V3Task::ನಿಯೋಜಿಸು_ಆಂತರಿಕ_ಗಮ್ಯ_Dpi(AstVar* portp, ತಾರ್ಕಿಕ isPtr, ಸ್ಥಿರಾಂಕ string& frSuffix,
                                   ಸ್ಥಿರಾಂಕ string& ಗಮ್ಯ_Suffix, ಸ್ಥಿರಾಂಕ string& fr_ಪೂರ್ವಪ್ರತ್ಯಯ) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ಸ್ಥಿರಾಂಕ string frName = fr_ಪೂರ್ವಪ್ರತ್ಯಯ + portp->name() + frSuffix;
    ಸ್ಥಿರಾಂಕ string ಗಮ್ಯ_Name = portp->name() + ಗಮ್ಯ_Suffix;
    ಸ್ಥಿರಾಂಕ string idx = portp->name() + "__Vidx";
    ಗಾತ್ರ_t unpack_ಗಾತ್ರ = 1;  // non-unpacked array is treated as size 1
    ಪೂರ್ಣಾಂಕ unpackDim = 0;
    ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpack_ಗಾತ್ರ = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(ಸುಳ್ಳು).ಎರಡನೇ;
        ಆದರೆ (unpackDim > 0) UASSERT_OBJ(unpack_ಗಾತ್ರ > 0, portp, "size must be greater than 0");
    }
    ಆದರೆ (portp->basicp()->is_Dpi_ಬಿಟ್_Vec() || portp->basicp()->isDpiLogicVec()) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ is_ಬಿಟ್ = portp->basicp()->is_Dpi_ಬಿಟ್_Vec();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needsFor = unpack_ಗಾತ್ರ > 1;
        ಆದರೆ (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvt_ಗಮ್ಯ_Str(unpack_ಗಾತ್ರ) + "; ++"
                   + idx + ") ";
        }

        stmt += (is_ಬಿಟ್ ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvt_ಗಮ್ಯ_Str(portp->ಅಗಲ()) + ", ";
        stmt += ಗಮ್ಯ_Name;
        ಆದರೆ (needsFor) {
            stmt += " + " + cvt_ಗಮ್ಯ_Str(portp->dtypep()->skipRefp()->ಅಗಲ_ಪದಗಳು()) + " * " + idx;
        }
        stmt += ", ";
        ಆದರೆ (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } ಇಲ್ಲದಿದ್ದರೆ {
            stmt += frName + ")";
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        ಸ್ಥಿರಾಂಕ string unpackAt = unpack_ಗಾತ್ರ > 1 ? "[" + idx + "]" : "[0]";
        ಆದರೆ (unpackDim > 0) {
            ಸ್ಥಿರಾಂಕ string forStmt = unpack_ಗಾತ್ರ > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvt_ಗಮ್ಯ_Str(unpack_ಗಾತ್ರ) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + ಗಮ್ಯ_Name + unpackAt;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += ಗಮ್ಯ_Name;
        }
        stmt += " = ";
        ಆದರೆ (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        ಆದರೆ (unpackDim > 0) {
            stmt += "(&" + frName;
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } ಇಲ್ಲದಿದ್ದರೆ {
            stmt += frName;
        }
        ಆದರೆ (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    ಹಿಂತಿರುಗಿ stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::ನಿಯೋಜಿಸು_Dpi_ಗಮ್ಯ_ಆಂತರಿಕ(ಸ್ಥಿರಾಂಕ std::string& lhsName, AstVar* varp) {
    std::string cvt;
    ತಾರ್ಕಿಕ useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_ಗಮ್ಯ_ಆಂತರಿಕ_Cvt_Stmt(varp->dtypep());
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಒಟ್ಟು = strides.ಖಾಲಿಗಳು() ? 1 : strides[0].ಮೊದಲ * strides[0].ಎರಡನೇ;
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_ಪದಗಳು = varp->basicp()->ಅಗಲ_ಪದಗಳು();
    string statements;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಒಟ್ಟು; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& stride : strides) {
            lhs += "[" + std::ಗಮ್ಯ_string((i / stride.ಎರಡನೇ) % stride.ಮೊದಲ) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        ಆದರೆ (useSvVec) {
            ಸ್ಥಿರಾಂಕ std::string ಸ್ಥಾನಾಂತರ = std::ಗಮ್ಯ_string(i * ಅಗಲ_ಪದಗಳು);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + ಸ್ಥಾನಾಂತರ + ");\n";
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ std::string elem = strides.ಖಾಲಿಗಳು() ? "" : "[" + std::ಗಮ್ಯ_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    ಹಿಂತಿರುಗಿ statements;
}

ಶೂನ್ಯ V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ಸ್ಥಿರಾಂಕ TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("task", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
