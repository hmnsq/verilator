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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Task.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3LanguageWords.h"
#అన్తర్భావ "V3SharedTmps.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

తరగతి Task_పునాది_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Task_పునాది_శీర్షం, V3_లేఖాచిత్రం_శీర్షం);
    Ast_కణుపు* m_impurep = శూన్యసూచిక;  // Node causing impure function w/ outside references
    తార్కిక m_noInline = అబద్ధం;  // Marked with pragma
బహిరంగ:
    స్పష్టమైన Task_పునాది_శీర్షం(V3_లేఖాచిత్రం* graphp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp} {}
    ~Task_పునాది_శీర్షం() override = అప్రమేయ;
    తార్కిక శుద్ధ_3() స్థిరరాశి { తిరిగి m_impurep == శూన్యసూచిక; }
    Ast_కణుపు* impure_కణుపు() స్థిరరాశి { తిరిగి m_impurep; }
    శూన్య impure(Ast_కణుపు* nodep) { m_impurep = nodep; }
    తార్కిక noInline() స్థిరరాశి { తిరిగి m_noInline; }
    శూన్య noInline(తార్కిక flag) { m_noInline = flag; }
};

తరగతి Task_FTask_శీర్షం final : బహిరంగ Task_పునాది_శీర్షం {
    VL_RTTI_IMPL(Task_FTask_శీర్షం, Task_పునాది_శీర్షం);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_కణుపు_FTask* స్థిరరాశి m_nodep;
    AstCFunc* m_cFuncp = శూన్యసూచిక;
    తార్కిక m_needsNonInlineCFunc = అబద్ధం;  // Needs CFunc for selected non-inlined call sites

బహిరంగ:
    Task_FTask_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు_FTask* nodep)
        : Task_పునాది_శీర్షం{graphp}
        , m_nodep{nodep} {}
    ~Task_FTask_శీర్షం() override = అప్రమేయ;
    Ast_కణుపు_FTask* nodep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_nodep; }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి nodep()->name(); }
    string dotColor() స్థిరరాశి override { తిరిగి శుద్ధ_3() ? "black" : "red"; }
    AstCFunc* cFuncp() స్థిరరాశి { తిరిగి m_cFuncp; }
    శూన్య cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    తార్కిక needsNonInlineCFunc() స్థిరరాశి { తిరిగి m_needsNonInlineCFunc; }
    శూన్య needsNonInlineCFunc(తార్కిక flag) { m_needsNonInlineCFunc = flag; }
};

తరగతి Task_సంకేతం_శీర్షం final : బహిరంగ Task_పునాది_శీర్షం {
    VL_RTTI_IMPL(Task_సంకేతం_శీర్షం, Task_పునాది_శీర్షం);
    // Top vertex for all calls not under another task
బహిరంగ:
    స్పష్టమైన Task_సంకేతం_శీర్షం(V3_లేఖాచిత్రం* graphp)
        : Task_పునాది_శీర్షం{graphp} {}
    ~Task_సంకేతం_శీర్షం() override = అప్రమేయ;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి "*CODE*"; }
    string dotColor() స్థిరరాశి override { తిరిగి "green"; }
};

తరగతి Task_అంచు final : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    VL_RTTI_IMPL(Task_అంచు, V3_లేఖాచిత్రం_అంచు)
బహిరంగ:
    Task_అంచు(V3_లేఖాచిత్రం* graphp, Task_పునాది_శీర్షం* fromp, Task_పునాది_శీర్షం* శిఖరం)
        : V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, 1, అబద్ధం} {}
    ~Task_అంచు() override = అప్రమేయ;
    string dotLabel() స్థిరరాశి override { తిరిగి "w" + cvt_గమ్యం_Str(బరువు_2()); }
};

//######################################################################

తరగతి TaskStateVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;
    స్థిరరాశి VNUser4_లోపల_Use m_inuser4;

    // TYPES
    ఉపయోగించి Var_గమ్యం_Scope_Map = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    ఉపయోగించి Func_గమ్యం_Class_Map = std::unordered_map<స్థిరరాశి Ast_కణుపు_FTask*, AstClass*>;
    // MEMBERS
    Var_గమ్యం_Scope_Map m_var_గమ్యం_Scope_Map;  // Map for Var -> VarScope mappings
    Func_గమ్యం_Class_Map m_func_గమ్యం_Class_Map;  // Map for ctor func -> class
    Ast_కణుపు_FTask* m_ctorp = శూన్యసూచిక;  // Class constructor
    AstClass* m_classp = శూన్యసూచిక;  // Current class
    V3_లేఖాచిత్రం m_call_లేఖాచిత్రం;  // Task call graph
    Task_పునాది_శీర్షం* m_curVxp;  // Current vertex we're adding to
    std::సదిశ<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    తార్కిక m_under_పోర్టు_Var = అబద్ధం;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

