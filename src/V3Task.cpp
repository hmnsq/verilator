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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Task.h"

#समावेश "V3Const.h"
#समावेश "V3ConstPool.h"
#समावेश "V3Control.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3Graph.h"
#समावेश "V3LanguageWords.h"
#समावेश "V3SharedTmps.h"
#समावेश "V3Stats.h"

#समावेश <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

श्रेणी TaskBaseVertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_गाँठ* m_impurep = nullptr;  // Node causing impure function w/ outside references
    bool m_noInline = false;  // Marked with pragma
खुला:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = शेष;
    bool pure() स्थिर { वापस m_impurep == nullptr; }
    Ast_गाँठ* impure_गाँठ() स्थिर { वापस m_impurep; }
    व्योम impure(Ast_गाँठ* nodep) { m_impurep = nodep; }
    bool noInline() स्थिर { वापस m_noInline; }
    व्योम noInline(bool flag) { m_noInline = flag; }
};

श्रेणी TaskFTaskVertex final : खुला TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_गाँठ_FTask* स्थिर m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    bool m_needsNonInlineCFunc = false;  // Needs CFunc for selected non-inlined call sites

खुला:
    TaskFTaskVertex(V3Graph* graphp, Ast_गाँठ_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = शेष;
    Ast_गाँठ_FTask* nodep() स्थिर VL_MT_STABLE { वापस m_nodep; }
    string name() स्थिर override VL_MT_STABLE { वापस nodep()->name(); }
    string dotColor() स्थिर override { वापस pure() ? "black" : "red"; }
    AstCFunc* cFuncp() स्थिर { वापस m_cFuncp; }
    व्योम cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    bool needsNonInlineCFunc() स्थिर { वापस m_needsNonInlineCFunc; }
    व्योम needsNonInlineCFunc(bool flag) { m_needsNonInlineCFunc = flag; }
};

श्रेणी TaskCodeVertex final : खुला TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
खुला:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = शेष;
    string name() स्थिर override VL_MT_STABLE { वापस "*CODE*"; }
    string dotColor() स्थिर override { वापस "green"; }
};

श्रेणी TaskEdge final : खुला V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
खुला:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* शीर्ष)
        : V3GraphEdge{graphp, fromp, शीर्ष, 1, false} {}
    ~TaskEdge() override = शेष;
    string dotLabel() स्थिर override { वापस "w" + cvtToStr(weight()); }
};

//######################################################################

श्रेणी TaskStateVisitor final : खुला VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    स्थिर VNUser3InUse m_inuser3;
    स्थिर VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<स्थिर Ast_गाँठ_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_गाँठ_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    bool m_underPortVar = false;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

