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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Task.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3LanguageWords.h"
#अन्तर्भाव "V3SharedTmps.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

वर्ग Task_आधारः_Vertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(Task_आधारः_Vertex, V3GraphVertex);
    Ast_ग्रन्थिः* m_impurep = शून्यसूचकः;  // Node causing impure function w/ outside references
    तार्किक m_noInline = असत्य;  // Marked with pragma
सार्वजनिक:
    स्पष्ट Task_आधारः_Vertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~Task_आधारः_Vertex() override = स्वभाव;
    तार्किक विशुद्धम्_3() ध्रुव { प्रत्यागम m_impurep == शून्यसूचकः; }
    Ast_ग्रन्थिः* impure_ग्रन्थिः() ध्रुव { प्रत्यागम m_impurep; }
    शून्य impure(Ast_ग्रन्थिः* nodep) { m_impurep = nodep; }
    तार्किक noInline() ध्रुव { प्रत्यागम m_noInline; }
    शून्य noInline(तार्किक flag) { m_noInline = flag; }
};

वर्ग TaskFTaskVertex final : सार्वजनिक Task_आधारः_Vertex {
    VL_RTTI_IMPL(TaskFTaskVertex, Task_आधारः_Vertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_ग्रन्थिः_FTask* ध्रुव m_nodep;
    AstCFunc* m_cFuncp = शून्यसूचकः;
    तार्किक m_needsNonInlineCFunc = असत्य;  // Needs CFunc for selected non-inlined call sites

सार्वजनिक:
    TaskFTaskVertex(V3Graph* graphp, Ast_ग्रन्थिः_FTask* nodep)
        : Task_आधारः_Vertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = स्वभाव;
    Ast_ग्रन्थिः_FTask* nodep() ध्रुव VL_MT_STABLE { प्रत्यागम m_nodep; }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम nodep()->name(); }
    string dotColor() ध्रुव override { प्रत्यागम विशुद्धम्_3() ? "black" : "red"; }
    AstCFunc* cFuncp() ध्रुव { प्रत्यागम m_cFuncp; }
    शून्य cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    तार्किक needsNonInlineCFunc() ध्रुव { प्रत्यागम m_needsNonInlineCFunc; }
    शून्य needsNonInlineCFunc(तार्किक flag) { m_needsNonInlineCFunc = flag; }
};

वर्ग Task_संहिता_Vertex final : सार्वजनिक Task_आधारः_Vertex {
    VL_RTTI_IMPL(Task_संहिता_Vertex, Task_आधारः_Vertex);
    // Top vertex for all calls not under another task
सार्वजनिक:
    स्पष्ट Task_संहिता_Vertex(V3Graph* graphp)
        : Task_आधारः_Vertex{graphp} {}
    ~Task_संहिता_Vertex() override = स्वभाव;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम "*CODE*"; }
    string dotColor() ध्रुव override { प्रत्यागम "green"; }
};

वर्ग TaskEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
सार्वजनिक:
    TaskEdge(V3Graph* graphp, Task_आधारः_Vertex* fromp, Task_आधारः_Vertex* शिखरम्)
        : V3GraphEdge{graphp, fromp, शिखरम्, 1, असत्य} {}
    ~TaskEdge() override = स्वभाव;
    string dotLabel() ध्रुव override { प्रत्यागम "w" + cvt_गन्तव्यम्_Str(weight()); }
};

//######################################################################

वर्ग TaskStateVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ध्रुव VNUser3_अन्तर्_Use m_inuser3;
    ध्रुव VNUser4_अन्तर्_Use m_inuser4;

    // TYPES
    उपयुज्य Var_गन्तव्यम्_Scope_Map = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    उपयुज्य Func_गन्तव्यम्_Class_Map = std::unordered_map<ध्रुव Ast_ग्रन्थिः_FTask*, AstClass*>;
    // MEMBERS
    Var_गन्तव्यम्_Scope_Map m_var_गन्तव्यम्_Scope_Map;  // Map for Var -> VarScope mappings
    Func_गन्तव्यम्_Class_Map m_func_गन्तव्यम्_Class_Map;  // Map for ctor func -> class
    Ast_ग्रन्थिः_FTask* m_ctorp = शून्यसूचकः;  // Class constructor
    AstClass* m_classp = शून्यसूचकः;  // Current class
    V3Graph m_callGraph;  // Task call graph
    Task_आधारः_Vertex* m_curVxp;  // Current vertex we're adding to
    std::सदिशम्<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    तार्किक m_under_पत्तनम्_Var = असत्य;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

