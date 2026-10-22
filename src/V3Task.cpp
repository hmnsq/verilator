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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Task.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3LanguageWords.h"
#ଅନ୍ତର୍ଭାଵ "V3SharedTmps.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

ଶ୍ରେଣୀ TaskBaseVertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_ଗଣ୍ଠି* m_impurep = nullptr;  // Node causing impure function w/ outside references
    ତାର୍କିକ m_noInline = ମିଥ୍ୟା;  // Marked with pragma
ସର୍ବସାଧାରଣ:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ତାର୍କିକ pure() ଅଚଳ { ଫେରନ୍ତୁ m_impurep == nullptr; }
    Ast_ଗଣ୍ଠି* impure_ଗଣ୍ଠି() ଅଚଳ { ଫେରନ୍ତୁ m_impurep; }
    ଶୂନ୍ୟ impure(Ast_ଗଣ୍ଠି* nodep) { m_impurep = nodep; }
    ତାର୍କିକ noInline() ଅଚଳ { ଫେରନ୍ତୁ m_noInline; }
    ଶୂନ୍ୟ noInline(ତାର୍କିକ flag) { m_noInline = flag; }
};

ଶ୍ରେଣୀ TaskFTaskVertex final : ସର୍ବସାଧାରଣ TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_ଗଣ୍ଠି_FTask* ଅଚଳ m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    ତାର୍କିକ m_needsNonInlineCFunc = ମିଥ୍ୟା;  // Needs CFunc for selected non-inlined call sites

ସର୍ବସାଧାରଣ:
    TaskFTaskVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    Ast_ଗଣ୍ଠି_FTask* nodep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_nodep; }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nodep()->name(); }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ pure() ? "black" : "red"; }
    AstCFunc* cFuncp() ଅଚଳ { ଫେରନ୍ତୁ m_cFuncp; }
    ଶୂନ୍ୟ cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    ତାର୍କିକ needsNonInlineCFunc() ଅଚଳ { ଫେରନ୍ତୁ m_needsNonInlineCFunc; }
    ଶୂନ୍ୟ needsNonInlineCFunc(ତାର୍କିକ flag) { m_needsNonInlineCFunc = flag; }
};

ଶ୍ରେଣୀ TaskCodeVertex final : ସର୍ବସାଧାରଣ TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
ସର୍ବସାଧାରଣ:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ "*CODE*"; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "green"; }
};

ଶ୍ରେଣୀ TaskEdge final : ସର୍ବସାଧାରଣ V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
ସର୍ବସାଧାରଣ:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* ଶୀର୍ଷ)
        : V3GraphEdge{graphp, fromp, ଶୀର୍ଷ, 1, ମିଥ୍ୟା} {}
    ~TaskEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string dotLabel() ଅଚଳ override { ଫେରନ୍ତୁ "w" + cvtToStr(weight()); }
};

//######################################################################

ଶ୍ରେଣୀ TaskStateVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ଅଚଳ VNUser3InUse m_inuser3;
    ଅଚଳ VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_ଗଣ୍ଠି_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    ତାର୍କିକ m_underPortVar = ମିଥ୍ୟା;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

