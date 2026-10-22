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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Task.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3ConstPool.h"
#অন্তর্ভাব "V3Control.h"
#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3LanguageWords.h"
#অন্তর্ভাব "V3SharedTmps.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

বর্গ TaskBaseVertex VL_NOT_FINAL : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_গ্রন্থি* m_impurep = nullptr;  // Node causing impure function w/ outside references
    তার্কিক m_noInline = অসত্য;  // Marked with pragma
সার্বজনিক:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = স্বভাব;
    তার্কিক pure() ধ্রুব { প্রত্যাগম m_impurep == nullptr; }
    Ast_গ্রন্থি* impure_গ্রন্থি() ধ্রুব { প্রত্যাগম m_impurep; }
    শূন্য impure(Ast_গ্রন্থি* nodep) { m_impurep = nodep; }
    তার্কিক noInline() ধ্রুব { প্রত্যাগম m_noInline; }
    শূন্য noInline(তার্কিক flag) { m_noInline = flag; }
};

বর্গ TaskFTaskVertex final : সার্বজনিক TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_গ্রন্থি_FTask* ধ্রুব m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    তার্কিক m_needsNonInlineCFunc = অসত্য;  // Needs CFunc for selected non-inlined call sites

সার্বজনিক:
    TaskFTaskVertex(V3Graph* graphp, Ast_গ্রন্থি_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = স্বভাব;
    Ast_গ্রন্থি_FTask* nodep() ধ্রুব VL_MT_STABLE { প্রত্যাগম m_nodep; }
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম nodep()->name(); }
    string dotColor() ধ্রুব override { প্রত্যাগম pure() ? "black" : "red"; }
    AstCFunc* cFuncp() ধ্রুব { প্রত্যাগম m_cFuncp; }
    শূন্য cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    তার্কিক needsNonInlineCFunc() ধ্রুব { প্রত্যাগম m_needsNonInlineCFunc; }
    শূন্য needsNonInlineCFunc(তার্কিক flag) { m_needsNonInlineCFunc = flag; }
};

বর্গ TaskCodeVertex final : সার্বজনিক TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
সার্বজনিক:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = স্বভাব;
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম "*CODE*"; }
    string dotColor() ধ্রুব override { প্রত্যাগম "green"; }
};

বর্গ TaskEdge final : সার্বজনিক V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
সার্বজনিক:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* শীর্ষ)
        : V3GraphEdge{graphp, fromp, শীর্ষ, 1, অসত্য} {}
    ~TaskEdge() override = স্বভাব;
    string dotLabel() ধ্রুব override { প্রত্যাগম "w" + cvtToStr(weight()); }
};

//######################################################################

বর্গ TaskStateVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ধ্রুব VNUser3InUse m_inuser3;
    ধ্রুব VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<ধ্রুব Ast_গ্রন্থি_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_গ্রন্থি_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    তার্কিক m_underPortVar = অসত্য;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