सार्वजनिक:
    // METHODS
    स्थिर तार्किक is_Virtual_Iface_विधानम्_Call(ध्रुव Ast_ग्रन्थिः_FTask_Ref* refp) {
        ध्रुव Ast_विधानम्_Call* ध्रुव mrefp = VN_CAST(refp, विधानम्_Call);
        यदि (!mrefp) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = mrefp->fromp();
        यदि (!fromp || !fromp->dtypep()) प्रत्यागम असत्य;
        ध्रुव AstIfaceRefDType* ध्रुव ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        प्रत्यागम ifrefp && ifrefp->isVirtual();
    }
    स्थिर तार्किक isIfaceFTaskScope(ध्रुव AstScope* scopep) {
        प्रत्यागम scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_ग्रन्थिः_FTask* nodep) {
        AstScope* ध्रुव scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        प्रत्यागम scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        ध्रुव स्वचालित iter = m_var_गन्तव्यम्_Scope_Map.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_var_गन्तव्यम्_Scope_Map.अन्तः_2(), nodep, "No scope for var");
        प्रत्यागम iter->second;
    }
    AstClass* getClassp(Ast_ग्रन्थिः_FTask* nodep) {
        AstClass* ध्रुव classp = m_func_गन्तव्यम्_Class_Map[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        प्रत्यागम classp;
    }
    शून्य remapFuncClassp(Ast_ग्रन्थिः_FTask* nodep, Ast_ग्रन्थिः_FTask* newp) {
        m_func_गन्तव्यम्_Class_Map[newp] = getClassp(nodep);
    }
    तार्किक ftaskNoInline(Ast_ग्रन्थिः_FTask* nodep) { प्रत्यागम getFTaskVertex(nodep)->noInline(); }
    तार्किक ftaskNeedsNonInlineCFunc(Ast_ग्रन्थिः_FTask* nodep) {
        प्रत्यागम getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_ग्रन्थिः_FTask* nodep) { प्रत्यागम getFTaskVertex(nodep)->cFuncp(); }
    शून्य ftaskCFuncp(Ast_ग्रन्थिः_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_ग्रन्थिः* checkImpure(Ast_ग्रन्थिः_FTask* nodep) { प्रत्यागम getFTaskVertex(nodep)->impure_ग्रन्थिः(); }

गुप्त:
    TaskFTaskVertex* getFTaskVertex(Ast_ग्रन्थिः_FTask* nodep) {
        यदि (!nodep->user4p()) nodep->user4p(नूतन TaskFTaskVertex{&m_callGraph, nodep});
        प्रत्यागम static_cast<TaskFTaskVertex*>(nodep->user4u().गन्तव्यम्_Graph_Vertex());
    }

    शून्य propagateImpureReason() {
        std::सदिशम्<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        प्रति (V3GraphVertex& vtx : m_callGraph.vertices()) {
            यदि (TaskFTaskVertex* ध्रुव ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                यदि (!ftVtxp->विशुद्धम्_3()) workList.emplace_पृष्ठभागः(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        यावत् (!workList.रिक्तानि()) {
            TaskFTaskVertex* ध्रुव calleep = workList.पृष्ठभागः();
            workList.निष्कासय_पृष्ठभागः();
            UASSERT_OBJ(!calleep->विशुद्धम्_3(), calleep->nodep(), "Should not enqueue pure vertex");
            प्रति (V3GraphEdge& edge : calleep->अन्तर्_धाराः()) {
                यदि (TaskFTaskVertex* ध्रुव callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    यदि (!callerp->विशुद्धम्_3()) अनुवर्त;  // Already processed or on workList
                    callerp->impure(calleep->impure_ग्रन्थिः());
                    workList.emplace_पृष्ठभागः(callerp);
                }
            }
        }
    }

    शून्य decideInlining() {
        प्रति (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* ध्रुव ftVtxp = vtx.cast<TaskFTaskVertex>();
            यदि (!ftVtxp) अनुवर्त;
            // Move on if already decided not to inline
            यदि (ftVtxp->noInline()) अनुवर्त;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            यदि (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(सत्य);
                अनुवर्त;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            यदि (v3Global.opt.fInlineFuncsEager()) अनुवर्त;

            Ast_ग्रन्थिः_FTask* ध्रुव ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            यदि (!ftaskp->verilogFunction()) अनुवर्त;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            यदि (ftaskp->task_लौकिकम्()) अनुवर्त;
            // If the function is pure, it's OK not to inline, so don't do it
            यदि (ftVtxp->विशुद्धम्_3()) ftVtxp->noInline(सत्य);
        }
    }

    // VISITORS
    शून्य दर्शय(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVarScope* ध्रुव vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ध्रुव varp = vscp->varp();
                यदि (varp->isFuncLocal() || varp->is_प्रयुक्तम्_Loop_Idx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_var_गन्तव्यम्_Scope_Map.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (Ast_ग्रन्थिः_FTask* ध्रुव taskp = VN_CAST(stmtp, ग्रन्थिः_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* ध्रुव taskVtxp = getFTaskVertex(nodep->taskp());
        नूतन TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        यदि (is_Virtual_Iface_विधानम्_Call(nodep)) taskVtxp->needsNonInlineCFunc(सत्य);
        // Do we have to disable inlining the function?
        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        यदि (!taskVtxp->noInline()) {  // Else short-circuit below
            प्रति (ध्रुव स्वचालित& itr : tconnects) {
                ध्रुव AstVar* ध्रुव portp = itr.first;
                ध्रुव AstArg* ध्रुव argp = itr.second;
                यदि (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = argp->exprp()) {
                    यदि ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(सत्य);
                    }
                }
            }
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        यदि (nodep->dpiImport()) m_curVxp->noInline(सत्य);
        यदि (nodep->class_विधानम्()) m_curVxp->noInline(सत्य);  // Until V3Task supports it
        यदि (nodep->recursive()) m_curVxp->noInline(सत्य);
        यदि (nodep->isConstructor()) {
            m_curVxp->noInline(सत्य);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_func_गन्तव्यम्_Class_Map[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstPragma* nodep) override {
        यदि (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(सत्य);
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य दर्शय(AstVar* nodep) override {
        VL_RESTORER(m_under_पत्तनम्_Var);
        यदि (nodep->isIO()) m_under_पत्तनम्_Var = सत्य;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        iterateChildren(nodep);
        यदि (m_under_पत्तनम्_Var) प्रत्यागम;
        AstVar* ध्रुव varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        यदि (nodep->access().isReadOnly() && varp->const_Pool_प्रविष्टिः()) प्रत्यागम;
        यदि (varp->user4u().गन्तव्यम्_Graph_Vertex() != m_curVxp) {
            यदि (m_curVxp->विशुद्धम्_3() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    शून्य दर्शय(AstClass* nodep) override {
        // Move initial statements into the constructor
        VL_RESTORER_CLEAR(m_initialps);
        VL_RESTORER(m_ctorp);
        VL_RESTORER(m_classp);
        m_ctorp = शून्यसूचकः;
        m_classp = nodep;
        {  // Find m_initialps, m_ctor
            iterateChildren(nodep);
        }
        UASSERT_OBJ(m_ctorp, nodep, "class constructor missing");  // LinkDot always makes it
        Ast_ग्रन्थिः* insertp = शून्यसूचकः;
        प्रति (AstInitialAutomatic* initialp : m_initialps) {
            यदि (Ast_ग्रन्थिः* ध्रुव movep = initialp->stmtsp()) {
                movep->unlink_Fr_पृष्ठभागः_With_अग्र();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(insertp, movep);
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(initialp->unlink_Fr_पृष्ठभागः()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    शून्य दर्शय(AstInitialAutomatic* nodep) override {
        m_initialps.प्रक्षिप_पृष्ठभागः(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = नूतन Task_संहिता_Vertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.remove_Redundant_धाराः_योग(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        यदि (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = स्वभाव;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

संरचना TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    स्थिर std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> unpackDimsAndStrides(AstVar* varp) {
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();
        std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> dimStrides;
        AstUnpackArrayDType* ध्रुव unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        यदि (!unpackp) प्रत्यागम dimStrides;

        ध्रुव std::सदिशम्<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ध्रुव आकार_t nDims = dims.आकार();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.पृष्ठभागः()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        प्रति (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        प्रत्यागम dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    स्थिर std::pair<std::string, तार्किक> dpi_गन्तव्यम्_आन्तरिकम्_Cvt_Stmt(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp();

        // DPI types using Primitive C types
        यदि (basicp->keyword() == VBasicDTypeKwd::CHANDLE) प्रत्यागम {"VL_CVT_VP_Q(", असत्य};
        यदि (basicp->keyword() == VBasicDTypeKwd::STRING) प्रत्यागम {"VL_CVT_N_CSTR(", असत्य};
        यदि (basicp->isDpiPrimitive()) प्रत्यागम {"(", असत्य};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ध्रुव std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ध्रुव वर्ण आकार_Char = dtypep->विस्तारः() <= 8    ? 'C'
                              : dtypep->विस्तारः() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ध्रुव std::string& आकार = std::गन्तव्यम्_string(dtypep->विस्तारः());
        // cppcheck-suppress strPlusChar
        प्रत्यागम {"VL_SET_"s + आकार_Char + "_" + vecType + "(" + आकार + ", ", सत्य};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

वर्ग TaskVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // TYPES
    उपयुज्य DpiCFuncs = std::map<ध्रुव string, std::tuple<Ast_ग्रन्थिः_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ध्रुव m_statep;  // Common state between visitors
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    Ast_शिखरम्_Scope* ध्रुव m_शिखरम्_Scopep = v3Global.rootp()->शिखरम्_Scopep();  // The AstTopScope
    AstScope* m_scopep = शून्यसूचकः;  // Current scope
    Ast_ग्रन्थिः* m_insStmtp = शून्यसूचकः;  // Where to insert statement
    तार्किक m_अन्तर्_Sensesp = असत्य;  // Are we under a senitem?
    तार्किक m_अन्तर्_New = असत्य;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    पूर्णाङ्क m_uncon_Var_अङ्कः = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpi_नामानि;  // Map of all created DPI functions
    std::map<std::string, ध्रुव AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ध्रुव string& name, AstVar* examplep) {
        AstVar* ध्रुव newvarp = नूतन AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(सत्य);
        funcp->सङ्कलय_Varsp(newvarp);
        AstVarScope* ध्रुव newvscp = नूतन AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->सङ्कलय_Varsp(newvscp);
        प्रत्यागम newvscp;
    }
    AstVarScope* create_निवेशः_Var(AstCFunc* funcp, ध्रुव string& name, VBasicDTypeKwd kwd) {
        AstVar* ध्रुव newvarp
            = नूतन AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(सत्य);
        newvarp->direction(VDirection::निवेशः);
        funcp->सङ्कलय_Argsp(newvarp);
        AstVarScope* ध्रुव newvscp = नूतन AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->सङ्कलय_Varsp(newvscp);
        प्रत्यागम newvscp;
    }
    स्थिर AstVarScope* const_Pool_सारणी(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ध्रुव refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ध्रुव vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
        प्रत्यागम vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ध्रुव Ast_ग्रन्थिः_FTask_Ref* refp, AstVar* invarp,
                                ध्रुव std::string& suffix) {
        यदि (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            प्रत्यागम const_Pool_सारणी(invarp);
        } अन्यथा {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* ध्रुव newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* ध्रुव newvarp = newvscp->varp();
            newvarp->funcLocal(असत्य);
            newvarp->propagateAttrFrom(invarp);
            newvarp->is_आन्तरिकम्(सत्य);
            प्रत्यागम newvscp;
        }
    }

    // Replace varrefs with new var pointer
    शून्य relink(Ast_ग्रन्थिः* nodep) {
        nodep->foreach_And_अग्र([](AstVarRef* refp) {
            यदि (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ध्रुव newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    Ast_नियोजय* connect_पत्तनम्_Make_अन्तर्_नियोजय(Ast_ग्रन्थिः_व्यञ्जकम्* pinp, AstVarScope* newvscp, तार्किक विशुद्धम्_Check) {
        // Create input assignment to go in FRONT of function call
        Ast_ग्रन्थिः_व्यञ्जकम्* अन्तर्_Pinp = pinp;
        यदि (AstResizeLValue* sinPinp = VN_CAST(अन्तर्_Pinp, ResizeLValue)) अन्तर्_Pinp = sinPinp->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* अन्तर्_Pin_Clonep
            = विशुद्धम्_Check ? अन्तर्_Pinp->clone_वृक्षः_विशुद्धम्(सत्य) : अन्तर्_Pinp->clone_वृक्षः(सत्य);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ध्रुव AstUnpackArrayDType* ध्रुव arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        यदि (arrayp && arrayp->is_Reverse_सम्बन्धि(अन्तर्_Pin_Clonep->dtypep())) {
            अन्तर्_Pin_Clonep
                = नूतन Ast_Cvt_Unpack_गन्तव्यम्_Unpack{pinp->fileline(), अन्तर्_Pin_Clonep, newvscp->dtypep()};
        }
        Ast_नियोजय* ध्रुव assp = नूतन Ast_नियोजय{
            pinp->fileline(), नूतन AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            अन्तर्_Pin_Clonep};
        assp->fileline()->modifyWarnOff(V3_Error_संहिता::BLKSEQ, सत्य);  // Ok if in <= block
        प्रत्यागम assp;
    }
    Ast_नियोजय* connect_पत्तनम्_Make_बहिर्गमनम्_नियोजय(AstVar* portp, Ast_ग्रन्थिः_व्यञ्जकम्* pinp, AstVarScope* newvscp,
                                        तार्किक विशुद्धम्_Check) {
        // If needed, remap size of function to caller's output size
        Ast_ग्रन्थिः_व्यञ्जकम्* बहिर्गमनम्_Pinp = pinp;
        Ast_ग्रन्थिः_व्यञ्जकम्* postRhsp = नूतन AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        यदि (AstResizeLValue* soutPinp = VN_CAST(बहिर्गमनम्_Pinp, ResizeLValue)) {
            बहिर्गमनम्_Pinp = soutPinp->lhsp();
            यदि (VN_IS(बहिर्गमनम्_Pinp, RToIRoundS) || VN_IS(बहिर्गमनम्_Pinp, RToIS)) {
                बहिर्गमनम्_Pinp = VN_AS(बहिर्गमनम्_Pinp, ग्रन्थिः_Uniop)->lhsp();
                यदि (postRhsp->isSigned()) {
                    postRhsp = नूतन AstISToRD{pinp->fileline(), postRhsp};
                } अन्यथा {
                    postRhsp = नूतन AstIToRD{pinp->fileline(), postRhsp};
                }
            } अन्यथा {
                यदि (Ast_ग्रन्थिः_Uniop* aoutPinp = VN_CAST(बहिर्गमनम्_Pinp, Extend)) {
                    बहिर्गमनम्_Pinp = aoutPinp->lhsp();
                } अन्यथा यदि (Ast_ग्रन्थिः_Uniop* aoutPinp = VN_CAST(बहिर्गमनम्_Pinp, ExtendS)) {
                    बहिर्गमनम्_Pinp = aoutPinp->lhsp();
                } अन्यथा यदि (AstSel* aoutPinp = VN_CAST(बहिर्गमनम्_Pinp, Sel)) {
                    बहिर्गमनम्_Pinp = aoutPinp->fromp();
                } अन्यथा {
                    बहिर्गमनम्_Pinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                यदि (बहिर्गमनम्_Pinp->विस्तारः() < portp->विस्तारः()) {
                    postRhsp = नूतन AstSel{pinp->fileline(), postRhsp, 0, pinp->विस्तारः()};
                } अन्यथा {  // pin width > port width
                    यदि (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = नूतन AstExtendS{pinp->fileline(), postRhsp};
                    } अन्यथा {
                        postRhsp = नूतन AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(बहिर्गमनम्_Pinp);
        }
        यदि (VN_IS(बहिर्गमनम्_Pinp, IToRD) || VN_IS(बहिर्गमनम्_Pinp, ISToRD)) {
            बहिर्गमनम्_Pinp = VN_AS(बहिर्गमनम्_Pinp, ग्रन्थिः_Uniop)->lhsp();
            postRhsp = नूतन AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(बहिर्गमनम्_Pinp);
        }
        // Put output assignment AFTER function call
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव बहिर्गमनम्_Pin_Clonep
            = विशुद्धम्_Check ? बहिर्गमनम्_Pinp->clone_वृक्षः_विशुद्धम्(सत्य) : बहिर्गमनम्_Pinp->clone_वृक्षः(सत्य);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ध्रुव AstUnpackArrayDType* ध्रुव arrayp
            = VN_CAST(बहिर्गमनम्_Pinp->dtypep()->skipRefp(), UnpackArrayDType);
        यदि (arrayp && arrayp->is_Reverse_सम्बन्धि(postRhsp->dtypep())) {
            postRhsp = नूतन Ast_Cvt_Unpack_गन्तव्यम्_Unpack{pinp->fileline(), postRhsp, बहिर्गमनम्_Pinp->dtypep()};
        }
        Ast_नियोजय* ध्रुव assp = नूतन Ast_नियोजय{pinp->fileline(), बहिर्गमनम्_Pin_Clonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3_Error_संहिता::BLKSEQ, सत्य);  // Ok if in <= block
        प्रत्यागम assp;
    }

    शून्य changeAtWriteRecurse(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp) {
        // Change nested at methods to writable variant
        यदि (AstCMethodHard* ध्रुव cMethodp = VN_CAST(exprp, CMethodHard)) {
            यदि (cMethodp->विधानम्() == VCMethod::ARRAY_AT) {
                cMethodp->विधानम्(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    शून्य connect_पत्तनम्(ध्रुव Ast_ग्रन्थिः_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_ग्रन्थिः* beginp,
                     तार्किक inlineTask) {
        Ast_ग्रन्थिः_व्यञ्जकम्* pinp = argp->exprp();
        यदि (inlineTask) {
            portp->unlink_Fr_पृष्ठभागः();
            प्रक्षिप_Deletep(portp);  // Remove it from the clone (not original)
        }
        यदि (!pinp) {
            // Too few arguments in function call
        } अन्यथा {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            यदि (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* ध्रुव varp = नूतन AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::गन्तव्यम्_string(m_uncon_Var_अङ्कः++),
                                                portp->dtypep()};
                m_modp->सङ्कलय_Stmtsp(varp);
                AstVarScope* ध्रुव newvscp = नूतन AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->सङ्कलय_Varsp(newvscp);
                AstVarRef* ध्रुव repp = नूतन AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(pinp), pinp);
                pinp = repp;
            }
            यदि (inlineTask) {
                प्रक्षिप_Deletep(pinp->unlink_Fr_पृष्ठभागः());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), argp);  // Args no longer needed
            }
            यदि (portp->isRef() || portp->isConstRef()) {
                तार्किक refArgOk = असत्य;
                यदि (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = सत्य;
                } अन्यथा यदि (AstCMethodHard* ध्रुव cMethodp = VN_CAST(pinp, CMethodHard)) {
                    यदि (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->विधानम्() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->विधानम्() == VCMethod::DYN_AT_WRITE_APPEND_पृष्ठभागः;
                    } अन्यथा {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->विधानम्() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                यदि (refArgOk) {
                    यदि (AstVarRef* ध्रुव varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } अन्यथा {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                यदि (inlineTask) {
                    यदि (AstVarRef* ध्रुव varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* ध्रुव localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } अन्यथा {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } अन्यथा यदि (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* ध्रुव newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                यदि (!inlineTask) {
                    pinp->replaceWith(
                        नूतन AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    प्रक्षिप_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                Ast_नियोजय* ध्रुव preassp = connect_पत्तनम्_Make_अन्तर्_नियोजय(pinp, newvscp, सत्य);
                beginp->सङ्कलय_अग्र_अत्र(preassp);

                Ast_नियोजय* ध्रुव postassp = connect_पत्तनम्_Make_बहिर्गमनम्_नियोजय(portp, pinp, newvscp, सत्य);
                beginp->सङ्कलय_अग्र(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } अन्यथा यदि (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ध्रुव newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                यदि (!inlineTask) {
                    pinp->replaceWith(नूतन AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    प्रक्षिप_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                Ast_नियोजय* ध्रुव postassp = connect_पत्तनम्_Make_बहिर्गमनम्_नियोजय(portp, pinp, newvscp, असत्य);
                // Put assignment BEHIND of all other statements
                beginp->सङ्कलय_अग्र(postassp);
            } अन्यथा यदि (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ध्रुव newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                Ast_नियोजय* ध्रुव preassp = connect_पत्तनम्_Make_अन्तर्_नियोजय(pinp, newvscp, असत्य);
                // Put assignment in FRONT of all other statements
                beginp->सङ्कलय_अग्र_अत्र(preassp);
            }
        }
    }

    तार्किक has_Ref_तर्कः(Ast_ग्रन्थिः_FTask* nodep) {
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                यदि (varp->isRef() || varp->isConstRef()) प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;
    }

    Ast_ग्रन्थिः* createInlinedFTask(Ast_ग्रन्थिः_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_ग्रन्थिः* ध्रुव newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_वृक्षः(सत्य) : शून्यसूचकः;
        Ast_ग्रन्थिः* ध्रुव beginp
            = नूतन AstComment{refp->fileline(), "Function: "s + refp->name(), सत्य};
        यदि (newbodysp) beginp->सङ्कलय_अग्र(newbodysp);
        यदि (debug() >= 9) beginp->dump_वृक्षः_And_अग्र(निर्गमः, "-  newbegi: ");
        //
        // Create input variables
        Ast_ग्रन्थिः::user2_Clear_वृक्षः();
        {
            ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            प्रति (ध्रुव स्वचालित& itr : tconnects) {
                AstVar* ध्रुव portp = itr.first;
                AstArg* ध्रुव argp = itr.second;
                connect_पत्तनम्(refp, portp, argp, beginp, सत्य);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_ग्रन्थिः* nextstmtp;
            प्रति (Ast_ग्रन्थिः* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    यदि (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* ध्रुव localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        यदि (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_ग्रन्थिः* ध्रुव crstp = नूतन Ast_नियोजय{
                                portp->fileline(),
                                नूतन AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                नूतन AstCReset{portp->fileline(), portp, असत्य}};
                            portp->replaceWith(crstp);
                        } अन्यथा {
                            portp->unlink_Fr_पृष्ठभागः();
                        }
                        प्रक्षिप_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        यदि (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        यदि (debug() >= 9) beginp->dump_वृक्षः_And_अग्र(निर्गमः, "-  iotask: ");
        प्रत्यागम beginp;
    }

    Ast_ग्रन्थिः* createNonInlinedFTask(Ast_ग्रन्थिः_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ध्रुव cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_ग्रन्थिः* ध्रुव beginp
            = नूतन AstComment{refp->fileline(), "Function: "s + refp->name(), सत्य};
        Ast_ग्रन्थिः_CCall* ccallp;
        यदि (VN_IS(refp, New)) {
            AstCNew* ध्रुव cnewp = नूतन AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } अन्यथा यदि (ध्रुव Ast_विधानम्_Call* ध्रुव mrefp = VN_CAST(refp, विधानम्_Call)) {
            ccallp = नूतन AstCMethodCall{refp->fileline(), mrefp->fromp()->unlink_Fr_पृष्ठभागः(), cfuncp};
            ccallp->dtype_समुच्चयः_Void();
            beginp->सङ्कलय_अग्र(ccallp->makeStmt());
        } अन्यथा {
            ccallp = नूतन AstCCall{refp->fileline(), cfuncp};
            ccallp->dtype_समुच्चयः_Void();
            beginp->सङ्कलय_अग्र(ccallp->makeStmt());
        }

        यदि (ध्रुव AstFuncRef* ध्रुव funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } अन्यथा यदि (ध्रुव AstTaskRef* ध्रुव taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } अन्यथा यदि (ध्रुव Ast_विधानम्_Call* ध्रुव विधानम्_Callp = VN_CAST(refp, विधानम्_Call)) {
            ccallp->superReference(विधानम्_Callp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            प्रति (ध्रुव स्वचालित& itr : tconnects) {
                AstVar* ध्रुव portp = itr.first;
                AstArg* ध्रुव argp = itr.second;
                connect_पत्तनम्(refp, portp, argp, beginp, असत्य);
            }
        }
        // First argument is symbol table, then output if a function
        ध्रुव तार्किक needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        यदि (needSyms) ccallp->argTypes("vlSymsp");

        यदि (refp->taskp()->dpiContext()) {
            AstScopeName* ध्रुव snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlink_Fr_पृष्ठभागः();
            File_पङ्क्तिः* ध्रुव flp = refp->fileline();
            // __Vscopep
            ccallp->सङ्कलय_Argsp(snp);
            // __Vfilenamep
            ccallp->सङ्कलय_Argsp(
                नूतन AstCExpr{flp, AstCExpr::विशुद्धम्{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->सङ्कलय_Argsp(नूतन AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        प्रति (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->सङ्कलय_Argsp(argp->exprp()->unlink_Fr_पृष्ठभागः());
        }

        यदि (outvscp) ccallp->सङ्कलय_Argsp(नूतन AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        यदि (debug() >= 9) beginp->dump_वृक्षः_And_अग्र(निर्गमः, "-  nitask: ");
        प्रत्यागम beginp;
    }

    string dpiSignature(Ast_ग्रन्थिः_FTask* nodep, AstVar* rtnvarp) ध्रुव {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        यदि (nodep->dpi_विशुद्धम्()) dpiproto += "pure ";
        यदि (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(सत्य, सत्य) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string तर्काः;
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    यदि (तर्काः != "") {
                        तर्काः += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ध्रुव std::string dpiType = portp->dpiArgType(असत्य, असत्य);
                    dpiproto += dpiType;
                    ध्रुव std::string vType = portp->dtypep()->अन्तर्भवति_Unpacked_Struct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(असत्य);
                    यदि (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    तर्काः += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        प्रत्यागम dpiproto;
    }

    स्थिर शून्य checkLegalCIdentifier(Ast_ग्रन्थिः* nodep, ध्रुव string& name) {
        यदि (name.अन्तः_2() != std::find_if(name.begin(), name.अन्तः_2(), [](वर्ण c) {
                प्रत्यागम !std::अक्षरम्_अङ्कः_वा(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    स्थिर Ast_ग्रन्थिः* create_Dpi_अस्थायी(AstVar* portp, ध्रुव string& suffix) {
        ध्रुव string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        प्रत्यागम नूतन AstCStmt{portp->fileline(), stmt};
    }

    शून्य unlinkAndClone(Ast_ग्रन्थिः_FTask* funcp, Ast_ग्रन्थिः* nodep, तार्किक with_अग्र) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        यदि (funcp->recursive()) {
            VNRelinker relinkHandle;
            यदि (with_अग्र) {
                nodep->unlink_Fr_पृष्ठभागः_With_अग्र(&relinkHandle);
            } अन्यथा {
                nodep->unlink_Fr_पृष्ठभागः(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_ग्रन्थिः* ध्रुव newp = nodep->clone_वृक्षः(with_अग्र);
            relinkHandle.relink(newp);
        } अन्यथा {
            यदि (with_अग्र) {
                nodep->unlink_Fr_पृष्ठभागः_With_अग्र();
            } अन्यथा {
                nodep->unlink_Fr_पृष्ठभागः();
            }
        }
    }

    स्थिर Ast_ग्रन्थिः* create_नियोजय_आन्तरिकम्_गन्तव्यम्_Dpi(AstVar* portp, तार्किक isPtr, ध्रुव string& frSuffix,
                                              ध्रुव string& गन्तव्यम्_Suffix) {
        ध्रुव string stmt = V3Task::नियोजय_आन्तरिकम्_गन्तव्यम्_Dpi(portp, isPtr, frSuffix, गन्तव्यम्_Suffix);
        प्रत्यागम नूतन AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    स्थिर Ast_ग्रन्थिः_Stmt* createDpiScalarCvt(Ast_ग्रन्थिः_व्यञ्जकम्* internalp, ध्रुव std::string& dpi,
                                           ध्रुव Ast_ग्रन्थिः_DType* dtypep, तार्किक गन्तव्यम्_Dpi) {
        File_पङ्क्तिः* ध्रुव flp = internalp->fileline();
        ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp();
        यदि (गन्तव्यम्_Dpi) {
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
            यदि (basicp->isDpiPrimitive()) {
                cstmtp->सङ्कलय(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->सङ्कलय(internalp);
                cstmtp->सङ्कलय(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } अन्यथा {
                cstmtp->सङ्कलय((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::गन्तव्यम्_string(dtypep->विस्तारः()) + ", " + dpi + ", ");
                cstmtp->सङ्कलय(internalp);
                cstmtp->सङ्कलय(");");
            }
            प्रत्यागम cstmtp;
        }
        std::string cvt;
        तार्किक useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_गन्तव्यम्_आन्तरिकम्_Cvt_Stmt(dtypep);
        यदि (useSvVec) {
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
            cstmtp->सङ्कलय(cvt);
            cstmtp->सङ्कलय(internalp);
            cstmtp->सङ्कलय(", " + dpi + ");");
            प्रत्यागम cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ध्रुव पूर्णाङ्क cwidth = basicp->विस्तारः_शब्दाः() * VL_EDATASIZE;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstCExpr{flp, cvt + dpi + ")", cwidth};
        प्रत्यागम नूतन Ast_नियोजय{flp, internalp, नूतन AstSel{flp, rhsp, 0, dtypep->विस्तारः()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    स्थिर Ast_ग्रन्थिः_Stmt* createDpiCvt(Ast_ग्रन्थिः_व्यञ्जकम्* internalp, ध्रुव std::string& dpi,
                                     ध्रुव Ast_ग्रन्थिः_DType* dtypep, तार्किक गन्तव्यम्_Dpi) {
        File_पङ्क्तिः* ध्रुव flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_ग्रन्थिः_Stmt* stmtsp = शून्यसूचकः;
        यदि (ध्रुव AstUnpackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            प्रति (पूर्णाङ्क i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elemp = नूतन AstArraySel{flp, internalp->clone_वृक्षः(असत्य), i};
                stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::गन्तव्यम्_string(i) + "]",
                                                       arrayp->subDTypep(), गन्तव्यम्_Dpi));
            }
        } अन्यथा यदि (dtypep->अन्तर्भवति_Unpacked_Struct()) {
            प्रति (ध्रुव AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ध्रुव selp
                    = नूतन AstStructSel{flp, internalp->clone_वृक्षः(असत्य), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_ग्रन्थिः::सङ्कलय_अग्र(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), गन्तव्यम्_Dpi));
            }
        } अन्यथा {
            प्रत्यागम createDpiScalarCvt(internalp, dpi, dtypep, गन्तव्यम्_Dpi);
        }
        VL_DO_DANGLING(internalp->delete_वृक्षः(), internalp);
        प्रत्यागम stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    स्थिर Ast_ग्रन्थिः_Stmt* createDpiArgCvt(AstVarScope* vscp, ध्रुव std::string& dpiName,
                                        तार्किक गन्तव्यम्_Dpi) {
        AstVar* ध्रुव varp = vscp->varp();
        File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
        ध्रुव std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ध्रुव पूर्णाङ्क योग = strides.रिक्तानि() ? 1 : strides[0].first * strides[0].second;
        ध्रुव तार्किक isStruct = varp->dtypep()->अन्तर्भवति_Unpacked_Struct();
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_ग्रन्थिः_Stmt* stmtsp = शून्यसूचकः;
        प्रति (पूर्णाङ्क i = 0; i < योग; ++i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* internalp
                = नूतन AstVarRef{flp, vscp, गन्तव्यम्_Dpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            प्रति (ध्रुव स्वचालित& stride : strides) {
                internalp = नूतन AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            यदि (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::गन्तव्यम्_string(i * varp->basicp()->विस्तारः_शब्दाः());
            } अन्यथा यदि (isStruct || !strides.रिक्तानि()) {
                dpi += "[" + std::गन्तव्यम्_string(i) + "]";
            }
            stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, गन्तव्यम्_Dpi));
        }
        प्रत्यागम stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_ग्रन्थिः_FTask* ध्रुव nodep, AstVar* ध्रुव rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        ध्रुव std::string cname = Ast_ग्रन्थिः::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ध्रुव वर्ण* ध्रुव tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ध्रुव std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(सत्य, सत्य) : "";
        // The function we are building
        AstCFunc* ध्रुव funcp = नूतन AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(सत्य);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(सत्य);
        funcp->प्रविष्टिः_Point(सत्य);
        funcp->isStatic(सत्य);
        funcp->protect(असत्य);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_शिखरम्_Scopep->scopep()->सङ्कलय_Blocksp(funcp);
        // Check the arguments before converting them
        यदि (!make_पत्तनम्_List(nodep, funcp)) प्रत्यागम शून्यसूचकः;

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
        AstCStmt* ध्रुव prep = नूतन AstCStmt{flp};
        funcp->सङ्कलय_Stmtsp(prep);
        // Static doesn't need save-restore as if below will re-fill proper value
        prep->सङ्कलय("static int __Vfuncnum = -1;\n");
        // First time init (faster than what the compiler does if we did a singleton
        prep->सङ्कलय("if (VL_UNLIKELY(__Vfuncnum == -1)) {\n");
        prep->सङ्कलय("__Vfuncnum = Verilated::exportFuncNum(\"" + nodep->cname() + "\");\n");
        prep->सङ्कलय("}\n");
        // If the find fails, it will throw an error
        prep->सङ्कलय("const VerilatedScope* const __Vscopep = Verilated::dpiScope();\n");
        // If 'dpiScope()' fails and '__Vscopep' is null; the exportFind function throws an error
        // If __Vcb is null the exportFind function throws and error
        ध्रुव std::string cbtype
            = VIdProtect::protect(v3Global.opt.उपसर्गः() + "__Vcb_" + nodep->cname() + "_t");
        prep->सङ्कलय(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ध्रुव callp = नूतन AstCStmt{flp};
        ध्रुव स्वचालित सङ्कलय_Func_Arg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ध्रुव vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(असत्य);
            portp->protect(असत्य);
            // Add argument to call
            ध्रुव VAccess access = portp->direction().pinAccess();
            callp->सङ्कलय(", ");
            callp->सङ्कलय(नूतन AstVarRef{portp->fileline(), vscp, access});
            प्रत्यागम vscp;
        };
        // Call callback
        callp->सङ्कलय("(*__Vcb)(");
        // First argument is the Syms
        callp->सङ्कलय("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_ग्रन्थिः* बहिर्गमनम्_Cvtsp = शून्यसूचकः;  // Conversions of output/inout arguments, after the call
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ध्रुव portp = VN_CAST(stmtp, Var);
            यदि (!portp) अनुवर्त;
            यदि (!portp->isIO()) अनुवर्त;
            यदि (portp->isFuncReturn()) अनुवर्त;
            यदि (portp == rtnvarp) अनुवर्त;  // Handled below
            // Add argument to call
            AstVarScope* ध्रुव outvscp = सङ्कलय_Func_Arg(portp);
            ध्रुव तार्किक isStruct = portp->dtypep()->अन्तर्भवति_Unpacked_Struct();
            यदि (portp->isWritable()) {
                बहिर्गमनम्_Cvtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(
                    बहिर्गमनम्_Cvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), सत्य)
                                       : create_नियोजय_आन्तरिकम्_गन्तव्यम्_Dpi(portp, सत्य, tmpSuffixp, ""));
            }
            यदि (!portp->isNonOutput()) अनुवर्त;
            // Convert input/inout arguments to dpi type
            ध्रुव std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(असत्य).second == 0
                      ? "*"
                      : "";
            funcp->सङ्कलय_Stmtsp(createDpiArgCvt(outvscp, deref + portp->name(), असत्य));
        }
        // Return value argument goes last
        यदि (rtnvarp) सङ्कलय_Func_Arg(rtnvarp);
        // Close statement
        callp->सङ्कलय(");");
        // Call the user function
        funcp->सङ्कलय_Stmtsp(callp);
        // Convert output/inout arguments back to internal type
        यदि (बहिर्गमनम्_Cvtsp) funcp->सङ्कलय_Stmtsp(बहिर्गमनम्_Cvtsp);
        // Convert return value
        यदि (rtnvarp) {
            funcp->सङ्कलय_Stmtsp(create_Dpi_अस्थायी(rtnvarp, ""));
            funcp->सङ्कलय_Stmtsp(create_नियोजय_आन्तरिकम्_गन्तव्यम्_Dpi(rtnvarp, असत्य, tmpSuffixp, ""));
            ध्रुव std::string सूचकांक = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{flp, "return " + rtnvarp->name() + सूचकांक + ";"});
        }
        प्रत्यागम funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_ग्रन्थिः_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ध्रुव string name = Ast_ग्रन्थिः::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ध्रुव string rtnType = rtnvarp            ? rtnvarp->dpiArgType(सत्य, सत्य)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ध्रुव funcp = नूतन AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(सत्य);
        funcp->dontCombine(सत्य);
        funcp->प्रविष्टिः_Point(असत्य);
        funcp->is_विधानम्(असत्य);
        funcp->protect(असत्य);
        funcp->dpi_विशुद्धम्(nodep->dpi_विशुद्धम्());

        ध्रुव पूर्णाङ्क cost = static_cast<पूर्णाङ्क>(V3Control::get_Profile_दत्त(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_शिखरम्_Scopep->scopep()->सङ्कलय_Blocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        यदि (!make_पत्तनम्_List(nodep, funcp)) प्रत्यागम शून्यसूचकः;
        प्रत्यागम funcp;
    }

    AstCFunc* getDpiFunc(Ast_ग्रन्थिः_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ध्रुव string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ध्रुव स्वचालित pair = m_dpi_नामानि.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, शून्यसूचकः));
        यदि (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ध्रुव funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            प्रत्यागम funcp;
        } अन्यथा {
            // Seen this cname import before. Check if it's the same prototype.
            ध्रुव Ast_ग्रन्थिः_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            यदि (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warn_अधिकम्()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                प्रत्यागम शून्यसूचकः;
            } अन्यथा {
                // Same signature, return the previously created CFunc
                प्रत्यागम firstFuncp;
            }
        }
    }

    स्थिर तार्किक make_पत्तनम्_List(Ast_ग्रन्थिः_FTask* nodep, AstCFunc* dpip) {
        तार्किक allOk = सत्य;
        // Copy nodep's list of function I/O to the new dpip c function
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isIO()) {
                    // Move it to new function
                    AstVar* ध्रुव newPortp = portp->clone_वृक्षः(असत्य);
                    newPortp->funcLocal(सत्य);
                    dpip->सङ्कलय_Argsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    यदि (!portp->basicp() && !portp->dtypep()->अन्तर्भवति_Unpacked_Struct()) {
                        allOk = असत्य;
                        portp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: DPI argument of type "
                                << portp->dtypep()->prettyTypeName() << '\n'
                                << portp->warn_अधिकम्()
                                << "... For best portability, use bit, byte, int, or longint");
                        // We don't warn on logic either, although the 4-stateness is lost.
                        // That's what other simulators do.
                    }
                }
            }
        }
        प्रत्यागम allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    स्थिर तार्किक isDpiCIdentifier(ध्रुव string& name) {
        प्रत्यागम !name.रिक्तानि() && VString::isIdentifier(name)
               && !std::अङ्कः_वा(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_शब्दाः::isKeyword(name).रिक्तानि();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    तार्किक checkDpiStruct(ध्रुव AstVar* portp, ध्रुव AstStructDType* sdtypep, तार्किक nested) {
        ध्रुव std::string name = sdtypep->dpiName();
        यदि (!(nested && name.रिक्तानि()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(सत्य) << "\n"
                              << portp->warn_अधिकम्()
                              << "... Suggest use a typedef with a C identifier name");
            प्रत्यागम असत्य;
        }
        प्रति (ध्रुव AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            यदि (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                प्रत्यागम असत्य;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ध्रुव Ast_ग्रन्थिः_DType* dtypep = itemp->subDTypep()->skipRefp();
            यावत् (ध्रुव AstUnpackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव subp = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType);
            ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ध्रुव तार्किक isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            यदि (dtypep->अन्तर्भवति_Unpacked_Struct()) {
                यदि (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), सत्य)) प्रत्यागम असत्य;
            } अन्यथा यदि (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(सत्य)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                प्रत्यागम असत्य;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        यदि (name.रिक्तानि()) प्रत्यागम सत्य;
        ध्रुव AstStructDType* ध्रुव firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        यदि (firstp->dpiTypedef() == sdtypep->dpiTypedef()) प्रत्यागम सत्य;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        प्रत्यागम असत्य;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    तार्किक check_Dpi_Struct_तर्काः(ध्रुव Ast_ग्रन्थिः_FTask* nodep) {
        तार्किक allOk = सत्य;
        प्रति (ध्रुव Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ध्रुव AstVar* ध्रुव portp = VN_CAST(stmtp, Var);
            यदि (portp && portp->isIO() && portp->dtypep()->अन्तर्भवति_Unpacked_Struct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       असत्य)
                        && allOk;
            }
        }
        प्रत्यागम allOk;
    }

    शून्य bodyDpiImportFunc(Ast_ग्रन्थिः_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ध्रुव वर्ण* ध्रुव tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        यदि (v3Global.opt.profExec()) {
            cfuncp->सङ्कलय_Stmtsp(AstCStmt::prof_Exec_Section_प्रक्षिप(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string तर्काः;
        प्रति (Ast_ग्रन्थिः* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ध्रुव portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                यदि (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    यदि (तर्काः != "") तर्काः += ", ";

                    यदि (portp->isDpiOpenArray()) {
                        Ast_ग्रन्थिः_DType* ध्रुव dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ध्रुव string propName = portp->name() + "__Vopenprops";
                        ध्रुव string prop_संहिता = portp->vlPropDecl(propName);
                        cfuncp->सङ्कलय_Stmtsp(नूतन AstCStmt{portp->fileline(), prop_संहिता});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ध्रुव string name = portp->name() + "__Vopenarray";
                        ध्रुव string var_संहिता
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->सङ्कलय_Stmtsp(नूतन AstCStmt{portp->fileline(), var_संहिता});
                        तर्काः += "&" + name;
                    } अन्यथा {
                        // Unpacked structures are passed by reference from a C array
                        ध्रुव तार्किक isStruct = portp->dtypep()->अन्तर्भवति_Unpacked_Struct();
                        यदि (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            यदि (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) तर्काः += "&";
                        }

                        तर्काः += portp->name() + tmpSuffixp;

                        cfuncp->सङ्कलय_Stmtsp(create_Dpi_अस्थायी(portp, tmpSuffixp));
                        यदि (portp->isNonOutput()) {
                            cfuncp->सङ्कलय_Stmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, सत्य)
                                    : create_नियोजय_आन्तरिकम्_गन्तव्यम्_Dpi(portp, असत्य, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        यदि (nodep->dpiContext()) {
            ध्रुव string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->सङ्कलय_Stmtsp(नूतन AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ध्रुव callp = नूतन AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtype_समुच्चयः_Void();
            callp->argTypes(तर्काः);
            यदि (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->सङ्कलय_Stmtsp(create_Dpi_अस्थायी(rtnvscp->varp(), tmpSuffixp));
                ध्रुव std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{nodep->fileline()};
                cstmtp->सङ्कलय(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->सङ्कलय(" = ");
                cstmtp->सङ्कलय(callp);  // RHS
                cstmtp->सङ्कलय(";");
                cfuncp->सङ्कलय_Stmtsp(cstmtp);
            } अन्यथा {
                // Othervise just call it
                cfuncp->सङ्कलय_Stmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        प्रति (Ast_ग्रन्थिः* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                portp->protect(असत्य);  // No additional exposure - already part of shown proto
                यदि (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ध्रुव portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->सङ्कलय_Stmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, असत्य));
                }
            }
        }

        यदि (v3Global.opt.profExec()) {
            cfuncp->सङ्कलय_Stmtsp(AstCStmt::prof_Exec_Section_निष्कासय(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ध्रुव netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        यदि (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_शिखरम्_Scopep->create_अस्थायी("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        प्रत्यागम dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_ग्रन्थिः_FTask* nodep, तार्किक ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_ग्रन्थिः::user2_Clear_वृक्षः();
        AstVar* rtnvarp = शून्यसूचकः;
        यदि (nodep->isFunction()) {
            AstVar* ध्रुव portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            यदि (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ध्रुव bdtypep = portp->dtypep()->basicp();
                यदि (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(सत्य)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम शून्यसूचकः;
                }
                यदि (!bdtypep->isDpiPrimitive()) {
                    यदि (bdtypep->isDpiBitVec() && portp->विस्तारः() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warn_अधिकम्()
                                       + "... Suggest make it an output argument instead?");
                    }
                    यदि (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } अन्यथा यदि (nodep->task_लौकिकम्()) {
                यदि (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warn_अधिकम्()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            यदि (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(असत्य);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, असत्य);
            rtnvarp = portp;
            rtnvarp->funcLocal(सत्य);
            rtnvarp->noCReset(सत्य);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            यदि (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(असत्य);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = शून्यसूचकः;
        यदि (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = check_Dpi_Struct_तर्काः(nodep) ? getDpiFunc(nodep, rtnvarp) : शून्यसूचकः;
            यदि (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                यदि (rtnvarp) VL_DO_DANGLING(प्रक्षिप_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम शून्यसूचकः;
            }
        }

        AstVarScope* rtnvscp = शून्यसूचकः;
        यदि (rtnvarp) {
            rtnvscp = नूतन AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->सङ्कलय_Varsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string उपसर्गः;
        यदि (nodep->dpiImport()) {
            उपसर्गः = "__Vdpiimwrap_";
        } अन्यथा यदि (nodep->dpiExport()) {
            उपसर्गः = "__Vdpiexp_";
        } अन्यथा यदि (ftaskNoInline) {
            उपसर्गः = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        यदि (!nodep->task_लौकिकम्() && !nodep->class_विधानम्()) suffix = "_" + m_scopep->nameDotless();
        ध्रुव string name = ((nodep->name() == "new") ? "new" : उपसर्गः + nodep->name() + suffix);
        AstCFunc* ध्रुव cfuncp = नूतन AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->task_लौकिकम्() && rtnvarp) ? rtnvarp->cPubArgType(सत्य, सत्य) : "")};
        // It's ok to combine imports because this is just a wrapper;
        // duplicate wrappers can get merged.
        cfuncp->dontCombine(!nodep->dpiImport());
        cfuncp->प्रविष्टिः_Point(!nodep->dpiImport());
        cfuncp->func_लौकिकम्(nodep->task_लौकिकम्());
        cfuncp->dpiContext(nodep->dpiContext());
        cfuncp->dpiExportImpl(nodep->dpiExport());
        cfuncp->dpiImportWrapper(nodep->dpiImport());
        cfuncp->recursive(nodep->recursive());
        // Hardcoded based on UVM usage; TODO make a verilated_std.vlt control for these
        cfuncp->unlikely(nodep->name() == "uvm_report_error" || nodep->name() == "uvm_report_info"
                         || nodep->name() == "uvm_report_fatal"
                         || nodep->name() == "uvm_report_warning");
        यदि (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(सत्य);
            cfuncp->isLoose(सत्य);
        } अन्यथा {
            cfuncp->isStatic(असत्य);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpi_विशुद्धम्(nodep->dpi_विशुद्धम्());
        यदि (nodep->name() == "new") cfuncp->isConstructor(सत्य);
        यदि (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(सत्य);
        यदि (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        यदि (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ध्रुव तार्किक needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->task_लौकिकम्()) || v3Global.opt.profExec());
        यदि (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        यदि (!nodep->dpiImport() && !nodep->task_लौकिकम्()) {
            // Need symbol table
            यदि (cfuncp->isConstructor()) {
                तार्किक isInterfaceClass = असत्य;
                यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                यदि (!isInterfaceClass) {
                    ध्रुव string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->सङ्कलय_Stmtsp(
                        नूतन AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        यदि (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_निवेशः_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_निवेशः_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_निवेशः_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        यदि (nodep->dpiExport()) {
            AstScopeName* ध्रुव snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(सत्य);
            snp->unlink_Fr_पृष्ठभागः();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        प्रति (Ast_ग्रन्थिः *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlink_Fr_पृष्ठभागः();
                    प्रक्षिप_Deletep(portp);
                    portp->user2p(const_Pool_सारणी(portp));
                } अन्यथा {
                    यदि (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, असत्य);
                        portp->funcLocal(सत्य);
                        यदि (portp->valuep()) प्रक्षिप_Deletep(portp->valuep()->unlink_Fr_पृष्ठभागः());
                        cfuncp->सङ्कलय_Argsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        यदि (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_ग्रन्थिः_DType* ध्रुव dtypep = portp->dtypep()->skipRefp();
                            यदि (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } अन्यथा {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(सत्य);
                    }
                    AstVarScope* ध्रुव newvscp
                        = नूतन AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->सङ्कलय_Varsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        यदि (rtnvarp) cfuncp->सङ्कलय_Argsp(rtnvarp);

        // Move body
        Ast_ग्रन्थिः* bodysp = nodep->stmtsp();
        यदि (bodysp) {
            unlinkAndClone(nodep, bodysp, सत्य);
            AstBegin* ध्रुव tempp
                = नूतन AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, असत्य};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            प्रति (Ast_ग्रन्थिः *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                    यदि (portp->isIO()) VL_DO_DANGLING(प्रक्षिप_Deletep(portp->unlink_Fr_पृष्ठभागः()), portp);
                }
            }
            यदि (tempp->stmtsp()) cfuncp->सङ्कलय_Stmtsp(tempp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            VL_DO_DANGLING(tempp->delete_वृक्षः(), tempp);
        }
        यदि (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        यदि (rtnvscp && nodep->task_लौकिकम्()) {
            cfuncp->सङ्कलय_Stmtsp(नूतन AstCReturn{
                rtnvscp->fileline(), नूतन AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        यदि (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            तार्किक writesNonLocals = असत्य;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* ध्रुव varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                यदि (varp->isFuncLocal()) प्रत्यागम;
                // Check if written
                यदि (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->समुच्चयः_Written_By_Dpi();
                    // Remember we had some
                    writesNonLocals = सत्य;
                }
                // Check if read
                यदि (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->समुच्चयः_Read_By_Dpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            यदि (writesNonLocals) {
                AstVarScope* ध्रुव dpiExportTriggerp = getDpiExporTrigger();
                File_पङ्क्तिः* ध्रुव flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                Ast_नियोजय* ध्रुव assignp
                    = नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    नूतन AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                यदि (v3Global.opt.systemC()) {
                    assignp->सङ्कलय_अग्र_अत्र(नूतन AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        यदि (nodep->needProcess()) cfuncp->समुच्चयः_Need_Process();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        प्रत्यागम cfuncp;
    }

    शून्य iterateIntoFTask(Ast_ग्रन्थिः_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    शून्य निवेशय_Before_Stmt(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->सङ्कलय_अत्र_This_As_अग्र(newp);
    }
    शून्य process_तर्काः(Ast_ग्रन्थिः_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        प्रति (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ध्रुव arrayp = VN_CAST(argp->exprp(), ConsArray);
            यदि (!arrayp) अनुवर्त;

            File_पङ्क्तिः* ध्रुव flp = arrayp->fileline();
            AstVarScope* ध्रुव substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->is_आन्तरिकम्(सत्य);

            Ast_नियोजय* ध्रुव assignp
                = नूतन Ast_नियोजय{flp, नूतन AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlink_Fr_पृष्ठभागः()};

            Ast_व्यञ्जकम्_Stmt* ध्रुव exprstmtp = नूतन Ast_व्यञ्जकम्_Stmt{
                flp, assignp, नूतन AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    तार्किक isIfaceLocalImpure(Ast_ग्रन्थिः_FTask* nodep, Ast_ग्रन्थिः* impurep) {
        ध्रुव AstScope* ध्रुव scopep = m_statep->getScope(nodep);
        यदि (!TaskStateVisitor::isIfaceFTaskScope(scopep)) प्रत्यागम असत्य;

        ध्रुव AstVarRef* ध्रुव refp = VN_CAST(impurep, VarRef);
        यदि (!refp || !refp->varScopep()) प्रत्यागम असत्य;

        प्रत्यागम refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    शून्य दर्शय(AstWith* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) {
            // Make sure that the return expression is converted only once
            प्रत्यागम;
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव withExprp = VN_AS(nodep->exprp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_व्यञ्जकम्);
        nodep->सङ्कलय_Exprp(नूतन AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    शून्य दर्शय(AstCNew* nodep) override {
        VL_RESTORER(m_अन्तर्_New);
        m_अन्तर्_New = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        यदि (m_अन्तर्_Sensesp && !nodep->is_विशुद्धम्()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(शून्यसूचकः);  // So V3Broken doesn't complain
            प्रत्यागम;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        यदि (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_गणनम्(), nodep, "No non-inline task, but no error issued");
            यदि (VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            } अन्यथा {
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
            प्रत्यागम;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = शून्यसूचकः;
        यदि (nodep->taskp()->isFunction()) {
            AstVar* ध्रुव fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            यदि (Ast_नियोजय* ध्रुव assignp = VN_CAST(nodep->backp(), नियोजय)) {
                यदि (AstVarRef* ध्रुव lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ध्रुव vscp = lhsp->varScopep();
                    यदि (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_वृक्षः(
                            fvarp->dtypep()->skipRefp())
                        && !has_Ref_तर्कः(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            यदि (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_ग्रन्थिः* beginp;
        AstCNew* cnewp = शून्यसूचकः;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ध्रुव तार्किक virtualIfaceCall
            = TaskStateVisitor::is_Virtual_Iface_विधानम्_Call(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        यदि (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            process_तर्काः(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } अन्यथा {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        यदि (VN_IS(nodep, New)) {  // New not legal as while() condition
            निवेशय_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा यदि (VN_IS(nodep->backp(), ग्रन्थिः_नियोजय)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            निवेशय_Before_Stmt(nodep, beginp);
            AstVarRef* ध्रुव outrefp = नूतन AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा यदि (!VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ध्रुव outrefp = नूतन AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_व्यञ्जकम्_Stmt* lambdap = नूतन Ast_व्यञ्जकम्_Stmt{nodep->fileline(), beginp, outrefp};

            यदि (m_अन्तर्_New) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_वृक्षः(असत्य);
                newvarp->funcLocal(सत्य);

                // Create a new VarScope for the new variable
                AstVarScope* ध्रुव newvscp
                    = नूतन AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->सङ्कलय_Varsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreach_And_अग्र([outvscp, newvscp](AstVarRef* refp) {
                    यदि (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->has_परिणाम(असत्य);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ध्रुव exprp = नूतन AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtype_समुच्चयः_String();
                lambdap->सङ्कलय_Stmtsp(नूतन AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            VIsCached::clear_Cache_वृक्षः();
        } अन्यथा {  // VN_IS(nodep->backp(), StmtExpr)
            निवेशय_Before_Stmt(nodep, beginp);
            यदि (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_संहिता::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* ध्रुव nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        यदि (!nodep->user1_समुच्चयः_Once()) {  // Just one creation needed per function
            // Expand functions in it
            पूर्णाङ्क modes = 0;
            यदि (nodep->dpiImport()) modes++;
            यदि (nodep->dpiExport()) modes++;
            यदि (nodep->task_लौकिकम्()) modes++;
            यदि (nodep->class_विधानम्()) modes++;
            यदि (v3Global.opt.protectIds() && nodep->task_लौकिकम्()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            यदि (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            ध्रुव तार्किक noInline = m_statep->ftaskNoInline(nodep);
            ध्रुव तार्किक needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            यदि (noInline && !nodep->class_विधानम्() && !nodep->recursive()) {
                यदि (Ast_ग्रन्थिः* ध्रुव impurep = m_statep->checkImpure(nodep)) {
                    यदि (!isIfaceLocalImpure(nodep, impurep)) {
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

            यदि (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->task_लौकिकम्()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_ग्रन्थिः_FTask* ध्रुव clonedFuncp = nodep->clone_वृक्षः(असत्य);
                यदि (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ध्रुव तार्किक makeNonInlineFunc = noInline || needsNonInlineCFunc;
                यदि (AstCFunc* ध्रुव cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->सङ्कलय_अग्र_अत्र(cfuncp);
                    यदि (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            यदि (nodep->isFunction()) {
                यदि (AstVar* ध्रुव portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* ध्रुव vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(vscp->unlink_Fr_पृष्ठभागः()), vscp);
                }
            }
            प्रति (Ast_ग्रन्थिः *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ध्रुव vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(vscp->unlink_Fr_पृष्ठभागः()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य दर्शय(Ast_नियोजय_Force* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::सदिशम्<Ast_ग्रन्थिः_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_ग्रन्थिः_FTask_Ref* refp) { refs.प्रक्षिप_पृष्ठभागः(refp); });
        प्रति (Ast_ग्रन्थिः_FTask_Ref* ध्रुव refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ध्रुव interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{
                nodep->fileline(), नूतन AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य)};
            Ast_Sen_वृक्षः* ध्रुव senTreep = नूतन Ast_Sen_वृक्षः{
                nodep->fileline(), नूतन AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            Ast_सक्रिय* ध्रुव activep
                = नूतन Ast_सक्रिय{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_वृक्षः_Storep(activep->sentreep());
            activep->सङ्कलय_Stmtsp(
                नूतन AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, शून्यसूचकः, assignp});
            m_scopep->सङ्कलय_Blocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(नूतन AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        यदि (!nodep->exprp()) VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_अन्तर्_Sensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_अन्तर्_Sensesp);
        m_अन्तर्_Sensesp = सत्य;
        iterateChildren(nodep);
    }
    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    TaskVisitor(AstNetlist* nodep, TaskStateVisitor* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~TaskVisitor() {
        V3Stats::सङ्कलय_Stat("Optimizations, Functions inlined", m_statInlines);
        V3Stats::सङ्कलय_Stat("Optimizations, Hierarchical DPI wrappers with costs",
                         m_statHierDpisWithCosts);
    }
};

//######################################################################
// Mark interface members under timing controls of interface CFuncs as interface-sensed

वर्ग TaskIfaceSensVisitor final : सार्वजनिक VNVisitorConst {
    // STATE
    तार्किक m_underIfaceFunc = असत्य;  // Under a CFunc owned by an interface scope
    तार्किक m_underSenses = असत्य;  // Under a sensitivity expression of such a CFunc

    // METHODS
    शून्य markSensesAndIterate(Ast_ग्रन्थिः* nodep) {
        यदि (!m_underIfaceFunc) प्रत्यागम;
        VL_RESTORER(m_underSenses);
        m_underSenses = सत्य;
        iterate_And_अग्र_Const_Null(nodep);
    }
    // VISITORS
    शून्य दर्शय(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_Sen_वृक्षः* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    शून्य दर्शय(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (!m_underSenses) प्रत्यागम;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        यदि (nodep->varp()->isFuncLocal()) प्रत्यागम;
        यदि (AstIface* ध्रुव ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = स्वभाव;
};

//######################################################################
// Task class functions

ध्रुव वर्ण* ध्रुव V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_ग्रन्थिः_FTask_Ref* nodep, Ast_ग्रन्थिः* taskStmtsp,
                                    V3TaskConnectState* statep, तार्किक makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ध्रुव std::string, पूर्णाङ्क> name_गन्तव्यम्_सूचकांक;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    पूर्णाङ्क tpinnum = 0;
    AstVar* sformatp = शून्यसूचकः;
    प्रति (Ast_ग्रन्थिः* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
            यदि (portp->isIO()) {
                tconnects.emplace_पृष्ठभागः(portp, static_cast<AstArg*>(शून्यसूचकः));
                name_गन्तव्यम्_सूचकांक.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                यदि (portp->attrSFormat()) {
                    sformatp = portp;
                } अन्यथा यदि (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    पूर्णाङ्क ppinnum = 0;
    तार्किक reorganize = असत्य;
    प्रति (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        यदि (!argp->name().रिक्तानि()) {
            // By name
            ध्रुव स्वचालित it = name_गन्तव्यम्_सूचकांक.find(argp->name());
            यदि (it == name_गन्तव्यम्_सूचकांक.अन्तः_2()) {
                यदि (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warn_अधिकम्() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), argp);
                }
            } अन्यथा {
                यदि (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
                }
                tconnects[it->second].second = argp;
                reorganize = सत्य;
            }
        } अन्यथा {  // By pin number
            यदि (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } अन्यथा यदि (ppinnum >= tpinnum) {
                यदि (sformatp) {
                    tconnects.emplace_पृष्ठभागः(sformatp, static_cast<AstArg*>(शून्यसूचकः));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } अन्यथा यदि (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warn_अधिकम्() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), argp);
                }
            } अन्यथा {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    यदि (!makeChanges) प्रत्यागम tconnects;

    // Connect missing ones
    VInsertion_समुच्चयः<ध्रुव AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    प्रति (पूर्णाङ्क i = 0; i < tpinnum; ++i) {
        AstVar* ध्रुव portp = tconnects[i].first;
        यदि (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* newvaluep = शून्यसूचकः;
            यदि (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = नूतन AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } अन्यथा यदि (AstFuncRef* ध्रुव funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ध्रुव Ast_ग्रन्थिः_FTask* ध्रुव funcp = funcRefp->taskp();
                यदि (funcp->class_विधानम्() && funcp->isStatic()) newvaluep = funcRefp;
            } अन्यथा यदि (AstConst* ध्रुव constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            यदि (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), ग्रन्थिः_व्यञ्जकम्));
                यदि (!VN_IS(newvaluep, Const)) {
                    यदि (statep) {
                        portp->pin_अङ्कः(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.निवेशय(portp);
                    } अन्यथा {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = नूतन AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_वृक्षः(सत्य) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ध्रुव newp = नूतन AstArg{nodep->fileline(), portp->name(), newvaluep};
            यदि (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(),
                            tconnects[i].second = शून्यसूचकः);
            }
            tconnects[i].second = newp;
            reorganize = सत्य;
        }
        यदि (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } अन्यथा {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
        AstArg* ध्रुव argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    यदि (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        यावत् (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlink_Fr_पृष्ठभागः();
        }
        प्रति (पूर्णाङ्क i = 0; i < tpinnum; ++i) {
            AstArg* ध्रुव argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->सङ्कलय_Argsp(argp);
        }
    }

    यदि (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_वृक्षः("-  ftref-out: ");
        प्रति (पूर्णाङ्क i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvt_गन्तव्यम्_Hex(tconnects[i].first)
                               << "  conn=" << cvt_गन्तव्यम्_Hex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    यदि (!argWrap.रिक्तानि()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        प्रत्यागम taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            शून्यसूचकः);
    }
    प्रत्यागम tconnects;
}

शून्य V3Task::taskConnectWrap(Ast_ग्रन्थिः_FTask_Ref* nodep, ध्रुव V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ध्रुव VInsertion_समुच्चयः<ध्रुव AstVar*>& argWrap) {
    statep->समुच्चयः_Did_Wrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    प्रति (ध्रुव AstVar* varp : argWrap) newname += "_" + cvt_गन्तव्यम्_Str(varp->pin_अङ्कः());
    ध्रुव स्वचालित pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(शून्यसूचकः));
    यदि (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_ग्रन्थिः_FTask* ध्रुव newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
        ध्रुव AstVar* ध्रुव portp = tconnect.first;
        AstArg* ध्रुव argp = tconnect.second;
        यदि (argWrap.exists(portp)) {  // Removed arg
            statep->प्रक्षिप_Deletep(argp->unlink_Fr_पृष्ठभागः());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_ग्रन्थिः_FTask* V3Task::taskConnectWrapNew(Ast_ग्रन्थिः_FTask* taskp, ध्रुव string& newname,
                                         ध्रुव V3TaskConnects& tconnects,
                                         ध्रुव VInsertion_समुच्चयः<ध्रुव AstVar*>& argWrap) {
    std::map<ध्रुव AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_ग्रन्थिः_FTask* ध्रुव newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->सङ्कलय_अग्र_अत्र(newTaskp);

    Ast_ग्रन्थिः_FTask_Ref* newCallp = शून्यसूचकः;
    Ast_ग्रन्थिः* newCallInsertp = शून्यसूचकः;
    यदि (VN_IS(taskp, Func)) {
        AstVar* ध्रुव fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ध्रुव newFVarp = fvarp->clone_वृक्षः(सत्य);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = नूतन AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = नूतन Ast_नियोजय{taskp->fileline(),
                            नूतन AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } अन्यथा यदि (VN_IS(taskp, Task)) {
        newCallp = नूतन AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = नूतन Ast_Stmt_व्यञ्जकम्{taskp->fileline(), newCallp};
    } अन्यथा {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
        AstVar* ध्रुव portp = tconnect.first;
        AstVar* newPortp;
        यदि (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = नूतन AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(सत्य);
            यदि (newPortp->valuep()) newPortp->valuep()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
            newTaskp->सङ्कलय_Stmtsp(newPortp);
        } अन्यथा {  // Defaulting arg
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = VN_AS(portp->valuep(), ग्रन्थिः_व्यञ्जकम्);
            यदि ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ध्रुव VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ध्रुव refp = VN_AS(valuep->clone_वृक्षः(असत्य), VarRef);
                refp->access(refAccess);
                AstArg* ध्रुव newArgp = नूतन AstArg{portp->fileline(), portp->name(), refp};
                newCallp->सङ्कलय_Argsp(newArgp);
                अनुवर्त;
            }
            // Create local temporary
            newPortp = नूतन AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(सत्य);
            newTaskp->सङ्कलय_Stmtsp(newPortp);
            // Runtime-assign it to the default
            यदि (!VN_IS(valuep, रिक्तानि_Queue)) {
                Ast_नियोजय* ध्रुव newAssignp
                    = नूतन Ast_नियोजय{valuep->fileline(),
                                    नूतन AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_वृक्षः(सत्य)};
                newTaskp->सङ्कलय_Stmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ध्रुव newArgp = नूतन AstArg{
            portp->fileline(), portp->name(),
            नूतन AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->सङ्कलय_Argsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->सङ्कलय_Stmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ध्रुव स्वचालित it = oldNewVars.find(refp->varp());
        यदि (it != oldNewVars.अन्तः_2()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    प्रत्यागम newTaskp;
}

string V3Task::नियोजय_आन्तरिकम्_गन्तव्यम्_Dpi(AstVar* portp, तार्किक isPtr, ध्रुव string& frSuffix,
                                   ध्रुव string& गन्तव्यम्_Suffix, ध्रुव string& fr_उपसर्गः) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ध्रुव string frName = fr_उपसर्गः + portp->name() + frSuffix;
    ध्रुव string गन्तव्यम्_Name = portp->name() + गन्तव्यम्_Suffix;
    ध्रुव string idx = portp->name() + "__Vidx";
    आकार_t unpack_आकार = 1;  // non-unpacked array is treated as size 1
    पूर्णाङ्क unpackDim = 0;
    यदि (AstUnpackArrayDType* ध्रुव unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpack_आकार = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(असत्य).second;
        यदि (unpackDim > 0) UASSERT_OBJ(unpack_आकार > 0, portp, "size must be greater than 0");
    }
    यदि (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        ध्रुव तार्किक isBit = portp->basicp()->isDpiBitVec();
        ध्रुव तार्किक needsFor = unpack_आकार > 1;
        यदि (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvt_गन्तव्यम्_Str(unpack_आकार) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvt_गन्तव्यम्_Str(portp->विस्तारः()) + ", ";
        stmt += गन्तव्यम्_Name;
        यदि (needsFor) {
            stmt += " + " + cvt_गन्तव्यम्_Str(portp->dtypep()->skipRefp()->विस्तारः_शब्दाः()) + " * " + idx;
        }
        stmt += ", ";
        यदि (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            प्रति (पूर्णाङ्क i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } अन्यथा {
            stmt += frName + ")";
        }
    } अन्यथा {
        ध्रुव तार्किक isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        ध्रुव तार्किक isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        ध्रुव string unpackAt = unpack_आकार > 1 ? "[" + idx + "]" : "[0]";
        यदि (unpackDim > 0) {
            ध्रुव string forStmt = unpack_आकार > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvt_गन्तव्यम्_Str(unpack_आकार) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + गन्तव्यम्_Name + unpackAt;
        } अन्यथा {
            यदि (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += गन्तव्यम्_Name;
        }
        stmt += " = ";
        यदि (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        यदि (unpackDim > 0) {
            stmt += "(&" + frName;
            प्रति (पूर्णाङ्क i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } अन्यथा {
            stmt += frName;
        }
        यदि (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    प्रत्यागम stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::नियोजय_Dpi_गन्तव्यम्_आन्तरिकम्(ध्रुव std::string& lhsName, AstVar* varp) {
    std::string cvt;
    तार्किक useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_गन्तव्यम्_आन्तरिकम्_Cvt_Stmt(varp->dtypep());
    ध्रुव std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ध्रुव पूर्णाङ्क योग = strides.रिक्तानि() ? 1 : strides[0].first * strides[0].second;
    ध्रुव पूर्णाङ्क विस्तारः_शब्दाः = varp->basicp()->विस्तारः_शब्दाः();
    string statements;
    प्रति (पूर्णाङ्क i = 0; i < योग; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        प्रति (ध्रुव स्वचालित& stride : strides) {
            lhs += "[" + std::गन्तव्यम्_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        यदि (useSvVec) {
            ध्रुव std::string विस्थापनम् = std::गन्तव्यम्_string(i * विस्तारः_शब्दाः);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + विस्थापनम् + ");\n";
        } अन्यथा {
            ध्रुव std::string elem = strides.रिक्तानि() ? "" : "[" + std::गन्तव्यम्_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    प्रत्यागम statements;
}

शून्य V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ध्रुव TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("task", 0, dump_वृक्षः_Either_Level() >= 3);
}
