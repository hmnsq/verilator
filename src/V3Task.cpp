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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Task.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3ConstPool.h"
#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3LanguageWords.h"
#அந்தர்பாவ "V3SharedTmps.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

வகுப்பு TaskBaseVertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_முனை* m_impurep = nullptr;  // Node causing impure function w/ outside references
    தருக்க m_noInline = பொய்;  // Marked with pragma
பொது:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = இயல்பு;
    தருக்க pure() மாறிலி { திரும்பு m_impurep == nullptr; }
    Ast_முனை* impure_முனை() மாறிலி { திரும்பு m_impurep; }
    வெறுமை impure(Ast_முனை* nodep) { m_impurep = nodep; }
    தருக்க noInline() மாறிலி { திரும்பு m_noInline; }
    வெறுமை noInline(தருக்க flag) { m_noInline = flag; }
};

வகுப்பு TaskFTaskVertex final : பொது TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_முனை_FTask* மாறிலி m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    தருக்க m_needsNonInlineCFunc = பொய்;  // Needs CFunc for selected non-inlined call sites

பொது:
    TaskFTaskVertex(V3Graph* graphp, Ast_முனை_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = இயல்பு;
    Ast_முனை_FTask* nodep() மாறிலி VL_MT_STABLE { திரும்பு m_nodep; }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு nodep()->name(); }
    string dotColor() மாறிலி override { திரும்பு pure() ? "black" : "red"; }
    AstCFunc* cFuncp() மாறிலி { திரும்பு m_cFuncp; }
    வெறுமை cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    தருக்க needsNonInlineCFunc() மாறிலி { திரும்பு m_needsNonInlineCFunc; }
    வெறுமை needsNonInlineCFunc(தருக்க flag) { m_needsNonInlineCFunc = flag; }
};

வகுப்பு TaskCodeVertex final : பொது TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
பொது:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = இயல்பு;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு "*CODE*"; }
    string dotColor() மாறிலி override { திரும்பு "green"; }
};

வகுப்பு TaskEdge final : பொது V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
பொது:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* உச்சி)
        : V3GraphEdge{graphp, fromp, உச்சி, 1, பொய்} {}
    ~TaskEdge() override = இயல்பு;
    string dotLabel() மாறிலி override { திரும்பு "w" + cvtToStr(weight()); }
};

//######################################################################

வகுப்பு TaskStateVisitor final : பொது VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    மாறிலி VNUser3InUse m_inuser3;
    மாறிலி VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<மாறிலி Ast_முனை_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_முனை_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    தருக்க m_underPortVar = பொய்;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

