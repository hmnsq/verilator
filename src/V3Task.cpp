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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Task.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3LanguageWords.h"
#അന്തര്ഭാവ "V3SharedTmps.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

വർഗ്ഗം TaskBaseVertex VL_NOT_FINAL : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_കണ്ണി* m_impurep = nullptr;  // Node causing impure function w/ outside references
    തർക്കം m_noInline = അസത്യം;  // Marked with pragma
പൊതു:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = സ്ഥിരസ്ഥിതി;
    തർക്കം pure() സ്ഥിരം { തിരികെ m_impurep == nullptr; }
    Ast_കണ്ണി* impure_കണ്ണി() സ്ഥിരം { തിരികെ m_impurep; }
    ശൂന്യം impure(Ast_കണ്ണി* nodep) { m_impurep = nodep; }
    തർക്കം noInline() സ്ഥിരം { തിരികെ m_noInline; }
    ശൂന്യം noInline(തർക്കം flag) { m_noInline = flag; }
};

വർഗ്ഗം TaskFTaskVertex final : പൊതു TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_കണ്ണി_FTask* സ്ഥിരം m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    തർക്കം m_needsNonInlineCFunc = അസത്യം;  // Needs CFunc for selected non-inlined call sites

പൊതു:
    TaskFTaskVertex(V3Graph* graphp, Ast_കണ്ണി_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = സ്ഥിരസ്ഥിതി;
    Ast_കണ്ണി_FTask* nodep() സ്ഥിരം VL_MT_STABLE { തിരികെ m_nodep; }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ nodep()->name(); }
    string dotColor() സ്ഥിരം override { തിരികെ pure() ? "black" : "red"; }
    AstCFunc* cFuncp() സ്ഥിരം { തിരികെ m_cFuncp; }
    ശൂന്യം cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    തർക്കം needsNonInlineCFunc() സ്ഥിരം { തിരികെ m_needsNonInlineCFunc; }
    ശൂന്യം needsNonInlineCFunc(തർക്കം flag) { m_needsNonInlineCFunc = flag; }
};

വർഗ്ഗം TaskCodeVertex final : പൊതു TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
പൊതു:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = സ്ഥിരസ്ഥിതി;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ "*CODE*"; }
    string dotColor() സ്ഥിരം override { തിരികെ "green"; }
};

വർഗ്ഗം TaskEdge final : പൊതു V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
പൊതു:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* മുകൾ)
        : V3GraphEdge{graphp, fromp, മുകൾ, 1, അസത്യം} {}
    ~TaskEdge() override = സ്ഥിരസ്ഥിതി;
    string dotLabel() സ്ഥിരം override { തിരികെ "w" + cvtToStr(weight()); }
};

//######################################################################

വർഗ്ഗം TaskStateVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    സ്ഥിരം VNUser3InUse m_inuser3;
    സ്ഥിരം VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<സ്ഥിരം Ast_കണ്ണി_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_കണ്ണി_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    തർക്കം m_underPortVar = അസത്യം;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

