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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Task.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3ConstPool.h"
#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3LanguageWords.h"
#અન્તર્ભાવ "V3SharedTmps.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

વર્ગ TaskBaseVertex VL_NOT_FINAL : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_ગાંઠ* m_impurep = nullptr;  // Node causing impure function w/ outside references
    તાર્કિક m_noInline = ખોટું;  // Marked with pragma
જાહેર:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = મૂળભૂત;
    તાર્કિક pure() અચલ { પાછા m_impurep == nullptr; }
    Ast_ગાંઠ* impure_ગાંઠ() અચલ { પાછા m_impurep; }
    શૂન્ય impure(Ast_ગાંઠ* nodep) { m_impurep = nodep; }
    તાર્કિક noInline() અચલ { પાછા m_noInline; }
    શૂન્ય noInline(તાર્કિક flag) { m_noInline = flag; }
};

વર્ગ TaskFTaskVertex final : જાહેર TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_ગાંઠ_FTask* અચલ m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    તાર્કિક m_needsNonInlineCFunc = ખોટું;  // Needs CFunc for selected non-inlined call sites

જાહેર:
    TaskFTaskVertex(V3Graph* graphp, Ast_ગાંઠ_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = મૂળભૂત;
    Ast_ગાંઠ_FTask* nodep() અચલ VL_MT_STABLE { પાછા m_nodep; }
    string name() અચલ override VL_MT_STABLE { પાછા nodep()->name(); }
    string dotColor() અચલ override { પાછા pure() ? "black" : "red"; }
    AstCFunc* cFuncp() અચલ { પાછા m_cFuncp; }
    શૂન્ય cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    તાર્કિક needsNonInlineCFunc() અચલ { પાછા m_needsNonInlineCFunc; }
    શૂન્ય needsNonInlineCFunc(તાર્કિક flag) { m_needsNonInlineCFunc = flag; }
};

વર્ગ TaskCodeVertex final : જાહેર TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
જાહેર:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = મૂળભૂત;
    string name() અચલ override VL_MT_STABLE { પાછા "*CODE*"; }
    string dotColor() અચલ override { પાછા "green"; }
};

વર્ગ TaskEdge final : જાહેર V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
જાહેર:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* ટોચ)
        : V3GraphEdge{graphp, fromp, ટોચ, 1, ખોટું} {}
    ~TaskEdge() override = મૂળભૂત;
    string dotLabel() અચલ override { પાછા "w" + cvtToStr(weight()); }
};

//######################################################################

વર્ગ TaskStateVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    અચલ VNUser3InUse m_inuser3;
    અચલ VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<અચલ Ast_ગાંઠ_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_ગાંઠ_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    તાર્કિક m_underPortVar = ખોટું;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

