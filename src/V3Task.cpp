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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Task.h"

#ضمن "V3Const.h"
#ضمن "V3ConstPool.h"
#ضمن "V3Control.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3Graph.h"
#ضمن "V3LanguageWords.h"
#ضمن "V3SharedTmps.h"
#ضمن "V3Stats.h"

#ضمن <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

صنف TaskBaseVertex VL_NOT_FINAL : عام V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_عقدة* m_impurep = nullptr;  // Node causing impure function w/ outside references
    منطقي m_noInline = خطأ;  // Marked with pragma
عام:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = افتراضي;
    منطقي pure() ثابت { إرجاع m_impurep == nullptr; }
    Ast_عقدة* impure_عقدة() ثابت { إرجاع m_impurep; }
    فراغ impure(Ast_عقدة* nodep) { m_impurep = nodep; }
    منطقي noInline() ثابت { إرجاع m_noInline; }
    فراغ noInline(منطقي flag) { m_noInline = flag; }
};

صنف TaskFTaskVertex final : عام TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_عقدة_FTask* ثابت m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    منطقي m_needsNonInlineCFunc = خطأ;  // Needs CFunc for selected non-inlined call sites

عام:
    TaskFTaskVertex(V3Graph* graphp, Ast_عقدة_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = افتراضي;
    Ast_عقدة_FTask* nodep() ثابت VL_MT_STABLE { إرجاع m_nodep; }
    string name() ثابت override VL_MT_STABLE { إرجاع nodep()->name(); }
    string dotColor() ثابت override { إرجاع pure() ? "black" : "red"; }
    AstCFunc* cFuncp() ثابت { إرجاع m_cFuncp; }
    فراغ cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    منطقي needsNonInlineCFunc() ثابت { إرجاع m_needsNonInlineCFunc; }
    فراغ needsNonInlineCFunc(منطقي flag) { m_needsNonInlineCFunc = flag; }
};

صنف TaskCodeVertex final : عام TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
عام:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = افتراضي;
    string name() ثابت override VL_MT_STABLE { إرجاع "*CODE*"; }
    string dotColor() ثابت override { إرجاع "green"; }
};

صنف TaskEdge final : عام V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
عام:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* قمة)
        : V3GraphEdge{graphp, fromp, قمة, 1, خطأ} {}
    ~TaskEdge() override = افتراضي;
    string dotLabel() ثابت override { إرجاع "w" + cvtToStr(weight()); }
};

//######################################################################

صنف TaskStateVisitor final : عام VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ثابت VNUser3InUse m_inuser3;
    ثابت VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<ثابت Ast_عقدة_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_عقدة_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    منطقي m_underPortVar = خطأ;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