പൊതു:
    // METHODS
    സ്ഥിതം തർക്കം isVirtualIfaceMethodCall(സ്ഥിരം Ast_കണ്ണി_FTask_Ref* refp) {
        സ്ഥിരം AstMethodCall* സ്ഥിരം mrefp = VN_CAST(refp, MethodCall);
        എങ്കിൽ (!mrefp) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = mrefp->fromp();
        എങ്കിൽ (!fromp || !fromp->dtypep()) തിരികെ അസത്യം;
        സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        തിരികെ ifrefp && ifrefp->isVirtual();
    }
    സ്ഥിതം തർക്കം isIfaceFTaskScope(സ്ഥിരം AstScope* scopep) {
        തിരികെ scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_കണ്ണി_FTask* nodep) {
        AstScope* സ്ഥിരം scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        തിരികെ scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        സ്ഥിരം auto iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        തിരികെ iter->second;
    }
    AstClass* getClassp(Ast_കണ്ണി_FTask* nodep) {
        AstClass* സ്ഥിരം classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        തിരികെ classp;
    }
    ശൂന്യം remapFuncClassp(Ast_കണ്ണി_FTask* nodep, Ast_കണ്ണി_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    തർക്കം ftaskNoInline(Ast_കണ്ണി_FTask* nodep) { തിരികെ getFTaskVertex(nodep)->noInline(); }
    തർക്കം ftaskNeedsNonInlineCFunc(Ast_കണ്ണി_FTask* nodep) {
        തിരികെ getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_കണ്ണി_FTask* nodep) { തിരികെ getFTaskVertex(nodep)->cFuncp(); }
    ശൂന്യം ftaskCFuncp(Ast_കണ്ണി_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_കണ്ണി* checkImpure(Ast_കണ്ണി_FTask* nodep) { തിരികെ getFTaskVertex(nodep)->impure_കണ്ണി(); }

സ്വകാര്യ:
    TaskFTaskVertex* getFTaskVertex(Ast_കണ്ണി_FTask* nodep) {
        എങ്കിൽ (!nodep->user4p()) nodep->user4p(പുതിയ TaskFTaskVertex{&m_callGraph, nodep});
        തിരികെ static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    ശൂന്യം propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        വേണ്ടി (V3GraphVertex& vtx : m_callGraph.vertices()) {
            എങ്കിൽ (TaskFTaskVertex* സ്ഥിരം ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                എങ്കിൽ (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        ആകുമ്പോൾ (!workList.empty()) {
            TaskFTaskVertex* സ്ഥിരം calleep = workList.back();
            workList.എടുക്കുക_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            വേണ്ടി (V3GraphEdge& edge : calleep->inEdges()) {
                എങ്കിൽ (TaskFTaskVertex* സ്ഥിരം callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    എങ്കിൽ (!callerp->pure()) തുടരുക;  // Already processed or on workList
                    callerp->impure(calleep->impure_കണ്ണി());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    ശൂന്യം decideInlining() {
        വേണ്ടി (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* സ്ഥിരം ftVtxp = vtx.cast<TaskFTaskVertex>();
            എങ്കിൽ (!ftVtxp) തുടരുക;
            // Move on if already decided not to inline
            എങ്കിൽ (ftVtxp->noInline()) തുടരുക;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            എങ്കിൽ (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(സത്യം);
                തുടരുക;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            എങ്കിൽ (v3Global.opt.fInlineFuncsEager()) തുടരുക;

            Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            എങ്കിൽ (!ftaskp->verilogFunction()) തുടരുക;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            എങ്കിൽ (ftaskp->taskPublic()) തുടരുക;
            // If the function is pure, it's OK not to inline, so don't do it
            എങ്കിൽ (ftVtxp->pure()) ftVtxp->noInline(സത്യം);
        }
    }

    // VISITORS
    ശൂന്യം visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVarScope* സ്ഥിരം vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* സ്ഥിരം varp = vscp->varp();
                എങ്കിൽ (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം taskp = VN_CAST(stmtp, കണ്ണി_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* സ്ഥിരം taskVtxp = getFTaskVertex(nodep->taskp());
        പുതിയ TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        എങ്കിൽ (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(സത്യം);
        // Do we have to disable inlining the function?
        സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        എങ്കിൽ (!taskVtxp->noInline()) {  // Else short-circuit below
            വേണ്ടി (സ്ഥിരം auto& itr : tconnects) {
                സ്ഥിരം AstVar* സ്ഥിരം portp = itr.first;
                സ്ഥിരം AstArg* സ്ഥിരം argp = itr.second;
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം pinp = argp->exprp()) {
                    എങ്കിൽ ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(സത്യം);
                    }
                }
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        എങ്കിൽ (nodep->dpiImport()) m_curVxp->noInline(സത്യം);
        എങ്കിൽ (nodep->classMethod()) m_curVxp->noInline(സത്യം);  // Until V3Task supports it
        എങ്കിൽ (nodep->recursive()) m_curVxp->noInline(സത്യം);
        എങ്കിൽ (nodep->isConstructor()) {
            m_curVxp->noInline(സത്യം);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstPragma* nodep) override {
        എങ്കിൽ (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        എങ്കിൽ (nodep->isIO()) m_underPortVar = സത്യം;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (m_underPortVar) തിരികെ;
        AstVar* സ്ഥിരം varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        എങ്കിൽ (nodep->access().isReadOnly() && varp->constPoolEntry()) തിരികെ;
        എങ്കിൽ (varp->user4u().toGraphVertex() != m_curVxp) {
            എങ്കിൽ (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    ശൂന്യം visit(AstClass* nodep) override {
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
        Ast_കണ്ണി* insertp = nullptr;
        വേണ്ടി (AstInitialAutomatic* initialp : m_initialps) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം movep = initialp->stmtsp()) {
                movep->unlinkFrBackWithNext();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_കണ്ണി::addNextNull(insertp, movep);
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    ശൂന്യം visit(AstInitialAutomatic* nodep) override {
        m_initialps.തള്ളുക_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = പുതിയ TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.removeRedundantEdgesSum(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        എങ്കിൽ (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

ഘടന TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    സ്ഥിതം std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> unpackDimsAndStrides(AstVar* varp) {
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> dimStrides;
        AstUnpackArrayDType* സ്ഥിരം unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        എങ്കിൽ (!unpackp) തിരികെ dimStrides;

        സ്ഥിരം std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        സ്ഥിരം size_t nDims = dims.size();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        വേണ്ടി (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        തിരികെ dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    സ്ഥിതം std::pair<std::string, തർക്കം> dpiToInternalCvtStmt(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = dtypep->basicp();

        // DPI types using Primitive C types
        എങ്കിൽ (basicp->keyword() == VBasicDTypeKwd::CHANDLE) തിരികെ {"VL_CVT_VP_Q(", അസത്യം};
        എങ്കിൽ (basicp->keyword() == VBasicDTypeKwd::STRING) തിരികെ {"VL_CVT_N_CSTR(", അസത്യം};
        എങ്കിൽ (basicp->isDpiPrimitive()) തിരികെ {"(", അസത്യം};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        സ്ഥിരം std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        സ്ഥിരം അക്ഷരം sizeChar = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        സ്ഥിരം std::string& size = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        തിരികെ {"VL_SET_"s + sizeChar + "_" + vecType + "(" + size + ", ", സത്യം};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

വർഗ്ഗം TaskVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<സ്ഥിരം string, std::tuple<Ast_കണ്ണി_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* സ്ഥിരം m_statep;  // Common state between visitors
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    Ast_മുകൾ_Scope* സ്ഥിരം m_മുകൾ_Scopep = v3Global.rootp()->മുകൾ_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_കണ്ണി* m_insStmtp = nullptr;  // Where to insert statement
    തർക്കം m_inSensesp = അസത്യം;  // Are we under a senitem?
    തർക്കം m_inNew = അസത്യം;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    പൂർണ്ണസംഖ്യ m_uncon_Var_അക്കം = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, സ്ഥിരം AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, സ്ഥിരം string& name, AstVar* examplep) {
        AstVar* സ്ഥിരം newvarp = പുതിയ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(സത്യം);
        funcp->addVarsp(newvarp);
        AstVarScope* സ്ഥിരം newvscp = പുതിയ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        തിരികെ newvscp;
    }
    AstVarScope* create_നിവേശം_Var(AstCFunc* funcp, സ്ഥിരം string& name, VBasicDTypeKwd kwd) {
        AstVar* സ്ഥിരം newvarp
            = പുതിയ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(സത്യം);
        newvarp->direction(VDirection::നിവേശം);
        funcp->addArgsp(newvarp);
        AstVarScope* സ്ഥിരം newvscp = പുതിയ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        തിരികെ newvscp;
    }
    സ്ഥിതം AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* സ്ഥിരം refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* സ്ഥിരം vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_വൃക്ഷം(), refp);
        തിരികെ vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(സ്ഥിരം Ast_കണ്ണി_FTask_Ref* refp, AstVar* invarp,
                                സ്ഥിരം std::string& suffix) {
        എങ്കിൽ (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            തിരികെ constPoolTable(invarp);
        } അല്ലെങ്കിൽ {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* സ്ഥിരം newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* സ്ഥിരം newvarp = newvscp->varp();
            newvarp->funcLocal(അസത്യം);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(സത്യം);
            തിരികെ newvscp;
        }
    }

    // Replace varrefs with new var pointer
    ശൂന്യം relink(Ast_കണ്ണി* nodep) {
        nodep->foreachAndNext([](AstVarRef* refp) {
            എങ്കിൽ (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* സ്ഥിരം newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_കണ്ണി_വ്യഞ്ജകം* pinp, AstVarScope* newvscp, തർക്കം pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_കണ്ണി_വ്യഞ്ജകം* inPinp = pinp;
        എങ്കിൽ (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* inPinClonep
            = pureCheck ? inPinp->clone_വൃക്ഷം_Pure(സത്യം) : inPinp->clone_വൃക്ഷം(സത്യം);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        എങ്കിൽ (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = പുതിയ AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* സ്ഥിരം assp = പുതിയ AstAssign{
            pinp->fileline(), പുതിയ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, സത്യം);  // Ok if in <= block
        തിരികെ assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_കണ്ണി_വ്യഞ്ജകം* pinp, AstVarScope* newvscp,
                                        തർക്കം pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_കണ്ണി_വ്യഞ്ജകം* outPinp = pinp;
        Ast_കണ്ണി_വ്യഞ്ജകം* postRhsp = പുതിയ AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        എങ്കിൽ (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            എങ്കിൽ (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, കണ്ണി_Uniop)->lhsp();
                എങ്കിൽ (postRhsp->isSigned()) {
                    postRhsp = പുതിയ AstISToRD{pinp->fileline(), postRhsp};
                } അല്ലെങ്കിൽ {
                    postRhsp = പുതിയ AstIToRD{pinp->fileline(), postRhsp};
                }
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (Ast_കണ്ണി_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } അല്ലെങ്കിൽ എങ്കിൽ (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } അല്ലെങ്കിൽ {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                എങ്കിൽ (outPinp->width() < portp->width()) {
                    postRhsp = പുതിയ AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } അല്ലെങ്കിൽ {  // pin width > port width
                    എങ്കിൽ (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = പുതിയ AstExtendS{pinp->fileline(), postRhsp};
                    } അല്ലെങ്കിൽ {
                        postRhsp = പുതിയ AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        എങ്കിൽ (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, കണ്ണി_Uniop)->lhsp();
            postRhsp = പുതിയ AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം outPinClonep
            = pureCheck ? outPinp->clone_വൃക്ഷം_Pure(സത്യം) : outPinp->clone_വൃക്ഷം(സത്യം);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        എങ്കിൽ (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = പുതിയ AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* സ്ഥിരം assp = പുതിയ AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, സത്യം);  // Ok if in <= block
        തിരികെ assp;
    }

    ശൂന്യം changeAtWriteRecurse(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp) {
        // Change nested at methods to writable variant
        എങ്കിൽ (AstCMethodHard* സ്ഥിരം cMethodp = VN_CAST(exprp, CMethodHard)) {
            എങ്കിൽ (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    ശൂന്യം connectPort(സ്ഥിരം Ast_കണ്ണി_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_കണ്ണി* beginp,
                     തർക്കം inlineTask) {
        Ast_കണ്ണി_വ്യഞ്ജകം* pinp = argp->exprp();
        എങ്കിൽ (inlineTask) {
            portp->unlinkFrBack();
            തള്ളുക_Deletep(portp);  // Remove it from the clone (not original)
        }
        എങ്കിൽ (!pinp) {
            // Too few arguments in function call
        } അല്ലെങ്കിൽ {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            എങ്കിൽ (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* സ്ഥിരം varp = പുതിയ AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_അക്കം++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* സ്ഥിരം newvscp = പുതിയ AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* സ്ഥിരം repp = പുതിയ AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(തള്ളുക_Deletep(pinp), pinp);
                pinp = repp;
            }
            എങ്കിൽ (inlineTask) {
                തള്ളുക_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_വൃക്ഷം(), argp);  // Args no longer needed
            }
            എങ്കിൽ (portp->isRef() || portp->isConstRef()) {
                തർക്കം refArgOk = അസത്യം;
                എങ്കിൽ (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (AstCMethodHard* സ്ഥിരം cMethodp = VN_CAST(pinp, CMethodHard)) {
                    എങ്കിൽ (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } അല്ലെങ്കിൽ {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                എങ്കിൽ (refArgOk) {
                    എങ്കിൽ (AstVarRef* സ്ഥിരം varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } അല്ലെങ്കിൽ {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                എങ്കിൽ (inlineTask) {
                    എങ്കിൽ (AstVarRef* സ്ഥിരം varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* സ്ഥിരം localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } അല്ലെങ്കിൽ {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* സ്ഥിരം newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                എങ്കിൽ (!inlineTask) {
                    pinp->replaceWith(
                        പുതിയ AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    തള്ളുക_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* സ്ഥിരം preassp = connectPortMakeInAssign(pinp, newvscp, സത്യം);
                beginp->addNextHere(preassp);

                AstAssign* സ്ഥിരം postassp = connectPortMakeOutAssign(portp, pinp, newvscp, സത്യം);
                beginp->addNext(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } അല്ലെങ്കിൽ എങ്കിൽ (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* സ്ഥിരം newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                എങ്കിൽ (!inlineTask) {
                    pinp->replaceWith(പുതിയ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    തള്ളുക_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* സ്ഥിരം postassp = connectPortMakeOutAssign(portp, pinp, newvscp, അസത്യം);
                // Put assignment BEHIND of all other statements
                beginp->addNext(postassp);
            } അല്ലെങ്കിൽ എങ്കിൽ (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* സ്ഥിരം newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* സ്ഥിരം preassp = connectPortMakeInAssign(pinp, newvscp, അസത്യം);
                // Put assignment in FRONT of all other statements
                beginp->addNextHere(preassp);
            }
        }
    }

    തർക്കം hasRefArgument(Ast_കണ്ണി_FTask* nodep) {
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (varp->isRef() || varp->isConstRef()) തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }

    Ast_കണ്ണി* createInlinedFTask(Ast_കണ്ണി_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_കണ്ണി* സ്ഥിരം newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_വൃക്ഷം(സത്യം) : nullptr;
        Ast_കണ്ണി* സ്ഥിരം beginp
            = പുതിയ AstComment{refp->fileline(), "Function: "s + refp->name(), സത്യം};
        എങ്കിൽ (newbodysp) beginp->addNext(newbodysp);
        എങ്കിൽ (debug() >= 9) beginp->dump_വൃക്ഷം_And_Next(cout, "-  newbegi: ");
        //
        // Create input variables
        Ast_കണ്ണി::user2_Clear_വൃക്ഷം();
        {
            സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            വേണ്ടി (സ്ഥിരം auto& itr : tconnects) {
                AstVar* സ്ഥിരം portp = itr.first;
                AstArg* സ്ഥിരം argp = itr.second;
                connectPort(refp, portp, argp, beginp, സത്യം);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_കണ്ണി* nextstmtp;
            വേണ്ടി (Ast_കണ്ണി* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    എങ്കിൽ (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* സ്ഥിരം localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        എങ്കിൽ (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_കണ്ണി* സ്ഥിരം crstp = പുതിയ AstAssign{
                                portp->fileline(),
                                പുതിയ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                പുതിയ AstCReset{portp->fileline(), portp, അസത്യം}};
                            portp->replaceWith(crstp);
                        } അല്ലെങ്കിൽ {
                            portp->unlinkFrBack();
                        }
                        തള്ളുക_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        എങ്കിൽ (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        എങ്കിൽ (debug() >= 9) beginp->dump_വൃക്ഷം_And_Next(cout, "-  iotask: ");
        തിരികെ beginp;
    }

    Ast_കണ്ണി* createNonInlinedFTask(Ast_കണ്ണി_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* സ്ഥിരം cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_കണ്ണി* സ്ഥിരം beginp
            = പുതിയ AstComment{refp->fileline(), "Function: "s + refp->name(), സത്യം};
        Ast_കണ്ണി_CCall* ccallp;
        എങ്കിൽ (VN_IS(refp, New)) {
            AstCNew* സ്ഥിരം cnewp = പുതിയ AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMethodCall* സ്ഥിരം mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = പുതിയ AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        } അല്ലെങ്കിൽ {
            ccallp = പുതിയ AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        }

        എങ്കിൽ (സ്ഥിരം AstFuncRef* സ്ഥിരം funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstTaskRef* സ്ഥിരം taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMethodCall* സ്ഥിരം methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            വേണ്ടി (സ്ഥിരം auto& itr : tconnects) {
                AstVar* സ്ഥിരം portp = itr.first;
                AstArg* സ്ഥിരം argp = itr.second;
                connectPort(refp, portp, argp, beginp, അസത്യം);
            }
        }
        // First argument is symbol table, then output if a function
        സ്ഥിരം തർക്കം needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        എങ്കിൽ (needSyms) ccallp->argTypes("vlSymsp");

        എങ്കിൽ (refp->taskp()->dpiContext()) {
            AstScopeName* സ്ഥിരം snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_വരി* സ്ഥിരം flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                പുതിയ AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(പുതിയ AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        വേണ്ടി (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        എങ്കിൽ (outvscp) ccallp->addArgsp(പുതിയ AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        എങ്കിൽ (debug() >= 9) beginp->dump_വൃക്ഷം_And_Next(cout, "-  nitask: ");
        തിരികെ beginp;
    }

    string dpiSignature(Ast_കണ്ണി_FTask* nodep, AstVar* rtnvarp) സ്ഥിരം {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        എങ്കിൽ (nodep->dpiPure()) dpiproto += "pure ";
        എങ്കിൽ (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(സത്യം, സത്യം) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    എങ്കിൽ (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    സ്ഥിരം std::string dpiType = portp->dpiArgType(അസത്യം, അസത്യം);
                    dpiproto += dpiType;
                    സ്ഥിരം std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(അസത്യം);
                    എങ്കിൽ (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        തിരികെ dpiproto;
    }

    സ്ഥിതം ശൂന്യം checkLegalCIdentifier(Ast_കണ്ണി* nodep, സ്ഥിരം string& name) {
        എങ്കിൽ (name.end() != std::find_if(name.begin(), name.end(), [](അക്ഷരം c) {
                തിരികെ !std::isalnum(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    സ്ഥിതം Ast_കണ്ണി* createDpiTemp(AstVar* portp, സ്ഥിരം string& suffix) {
        സ്ഥിരം string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        തിരികെ പുതിയ AstCStmt{portp->fileline(), stmt};
    }

    ശൂന്യം unlinkAndClone(Ast_കണ്ണി_FTask* funcp, Ast_കണ്ണി* nodep, തർക്കം withNext) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        എങ്കിൽ (funcp->recursive()) {
            VNRelinker relinkHandle;
            എങ്കിൽ (withNext) {
                nodep->unlinkFrBackWithNext(&relinkHandle);
            } അല്ലെങ്കിൽ {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_കണ്ണി* സ്ഥിരം newp = nodep->clone_വൃക്ഷം(withNext);
            relinkHandle.relink(newp);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (withNext) {
                nodep->unlinkFrBackWithNext();
            } അല്ലെങ്കിൽ {
                nodep->unlinkFrBack();
            }
        }
    }

    സ്ഥിതം Ast_കണ്ണി* createAssignInternalToDpi(AstVar* portp, തർക്കം isPtr, സ്ഥിരം string& frSuffix,
                                              സ്ഥിരം string& toSuffix) {
        സ്ഥിരം string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        തിരികെ പുതിയ AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    സ്ഥിതം Ast_കണ്ണി_Stmt* createDpiScalarCvt(Ast_കണ്ണി_വ്യഞ്ജകം* internalp, സ്ഥിരം std::string& dpi,
                                           സ്ഥിരം Ast_കണ്ണി_DType* dtypep, തർക്കം toDpi) {
        File_വരി* സ്ഥിരം flp = internalp->fileline();
        സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = dtypep->basicp();
        എങ്കിൽ (toDpi) {
            AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{flp};
            എങ്കിൽ (basicp->isDpiPrimitive()) {
                cstmtp->add(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->add(internalp);
                cstmtp->add(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } അല്ലെങ്കിൽ {
                cstmtp->add((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::to_string(dtypep->width()) + ", " + dpi + ", ");
                cstmtp->add(internalp);
                cstmtp->add(");");
            }
            തിരികെ cstmtp;
        }
        std::string cvt;
        തർക്കം useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        എങ്കിൽ (useSvVec) {
            AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            തിരികെ cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        സ്ഥിരം പൂർണ്ണസംഖ്യ cwidth = basicp->width_വാക്കുകൾ() * VL_EDATASIZE;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstCExpr{flp, cvt + dpi + ")", cwidth};
        തിരികെ പുതിയ AstAssign{flp, internalp, പുതിയ AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    സ്ഥിതം Ast_കണ്ണി_Stmt* createDpiCvt(Ast_കണ്ണി_വ്യഞ്ജകം* internalp, സ്ഥിരം std::string& dpi,
                                     സ്ഥിരം Ast_കണ്ണി_DType* dtypep, തർക്കം toDpi) {
        File_വരി* സ്ഥിരം flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_കണ്ണി_Stmt* stmtsp = nullptr;
        എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elemp = പുതിയ AstArraySel{flp, internalp->clone_വൃക്ഷം(അസത്യം), i};
                stmtsp = Ast_കണ്ണി::addNext(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (dtypep->containsUnpackedStruct()) {
            വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* സ്ഥിരം selp
                    = പുതിയ AstStructSel{flp, internalp->clone_വൃക്ഷം(അസത്യം), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_കണ്ണി::addNext(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } അല്ലെങ്കിൽ {
            തിരികെ createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_വൃക്ഷം(), internalp);
        തിരികെ stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    സ്ഥിതം Ast_കണ്ണി_Stmt* createDpiArgCvt(AstVarScope* vscp, സ്ഥിരം std::string& dpiName,
                                        തർക്കം toDpi) {
        AstVar* സ്ഥിരം varp = vscp->varp();
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        സ്ഥിരം std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        സ്ഥിരം പൂർണ്ണസംഖ്യ total = strides.empty() ? 1 : strides[0].first * strides[0].second;
        സ്ഥിരം തർക്കം isStruct = varp->dtypep()->containsUnpackedStruct();
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_കണ്ണി_Stmt* stmtsp = nullptr;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < total; ++i) {
            Ast_കണ്ണി_വ്യഞ്ജകം* internalp
                = പുതിയ AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            വേണ്ടി (സ്ഥിരം auto& stride : strides) {
                internalp = പുതിയ AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            എങ്കിൽ (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_വാക്കുകൾ());
            } അല്ലെങ്കിൽ എങ്കിൽ (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_കണ്ണി::addNext(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        തിരികെ stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_കണ്ണി_FTask* സ്ഥിരം nodep, AstVar* സ്ഥിരം rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        സ്ഥിരം std::string cname = Ast_കണ്ണി::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        സ്ഥിരം അക്ഷരം* സ്ഥിരം tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        സ്ഥിരം std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(സത്യം, സത്യം) : "";
        // The function we are building
        AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(സത്യം);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(സത്യം);
        funcp->entryPoint(സത്യം);
        funcp->isStatic(സത്യം);
        funcp->protect(അസത്യം);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_മുകൾ_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        എങ്കിൽ (!makePortList(nodep, funcp)) തിരികെ nullptr;

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
        AstCStmt* സ്ഥിരം prep = പുതിയ AstCStmt{flp};
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
        സ്ഥിരം std::string cbtype
            = VIdProtect::protect(v3Global.opt.prefix() + "__Vcb_" + nodep->cname() + "_t");
        prep->add(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* സ്ഥിരം callp = പുതിയ AstCStmt{flp};
        സ്ഥിരം auto addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* സ്ഥിരം vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(അസത്യം);
            portp->protect(അസത്യം);
            // Add argument to call
            സ്ഥിരം VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(പുതിയ AstVarRef{portp->fileline(), vscp, access});
            തിരികെ vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_കണ്ണി* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var);
            എങ്കിൽ (!portp) തുടരുക;
            എങ്കിൽ (!portp->isIO()) തുടരുക;
            എങ്കിൽ (portp->isFuncReturn()) തുടരുക;
            എങ്കിൽ (portp == rtnvarp) തുടരുക;  // Handled below
            // Add argument to call
            AstVarScope* സ്ഥിരം outvscp = addFuncArg(portp);
            സ്ഥിരം തർക്കം isStruct = portp->dtypep()->containsUnpackedStruct();
            എങ്കിൽ (portp->isWritable()) {
                outCvtsp = Ast_കണ്ണി::addNext(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), സത്യം)
                                       : createAssignInternalToDpi(portp, സത്യം, tmpSuffixp, ""));
            }
            എങ്കിൽ (!portp->isNonOutput()) തുടരുക;
            // Convert input/inout arguments to dpi type
            സ്ഥിരം std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(അസത്യം).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), അസത്യം));
        }
        // Return value argument goes last
        എങ്കിൽ (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        എങ്കിൽ (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        എങ്കിൽ (rtnvarp) {
            funcp->addStmtsp(createDpiTemp(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, അസത്യം, tmpSuffixp, ""));
            സ്ഥിരം std::string index = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(പുതിയ AstCStmt{flp, "return " + rtnvarp->name() + index + ";"});
        }
        തിരികെ funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_കണ്ണി_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        സ്ഥിരം string name = Ast_കണ്ണി::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        സ്ഥിരം string rtnType = rtnvarp            ? rtnvarp->dpiArgType(സത്യം, സത്യം)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(സത്യം);
        funcp->dontCombine(സത്യം);
        funcp->entryPoint(അസത്യം);
        funcp->isMethod(അസത്യം);
        funcp->protect(അസത്യം);
        funcp->dpiPure(nodep->dpiPure());

        സ്ഥിരം പൂർണ്ണസംഖ്യ cost = static_cast<പൂർണ്ണസംഖ്യ>(V3Control::getProfileData(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_മുകൾ_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        എങ്കിൽ (!makePortList(nodep, funcp)) തിരികെ nullptr;
        തിരികെ funcp;
    }

    AstCFunc* getDpiFunc(Ast_കണ്ണി_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        സ്ഥിരം string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        സ്ഥിരം auto pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        എങ്കിൽ (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* സ്ഥിരം funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            തിരികെ funcp;
        } അല്ലെങ്കിൽ {
            // Seen this cname import before. Check if it's the same prototype.
            സ്ഥിരം Ast_കണ്ണി_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            എങ്കിൽ (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                തിരികെ nullptr;
            } അല്ലെങ്കിൽ {
                // Same signature, return the previously created CFunc
                തിരികെ firstFuncp;
            }
        }
    }

    സ്ഥിതം തർക്കം makePortList(Ast_കണ്ണി_FTask* nodep, AstCFunc* dpip) {
        തർക്കം allOk = സത്യം;
        // Copy nodep's list of function I/O to the new dpip c function
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (portp->isIO()) {
                    // Move it to new function
                    AstVar* സ്ഥിരം newPortp = portp->clone_വൃക്ഷം(അസത്യം);
                    newPortp->funcLocal(സത്യം);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    എങ്കിൽ (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = അസത്യം;
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
        തിരികെ allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    സ്ഥിതം തർക്കം isDpiCIdentifier(സ്ഥിരം string& name) {
        തിരികെ !name.empty() && VString::isIdentifier(name)
               && !std::isdigit(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_വാക്കുകൾ::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    തർക്കം checkDpiStruct(സ്ഥിരം AstVar* portp, സ്ഥിരം AstStructDType* sdtypep, തർക്കം nested) {
        സ്ഥിരം std::string name = sdtypep->dpiName();
        എങ്കിൽ (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(സത്യം) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            തിരികെ അസത്യം;
        }
        വേണ്ടി (സ്ഥിരം AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            എങ്കിൽ (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                തിരികെ അസത്യം;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            സ്ഥിരം Ast_കണ്ണി_DType* dtypep = itemp->subDTypep()->skipRefp();
            ആകുമ്പോൾ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം subp = VN_CAST(dtypep, കണ്ണി_UOr_Struct_DType);
            സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            സ്ഥിരം തർക്കം isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            എങ്കിൽ (dtypep->containsUnpackedStruct()) {
                എങ്കിൽ (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), സത്യം)) തിരികെ അസത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(സത്യം)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                തിരികെ അസത്യം;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        എങ്കിൽ (name.empty()) തിരികെ സത്യം;
        സ്ഥിരം AstStructDType* സ്ഥിരം firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        എങ്കിൽ (firstp->dpiTypedef() == sdtypep->dpiTypedef()) തിരികെ സത്യം;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        തിരികെ അസത്യം;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    തർക്കം checkDpiStructArgs(സ്ഥിരം Ast_കണ്ണി_FTask* nodep) {
        തർക്കം allOk = സത്യം;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            സ്ഥിരം AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var);
            എങ്കിൽ (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       അസത്യം)
                        && allOk;
            }
        }
        തിരികെ allOk;
    }

    ശൂന്യം bodyDpiImportFunc(Ast_കണ്ണി_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        സ്ഥിരം അക്ഷരം* സ്ഥിരം tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        എങ്കിൽ (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_തള്ളുക(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        വേണ്ടി (Ast_കണ്ണി* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                AstVarScope* സ്ഥിരം portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                എങ്കിൽ (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    എങ്കിൽ (args != "") args += ", ";

                    എങ്കിൽ (portp->isDpiOpenArray()) {
                        Ast_കണ്ണി_DType* സ്ഥിരം dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        സ്ഥിരം string propName = portp->name() + "__Vopenprops";
                        സ്ഥിരം string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(പുതിയ AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        സ്ഥിരം string name = portp->name() + "__Vopenarray";
                        സ്ഥിരം string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(പുതിയ AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } അല്ലെങ്കിൽ {
                        // Unpacked structures are passed by reference from a C array
                        സ്ഥിരം തർക്കം isStruct = portp->dtypep()->containsUnpackedStruct();
                        എങ്കിൽ (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            എങ്കിൽ (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(createDpiTemp(portp, tmpSuffixp));
                        എങ്കിൽ (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, സത്യം)
                                    : createAssignInternalToDpi(portp, അസത്യം, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        എങ്കിൽ (nodep->dpiContext()) {
            സ്ഥിരം string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(പുതിയ AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            എങ്കിൽ (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(createDpiTemp(rtnvscp->varp(), tmpSuffixp));
                സ്ഥിരം std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{nodep->fileline()};
                cstmtp->add(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->add(" = ");
                cstmtp->add(callp);  // RHS
                cstmtp->add(";");
                cfuncp->addStmtsp(cstmtp);
            } അല്ലെങ്കിൽ {
                // Othervise just call it
                cfuncp->addStmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        വേണ്ടി (Ast_കണ്ണി* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                portp->protect(അസത്യം);  // No additional exposure - already part of shown proto
                എങ്കിൽ (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* സ്ഥിരം portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, അസത്യം));
                }
            }
        }

        എങ്കിൽ (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_എടുക്കുക(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        എങ്കിൽ (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_മുകൾ_Scopep->createTemp("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        തിരികെ dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_കണ്ണി_FTask* nodep, തർക്കം ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_കണ്ണി::user2_Clear_വൃക്ഷം();
        AstVar* rtnvarp = nullptr;
        എങ്കിൽ (nodep->isFunction()) {
            AstVar* സ്ഥിരം portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            എങ്കിൽ (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* സ്ഥിരം bdtypep = portp->dtypep()->basicp();
                എങ്കിൽ (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(സത്യം)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ nullptr;
                }
                എങ്കിൽ (!bdtypep->isDpiPrimitive()) {
                    എങ്കിൽ (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    എങ്കിൽ (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->taskPublic()) {
                എങ്കിൽ (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            എങ്കിൽ (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(അസത്യം);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, അസത്യം);
            rtnvarp = portp;
            rtnvarp->funcLocal(സത്യം);
            rtnvarp->noCReset(സത്യം);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            എങ്കിൽ (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(അസത്യം);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = nullptr;
        എങ്കിൽ (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            എങ്കിൽ (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                എങ്കിൽ (rtnvarp) VL_DO_DANGLING(തള്ളുക_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        എങ്കിൽ (rtnvarp) {
            rtnvscp = പുതിയ AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        എങ്കിൽ (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } അല്ലെങ്കിൽ എങ്കിൽ (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        എങ്കിൽ (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        സ്ഥിരം string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* സ്ഥിരം cfuncp = പുതിയ AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->taskPublic() && rtnvarp) ? rtnvarp->cPubArgType(സത്യം, സത്യം) : "")};
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
        എങ്കിൽ (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(സത്യം);
            cfuncp->isLoose(സത്യം);
        } അല്ലെങ്കിൽ {
            cfuncp->isStatic(അസത്യം);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        എങ്കിൽ (nodep->name() == "new") cfuncp->isConstructor(സത്യം);
        എങ്കിൽ (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(സത്യം);
        എങ്കിൽ (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        എങ്കിൽ (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        സ്ഥിരം തർക്കം needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        എങ്കിൽ (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        എങ്കിൽ (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            എങ്കിൽ (cfuncp->isConstructor()) {
                തർക്കം isInterfaceClass = അസത്യം;
                എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                എങ്കിൽ (!isInterfaceClass) {
                    സ്ഥിരം string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        പുതിയ AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        എങ്കിൽ (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_നിവേശം_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_നിവേശം_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_നിവേശം_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        എങ്കിൽ (nodep->dpiExport()) {
            AstScopeName* സ്ഥിരം snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(സത്യം);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        വേണ്ടി (Ast_കണ്ണി *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    തള്ളുക_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, അസത്യം);
                        portp->funcLocal(സത്യം);
                        എങ്കിൽ (portp->valuep()) തള്ളുക_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        എങ്കിൽ (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = portp->dtypep()->skipRefp();
                            എങ്കിൽ (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } അല്ലെങ്കിൽ {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(സത്യം);
                    }
                    AstVarScope* സ്ഥിരം newvscp
                        = പുതിയ AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        എങ്കിൽ (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_കണ്ണി* bodysp = nodep->stmtsp();
        എങ്കിൽ (bodysp) {
            unlinkAndClone(nodep, bodysp, സത്യം);
            AstBegin* സ്ഥിരം tempp
                = പുതിയ AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, അസത്യം};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            വേണ്ടി (Ast_കണ്ണി *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                    എങ്കിൽ (portp->isIO()) VL_DO_DANGLING(തള്ളുക_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            എങ്കിൽ (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(tempp->delete_വൃക്ഷം(), tempp);
        }
        എങ്കിൽ (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        എങ്കിൽ (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(പുതിയ AstCReturn{
                rtnvscp->fileline(), പുതിയ AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        എങ്കിൽ (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            തർക്കം writesNonLocals = അസത്യം;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* സ്ഥിരം varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                എങ്കിൽ (varp->isFuncLocal()) തിരികെ;
                // Check if written
                എങ്കിൽ (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = സത്യം;
                }
                // Check if read
                എങ്കിൽ (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            എങ്കിൽ (writesNonLocals) {
                AstVarScope* സ്ഥിരം dpiExportTriggerp = getDpiExporTrigger();
                File_വരി* സ്ഥിരം flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* സ്ഥിരം assignp
                    = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    പുതിയ AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                എങ്കിൽ (v3Global.opt.systemC()) {
                    assignp->addNextHere(പുതിയ AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        എങ്കിൽ (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        തിരികെ cfuncp;
    }

    ശൂന്യം iterateIntoFTask(Ast_കണ്ണി_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    ശൂന്യം തിരുകുക_Before_Stmt(Ast_കണ്ണി* nodep, Ast_കണ്ണി* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->addHereThisAsNext(newp);
    }
    ശൂന്യം processArgs(Ast_കണ്ണി_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        വേണ്ടി (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* സ്ഥിരം arrayp = VN_CAST(argp->exprp(), ConsArray);
            എങ്കിൽ (!arrayp) തുടരുക;

            File_വരി* സ്ഥിരം flp = arrayp->fileline();
            AstVarScope* സ്ഥിരം substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(സത്യം);

            AstAssign* സ്ഥിരം assignp
                = പുതിയ AstAssign{flp, പുതിയ AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം exprstmtp = പുതിയ Ast_വ്യഞ്ജകം_Stmt{
                flp, assignp, പുതിയ AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    തർക്കം isIfaceLocalImpure(Ast_കണ്ണി_FTask* nodep, Ast_കണ്ണി* impurep) {
        സ്ഥിരം AstScope* സ്ഥിരം scopep = m_statep->getScope(nodep);
        എങ്കിൽ (!TaskStateVisitor::isIfaceFTaskScope(scopep)) തിരികെ അസത്യം;

        സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(impurep, VarRef);
        എങ്കിൽ (!refp || !refp->varScopep()) തിരികെ അസത്യം;

        തിരികെ refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ശൂന്യം visit(AstWith* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            തിരികെ;
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
        nodep->addExprp(പുതിയ AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ശൂന്യം visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            തിരികെ;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        എങ്കിൽ (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_എണ്ണം(), nodep, "No non-inline task, but no error issued");
            എങ്കിൽ (VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)) {
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            } അല്ലെങ്കിൽ {
                nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
            തിരികെ;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        എങ്കിൽ (nodep->taskp()->isFunction()) {
            AstVar* സ്ഥിരം fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            എങ്കിൽ (AstAssign* സ്ഥിരം assignp = VN_CAST(nodep->backp(), Assign)) {
                എങ്കിൽ (AstVarRef* സ്ഥിരം lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* സ്ഥിരം vscp = lhsp->varScopep();
                    എങ്കിൽ (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_വൃക്ഷം(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            എങ്കിൽ (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_കണ്ണി* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        സ്ഥിരം തർക്കം virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        എങ്കിൽ (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } അല്ലെങ്കിൽ {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        എങ്കിൽ (VN_IS(nodep, New)) {  // New not legal as while() condition
            തിരുകുക_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->backp(), കണ്ണി_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            തിരുകുക_Before_Stmt(nodep, beginp);
            AstVarRef* സ്ഥിരം outrefp = പുതിയ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* സ്ഥിരം outrefp = പുതിയ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_വ്യഞ്ജകം_Stmt* lambdap = പുതിയ Ast_വ്യഞ്ജകം_Stmt{nodep->fileline(), beginp, outrefp};

            എങ്കിൽ (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_വൃക്ഷം(അസത്യം);
                newvarp->funcLocal(സത്യം);

                // Create a new VarScope for the new variable
                AstVarScope* സ്ഥിരം newvscp
                    = പുതിയ AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreachAndNext([outvscp, newvscp](AstVarRef* refp) {
                    എങ്കിൽ (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->hasResult(അസത്യം);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* സ്ഥിരം exprp = പുതിയ AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(പുതിയ AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            VIsCached::clear_Cache_വൃക്ഷം();
        } അല്ലെങ്കിൽ {  // VN_IS(nodep->backp(), StmtExpr)
            തിരുകുക_Before_Stmt(nodep, beginp);
            എങ്കിൽ (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* സ്ഥിരം nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        എങ്കിൽ (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            പൂർണ്ണസംഖ്യ modes = 0;
            എങ്കിൽ (nodep->dpiImport()) modes++;
            എങ്കിൽ (nodep->dpiExport()) modes++;
            എങ്കിൽ (nodep->taskPublic()) modes++;
            എങ്കിൽ (nodep->classMethod()) modes++;
            എങ്കിൽ (v3Global.opt.protectIds() && nodep->taskPublic()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            എങ്കിൽ (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            സ്ഥിരം തർക്കം noInline = m_statep->ftaskNoInline(nodep);
            സ്ഥിരം തർക്കം needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            എങ്കിൽ (noInline && !nodep->classMethod() && !nodep->recursive()) {
                എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം impurep = m_statep->checkImpure(nodep)) {
                    എങ്കിൽ (!isIfaceLocalImpure(nodep, impurep)) {
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

            എങ്കിൽ (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_കണ്ണി_FTask* സ്ഥിരം clonedFuncp = nodep->clone_വൃക്ഷം(അസത്യം);
                എങ്കിൽ (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                സ്ഥിരം തർക്കം makeNonInlineFunc = noInline || needsNonInlineCFunc;
                എങ്കിൽ (AstCFunc* സ്ഥിരം cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->addNextHere(cfuncp);
                    എങ്കിൽ (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            എങ്കിൽ (nodep->isFunction()) {
                എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* സ്ഥിരം vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            വേണ്ടി (Ast_കണ്ണി *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* സ്ഥിരം vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_കണ്ണി_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_കണ്ണി_FTask_Ref* refp) { refs.തള്ളുക_back(refp); });
        വേണ്ടി (Ast_കണ്ണി_FTask_Ref* സ്ഥിരം refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* സ്ഥിരം interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{
                nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം)};
            Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = പുതിയ Ast_Sen_വൃക്ഷം{
                nodep->fileline(), പുതിയ AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* സ്ഥിരം activep
                = പുതിയ AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_വൃക്ഷം_Storep(activep->sentreep());
            activep->addStmtsp(
                പുതിയ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(പുതിയ AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_വൃക്ഷം(), refp);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = സത്യം;
        iterateChildren(nodep);
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
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

വർഗ്ഗം TaskIfaceSensVisitor final : പൊതു VNVisitorConst {
    // STATE
    തർക്കം m_underIfaceFunc = അസത്യം;  // Under a CFunc owned by an interface scope
    തർക്കം m_underSenses = അസത്യം;  // Under a sensitivity expression of such a CFunc

    // METHODS
    ശൂന്യം markSensesAndIterate(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!m_underIfaceFunc) തിരികെ;
        VL_RESTORER(m_underSenses);
        m_underSenses = സത്യം;
        iterateAndNextConstNull(nodep);
    }
    // VISITORS
    ശൂന്യം visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_Sen_വൃക്ഷം* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    ശൂന്യം visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (!m_underSenses) തിരികെ;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        എങ്കിൽ (nodep->varp()->isFuncLocal()) തിരികെ;
        എങ്കിൽ (AstIface* സ്ഥിരം ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Task class functions

സ്ഥിരം അക്ഷരം* സ്ഥിരം V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_കണ്ണി_FTask_Ref* nodep, Ast_കണ്ണി* taskStmtsp,
                                    V3TaskConnectState* statep, തർക്കം makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<സ്ഥിരം std::string, പൂർണ്ണസംഖ്യ> nameToIndex;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    പൂർണ്ണസംഖ്യ tpinnum = 0;
    AstVar* sformatp = nullptr;
    വേണ്ടി (Ast_കണ്ണി* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
            എങ്കിൽ (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                nameToIndex.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                എങ്കിൽ (portp->attrSFormat()) {
                    sformatp = portp;
                } അല്ലെങ്കിൽ എങ്കിൽ (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    പൂർണ്ണസംഖ്യ ppinnum = 0;
    തർക്കം reorganize = അസത്യം;
    വേണ്ടി (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        എങ്കിൽ (!argp->name().empty()) {
            // By name
            സ്ഥിരം auto it = nameToIndex.find(argp->name());
            എങ്കിൽ (it == nameToIndex.end()) {
                എങ്കിൽ (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_വൃക്ഷം(), argp);
                }
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_വൃക്ഷം();
                }
                tconnects[it->second].second = argp;
                reorganize = സത്യം;
            }
        } അല്ലെങ്കിൽ {  // By pin number
            എങ്കിൽ (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } അല്ലെങ്കിൽ എങ്കിൽ (ppinnum >= tpinnum) {
                എങ്കിൽ (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } അല്ലെങ്കിൽ എങ്കിൽ (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_വൃക്ഷം(), argp);
                }
            } അല്ലെങ്കിൽ {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    എങ്കിൽ (!makeChanges) തിരികെ tconnects;

    // Connect missing ones
    VInsertionSet<സ്ഥിരം AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < tpinnum; ++i) {
        AstVar* സ്ഥിരം portp = tconnects[i].first;
        എങ്കിൽ (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* newvaluep = nullptr;
            എങ്കിൽ (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = പുതിയ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } അല്ലെങ്കിൽ എങ്കിൽ (AstFuncRef* സ്ഥിരം funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം funcp = funcRefp->taskp();
                എങ്കിൽ (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } അല്ലെങ്കിൽ എങ്കിൽ (AstConst* സ്ഥിരം constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            എങ്കിൽ (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), കണ്ണി_വ്യഞ്ജകം));
                എങ്കിൽ (!VN_IS(newvaluep, Const)) {
                    എങ്കിൽ (statep) {
                        portp->pin_അക്കം(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.തിരുകുക(portp);
                    } അല്ലെങ്കിൽ {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = പുതിയ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_വൃക്ഷം(സത്യം) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* സ്ഥിരം newp = പുതിയ AstArg{nodep->fileline(), portp->name(), newvaluep};
            എങ്കിൽ (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_വൃക്ഷം(),
                            tconnects[i].second = nullptr);
            }
            tconnects[i].second = newp;
            reorganize = സത്യം;
        }
        എങ്കിൽ (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } അല്ലെങ്കിൽ {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
        AstArg* സ്ഥിരം argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    എങ്കിൽ (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        ആകുമ്പോൾ (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < tpinnum; ++i) {
            AstArg* സ്ഥിരം argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    എങ്കിൽ (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_വൃക്ഷം("-  ftref-out: ");
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    എങ്കിൽ (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        തിരികെ taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    തിരികെ tconnects;
}

ശൂന്യം V3Task::taskConnectWrap(Ast_കണ്ണി_FTask_Ref* nodep, സ്ഥിരം V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             സ്ഥിരം VInsertionSet<സ്ഥിരം AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    വേണ്ടി (സ്ഥിരം AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_അക്കം());
    സ്ഥിരം auto pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    എങ്കിൽ (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_കണ്ണി_FTask* സ്ഥിരം newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
        സ്ഥിരം AstVar* സ്ഥിരം portp = tconnect.first;
        AstArg* സ്ഥിരം argp = tconnect.second;
        എങ്കിൽ (argWrap.exists(portp)) {  // Removed arg
            statep->തള്ളുക_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_കണ്ണി_FTask* V3Task::taskConnectWrapNew(Ast_കണ്ണി_FTask* taskp, സ്ഥിരം string& newname,
                                         സ്ഥിരം V3TaskConnects& tconnects,
                                         സ്ഥിരം VInsertionSet<സ്ഥിരം AstVar*>& argWrap) {
    std::map<സ്ഥിരം AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_കണ്ണി_FTask* സ്ഥിരം newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->addNextHere(newTaskp);

    Ast_കണ്ണി_FTask_Ref* newCallp = nullptr;
    Ast_കണ്ണി* newCallInsertp = nullptr;
    എങ്കിൽ (VN_IS(taskp, Func)) {
        AstVar* സ്ഥിരം fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* സ്ഥിരം newFVarp = fvarp->clone_വൃക്ഷം(സത്യം);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = പുതിയ AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = പുതിയ AstAssign{taskp->fileline(),
                            പുതിയ AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(taskp, Task)) {
        newCallp = പുതിയ AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = പുതിയ Ast_Stmt_വ്യഞ്ജകം{taskp->fileline(), newCallp};
    } അല്ലെങ്കിൽ {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
        AstVar* സ്ഥിരം portp = tconnect.first;
        AstVar* newPortp;
        എങ്കിൽ (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = പുതിയ AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(സത്യം);
            എങ്കിൽ (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_വൃക്ഷം();
            newTaskp->addStmtsp(newPortp);
        } അല്ലെങ്കിൽ {  // Defaulting arg
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = VN_AS(portp->valuep(), കണ്ണി_വ്യഞ്ജകം);
            എങ്കിൽ ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                സ്ഥിരം VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* സ്ഥിരം refp = VN_AS(valuep->clone_വൃക്ഷം(അസത്യം), VarRef);
                refp->access(refAccess);
                AstArg* സ്ഥിരം newArgp = പുതിയ AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                തുടരുക;
            }
            // Create local temporary
            newPortp = പുതിയ AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(സത്യം);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            എങ്കിൽ (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* സ്ഥിരം newAssignp
                    = പുതിയ AstAssign{valuep->fileline(),
                                    പുതിയ AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_വൃക്ഷം(സത്യം)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* സ്ഥിരം newArgp = പുതിയ AstArg{
            portp->fileline(), portp->name(),
            പുതിയ AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        സ്ഥിരം auto it = oldNewVars.find(refp->varp());
        എങ്കിൽ (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    തിരികെ newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, തർക്കം isPtr, സ്ഥിരം string& frSuffix,
                                   സ്ഥിരം string& toSuffix, സ്ഥിരം string& frPrefix) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    സ്ഥിരം string frName = frPrefix + portp->name() + frSuffix;
    സ്ഥിരം string toName = portp->name() + toSuffix;
    സ്ഥിരം string idx = portp->name() + "__Vidx";
    size_t unpackSize = 1;  // non-unpacked array is treated as size 1
    പൂർണ്ണസംഖ്യ unpackDim = 0;
    എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpackSize = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(അസത്യം).second;
        എങ്കിൽ (unpackDim > 0) UASSERT_OBJ(unpackSize > 0, portp, "size must be greater than 0");
    }
    എങ്കിൽ (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        സ്ഥിരം തർക്കം isBit = portp->basicp()->isDpiBitVec();
        സ്ഥിരം തർക്കം needsFor = unpackSize > 1;
        എങ്കിൽ (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpackSize) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        എങ്കിൽ (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_വാക്കുകൾ()) + " * " + idx;
        }
        stmt += ", ";
        എങ്കിൽ (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } അല്ലെങ്കിൽ {
            stmt += frName + ")";
        }
    } അല്ലെങ്കിൽ {
        സ്ഥിരം തർക്കം isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        സ്ഥിരം തർക്കം isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        സ്ഥിരം string unpackAt = unpackSize > 1 ? "[" + idx + "]" : "[0]";
        എങ്കിൽ (unpackDim > 0) {
            സ്ഥിരം string forStmt = unpackSize > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpackSize) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        എങ്കിൽ (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        എങ്കിൽ (unpackDim > 0) {
            stmt += "(&" + frName;
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } അല്ലെങ്കിൽ {
            stmt += frName;
        }
        എങ്കിൽ (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    തിരികെ stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(സ്ഥിരം std::string& lhsName, AstVar* varp) {
    std::string cvt;
    തർക്കം useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    സ്ഥിരം std::vector<std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    സ്ഥിരം പൂർണ്ണസംഖ്യ total = strides.empty() ? 1 : strides[0].first * strides[0].second;
    സ്ഥിരം പൂർണ്ണസംഖ്യ width_വാക്കുകൾ = varp->basicp()->width_വാക്കുകൾ();
    string statements;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < total; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        വേണ്ടി (സ്ഥിരം auto& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        എങ്കിൽ (useSvVec) {
            സ്ഥിരം std::string offset = std::to_string(i * width_വാക്കുകൾ);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } അല്ലെങ്കിൽ {
            സ്ഥിരം std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    തിരികെ statements;
}

ശൂന്യം V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        സ്ഥിരം TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_വൃക്ഷം("task", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