જાહેર:
    // METHODS
    સ્થિર તાર્કિક isVirtualIfaceMethodCall(અચલ Ast_ગાંઠ_FTask_Ref* refp) {
        અચલ AstMethodCall* અચલ mrefp = VN_CAST(refp, MethodCall);
        જો (!mrefp) પાછા ખોટું;
        અચલ Ast_ગાંઠ_પદાવલિ* અચલ fromp = mrefp->fromp();
        જો (!fromp || !fromp->dtypep()) પાછા ખોટું;
        અચલ AstIfaceRefDType* અચલ ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        પાછા ifrefp && ifrefp->isVirtual();
    }
    સ્થિર તાર્કિક isIfaceFTaskScope(અચલ AstScope* scopep) {
        પાછા scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_ગાંઠ_FTask* nodep) {
        AstScope* અચલ scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        પાછા scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        અચલ auto iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        પાછા iter->second;
    }
    AstClass* getClassp(Ast_ગાંઠ_FTask* nodep) {
        AstClass* અચલ classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        પાછા classp;
    }
    શૂન્ય remapFuncClassp(Ast_ગાંઠ_FTask* nodep, Ast_ગાંઠ_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    તાર્કિક ftaskNoInline(Ast_ગાંઠ_FTask* nodep) { પાછા getFTaskVertex(nodep)->noInline(); }
    તાર્કિક ftaskNeedsNonInlineCFunc(Ast_ગાંઠ_FTask* nodep) {
        પાછા getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_ગાંઠ_FTask* nodep) { પાછા getFTaskVertex(nodep)->cFuncp(); }
    શૂન્ય ftaskCFuncp(Ast_ગાંઠ_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_ગાંઠ* checkImpure(Ast_ગાંઠ_FTask* nodep) { પાછા getFTaskVertex(nodep)->impure_ગાંઠ(); }

ખાનગી:
    TaskFTaskVertex* getFTaskVertex(Ast_ગાંઠ_FTask* nodep) {
        જો (!nodep->user4p()) nodep->user4p(નવું TaskFTaskVertex{&m_callGraph, nodep});
        પાછા static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    શૂન્ય propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        માટે (V3GraphVertex& vtx : m_callGraph.vertices()) {
            જો (TaskFTaskVertex* અચલ ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                જો (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        જ્યારે (!workList.empty()) {
            TaskFTaskVertex* અચલ calleep = workList.back();
            workList.કાઢો_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            માટે (V3GraphEdge& edge : calleep->inEdges()) {
                જો (TaskFTaskVertex* અચલ callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    જો (!callerp->pure()) ચાલુ;  // Already processed or on workList
                    callerp->impure(calleep->impure_ગાંઠ());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    શૂન્ય decideInlining() {
        માટે (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* અચલ ftVtxp = vtx.cast<TaskFTaskVertex>();
            જો (!ftVtxp) ચાલુ;
            // Move on if already decided not to inline
            જો (ftVtxp->noInline()) ચાલુ;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            જો (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(સાચું);
                ચાલુ;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            જો (v3Global.opt.fInlineFuncsEager()) ચાલુ;

            Ast_ગાંઠ_FTask* અચલ ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            જો (!ftaskp->verilogFunction()) ચાલુ;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            જો (ftaskp->taskPublic()) ચાલુ;
            // If the function is pure, it's OK not to inline, so don't do it
            જો (ftVtxp->pure()) ftVtxp->noInline(સાચું);
        }
    }

    // VISITORS
    શૂન્ય visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        માટે (Ast_ગાંઠ* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVarScope* અચલ vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* અચલ varp = vscp->varp();
                જો (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        માટે (Ast_ગાંઠ* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            જો (Ast_ગાંઠ_FTask* અચલ taskp = VN_CAST(stmtp, ગાંઠ_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* અચલ taskVtxp = getFTaskVertex(nodep->taskp());
        નવું TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        જો (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(સાચું);
        // Do we have to disable inlining the function?
        અચલ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        જો (!taskVtxp->noInline()) {  // Else short-circuit below
            માટે (અચલ auto& itr : tconnects) {
                અચલ AstVar* અચલ portp = itr.first;
                અચલ AstArg* અચલ argp = itr.second;
                જો (અચલ Ast_ગાંઠ_પદાવલિ* અચલ pinp = argp->exprp()) {
                    જો ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(સાચું);
                    }
                }
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        જો (nodep->dpiImport()) m_curVxp->noInline(સાચું);
        જો (nodep->classMethod()) m_curVxp->noInline(સાચું);  // Until V3Task supports it
        જો (nodep->recursive()) m_curVxp->noInline(સાચું);
        જો (nodep->isConstructor()) {
            m_curVxp->noInline(સાચું);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstPragma* nodep) override {
        જો (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર {
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        જો (nodep->isIO()) m_underPortVar = સાચું;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        જો (m_underPortVar) પાછા;
        AstVar* અચલ varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        જો (nodep->access().isReadOnly() && varp->constPoolEntry()) પાછા;
        જો (varp->user4u().toGraphVertex() != m_curVxp) {
            જો (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    શૂન્ય visit(AstClass* nodep) override {
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
        Ast_ગાંઠ* insertp = nullptr;
        માટે (AstInitialAutomatic* initialp : m_initialps) {
            જો (Ast_ગાંઠ* અચલ movep = initialp->stmtsp()) {
                movep->unlinkFrBackWithNext();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_ગાંઠ::addNextNull(insertp, movep);
            }
            VL_DO_DANGLING(ધકેલો_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    શૂન્ય visit(AstInitialAutomatic* nodep) override {
        m_initialps.ધકેલો_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = નવું TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.removeRedundantEdgesSum(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        જો (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = મૂળભૂત;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

સંરચના TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    સ્થિર std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> unpackDimsAndStrides(AstVar* varp) {
        Ast_ગાંઠ_DType* અચલ dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> dimStrides;
        AstUnpackArrayDType* અચલ unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        જો (!unpackp) પાછા dimStrides;

        અચલ std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        અચલ size_t nDims = dims.size();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        માટે (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        પાછા dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    સ્થિર std::pair<std::string, તાર્કિક> dpiToInternalCvtStmt(અચલ Ast_ગાંઠ_DType* dtypep) {
        અચલ AstBasicDType* અચલ basicp = dtypep->basicp();

        // DPI types using Primitive C types
        જો (basicp->keyword() == VBasicDTypeKwd::CHANDLE) પાછા {"VL_CVT_VP_Q(", ખોટું};
        જો (basicp->keyword() == VBasicDTypeKwd::STRING) પાછા {"VL_CVT_N_CSTR(", ખોટું};
        જો (basicp->isDpiPrimitive()) પાછા {"(", ખોટું};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        અચલ std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        અચલ અક્ષર sizeChar = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        અચલ std::string& size = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        પાછા {"VL_SET_"s + sizeChar + "_" + vecType + "(" + size + ", ", સાચું};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

વર્ગ TaskVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<અચલ string, std::tuple<Ast_ગાંઠ_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* અચલ m_statep;  // Common state between visitors
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    Ast_ટોચ_Scope* અચલ m_ટોચ_Scopep = v3Global.rootp()->ટોચ_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_ગાંઠ* m_insStmtp = nullptr;  // Where to insert statement
    તાર્કિક m_inSensesp = ખોટું;  // Are we under a senitem?
    તાર્કિક m_inNew = ખોટું;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    પૂર્ણાંક m_uncon_Var_અંક = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, અચલ AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, અચલ string& name, AstVar* examplep) {
        AstVar* અચલ newvarp = નવું AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(સાચું);
        funcp->addVarsp(newvarp);
        AstVarScope* અચલ newvscp = નવું AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        પાછા newvscp;
    }
    AstVarScope* create_આગત_Var(AstCFunc* funcp, અચલ string& name, VBasicDTypeKwd kwd) {
        AstVar* અચલ newvarp
            = નવું AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(સાચું);
        newvarp->direction(VDirection::આગત);
        funcp->addArgsp(newvarp);
        AstVarScope* અચલ newvscp = નવું AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        પાછા newvscp;
    }
    સ્થિર AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* અચલ refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* અચલ vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_વૃક્ષ(), refp);
        પાછા vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(અચલ Ast_ગાંઠ_FTask_Ref* refp, AstVar* invarp,
                                અચલ std::string& suffix) {
        જો (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            પાછા constPoolTable(invarp);
        } નહીંતર {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* અચલ newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* અચલ newvarp = newvscp->varp();
            newvarp->funcLocal(ખોટું);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(સાચું);
            પાછા newvscp;
        }
    }

    // Replace varrefs with new var pointer
    શૂન્ય relink(Ast_ગાંઠ* nodep) {
        nodep->foreachAndNext([](AstVarRef* refp) {
            જો (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* અચલ newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_ગાંઠ_પદાવલિ* pinp, AstVarScope* newvscp, તાર્કિક pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_ગાંઠ_પદાવલિ* inPinp = pinp;
        જો (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_ગાંઠ_પદાવલિ* inPinClonep
            = pureCheck ? inPinp->clone_વૃક્ષ_Pure(સાચું) : inPinp->clone_વૃક્ષ(સાચું);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        અચલ AstUnpackArrayDType* અચલ arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        જો (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = નવું AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* અચલ assp = નવું AstAssign{
            pinp->fileline(), નવું AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, સાચું);  // Ok if in <= block
        પાછા assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_ગાંઠ_પદાવલિ* pinp, AstVarScope* newvscp,
                                        તાર્કિક pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_ગાંઠ_પદાવલિ* outPinp = pinp;
        Ast_ગાંઠ_પદાવલિ* postRhsp = નવું AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        જો (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            જો (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, ગાંઠ_Uniop)->lhsp();
                જો (postRhsp->isSigned()) {
                    postRhsp = નવું AstISToRD{pinp->fileline(), postRhsp};
                } નહીંતર {
                    postRhsp = નવું AstIToRD{pinp->fileline(), postRhsp};
                }
            } નહીંતર {
                જો (Ast_ગાંઠ_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } નહીંતર જો (Ast_ગાંઠ_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } નહીંતર જો (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } નહીંતર {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                જો (outPinp->width() < portp->width()) {
                    postRhsp = નવું AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } નહીંતર {  // pin width > port width
                    જો (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = નવું AstExtendS{pinp->fileline(), postRhsp};
                    } નહીંતર {
                        postRhsp = નવું AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        જો (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, ગાંઠ_Uniop)->lhsp();
            postRhsp = નવું AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_ગાંઠ_પદાવલિ* અચલ outPinClonep
            = pureCheck ? outPinp->clone_વૃક્ષ_Pure(સાચું) : outPinp->clone_વૃક્ષ(સાચું);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        અચલ AstUnpackArrayDType* અચલ arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        જો (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = નવું AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* અચલ assp = નવું AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, સાચું);  // Ok if in <= block
        પાછા assp;
    }

    શૂન્ય changeAtWriteRecurse(Ast_ગાંઠ_પદાવલિ* અચલ exprp) {
        // Change nested at methods to writable variant
        જો (AstCMethodHard* અચલ cMethodp = VN_CAST(exprp, CMethodHard)) {
            જો (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    શૂન્ય connectPort(અચલ Ast_ગાંઠ_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_ગાંઠ* beginp,
                     તાર્કિક inlineTask) {
        Ast_ગાંઠ_પદાવલિ* pinp = argp->exprp();
        જો (inlineTask) {
            portp->unlinkFrBack();
            ધકેલો_Deletep(portp);  // Remove it from the clone (not original)
        }
        જો (!pinp) {
            // Too few arguments in function call
        } નહીંતર {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            જો (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* અચલ varp = નવું AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_અંક++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* અચલ newvscp = નવું AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* અચલ repp = નવું AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(ધકેલો_Deletep(pinp), pinp);
                pinp = repp;
            }
            જો (inlineTask) {
                ધકેલો_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_વૃક્ષ(), argp);  // Args no longer needed
            }
            જો (portp->isRef() || portp->isConstRef()) {
                તાર્કિક refArgOk = ખોટું;
                જો (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = સાચું;
                } નહીંતર જો (AstCMethodHard* અચલ cMethodp = VN_CAST(pinp, CMethodHard)) {
                    જો (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } નહીંતર {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                જો (refArgOk) {
                    જો (AstVarRef* અચલ varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } નહીંતર {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                જો (inlineTask) {
                    જો (AstVarRef* અચલ varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* અચલ localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } નહીંતર {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } નહીંતર જો (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* અચલ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                જો (!inlineTask) {
                    pinp->replaceWith(
                        નવું AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    ધકેલો_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* અચલ preassp = connectPortMakeInAssign(pinp, newvscp, સાચું);
                beginp->addNextHere(preassp);

                AstAssign* અચલ postassp = connectPortMakeOutAssign(portp, pinp, newvscp, સાચું);
                beginp->addNext(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } નહીંતર જો (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* અચલ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                જો (!inlineTask) {
                    pinp->replaceWith(નવું AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    ધકેલો_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* અચલ postassp = connectPortMakeOutAssign(portp, pinp, newvscp, ખોટું);
                // Put assignment BEHIND of all other statements
                beginp->addNext(postassp);
            } નહીંતર જો (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* અચલ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* અચલ preassp = connectPortMakeInAssign(pinp, newvscp, ખોટું);
                // Put assignment in FRONT of all other statements
                beginp->addNextHere(preassp);
            }
        }
    }

    તાર્કિક hasRefArgument(Ast_ગાંઠ_FTask* nodep) {
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (varp->isRef() || varp->isConstRef()) પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }

    Ast_ગાંઠ* createInlinedFTask(Ast_ગાંઠ_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_ગાંઠ* અચલ newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_વૃક્ષ(સાચું) : nullptr;
        Ast_ગાંઠ* અચલ beginp
            = નવું AstComment{refp->fileline(), "Function: "s + refp->name(), સાચું};
        જો (newbodysp) beginp->addNext(newbodysp);
        જો (debug() >= 9) beginp->dump_વૃક્ષ_And_Next(cout, "-  newbegi: ");
        //
        // Create input variables
        Ast_ગાંઠ::user2_Clear_વૃક્ષ();
        {
            અચલ V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            માટે (અચલ auto& itr : tconnects) {
                AstVar* અચલ portp = itr.first;
                AstArg* અચલ argp = itr.second;
                connectPort(refp, portp, argp, beginp, સાચું);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_ગાંઠ* nextstmtp;
            માટે (Ast_ગાંઠ* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    જો (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* અચલ localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        જો (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_ગાંઠ* અચલ crstp = નવું AstAssign{
                                portp->fileline(),
                                નવું AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                નવું AstCReset{portp->fileline(), portp, ખોટું}};
                            portp->replaceWith(crstp);
                        } નહીંતર {
                            portp->unlinkFrBack();
                        }
                        ધકેલો_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        જો (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        જો (debug() >= 9) beginp->dump_વૃક્ષ_And_Next(cout, "-  iotask: ");
        પાછા beginp;
    }

    Ast_ગાંઠ* createNonInlinedFTask(Ast_ગાંઠ_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* અચલ cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_ગાંઠ* અચલ beginp
            = નવું AstComment{refp->fileline(), "Function: "s + refp->name(), સાચું};
        Ast_ગાંઠ_CCall* ccallp;
        જો (VN_IS(refp, New)) {
            AstCNew* અચલ cnewp = નવું AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } નહીંતર જો (અચલ AstMethodCall* અચલ mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = નવું AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        } નહીંતર {
            ccallp = નવું AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        }

        જો (અચલ AstFuncRef* અચલ funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } નહીંતર જો (અચલ AstTaskRef* અચલ taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } નહીંતર જો (અચલ AstMethodCall* અચલ methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            અચલ V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            માટે (અચલ auto& itr : tconnects) {
                AstVar* અચલ portp = itr.first;
                AstArg* અચલ argp = itr.second;
                connectPort(refp, portp, argp, beginp, ખોટું);
            }
        }
        // First argument is symbol table, then output if a function
        અચલ તાર્કિક needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        જો (needSyms) ccallp->argTypes("vlSymsp");

        જો (refp->taskp()->dpiContext()) {
            AstScopeName* અચલ snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_લીટી* અચલ flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                નવું AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(નવું AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        માટે (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        જો (outvscp) ccallp->addArgsp(નવું AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        જો (debug() >= 9) beginp->dump_વૃક્ષ_And_Next(cout, "-  nitask: ");
        પાછા beginp;
    }

    string dpiSignature(Ast_ગાંઠ_FTask* nodep, AstVar* rtnvarp) અચલ {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        જો (nodep->dpiPure()) dpiproto += "pure ";
        જો (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(સાચું, સાચું) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (અચલ AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                જો (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    જો (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    અચલ std::string dpiType = portp->dpiArgType(ખોટું, ખોટું);
                    dpiproto += dpiType;
                    અચલ std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(ખોટું);
                    જો (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        પાછા dpiproto;
    }

    સ્થિર શૂન્ય checkLegalCIdentifier(Ast_ગાંઠ* nodep, અચલ string& name) {
        જો (name.end() != std::find_if(name.begin(), name.end(), [](અક્ષર c) {
                પાછા !std::isalnum(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    સ્થિર Ast_ગાંઠ* createDpiTemp(AstVar* portp, અચલ string& suffix) {
        અચલ string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        પાછા નવું AstCStmt{portp->fileline(), stmt};
    }

    શૂન્ય unlinkAndClone(Ast_ગાંઠ_FTask* funcp, Ast_ગાંઠ* nodep, તાર્કિક withNext) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        જો (funcp->recursive()) {
            VNRelinker relinkHandle;
            જો (withNext) {
                nodep->unlinkFrBackWithNext(&relinkHandle);
            } નહીંતર {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_ગાંઠ* અચલ newp = nodep->clone_વૃક્ષ(withNext);
            relinkHandle.relink(newp);
        } નહીંતર {
            જો (withNext) {
                nodep->unlinkFrBackWithNext();
            } નહીંતર {
                nodep->unlinkFrBack();
            }
        }
    }

    સ્થિર Ast_ગાંઠ* createAssignInternalToDpi(AstVar* portp, તાર્કિક isPtr, અચલ string& frSuffix,
                                              અચલ string& toSuffix) {
        અચલ string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        પાછા નવું AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    સ્થિર Ast_ગાંઠ_Stmt* createDpiScalarCvt(Ast_ગાંઠ_પદાવલિ* internalp, અચલ std::string& dpi,
                                           અચલ Ast_ગાંઠ_DType* dtypep, તાર્કિક toDpi) {
        File_લીટી* અચલ flp = internalp->fileline();
        અચલ AstBasicDType* અચલ basicp = dtypep->basicp();
        જો (toDpi) {
            AstCStmt* અચલ cstmtp = નવું AstCStmt{flp};
            જો (basicp->isDpiPrimitive()) {
                cstmtp->add(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->add(internalp);
                cstmtp->add(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } નહીંતર {
                cstmtp->add((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::to_string(dtypep->width()) + ", " + dpi + ", ");
                cstmtp->add(internalp);
                cstmtp->add(");");
            }
            પાછા cstmtp;
        }
        std::string cvt;
        તાર્કિક useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        જો (useSvVec) {
            AstCStmt* અચલ cstmtp = નવું AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            પાછા cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        અચલ પૂર્ણાંક cwidth = basicp->width_શબ્દો() * VL_EDATASIZE;
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstCExpr{flp, cvt + dpi + ")", cwidth};
        પાછા નવું AstAssign{flp, internalp, નવું AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    સ્થિર Ast_ગાંઠ_Stmt* createDpiCvt(Ast_ગાંઠ_પદાવલિ* internalp, અચલ std::string& dpi,
                                     અચલ Ast_ગાંઠ_DType* dtypep, તાર્કિક toDpi) {
        File_લીટી* અચલ flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_ગાંઠ_Stmt* stmtsp = nullptr;
        જો (અચલ AstUnpackArrayDType* અચલ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            માટે (પૂર્ણાંક i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_ગાંઠ_પદાવલિ* અચલ elemp = નવું AstArraySel{flp, internalp->clone_વૃક્ષ(ખોટું), i};
                stmtsp = Ast_ગાંઠ::addNext(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } નહીંતર જો (dtypep->containsUnpackedStruct()) {
            માટે (અચલ AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* અચલ selp
                    = નવું AstStructSel{flp, internalp->clone_વૃક્ષ(ખોટું), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_ગાંઠ::addNext(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } નહીંતર {
            પાછા createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_વૃક્ષ(), internalp);
        પાછા stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    સ્થિર Ast_ગાંઠ_Stmt* createDpiArgCvt(AstVarScope* vscp, અચલ std::string& dpiName,
                                        તાર્કિક toDpi) {
        AstVar* અચલ varp = vscp->varp();
        File_લીટી* અચલ flp = vscp->fileline();
        અચલ std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        અચલ પૂર્ણાંક total = strides.empty() ? 1 : strides[0].first * strides[0].second;
        અચલ તાર્કિક isStruct = varp->dtypep()->containsUnpackedStruct();
        અચલ Ast_ગાંઠ_DType* અચલ elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_ગાંઠ_Stmt* stmtsp = nullptr;
        માટે (પૂર્ણાંક i = 0; i < total; ++i) {
            Ast_ગાંઠ_પદાવલિ* internalp
                = નવું AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            માટે (અચલ auto& stride : strides) {
                internalp = નવું AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            જો (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_શબ્દો());
            } નહીંતર જો (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_ગાંઠ::addNext(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        પાછા stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_ગાંઠ_FTask* અચલ nodep, AstVar* અચલ rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_લીટી* અચલ flp = nodep->fileline();
        અચલ std::string cname = Ast_ગાંઠ::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        અચલ અક્ષર* અચલ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        અચલ std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(સાચું, સાચું) : "";
        // The function we are building
        AstCFunc* અચલ funcp = નવું AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(સાચું);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(સાચું);
        funcp->entryPoint(સાચું);
        funcp->isStatic(સાચું);
        funcp->protect(ખોટું);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_ટોચ_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        જો (!makePortList(nodep, funcp)) પાછા nullptr;

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
        AstCStmt* અચલ prep = નવું AstCStmt{flp};
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
        અચલ std::string cbtype
            = VIdProtect::protect(v3Global.opt.prefix() + "__Vcb_" + nodep->cname() + "_t");
        prep->add(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* અચલ callp = નવું AstCStmt{flp};
        અચલ auto addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* અચલ vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(ખોટું);
            portp->protect(ખોટું);
            // Add argument to call
            અચલ VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(નવું AstVarRef{portp->fileline(), vscp, access});
            પાછા vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_ગાંઠ* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* અચલ portp = VN_CAST(stmtp, Var);
            જો (!portp) ચાલુ;
            જો (!portp->isIO()) ચાલુ;
            જો (portp->isFuncReturn()) ચાલુ;
            જો (portp == rtnvarp) ચાલુ;  // Handled below
            // Add argument to call
            AstVarScope* અચલ outvscp = addFuncArg(portp);
            અચલ તાર્કિક isStruct = portp->dtypep()->containsUnpackedStruct();
            જો (portp->isWritable()) {
                outCvtsp = Ast_ગાંઠ::addNext(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), સાચું)
                                       : createAssignInternalToDpi(portp, સાચું, tmpSuffixp, ""));
            }
            જો (!portp->isNonOutput()) ચાલુ;
            // Convert input/inout arguments to dpi type
            અચલ std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(ખોટું).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), ખોટું));
        }
        // Return value argument goes last
        જો (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        જો (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        જો (rtnvarp) {
            funcp->addStmtsp(createDpiTemp(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, ખોટું, tmpSuffixp, ""));
            અચલ std::string index = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(નવું AstCStmt{flp, "return " + rtnvarp->name() + index + ";"});
        }
        પાછા funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_ગાંઠ_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        અચલ string name = Ast_ગાંઠ::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        અચલ string rtnType = rtnvarp            ? rtnvarp->dpiArgType(સાચું, સાચું)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* અચલ funcp = નવું AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(સાચું);
        funcp->dontCombine(સાચું);
        funcp->entryPoint(ખોટું);
        funcp->isMethod(ખોટું);
        funcp->protect(ખોટું);
        funcp->dpiPure(nodep->dpiPure());

        અચલ પૂર્ણાંક cost = static_cast<પૂર્ણાંક>(V3Control::getProfileData(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_ટોચ_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        જો (!makePortList(nodep, funcp)) પાછા nullptr;
        પાછા funcp;
    }

    AstCFunc* getDpiFunc(Ast_ગાંઠ_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        અચલ string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        અચલ auto pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        જો (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* અચલ funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            પાછા funcp;
        } નહીંતર {
            // Seen this cname import before. Check if it's the same prototype.
            અચલ Ast_ગાંઠ_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            જો (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                પાછા nullptr;
            } નહીંતર {
                // Same signature, return the previously created CFunc
                પાછા firstFuncp;
            }
        }
    }

    સ્થિર તાર્કિક makePortList(Ast_ગાંઠ_FTask* nodep, AstCFunc* dpip) {
        તાર્કિક allOk = સાચું;
        // Copy nodep's list of function I/O to the new dpip c function
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                જો (portp->isIO()) {
                    // Move it to new function
                    AstVar* અચલ newPortp = portp->clone_વૃક્ષ(ખોટું);
                    newPortp->funcLocal(સાચું);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    જો (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = ખોટું;
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
        પાછા allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    સ્થિર તાર્કિક isDpiCIdentifier(અચલ string& name) {
        પાછા !name.empty() && VString::isIdentifier(name)
               && !std::isdigit(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_શબ્દો::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    તાર્કિક checkDpiStruct(અચલ AstVar* portp, અચલ AstStructDType* sdtypep, તાર્કિક nested) {
        અચલ std::string name = sdtypep->dpiName();
        જો (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(સાચું) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            પાછા ખોટું;
        }
        માટે (અચલ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            જો (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                પાછા ખોટું;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            અચલ Ast_ગાંઠ_DType* dtypep = itemp->subDTypep()->skipRefp();
            જ્યારે (અચલ AstUnpackArrayDType* અચલ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ subp = VN_CAST(dtypep, ગાંઠ_UOr_Struct_DType);
            અચલ AstBasicDType* અચલ basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            અચલ તાર્કિક isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            જો (dtypep->containsUnpackedStruct()) {
                જો (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), સાચું)) પાછા ખોટું;
            } નહીંતર જો (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(સાચું)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                પાછા ખોટું;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        જો (name.empty()) પાછા સાચું;
        અચલ AstStructDType* અચલ firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        જો (firstp->dpiTypedef() == sdtypep->dpiTypedef()) પાછા સાચું;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        પાછા ખોટું;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    તાર્કિક checkDpiStructArgs(અચલ Ast_ગાંઠ_FTask* nodep) {
        તાર્કિક allOk = સાચું;
        માટે (અચલ Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            અચલ AstVar* અચલ portp = VN_CAST(stmtp, Var);
            જો (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       ખોટું)
                        && allOk;
            }
        }
        પાછા allOk;
    }

    શૂન્ય bodyDpiImportFunc(Ast_ગાંઠ_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        અચલ અક્ષર* અચલ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        જો (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_ધકેલો(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        માટે (Ast_ગાંઠ* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                AstVarScope* અચલ portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                જો (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    જો (args != "") args += ", ";

                    જો (portp->isDpiOpenArray()) {
                        Ast_ગાંઠ_DType* અચલ dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        અચલ string propName = portp->name() + "__Vopenprops";
                        અચલ string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(નવું AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        અચલ string name = portp->name() + "__Vopenarray";
                        અચલ string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(નવું AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } નહીંતર {
                        // Unpacked structures are passed by reference from a C array
                        અચલ તાર્કિક isStruct = portp->dtypep()->containsUnpackedStruct();
                        જો (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            જો (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(createDpiTemp(portp, tmpSuffixp));
                        જો (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, સાચું)
                                    : createAssignInternalToDpi(portp, ખોટું, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        જો (nodep->dpiContext()) {
            અચલ string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(નવું AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* અચલ callp = નવું AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            જો (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(createDpiTemp(rtnvscp->varp(), tmpSuffixp));
                અચલ std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* અચલ cstmtp = નવું AstCStmt{nodep->fileline()};
                cstmtp->add(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->add(" = ");
                cstmtp->add(callp);  // RHS
                cstmtp->add(";");
                cfuncp->addStmtsp(cstmtp);
            } નહીંતર {
                // Othervise just call it
                cfuncp->addStmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        માટે (Ast_ગાંઠ* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                portp->protect(ખોટું);  // No additional exposure - already part of shown proto
                જો (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* અચલ portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ખોટું));
                }
            }
        }

        જો (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_કાઢો(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* અચલ netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        જો (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_ટોચ_Scopep->createTemp("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        પાછા dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_ગાંઠ_FTask* nodep, તાર્કિક ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_ગાંઠ::user2_Clear_વૃક્ષ();
        AstVar* rtnvarp = nullptr;
        જો (nodep->isFunction()) {
            AstVar* અચલ portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            જો (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* અચલ bdtypep = portp->dtypep()->basicp();
                જો (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(સાચું)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા nullptr;
                }
                જો (!bdtypep->isDpiPrimitive()) {
                    જો (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    જો (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } નહીંતર જો (nodep->taskPublic()) {
                જો (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            જો (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(ખોટું);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, ખોટું);
            rtnvarp = portp;
            rtnvarp->funcLocal(સાચું);
            rtnvarp->noCReset(સાચું);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            જો (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(ખોટું);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = nullptr;
        જો (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            જો (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                જો (rtnvarp) VL_DO_DANGLING(ધકેલો_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        જો (rtnvarp) {
            rtnvscp = નવું AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        જો (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } નહીંતર જો (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } નહીંતર જો (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        જો (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        અચલ string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* અચલ cfuncp = નવું AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->taskPublic() && rtnvarp) ? rtnvarp->cPubArgType(સાચું, સાચું) : "")};
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
        જો (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(સાચું);
            cfuncp->isLoose(સાચું);
        } નહીંતર {
            cfuncp->isStatic(ખોટું);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        જો (nodep->name() == "new") cfuncp->isConstructor(સાચું);
        જો (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(સાચું);
        જો (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        જો (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        અચલ તાર્કિક needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        જો (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        જો (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            જો (cfuncp->isConstructor()) {
                તાર્કિક isInterfaceClass = ખોટું;
                જો (અચલ AstClass* અચલ classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                જો (!isInterfaceClass) {
                    અચલ string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        નવું AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        જો (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_આગત_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_આગત_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_આગત_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        જો (nodep->dpiExport()) {
            AstScopeName* અચલ snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(સાચું);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        માટે (Ast_ગાંઠ *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                જો (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    ધકેલો_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } નહીંતર {
                    જો (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, ખોટું);
                        portp->funcLocal(સાચું);
                        જો (portp->valuep()) ધકેલો_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        જો (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_ગાંઠ_DType* અચલ dtypep = portp->dtypep()->skipRefp();
                            જો (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } નહીંતર {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(સાચું);
                    }
                    AstVarScope* અચલ newvscp
                        = નવું AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        જો (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_ગાંઠ* bodysp = nodep->stmtsp();
        જો (bodysp) {
            unlinkAndClone(nodep, bodysp, સાચું);
            AstBegin* અચલ tempp
                = નવું AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, ખોટું};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            માટે (Ast_ગાંઠ *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                    જો (portp->isIO()) VL_DO_DANGLING(ધકેલો_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            જો (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(tempp->delete_વૃક્ષ(), tempp);
        }
        જો (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        જો (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(નવું AstCReturn{
                rtnvscp->fileline(), નવું AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        જો (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            તાર્કિક writesNonLocals = ખોટું;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* અચલ varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                જો (varp->isFuncLocal()) પાછા;
                // Check if written
                જો (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = સાચું;
                }
                // Check if read
                જો (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            જો (writesNonLocals) {
                AstVarScope* અચલ dpiExportTriggerp = getDpiExporTrigger();
                File_લીટી* અચલ flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* અચલ assignp
                    = નવું AstAssign{flp, નવું AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    નવું AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                જો (v3Global.opt.systemC()) {
                    assignp->addNextHere(નવું AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        જો (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        પાછા cfuncp;
    }

    શૂન્ય iterateIntoFTask(Ast_ગાંઠ_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    શૂન્ય ઉમેરો_Before_Stmt(Ast_ગાંઠ* nodep, Ast_ગાંઠ* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->addHereThisAsNext(newp);
    }
    શૂન્ય processArgs(Ast_ગાંઠ_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        માટે (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* અચલ arrayp = VN_CAST(argp->exprp(), ConsArray);
            જો (!arrayp) ચાલુ;

            File_લીટી* અચલ flp = arrayp->fileline();
            AstVarScope* અચલ substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(સાચું);

            AstAssign* અચલ assignp
                = નવું AstAssign{flp, નવું AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_પદાવલિ_Stmt* અચલ exprstmtp = નવું Ast_પદાવલિ_Stmt{
                flp, assignp, નવું AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    તાર્કિક isIfaceLocalImpure(Ast_ગાંઠ_FTask* nodep, Ast_ગાંઠ* impurep) {
        અચલ AstScope* અચલ scopep = m_statep->getScope(nodep);
        જો (!TaskStateVisitor::isIfaceFTaskScope(scopep)) પાછા ખોટું;

        અચલ AstVarRef* અચલ refp = VN_CAST(impurep, VarRef);
        જો (!refp || !refp->varScopep()) પાછા ખોટું;

        પાછા refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    શૂન્ય visit(AstWith* nodep) override {
        જો (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            પાછા;
        }
        Ast_ગાંઠ_પદાવલિ* અચલ withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), ગાંઠ_પદાવલિ);
        nodep->addExprp(નવું AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    શૂન્ય visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        જો (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            પાછા;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        જો (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_ગણતરી(), nodep, "No non-inline task, but no error issued");
            જો (VN_IS(nodep->backp(), Stmt_પદાવલિ)) {
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            } નહીંતર {
                nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
            પાછા;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        જો (nodep->taskp()->isFunction()) {
            AstVar* અચલ fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            જો (AstAssign* અચલ assignp = VN_CAST(nodep->backp(), Assign)) {
                જો (AstVarRef* અચલ lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* અચલ vscp = lhsp->varScopep();
                    જો (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_વૃક્ષ(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            જો (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_ગાંઠ* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        અચલ તાર્કિક virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        જો (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } નહીંતર {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        જો (VN_IS(nodep, New)) {  // New not legal as while() condition
            ઉમેરો_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (VN_IS(nodep->backp(), ગાંઠ_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            ઉમેરો_Before_Stmt(nodep, beginp);
            AstVarRef* અચલ outrefp = નવું AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (!VN_IS(nodep->backp(), Stmt_પદાવલિ)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* અચલ outrefp = નવું AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_પદાવલિ_Stmt* lambdap = નવું Ast_પદાવલિ_Stmt{nodep->fileline(), beginp, outrefp};

            જો (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_વૃક્ષ(ખોટું);
                newvarp->funcLocal(સાચું);

                // Create a new VarScope for the new variable
                AstVarScope* અચલ newvscp
                    = નવું AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreachAndNext([outvscp, newvscp](AstVarRef* refp) {
                    જો (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->hasResult(ખોટું);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* અચલ exprp = નવું AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(નવું AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            VIsCached::clear_Cache_વૃક્ષ();
        } નહીંતર {  // VN_IS(nodep->backp(), StmtExpr)
            ઉમેરો_Before_Stmt(nodep, beginp);
            જો (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* અચલ nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        જો (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            પૂર્ણાંક modes = 0;
            જો (nodep->dpiImport()) modes++;
            જો (nodep->dpiExport()) modes++;
            જો (nodep->taskPublic()) modes++;
            જો (nodep->classMethod()) modes++;
            જો (v3Global.opt.protectIds() && nodep->taskPublic()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            જો (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            અચલ તાર્કિક noInline = m_statep->ftaskNoInline(nodep);
            અચલ તાર્કિક needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            જો (noInline && !nodep->classMethod() && !nodep->recursive()) {
                જો (Ast_ગાંઠ* અચલ impurep = m_statep->checkImpure(nodep)) {
                    જો (!isIfaceLocalImpure(nodep, impurep)) {
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

            જો (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_ગાંઠ_FTask* અચલ clonedFuncp = nodep->clone_વૃક્ષ(ખોટું);
                જો (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                અચલ તાર્કિક makeNonInlineFunc = noInline || needsNonInlineCFunc;
                જો (AstCFunc* અચલ cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->addNextHere(cfuncp);
                    જો (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            જો (nodep->isFunction()) {
                જો (AstVar* અચલ portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* અચલ vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ધકેલો_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            માટે (Ast_ગાંઠ *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* અચલ vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ધકેલો_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_ગાંઠ_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_ગાંઠ_FTask_Ref* refp) { refs.ધકેલો_back(refp); });
        માટે (Ast_ગાંઠ_FTask_Ref* અચલ refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* અચલ interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* અચલ assignp = નવું AstAssign{
                nodep->fileline(), નવું AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_વૃક્ષ_Pure(ખોટું)};
            Ast_Sen_વૃક્ષ* અચલ senTreep = નવું Ast_Sen_વૃક્ષ{
                nodep->fileline(), નવું AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* અચલ activep
                = નવું AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_વૃક્ષ_Storep(activep->sentreep());
            activep->addStmtsp(
                નવું AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(નવું AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_વૃક્ષ(), refp);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        જો (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = સાચું;
        iterateChildren(nodep);
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
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

વર્ગ TaskIfaceSensVisitor final : જાહેર VNVisitorConst {
    // STATE
    તાર્કિક m_underIfaceFunc = ખોટું;  // Under a CFunc owned by an interface scope
    તાર્કિક m_underSenses = ખોટું;  // Under a sensitivity expression of such a CFunc

    // METHODS
    શૂન્ય markSensesAndIterate(Ast_ગાંઠ* nodep) {
        જો (!m_underIfaceFunc) પાછા;
        VL_RESTORER(m_underSenses);
        m_underSenses = સાચું;
        iterateAndNextConstNull(nodep);
    }
    // VISITORS
    શૂન્ય visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_Sen_વૃક્ષ* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    શૂન્ય visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        જો (!m_underSenses) પાછા;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        જો (nodep->varp()->isFuncLocal()) પાછા;
        જો (AstIface* અચલ ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = મૂળભૂત;
};

//######################################################################
// Task class functions

અચલ અક્ષર* અચલ V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_ગાંઠ_FTask_Ref* nodep, Ast_ગાંઠ* taskStmtsp,
                                    V3TaskConnectState* statep, તાર્કિક makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<અચલ std::string, પૂર્ણાંક> nameToIndex;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    પૂર્ણાંક tpinnum = 0;
    AstVar* sformatp = nullptr;
    માટે (Ast_ગાંઠ* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
            જો (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                nameToIndex.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                જો (portp->attrSFormat()) {
                    sformatp = portp;
                } નહીંતર જો (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    પૂર્ણાંક ppinnum = 0;
    તાર્કિક reorganize = ખોટું;
    માટે (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        જો (!argp->name().empty()) {
            // By name
            અચલ auto it = nameToIndex.find(argp->name());
            જો (it == nameToIndex.end()) {
                જો (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_વૃક્ષ(), argp);
                }
            } નહીંતર {
                જો (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_વૃક્ષ();
                }
                tconnects[it->second].second = argp;
                reorganize = સાચું;
            }
        } નહીંતર {  // By pin number
            જો (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } નહીંતર જો (ppinnum >= tpinnum) {
                જો (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } નહીંતર જો (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_વૃક્ષ(), argp);
                }
            } નહીંતર {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    જો (!makeChanges) પાછા tconnects;

    // Connect missing ones
    VInsertionSet<અચલ AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    માટે (પૂર્ણાંક i = 0; i < tpinnum; ++i) {
        AstVar* અચલ portp = tconnects[i].first;
        જો (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_ગાંઠ_પદાવલિ* newvaluep = nullptr;
            જો (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = નવું AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } નહીંતર જો (AstFuncRef* અચલ funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                અચલ Ast_ગાંઠ_FTask* અચલ funcp = funcRefp->taskp();
                જો (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } નહીંતર જો (AstConst* અચલ constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            જો (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), ગાંઠ_પદાવલિ));
                જો (!VN_IS(newvaluep, Const)) {
                    જો (statep) {
                        portp->pin_અંક(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.ઉમેરો(portp);
                    } નહીંતર {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = નવું AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_વૃક્ષ(સાચું) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* અચલ newp = નવું AstArg{nodep->fileline(), portp->name(), newvaluep};
            જો (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_વૃક્ષ(),
                            tconnects[i].second = nullptr);
            }
            tconnects[i].second = newp;
            reorganize = સાચું;
        }
        જો (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } નહીંતર {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    માટે (અચલ auto& tconnect : tconnects) {
        AstArg* અચલ argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    જો (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        જ્યારે (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        માટે (પૂર્ણાંક i = 0; i < tpinnum; ++i) {
            AstArg* અચલ argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    જો (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_વૃક્ષ("-  ftref-out: ");
        માટે (પૂર્ણાંક i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    જો (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        પાછા taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    પાછા tconnects;
}

શૂન્ય V3Task::taskConnectWrap(Ast_ગાંઠ_FTask_Ref* nodep, અચલ V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             અચલ VInsertionSet<અચલ AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    માટે (અચલ AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_અંક());
    અચલ auto pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    જો (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_ગાંઠ_FTask* અચલ newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    માટે (અચલ auto& tconnect : tconnects) {
        અચલ AstVar* અચલ portp = tconnect.first;
        AstArg* અચલ argp = tconnect.second;
        જો (argWrap.exists(portp)) {  // Removed arg
            statep->ધકેલો_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_ગાંઠ_FTask* V3Task::taskConnectWrapNew(Ast_ગાંઠ_FTask* taskp, અચલ string& newname,
                                         અચલ V3TaskConnects& tconnects,
                                         અચલ VInsertionSet<અચલ AstVar*>& argWrap) {
    std::map<અચલ AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_ગાંઠ_FTask* અચલ newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->addNextHere(newTaskp);

    Ast_ગાંઠ_FTask_Ref* newCallp = nullptr;
    Ast_ગાંઠ* newCallInsertp = nullptr;
    જો (VN_IS(taskp, Func)) {
        AstVar* અચલ fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* અચલ newFVarp = fvarp->clone_વૃક્ષ(સાચું);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = નવું AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = નવું AstAssign{taskp->fileline(),
                            નવું AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } નહીંતર જો (VN_IS(taskp, Task)) {
        newCallp = નવું AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = નવું Ast_Stmt_પદાવલિ{taskp->fileline(), newCallp};
    } નહીંતર {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    માટે (અચલ auto& tconnect : tconnects) {
        AstVar* અચલ portp = tconnect.first;
        AstVar* newPortp;
        જો (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = નવું AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(સાચું);
            જો (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_વૃક્ષ();
            newTaskp->addStmtsp(newPortp);
        } નહીંતર {  // Defaulting arg
            Ast_ગાંઠ_પદાવલિ* અચલ valuep = VN_AS(portp->valuep(), ગાંઠ_પદાવલિ);
            જો ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                અચલ VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* અચલ refp = VN_AS(valuep->clone_વૃક્ષ(ખોટું), VarRef);
                refp->access(refAccess);
                AstArg* અચલ newArgp = નવું AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                ચાલુ;
            }
            // Create local temporary
            newPortp = નવું AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(સાચું);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            જો (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* અચલ newAssignp
                    = નવું AstAssign{valuep->fileline(),
                                    નવું AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_વૃક્ષ(સાચું)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* અચલ newArgp = નવું AstArg{
            portp->fileline(), portp->name(),
            નવું AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        અચલ auto it = oldNewVars.find(refp->varp());
        જો (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    પાછા newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, તાર્કિક isPtr, અચલ string& frSuffix,
                                   અચલ string& toSuffix, અચલ string& frPrefix) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    અચલ string frName = frPrefix + portp->name() + frSuffix;
    અચલ string toName = portp->name() + toSuffix;
    અચલ string idx = portp->name() + "__Vidx";
    size_t unpackSize = 1;  // non-unpacked array is treated as size 1
    પૂર્ણાંક unpackDim = 0;
    જો (AstUnpackArrayDType* અચલ unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpackSize = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(ખોટું).second;
        જો (unpackDim > 0) UASSERT_OBJ(unpackSize > 0, portp, "size must be greater than 0");
    }
    જો (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        અચલ તાર્કિક isBit = portp->basicp()->isDpiBitVec();
        અચલ તાર્કિક needsFor = unpackSize > 1;
        જો (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpackSize) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        જો (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_શબ્દો()) + " * " + idx;
        }
        stmt += ", ";
        જો (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            માટે (પૂર્ણાંક i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } નહીંતર {
            stmt += frName + ")";
        }
    } નહીંતર {
        અચલ તાર્કિક isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        અચલ તાર્કિક isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        અચલ string unpackAt = unpackSize > 1 ? "[" + idx + "]" : "[0]";
        જો (unpackDim > 0) {
            અચલ string forStmt = unpackSize > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpackSize) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } નહીંતર {
            જો (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        જો (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        જો (unpackDim > 0) {
            stmt += "(&" + frName;
            માટે (પૂર્ણાંક i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } નહીંતર {
            stmt += frName;
        }
        જો (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    પાછા stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(અચલ std::string& lhsName, AstVar* varp) {
    std::string cvt;
    તાર્કિક useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    અચલ std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    અચલ પૂર્ણાંક total = strides.empty() ? 1 : strides[0].first * strides[0].second;
    અચલ પૂર્ણાંક width_શબ્દો = varp->basicp()->width_શબ્દો();
    string statements;
    માટે (પૂર્ણાંક i = 0; i < total; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        માટે (અચલ auto& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        જો (useSvVec) {
            અચલ std::string offset = std::to_string(i * width_શબ્દો);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } નહીંતર {
            અચલ std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    પાછા statements;
}

શૂન્ય V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        અચલ TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_વૃક્ષ("task", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