బహిరంగ:
    // METHODS
    స్థిర తార్కిక is_Virtual_Iface_పద్ధతి_Call(స్థిరరాశి Ast_కణుపు_FTask_Ref* refp) {
        స్థిరరాశి Ast_పద్ధతి_Call* స్థిరరాశి mrefp = VN_CAST(refp, పద్ధతి_Call);
        ఒకవేళ (!mrefp) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి fromp = mrefp->fromp();
        ఒకవేళ (!fromp || !fromp->dtypep()) తిరిగి అబద్ధం;
        స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        తిరిగి ifrefp && ifrefp->isVirtual();
    }
    స్థిర తార్కిక isIfaceFTaskScope(స్థిరరాశి AstScope* scopep) {
        తిరిగి scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_కణుపు_FTask* nodep) {
        AstScope* స్థిరరాశి scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        తిరిగి scopep;
    }
    AstVarScope* కనుగొను_Var_Scope(AstScope* scopep, AstVar* nodep) {
        స్థిరరాశి స్వయంచాలక iter = m_var_గమ్యం_Scope_Map.కనుగొను(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_var_గమ్యం_Scope_Map.చివర(), nodep, "No scope for var");
        తిరిగి iter->రెండవ;
    }
    AstClass* getClassp(Ast_కణుపు_FTask* nodep) {
        AstClass* స్థిరరాశి classp = m_func_గమ్యం_Class_Map[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        తిరిగి classp;
    }
    శూన్య remapFuncClassp(Ast_కణుపు_FTask* nodep, Ast_కణుపు_FTask* newp) {
        m_func_గమ్యం_Class_Map[newp] = getClassp(nodep);
    }
    తార్కిక ftaskNoInline(Ast_కణుపు_FTask* nodep) { తిరిగి get_FTask_శీర్షం(nodep)->noInline(); }
    తార్కిక ftaskNeedsNonInlineCFunc(Ast_కణుపు_FTask* nodep) {
        తిరిగి get_FTask_శీర్షం(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_కణుపు_FTask* nodep) { తిరిగి get_FTask_శీర్షం(nodep)->cFuncp(); }
    శూన్య ftaskCFuncp(Ast_కణుపు_FTask* nodep, AstCFunc* cfuncp) {
        get_FTask_శీర్షం(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_కణుపు* checkImpure(Ast_కణుపు_FTask* nodep) { తిరిగి get_FTask_శీర్షం(nodep)->impure_కణుపు(); }

అంతరంగ:
    Task_FTask_శీర్షం* get_FTask_శీర్షం(Ast_కణుపు_FTask* nodep) {
        ఒకవేళ (!nodep->user4p()) nodep->user4p(కొత్త Task_FTask_శీర్షం{&m_call_లేఖాచిత్రం, nodep});
        తిరిగి static_cast<Task_FTask_శీర్షం*>(nodep->user4u().గమ్యం_లేఖాచిత్రం_శీర్షం());
    }

    శూన్య propagateImpureReason() {
        std::సదిశ<Task_FTask_శీర్షం*> workList;
        // Gather all impure FTask vertices
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_call_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (Task_FTask_శీర్షం* స్థిరరాశి ftVtxp = vtx.cast<Task_FTask_శీర్షం>()) {
                ఒకవేళ (!ftVtxp->శుద్ధ_3()) workList.emplace_వెనుక(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        అయితే (!workList.ఖాళీలు()) {
            Task_FTask_శీర్షం* స్థిరరాశి calleep = workList.వెనుక();
            workList.తీయి_వెనుక();
            UASSERT_OBJ(!calleep->శుద్ధ_3(), calleep->nodep(), "Should not enqueue pure vertex");
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : calleep->లోపల_అంచులు()) {
                ఒకవేళ (Task_FTask_శీర్షం* స్థిరరాశి callerp = అంచు.fromp()->cast<Task_FTask_శీర్షం>()) {
                    ఒకవేళ (!callerp->శుద్ధ_3()) కొనసాగు;  // Already processed or on workList
                    callerp->impure(calleep->impure_కణుపు());
                    workList.emplace_వెనుక(callerp);
                }
            }
        }
    }

    శూన్య decideInlining() {
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_call_లేఖాచిత్రం.vertices()) {
            Task_FTask_శీర్షం* స్థిరరాశి ftVtxp = vtx.cast<Task_FTask_శీర్షం>();
            ఒకవేళ (!ftVtxp) కొనసాగు;
            // Move on if already decided not to inline
            ఒకవేళ (ftVtxp->noInline()) కొనసాగు;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            ఒకవేళ (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(నిజం);
                కొనసాగు;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            ఒకవేళ (v3Global.opt.fInlineFuncsEager()) కొనసాగు;

            Ast_కణుపు_FTask* స్థిరరాశి ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            ఒకవేళ (!ftaskp->verilogFunction()) కొనసాగు;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            ఒకవేళ (ftaskp->task_ప్రజా()) కొనసాగు;
            // If the function is pure, it's OK not to inline, so don't do it
            ఒకవేళ (ftVtxp->శుద్ధ_3()) ftVtxp->noInline(నిజం);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        కొరకు (Ast_కణుపు* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVarScope* స్థిరరాశి vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* స్థిరరాశి varp = vscp->varp();
                ఒకవేళ (varp->isFuncLocal() || varp->is_వాడిన_Loop_Idx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_var_గమ్యం_Scope_Map.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        కొరకు (Ast_కణుపు* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి taskp = VN_CAST(stmtp, కణుపు_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        Task_FTask_శీర్షం* స్థిరరాశి taskVtxp = get_FTask_శీర్షం(nodep->taskp());
        కొత్త Task_అంచు{&m_call_లేఖాచిత్రం, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        ఒకవేళ (is_Virtual_Iface_పద్ధతి_Call(nodep)) taskVtxp->needsNonInlineCFunc(నిజం);
        // Do we have to disable inlining the function?
        స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        ఒకవేళ (!taskVtxp->noInline()) {  // Else short-circuit below
            కొరకు (స్థిరరాశి స్వయంచాలక& itr : tconnects) {
                స్థిరరాశి AstVar* స్థిరరాశి portp = itr.మొదటి;
                స్థిరరాశి AstArg* స్థిరరాశి argp = itr.రెండవ;
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి pinp = argp->exprp()) {
                    ఒకవేళ ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(నిజం);
                    }
                }
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = get_FTask_శీర్షం(nodep);
        ఒకవేళ (nodep->dpiImport()) m_curVxp->noInline(నిజం);
        ఒకవేళ (nodep->class_పద్ధతి()) m_curVxp->noInline(నిజం);  // Until V3Task supports it
        ఒకవేళ (nodep->recursive()) m_curVxp->noInline(నిజం);
        ఒకవేళ (nodep->isConstructor()) {
            m_curVxp->noInline(నిజం);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_func_గమ్యం_Class_Map[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstPragma* nodep) override {
        ఒకవేళ (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(నిజం);
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        } లేకపోతే {
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        VL_RESTORER(m_under_పోర్టు_Var);
        ఒకవేళ (nodep->isIO()) m_under_పోర్టు_Var = నిజం;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (m_under_పోర్టు_Var) తిరిగి;
        AstVar* స్థిరరాశి varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        ఒకవేళ (nodep->access().isReadOnly() && varp->const_Pool_నమోదు()) తిరిగి;
        ఒకవేళ (varp->user4u().గమ్యం_లేఖాచిత్రం_శీర్షం() != m_curVxp) {
            ఒకవేళ (m_curVxp->శుద్ధ_3() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    శూన్య సందర్శించు(AstClass* nodep) override {
        // Move initial statements into the constructor
        VL_RESTORER_CLEAR(m_initialps);
        VL_RESTORER(m_ctorp);
        VL_RESTORER(m_classp);
        m_ctorp = శూన్యసూచిక;
        m_classp = nodep;
        {  // Find m_initialps, m_ctor
            iterateChildren(nodep);
        }
        UASSERT_OBJ(m_ctorp, nodep, "class constructor missing");  // LinkDot always makes it
        Ast_కణుపు* insertp = శూన్యసూచిక;
        కొరకు (AstInitialAutomatic* initialp : m_initialps) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి movep = initialp->stmtsp()) {
                movep->unlink_Fr_వెనుక_With_తదుపరి();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_కణుపు::కూడు_తదుపరి_Null(insertp, movep);
            }
            VL_DO_DANGLING(నెట్టు_Deletep(initialp->unlink_Fr_వెనుక()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    శూన్య సందర్శించు(AstInitialAutomatic* nodep) override {
        m_initialps.నెట్టు_వెనుక(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = కొత్త Task_సంకేతం_శీర్షం{&m_call_లేఖాచిత్రం};
        //
        iterate(nodep);
        //
        m_call_లేఖాచిత్రం.remove_Redundant_అంచులు_కూడిక(&Task_అంచు::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        ఒకవేళ (dump_లేఖాచిత్రం_Level()) m_call_లేఖాచిత్రం.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = అప్రమేయ;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

నిర్మాణం TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    స్థిర std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> unpackDimsAndStrides(AstVar* varp) {
        Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypep()->skipRefp();
        std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> dimStrides;
        AstUnpackArrayDType* స్థిరరాశి unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ఒకవేళ (!unpackp) తిరిగి dimStrides;

        స్థిరరాశి std::సదిశ<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        స్థిరరాశి కొలత_t nDims = dims.కొలత();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].మొదటి = dims.వెనుక()->elementsConst();
        dimStrides[nDims - 1].రెండవ = 1;
        // Rest are densly packed
        కొరకు (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].మొదటి = dims[i]->elementsConst();
            dimStrides[i].రెండవ = dimStrides[i + 1].మొదటి * dimStrides[i + 1].రెండవ;
        }
        తిరిగి dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    స్థిర std::pair<std::string, తార్కిక> dpi_గమ్యం_అంతర్గత_Cvt_Stmt(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = dtypep->basicp();

        // DPI types using Primitive C types
        ఒకవేళ (basicp->keyword() == VBasicDTypeKwd::CHANDLE) తిరిగి {"VL_CVT_VP_Q(", అబద్ధం};
        ఒకవేళ (basicp->keyword() == VBasicDTypeKwd::STRING) తిరిగి {"VL_CVT_N_CSTR(", అబద్ధం};
        ఒకవేళ (basicp->isDpiPrimitive()) తిరిగి {"(", అబద్ధం};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->is_Dpi_బిట్టు_Vec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        స్థిరరాశి std::string vecType = basicp->is_Dpi_బిట్టు_Vec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        స్థిరరాశి అక్షరం కొలత_Char = dtypep->వెడల్పు() <= 8    ? 'C'
                              : dtypep->వెడల్పు() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        స్థిరరాశి std::string& కొలత = std::గమ్యం_string(dtypep->వెడల్పు());
        // cppcheck-suppress strPlusChar
        తిరిగి {"VL_SET_"s + కొలత_Char + "_" + vecType + "(" + కొలత + ", ", నిజం};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

తరగతి TaskVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // TYPES
    ఉపయోగించి DpiCFuncs = std::map<స్థిరరాశి string, std::tuple<Ast_కణుపు_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* స్థిరరాశి m_statep;  // Common state between visitors
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    Ast_శిఖరం_Scope* స్థిరరాశి m_శిఖరం_Scopep = v3Global.rootp()->శిఖరం_Scopep();  // The AstTopScope
    AstScope* m_scopep = శూన్యసూచిక;  // Current scope
    Ast_కణుపు* m_insStmtp = శూన్యసూచిక;  // Where to insert statement
    తార్కిక m_లోపల_Sensesp = అబద్ధం;  // Are we under a senitem?
    తార్కిక m_లోపల_New = అబద్ధం;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    పూర్ణాంకం m_uncon_Var_అంకె = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpi_పేర్లు;  // Map of all created DPI functions
    std::map<std::string, స్థిరరాశి AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, స్థిరరాశి string& name, AstVar* examplep) {
        AstVar* స్థిరరాశి newvarp = కొత్త AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(నిజం);
        funcp->కూడు_Varsp(newvarp);
        AstVarScope* స్థిరరాశి newvscp = కొత్త AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->కూడు_Varsp(newvscp);
        తిరిగి newvscp;
    }
    AstVarScope* create_ఆదానం_Var(AstCFunc* funcp, స్థిరరాశి string& name, VBasicDTypeKwd kwd) {
        AstVar* స్థిరరాశి newvarp
            = కొత్త AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->కనుగొను_Basic_DType(kwd)};
        newvarp->funcLocal(నిజం);
        newvarp->direction(VDirection::ఆదానం);
        funcp->కూడు_Argsp(newvarp);
        AstVarScope* స్థిరరాశి newvscp = కొత్త AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->కూడు_Varsp(newvscp);
        తిరిగి newvscp;
    }
    స్థిర AstVarScope* const_Pool_పట్టిక(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* స్థిరరాశి refp = V3ConstPool::కనుగొను(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* స్థిరరాశి vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_వృక్షం(), refp);
        తిరిగి vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(స్థిరరాశి Ast_కణుపు_FTask_Ref* refp, AstVar* invarp,
                                స్థిరరాశి std::string& suffix) {
        ఒకవేళ (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            తిరిగి const_Pool_పట్టిక(invarp);
        } లేకపోతే {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* స్థిరరాశి newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* స్థిరరాశి newvarp = newvscp->varp();
            newvarp->funcLocal(అబద్ధం);
            newvarp->propagateAttrFrom(invarp);
            newvarp->is_అంతర్గత(నిజం);
            తిరిగి newvscp;
        }
    }

    // Replace varrefs with new var pointer
    శూన్య relink(Ast_కణుపు* nodep) {
        nodep->foreach_And_తదుపరి([](AstVarRef* refp) {
            ఒకవేళ (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* స్థిరరాశి newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    Ast_కేటాయించు* connect_పోర్టు_Make_లోపల_కేటాయించు(Ast_కణుపు_సమాసం* pinp, AstVarScope* newvscp, తార్కిక శుద్ధ_Check) {
        // Create input assignment to go in FRONT of function call
        Ast_కణుపు_సమాసం* లోపల_Pinp = pinp;
        ఒకవేళ (AstResizeLValue* sinPinp = VN_CAST(లోపల_Pinp, ResizeLValue)) లోపల_Pinp = sinPinp->lhsp();
        Ast_కణుపు_సమాసం* లోపల_Pin_Clonep
            = శుద్ధ_Check ? లోపల_Pinp->clone_వృక్షం_శుద్ధ(నిజం) : లోపల_Pinp->clone_వృక్షం(నిజం);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        ఒకవేళ (arrayp && arrayp->is_తిరగేయి_యొక్క(లోపల_Pin_Clonep->dtypep())) {
            లోపల_Pin_Clonep
                = కొత్త Ast_Cvt_Unpack_గమ్యం_Unpack{pinp->fileline(), లోపల_Pin_Clonep, newvscp->dtypep()};
        }
        Ast_కేటాయించు* స్థిరరాశి assp = కొత్త Ast_కేటాయించు{
            pinp->fileline(), కొత్త AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            లోపల_Pin_Clonep};
        assp->fileline()->modifyWarnOff(V3_Error_సంకేతం::BLKSEQ, నిజం);  // Ok if in <= block
        తిరిగి assp;
    }
    Ast_కేటాయించు* connect_పోర్టు_Make_బహిర్గతం_కేటాయించు(AstVar* portp, Ast_కణుపు_సమాసం* pinp, AstVarScope* newvscp,
                                        తార్కిక శుద్ధ_Check) {
        // If needed, remap size of function to caller's output size
        Ast_కణుపు_సమాసం* బహిర్గతం_Pinp = pinp;
        Ast_కణుపు_సమాసం* postRhsp = కొత్త AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        ఒకవేళ (AstResizeLValue* soutPinp = VN_CAST(బహిర్గతం_Pinp, ResizeLValue)) {
            బహిర్గతం_Pinp = soutPinp->lhsp();
            ఒకవేళ (VN_IS(బహిర్గతం_Pinp, RToIRoundS) || VN_IS(బహిర్గతం_Pinp, RToIS)) {
                బహిర్గతం_Pinp = VN_AS(బహిర్గతం_Pinp, కణుపు_Uniop)->lhsp();
                ఒకవేళ (postRhsp->isSigned()) {
                    postRhsp = కొత్త AstISToRD{pinp->fileline(), postRhsp};
                } లేకపోతే {
                    postRhsp = కొత్త AstIToRD{pinp->fileline(), postRhsp};
                }
            } లేకపోతే {
                ఒకవేళ (Ast_కణుపు_Uniop* aoutPinp = VN_CAST(బహిర్గతం_Pinp, Extend)) {
                    బహిర్గతం_Pinp = aoutPinp->lhsp();
                } లేకపోతే ఒకవేళ (Ast_కణుపు_Uniop* aoutPinp = VN_CAST(బహిర్గతం_Pinp, ExtendS)) {
                    బహిర్గతం_Pinp = aoutPinp->lhsp();
                } లేకపోతే ఒకవేళ (AstSel* aoutPinp = VN_CAST(బహిర్గతం_Pinp, Sel)) {
                    బహిర్గతం_Pinp = aoutPinp->fromp();
                } లేకపోతే {
                    బహిర్గతం_Pinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                ఒకవేళ (బహిర్గతం_Pinp->వెడల్పు() < portp->వెడల్పు()) {
                    postRhsp = కొత్త AstSel{pinp->fileline(), postRhsp, 0, pinp->వెడల్పు()};
                } లేకపోతే {  // pin width > port width
                    ఒకవేళ (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = కొత్త AstExtendS{pinp->fileline(), postRhsp};
                    } లేకపోతే {
                        postRhsp = కొత్త AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(బహిర్గతం_Pinp);
        }
        ఒకవేళ (VN_IS(బహిర్గతం_Pinp, IToRD) || VN_IS(బహిర్గతం_Pinp, ISToRD)) {
            బహిర్గతం_Pinp = VN_AS(బహిర్గతం_Pinp, కణుపు_Uniop)->lhsp();
            postRhsp = కొత్త AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(బహిర్గతం_Pinp);
        }
        // Put output assignment AFTER function call
        Ast_కణుపు_సమాసం* స్థిరరాశి బహిర్గతం_Pin_Clonep
            = శుద్ధ_Check ? బహిర్గతం_Pinp->clone_వృక్షం_శుద్ధ(నిజం) : బహిర్గతం_Pinp->clone_వృక్షం(నిజం);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp
            = VN_CAST(బహిర్గతం_Pinp->dtypep()->skipRefp(), UnpackArrayDType);
        ఒకవేళ (arrayp && arrayp->is_తిరగేయి_యొక్క(postRhsp->dtypep())) {
            postRhsp = కొత్త Ast_Cvt_Unpack_గమ్యం_Unpack{pinp->fileline(), postRhsp, బహిర్గతం_Pinp->dtypep()};
        }
        Ast_కేటాయించు* స్థిరరాశి assp = కొత్త Ast_కేటాయించు{pinp->fileline(), బహిర్గతం_Pin_Clonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3_Error_సంకేతం::BLKSEQ, నిజం);  // Ok if in <= block
        తిరిగి assp;
    }

    శూన్య changeAtWriteRecurse(Ast_కణుపు_సమాసం* స్థిరరాశి exprp) {
        // Change nested at methods to writable variant
        ఒకవేళ (AstCMethodHard* స్థిరరాశి cMethodp = VN_CAST(exprp, CMethodHard)) {
            ఒకవేళ (cMethodp->పద్ధతి() == VCMethod::ARRAY_AT) {
                cMethodp->పద్ధతి(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    శూన్య connect_పోర్టు(స్థిరరాశి Ast_కణుపు_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_కణుపు* beginp,
                     తార్కిక inlineTask) {
        Ast_కణుపు_సమాసం* pinp = argp->exprp();
        ఒకవేళ (inlineTask) {
            portp->unlink_Fr_వెనుక();
            నెట్టు_Deletep(portp);  // Remove it from the clone (not original)
        }
        ఒకవేళ (!pinp) {
            // Too few arguments in function call
        } లేకపోతే {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            ఒకవేళ (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* స్థిరరాశి varp = కొత్త AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::గమ్యం_string(m_uncon_Var_అంకె++),
                                                portp->dtypep()};
                m_modp->కూడు_Stmtsp(varp);
                AstVarScope* స్థిరరాశి newvscp = కొత్త AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->కూడు_Varsp(newvscp);
                AstVarRef* స్థిరరాశి repp = కొత్త AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(నెట్టు_Deletep(pinp), pinp);
                pinp = repp;
            }
            ఒకవేళ (inlineTask) {
                నెట్టు_Deletep(pinp->unlink_Fr_వెనుక());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlink_Fr_వెనుక()->delete_వృక్షం(), argp);  // Args no longer needed
            }
            ఒకవేళ (portp->isRef() || portp->isConstRef()) {
                తార్కిక refArgOk = అబద్ధం;
                ఒకవేళ (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = నిజం;
                } లేకపోతే ఒకవేళ (AstCMethodHard* స్థిరరాశి cMethodp = VN_CAST(pinp, CMethodHard)) {
                    ఒకవేళ (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), వరుస_DType)) {
                        refArgOk = cMethodp->పద్ధతి() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->పద్ధతి() == VCMethod::DYN_AT_WRITE_APPEND_వెనుక;
                    } లేకపోతే {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->పద్ధతి() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                ఒకవేళ (refArgOk) {
                    ఒకవేళ (AstVarRef* స్థిరరాశి varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } లేకపోతే {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                ఒకవేళ (inlineTask) {
                    ఒకవేళ (AstVarRef* స్థిరరాశి varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* స్థిరరాశి localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } లేకపోతే {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } లేకపోతే ఒకవేళ (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* స్థిరరాశి newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ఒకవేళ (!inlineTask) {
                    pinp->replaceWith(
                        కొత్త AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    నెట్టు_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                Ast_కేటాయించు* స్థిరరాశి preassp = connect_పోర్టు_Make_లోపల_కేటాయించు(pinp, newvscp, నిజం);
                beginp->కూడు_తదుపరి_ఇక్కడ(preassp);

                Ast_కేటాయించు* స్థిరరాశి postassp = connect_పోర్టు_Make_బహిర్గతం_కేటాయించు(portp, pinp, newvscp, నిజం);
                beginp->కూడు_తదుపరి(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } లేకపోతే ఒకవేళ (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* స్థిరరాశి newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ఒకవేళ (!inlineTask) {
                    pinp->replaceWith(కొత్త AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    నెట్టు_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                Ast_కేటాయించు* స్థిరరాశి postassp = connect_పోర్టు_Make_బహిర్గతం_కేటాయించు(portp, pinp, newvscp, అబద్ధం);
                // Put assignment BEHIND of all other statements
                beginp->కూడు_తదుపరి(postassp);
            } లేకపోతే ఒకవేళ (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* స్థిరరాశి newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                Ast_కేటాయించు* స్థిరరాశి preassp = connect_పోర్టు_Make_లోపల_కేటాయించు(pinp, newvscp, అబద్ధం);
                // Put assignment in FRONT of all other statements
                beginp->కూడు_తదుపరి_ఇక్కడ(preassp);
            }
        }
    }

    తార్కిక has_Ref_వాదం(Ast_కణుపు_FTask* nodep) {
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (varp->isRef() || varp->isConstRef()) తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }

    Ast_కణుపు* createInlinedFTask(Ast_కణుపు_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_కణుపు* స్థిరరాశి newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_వృక్షం(నిజం) : శూన్యసూచిక;
        Ast_కణుపు* స్థిరరాశి beginp
            = కొత్త AstComment{refp->fileline(), "Function: "s + refp->name(), నిజం};
        ఒకవేళ (newbodysp) beginp->కూడు_తదుపరి(newbodysp);
        ఒకవేళ (debug() >= 9) beginp->dump_వృక్షం_And_తదుపరి(బయటకు, "-  newbegi: ");
        //
        // Create input variables
        Ast_కణుపు::user2_Clear_వృక్షం();
        {
            స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            కొరకు (స్థిరరాశి స్వయంచాలక& itr : tconnects) {
                AstVar* స్థిరరాశి portp = itr.మొదటి;
                AstArg* స్థిరరాశి argp = itr.రెండవ;
                connect_పోర్టు(refp, portp, argp, beginp, నిజం);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_కణుపు* nextstmtp;
            కొరకు (Ast_కణుపు* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    ఒకవేళ (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* స్థిరరాశి localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        ఒకవేళ (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_కణుపు* స్థిరరాశి crstp = కొత్త Ast_కేటాయించు{
                                portp->fileline(),
                                కొత్త AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                కొత్త AstCReset{portp->fileline(), portp, అబద్ధం}};
                            portp->replaceWith(crstp);
                        } లేకపోతే {
                            portp->unlink_Fr_వెనుక();
                        }
                        నెట్టు_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        ఒకవేళ (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        ఒకవేళ (debug() >= 9) beginp->dump_వృక్షం_And_తదుపరి(బయటకు, "-  iotask: ");
        తిరిగి beginp;
    }

    Ast_కణుపు* createNonInlinedFTask(Ast_కణుపు_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* స్థిరరాశి cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_కణుపు* స్థిరరాశి beginp
            = కొత్త AstComment{refp->fileline(), "Function: "s + refp->name(), నిజం};
        Ast_కణుపు_CCall* ccallp;
        ఒకవేళ (VN_IS(refp, New)) {
            AstCNew* స్థిరరాశి cnewp = కొత్త AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_పద్ధతి_Call* స్థిరరాశి mrefp = VN_CAST(refp, పద్ధతి_Call)) {
            ccallp = కొత్త AstCMethodCall{refp->fileline(), mrefp->fromp()->unlink_Fr_వెనుక(), cfuncp};
            ccallp->dtype_సమితి_Void();
            beginp->కూడు_తదుపరి(ccallp->makeStmt());
        } లేకపోతే {
            ccallp = కొత్త AstCCall{refp->fileline(), cfuncp};
            ccallp->dtype_సమితి_Void();
            beginp->కూడు_తదుపరి(ccallp->makeStmt());
        }

        ఒకవేళ (స్థిరరాశి AstFuncRef* స్థిరరాశి funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstTaskRef* స్థిరరాశి taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp = VN_CAST(refp, పద్ధతి_Call)) {
            ccallp->superReference(పద్ధతి_Callp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            కొరకు (స్థిరరాశి స్వయంచాలక& itr : tconnects) {
                AstVar* స్థిరరాశి portp = itr.మొదటి;
                AstArg* స్థిరరాశి argp = itr.రెండవ;
                connect_పోర్టు(refp, portp, argp, beginp, అబద్ధం);
            }
        }
        // First argument is symbol table, then output if a function
        స్థిరరాశి తార్కిక needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        ఒకవేళ (needSyms) ccallp->argTypes("vlSymsp");

        ఒకవేళ (refp->taskp()->dpiContext()) {
            AstScopeName* స్థిరరాశి snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlink_Fr_వెనుక();
            File_పంక్తి* స్థిరరాశి flp = refp->fileline();
            // __Vscopep
            ccallp->కూడు_Argsp(snp);
            // __Vfilenamep
            ccallp->కూడు_Argsp(
                కొత్త AstCExpr{flp, AstCExpr::శుద్ధ{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->కూడు_Argsp(కొత్త AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        కొరకు (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->కూడు_Argsp(argp->exprp()->unlink_Fr_వెనుక());
        }

        ఒకవేళ (outvscp) ccallp->కూడు_Argsp(కొత్త AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        ఒకవేళ (debug() >= 9) beginp->dump_వృక్షం_And_తదుపరి(బయటకు, "-  nitask: ");
        తిరిగి beginp;
    }

    string dpiSignature(Ast_కణుపు_FTask* nodep, AstVar* rtnvarp) స్థిరరాశి {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        ఒకవేళ (nodep->dpi_శుద్ధ()) dpiproto += "pure ";
        ఒకవేళ (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(నిజం, నిజం) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string వాదనలు;
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    ఒకవేళ (వాదనలు != "") {
                        వాదనలు += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    స్థిరరాశి std::string dpiType = portp->dpiArgType(అబద్ధం, అబద్ధం);
                    dpiproto += dpiType;
                    స్థిరరాశి std::string vType = portp->dtypep()->కలిగియుంది_Unpacked_Struct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(అబద్ధం);
                    ఒకవేళ (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    వాదనలు += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        తిరిగి dpiproto;
    }

    స్థిర శూన్య checkLegalCIdentifier(Ast_కణుపు* nodep, స్థిరరాశి string& name) {
        ఒకవేళ (name.చివర() != std::కనుగొను_if(name.begin(), name.చివర(), [](అక్షరం c) {
                తిరిగి !std::అక్షరమా_అంకెనా(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    స్థిర Ast_కణుపు* create_Dpi_తాత్కాలిక(AstVar* portp, స్థిరరాశి string& suffix) {
        స్థిరరాశి string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        తిరిగి కొత్త AstCStmt{portp->fileline(), stmt};
    }

    శూన్య unlinkAndClone(Ast_కణుపు_FTask* funcp, Ast_కణుపు* nodep, తార్కిక with_తదుపరి) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        ఒకవేళ (funcp->recursive()) {
            VNRelinker relinkHandle;
            ఒకవేళ (with_తదుపరి) {
                nodep->unlink_Fr_వెనుక_With_తదుపరి(&relinkHandle);
            } లేకపోతే {
                nodep->unlink_Fr_వెనుక(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_కణుపు* స్థిరరాశి newp = nodep->clone_వృక్షం(with_తదుపరి);
            relinkHandle.relink(newp);
        } లేకపోతే {
            ఒకవేళ (with_తదుపరి) {
                nodep->unlink_Fr_వెనుక_With_తదుపరి();
            } లేకపోతే {
                nodep->unlink_Fr_వెనుక();
            }
        }
    }

    స్థిర Ast_కణుపు* create_కేటాయించు_అంతర్గత_గమ్యం_Dpi(AstVar* portp, తార్కిక isPtr, స్థిరరాశి string& frSuffix,
                                              స్థిరరాశి string& గమ్యం_Suffix) {
        స్థిరరాశి string stmt = V3Task::కేటాయించు_అంతర్గత_గమ్యం_Dpi(portp, isPtr, frSuffix, గమ్యం_Suffix);
        తిరిగి కొత్త AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    స్థిర Ast_కణుపు_Stmt* createDpiScalarCvt(Ast_కణుపు_సమాసం* internalp, స్థిరరాశి std::string& dpi,
                                           స్థిరరాశి Ast_కణుపు_DType* dtypep, తార్కిక గమ్యం_Dpi) {
        File_పంక్తి* స్థిరరాశి flp = internalp->fileline();
        స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = dtypep->basicp();
        ఒకవేళ (గమ్యం_Dpi) {
            AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{flp};
            ఒకవేళ (basicp->isDpiPrimitive()) {
                cstmtp->కూడు(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->కూడు(internalp);
                cstmtp->కూడు(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } లేకపోతే {
                cstmtp->కూడు((basicp->is_Dpi_బిట్టు_Vec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::గమ్యం_string(dtypep->వెడల్పు()) + ", " + dpi + ", ");
                cstmtp->కూడు(internalp);
                cstmtp->కూడు(");");
            }
            తిరిగి cstmtp;
        }
        std::string cvt;
        తార్కిక useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_గమ్యం_అంతర్గత_Cvt_Stmt(dtypep);
        ఒకవేళ (useSvVec) {
            AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{flp};
            cstmtp->కూడు(cvt);
            cstmtp->కూడు(internalp);
            cstmtp->కూడు(", " + dpi + ");");
            తిరిగి cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        స్థిరరాశి పూర్ణాంకం cwidth = basicp->వెడల్పు_పదాలు() * VL_EDATASIZE;
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstCExpr{flp, cvt + dpi + ")", cwidth};
        తిరిగి కొత్త Ast_కేటాయించు{flp, internalp, కొత్త AstSel{flp, rhsp, 0, dtypep->వెడల్పు()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    స్థిర Ast_కణుపు_Stmt* createDpiCvt(Ast_కణుపు_సమాసం* internalp, స్థిరరాశి std::string& dpi,
                                     స్థిరరాశి Ast_కణుపు_DType* dtypep, తార్కిక గమ్యం_Dpi) {
        File_పంక్తి* స్థిరరాశి flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_కణుపు_Stmt* stmtsp = శూన్యసూచిక;
        ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            కొరకు (పూర్ణాంకం i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_కణుపు_సమాసం* స్థిరరాశి elemp = కొత్త AstArraySel{flp, internalp->clone_వృక్షం(అబద్ధం), i};
                stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::గమ్యం_string(i) + "]",
                                                       arrayp->subDTypep(), గమ్యం_Dpi));
            }
        } లేకపోతే ఒకవేళ (dtypep->కలిగియుంది_Unpacked_Struct()) {
            కొరకు (స్థిరరాశి AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* స్థిరరాశి selp
                    = కొత్త AstStructSel{flp, internalp->clone_వృక్షం(అబద్ధం), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_కణుపు::కూడు_తదుపరి(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), గమ్యం_Dpi));
            }
        } లేకపోతే {
            తిరిగి createDpiScalarCvt(internalp, dpi, dtypep, గమ్యం_Dpi);
        }
        VL_DO_DANGLING(internalp->delete_వృక్షం(), internalp);
        తిరిగి stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    స్థిర Ast_కణుపు_Stmt* createDpiArgCvt(AstVarScope* vscp, స్థిరరాశి std::string& dpiName,
                                        తార్కిక గమ్యం_Dpi) {
        AstVar* స్థిరరాశి varp = vscp->varp();
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();
        స్థిరరాశి std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        స్థిరరాశి పూర్ణాంకం మొత్తం = strides.ఖాళీలు() ? 1 : strides[0].మొదటి * strides[0].రెండవ;
        స్థిరరాశి తార్కిక isStruct = varp->dtypep()->కలిగియుంది_Unpacked_Struct();
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_కణుపు_Stmt* stmtsp = శూన్యసూచిక;
        కొరకు (పూర్ణాంకం i = 0; i < మొత్తం; ++i) {
            Ast_కణుపు_సమాసం* internalp
                = కొత్త AstVarRef{flp, vscp, గమ్యం_Dpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            కొరకు (స్థిరరాశి స్వయంచాలక& stride : strides) {
                internalp = కొత్త AstArraySel(flp, internalp, (i / stride.రెండవ) % stride.మొదటి);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            ఒకవేళ (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::గమ్యం_string(i * varp->basicp()->వెడల్పు_పదాలు());
            } లేకపోతే ఒకవేళ (isStruct || !strides.ఖాళీలు()) {
                dpi += "[" + std::గమ్యం_string(i) + "]";
            }
            stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, గమ్యం_Dpi));
        }
        తిరిగి stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_కణుపు_FTask* స్థిరరాశి nodep, AstVar* స్థిరరాశి rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        స్థిరరాశి std::string cname = Ast_కణుపు::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        స్థిరరాశి అక్షరం* స్థిరరాశి tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        స్థిరరాశి std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(నిజం, నిజం) : "";
        // The function we are building
        AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(నిజం);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(నిజం);
        funcp->నమోదు_బిందువు(నిజం);
        funcp->isStatic(నిజం);
        funcp->protect(అబద్ధం);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_శిఖరం_Scopep->scopep()->కూడు_Blocksp(funcp);
        // Check the arguments before converting them
        ఒకవేళ (!make_పోర్టు_List(nodep, funcp)) తిరిగి శూన్యసూచిక;

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
        AstCStmt* స్థిరరాశి prep = కొత్త AstCStmt{flp};
        funcp->కూడు_Stmtsp(prep);
        // Static doesn't need save-restore as if below will re-fill proper value
        prep->కూడు("static int __Vfuncnum = -1;\n");
        // First time init (faster than what the compiler does if we did a singleton
        prep->కూడు("if (VL_UNLIKELY(__Vfuncnum == -1)) {\n");
        prep->కూడు("__Vfuncnum = Verilated::exportFuncNum(\"" + nodep->cname() + "\");\n");
        prep->కూడు("}\n");
        // If the find fails, it will throw an error
        prep->కూడు("const VerilatedScope* const __Vscopep = Verilated::dpiScope();\n");
        // If 'dpiScope()' fails and '__Vscopep' is null; the exportFind function throws an error
        // If __Vcb is null the exportFind function throws and error
        స్థిరరాశి std::string cbtype
            = VIdProtect::protect(v3Global.opt.ఉపసర్గ() + "__Vcb_" + nodep->cname() + "_t");
        prep->కూడు(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* స్థిరరాశి callp = కొత్త AstCStmt{flp};
        స్థిరరాశి స్వయంచాలక కూడు_Func_Arg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* స్థిరరాశి vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(అబద్ధం);
            portp->protect(అబద్ధం);
            // Add argument to call
            స్థిరరాశి VAccess access = portp->direction().pinAccess();
            callp->కూడు(", ");
            callp->కూడు(కొత్త AstVarRef{portp->fileline(), vscp, access});
            తిరిగి vscp;
        };
        // Call callback
        callp->కూడు("(*__Vcb)(");
        // First argument is the Syms
        callp->కూడు("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_కణుపు* బహిర్గతం_Cvtsp = శూన్యసూచిక;  // Conversions of output/inout arguments, after the call
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var);
            ఒకవేళ (!portp) కొనసాగు;
            ఒకవేళ (!portp->isIO()) కొనసాగు;
            ఒకవేళ (portp->isFuncReturn()) కొనసాగు;
            ఒకవేళ (portp == rtnvarp) కొనసాగు;  // Handled below
            // Add argument to call
            AstVarScope* స్థిరరాశి outvscp = కూడు_Func_Arg(portp);
            స్థిరరాశి తార్కిక isStruct = portp->dtypep()->కలిగియుంది_Unpacked_Struct();
            ఒకవేళ (portp->isWritable()) {
                బహిర్గతం_Cvtsp = Ast_కణుపు::కూడు_తదుపరి(
                    బహిర్గతం_Cvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), నిజం)
                                       : create_కేటాయించు_అంతర్గత_గమ్యం_Dpi(portp, నిజం, tmpSuffixp, ""));
            }
            ఒకవేళ (!portp->isNonOutput()) కొనసాగు;
            // Convert input/inout arguments to dpi type
            స్థిరరాశి std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(అబద్ధం).రెండవ == 0
                      ? "*"
                      : "";
            funcp->కూడు_Stmtsp(createDpiArgCvt(outvscp, deref + portp->name(), అబద్ధం));
        }
        // Return value argument goes last
        ఒకవేళ (rtnvarp) కూడు_Func_Arg(rtnvarp);
        // Close statement
        callp->కూడు(");");
        // Call the user function
        funcp->కూడు_Stmtsp(callp);
        // Convert output/inout arguments back to internal type
        ఒకవేళ (బహిర్గతం_Cvtsp) funcp->కూడు_Stmtsp(బహిర్గతం_Cvtsp);
        // Convert return value
        ఒకవేళ (rtnvarp) {
            funcp->కూడు_Stmtsp(create_Dpi_తాత్కాలిక(rtnvarp, ""));
            funcp->కూడు_Stmtsp(create_కేటాయించు_అంతర్గత_గమ్యం_Dpi(rtnvarp, అబద్ధం, tmpSuffixp, ""));
            స్థిరరాశి std::string సూచీ = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->కూడు_Stmtsp(కొత్త AstCStmt{flp, "return " + rtnvarp->name() + సూచీ + ";"});
        }
        తిరిగి funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_కణుపు_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        స్థిరరాశి string name = Ast_కణుపు::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        స్థిరరాశి string rtnType = rtnvarp            ? rtnvarp->dpiArgType(నిజం, నిజం)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(నిజం);
        funcp->dontCombine(నిజం);
        funcp->నమోదు_బిందువు(అబద్ధం);
        funcp->is_పద్ధతి(అబద్ధం);
        funcp->protect(అబద్ధం);
        funcp->dpi_శుద్ధ(nodep->dpi_శుద్ధ());

        స్థిరరాశి పూర్ణాంకం ఖర్చు = static_cast<పూర్ణాంకం>(V3Control::get_Profile_దత్తాంశం(funcp->name()));
        m_statHierDpisWithCosts += (ఖర్చు != 0);
        funcp->ఖర్చు(ఖర్చు);

        // Add DPI Import to top, since it's a global function
        m_శిఖరం_Scopep->scopep()->కూడు_Blocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        ఒకవేళ (!make_పోర్టు_List(nodep, funcp)) తిరిగి శూన్యసూచిక;
        తిరిగి funcp;
    }

    AstCFunc* getDpiFunc(Ast_కణుపు_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        స్థిరరాశి string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        స్థిరరాశి స్వయంచాలక pair = m_dpi_పేర్లు.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, శూన్యసూచిక));
        ఒకవేళ (pair.రెండవ) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* స్థిరరాశి funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.మొదటి->రెండవ) = funcp;
            తిరిగి funcp;
        } లేకపోతే {
            // Seen this cname import before. Check if it's the same prototype.
            స్థిరరాశి Ast_కణుపు_FTask* మొదటి_Nodep;
            string మొదటి_Signature;
            AstCFunc* మొదటి_Funcp;
            std::tie(మొదటి_Nodep, మొదటి_Signature, మొదటి_Funcp) = pair.మొదటి->రెండవ;
            ఒకవేళ (signature != మొదటి_Signature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warn_మరిన్ని()  //
                               << "... New signature:      " << signature << '\n'  //
                               << మొదటి_Nodep->warnOther()
                               << "... Original signature: " << మొదటి_Signature << '\n'  //
                               << మొదటి_Nodep->warnContextSecondary());
                తిరిగి శూన్యసూచిక;
            } లేకపోతే {
                // Same signature, return the previously created CFunc
                తిరిగి మొదటి_Funcp;
            }
        }
    }

    స్థిర తార్కిక make_పోర్టు_List(Ast_కణుపు_FTask* nodep, AstCFunc* dpip) {
        తార్కిక allOk = నిజం;
        // Copy nodep's list of function I/O to the new dpip c function
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (portp->isIO()) {
                    // Move it to new function
                    AstVar* స్థిరరాశి newPortp = portp->clone_వృక్షం(అబద్ధం);
                    newPortp->funcLocal(నిజం);
                    dpip->కూడు_Argsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    ఒకవేళ (!portp->basicp() && !portp->dtypep()->కలిగియుంది_Unpacked_Struct()) {
                        allOk = అబద్ధం;
                        portp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: DPI argument of type "
                                << portp->dtypep()->prettyTypeName() << '\n'
                                << portp->warn_మరిన్ని()
                                << "... For best portability, use bit, byte, int, or longint");
                        // We don't warn on logic either, although the 4-stateness is lost.
                        // That's what other simulators do.
                    }
                }
            }
        }
        తిరిగి allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    స్థిర తార్కిక isDpiCIdentifier(స్థిరరాశి string& name) {
        తిరిగి !name.ఖాళీలు() && VString::isIdentifier(name)
               && !std::అంకెనా(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_పదాలు::isKeyword(name).ఖాళీలు();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    తార్కిక checkDpiStruct(స్థిరరాశి AstVar* portp, స్థిరరాశి AstStructDType* sdtypep, తార్కిక nested) {
        స్థిరరాశి std::string name = sdtypep->dpiName();
        ఒకవేళ (!(nested && name.ఖాళీలు()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(నిజం) << "\n"
                              << portp->warn_మరిన్ని()
                              << "... Suggest use a typedef with a C identifier name");
            తిరిగి అబద్ధం;
        }
        కొరకు (స్థిరరాశి AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ఒకవేళ (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                తిరిగి అబద్ధం;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            స్థిరరాశి Ast_కణుపు_DType* dtypep = itemp->subDTypep()->skipRefp();
            అయితే (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి subp = VN_CAST(dtypep, కణుపు_UOr_Struct_DType);
            స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            స్థిరరాశి తార్కిక isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            ఒకవేళ (dtypep->కలిగియుంది_Unpacked_Struct()) {
                ఒకవేళ (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), నిజం)) తిరిగి అబద్ధం;
            } లేకపోతే ఒకవేళ (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(నిజం)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                తిరిగి అబద్ధం;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        ఒకవేళ (name.ఖాళీలు()) తిరిగి నిజం;
        స్థిరరాశి AstStructDType* స్థిరరాశి firstp = m_dpiStructs.emplace(name, sdtypep).మొదటి->రెండవ;
        ఒకవేళ (firstp->dpiTypedef() == sdtypep->dpiTypedef()) తిరిగి నిజం;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        తిరిగి అబద్ధం;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    తార్కిక check_Dpi_Struct_వాదనలు(స్థిరరాశి Ast_కణుపు_FTask* nodep) {
        తార్కిక allOk = నిజం;
        కొరకు (స్థిరరాశి Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            స్థిరరాశి AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var);
            ఒకవేళ (portp && portp->isIO() && portp->dtypep()->కలిగియుంది_Unpacked_Struct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       అబద్ధం)
                        && allOk;
            }
        }
        తిరిగి allOk;
    }

    శూన్య bodyDpiImportFunc(Ast_కణుపు_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        స్థిరరాశి అక్షరం* స్థిరరాశి tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        ఒకవేళ (v3Global.opt.profExec()) {
            cfuncp->కూడు_Stmtsp(AstCStmt::prof_Exec_Section_నెట్టు(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string వాదనలు;
        కొరకు (Ast_కణుపు* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                AstVarScope* స్థిరరాశి portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                ఒకవేళ (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    ఒకవేళ (వాదనలు != "") వాదనలు += ", ";

                    ఒకవేళ (portp->isDpiOpenArray()) {
                        Ast_కణుపు_DType* స్థిరరాశి dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, వరుస_DType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        స్థిరరాశి string propName = portp->name() + "__Vopenprops";
                        స్థిరరాశి string prop_సంకేతం = portp->vlPropDecl(propName);
                        cfuncp->కూడు_Stmtsp(కొత్త AstCStmt{portp->fileline(), prop_సంకేతం});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        స్థిరరాశి string name = portp->name() + "__Vopenarray";
                        స్థిరరాశి string var_సంకేతం
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->కూడు_Stmtsp(కొత్త AstCStmt{portp->fileline(), var_సంకేతం});
                        వాదనలు += "&" + name;
                    } లేకపోతే {
                        // Unpacked structures are passed by reference from a C array
                        స్థిరరాశి తార్కిక isStruct = portp->dtypep()->కలిగియుంది_Unpacked_Struct();
                        ఒకవేళ (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            ఒకవేళ (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) వాదనలు += "&";
                        }

                        వాదనలు += portp->name() + tmpSuffixp;

                        cfuncp->కూడు_Stmtsp(create_Dpi_తాత్కాలిక(portp, tmpSuffixp));
                        ఒకవేళ (portp->isNonOutput()) {
                            cfuncp->కూడు_Stmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, నిజం)
                                    : create_కేటాయించు_అంతర్గత_గమ్యం_Dpi(portp, అబద్ధం, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        ఒకవేళ (nodep->dpiContext()) {
            స్థిరరాశి string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->కూడు_Stmtsp(కొత్త AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* స్థిరరాశి callp = కొత్త AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtype_సమితి_Void();
            callp->argTypes(వాదనలు);
            ఒకవేళ (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->కూడు_Stmtsp(create_Dpi_తాత్కాలిక(rtnvscp->varp(), tmpSuffixp));
                స్థిరరాశి std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{nodep->fileline()};
                cstmtp->కూడు(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->కూడు(" = ");
                cstmtp->కూడు(callp);  // RHS
                cstmtp->కూడు(";");
                cfuncp->కూడు_Stmtsp(cstmtp);
            } లేకపోతే {
                // Othervise just call it
                cfuncp->కూడు_Stmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        కొరకు (Ast_కణుపు* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                portp->protect(అబద్ధం);  // No additional exposure - already part of shown proto
                ఒకవేళ (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* స్థిరరాశి portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->కూడు_Stmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, అబద్ధం));
                }
            }
        }

        ఒకవేళ (v3Global.opt.profExec()) {
            cfuncp->కూడు_Stmtsp(AstCStmt::prof_Exec_Section_తీయి(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        ఒకవేళ (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_శిఖరం_Scopep->create_తాత్కాలిక("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        తిరిగి dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_కణుపు_FTask* nodep, తార్కిక ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_కణుపు::user2_Clear_వృక్షం();
        AstVar* rtnvarp = శూన్యసూచిక;
        ఒకవేళ (nodep->isFunction()) {
            AstVar* స్థిరరాశి portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            ఒకవేళ (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* స్థిరరాశి bdtypep = portp->dtypep()->basicp();
                ఒకవేళ (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(నిజం)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి శూన్యసూచిక;
                }
                ఒకవేళ (!bdtypep->isDpiPrimitive()) {
                    ఒకవేళ (bdtypep->is_Dpi_బిట్టు_Vec() && portp->వెడల్పు() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warn_మరిన్ని()
                                       + "... Suggest make it an output argument instead?");
                    }
                    ఒకవేళ (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } లేకపోతే ఒకవేళ (nodep->task_ప్రజా()) {
                ఒకవేళ (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warn_మరిన్ని()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            ఒకవేళ (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(అబద్ధం);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, అబద్ధం);
            rtnvarp = portp;
            rtnvarp->funcLocal(నిజం);
            rtnvarp->noCReset(నిజం);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            ఒకవేళ (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(అబద్ధం);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = శూన్యసూచిక;
        ఒకవేళ (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpi_Open_మాతృ() && nodep->dpi_Open_బిడ్డ()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = check_Dpi_Struct_వాదనలు(nodep) ? getDpiFunc(nodep, rtnvarp) : శూన్యసూచిక;
            ఒకవేళ (!dpiFuncp || (nodep->dpiImport() && nodep->dpi_Open_మాతృ())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                ఒకవేళ (rtnvarp) VL_DO_DANGLING(నెట్టు_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి శూన్యసూచిక;
            }
        }

        AstVarScope* rtnvscp = శూన్యసూచిక;
        ఒకవేళ (rtnvarp) {
            rtnvscp = కొత్త AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->కూడు_Varsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string ఉపసర్గ;
        ఒకవేళ (nodep->dpiImport()) {
            ఉపసర్గ = "__Vdpiimwrap_";
        } లేకపోతే ఒకవేళ (nodep->dpiExport()) {
            ఉపసర్గ = "__Vdpiexp_";
        } లేకపోతే ఒకవేళ (ftaskNoInline) {
            ఉపసర్గ = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        ఒకవేళ (!nodep->task_ప్రజా() && !nodep->class_పద్ధతి()) suffix = "_" + m_scopep->nameDotless();
        స్థిరరాశి string name = ((nodep->name() == "new") ? "new" : ఉపసర్గ + nodep->name() + suffix);
        AstCFunc* స్థిరరాశి cfuncp = కొత్త AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->task_ప్రజా() && rtnvarp) ? rtnvarp->cPubArgType(నిజం, నిజం) : "")};
        // It's ok to combine imports because this is just a wrapper;
        // duplicate wrappers can get merged.
        cfuncp->dontCombine(!nodep->dpiImport());
        cfuncp->నమోదు_బిందువు(!nodep->dpiImport());
        cfuncp->func_ప్రజా(nodep->task_ప్రజా());
        cfuncp->dpiContext(nodep->dpiContext());
        cfuncp->dpiExportImpl(nodep->dpiExport());
        cfuncp->dpiImportWrapper(nodep->dpiImport());
        cfuncp->recursive(nodep->recursive());
        // Hardcoded based on UVM usage; TODO make a verilated_std.vlt control for these
        cfuncp->unlikely(nodep->name() == "uvm_report_error" || nodep->name() == "uvm_report_info"
                         || nodep->name() == "uvm_report_fatal"
                         || nodep->name() == "uvm_report_warning");
        ఒకవేళ (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(నిజం);
            cfuncp->isLoose(నిజం);
        } లేకపోతే {
            cfuncp->isStatic(అబద్ధం);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpi_శుద్ధ(nodep->dpi_శుద్ధ());
        ఒకవేళ (nodep->name() == "new") cfuncp->isConstructor(నిజం);
        ఒకవేళ (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(నిజం);
        ఒకవేళ (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        ఒకవేళ (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        స్థిరరాశి తార్కిక needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->task_ప్రజా()) || v3Global.opt.profExec());
        ఒకవేళ (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        ఒకవేళ (!nodep->dpiImport() && !nodep->task_ప్రజా()) {
            // Need symbol table
            ఒకవేళ (cfuncp->isConstructor()) {
                తార్కిక isInterfaceClass = అబద్ధం;
                ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                ఒకవేళ (!isInterfaceClass) {
                    స్థిరరాశి string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->కూడు_Stmtsp(
                        కొత్త AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        ఒకవేళ (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_ఆదానం_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_ఆదానం_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_ఆదానం_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        ఒకవేళ (nodep->dpiExport()) {
            AstScopeName* స్థిరరాశి snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(నిజం);
            snp->unlink_Fr_వెనుక();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        కొరకు (Ast_కణుపు *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlink_Fr_వెనుక();
                    నెట్టు_Deletep(portp);
                    portp->user2p(const_Pool_పట్టిక(portp));
                } లేకపోతే {
                    ఒకవేళ (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, అబద్ధం);
                        portp->funcLocal(నిజం);
                        ఒకవేళ (portp->valuep()) నెట్టు_Deletep(portp->valuep()->unlink_Fr_వెనుక());
                        cfuncp->కూడు_Argsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        ఒకవేళ (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_కణుపు_DType* స్థిరరాశి dtypep = portp->dtypep()->skipRefp();
                            ఒకవేళ (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } లేకపోతే {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(నిజం);
                    }
                    AstVarScope* స్థిరరాశి newvscp
                        = కొత్త AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->కూడు_Varsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        ఒకవేళ (rtnvarp) cfuncp->కూడు_Argsp(rtnvarp);

        // Move body
        Ast_కణుపు* bodysp = nodep->stmtsp();
        ఒకవేళ (bodysp) {
            unlinkAndClone(nodep, bodysp, నిజం);
            AstBegin* స్థిరరాశి tempp
                = కొత్త AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, అబద్ధం};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            కొరకు (Ast_కణుపు *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                    ఒకవేళ (portp->isIO()) VL_DO_DANGLING(నెట్టు_Deletep(portp->unlink_Fr_వెనుక()), portp);
                }
            }
            ఒకవేళ (tempp->stmtsp()) cfuncp->కూడు_Stmtsp(tempp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
            VL_DO_DANGLING(tempp->delete_వృక్షం(), tempp);
        }
        ఒకవేళ (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        ఒకవేళ (rtnvscp && nodep->task_ప్రజా()) {
            cfuncp->కూడు_Stmtsp(కొత్త AstCReturn{
                rtnvscp->fileline(), కొత్త AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        ఒకవేళ (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            తార్కిక writesNonLocals = అబద్ధం;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* స్థిరరాశి varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                ఒకవేళ (varp->isFuncLocal()) తిరిగి;
                // Check if written
                ఒకవేళ (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->సమితి_Written_By_Dpi();
                    // Remember we had some
                    writesNonLocals = నిజం;
                }
                // Check if read
                ఒకవేళ (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->సమితి_Read_By_Dpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            ఒకవేళ (writesNonLocals) {
                AstVarScope* స్థిరరాశి dpiExportTriggerp = getDpiExporTrigger();
                File_పంక్తి* స్థిరరాశి flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                Ast_కేటాయించు* స్థిరరాశి assignp
                    = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    కొత్త AstConst{flp, AstConst::బిట్టు_True{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                ఒకవేళ (v3Global.opt.systemC()) {
                    assignp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        ఒకవేళ (nodep->needProcess()) cfuncp->సమితి_Need_Process();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        తిరిగి cfuncp;
    }

    శూన్య iterateIntoFTask(Ast_కణుపు_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    శూన్య చొప్పించు_Before_Stmt(Ast_కణుపు* nodep, Ast_కణుపు* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
    }
    శూన్య process_వాదనలు(Ast_కణుపు_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        కొరకు (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* స్థిరరాశి arrayp = VN_CAST(argp->exprp(), ConsArray);
            ఒకవేళ (!arrayp) కొనసాగు;

            File_పంక్తి* స్థిరరాశి flp = arrayp->fileline();
            AstVarScope* స్థిరరాశి substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->is_అంతర్గత(నిజం);

            Ast_కేటాయించు* స్థిరరాశి assignp
                = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlink_Fr_వెనుక()};

            Ast_సమాసం_Stmt* స్థిరరాశి exprstmtp = కొత్త Ast_సమాసం_Stmt{
                flp, assignp, కొత్త AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    తార్కిక isIfaceLocalImpure(Ast_కణుపు_FTask* nodep, Ast_కణుపు* impurep) {
        స్థిరరాశి AstScope* స్థిరరాశి scopep = m_statep->getScope(nodep);
        ఒకవేళ (!TaskStateVisitor::isIfaceFTaskScope(scopep)) తిరిగి అబద్ధం;

        స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(impurep, VarRef);
        ఒకవేళ (!refp || !refp->varScopep()) తిరిగి అబద్ధం;

        తిరిగి refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    శూన్య సందర్శించు(AstWith* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) {
            // Make sure that the return expression is converted only once
            తిరిగి;
        }
        Ast_కణుపు_సమాసం* స్థిరరాశి withExprp = VN_AS(nodep->exprp()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
        nodep->కూడు_Exprp(కొత్త AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    శూన్య సందర్శించు(AstCNew* nodep) override {
        VL_RESTORER(m_లోపల_New);
        m_లోపల_New = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (m_లోపల_Sensesp && !nodep->is_శుద్ధ()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(శూన్యసూచిక);  // So V3Broken doesn't complain
            తిరిగి;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        ఒకవేళ (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_లెక్క(), nodep, "No non-inline task, but no error issued");
            ఒకవేళ (VN_IS(nodep->backp(), Stmt_సమాసం)) {
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            } లేకపోతే {
                nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}});
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
            తిరిగి;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = శూన్యసూచిక;
        ఒకవేళ (nodep->taskp()->isFunction()) {
            AstVar* స్థిరరాశి fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            ఒకవేళ (Ast_కేటాయించు* స్థిరరాశి assignp = VN_CAST(nodep->backp(), కేటాయించు)) {
                ఒకవేళ (AstVarRef* స్థిరరాశి lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* స్థిరరాశి vscp = lhsp->varScopep();
                    ఒకవేళ (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_వృక్షం(
                            fvarp->dtypep()->skipRefp())
                        && !has_Ref_వాదం(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            ఒకవేళ (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_కణుపు* beginp;
        AstCNew* cnewp = శూన్యసూచిక;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        స్థిరరాశి తార్కిక virtualIfaceCall
            = TaskStateVisitor::is_Virtual_Iface_పద్ధతి_Call(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        ఒకవేళ (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            process_వాదనలు(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } లేకపోతే {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        ఒకవేళ (VN_IS(nodep, New)) {  // New not legal as while() condition
            చొప్పించు_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (VN_IS(nodep->backp(), కణుపు_కేటాయించు)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            చొప్పించు_Before_Stmt(nodep, beginp);
            AstVarRef* స్థిరరాశి outrefp = కొత్త AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (!VN_IS(nodep->backp(), Stmt_సమాసం)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* స్థిరరాశి outrefp = కొత్త AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_సమాసం_Stmt* lambdap = కొత్త Ast_సమాసం_Stmt{nodep->fileline(), beginp, outrefp};

            ఒకవేళ (m_లోపల_New) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_వృక్షం(అబద్ధం);
                newvarp->funcLocal(నిజం);

                // Create a new VarScope for the new variable
                AstVarScope* స్థిరరాశి newvscp
                    = కొత్త AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->కూడు_Varsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreach_And_తదుపరి([outvscp, newvscp](AstVarRef* refp) {
                    ఒకవేళ (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->has_ఫలితం(అబద్ధం);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* స్థిరరాశి exprp = కొత్త AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtype_సమితి_String();
                lambdap->కూడు_Stmtsp(కొత్త AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            VIsCached::clear_Cache_వృక్షం();
        } లేకపోతే {  // VN_IS(nodep->backp(), StmtExpr)
            చొప్పించు_Before_Stmt(nodep, beginp);
            ఒకవేళ (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_సంకేతం::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* స్థిరరాశి nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        ఒకవేళ (!nodep->user1_సమితి_Once()) {  // Just one creation needed per function
            // Expand functions in it
            పూర్ణాంకం modes = 0;
            ఒకవేళ (nodep->dpiImport()) modes++;
            ఒకవేళ (nodep->dpiExport()) modes++;
            ఒకవేళ (nodep->task_ప్రజా()) modes++;
            ఒకవేళ (nodep->class_పద్ధతి()) modes++;
            ఒకవేళ (v3Global.opt.protectIds() && nodep->task_ప్రజా()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            ఒకవేళ (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            స్థిరరాశి తార్కిక noInline = m_statep->ftaskNoInline(nodep);
            స్థిరరాశి తార్కిక needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            ఒకవేళ (noInline && !nodep->class_పద్ధతి() && !nodep->recursive()) {
                ఒకవేళ (Ast_కణుపు* స్థిరరాశి impurep = m_statep->checkImpure(nodep)) {
                    ఒకవేళ (!isIfaceLocalImpure(nodep, impurep)) {
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

            ఒకవేళ (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->task_ప్రజా()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_కణుపు_FTask* స్థిరరాశి clonedFuncp = nodep->clone_వృక్షం(అబద్ధం);
                ఒకవేళ (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                స్థిరరాశి తార్కిక makeNonInlineFunc = noInline || needsNonInlineCFunc;
                ఒకవేళ (AstCFunc* స్థిరరాశి cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->కూడు_తదుపరి_ఇక్కడ(cfuncp);
                    ఒకవేళ (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            ఒకవేళ (nodep->isFunction()) {
                ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* స్థిరరాశి vscp = m_statep->కనుగొను_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(నెట్టు_Deletep(vscp->unlink_Fr_వెనుక()), vscp);
                }
            }
            కొరకు (Ast_కణుపు *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* స్థిరరాశి vscp = m_statep->కనుగొను_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(నెట్టు_Deletep(vscp->unlink_Fr_వెనుక()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Force* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::సదిశ<Ast_కణుపు_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_కణుపు_FTask_Ref* refp) { refs.నెట్టు_వెనుక(refp); });
        కొరకు (Ast_కణుపు_FTask_Ref* స్థిరరాశి refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* స్థిరరాశి interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{
                nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం)};
            Ast_Sen_వృక్షం* స్థిరరాశి senTreep = కొత్త Ast_Sen_వృక్షం{
                nodep->fileline(), కొత్త Ast_Sen_అంశం{nodep->fileline(), Ast_Sen_అంశం::Combo{}}};
            Ast_చురుకైన* స్థిరరాశి activep
                = కొత్త Ast_చురుకైన{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_వృక్షం_Storep(activep->sentreep());
            activep->కూడు_Stmtsp(
                కొత్త AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, శూన్యసూచిక, assignp});
            m_scopep->కూడు_Blocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(కొత్త AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_వృక్షం(), refp);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        ఒకవేళ (!nodep->exprp()) VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override {
        UASSERT_OBJ(!m_లోపల_Sensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_లోపల_Sensesp);
        m_లోపల_Sensesp = నిజం;
        iterateChildren(nodep);
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    TaskVisitor(AstNetlist* nodep, TaskStateVisitor* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~TaskVisitor() {
        V3Stats::కూడు_Stat("Optimizations, Functions inlined", m_statInlines);
        V3Stats::కూడు_Stat("Optimizations, Hierarchical DPI wrappers with costs",
                         m_statHierDpisWithCosts);
    }
};

//######################################################################
// Mark interface members under timing controls of interface CFuncs as interface-sensed

తరగతి TaskIfaceSensVisitor final : బహిరంగ VNVisitorConst {
    // STATE
    తార్కిక m_underIfaceFunc = అబద్ధం;  // Under a CFunc owned by an interface scope
    తార్కిక m_underSenses = అబద్ధం;  // Under a sensitivity expression of such a CFunc

    // METHODS
    శూన్య markSensesAndIterate(Ast_కణుపు* nodep) {
        ఒకవేళ (!m_underIfaceFunc) తిరిగి;
        VL_RESTORER(m_underSenses);
        m_underSenses = నిజం;
        iterate_And_తదుపరి_Const_Null(nodep);
    }
    // VISITORS
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_Sen_వృక్షం* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    శూన్య సందర్శించు(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (!m_underSenses) తిరిగి;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        ఒకవేళ (nodep->varp()->isFuncLocal()) తిరిగి;
        ఒకవేళ (AstIface* స్థిరరాశి ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = అప్రమేయ;
};

//######################################################################
// Task class functions

స్థిరరాశి అక్షరం* స్థిరరాశి V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_కణుపు_FTask_Ref* nodep, Ast_కణుపు* taskStmtsp,
                                    V3TaskConnectState* statep, తార్కిక makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<స్థిరరాశి std::string, పూర్ణాంకం> name_గమ్యం_సూచీ;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    పూర్ణాంకం tpinnum = 0;
    AstVar* sformatp = శూన్యసూచిక;
    కొరకు (Ast_కణుపు* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
            ఒకవేళ (portp->isIO()) {
                tconnects.emplace_వెనుక(portp, static_cast<AstArg*>(శూన్యసూచిక));
                name_గమ్యం_సూచీ.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                ఒకవేళ (portp->attrSFormat()) {
                    sformatp = portp;
                } లేకపోతే ఒకవేళ (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    పూర్ణాంకం ppinnum = 0;
    తార్కిక reorganize = అబద్ధం;
    కొరకు (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        ఒకవేళ (!argp->name().ఖాళీలు()) {
            // By name
            స్థిరరాశి స్వయంచాలక it = name_గమ్యం_సూచీ.కనుగొను(argp->name());
            ఒకవేళ (it == name_గమ్యం_సూచీ.చివర()) {
                ఒకవేళ (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warn_మరిన్ని() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_వెనుక()->delete_వృక్షం(), argp);
                }
            } లేకపోతే {
                ఒకవేళ (tconnects[it->రెండవ].రెండవ && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->రెండవ].రెండవ->unlink_Fr_వెనుక()->delete_వృక్షం();
                }
                tconnects[it->రెండవ].రెండవ = argp;
                reorganize = నిజం;
            }
        } లేకపోతే {  // By pin number
            ఒకవేళ (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } లేకపోతే ఒకవేళ (ppinnum >= tpinnum) {
                ఒకవేళ (sformatp) {
                    tconnects.emplace_వెనుక(sformatp, static_cast<AstArg*>(శూన్యసూచిక));
                    tconnects[ppinnum].రెండవ = argp;
                    ++tpinnum;
                } లేకపోతే ఒకవేళ (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warn_మరిన్ని() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_వెనుక()->delete_వృక్షం(), argp);
                }
            } లేకపోతే {
                tconnects[ppinnum].రెండవ = argp;
            }
        }
        ++ppinnum;
    }

    ఒకవేళ (!makeChanges) తిరిగి tconnects;

    // Connect missing ones
    VInsertion_సమితి<స్థిరరాశి AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    కొరకు (పూర్ణాంకం i = 0; i < tpinnum; ++i) {
        AstVar* స్థిరరాశి portp = tconnects[i].మొదటి;
        ఒకవేళ (!tconnects[i].రెండవ || !tconnects[i].రెండవ->exprp()) {
            Ast_కణుపు_సమాసం* newvaluep = శూన్యసూచిక;
            ఒకవేళ (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = కొత్త AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } లేకపోతే ఒకవేళ (AstFuncRef* స్థిరరాశి funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి funcp = funcRefp->taskp();
                ఒకవేళ (funcp->class_పద్ధతి() && funcp->isStatic()) newvaluep = funcRefp;
            } లేకపోతే ఒకవేళ (AstConst* స్థిరరాశి constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            ఒకవేళ (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), కణుపు_సమాసం));
                ఒకవేళ (!VN_IS(newvaluep, Const)) {
                    ఒకవేళ (statep) {
                        portp->pin_అంకె(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.చొప్పించు(portp);
                    } లేకపోతే {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = కొత్త AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_వృక్షం(నిజం) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* స్థిరరాశి newp = కొత్త AstArg{nodep->fileline(), portp->name(), newvaluep};
            ఒకవేళ (tconnects[i].రెండవ) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].రెండవ->unlink_Fr_వెనుక()->delete_వృక్షం(),
                            tconnects[i].రెండవ = శూన్యసూచిక);
            }
            tconnects[i].రెండవ = newp;
            reorganize = నిజం;
        }
        ఒకవేళ (tconnects[i].రెండవ) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].రెండవ);
        } లేకపోతే {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
        AstArg* స్థిరరాశి argp = tconnect.రెండవ;
        argp->name("");  // Can forget name as will add back in pin order
    }

    ఒకవేళ (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        అయితే (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlink_Fr_వెనుక();
        }
        కొరకు (పూర్ణాంకం i = 0; i < tpinnum; ++i) {
            AstArg* స్థిరరాశి argp = tconnects[i].రెండవ;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->కూడు_Argsp(argp);
        }
    }

    ఒకవేళ (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_వృక్షం("-  ftref-out: ");
        కొరకు (పూర్ణాంకం i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvt_గమ్యం_Hex(tconnects[i].మొదటి)
                               << "  conn=" << cvt_గమ్యం_Hex(tconnects[i].రెండవ));
        }
    }  // LCOV_EXCL_STOP

    ఒకవేళ (!argWrap.ఖాళీలు()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        తిరిగి taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            శూన్యసూచిక);
    }
    తిరిగి tconnects;
}

శూన్య V3Task::taskConnectWrap(Ast_కణుపు_FTask_Ref* nodep, స్థిరరాశి V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             స్థిరరాశి VInsertion_సమితి<స్థిరరాశి AstVar*>& argWrap) {
    statep->సమితి_Did_Wrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    కొరకు (స్థిరరాశి AstVar* varp : argWrap) newname += "_" + cvt_గమ్యం_Str(varp->pin_అంకె());
    స్థిరరాశి స్వయంచాలక pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(శూన్యసూచిక));
    ఒకవేళ (pair.రెండవ) {
        pair.మొదటి->రెండవ = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_కణుపు_FTask* స్థిరరాశి newTaskp = pair.మొదటి->రెండవ;

    // Remove the defaulted arguments from original outside call
    కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
        స్థిరరాశి AstVar* స్థిరరాశి portp = tconnect.మొదటి;
        AstArg* స్థిరరాశి argp = tconnect.రెండవ;
        ఒకవేళ (argWrap.exists(portp)) {  // Removed arg
            statep->నెట్టు_Deletep(argp->unlink_Fr_వెనుక());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_కణుపు_FTask* V3Task::taskConnectWrapNew(Ast_కణుపు_FTask* taskp, స్థిరరాశి string& newname,
                                         స్థిరరాశి V3TaskConnects& tconnects,
                                         స్థిరరాశి VInsertion_సమితి<స్థిరరాశి AstVar*>& argWrap) {
    std::map<స్థిరరాశి AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_కణుపు_FTask* స్థిరరాశి newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->కూడు_తదుపరి_ఇక్కడ(newTaskp);

    Ast_కణుపు_FTask_Ref* newCallp = శూన్యసూచిక;
    Ast_కణుపు* newCallInsertp = శూన్యసూచిక;
    ఒకవేళ (VN_IS(taskp, Func)) {
        AstVar* స్థిరరాశి fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* స్థిరరాశి newFVarp = fvarp->clone_వృక్షం(నిజం);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = కొత్త AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = కొత్త Ast_కేటాయించు{taskp->fileline(),
                            కొత్త AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } లేకపోతే ఒకవేళ (VN_IS(taskp, Task)) {
        newCallp = కొత్త AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = కొత్త Ast_Stmt_సమాసం{taskp->fileline(), newCallp};
    } లేకపోతే {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
        AstVar* స్థిరరాశి portp = tconnect.మొదటి;
        AstVar* newPortp;
        ఒకవేళ (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = కొత్త AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(నిజం);
            ఒకవేళ (newPortp->valuep()) newPortp->valuep()->unlink_Fr_వెనుక()->delete_వృక్షం();
            newTaskp->కూడు_Stmtsp(newPortp);
        } లేకపోతే {  // Defaulting arg
            Ast_కణుపు_సమాసం* స్థిరరాశి valuep = VN_AS(portp->valuep(), కణుపు_సమాసం);
            ఒకవేళ ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                స్థిరరాశి VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* స్థిరరాశి refp = VN_AS(valuep->clone_వృక్షం(అబద్ధం), VarRef);
                refp->access(refAccess);
                AstArg* స్థిరరాశి newArgp = కొత్త AstArg{portp->fileline(), portp->name(), refp};
                newCallp->కూడు_Argsp(newArgp);
                కొనసాగు;
            }
            // Create local temporary
            newPortp = కొత్త AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(నిజం);
            newTaskp->కూడు_Stmtsp(newPortp);
            // Runtime-assign it to the default
            ఒకవేళ (!VN_IS(valuep, ఖాళీలు_వరుస)) {
                Ast_కేటాయించు* స్థిరరాశి newAssignp
                    = కొత్త Ast_కేటాయించు{valuep->fileline(),
                                    కొత్త AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_వృక్షం(నిజం)};
                newTaskp->కూడు_Stmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* స్థిరరాశి newArgp = కొత్త AstArg{
            portp->fileline(), portp->name(),
            కొత్త AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->కూడు_Argsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->కూడు_Stmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        స్థిరరాశి స్వయంచాలక it = oldNewVars.కనుగొను(refp->varp());
        ఒకవేళ (it != oldNewVars.చివర()) refp->varp(it->రెండవ);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    తిరిగి newTaskp;
}

string V3Task::కేటాయించు_అంతర్గత_గమ్యం_Dpi(AstVar* portp, తార్కిక isPtr, స్థిరరాశి string& frSuffix,
                                   స్థిరరాశి string& గమ్యం_Suffix, స్థిరరాశి string& fr_ఉపసర్గ) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    స్థిరరాశి string frName = fr_ఉపసర్గ + portp->name() + frSuffix;
    స్థిరరాశి string గమ్యం_Name = portp->name() + గమ్యం_Suffix;
    స్థిరరాశి string idx = portp->name() + "__Vidx";
    కొలత_t unpack_కొలత = 1;  // non-unpacked array is treated as size 1
    పూర్ణాంకం unpackDim = 0;
    ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpack_కొలత = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(అబద్ధం).రెండవ;
        ఒకవేళ (unpackDim > 0) UASSERT_OBJ(unpack_కొలత > 0, portp, "size must be greater than 0");
    }
    ఒకవేళ (portp->basicp()->is_Dpi_బిట్టు_Vec() || portp->basicp()->isDpiLogicVec()) {
        స్థిరరాశి తార్కిక is_బిట్టు = portp->basicp()->is_Dpi_బిట్టు_Vec();
        స్థిరరాశి తార్కిక needsFor = unpack_కొలత > 1;
        ఒకవేళ (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvt_గమ్యం_Str(unpack_కొలత) + "; ++"
                   + idx + ") ";
        }

        stmt += (is_బిట్టు ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvt_గమ్యం_Str(portp->వెడల్పు()) + ", ";
        stmt += గమ్యం_Name;
        ఒకవేళ (needsFor) {
            stmt += " + " + cvt_గమ్యం_Str(portp->dtypep()->skipRefp()->వెడల్పు_పదాలు()) + " * " + idx;
        }
        stmt += ", ";
        ఒకవేళ (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            కొరకు (పూర్ణాంకం i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } లేకపోతే {
            stmt += frName + ")";
        }
    } లేకపోతే {
        స్థిరరాశి తార్కిక isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        స్థిరరాశి తార్కిక isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        స్థిరరాశి string unpackAt = unpack_కొలత > 1 ? "[" + idx + "]" : "[0]";
        ఒకవేళ (unpackDim > 0) {
            స్థిరరాశి string forStmt = unpack_కొలత > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvt_గమ్యం_Str(unpack_కొలత) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + గమ్యం_Name + unpackAt;
        } లేకపోతే {
            ఒకవేళ (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += గమ్యం_Name;
        }
        stmt += " = ";
        ఒకవేళ (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        ఒకవేళ (unpackDim > 0) {
            stmt += "(&" + frName;
            కొరకు (పూర్ణాంకం i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } లేకపోతే {
            stmt += frName;
        }
        ఒకవేళ (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    తిరిగి stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::కేటాయించు_Dpi_గమ్యం_అంతర్గత(స్థిరరాశి std::string& lhsName, AstVar* varp) {
    std::string cvt;
    తార్కిక useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_గమ్యం_అంతర్గత_Cvt_Stmt(varp->dtypep());
    స్థిరరాశి std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    స్థిరరాశి పూర్ణాంకం మొత్తం = strides.ఖాళీలు() ? 1 : strides[0].మొదటి * strides[0].రెండవ;
    స్థిరరాశి పూర్ణాంకం వెడల్పు_పదాలు = varp->basicp()->వెడల్పు_పదాలు();
    string statements;
    కొరకు (పూర్ణాంకం i = 0; i < మొత్తం; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        కొరకు (స్థిరరాశి స్వయంచాలక& stride : strides) {
            lhs += "[" + std::గమ్యం_string((i / stride.రెండవ) % stride.మొదటి) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        ఒకవేళ (useSvVec) {
            స్థిరరాశి std::string స్థానభ్రంశం = std::గమ్యం_string(i * వెడల్పు_పదాలు);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + స్థానభ్రంశం + ");\n";
        } లేకపోతే {
            స్థిరరాశి std::string elem = strides.ఖాళీలు() ? "" : "[" + std::గమ్యం_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    తిరిగి statements;
}

శూన్య V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        స్థిరరాశి TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_వృక్షం("task", 0, dump_వృక్షం_Either_Level() >= 3);
}