সার্বজনিক:
    // METHODS
    স্থির তার্কিক isVirtualIfaceMethodCall(ধ্রুব Ast_গ্রন্থি_FTask_Ref* refp) {
        ধ্রুব AstMethodCall* ধ্রুব mrefp = VN_CAST(refp, MethodCall);
        যদি (!mrefp) প্রত্যাগম অসত্য;
        ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব fromp = mrefp->fromp();
        যদি (!fromp || !fromp->dtypep()) প্রত্যাগম অসত্য;
        ধ্রুব AstIfaceRefDType* ধ্রুব ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        প্রত্যাগম ifrefp && ifrefp->isVirtual();
    }
    স্থির তার্কিক isIfaceFTaskScope(ধ্রুব AstScope* scopep) {
        প্রত্যাগম scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_গ্রন্থি_FTask* nodep) {
        AstScope* ধ্রুব scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        প্রত্যাগম scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        ধ্রুব auto iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        প্রত্যাগম iter->second;
    }
    AstClass* getClassp(Ast_গ্রন্থি_FTask* nodep) {
        AstClass* ধ্রুব classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        প্রত্যাগম classp;
    }
    শূন্য remapFuncClassp(Ast_গ্রন্থি_FTask* nodep, Ast_গ্রন্থি_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    তার্কিক ftaskNoInline(Ast_গ্রন্থি_FTask* nodep) { প্রত্যাগম getFTaskVertex(nodep)->noInline(); }
    তার্কিক ftaskNeedsNonInlineCFunc(Ast_গ্রন্থি_FTask* nodep) {
        প্রত্যাগম getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_গ্রন্থি_FTask* nodep) { প্রত্যাগম getFTaskVertex(nodep)->cFuncp(); }
    শূন্য ftaskCFuncp(Ast_গ্রন্থি_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_গ্রন্থি* checkImpure(Ast_গ্রন্থি_FTask* nodep) { প্রত্যাগম getFTaskVertex(nodep)->impure_গ্রন্থি(); }

গুপ্ত:
    TaskFTaskVertex* getFTaskVertex(Ast_গ্রন্থি_FTask* nodep) {
        যদি (!nodep->user4p()) nodep->user4p(নূতন TaskFTaskVertex{&m_callGraph, nodep});
        প্রত্যাগম static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    শূন্য propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        প্রতি (V3GraphVertex& vtx : m_callGraph.vertices()) {
            যদি (TaskFTaskVertex* ধ্রুব ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                যদি (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        যাবত্ (!workList.empty()) {
            TaskFTaskVertex* ধ্রুব calleep = workList.back();
            workList.বের_করো_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            প্রতি (V3GraphEdge& edge : calleep->inEdges()) {
                যদি (TaskFTaskVertex* ধ্রুব callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    যদি (!callerp->pure()) অনুবর্ত;  // Already processed or on workList
                    callerp->impure(calleep->impure_গ্রন্থি());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    শূন্য decideInlining() {
        প্রতি (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* ধ্রুব ftVtxp = vtx.cast<TaskFTaskVertex>();
            যদি (!ftVtxp) অনুবর্ত;
            // Move on if already decided not to inline
            যদি (ftVtxp->noInline()) অনুবর্ত;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            যদি (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(সত্য);
                অনুবর্ত;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            যদি (v3Global.opt.fInlineFuncsEager()) অনুবর্ত;

            Ast_গ্রন্থি_FTask* ধ্রুব ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            যদি (!ftaskp->verilogFunction()) অনুবর্ত;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            যদি (ftaskp->taskPublic()) অনুবর্ত;
            // If the function is pure, it's OK not to inline, so don't do it
            যদি (ftVtxp->pure()) ftVtxp->noInline(সত্য);
        }
    }

    // VISITORS
    শূন্য visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        প্রতি (Ast_গ্রন্থি* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVarScope* ধ্রুব vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ধ্রুব varp = vscp->varp();
                যদি (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        প্রতি (Ast_গ্রন্থি* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (Ast_গ্রন্থি_FTask* ধ্রুব taskp = VN_CAST(stmtp, গ্রন্থি_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* ধ্রুব taskVtxp = getFTaskVertex(nodep->taskp());
        নূতন TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        যদি (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(সত্য);
        // Do we have to disable inlining the function?
        ধ্রুব V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        যদি (!taskVtxp->noInline()) {  // Else short-circuit below
            প্রতি (ধ্রুব auto& itr : tconnects) {
                ধ্রুব AstVar* ধ্রুব portp = itr.first;
                ধ্রুব AstArg* ধ্রুব argp = itr.second;
                যদি (ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব pinp = argp->exprp()) {
                    যদি ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(সত্য);
                    }
                }
            }
        }
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        যদি (nodep->dpiImport()) m_curVxp->noInline(সত্য);
        যদি (nodep->classMethod()) m_curVxp->noInline(সত্য);  // Until V3Task supports it
        যদি (nodep->recursive()) m_curVxp->noInline(সত্য);
        যদি (nodep->isConstructor()) {
            m_curVxp->noInline(সত্য);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstPragma* nodep) override {
        যদি (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(সত্য);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        যদি (nodep->isIO()) m_underPortVar = সত্য;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    শূন্য visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        যদি (m_underPortVar) প্রত্যাগম;
        AstVar* ধ্রুব varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        যদি (nodep->access().isReadOnly() && varp->constPoolEntry()) প্রত্যাগম;
        যদি (varp->user4u().toGraphVertex() != m_curVxp) {
            যদি (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    শূন্য visit(AstClass* nodep) override {
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
        Ast_গ্রন্থি* insertp = nullptr;
        প্রতি (AstInitialAutomatic* initialp : m_initialps) {
            যদি (Ast_গ্রন্থি* ধ্রুব movep = initialp->stmtsp()) {
                movep->unlinkFrBackWithNext();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_গ্রন্থি::addNextNull(insertp, movep);
            }
            VL_DO_DANGLING(ঠেলো_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    শূন্য visit(AstInitialAutomatic* nodep) override {
        m_initialps.ঠেলো_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = নূতন TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.removeRedundantEdgesSum(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        যদি (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = স্বভাব;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

সংরচনা TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    স্থির std::vector<std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক>> unpackDimsAndStrides(AstVar* varp) {
        Ast_গ্রন্থি_DType* ধ্রুব dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক>> dimStrides;
        AstUnpackArrayDType* ধ্রুব unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        যদি (!unpackp) প্রত্যাগম dimStrides;

        ধ্রুব std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ধ্রুব size_t nDims = dims.size();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        প্রতি (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        প্রত্যাগম dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    স্থির std::pair<std::string, তার্কিক> dpiToInternalCvtStmt(ধ্রুব Ast_গ্রন্থি_DType* dtypep) {
        ধ্রুব AstBasicDType* ধ্রুব basicp = dtypep->basicp();

        // DPI types using Primitive C types
        যদি (basicp->keyword() == VBasicDTypeKwd::CHANDLE) প্রত্যাগম {"VL_CVT_VP_Q(", অসত্য};
        যদি (basicp->keyword() == VBasicDTypeKwd::STRING) প্রত্যাগম {"VL_CVT_N_CSTR(", অসত্য};
        যদি (basicp->isDpiPrimitive()) প্রত্যাগম {"(", অসত্য};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ধ্রুব std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ধ্রুব বর্ণ sizeChar = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ধ্রুব std::string& size = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        প্রত্যাগম {"VL_SET_"s + sizeChar + "_" + vecType + "(" + size + ", ", সত্য};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

বর্গ TaskVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<ধ্রুব string, std::tuple<Ast_গ্রন্থি_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ধ্রুব m_statep;  // Common state between visitors
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    Ast_শীর্ষ_Scope* ধ্রুব m_শীর্ষ_Scopep = v3Global.rootp()->শীর্ষ_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_গ্রন্থি* m_insStmtp = nullptr;  // Where to insert statement
    তার্কিক m_inSensesp = অসত্য;  // Are we under a senitem?
    তার্কিক m_inNew = অসত্য;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    পূর্ণাঙ্ক m_uncon_Var_অঙ্ক = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, ধ্রুব AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ধ্রুব string& name, AstVar* examplep) {
        AstVar* ধ্রুব newvarp = নূতন AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(সত্য);
        funcp->addVarsp(newvarp);
        AstVarScope* ধ্রুব newvscp = নূতন AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        প্রত্যাগম newvscp;
    }
    AstVarScope* create_প্রবেশ_Var(AstCFunc* funcp, ধ্রুব string& name, VBasicDTypeKwd kwd) {
        AstVar* ধ্রুব newvarp
            = নূতন AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(সত্য);
        newvarp->direction(VDirection::প্রবেশ);
        funcp->addArgsp(newvarp);
        AstVarScope* ধ্রুব newvscp = নূতন AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        প্রত্যাগম newvscp;
    }
    স্থির AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ধ্রুব refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ধ্রুব vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_বৃক্ষ(), refp);
        প্রত্যাগম vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ধ্রুব Ast_গ্রন্থি_FTask_Ref* refp, AstVar* invarp,
                                ধ্রুব std::string& suffix) {
        যদি (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            প্রত্যাগম constPoolTable(invarp);
        } অন্যথা {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* ধ্রুব newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* ধ্রুব newvarp = newvscp->varp();
            newvarp->funcLocal(অসত্য);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(সত্য);
            প্রত্যাগম newvscp;
        }
    }

    // Replace varrefs with new var pointer
    শূন্য relink(Ast_গ্রন্থি* nodep) {
        nodep->foreachAndNext([](AstVarRef* refp) {
            যদি (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ধ্রুব newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_গ্রন্থি_রাশিমালা* pinp, AstVarScope* newvscp, তার্কিক pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_গ্রন্থি_রাশিমালা* inPinp = pinp;
        যদি (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_গ্রন্থি_রাশিমালা* inPinClonep
            = pureCheck ? inPinp->clone_বৃক্ষ_Pure(সত্য) : inPinp->clone_বৃক্ষ(সত্য);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ধ্রুব AstUnpackArrayDType* ধ্রুব arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        যদি (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = নূতন AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* ধ্রুব assp = নূতন AstAssign{
            pinp->fileline(), নূতন AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, সত্য);  // Ok if in <= block
        প্রত্যাগম assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_গ্রন্থি_রাশিমালা* pinp, AstVarScope* newvscp,
                                        তার্কিক pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_গ্রন্থি_রাশিমালা* outPinp = pinp;
        Ast_গ্রন্থি_রাশিমালা* postRhsp = নূতন AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        যদি (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            যদি (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, গ্রন্থি_Uniop)->lhsp();
                যদি (postRhsp->isSigned()) {
                    postRhsp = নূতন AstISToRD{pinp->fileline(), postRhsp};
                } অন্যথা {
                    postRhsp = নূতন AstIToRD{pinp->fileline(), postRhsp};
                }
            } অন্যথা {
                যদি (Ast_গ্রন্থি_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } অন্যথা যদি (Ast_গ্রন্থি_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } অন্যথা যদি (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } অন্যথা {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                যদি (outPinp->width() < portp->width()) {
                    postRhsp = নূতন AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } অন্যথা {  // pin width > port width
                    যদি (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = নূতন AstExtendS{pinp->fileline(), postRhsp};
                    } অন্যথা {
                        postRhsp = নূতন AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        যদি (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, গ্রন্থি_Uniop)->lhsp();
            postRhsp = নূতন AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব outPinClonep
            = pureCheck ? outPinp->clone_বৃক্ষ_Pure(সত্য) : outPinp->clone_বৃক্ষ(সত্য);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ধ্রুব AstUnpackArrayDType* ধ্রুব arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        যদি (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = নূতন AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* ধ্রুব assp = নূতন AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, সত্য);  // Ok if in <= block
        প্রত্যাগম assp;
    }

    শূন্য changeAtWriteRecurse(Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp) {
        // Change nested at methods to writable variant
        যদি (AstCMethodHard* ধ্রুব cMethodp = VN_CAST(exprp, CMethodHard)) {
            যদি (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    শূন্য connectPort(ধ্রুব Ast_গ্রন্থি_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_গ্রন্থি* beginp,
                     তার্কিক inlineTask) {
        Ast_গ্রন্থি_রাশিমালা* pinp = argp->exprp();
        যদি (inlineTask) {
            portp->unlinkFrBack();
            ঠেলো_Deletep(portp);  // Remove it from the clone (not original)
        }
        যদি (!pinp) {
            // Too few arguments in function call
        } অন্যথা {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            যদি (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* ধ্রুব varp = নূতন AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_অঙ্ক++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* ধ্রুব newvscp = নূতন AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* ধ্রুব repp = নূতন AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(ঠেলো_Deletep(pinp), pinp);
                pinp = repp;
            }
            যদি (inlineTask) {
                ঠেলো_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);  // Args no longer needed
            }
            যদি (portp->isRef() || portp->isConstRef()) {
                তার্কিক refArgOk = অসত্য;
                যদি (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = সত্য;
                } অন্যথা যদি (AstCMethodHard* ধ্রুব cMethodp = VN_CAST(pinp, CMethodHard)) {
                    যদি (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } অন্যথা {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                যদি (refArgOk) {
                    যদি (AstVarRef* ধ্রুব varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } অন্যথা {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                যদি (inlineTask) {
                    যদি (AstVarRef* ধ্রুব varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* ধ্রুব localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } অন্যথা {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } অন্যথা যদি (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* ধ্রুব newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                যদি (!inlineTask) {
                    pinp->replaceWith(
                        নূতন AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    ঠেলো_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* ধ্রুব preassp = connectPortMakeInAssign(pinp, newvscp, সত্য);
                beginp->addNextHere(preassp);

                AstAssign* ধ্রুব postassp = connectPortMakeOutAssign(portp, pinp, newvscp, সত্য);
                beginp->addNext(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } অন্যথা যদি (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ধ্রুব newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                যদি (!inlineTask) {
                    pinp->replaceWith(নূতন AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    ঠেলো_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* ধ্রুব postassp = connectPortMakeOutAssign(portp, pinp, newvscp, অসত্য);
                // Put assignment BEHIND of all other statements
                beginp->addNext(postassp);
            } অন্যথা যদি (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ধ্রুব newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* ধ্রুব preassp = connectPortMakeInAssign(pinp, newvscp, অসত্য);
                // Put assignment in FRONT of all other statements
                beginp->addNextHere(preassp);
            }
        }
    }

    তার্কিক hasRefArgument(Ast_গ্রন্থি_FTask* nodep) {
        প্রতি (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুব varp = VN_CAST(stmtp, Var)) {
                যদি (varp->isRef() || varp->isConstRef()) প্রত্যাগম সত্য;
            }
        }
        প্রত্যাগম অসত্য;
    }

    Ast_গ্রন্থি* createInlinedFTask(Ast_গ্রন্থি_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_গ্রন্থি* ধ্রুব newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_বৃক্ষ(সত্য) : nullptr;
        Ast_গ্রন্থি* ধ্রুব beginp
            = নূতন AstComment{refp->fileline(), "Function: "s + refp->name(), সত্য};
        যদি (newbodysp) beginp->addNext(newbodysp);
        যদি (debug() >= 9) beginp->dump_বৃক্ষ_And_Next(cout, "-  newbegi: ");
        //
        // Create input variables
        Ast_গ্রন্থি::user2_Clear_বৃক্ষ();
        {
            ধ্রুব V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            প্রতি (ধ্রুব auto& itr : tconnects) {
                AstVar* ধ্রুব portp = itr.first;
                AstArg* ধ্রুব argp = itr.second;
                connectPort(refp, portp, argp, beginp, সত্য);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_গ্রন্থি* nextstmtp;
            প্রতি (Ast_গ্রন্থি* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    যদি (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* ধ্রুব localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        যদি (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_গ্রন্থি* ধ্রুব crstp = নূতন AstAssign{
                                portp->fileline(),
                                নূতন AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                নূতন AstCReset{portp->fileline(), portp, অসত্য}};
                            portp->replaceWith(crstp);
                        } অন্যথা {
                            portp->unlinkFrBack();
                        }
                        ঠেলো_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        যদি (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        যদি (debug() >= 9) beginp->dump_বৃক্ষ_And_Next(cout, "-  iotask: ");
        প্রত্যাগম beginp;
    }

    Ast_গ্রন্থি* createNonInlinedFTask(Ast_গ্রন্থি_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ধ্রুব cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_গ্রন্থি* ধ্রুব beginp
            = নূতন AstComment{refp->fileline(), "Function: "s + refp->name(), সত্য};
        Ast_গ্রন্থি_CCall* ccallp;
        যদি (VN_IS(refp, New)) {
            AstCNew* ধ্রুব cnewp = নূতন AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } অন্যথা যদি (ধ্রুব AstMethodCall* ধ্রুব mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = নূতন AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        } অন্যথা {
            ccallp = নূতন AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        }

        যদি (ধ্রুব AstFuncRef* ধ্রুব funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } অন্যথা যদি (ধ্রুব AstTaskRef* ধ্রুব taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } অন্যথা যদি (ধ্রুব AstMethodCall* ধ্রুব methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ধ্রুব V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            প্রতি (ধ্রুব auto& itr : tconnects) {
                AstVar* ধ্রুব portp = itr.first;
                AstArg* ধ্রুব argp = itr.second;
                connectPort(refp, portp, argp, beginp, অসত্য);
            }
        }
        // First argument is symbol table, then output if a function
        ধ্রুব তার্কিক needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        যদি (needSyms) ccallp->argTypes("vlSymsp");

        যদি (refp->taskp()->dpiContext()) {
            AstScopeName* ধ্রুব snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_সারি* ধ্রুব flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                নূতন AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(নূতন AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        প্রতি (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        যদি (outvscp) ccallp->addArgsp(নূতন AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        যদি (debug() >= 9) beginp->dump_বৃক্ষ_And_Next(cout, "-  nitask: ");
        প্রত্যাগম beginp;
    }

    string dpiSignature(Ast_গ্রন্থি_FTask* nodep, AstVar* rtnvarp) ধ্রুব {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        যদি (nodep->dpiPure()) dpiproto += "pure ";
        যদি (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(সত্য, সত্য) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        প্রতি (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (ধ্রুব AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                যদি (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    যদি (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ধ্রুব std::string dpiType = portp->dpiArgType(অসত্য, অসত্য);
                    dpiproto += dpiType;
                    ধ্রুব std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(অসত্য);
                    যদি (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        প্রত্যাগম dpiproto;
    }

    স্থির শূন্য checkLegalCIdentifier(Ast_গ্রন্থি* nodep, ধ্রুব string& name) {
        যদি (name.end() != std::find_if(name.begin(), name.end(), [](বর্ণ c) {
                প্রত্যাগম !std::isalnum(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    স্থির Ast_গ্রন্থি* createDpiTemp(AstVar* portp, ধ্রুব string& suffix) {
        ধ্রুব string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        প্রত্যাগম নূতন AstCStmt{portp->fileline(), stmt};
    }

    শূন্য unlinkAndClone(Ast_গ্রন্থি_FTask* funcp, Ast_গ্রন্থি* nodep, তার্কিক withNext) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        যদি (funcp->recursive()) {
            VNRelinker relinkHandle;
            যদি (withNext) {
                nodep->unlinkFrBackWithNext(&relinkHandle);
            } অন্যথা {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_গ্রন্থি* ধ্রুব newp = nodep->clone_বৃক্ষ(withNext);
            relinkHandle.relink(newp);
        } অন্যথা {
            যদি (withNext) {
                nodep->unlinkFrBackWithNext();
            } অন্যথা {
                nodep->unlinkFrBack();
            }
        }
    }

    স্থির Ast_গ্রন্থি* createAssignInternalToDpi(AstVar* portp, তার্কিক isPtr, ধ্রুব string& frSuffix,
                                              ধ্রুব string& toSuffix) {
        ধ্রুব string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        প্রত্যাগম নূতন AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    স্থির Ast_গ্রন্থি_Stmt* createDpiScalarCvt(Ast_গ্রন্থি_রাশিমালা* internalp, ধ্রুব std::string& dpi,
                                           ধ্রুব Ast_গ্রন্থি_DType* dtypep, তার্কিক toDpi) {
        File_সারি* ধ্রুব flp = internalp->fileline();
        ধ্রুব AstBasicDType* ধ্রুব basicp = dtypep->basicp();
        যদি (toDpi) {
            AstCStmt* ধ্রুব cstmtp = নূতন AstCStmt{flp};
            যদি (basicp->isDpiPrimitive()) {
                cstmtp->add(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->add(internalp);
                cstmtp->add(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } অন্যথা {
                cstmtp->add((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::to_string(dtypep->width()) + ", " + dpi + ", ");
                cstmtp->add(internalp);
                cstmtp->add(");");
            }
            প্রত্যাগম cstmtp;
        }
        std::string cvt;
        তার্কিক useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        যদি (useSvVec) {
            AstCStmt* ধ্রুব cstmtp = নূতন AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            প্রত্যাগম cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ধ্রুব পূর্ণাঙ্ক cwidth = basicp->width_শব্দগুলি() * VL_EDATASIZE;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = নূতন AstCExpr{flp, cvt + dpi + ")", cwidth};
        প্রত্যাগম নূতন AstAssign{flp, internalp, নূতন AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    স্থির Ast_গ্রন্থি_Stmt* createDpiCvt(Ast_গ্রন্থি_রাশিমালা* internalp, ধ্রুব std::string& dpi,
                                     ধ্রুব Ast_গ্রন্থি_DType* dtypep, তার্কিক toDpi) {
        File_সারি* ধ্রুব flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_গ্রন্থি_Stmt* stmtsp = nullptr;
        যদি (ধ্রুব AstUnpackArrayDType* ধ্রুব arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            প্রতি (পূর্ণাঙ্ক i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব elemp = নূতন AstArraySel{flp, internalp->clone_বৃক্ষ(অসত্য), i};
                stmtsp = Ast_গ্রন্থি::addNext(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } অন্যথা যদি (dtypep->containsUnpackedStruct()) {
            প্রতি (ধ্রুব AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ধ্রুব selp
                    = নূতন AstStructSel{flp, internalp->clone_বৃক্ষ(অসত্য), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_গ্রন্থি::addNext(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } অন্যথা {
            প্রত্যাগম createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_বৃক্ষ(), internalp);
        প্রত্যাগম stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    স্থির Ast_গ্রন্থি_Stmt* createDpiArgCvt(AstVarScope* vscp, ধ্রুব std::string& dpiName,
                                        তার্কিক toDpi) {
        AstVar* ধ্রুব varp = vscp->varp();
        File_সারি* ধ্রুব flp = vscp->fileline();
        ধ্রুব std::vector<std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ধ্রুব পূর্ণাঙ্ক total = strides.empty() ? 1 : strides[0].first * strides[0].second;
        ধ্রুব তার্কিক isStruct = varp->dtypep()->containsUnpackedStruct();
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_গ্রন্থি_Stmt* stmtsp = nullptr;
        প্রতি (পূর্ণাঙ্ক i = 0; i < total; ++i) {
            Ast_গ্রন্থি_রাশিমালা* internalp
                = নূতন AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            প্রতি (ধ্রুব auto& stride : strides) {
                internalp = নূতন AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            যদি (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_শব্দগুলি());
            } অন্যথা যদি (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_গ্রন্থি::addNext(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        প্রত্যাগম stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_গ্রন্থি_FTask* ধ্রুব nodep, AstVar* ধ্রুব rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_সারি* ধ্রুব flp = nodep->fileline();
        ধ্রুব std::string cname = Ast_গ্রন্থি::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ধ্রুব বর্ণ* ধ্রুব tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ধ্রুব std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(সত্য, সত্য) : "";
        // The function we are building
        AstCFunc* ধ্রুব funcp = নূতন AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(সত্য);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(সত্য);
        funcp->entryPoint(সত্য);
        funcp->isStatic(সত্য);
        funcp->protect(অসত্য);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_শীর্ষ_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        যদি (!makePortList(nodep, funcp)) প্রত্যাগম nullptr;

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
        AstCStmt* ধ্রুব prep = নূতন AstCStmt{flp};
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
        ধ্রুব std::string cbtype
            = VIdProtect::protect(v3Global.opt.prefix() + "__Vcb_" + nodep->cname() + "_t");
        prep->add(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ধ্রুব callp = নূতন AstCStmt{flp};
        ধ্রুব auto addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ধ্রুব vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(অসত্য);
            portp->protect(অসত্য);
            // Add argument to call
            ধ্রুব VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(নূতন AstVarRef{portp->fileline(), vscp, access});
            প্রত্যাগম vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_গ্রন্থি* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        প্রতি (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ধ্রুব portp = VN_CAST(stmtp, Var);
            যদি (!portp) অনুবর্ত;
            যদি (!portp->isIO()) অনুবর্ত;
            যদি (portp->isFuncReturn()) অনুবর্ত;
            যদি (portp == rtnvarp) অনুবর্ত;  // Handled below
            // Add argument to call
            AstVarScope* ধ্রুব outvscp = addFuncArg(portp);
            ধ্রুব তার্কিক isStruct = portp->dtypep()->containsUnpackedStruct();
            যদি (portp->isWritable()) {
                outCvtsp = Ast_গ্রন্থি::addNext(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), সত্য)
                                       : createAssignInternalToDpi(portp, সত্য, tmpSuffixp, ""));
            }
            যদি (!portp->isNonOutput()) অনুবর্ত;
            // Convert input/inout arguments to dpi type
            ধ্রুব std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(অসত্য).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), অসত্য));
        }
        // Return value argument goes last
        যদি (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        যদি (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        যদি (rtnvarp) {
            funcp->addStmtsp(createDpiTemp(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, অসত্য, tmpSuffixp, ""));
            ধ্রুব std::string index = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(নূতন AstCStmt{flp, "return " + rtnvarp->name() + index + ";"});
        }
        প্রত্যাগম funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_গ্রন্থি_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ধ্রুব string name = Ast_গ্রন্থি::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ধ্রুব string rtnType = rtnvarp            ? rtnvarp->dpiArgType(সত্য, সত্য)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ধ্রুব funcp = নূতন AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(সত্য);
        funcp->dontCombine(সত্য);
        funcp->entryPoint(অসত্য);
        funcp->isMethod(অসত্য);
        funcp->protect(অসত্য);
        funcp->dpiPure(nodep->dpiPure());

        ধ্রুব পূর্ণাঙ্ক cost = static_cast<পূর্ণাঙ্ক>(V3Control::getProfileData(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_শীর্ষ_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        যদি (!makePortList(nodep, funcp)) প্রত্যাগম nullptr;
        প্রত্যাগম funcp;
    }

    AstCFunc* getDpiFunc(Ast_গ্রন্থি_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ধ্রুব string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ধ্রুব auto pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        যদি (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ধ্রুব funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            প্রত্যাগম funcp;
        } অন্যথা {
            // Seen this cname import before. Check if it's the same prototype.
            ধ্রুব Ast_গ্রন্থি_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            যদি (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                প্রত্যাগম nullptr;
            } অন্যথা {
                // Same signature, return the previously created CFunc
                প্রত্যাগম firstFuncp;
            }
        }
    }

    স্থির তার্কিক makePortList(Ast_গ্রন্থি_FTask* nodep, AstCFunc* dpip) {
        তার্কিক allOk = সত্য;
        // Copy nodep's list of function I/O to the new dpip c function
        প্রতি (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                যদি (portp->isIO()) {
                    // Move it to new function
                    AstVar* ধ্রুব newPortp = portp->clone_বৃক্ষ(অসত্য);
                    newPortp->funcLocal(সত্য);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    যদি (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = অসত্য;
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
        প্রত্যাগম allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    স্থির তার্কিক isDpiCIdentifier(ধ্রুব string& name) {
        প্রত্যাগম !name.empty() && VString::isIdentifier(name)
               && !std::isdigit(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_শব্দগুলি::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    তার্কিক checkDpiStruct(ধ্রুব AstVar* portp, ধ্রুব AstStructDType* sdtypep, তার্কিক nested) {
        ধ্রুব std::string name = sdtypep->dpiName();
        যদি (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(সত্য) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            প্রত্যাগম অসত্য;
        }
        প্রতি (ধ্রুব AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            যদি (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                প্রত্যাগম অসত্য;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ধ্রুব Ast_গ্রন্থি_DType* dtypep = itemp->subDTypep()->skipRefp();
            যাবত্ (ধ্রুব AstUnpackArrayDType* ধ্রুব arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ধ্রুব Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব subp = VN_CAST(dtypep, গ্রন্থি_UOr_Struct_DType);
            ধ্রুব AstBasicDType* ধ্রুব basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ধ্রুব তার্কিক isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            যদি (dtypep->containsUnpackedStruct()) {
                যদি (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), সত্য)) প্রত্যাগম অসত্য;
            } অন্যথা যদি (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(সত্য)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                প্রত্যাগম অসত্য;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        যদি (name.empty()) প্রত্যাগম সত্য;
        ধ্রুব AstStructDType* ধ্রুব firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        যদি (firstp->dpiTypedef() == sdtypep->dpiTypedef()) প্রত্যাগম সত্য;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        প্রত্যাগম অসত্য;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    তার্কিক checkDpiStructArgs(ধ্রুব Ast_গ্রন্থি_FTask* nodep) {
        তার্কিক allOk = সত্য;
        প্রতি (ধ্রুব Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ধ্রুব AstVar* ধ্রুব portp = VN_CAST(stmtp, Var);
            যদি (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       অসত্য)
                        && allOk;
            }
        }
        প্রত্যাগম allOk;
    }

    শূন্য bodyDpiImportFunc(Ast_গ্রন্থি_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ধ্রুব বর্ণ* ধ্রুব tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        যদি (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_ঠেলো(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        প্রতি (Ast_গ্রন্থি* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ধ্রুব portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                যদি (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    যদি (args != "") args += ", ";

                    যদি (portp->isDpiOpenArray()) {
                        Ast_গ্রন্থি_DType* ধ্রুব dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ধ্রুব string propName = portp->name() + "__Vopenprops";
                        ধ্রুব string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(নূতন AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ধ্রুব string name = portp->name() + "__Vopenarray";
                        ধ্রুব string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(নূতন AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } অন্যথা {
                        // Unpacked structures are passed by reference from a C array
                        ধ্রুব তার্কিক isStruct = portp->dtypep()->containsUnpackedStruct();
                        যদি (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            যদি (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(createDpiTemp(portp, tmpSuffixp));
                        যদি (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, সত্য)
                                    : createAssignInternalToDpi(portp, অসত্য, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        যদি (nodep->dpiContext()) {
            ধ্রুব string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(নূতন AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ধ্রুব callp = নূতন AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            যদি (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(createDpiTemp(rtnvscp->varp(), tmpSuffixp));
                ধ্রুব std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ধ্রুব cstmtp = নূতন AstCStmt{nodep->fileline()};
                cstmtp->add(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->add(" = ");
                cstmtp->add(callp);  // RHS
                cstmtp->add(";");
                cfuncp->addStmtsp(cstmtp);
            } অন্যথা {
                // Othervise just call it
                cfuncp->addStmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        প্রতি (Ast_গ্রন্থি* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                portp->protect(অসত্য);  // No additional exposure - already part of shown proto
                যদি (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ধ্রুব portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, অসত্য));
                }
            }
        }

        যদি (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_বের_করো(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ধ্রুব netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        যদি (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_শীর্ষ_Scopep->createTemp("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        প্রত্যাগম dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_গ্রন্থি_FTask* nodep, তার্কিক ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_গ্রন্থি::user2_Clear_বৃক্ষ();
        AstVar* rtnvarp = nullptr;
        যদি (nodep->isFunction()) {
            AstVar* ধ্রুব portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            যদি (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ধ্রুব bdtypep = portp->dtypep()->basicp();
                যদি (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(সত্য)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    প্রত্যাগম nullptr;
                }
                যদি (!bdtypep->isDpiPrimitive()) {
                    যদি (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    যদি (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } অন্যথা যদি (nodep->taskPublic()) {
                যদি (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            যদি (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(অসত্য);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, অসত্য);
            rtnvarp = portp;
            rtnvarp->funcLocal(সত্য);
            rtnvarp->noCReset(সত্য);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            যদি (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(অসত্য);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = nullptr;
        যদি (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            যদি (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                যদি (rtnvarp) VL_DO_DANGLING(ঠেলো_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                প্রত্যাগম nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        যদি (rtnvarp) {
            rtnvscp = নূতন AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        যদি (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } অন্যথা যদি (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } অন্যথা যদি (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        যদি (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        ধ্রুব string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* ধ্রুব cfuncp = নূতন AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->taskPublic() && rtnvarp) ? rtnvarp->cPubArgType(সত্য, সত্য) : "")};
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
        যদি (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(সত্য);
            cfuncp->isLoose(সত্য);
        } অন্যথা {
            cfuncp->isStatic(অসত্য);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        যদি (nodep->name() == "new") cfuncp->isConstructor(সত্য);
        যদি (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(সত্য);
        যদি (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        যদি (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ধ্রুব তার্কিক needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        যদি (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        যদি (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            যদি (cfuncp->isConstructor()) {
                তার্কিক isInterfaceClass = অসত্য;
                যদি (ধ্রুব AstClass* ধ্রুব classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                যদি (!isInterfaceClass) {
                    ধ্রুব string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        নূতন AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        যদি (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_প্রবেশ_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_প্রবেশ_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_প্রবেশ_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        যদি (nodep->dpiExport()) {
            AstScopeName* ধ্রুব snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(সত্য);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        প্রতি (Ast_গ্রন্থি *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                যদি (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    ঠেলো_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } অন্যথা {
                    যদি (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, অসত্য);
                        portp->funcLocal(সত্য);
                        যদি (portp->valuep()) ঠেলো_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        যদি (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_গ্রন্থি_DType* ধ্রুব dtypep = portp->dtypep()->skipRefp();
                            যদি (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } অন্যথা {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(সত্য);
                    }
                    AstVarScope* ধ্রুব newvscp
                        = নূতন AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        যদি (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_গ্রন্থি* bodysp = nodep->stmtsp();
        যদি (bodysp) {
            unlinkAndClone(nodep, bodysp, সত্য);
            AstBegin* ধ্রুব tempp
                = নূতন AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, অসত্য};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            প্রতি (Ast_গ্রন্থি *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                    যদি (portp->isIO()) VL_DO_DANGLING(ঠেলো_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            যদি (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(tempp->delete_বৃক্ষ(), tempp);
        }
        যদি (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        যদি (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(নূতন AstCReturn{
                rtnvscp->fileline(), নূতন AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        যদি (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            তার্কিক writesNonLocals = অসত্য;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* ধ্রুব varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                যদি (varp->isFuncLocal()) প্রত্যাগম;
                // Check if written
                যদি (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = সত্য;
                }
                // Check if read
                যদি (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            যদি (writesNonLocals) {
                AstVarScope* ধ্রুব dpiExportTriggerp = getDpiExporTrigger();
                File_সারি* ধ্রুব flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* ধ্রুব assignp
                    = নূতন AstAssign{flp, নূতন AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    নূতন AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                যদি (v3Global.opt.systemC()) {
                    assignp->addNextHere(নূতন AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        যদি (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        প্রত্যাগম cfuncp;
    }

    শূন্য iterateIntoFTask(Ast_গ্রন্থি_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    শূন্য ঢোকাও_Before_Stmt(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->addHereThisAsNext(newp);
    }
    শূন্য processArgs(Ast_গ্রন্থি_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        প্রতি (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ধ্রুব arrayp = VN_CAST(argp->exprp(), ConsArray);
            যদি (!arrayp) অনুবর্ত;

            File_সারি* ধ্রুব flp = arrayp->fileline();
            AstVarScope* ধ্রুব substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(সত্য);

            AstAssign* ধ্রুব assignp
                = নূতন AstAssign{flp, নূতন AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_রাশিমালা_Stmt* ধ্রুব exprstmtp = নূতন Ast_রাশিমালা_Stmt{
                flp, assignp, নূতন AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    তার্কিক isIfaceLocalImpure(Ast_গ্রন্থি_FTask* nodep, Ast_গ্রন্থি* impurep) {
        ধ্রুব AstScope* ধ্রুব scopep = m_statep->getScope(nodep);
        যদি (!TaskStateVisitor::isIfaceFTaskScope(scopep)) প্রত্যাগম অসত্য;

        ধ্রুব AstVarRef* ধ্রুব refp = VN_CAST(impurep, VarRef);
        যদি (!refp || !refp->varScopep()) প্রত্যাগম অসত্য;

        প্রত্যাগম refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    শূন্য visit(AstWith* nodep) override {
        যদি (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            প্রত্যাগম;
        }
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
        nodep->addExprp(নূতন AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    শূন্য visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    শূন্য visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        যদি (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            প্রত্যাগম;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        যদি (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_গুনতি(), nodep, "No non-inline task, but no error issued");
            যদি (VN_IS(nodep->backp(), Stmt_রাশিমালা)) {
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            } অন্যথা {
                nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
            প্রত্যাগম;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        যদি (nodep->taskp()->isFunction()) {
            AstVar* ধ্রুব fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            যদি (AstAssign* ধ্রুব assignp = VN_CAST(nodep->backp(), Assign)) {
                যদি (AstVarRef* ধ্রুব lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ধ্রুব vscp = lhsp->varScopep();
                    যদি (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_বৃক্ষ(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            যদি (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_গ্রন্থি* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ধ্রুব তার্কিক virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        যদি (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } অন্যথা {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        যদি (VN_IS(nodep, New)) {  // New not legal as while() condition
            ঢোকাও_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (VN_IS(nodep->backp(), গ্রন্থি_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            ঢোকাও_Before_Stmt(nodep, beginp);
            AstVarRef* ধ্রুব outrefp = নূতন AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (!VN_IS(nodep->backp(), Stmt_রাশিমালা)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ধ্রুব outrefp = নূতন AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_রাশিমালা_Stmt* lambdap = নূতন Ast_রাশিমালা_Stmt{nodep->fileline(), beginp, outrefp};

            যদি (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_বৃক্ষ(অসত্য);
                newvarp->funcLocal(সত্য);

                // Create a new VarScope for the new variable
                AstVarScope* ধ্রুব newvscp
                    = নূতন AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreachAndNext([outvscp, newvscp](AstVarRef* refp) {
                    যদি (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->hasResult(অসত্য);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ধ্রুব exprp = নূতন AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(নূতন AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            VIsCached::clear_Cache_বৃক্ষ();
        } অন্যথা {  // VN_IS(nodep->backp(), StmtExpr)
            ঢোকাও_Before_Stmt(nodep, beginp);
            যদি (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* ধ্রুব nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        যদি (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            পূর্ণাঙ্ক modes = 0;
            যদি (nodep->dpiImport()) modes++;
            যদি (nodep->dpiExport()) modes++;
            যদি (nodep->taskPublic()) modes++;
            যদি (nodep->classMethod()) modes++;
            যদি (v3Global.opt.protectIds() && nodep->taskPublic()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            যদি (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            ধ্রুব তার্কিক noInline = m_statep->ftaskNoInline(nodep);
            ধ্রুব তার্কিক needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            যদি (noInline && !nodep->classMethod() && !nodep->recursive()) {
                যদি (Ast_গ্রন্থি* ধ্রুব impurep = m_statep->checkImpure(nodep)) {
                    যদি (!isIfaceLocalImpure(nodep, impurep)) {
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

            যদি (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_গ্রন্থি_FTask* ধ্রুব clonedFuncp = nodep->clone_বৃক্ষ(অসত্য);
                যদি (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ধ্রুব তার্কিক makeNonInlineFunc = noInline || needsNonInlineCFunc;
                যদি (AstCFunc* ধ্রুব cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->addNextHere(cfuncp);
                    যদি (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            যদি (nodep->isFunction()) {
                যদি (AstVar* ধ্রুব portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* ধ্রুব vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ঠেলো_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            প্রতি (Ast_গ্রন্থি *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ধ্রুব vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ঠেলো_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_গ্রন্থি_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_গ্রন্থি_FTask_Ref* refp) { refs.ঠেলো_back(refp); });
        প্রতি (Ast_গ্রন্থি_FTask_Ref* ধ্রুব refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ধ্রুব interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* ধ্রুব assignp = নূতন AstAssign{
                nodep->fileline(), নূতন AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_বৃক্ষ_Pure(অসত্য)};
            Ast_Sen_বৃক্ষ* ধ্রুব senTreep = নূতন Ast_Sen_বৃক্ষ{
                nodep->fileline(), নূতন AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* ধ্রুব activep
                = নূতন AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_বৃক্ষ_Storep(activep->sentreep());
            activep->addStmtsp(
                নূতন AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(নূতন AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_বৃক্ষ(), refp);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    শূন্য visit(Ast_গ্রন্থি_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        যদি (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = সত্য;
        iterateChildren(nodep);
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
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

বর্গ TaskIfaceSensVisitor final : সার্বজনিক VNVisitorConst {
    // STATE
    তার্কিক m_underIfaceFunc = অসত্য;  // Under a CFunc owned by an interface scope
    তার্কিক m_underSenses = অসত্য;  // Under a sensitivity expression of such a CFunc

    // METHODS
    শূন্য markSensesAndIterate(Ast_গ্রন্থি* nodep) {
        যদি (!m_underIfaceFunc) প্রত্যাগম;
        VL_RESTORER(m_underSenses);
        m_underSenses = সত্য;
        iterateAndNextConstNull(nodep);
    }
    // VISITORS
    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    শূন্য visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_underSenses) প্রত্যাগম;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        যদি (nodep->varp()->isFuncLocal()) প্রত্যাগম;
        যদি (AstIface* ধ্রুব ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = স্বভাব;
};

//######################################################################
// Task class functions

ধ্রুব বর্ণ* ধ্রুব V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_গ্রন্থি_FTask_Ref* nodep, Ast_গ্রন্থি* taskStmtsp,
                                    V3TaskConnectState* statep, তার্কিক makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ধ্রুব std::string, পূর্ণাঙ্ক> nameToIndex;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    পূর্ণাঙ্ক tpinnum = 0;
    AstVar* sformatp = nullptr;
    প্রতি (Ast_গ্রন্থি* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
            যদি (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                nameToIndex.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                যদি (portp->attrSFormat()) {
                    sformatp = portp;
                } অন্যথা যদি (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    পূর্ণাঙ্ক ppinnum = 0;
    তার্কিক reorganize = অসত্য;
    প্রতি (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        যদি (!argp->name().empty()) {
            // By name
            ধ্রুব auto it = nameToIndex.find(argp->name());
            যদি (it == nameToIndex.end()) {
                যদি (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
                }
            } অন্যথা {
                যদি (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_বৃক্ষ();
                }
                tconnects[it->second].second = argp;
                reorganize = সত্য;
            }
        } অন্যথা {  // By pin number
            যদি (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } অন্যথা যদি (ppinnum >= tpinnum) {
                যদি (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } অন্যথা যদি (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
                }
            } অন্যথা {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    যদি (!makeChanges) প্রত্যাগম tconnects;

    // Connect missing ones
    VInsertionSet<ধ্রুব AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    প্রতি (পূর্ণাঙ্ক i = 0; i < tpinnum; ++i) {
        AstVar* ধ্রুব portp = tconnects[i].first;
        যদি (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_গ্রন্থি_রাশিমালা* newvaluep = nullptr;
            যদি (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = নূতন AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } অন্যথা যদি (AstFuncRef* ধ্রুব funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ধ্রুব Ast_গ্রন্থি_FTask* ধ্রুব funcp = funcRefp->taskp();
                যদি (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } অন্যথা যদি (AstConst* ধ্রুব constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            যদি (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), গ্রন্থি_রাশিমালা));
                যদি (!VN_IS(newvaluep, Const)) {
                    যদি (statep) {
                        portp->pin_অঙ্ক(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.ঢোকাও(portp);
                    } অন্যথা {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = নূতন AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_বৃক্ষ(সত্য) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ধ্রুব newp = নূতন AstArg{nodep->fileline(), portp->name(), newvaluep};
            যদি (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_বৃক্ষ(),
                            tconnects[i].second = nullptr);
            }
            tconnects[i].second = newp;
            reorganize = সত্য;
        }
        যদি (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } অন্যথা {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    প্রতি (ধ্রুব auto& tconnect : tconnects) {
        AstArg* ধ্রুব argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    যদি (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        যাবত্ (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        প্রতি (পূর্ণাঙ্ক i = 0; i < tpinnum; ++i) {
            AstArg* ধ্রুব argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    যদি (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_বৃক্ষ("-  ftref-out: ");
        প্রতি (পূর্ণাঙ্ক i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    যদি (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        প্রত্যাগম taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    প্রত্যাগম tconnects;
}

শূন্য V3Task::taskConnectWrap(Ast_গ্রন্থি_FTask_Ref* nodep, ধ্রুব V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ধ্রুব VInsertionSet<ধ্রুব AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    প্রতি (ধ্রুব AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_অঙ্ক());
    ধ্রুব auto pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    যদি (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_গ্রন্থি_FTask* ধ্রুব newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    প্রতি (ধ্রুব auto& tconnect : tconnects) {
        ধ্রুব AstVar* ধ্রুব portp = tconnect.first;
        AstArg* ধ্রুব argp = tconnect.second;
        যদি (argWrap.exists(portp)) {  // Removed arg
            statep->ঠেলো_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_গ্রন্থি_FTask* V3Task::taskConnectWrapNew(Ast_গ্রন্থি_FTask* taskp, ধ্রুব string& newname,
                                         ধ্রুব V3TaskConnects& tconnects,
                                         ধ্রুব VInsertionSet<ধ্রুব AstVar*>& argWrap) {
    std::map<ধ্রুব AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_গ্রন্থি_FTask* ধ্রুব newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->addNextHere(newTaskp);

    Ast_গ্রন্থি_FTask_Ref* newCallp = nullptr;
    Ast_গ্রন্থি* newCallInsertp = nullptr;
    যদি (VN_IS(taskp, Func)) {
        AstVar* ধ্রুব fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ধ্রুব newFVarp = fvarp->clone_বৃক্ষ(সত্য);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = নূতন AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = নূতন AstAssign{taskp->fileline(),
                            নূতন AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } অন্যথা যদি (VN_IS(taskp, Task)) {
        newCallp = নূতন AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = নূতন Ast_Stmt_রাশিমালা{taskp->fileline(), newCallp};
    } অন্যথা {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    প্রতি (ধ্রুব auto& tconnect : tconnects) {
        AstVar* ধ্রুব portp = tconnect.first;
        AstVar* newPortp;
        যদি (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = নূতন AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(সত্য);
            যদি (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_বৃক্ষ();
            newTaskp->addStmtsp(newPortp);
        } অন্যথা {  // Defaulting arg
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব valuep = VN_AS(portp->valuep(), গ্রন্থি_রাশিমালা);
            যদি ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ধ্রুব VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ধ্রুব refp = VN_AS(valuep->clone_বৃক্ষ(অসত্য), VarRef);
                refp->access(refAccess);
                AstArg* ধ্রুব newArgp = নূতন AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                অনুবর্ত;
            }
            // Create local temporary
            newPortp = নূতন AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(সত্য);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            যদি (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* ধ্রুব newAssignp
                    = নূতন AstAssign{valuep->fileline(),
                                    নূতন AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_বৃক্ষ(সত্য)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ধ্রুব newArgp = নূতন AstArg{
            portp->fileline(), portp->name(),
            নূতন AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ধ্রুব auto it = oldNewVars.find(refp->varp());
        যদি (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    প্রত্যাগম newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, তার্কিক isPtr, ধ্রুব string& frSuffix,
                                   ধ্রুব string& toSuffix, ধ্রুব string& frPrefix) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ধ্রুব string frName = frPrefix + portp->name() + frSuffix;
    ধ্রুব string toName = portp->name() + toSuffix;
    ধ্রুব string idx = portp->name() + "__Vidx";
    size_t unpackSize = 1;  // non-unpacked array is treated as size 1
    পূর্ণাঙ্ক unpackDim = 0;
    যদি (AstUnpackArrayDType* ধ্রুব unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpackSize = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(অসত্য).second;
        যদি (unpackDim > 0) UASSERT_OBJ(unpackSize > 0, portp, "size must be greater than 0");
    }
    যদি (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        ধ্রুব তার্কিক isBit = portp->basicp()->isDpiBitVec();
        ধ্রুব তার্কিক needsFor = unpackSize > 1;
        যদি (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpackSize) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        যদি (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_শব্দগুলি()) + " * " + idx;
        }
        stmt += ", ";
        যদি (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            প্রতি (পূর্ণাঙ্ক i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } অন্যথা {
            stmt += frName + ")";
        }
    } অন্যথা {
        ধ্রুব তার্কিক isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        ধ্রুব তার্কিক isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        ধ্রুব string unpackAt = unpackSize > 1 ? "[" + idx + "]" : "[0]";
        যদি (unpackDim > 0) {
            ধ্রুব string forStmt = unpackSize > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpackSize) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } অন্যথা {
            যদি (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        যদি (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        যদি (unpackDim > 0) {
            stmt += "(&" + frName;
            প্রতি (পূর্ণাঙ্ক i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } অন্যথা {
            stmt += frName;
        }
        যদি (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    প্রত্যাগম stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(ধ্রুব std::string& lhsName, AstVar* varp) {
    std::string cvt;
    তার্কিক useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    ধ্রুব std::vector<std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ধ্রুব পূর্ণাঙ্ক total = strides.empty() ? 1 : strides[0].first * strides[0].second;
    ধ্রুব পূর্ণাঙ্ক width_শব্দগুলি = varp->basicp()->width_শব্দগুলি();
    string statements;
    প্রতি (পূর্ণাঙ্ক i = 0; i < total; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        প্রতি (ধ্রুব auto& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        যদি (useSvVec) {
            ধ্রুব std::string offset = std::to_string(i * width_শব্দগুলি);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } অন্যথা {
            ধ্রুব std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    প্রত্যাগম statements;
}

শূন্য V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ধ্রুব TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_বৃক্ষ("task", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