عام:
    // METHODS
    ساكن منطقي isVirtualIfaceMethodCall(ثابت Ast_عقدة_FTask_Ref* refp) {
        ثابت AstMethodCall* ثابت mrefp = VN_CAST(refp, MethodCall);
        إذا (!mrefp) إرجاع خطأ;
        ثابت Ast_عقدة_تعبير* ثابت fromp = mrefp->fromp();
        إذا (!fromp || !fromp->dtypep()) إرجاع خطأ;
        ثابت AstIfaceRefDType* ثابت ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        إرجاع ifrefp && ifrefp->isVirtual();
    }
    ساكن منطقي isIfaceFTaskScope(ثابت AstScope* scopep) {
        إرجاع scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_عقدة_FTask* nodep) {
        AstScope* ثابت scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        إرجاع scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        ثابت auto iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        إرجاع iter->second;
    }
    AstClass* getClassp(Ast_عقدة_FTask* nodep) {
        AstClass* ثابت classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        إرجاع classp;
    }
    فراغ remapFuncClassp(Ast_عقدة_FTask* nodep, Ast_عقدة_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    منطقي ftaskNoInline(Ast_عقدة_FTask* nodep) { إرجاع getFTaskVertex(nodep)->noInline(); }
    منطقي ftaskNeedsNonInlineCFunc(Ast_عقدة_FTask* nodep) {
        إرجاع getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_عقدة_FTask* nodep) { إرجاع getFTaskVertex(nodep)->cFuncp(); }
    فراغ ftaskCFuncp(Ast_عقدة_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_عقدة* checkImpure(Ast_عقدة_FTask* nodep) { إرجاع getFTaskVertex(nodep)->impure_عقدة(); }

خاص:
    TaskFTaskVertex* getFTaskVertex(Ast_عقدة_FTask* nodep) {
        إذا (!nodep->user4p()) nodep->user4p(جديد TaskFTaskVertex{&m_callGraph, nodep});
        إرجاع static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    فراغ propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        لكل (V3GraphVertex& vtx : m_callGraph.vertices()) {
            إذا (TaskFTaskVertex* ثابت ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                إذا (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        بينما (!workList.empty()) {
            TaskFTaskVertex* ثابت calleep = workList.back();
            workList.اسحب_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            لكل (V3GraphEdge& edge : calleep->inEdges()) {
                إذا (TaskFTaskVertex* ثابت callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    إذا (!callerp->pure()) استمر;  // Already processed or on workList
                    callerp->impure(calleep->impure_عقدة());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    فراغ decideInlining() {
        لكل (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* ثابت ftVtxp = vtx.cast<TaskFTaskVertex>();
            إذا (!ftVtxp) استمر;
            // Move on if already decided not to inline
            إذا (ftVtxp->noInline()) استمر;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            إذا (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(true);
                استمر;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            إذا (v3Global.opt.fInlineFuncsEager()) استمر;

            Ast_عقدة_FTask* ثابت ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            إذا (!ftaskp->verilogFunction()) استمر;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            إذا (ftaskp->taskPublic()) استمر;
            // If the function is pure, it's OK not to inline, so don't do it
            إذا (ftVtxp->pure()) ftVtxp->noInline(true);
        }
    }

    // VISITORS
    فراغ visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        لكل (Ast_عقدة* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVarScope* ثابت vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ثابت varp = vscp->varp();
                إذا (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        لكل (Ast_عقدة* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_عقدة_FTask* ثابت taskp = VN_CAST(stmtp, عقدة_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    فراغ visit(Ast_عقدة_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* ثابت taskVtxp = getFTaskVertex(nodep->taskp());
        جديد TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        إذا (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(true);
        // Do we have to disable inlining the function?
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        إذا (!taskVtxp->noInline()) {  // Else short-circuit below
            لكل (ثابت auto& itr : tconnects) {
                ثابت AstVar* ثابت portp = itr.first;
                ثابت AstArg* ثابت argp = itr.second;
                إذا (ثابت Ast_عقدة_تعبير* ثابت pinp = argp->exprp()) {
                    إذا ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(true);
                    }
                }
            }
        }
    }
    فراغ visit(Ast_عقدة_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        إذا (nodep->dpiImport()) m_curVxp->noInline(true);
        إذا (nodep->classMethod()) m_curVxp->noInline(true);  // Until V3Task supports it
        إذا (nodep->recursive()) m_curVxp->noInline(true);
        إذا (nodep->isConstructor()) {
            m_curVxp->noInline(true);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_شجرة(), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        إذا (nodep->isIO()) m_underPortVar = true;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    فراغ visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        إذا (m_underPortVar) إرجاع;
        AstVar* ثابت varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        إذا (nodep->access().isReadOnly() && varp->constPoolEntry()) إرجاع;
        إذا (varp->user4u().toGraphVertex() != m_curVxp) {
            إذا (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    فراغ visit(AstClass* nodep) override {
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
        Ast_عقدة* insertp = nullptr;
        لكل (AstInitialAutomatic* initialp : m_initialps) {
            إذا (Ast_عقدة* ثابت movep = initialp->stmtsp()) {
                movep->unlinkFrBackWithNext();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_عقدة::addNextNull(insertp, movep);
            }
            VL_DO_DANGLING(ادفع_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    فراغ visit(AstInitialAutomatic* nodep) override {
        m_initialps.ادفع_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = جديد TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.removeRedundantEdgesSum(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        إذا (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = افتراضي;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

بنية TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    ساكن std::vector<std::pair<صحيح, صحيح>> unpackDimsAndStrides(AstVar* varp) {
        Ast_عقدة_DType* ثابت dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<صحيح, صحيح>> dimStrides;
        AstUnpackArrayDType* ثابت unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        إذا (!unpackp) إرجاع dimStrides;

        ثابت std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ثابت size_t nDims = dims.size();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        لكل (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        إرجاع dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    ساكن std::pair<std::string, منطقي> dpiToInternalCvtStmt(ثابت Ast_عقدة_DType* dtypep) {
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();

        // DPI types using Primitive C types
        إذا (basicp->keyword() == VBasicDTypeKwd::CHANDLE) إرجاع {"VL_CVT_VP_Q(", خطأ};
        إذا (basicp->keyword() == VBasicDTypeKwd::STRING) إرجاع {"VL_CVT_N_CSTR(", خطأ};
        إذا (basicp->isDpiPrimitive()) إرجاع {"(", خطأ};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ثابت std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ثابت حرف sizeChar = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ثابت std::string& size = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        إرجاع {"VL_SET_"s + sizeChar + "_" + vecType + "(" + size + ", ", true};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

صنف TaskVisitor final : عام VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<ثابت string, std::tuple<Ast_عقدة_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ثابت m_statep;  // Common state between visitors
    Ast_عقدة_Module* m_modp = nullptr;  // Current module
    Ast_قمة_Scope* ثابت m_قمة_Scopep = v3Global.rootp()->قمة_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_عقدة* m_insStmtp = nullptr;  // Where to insert statement
    منطقي m_inSensesp = خطأ;  // Are we under a senitem?
    منطقي m_inNew = خطأ;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    صحيح m_uncon_Var_رقم = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, ثابت AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ثابت string& name, AstVar* examplep) {
        AstVar* ثابت newvarp = جديد AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(true);
        funcp->addVarsp(newvarp);
        AstVarScope* ثابت newvscp = جديد AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        إرجاع newvscp;
    }
    AstVarScope* create_إدخال_Var(AstCFunc* funcp, ثابت string& name, VBasicDTypeKwd kwd) {
        AstVar* ثابت newvarp
            = جديد AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(true);
        newvarp->direction(VDirection::إدخال);
        funcp->addArgsp(newvarp);
        AstVarScope* ثابت newvscp = جديد AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        إرجاع newvscp;
    }
    ساكن AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ثابت refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ثابت vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_شجرة(), refp);
        إرجاع vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ثابت Ast_عقدة_FTask_Ref* refp, AstVar* invarp,
                                ثابت std::string& suffix) {
        إذا (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            إرجاع constPoolTable(invarp);
        } وإلا {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* ثابت newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* ثابت newvarp = newvscp->varp();
            newvarp->funcLocal(خطأ);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(true);
            إرجاع newvscp;
        }
    }

    // Replace varrefs with new var pointer
    فراغ relink(Ast_عقدة* nodep) {
        nodep->foreachAndNext([](AstVarRef* refp) {
            إذا (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ثابت newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_عقدة_تعبير* pinp, AstVarScope* newvscp, منطقي pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_عقدة_تعبير* inPinp = pinp;
        إذا (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_عقدة_تعبير* inPinClonep
            = pureCheck ? inPinp->clone_شجرة_Pure(true) : inPinp->clone_شجرة(true);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = جديد AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* ثابت assp = جديد AstAssign{
            pinp->fileline(), جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, true);  // Ok if in <= block
        إرجاع assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_عقدة_تعبير* pinp, AstVarScope* newvscp,
                                        منطقي pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_عقدة_تعبير* outPinp = pinp;
        Ast_عقدة_تعبير* postRhsp = جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        إذا (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            إذا (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, عقدة_Uniop)->lhsp();
                إذا (postRhsp->isSigned()) {
                    postRhsp = جديد AstISToRD{pinp->fileline(), postRhsp};
                } وإلا {
                    postRhsp = جديد AstIToRD{pinp->fileline(), postRhsp};
                }
            } وإلا {
                إذا (Ast_عقدة_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } وإلا إذا (Ast_عقدة_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } وإلا إذا (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } وإلا {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                إذا (outPinp->width() < portp->width()) {
                    postRhsp = جديد AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } وإلا {  // pin width > port width
                    إذا (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = جديد AstExtendS{pinp->fileline(), postRhsp};
                    } وإلا {
                        postRhsp = جديد AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        إذا (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, عقدة_Uniop)->lhsp();
            postRhsp = جديد AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_عقدة_تعبير* ثابت outPinClonep
            = pureCheck ? outPinp->clone_شجرة_Pure(true) : outPinp->clone_شجرة(true);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = جديد AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* ثابت assp = جديد AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, true);  // Ok if in <= block
        إرجاع assp;
    }

    فراغ changeAtWriteRecurse(Ast_عقدة_تعبير* ثابت exprp) {
        // Change nested at methods to writable variant
        إذا (AstCMethodHard* ثابت cMethodp = VN_CAST(exprp, CMethodHard)) {
            إذا (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    فراغ connectPort(ثابت Ast_عقدة_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_عقدة* beginp,
                     منطقي inlineTask) {
        Ast_عقدة_تعبير* pinp = argp->exprp();
        إذا (inlineTask) {
            portp->unlinkFrBack();
            ادفع_Deletep(portp);  // Remove it from the clone (not original)
        }
        إذا (!pinp) {
            // Too few arguments in function call
        } وإلا {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            إذا (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* ثابت varp = جديد AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_رقم++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* ثابت newvscp = جديد AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* ثابت repp = جديد AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(ادفع_Deletep(pinp), pinp);
                pinp = repp;
            }
            إذا (inlineTask) {
                ادفع_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_شجرة(), argp);  // Args no longer needed
            }
            إذا (portp->isRef() || portp->isConstRef()) {
                منطقي refArgOk = خطأ;
                إذا (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = true;
                } وإلا إذا (AstCMethodHard* ثابت cMethodp = VN_CAST(pinp, CMethodHard)) {
                    إذا (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } وإلا {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                إذا (refArgOk) {
                    إذا (AstVarRef* ثابت varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } وإلا {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                إذا (inlineTask) {
                    إذا (AstVarRef* ثابت varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* ثابت localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } وإلا {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } وإلا إذا (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                إذا (!inlineTask) {
                    pinp->replaceWith(
                        جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    ادفع_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* ثابت preassp = connectPortMakeInAssign(pinp, newvscp, true);
                beginp->addNextHere(preassp);

                AstAssign* ثابت postassp = connectPortMakeOutAssign(portp, pinp, newvscp, true);
                beginp->addNext(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } وإلا إذا (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                إذا (!inlineTask) {
                    pinp->replaceWith(جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    ادفع_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* ثابت postassp = connectPortMakeOutAssign(portp, pinp, newvscp, خطأ);
                // Put assignment BEHIND of all other statements
                beginp->addNext(postassp);
            } وإلا إذا (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* ثابت preassp = connectPortMakeInAssign(pinp, newvscp, خطأ);
                // Put assignment in FRONT of all other statements
                beginp->addNextHere(preassp);
            }
        }
    }

    منطقي hasRefArgument(Ast_عقدة_FTask* nodep) {
        لكل (Ast_عقدة* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isRef() || varp->isConstRef()) إرجاع true;
            }
        }
        إرجاع خطأ;
    }

    Ast_عقدة* createInlinedFTask(Ast_عقدة_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_عقدة* ثابت newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_شجرة(true) : nullptr;
        Ast_عقدة* ثابت beginp
            = جديد AstComment{refp->fileline(), "Function: "s + refp->name(), true};
        إذا (newbodysp) beginp->addNext(newbodysp);
        إذا (debug() >= 9) beginp->dump_شجرة_And_Next(cout, "-  newbegi: ");
        //
        // Create input variables
        Ast_عقدة::user2_Clear_شجرة();
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            لكل (ثابت auto& itr : tconnects) {
                AstVar* ثابت portp = itr.first;
                AstArg* ثابت argp = itr.second;
                connectPort(refp, portp, argp, beginp, true);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_عقدة* nextstmtp;
            لكل (Ast_عقدة* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    إذا (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* ثابت localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        إذا (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_عقدة* ثابت crstp = جديد AstAssign{
                                portp->fileline(),
                                جديد AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                جديد AstCReset{portp->fileline(), portp, خطأ}};
                            portp->replaceWith(crstp);
                        } وإلا {
                            portp->unlinkFrBack();
                        }
                        ادفع_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        إذا (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        إذا (debug() >= 9) beginp->dump_شجرة_And_Next(cout, "-  iotask: ");
        إرجاع beginp;
    }

    Ast_عقدة* createNonInlinedFTask(Ast_عقدة_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ثابت cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_عقدة* ثابت beginp
            = جديد AstComment{refp->fileline(), "Function: "s + refp->name(), true};
        Ast_عقدة_CCall* ccallp;
        إذا (VN_IS(refp, New)) {
            AstCNew* ثابت cnewp = جديد AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } وإلا إذا (ثابت AstMethodCall* ثابت mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = جديد AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        } وإلا {
            ccallp = جديد AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        }

        إذا (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } وإلا إذا (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } وإلا إذا (ثابت AstMethodCall* ثابت methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            لكل (ثابت auto& itr : tconnects) {
                AstVar* ثابت portp = itr.first;
                AstArg* ثابت argp = itr.second;
                connectPort(refp, portp, argp, beginp, خطأ);
            }
        }
        // First argument is symbol table, then output if a function
        ثابت منطقي needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        إذا (needSyms) ccallp->argTypes("vlSymsp");

        إذا (refp->taskp()->dpiContext()) {
            AstScopeName* ثابت snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_سطر* ثابت flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                جديد AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(جديد AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        لكل (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        إذا (outvscp) ccallp->addArgsp(جديد AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        إذا (debug() >= 9) beginp->dump_شجرة_And_Next(cout, "-  nitask: ");
        إرجاع beginp;
    }

    string dpiSignature(Ast_عقدة_FTask* nodep, AstVar* rtnvarp) ثابت {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        إذا (nodep->dpiPure()) dpiproto += "pure ";
        إذا (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(true, true) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        لكل (Ast_عقدة* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    إذا (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ثابت std::string dpiType = portp->dpiArgType(خطأ, خطأ);
                    dpiproto += dpiType;
                    ثابت std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(خطأ);
                    إذا (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        إرجاع dpiproto;
    }

    ساكن فراغ checkLegalCIdentifier(Ast_عقدة* nodep, ثابت string& name) {
        إذا (name.end() != std::find_if(name.begin(), name.end(), [](حرف c) {
                إرجاع !std::isalnum(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    ساكن Ast_عقدة* createDpiTemp(AstVar* portp, ثابت string& suffix) {
        ثابت string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        إرجاع جديد AstCStmt{portp->fileline(), stmt};
    }

    فراغ unlinkAndClone(Ast_عقدة_FTask* funcp, Ast_عقدة* nodep, منطقي withNext) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        إذا (funcp->recursive()) {
            VNRelinker relinkHandle;
            إذا (withNext) {
                nodep->unlinkFrBackWithNext(&relinkHandle);
            } وإلا {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_عقدة* ثابت newp = nodep->clone_شجرة(withNext);
            relinkHandle.relink(newp);
        } وإلا {
            إذا (withNext) {
                nodep->unlinkFrBackWithNext();
            } وإلا {
                nodep->unlinkFrBack();
            }
        }
    }

    ساكن Ast_عقدة* createAssignInternalToDpi(AstVar* portp, منطقي isPtr, ثابت string& frSuffix,
                                              ثابت string& toSuffix) {
        ثابت string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        إرجاع جديد AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    ساكن Ast_عقدة_Stmt* createDpiScalarCvt(Ast_عقدة_تعبير* internalp, ثابت std::string& dpi,
                                           ثابت Ast_عقدة_DType* dtypep, منطقي toDpi) {
        File_سطر* ثابت flp = internalp->fileline();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        إذا (toDpi) {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            إذا (basicp->isDpiPrimitive()) {
                cstmtp->add(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->add(internalp);
                cstmtp->add(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } وإلا {
                cstmtp->add((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::to_string(dtypep->width()) + ", " + dpi + ", ");
                cstmtp->add(internalp);
                cstmtp->add(");");
            }
            إرجاع cstmtp;
        }
        std::string cvt;
        منطقي useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        إذا (useSvVec) {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            إرجاع cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ثابت صحيح cwidth = basicp->width_كلمات() * VL_EDATASIZE;
        Ast_عقدة_تعبير* ثابت rhsp = جديد AstCExpr{flp, cvt + dpi + ")", cwidth};
        إرجاع جديد AstAssign{flp, internalp, جديد AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    ساكن Ast_عقدة_Stmt* createDpiCvt(Ast_عقدة_تعبير* internalp, ثابت std::string& dpi,
                                     ثابت Ast_عقدة_DType* dtypep, منطقي toDpi) {
        File_سطر* ثابت flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_عقدة_Stmt* stmtsp = nullptr;
        إذا (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            لكل (صحيح i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_عقدة_تعبير* ثابت elemp = جديد AstArraySel{flp, internalp->clone_شجرة(خطأ), i};
                stmtsp = Ast_عقدة::addNext(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } وإلا إذا (dtypep->containsUnpackedStruct()) {
            لكل (ثابت AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ثابت selp
                    = جديد AstStructSel{flp, internalp->clone_شجرة(خطأ), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_عقدة::addNext(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } وإلا {
            إرجاع createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_شجرة(), internalp);
        إرجاع stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    ساكن Ast_عقدة_Stmt* createDpiArgCvt(AstVarScope* vscp, ثابت std::string& dpiName,
                                        منطقي toDpi) {
        AstVar* ثابت varp = vscp->varp();
        File_سطر* ثابت flp = vscp->fileline();
        ثابت std::vector<std::pair<صحيح, صحيح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ثابت صحيح total = strides.empty() ? 1 : strides[0].first * strides[0].second;
        ثابت منطقي isStruct = varp->dtypep()->containsUnpackedStruct();
        ثابت Ast_عقدة_DType* ثابت elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_عقدة_Stmt* stmtsp = nullptr;
        لكل (صحيح i = 0; i < total; ++i) {
            Ast_عقدة_تعبير* internalp
                = جديد AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            لكل (ثابت auto& stride : strides) {
                internalp = جديد AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            إذا (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_كلمات());
            } وإلا إذا (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_عقدة::addNext(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        إرجاع stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_عقدة_FTask* ثابت nodep, AstVar* ثابت rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_سطر* ثابت flp = nodep->fileline();
        ثابت std::string cname = Ast_عقدة::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ثابت حرف* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ثابت std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(true, true) : "";
        // The function we are building
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(true);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(true);
        funcp->entryPoint(true);
        funcp->isStatic(true);
        funcp->protect(خطأ);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_قمة_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        إذا (!makePortList(nodep, funcp)) إرجاع nullptr;

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
        AstCStmt* ثابت prep = جديد AstCStmt{flp};
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
        ثابت std::string cbtype
            = VIdProtect::protect(v3Global.opt.prefix() + "__Vcb_" + nodep->cname() + "_t");
        prep->add(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ثابت callp = جديد AstCStmt{flp};
        ثابت auto addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ثابت vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(خطأ);
            portp->protect(خطأ);
            // Add argument to call
            ثابت VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(جديد AstVarRef{portp->fileline(), vscp, access});
            إرجاع vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_عقدة* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        لكل (Ast_عقدة* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت portp = VN_CAST(stmtp, Var);
            إذا (!portp) استمر;
            إذا (!portp->isIO()) استمر;
            إذا (portp->isFuncReturn()) استمر;
            إذا (portp == rtnvarp) استمر;  // Handled below
            // Add argument to call
            AstVarScope* ثابت outvscp = addFuncArg(portp);
            ثابت منطقي isStruct = portp->dtypep()->containsUnpackedStruct();
            إذا (portp->isWritable()) {
                outCvtsp = Ast_عقدة::addNext(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), true)
                                       : createAssignInternalToDpi(portp, true, tmpSuffixp, ""));
            }
            إذا (!portp->isNonOutput()) استمر;
            // Convert input/inout arguments to dpi type
            ثابت std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(خطأ).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), خطأ));
        }
        // Return value argument goes last
        إذا (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        إذا (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        إذا (rtnvarp) {
            funcp->addStmtsp(createDpiTemp(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, خطأ, tmpSuffixp, ""));
            ثابت std::string index = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(جديد AstCStmt{flp, "return " + rtnvarp->name() + index + ";"});
        }
        إرجاع funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_عقدة_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ثابت string name = Ast_عقدة::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ثابت string rtnType = rtnvarp            ? rtnvarp->dpiArgType(true, true)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ثابت funcp = جديد AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(true);
        funcp->dontCombine(true);
        funcp->entryPoint(خطأ);
        funcp->isMethod(خطأ);
        funcp->protect(خطأ);
        funcp->dpiPure(nodep->dpiPure());

        ثابت صحيح cost = static_cast<صحيح>(V3Control::getProfileData(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_قمة_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        إذا (!makePortList(nodep, funcp)) إرجاع nullptr;
        إرجاع funcp;
    }

    AstCFunc* getDpiFunc(Ast_عقدة_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ثابت string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ثابت auto pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        إذا (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ثابت funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            إرجاع funcp;
        } وإلا {
            // Seen this cname import before. Check if it's the same prototype.
            ثابت Ast_عقدة_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            إذا (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                إرجاع nullptr;
            } وإلا {
                // Same signature, return the previously created CFunc
                إرجاع firstFuncp;
            }
        }
    }

    ساكن منطقي makePortList(Ast_عقدة_FTask* nodep, AstCFunc* dpip) {
        منطقي allOk = true;
        // Copy nodep's list of function I/O to the new dpip c function
        لكل (Ast_عقدة* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO()) {
                    // Move it to new function
                    AstVar* ثابت newPortp = portp->clone_شجرة(خطأ);
                    newPortp->funcLocal(true);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    إذا (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = خطأ;
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
        إرجاع allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    ساكن منطقي isDpiCIdentifier(ثابت string& name) {
        إرجاع !name.empty() && VString::isIdentifier(name)
               && !std::isdigit(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_كلمات::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    منطقي checkDpiStruct(ثابت AstVar* portp, ثابت AstStructDType* sdtypep, منطقي nested) {
        ثابت std::string name = sdtypep->dpiName();
        إذا (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(true) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            إرجاع خطأ;
        }
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                إرجاع خطأ;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ثابت Ast_عقدة_DType* dtypep = itemp->subDTypep()->skipRefp();
            بينما (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ثابت Ast_عقدة_UOr_Struct_DType* ثابت subp = VN_CAST(dtypep, عقدة_UOr_Struct_DType);
            ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ثابت منطقي isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            إذا (dtypep->containsUnpackedStruct()) {
                إذا (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), true)) إرجاع خطأ;
            } وإلا إذا (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(true)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                إرجاع خطأ;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        إذا (name.empty()) إرجاع true;
        ثابت AstStructDType* ثابت firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        إذا (firstp->dpiTypedef() == sdtypep->dpiTypedef()) إرجاع true;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        إرجاع خطأ;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    منطقي checkDpiStructArgs(ثابت Ast_عقدة_FTask* nodep) {
        منطقي allOk = true;
        لكل (ثابت Ast_عقدة* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var);
            إذا (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       خطأ)
                        && allOk;
            }
        }
        إرجاع allOk;
    }

    فراغ bodyDpiImportFunc(Ast_عقدة_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ثابت حرف* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        إذا (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_ادفع(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        لكل (Ast_عقدة* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ثابت portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                إذا (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    إذا (args != "") args += ", ";

                    إذا (portp->isDpiOpenArray()) {
                        Ast_عقدة_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ثابت string propName = portp->name() + "__Vopenprops";
                        ثابت string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(جديد AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ثابت string name = portp->name() + "__Vopenarray";
                        ثابت string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(جديد AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } وإلا {
                        // Unpacked structures are passed by reference from a C array
                        ثابت منطقي isStruct = portp->dtypep()->containsUnpackedStruct();
                        إذا (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            إذا (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(createDpiTemp(portp, tmpSuffixp));
                        إذا (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, true)
                                    : createAssignInternalToDpi(portp, خطأ, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        إذا (nodep->dpiContext()) {
            ثابت string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(جديد AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ثابت callp = جديد AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            إذا (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(createDpiTemp(rtnvscp->varp(), tmpSuffixp));
                ثابت std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ثابت cstmtp = جديد AstCStmt{nodep->fileline()};
                cstmtp->add(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->add(" = ");
                cstmtp->add(callp);  // RHS
                cstmtp->add(";");
                cfuncp->addStmtsp(cstmtp);
            } وإلا {
                // Othervise just call it
                cfuncp->addStmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        لكل (Ast_عقدة* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                portp->protect(خطأ);  // No additional exposure - already part of shown proto
                إذا (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ثابت portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, خطأ));
                }
            }
        }

        إذا (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_اسحب(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        إذا (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_قمة_Scopep->createTemp("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        إرجاع dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_عقدة_FTask* nodep, منطقي ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_عقدة::user2_Clear_شجرة();
        AstVar* rtnvarp = nullptr;
        إذا (nodep->isFunction()) {
            AstVar* ثابت portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            إذا (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ثابت bdtypep = portp->dtypep()->basicp();
                إذا (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(true)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                    إرجاع nullptr;
                }
                إذا (!bdtypep->isDpiPrimitive()) {
                    إذا (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    إذا (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } وإلا إذا (nodep->taskPublic()) {
                إذا (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            إذا (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(خطأ);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, خطأ);
            rtnvarp = portp;
            rtnvarp->funcLocal(true);
            rtnvarp->noCReset(true);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            إذا (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(خطأ);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = nullptr;
        إذا (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            إذا (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                إذا (rtnvarp) VL_DO_DANGLING(ادفع_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                إرجاع nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        إذا (rtnvarp) {
            rtnvscp = جديد AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        إذا (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } وإلا إذا (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } وإلا إذا (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        إذا (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        ثابت string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* ثابت cfuncp = جديد AstCFunc{
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
        إذا (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(true);
            cfuncp->isLoose(true);
        } وإلا {
            cfuncp->isStatic(خطأ);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        إذا (nodep->name() == "new") cfuncp->isConstructor(true);
        إذا (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(true);
        إذا (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        إذا (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ثابت منطقي needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        إذا (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        إذا (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            إذا (cfuncp->isConstructor()) {
                منطقي isInterfaceClass = خطأ;
                إذا (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                إذا (!isInterfaceClass) {
                    ثابت string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        جديد AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        إذا (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_إدخال_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_إدخال_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_إدخال_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        إذا (nodep->dpiExport()) {
            AstScopeName* ثابت snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(true);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        لكل (Ast_عقدة *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    ادفع_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } وإلا {
                    إذا (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, خطأ);
                        portp->funcLocal(true);
                        إذا (portp->valuep()) ادفع_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        إذا (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_عقدة_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                            إذا (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } وإلا {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(true);
                    }
                    AstVarScope* ثابت newvscp
                        = جديد AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        إذا (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_عقدة* bodysp = nodep->stmtsp();
        إذا (bodysp) {
            unlinkAndClone(nodep, bodysp, true);
            AstBegin* ثابت tempp
                = جديد AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, خطأ};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            لكل (Ast_عقدة *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    إذا (portp->isIO()) VL_DO_DANGLING(ادفع_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            إذا (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(tempp->delete_شجرة(), tempp);
        }
        إذا (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        إذا (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(جديد AstCReturn{
                rtnvscp->fileline(), جديد AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        إذا (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            منطقي writesNonLocals = خطأ;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* ثابت varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                إذا (varp->isFuncLocal()) إرجاع;
                // Check if written
                إذا (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = true;
                }
                // Check if read
                إذا (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            إذا (writesNonLocals) {
                AstVarScope* ثابت dpiExportTriggerp = getDpiExporTrigger();
                File_سطر* ثابت flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* ثابت assignp
                    = جديد AstAssign{flp, جديد AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    جديد AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                إذا (v3Global.opt.systemC()) {
                    assignp->addNextHere(جديد AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        إذا (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        إرجاع cfuncp;
    }

    فراغ iterateIntoFTask(Ast_عقدة_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    فراغ أدرج_Before_Stmt(Ast_عقدة* nodep, Ast_عقدة* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->addHereThisAsNext(newp);
    }
    فراغ processArgs(Ast_عقدة_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ثابت arrayp = VN_CAST(argp->exprp(), ConsArray);
            إذا (!arrayp) استمر;

            File_سطر* ثابت flp = arrayp->fileline();
            AstVarScope* ثابت substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(true);

            AstAssign* ثابت assignp
                = جديد AstAssign{flp, جديد AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_تعبير_Stmt* ثابت exprstmtp = جديد Ast_تعبير_Stmt{
                flp, assignp, جديد AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    منطقي isIfaceLocalImpure(Ast_عقدة_FTask* nodep, Ast_عقدة* impurep) {
        ثابت AstScope* ثابت scopep = m_statep->getScope(nodep);
        إذا (!TaskStateVisitor::isIfaceFTaskScope(scopep)) إرجاع خطأ;

        ثابت AstVarRef* ثابت refp = VN_CAST(impurep, VarRef);
        إذا (!refp || !refp->varScopep()) إرجاع خطأ;

        إرجاع refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    فراغ visit(AstWith* nodep) override {
        إذا (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            إرجاع;
        }
        Ast_عقدة_تعبير* ثابت withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), عقدة_تعبير);
        nodep->addExprp(جديد AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    فراغ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    فراغ visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = true;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_عقدة_FTask_Ref* nodep) override {
        إذا (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            إرجاع;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        إذا (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_تعداد(), nodep, "No non-inline task, but no error issued");
            إذا (VN_IS(nodep->backp(), Stmt_تعبير)) {
                VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
            } وإلا {
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            }
            إرجاع;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        إذا (nodep->taskp()->isFunction()) {
            AstVar* ثابت fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            إذا (AstAssign* ثابت assignp = VN_CAST(nodep->backp(), Assign)) {
                إذا (AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ثابت vscp = lhsp->varScopep();
                    إذا (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_شجرة(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            إذا (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_عقدة* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ثابت منطقي virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        إذا (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } وإلا {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        إذا (VN_IS(nodep, New)) {  // New not legal as while() condition
            أدرج_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
        } وإلا إذا (VN_IS(nodep->backp(), عقدة_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            أدرج_Before_Stmt(nodep, beginp);
            AstVarRef* ثابت outrefp = جديد AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
        } وإلا إذا (!VN_IS(nodep->backp(), Stmt_تعبير)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ثابت outrefp = جديد AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_تعبير_Stmt* lambdap = جديد Ast_تعبير_Stmt{nodep->fileline(), beginp, outrefp};

            إذا (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_شجرة(خطأ);
                newvarp->funcLocal(true);

                // Create a new VarScope for the new variable
                AstVarScope* ثابت newvscp
                    = جديد AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreachAndNext([outvscp, newvscp](AstVarRef* refp) {
                    إذا (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->hasResult(خطأ);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(جديد AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
            VIsCached::clear_Cache_شجرة();
        } وإلا {  // VN_IS(nodep->backp(), StmtExpr)
            أدرج_Before_Stmt(nodep, beginp);
            إذا (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    فراغ visit(Ast_عقدة_FTask* ثابت nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        إذا (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            صحيح modes = 0;
            إذا (nodep->dpiImport()) modes++;
            إذا (nodep->dpiExport()) modes++;
            إذا (nodep->taskPublic()) modes++;
            إذا (nodep->classMethod()) modes++;
            إذا (v3Global.opt.protectIds() && nodep->taskPublic()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            إذا (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            ثابت منطقي noInline = m_statep->ftaskNoInline(nodep);
            ثابت منطقي needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            إذا (noInline && !nodep->classMethod() && !nodep->recursive()) {
                إذا (Ast_عقدة* ثابت impurep = m_statep->checkImpure(nodep)) {
                    إذا (!isIfaceLocalImpure(nodep, impurep)) {
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

            إذا (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_عقدة_FTask* ثابت clonedFuncp = nodep->clone_شجرة(خطأ);
                إذا (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ثابت منطقي makeNonInlineFunc = noInline || needsNonInlineCFunc;
                إذا (AstCFunc* ثابت cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->addNextHere(cfuncp);
                    إذا (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            إذا (nodep->isFunction()) {
                إذا (AstVar* ثابت portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* ثابت vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ادفع_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            لكل (Ast_عقدة *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ثابت vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ادفع_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        }
    }
    فراغ visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_عقدة_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_عقدة_FTask_Ref* refp) { refs.ادفع_back(refp); });
        لكل (Ast_عقدة_FTask_Ref* ثابت refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ثابت interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* ثابت assignp = جديد AstAssign{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_شجرة_Pure(خطأ)};
            Ast_Sen_شجرة* ثابت senTreep = جديد Ast_Sen_شجرة{
                nodep->fileline(), جديد AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* ثابت activep
                = جديد AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_شجرة_Storep(activep->sentreep());
            activep->addStmtsp(
                جديد AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(جديد AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_شجرة(), refp);
        }
    }
    فراغ visit(Ast_عقدة_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    فراغ visit(Ast_عقدة_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_Stmt_تعبير* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_شجرة(), nodep);
    }
    فراغ visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = true;
        iterateChildren(nodep);
    }
    //--------------------
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
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

صنف TaskIfaceSensVisitor final : عام VNVisitorConst {
    // STATE
    منطقي m_underIfaceFunc = خطأ;  // Under a CFunc owned by an interface scope
    منطقي m_underSenses = خطأ;  // Under a sensitivity expression of such a CFunc

    // METHODS
    فراغ markSensesAndIterate(Ast_عقدة* nodep) {
        إذا (!m_underIfaceFunc) إرجاع;
        VL_RESTORER(m_underSenses);
        m_underSenses = true;
        iterateAndNextConstNull(nodep);
    }
    // VISITORS
    فراغ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    فراغ visit(Ast_Sen_شجرة* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    فراغ visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    فراغ visit(AstVarRef* nodep) override {
        إذا (!m_underSenses) إرجاع;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        إذا (nodep->varp()->isFuncLocal()) إرجاع;
        إذا (AstIface* ثابت ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    فراغ visit(Ast_عقدة* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

ثابت حرف* ثابت V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_عقدة_FTask_Ref* nodep, Ast_عقدة* taskStmtsp,
                                    V3TaskConnectState* statep, منطقي makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ثابت std::string, صحيح> nameToIndex;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    صحيح tpinnum = 0;
    AstVar* sformatp = nullptr;
    لكل (Ast_عقدة* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            إذا (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                nameToIndex.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                إذا (portp->attrSFormat()) {
                    sformatp = portp;
                } وإلا إذا (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    صحيح ppinnum = 0;
    منطقي reorganize = خطأ;
    لكل (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        إذا (!argp->name().empty()) {
            // By name
            ثابت auto it = nameToIndex.find(argp->name());
            إذا (it == nameToIndex.end()) {
                إذا (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_شجرة(), argp);
                }
            } وإلا {
                إذا (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_شجرة();
                }
                tconnects[it->second].second = argp;
                reorganize = true;
            }
        } وإلا {  // By pin number
            إذا (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } وإلا إذا (ppinnum >= tpinnum) {
                إذا (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } وإلا إذا (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_شجرة(), argp);
                }
            } وإلا {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    إذا (!makeChanges) إرجاع tconnects;

    // Connect missing ones
    VInsertionSet<ثابت AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    لكل (صحيح i = 0; i < tpinnum; ++i) {
        AstVar* ثابت portp = tconnects[i].first;
        إذا (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_عقدة_تعبير* newvaluep = nullptr;
            إذا (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = جديد AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } وإلا إذا (AstFuncRef* ثابت funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ثابت Ast_عقدة_FTask* ثابت funcp = funcRefp->taskp();
                إذا (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } وإلا إذا (AstConst* ثابت constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            إذا (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), عقدة_تعبير));
                إذا (!VN_IS(newvaluep, Const)) {
                    إذا (statep) {
                        portp->pin_رقم(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.أدرج(portp);
                    } وإلا {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = جديد AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_شجرة(true) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ثابت newp = جديد AstArg{nodep->fileline(), portp->name(), newvaluep};
            إذا (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_شجرة(),
                            tconnects[i].second = nullptr);
            }
            tconnects[i].second = newp;
            reorganize = true;
        }
        إذا (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } وإلا {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    لكل (ثابت auto& tconnect : tconnects) {
        AstArg* ثابت argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    إذا (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        بينما (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        لكل (صحيح i = 0; i < tpinnum; ++i) {
            AstArg* ثابت argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    إذا (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_شجرة("-  ftref-out: ");
        لكل (صحيح i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    إذا (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        إرجاع taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    إرجاع tconnects;
}

فراغ V3Task::taskConnectWrap(Ast_عقدة_FTask_Ref* nodep, ثابت V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ثابت VInsertionSet<ثابت AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    لكل (ثابت AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_رقم());
    ثابت auto pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    إذا (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_عقدة_FTask* ثابت newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    لكل (ثابت auto& tconnect : tconnects) {
        ثابت AstVar* ثابت portp = tconnect.first;
        AstArg* ثابت argp = tconnect.second;
        إذا (argWrap.exists(portp)) {  // Removed arg
            statep->ادفع_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_عقدة_FTask* V3Task::taskConnectWrapNew(Ast_عقدة_FTask* taskp, ثابت string& newname,
                                         ثابت V3TaskConnects& tconnects,
                                         ثابت VInsertionSet<ثابت AstVar*>& argWrap) {
    std::map<ثابت AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_عقدة_FTask* ثابت newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->addNextHere(newTaskp);

    Ast_عقدة_FTask_Ref* newCallp = nullptr;
    Ast_عقدة* newCallInsertp = nullptr;
    إذا (VN_IS(taskp, Func)) {
        AstVar* ثابت fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ثابت newFVarp = fvarp->clone_شجرة(true);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = جديد AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = جديد AstAssign{taskp->fileline(),
                            جديد AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } وإلا إذا (VN_IS(taskp, Task)) {
        newCallp = جديد AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = جديد Ast_Stmt_تعبير{taskp->fileline(), newCallp};
    } وإلا {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    لكل (ثابت auto& tconnect : tconnects) {
        AstVar* ثابت portp = tconnect.first;
        AstVar* newPortp;
        إذا (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = جديد AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(true);
            إذا (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_شجرة();
            newTaskp->addStmtsp(newPortp);
        } وإلا {  // Defaulting arg
            Ast_عقدة_تعبير* ثابت valuep = VN_AS(portp->valuep(), عقدة_تعبير);
            إذا ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ثابت VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ثابت refp = VN_AS(valuep->clone_شجرة(خطأ), VarRef);
                refp->access(refAccess);
                AstArg* ثابت newArgp = جديد AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                استمر;
            }
            // Create local temporary
            newPortp = جديد AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(true);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            إذا (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* ثابت newAssignp
                    = جديد AstAssign{valuep->fileline(),
                                    جديد AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_شجرة(true)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ثابت newArgp = جديد AstArg{
            portp->fileline(), portp->name(),
            جديد AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ثابت auto it = oldNewVars.find(refp->varp());
        إذا (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    إرجاع newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, منطقي isPtr, ثابت string& frSuffix,
                                   ثابت string& toSuffix, ثابت string& frPrefix) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ثابت string frName = frPrefix + portp->name() + frSuffix;
    ثابت string toName = portp->name() + toSuffix;
    ثابت string idx = portp->name() + "__Vidx";
    size_t unpackSize = 1;  // non-unpacked array is treated as size 1
    صحيح unpackDim = 0;
    إذا (AstUnpackArrayDType* ثابت unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpackSize = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(خطأ).second;
        إذا (unpackDim > 0) UASSERT_OBJ(unpackSize > 0, portp, "size must be greater than 0");
    }
    إذا (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        ثابت منطقي isBit = portp->basicp()->isDpiBitVec();
        ثابت منطقي needsFor = unpackSize > 1;
        إذا (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpackSize) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        إذا (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_كلمات()) + " * " + idx;
        }
        stmt += ", ";
        إذا (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            لكل (صحيح i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } وإلا {
            stmt += frName + ")";
        }
    } وإلا {
        ثابت منطقي isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        ثابت منطقي isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        ثابت string unpackAt = unpackSize > 1 ? "[" + idx + "]" : "[0]";
        إذا (unpackDim > 0) {
            ثابت string forStmt = unpackSize > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpackSize) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } وإلا {
            إذا (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        إذا (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        إذا (unpackDim > 0) {
            stmt += "(&" + frName;
            لكل (صحيح i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } وإلا {
            stmt += frName;
        }
        إذا (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    إرجاع stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(ثابت std::string& lhsName, AstVar* varp) {
    std::string cvt;
    منطقي useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    ثابت std::vector<std::pair<صحيح, صحيح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ثابت صحيح total = strides.empty() ? 1 : strides[0].first * strides[0].second;
    ثابت صحيح width_كلمات = varp->basicp()->width_كلمات();
    string statements;
    لكل (صحيح i = 0; i < total; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        لكل (ثابت auto& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        إذا (useSvVec) {
            ثابت std::string offset = std::to_string(i * width_كلمات);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } وإلا {
            ثابت std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    إرجاع statements;
}

فراغ V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ثابت TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_شجرة("task", 0, dump_شجرة_Either_Level() >= 3);
}
