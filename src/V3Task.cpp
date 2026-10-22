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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Task.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3LanguageWords.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <tuple>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph subclasses

ವರ್ಗ TaskBaseVertex VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_ಗಂಟು* m_impurep = nullptr;  // Node causing impure function w/ outside references
    ತಾರ್ಕಿಕ m_noInline = ಅಸತ್ಯ;  // Marked with pragma
ಸಾರ್ವಜನಿಕ:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = ಸ್ವಭಾವ;
    ತಾರ್ಕಿಕ pure() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_impurep == nullptr; }
    Ast_ಗಂಟು* impure_ಗಂಟು() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_impurep; }
    ಶೂನ್ಯ impure(Ast_ಗಂಟು* nodep) { m_impurep = nodep; }
    ತಾರ್ಕಿಕ noInline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_noInline; }
    ಶೂನ್ಯ noInline(ತಾರ್ಕಿಕ flag) { m_noInline = flag; }
};

ವರ್ಗ TaskFTaskVertex final : ಸಾರ್ವಜನಿಕ TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_ಗಂಟು_FTask* ಧ್ರುವ m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    ತಾರ್ಕಿಕ m_needsNonInlineCFunc = ಅಸತ್ಯ;  // Needs CFunc for selected non-inlined call sites

ಸಾರ್ವಜನಿಕ:
    TaskFTaskVertex(V3Graph* graphp, Ast_ಗಂಟು_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = ಸ್ವಭಾವ;
    Ast_ಗಂಟು_FTask* nodep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_nodep; }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nodep()->name(); }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ pure() ? "black" : "red"; }
    AstCFunc* cFuncp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cFuncp; }
    ಶೂನ್ಯ cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    ತಾರ್ಕಿಕ needsNonInlineCFunc() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_needsNonInlineCFunc; }
    ಶೂನ್ಯ needsNonInlineCFunc(ತಾರ್ಕಿಕ flag) { m_needsNonInlineCFunc = flag; }
};

ವರ್ಗ TaskCodeVertex final : ಸಾರ್ವಜನಿಕ TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
ಸಾರ್ವಜನಿಕ:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = ಸ್ವಭಾವ;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ "*CODE*"; }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "green"; }
};

ವರ್ಗ TaskEdge final : ಸಾರ್ವಜನಿಕ V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
ಸಾರ್ವಜನಿಕ:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* ತುದಿ)
        : V3GraphEdge{graphp, fromp, ತುದಿ, 1, ಅಸತ್ಯ} {}
    ~TaskEdge() override = ಸ್ವಭಾವ;
    string dotLabel() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "w" + cvtToStr(weight()); }
};

//######################################################################