खुला:
    // METHODS
    अटल bool isVirtualIfaceMethodCall(स्थिर Ast_गाँठ_FTask_Ref* refp) {
        स्थिर AstMethodCall* स्थिर mrefp = VN_CAST(refp, MethodCall);
        अगर (!mrefp) वापस false;
        स्थिर Ast_गाँठ_व्यंजक* स्थिर fromp = mrefp->fromp();
        अगर (!fromp || !fromp->dtypep()) वापस false;
        स्थिर AstIfaceRefDType* स्थिर ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        वापस ifrefp && ifrefp->isVirtual();
    }
    अटल bool isIfaceFTaskScope(स्थिर AstScope* scopep) {
        वापस scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_गाँठ_FTask* nodep) {
        AstScope* स्थिर scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        वापस scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        स्थिर स्वतः iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        वापस iter->second;
    }
    AstClass* getClassp(Ast_गाँठ_FTask* nodep) {
        AstClass* स्थिर classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        वापस classp;
    }
    व्योम remapFuncClassp(Ast_गाँठ_FTask* nodep, Ast_गाँठ_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    bool ftaskNoInline(Ast_गाँठ_FTask* nodep) { वापस getFTaskVertex(nodep)->noInline(); }
    bool ftaskNeedsNonInlineCFunc(Ast_गाँठ_FTask* nodep) {
        वापस getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_गाँठ_FTask* nodep) { वापस getFTaskVertex(nodep)->cFuncp(); }
    व्योम ftaskCFuncp(Ast_गाँठ_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_गाँठ* checkImpure(Ast_गाँठ_FTask* nodep) { वापस getFTaskVertex(nodep)->impure_गाँठ(); }

निजी:
    TaskFTaskVertex* getFTaskVertex(Ast_गाँठ_FTask* nodep) {
        अगर (!nodep->user4p()) nodep->user4p(नया TaskFTaskVertex{&m_callGraph, nodep});
        वापस static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    व्योम propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        क्रम (V3GraphVertex& vtx : m_callGraph.vertices()) {
            अगर (TaskFTaskVertex* स्थिर ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                अगर (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        जबतक (!workList.empty()) {
            TaskFTaskVertex* स्थिर calleep = workList.back();
            workList.निकालो_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            क्रम (V3GraphEdge& edge : calleep->inEdges()) {
                अगर (TaskFTaskVertex* स्थिर callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    अगर (!callerp->pure()) जारी;  // Already processed or on workList
                    callerp->impure(calleep->impure_गाँठ());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    व्योम decideInlining() {
        क्रम (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* स्थिर ftVtxp = vtx.cast<TaskFTaskVertex>();
            अगर (!ftVtxp) जारी;
            // Move on if already decided not to inline
            अगर (ftVtxp->noInline()) जारी;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            अगर (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(true);
                जारी;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            अगर (v3Global.opt.fInlineFuncsEager()) जारी;

            Ast_गाँठ_FTask* स्थिर ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            अगर (!ftaskp->verilogFunction()) जारी;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            अगर (ftaskp->taskPublic()) जारी;
            // If the function is pure, it's OK not to inline, so don't do it
            अगर (ftVtxp->pure()) ftVtxp->noInline(true);
        }
    }

    // VISITORS
    व्योम visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        क्रम (Ast_गाँठ* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVarScope* स्थिर vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* स्थिर varp = vscp->varp();
                अगर (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        क्रम (Ast_गाँठ* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (Ast_गाँठ_FTask* स्थिर taskp = VN_CAST(stmtp, गाँठ_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* स्थिर taskVtxp = getFTaskVertex(nodep->taskp());
        नया TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        अगर (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(true);
        // Do we have to disable inlining the function?
        स्थिर V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        अगर (!taskVtxp->noInline()) {  // Else short-circuit below
            क्रम (स्थिर स्वतः& itr : tconnects) {
                स्थिर AstVar* स्थिर portp = itr.first;
                स्थिर AstArg* स्थिर argp = itr.second;
                अगर (स्थिर Ast_गाँठ_व्यंजक* स्थिर pinp = argp->exprp()) {
                    अगर ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(true);
                    }
                }
            }
        }
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        अगर (nodep->dpiImport()) m_curVxp->noInline(true);
        अगर (nodep->classMethod()) m_curVxp->noInline(true);  // Until V3Task supports it
        अगर (nodep->recursive()) m_curVxp->noInline(true);
        अगर (nodep->isConstructor()) {
            m_curVxp->noInline(true);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstPragma* nodep) override {
        अगर (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    व्योम visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        अगर (nodep->isIO()) m_underPortVar = true;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    व्योम visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        अगर (m_underPortVar) वापस;
        AstVar* स्थिर varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        अगर (nodep->access().isReadOnly() && varp->constPoolEntry()) वापस;
        अगर (varp->user4u().toGraphVertex() != m_curVxp) {
            अगर (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    व्योम visit(AstClass* nodep) override {
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
        Ast_गाँठ* insertp = nullptr;
        क्रम (AstInitialAutomatic* initialp : m_initialps) {
            अगर (Ast_गाँठ* स्थिर movep = initialp->stmtsp()) {
                movep->unlink_Fr_Back_With_अग्र();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_गाँठ::add_अग्र_Null(insertp, movep);
            }
            VL_DO_DANGLING(धकेलो_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.सफा();
        m_ctorp->prependStmtsp(insertp);
    }
    व्योम visit(AstInitialAutomatic* nodep) override {
        m_initialps.धकेलो_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = नया TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.remove_Redundant_Edges_योग(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        अगर (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = शेष;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

काष्ठा TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    अटल std::vector<std::pair<पूर्णांक, पूर्णांक>> unpackDimsAndStrides(AstVar* varp) {
        Ast_गाँठ_DType* स्थिर dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<पूर्णांक, पूर्णांक>> dimStrides;
        AstUnpackArrayDType* स्थिर unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        अगर (!unpackp) वापस dimStrides;

        स्थिर std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        स्थिर माप_प्रकार nDims = dims.आकार();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        क्रम (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        वापस dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    अटल std::pair<std::string, bool> dpiToInternalCvtStmt(स्थिर Ast_गाँठ_DType* dtypep) {
        स्थिर AstBasicDType* स्थिर basicp = dtypep->basicp();

        // DPI types using Primitive C types
        अगर (basicp->keyword() == VBasicDTypeKwd::CHANDLE) वापस {"VL_CVT_VP_Q(", false};
        अगर (basicp->keyword() == VBasicDTypeKwd::STRING) वापस {"VL_CVT_N_CSTR(", false};
        अगर (basicp->isDpiPrimitive()) वापस {"(", false};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        स्थिर std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        स्थिर अक्षर आकार_Char = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        स्थिर std::string& आकार = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        वापस {"VL_SET_"s + आकार_Char + "_" + vecType + "(" + आकार + ", ", true};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

श्रेणी TaskVisitor final : खुला VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<स्थिर string, std::tuple<Ast_गाँठ_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* स्थिर m_statep;  // Common state between visitors
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    Ast_शीर्ष_Scope* स्थिर m_शीर्ष_Scopep = v3Global.rootp()->शीर्ष_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_गाँठ* m_insStmtp = nullptr;  // Where to insert statement
    bool m_inSensesp = false;  // Are we under a senitem?
    bool m_inNew = false;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    पूर्णांक m_uncon_Var_अंक = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, स्थिर AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, स्थिर string& name, AstVar* examplep) {
        AstVar* स्थिर newvarp = नया AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(true);
        funcp->addVarsp(newvarp);
        AstVarScope* स्थिर newvscp = नया AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        वापस newvscp;
    }
    AstVarScope* create_निवेश_Var(AstCFunc* funcp, स्थिर string& name, VBasicDTypeKwd kwd) {
        AstVar* स्थिर newvarp
            = नया AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(true);
        newvarp->direction(VDirection::निवेश);
        funcp->addArgsp(newvarp);
        AstVarScope* स्थिर newvscp = नया AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        वापस newvscp;
    }
    अटल AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* स्थिर refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* स्थिर vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_वृक्ष(), refp);
        वापस vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(स्थिर Ast_गाँठ_FTask_Ref* refp, AstVar* invarp,
                                स्थिर std::string& suffix) {
        अगर (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            वापस constPoolTable(invarp);
        } अन्यथा {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* स्थिर newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* स्थिर newvarp = newvscp->varp();
            newvarp->funcLocal(false);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(true);
            वापस newvscp;
        }
    }

    // Replace varrefs with new var pointer
    व्योम relink(Ast_गाँठ* nodep) {
        nodep->foreach_And_अग्र([](AstVarRef* refp) {
            अगर (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* स्थिर newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_गाँठ_व्यंजक* pinp, AstVarScope* newvscp, bool pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_गाँठ_व्यंजक* inPinp = pinp;
        अगर (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_गाँठ_व्यंजक* inPinClonep
            = pureCheck ? inPinp->clone_वृक्ष_Pure(true) : inPinp->clone_वृक्ष(true);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        स्थिर AstUnpackArrayDType* स्थिर arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        अगर (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = नया AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* स्थिर assp = नया AstAssign{
            pinp->fileline(), नया AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, true);  // Ok if in <= block
        वापस assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_गाँठ_व्यंजक* pinp, AstVarScope* newvscp,
                                        bool pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_गाँठ_व्यंजक* outPinp = pinp;
        Ast_गाँठ_व्यंजक* postRhsp = नया AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        अगर (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            अगर (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, गाँठ_Uniop)->lhsp();
                अगर (postRhsp->isSigned()) {
                    postRhsp = नया AstISToRD{pinp->fileline(), postRhsp};
                } अन्यथा {
                    postRhsp = नया AstIToRD{pinp->fileline(), postRhsp};
                }
            } अन्यथा {
                अगर (Ast_गाँठ_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } अन्यथा अगर (Ast_गाँठ_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } अन्यथा अगर (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } अन्यथा {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                अगर (outPinp->width() < portp->width()) {
                    postRhsp = नया AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } अन्यथा {  // pin width > port width
                    अगर (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = नया AstExtendS{pinp->fileline(), postRhsp};
                    } अन्यथा {
                        postRhsp = नया AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        अगर (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, गाँठ_Uniop)->lhsp();
            postRhsp = नया AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_गाँठ_व्यंजक* स्थिर outPinClonep
            = pureCheck ? outPinp->clone_वृक्ष_Pure(true) : outPinp->clone_वृक्ष(true);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        स्थिर AstUnpackArrayDType* स्थिर arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        अगर (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = नया AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* स्थिर assp = नया AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, true);  // Ok if in <= block
        वापस assp;
    }

    व्योम changeAtWriteRecurse(Ast_गाँठ_व्यंजक* स्थिर exprp) {
        // Change nested at methods to writable variant
        अगर (AstCMethodHard* स्थिर cMethodp = VN_CAST(exprp, CMethodHard)) {
            अगर (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    व्योम connectPort(स्थिर Ast_गाँठ_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_गाँठ* beginp,
                     bool inlineTask) {
        Ast_गाँठ_व्यंजक* pinp = argp->exprp();
        अगर (inlineTask) {
            portp->unlinkFrBack();
            धकेलो_Deletep(portp);  // Remove it from the clone (not original)
        }
        अगर (!pinp) {
            // Too few arguments in function call
        } अन्यथा {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            अगर (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* स्थिर varp = नया AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_अंक++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* स्थिर newvscp = नया AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* स्थिर repp = नया AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(धकेलो_Deletep(pinp), pinp);
                pinp = repp;
            }
            अगर (inlineTask) {
                धकेलो_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्ष(), argp);  // Args no longer needed
            }
            अगर (portp->isRef() || portp->isConstRef()) {
                bool refArgOk = false;
                अगर (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = true;
                } अन्यथा अगर (AstCMethodHard* स्थिर cMethodp = VN_CAST(pinp, CMethodHard)) {
                    अगर (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } अन्यथा {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                अगर (refArgOk) {
                    अगर (AstVarRef* स्थिर varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } अन्यथा {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                अगर (inlineTask) {
                    अगर (AstVarRef* स्थिर varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* स्थिर localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } अन्यथा {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } अन्यथा अगर (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* स्थिर newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                अगर (!inlineTask) {
                    pinp->replaceWith(
                        नया AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    धकेलो_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* स्थिर preassp = connectPortMakeInAssign(pinp, newvscp, true);
                beginp->add_अग्र_Here(preassp);

                AstAssign* स्थिर postassp = connectPortMakeOutAssign(portp, pinp, newvscp, true);
                beginp->add_अग्र(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } अन्यथा अगर (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* स्थिर newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                अगर (!inlineTask) {
                    pinp->replaceWith(नया AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    धकेलो_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* स्थिर postassp = connectPortMakeOutAssign(portp, pinp, newvscp, false);
                // Put assignment BEHIND of all other statements
                beginp->add_अग्र(postassp);
            } अन्यथा अगर (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* स्थिर newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* स्थिर preassp = connectPortMakeInAssign(pinp, newvscp, false);
                // Put assignment in FRONT of all other statements
                beginp->add_अग्र_Here(preassp);
            }
        }
    }

    bool hasRefArgument(Ast_गाँठ_FTask* nodep) {
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (varp->isRef() || varp->isConstRef()) वापस true;
            }
        }
        वापस false;
    }

    Ast_गाँठ* createInlinedFTask(Ast_गाँठ_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_गाँठ* स्थिर newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_वृक्ष(true) : nullptr;
        Ast_गाँठ* स्थिर beginp
            = नया AstComment{refp->fileline(), "Function: "s + refp->name(), true};
        अगर (newbodysp) beginp->add_अग्र(newbodysp);
        अगर (debug() >= 9) beginp->dump_वृक्ष_And_अग्र(अ_बाहर, "-  newbegi: ");
        //
        // Create input variables
        Ast_गाँठ::user2_Clear_वृक्ष();
        {
            स्थिर V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            क्रम (स्थिर स्वतः& itr : tconnects) {
                AstVar* स्थिर portp = itr.first;
                AstArg* स्थिर argp = itr.second;
                connectPort(refp, portp, argp, beginp, true);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_गाँठ* nextstmtp;
            क्रम (Ast_गाँठ* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    अगर (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* स्थिर localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        अगर (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_गाँठ* स्थिर crstp = नया AstAssign{
                                portp->fileline(),
                                नया AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                नया AstCReset{portp->fileline(), portp, false}};
                            portp->replaceWith(crstp);
                        } अन्यथा {
                            portp->unlinkFrBack();
                        }
                        धकेलो_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        अगर (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        अगर (debug() >= 9) beginp->dump_वृक्ष_And_अग्र(अ_बाहर, "-  iotask: ");
        वापस beginp;
    }

    Ast_गाँठ* createNonInlinedFTask(Ast_गाँठ_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* स्थिर cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_गाँठ* स्थिर beginp
            = नया AstComment{refp->fileline(), "Function: "s + refp->name(), true};
        Ast_गाँठ_CCall* ccallp;
        अगर (VN_IS(refp, New)) {
            AstCNew* स्थिर cnewp = नया AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } अन्यथा अगर (स्थिर AstMethodCall* स्थिर mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = नया AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->add_अग्र(ccallp->makeStmt());
        } अन्यथा {
            ccallp = नया AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->add_अग्र(ccallp->makeStmt());
        }

        अगर (स्थिर AstFuncRef* स्थिर funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } अन्यथा अगर (स्थिर AstTaskRef* स्थिर taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } अन्यथा अगर (स्थिर AstMethodCall* स्थिर methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            स्थिर V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            क्रम (स्थिर स्वतः& itr : tconnects) {
                AstVar* स्थिर portp = itr.first;
                AstArg* स्थिर argp = itr.second;
                connectPort(refp, portp, argp, beginp, false);
            }
        }
        // First argument is symbol table, then output if a function
        स्थिर bool needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        अगर (needSyms) ccallp->argTypes("vlSymsp");

        अगर (refp->taskp()->dpiContext()) {
            AstScopeName* स्थिर snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_पंक्ति* स्थिर flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                नया AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(नया AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        क्रम (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        अगर (outvscp) ccallp->addArgsp(नया AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        अगर (debug() >= 9) beginp->dump_वृक्ष_And_अग्र(अ_बाहर, "-  nitask: ");
        वापस beginp;
    }

    string dpiSignature(Ast_गाँठ_FTask* nodep, AstVar* rtnvarp) स्थिर {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        अगर (nodep->dpiPure()) dpiproto += "pure ";
        अगर (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(true, true) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (स्थिर AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                अगर (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    अगर (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    स्थिर std::string dpiType = portp->dpiArgType(false, false);
                    dpiproto += dpiType;
                    स्थिर std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(false);
                    अगर (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        वापस dpiproto;
    }

    अटल व्योम checkLegalCIdentifier(Ast_गाँठ* nodep, स्थिर string& name) {
        अगर (name.end() != std::find_if(name.begin(), name.end(), [](अक्षर c) {
                वापस !std::है_अक्षर_अंक(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    अटल Ast_गाँठ* create_Dpi_अस्थायी(AstVar* portp, स्थिर string& suffix) {
        स्थिर string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        वापस नया AstCStmt{portp->fileline(), stmt};
    }

    व्योम unlinkAndClone(Ast_गाँठ_FTask* funcp, Ast_गाँठ* nodep, bool with_अग्र) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        अगर (funcp->recursive()) {
            VNRelinker relinkHandle;
            अगर (with_अग्र) {
                nodep->unlink_Fr_Back_With_अग्र(&relinkHandle);
            } अन्यथा {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_गाँठ* स्थिर newp = nodep->clone_वृक्ष(with_अग्र);
            relinkHandle.relink(newp);
        } अन्यथा {
            अगर (with_अग्र) {
                nodep->unlink_Fr_Back_With_अग्र();
            } अन्यथा {
                nodep->unlinkFrBack();
            }
        }
    }

    अटल Ast_गाँठ* createAssignInternalToDpi(AstVar* portp, bool isPtr, स्थिर string& frSuffix,
                                              स्थिर string& toSuffix) {
        स्थिर string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        वापस नया AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    अटल Ast_गाँठ_Stmt* createDpiScalarCvt(Ast_गाँठ_व्यंजक* internalp, स्थिर std::string& dpi,
                                           स्थिर Ast_गाँठ_DType* dtypep, bool toDpi) {
        File_पंक्ति* स्थिर flp = internalp->fileline();
        स्थिर AstBasicDType* स्थिर basicp = dtypep->basicp();
        अगर (toDpi) {
            AstCStmt* स्थिर cstmtp = नया AstCStmt{flp};
            अगर (basicp->isDpiPrimitive()) {
                cstmtp->add(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->add(internalp);
                cstmtp->add(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } अन्यथा {
                cstmtp->add((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::to_string(dtypep->width()) + ", " + dpi + ", ");
                cstmtp->add(internalp);
                cstmtp->add(");");
            }
            वापस cstmtp;
        }
        std::string cvt;
        bool useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        अगर (useSvVec) {
            AstCStmt* स्थिर cstmtp = नया AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            वापस cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        स्थिर पूर्णांक cwidth = basicp->width_शब्दगण() * VL_EDATASIZE;
        Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstCExpr{flp, cvt + dpi + ")", cwidth};
        वापस नया AstAssign{flp, internalp, नया AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    अटल Ast_गाँठ_Stmt* createDpiCvt(Ast_गाँठ_व्यंजक* internalp, स्थिर std::string& dpi,
                                     स्थिर Ast_गाँठ_DType* dtypep, bool toDpi) {
        File_पंक्ति* स्थिर flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_गाँठ_Stmt* stmtsp = nullptr;
        अगर (स्थिर AstUnpackArrayDType* स्थिर arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            क्रम (पूर्णांक i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_गाँठ_व्यंजक* स्थिर elemp = नया AstArraySel{flp, internalp->clone_वृक्ष(false), i};
                stmtsp = Ast_गाँठ::add_अग्र(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } अन्यथा अगर (dtypep->containsUnpackedStruct()) {
            क्रम (स्थिर AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* स्थिर selp
                    = नया AstStructSel{flp, internalp->clone_वृक्ष(false), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_गाँठ::add_अग्र(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } अन्यथा {
            वापस createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_वृक्ष(), internalp);
        वापस stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    अटल Ast_गाँठ_Stmt* createDpiArgCvt(AstVarScope* vscp, स्थिर std::string& dpiName,
                                        bool toDpi) {
        AstVar* स्थिर varp = vscp->varp();
        File_पंक्ति* स्थिर flp = vscp->fileline();
        स्थिर std::vector<std::pair<पूर्णांक, पूर्णांक>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        स्थिर पूर्णांक योग = strides.empty() ? 1 : strides[0].first * strides[0].second;
        स्थिर bool isStruct = varp->dtypep()->containsUnpackedStruct();
        स्थिर Ast_गाँठ_DType* स्थिर elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_गाँठ_Stmt* stmtsp = nullptr;
        क्रम (पूर्णांक i = 0; i < योग; ++i) {
            Ast_गाँठ_व्यंजक* internalp
                = नया AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            क्रम (स्थिर स्वतः& stride : strides) {
                internalp = नया AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            अगर (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_शब्दगण());
            } अन्यथा अगर (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_गाँठ::add_अग्र(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        वापस stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_गाँठ_FTask* स्थिर nodep, AstVar* स्थिर rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_पंक्ति* स्थिर flp = nodep->fileline();
        स्थिर std::string cname = Ast_गाँठ::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        स्थिर अक्षर* स्थिर tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        स्थिर std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(true, true) : "";
        // The function we are building
        AstCFunc* स्थिर funcp = नया AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(true);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(true);
        funcp->entryPoint(true);
        funcp->isStatic(true);
        funcp->protect(false);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_शीर्ष_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        अगर (!makePortList(nodep, funcp)) वापस nullptr;

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
        AstCStmt* स्थिर prep = नया AstCStmt{flp};
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
        स्थिर std::string cbtype
            = VIdProtect::protect(v3Global.opt.prefix() + "__Vcb_" + nodep->cname() + "_t");
        prep->add(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* स्थिर callp = नया AstCStmt{flp};
        स्थिर स्वतः addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* स्थिर vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(false);
            portp->protect(false);
            // Add argument to call
            स्थिर VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(नया AstVarRef{portp->fileline(), vscp, access});
            वापस vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_गाँठ* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* स्थिर portp = VN_CAST(stmtp, Var);
            अगर (!portp) जारी;
            अगर (!portp->isIO()) जारी;
            अगर (portp->isFuncReturn()) जारी;
            अगर (portp == rtnvarp) जारी;  // Handled below
            // Add argument to call
            AstVarScope* स्थिर outvscp = addFuncArg(portp);
            स्थिर bool isStruct = portp->dtypep()->containsUnpackedStruct();
            अगर (portp->isWritable()) {
                outCvtsp = Ast_गाँठ::add_अग्र(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), true)
                                       : createAssignInternalToDpi(portp, true, tmpSuffixp, ""));
            }
            अगर (!portp->isNonOutput()) जारी;
            // Convert input/inout arguments to dpi type
            स्थिर std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(false).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), false));
        }
        // Return value argument goes last
        अगर (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        अगर (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        अगर (rtnvarp) {
            funcp->addStmtsp(create_Dpi_अस्थायी(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, false, tmpSuffixp, ""));
            स्थिर std::string सूचकांक_2 = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(नया AstCStmt{flp, "return " + rtnvarp->name() + सूचकांक_2 + ";"});
        }
        वापस funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_गाँठ_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        स्थिर string name = Ast_गाँठ::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        स्थिर string rtnType = rtnvarp            ? rtnvarp->dpiArgType(true, true)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* स्थिर funcp = नया AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(true);
        funcp->dontCombine(true);
        funcp->entryPoint(false);
        funcp->isMethod(false);
        funcp->protect(false);
        funcp->dpiPure(nodep->dpiPure());

        स्थिर पूर्णांक cost = static_cast<पूर्णांक>(V3Control::get_Profile_तथ्य(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_शीर्ष_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        अगर (!makePortList(nodep, funcp)) वापस nullptr;
        वापस funcp;
    }

    AstCFunc* getDpiFunc(Ast_गाँठ_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        स्थिर string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        स्थिर स्वतः pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        अगर (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* स्थिर funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::लो<2>(pair.first->second) = funcp;
            वापस funcp;
        } अन्यथा {
            // Seen this cname import before. Check if it's the same prototype.
            स्थिर Ast_गाँठ_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            अगर (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                वापस nullptr;
            } अन्यथा {
                // Same signature, return the previously created CFunc
                वापस firstFuncp;
            }
        }
    }

    अटल bool makePortList(Ast_गाँठ_FTask* nodep, AstCFunc* dpip) {
        bool allOk = true;
        // Copy nodep's list of function I/O to the new dpip c function
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                अगर (portp->isIO()) {
                    // Move it to new function
                    AstVar* स्थिर newPortp = portp->clone_वृक्ष(false);
                    newPortp->funcLocal(true);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    अगर (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = false;
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
        वापस allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    अटल bool isDpiCIdentifier(स्थिर string& name) {
        वापस !name.empty() && VString::isIdentifier(name)
               && !std::है_अंक(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::है_बड़ा(name[1])))
               && V3_Language_शब्दगण::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    bool checkDpiStruct(स्थिर AstVar* portp, स्थिर AstStructDType* sdtypep, bool nested) {
        स्थिर std::string name = sdtypep->dpiName();
        अगर (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(true) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            वापस false;
        }
        क्रम (स्थिर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            अगर (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                वापस false;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            स्थिर Ast_गाँठ_DType* dtypep = itemp->subDTypep()->skipRefp();
            जबतक (स्थिर AstUnpackArrayDType* स्थिर arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर subp = VN_CAST(dtypep, गाँठ_UOr_Struct_DType);
            स्थिर AstBasicDType* स्थिर basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            स्थिर bool isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            अगर (dtypep->containsUnpackedStruct()) {
                अगर (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), true)) वापस false;
            } अन्यथा अगर (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(true)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                वापस false;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        अगर (name.empty()) वापस true;
        स्थिर AstStructDType* स्थिर firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        अगर (firstp->dpiTypedef() == sdtypep->dpiTypedef()) वापस true;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        वापस false;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    bool checkDpiStructArgs(स्थिर Ast_गाँठ_FTask* nodep) {
        bool allOk = true;
        क्रम (स्थिर Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            स्थिर AstVar* स्थिर portp = VN_CAST(stmtp, Var);
            अगर (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       false)
                        && allOk;
            }
        }
        वापस allOk;
    }

    व्योम bodyDpiImportFunc(Ast_गाँठ_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        स्थिर अक्षर* स्थिर tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        अगर (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_धकेलो(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        क्रम (Ast_गाँठ* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                AstVarScope* स्थिर portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                अगर (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    अगर (args != "") args += ", ";

                    अगर (portp->isDpiOpenArray()) {
                        Ast_गाँठ_DType* स्थिर dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        स्थिर string propName = portp->name() + "__Vopenprops";
                        स्थिर string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(नया AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        स्थिर string name = portp->name() + "__Vopenarray";
                        स्थिर string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(नया AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } अन्यथा {
                        // Unpacked structures are passed by reference from a C array
                        स्थिर bool isStruct = portp->dtypep()->containsUnpackedStruct();
                        अगर (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            अगर (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(create_Dpi_अस्थायी(portp, tmpSuffixp));
                        अगर (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, true)
                                    : createAssignInternalToDpi(portp, false, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        अगर (nodep->dpiContext()) {
            स्थिर string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(नया AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* स्थिर callp = नया AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            अगर (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(create_Dpi_अस्थायी(rtnvscp->varp(), tmpSuffixp));
                स्थिर std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* स्थिर cstmtp = नया AstCStmt{nodep->fileline()};
                cstmtp->add(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->add(" = ");
                cstmtp->add(callp);  // RHS
                cstmtp->add(";");
                cfuncp->addStmtsp(cstmtp);
            } अन्यथा {
                // Othervise just call it
                cfuncp->addStmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        क्रम (Ast_गाँठ* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                portp->protect(false);  // No additional exposure - already part of shown proto
                अगर (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* स्थिर portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, false));
                }
            }
        }

        अगर (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_निकालो(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* स्थिर netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        अगर (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_शीर्ष_Scopep->create_अस्थायी("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        वापस dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_गाँठ_FTask* nodep, bool ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_गाँठ::user2_Clear_वृक्ष();
        AstVar* rtnvarp = nullptr;
        अगर (nodep->isFunction()) {
            AstVar* स्थिर portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            अगर (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* स्थिर bdtypep = portp->dtypep()->basicp();
                अगर (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(true)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस nullptr;
                }
                अगर (!bdtypep->isDpiPrimitive()) {
                    अगर (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    अगर (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } अन्यथा अगर (nodep->taskPublic()) {
                अगर (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            अगर (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(false);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, false);
            rtnvarp = portp;
            rtnvarp->funcLocal(true);
            rtnvarp->noCReset(true);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            अगर (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(false);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = nullptr;
        अगर (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            अगर (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                अगर (rtnvarp) VL_DO_DANGLING(धकेलो_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        अगर (rtnvarp) {
            rtnvscp = नया AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        अगर (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } अन्यथा अगर (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } अन्यथा अगर (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        अगर (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        स्थिर string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* स्थिर cfuncp = नया AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->taskPublic() && rtnvarp) ? rtnvarp->cPubArgType(true, true) : "")};
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
        अगर (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(true);
            cfuncp->isLoose(true);
        } अन्यथा {
            cfuncp->isStatic(false);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        अगर (nodep->name() == "new") cfuncp->isConstructor(true);
        अगर (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(true);
        अगर (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        अगर (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        स्थिर bool needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        अगर (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        अगर (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            अगर (cfuncp->isConstructor()) {
                bool isInterfaceClass = false;
                अगर (स्थिर AstClass* स्थिर classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                अगर (!isInterfaceClass) {
                    स्थिर string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        नया AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        अगर (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_निवेश_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_निवेश_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_निवेश_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        अगर (nodep->dpiExport()) {
            AstScopeName* स्थिर snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(true);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        क्रम (Ast_गाँठ *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                अगर (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    धकेलो_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } अन्यथा {
                    अगर (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, false);
                        portp->funcLocal(true);
                        अगर (portp->valuep()) धकेलो_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        अगर (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_गाँठ_DType* स्थिर dtypep = portp->dtypep()->skipRefp();
                            अगर (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } अन्यथा {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(true);
                    }
                    AstVarScope* स्थिर newvscp
                        = नया AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        अगर (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_गाँठ* bodysp = nodep->stmtsp();
        अगर (bodysp) {
            unlinkAndClone(nodep, bodysp, true);
            AstBegin* स्थिर tempp
                = नया AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, false};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            क्रम (Ast_गाँठ *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                    अगर (portp->isIO()) VL_DO_DANGLING(धकेलो_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            अगर (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlink_Fr_Back_With_अग्र());
            VL_DO_DANGLING(tempp->delete_वृक्ष(), tempp);
        }
        अगर (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        अगर (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(नया AstCReturn{
                rtnvscp->fileline(), नया AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        अगर (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            bool writesNonLocals = false;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* स्थिर varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                अगर (varp->isFuncLocal()) वापस;
                // Check if written
                अगर (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = true;
                }
                // Check if read
                अगर (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            अगर (writesNonLocals) {
                AstVarScope* स्थिर dpiExportTriggerp = getDpiExporTrigger();
                File_पंक्ति* स्थिर flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* स्थिर assignp
                    = नया AstAssign{flp, नया AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    नया AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                अगर (v3Global.opt.systemC()) {
                    assignp->add_अग्र_Here(नया AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        अगर (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        वापस cfuncp;
    }

    व्योम iterateIntoFTask(Ast_गाँठ_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    व्योम जोड़ो_Before_Stmt(Ast_गाँठ* nodep, Ast_गाँठ* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->add_Here_This_As_अग्र(newp);
    }
    व्योम processArgs(Ast_गाँठ_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        क्रम (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* स्थिर arrayp = VN_CAST(argp->exprp(), ConsArray);
            अगर (!arrayp) जारी;

            File_पंक्ति* स्थिर flp = arrayp->fileline();
            AstVarScope* स्थिर substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(true);

            AstAssign* स्थिर assignp
                = नया AstAssign{flp, नया AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_व्यंजक_Stmt* स्थिर exprstmtp = नया Ast_व्यंजक_Stmt{
                flp, assignp, नया AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    bool isIfaceLocalImpure(Ast_गाँठ_FTask* nodep, Ast_गाँठ* impurep) {
        स्थिर AstScope* स्थिर scopep = m_statep->getScope(nodep);
        अगर (!TaskStateVisitor::isIfaceFTaskScope(scopep)) वापस false;

        स्थिर AstVarRef* स्थिर refp = VN_CAST(impurep, VarRef);
        अगर (!refp || !refp->varScopep()) वापस false;

        वापस refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    व्योम visit(AstWith* nodep) override {
        अगर (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            वापस;
        }
        Ast_गाँठ_व्यंजक* स्थिर withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), गाँठ_व्यंजक);
        nodep->addExprp(नया AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    व्योम visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    व्योम visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = true;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        अगर (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            वापस;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        अगर (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_गिनती(), nodep, "No non-inline task, but no error issued");
            अगर (VN_IS(nodep->backp(), Stmt_व्यंजक)) {
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            } अन्यथा {
                nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            }
            वापस;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        अगर (nodep->taskp()->isFunction()) {
            AstVar* स्थिर fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            अगर (AstAssign* स्थिर assignp = VN_CAST(nodep->backp(), Assign)) {
                अगर (AstVarRef* स्थिर lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* स्थिर vscp = lhsp->varScopep();
                    अगर (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_वृक्ष(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            अगर (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_गाँठ* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        स्थिर bool virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        अगर (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } अन्यथा {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        अगर (VN_IS(nodep, New)) {  // New not legal as while() condition
            जोड़ो_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा अगर (VN_IS(nodep->backp(), गाँठ_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            जोड़ो_Before_Stmt(nodep, beginp);
            AstVarRef* स्थिर outrefp = नया AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा अगर (!VN_IS(nodep->backp(), Stmt_व्यंजक)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* स्थिर outrefp = नया AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_व्यंजक_Stmt* lambdap = नया Ast_व्यंजक_Stmt{nodep->fileline(), beginp, outrefp};

            अगर (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_वृक्ष(false);
                newvarp->funcLocal(true);

                // Create a new VarScope for the new variable
                AstVarScope* स्थिर newvscp
                    = नया AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreach_And_अग्र([outvscp, newvscp](AstVarRef* refp) {
                    अगर (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->has_परिणाम(false);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* स्थिर exprp = नया AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(नया AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            VIsCached::clear_Cache_वृक्ष();
        } अन्यथा {  // VN_IS(nodep->backp(), StmtExpr)
            जोड़ो_Before_Stmt(nodep, beginp);
            अगर (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    व्योम visit(Ast_गाँठ_FTask* स्थिर nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        अगर (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            पूर्णांक modes = 0;
            अगर (nodep->dpiImport()) modes++;
            अगर (nodep->dpiExport()) modes++;
            अगर (nodep->taskPublic()) modes++;
            अगर (nodep->classMethod()) modes++;
            अगर (v3Global.opt.protectIds() && nodep->taskPublic()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            अगर (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            स्थिर bool noInline = m_statep->ftaskNoInline(nodep);
            स्थिर bool needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            अगर (noInline && !nodep->classMethod() && !nodep->recursive()) {
                अगर (Ast_गाँठ* स्थिर impurep = m_statep->checkImpure(nodep)) {
                    अगर (!isIfaceLocalImpure(nodep, impurep)) {
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

            अगर (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_गाँठ_FTask* स्थिर clonedFuncp = nodep->clone_वृक्ष(false);
                अगर (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                स्थिर bool makeNonInlineFunc = noInline || needsNonInlineCFunc;
                अगर (AstCFunc* स्थिर cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->add_अग्र_Here(cfuncp);
                    अगर (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            अगर (nodep->isFunction()) {
                अगर (AstVar* स्थिर portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* स्थिर vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(धकेलो_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            क्रम (Ast_गाँठ *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* स्थिर vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(धकेलो_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }
    व्योम visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_गाँठ_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_गाँठ_FTask_Ref* refp) { refs.धकेलो_back(refp); });
        क्रम (Ast_गाँठ_FTask_Ref* स्थिर refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* स्थिर interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* स्थिर assignp = नया AstAssign{
                nodep->fileline(), नया AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_वृक्ष_Pure(false)};
            Ast_Sen_वृक्ष* स्थिर senTreep = नया Ast_Sen_वृक्ष{
                nodep->fileline(), नया AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* स्थिर activep
                = नया AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_वृक्ष_Storep(activep->sentreep());
            activep->addStmtsp(
                नया AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(नया AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_वृक्ष(), refp);
        }
    }
    व्योम visit(Ast_गाँठ_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    व्योम visit(Ast_गाँठ_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_Stmt_व्यंजक* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        अगर (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = true;
        iterateChildren(nodep);
    }
    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
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

श्रेणी TaskIfaceSensVisitor final : खुला VNVisitorConst {
    // STATE
    bool m_underIfaceFunc = false;  // Under a CFunc owned by an interface scope
    bool m_underSenses = false;  // Under a sensitivity expression of such a CFunc

    // METHODS
    व्योम markSensesAndIterate(Ast_गाँठ* nodep) {
        अगर (!m_underIfaceFunc) वापस;
        VL_RESTORER(m_underSenses);
        m_underSenses = true;
        iterate_And_अग्र_Const_Null(nodep);
    }
    // VISITORS
    व्योम visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    व्योम visit(Ast_Sen_वृक्ष* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    व्योम visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }
    व्योम visit(AstVarRef* nodep) override {
        अगर (!m_underSenses) वापस;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        अगर (nodep->varp()->isFuncLocal()) वापस;
        अगर (AstIface* स्थिर ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = शेष;
};

//######################################################################
// Task class functions

स्थिर अक्षर* स्थिर V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_गाँठ_FTask_Ref* nodep, Ast_गाँठ* taskStmtsp,
                                    V3TaskConnectState* statep, bool makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<स्थिर std::string, पूर्णांक> name_To_सूचकांक;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    पूर्णांक tpinnum = 0;
    AstVar* sformatp = nullptr;
    क्रम (Ast_गाँठ* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
            अगर (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                name_To_सूचकांक.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                अगर (portp->attrSFormat()) {
                    sformatp = portp;
                } अन्यथा अगर (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    पूर्णांक ppinnum = 0;
    bool reorganize = false;
    क्रम (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        अगर (!argp->name().empty()) {
            // By name
            स्थिर स्वतः it = name_To_सूचकांक.find(argp->name());
            अगर (it == name_To_सूचकांक.end()) {
                अगर (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्ष(), argp);
                }
            } अन्यथा {
                अगर (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_वृक्ष();
                }
                tconnects[it->second].second = argp;
                reorganize = true;
            }
        } अन्यथा {  // By pin number
            अगर (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } अन्यथा अगर (ppinnum >= tpinnum) {
                अगर (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } अन्यथा अगर (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्ष(), argp);
                }
            } अन्यथा {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    अगर (!makeChanges) वापस tconnects;

    // Connect missing ones
    VInsertionSet<स्थिर AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    क्रम (पूर्णांक i = 0; i < tpinnum; ++i) {
        AstVar* स्थिर portp = tconnects[i].first;
        अगर (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_गाँठ_व्यंजक* newvaluep = nullptr;
            अगर (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = नया AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } अन्यथा अगर (AstFuncRef* स्थिर funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                स्थिर Ast_गाँठ_FTask* स्थिर funcp = funcRefp->taskp();
                अगर (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } अन्यथा अगर (AstConst* स्थिर constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            अगर (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), गाँठ_व्यंजक));
                अगर (!VN_IS(newvaluep, Const)) {
                    अगर (statep) {
                        portp->pin_अंक(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.जोड़ो(portp);
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
                        newvaluep = नया AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_वृक्ष(true) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* स्थिर newp = नया AstArg{nodep->fileline(), portp->name(), newvaluep};
            अगर (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_वृक्ष(),
                            tconnects[i].second = nullptr);
            }
            tconnects[i].second = newp;
            reorganize = true;
        }
        अगर (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } अन्यथा {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    क्रम (स्थिर स्वतः& tconnect : tconnects) {
        AstArg* स्थिर argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    अगर (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        जबतक (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        क्रम (पूर्णांक i = 0; i < tpinnum; ++i) {
            AstArg* स्थिर argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    अगर (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_वृक्ष("-  ftref-out: ");
        क्रम (पूर्णांक i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    अगर (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        वापस taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    वापस tconnects;
}

व्योम V3Task::taskConnectWrap(Ast_गाँठ_FTask_Ref* nodep, स्थिर V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             स्थिर VInsertionSet<स्थिर AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    क्रम (स्थिर AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_अंक());
    स्थिर स्वतः pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    अगर (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_गाँठ_FTask* स्थिर newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    क्रम (स्थिर स्वतः& tconnect : tconnects) {
        स्थिर AstVar* स्थिर portp = tconnect.first;
        AstArg* स्थिर argp = tconnect.second;
        अगर (argWrap.exists(portp)) {  // Removed arg
            statep->धकेलो_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_गाँठ_FTask* V3Task::taskConnectWrapNew(Ast_गाँठ_FTask* taskp, स्थिर string& newname,
                                         स्थिर V3TaskConnects& tconnects,
                                         स्थिर VInsertionSet<स्थिर AstVar*>& argWrap) {
    std::map<स्थिर AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_गाँठ_FTask* स्थिर newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->add_अग्र_Here(newTaskp);

    Ast_गाँठ_FTask_Ref* newCallp = nullptr;
    Ast_गाँठ* newCallInsertp = nullptr;
    अगर (VN_IS(taskp, Func)) {
        AstVar* स्थिर fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* स्थिर newFVarp = fvarp->clone_वृक्ष(true);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = नया AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = नया AstAssign{taskp->fileline(),
                            नया AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } अन्यथा अगर (VN_IS(taskp, Task)) {
        newCallp = नया AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = नया Ast_Stmt_व्यंजक{taskp->fileline(), newCallp};
    } अन्यथा {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    क्रम (स्थिर स्वतः& tconnect : tconnects) {
        AstVar* स्थिर portp = tconnect.first;
        AstVar* newPortp;
        अगर (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = नया AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(true);
            अगर (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_वृक्ष();
            newTaskp->addStmtsp(newPortp);
        } अन्यथा {  // Defaulting arg
            Ast_गाँठ_व्यंजक* स्थिर valuep = VN_AS(portp->valuep(), गाँठ_व्यंजक);
            अगर ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                स्थिर VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* स्थिर refp = VN_AS(valuep->clone_वृक्ष(false), VarRef);
                refp->access(refAccess);
                AstArg* स्थिर newArgp = नया AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                जारी;
            }
            // Create local temporary
            newPortp = नया AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(true);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            अगर (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* स्थिर newAssignp
                    = नया AstAssign{valuep->fileline(),
                                    नया AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_वृक्ष(true)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* स्थिर newArgp = नया AstArg{
            portp->fileline(), portp->name(),
            नया AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        स्थिर स्वतः it = oldNewVars.find(refp->varp());
        अगर (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    वापस newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, bool isPtr, स्थिर string& frSuffix,
                                   स्थिर string& toSuffix, स्थिर string& frPrefix) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    स्थिर string frName = frPrefix + portp->name() + frSuffix;
    स्थिर string toName = portp->name() + toSuffix;
    स्थिर string idx = portp->name() + "__Vidx";
    माप_प्रकार unpack_आकार = 1;  // non-unpacked array is treated as size 1
    पूर्णांक unpackDim = 0;
    अगर (AstUnpackArrayDType* स्थिर unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpack_आकार = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(false).second;
        अगर (unpackDim > 0) UASSERT_OBJ(unpack_आकार > 0, portp, "size must be greater than 0");
    }
    अगर (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        स्थिर bool isBit = portp->basicp()->isDpiBitVec();
        स्थिर bool needsFor = unpack_आकार > 1;
        अगर (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpack_आकार) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        अगर (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_शब्दगण()) + " * " + idx;
        }
        stmt += ", ";
        अगर (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            क्रम (पूर्णांक i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } अन्यथा {
            stmt += frName + ")";
        }
    } अन्यथा {
        स्थिर bool isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        स्थिर bool isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        स्थिर string unpackAt = unpack_आकार > 1 ? "[" + idx + "]" : "[0]";
        अगर (unpackDim > 0) {
            स्थिर string forStmt = unpack_आकार > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpack_आकार) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } अन्यथा {
            अगर (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        अगर (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        अगर (unpackDim > 0) {
            stmt += "(&" + frName;
            क्रम (पूर्णांक i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } अन्यथा {
            stmt += frName;
        }
        अगर (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    वापस stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(स्थिर std::string& lhsName, AstVar* varp) {
    std::string cvt;
    bool useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    स्थिर std::vector<std::pair<पूर्णांक, पूर्णांक>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    स्थिर पूर्णांक योग = strides.empty() ? 1 : strides[0].first * strides[0].second;
    स्थिर पूर्णांक width_शब्दगण = varp->basicp()->width_शब्दगण();
    string statements;
    क्रम (पूर्णांक i = 0; i < योग; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        क्रम (स्थिर स्वतः& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        अगर (useSvVec) {
            स्थिर std::string offset = std::to_string(i * width_शब्दगण);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } अन्यथा {
            स्थिर std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    वापस statements;
}

व्योम V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        स्थिर TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("task", 0, dump_वृक्ष_Either_Level() >= 3);
}
