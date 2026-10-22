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

वर्ग TaskBaseVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TaskBaseVertex, V3GraphVertex);
    Ast_गाँठो* m_impurep = nullptr;  // Node causing impure function w/ outside references
    तार्किक m_noInline = असत्य;  // Marked with pragma
सार्वजनिक:
    explicit TaskBaseVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~TaskBaseVertex() override = स्वभाव;
    तार्किक pure() अचर { फर्काऊ m_impurep == nullptr; }
    Ast_गाँठो* impure_गाँठो() अचर { फर्काऊ m_impurep; }
    शून्य impure(Ast_गाँठो* nodep) { m_impurep = nodep; }
    तार्किक noInline() अचर { फर्काऊ m_noInline; }
    शून्य noInline(तार्किक flag) { m_noInline = flag; }
};

वर्ग TaskFTaskVertex final : सार्वजनिक TaskBaseVertex {
    VL_RTTI_IMPL(TaskFTaskVertex, TaskBaseVertex);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_गाँठो_FTask* अचर m_nodep;
    AstCFunc* m_cFuncp = nullptr;
    तार्किक m_needsNonInlineCFunc = असत्य;  // Needs CFunc for selected non-inlined call sites

सार्वजनिक:
    TaskFTaskVertex(V3Graph* graphp, Ast_गाँठो_FTask* nodep)
        : TaskBaseVertex{graphp}
        , m_nodep{nodep} {}
    ~TaskFTaskVertex() override = स्वभाव;
    Ast_गाँठो_FTask* nodep() अचर VL_MT_STABLE { फर्काऊ m_nodep; }
    string name() अचर override VL_MT_STABLE { फर्काऊ nodep()->name(); }
    string dotColor() अचर override { फर्काऊ pure() ? "black" : "red"; }
    AstCFunc* cFuncp() अचर { फर्काऊ m_cFuncp; }
    शून्य cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    तार्किक needsNonInlineCFunc() अचर { फर्काऊ m_needsNonInlineCFunc; }
    शून्य needsNonInlineCFunc(तार्किक flag) { m_needsNonInlineCFunc = flag; }
};

वर्ग TaskCodeVertex final : सार्वजनिक TaskBaseVertex {
    VL_RTTI_IMPL(TaskCodeVertex, TaskBaseVertex);
    // Top vertex for all calls not under another task
सार्वजनिक:
    explicit TaskCodeVertex(V3Graph* graphp)
        : TaskBaseVertex{graphp} {}
    ~TaskCodeVertex() override = स्वभाव;
    string name() अचर override VL_MT_STABLE { फर्काऊ "*CODE*"; }
    string dotColor() अचर override { फर्काऊ "green"; }
};

वर्ग TaskEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(TaskEdge, V3GraphEdge)
सार्वजनिक:
    TaskEdge(V3Graph* graphp, TaskBaseVertex* fromp, TaskBaseVertex* शीर्ष)
        : V3GraphEdge{graphp, fromp, शीर्ष, 1, असत्य} {}
    ~TaskEdge() override = स्वभाव;
    string dotLabel() अचर override { फर्काऊ "w" + cvtToStr(weight()); }
};

//######################################################################

वर्ग TaskStateVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    अचर VNUser3InUse m_inuser3;
    अचर VNUser4InUse m_inuser4;

    // TYPES
    using VarToScopeMap = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    using FuncToClassMap = std::unordered_map<अचर Ast_गाँठो_FTask*, AstClass*>;
    // MEMBERS
    VarToScopeMap m_varToScopeMap;  // Map for Var -> VarScope mappings
    FuncToClassMap m_funcToClassMap;  // Map for ctor func -> class
    Ast_गाँठो_FTask* m_ctorp = nullptr;  // Class constructor
    AstClass* m_classp = nullptr;  // Current class
    V3Graph m_callGraph;  // Task call graph
    TaskBaseVertex* m_curVxp;  // Current vertex we're adding to
    std::vector<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    तार्किक m_underPortVar = असत्य;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