ವರ್ಗ TaskStateVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ಧ್ರುವ VNUser3InUse m_inuser3;
    ಧ್ರುವ VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<ಧ್ರುವ Ast_ಗಂಟು_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_ಗಂಟು_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    ತಾರ್ಕಿಕ m_underPortVar = ಅಸತ್ಯ;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isVirtualIfaceMethodCall(ಧ್ರುವ Ast_ಗಂಟು_FTask_Ref* refp) {
        ಧ್ರುವ AstMethodCall* ಧ್ರುವ mrefp = VN_CAST(refp, MethodCall);
        ಯದಿ (!mrefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = mrefp->fromp();
        ಯದಿ (!fromp || !fromp->dtypep()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        ಪ್ರತ್ಯಾಗಮ ifrefp && ifrefp->isVirtual();
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isIfaceFTaskScope(ಧ್ರುವ AstScope* scopep) {
        ಪ್ರತ್ಯಾಗಮ scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_ಗಂಟು_FTask* nodep) {
        AstScope* ಧ್ರುವ scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        ಪ್ರತ್ಯಾಗಮ scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        ಧ್ರುವ auto iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        ಪ್ರತ್ಯಾಗಮ iter->second;
    }
    AstClass* getClassp(Ast_ಗಂಟು_FTask* nodep) {
        AstClass* ಧ್ರುವ classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        ಪ್ರತ್ಯಾಗಮ classp;
    }
    ಶೂನ್ಯ remapFuncClassp(Ast_ಗಂಟು_FTask* nodep, Ast_ಗಂಟು_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    ತಾರ್ಕಿಕ ftaskNoInline(Ast_ಗಂಟು_FTask* nodep) { ಪ್ರತ್ಯಾಗಮ getFTaskVertex(nodep)->noInline(); }
    ತಾರ್ಕಿಕ ftaskNeedsNonInlineCFunc(Ast_ಗಂಟು_FTask* nodep) {
        ಪ್ರತ್ಯಾಗಮ getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_ಗಂಟು_FTask* nodep) { ಪ್ರತ್ಯಾಗಮ getFTaskVertex(nodep)->cFuncp(); }
    ಶೂನ್ಯ ftaskCFuncp(Ast_ಗಂಟು_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_ಗಂಟು* checkImpure(Ast_ಗಂಟು_FTask* nodep) { ಪ್ರತ್ಯಾಗಮ getFTaskVertex(nodep)->impure_ಗಂಟು(); }

ಗುಪ್ತ:
    TaskFTaskVertex* getFTaskVertex(Ast_ಗಂಟು_FTask* nodep) {
        ಯದಿ (!nodep->user4p()) nodep->user4p(ನೂತನ TaskFTaskVertex{&m_callGraph, nodep});
        ಪ್ರತ್ಯಾಗಮ static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    ಶೂನ್ಯ propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        ಪ್ರತಿ (V3GraphVertex& vtx : m_callGraph.vertices()) {
            ಯದಿ (TaskFTaskVertex* ಧ್ರುವ ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                ಯದಿ (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        ಯಾವತ್ (!workList.empty()) {
            TaskFTaskVertex* ಧ್ರುವ calleep = workList.back();
            workList.ತೆಗೆ_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            ಪ್ರತಿ (V3GraphEdge& edge : calleep->inEdges()) {
                ಯದಿ (TaskFTaskVertex* ಧ್ರುವ callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    ಯದಿ (!callerp->pure()) ಅನುವರ್ತ;  // Already processed or on workList
                    callerp->impure(calleep->impure_ಗಂಟು());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    ಶೂನ್ಯ decideInlining() {
        ಪ್ರತಿ (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* ಧ್ರುವ ftVtxp = vtx.cast<TaskFTaskVertex>();
            ಯದಿ (!ftVtxp) ಅನುವರ್ತ;
            // Move on if already decided not to inline
            ಯದಿ (ftVtxp->noInline()) ಅನುವರ್ತ;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            ಯದಿ (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(ಸತ್ಯ);
                ಅನುವರ್ತ;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            ಯದಿ (v3Global.opt.fInlineFuncsEager()) ಅನುವರ್ತ;

            Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            ಯದಿ (!ftaskp->verilogFunction()) ಅನುವರ್ತ;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            ಯದಿ (ftaskp->taskPublic()) ಅನುವರ್ತ;
            // If the function is pure, it's OK not to inline, so don't do it
            ಯದಿ (ftVtxp->pure()) ftVtxp->noInline(ಸತ್ಯ);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVarScope* ಧ್ರುವ vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ಧ್ರುವ varp = vscp->varp();
                ಯದಿ (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ taskp = VN_CAST(stmtp, ಗಂಟು_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* ಧ್ರುವ taskVtxp = getFTaskVertex(nodep->taskp());
        ನೂತನ TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        ಯದಿ (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(ಸತ್ಯ);
        // Do we have to disable inlining the function?
        ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        ಯದಿ (!taskVtxp->noInline()) {  // Else short-circuit below
            ಪ್ರತಿ (ಧ್ರುವ auto& itr : tconnects) {
                ಧ್ರುವ AstVar* ಧ್ರುವ portp = itr.first;
                ಧ್ರುವ AstArg* ಧ್ರುವ argp = itr.second;
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ pinp = argp->exprp()) {
                    ಯದಿ ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(ಸತ್ಯ);
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        ಯದಿ (nodep->dpiImport()) m_curVxp->noInline(ಸತ್ಯ);
        ಯದಿ (nodep->classMethod()) m_curVxp->noInline(ಸತ್ಯ);  // Until V3Task supports it
        ಯದಿ (nodep->recursive()) m_curVxp->noInline(ಸತ್ಯ);
        ಯದಿ (nodep->isConstructor()) {
            m_curVxp->noInline(ಸತ್ಯ);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstPragma* nodep) override {
        ಯದಿ (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(ಸತ್ಯ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        ಯದಿ (nodep->isIO()) m_underPortVar = ಸತ್ಯ;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (m_underPortVar) ಪ್ರತ್ಯಾಗಮ;
        AstVar* ಧ್ರುವ varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        ಯದಿ (nodep->access().isReadOnly() && varp->constPoolEntry()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (varp->user4u().toGraphVertex() != m_curVxp) {
            ಯದಿ (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstClass* nodep) override {
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
        Ast_ಗಂಟು* insertp = nullptr;
        ಪ್ರತಿ (AstInitialAutomatic* initialp : m_initialps) {
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ movep = initialp->stmtsp()) {
                movep->unlinkFrBackWithNext();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_ಗಂಟು::addNextNull(insertp, movep);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    ಶೂನ್ಯ visit(AstInitialAutomatic* nodep) override {
        m_initialps.ತಳ್ಳು_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = ನೂತನ TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.removeRedundantEdgesSum(&TaskEdge::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        ಯದಿ (dumpGraphLevel()) m_callGraph.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

ಸಂರಚನಾ TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    ಸ್ಥಿರ std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> unpackDimsAndStrides(AstVar* varp) {
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> dimStrides;
        AstUnpackArrayDType* ಧ್ರುವ unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ಯದಿ (!unpackp) ಪ್ರತ್ಯಾಗಮ dimStrides;

        ಧ್ರುವ std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ಧ್ರುವ size_t nDims = dims.size();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        ಪ್ರತಿ (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        ಪ್ರತ್ಯಾಗಮ dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    ಸ್ಥಿರ std::pair<std::string, ತಾರ್ಕಿಕ> dpiToInternalCvtStmt(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = dtypep->basicp();

        // DPI types using Primitive C types
        ಯದಿ (basicp->keyword() == VBasicDTypeKwd::CHANDLE) ಪ್ರತ್ಯಾಗಮ {"VL_CVT_VP_Q(", ಅಸತ್ಯ};
        ಯದಿ (basicp->keyword() == VBasicDTypeKwd::STRING) ಪ್ರತ್ಯಾಗಮ {"VL_CVT_N_CSTR(", ಅಸತ್ಯ};
        ಯದಿ (basicp->isDpiPrimitive()) ಪ್ರತ್ಯಾಗಮ {"(", ಅಸತ್ಯ};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ಧ್ರುವ std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ಧ್ರುವ ವರ್ಣ sizeChar = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ಧ್ರುವ std::string& size = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        ಪ್ರತ್ಯಾಗಮ {"VL_SET_"s + sizeChar + "_" + vecType + "(" + size + ", ", ಸತ್ಯ};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

ವರ್ಗ TaskVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<ಧ್ರುವ string, std::tuple<Ast_ಗಂಟು_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ಧ್ರುವ m_statep;  // Common state between visitors
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    Ast_ತುದಿ_Scope* ಧ್ರುವ m_ತುದಿ_Scopep = v3Global.rootp()->ತುದಿ_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_ಗಂಟು* m_insStmtp = nullptr;  // Where to insert statement
    ತಾರ್ಕಿಕ m_inSensesp = ಅಸತ್ಯ;  // Are we under a senitem?
    ತಾರ್ಕಿಕ m_inNew = ಅಸತ್ಯ;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    ಪೂರ್ಣಾಙ್ಕ m_uncon_Var_ಅಂಕೆ = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, ಧ್ರುವ AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ಧ್ರುವ string& name, AstVar* examplep) {
        AstVar* ಧ್ರುವ newvarp = ನೂತನ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(ಸತ್ಯ);
        funcp->addVarsp(newvarp);
        AstVarScope* ಧ್ರುವ newvscp = ನೂತನ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        ಪ್ರತ್ಯಾಗಮ newvscp;
    }
    AstVarScope* create_ಒಳಹರಿವು_Var(AstCFunc* funcp, ಧ್ರುವ string& name, VBasicDTypeKwd kwd) {
        AstVar* ಧ್ರುವ newvarp
            = ನೂತನ AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(ಸತ್ಯ);
        newvarp->direction(VDirection::ಒಳಹರಿವು);
        funcp->addArgsp(newvarp);
        AstVarScope* ಧ್ರುವ newvscp = ನೂತನ AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        ಪ್ರತ್ಯಾಗಮ newvscp;
    }
    ಸ್ಥಿರ AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ಧ್ರುವ refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
        ಪ್ರತ್ಯಾಗಮ vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ಧ್ರುವ Ast_ಗಂಟು_FTask_Ref* refp, AstVar* invarp,
                                ಧ್ರುವ std::string& suffix) {
        ಯದಿ (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            ಪ್ರತ್ಯಾಗಮ constPoolTable(invarp);
        } ಅನ್ಯಥಾ {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* ಧ್ರುವ newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* ಧ್ರುವ newvarp = newvscp->varp();
            newvarp->funcLocal(ಅಸತ್ಯ);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ newvscp;
        }
    }

    // Replace varrefs with new var pointer
    ಶೂನ್ಯ relink(Ast_ಗಂಟು* nodep) {
        nodep->foreachAndNext([](AstVarRef* refp) {
            ಯದಿ (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ಧ್ರುವ newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp, AstVarScope* newvscp, ತಾರ್ಕಿಕ pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inPinp = pinp;
        ಯದಿ (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inPinClonep
            = pureCheck ? inPinp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ) : inPinp->clone_ವೃಕ್ಷ(ಸತ್ಯ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        ಯದಿ (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = ನೂತನ AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* ಧ್ರುವ assp = ನೂತನ AstAssign{
            pinp->fileline(), ನೂತನ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, ಸತ್ಯ);  // Ok if in <= block
        ಪ್ರತ್ಯಾಗಮ assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp, AstVarScope* newvscp,
                                        ತಾರ್ಕಿಕ pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outPinp = pinp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* postRhsp = ನೂತನ AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        ಯದಿ (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            ಯದಿ (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, ಗಂಟು_Uniop)->lhsp();
                ಯದಿ (postRhsp->isSigned()) {
                    postRhsp = ನೂತನ AstISToRD{pinp->fileline(), postRhsp};
                } ಅನ್ಯಥಾ {
                    postRhsp = ನೂತನ AstIToRD{pinp->fileline(), postRhsp};
                }
            } ಅನ್ಯಥಾ {
                ಯದಿ (Ast_ಗಂಟು_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } ಅನ್ಯಥಾ ಯದಿ (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } ಅನ್ಯಥಾ {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                ಯದಿ (outPinp->width() < portp->width()) {
                    postRhsp = ನೂತನ AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } ಅನ್ಯಥಾ {  // pin width > port width
                    ಯದಿ (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = ನೂತನ AstExtendS{pinp->fileline(), postRhsp};
                    } ಅನ್ಯಥಾ {
                        postRhsp = ನೂತನ AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        ಯದಿ (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, ಗಂಟು_Uniop)->lhsp();
            postRhsp = ನೂತನ AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ outPinClonep
            = pureCheck ? outPinp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ) : outPinp->clone_ವೃಕ್ಷ(ಸತ್ಯ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        ಯದಿ (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = ನೂತನ AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* ಧ್ರುವ assp = ನೂತನ AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, ಸತ್ಯ);  // Ok if in <= block
        ಪ್ರತ್ಯಾಗಮ assp;
    }

    ಶೂನ್ಯ changeAtWriteRecurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp) {
        // Change nested at methods to writable variant
        ಯದಿ (AstCMethodHard* ಧ್ರುವ cMethodp = VN_CAST(exprp, CMethodHard)) {
            ಯದಿ (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    ಶೂನ್ಯ connectPort(ಧ್ರುವ Ast_ಗಂಟು_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_ಗಂಟು* beginp,
                     ತಾರ್ಕಿಕ inlineTask) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp = argp->exprp();
        ಯದಿ (inlineTask) {
            portp->unlinkFrBack();
            ತಳ್ಳು_Deletep(portp);  // Remove it from the clone (not original)
        }
        ಯದಿ (!pinp) {
            // Too few arguments in function call
        } ಅನ್ಯಥಾ {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            ಯದಿ (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_ಅಂಕೆ++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* ಧ್ರುವ newvscp = ನೂತನ AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* ಧ್ರುವ repp = ನೂತನ AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(pinp), pinp);
                pinp = repp;
            }
            ಯದಿ (inlineTask) {
                ತಳ್ಳು_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_ವೃಕ್ಷ(), argp);  // Args no longer needed
            }
            ಯದಿ (portp->isRef() || portp->isConstRef()) {
                ತಾರ್ಕಿಕ refArgOk = ಅಸತ್ಯ;
                ಯದಿ (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (AstCMethodHard* ಧ್ರುವ cMethodp = VN_CAST(pinp, CMethodHard)) {
                    ಯದಿ (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } ಅನ್ಯಥಾ {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                ಯದಿ (refArgOk) {
                    ಯದಿ (AstVarRef* ಧ್ರುವ varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } ಅನ್ಯಥಾ {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                ಯದಿ (inlineTask) {
                    ಯದಿ (AstVarRef* ಧ್ರುವ varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* ಧ್ರುವ localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } ಅನ್ಯಥಾ {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } ಅನ್ಯಥಾ ಯದಿ (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* ಧ್ರುವ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ಯದಿ (!inlineTask) {
                    pinp->replaceWith(
                        ನೂತನ AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    ತಳ್ಳು_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* ಧ್ರುವ preassp = connectPortMakeInAssign(pinp, newvscp, ಸತ್ಯ);
                beginp->addNextHere(preassp);

                AstAssign* ಧ್ರುವ postassp = connectPortMakeOutAssign(portp, pinp, newvscp, ಸತ್ಯ);
                beginp->addNext(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } ಅನ್ಯಥಾ ಯದಿ (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ಧ್ರುವ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                ಯದಿ (!inlineTask) {
                    pinp->replaceWith(ನೂತನ AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    ತಳ್ಳು_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* ಧ್ರುವ postassp = connectPortMakeOutAssign(portp, pinp, newvscp, ಅಸತ್ಯ);
                // Put assignment BEHIND of all other statements
                beginp->addNext(postassp);
            } ಅನ್ಯಥಾ ಯದಿ (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ಧ್ರುವ newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* ಧ್ರುವ preassp = connectPortMakeInAssign(pinp, newvscp, ಅಸತ್ಯ);
                // Put assignment in FRONT of all other statements
                beginp->addNextHere(preassp);
            }
        }
    }

    ತಾರ್ಕಿಕ hasRefArgument(Ast_ಗಂಟು_FTask* nodep) {
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (varp->isRef() || varp->isConstRef()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    Ast_ಗಂಟು* createInlinedFTask(Ast_ಗಂಟು_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_ಗಂಟು* ಧ್ರುವ newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_ವೃಕ್ಷ(ಸತ್ಯ) : nullptr;
        Ast_ಗಂಟು* ಧ್ರುವ beginp
            = ನೂತನ AstComment{refp->fileline(), "Function: "s + refp->name(), ಸತ್ಯ};
        ಯದಿ (newbodysp) beginp->addNext(newbodysp);
        ಯದಿ (debug() >= 9) beginp->dump_ವೃಕ್ಷ_And_Next(cout, "-  newbegi: ");
        //
        // Create input variables
        Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
        {
            ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            ಪ್ರತಿ (ಧ್ರುವ auto& itr : tconnects) {
                AstVar* ಧ್ರುವ portp = itr.first;
                AstArg* ಧ್ರುವ argp = itr.second;
                connectPort(refp, portp, argp, beginp, ಸತ್ಯ);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_ಗಂಟು* nextstmtp;
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    ಯದಿ (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* ಧ್ರುವ localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        ಯದಿ (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_ಗಂಟು* ಧ್ರುವ crstp = ನೂತನ AstAssign{
                                portp->fileline(),
                                ನೂತನ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                ನೂತನ AstCReset{portp->fileline(), portp, ಅಸತ್ಯ}};
                            portp->replaceWith(crstp);
                        } ಅನ್ಯಥಾ {
                            portp->unlinkFrBack();
                        }
                        ತಳ್ಳು_Deletep(portp);  // Remove it from the clone (not original)
                    }
                }
            }
        }
        // Create function output variables
        ಯದಿ (outvscp) {
            // UINFO(0, "setflag on " << funcp->fvarp() << " to " << outvscp);
            refp->taskp()->fvarp()->user2p(outvscp);
        }
        // Replace variable refs
        relink(beginp);
        //
        ಯದಿ (debug() >= 9) beginp->dump_ವೃಕ್ಷ_And_Next(cout, "-  iotask: ");
        ಪ್ರತ್ಯಾಗಮ beginp;
    }

    Ast_ಗಂಟು* createNonInlinedFTask(Ast_ಗಂಟು_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ಧ್ರುವ cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_ಗಂಟು* ಧ್ರುವ beginp
            = ನೂತನ AstComment{refp->fileline(), "Function: "s + refp->name(), ಸತ್ಯ};
        Ast_ಗಂಟು_CCall* ccallp;
        ಯದಿ (VN_IS(refp, New)) {
            AstCNew* ಧ್ರುವ cnewp = ನೂತನ AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMethodCall* ಧ್ರುವ mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = ನೂತನ AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        } ಅನ್ಯಥಾ {
            ccallp = ನೂತನ AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        }

        ಯದಿ (ಧ್ರುವ AstFuncRef* ಧ್ರುವ funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstTaskRef* ಧ್ರುವ taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMethodCall* ಧ್ರುವ methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            ಪ್ರತಿ (ಧ್ರುವ auto& itr : tconnects) {
                AstVar* ಧ್ರುವ portp = itr.first;
                AstArg* ಧ್ರುವ argp = itr.second;
                connectPort(refp, portp, argp, beginp, ಅಸತ್ಯ);
            }
        }
        // First argument is symbol table, then output if a function
        ಧ್ರುವ ತಾರ್ಕಿಕ needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        ಯದಿ (needSyms) ccallp->argTypes("vlSymsp");

        ಯದಿ (refp->taskp()->dpiContext()) {
            AstScopeName* ಧ್ರುವ snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_ಸಾಲು* ಧ್ರುವ flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                ನೂತನ AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(ನೂತನ AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        ಪ್ರತಿ (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        ಯದಿ (outvscp) ccallp->addArgsp(ನೂತನ AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        ಯದಿ (debug() >= 9) beginp->dump_ವೃಕ್ಷ_And_Next(cout, "-  nitask: ");
        ಪ್ರತ್ಯಾಗಮ beginp;
    }

    string dpiSignature(Ast_ಗಂಟು_FTask* nodep, AstVar* rtnvarp) ಧ್ರುವ {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        ಯದಿ (nodep->dpiPure()) dpiproto += "pure ";
        ಯದಿ (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(ಸತ್ಯ, ಸತ್ಯ) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                ಯದಿ (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    ಯದಿ (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ಧ್ರುವ std::string dpiType = portp->dpiArgType(ಅಸತ್ಯ, ಅಸತ್ಯ);
                    dpiproto += dpiType;
                    ಧ್ರುವ std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(ಅಸತ್ಯ);
                    ಯದಿ (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        ಪ್ರತ್ಯಾಗಮ dpiproto;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ checkLegalCIdentifier(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& name) {
        ಯದಿ (name.end() != std::find_if(name.begin(), name.end(), [](ವರ್ಣ c) {
                ಪ್ರತ್ಯಾಗಮ !std::isalnum(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* createDpiTemp(AstVar* portp, ಧ್ರುವ string& suffix) {
        ಧ್ರುವ string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCStmt{portp->fileline(), stmt};
    }

    ಶೂನ್ಯ unlinkAndClone(Ast_ಗಂಟು_FTask* funcp, Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ withNext) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        ಯದಿ (funcp->recursive()) {
            VNRelinker relinkHandle;
            ಯದಿ (withNext) {
                nodep->unlinkFrBackWithNext(&relinkHandle);
            } ಅನ್ಯಥಾ {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_ಗಂಟು* ಧ್ರುವ newp = nodep->clone_ವೃಕ್ಷ(withNext);
            relinkHandle.relink(newp);
        } ಅನ್ಯಥಾ {
            ಯದಿ (withNext) {
                nodep->unlinkFrBackWithNext();
            } ಅನ್ಯಥಾ {
                nodep->unlinkFrBack();
            }
        }
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* createAssignInternalToDpi(AstVar* portp, ತಾರ್ಕಿಕ isPtr, ಧ್ರುವ string& frSuffix,
                                              ಧ್ರುವ string& toSuffix) {
        ಧ್ರುವ string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* createDpiScalarCvt(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* internalp, ಧ್ರುವ std::string& dpi,
                                           ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep, ತಾರ್ಕಿಕ toDpi) {
        File_ಸಾಲು* ಧ್ರುವ flp = internalp->fileline();
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = dtypep->basicp();
        ಯದಿ (toDpi) {
            AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{flp};
            ಯದಿ (basicp->isDpiPrimitive()) {
                cstmtp->add(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->add(internalp);
                cstmtp->add(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } ಅನ್ಯಥಾ {
                cstmtp->add((basicp->isDpiBitVec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::to_string(dtypep->width()) + ", " + dpi + ", ");
                cstmtp->add(internalp);
                cstmtp->add(");");
            }
            ಪ್ರತ್ಯಾಗಮ cstmtp;
        }
        std::string cvt;
        ತಾರ್ಕಿಕ useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        ಯದಿ (useSvVec) {
            AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            ಪ್ರತ್ಯಾಗಮ cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ cwidth = basicp->width_ಪದಗಳು() * VL_EDATASIZE;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstCExpr{flp, cvt + dpi + ")", cwidth};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, internalp, ನೂತನ AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* createDpiCvt(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* internalp, ಧ್ರುವ std::string& dpi,
                                     ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep, ತಾರ್ಕಿಕ toDpi) {
        File_ಸಾಲು* ಧ್ರುವ flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_ಗಂಟು_Stmt* stmtsp = nullptr;
        ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elemp = ನೂತನ AstArraySel{flp, internalp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), i};
                stmtsp = Ast_ಗಂಟು::addNext(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } ಅನ್ಯಥಾ ಯದಿ (dtypep->containsUnpackedStruct()) {
            ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ಧ್ರುವ selp
                    = ನೂತನ AstStructSel{flp, internalp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_ಗಂಟು::addNext(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_ವೃಕ್ಷ(), internalp);
        ಪ್ರತ್ಯಾಗಮ stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* createDpiArgCvt(AstVarScope* vscp, ಧ್ರುವ std::string& dpiName,
                                        ತಾರ್ಕಿಕ toDpi) {
        AstVar* ಧ್ರುವ varp = vscp->varp();
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();
        ಧ್ರುವ std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ total = strides.empty() ? 1 : strides[0].first * strides[0].second;
        ಧ್ರುವ ತಾರ್ಕಿಕ isStruct = varp->dtypep()->containsUnpackedStruct();
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_ಗಂಟು_Stmt* stmtsp = nullptr;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < total; ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* internalp
                = ನೂತನ AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            ಪ್ರತಿ (ಧ್ರುವ auto& stride : strides) {
                internalp = ನೂತನ AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            ಯದಿ (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_ಪದಗಳು());
            } ಅನ್ಯಥಾ ಯದಿ (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_ಗಂಟು::addNext(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        ಪ್ರತ್ಯಾಗಮ stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_ಗಂಟು_FTask* ಧ್ರುವ nodep, AstVar* ಧ್ರುವ rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ಧ್ರುವ std::string cname = Ast_ಗಂಟು::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ಧ್ರುವ std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(ಸತ್ಯ, ಸತ್ಯ) : "";
        // The function we are building
        AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(ಸತ್ಯ);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(ಸತ್ಯ);
        funcp->entryPoint(ಸತ್ಯ);
        funcp->isStatic(ಸತ್ಯ);
        funcp->protect(ಅಸತ್ಯ);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_ತುದಿ_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        ಯದಿ (!makePortList(nodep, funcp)) ಪ್ರತ್ಯಾಗಮ nullptr;

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
        AstCStmt* ಧ್ರುವ prep = ನೂತನ AstCStmt{flp};
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
        ಧ್ರುವ std::string cbtype
            = VIdProtect::protect(v3Global.opt.prefix() + "__Vcb_" + nodep->cname() + "_t");
        prep->add(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ಧ್ರುವ callp = ನೂತನ AstCStmt{flp};
        ಧ್ರುವ auto addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ಧ್ರುವ vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(ಅಸತ್ಯ);
            portp->protect(ಅಸತ್ಯ);
            // Add argument to call
            ಧ್ರುವ VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(ನೂತನ AstVarRef{portp->fileline(), vscp, access});
            ಪ್ರತ್ಯಾಗಮ vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_ಗಂಟು* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var);
            ಯದಿ (!portp) ಅನುವರ್ತ;
            ಯದಿ (!portp->isIO()) ಅನುವರ್ತ;
            ಯದಿ (portp->isFuncReturn()) ಅನುವರ್ತ;
            ಯದಿ (portp == rtnvarp) ಅನುವರ್ತ;  // Handled below
            // Add argument to call
            AstVarScope* ಧ್ರುವ outvscp = addFuncArg(portp);
            ಧ್ರುವ ತಾರ್ಕಿಕ isStruct = portp->dtypep()->containsUnpackedStruct();
            ಯದಿ (portp->isWritable()) {
                outCvtsp = Ast_ಗಂಟು::addNext(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), ಸತ್ಯ)
                                       : createAssignInternalToDpi(portp, ಸತ್ಯ, tmpSuffixp, ""));
            }
            ಯದಿ (!portp->isNonOutput()) ಅನುವರ್ತ;
            // Convert input/inout arguments to dpi type
            ಧ್ರುವ std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(ಅಸತ್ಯ).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), ಅಸತ್ಯ));
        }
        // Return value argument goes last
        ಯದಿ (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        ಯದಿ (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        ಯದಿ (rtnvarp) {
            funcp->addStmtsp(createDpiTemp(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, ಅಸತ್ಯ, tmpSuffixp, ""));
            ಧ್ರುವ std::string index = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(ನೂತನ AstCStmt{flp, "return " + rtnvarp->name() + index + ";"});
        }
        ಪ್ರತ್ಯಾಗಮ funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_ಗಂಟು_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ಧ್ರುವ string name = Ast_ಗಂಟು::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ಧ್ರುವ string rtnType = rtnvarp            ? rtnvarp->dpiArgType(ಸತ್ಯ, ಸತ್ಯ)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(ಸತ್ಯ);
        funcp->dontCombine(ಸತ್ಯ);
        funcp->entryPoint(ಅಸತ್ಯ);
        funcp->isMethod(ಅಸತ್ಯ);
        funcp->protect(ಅಸತ್ಯ);
        funcp->dpiPure(nodep->dpiPure());

        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ cost = static_cast<ಪೂರ್ಣಾಙ್ಕ>(V3Control::getProfileData(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_ತುದಿ_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        ಯದಿ (!makePortList(nodep, funcp)) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ funcp;
    }

    AstCFunc* getDpiFunc(Ast_ಗಂಟು_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ಧ್ರುವ string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ಧ್ರುವ auto pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        ಯದಿ (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ಧ್ರುವ funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            ಪ್ರತ್ಯಾಗಮ funcp;
        } ಅನ್ಯಥಾ {
            // Seen this cname import before. Check if it's the same prototype.
            ಧ್ರುವ Ast_ಗಂಟು_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            ಯದಿ (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                ಪ್ರತ್ಯಾಗಮ nullptr;
            } ಅನ್ಯಥಾ {
                // Same signature, return the previously created CFunc
                ಪ್ರತ್ಯಾಗಮ firstFuncp;
            }
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ makePortList(Ast_ಗಂಟು_FTask* nodep, AstCFunc* dpip) {
        ತಾರ್ಕಿಕ allOk = ಸತ್ಯ;
        // Copy nodep's list of function I/O to the new dpip c function
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                ಯದಿ (portp->isIO()) {
                    // Move it to new function
                    AstVar* ಧ್ರುವ newPortp = portp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    newPortp->funcLocal(ಸತ್ಯ);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    ಯದಿ (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = ಅಸತ್ಯ;
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
        ಪ್ರತ್ಯಾಗಮ allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isDpiCIdentifier(ಧ್ರುವ string& name) {
        ಪ್ರತ್ಯಾಗಮ !name.empty() && VString::isIdentifier(name)
               && !std::isdigit(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_ಪದಗಳು::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    ತಾರ್ಕಿಕ checkDpiStruct(ಧ್ರುವ AstVar* portp, ಧ್ರುವ AstStructDType* sdtypep, ತಾರ್ಕಿಕ nested) {
        ಧ್ರುವ std::string name = sdtypep->dpiName();
        ಯದಿ (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(ಸತ್ಯ) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತಿ (ಧ್ರುವ AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            ಯದಿ (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep = itemp->subDTypep()->skipRefp();
            ಯಾವತ್ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ subp = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType);
            ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ಧ್ರುವ ತಾರ್ಕಿಕ isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            ಯದಿ (dtypep->containsUnpackedStruct()) {
                ಯದಿ (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), ಸತ್ಯ)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(ಸತ್ಯ)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        ಯದಿ (name.empty()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಧ್ರುವ AstStructDType* ಧ್ರುವ firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        ಯದಿ (firstp->dpiTypedef() == sdtypep->dpiTypedef()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    ತಾರ್ಕಿಕ checkDpiStructArgs(ಧ್ರುವ Ast_ಗಂಟು_FTask* nodep) {
        ತಾರ್ಕಿಕ allOk = ಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಧ್ರುವ AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var);
            ಯದಿ (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       ಅಸತ್ಯ)
                        && allOk;
            }
        }
        ಪ್ರತ್ಯಾಗಮ allOk;
    }

    ಶೂನ್ಯ bodyDpiImportFunc(Ast_ಗಂಟು_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        ಯದಿ (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_ತಳ್ಳು(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ಧ್ರುವ portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                ಯದಿ (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    ಯದಿ (args != "") args += ", ";

                    ಯದಿ (portp->isDpiOpenArray()) {
                        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ಧ್ರುವ string propName = portp->name() + "__Vopenprops";
                        ಧ್ರುವ string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(ನೂತನ AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ಧ್ರುವ string name = portp->name() + "__Vopenarray";
                        ಧ್ರುವ string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(ನೂತನ AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } ಅನ್ಯಥಾ {
                        // Unpacked structures are passed by reference from a C array
                        ಧ್ರುವ ತಾರ್ಕಿಕ isStruct = portp->dtypep()->containsUnpackedStruct();
                        ಯದಿ (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            ಯದಿ (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(createDpiTemp(portp, tmpSuffixp));
                        ಯದಿ (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ಸತ್ಯ)
                                    : createAssignInternalToDpi(portp, ಅಸತ್ಯ, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        ಯದಿ (nodep->dpiContext()) {
            ಧ್ರುವ string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(ನೂತನ AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            ಯದಿ (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(createDpiTemp(rtnvscp->varp(), tmpSuffixp));
                ಧ್ರುವ std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{nodep->fileline()};
                cstmtp->add(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->add(" = ");
                cstmtp->add(callp);  // RHS
                cstmtp->add(";");
                cfuncp->addStmtsp(cstmtp);
            } ಅನ್ಯಥಾ {
                // Othervise just call it
                cfuncp->addStmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                portp->protect(ಅಸತ್ಯ);  // No additional exposure - already part of shown proto
                ಯದಿ (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ಧ್ರುವ portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ಅಸತ್ಯ));
                }
            }
        }

        ಯದಿ (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_ತೆಗೆ(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        ಯದಿ (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_ತುದಿ_Scopep->createTemp("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        ಪ್ರತ್ಯಾಗಮ dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_ಗಂಟು_FTask* nodep, ತಾರ್ಕಿಕ ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
        AstVar* rtnvarp = nullptr;
        ಯದಿ (nodep->isFunction()) {
            AstVar* ಧ್ರುವ portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            ಯದಿ (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ಧ್ರುವ bdtypep = portp->dtypep()->basicp();
                ಯದಿ (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(ಸತ್ಯ)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ nullptr;
                }
                ಯದಿ (!bdtypep->isDpiPrimitive()) {
                    ಯದಿ (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    ಯದಿ (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } ಅನ್ಯಥಾ ಯದಿ (nodep->taskPublic()) {
                ಯದಿ (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            ಯದಿ (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(ಅಸತ್ಯ);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, ಅಸತ್ಯ);
            rtnvarp = portp;
            rtnvarp->funcLocal(ಸತ್ಯ);
            rtnvarp->noCReset(ಸತ್ಯ);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            ಯದಿ (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(ಅಸತ್ಯ);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = nullptr;
        ಯದಿ (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            ಯದಿ (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                ಯದಿ (rtnvarp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        ಯದಿ (rtnvarp) {
            rtnvscp = ನೂತನ AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        ಯದಿ (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } ಅನ್ಯಥಾ ಯದಿ (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } ಅನ್ಯಥಾ ಯದಿ (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        ಯದಿ (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        ಧ್ರುವ string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* ಧ್ರುವ cfuncp = ನೂತನ AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->taskPublic() && rtnvarp) ? rtnvarp->cPubArgType(ಸತ್ಯ, ಸತ್ಯ) : "")};
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
        ಯದಿ (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(ಸತ್ಯ);
            cfuncp->isLoose(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            cfuncp->isStatic(ಅಸತ್ಯ);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        ಯದಿ (nodep->name() == "new") cfuncp->isConstructor(ಸತ್ಯ);
        ಯದಿ (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(ಸತ್ಯ);
        ಯದಿ (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        ಯದಿ (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ಧ್ರುವ ತಾರ್ಕಿಕ needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        ಯದಿ (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        ಯದಿ (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            ಯದಿ (cfuncp->isConstructor()) {
                ತಾರ್ಕಿಕ isInterfaceClass = ಅಸತ್ಯ;
                ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                ಯದಿ (!isInterfaceClass) {
                    ಧ್ರುವ string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        ನೂತನ AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        ಯದಿ (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_ಒಳಹರಿವು_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_ಒಳಹರಿವು_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_ಒಳಹರಿವು_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        ಯದಿ (nodep->dpiExport()) {
            AstScopeName* ಧ್ರುವ snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(ಸತ್ಯ);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                ಯದಿ (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    ತಳ್ಳು_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } ಅನ್ಯಥಾ {
                    ಯದಿ (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, ಅಸತ್ಯ);
                        portp->funcLocal(ಸತ್ಯ);
                        ಯದಿ (portp->valuep()) ತಳ್ಳು_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        ಯದಿ (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = portp->dtypep()->skipRefp();
                            ಯದಿ (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } ಅನ್ಯಥಾ {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(ಸತ್ಯ);
                    }
                    AstVarScope* ಧ್ರುವ newvscp
                        = ನೂತನ AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        ಯದಿ (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_ಗಂಟು* bodysp = nodep->stmtsp();
        ಯದಿ (bodysp) {
            unlinkAndClone(nodep, bodysp, ಸತ್ಯ);
            AstBegin* ಧ್ರುವ tempp
                = ನೂತನ AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, ಅಸತ್ಯ};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                    ಯದಿ (portp->isIO()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            ಯದಿ (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(tempp->delete_ವೃಕ್ಷ(), tempp);
        }
        ಯದಿ (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        ಯದಿ (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(ನೂತನ AstCReturn{
                rtnvscp->fileline(), ನೂತನ AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        ಯದಿ (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            ತಾರ್ಕಿಕ writesNonLocals = ಅಸತ್ಯ;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* ಧ್ರುವ varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                ಯದಿ (varp->isFuncLocal()) ಪ್ರತ್ಯಾಗಮ;
                // Check if written
                ಯದಿ (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = ಸತ್ಯ;
                }
                // Check if read
                ಯದಿ (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            ಯದಿ (writesNonLocals) {
                AstVarScope* ಧ್ರುವ dpiExportTriggerp = getDpiExporTrigger();
                File_ಸಾಲು* ಧ್ರುವ flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* ಧ್ರುವ assignp
                    = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    ನೂತನ AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                ಯದಿ (v3Global.opt.systemC()) {
                    assignp->addNextHere(ನೂತನ AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        ಯದಿ (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        ಪ್ರತ್ಯಾಗಮ cfuncp;
    }

    ಶೂನ್ಯ iterateIntoFTask(Ast_ಗಂಟು_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    ಶೂನ್ಯ ಸೇರಿಸು_Before_Stmt(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->addHereThisAsNext(newp);
    }
    ಶೂನ್ಯ processArgs(Ast_ಗಂಟು_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        ಪ್ರತಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ಧ್ರುವ arrayp = VN_CAST(argp->exprp(), ConsArray);
            ಯದಿ (!arrayp) ಅನುವರ್ತ;

            File_ಸಾಲು* ಧ್ರುವ flp = arrayp->fileline();
            AstVarScope* ಧ್ರುವ substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(ಸತ್ಯ);

            AstAssign* ಧ್ರುವ assignp
                = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ exprstmtp = ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                flp, assignp, ನೂತನ AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    ತಾರ್ಕಿಕ isIfaceLocalImpure(Ast_ಗಂಟು_FTask* nodep, Ast_ಗಂಟು* impurep) {
        ಧ್ರುವ AstScope* ಧ್ರುವ scopep = m_statep->getScope(nodep);
        ಯದಿ (!TaskStateVisitor::isIfaceFTaskScope(scopep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(impurep, VarRef);
        ಯದಿ (!refp || !refp->varScopep()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಪ್ರತ್ಯಾಗಮ refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ಶೂನ್ಯ visit(AstWith* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            ಪ್ರತ್ಯಾಗಮ;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        nodep->addExprp(ನೂತನ AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    ಶೂನ್ಯ visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಯದಿ (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        ಯದಿ (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_ಎಣಿಕೆ(), nodep, "No non-inline task, but no error issued");
            ಯದಿ (VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            } ಅನ್ಯಥಾ {
                nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        ಯದಿ (nodep->taskp()->isFunction()) {
            AstVar* ಧ್ರುವ fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            ಯದಿ (AstAssign* ಧ್ರುವ assignp = VN_CAST(nodep->backp(), Assign)) {
                ಯದಿ (AstVarRef* ಧ್ರುವ lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ಧ್ರುವ vscp = lhsp->varScopep();
                    ಯದಿ (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_ವೃಕ್ಷ(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            ಯದಿ (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_ಗಂಟು* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ಧ್ರುವ ತಾರ್ಕಿಕ virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        ಯದಿ (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } ಅನ್ಯಥಾ {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        ಯದಿ (VN_IS(nodep, New)) {  // New not legal as while() condition
            ಸೇರಿಸು_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->backp(), ಗಂಟು_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            ಸೇರಿಸು_Before_Stmt(nodep, beginp);
            AstVarRef* ಧ್ರುವ outrefp = ನೂತನ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ಧ್ರುವ outrefp = ನೂತನ AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* lambdap = ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{nodep->fileline(), beginp, outrefp};

            ಯದಿ (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                newvarp->funcLocal(ಸತ್ಯ);

                // Create a new VarScope for the new variable
                AstVarScope* ಧ್ರುವ newvscp
                    = ನೂತನ AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreachAndNext([outvscp, newvscp](AstVarRef* refp) {
                    ಯದಿ (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->hasResult(ಅಸತ್ಯ);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ಧ್ರುವ exprp = ನೂತನ AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(ನೂತನ AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            VIsCached::clear_Cache_ವೃಕ್ಷ();
        } ಅನ್ಯಥಾ {  // VN_IS(nodep->backp(), StmtExpr)
            ಸೇರಿಸು_Before_Stmt(nodep, beginp);
            ಯದಿ (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* ಧ್ರುವ nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        ಯದಿ (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            ಪೂರ್ಣಾಙ್ಕ modes = 0;
            ಯದಿ (nodep->dpiImport()) modes++;
            ಯದಿ (nodep->dpiExport()) modes++;
            ಯದಿ (nodep->taskPublic()) modes++;
            ಯದಿ (nodep->classMethod()) modes++;
            ಯದಿ (v3Global.opt.protectIds() && nodep->taskPublic()) {
                // We always call protect() on names, we don't check if public or not
                // Hence any external references wouldn't be able to find the refed public object.
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Using --protect-ids with public function");
            }
            ಯದಿ (modes > 1) {
                nodep->v3error("Cannot mix DPI import, DPI export, class methods, and/or public "
                               "on same function: "
                               << nodep->prettyNameQ());
            }

            ಧ್ರುವ ತಾರ್ಕಿಕ noInline = m_statep->ftaskNoInline(nodep);
            ಧ್ರುವ ತಾರ್ಕಿಕ needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            ಯದಿ (noInline && !nodep->classMethod() && !nodep->recursive()) {
                ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ impurep = m_statep->checkImpure(nodep)) {
                    ಯದಿ (!isIfaceLocalImpure(nodep, impurep)) {
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

            ಯದಿ (noInline || needsNonInlineCFunc || nodep->dpiImport() || nodep->dpiExport()
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_ಗಂಟು_FTask* ಧ್ರುವ clonedFuncp = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                ಯದಿ (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ಧ್ರುವ ತಾರ್ಕಿಕ makeNonInlineFunc = noInline || needsNonInlineCFunc;
                ಯದಿ (AstCFunc* ಧ್ರುವ cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->addNextHere(cfuncp);
                    ಯದಿ (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            ಯದಿ (nodep->isFunction()) {
                ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* ಧ್ರುವ vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ಧ್ರುವ vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_ಗಂಟು_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_ಗಂಟು_FTask_Ref* refp) { refs.ತಳ್ಳು_back(refp); });
        ಪ್ರತಿ (Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ಧ್ರುವ interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{
                nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
            Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = ನೂತನ Ast_Sen_ವೃಕ್ಷ{
                nodep->fileline(), ನೂತನ AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* ಧ್ರುವ activep
                = ನೂತನ AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
            activep->addStmtsp(
                ನೂತನ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(ನೂತನ AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        ಯದಿ (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
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

ವರ್ಗ TaskIfaceSensVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    ತಾರ್ಕಿಕ m_underIfaceFunc = ಅಸತ್ಯ;  // Under a CFunc owned by an interface scope
    ತಾರ್ಕಿಕ m_underSenses = ಅಸತ್ಯ;  // Under a sensitivity expression of such a CFunc

    // METHODS
    ಶೂನ್ಯ markSensesAndIterate(Ast_ಗಂಟು* nodep) {
        ಯದಿ (!m_underIfaceFunc) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_underSenses);
        m_underSenses = ಸತ್ಯ;
        iterateAndNextConstNull(nodep);
    }
    // VISITORS
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_Sen_ವೃಕ್ಷ* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    ಶೂನ್ಯ visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (!m_underSenses) ಪ್ರತ್ಯಾಗಮ;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        ಯದಿ (nodep->varp()->isFuncLocal()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (AstIface* ಧ್ರುವ ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Task class functions

ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_ಗಂಟು_FTask_Ref* nodep, Ast_ಗಂಟು* taskStmtsp,
                                    V3TaskConnectState* statep, ತಾರ್ಕಿಕ makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ಧ್ರುವ std::string, ಪೂರ್ಣಾಙ್ಕ> nameToIndex;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    ಪೂರ್ಣಾಙ್ಕ tpinnum = 0;
    AstVar* sformatp = nullptr;
    ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
            ಯದಿ (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                nameToIndex.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                ಯದಿ (portp->attrSFormat()) {
                    sformatp = portp;
                } ಅನ್ಯಥಾ ಯದಿ (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    ಪೂರ್ಣಾಙ್ಕ ppinnum = 0;
    ತಾರ್ಕಿಕ reorganize = ಅಸತ್ಯ;
    ಪ್ರತಿ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        ಯದಿ (!argp->name().empty()) {
            // By name
            ಧ್ರುವ auto it = nameToIndex.find(argp->name());
            ಯದಿ (it == nameToIndex.end()) {
                ಯದಿ (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_ವೃಕ್ಷ(), argp);
                }
            } ಅನ್ಯಥಾ {
                ಯದಿ (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_ವೃಕ್ಷ();
                }
                tconnects[it->second].second = argp;
                reorganize = ಸತ್ಯ;
            }
        } ಅನ್ಯಥಾ {  // By pin number
            ಯದಿ (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } ಅನ್ಯಥಾ ಯದಿ (ppinnum >= tpinnum) {
                ಯದಿ (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } ಅನ್ಯಥಾ ಯದಿ (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_ವೃಕ್ಷ(), argp);
                }
            } ಅನ್ಯಥಾ {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    ಯದಿ (!makeChanges) ಪ್ರತ್ಯಾಗಮ tconnects;

    // Connect missing ones
    VInsertionSet<ಧ್ರುವ AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < tpinnum; ++i) {
        AstVar* ಧ್ರುವ portp = tconnects[i].first;
        ಯದಿ (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newvaluep = nullptr;
            ಯದಿ (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = ನೂತನ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } ಅನ್ಯಥಾ ಯದಿ (AstFuncRef* ಧ್ರುವ funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ಧ್ರುವ Ast_ಗಂಟು_FTask* ಧ್ರುವ funcp = funcRefp->taskp();
                ಯದಿ (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } ಅನ್ಯಥಾ ಯದಿ (AstConst* ಧ್ರುವ constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            ಯದಿ (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
                ಯದಿ (!VN_IS(newvaluep, Const)) {
                    ಯದಿ (statep) {
                        portp->pin_ಅಂಕೆ(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.ಸೇರಿಸು(portp);
                    } ಅನ್ಯಥಾ {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = ನೂತನ AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_ವೃಕ್ಷ(ಸತ್ಯ) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ಧ್ರುವ newp = ನೂತನ AstArg{nodep->fileline(), portp->name(), newvaluep};
            ಯದಿ (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_ವೃಕ್ಷ(),
                            tconnects[i].second = nullptr);
            }
            tconnects[i].second = newp;
            reorganize = ಸತ್ಯ;
        }
        ಯದಿ (tconnects[i].second) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].second);
        } ಅನ್ಯಥಾ {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
        AstArg* ಧ್ರುವ argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    ಯದಿ (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        ಯಾವತ್ (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < tpinnum; ++i) {
            AstArg* ಧ್ರುವ argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    ಯದಿ (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_ವೃಕ್ಷ("-  ftref-out: ");
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    ಯದಿ (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        ಪ್ರತ್ಯಾಗಮ taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    ಪ್ರತ್ಯಾಗಮ tconnects;
}

ಶೂನ್ಯ V3Task::taskConnectWrap(Ast_ಗಂಟು_FTask_Ref* nodep, ಧ್ರುವ V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ಧ್ರುವ VInsertionSet<ಧ್ರುವ AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    ಪ್ರತಿ (ಧ್ರುವ AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_ಅಂಕೆ());
    ಧ್ರುವ auto pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    ಯದಿ (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_ಗಂಟು_FTask* ಧ್ರುವ newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
        ಧ್ರುವ AstVar* ಧ್ರುವ portp = tconnect.first;
        AstArg* ಧ್ರುವ argp = tconnect.second;
        ಯದಿ (argWrap.exists(portp)) {  // Removed arg
            statep->ತಳ್ಳು_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_ಗಂಟು_FTask* V3Task::taskConnectWrapNew(Ast_ಗಂಟು_FTask* taskp, ಧ್ರುವ string& newname,
                                         ಧ್ರುವ V3TaskConnects& tconnects,
                                         ಧ್ರುವ VInsertionSet<ಧ್ರುವ AstVar*>& argWrap) {
    std::map<ಧ್ರುವ AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_ಗಂಟು_FTask* ಧ್ರುವ newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->addNextHere(newTaskp);

    Ast_ಗಂಟು_FTask_Ref* newCallp = nullptr;
    Ast_ಗಂಟು* newCallInsertp = nullptr;
    ಯದಿ (VN_IS(taskp, Func)) {
        AstVar* ಧ್ರುವ fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ಧ್ರುವ newFVarp = fvarp->clone_ವೃಕ್ಷ(ಸತ್ಯ);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = ನೂತನ AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = ನೂತನ AstAssign{taskp->fileline(),
                            ನೂತನ AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(taskp, Task)) {
        newCallp = ನೂತನ AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{taskp->fileline(), newCallp};
    } ಅನ್ಯಥಾ {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
        AstVar* ಧ್ರುವ portp = tconnect.first;
        AstVar* newPortp;
        ಯದಿ (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = ನೂತನ AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(ಸತ್ಯ);
            ಯದಿ (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
            newTaskp->addStmtsp(newPortp);
        } ಅನ್ಯಥಾ {  // Defaulting arg
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = VN_AS(portp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಯದಿ ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ಧ್ರುವ VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ಧ್ರುವ refp = VN_AS(valuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), VarRef);
                refp->access(refAccess);
                AstArg* ಧ್ರುವ newArgp = ನೂತನ AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                ಅನುವರ್ತ;
            }
            // Create local temporary
            newPortp = ನೂತನ AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(ಸತ್ಯ);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            ಯದಿ (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* ಧ್ರುವ newAssignp
                    = ನೂತನ AstAssign{valuep->fileline(),
                                    ನೂತನ AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_ವೃಕ್ಷ(ಸತ್ಯ)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ಧ್ರುವ newArgp = ನೂತನ AstArg{
            portp->fileline(), portp->name(),
            ನೂತನ AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ಧ್ರುವ auto it = oldNewVars.find(refp->varp());
        ಯದಿ (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    ಪ್ರತ್ಯಾಗಮ newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, ತಾರ್ಕಿಕ isPtr, ಧ್ರುವ string& frSuffix,
                                   ಧ್ರುವ string& toSuffix, ಧ್ರುವ string& frPrefix) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ಧ್ರುವ string frName = frPrefix + portp->name() + frSuffix;
    ಧ್ರುವ string toName = portp->name() + toSuffix;
    ಧ್ರುವ string idx = portp->name() + "__Vidx";
    size_t unpackSize = 1;  // non-unpacked array is treated as size 1
    ಪೂರ್ಣಾಙ್ಕ unpackDim = 0;
    ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpackSize = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(ಅಸತ್ಯ).second;
        ಯದಿ (unpackDim > 0) UASSERT_OBJ(unpackSize > 0, portp, "size must be greater than 0");
    }
    ಯದಿ (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        ಧ್ರುವ ತಾರ್ಕಿಕ isBit = portp->basicp()->isDpiBitVec();
        ಧ್ರುವ ತಾರ್ಕಿಕ needsFor = unpackSize > 1;
        ಯದಿ (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpackSize) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        ಯದಿ (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_ಪದಗಳು()) + " * " + idx;
        }
        stmt += ", ";
        ಯದಿ (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } ಅನ್ಯಥಾ {
            stmt += frName + ")";
        }
    } ಅನ್ಯಥಾ {
        ಧ್ರುವ ತಾರ್ಕಿಕ isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        ಧ್ರುವ ತಾರ್ಕಿಕ isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        ಧ್ರುವ string unpackAt = unpackSize > 1 ? "[" + idx + "]" : "[0]";
        ಯದಿ (unpackDim > 0) {
            ಧ್ರುವ string forStmt = unpackSize > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpackSize) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } ಅನ್ಯಥಾ {
            ಯದಿ (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        ಯದಿ (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        ಯದಿ (unpackDim > 0) {
            stmt += "(&" + frName;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } ಅನ್ಯಥಾ {
            stmt += frName;
        }
        ಯದಿ (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    ಪ್ರತ್ಯಾಗಮ stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(ಧ್ರುವ std::string& lhsName, AstVar* varp) {
    std::string cvt;
    ತಾರ್ಕಿಕ useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    ಧ್ರುವ std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ total = strides.empty() ? 1 : strides[0].first * strides[0].second;
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width_ಪದಗಳು = varp->basicp()->width_ಪದಗಳು();
    string statements;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < total; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        ಪ್ರತಿ (ಧ್ರುವ auto& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        ಯದಿ (useSvVec) {
            ಧ್ರುವ std::string offset = std::to_string(i * width_ಪದಗಳು);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    ಪ್ರತ್ಯಾಗಮ statements;
}

ಶೂನ್ಯ V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ಧ್ರುವ TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("task", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