ସର୍ବସାଧାରଣ:
    // METHODS
    ସ୍ଥିର ତାର୍କିକ isVirtualIfaceMethodCall(ଅଚଳ Ast_ଗଣ୍ଠି_FTask_Ref* refp) {
        ଅଚଳ AstMethodCall* ଅଚଳ mrefp = VN_CAST(refp, MethodCall);
        ଯଦି (!mrefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = mrefp->fromp();
        ଯଦି (!fromp || !fromp->dtypep()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstIfaceRefDType* ଅଚଳ ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        ଫେରନ୍ତୁ ifrefp && ifrefp->isVirtual();
    }
    ସ୍ଥିର ତାର୍କିକ isIfaceFTaskScope(ଅଚଳ AstScope* scopep) {
        ଫେରନ୍ତୁ scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_ଗଣ୍ଠି_FTask* nodep) {
        AstScope* ଅଚଳ scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        ଫେରନ୍ତୁ scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        ଅଚଳ auto iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        ଫେରନ୍ତୁ iter->second;
    }
    AstClass* getClassp(Ast_ଗଣ୍ଠି_FTask* nodep) {
        AstClass* ଅଚଳ classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        ଫେରନ୍ତୁ classp;
    }
    ଶୂନ୍ୟ remapFuncClassp(Ast_ଗଣ୍ଠି_FTask* nodep, Ast_ଗଣ୍ଠି_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    ତାର୍କିକ ftaskNoInline(Ast_ଗଣ୍ଠି_FTask* nodep) { ଫେରନ୍ତୁ getFTaskVertex(nodep)->noInline(); }
    ତାର୍କିକ ftaskNeedsNonInlineCFunc(Ast_ଗଣ୍ଠି_FTask* nodep) {
        ଫେରନ୍ତୁ getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_ଗଣ୍ଠି_FTask* nodep) { ଫେରନ୍ତୁ getFTaskVertex(nodep)->cFuncp(); }
    ଶୂନ୍ୟ ftaskCFuncp(Ast_ଗଣ୍ଠି_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_ଗଣ୍ଠି* checkImpure(Ast_ଗଣ୍ଠି_FTask* nodep) { ଫେରନ୍ତୁ getFTaskVertex(nodep)->impure_ଗଣ୍ଠି(); }

ବ୍ୟକ୍ତିଗତ:
    TaskFTaskVertex* getFTaskVertex(Ast_ଗଣ୍ଠି_FTask* nodep) {
        ଯଦି (!nodep->user4p()) nodep->user4p(ନୂତନ TaskFTaskVertex{&m_callGraph, nodep});
        ଫେରନ୍ତୁ static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    ଶୂନ୍ୟ propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        ପାଇଁ (V3GraphVertex& vtx : m_callGraph.vertices()) {
            ଯଦି (TaskFTaskVertex* ଅଚଳ ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                ଯଦି (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        ଯେତେବେଳେ (!workList.empty()) {
            TaskFTaskVertex* ଅଚଳ calleep = workList.back();
            workList.କାଢ଼_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            ପାଇଁ (V3GraphEdge& edge : calleep->inEdges()) {
                ଯଦି (TaskFTaskVertex* ଅଚଳ callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    ଯଦି (!callerp->pure()) ଜାରି;  // Already processed or on workList
                    callerp->impure(calleep->impure_ଗଣ୍ଠି());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    ଶୂନ୍ୟ decideInlining() {
        ପାଇଁ (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* ଅଚଳ ftVtxp = vtx.cast<TaskFTaskVertex>();
            ଯଦି (!ftVtxp) ଜାରି;
            // Move on if already decided not to inline
            ଯଦି (ftVtxp->noInline()) ଜାରି;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            ଯଦି (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(ସତ୍ୟ);
                ଜାରି;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            ଯଦି (v3Global.opt.fInlineFuncsEager()) ଜାରି;

            Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            ଯଦି (!ftaskp->verilogFunction()) ଜାରି;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            ଯଦି (ftaskp->taskPublic()) ଜାରି;
            // If the function is pure, it's OK not to inline, so don't do it
            ଯଦି (ftVtxp->pure()) ftVtxp->noInline(ସତ୍ୟ);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVarScope* ଅଚଳ vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ଅଚଳ varp = vscp->varp();
                ଯଦି (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ taskp = VN_CAST(stmtp, ଗଣ୍ଠି_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* ଅଚଳ taskVtxp = getFTaskVertex(nodep->taskp());
        ନୂତନ TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        ଯଦି (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(ସତ୍ୟ);
        // Do we have to disable inlining the function?
        ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        ଯଦି (!taskVtxp->noInline()) {  // Else short-circuit below
            ପାଇଁ (ଅଚଳ auto& itr : tconnects) {
                ଅଚଳ AstVar* ଅଚଳ portp = itr.first;
                ଅଚଳ AstArg* ଅଚଳ argp = itr.second;
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ pinp = argp->exprp()) {
                    ଯଦି ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(ସତ୍ୟ);
                    }
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        ଯଦି (nodep->dpiImport()) m_curVxp->noInline(ସତ୍ୟ);
        ଯଦି (nodep->classMethod()) m_curVxp->noInline(ସତ୍ୟ);  // Until V3Task supports it
        ଯଦି (nodep->recursive()) m_curVxp->noInline(ସତ୍ୟ);
        ଯଦି (nodep->isConstructor()) {
            m_curVxp->noInline(ସତ୍ୟ);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstPragma* nodep) override {
        ଯଦି (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        ଯଦି (nodep->isIO()) m_underPortVar = ସତ୍ୟ;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ଯଦି (m_underPortVar) ଫେରନ୍ତୁ;
        AstVar* ଅଚଳ varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        ଯଦି (nodep->access().isReadOnly() && varp->constPoolEntry()) ଫେରନ୍ତୁ;
        ଯଦି (varp->user4u().toGraphVertex() != m_curVxp) {
            ଯଦି (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        // Move initial statements into the constructor
        VL_RESTORER_CLEAR(m_initialps);
        VL_RESTORER(m_ctorp);
        VL_RESTORER(m_classp);
        m_ctorp = nullptr;
        m_classp = nodep;
        {  // Find m_initialps, m_ctor
            iterateChildren(nodep);
        }
        UASSERT_OBJ(m_ctorp, nodep, "class constructor missing");  // LinkDot always makes it
        Ast_ଗଣ୍ଠି* insertp = nullptr;
        ପାଇଁ (AstInitialAutomatic* initialp : m_initialps) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ movep = initialp->stmtsp()) {
                movep->unlinkFrBackWithNext();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_ଗଣ୍ଠି::addNextNull(insertp, movep);
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    ଶୂନ୍ୟ visit(AstInitialAutomatic* nodep) override {
        m_initialps.ଠେଲ_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = ନୂତନ TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.removeRedundantEdgesSum(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        ଯଦି (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

ସଂରଚନା TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    ସ୍ଥିର std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> unpackDimsAndStrides(AstVar* varp) {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> dimStrides;
        AstUnpackArrayDType* ଅଚଳ unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ଯଦି (!unpackp) ଫେରନ୍ତୁ dimStrides;

        ଅଚଳ std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ଅଚଳ size_t nDims = dims.size();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        ପାଇଁ (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        ଫେରନ୍ତୁ dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    ସ୍ଥିର std::pair<std::string, ତାର୍କିକ> dpiToInternalCvtStmt(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଅଚଳ AstBasicDType* ଅଚଳ basicp = dtypep->basicp();

        // DPI types using Primitive C types
        ଯଦି (basicp->keyword() == VBasicDTypeKwd::CHANDLE) ଫେରନ୍ତୁ {"VL_CVT_VP_Q(", ମିଥ୍ୟା};
        ଯଦି (basicp->keyword() == VBasicDTypeKwd::STRING) ଫେରନ୍ତୁ {"VL_CVT_N_CSTR(", ମିଥ୍ୟା};
        ଯଦି (basicp->isDpiPrimitive()) ଫେରନ୍ତୁ {"(", ମିଥ୍ୟା};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ଅଚଳ std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ଅଚଳ ଅକ୍ଷର sizeChar = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ଅଚଳ std::string& size = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        ଫେରନ୍ତୁ {"VL_SET_"s + sizeChar + "_" + vecType + "(" + size + ", ", ସତ୍ୟ};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

ଶ୍ରେଣୀ TaskVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<ଅଚଳ string, std::tuple<Ast_ଗଣ୍ଠି_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ଅଚଳ m_statep;  // Common state between visitors
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    Ast_ଶୀର୍ଷ_Scope* ଅଚଳ m_ଶୀର୍ଷ_Scopep = v3Global.rootp()->ଶୀର୍ଷ_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_ଗଣ୍ଠି* m_insStmtp = nullptr;  // Where to insert statement
    ତାର୍କିକ m_inSensesp = ମିଥ୍ୟା;  // Are we under a senitem?
    ତାର୍କିକ m_inNew = ମିଥ୍ୟା;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_uncon_Var_ଅଙ୍କ = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, ଅଚଳ AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ଅଚଳ string& name, AstVar* examplep) {
        AstVar* ଅଚଳ newvarp = ନୂତନ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(ସତ୍ୟ);
        funcp->addVarsp(newvarp);
        AstVarScope* ଅଚଳ newvscp = ନୂତନ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        ଫେରନ୍ତୁ newvscp;
    }
    AstVarScope* create_ନିବେଶ_Var(AstCFunc* funcp, ଅଚଳ string& name, VBasicDTypeKwd kwd) {
        AstVar* ଅଚଳ newvarp
            = ନୂତନ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(ସତ୍ୟ);
        newvarp->direction(VDirection::ନିବେଶ);
        funcp->addArgsp(newvarp);
        AstVarScope* ଅଚଳ newvscp = ନୂତନ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        ଫେରନ୍ତୁ newvscp;
    }
    ସ୍ଥିର AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ଅଚଳ refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ଅଚଳ vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_ବୃକ୍ଷ(), refp);
        ଫେରନ୍ତୁ vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ଅଚଳ Ast_ଗଣ୍ଠି_FTask_Ref* refp, AstVar* invarp,
                                ଅଚଳ std::string& suffix) {
        ଯଦି (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            ଫେରନ୍ତୁ constPoolTable(invarp);
        } ଅନ୍ୟଥା {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* ଅଚଳ newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* ଅଚଳ newvarp = newvscp->varp();
            newvarp->funcLocal(ମିଥ୍ୟା);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(ସତ୍ୟ);
            ଫେରନ୍ତୁ newvscp;
        }
    }

    // Replace varrefs with new var pointer
    ଶୂନ୍ୟ relink(Ast_ଗଣ୍ଠି* nodep) {
        nodep->foreachAndNext([](AstVarRef* refp) {
            ଯଦି (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ଅଚଳ newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pinp, AstVarScope* newvscp, ତାର୍କିକ pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* inPinp = pinp;
        ଯଦି (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* inPinClonep
            = pureCheck ? inPinp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ) : inPinp->clone_ବୃକ୍ଷ(ସତ୍ୟ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        ଯଦି (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = ନୂତନ AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* ଅଚଳ assp = ନୂତନ AstAssign{
            pinp->fileline(), ନୂତନ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, ସତ୍ୟ);  // Ok if in <= block
        ଫେରନ୍ତୁ assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pinp, AstVarScope* newvscp,
                                        ତାର୍କିକ pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* outPinp = pinp;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* postRhsp = ନୂତନ AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        ଯଦି (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            ଯଦି (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, ଗଣ୍ଠି_Uniop)->lhsp();
                ଯଦି (postRhsp->isSigned()) {
                    postRhsp = ନୂତନ AstISToRD{pinp->fileline(), postRhsp};
                } ଅନ୍ୟଥା {
                    postRhsp = ନୂତନ AstIToRD{pinp->fileline(), postRhsp};
                }
            } ଅନ୍ୟଥା {
                ଯଦି (Ast_ଗଣ୍ଠି_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } ଅନ୍ୟଥା ଯଦି (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } ଅନ୍ୟଥା {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                ଯଦି (outPinp->width() < portp->width()) {
                    postRhsp = ନୂତନ AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } ଅନ୍ୟଥା {  // pin width > port width
                    ଯଦି (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = ନୂତନ AstExtendS{pinp->fileline(), postRhsp};
                    } ଅନ୍ୟଥା {
                        postRhsp = ନୂତନ AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        ଯଦି (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, ଗଣ୍ଠି_Uniop)->lhsp();
            postRhsp = ନୂତନ AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ outPinClonep
            = pureCheck ? outPinp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ) : outPinp->clone_ବୃକ୍ଷ(ସତ୍ୟ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        ଯଦି (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = ନୂତନ AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* ଅଚଳ assp = ନୂତନ AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, ସତ୍ୟ);  // Ok if in <= block
        ଫେରନ୍ତୁ assp;
    }

    ଶୂନ୍ୟ changeAtWriteRecurse(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp) {
        // Change nested at methods to writable variant
        ଯଦି (AstCMethodHard* ଅଚଳ cMethodp = VN_CAST(exprp, CMethodHard)) {
            ଯଦି (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    ଶୂନ୍ୟ connectPort(ଅଚଳ Ast_ଗଣ୍ଠି_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_ଗଣ୍ଠି* beginp,
                     ତାର୍କିକ inlineTask) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pinp = argp->exprp();
        ଯଦି (inlineTask) {
            portp->unlinkFrBack();
            ଠେଲ_Deletep(portp);  // Remove it from the clone (not original)
        }
        ଯଦି (!pinp) {
            // Too few arguments in function call
        } ଅନ୍ୟଥା {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            ଯଦି (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* ଅଚଳ varp = ନୂତନ AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_ଅଙ୍କ++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* ଅଚଳ newvscp = ନୂତନ AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* ଅଚଳ repp = ନୂତନ AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(ଠେଲ_Deletep(pinp), pinp);
                pinp = repp;
            }
            ଯଦି (inlineTask) {
                ଠେଲ_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_ବୃକ୍ଷ(), argp);  // Args no longer needed
            }
            ଯଦି (portp->isRef() || portp->isConstRef()) {
                ତାର୍କିକ refArgOk = ମିଥ୍ୟା;
                ଯଦି (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (AstCMethodHard* ଅଚଳ cMethodp = VN_CAST(pinp, CMethodHard)) {
                    ଯଦି (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } ଅନ୍ୟଥା {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                ଯଦି (refArgOk) {
                    ଯଦି (AstVarRef* ଅଚଳ varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } ଅନ୍ୟଥା {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                ଯଦି (inlineTask) {
                    ଯଦି (AstVarRef* ଅଚଳ varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* ଅଚଳ localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } ଅନ୍ୟଥା {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } ଅନ୍ୟଥା ଯଦି (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* ଅଚଳ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ଯଦି (!inlineTask) {
                    pinp->replaceWith(
                        ନୂତନ AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    ଠେଲ_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* ଅଚଳ preassp = connectPortMakeInAssign(pinp, newvscp, ସତ୍ୟ);
                beginp->addNextHere(preassp);

                AstAssign* ଅଚଳ postassp = connectPortMakeOutAssign(portp, pinp, newvscp, ସତ୍ୟ);
                beginp->addNext(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } ଅନ୍ୟଥା ଯଦି (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ଅଚଳ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ଯଦି (!inlineTask) {
                    pinp->replaceWith(ନୂତନ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    ଠେଲ_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* ଅଚଳ postassp = connectPortMakeOutAssign(portp, pinp, newvscp, ମିଥ୍ୟା);
                // Put assignment BEHIND of all other statements
                beginp->addNext(postassp);
            } ଅନ୍ୟଥା ଯଦି (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ଅଚଳ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* ଅଚଳ preassp = connectPortMakeInAssign(pinp, newvscp, ମିଥ୍ୟା);
                // Put assignment in FRONT of all other statements
                beginp->addNextHere(preassp);
            }
        }
    }

    ତାର୍କିକ hasRefArgument(Ast_ଗଣ୍ଠି_FTask* nodep) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (varp->isRef() || varp->isConstRef()) ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    Ast_ଗଣ୍ଠି* createInlinedFTask(Ast_ଗଣ୍ଠି_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_ଗଣ୍ଠି* ଅଚଳ newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_ବୃକ୍ଷ(ସତ୍ୟ) : nullptr;
        Ast_ଗଣ୍ଠି* ଅଚଳ beginp
            = ନୂତନ AstComment{refp->fileline(), "Function: "s + refp->name(), ସତ୍ୟ};
        ଯଦି (newbodysp) beginp->addNext(newbodysp);
        ଯଦି (debug() >= 9) beginp->dump_ବୃକ୍ଷ_And_Next(cout, "-  newbegi: ");
        //
        // Create input variables
        Ast_ଗଣ୍ଠି::user2_Clear_ବୃକ୍ଷ();
        {
            ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            ପାଇଁ (ଅଚଳ auto& itr : tconnects) {
                AstVar* ଅଚଳ portp = itr.first;
                AstArg* ଅଚଳ argp = itr.second;
                connectPort(refp, portp, argp, beginp, ସତ୍ୟ);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_ଗଣ୍ଠି* nextstmtp;
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    ଯଦି (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* ଅଚଳ localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        ଯଦି (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_ଗଣ୍ଠି* ଅଚଳ crstp = ନୂତନ AstAssign{
                                portp->fileline(),
                                ନୂତନ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                ନୂତନ AstCReset{portp->fileline(), portp, ମିଥ୍ୟା}};
                            portp->replaceWith(crstp);
                        } ଅନ୍ୟଥା {
                            portp->unlinkFrBack();
                        }
                        ଠେଲ_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        ଯଦି (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        ଯଦି (debug() >= 9) beginp->dump_ବୃକ୍ଷ_And_Next(cout, "-  iotask: ");
        ଫେରନ୍ତୁ beginp;
    }

    Ast_ଗଣ୍ଠି* createNonInlinedFTask(Ast_ଗଣ୍ଠି_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ଅଚଳ cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_ଗଣ୍ଠି* ଅଚଳ beginp
            = ନୂତନ AstComment{refp->fileline(), "Function: "s + refp->name(), ସତ୍ୟ};
        Ast_ଗଣ୍ଠି_CCall* ccallp;
        ଯଦି (VN_IS(refp, New)) {
            AstCNew* ଅଚଳ cnewp = ନୂତନ AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMethodCall* ଅଚଳ mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = ନୂତନ AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        } ଅନ୍ୟଥା {
            ccallp = ନୂତନ AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        }

        ଯଦି (ଅଚଳ AstFuncRef* ଅଚଳ funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstTaskRef* ଅଚଳ taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMethodCall* ଅଚଳ methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            ପାଇଁ (ଅଚଳ auto& itr : tconnects) {
                AstVar* ଅଚଳ portp = itr.first;
                AstArg* ଅଚଳ argp = itr.second;
                connectPort(refp, portp, argp, beginp, ମିଥ୍ୟା);
            }
        }
        // First argument is symbol table, then output if a function
        ଅଚଳ ତାର୍କିକ needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        ଯଦି (needSyms) ccallp->argTypes("vlSymsp");

        ଯଦି (refp->taskp()->dpiContext()) {
            AstScopeName* ଅଚଳ snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_ଧାଡ଼ି* ଅଚଳ flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                ନୂତନ AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(ନୂତନ AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        ପାଇଁ (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        ଯଦି (outvscp) ccallp->addArgsp(ନୂତନ AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        ଯଦି (debug() >= 9) beginp->dump_ବୃକ୍ଷ_And_Next(cout, "-  nitask: ");
        ଫେରନ୍ତୁ beginp;
    }

    string dpiSignature(Ast_ଗଣ୍ଠି_FTask* nodep, AstVar* rtnvarp) ଅଚଳ {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        ଯଦି (nodep->dpiPure()) dpiproto += "pure ";
        ଯଦି (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(ସତ୍ୟ, ସତ୍ୟ) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                ଯଦି (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    ଯଦି (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ଅଚଳ std::string dpiType = portp->dpiArgType(ମିଥ୍ୟା, ମିଥ୍ୟା);
                    dpiproto += dpiType;
                    ଅଚଳ std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(ମିଥ୍ୟା);
                    ଯଦି (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        ଫେରନ୍ତୁ dpiproto;
    }

    ସ୍ଥିର ଶୂନ୍ୟ checkLegalCIdentifier(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& name) {
        ଯଦି (name.end() != std::find_if(name.begin(), name.end(), [](ଅକ୍ଷର c) {
                ଫେରନ୍ତୁ !std::isalnum(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି* createDpiTemp(AstVar* portp, ଅଚଳ string& suffix) {
        ଅଚଳ string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        ଫେରନ୍ତୁ ନୂତନ AstCStmt{portp->fileline(), stmt};
    }

    ଶୂନ୍ୟ unlinkAndClone(Ast_ଗଣ୍ଠି_FTask* funcp, Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ withNext) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        ଯଦି (funcp->recursive()) {
            VNRelinker relinkHandle;
            ଯଦି (withNext) {
                nodep->unlinkFrBackWithNext(&relinkHandle);
            } ଅନ୍ୟଥା {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = nodep->clone_ବୃକ୍ଷ(withNext);
            relinkHandle.relink(newp);
        } ଅନ୍ୟଥା {
            ଯଦି (withNext) {
                nodep->unlinkFrBackWithNext();
            } ଅନ୍ୟଥା {
                nodep->unlinkFrBack();
            }
        }
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି* createAssignInternalToDpi(AstVar* portp, ତାର୍କିକ isPtr, ଅଚଳ string& frSuffix,
                                              ଅଚଳ string& toSuffix) {
        ଅଚଳ string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        ଫେରନ୍ତୁ ନୂତନ AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Stmt* createDpiScalarCvt(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* internalp, ଅଚଳ std::string& dpi,
                                           ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep, ତାର୍କିକ toDpi) {
        File_ଧାଡ଼ି* ଅଚଳ flp = internalp->fileline();
        ଅଚଳ AstBasicDType* ଅଚଳ basicp = dtypep->basicp();
        ଯଦି (toDpi) {
            AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{flp};
            ଯଦି (basicp->isDpiPrimitive()) {
                cstmtp->add(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->add(internalp);
                cstmtp->add(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } ଅନ୍ୟଥା {
                cstmtp->add((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::to_string(dtypep->width()) + ", " + dpi + ", ");
                cstmtp->add(internalp);
                cstmtp->add(");");
            }
            ଫେରନ୍ତୁ cstmtp;
        }
        std::string cvt;
        ତାର୍କିକ useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        ଯଦି (useSvVec) {
            AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            ଫେରନ୍ତୁ cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା cwidth = basicp->width_ଶବ୍ଦଗୁଡ଼ିକ() * VL_EDATASIZE;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstCExpr{flp, cvt + dpi + ")", cwidth};
        ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, internalp, ନୂତନ AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Stmt* createDpiCvt(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* internalp, ଅଚଳ std::string& dpi,
                                     ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep, ତାର୍କିକ toDpi) {
        File_ଧାଡ଼ି* ଅଚଳ flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_ଗଣ୍ଠି_Stmt* stmtsp = nullptr;
        ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elemp = ନୂତନ AstArraySel{flp, internalp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), i};
                stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } ଅନ୍ୟଥା ଯଦି (dtypep->containsUnpackedStruct()) {
            ପାଇଁ (ଅଚଳ AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ଅଚଳ selp
                    = ନୂତନ AstStructSel{flp, internalp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_ଗଣ୍ଠି::addNext(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_ବୃକ୍ଷ(), internalp);
        ଫେରନ୍ତୁ stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Stmt* createDpiArgCvt(AstVarScope* vscp, ଅଚଳ std::string& dpiName,
                                        ତାର୍କିକ toDpi) {
        AstVar* ଅଚଳ varp = vscp->varp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();
        ଅଚଳ std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା total = strides.empty() ? 1 : strides[0].first * strides[0].second;
        ଅଚଳ ତାର୍କିକ isStruct = varp->dtypep()->containsUnpackedStruct();
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_ଗଣ୍ଠି_Stmt* stmtsp = nullptr;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < total; ++i) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* internalp
                = ନୂତନ AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            ପାଇଁ (ଅଚଳ auto& stride : strides) {
                internalp = ନୂତନ AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            ଯଦି (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_ଶବ୍ଦଗୁଡ଼ିକ());
            } ଅନ୍ୟଥା ଯଦି (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        ଫେରନ୍ତୁ stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_ଗଣ୍ଠି_FTask* ଅଚଳ nodep, AstVar* ଅଚଳ rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଅଚଳ std::string cname = Ast_ଗଣ୍ଠି::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ଅଚଳ std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(ସତ୍ୟ, ସତ୍ୟ) : "";
        // The function we are building
        AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(ସତ୍ୟ);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(ସତ୍ୟ);
        funcp->entryPoint(ସତ୍ୟ);
        funcp->isStatic(ସତ୍ୟ);
        funcp->protect(ମିଥ୍ୟା);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_ଶୀର୍ଷ_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        ଯଦି (!makePortList(nodep, funcp)) ଫେରନ୍ତୁ nullptr;

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
        AstCStmt* ଅଚଳ prep = ନୂତନ AstCStmt{flp};
        funcp->addStmtsp(prep);
        // Static doesn't need save-restore as if below will re-fill proper value
        prep->add("static int __Vfuncnum = -1;\n");
        // First time init (faster than what the compiler does if we did a singleton
        prep->add("if (VL_UNLIKELY(__Vfuncnum == -1)) {\n");
        prep->add("__Vfuncnum = Verilated::exportFuncNum(\"" + nodep->cname() + "\");\n");
        prep->add("}\n");
        // If the find fails, it will throw an error
        prep->add("const VerilatedScope* const __Vscopep = Verilated::dpiScope();\n");
        // If 'dpiScope()' fails and '__Vscopep' is null; the exportFind function throws an error
        // If __Vcb is null the exportFind function throws and error
        ଅଚଳ std::string cbtype
            = VIdProtect::protect(v3Global.opt.prefix() + "__Vcb_" + nodep->cname() + "_t");
        prep->add(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ଅଚଳ callp = ନୂତନ AstCStmt{flp};
        ଅଚଳ auto addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ଅଚଳ vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(ମିଥ୍ୟା);
            portp->protect(ମିଥ୍ୟା);
            // Add argument to call
            ଅଚଳ VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(ନୂତନ AstVarRef{portp->fileline(), vscp, access});
            ଫେରନ୍ତୁ vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_ଗଣ୍ଠି* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var);
            ଯଦି (!portp) ଜାରି;
            ଯଦି (!portp->isIO()) ଜାରି;
            ଯଦି (portp->isFuncReturn()) ଜାରି;
            ଯଦି (portp == rtnvarp) ଜାରି;  // Handled below
            // Add argument to call
            AstVarScope* ଅଚଳ outvscp = addFuncArg(portp);
            ଅଚଳ ତାର୍କିକ isStruct = portp->dtypep()->containsUnpackedStruct();
            ଯଦି (portp->isWritable()) {
                outCvtsp = Ast_ଗଣ୍ଠି::addNext(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), ସତ୍ୟ)
                                       : createAssignInternalToDpi(portp, ସତ୍ୟ, tmpSuffixp, ""));
            }
            ଯଦି (!portp->isNonOutput()) ଜାରି;
            // Convert input/inout arguments to dpi type
            ଅଚଳ std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(ମିଥ୍ୟା).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), ମିଥ୍ୟା));
        }
        // Return value argument goes last
        ଯଦି (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        ଯଦି (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        ଯଦି (rtnvarp) {
            funcp->addStmtsp(createDpiTemp(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, ମିଥ୍ୟା, tmpSuffixp, ""));
            ଅଚଳ std::string index = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(ନୂତନ AstCStmt{flp, "return " + rtnvarp->name() + index + ";"});
        }
        ଫେରନ୍ତୁ funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_ଗଣ୍ଠି_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ଅଚଳ string name = Ast_ଗଣ୍ଠି::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ଅଚଳ string rtnType = rtnvarp            ? rtnvarp->dpiArgType(ସତ୍ୟ, ସତ୍ୟ)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(ସତ୍ୟ);
        funcp->dontCombine(ସତ୍ୟ);
        funcp->entryPoint(ମିଥ୍ୟା);
        funcp->isMethod(ମିଥ୍ୟା);
        funcp->protect(ମିଥ୍ୟା);
        funcp->dpiPure(nodep->dpiPure());

        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା cost = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(V3Control::getProfileData(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_ଶୀର୍ଷ_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        ଯଦି (!makePortList(nodep, funcp)) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ funcp;
    }

    AstCFunc* getDpiFunc(Ast_ଗଣ୍ଠି_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ଅଚଳ string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ଅଚଳ auto pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        ଯଦି (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ଅଚଳ funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            ଫେରନ୍ତୁ funcp;
        } ଅନ୍ୟଥା {
            // Seen this cname import before. Check if it's the same prototype.
            ଅଚଳ Ast_ଗଣ୍ଠି_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            ଯଦି (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                ଫେରନ୍ତୁ nullptr;
            } ଅନ୍ୟଥା {
                // Same signature, return the previously created CFunc
                ଫେରନ୍ତୁ firstFuncp;
            }
        }
    }

    ସ୍ଥିର ତାର୍କିକ makePortList(Ast_ଗଣ୍ଠି_FTask* nodep, AstCFunc* dpip) {
        ତାର୍କିକ allOk = ସତ୍ୟ;
        // Copy nodep's list of function I/O to the new dpip c function
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                ଯଦି (portp->isIO()) {
                    // Move it to new function
                    AstVar* ଅଚଳ newPortp = portp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    newPortp->funcLocal(ସତ୍ୟ);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    ଯଦି (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = ମିଥ୍ୟା;
                        portp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: DPI argument of type "
                                << portp->dtypep()->prettyTypeName() << '\n'
                                << portp->warnMore()
                                << "... For best portability, use bit, byte, int, or longint");
                        // We don't warn on logic either, although the 4-stateness is lost.
                        // That's what other simulators do.
                    }
                }
            }
        }
        ଫେରନ୍ତୁ allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    ସ୍ଥିର ତାର୍କିକ isDpiCIdentifier(ଅଚଳ string& name) {
        ଫେରନ୍ତୁ !name.empty() && VString::isIdentifier(name)
               && !std::isdigit(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_ଶବ୍ଦଗୁଡ଼ିକ::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    ତାର୍କିକ checkDpiStruct(ଅଚଳ AstVar* portp, ଅଚଳ AstStructDType* sdtypep, ତାର୍କିକ nested) {
        ଅଚଳ std::string name = sdtypep->dpiName();
        ଯଦି (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(ସତ୍ୟ) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ପାଇଁ (ଅଚଳ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ଯଦି (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep = itemp->subDTypep()->skipRefp();
            ଯେତେବେଳେ (ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ subp = VN_CAST(dtypep, ଗଣ୍ଠି_UOr_Struct_DType);
            ଅଚଳ AstBasicDType* ଅଚଳ basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ଅଚଳ ତାର୍କିକ isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            ଯଦି (dtypep->containsUnpackedStruct()) {
                ଯଦି (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), ସତ୍ୟ)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            } ଅନ୍ୟଥା ଯଦି (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(ସତ୍ୟ)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        ଯଦି (name.empty()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଅଚଳ AstStructDType* ଅଚଳ firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        ଯଦି (firstp->dpiTypedef() == sdtypep->dpiTypedef()) ଫେରନ୍ତୁ ସତ୍ୟ;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    ତାର୍କିକ checkDpiStructArgs(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* nodep) {
        ତାର୍କିକ allOk = ସତ୍ୟ;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଅଚଳ AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var);
            ଯଦି (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       ମିଥ୍ୟା)
                        && allOk;
            }
        }
        ଫେରନ୍ତୁ allOk;
    }

    ଶୂନ୍ୟ bodyDpiImportFunc(Ast_ଗଣ୍ଠି_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        ଯଦି (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_ଠେଲ(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ଅଚଳ portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                ଯଦି (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    ଯଦି (args != "") args += ", ";

                    ଯଦି (portp->isDpiOpenArray()) {
                        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ଅଚଳ string propName = portp->name() + "__Vopenprops";
                        ଅଚଳ string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(ନୂତନ AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ଅଚଳ string name = portp->name() + "__Vopenarray";
                        ଅଚଳ string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(ନୂତନ AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } ଅନ୍ୟଥା {
                        // Unpacked structures are passed by reference from a C array
                        ଅଚଳ ତାର୍କିକ isStruct = portp->dtypep()->containsUnpackedStruct();
                        ଯଦି (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            ଯଦି (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(createDpiTemp(portp, tmpSuffixp));
                        ଯଦି (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ସତ୍ୟ)
                                    : createAssignInternalToDpi(portp, ମିଥ୍ୟା, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        ଯଦି (nodep->dpiContext()) {
            ଅଚଳ string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(ନୂତନ AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            ଯଦି (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(createDpiTemp(rtnvscp->varp(), tmpSuffixp));
                ଅଚଳ std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{nodep->fileline()};
                cstmtp->add(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->add(" = ");
                cstmtp->add(callp);  // RHS
                cstmtp->add(";");
                cfuncp->addStmtsp(cstmtp);
            } ଅନ୍ୟଥା {
                // Othervise just call it
                cfuncp->addStmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                portp->protect(ମିଥ୍ୟା);  // No additional exposure - already part of shown proto
                ଯଦି (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ଅଚଳ portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ମିଥ୍ୟା));
                }
            }
        }

        ଯଦି (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_କାଢ଼(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        ଯଦି (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_ଶୀର୍ଷ_Scopep->createTemp("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        ଫେରନ୍ତୁ dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_ଗଣ୍ଠି_FTask* nodep, ତାର୍କିକ ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_ଗଣ୍ଠି::user2_Clear_ବୃକ୍ଷ();
        AstVar* rtnvarp = nullptr;
        ଯଦି (nodep->isFunction()) {
            AstVar* ଅଚଳ portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            ଯଦି (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ଅଚଳ bdtypep = portp->dtypep()->basicp();
                ଯଦି (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(ସତ୍ୟ)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ nullptr;
                }
                ଯଦି (!bdtypep->isDpiPrimitive()) {
                    ଯଦି (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    ଯଦି (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } ଅନ୍ୟଥା ଯଦି (nodep->taskPublic()) {
                ଯଦି (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            ଯଦି (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(ମିଥ୍ୟା);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, ମିଥ୍ୟା);
            rtnvarp = portp;
            rtnvarp->funcLocal(ସତ୍ୟ);
            rtnvarp->noCReset(ସତ୍ୟ);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            ଯଦି (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(ମିଥ୍ୟା);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = nullptr;
        ଯଦି (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            ଯଦି (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                ଯଦି (rtnvarp) VL_DO_DANGLING(ଠେଲ_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        ଯଦି (rtnvarp) {
            rtnvscp = ନୂତନ AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        ଯଦି (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } ଅନ୍ୟଥା ଯଦି (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } ଅନ୍ୟଥା ଯଦି (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        ଯଦି (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        ଅଚଳ string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* ଅଚଳ cfuncp = ନୂତନ AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->taskPublic() && rtnvarp) ? rtnvarp->cPubArgType(ସତ୍ୟ, ସତ୍ୟ) : "")};
        // It's ok to combine imports because this is just a wrapper;
        // duplicate wrappers can get merged.
        cfuncp->dontCombine(!nodep->dpiImport());
        cfuncp->entryPoint(!nodep->dpiImport());
        cfuncp->funcPublic(nodep->taskPublic());
        cfuncp->dpiContext(nodep->dpiContext());
        cfuncp->dpiExportImpl(nodep->dpiExport());
        cfuncp->dpiImportWrapper(nodep->dpiImport());
        cfuncp->recursive(nodep->recursive());
        // Hardcoded based on UVM usage; TODO make a verilated_std.vlt control for these
        cfuncp->unlikely(nodep->name() == "uvm_report_error" || nodep->name() == "uvm_report_info"
                         || nodep->name() == "uvm_report_fatal"
                         || nodep->name() == "uvm_report_warning");
        ଯଦି (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(ସତ୍ୟ);
            cfuncp->isLoose(ସତ୍ୟ);
        } ଅନ୍ୟଥା {
            cfuncp->isStatic(ମିଥ୍ୟା);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        ଯଦି (nodep->name() == "new") cfuncp->isConstructor(ସତ୍ୟ);
        ଯଦି (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(ସତ୍ୟ);
        ଯଦି (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        ଯଦି (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ଅଚଳ ତାର୍କିକ needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        ଯଦି (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        ଯଦି (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            ଯଦି (cfuncp->isConstructor()) {
                ତାର୍କିକ isInterfaceClass = ମିଥ୍ୟା;
                ଯଦି (ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                ଯଦି (!isInterfaceClass) {
                    ଅଚଳ string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        ନୂତନ AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        ଯଦି (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_ନିବେଶ_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_ନିବେଶ_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_ନିବେଶ_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        ଯଦି (nodep->dpiExport()) {
            AstScopeName* ଅଚଳ snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(ସତ୍ୟ);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                ଯଦି (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    ଠେଲ_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } ଅନ୍ୟଥା {
                    ଯଦି (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, ମିଥ୍ୟା);
                        portp->funcLocal(ସତ୍ୟ);
                        ଯଦି (portp->valuep()) ଠେଲ_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        ଯଦି (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = portp->dtypep()->skipRefp();
                            ଯଦି (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } ଅନ୍ୟଥା {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(ସତ୍ୟ);
                    }
                    AstVarScope* ଅଚଳ newvscp
                        = ନୂତନ AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        ଯଦି (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_ଗଣ୍ଠି* bodysp = nodep->stmtsp();
        ଯଦି (bodysp) {
            unlinkAndClone(nodep, bodysp, ସତ୍ୟ);
            AstBegin* ଅଚଳ tempp
                = ନୂତନ AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, ମିଥ୍ୟା};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                    ଯଦି (portp->isIO()) VL_DO_DANGLING(ଠେଲ_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            ଯଦି (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(tempp->delete_ବୃକ୍ଷ(), tempp);
        }
        ଯଦି (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        ଯଦି (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(ନୂତନ AstCReturn{
                rtnvscp->fileline(), ନୂତନ AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        ଯଦି (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            ତାର୍କିକ writesNonLocals = ମିଥ୍ୟା;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* ଅଚଳ varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                ଯଦି (varp->isFuncLocal()) ଫେରନ୍ତୁ;
                // Check if written
                ଯଦି (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = ସତ୍ୟ;
                }
                // Check if read
                ଯଦି (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            ଯଦି (writesNonLocals) {
                AstVarScope* ଅଚଳ dpiExportTriggerp = getDpiExporTrigger();
                File_ଧାଡ଼ି* ଅଚଳ flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* ଅଚଳ assignp
                    = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    ନୂତନ AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                ଯଦି (v3Global.opt.systemC()) {
                    assignp->addNextHere(ନୂତନ AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        ଯଦି (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        ଫେରନ୍ତୁ cfuncp;
    }

    ଶୂନ୍ୟ iterateIntoFTask(Ast_ଗଣ୍ଠି_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Before_Stmt(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->addHereThisAsNext(newp);
    }
    ଶୂନ୍ୟ processArgs(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        ପାଇଁ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ଅଚଳ arrayp = VN_CAST(argp->exprp(), ConsArray);
            ଯଦି (!arrayp) ଜାରି;

            File_ଧାଡ଼ି* ଅଚଳ flp = arrayp->fileline();
            AstVarScope* ଅଚଳ substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(ସତ୍ୟ);

            AstAssign* ଅଚଳ assignp
                = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ exprstmtp = ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{
                flp, assignp, ନୂତନ AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    ତାର୍କିକ isIfaceLocalImpure(Ast_ଗଣ୍ଠି_FTask* nodep, Ast_ଗଣ୍ଠି* impurep) {
        ଅଚଳ AstScope* ଅଚଳ scopep = m_statep->getScope(nodep);
        ଯଦି (!TaskStateVisitor::isIfaceFTaskScope(scopep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(impurep, VarRef);
        ଯଦି (!refp || !refp->varScopep()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ଫେରନ୍ତୁ refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {
        ଯଦି (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            ଫେରନ୍ତୁ;
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        nodep->addExprp(ନୂତନ AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ଶୂନ୍ୟ visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            ଫେରନ୍ତୁ;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        ଯଦି (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_ଗଣନା(), nodep, "No non-inline task, but no error issued");
            ଯଦି (VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)) {
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            } ଅନ୍ୟଥା {
                nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
            ଫେରନ୍ତୁ;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        ଯଦି (nodep->taskp()->isFunction()) {
            AstVar* ଅଚଳ fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            ଯଦି (AstAssign* ଅଚଳ assignp = VN_CAST(nodep->backp(), Assign)) {
                ଯଦି (AstVarRef* ଅଚଳ lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ଅଚଳ vscp = lhsp->varScopep();
                    ଯଦି (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_ବୃକ୍ଷ(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            ଯଦି (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_ଗଣ୍ଠି* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ଅଚଳ ତାର୍କିକ virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        ଯଦି (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } ଅନ୍ୟଥା {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        ଯଦି (VN_IS(nodep, New)) {  // New not legal as while() condition
            ଭର୍ତ୍ତି_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->backp(), ଗଣ୍ଠି_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            ଭର୍ତ୍ତି_Before_Stmt(nodep, beginp);
            AstVarRef* ଅଚଳ outrefp = ନୂତନ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (!VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ଅଚଳ outrefp = ନୂତନ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_ପଦସମଷ୍ଟି_Stmt* lambdap = ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{nodep->fileline(), beginp, outrefp};

            ଯଦି (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                newvarp->funcLocal(ସତ୍ୟ);

                // Create a new VarScope for the new variable
                AstVarScope* ଅଚଳ newvscp
                    = ନୂତନ AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreachAndNext([outvscp, newvscp](AstVarRef* refp) {
                    ଯଦି (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->hasResult(ମିଥ୍ୟା);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ଅଚଳ exprp = ନୂତନ AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(ନୂତନ AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            VIsCached::clear_Cache_ବୃକ୍ଷ();
        } ଅନ୍ୟଥା {  // VN_IS(nodep->backp(), StmtExpr)
            ଭର୍ତ୍ତି_Before_Stmt(nodep, beginp);
            ଯଦି (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* ଅଚଳ nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        ଯଦି (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା modes = 0;
            ଯଦି (nodep->dpiImport()) modes++;
            ଯଦି (nodep->dpiExport()) modes++;
            ଯଦି (nodep->taskPublic()) modes++;
            ଯଦି (nodep->classMethod()) modes++;
            ଯଦି (v3Global.opt.protectIds() && nodep->taskPublic()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            ଯଦି (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            ଅଚଳ ତାର୍କିକ noInline = m_statep->ftaskNoInline(nodep);
            ଅଚଳ ତାର୍କିକ needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            ଯଦି (noInline && !nodep->classMethod() && !nodep->recursive()) {
                ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ impurep = m_statep->checkImpure(nodep)) {
                    ଯଦି (!isIfaceLocalImpure(nodep, impurep)) {
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

            ଯଦି (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_ଗଣ୍ଠି_FTask* ଅଚଳ clonedFuncp = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                ଯଦି (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ଅଚଳ ତାର୍କିକ makeNonInlineFunc = noInline || needsNonInlineCFunc;
                ଯଦି (AstCFunc* ଅଚଳ cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->addNextHere(cfuncp);
                    ଯଦି (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            ଯଦି (nodep->isFunction()) {
                ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* ଅଚଳ vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ଅଚଳ vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_ଗଣ୍ଠି_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_ଗଣ୍ଠି_FTask_Ref* refp) { refs.ଠେଲ_back(refp); });
        ପାଇଁ (Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ଅଚଳ interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{
                nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
            Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = ନୂତନ Ast_Sen_ବୃକ୍ଷ{
                nodep->fileline(), ନୂତନ AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* ଅଚଳ activep
                = ନୂତନ AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_ବୃକ୍ଷ_Storep(activep->sentreep());
            activep->addStmtsp(
                ନୂତନ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(ନୂତନ AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_ବୃକ୍ଷ(), refp);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        ଯଦି (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    TaskVisitor(AstNetlist* nodep, TaskStateVisitor* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~TaskVisitor() {
        V3Stats::addStat("Optimizations, Functions inlined", m_statInlines);
        V3Stats::addStat("Optimizations, Hierarchical DPI wrappers with costs",
                         m_statHierDpisWithCosts);
    }
};

//######################################################################
// Mark interface members under timing controls of interface CFuncs as interface-sensed

ଶ୍ରେଣୀ TaskIfaceSensVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // STATE
    ତାର୍କିକ m_underIfaceFunc = ମିଥ୍ୟା;  // Under a CFunc owned by an interface scope
    ତାର୍କିକ m_underSenses = ମିଥ୍ୟା;  // Under a sensitivity expression of such a CFunc

    // METHODS
    ଶୂନ୍ୟ markSensesAndIterate(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!m_underIfaceFunc) ଫେରନ୍ତୁ;
        VL_RESTORER(m_underSenses);
        m_underSenses = ସତ୍ୟ;
        iterateAndNextConstNull(nodep);
    }
    // VISITORS
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Sen_ବୃକ୍ଷ* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    ଶୂନ୍ୟ visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (!m_underSenses) ଫେରନ୍ତୁ;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        ଯଦି (nodep->varp()->isFuncLocal()) ଫେରନ୍ତୁ;
        ଯଦି (AstIface* ଅଚଳ ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Task class functions

ଅଚଳ ଅକ୍ଷର* ଅଚଳ V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_ଗଣ୍ଠି_FTask_Ref* nodep, Ast_ଗଣ୍ଠି* taskStmtsp,
                                    V3TaskConnectState* statep, ତାର୍କିକ makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ଅଚଳ std::string, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> nameToIndex;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା tpinnum = 0;
    AstVar* sformatp = nullptr;
    ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
            ଯଦି (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                nameToIndex.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                ଯଦି (portp->attrSFormat()) {
                    sformatp = portp;
                } ଅନ୍ୟଥା ଯଦି (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ppinnum = 0;
    ତାର୍କିକ reorganize = ମିଥ୍ୟା;
    ପାଇଁ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        ଯଦି (!argp->name().empty()) {
            // By name
            ଅଚଳ auto it = nameToIndex.find(argp->name());
            ଯଦି (it == nameToIndex.end()) {
                ଯଦି (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_ବୃକ୍ଷ(), argp);
                }
            } ଅନ୍ୟଥା {
                ଯଦି (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_ବୃକ୍ଷ();
                }
                tconnects[it->second].second = argp;
                reorganize = ସତ୍ୟ;
            }
        } ଅନ୍ୟଥା {  // By pin number
            ଯଦି (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } ଅନ୍ୟଥା ଯଦି (ppinnum >= tpinnum) {
                ଯଦି (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } ଅନ୍ୟଥା ଯଦି (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_ବୃକ୍ଷ(), argp);
                }
            } ଅନ୍ୟଥା {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    ଯଦି (!makeChanges) ଫେରନ୍ତୁ tconnects;

    // Connect missing ones
    VInsertionSet<ଅଚଳ AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < tpinnum; ++i) {
        AstVar* ଅଚଳ portp = tconnects[i].first;
        ଯଦି (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newvaluep = nullptr;
            ଯଦି (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = ନୂତନ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } ଅନ୍ୟଥା ଯଦି (AstFuncRef* ଅଚଳ funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ funcp = funcRefp->taskp();
                ଯଦି (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } ଅନ୍ୟଥା ଯଦି (AstConst* ଅଚଳ constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            ଯଦି (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
                ଯଦି (!VN_IS(newvaluep, Const)) {
                    ଯଦି (statep) {
                        portp->pin_ଅଙ୍କ(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.ଭର୍ତ୍ତି(portp);
                    } ଅନ୍ୟଥା {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = ନୂତନ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_ବୃକ୍ଷ(ସତ୍ୟ) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ଅଚଳ newp = ନୂତନ AstArg{nodep->fileline(), portp->name(), newvaluep};
            ଯଦି (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_ବୃକ୍ଷ(),
                            tconnects[i].second = nullptr);
            }
            tconnects[i].second = newp;
            reorganize = ସତ୍ୟ;
        }
        ଯଦି (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } ଅନ୍ୟଥା {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
        AstArg* ଅଚଳ argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    ଯଦି (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        ଯେତେବେଳେ (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < tpinnum; ++i) {
            AstArg* ଅଚଳ argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    ଯଦି (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_ବୃକ୍ଷ("-  ftref-out: ");
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    ଯଦି (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        ଫେରନ୍ତୁ taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    ଫେରନ୍ତୁ tconnects;
}

ଶୂନ୍ୟ V3Task::taskConnectWrap(Ast_ଗଣ୍ଠି_FTask_Ref* nodep, ଅଚଳ V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ଅଚଳ VInsertionSet<ଅଚଳ AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    ପାଇଁ (ଅଚଳ AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_ଅଙ୍କ());
    ଅଚଳ auto pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    ଯଦି (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_ଗଣ୍ଠି_FTask* ଅଚଳ newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
        ଅଚଳ AstVar* ଅଚଳ portp = tconnect.first;
        AstArg* ଅଚଳ argp = tconnect.second;
        ଯଦି (argWrap.exists(portp)) {  // Removed arg
            statep->ଠେଲ_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_ଗଣ୍ଠି_FTask* V3Task::taskConnectWrapNew(Ast_ଗଣ୍ଠି_FTask* taskp, ଅଚଳ string& newname,
                                         ଅଚଳ V3TaskConnects& tconnects,
                                         ଅଚଳ VInsertionSet<ଅଚଳ AstVar*>& argWrap) {
    std::map<ଅଚଳ AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_ଗଣ୍ଠି_FTask* ଅଚଳ newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->addNextHere(newTaskp);

    Ast_ଗଣ୍ଠି_FTask_Ref* newCallp = nullptr;
    Ast_ଗଣ୍ଠି* newCallInsertp = nullptr;
    ଯଦି (VN_IS(taskp, Func)) {
        AstVar* ଅଚଳ fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ଅଚଳ newFVarp = fvarp->clone_ବୃକ୍ଷ(ସତ୍ୟ);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = ନୂତନ AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = ନୂତନ AstAssign{taskp->fileline(),
                            ନୂତନ AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } ଅନ୍ୟଥା ଯଦି (VN_IS(taskp, Task)) {
        newCallp = ନୂତନ AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{taskp->fileline(), newCallp};
    } ଅନ୍ୟଥା {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
        AstVar* ଅଚଳ portp = tconnect.first;
        AstVar* newPortp;
        ଯଦି (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = ନୂତନ AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(ସତ୍ୟ);
            ଯଦି (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
            newTaskp->addStmtsp(newPortp);
        } ଅନ୍ୟଥା {  // Defaulting arg
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = VN_AS(portp->valuep(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            ଯଦି ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ଅଚଳ VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ଅଚଳ refp = VN_AS(valuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), VarRef);
                refp->access(refAccess);
                AstArg* ଅଚଳ newArgp = ନୂତନ AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                ଜାରି;
            }
            // Create local temporary
            newPortp = ନୂତନ AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(ସତ୍ୟ);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            ଯଦି (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* ଅଚଳ newAssignp
                    = ନୂତନ AstAssign{valuep->fileline(),
                                    ନୂତନ AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_ବୃକ୍ଷ(ସତ୍ୟ)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ଅଚଳ newArgp = ନୂତନ AstArg{
            portp->fileline(), portp->name(),
            ନୂତନ AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ଅଚଳ auto it = oldNewVars.find(refp->varp());
        ଯଦି (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    ଫେରନ୍ତୁ newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, ତାର୍କିକ isPtr, ଅଚଳ string& frSuffix,
                                   ଅଚଳ string& toSuffix, ଅଚଳ string& frPrefix) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ଅଚଳ string frName = frPrefix + portp->name() + frSuffix;
    ଅଚଳ string toName = portp->name() + toSuffix;
    ଅଚଳ string idx = portp->name() + "__Vidx";
    size_t unpackSize = 1;  // non-unpacked array is treated as size 1
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା unpackDim = 0;
    ଯଦି (AstUnpackArrayDType* ଅଚଳ unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpackSize = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(ମିଥ୍ୟା).second;
        ଯଦି (unpackDim > 0) UASSERT_OBJ(unpackSize > 0, portp, "size must be greater than 0");
    }
    ଯଦି (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        ଅଚଳ ତାର୍କିକ isBit = portp->basicp()->isDpiBitVec();
        ଅଚଳ ତାର୍କିକ needsFor = unpackSize > 1;
        ଯଦି (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpackSize) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        ଯଦି (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_ଶବ୍ଦଗୁଡ଼ିକ()) + " * " + idx;
        }
        stmt += ", ";
        ଯଦି (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } ଅନ୍ୟଥା {
            stmt += frName + ")";
        }
    } ଅନ୍ୟଥା {
        ଅଚଳ ତାର୍କିକ isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        ଅଚଳ ତାର୍କିକ isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        ଅଚଳ string unpackAt = unpackSize > 1 ? "[" + idx + "]" : "[0]";
        ଯଦି (unpackDim > 0) {
            ଅଚଳ string forStmt = unpackSize > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpackSize) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } ଅନ୍ୟଥା {
            ଯଦି (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        ଯଦି (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        ଯଦି (unpackDim > 0) {
            stmt += "(&" + frName;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } ଅନ୍ୟଥା {
            stmt += frName;
        }
        ଯଦି (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    ଫେରନ୍ତୁ stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(ଅଚଳ std::string& lhsName, AstVar* varp) {
    std::string cvt;
    ତାର୍କିକ useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    ଅଚଳ std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା total = strides.empty() ? 1 : strides[0].first * strides[0].second;
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width_ଶବ୍ଦଗୁଡ଼ିକ = varp->basicp()->width_ଶବ୍ଦଗୁଡ଼ିକ();
    string statements;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < total; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        ପାଇଁ (ଅଚଳ auto& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        ଯଦି (useSvVec) {
            ଅଚଳ std::string offset = std::to_string(i * width_ଶବ୍ଦଗୁଡ଼ିକ);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } ଅନ୍ୟଥା {
            ଅଚଳ std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    ଫେରନ୍ତୁ statements;
}

ଶୂନ୍ୟ V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ଅଚଳ TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("task", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