सार्वजनिक:
    // METHODS
    स्थिर तार्किक isVirtualIfaceMethodCall(अचर Ast_गाँठो_FTask_Ref* refp) {
        अचर AstMethodCall* अचर mrefp = VN_CAST(refp, MethodCall);
        यदि (!mrefp) फर्काऊ असत्य;
        अचर Ast_गाँठो_अभिव्यक्ति* अचर fromp = mrefp->fromp();
        यदि (!fromp || !fromp->dtypep()) फर्काऊ असत्य;
        अचर AstIfaceRefDType* अचर ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        फर्काऊ ifrefp && ifrefp->isVirtual();
    }
    स्थिर तार्किक isIfaceFTaskScope(अचर AstScope* scopep) {
        फर्काऊ scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_गाँठो_FTask* nodep) {
        AstScope* अचर scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        फर्काऊ scopep;
    }
    AstVarScope* findVarScope(AstScope* scopep, AstVar* nodep) {
        अचर स्वतः iter = m_varToScopeMap.find(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_varToScopeMap.end(), nodep, "No scope for var");
        फर्काऊ iter->second;
    }
    AstClass* getClassp(Ast_गाँठो_FTask* nodep) {
        AstClass* अचर classp = m_funcToClassMap[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        फर्काऊ classp;
    }
    शून्य remapFuncClassp(Ast_गाँठो_FTask* nodep, Ast_गाँठो_FTask* newp) {
        m_funcToClassMap[newp] = getClassp(nodep);
    }
    तार्किक ftaskNoInline(Ast_गाँठो_FTask* nodep) { फर्काऊ getFTaskVertex(nodep)->noInline(); }
    तार्किक ftaskNeedsNonInlineCFunc(Ast_गाँठो_FTask* nodep) {
        फर्काऊ getFTaskVertex(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_गाँठो_FTask* nodep) { फर्काऊ getFTaskVertex(nodep)->cFuncp(); }
    शून्य ftaskCFuncp(Ast_गाँठो_FTask* nodep, AstCFunc* cfuncp) {
        getFTaskVertex(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_गाँठो* checkImpure(Ast_गाँठो_FTask* nodep) { फर्काऊ getFTaskVertex(nodep)->impure_गाँठो(); }

गुप्त:
    TaskFTaskVertex* getFTaskVertex(Ast_गाँठो_FTask* nodep) {
        यदि (!nodep->user4p()) nodep->user4p(नूतन TaskFTaskVertex{&m_callGraph, nodep});
        फर्काऊ static_cast<TaskFTaskVertex*>(nodep->user4u().toGraphVertex());
    }

    शून्य propagateImpureReason() {
        std::vector<TaskFTaskVertex*> workList;
        // Gather all impure FTask vertices
        लागि (V3GraphVertex& vtx : m_callGraph.vertices()) {
            यदि (TaskFTaskVertex* अचर ftVtxp = vtx.cast<TaskFTaskVertex>()) {
                यदि (!ftVtxp->pure()) workList.emplace_back(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        जबसम्म (!workList.empty()) {
            TaskFTaskVertex* अचर calleep = workList.back();
            workList.झिक_back();
            UASSERT_OBJ(!calleep->pure(), calleep->nodep(), "Should not enqueue pure vertex");
            लागि (V3GraphEdge& edge : calleep->inEdges()) {
                यदि (TaskFTaskVertex* अचर callerp = edge.fromp()->cast<TaskFTaskVertex>()) {
                    यदि (!callerp->pure()) जारी;  // Already processed or on workList
                    callerp->impure(calleep->impure_गाँठो());
                    workList.emplace_back(callerp);
                }
            }
        }
    }

    शून्य decideInlining() {
        लागि (V3GraphVertex& vtx : m_callGraph.vertices()) {
            TaskFTaskVertex* अचर ftVtxp = vtx.cast<TaskFTaskVertex>();
            यदि (!ftVtxp) जारी;
            // Move on if already decided not to inline
            यदि (ftVtxp->noInline()) जारी;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            यदि (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(सत्य);
                जारी;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            यदि (v3Global.opt.fInlineFuncsEager()) जारी;

            Ast_गाँठो_FTask* अचर ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            यदि (!ftaskp->verilogFunction()) जारी;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            यदि (ftaskp->taskPublic()) जारी;
            // If the function is pure, it's OK not to inline, so don't do it
            यदि (ftVtxp->pure()) ftVtxp->noInline(सत्य);
        }
    }

    // VISITORS
    शून्य visit(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        लागि (Ast_गाँठो* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVarScope* अचर vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* अचर varp = vscp->varp();
                यदि (varp->isFuncLocal() || varp->isUsedLoopIdx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_varToScopeMap.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        लागि (Ast_गाँठो* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (Ast_गाँठो_FTask* अचर taskp = VN_CAST(stmtp, गाँठो_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        TaskFTaskVertex* अचर taskVtxp = getFTaskVertex(nodep->taskp());
        नूतन TaskEdge{&m_callGraph, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        यदि (isVirtualIfaceMethodCall(nodep)) taskVtxp->needsNonInlineCFunc(सत्य);
        // Do we have to disable inlining the function?
        अचर V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        यदि (!taskVtxp->noInline()) {  // Else short-circuit below
            लागि (अचर स्वतः& itr : tconnects) {
                अचर AstVar* अचर portp = itr.first;
                अचर AstArg* अचर argp = itr.second;
                यदि (अचर Ast_गाँठो_अभिव्यक्ति* अचर pinp = argp->exprp()) {
                    यदि ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(सत्य);
                    }
                }
            }
        }
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = getFTaskVertex(nodep);
        यदि (nodep->dpiImport()) m_curVxp->noInline(सत्य);
        यदि (nodep->classMethod()) m_curVxp->noInline(सत्य);  // Until V3Task supports it
        यदि (nodep->recursive()) m_curVxp->noInline(सत्य);
        यदि (nodep->isConstructor()) {
            m_curVxp->noInline(सत्य);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_funcToClassMap[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstPragma* nodep) override {
        यदि (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(सत्य);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstVar* nodep) override {
        VL_RESTORER(m_underPortVar);
        यदि (nodep->isIO()) m_underPortVar = सत्य;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    शून्य visit(AstVarRef* nodep) override {
        iterateChildren(nodep);
        यदि (m_underPortVar) फर्काऊ;
        AstVar* अचर varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        यदि (nodep->access().isReadOnly() && varp->constPoolEntry()) फर्काऊ;
        यदि (varp->user4u().toGraphVertex() != m_curVxp) {
            यदि (m_curVxp->pure() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    शून्य visit(AstClass* nodep) override {
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
        Ast_गाँठो* insertp = nullptr;
        लागि (AstInitialAutomatic* initialp : m_initialps) {
            यदि (Ast_गाँठो* अचर movep = initialp->stmtsp()) {
                movep->unlinkFrBackWithNext();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_गाँठो::addNextNull(insertp, movep);
            }
            VL_DO_DANGLING(थप_Deletep(initialp->unlinkFrBack()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    शून्य visit(AstInitialAutomatic* nodep) override {
        m_initialps.थप_back(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = नूतन TaskCodeVertex{&m_callGraph};
        //
        iterate(nodep);
        //
        m_callGraph.removeRedundantEdgesSum(&TaskEdge::followAlwaysTrue);
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
    स्थिर std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> unpackDimsAndStrides(AstVar* varp) {
        Ast_गाँठो_DType* अचर dtypep = varp->dtypep()->skipRefp();
        std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> dimStrides;
        AstUnpackArrayDType* अचर unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        यदि (!unpackp) फर्काऊ dimStrides;

        अचर std::vector<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        अचर माप_प्रकार nDims = dims.size();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].first = dims.back()->elementsConst();
        dimStrides[nDims - 1].second = 1;
        // Rest are densly packed
        लागि (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].first = dims[i]->elementsConst();
            dimStrides[i].second = dimStrides[i + 1].first * dimStrides[i + 1].second;
        }
        फर्काऊ dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    स्थिर std::pair<std::string, तार्किक> dpiToInternalCvtStmt(अचर Ast_गाँठो_DType* dtypep) {
        अचर AstBasicDType* अचर basicp = dtypep->basicp();

        // DPI types using Primitive C types
        यदि (basicp->keyword() == VBasicDTypeKwd::CHANDLE) फर्काऊ {"VL_CVT_VP_Q(", असत्य};
        यदि (basicp->keyword() == VBasicDTypeKwd::STRING) फर्काऊ {"VL_CVT_N_CSTR(", असत्य};
        यदि (basicp->isDpiPrimitive()) फर्काऊ {"(", असत्य};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->isDpiBitVec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        अचर std::string vecType = basicp->isDpiBitVec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        अचर वर्ण sizeChar = dtypep->width() <= 8    ? 'C'
                              : dtypep->width() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        अचर std::string& size = std::to_string(dtypep->width());
        // cppcheck-suppress strPlusChar
        फर्काऊ {"VL_SET_"s + sizeChar + "_" + vecType + "(" + size + ", ", सत्य};
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
    अचर VNUser1InUse m_inuser1;
    अचर VNUser2InUse m_inuser2;

    // TYPES
    using DpiCFuncs = std::map<अचर string, std::tuple<Ast_गाँठो_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* अचर m_statep;  // Common state between visitors
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    Ast_शीर्ष_Scope* अचर m_शीर्ष_Scopep = v3Global.rootp()->शीर्ष_Scopep();  // The AstTopScope
    AstScope* m_scopep = nullptr;  // Current scope
    Ast_गाँठो* m_insStmtp = nullptr;  // Where to insert statement
    तार्किक m_inSensesp = असत्य;  // Are we under a senitem?
    तार्किक m_inNew = असत्य;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    पूर्णाङ्क m_uncon_Var_अङ्क = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpiNames;  // Map of all created DPI functions
    std::map<std::string, अचर AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, अचर string& name, AstVar* examplep) {
        AstVar* अचर newvarp = नूतन AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(सत्य);
        funcp->addVarsp(newvarp);
        AstVarScope* अचर newvscp = नूतन AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        फर्काऊ newvscp;
    }
    AstVarScope* create_आगत_Var(AstCFunc* funcp, अचर string& name, VBasicDTypeKwd kwd) {
        AstVar* अचर newvarp
            = नूतन AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->findBasicDType(kwd)};
        newvarp->funcLocal(सत्य);
        newvarp->direction(VDirection::आगत);
        funcp->addArgsp(newvarp);
        AstVarScope* अचर newvscp = नूतन AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->addVarsp(newvscp);
        फर्काऊ newvscp;
    }
    स्थिर AstVarScope* constPoolTable(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* अचर refp = V3ConstPool::find(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* अचर vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_रूख(), refp);
        फर्काऊ vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(अचर Ast_गाँठो_FTask_Ref* refp, AstVar* invarp,
                                अचर std::string& suffix) {
        यदि (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            फर्काऊ constPoolTable(invarp);
        } अन्यथा {
            // We could create under either the ref's scope or the ftask's scope.
            // It shouldn't matter, as they are only local variables.
            // We choose to do it under whichever called this function, which results
            // in more cache locality.
            // Share the variable with the other scopes of the module, so the same calls in
            // instances use the same variables
            V3SharedTmps& tmps = VN_IS(refp, FuncRef) ? m_funcTmps : m_taskTmps;
            AstVarScope* अचर newvscp = tmps.make(invarp->fileline(), m_scopep, invarp->dtypep(),
                                                   refp->taskp()->shortName() + suffix);
            AstVar* अचर newvarp = newvscp->varp();
            newvarp->funcLocal(असत्य);
            newvarp->propagateAttrFrom(invarp);
            newvarp->isInternal(सत्य);
            फर्काऊ newvscp;
        }
    }

    // Replace varrefs with new var pointer
    शून्य relink(Ast_गाँठो* nodep) {
        nodep->foreachAndNext([](AstVarRef* refp) {
            यदि (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* अचर newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    AstAssign* connectPortMakeInAssign(Ast_गाँठो_अभिव्यक्ति* pinp, AstVarScope* newvscp, तार्किक pureCheck) {
        // Create input assignment to go in FRONT of function call
        Ast_गाँठो_अभिव्यक्ति* inPinp = pinp;
        यदि (AstResizeLValue* sinPinp = VN_CAST(inPinp, ResizeLValue)) inPinp = sinPinp->lhsp();
        Ast_गाँठो_अभिव्यक्ति* inPinClonep
            = pureCheck ? inPinp->clone_रूख_Pure(सत्य) : inPinp->clone_रूख(सत्य);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        अचर AstUnpackArrayDType* अचर arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        यदि (arrayp && arrayp->isReverseOf(inPinClonep->dtypep())) {
            inPinClonep
                = नूतन AstCvtUnpackToUnpack{pinp->fileline(), inPinClonep, newvscp->dtypep()};
        }
        AstAssign* अचर assp = नूतन AstAssign{
            pinp->fileline(), नूतन AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            inPinClonep};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, सत्य);  // Ok if in <= block
        फर्काऊ assp;
    }
    AstAssign* connectPortMakeOutAssign(AstVar* portp, Ast_गाँठो_अभिव्यक्ति* pinp, AstVarScope* newvscp,
                                        तार्किक pureCheck) {
        // If needed, remap size of function to caller's output size
        Ast_गाँठो_अभिव्यक्ति* outPinp = pinp;
        Ast_गाँठो_अभिव्यक्ति* postRhsp = नूतन AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        यदि (AstResizeLValue* soutPinp = VN_CAST(outPinp, ResizeLValue)) {
            outPinp = soutPinp->lhsp();
            यदि (VN_IS(outPinp, RToIRoundS) || VN_IS(outPinp, RToIS)) {
                outPinp = VN_AS(outPinp, गाँठो_Uniop)->lhsp();
                यदि (postRhsp->isSigned()) {
                    postRhsp = नूतन AstISToRD{pinp->fileline(), postRhsp};
                } अन्यथा {
                    postRhsp = नूतन AstIToRD{pinp->fileline(), postRhsp};
                }
            } अन्यथा {
                यदि (Ast_गाँठो_Uniop* aoutPinp = VN_CAST(outPinp, Extend)) {
                    outPinp = aoutPinp->lhsp();
                } अन्यथा यदि (Ast_गाँठो_Uniop* aoutPinp = VN_CAST(outPinp, ExtendS)) {
                    outPinp = aoutPinp->lhsp();
                } अन्यथा यदि (AstSel* aoutPinp = VN_CAST(outPinp, Sel)) {
                    outPinp = aoutPinp->fromp();
                } अन्यथा {
                    outPinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                यदि (outPinp->width() < portp->width()) {
                    postRhsp = नूतन AstSel{pinp->fileline(), postRhsp, 0, pinp->width()};
                } अन्यथा {  // pin width > port width
                    यदि (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = नूतन AstExtendS{pinp->fileline(), postRhsp};
                    } अन्यथा {
                        postRhsp = नूतन AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(outPinp);
        }
        यदि (VN_IS(outPinp, IToRD) || VN_IS(outPinp, ISToRD)) {
            outPinp = VN_AS(outPinp, गाँठो_Uniop)->lhsp();
            postRhsp = नूतन AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(outPinp);
        }
        // Put output assignment AFTER function call
        Ast_गाँठो_अभिव्यक्ति* अचर outPinClonep
            = pureCheck ? outPinp->clone_रूख_Pure(सत्य) : outPinp->clone_रूख(सत्य);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        अचर AstUnpackArrayDType* अचर arrayp
            = VN_CAST(outPinp->dtypep()->skipRefp(), UnpackArrayDType);
        यदि (arrayp && arrayp->isReverseOf(postRhsp->dtypep())) {
            postRhsp = नूतन AstCvtUnpackToUnpack{pinp->fileline(), postRhsp, outPinp->dtypep()};
        }
        AstAssign* अचर assp = नूतन AstAssign{pinp->fileline(), outPinClonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, सत्य);  // Ok if in <= block
        फर्काऊ assp;
    }

    शून्य changeAtWriteRecurse(Ast_गाँठो_अभिव्यक्ति* अचर exprp) {
        // Change nested at methods to writable variant
        यदि (AstCMethodHard* अचर cMethodp = VN_CAST(exprp, CMethodHard)) {
            यदि (cMethodp->method() == VCMethod::ARRAY_AT) {
                cMethodp->method(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    शून्य connectPort(अचर Ast_गाँठो_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_गाँठो* beginp,
                     तार्किक inlineTask) {
        Ast_गाँठो_अभिव्यक्ति* pinp = argp->exprp();
        यदि (inlineTask) {
            portp->unlinkFrBack();
            थप_Deletep(portp);  // Remove it from the clone (not original)
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
                AstVar* अचर varp = नूतन AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::to_string(m_uncon_Var_अङ्क++),
                                                portp->dtypep()};
                m_modp->addStmtsp(varp);
                AstVarScope* अचर newvscp = नूतन AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->addVarsp(newvscp);
                AstVarRef* अचर repp = नूतन AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(थप_Deletep(pinp), pinp);
                pinp = repp;
            }
            यदि (inlineTask) {
                थप_Deletep(pinp->unlinkFrBack());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_रूख(), argp);  // Args no longer needed
            }
            यदि (portp->isRef() || portp->isConstRef()) {
                तार्किक refArgOk = असत्य;
                यदि (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = सत्य;
                } अन्यथा यदि (AstCMethodHard* अचर cMethodp = VN_CAST(pinp, CMethodHard)) {
                    यदि (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), QueueDType)) {
                        refArgOk = cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->method() == VCMethod::DYN_AT_WRITE_APPEND_BACK;
                    } अन्यथा {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->method() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                यदि (refArgOk) {
                    यदि (AstVarRef* अचर varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } अन्यथा {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                यदि (inlineTask) {
                    यदि (AstVarRef* अचर varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* अचर localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } अन्यथा {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } अन्यथा यदि (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* अचर newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                यदि (!inlineTask) {
                    pinp->replaceWith(
                        नूतन AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    थप_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                AstAssign* अचर preassp = connectPortMakeInAssign(pinp, newvscp, सत्य);
                beginp->addNextHere(preassp);

                AstAssign* अचर postassp = connectPortMakeOutAssign(portp, pinp, newvscp, सत्य);
                beginp->addNext(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } अन्यथा यदि (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* अचर newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                यदि (!inlineTask) {
                    pinp->replaceWith(नूतन AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    थप_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                AstAssign* अचर postassp = connectPortMakeOutAssign(portp, pinp, newvscp, असत्य);
                // Put assignment BEHIND of all other statements
                beginp->addNext(postassp);
            } अन्यथा यदि (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* अचर newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                AstAssign* अचर preassp = connectPortMakeInAssign(pinp, newvscp, असत्य);
                // Put assignment in FRONT of all other statements
                beginp->addNextHere(preassp);
            }
        }
    }

    तार्किक hasRefArgument(Ast_गाँठो_FTask* nodep) {
        लागि (Ast_गाँठो* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (varp->isRef() || varp->isConstRef()) फर्काऊ सत्य;
            }
        }
        फर्काऊ असत्य;
    }

    Ast_गाँठो* createInlinedFTask(Ast_गाँठो_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_गाँठो* अचर newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_रूख(सत्य) : nullptr;
        Ast_गाँठो* अचर beginp
            = नूतन AstComment{refp->fileline(), "Function: "s + refp->name(), सत्य};
        यदि (newbodysp) beginp->addNext(newbodysp);
        यदि (debug() >= 9) beginp->dump_रूख_And_Next(cout, "-  newbegi: ");
        //
        // Create input variables
        Ast_गाँठो::user2_Clear_रूख();
        {
            अचर V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            लागि (अचर स्वतः& itr : tconnects) {
                AstVar* अचर portp = itr.first;
                AstArg* अचर argp = itr.second;
                connectPort(refp, portp, argp, beginp, सत्य);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_गाँठो* nextstmtp;
            लागि (Ast_गाँठो* stmtp = beginp; stmtp; stmtp = nextstmtp) {
                nextstmtp = stmtp->nextp();
                यदि (AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                    // Any I/O variables that fell out of above loop were already linked
                    यदि (!portp->user2p()) {
                        // Move it to a new localized variable
                        AstVarScope* अचर localVscp
                            = createVarScope(refp, portp, "__" + portp->shortName());
                        portp->user2p(localVscp);
                        यदि (portp->needsCReset() && portp->lifetime().isAutomatic()
                            && !portp->valuep()) {
                            // Reset automatic var to its default, on each invocation of function
                            Ast_गाँठो* अचर crstp = नूतन AstAssign{
                                portp->fileline(),
                                नूतन AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                नूतन AstCReset{portp->fileline(), portp, असत्य}};
                            portp->replaceWith(crstp);
                        } अन्यथा {
                            portp->unlinkFrBack();
                        }
                        थप_Deletep(portp);  // Remove it from the clone (not original)
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
        यदि (debug() >= 9) beginp->dump_रूख_And_Next(cout, "-  iotask: ");
        फर्काऊ beginp;
    }

    Ast_गाँठो* createNonInlinedFTask(Ast_गाँठो_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* अचर cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_गाँठो* अचर beginp
            = नूतन AstComment{refp->fileline(), "Function: "s + refp->name(), सत्य};
        Ast_गाँठो_CCall* ccallp;
        यदि (VN_IS(refp, New)) {
            AstCNew* अचर cnewp = नूतन AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } अन्यथा यदि (अचर AstMethodCall* अचर mrefp = VN_CAST(refp, MethodCall)) {
            ccallp = नूतन AstCMethodCall{refp->fileline(), mrefp->fromp()->unlinkFrBack(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        } अन्यथा {
            ccallp = नूतन AstCCall{refp->fileline(), cfuncp};
            ccallp->dtypeSetVoid();
            beginp->addNext(ccallp->makeStmt());
        }

        यदि (अचर AstFuncRef* अचर funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } अन्यथा यदि (अचर AstTaskRef* अचर taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } अन्यथा यदि (अचर AstMethodCall* अचर methodCallp = VN_CAST(refp, MethodCall)) {
            ccallp->superReference(methodCallp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            अचर V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            लागि (अचर स्वतः& itr : tconnects) {
                AstVar* अचर portp = itr.first;
                AstArg* अचर argp = itr.second;
                connectPort(refp, portp, argp, beginp, असत्य);
            }
        }
        // First argument is symbol table, then output if a function
        अचर तार्किक needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        यदि (needSyms) ccallp->argTypes("vlSymsp");

        यदि (refp->taskp()->dpiContext()) {
            AstScopeName* अचर snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlinkFrBack();
            File_पङ्क्ति* अचर flp = refp->fileline();
            // __Vscopep
            ccallp->addArgsp(snp);
            // __Vfilenamep
            ccallp->addArgsp(
                नूतन AstCExpr{flp, AstCExpr::Pure{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->addArgsp(नूतन AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        लागि (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->addArgsp(argp->exprp()->unlinkFrBack());
        }

        यदि (outvscp) ccallp->addArgsp(नूतन AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        यदि (debug() >= 9) beginp->dump_रूख_And_Next(cout, "-  nitask: ");
        फर्काऊ beginp;
    }

    string dpiSignature(Ast_गाँठो_FTask* nodep, AstVar* rtnvarp) अचर {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        यदि (nodep->dpiPure()) dpiproto += "pure ";
        यदि (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(सत्य, सत्य) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string args;
        लागि (Ast_गाँठो* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (अचर AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    यदि (args != "") {
                        args += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    अचर std::string dpiType = portp->dpiArgType(असत्य, असत्य);
                    dpiproto += dpiType;
                    अचर std::string vType = portp->dtypep()->containsUnpackedStruct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(असत्य);
                    यदि (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    args += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        फर्काऊ dpiproto;
    }

    स्थिर शून्य checkLegalCIdentifier(Ast_गाँठो* nodep, अचर string& name) {
        यदि (name.end() != std::find_if(name.begin(), name.end(), [](वर्ण c) {
                फर्काऊ !std::है_अक्षर_अंक(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    स्थिर Ast_गाँठो* createDpiTemp(AstVar* portp, अचर string& suffix) {
        अचर string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        फर्काऊ नूतन AstCStmt{portp->fileline(), stmt};
    }

    शून्य unlinkAndClone(Ast_गाँठो_FTask* funcp, Ast_गाँठो* nodep, तार्किक withNext) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        यदि (funcp->recursive()) {
            VNRelinker relinkHandle;
            यदि (withNext) {
                nodep->unlinkFrBackWithNext(&relinkHandle);
            } अन्यथा {
                nodep->unlinkFrBack(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_गाँठो* अचर newp = nodep->clone_रूख(withNext);
            relinkHandle.relink(newp);
        } अन्यथा {
            यदि (withNext) {
                nodep->unlinkFrBackWithNext();
            } अन्यथा {
                nodep->unlinkFrBack();
            }
        }
    }

    स्थिर Ast_गाँठो* createAssignInternalToDpi(AstVar* portp, तार्किक isPtr, अचर string& frSuffix,
                                              अचर string& toSuffix) {
        अचर string stmt = V3Task::assignInternalToDpi(portp, isPtr, frSuffix, toSuffix);
        फर्काऊ नूतन AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    स्थिर Ast_गाँठो_Stmt* createDpiScalarCvt(Ast_गाँठो_अभिव्यक्ति* internalp, अचर std::string& dpi,
                                           अचर Ast_गाँठो_DType* dtypep, तार्किक toDpi) {
        File_पङ्क्ति* अचर flp = internalp->fileline();
        अचर AstBasicDType* अचर basicp = dtypep->basicp();
        यदि (toDpi) {
            AstCStmt* अचर cstmtp = नूतन AstCStmt{flp};
            यदि (basicp->isDpiPrimitive()) {
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
            फर्काऊ cstmtp;
        }
        std::string cvt;
        तार्किक useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(dtypep);
        यदि (useSvVec) {
            AstCStmt* अचर cstmtp = नूतन AstCStmt{flp};
            cstmtp->add(cvt);
            cstmtp->add(internalp);
            cstmtp->add(", " + dpi + ");");
            फर्काऊ cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        अचर पूर्णाङ्क cwidth = basicp->width_शब्दहरू() * VL_EDATASIZE;
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstCExpr{flp, cvt + dpi + ")", cwidth};
        फर्काऊ नूतन AstAssign{flp, internalp, नूतन AstSel{flp, rhsp, 0, dtypep->width()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    स्थिर Ast_गाँठो_Stmt* createDpiCvt(Ast_गाँठो_अभिव्यक्ति* internalp, अचर std::string& dpi,
                                     अचर Ast_गाँठो_DType* dtypep, तार्किक toDpi) {
        File_पङ्क्ति* अचर flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_गाँठो_Stmt* stmtsp = nullptr;
        यदि (अचर AstUnpackArrayDType* अचर arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            लागि (पूर्णाङ्क i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_गाँठो_अभिव्यक्ति* अचर elemp = नूतन AstArraySel{flp, internalp->clone_रूख(असत्य), i};
                stmtsp = Ast_गाँठो::addNext(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::to_string(i) + "]",
                                                       arrayp->subDTypep(), toDpi));
            }
        } अन्यथा यदि (dtypep->containsUnpackedStruct()) {
            लागि (अचर AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* अचर selp
                    = नूतन AstStructSel{flp, internalp->clone_रूख(असत्य), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_गाँठो::addNext(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), toDpi));
            }
        } अन्यथा {
            फर्काऊ createDpiScalarCvt(internalp, dpi, dtypep, toDpi);
        }
        VL_DO_DANGLING(internalp->delete_रूख(), internalp);
        फर्काऊ stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    स्थिर Ast_गाँठो_Stmt* createDpiArgCvt(AstVarScope* vscp, अचर std::string& dpiName,
                                        तार्किक toDpi) {
        AstVar* अचर varp = vscp->varp();
        File_पङ्क्ति* अचर flp = vscp->fileline();
        अचर std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        अचर पूर्णाङ्क total = strides.empty() ? 1 : strides[0].first * strides[0].second;
        अचर तार्किक isStruct = varp->dtypep()->containsUnpackedStruct();
        अचर Ast_गाँठो_DType* अचर elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_गाँठो_Stmt* stmtsp = nullptr;
        लागि (पूर्णाङ्क i = 0; i < total; ++i) {
            Ast_गाँठो_अभिव्यक्ति* internalp
                = नूतन AstVarRef{flp, vscp, toDpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            लागि (अचर स्वतः& stride : strides) {
                internalp = नूतन AstArraySel(flp, internalp, (i / stride.second) % stride.first);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            यदि (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::to_string(i * varp->basicp()->width_शब्दहरू());
            } अन्यथा यदि (isStruct || !strides.empty()) {
                dpi += "[" + std::to_string(i) + "]";
            }
            stmtsp = Ast_गाँठो::addNext(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, toDpi));
        }
        फर्काऊ stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_गाँठो_FTask* अचर nodep, AstVar* अचर rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_पङ्क्ति* अचर flp = nodep->fileline();
        अचर std::string cname = Ast_गाँठो::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        अचर वर्ण* अचर tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        अचर std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(सत्य, सत्य) : "";
        // The function we are building
        AstCFunc* अचर funcp = नूतन AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(सत्य);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(सत्य);
        funcp->entryPoint(सत्य);
        funcp->isStatic(सत्य);
        funcp->protect(असत्य);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_शीर्ष_Scopep->scopep()->addBlocksp(funcp);
        // Check the arguments before converting them
        यदि (!makePortList(nodep, funcp)) फर्काऊ nullptr;

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
        AstCStmt* अचर prep = नूतन AstCStmt{flp};
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
        अचर std::string cbtype
            = VIdProtect::protect(v3Global.opt.prefix() + "__Vcb_" + nodep->cname() + "_t");
        prep->add(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* अचर callp = नूतन AstCStmt{flp};
        अचर स्वतः addFuncArg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* अचर vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(असत्य);
            portp->protect(असत्य);
            // Add argument to call
            अचर VAccess access = portp->direction().pinAccess();
            callp->add(", ");
            callp->add(नूतन AstVarRef{portp->fileline(), vscp, access});
            फर्काऊ vscp;
        };
        // Call callback
        callp->add("(*__Vcb)(");
        // First argument is the Syms
        callp->add("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_गाँठो* outCvtsp = nullptr;  // Conversions of output/inout arguments, after the call
        लागि (Ast_गाँठो* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* अचर portp = VN_CAST(stmtp, Var);
            यदि (!portp) जारी;
            यदि (!portp->isIO()) जारी;
            यदि (portp->isFuncReturn()) जारी;
            यदि (portp == rtnvarp) जारी;  // Handled below
            // Add argument to call
            AstVarScope* अचर outvscp = addFuncArg(portp);
            अचर तार्किक isStruct = portp->dtypep()->containsUnpackedStruct();
            यदि (portp->isWritable()) {
                outCvtsp = Ast_गाँठो::addNext(
                    outCvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), सत्य)
                                       : createAssignInternalToDpi(portp, सत्य, tmpSuffixp, ""));
            }
            यदि (!portp->isNonOutput()) जारी;
            // Convert input/inout arguments to dpi type
            अचर std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(असत्य).second == 0
                      ? "*"
                      : "";
            funcp->addStmtsp(createDpiArgCvt(outvscp, deref + portp->name(), असत्य));
        }
        // Return value argument goes last
        यदि (rtnvarp) addFuncArg(rtnvarp);
        // Close statement
        callp->add(");");
        // Call the user function
        funcp->addStmtsp(callp);
        // Convert output/inout arguments back to internal type
        यदि (outCvtsp) funcp->addStmtsp(outCvtsp);
        // Convert return value
        यदि (rtnvarp) {
            funcp->addStmtsp(createDpiTemp(rtnvarp, ""));
            funcp->addStmtsp(createAssignInternalToDpi(rtnvarp, असत्य, tmpSuffixp, ""));
            अचर std::string index = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->addStmtsp(नूतन AstCStmt{flp, "return " + rtnvarp->name() + index + ";"});
        }
        फर्काऊ funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_गाँठो_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        अचर string name = Ast_गाँठो::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        अचर string rtnType = rtnvarp            ? rtnvarp->dpiArgType(सत्य, सत्य)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* अचर funcp = नूतन AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(सत्य);
        funcp->dontCombine(सत्य);
        funcp->entryPoint(असत्य);
        funcp->isMethod(असत्य);
        funcp->protect(असत्य);
        funcp->dpiPure(nodep->dpiPure());

        अचर पूर्णाङ्क cost = static_cast<पूर्णाङ्क>(V3Control::getProfileData(funcp->name()));
        m_statHierDpisWithCosts += (cost != 0);
        funcp->cost(cost);

        // Add DPI Import to top, since it's a global function
        m_शीर्ष_Scopep->scopep()->addBlocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        यदि (!makePortList(nodep, funcp)) फर्काऊ nullptr;
        फर्काऊ funcp;
    }

    AstCFunc* getDpiFunc(Ast_गाँठो_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        अचर string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        अचर स्वतः pair = m_dpiNames.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, nullptr));
        यदि (pair.second) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* अचर funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.first->second) = funcp;
            फर्काऊ funcp;
        } अन्यथा {
            // Seen this cname import before. Check if it's the same prototype.
            अचर Ast_गाँठो_FTask* firstNodep;
            string firstSignature;
            AstCFunc* firstFuncp;
            std::tie(firstNodep, firstSignature, firstFuncp) = pair.first->second;
            यदि (signature != firstSignature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warnMore()  //
                               << "... New signature:      " << signature << '\n'  //
                               << firstNodep->warnOther()
                               << "... Original signature: " << firstSignature << '\n'  //
                               << firstNodep->warnContextSecondary());
                फर्काऊ nullptr;
            } अन्यथा {
                // Same signature, return the previously created CFunc
                फर्काऊ firstFuncp;
            }
        }
    }

    स्थिर तार्किक makePortList(Ast_गाँठो_FTask* nodep, AstCFunc* dpip) {
        तार्किक allOk = सत्य;
        // Copy nodep's list of function I/O to the new dpip c function
        लागि (Ast_गाँठो* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isIO()) {
                    // Move it to new function
                    AstVar* अचर newPortp = portp->clone_रूख(असत्य);
                    newPortp->funcLocal(सत्य);
                    dpip->addArgsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    यदि (!portp->basicp() && !portp->dtypep()->containsUnpackedStruct()) {
                        allOk = असत्य;
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
        फर्काऊ allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    स्थिर तार्किक isDpiCIdentifier(अचर string& name) {
        फर्काऊ !name.empty() && VString::isIdentifier(name)
               && !std::है_अंक(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::है_बड़ा(name[1])))
               && V3_Language_शब्दहरू::isKeyword(name).empty();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    तार्किक checkDpiStruct(अचर AstVar* portp, अचर AstStructDType* sdtypep, तार्किक nested) {
        अचर std::string name = sdtypep->dpiName();
        यदि (!(nested && name.empty()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(सत्य) << "\n"
                              << portp->warnMore()
                              << "... Suggest use a typedef with a C identifier name");
            फर्काऊ असत्य;
        }
        लागि (अचर AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            यदि (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                फर्काऊ असत्य;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            अचर Ast_गाँठो_DType* dtypep = itemp->subDTypep()->skipRefp();
            जबसम्म (अचर AstUnpackArrayDType* अचर arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            अचर Ast_गाँठो_UOr_Struct_DType* अचर subp = VN_CAST(dtypep, गाँठो_UOr_Struct_DType);
            अचर AstBasicDType* अचर basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            अचर तार्किक isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            यदि (dtypep->containsUnpackedStruct()) {
                यदि (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), सत्य)) फर्काऊ असत्य;
            } अन्यथा यदि (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(सत्य)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                फर्काऊ असत्य;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        यदि (name.empty()) फर्काऊ सत्य;
        अचर AstStructDType* अचर firstp = m_dpiStructs.emplace(name, sdtypep).first->second;
        यदि (firstp->dpiTypedef() == sdtypep->dpiTypedef()) फर्काऊ सत्य;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        फर्काऊ असत्य;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    तार्किक checkDpiStructArgs(अचर Ast_गाँठो_FTask* nodep) {
        तार्किक allOk = सत्य;
        लागि (अचर Ast_गाँठो* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अचर AstVar* अचर portp = VN_CAST(stmtp, Var);
            यदि (portp && portp->isIO() && portp->dtypep()->containsUnpackedStruct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       असत्य)
                        && allOk;
            }
        }
        फर्काऊ allOk;
    }

    शून्य bodyDpiImportFunc(Ast_गाँठो_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        अचर वर्ण* अचर tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        यदि (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_थप(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string args;
        लागि (Ast_गाँठो* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                AstVarScope* अचर portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                यदि (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    यदि (args != "") args += ", ";

                    यदि (portp->isDpiOpenArray()) {
                        Ast_गाँठो_DType* अचर dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, QueueDType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        अचर string propName = portp->name() + "__Vopenprops";
                        अचर string propCode = portp->vlPropDecl(propName);
                        cfuncp->addStmtsp(नूतन AstCStmt{portp->fileline(), propCode});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        अचर string name = portp->name() + "__Vopenarray";
                        अचर string varCode
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->addStmtsp(नूतन AstCStmt{portp->fileline(), varCode});
                        args += "&" + name;
                    } अन्यथा {
                        // Unpacked structures are passed by reference from a C array
                        अचर तार्किक isStruct = portp->dtypep()->containsUnpackedStruct();
                        यदि (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            यदि (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) args += "&";
                        }

                        args += portp->name() + tmpSuffixp;

                        cfuncp->addStmtsp(createDpiTemp(portp, tmpSuffixp));
                        यदि (portp->isNonOutput()) {
                            cfuncp->addStmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, सत्य)
                                    : createAssignInternalToDpi(portp, असत्य, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        यदि (nodep->dpiContext()) {
            अचर string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->addStmtsp(नूतन AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* अचर callp = नूतन AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtypeSetVoid();
            callp->argTypes(args);
            यदि (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->addStmtsp(createDpiTemp(rtnvscp->varp(), tmpSuffixp));
                अचर std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* अचर cstmtp = नूतन AstCStmt{nodep->fileline()};
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
        लागि (Ast_गाँठो* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                portp->protect(असत्य);  // No additional exposure - already part of shown proto
                यदि (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* अचर portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->addStmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, असत्य));
                }
            }
        }

        यदि (v3Global.opt.profExec()) {
            cfuncp->addStmtsp(AstCStmt::prof_Exec_Section_झिक(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* अचर netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        यदि (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_शीर्ष_Scopep->createTemp("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        फर्काऊ dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_गाँठो_FTask* nodep, तार्किक ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_गाँठो::user2_Clear_रूख();
        AstVar* rtnvarp = nullptr;
        यदि (nodep->isFunction()) {
            AstVar* अचर portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            यदि (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* अचर bdtypep = portp->dtypep()->basicp();
                यदि (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(सत्य)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(थप_Deletep(nodep), nodep);
                    फर्काऊ nullptr;
                }
                यदि (!bdtypep->isDpiPrimitive()) {
                    यदि (bdtypep->isDpiBitVec() && portp->width() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warnMore()
                                       + "... Suggest make it an output argument instead?");
                    }
                    यदि (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } अन्यथा यदि (nodep->taskPublic()) {
                यदि (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warnMore()
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
        AstCFunc* dpiFuncp = nullptr;
        यदि (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpiOpenParent() && nodep->dpiOpenChild()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = checkDpiStructArgs(nodep) ? getDpiFunc(nodep, rtnvarp) : nullptr;
            यदि (!dpiFuncp || (nodep->dpiImport() && nodep->dpiOpenParent())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                यदि (rtnvarp) VL_DO_DANGLING(थप_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(थप_Deletep(nodep), nodep);
                फर्काऊ nullptr;
            }
        }

        AstVarScope* rtnvscp = nullptr;
        यदि (rtnvarp) {
            rtnvscp = नूतन AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->addVarsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string prefix;
        यदि (nodep->dpiImport()) {
            prefix = "__Vdpiimwrap_";
        } अन्यथा यदि (nodep->dpiExport()) {
            prefix = "__Vdpiexp_";
        } अन्यथा यदि (ftaskNoInline) {
            prefix = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        यदि (!nodep->taskPublic() && !nodep->classMethod()) suffix = "_" + m_scopep->nameDotless();
        अचर string name = ((nodep->name() == "new") ? "new" : prefix + nodep->name() + suffix);
        AstCFunc* अचर cfuncp = नूतन AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->taskPublic() && rtnvarp) ? rtnvarp->cPubArgType(सत्य, सत्य) : "")};
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
        यदि (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(सत्य);
            cfuncp->isLoose(सत्य);
        } अन्यथा {
            cfuncp->isStatic(असत्य);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpiPure(nodep->dpiPure());
        यदि (nodep->name() == "new") cfuncp->isConstructor(सत्य);
        यदि (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(सत्य);
        यदि (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        यदि (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        अचर तार्किक needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->taskPublic()) || v3Global.opt.profExec());
        यदि (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        यदि (!nodep->dpiImport() && !nodep->taskPublic()) {
            // Need symbol table
            यदि (cfuncp->isConstructor()) {
                तार्किक isInterfaceClass = असत्य;
                यदि (अचर AstClass* अचर classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                यदि (!isInterfaceClass) {
                    अचर string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->addStmtsp(
                        नूतन AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        यदि (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_आगत_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_आगत_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_आगत_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        यदि (nodep->dpiExport()) {
            AstScopeName* अचर snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(सत्य);
            snp->unlinkFrBack();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        लागि (Ast_गाँठो *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            यदि (AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlinkFrBack();
                    थप_Deletep(portp);
                    portp->user2p(constPoolTable(portp));
                } अन्यथा {
                    यदि (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, असत्य);
                        portp->funcLocal(सत्य);
                        यदि (portp->valuep()) थप_Deletep(portp->valuep()->unlinkFrBack());
                        cfuncp->addArgsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        यदि (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_गाँठो_DType* अचर dtypep = portp->dtypep()->skipRefp();
                            यदि (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } अन्यथा {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(सत्य);
                    }
                    AstVarScope* अचर newvscp
                        = नूतन AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->addVarsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        यदि (rtnvarp) cfuncp->addArgsp(rtnvarp);

        // Move body
        Ast_गाँठो* bodysp = nodep->stmtsp();
        यदि (bodysp) {
            unlinkAndClone(nodep, bodysp, सत्य);
            AstBegin* अचर tempp
                = नूतन AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, असत्य};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            लागि (Ast_गाँठो *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                यदि (AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                    यदि (portp->isIO()) VL_DO_DANGLING(थप_Deletep(portp->unlinkFrBack()), portp);
                }
            }
            यदि (tempp->stmtsp()) cfuncp->addStmtsp(tempp->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(tempp->delete_रूख(), tempp);
        }
        यदि (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        यदि (rtnvscp && nodep->taskPublic()) {
            cfuncp->addStmtsp(नूतन AstCReturn{
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
                AstVar* अचर varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                यदि (varp->isFuncLocal()) फर्काऊ;
                // Check if written
                यदि (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->setWrittenByDpi();
                    // Remember we had some
                    writesNonLocals = सत्य;
                }
                // Check if read
                यदि (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->setReadByDpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            यदि (writesNonLocals) {
                AstVarScope* अचर dpiExportTriggerp = getDpiExporTrigger();
                File_पङ्क्ति* अचर flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                AstAssign* अचर assignp
                    = नूतन AstAssign{flp, नूतन AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    नूतन AstConst{flp, AstConst::BitTrue{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                यदि (v3Global.opt.systemC()) {
                    assignp->addNextHere(नूतन AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        यदि (nodep->needProcess()) cfuncp->setNeedProcess();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        फर्काऊ cfuncp;
    }

    शून्य iterateIntoFTask(Ast_गाँठो_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    शून्य घुसाउ_Before_Stmt(Ast_गाँठो* nodep, Ast_गाँठो* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->addHereThisAsNext(newp);
    }
    शून्य processArgs(Ast_गाँठो_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        लागि (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* अचर arrayp = VN_CAST(argp->exprp(), ConsArray);
            यदि (!arrayp) जारी;

            File_पङ्क्ति* अचर flp = arrayp->fileline();
            AstVarScope* अचर substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->isInternal(सत्य);

            AstAssign* अचर assignp
                = नूतन AstAssign{flp, नूतन AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlinkFrBack()};

            Ast_अभिव्यक्ति_Stmt* अचर exprstmtp = नूतन Ast_अभिव्यक्ति_Stmt{
                flp, assignp, नूतन AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    तार्किक isIfaceLocalImpure(Ast_गाँठो_FTask* nodep, Ast_गाँठो* impurep) {
        अचर AstScope* अचर scopep = m_statep->getScope(nodep);
        यदि (!TaskStateVisitor::isIfaceFTaskScope(scopep)) फर्काऊ असत्य;

        अचर AstVarRef* अचर refp = VN_CAST(impurep, VarRef);
        यदि (!refp || !refp->varScopep()) फर्काऊ असत्य;

        फर्काऊ refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    शून्य visit(AstWith* nodep) override {
        यदि (nodep->user1SetOnce()) {
            // Make sure that the return expression is converted only once
            फर्काऊ;
        }
        Ast_गाँठो_अभिव्यक्ति* अचर withExprp = VN_AS(nodep->exprp()->unlinkFrBack(), गाँठो_अभिव्यक्ति);
        nodep->addExprp(नूतन AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    शून्य visit(AstCNew* nodep) override {
        VL_RESTORER(m_inNew);
        m_inNew = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        यदि (m_inSensesp && !nodep->isPure()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(nullptr);  // So V3Broken doesn't complain
            फर्काऊ;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        यदि (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_गन्ती(), nodep, "No non-inline task, but no error issued");
            यदि (VN_IS(nodep->backp(), Stmt_अभिव्यक्ति)) {
                VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
            } अन्यथा {
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
                VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            }
            फर्काऊ;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = nullptr;
        यदि (nodep->taskp()->isFunction()) {
            AstVar* अचर fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            यदि (AstAssign* अचर assignp = VN_CAST(nodep->backp(), Assign)) {
                यदि (AstVarRef* अचर lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* अचर vscp = lhsp->varScopep();
                    यदि (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_रूख(
                            fvarp->dtypep()->skipRefp())
                        && !hasRefArgument(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            यदि (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_गाँठो* beginp;
        AstCNew* cnewp = nullptr;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        अचर तार्किक virtualIfaceCall
            = TaskStateVisitor::isVirtualIfaceMethodCall(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        यदि (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            processArgs(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } अन्यथा {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        यदि (VN_IS(nodep, New)) {  // New not legal as while() condition
            घुसाउ_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
        } अन्यथा यदि (VN_IS(nodep->backp(), गाँठो_Assign)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            घुसाउ_Before_Stmt(nodep, beginp);
            AstVarRef* अचर outrefp = नूतन AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
        } अन्यथा यदि (!VN_IS(nodep->backp(), Stmt_अभिव्यक्ति)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* अचर outrefp = नूतन AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_अभिव्यक्ति_Stmt* lambdap = नूतन Ast_अभिव्यक्ति_Stmt{nodep->fileline(), beginp, outrefp};

            यदि (m_inNew) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_रूख(असत्य);
                newvarp->funcLocal(सत्य);

                // Create a new VarScope for the new variable
                AstVarScope* अचर newvscp
                    = नूतन AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->addVarsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreachAndNext([outvscp, newvscp](AstVarRef* refp) {
                    यदि (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->hasResult(असत्य);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* अचर exprp = नूतन AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtypeSetString();
                lambdap->addStmtsp(नूतन AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
            VIsCached::clear_Cache_रूख();
        } अन्यथा {  // VN_IS(nodep->backp(), StmtExpr)
            घुसाउ_Before_Stmt(nodep, beginp);
            यदि (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    शून्य visit(Ast_गाँठो_FTask* अचर nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        यदि (!nodep->user1SetOnce()) {  // Just one creation needed per function
            // Expand functions in it
            पूर्णाङ्क modes = 0;
            यदि (nodep->dpiImport()) modes++;
            यदि (nodep->dpiExport()) modes++;
            यदि (nodep->taskPublic()) modes++;
            यदि (nodep->classMethod()) modes++;
            यदि (v3Global.opt.protectIds() && nodep->taskPublic()) {
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

            अचर तार्किक noInline = m_statep->ftaskNoInline(nodep);
            अचर तार्किक needsNonInlineCFunc = m_statep->ftaskNeedsNonInlineCFunc(nodep);
            // Warn if not inlining an impure ftask (unless method, recursive,
            // or interface-local member access, which is not truly external).
            // Will likely not schedule correctly.
            // TODO: Why not if recursive? It will not work ...
            यदि (noInline && !nodep->classMethod() && !nodep->recursive()) {
                यदि (Ast_गाँठो* अचर impurep = m_statep->checkImpure(nodep)) {
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
                || nodep->taskPublic()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_गाँठो_FTask* अचर clonedFuncp = nodep->clone_रूख(असत्य);
                यदि (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                अचर तार्किक makeNonInlineFunc = noInline || needsNonInlineCFunc;
                यदि (AstCFunc* अचर cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->addNextHere(cfuncp);
                    यदि (nodep->dpiImport() || makeNonInlineFunc) {
                        m_statep->ftaskCFuncp(nodep, cfuncp);
                    }
                    iterateIntoFTask(clonedFuncp);  // Do the clone too
                }
            }

            // Any variables inside the function still have varscopes pointing to them.
            // We're going to delete the vars, so delete the varscopes.
            यदि (nodep->isFunction()) {
                यदि (AstVar* अचर portp = VN_CAST(nodep->fvarp(), Var)) {
                    AstVarScope* अचर vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(थप_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            लागि (Ast_गाँठो *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                यदि (AstVar* अचर portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* अचर vscp = m_statep->findVarScope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(थप_Deletep(vscp->unlinkFrBack()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlinkFrBack();
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        }
    }
    शून्य visit(AstAssignForce* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::vector<Ast_गाँठो_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_गाँठो_FTask_Ref* refp) { refs.थप_back(refp); });
        लागि (Ast_गाँठो_FTask_Ref* अचर refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* अचर interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            AstAssign* अचर assignp = नूतन AstAssign{
                nodep->fileline(), नूतन AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_रूख_Pure(असत्य)};
            Ast_Sen_रूख* अचर senTreep = नूतन Ast_Sen_रूख{
                nodep->fileline(), नूतन AstSenItem{nodep->fileline(), AstSenItem::Combo{}}};
            AstActive* अचर activep
                = नूतन AstActive{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_रूख_Storep(activep->sentreep());
            activep->addStmtsp(
                नूतन AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, assignp});
            m_scopep->addBlocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(नूतन AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_रूख(), refp);
        }
    }
    शून्य visit(Ast_गाँठो_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    शून्य visit(Ast_गाँठो_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_Stmt_अभिव्यक्ति* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        यदि (!nodep->exprp()) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
    }
    शून्य visit(AstSenItem* nodep) override {
        UASSERT_OBJ(!m_inSensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_inSensesp);
        m_inSensesp = सत्य;
        iterateChildren(nodep);
    }
    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
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

वर्ग TaskIfaceSensVisitor final : सार्वजनिक VNVisitorConst {
    // STATE
    तार्किक m_underIfaceFunc = असत्य;  // Under a CFunc owned by an interface scope
    तार्किक m_underSenses = असत्य;  // Under a sensitivity expression of such a CFunc

    // METHODS
    शून्य markSensesAndIterate(Ast_गाँठो* nodep) {
        यदि (!m_underIfaceFunc) फर्काऊ;
        VL_RESTORER(m_underSenses);
        m_underSenses = सत्य;
        iterateAndNextConstNull(nodep);
    }
    // VISITORS
    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_Sen_रूख* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    शून्य visit(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterateAndNextConstNull(nodep->stmtsp());
    }
    शून्य visit(AstVarRef* nodep) override {
        यदि (!m_underSenses) फर्काऊ;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        यदि (nodep->varp()->isFuncLocal()) फर्काऊ;
        यदि (AstIface* अचर ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = स्वभाव;
};

//######################################################################
// Task class functions

अचर वर्ण* अचर V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_गाँठो_FTask_Ref* nodep, Ast_गाँठो* taskStmtsp,
                                    V3TaskConnectState* statep, तार्किक makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<अचर std::string, पूर्णाङ्क> nameToIndex;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    पूर्णाङ्क tpinnum = 0;
    AstVar* sformatp = nullptr;
    लागि (Ast_गाँठो* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        यदि (AstVar* अचर portp = VN_CAST(stmtp, Var)) {
            यदि (portp->isIO()) {
                tconnects.emplace_back(portp, static_cast<AstArg*>(nullptr));
                nameToIndex.emplace(portp->name(), tpinnum);  // For name based connections
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
    लागि (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        यदि (!argp->name().empty()) {
            // By name
            अचर स्वतः it = nameToIndex.find(argp->name());
            यदि (it == nameToIndex.end()) {
                यदि (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warnMore() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_रूख(), argp);
                }
            } अन्यथा {
                यदि (tconnects[it->second].second && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->second].second->unlinkFrBack()->delete_रूख();
                }
                tconnects[it->second].second = argp;
                reorganize = सत्य;
            }
        } अन्यथा {  // By pin number
            यदि (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } अन्यथा यदि (ppinnum >= tpinnum) {
                यदि (sformatp) {
                    tconnects.emplace_back(sformatp, static_cast<AstArg*>(nullptr));
                    tconnects[ppinnum].second = argp;
                    ++tpinnum;
                } अन्यथा यदि (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warnMore() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlinkFrBack()->delete_रूख(), argp);
                }
            } अन्यथा {
                tconnects[ppinnum].second = argp;
            }
        }
        ++ppinnum;
    }

    यदि (!makeChanges) फर्काऊ tconnects;

    // Connect missing ones
    VInsertionSet<अचर AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    लागि (पूर्णाङ्क i = 0; i < tpinnum; ++i) {
        AstVar* अचर portp = tconnects[i].first;
        यदि (!tconnects[i].second || !tconnects[i].second->exprp()) {
            Ast_गाँठो_अभिव्यक्ति* newvaluep = nullptr;
            यदि (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = नूतन AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } अन्यथा यदि (AstFuncRef* अचर funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                अचर Ast_गाँठो_FTask* अचर funcp = funcRefp->taskp();
                यदि (funcp->classMethod() && funcp->isStatic()) newvaluep = funcRefp;
            } अन्यथा यदि (AstConst* अचर constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            यदि (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), गाँठो_अभिव्यक्ति));
                यदि (!VN_IS(newvaluep, Const)) {
                    यदि (statep) {
                        portp->pin_अङ्क(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.घुसाउ(portp);
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
            newvaluep = newvaluep->backp() ? newvaluep->clone_रूख(सत्य) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* अचर newp = नूतन AstArg{nodep->fileline(), portp->name(), newvaluep};
            यदि (tconnects[i].second) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].second->unlinkFrBack()->delete_रूख(),
                            tconnects[i].second = nullptr);
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

    लागि (अचर स्वतः& tconnect : tconnects) {
        AstArg* अचर argp = tconnect.second;
        argp->name("");  // Can forget name as will add back in pin order
    }

    यदि (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        जबसम्म (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlinkFrBack();
        }
        लागि (पूर्णाङ्क i = 0; i < tpinnum; ++i) {
            AstArg* अचर argp = tconnects[i].second;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->addArgsp(argp);
        }
    }

    यदि (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_रूख("-  ftref-out: ");
        लागि (पूर्णाङ्क i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvtToHex(tconnects[i].first)
                               << "  conn=" << cvtToHex(tconnects[i].second));
        }
    }  // LCOV_EXCL_STOP

    यदि (!argWrap.empty()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        फर्काऊ taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            nullptr);
    }
    फर्काऊ tconnects;
}

शून्य V3Task::taskConnectWrap(Ast_गाँठो_FTask_Ref* nodep, अचर V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             अचर VInsertionSet<अचर AstVar*>& argWrap) {
    statep->setDidWrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    लागि (अचर AstVar* varp : argWrap) newname += "_" + cvtToStr(varp->pin_अङ्क());
    अचर स्वतः pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(nullptr));
    यदि (pair.second) {
        pair.first->second = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_गाँठो_FTask* अचर newTaskp = pair.first->second;

    // Remove the defaulted arguments from original outside call
    लागि (अचर स्वतः& tconnect : tconnects) {
        अचर AstVar* अचर portp = tconnect.first;
        AstArg* अचर argp = tconnect.second;
        यदि (argWrap.exists(portp)) {  // Removed arg
            statep->थप_Deletep(argp->unlinkFrBack());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_गाँठो_FTask* V3Task::taskConnectWrapNew(Ast_गाँठो_FTask* taskp, अचर string& newname,
                                         अचर V3TaskConnects& tconnects,
                                         अचर VInsertionSet<अचर AstVar*>& argWrap) {
    std::map<अचर AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_गाँठो_FTask* अचर newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->addNextHere(newTaskp);

    Ast_गाँठो_FTask_Ref* newCallp = nullptr;
    Ast_गाँठो* newCallInsertp = nullptr;
    यदि (VN_IS(taskp, Func)) {
        AstVar* अचर fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* अचर newFVarp = fvarp->clone_रूख(सत्य);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = नूतन AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = नूतन AstAssign{taskp->fileline(),
                            नूतन AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } अन्यथा यदि (VN_IS(taskp, Task)) {
        newCallp = नूतन AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = नूतन Ast_Stmt_अभिव्यक्ति{taskp->fileline(), newCallp};
    } अन्यथा {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    लागि (अचर स्वतः& tconnect : tconnects) {
        AstVar* अचर portp = tconnect.first;
        AstVar* newPortp;
        यदि (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = नूतन AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(सत्य);
            यदि (newPortp->valuep()) newPortp->valuep()->unlinkFrBack()->delete_रूख();
            newTaskp->addStmtsp(newPortp);
        } अन्यथा {  // Defaulting arg
            Ast_गाँठो_अभिव्यक्ति* अचर valuep = VN_AS(portp->valuep(), गाँठो_अभिव्यक्ति);
            यदि ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                अचर VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* अचर refp = VN_AS(valuep->clone_रूख(असत्य), VarRef);
                refp->access(refAccess);
                AstArg* अचर newArgp = नूतन AstArg{portp->fileline(), portp->name(), refp};
                newCallp->addArgsp(newArgp);
                जारी;
            }
            // Create local temporary
            newPortp = नूतन AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(सत्य);
            newTaskp->addStmtsp(newPortp);
            // Runtime-assign it to the default
            यदि (!VN_IS(valuep, EmptyQueue)) {
                AstAssign* अचर newAssignp
                    = नूतन AstAssign{valuep->fileline(),
                                    नूतन AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_रूख(सत्य)};
                newTaskp->addStmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* अचर newArgp = नूतन AstArg{
            portp->fileline(), portp->name(),
            नूतन AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->addArgsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->addStmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        अचर स्वतः it = oldNewVars.find(refp->varp());
        यदि (it != oldNewVars.end()) refp->varp(it->second);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    फर्काऊ newTaskp;
}

string V3Task::assignInternalToDpi(AstVar* portp, तार्किक isPtr, अचर string& frSuffix,
                                   अचर string& toSuffix, अचर string& frPrefix) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    अचर string frName = frPrefix + portp->name() + frSuffix;
    अचर string toName = portp->name() + toSuffix;
    अचर string idx = portp->name() + "__Vidx";
    माप_प्रकार unpackSize = 1;  // non-unpacked array is treated as size 1
    पूर्णाङ्क unpackDim = 0;
    यदि (AstUnpackArrayDType* अचर unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpackSize = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(असत्य).second;
        यदि (unpackDim > 0) UASSERT_OBJ(unpackSize > 0, portp, "size must be greater than 0");
    }
    यदि (portp->basicp()->isDpiBitVec() || portp->basicp()->isDpiLogicVec()) {
        अचर तार्किक isBit = portp->basicp()->isDpiBitVec();
        अचर तार्किक needsFor = unpackSize > 1;
        यदि (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvtToStr(unpackSize) + "; ++"
                   + idx + ") ";
        }

        stmt += (isBit ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvtToStr(portp->width()) + ", ";
        stmt += toName;
        यदि (needsFor) {
            stmt += " + " + cvtToStr(portp->dtypep()->skipRefp()->width_शब्दहरू()) + " * " + idx;
        }
        stmt += ", ";
        यदि (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            लागि (पूर्णाङ्क i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } अन्यथा {
            stmt += frName + ")";
        }
    } अन्यथा {
        अचर तार्किक isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        अचर तार्किक isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        अचर string unpackAt = unpackSize > 1 ? "[" + idx + "]" : "[0]";
        यदि (unpackDim > 0) {
            अचर string forStmt = unpackSize > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvtToStr(unpackSize) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + toName + unpackAt;
        } अन्यथा {
            यदि (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += toName;
        }
        stmt += " = ";
        यदि (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        यदि (unpackDim > 0) {
            stmt += "(&" + frName;
            लागि (पूर्णाङ्क i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } अन्यथा {
            stmt += frName;
        }
        यदि (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    फर्काऊ stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::assignDpiToInternal(अचर std::string& lhsName, AstVar* varp) {
    std::string cvt;
    तार्किक useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpiToInternalCvtStmt(varp->dtypep());
    अचर std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    अचर पूर्णाङ्क total = strides.empty() ? 1 : strides[0].first * strides[0].second;
    अचर पूर्णाङ्क width_शब्दहरू = varp->basicp()->width_शब्दहरू();
    string statements;
    लागि (पूर्णाङ्क i = 0; i < total; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        लागि (अचर स्वतः& stride : strides) {
            lhs += "[" + std::to_string((i / stride.second) % stride.first) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        यदि (useSvVec) {
            अचर std::string offset = std::to_string(i * width_शब्दहरू);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + offset + ");\n";
        } अन्यथा {
            अचर std::string elem = strides.empty() ? "" : "[" + std::to_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    फर्काऊ statements;
}

शून्य V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        अचर TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_रूख("task", 0, dump_रूख_Either_Level() >= 3);
}
