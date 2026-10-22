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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Task.h"

#شامل "V3Const.h"
#شامل "V3ConstPool.h"
#شامل "V3Control.h"
#شامل "V3EmitCBase.h"
#شامل "V3Graph.h"
#شامل "V3LanguageWords.h"
#شامل "V3SharedTmps.h"
#شامل "V3Stats.h"

#شامل <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

رده TaskBaseVertex VL_NOT_FINAL : عمومی V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_گره* m_impurep = nullptr;  // Node causing impure function w/ outside references
    منطقی m_noInline = نادرست;  // Marked with pragma
عمومی:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = پیشفرض;
    منطقی pure() ثابت { بازگشت m_impurep == nullptr; }
    Ast_گره* impure_گره() ثابت { بازگشت m_impurep; }
    تهی impure(Ast_گره* nodep) { m_impurep = nodep; }
    منطقی noInline() ثابت { بازگشت m_noInline; }
    تهی noInline(منطقی flag) { m_noInline = flag; }
};

رده TaskFTaskVertex final : عمومی TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_گره_FTask* ثابت m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    منطقی m_needsNonInlineCFunc = نادرست;  // Needs CFunc for selected non-inlined call sites

عمومی:
    TaskFTaskVertex(V3Graph* graphp, Ast_گره_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = پیشفرض;
    Ast_گره_FTask* nodep() ثابت VL_MT_STABLE { بازگشت m_nodep; }
    string name() ثابت override VL_MT_STABLE { بازگشت nodep()->name(); }
    string dotColor() ثابت override { بازگشت pure() ? "black" : "red"; }
    AstCFunc* cFuncp() ثابت { بازگشت m_cFuncp; }
    تهی cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    منطقی needsNonInlineCFunc() ثابت { بازگشت m_needsNonInlineCFunc; }
    تهی needsNonInlineCFunc(منطقی flag) { m_needsNonInlineCFunc = flag; }
};

رده TaskCodeVertex final : عمومی TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
عمومی:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = پیشفرض;
    string name() ثابت override VL_MT_STABLE { بازگشت "*CODE*"; }
    string dotColor() ثابت override { بازگشت "green"; }
};

رده TaskEdge final : عمومی V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
عمومی:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* بالاترین)
        : V3GraphEdge{graphp, fromp, بالاترین, 1, نادرست} {}
    ~TaskEdge() override = پیشفرض;
    string dotLabel() ثابت override { بازگشت "w" + cvtToStr(weight()); }
};

//######################################################################

رده TaskStateVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ثابت VNUser3InUse m_inuser3;
    ثابت VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<ثابت Ast_گره_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_گره_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    منطقی m_underPortVar = نادرست;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