பொது:
    // METHODS
    நிலையான தருக்க isVirtualIfaceMethodCall(மாறிலி Ast_முனை_FTask_Ref* refp) {
        மாறிலி AstMethodCall* மாறிலி mrefp = VN_CAST(refp, MethodCall);
        எனில் (!mrefp) திரும்பு பொய்;
        மாறிலி Ast_முனை_கோவை* மாறிலி fromp = mrefp->fromp();
        எனில் (!fromp || !fromp->dtypep()) திரும்பு பொய்;
        மாறிலி AstIfaceRefDType* மாறிலி ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        திரும்பு ifrefp && ifrefp->isVirtual();
    }
    நிலையான தருக்க isIfaceFTaskScope(மாறிலி AstScope* scopep) {
        திரும்பு scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_முனை_FTask* nodep) {
        AstScope* மாறிலி scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        திரும்பு scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        மாறிலி auto iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        திரும்பு iter->second;
    }
    AstClass* getClassp(Ast_முனை_FTask* nodep) {
        AstClass* மாறிலி classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        திரும்பு classp;
    }
    வெறுமை remapFuncClassp(Ast_முனை_FTask* nodep, Ast_முனை_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    தருக்க ftaskNoInline(Ast_முனை_FTask* nodep) { திரும்பு getFTaskVertex(nodep)->noInline(); }
    தருக்க ftaskNeedsNonInlineCFunc(Ast_முனை_FTask* nodep) {
        திரும்பு getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_முனை_FTask* nodep) { திரும்பு getFTaskVertex(nodep)->cFuncp(); }
    வெறுமை ftaskCFuncp(Ast_முனை_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_முனை* checkImpure(Ast_முனை_FTask* nodep) { திரும்பு getFTaskVertex(nodep)->impure_முனை(); }

தனிப்பட்ட:
    TaskFTaskVertex* getFTaskVertex(Ast_முனை_FTask* nodep) {
        எனில் (!nodep->user4p()) nodep->user4p(புதிய TaskFTaskVertex{&m_callGraph, nodep});
        திரும்பு static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    வெறுமை propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        க்கு (V3GraphVertex& vtx : m_callGraph.vertices()) {
            எனில் (TaskFTaskVertex* மாறிலி ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                எனில் (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        போது (!workList.empty()) {
            TaskFTaskVertex* மாறிலி calleep = workList.back();
            workList.எடு_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            க்கு (V3GraphEdge& edge : calleep->inEdges()) {
                எனில் (TaskFTaskVertex* மாறிலி callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    எனில் (!callerp->pure()) தொடர்;  // Already processed or on workList
                    callerp->impure(calleep->impure_முனை());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    வெறுமை decideInlining() {
        க்கு (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* மாறிலி ftVtxp = vtx.cast<TaskFTaskVertex>();
            எனில் (!ftVtxp) தொடர்;
            // Move on if already decided not to inline
            எனில் (ftVtxp->noInline()) தொடர்;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            எனில் (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(உண்மை);
                தொடர்;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            எனில் (v3Global.opt.fInlineFuncsEager()) தொடர்;

            Ast_முனை_FTask* மாறிலி ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            எனில் (!ftaskp->verilogFunction()) தொடர்;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            எனில் (ftaskp->taskPublic()) தொடர்;
            // If the function is pure, it's OK not to inline, so don't do it
            எனில் (ftVtxp->pure()) ftVtxp->noInline(உண்மை);
        }
    }

    // VISITORS
    வெறுமை visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        க்கு (Ast_முனை* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVarScope* மாறிலி vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* மாறிலி varp = vscp->varp();
                எனில் (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        க்கு (Ast_முனை* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (Ast_முனை_FTask* மாறிலி taskp = VN_CAST(stmtp, முனை_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* மாறிலி taskVtxp = getFTaskVertex(nodep->taskp());
        புதிய TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        எனில் (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(உண்மை);
        // Do we have to disable inlining the function?
        மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        எனில் (!taskVtxp->noInline()) {  // Else short-circuit below
            க்கு (மாறிலி auto& itr : tconnects) {
                மாறிலி AstVar* மாறிலி portp = itr.first;
                மாறிலி AstArg* மாறிலி argp = itr.second;
                எனில் (மாறிலி Ast_முனை_கோவை* மாறிலி pinp = argp->exprp()) {
                    எனில் ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(உண்மை);
                    }
                }
            }
        }
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        எனில் (nodep->dpiImport()) m_curVxp->noInline(உண்மை);
        எனில் (nodep->classMethod()) m_curVxp->noInline(உண்மை);  // Until V3Task supports it
        எனில் (nodep->recursive()) m_curVxp->noInline(உண்மை);
        எனில் (nodep->isConstructor()) {
            m_curVxp->noInline(உண்மை);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstPragma* nodep) override {
        எனில் (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        எனில் (nodep->isIO()) m_underPortVar = உண்மை;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    வெறுமை visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        எனில் (m_underPortVar) திரும்பு;
        AstVar* மாறிலி varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        எனில் (nodep->access().isReadOnly() && varp->constPoolEntry()) திரும்பு;
        எனில் (varp->user4u().toGraphVertex() != m_curVxp) {
            எனில் (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    வெறுமை visit(AstClass* nodep) override {
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
        Ast_முனை* insertp = nullptr;
        க்கு (AstInitialAutomatic* initialp : m_initialps) {
            எனில் (Ast_முனை* மாறிலி movep = initialp->stmtsp()) {
                movep->unlinkFrBackWithNext();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_முனை::addNextNull(insertp, movep);
            }
            VL_DO_DANGLING(தள்ளு_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    வெறுமை visit(AstInitialAutomatic* nodep) override {
        m_initialps.தள்ளு_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = புதிய TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.removeRedundantEdgesSum(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        எனில் (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = இயல்பு;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

கட்டமைப்பு TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    நிலையான std::vector<std::pair<முழு, முழு>> unpackDimsAndStrides(AstVar* varp) {
        Ast_முனை_DType* மாறிலி dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<முழு, முழு>> dimStrides;
        AstUnpackArrayDType* மாறிலி unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        எனில் (!unpackp) திரும்பு dimStrides;

        மாறிலி std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        மாறிலி size_t nDims = dims.size();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        க்கு (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        திரும்பு dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    நிலையான std::pair<std::string, தருக்க> dpiToInternalCvtStmt(மாறிலி Ast_முனை_DType* dtypep) {
        மாறிலி AstBasicDType* மாறிலி basicp = dtypep->basicp();

        // DPI types using Primitive C types
        எனில் (basicp->keyword() == VBasicDTypeKwd::CHANDLE) திரும்பு {"VL_CVT_VP_Q(", பொய்};
        எனில் (basicp->keyword() == VBasicDTypeKwd::STRING) திரும்பு {"VL_CVT_N_CSTR(", பொய்};
        எனில் (basicp->isDpiPrimitive()) திரும்பு {"(", பொய்};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        மாறிலி std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        மாறிலி எழுத்து sizeChar = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        மாறிலி std::string& size = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        திரும்பு {"VL_SET_"s + sizeChar + "_" + vecType + "(" + size + ", ", உண்மை};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

வகுப்பு TaskVisitor final : பொது VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<மாறிலி string, std::tuple<Ast_முனை_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* மாறிலி m_statep;  // Common state between visitors
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    Ast_உச்சி_Scope* மாறிலி m_உச்சி_Scopep = v3Global.rootp()->உச்சி_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_முனை* m_insStmtp = nullptr;  // Where to insert statement
    தருக்க m_inSensesp = பொய்;  // Are we under a senitem?
    தருக்க m_inNew = பொய்;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    முழு m_uncon_Var_இலக்கம் = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, மாறிலி AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, மாறிலி string& name, AstVar* examplep) {
        AstVar* மாறிலி newvarp = புதிய AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(உண்மை);
        funcp->addVarsp(newvarp);
        AstVarScope* மாறிலி newvscp = புதிய AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        திரும்பு newvscp;
    }
    AstVarScope* create_உள்ளீடு_Var(AstCFunc* funcp, மாறிலி string& name, VBasicDTypeKwd kwd) {
        AstVar* மாறிலி newvarp
            = புதிய AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(உண்மை);
        newvarp->direction(VDirection::உள்ளீடு);
        funcp->addArgsp(newvarp);
        AstVarScope* மாறிலி newvscp = புதிய AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        திரும்பு newvscp;
    }
    நிலையான AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* மாறிலி refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* மாறிலி vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_மரம்(), refp);
        திரும்பு vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(மாறிலி Ast_முனை_FTask_Ref* refp, AstVar* invarp,
                                மாறிலி std::string& suffix) {
        எனில் (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            திரும்பு constPoolTable(invarp);
        } இல்லையெனில் {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* மாறிலி newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* மாறிலி newvarp = newvscp->varp();
            newvarp->funcLocal(பொய்);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(உண்மை);
            திரும்பு newvscp;
        }
    }

    // Replace varrefs with new var pointer
    வெறுமை relink(Ast_முனை* nodep) {
        nodep->foreachAndNext([](AstVarRef* refp) {
            எனில் (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* மாறிலி newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_முனை_கோவை* pinp, AstVarScope* newvscp, தருக்க pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_முனை_கோவை* inPinp = pinp;
        எனில் (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_முனை_கோவை* inPinClonep
            = pureCheck ? inPinp->clone_மரம்_Pure(உண்மை) : inPinp->clone_மரம்(உண்மை);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        மாறிலி AstUnpackArrayDType* மாறிலி arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        எனில் (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = புதிய AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* மாறிலி assp = புதிய AstAssign{
            pinp->fileline(), புதிய AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, உண்மை);  // Ok if in <= block
        திரும்பு assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_முனை_கோவை* pinp, AstVarScope* newvscp,
                                        தருக்க pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_முனை_கோவை* outPinp = pinp;
        Ast_முனை_கோவை* postRhsp = புதிய AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        எனில் (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            எனில் (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, முனை_Uniop)->lhsp();
                எனில் (postRhsp->isSigned()) {
                    postRhsp = புதிய AstISToRD{pinp->fileline(), postRhsp};
                } இல்லையெனில் {
                    postRhsp = புதிய AstIToRD{pinp->fileline(), postRhsp};
                }
            } இல்லையெனில் {
                எனில் (Ast_முனை_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } இல்லையெனில் எனில் (Ast_முனை_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } இல்லையெனில் எனில் (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } இல்லையெனில் {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                எனில் (outPinp->width() < portp->width()) {
                    postRhsp = புதிய AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } இல்லையெனில் {  // pin width > port width
                    எனில் (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = புதிய AstExtendS{pinp->fileline(), postRhsp};
                    } இல்லையெனில் {
                        postRhsp = புதிய AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        எனில் (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, முனை_Uniop)->lhsp();
            postRhsp = புதிய AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_முனை_கோவை* மாறிலி outPinClonep
            = pureCheck ? outPinp->clone_மரம்_Pure(உண்மை) : outPinp->clone_மரம்(உண்மை);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        மாறிலி AstUnpackArrayDType* மாறிலி arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        எனில் (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = புதிய AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* மாறிலி assp = புதிய AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, உண்மை);  // Ok if in <= block
        திரும்பு assp;
    }

    வெறுமை changeAtWriteRecurse(Ast_முனை_கோவை* மாறிலி exprp) {
        // Change nested at methods to writable variant
        எனில் (AstCMethodHard* மாறிலி cMethodp = VN_CAST(exprp, CMethodHard)) {
            எனில் (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    வெறுமை connectPort(மாறிலி Ast_முனை_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_முனை* beginp,
                     தருக்க inlineTask) {
        Ast_முனை_கோவை* pinp = argp->exprp();
        எனில் (inlineTask) {
            portp->unlinkFrBack();
            தள்ளு_Deletep(portp);  // Remove it from the clone (not original)
        }
        எனில் (!pinp) {
            // Too few arguments in function call
        } இல்லையெனில் {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            எனில் (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* மாறிலி varp = புதிய AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_இலக்கம்++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* மாறிலி newvscp = புதிய AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* மாறிலி repp = புதிய AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(தள்ளு_Deletep(pinp), pinp);
                pinp = repp;
            }
            எனில் (inlineTask) {
                தள்ளு_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_மரம்(), argp);  // Args no longer needed
            }
            எனில் (portp->isRef() || portp->isConstRef()) {
                தருக்க refArgOk = பொய்;
                எனில் (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = உண்மை;
                } இல்லையெனில் எனில் (AstCMethodHard* மாறிலி cMethodp = VN_CAST(pinp, CMethodHard)) {
                    எனில் (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } இல்லையெனில் {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                எனில் (refArgOk) {
                    எனில் (AstVarRef* மாறிலி varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } இல்லையெனில் {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                எனில் (inlineTask) {
                    எனில் (AstVarRef* மாறிலி varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* மாறிலி localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } இல்லையெனில் {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } இல்லையெனில் எனில் (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* மாறிலி newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                எனில் (!inlineTask) {
                    pinp->replaceWith(
                        புதிய AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    தள்ளு_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* மாறிலி preassp = connectPortMakeInAssign(pinp, newvscp, உண்மை);
                beginp->addNextHere(preassp);

                AstAssign* மாறிலி postassp = connectPortMakeOutAssign(portp, pinp, newvscp, உண்மை);
                beginp->addNext(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } இல்லையெனில் எனில் (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* மாறிலி newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                எனில் (!inlineTask) {
                    pinp->replaceWith(புதிய AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    தள்ளு_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* மாறிலி postassp = connectPortMakeOutAssign(portp, pinp, newvscp, பொய்);
                // Put assignment BEHIND of all other statements
                beginp->addNext(postassp);
            } இல்லையெனில் எனில் (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* மாறிலி newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* மாறிலி preassp = connectPortMakeInAssign(pinp, newvscp, பொய்);
                // Put assignment in FRONT of all other statements
                beginp->addNextHere(preassp);
            }
        }
    }

    தருக்க hasRefArgument(Ast_முனை_FTask* nodep) {
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (varp->isRef() || varp->isConstRef()) திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;
    }

    Ast_முனை* createInlinedFTask(Ast_முனை_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_முனை* மாறிலி newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_மரம்(உண்மை) : nullptr;
        Ast_முனை* மாறிலி beginp
            = புதிய AstComment{refp->fileline(), "Function: "s + refp->name(), உண்மை};
        எனில் (newbodysp) beginp->addNext(newbodysp);
        எனில் (debug() >= 9) beginp->dump_மரம்_And_Next(cout, "-  newbegi: ");
        //
        // Create input variables
        Ast_முனை::user2_Clear_மரம்();
        {
            மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            க்கு (மாறிலி auto& itr : tconnects) {
                AstVar* மாறிலி portp = itr.first;
                AstArg* மாறிலி argp = itr.second;
                connectPort(refp, portp, argp, beginp, உண்மை);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_முனை* nextstmtp;
            க்கு (Ast_முனை* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    எனில் (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* மாறிலி localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        எனில் (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_முனை* மாறிலி crstp = புதிய AstAssign{
                                portp->fileline(),
                                புதிய AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                புதிய AstCReset{portp->fileline(), portp, பொய்}};
                            portp->replaceWith(crstp);
                        } இல்லையெனில் {
                            portp->unlinkFrBack();
                        }
                        தள்ளு_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        எனில் (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        எனில் (debug() >= 9) beginp->dump_மரம்_And_Next(cout, "-  iotask: ");
        திரும்பு beginp;
    }

    Ast_முனை* createNonInlinedFTask(Ast_முனை_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* மாறிலி cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_முனை* மாறிலி beginp
            = புதிய AstComment{refp->fileline(), "Function: "s + refp->name(), உண்மை};
        Ast_முனை_CCall* ccallp;
        எனில் (VN_IS(refp, New)) {
            AstCNew* மாறிலி cnewp = புதிய AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } இல்லையெனில் எனில் (மாறிலி AstMethodCall* மாறிலி mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = புதிய AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        } இல்லையெனில் {
            ccallp = புதிய AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        }

        எனில் (மாறிலி AstFuncRef* மாறிலி funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } இல்லையெனில் எனில் (மாறிலி AstTaskRef* மாறிலி taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } இல்லையெனில் எனில் (மாறிலி AstMethodCall* மாறிலி methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            மாறிலி V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            க்கு (மாறிலி auto& itr : tconnects) {
                AstVar* மாறிலி portp = itr.first;
                AstArg* மாறிலி argp = itr.second;
                connectPort(refp, portp, argp, beginp, பொய்);
            }
        }
        // First argument is symbol table, then output if a function
        மாறிலி தருக்க needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        எனில் (needSyms) ccallp->argTypes("vlSymsp");

        எனில் (refp->taskp()->dpiContext()) {
            AstScopeName* மாறிலி snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_வரி* மாறிலி flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                புதிய AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(புதிய AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        க்கு (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        எனில் (outvscp) ccallp->addArgsp(புதிய AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        எனில் (debug() >= 9) beginp->dump_மரம்_And_Next(cout, "-  nitask: ");
        திரும்பு beginp;
    }

    string dpiSignature(Ast_முனை_FTask* nodep, AstVar* rtnvarp) மாறிலி {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        எனில் (nodep->dpiPure()) dpiproto += "pure ";
        எனில் (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(உண்மை, உண்மை) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (மாறிலி AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                எனில் (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    எனில் (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    மாறிலி std::string dpiType = portp->dpiArgType(பொய், பொய்);
                    dpiproto += dpiType;
                    மாறிலி std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(பொய்);
                    எனில் (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        திரும்பு dpiproto;
    }

    நிலையான வெறுமை checkLegalCIdentifier(Ast_முனை* nodep, மாறிலி string& name) {
        எனில் (name.end() != std::find_if(name.begin(), name.end(), [](எழுத்து c) {
                திரும்பு !std::isalnum(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    நிலையான Ast_முனை* createDpiTemp(AstVar* portp, மாறிலி string& suffix) {
        மாறிலி string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        திரும்பு புதிய AstCStmt{portp->fileline(), stmt};
    }

    வெறுமை unlinkAndClone(Ast_முனை_FTask* funcp, Ast_முனை* nodep, தருக்க withNext) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        எனில் (funcp->recursive()) {
            VNRelinker relinkHandle;
            எனில் (withNext) {
                nodep->unlinkFrBackWithNext(&relinkHandle);
            } இல்லையெனில் {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_முனை* மாறிலி newp = nodep->clone_மரம்(withNext);
            relinkHandle.relink(newp);
        } இல்லையெனில் {
            எனில் (withNext) {
                nodep->unlinkFrBackWithNext();
            } இல்லையெனில் {
                nodep->unlinkFrBack();
            }
        }
    }

    நிலையான Ast_முனை* createAssignInternalToDpi(AstVar* portp, தருக்க isPtr, மாறிலி string& frSuffix,
                                              மாறிலி string& toSuffix) {
        மாறிலி string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        திரும்பு புதிய AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    நிலையான Ast_முனை_Stmt* createDpiScalarCvt(Ast_முனை_கோவை* internalp, மாறிலி std::string& dpi,
                                           மாறிலி Ast_முனை_DType* dtypep, தருக்க toDpi) {
        File_வரி* மாறிலி flp = internalp->fileline();
        மாறிலி AstBasicDType* மாறிலி basicp = dtypep->basicp();
        எனில் (toDpi) {
            AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{flp};
            எனில் (basicp->isDpiPrimitive()) {
                cstmtp->add(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->add(internalp);
                cstmtp->add(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } இல்லையெனில் {
                cstmtp->add((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::to_string(dtypep->width()) + ", " + dpi + ", ");
                cstmtp->add(internalp);
                cstmtp->add(");");
            }
            திரும்பு cstmtp;
        }
        std::string cvt;
        தருக்க useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        எனில் (useSvVec) {
            AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            திரும்பு cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        மாறிலி முழு cwidth = basicp->width_சொற்கள்() * VL_EDATASIZE;
        Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstCExpr{flp, cvt + dpi + ")", cwidth};
        திரும்பு புதிய AstAssign{flp, internalp, புதிய AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    நிலையான Ast_முனை_Stmt* createDpiCvt(Ast_முனை_கோவை* internalp, மாறிலி std::string& dpi,
                                     மாறிலி Ast_முனை_DType* dtypep, தருக்க toDpi) {
        File_வரி* மாறிலி flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_முனை_Stmt* stmtsp = nullptr;
        எனில் (மாறிலி AstUnpackArrayDType* மாறிலி arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            க்கு (முழு i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_முனை_கோவை* மாறிலி elemp = புதிய AstArraySel{flp, internalp->clone_மரம்(பொய்), i};
                stmtsp = Ast_முனை::addNext(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } இல்லையெனில் எனில் (dtypep->containsUnpackedStruct()) {
            க்கு (மாறிலி AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* மாறிலி selp
                    = புதிய AstStructSel{flp, internalp->clone_மரம்(பொய்), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_முனை::addNext(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } இல்லையெனில் {
            திரும்பு createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_மரம்(), internalp);
        திரும்பு stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    நிலையான Ast_முனை_Stmt* createDpiArgCvt(AstVarScope* vscp, மாறிலி std::string& dpiName,
                                        தருக்க toDpi) {
        AstVar* மாறிலி varp = vscp->varp();
        File_வரி* மாறிலி flp = vscp->fileline();
        மாறிலி std::vector<std::pair<முழு, முழு>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        மாறிலி முழு total = strides.empty() ? 1 : strides[0].first * strides[0].second;
        மாறிலி தருக்க isStruct = varp->dtypep()->containsUnpackedStruct();
        மாறிலி Ast_முனை_DType* மாறிலி elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_முனை_Stmt* stmtsp = nullptr;
        க்கு (முழு i = 0; i < total; ++i) {
            Ast_முனை_கோவை* internalp
                = புதிய AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            க்கு (மாறிலி auto& stride : strides) {
                internalp = புதிய AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            எனில் (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_சொற்கள்());
            } இல்லையெனில் எனில் (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_முனை::addNext(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        திரும்பு stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_முனை_FTask* மாறிலி nodep, AstVar* மாறிலி rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_வரி* மாறிலி flp = nodep->fileline();
        மாறிலி std::string cname = Ast_முனை::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        மாறிலி எழுத்து* மாறிலி tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        மாறிலி std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(உண்மை, உண்மை) : "";
        // The function we are building
        AstCFunc* மாறிலி funcp = புதிய AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(உண்மை);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(உண்மை);
        funcp->entryPoint(உண்மை);
        funcp->isStatic(உண்மை);
        funcp->protect(பொய்);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_உச்சி_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        எனில் (!makePortList(nodep, funcp)) திரும்பு nullptr;

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
        AstCStmt* மாறிலி prep = புதிய AstCStmt{flp};
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
        மாறிலி std::string cbtype
            = VIdProtect::protect(v3Global.opt.prefix() + "__Vcb_" + nodep->cname() + "_t");
        prep->add(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* மாறிலி callp = புதிய AstCStmt{flp};
        மாறிலி auto addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* மாறிலி vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(பொய்);
            portp->protect(பொய்);
            // Add argument to call
            மாறிலி VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(புதிய AstVarRef{portp->fileline(), vscp, access});
            திரும்பு vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_முனை* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* மாறிலி portp = VN_CAST(stmtp, Var);
            எனில் (!portp) தொடர்;
            எனில் (!portp->isIO()) தொடர்;
            எனில் (portp->isFuncReturn()) தொடர்;
            எனில் (portp == rtnvarp) தொடர்;  // Handled below
            // Add argument to call
            AstVarScope* மாறிலி outvscp = addFuncArg(portp);
            மாறிலி தருக்க isStruct = portp->dtypep()->containsUnpackedStruct();
            எனில் (portp->isWritable()) {
                outCvtsp = Ast_முனை::addNext(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), உண்மை)
                                       : createAssignInternalToDpi(portp, உண்மை, tmpSuffixp, ""));
            }
            எனில் (!portp->isNonOutput()) தொடர்;
            // Convert input/inout arguments to dpi type
            மாறிலி std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(பொய்).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), பொய்));
        }
        // Return value argument goes last
        எனில் (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        எனில் (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        எனில் (rtnvarp) {
            funcp->addStmtsp(createDpiTemp(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, பொய், tmpSuffixp, ""));
            மாறிலி std::string index = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(புதிய AstCStmt{flp, "return " + rtnvarp->name() + index + ";"});
        }
        திரும்பு funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_முனை_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        மாறிலி string name = Ast_முனை::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        மாறிலி string rtnType = rtnvarp            ? rtnvarp->dpiArgType(உண்மை, உண்மை)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* மாறிலி funcp = புதிய AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(உண்மை);
        funcp->dontCombine(உண்மை);
        funcp->entryPoint(பொய்);
        funcp->isMethod(பொய்);
        funcp->protect(பொய்);
        funcp->dpiPure(nodep->dpiPure());

        மாறிலி முழு cost = static_cast<முழு>(V3Control::getProfileData(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_உச்சி_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        எனில் (!makePortList(nodep, funcp)) திரும்பு nullptr;
        திரும்பு funcp;
    }

    AstCFunc* getDpiFunc(Ast_முனை_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        மாறிலி string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        மாறிலி auto pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        எனில் (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* மாறிலி funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            திரும்பு funcp;
        } இல்லையெனில் {
            // Seen this cname import before. Check if it's the same prototype.
            மாறிலி Ast_முனை_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            எனில் (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                திரும்பு nullptr;
            } இல்லையெனில் {
                // Same signature, return the previously created CFunc
                திரும்பு firstFuncp;
            }
        }
    }

    நிலையான தருக்க makePortList(Ast_முனை_FTask* nodep, AstCFunc* dpip) {
        தருக்க allOk = உண்மை;
        // Copy nodep's list of function I/O to the new dpip c function
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                எனில் (portp->isIO()) {
                    // Move it to new function
                    AstVar* மாறிலி newPortp = portp->clone_மரம்(பொய்);
                    newPortp->funcLocal(உண்மை);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    எனில் (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = பொய்;
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
        திரும்பு allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    நிலையான தருக்க isDpiCIdentifier(மாறிலி string& name) {
        திரும்பு !name.empty() && VString::isIdentifier(name)
               && !std::isdigit(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_சொற்கள்::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    தருக்க checkDpiStruct(மாறிலி AstVar* portp, மாறிலி AstStructDType* sdtypep, தருக்க nested) {
        மாறிலி std::string name = sdtypep->dpiName();
        எனில் (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(உண்மை) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            திரும்பு பொய்;
        }
        க்கு (மாறிலி AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            எனில் (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                திரும்பு பொய்;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            மாறிலி Ast_முனை_DType* dtypep = itemp->subDTypep()->skipRefp();
            போது (மாறிலி AstUnpackArrayDType* மாறிலி arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி subp = VN_CAST(dtypep, முனை_UOr_Struct_DType);
            மாறிலி AstBasicDType* மாறிலி basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            மாறிலி தருக்க isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            எனில் (dtypep->containsUnpackedStruct()) {
                எனில் (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), உண்மை)) திரும்பு பொய்;
            } இல்லையெனில் எனில் (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(உண்மை)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                திரும்பு பொய்;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        எனில் (name.empty()) திரும்பு உண்மை;
        மாறிலி AstStructDType* மாறிலி firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        எனில் (firstp->dpiTypedef() == sdtypep->dpiTypedef()) திரும்பு உண்மை;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        திரும்பு பொய்;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    தருக்க checkDpiStructArgs(மாறிலி Ast_முனை_FTask* nodep) {
        தருக்க allOk = உண்மை;
        க்கு (மாறிலி Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            மாறிலி AstVar* மாறிலி portp = VN_CAST(stmtp, Var);
            எனில் (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       பொய்)
                        && allOk;
            }
        }
        திரும்பு allOk;
    }

    வெறுமை bodyDpiImportFunc(Ast_முனை_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        மாறிலி எழுத்து* மாறிலி tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        எனில் (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_தள்ளு(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        க்கு (Ast_முனை* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                AstVarScope* மாறிலி portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                எனில் (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    எனில் (args != "") args += ", ";

                    எனில் (portp->isDpiOpenArray()) {
                        Ast_முனை_DType* மாறிலி dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        மாறிலி string propName = portp->name() + "__Vopenprops";
                        மாறிலி string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(புதிய AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        மாறிலி string name = portp->name() + "__Vopenarray";
                        மாறிலி string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(புதிய AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } இல்லையெனில் {
                        // Unpacked structures are passed by reference from a C array
                        மாறிலி தருக்க isStruct = portp->dtypep()->containsUnpackedStruct();
                        எனில் (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            எனில் (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(createDpiTemp(portp, tmpSuffixp));
                        எனில் (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, உண்மை)
                                    : createAssignInternalToDpi(portp, பொய், "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        எனில் (nodep->dpiContext()) {
            மாறிலி string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(புதிய AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* மாறிலி callp = புதிய AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            எனில் (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(createDpiTemp(rtnvscp->varp(), tmpSuffixp));
                மாறிலி std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{nodep->fileline()};
                cstmtp->add(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->add(" = ");
                cstmtp->add(callp);  // RHS
                cstmtp->add(";");
                cfuncp->addStmtsp(cstmtp);
            } இல்லையெனில் {
                // Othervise just call it
                cfuncp->addStmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        க்கு (Ast_முனை* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                portp->protect(பொய்);  // No additional exposure - already part of shown proto
                எனில் (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* மாறிலி portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, பொய்));
                }
            }
        }

        எனில் (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_எடு(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* மாறிலி netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        எனில் (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_உச்சி_Scopep->createTemp("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        திரும்பு dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_முனை_FTask* nodep, தருக்க ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_முனை::user2_Clear_மரம்();
        AstVar* rtnvarp = nullptr;
        எனில் (nodep->isFunction()) {
            AstVar* மாறிலி portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            எனில் (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* மாறிலி bdtypep = portp->dtypep()->basicp();
                எனில் (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(உண்மை)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு nullptr;
                }
                எனில் (!bdtypep->isDpiPrimitive()) {
                    எனில் (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    எனில் (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } இல்லையெனில் எனில் (nodep->taskPublic()) {
                எனில் (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            எனில் (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(பொய்);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, பொய்);
            rtnvarp = portp;
            rtnvarp->funcLocal(உண்மை);
            rtnvarp->noCReset(உண்மை);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            எனில் (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(பொய்);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = nullptr;
        எனில் (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            எனில் (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                எனில் (rtnvarp) VL_DO_DANGLING(தள்ளு_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        எனில் (rtnvarp) {
            rtnvscp = புதிய AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        எனில் (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } இல்லையெனில் எனில் (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } இல்லையெனில் எனில் (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        எனில் (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        மாறிலி string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* மாறிலி cfuncp = புதிய AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->taskPublic() && rtnvarp) ? rtnvarp->cPubArgType(உண்மை, உண்மை) : "")};
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
        எனில் (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(உண்மை);
            cfuncp->isLoose(உண்மை);
        } இல்லையெனில் {
            cfuncp->isStatic(பொய்);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        எனில் (nodep->name() == "new") cfuncp->isConstructor(உண்மை);
        எனில் (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(உண்மை);
        எனில் (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        எனில் (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        மாறிலி தருக்க needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        எனில் (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        எனில் (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            எனில் (cfuncp->isConstructor()) {
                தருக்க isInterfaceClass = பொய்;
                எனில் (மாறிலி AstClass* மாறிலி classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                எனில் (!isInterfaceClass) {
                    மாறிலி string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        புதிய AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        எனில் (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_உள்ளீடு_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_உள்ளீடு_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_உள்ளீடு_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        எனில் (nodep->dpiExport()) {
            AstScopeName* மாறிலி snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(உண்மை);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        க்கு (Ast_முனை *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                எனில் (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    தள்ளு_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } இல்லையெனில் {
                    எனில் (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, பொய்);
                        portp->funcLocal(உண்மை);
                        எனில் (portp->valuep()) தள்ளு_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        எனில் (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_முனை_DType* மாறிலி dtypep = portp->dtypep()->skipRefp();
                            எனில் (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } இல்லையெனில் {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(உண்மை);
                    }
                    AstVarScope* மாறிலி newvscp
                        = புதிய AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        எனில் (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_முனை* bodysp = nodep->stmtsp();
        எனில் (bodysp) {
            unlinkAndClone(nodep, bodysp, உண்மை);
            AstBegin* மாறிலி tempp
                = புதிய AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, பொய்};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            க்கு (Ast_முனை *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                    எனில் (portp->isIO()) VL_DO_DANGLING(தள்ளு_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            எனில் (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(tempp->delete_மரம்(), tempp);
        }
        எனில் (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        எனில் (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(புதிய AstCReturn{
                rtnvscp->fileline(), புதிய AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        எனில் (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            தருக்க writesNonLocals = பொய்;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* மாறிலி varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                எனில் (varp->isFuncLocal()) திரும்பு;
                // Check if written
                எனில் (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = உண்மை;
                }
                // Check if read
                எனில் (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            எனில் (writesNonLocals) {
                AstVarScope* மாறிலி dpiExportTriggerp = getDpiExporTrigger();
                File_வரி* மாறிலி flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* மாறிலி assignp
                    = புதிய AstAssign{flp, புதிய AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    புதிய AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                எனில் (v3Global.opt.systemC()) {
                    assignp->addNextHere(புதிய AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        எனில் (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        திரும்பு cfuncp;
    }

    வெறுமை iterateIntoFTask(Ast_முனை_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    வெறுமை செருகு_Before_Stmt(Ast_முனை* nodep, Ast_முனை* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->addHereThisAsNext(newp);
    }
    வெறுமை processArgs(Ast_முனை_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        க்கு (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* மாறிலி arrayp = VN_CAST(argp->exprp(), ConsArray);
            எனில் (!arrayp) தொடர்;

            File_வரி* மாறிலி flp = arrayp->fileline();
            AstVarScope* மாறிலி substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(உண்மை);

            AstAssign* மாறிலி assignp
                = புதிய AstAssign{flp, புதிய AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_கோவை_Stmt* மாறிலி exprstmtp = புதிய Ast_கோவை_Stmt{
                flp, assignp, புதிய AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    தருக்க isIfaceLocalImpure(Ast_முனை_FTask* nodep, Ast_முனை* impurep) {
        மாறிலி AstScope* மாறிலி scopep = m_statep->getScope(nodep);
        எனில் (!TaskStateVisitor::isIfaceFTaskScope(scopep)) திரும்பு பொய்;

        மாறிலி AstVarRef* மாறிலி refp = VN_CAST(impurep, VarRef);
        எனில் (!refp || !refp->varScopep()) திரும்பு பொய்;

        திரும்பு refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    வெறுமை visit(AstWith* nodep) override {
        எனில் (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            திரும்பு;
        }
        Ast_முனை_கோவை* மாறிலி withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), முனை_கோவை);
        nodep->addExprp(புதிய AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    வெறுமை visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    வெறுமை visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        எனில் (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            திரும்பு;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        எனில் (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_மொத்தம்(), nodep, "No non-inline task, but no error issued");
            எனில் (VN_IS(nodep->backp(), Stmt_கோவை)) {
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            } இல்லையெனில் {
                nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
            திரும்பு;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        எனில் (nodep->taskp()->isFunction()) {
            AstVar* மாறிலி fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            எனில் (AstAssign* மாறிலி assignp = VN_CAST(nodep->backp(), Assign)) {
                எனில் (AstVarRef* மாறிலி lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* மாறிலி vscp = lhsp->varScopep();
                    எனில் (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_மரம்(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            எனில் (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_முனை* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        மாறிலி தருக்க virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        எனில் (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } இல்லையெனில் {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        எனில் (VN_IS(nodep, New)) {  // New not legal as while() condition
            செருகு_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (VN_IS(nodep->backp(), முனை_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            செருகு_Before_Stmt(nodep, beginp);
            AstVarRef* மாறிலி outrefp = புதிய AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (!VN_IS(nodep->backp(), Stmt_கோவை)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* மாறிலி outrefp = புதிய AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_கோவை_Stmt* lambdap = புதிய Ast_கோவை_Stmt{nodep->fileline(), beginp, outrefp};

            எனில் (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_மரம்(பொய்);
                newvarp->funcLocal(உண்மை);

                // Create a new VarScope for the new variable
                AstVarScope* மாறிலி newvscp
                    = புதிய AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreachAndNext([outvscp, newvscp](AstVarRef* refp) {
                    எனில் (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->hasResult(பொய்);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* மாறிலி exprp = புதிய AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(புதிய AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            VIsCached::clear_Cache_மரம்();
        } இல்லையெனில் {  // VN_IS(nodep->backp(), StmtExpr)
            செருகு_Before_Stmt(nodep, beginp);
            எனில் (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    வெறுமை visit(Ast_முனை_FTask* மாறிலி nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        எனில் (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            முழு modes = 0;
            எனில் (nodep->dpiImport()) modes++;
            எனில் (nodep->dpiExport()) modes++;
            எனில் (nodep->taskPublic()) modes++;
            எனில் (nodep->classMethod()) modes++;
            எனில் (v3Global.opt.protectIds() && nodep->taskPublic()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            எனில் (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            மாறிலி தருக்க noInline = m_statep->ftaskNoInline(nodep);
            மாறிலி தருக்க needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            எனில் (noInline && !nodep->classMethod() && !nodep->recursive()) {
                எனில் (Ast_முனை* மாறிலி impurep = m_statep->checkImpure(nodep)) {
                    எனில் (!isIfaceLocalImpure(nodep, impurep)) {
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

            எனில் (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_முனை_FTask* மாறிலி clonedFuncp = nodep->clone_மரம்(பொய்);
                எனில் (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                மாறிலி தருக்க makeNonInlineFunc = noInline || needsNonInlineCFunc;
                எனில் (AstCFunc* மாறிலி cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->addNextHere(cfuncp);
                    எனில் (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            எனில் (nodep->isFunction()) {
                எனில் (AstVar* மாறிலி portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* மாறிலி vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(தள்ளு_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            க்கு (Ast_முனை *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* மாறிலி vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(தள்ளு_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_முனை_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_முனை_FTask_Ref* refp) { refs.தள்ளு_back(refp); });
        க்கு (Ast_முனை_FTask_Ref* மாறிலி refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* மாறிலி interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* மாறிலி assignp = புதிய AstAssign{
                nodep->fileline(), புதிய AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_மரம்_Pure(பொய்)};
            Ast_Sen_மரம்* மாறிலி senTreep = புதிய Ast_Sen_மரம்{
                nodep->fileline(), புதிய AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* மாறிலி activep
                = புதிய AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_மரம்_Storep(activep->sentreep());
            activep->addStmtsp(
                புதிய AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(புதிய AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_மரம்(), refp);
        }
    }
    வெறுமை visit(Ast_முனை_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        எனில் (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = உண்மை;
        iterateChildren(nodep);
    }
    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
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

வகுப்பு TaskIfaceSensVisitor final : பொது VNVisitorConst {
    // STATE
    தருக்க m_underIfaceFunc = பொய்;  // Under a CFunc owned by an interface scope
    தருக்க m_underSenses = பொய்;  // Under a sensitivity expression of such a CFunc

    // METHODS
    வெறுமை markSensesAndIterate(Ast_முனை* nodep) {
        எனில் (!m_underIfaceFunc) திரும்பு;
        VL_RESTORER(m_underSenses);
        m_underSenses = உண்மை;
        iterateAndNextConstNull(nodep);
    }
    // VISITORS
    வெறுமை visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_Sen_மரம்* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    வெறுமை visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (!m_underSenses) திரும்பு;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        எனில் (nodep->varp()->isFuncLocal()) திரும்பு;
        எனில் (AstIface* மாறிலி ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = இயல்பு;
};

//######################################################################
// Task class functions

மாறிலி எழுத்து* மாறிலி V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_முனை_FTask_Ref* nodep, Ast_முனை* taskStmtsp,
                                    V3TaskConnectState* statep, தருக்க makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<மாறிலி std::string, முழு> nameToIndex;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    முழு tpinnum = 0;
    AstVar* sformatp = nullptr;
    க்கு (Ast_முனை* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
            எனில் (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                nameToIndex.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                எனில் (portp->attrSFormat()) {
                    sformatp = portp;
                } இல்லையெனில் எனில் (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    முழு ppinnum = 0;
    தருக்க reorganize = பொய்;
    க்கு (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        எனில் (!argp->name().empty()) {
            // By name
            மாறிலி auto it = nameToIndex.find(argp->name());
            எனில் (it == nameToIndex.end()) {
                எனில் (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_மரம்(), argp);
                }
            } இல்லையெனில் {
                எனில் (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_மரம்();
                }
                tconnects[it->second].second = argp;
                reorganize = உண்மை;
            }
        } இல்லையெனில் {  // By pin number
            எனில் (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } இல்லையெனில் எனில் (ppinnum >= tpinnum) {
                எனில் (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } இல்லையெனில் எனில் (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_மரம்(), argp);
                }
            } இல்லையெனில் {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    எனில் (!makeChanges) திரும்பு tconnects;

    // Connect missing ones
    VInsertionSet<மாறிலி AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    க்கு (முழு i = 0; i < tpinnum; ++i) {
        AstVar* மாறிலி portp = tconnects[i].first;
        எனில் (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_முனை_கோவை* newvaluep = nullptr;
            எனில் (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = புதிய AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } இல்லையெனில் எனில் (AstFuncRef* மாறிலி funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                மாறிலி Ast_முனை_FTask* மாறிலி funcp = funcRefp->taskp();
                எனில் (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } இல்லையெனில் எனில் (AstConst* மாறிலி constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            எனில் (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), முனை_கோவை));
                எனில் (!VN_IS(newvaluep, Const)) {
                    எனில் (statep) {
                        portp->pin_இலக்கம்(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.செருகு(portp);
                    } இல்லையெனில் {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = புதிய AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_மரம்(உண்மை) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* மாறிலி newp = புதிய AstArg{nodep->fileline(), portp->name(), newvaluep};
            எனில் (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_மரம்(),
                            tconnects[i].second = nullptr);
            }
            tconnects[i].second = newp;
            reorganize = உண்மை;
        }
        எனில் (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } இல்லையெனில் {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    க்கு (மாறிலி auto& tconnect : tconnects) {
        AstArg* மாறிலி argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    எனில் (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        போது (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        க்கு (முழு i = 0; i < tpinnum; ++i) {
            AstArg* மாறிலி argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    எனில் (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_மரம்("-  ftref-out: ");
        க்கு (முழு i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    எனில் (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        திரும்பு taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    திரும்பு tconnects;
}

வெறுமை V3Task::taskConnectWrap(Ast_முனை_FTask_Ref* nodep, மாறிலி V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             மாறிலி VInsertionSet<மாறிலி AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    க்கு (மாறிலி AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_இலக்கம்());
    மாறிலி auto pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    எனில் (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_முனை_FTask* மாறிலி newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    க்கு (மாறிலி auto& tconnect : tconnects) {
        மாறிலி AstVar* மாறிலி portp = tconnect.first;
        AstArg* மாறிலி argp = tconnect.second;
        எனில் (argWrap.exists(portp)) {  // Removed arg
            statep->தள்ளு_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_முனை_FTask* V3Task::taskConnectWrapNew(Ast_முனை_FTask* taskp, மாறிலி string& newname,
                                         மாறிலி V3TaskConnects& tconnects,
                                         மாறிலி VInsertionSet<மாறிலி AstVar*>& argWrap) {
    std::map<மாறிலி AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_முனை_FTask* மாறிலி newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->addNextHere(newTaskp);

    Ast_முனை_FTask_Ref* newCallp = nullptr;
    Ast_முனை* newCallInsertp = nullptr;
    எனில் (VN_IS(taskp, Func)) {
        AstVar* மாறிலி fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* மாறிலி newFVarp = fvarp->clone_மரம்(உண்மை);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = புதிய AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = புதிய AstAssign{taskp->fileline(),
                            புதிய AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } இல்லையெனில் எனில் (VN_IS(taskp, Task)) {
        newCallp = புதிய AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = புதிய Ast_Stmt_கோவை{taskp->fileline(), newCallp};
    } இல்லையெனில் {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    க்கு (மாறிலி auto& tconnect : tconnects) {
        AstVar* மாறிலி portp = tconnect.first;
        AstVar* newPortp;
        எனில் (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = புதிய AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(உண்மை);
            எனில் (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_மரம்();
            newTaskp->addStmtsp(newPortp);
        } இல்லையெனில் {  // Defaulting arg
            Ast_முனை_கோவை* மாறிலி valuep = VN_AS(portp->valuep(), முனை_கோவை);
            எனில் ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                மாறிலி VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* மாறிலி refp = VN_AS(valuep->clone_மரம்(பொய்), VarRef);
                refp->access(refAccess);
                AstArg* மாறிலி newArgp = புதிய AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                தொடர்;
            }
            // Create local temporary
            newPortp = புதிய AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(உண்மை);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            எனில் (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* மாறிலி newAssignp
                    = புதிய AstAssign{valuep->fileline(),
                                    புதிய AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_மரம்(உண்மை)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* மாறிலி newArgp = புதிய AstArg{
            portp->fileline(), portp->name(),
            புதிய AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        மாறிலி auto it = oldNewVars.find(refp->varp());
        எனில் (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    திரும்பு newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, தருக்க isPtr, மாறிலி string& frSuffix,
                                   மாறிலி string& toSuffix, மாறிலி string& frPrefix) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    மாறிலி string frName = frPrefix + portp->name() + frSuffix;
    மாறிலி string toName = portp->name() + toSuffix;
    மாறிலி string idx = portp->name() + "__Vidx";
    size_t unpackSize = 1;  // non-unpacked array is treated as size 1
    முழு unpackDim = 0;
    எனில் (AstUnpackArrayDType* மாறிலி unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpackSize = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(பொய்).second;
        எனில் (unpackDim > 0) UASSERT_OBJ(unpackSize > 0, portp, "size must be greater than 0");
    }
    எனில் (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        மாறிலி தருக்க isBit = portp->basicp()->isDpiBitVec();
        மாறிலி தருக்க needsFor = unpackSize > 1;
        எனில் (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpackSize) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        எனில் (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_சொற்கள்()) + " * " + idx;
        }
        stmt += ", ";
        எனில் (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            க்கு (முழு i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } இல்லையெனில் {
            stmt += frName + ")";
        }
    } இல்லையெனில் {
        மாறிலி தருக்க isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        மாறிலி தருக்க isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        மாறிலி string unpackAt = unpackSize > 1 ? "[" + idx + "]" : "[0]";
        எனில் (unpackDim > 0) {
            மாறிலி string forStmt = unpackSize > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpackSize) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } இல்லையெனில் {
            எனில் (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        எனில் (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        எனில் (unpackDim > 0) {
            stmt += "(&" + frName;
            க்கு (முழு i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } இல்லையெனில் {
            stmt += frName;
        }
        எனில் (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    திரும்பு stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(மாறிலி std::string& lhsName, AstVar* varp) {
    std::string cvt;
    தருக்க useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    மாறிலி std::vector<std::pair<முழு, முழு>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    மாறிலி முழு total = strides.empty() ? 1 : strides[0].first * strides[0].second;
    மாறிலி முழு width_சொற்கள் = varp->basicp()->width_சொற்கள்();
    string statements;
    க்கு (முழு i = 0; i < total; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        க்கு (மாறிலி auto& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        எனில் (useSvVec) {
            மாறிலி std::string offset = std::to_string(i * width_சொற்கள்);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } இல்லையெனில் {
            மாறிலி std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    திரும்பு statements;
}

வெறுமை V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        மாறிலி TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_மரம்("task", 0, dump_மரம்_Either_Level() >= 3);
}