عمومی:
    // METHODS
    ایستا منطقی isVirtualIfaceMethodCall(ثابت Ast_گره_FTask_Ref* refp) {
        ثابت AstMethodCall* ثابت mrefp = VN_CAST(refp, MethodCall);
        اگر (!mrefp) بازگشت نادرست;
        ثابت Ast_گره_عبارت* ثابت fromp = mrefp->fromp();
        اگر (!fromp || !fromp->dtypep()) بازگشت نادرست;
        ثابت AstIfaceRefDType* ثابت ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        بازگشت ifrefp && ifrefp->isVirtual();
    }
    ایستا منطقی isIfaceFTaskScope(ثابت AstScope* scopep) {
        بازگشت scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_گره_FTask* nodep) {
        AstScope* ثابت scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        بازگشت scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        ثابت auto iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        بازگشت iter->second;
    }
    AstClass* getClassp(Ast_گره_FTask* nodep) {
        AstClass* ثابت classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        بازگشت classp;
    }
    تهی remapFuncClassp(Ast_گره_FTask* nodep, Ast_گره_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    منطقی ftaskNoInline(Ast_گره_FTask* nodep) { بازگشت getFTaskVertex(nodep)->noInline(); }
    منطقی ftaskNeedsNonInlineCFunc(Ast_گره_FTask* nodep) {
        بازگشت getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_گره_FTask* nodep) { بازگشت getFTaskVertex(nodep)->cFuncp(); }
    تهی ftaskCFuncp(Ast_گره_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_گره* checkImpure(Ast_گره_FTask* nodep) { بازگشت getFTaskVertex(nodep)->impure_گره(); }

خصوصی:
    TaskFTaskVertex* getFTaskVertex(Ast_گره_FTask* nodep) {
        اگر (!nodep->user4p()) nodep->user4p(جدید TaskFTaskVertex{&m_callGraph, nodep});
        بازگشت static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    تهی propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        برای (V3GraphVertex& vtx : m_callGraph.vertices()) {
            اگر (TaskFTaskVertex* ثابت ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                اگر (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        وقتی (!workList.empty()) {
            TaskFTaskVertex* ثابت calleep = workList.back();
            workList.برداشت_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            برای (V3GraphEdge& edge : calleep->inEdges()) {
                اگر (TaskFTaskVertex* ثابت callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    اگر (!callerp->pure()) ادامه;  // Already processed or on workList
                    callerp->impure(calleep->impure_گره());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    تهی decideInlining() {
        برای (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* ثابت ftVtxp = vtx.cast<TaskFTaskVertex>();
            اگر (!ftVtxp) ادامه;
            // Move on if already decided not to inline
            اگر (ftVtxp->noInline()) ادامه;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            اگر (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(درست);
                ادامه;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            اگر (v3Global.opt.fInlineFuncsEager()) ادامه;

            Ast_گره_FTask* ثابت ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            اگر (!ftaskp->verilogFunction()) ادامه;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            اگر (ftaskp->taskPublic()) ادامه;
            // If the function is pure, it's OK not to inline, so don't do it
            اگر (ftVtxp->pure()) ftVtxp->noInline(درست);
        }
    }

    // VISITORS
    تهی visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        برای (Ast_گره* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVarScope* ثابت vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ثابت varp = vscp->varp();
                اگر (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        برای (Ast_گره* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (Ast_گره_FTask* ثابت taskp = VN_CAST(stmtp, گره_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* ثابت taskVtxp = getFTaskVertex(nodep->taskp());
        جدید TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        اگر (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(درست);
        // Do we have to disable inlining the function?
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        اگر (!taskVtxp->noInline()) {  // Else short-circuit below
            برای (ثابت auto& itr : tconnects) {
                ثابت AstVar* ثابت portp = itr.first;
                ثابت AstArg* ثابت argp = itr.second;
                اگر (ثابت Ast_گره_عبارت* ثابت pinp = argp->exprp()) {
                    اگر ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(درست);
                    }
                }
            }
        }
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        اگر (nodep->dpiImport()) m_curVxp->noInline(درست);
        اگر (nodep->classMethod()) m_curVxp->noInline(درست);  // Until V3Task supports it
        اگر (nodep->recursive()) m_curVxp->noInline(درست);
        اگر (nodep->isConstructor()) {
            m_curVxp->noInline(درست);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    تهی visit(AstPragma* nodep) override {
        اگر (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه {
            iterateChildren(nodep);
        }
    }
    تهی visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        اگر (nodep->isIO()) m_underPortVar = درست;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    تهی visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        اگر (m_underPortVar) بازگشت;
        AstVar* ثابت varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        اگر (nodep->access().isReadOnly() && varp->constPoolEntry()) بازگشت;
        اگر (varp->user4u().toGraphVertex() != m_curVxp) {
            اگر (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    تهی visit(AstClass* nodep) override {
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
        Ast_گره* insertp = nullptr;
        برای (AstInitialAutomatic* initialp : m_initialps) {
            اگر (Ast_گره* ثابت movep = initialp->stmtsp()) {
                movep->unlinkFrBackWithNext();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_گره::addNextNull(insertp, movep);
            }
            VL_DO_DANGLING(افزودن_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    تهی visit(AstInitialAutomatic* nodep) override {
        m_initialps.افزودن_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = جدید TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.removeRedundantEdgesSum(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        اگر (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = پیشفرض;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

ساختار TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    ایستا std::vector<std::pair<صحیح, صحیح>> unpackDimsAndStrides(AstVar* varp) {
        Ast_گره_DType* ثابت dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<صحیح, صحیح>> dimStrides;
        AstUnpackArrayDType* ثابت unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        اگر (!unpackp) بازگشت dimStrides;

        ثابت std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ثابت size_t nDims = dims.size();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        برای (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        بازگشت dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    ایستا std::pair<std::string, منطقی> dpiToInternalCvtStmt(ثابت Ast_گره_DType* dtypep) {
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();

        // DPI types using Primitive C types
        اگر (basicp->keyword() == VBasicDTypeKwd::CHANDLE) بازگشت {"VL_CVT_VP_Q(", نادرست};
        اگر (basicp->keyword() == VBasicDTypeKwd::STRING) بازگشت {"VL_CVT_N_CSTR(", نادرست};
        اگر (basicp->isDpiPrimitive()) بازگشت {"(", نادرست};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ثابت std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ثابت نویسه sizeChar = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ثابت std::string& size = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        بازگشت {"VL_SET_"s + sizeChar + "_" + vecType + "(" + size + ", ", درست};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

رده TaskVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<ثابت string, std::tuple<Ast_گره_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ثابت m_statep;  // Common state between visitors
    Ast_گره_Module* m_modp = nullptr;  // Current module
    Ast_بالاترین_Scope* ثابت m_بالاترین_Scopep = v3Global.rootp()->بالاترین_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_گره* m_insStmtp = nullptr;  // Where to insert statement
    منطقی m_inSensesp = نادرست;  // Are we under a senitem?
    منطقی m_inNew = نادرست;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    صحیح m_uncon_Var_رقم = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, ثابت AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ثابت string& name, AstVar* examplep) {
        AstVar* ثابت newvarp = جدید AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(درست);
        funcp->addVarsp(newvarp);
        AstVarScope* ثابت newvscp = جدید AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        بازگشت newvscp;
    }
    AstVarScope* create_درون_داد_Var(AstCFunc* funcp, ثابت string& name, VBasicDTypeKwd kwd) {
        AstVar* ثابت newvarp
            = جدید AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(درست);
        newvarp->direction(VDirection::درون_داد);
        funcp->addArgsp(newvarp);
        AstVarScope* ثابت newvscp = جدید AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        بازگشت newvscp;
    }
    ایستا AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ثابت refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ثابت vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_درخت(), refp);
        بازگشت vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ثابت Ast_گره_FTask_Ref* refp, AstVar* invarp,
                                ثابت std::string& suffix) {
        اگر (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            بازگشت constPoolTable(invarp);
        } وگرنه {
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
            newvarp->funcLocal(نادرست);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(درست);
            بازگشت newvscp;
        }
    }

    // Replace varrefs with new var pointer
    تهی relink(Ast_گره* nodep) {
        nodep->foreachAndNext([](AstVarRef* refp) {
            اگر (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ثابت newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_گره_عبارت* pinp, AstVarScope* newvscp, منطقی pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_گره_عبارت* inPinp = pinp;
        اگر (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_گره_عبارت* inPinClonep
            = pureCheck ? inPinp->clone_درخت_Pure(درست) : inPinp->clone_درخت(درست);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = جدید AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* ثابت assp = جدید AstAssign{
            pinp->fileline(), جدید AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, درست);  // Ok if in <= block
        بازگشت assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_گره_عبارت* pinp, AstVarScope* newvscp,
                                        منطقی pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_گره_عبارت* outPinp = pinp;
        Ast_گره_عبارت* postRhsp = جدید AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        اگر (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            اگر (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, گره_Uniop)->lhsp();
                اگر (postRhsp->isSigned()) {
                    postRhsp = جدید AstISToRD{pinp->fileline(), postRhsp};
                } وگرنه {
                    postRhsp = جدید AstIToRD{pinp->fileline(), postRhsp};
                }
            } وگرنه {
                اگر (Ast_گره_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } وگرنه اگر (Ast_گره_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } وگرنه اگر (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } وگرنه {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                اگر (outPinp->width() < portp->width()) {
                    postRhsp = جدید AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } وگرنه {  // pin width > port width
                    اگر (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = جدید AstExtendS{pinp->fileline(), postRhsp};
                    } وگرنه {
                        postRhsp = جدید AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        اگر (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, گره_Uniop)->lhsp();
            postRhsp = جدید AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_گره_عبارت* ثابت outPinClonep
            = pureCheck ? outPinp->clone_درخت_Pure(درست) : outPinp->clone_درخت(درست);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = جدید AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* ثابت assp = جدید AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, درست);  // Ok if in <= block
        بازگشت assp;
    }

    تهی changeAtWriteRecurse(Ast_گره_عبارت* ثابت exprp) {
        // Change nested at methods to writable variant
        اگر (AstCMethodHard* ثابت cMethodp = VN_CAST(exprp, CMethodHard)) {
            اگر (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    تهی connectPort(ثابت Ast_گره_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_گره* beginp,
                     منطقی inlineTask) {
        Ast_گره_عبارت* pinp = argp->exprp();
        اگر (inlineTask) {
            portp->unlinkFrBack();
            افزودن_Deletep(portp);  // Remove it from the clone (not original)
        }
        اگر (!pinp) {
            // Too few arguments in function call
        } وگرنه {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            اگر (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* ثابت varp = جدید AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_رقم++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* ثابت newvscp = جدید AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* ثابت repp = جدید AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(افزودن_Deletep(pinp), pinp);
                pinp = repp;
            }
            اگر (inlineTask) {
                افزودن_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_درخت(), argp);  // Args no longer needed
            }
            اگر (portp->isRef() || portp->isConstRef()) {
                منطقی refArgOk = نادرست;
                اگر (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = درست;
                } وگرنه اگر (AstCMethodHard* ثابت cMethodp = VN_CAST(pinp, CMethodHard)) {
                    اگر (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } وگرنه {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                اگر (refArgOk) {
                    اگر (AstVarRef* ثابت varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } وگرنه {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                اگر (inlineTask) {
                    اگر (AstVarRef* ثابت varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* ثابت localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } وگرنه {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } وگرنه اگر (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                اگر (!inlineTask) {
                    pinp->replaceWith(
                        جدید AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    افزودن_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* ثابت preassp = connectPortMakeInAssign(pinp, newvscp, درست);
                beginp->addNextHere(preassp);

                AstAssign* ثابت postassp = connectPortMakeOutAssign(portp, pinp, newvscp, درست);
                beginp->addNext(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } وگرنه اگر (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                اگر (!inlineTask) {
                    pinp->replaceWith(جدید AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    افزودن_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* ثابت postassp = connectPortMakeOutAssign(portp, pinp, newvscp, نادرست);
                // Put assignment BEHIND of all other statements
                beginp->addNext(postassp);
            } وگرنه اگر (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* ثابت preassp = connectPortMakeInAssign(pinp, newvscp, نادرست);
                // Put assignment in FRONT of all other statements
                beginp->addNextHere(preassp);
            }
        }
    }

    منطقی hasRefArgument(Ast_گره_FTask* nodep) {
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isRef() || varp->isConstRef()) بازگشت درست;
            }
        }
        بازگشت نادرست;
    }

    Ast_گره* createInlinedFTask(Ast_گره_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_گره* ثابت newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_درخت(درست) : nullptr;
        Ast_گره* ثابت beginp
            = جدید AstComment{refp->fileline(), "Function: "s + refp->name(), درست};
        اگر (newbodysp) beginp->addNext(newbodysp);
        اگر (debug() >= 9) beginp->dump_درخت_And_Next(cout, "-  newbegi: ");
        //
        // Create input variables
        Ast_گره::user2_Clear_درخت();
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            برای (ثابت auto& itr : tconnects) {
                AstVar* ثابت portp = itr.first;
                AstArg* ثابت argp = itr.second;
                connectPort(refp, portp, argp, beginp, درست);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_گره* nextstmtp;
            برای (Ast_گره* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    اگر (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* ثابت localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        اگر (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_گره* ثابت crstp = جدید AstAssign{
                                portp->fileline(),
                                جدید AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                جدید AstCReset{portp->fileline(), portp, نادرست}};
                            portp->replaceWith(crstp);
                        } وگرنه {
                            portp->unlinkFrBack();
                        }
                        افزودن_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        اگر (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        اگر (debug() >= 9) beginp->dump_درخت_And_Next(cout, "-  iotask: ");
        بازگشت beginp;
    }

    Ast_گره* createNonInlinedFTask(Ast_گره_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ثابت cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_گره* ثابت beginp
            = جدید AstComment{refp->fileline(), "Function: "s + refp->name(), درست};
        Ast_گره_CCall* ccallp;
        اگر (VN_IS(refp, New)) {
            AstCNew* ثابت cnewp = جدید AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } وگرنه اگر (ثابت AstMethodCall* ثابت mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = جدید AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        } وگرنه {
            ccallp = جدید AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        }

        اگر (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } وگرنه اگر (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } وگرنه اگر (ثابت AstMethodCall* ثابت methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            برای (ثابت auto& itr : tconnects) {
                AstVar* ثابت portp = itr.first;
                AstArg* ثابت argp = itr.second;
                connectPort(refp, portp, argp, beginp, نادرست);
            }
        }
        // First argument is symbol table, then output if a function
        ثابت منطقی needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        اگر (needSyms) ccallp->argTypes("vlSymsp");

        اگر (refp->taskp()->dpiContext()) {
            AstScopeName* ثابت snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_سطر* ثابت flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                جدید AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(جدید AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        برای (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        اگر (outvscp) ccallp->addArgsp(جدید AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        اگر (debug() >= 9) beginp->dump_درخت_And_Next(cout, "-  nitask: ");
        بازگشت beginp;
    }

    string dpiSignature(Ast_گره_FTask* nodep, AstVar* rtnvarp) ثابت {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        اگر (nodep->dpiPure()) dpiproto += "pure ";
        اگر (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(درست, درست) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    اگر (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ثابت std::string dpiType = portp->dpiArgType(نادرست, نادرست);
                    dpiproto += dpiType;
                    ثابت std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(نادرست);
                    اگر (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        بازگشت dpiproto;
    }

    ایستا تهی checkLegalCIdentifier(Ast_گره* nodep, ثابت string& name) {
        اگر (name.end() != std::find_if(name.begin(), name.end(), [](نویسه c) {
                بازگشت !std::isalnum(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    ایستا Ast_گره* createDpiTemp(AstVar* portp, ثابت string& suffix) {
        ثابت string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        بازگشت جدید AstCStmt{portp->fileline(), stmt};
    }

    تهی unlinkAndClone(Ast_گره_FTask* funcp, Ast_گره* nodep, منطقی withNext) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        اگر (funcp->recursive()) {
            VNRelinker relinkHandle;
            اگر (withNext) {
                nodep->unlinkFrBackWithNext(&relinkHandle);
            } وگرنه {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_گره* ثابت newp = nodep->clone_درخت(withNext);
            relinkHandle.relink(newp);
        } وگرنه {
            اگر (withNext) {
                nodep->unlinkFrBackWithNext();
            } وگرنه {
                nodep->unlinkFrBack();
            }
        }
    }

    ایستا Ast_گره* createAssignInternalToDpi(AstVar* portp, منطقی isPtr, ثابت string& frSuffix,
                                              ثابت string& toSuffix) {
        ثابت string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        بازگشت جدید AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    ایستا Ast_گره_Stmt* createDpiScalarCvt(Ast_گره_عبارت* internalp, ثابت std::string& dpi,
                                           ثابت Ast_گره_DType* dtypep, منطقی toDpi) {
        File_سطر* ثابت flp = internalp->fileline();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        اگر (toDpi) {
            AstCStmt* ثابت cstmtp = جدید AstCStmt{flp};
            اگر (basicp->isDpiPrimitive()) {
                cstmtp->add(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->add(internalp);
                cstmtp->add(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } وگرنه {
                cstmtp->add((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::to_string(dtypep->width()) + ", " + dpi + ", ");
                cstmtp->add(internalp);
                cstmtp->add(");");
            }
            بازگشت cstmtp;
        }
        std::string cvt;
        منطقی useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        اگر (useSvVec) {
            AstCStmt* ثابت cstmtp = جدید AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            بازگشت cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ثابت صحیح cwidth = basicp->width_واژگان() * VL_EDATASIZE;
        Ast_گره_عبارت* ثابت rhsp = جدید AstCExpr{flp, cvt + dpi + ")", cwidth};
        بازگشت جدید AstAssign{flp, internalp, جدید AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    ایستا Ast_گره_Stmt* createDpiCvt(Ast_گره_عبارت* internalp, ثابت std::string& dpi,
                                     ثابت Ast_گره_DType* dtypep, منطقی toDpi) {
        File_سطر* ثابت flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_گره_Stmt* stmtsp = nullptr;
        اگر (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            برای (صحیح i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_گره_عبارت* ثابت elemp = جدید AstArraySel{flp, internalp->clone_درخت(نادرست), i};
                stmtsp = Ast_گره::addNext(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } وگرنه اگر (dtypep->containsUnpackedStruct()) {
            برای (ثابت AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ثابت selp
                    = جدید AstStructSel{flp, internalp->clone_درخت(نادرست), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_گره::addNext(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } وگرنه {
            بازگشت createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_درخت(), internalp);
        بازگشت stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    ایستا Ast_گره_Stmt* createDpiArgCvt(AstVarScope* vscp, ثابت std::string& dpiName,
                                        منطقی toDpi) {
        AstVar* ثابت varp = vscp->varp();
        File_سطر* ثابت flp = vscp->fileline();
        ثابت std::vector<std::pair<صحیح, صحیح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ثابت صحیح total = strides.empty() ? 1 : strides[0].first * strides[0].second;
        ثابت منطقی isStruct = varp->dtypep()->containsUnpackedStruct();
        ثابت Ast_گره_DType* ثابت elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_گره_Stmt* stmtsp = nullptr;
        برای (صحیح i = 0; i < total; ++i) {
            Ast_گره_عبارت* internalp
                = جدید AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            برای (ثابت auto& stride : strides) {
                internalp = جدید AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            اگر (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_واژگان());
            } وگرنه اگر (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_گره::addNext(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        بازگشت stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_گره_FTask* ثابت nodep, AstVar* ثابت rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_سطر* ثابت flp = nodep->fileline();
        ثابت std::string cname = Ast_گره::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ثابت نویسه* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ثابت std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(درست, درست) : "";
        // The function we are building
        AstCFunc* ثابت funcp = جدید AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(درست);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(درست);
        funcp->entryPoint(درست);
        funcp->isStatic(درست);
        funcp->protect(نادرست);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_بالاترین_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        اگر (!makePortList(nodep, funcp)) بازگشت nullptr;

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
        AstCStmt* ثابت prep = جدید AstCStmt{flp};
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
        AstCStmt* ثابت callp = جدید AstCStmt{flp};
        ثابت auto addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ثابت vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(نادرست);
            portp->protect(نادرست);
            // Add argument to call
            ثابت VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(جدید AstVarRef{portp->fileline(), vscp, access});
            بازگشت vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_گره* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت portp = VN_CAST(stmtp, Var);
            اگر (!portp) ادامه;
            اگر (!portp->isIO()) ادامه;
            اگر (portp->isFuncReturn()) ادامه;
            اگر (portp == rtnvarp) ادامه;  // Handled below
            // Add argument to call
            AstVarScope* ثابت outvscp = addFuncArg(portp);
            ثابت منطقی isStruct = portp->dtypep()->containsUnpackedStruct();
            اگر (portp->isWritable()) {
                outCvtsp = Ast_گره::addNext(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), درست)
                                       : createAssignInternalToDpi(portp, درست, tmpSuffixp, ""));
            }
            اگر (!portp->isNonOutput()) ادامه;
            // Convert input/inout arguments to dpi type
            ثابت std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(نادرست).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), نادرست));
        }
        // Return value argument goes last
        اگر (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        اگر (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        اگر (rtnvarp) {
            funcp->addStmtsp(createDpiTemp(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, نادرست, tmpSuffixp, ""));
            ثابت std::string index = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(جدید AstCStmt{flp, "return " + rtnvarp->name() + index + ";"});
        }
        بازگشت funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_گره_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ثابت string name = Ast_گره::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ثابت string rtnType = rtnvarp            ? rtnvarp->dpiArgType(درست, درست)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ثابت funcp = جدید AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(درست);
        funcp->dontCombine(درست);
        funcp->entryPoint(نادرست);
        funcp->isMethod(نادرست);
        funcp->protect(نادرست);
        funcp->dpiPure(nodep->dpiPure());

        ثابت صحیح cost = static_cast<صحیح>(V3Control::getProfileData(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_بالاترین_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        اگر (!makePortList(nodep, funcp)) بازگشت nullptr;
        بازگشت funcp;
    }

    AstCFunc* getDpiFunc(Ast_گره_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ثابت string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ثابت auto pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        اگر (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ثابت funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            بازگشت funcp;
        } وگرنه {
            // Seen this cname import before. Check if it's the same prototype.
            ثابت Ast_گره_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            اگر (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                بازگشت nullptr;
            } وگرنه {
                // Same signature, return the previously created CFunc
                بازگشت firstFuncp;
            }
        }
    }

    ایستا منطقی makePortList(Ast_گره_FTask* nodep, AstCFunc* dpip) {
        منطقی allOk = درست;
        // Copy nodep's list of function I/O to the new dpip c function
        برای (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isIO()) {
                    // Move it to new function
                    AstVar* ثابت newPortp = portp->clone_درخت(نادرست);
                    newPortp->funcLocal(درست);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    اگر (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = نادرست;
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
        بازگشت allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    ایستا منطقی isDpiCIdentifier(ثابت string& name) {
        بازگشت !name.empty() && VString::isIdentifier(name)
               && !std::isdigit(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_واژگان::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    منطقی checkDpiStruct(ثابت AstVar* portp, ثابت AstStructDType* sdtypep, منطقی nested) {
        ثابت std::string name = sdtypep->dpiName();
        اگر (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(درست) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            بازگشت نادرست;
        }
        برای (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            اگر (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                بازگشت نادرست;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ثابت Ast_گره_DType* dtypep = itemp->subDTypep()->skipRefp();
            وقتی (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ثابت Ast_گره_UOr_Struct_DType* ثابت subp = VN_CAST(dtypep, گره_UOr_Struct_DType);
            ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ثابت منطقی isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            اگر (dtypep->containsUnpackedStruct()) {
                اگر (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), درست)) بازگشت نادرست;
            } وگرنه اگر (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(درست)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                بازگشت نادرست;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        اگر (name.empty()) بازگشت درست;
        ثابت AstStructDType* ثابت firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        اگر (firstp->dpiTypedef() == sdtypep->dpiTypedef()) بازگشت درست;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        بازگشت نادرست;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    منطقی checkDpiStructArgs(ثابت Ast_گره_FTask* nodep) {
        منطقی allOk = درست;
        برای (ثابت Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var);
            اگر (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       نادرست)
                        && allOk;
            }
        }
        بازگشت allOk;
    }

    تهی bodyDpiImportFunc(Ast_گره_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ثابت نویسه* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        اگر (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_افزودن(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        برای (Ast_گره* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ثابت portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                اگر (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    اگر (args != "") args += ", ";

                    اگر (portp->isDpiOpenArray()) {
                        Ast_گره_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ثابت string propName = portp->name() + "__Vopenprops";
                        ثابت string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(جدید AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ثابت string name = portp->name() + "__Vopenarray";
                        ثابت string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(جدید AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } وگرنه {
                        // Unpacked structures are passed by reference from a C array
                        ثابت منطقی isStruct = portp->dtypep()->containsUnpackedStruct();
                        اگر (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            اگر (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(createDpiTemp(portp, tmpSuffixp));
                        اگر (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, درست)
                                    : createAssignInternalToDpi(portp, نادرست, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        اگر (nodep->dpiContext()) {
            ثابت string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(جدید AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ثابت callp = جدید AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            اگر (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(createDpiTemp(rtnvscp->varp(), tmpSuffixp));
                ثابت std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ثابت cstmtp = جدید AstCStmt{nodep->fileline()};
                cstmtp->add(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->add(" = ");
                cstmtp->add(callp);  // RHS
                cstmtp->add(";");
                cfuncp->addStmtsp(cstmtp);
            } وگرنه {
                // Othervise just call it
                cfuncp->addStmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        برای (Ast_گره* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                portp->protect(نادرست);  // No additional exposure - already part of shown proto
                اگر (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ثابت portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, نادرست));
                }
            }
        }

        اگر (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_برداشت(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        اگر (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_بالاترین_Scopep->createTemp("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        بازگشت dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_گره_FTask* nodep, منطقی ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_گره::user2_Clear_درخت();
        AstVar* rtnvarp = nullptr;
        اگر (nodep->isFunction()) {
            AstVar* ثابت portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            اگر (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ثابت bdtypep = portp->dtypep()->basicp();
                اگر (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(درست)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت nullptr;
                }
                اگر (!bdtypep->isDpiPrimitive()) {
                    اگر (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    اگر (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } وگرنه اگر (nodep->taskPublic()) {
                اگر (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            اگر (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(نادرست);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, نادرست);
            rtnvarp = portp;
            rtnvarp->funcLocal(درست);
            rtnvarp->noCReset(درست);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            اگر (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(نادرست);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = nullptr;
        اگر (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            اگر (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                اگر (rtnvarp) VL_DO_DANGLING(افزودن_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        اگر (rtnvarp) {
            rtnvscp = جدید AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        اگر (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } وگرنه اگر (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } وگرنه اگر (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        اگر (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        ثابت string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* ثابت cfuncp = جدید AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->taskPublic() && rtnvarp) ? rtnvarp->cPubArgType(درست, درست) : "")};
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
        اگر (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(درست);
            cfuncp->isLoose(درست);
        } وگرنه {
            cfuncp->isStatic(نادرست);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        اگر (nodep->name() == "new") cfuncp->isConstructor(درست);
        اگر (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(درست);
        اگر (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        اگر (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ثابت منطقی needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        اگر (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        اگر (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            اگر (cfuncp->isConstructor()) {
                منطقی isInterfaceClass = نادرست;
                اگر (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                اگر (!isInterfaceClass) {
                    ثابت string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        جدید AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        اگر (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_درون_داد_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_درون_داد_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_درون_داد_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        اگر (nodep->dpiExport()) {
            AstScopeName* ثابت snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(درست);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        برای (Ast_گره *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    افزودن_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } وگرنه {
                    اگر (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, نادرست);
                        portp->funcLocal(درست);
                        اگر (portp->valuep()) افزودن_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        اگر (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_گره_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                            اگر (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } وگرنه {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(درست);
                    }
                    AstVarScope* ثابت newvscp
                        = جدید AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        اگر (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_گره* bodysp = nodep->stmtsp();
        اگر (bodysp) {
            unlinkAndClone(nodep, bodysp, درست);
            AstBegin* ثابت tempp
                = جدید AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, نادرست};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            برای (Ast_گره *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    اگر (portp->isIO()) VL_DO_DANGLING(افزودن_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            اگر (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(tempp->delete_درخت(), tempp);
        }
        اگر (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        اگر (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(جدید AstCReturn{
                rtnvscp->fileline(), جدید AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        اگر (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            منطقی writesNonLocals = نادرست;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* ثابت varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                اگر (varp->isFuncLocal()) بازگشت;
                // Check if written
                اگر (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = درست;
                }
                // Check if read
                اگر (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            اگر (writesNonLocals) {
                AstVarScope* ثابت dpiExportTriggerp = getDpiExporTrigger();
                File_سطر* ثابت flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* ثابت assignp
                    = جدید AstAssign{flp, جدید AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    جدید AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                اگر (v3Global.opt.systemC()) {
                    assignp->addNextHere(جدید AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        اگر (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        بازگشت cfuncp;
    }

    تهی iterateIntoFTask(Ast_گره_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    تهی درج_Before_Stmt(Ast_گره* nodep, Ast_گره* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->addHereThisAsNext(newp);
    }
    تهی processArgs(Ast_گره_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        برای (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ثابت arrayp = VN_CAST(argp->exprp(), ConsArray);
            اگر (!arrayp) ادامه;

            File_سطر* ثابت flp = arrayp->fileline();
            AstVarScope* ثابت substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(درست);

            AstAssign* ثابت assignp
                = جدید AstAssign{flp, جدید AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_عبارت_Stmt* ثابت exprstmtp = جدید Ast_عبارت_Stmt{
                flp, assignp, جدید AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    منطقی isIfaceLocalImpure(Ast_گره_FTask* nodep, Ast_گره* impurep) {
        ثابت AstScope* ثابت scopep = m_statep->getScope(nodep);
        اگر (!TaskStateVisitor::isIfaceFTaskScope(scopep)) بازگشت نادرست;

        ثابت AstVarRef* ثابت refp = VN_CAST(impurep, VarRef);
        اگر (!refp || !refp->varScopep()) بازگشت نادرست;

        بازگشت refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    تهی visit(AstWith* nodep) override {
        اگر (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            بازگشت;
        }
        Ast_گره_عبارت* ثابت withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), گره_عبارت);
        nodep->addExprp(جدید AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    تهی visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    تهی visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = درست;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        اگر (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            بازگشت;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        اگر (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_شمار(), nodep, "No non-inline task, but no error issued");
            اگر (VN_IS(nodep->backp(), Stmt_عبارت)) {
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            } وگرنه {
                nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
            بازگشت;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        اگر (nodep->taskp()->isFunction()) {
            AstVar* ثابت fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            اگر (AstAssign* ثابت assignp = VN_CAST(nodep->backp(), Assign)) {
                اگر (AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ثابت vscp = lhsp->varScopep();
                    اگر (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_درخت(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            اگر (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_گره* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ثابت منطقی virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        اگر (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } وگرنه {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        اگر (VN_IS(nodep, New)) {  // New not legal as while() condition
            درج_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه اگر (VN_IS(nodep->backp(), گره_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            درج_Before_Stmt(nodep, beginp);
            AstVarRef* ثابت outrefp = جدید AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وگرنه اگر (!VN_IS(nodep->backp(), Stmt_عبارت)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ثابت outrefp = جدید AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_عبارت_Stmt* lambdap = جدید Ast_عبارت_Stmt{nodep->fileline(), beginp, outrefp};

            اگر (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_درخت(نادرست);
                newvarp->funcLocal(درست);

                // Create a new VarScope for the new variable
                AstVarScope* ثابت newvscp
                    = جدید AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreachAndNext([outvscp, newvscp](AstVarRef* refp) {
                    اگر (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->hasResult(نادرست);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ثابت exprp = جدید AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(جدید AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            VIsCached::clear_Cache_درخت();
        } وگرنه {  // VN_IS(nodep->backp(), StmtExpr)
            درج_Before_Stmt(nodep, beginp);
            اگر (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    تهی visit(Ast_گره_FTask* ثابت nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        اگر (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            صحیح modes = 0;
            اگر (nodep->dpiImport()) modes++;
            اگر (nodep->dpiExport()) modes++;
            اگر (nodep->taskPublic()) modes++;
            اگر (nodep->classMethod()) modes++;
            اگر (v3Global.opt.protectIds() && nodep->taskPublic()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            اگر (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            ثابت منطقی noInline = m_statep->ftaskNoInline(nodep);
            ثابت منطقی needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            اگر (noInline && !nodep->classMethod() && !nodep->recursive()) {
                اگر (Ast_گره* ثابت impurep = m_statep->checkImpure(nodep)) {
                    اگر (!isIfaceLocalImpure(nodep, impurep)) {
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

            اگر (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_گره_FTask* ثابت clonedFuncp = nodep->clone_درخت(نادرست);
                اگر (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ثابت منطقی makeNonInlineFunc = noInline || needsNonInlineCFunc;
                اگر (AstCFunc* ثابت cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->addNextHere(cfuncp);
                    اگر (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            اگر (nodep->isFunction()) {
                اگر (AstVar* ثابت portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* ثابت vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(افزودن_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            برای (Ast_گره *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ثابت vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(افزودن_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    تهی visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_گره_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_گره_FTask_Ref* refp) { refs.افزودن_back(refp); });
        برای (Ast_گره_FTask_Ref* ثابت refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ثابت interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* ثابت assignp = جدید AstAssign{
                nodep->fileline(), جدید AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_درخت_Pure(نادرست)};
            Ast_Sen_درخت* ثابت senTreep = جدید Ast_Sen_درخت{
                nodep->fileline(), جدید AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* ثابت activep
                = جدید AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_درخت_Storep(activep->sentreep());
            activep->addStmtsp(
                جدید AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(جدید AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_درخت(), refp);
        }
    }
    تهی visit(Ast_گره_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    تهی visit(Ast_گره_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_Stmt_عبارت* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        اگر (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }
    تهی visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = درست;
        iterateChildren(nodep);
    }
    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
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

رده TaskIfaceSensVisitor final : عمومی VNVisitorConst {
    // STATE
    منطقی m_underIfaceFunc = نادرست;  // Under a CFunc owned by an interface scope
    منطقی m_underSenses = نادرست;  // Under a sensitivity expression of such a CFunc

    // METHODS
    تهی markSensesAndIterate(Ast_گره* nodep) {
        اگر (!m_underIfaceFunc) بازگشت;
        VL_RESTORER(m_underSenses);
        m_underSenses = درست;
        iterateAndNextConstNull(nodep);
    }
    // VISITORS
    تهی visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    تهی visit(Ast_Sen_درخت* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    تهی visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    تهی visit(AstVarRef* nodep) override {
        اگر (!m_underSenses) بازگشت;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        اگر (nodep->varp()->isFuncLocal()) بازگشت;
        اگر (AstIface* ثابت ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = پیشفرض;
};

//######################################################################
// Task class functions

ثابت نویسه* ثابت V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_گره_FTask_Ref* nodep, Ast_گره* taskStmtsp,
                                    V3TaskConnectState* statep, منطقی makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ثابت std::string, صحیح> nameToIndex;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    صحیح tpinnum = 0;
    AstVar* sformatp = nullptr;
    برای (Ast_گره* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            اگر (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                nameToIndex.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                اگر (portp->attrSFormat()) {
                    sformatp = portp;
                } وگرنه اگر (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    صحیح ppinnum = 0;
    منطقی reorganize = نادرست;
    برای (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        اگر (!argp->name().empty()) {
            // By name
            ثابت auto it = nameToIndex.find(argp->name());
            اگر (it == nameToIndex.end()) {
                اگر (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_درخت(), argp);
                }
            } وگرنه {
                اگر (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_درخت();
                }
                tconnects[it->second].second = argp;
                reorganize = درست;
            }
        } وگرنه {  // By pin number
            اگر (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } وگرنه اگر (ppinnum >= tpinnum) {
                اگر (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } وگرنه اگر (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_درخت(), argp);
                }
            } وگرنه {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    اگر (!makeChanges) بازگشت tconnects;

    // Connect missing ones
    VInsertionSet<ثابت AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    برای (صحیح i = 0; i < tpinnum; ++i) {
        AstVar* ثابت portp = tconnects[i].first;
        اگر (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_گره_عبارت* newvaluep = nullptr;
            اگر (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = جدید AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } وگرنه اگر (AstFuncRef* ثابت funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ثابت Ast_گره_FTask* ثابت funcp = funcRefp->taskp();
                اگر (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } وگرنه اگر (AstConst* ثابت constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            اگر (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), گره_عبارت));
                اگر (!VN_IS(newvaluep, Const)) {
                    اگر (statep) {
                        portp->pin_رقم(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.درج(portp);
                    } وگرنه {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = جدید AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_درخت(درست) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ثابت newp = جدید AstArg{nodep->fileline(), portp->name(), newvaluep};
            اگر (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_درخت(),
                            tconnects[i].second = nullptr);
            }
            tconnects[i].second = newp;
            reorganize = درست;
        }
        اگر (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } وگرنه {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    برای (ثابت auto& tconnect : tconnects) {
        AstArg* ثابت argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    اگر (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        وقتی (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        برای (صحیح i = 0; i < tpinnum; ++i) {
            AstArg* ثابت argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    اگر (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_درخت("-  ftref-out: ");
        برای (صحیح i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    اگر (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        بازگشت taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    بازگشت tconnects;
}

تهی V3Task::taskConnectWrap(Ast_گره_FTask_Ref* nodep, ثابت V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ثابت VInsertionSet<ثابت AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    برای (ثابت AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_رقم());
    ثابت auto pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    اگر (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_گره_FTask* ثابت newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    برای (ثابت auto& tconnect : tconnects) {
        ثابت AstVar* ثابت portp = tconnect.first;
        AstArg* ثابت argp = tconnect.second;
        اگر (argWrap.exists(portp)) {  // Removed arg
            statep->افزودن_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_گره_FTask* V3Task::taskConnectWrapNew(Ast_گره_FTask* taskp, ثابت string& newname,
                                         ثابت V3TaskConnects& tconnects,
                                         ثابت VInsertionSet<ثابت AstVar*>& argWrap) {
    std::map<ثابت AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_گره_FTask* ثابت newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->addNextHere(newTaskp);

    Ast_گره_FTask_Ref* newCallp = nullptr;
    Ast_گره* newCallInsertp = nullptr;
    اگر (VN_IS(taskp, Func)) {
        AstVar* ثابت fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ثابت newFVarp = fvarp->clone_درخت(درست);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = جدید AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = جدید AstAssign{taskp->fileline(),
                            جدید AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } وگرنه اگر (VN_IS(taskp, Task)) {
        newCallp = جدید AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = جدید Ast_Stmt_عبارت{taskp->fileline(), newCallp};
    } وگرنه {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    برای (ثابت auto& tconnect : tconnects) {
        AstVar* ثابت portp = tconnect.first;
        AstVar* newPortp;
        اگر (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = جدید AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(درست);
            اگر (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_درخت();
            newTaskp->addStmtsp(newPortp);
        } وگرنه {  // Defaulting arg
            Ast_گره_عبارت* ثابت valuep = VN_AS(portp->valuep(), گره_عبارت);
            اگر ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ثابت VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ثابت refp = VN_AS(valuep->clone_درخت(نادرست), VarRef);
                refp->access(refAccess);
                AstArg* ثابت newArgp = جدید AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                ادامه;
            }
            // Create local temporary
            newPortp = جدید AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(درست);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            اگر (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* ثابت newAssignp
                    = جدید AstAssign{valuep->fileline(),
                                    جدید AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_درخت(درست)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ثابت newArgp = جدید AstArg{
            portp->fileline(), portp->name(),
            جدید AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ثابت auto it = oldNewVars.find(refp->varp());
        اگر (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    بازگشت newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, منطقی isPtr, ثابت string& frSuffix,
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
    صحیح unpackDim = 0;
    اگر (AstUnpackArrayDType* ثابت unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpackSize = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(نادرست).second;
        اگر (unpackDim > 0) UASSERT_OBJ(unpackSize > 0, portp, "size must be greater than 0");
    }
    اگر (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        ثابت منطقی isBit = portp->basicp()->isDpiBitVec();
        ثابت منطقی needsFor = unpackSize > 1;
        اگر (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpackSize) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        اگر (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_واژگان()) + " * " + idx;
        }
        stmt += ", ";
        اگر (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            برای (صحیح i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } وگرنه {
            stmt += frName + ")";
        }
    } وگرنه {
        ثابت منطقی isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        ثابت منطقی isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        ثابت string unpackAt = unpackSize > 1 ? "[" + idx + "]" : "[0]";
        اگر (unpackDim > 0) {
            ثابت string forStmt = unpackSize > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpackSize) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } وگرنه {
            اگر (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        اگر (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        اگر (unpackDim > 0) {
            stmt += "(&" + frName;
            برای (صحیح i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } وگرنه {
            stmt += frName;
        }
        اگر (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    بازگشت stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(ثابت std::string& lhsName, AstVar* varp) {
    std::string cvt;
    منطقی useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    ثابت std::vector<std::pair<صحیح, صحیح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ثابت صحیح total = strides.empty() ? 1 : strides[0].first * strides[0].second;
    ثابت صحیح width_واژگان = varp->basicp()->width_واژگان();
    string statements;
    برای (صحیح i = 0; i < total; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        برای (ثابت auto& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        اگر (useSvVec) {
            ثابت std::string offset = std::to_string(i * width_واژگان);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } وگرنه {
            ثابت std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    بازگشت statements;
}

تهی V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ثابت TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("task", 0, dump_درخت_Either_Level() >= 3);
}
