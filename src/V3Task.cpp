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

صنف Task_بنسټ_راس VL_NOT_FINAL : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Task_بنسټ_راس, V3_ګراف_راس);
    Ast_غوټه* m_impurep = تش_اشاره;  // Node causing impure function w/ outside references
    منطقي m_noInline = خطأ;  // Marked with pragma
عام:
    څرګند Task_بنسټ_راس(V3_ګراف* graphp)
        : V3_ګراف_راس{graphp} {}
    ~Task_بنسټ_راس() override = افتراضي;
    منطقي خالص_3() ثابت { أرجع m_impurep == تش_اشاره; }
    Ast_غوټه* impure_غوټه() ثابت { أرجع m_impurep; }
    فراغ impure(Ast_غوټه* nodep) { m_impurep = nodep; }
    منطقي noInline() ثابت { أرجع m_noInline; }
    فراغ noInline(منطقي flag) { m_noInline = flag; }
};

صنف Task_FTask_راس final : عام Task_بنسټ_راس {
    VL_RTTI_IMPL(Task_FTask_راس, Task_بنسټ_راس);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_غوټه_FTask* ثابت m_nodep;
    AstCFunc* m_cFuncp = تش_اشاره;
    منطقي m_needsNonInlineCFunc = خطأ;  // Needs CFunc for selected non-inlined call sites

عام:
    Task_FTask_راس(V3_ګراف* graphp, Ast_غوټه_FTask* nodep)
        : Task_بنسټ_راس{graphp}
        , m_nodep{nodep} {}
    ~Task_FTask_راس() override = افتراضي;
    Ast_غوټه_FTask* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
    string name() ثابت override VL_MT_STABLE { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع خالص_3() ? "black" : "red"; }
    AstCFunc* cFuncp() ثابت { أرجع m_cFuncp; }
    فراغ cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    منطقي needsNonInlineCFunc() ثابت { أرجع m_needsNonInlineCFunc; }
    فراغ needsNonInlineCFunc(منطقي flag) { m_needsNonInlineCFunc = flag; }
};

صنف Task_کوډ_راس final : عام Task_بنسټ_راس {
    VL_RTTI_IMPL(Task_کوډ_راس, Task_بنسټ_راس);
    // Top vertex for all calls not under another task
عام:
    څرګند Task_کوډ_راس(V3_ګراف* graphp)
        : Task_بنسټ_راس{graphp} {}
    ~Task_کوډ_راس() override = افتراضي;
    string name() ثابت override VL_MT_STABLE { أرجع "*CODE*"; }
    string dotColor() ثابت override { أرجع "green"; }
};

صنف Task_څنډه final : عام V3_ګراف_څنډه {
    VL_RTTI_IMPL(Task_څنډه, V3_ګراف_څنډه)
عام:
    Task_څنډه(V3_ګراف* graphp, Task_بنسټ_راس* fromp, Task_بنسټ_راس* سر)
        : V3_ګراف_څنډه{graphp, fromp, سر, 1, خطأ} {}
    ~Task_څنډه() override = افتراضي;
    string dotLabel() ثابت override { أرجع "w" + cvt_منزل_Str(وزن_2()); }
};

//######################################################################

صنف TaskStateVisitor final : عام VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ثابت VNUser3_دننه_Use m_inuser3;
    ثابت VNUser4_دننه_Use m_inuser4;

    // TYPES
    کارول Var_منزل_Scope_Map = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    کارول Func_منزل_Class_Map = std::unordered_map<ثابت Ast_غوټه_FTask*, AstClass*>;
    // MEMBERS
    Var_منزل_Scope_Map m_var_منزل_Scope_Map;  // Map for Var -> VarScope mappings
    Func_منزل_Class_Map m_func_منزل_Class_Map;  // Map for ctor func -> class
    Ast_غوټه_FTask* m_ctorp = تش_اشاره;  // Class constructor
    AstClass* m_classp = تش_اشاره;  // Current class
    V3_ګراف m_call_ګراف;  // Task call graph
    Task_بنسټ_راس* m_curVxp;  // Current vertex we're adding to
    std::ویکټور<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    منطقي m_under_پورټ_Var = خطأ;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

عام:
    // METHODS
    ساكن منطقي is_Virtual_Iface_لاره_Call(ثابت Ast_غوټه_FTask_Ref* refp) {
        ثابت Ast_لاره_Call* ثابت mrefp = VN_CAST(refp, لاره_Call);
        إذا (!mrefp) أرجع خطأ;
        ثابت Ast_غوټه_بیان* ثابت fromp = mrefp->fromp();
        إذا (!fromp || !fromp->dtypep()) أرجع خطأ;
        ثابت AstIfaceRefDType* ثابت ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        أرجع ifrefp && ifrefp->isVirtual();
    }
    ساكن منطقي isIfaceFTaskScope(ثابت AstScope* scopep) {
        أرجع scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_غوټه_FTask* nodep) {
        AstScope* ثابت scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        أرجع scopep;
    }
    AstVarScope* پیدا_کړه_Var_Scope(AstScope* scopep, AstVar* nodep) {
        ثابت خپلکاری iter = m_var_منزل_Scope_Map.پیدا_کړه(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_var_منزل_Scope_Map.پای_ټکی(), nodep, "No scope for var");
        أرجع iter->دوهم;
    }
    AstClass* getClassp(Ast_غوټه_FTask* nodep) {
        AstClass* ثابت classp = m_func_منزل_Class_Map[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        أرجع classp;
    }
    فراغ remapFuncClassp(Ast_غوټه_FTask* nodep, Ast_غوټه_FTask* newp) {
        m_func_منزل_Class_Map[newp] = getClassp(nodep);
    }
    منطقي ftaskNoInline(Ast_غوټه_FTask* nodep) { أرجع get_FTask_راس(nodep)->noInline(); }
    منطقي ftaskNeedsNonInlineCFunc(Ast_غوټه_FTask* nodep) {
        أرجع get_FTask_راس(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_غوټه_FTask* nodep) { أرجع get_FTask_راس(nodep)->cFuncp(); }
    فراغ ftaskCFuncp(Ast_غوټه_FTask* nodep, AstCFunc* cfuncp) {
        get_FTask_راس(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_غوټه* checkImpure(Ast_غوټه_FTask* nodep) { أرجع get_FTask_راس(nodep)->impure_غوټه(); }

خاص:
    Task_FTask_راس* get_FTask_راس(Ast_غوټه_FTask* nodep) {
        إذا (!nodep->user4p()) nodep->user4p(جديد Task_FTask_راس{&m_call_ګراف, nodep});
        أرجع static_cast<Task_FTask_راس*>(nodep->user4u().منزل_ګراف_راس());
    }

    فراغ propagateImpureReason() {
        std::ویکټور<Task_FTask_راس*> workList;
        // Gather all impure FTask vertices
        لكل (V3_ګراف_راس& vtx : m_call_ګراف.vertices()) {
            إذا (Task_FTask_راس* ثابت ftVtxp = vtx.cast<Task_FTask_راس>()) {
                إذا (!ftVtxp->خالص_3()) workList.emplace_شا(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        طالما (!workList.تشه()) {
            Task_FTask_راس* ثابت calleep = workList.شا();
            workList.راوباسه_شا();
            UASSERT_OBJ(!calleep->خالص_3(), calleep->nodep(), "Should not enqueue pure vertex");
            لكل (V3_ګراف_څنډه& څنډه : calleep->دننه_څنډې()) {
                إذا (Task_FTask_راس* ثابت callerp = څنډه.fromp()->cast<Task_FTask_راس>()) {
                    إذا (!callerp->خالص_3()) استمر;  // Already processed or on workList
                    callerp->impure(calleep->impure_غوټه());
                    workList.emplace_شا(callerp);
                }
            }
        }
    }

    فراغ decideInlining() {
        لكل (V3_ګراف_راس& vtx : m_call_ګراف.vertices()) {
            Task_FTask_راس* ثابت ftVtxp = vtx.cast<Task_FTask_راس>();
            إذا (!ftVtxp) استمر;
            // Move on if already decided not to inline
            إذا (ftVtxp->noInline()) استمر;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            إذا (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(ریښتیا);
                استمر;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            إذا (v3Global.opt.fInlineFuncsEager()) استمر;

            Ast_غوټه_FTask* ثابت ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            إذا (!ftaskp->verilogFunction()) استمر;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            إذا (ftaskp->task_عامه()) استمر;
            // If the function is pure, it's OK not to inline, so don't do it
            إذا (ftVtxp->خالص_3()) ftVtxp->noInline(ریښتیا);
        }
    }

    // VISITORS
    فراغ لیدنه(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        لكل (Ast_غوټه* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVarScope* ثابت vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ثابت varp = vscp->varp();
                إذا (varp->isFuncLocal() || varp->is_کارول_شوی_Loop_Idx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_var_منزل_Scope_Map.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        لكل (Ast_غوټه* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_غوټه_FTask* ثابت taskp = VN_CAST(stmtp, غوټه_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        Task_FTask_راس* ثابت taskVtxp = get_FTask_راس(nodep->taskp());
        جديد Task_څنډه{&m_call_ګراف, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        إذا (is_Virtual_Iface_لاره_Call(nodep)) taskVtxp->needsNonInlineCFunc(ریښتیا);
        // Do we have to disable inlining the function?
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        إذا (!taskVtxp->noInline()) {  // Else short-circuit below
            لكل (ثابت خپلکاری& itr : tconnects) {
                ثابت AstVar* ثابت portp = itr.لومړی;
                ثابت AstArg* ثابت argp = itr.دوهم;
                إذا (ثابت Ast_غوټه_بیان* ثابت pinp = argp->exprp()) {
                    إذا ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(ریښتیا);
                    }
                }
            }
        }
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = get_FTask_راس(nodep);
        إذا (nodep->dpiImport()) m_curVxp->noInline(ریښتیا);
        إذا (nodep->class_لاره()) m_curVxp->noInline(ریښتیا);  // Until V3Task supports it
        إذا (nodep->recursive()) m_curVxp->noInline(ریښتیا);
        إذا (nodep->isConstructor()) {
            m_curVxp->noInline(ریښتیا);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_func_منزل_Class_Map[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(ریښتیا);
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstVar* nodep) override {
        VL_RESTORER(m_under_پورټ_Var);
        إذا (nodep->isIO()) m_under_پورټ_Var = ریښتیا;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        iterateChildren(nodep);
        إذا (m_under_پورټ_Var) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        إذا (nodep->access().isReadOnly() && varp->const_Pool_ثبت()) أرجع;
        إذا (varp->user4u().منزل_ګراف_راس() != m_curVxp) {
            إذا (m_curVxp->خالص_3() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    فراغ لیدنه(AstClass* nodep) override {
        // Move initial statements into the constructor
        VL_RESTORER_CLEAR(m_initialps);
        VL_RESTORER(m_ctorp);
        VL_RESTORER(m_classp);
        m_ctorp = تش_اشاره;
        m_classp = nodep;
        {  // Find m_initialps, m_ctor
            iterateChildren(nodep);
        }
        UASSERT_OBJ(m_ctorp, nodep, "class constructor missing");  // LinkDot always makes it
        Ast_غوټه* insertp = تش_اشاره;
        لكل (AstInitialAutomatic* initialp : m_initialps) {
            إذا (Ast_غوټه* ثابت movep = initialp->stmtsp()) {
                movep->unlink_Fr_شا_With_بل();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_غوټه::جمع_کړه_بل_Null(insertp, movep);
            }
            VL_DO_DANGLING(ټېل_Deletep(initialp->unlink_Fr_شا()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    فراغ لیدنه(AstInitialAutomatic* nodep) override {
        m_initialps.ټېل_شا(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = جديد Task_کوډ_راس{&m_call_ګراف};
        //
        iterate(nodep);
        //
        m_call_ګراف.remove_Redundant_څنډې_مجموعه(&Task_څنډه::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        إذا (dump_ګراف_Level()) m_call_ګراف.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = افتراضي;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

بنية TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    ساكن std::ویکټور<std::pair<صحيح, صحيح>> unpackDimsAndStrides(AstVar* varp) {
        Ast_غوټه_DType* ثابت dtypep = varp->dtypep()->skipRefp();
        std::ویکټور<std::pair<صحيح, صحيح>> dimStrides;
        AstUnpackArrayDType* ثابت unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        إذا (!unpackp) أرجع dimStrides;

        ثابت std::ویکټور<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ثابت اندازه_t nDims = dims.اندازه();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].لومړی = dims.شا()->elementsConst();
        dimStrides[nDims - 1].دوهم = 1;
        // Rest are densly packed
        لكل (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].لومړی = dims[i]->elementsConst();
            dimStrides[i].دوهم = dimStrides[i + 1].لومړی * dimStrides[i + 1].دوهم;
        }
        أرجع dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    ساكن std::pair<std::string, منطقي> dpi_منزل_دننی_Cvt_Stmt(ثابت Ast_غوټه_DType* dtypep) {
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();

        // DPI types using Primitive C types
        إذا (basicp->keyword() == VBasicDTypeKwd::CHANDLE) أرجع {"VL_CVT_VP_Q(", خطأ};
        إذا (basicp->keyword() == VBasicDTypeKwd::STRING) أرجع {"VL_CVT_N_CSTR(", خطأ};
        إذا (basicp->isDpiPrimitive()) أرجع {"(", خطأ};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->is_Dpi_بټ_Vec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ثابت std::string vecType = basicp->is_Dpi_بټ_Vec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ثابت حرف اندازه_Char = dtypep->پلنوالی() <= 8    ? 'C'
                              : dtypep->پلنوالی() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ثابت std::string& اندازه = std::منزل_string(dtypep->پلنوالی());
        // cppcheck-suppress strPlusChar
        أرجع {"VL_SET_"s + اندازه_Char + "_" + vecType + "(" + اندازه + ", ", ریښتیا};
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
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;

    // TYPES
    کارول DpiCFuncs = std::map<ثابت string, std::tuple<Ast_غوټه_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ثابت m_statep;  // Common state between visitors
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    Ast_سر_Scope* ثابت m_سر_Scopep = v3Global.rootp()->سر_Scopep();  // The AstTopScope
    AstScope* m_scopep = تش_اشاره;  // Current scope
    Ast_غوټه* m_insStmtp = تش_اشاره;  // Where to insert statement
    منطقي m_دننه_Sensesp = خطأ;  // Are we under a senitem?
    منطقي m_دننه_New = خطأ;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    صحيح m_uncon_Var_عدد = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpi_نومونه;  // Map of all created DPI functions
    std::map<std::string, ثابت AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ثابت string& name, AstVar* examplep) {
        AstVar* ثابت newvarp = جديد AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(ریښتیا);
        funcp->جمع_کړه_Varsp(newvarp);
        AstVarScope* ثابت newvscp = جديد AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->جمع_کړه_Varsp(newvscp);
        أرجع newvscp;
    }
    AstVarScope* create_ننوت_Var(AstCFunc* funcp, ثابت string& name, VBasicDTypeKwd kwd) {
        AstVar* ثابت newvarp
            = جديد AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->پیدا_کړه_Basic_DType(kwd)};
        newvarp->funcLocal(ریښتیا);
        newvarp->direction(VDirection::ننوت);
        funcp->جمع_کړه_Argsp(newvarp);
        AstVarScope* ثابت newvscp = جديد AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->جمع_کړه_Varsp(newvscp);
        أرجع newvscp;
    }
    ساكن AstVarScope* const_Pool_جدول(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ثابت refp = V3ConstPool::پیدا_کړه(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ثابت vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_ونه(), refp);
        أرجع vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ثابت Ast_غوټه_FTask_Ref* refp, AstVar* invarp,
                                ثابت std::string& suffix) {
        إذا (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            أرجع const_Pool_جدول(invarp);
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
            newvarp->is_دننی(ریښتیا);
            أرجع newvscp;
        }
    }

    // Replace varrefs with new var pointer
    فراغ relink(Ast_غوټه* nodep) {
        nodep->foreach_And_بل([](AstVarRef* refp) {
            إذا (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ثابت newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    Ast_ټاکل* connect_پورټ_Make_دننه_ټاکل(Ast_غوټه_بیان* pinp, AstVarScope* newvscp, منطقي خالص_Check) {
        // Create input assignment to go in FRONT of function call
        Ast_غوټه_بیان* دننه_Pinp = pinp;
        إذا (AstResizeLValue* sinPinp = VN_CAST(دننه_Pinp, ResizeLValue)) دننه_Pinp = sinPinp->lhsp();
        Ast_غوټه_بیان* دننه_Pin_Clonep
            = خالص_Check ? دننه_Pinp->clone_ونه_خالص(ریښتیا) : دننه_Pinp->clone_ونه(ریښتیا);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (arrayp && arrayp->is_سرچپه_د(دننه_Pin_Clonep->dtypep())) {
            دننه_Pin_Clonep
                = جديد Ast_Cvt_Unpack_منزل_Unpack{pinp->fileline(), دننه_Pin_Clonep, newvscp->dtypep()};
        }
        Ast_ټاکل* ثابت assp = جديد Ast_ټاکل{
            pinp->fileline(), جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            دننه_Pin_Clonep};
        assp->fileline()->modifyWarnOff(V3_Error_کوډ::BLKSEQ, ریښتیا);  // Ok if in <= block
        أرجع assp;
    }
    Ast_ټاکل* connect_پورټ_Make_بهر_ټاکل(AstVar* portp, Ast_غوټه_بیان* pinp, AstVarScope* newvscp,
                                        منطقي خالص_Check) {
        // If needed, remap size of function to caller's output size
        Ast_غوټه_بیان* بهر_Pinp = pinp;
        Ast_غوټه_بیان* postRhsp = جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        إذا (AstResizeLValue* soutPinp = VN_CAST(بهر_Pinp, ResizeLValue)) {
            بهر_Pinp = soutPinp->lhsp();
            إذا (VN_IS(بهر_Pinp, RToIRoundS) || VN_IS(بهر_Pinp, RToIS)) {
                بهر_Pinp = VN_AS(بهر_Pinp, غوټه_Uniop)->lhsp();
                إذا (postRhsp->isSigned()) {
                    postRhsp = جديد AstISToRD{pinp->fileline(), postRhsp};
                } وإلا {
                    postRhsp = جديد AstIToRD{pinp->fileline(), postRhsp};
                }
            } وإلا {
                إذا (Ast_غوټه_Uniop* aoutPinp = VN_CAST(بهر_Pinp, Extend)) {
                    بهر_Pinp = aoutPinp->lhsp();
                } وإلا إذا (Ast_غوټه_Uniop* aoutPinp = VN_CAST(بهر_Pinp, ExtendS)) {
                    بهر_Pinp = aoutPinp->lhsp();
                } وإلا إذا (AstSel* aoutPinp = VN_CAST(بهر_Pinp, Sel)) {
                    بهر_Pinp = aoutPinp->fromp();
                } وإلا {
                    بهر_Pinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                إذا (بهر_Pinp->پلنوالی() < portp->پلنوالی()) {
                    postRhsp = جديد AstSel{pinp->fileline(), postRhsp, 0, pinp->پلنوالی()};
                } وإلا {  // pin width > port width
                    إذا (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = جديد AstExtendS{pinp->fileline(), postRhsp};
                    } وإلا {
                        postRhsp = جديد AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(بهر_Pinp);
        }
        إذا (VN_IS(بهر_Pinp, IToRD) || VN_IS(بهر_Pinp, ISToRD)) {
            بهر_Pinp = VN_AS(بهر_Pinp, غوټه_Uniop)->lhsp();
            postRhsp = جديد AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(بهر_Pinp);
        }
        // Put output assignment AFTER function call
        Ast_غوټه_بیان* ثابت بهر_Pin_Clonep
            = خالص_Check ? بهر_Pinp->clone_ونه_خالص(ریښتیا) : بهر_Pinp->clone_ونه(ریښتیا);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(بهر_Pinp->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (arrayp && arrayp->is_سرچپه_د(postRhsp->dtypep())) {
            postRhsp = جديد Ast_Cvt_Unpack_منزل_Unpack{pinp->fileline(), postRhsp, بهر_Pinp->dtypep()};
        }
        Ast_ټاکل* ثابت assp = جديد Ast_ټاکل{pinp->fileline(), بهر_Pin_Clonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3_Error_کوډ::BLKSEQ, ریښتیا);  // Ok if in <= block
        أرجع assp;
    }

    فراغ changeAtWriteRecurse(Ast_غوټه_بیان* ثابت exprp) {
        // Change nested at methods to writable variant
        إذا (AstCMethodHard* ثابت cMethodp = VN_CAST(exprp, CMethodHard)) {
            إذا (cMethodp->لاره() == VCMethod::ARRAY_AT) {
                cMethodp->لاره(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    فراغ connect_پورټ(ثابت Ast_غوټه_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_غوټه* beginp,
                     منطقي inlineTask) {
        Ast_غوټه_بیان* pinp = argp->exprp();
        إذا (inlineTask) {
            portp->unlink_Fr_شا();
            ټېل_Deletep(portp);  // Remove it from the clone (not original)
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
                                                    + std::منزل_string(m_uncon_Var_عدد++),
                                                portp->dtypep()};
                m_modp->جمع_کړه_Stmtsp(varp);
                AstVarScope* ثابت newvscp = جديد AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->جمع_کړه_Varsp(newvscp);
                AstVarRef* ثابت repp = جديد AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(ټېل_Deletep(pinp), pinp);
                pinp = repp;
            }
            إذا (inlineTask) {
                ټېل_Deletep(pinp->unlink_Fr_شا());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlink_Fr_شا()->delete_ونه(), argp);  // Args no longer needed
            }
            إذا (portp->isRef() || portp->isConstRef()) {
                منطقي refArgOk = خطأ;
                إذا (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = ریښتیا;
                } وإلا إذا (AstCMethodHard* ثابت cMethodp = VN_CAST(pinp, CMethodHard)) {
                    إذا (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), کتار_DType)) {
                        refArgOk = cMethodp->لاره() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->لاره() == VCMethod::DYN_AT_WRITE_APPEND_شا;
                    } وإلا {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->لاره() == VCMethod::ARRAY_AT_WRITE;
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
                    ټېل_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                Ast_ټاکل* ثابت preassp = connect_پورټ_Make_دننه_ټاکل(pinp, newvscp, ریښتیا);
                beginp->جمع_کړه_بل_دلته(preassp);

                Ast_ټاکل* ثابت postassp = connect_پورټ_Make_بهر_ټاکل(portp, pinp, newvscp, ریښتیا);
                beginp->جمع_کړه_بل(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } وإلا إذا (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                إذا (!inlineTask) {
                    pinp->replaceWith(جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    ټېل_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                Ast_ټاکل* ثابت postassp = connect_پورټ_Make_بهر_ټاکل(portp, pinp, newvscp, خطأ);
                // Put assignment BEHIND of all other statements
                beginp->جمع_کړه_بل(postassp);
            } وإلا إذا (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                Ast_ټاکل* ثابت preassp = connect_پورټ_Make_دننه_ټاکل(pinp, newvscp, خطأ);
                // Put assignment in FRONT of all other statements
                beginp->جمع_کړه_بل_دلته(preassp);
            }
        }
    }

    منطقي has_Ref_دلیل(Ast_غوټه_FTask* nodep) {
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isRef() || varp->isConstRef()) أرجع ریښتیا;
            }
        }
        أرجع خطأ;
    }

    Ast_غوټه* createInlinedFTask(Ast_غوټه_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_غوټه* ثابت newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_ونه(ریښتیا) : تش_اشاره;
        Ast_غوټه* ثابت beginp
            = جديد AstComment{refp->fileline(), "Function: "s + refp->name(), ریښتیا};
        إذا (newbodysp) beginp->جمع_کړه_بل(newbodysp);
        إذا (debug() >= 9) beginp->dump_ونه_And_بل(وتنه, "-  newbegi: ");
        //
        // Create input variables
        Ast_غوټه::user2_Clear_ونه();
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            لكل (ثابت خپلکاری& itr : tconnects) {
                AstVar* ثابت portp = itr.لومړی;
                AstArg* ثابت argp = itr.دوهم;
                connect_پورټ(refp, portp, argp, beginp, ریښتیا);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_غوټه* nextstmtp;
            لكل (Ast_غوټه* stmtp = beginp; stmtp; stmtp = nextstmtp) {
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
                            Ast_غوټه* ثابت crstp = جديد Ast_ټاکل{
                                portp->fileline(),
                                جديد AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                جديد AstCReset{portp->fileline(), portp, خطأ}};
                            portp->replaceWith(crstp);
                        } وإلا {
                            portp->unlink_Fr_شا();
                        }
                        ټېل_Deletep(portp);  // Remove it from the clone (not original)
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
        إذا (debug() >= 9) beginp->dump_ونه_And_بل(وتنه, "-  iotask: ");
        أرجع beginp;
    }

    Ast_غوټه* createNonInlinedFTask(Ast_غوټه_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ثابت cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_غوټه* ثابت beginp
            = جديد AstComment{refp->fileline(), "Function: "s + refp->name(), ریښتیا};
        Ast_غوټه_CCall* ccallp;
        إذا (VN_IS(refp, New)) {
            AstCNew* ثابت cnewp = جديد AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } وإلا إذا (ثابت Ast_لاره_Call* ثابت mrefp = VN_CAST(refp, لاره_Call)) {
            ccallp = جديد AstCMethodCall{refp->fileline(), mrefp->fromp()->unlink_Fr_شا(), cfuncp};
            ccallp->dtype_سېټ_Void();
            beginp->جمع_کړه_بل(ccallp->makeStmt());
        } وإلا {
            ccallp = جديد AstCCall{refp->fileline(), cfuncp};
            ccallp->dtype_سېټ_Void();
            beginp->جمع_کړه_بل(ccallp->makeStmt());
        }

        إذا (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } وإلا إذا (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } وإلا إذا (ثابت Ast_لاره_Call* ثابت لاره_Callp = VN_CAST(refp, لاره_Call)) {
            ccallp->superReference(لاره_Callp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            لكل (ثابت خپلکاری& itr : tconnects) {
                AstVar* ثابت portp = itr.لومړی;
                AstArg* ثابت argp = itr.دوهم;
                connect_پورټ(refp, portp, argp, beginp, خطأ);
            }
        }
        // First argument is symbol table, then output if a function
        ثابت منطقي needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        إذا (needSyms) ccallp->argTypes("vlSymsp");

        إذا (refp->taskp()->dpiContext()) {
            AstScopeName* ثابت snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlink_Fr_شا();
            File_کرښه* ثابت flp = refp->fileline();
            // __Vscopep
            ccallp->جمع_کړه_Argsp(snp);
            // __Vfilenamep
            ccallp->جمع_کړه_Argsp(
                جديد AstCExpr{flp, AstCExpr::خالص{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->جمع_کړه_Argsp(جديد AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        لكل (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->جمع_کړه_Argsp(argp->exprp()->unlink_Fr_شا());
        }

        إذا (outvscp) ccallp->جمع_کړه_Argsp(جديد AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        إذا (debug() >= 9) beginp->dump_ونه_And_بل(وتنه, "-  nitask: ");
        أرجع beginp;
    }

    string dpiSignature(Ast_غوټه_FTask* nodep, AstVar* rtnvarp) ثابت {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        إذا (nodep->dpi_خالص()) dpiproto += "pure ";
        إذا (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(ریښتیا, ریښتیا) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string دلیلونه;
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    إذا (دلیلونه != "") {
                        دلیلونه += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ثابت std::string dpiType = portp->dpiArgType(خطأ, خطأ);
                    dpiproto += dpiType;
                    ثابت std::string vType = portp->dtypep()->لري_Unpacked_Struct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(خطأ);
                    إذا (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    دلیلونه += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        أرجع dpiproto;
    }

    ساكن فراغ checkLegalCIdentifier(Ast_غوټه* nodep, ثابت string& name) {
        إذا (name.پای_ټکی() != std::پیدا_کړه_if(name.begin(), name.پای_ټکی(), [](حرف c) {
                أرجع !std::که_توری_یا_عدد(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    ساكن Ast_غوټه* create_Dpi_لنډمهاله(AstVar* portp, ثابت string& suffix) {
        ثابت string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        أرجع جديد AstCStmt{portp->fileline(), stmt};
    }

    فراغ unlinkAndClone(Ast_غوټه_FTask* funcp, Ast_غوټه* nodep, منطقي with_بل) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        إذا (funcp->recursive()) {
            VNRelinker relinkHandle;
            إذا (with_بل) {
                nodep->unlink_Fr_شا_With_بل(&relinkHandle);
            } وإلا {
                nodep->unlink_Fr_شا(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_غوټه* ثابت newp = nodep->clone_ونه(with_بل);
            relinkHandle.relink(newp);
        } وإلا {
            إذا (with_بل) {
                nodep->unlink_Fr_شا_With_بل();
            } وإلا {
                nodep->unlink_Fr_شا();
            }
        }
    }

    ساكن Ast_غوټه* create_ټاکل_دننی_منزل_Dpi(AstVar* portp, منطقي isPtr, ثابت string& frSuffix,
                                              ثابت string& منزل_Suffix) {
        ثابت string stmt = V3Task::ټاکل_دننی_منزل_Dpi(portp, isPtr, frSuffix, منزل_Suffix);
        أرجع جديد AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    ساكن Ast_غوټه_Stmt* createDpiScalarCvt(Ast_غوټه_بیان* internalp, ثابت std::string& dpi,
                                           ثابت Ast_غوټه_DType* dtypep, منطقي منزل_Dpi) {
        File_کرښه* ثابت flp = internalp->fileline();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        إذا (منزل_Dpi) {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            إذا (basicp->isDpiPrimitive()) {
                cstmtp->جمع_کړه(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->جمع_کړه(internalp);
                cstmtp->جمع_کړه(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } وإلا {
                cstmtp->جمع_کړه((basicp->is_Dpi_بټ_Vec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::منزل_string(dtypep->پلنوالی()) + ", " + dpi + ", ");
                cstmtp->جمع_کړه(internalp);
                cstmtp->جمع_کړه(");");
            }
            أرجع cstmtp;
        }
        std::string cvt;
        منطقي useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_منزل_دننی_Cvt_Stmt(dtypep);
        إذا (useSvVec) {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            cstmtp->جمع_کړه(cvt);
            cstmtp->جمع_کړه(internalp);
            cstmtp->جمع_کړه(", " + dpi + ");");
            أرجع cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ثابت صحيح cwidth = basicp->پلنوالی_ټکي() * VL_EDATASIZE;
        Ast_غوټه_بیان* ثابت rhsp = جديد AstCExpr{flp, cvt + dpi + ")", cwidth};
        أرجع جديد Ast_ټاکل{flp, internalp, جديد AstSel{flp, rhsp, 0, dtypep->پلنوالی()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    ساكن Ast_غوټه_Stmt* createDpiCvt(Ast_غوټه_بیان* internalp, ثابت std::string& dpi,
                                     ثابت Ast_غوټه_DType* dtypep, منطقي منزل_Dpi) {
        File_کرښه* ثابت flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_غوټه_Stmt* stmtsp = تش_اشاره;
        إذا (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            لكل (صحيح i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_غوټه_بیان* ثابت elemp = جديد AstArraySel{flp, internalp->clone_ونه(خطأ), i};
                stmtsp = Ast_غوټه::جمع_کړه_بل(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::منزل_string(i) + "]",
                                                       arrayp->subDTypep(), منزل_Dpi));
            }
        } وإلا إذا (dtypep->لري_Unpacked_Struct()) {
            لكل (ثابت AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ثابت selp
                    = جديد AstStructSel{flp, internalp->clone_ونه(خطأ), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_غوټه::جمع_کړه_بل(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), منزل_Dpi));
            }
        } وإلا {
            أرجع createDpiScalarCvt(internalp, dpi, dtypep, منزل_Dpi);
        }
        VL_DO_DANGLING(internalp->delete_ونه(), internalp);
        أرجع stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    ساكن Ast_غوټه_Stmt* createDpiArgCvt(AstVarScope* vscp, ثابت std::string& dpiName,
                                        منطقي منزل_Dpi) {
        AstVar* ثابت varp = vscp->varp();
        File_کرښه* ثابت flp = vscp->fileline();
        ثابت std::ویکټور<std::pair<صحيح, صحيح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ثابت صحيح ټول = strides.تشه() ? 1 : strides[0].لومړی * strides[0].دوهم;
        ثابت منطقي isStruct = varp->dtypep()->لري_Unpacked_Struct();
        ثابت Ast_غوټه_DType* ثابت elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_غوټه_Stmt* stmtsp = تش_اشاره;
        لكل (صحيح i = 0; i < ټول; ++i) {
            Ast_غوټه_بیان* internalp
                = جديد AstVarRef{flp, vscp, منزل_Dpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            لكل (ثابت خپلکاری& stride : strides) {
                internalp = جديد AstArraySel(flp, internalp, (i / stride.دوهم) % stride.لومړی);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            إذا (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::منزل_string(i * varp->basicp()->پلنوالی_ټکي());
            } وإلا إذا (isStruct || !strides.تشه()) {
                dpi += "[" + std::منزل_string(i) + "]";
            }
            stmtsp = Ast_غوټه::جمع_کړه_بل(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, منزل_Dpi));
        }
        أرجع stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_غوټه_FTask* ثابت nodep, AstVar* ثابت rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_کرښه* ثابت flp = nodep->fileline();
        ثابت std::string cname = Ast_غوټه::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ثابت حرف* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ثابت std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(ریښتیا, ریښتیا) : "";
        // The function we are building
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(ریښتیا);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(ریښتیا);
        funcp->ثبت_ټکی(ریښتیا);
        funcp->isStatic(ریښتیا);
        funcp->protect(خطأ);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_سر_Scopep->scopep()->جمع_کړه_Blocksp(funcp);
        // Check the arguments before converting them
        إذا (!make_پورټ_List(nodep, funcp)) أرجع تش_اشاره;

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
        funcp->جمع_کړه_Stmtsp(prep);
        // Static doesn't need save-restore as if below will re-fill proper value
        prep->جمع_کړه("static int __Vfuncnum = -1;\n");
        // First time init (faster than what the compiler does if we did a singleton
        prep->جمع_کړه("if (VL_UNLIKELY(__Vfuncnum == -1)) {\n");
        prep->جمع_کړه("__Vfuncnum = Verilated::exportFuncNum(\"" + nodep->cname() + "\");\n");
        prep->جمع_کړه("}\n");
        // If the find fails, it will throw an error
        prep->جمع_کړه("const VerilatedScope* const __Vscopep = Verilated::dpiScope();\n");
        // If 'dpiScope()' fails and '__Vscopep' is null; the exportFind function throws an error
        // If __Vcb is null the exportFind function throws and error
        ثابت std::string cbtype
            = VIdProtect::protect(v3Global.opt.مختاړی() + "__Vcb_" + nodep->cname() + "_t");
        prep->جمع_کړه(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ثابت callp = جديد AstCStmt{flp};
        ثابت خپلکاری جمع_کړه_Func_Arg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ثابت vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(خطأ);
            portp->protect(خطأ);
            // Add argument to call
            ثابت VAccess access = portp->direction().pinAccess();
            callp->جمع_کړه(", ");
            callp->جمع_کړه(جديد AstVarRef{portp->fileline(), vscp, access});
            أرجع vscp;
        };
        // Call callback
        callp->جمع_کړه("(*__Vcb)(");
        // First argument is the Syms
        callp->جمع_کړه("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_غوټه* بهر_Cvtsp = تش_اشاره;  // Conversions of output/inout arguments, after the call
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت portp = VN_CAST(stmtp, Var);
            إذا (!portp) استمر;
            إذا (!portp->isIO()) استمر;
            إذا (portp->isFuncReturn()) استمر;
            إذا (portp == rtnvarp) استمر;  // Handled below
            // Add argument to call
            AstVarScope* ثابت outvscp = جمع_کړه_Func_Arg(portp);
            ثابت منطقي isStruct = portp->dtypep()->لري_Unpacked_Struct();
            إذا (portp->isWritable()) {
                بهر_Cvtsp = Ast_غوټه::جمع_کړه_بل(
                    بهر_Cvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), ریښتیا)
                                       : create_ټاکل_دننی_منزل_Dpi(portp, ریښتیا, tmpSuffixp, ""));
            }
            إذا (!portp->isNonOutput()) استمر;
            // Convert input/inout arguments to dpi type
            ثابت std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(خطأ).دوهم == 0
                      ? "*"
                      : "";
            funcp->جمع_کړه_Stmtsp(createDpiArgCvt(outvscp, deref + portp->name(), خطأ));
        }
        // Return value argument goes last
        إذا (rtnvarp) جمع_کړه_Func_Arg(rtnvarp);
        // Close statement
        callp->جمع_کړه(");");
        // Call the user function
        funcp->جمع_کړه_Stmtsp(callp);
        // Convert output/inout arguments back to internal type
        إذا (بهر_Cvtsp) funcp->جمع_کړه_Stmtsp(بهر_Cvtsp);
        // Convert return value
        إذا (rtnvarp) {
            funcp->جمع_کړه_Stmtsp(create_Dpi_لنډمهاله(rtnvarp, ""));
            funcp->جمع_کړه_Stmtsp(create_ټاکل_دننی_منزل_Dpi(rtnvarp, خطأ, tmpSuffixp, ""));
            ثابت std::string شاخص = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->جمع_کړه_Stmtsp(جديد AstCStmt{flp, "return " + rtnvarp->name() + شاخص + ";"});
        }
        أرجع funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_غوټه_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ثابت string name = Ast_غوټه::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ثابت string rtnType = rtnvarp            ? rtnvarp->dpiArgType(ریښتیا, ریښتیا)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ثابت funcp = جديد AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(ریښتیا);
        funcp->dontCombine(ریښتیا);
        funcp->ثبت_ټکی(خطأ);
        funcp->is_لاره(خطأ);
        funcp->protect(خطأ);
        funcp->dpi_خالص(nodep->dpi_خالص());

        ثابت صحيح لګښت = static_cast<صحيح>(V3Control::get_Profile_معلومات(funcp->name()));
        m_statHierDpisWithCosts += (لګښت != 0);
        funcp->لګښت(لګښت);

        // Add DPI Import to top, since it's a global function
        m_سر_Scopep->scopep()->جمع_کړه_Blocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        إذا (!make_پورټ_List(nodep, funcp)) أرجع تش_اشاره;
        أرجع funcp;
    }

    AstCFunc* getDpiFunc(Ast_غوټه_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ثابت string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ثابت خپلکاری pair = m_dpi_نومونه.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, تش_اشاره));
        إذا (pair.دوهم) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ثابت funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.لومړی->دوهم) = funcp;
            أرجع funcp;
        } وإلا {
            // Seen this cname import before. Check if it's the same prototype.
            ثابت Ast_غوټه_FTask* لومړی_Nodep;
            string لومړی_Signature;
            AstCFunc* لومړی_Funcp;
            std::tie(لومړی_Nodep, لومړی_Signature, لومړی_Funcp) = pair.لومړی->دوهم;
            إذا (signature != لومړی_Signature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warn_نور()  //
                               << "... New signature:      " << signature << '\n'  //
                               << لومړی_Nodep->warnOther()
                               << "... Original signature: " << لومړی_Signature << '\n'  //
                               << لومړی_Nodep->warnContextSecondary());
                أرجع تش_اشاره;
            } وإلا {
                // Same signature, return the previously created CFunc
                أرجع لومړی_Funcp;
            }
        }
    }

    ساكن منطقي make_پورټ_List(Ast_غوټه_FTask* nodep, AstCFunc* dpip) {
        منطقي allOk = ریښتیا;
        // Copy nodep's list of function I/O to the new dpip c function
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO()) {
                    // Move it to new function
                    AstVar* ثابت newPortp = portp->clone_ونه(خطأ);
                    newPortp->funcLocal(ریښتیا);
                    dpip->جمع_کړه_Argsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    إذا (!portp->basicp() && !portp->dtypep()->لري_Unpacked_Struct()) {
                        allOk = خطأ;
                        portp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: DPI argument of type "
                                << portp->dtypep()->prettyTypeName() << '\n'
                                << portp->warn_نور()
                                << "... For best portability, use bit, byte, int, or longint");
                        // We don't warn on logic either, although the 4-stateness is lost.
                        // That's what other simulators do.
                    }
                }
            }
        }
        أرجع allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    ساكن منطقي isDpiCIdentifier(ثابت string& name) {
        أرجع !name.تشه() && VString::isIdentifier(name)
               && !std::که_عدد(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_ټکي::isKeyword(name).تشه();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    منطقي checkDpiStruct(ثابت AstVar* portp, ثابت AstStructDType* sdtypep, منطقي nested) {
        ثابت std::string name = sdtypep->dpiName();
        إذا (!(nested && name.تشه()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(ریښتیا) << "\n"
                              << portp->warn_نور()
                              << "... Suggest use a typedef with a C identifier name");
            أرجع خطأ;
        }
        لكل (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            إذا (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                أرجع خطأ;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ثابت Ast_غوټه_DType* dtypep = itemp->subDTypep()->skipRefp();
            طالما (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ثابت Ast_غوټه_UOr_Struct_DType* ثابت subp = VN_CAST(dtypep, غوټه_UOr_Struct_DType);
            ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ثابت منطقي isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            إذا (dtypep->لري_Unpacked_Struct()) {
                إذا (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), ریښتیا)) أرجع خطأ;
            } وإلا إذا (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(ریښتیا)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                أرجع خطأ;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        إذا (name.تشه()) أرجع ریښتیا;
        ثابت AstStructDType* ثابت firstp = m_dpiStructs.emplace(name, sdtypep).لومړی->دوهم;
        إذا (firstp->dpiTypedef() == sdtypep->dpiTypedef()) أرجع ریښتیا;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        أرجع خطأ;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    منطقي check_Dpi_Struct_دلیلونه(ثابت Ast_غوټه_FTask* nodep) {
        منطقي allOk = ریښتیا;
        لكل (ثابت Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var);
            إذا (portp && portp->isIO() && portp->dtypep()->لري_Unpacked_Struct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       خطأ)
                        && allOk;
            }
        }
        أرجع allOk;
    }

    فراغ bodyDpiImportFunc(Ast_غوټه_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ثابت حرف* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        إذا (v3Global.opt.profExec()) {
            cfuncp->جمع_کړه_Stmtsp(AstCStmt::prof_Exec_Section_ټېل(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string دلیلونه;
        لكل (Ast_غوټه* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ثابت portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                إذا (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    إذا (دلیلونه != "") دلیلونه += ", ";

                    إذا (portp->isDpiOpenArray()) {
                        Ast_غوټه_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, کتار_DType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ثابت string propName = portp->name() + "__Vopenprops";
                        ثابت string prop_کوډ = portp->vlPropDecl(propName);
                        cfuncp->جمع_کړه_Stmtsp(جديد AstCStmt{portp->fileline(), prop_کوډ});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ثابت string name = portp->name() + "__Vopenarray";
                        ثابت string var_کوډ
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->جمع_کړه_Stmtsp(جديد AstCStmt{portp->fileline(), var_کوډ});
                        دلیلونه += "&" + name;
                    } وإلا {
                        // Unpacked structures are passed by reference from a C array
                        ثابت منطقي isStruct = portp->dtypep()->لري_Unpacked_Struct();
                        إذا (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            إذا (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) دلیلونه += "&";
                        }

                        دلیلونه += portp->name() + tmpSuffixp;

                        cfuncp->جمع_کړه_Stmtsp(create_Dpi_لنډمهاله(portp, tmpSuffixp));
                        إذا (portp->isNonOutput()) {
                            cfuncp->جمع_کړه_Stmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, ریښتیا)
                                    : create_ټاکل_دننی_منزل_Dpi(portp, خطأ, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        إذا (nodep->dpiContext()) {
            ثابت string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->جمع_کړه_Stmtsp(جديد AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ثابت callp = جديد AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtype_سېټ_Void();
            callp->argTypes(دلیلونه);
            إذا (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->جمع_کړه_Stmtsp(create_Dpi_لنډمهاله(rtnvscp->varp(), tmpSuffixp));
                ثابت std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ثابت cstmtp = جديد AstCStmt{nodep->fileline()};
                cstmtp->جمع_کړه(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->جمع_کړه(" = ");
                cstmtp->جمع_کړه(callp);  // RHS
                cstmtp->جمع_کړه(";");
                cfuncp->جمع_کړه_Stmtsp(cstmtp);
            } وإلا {
                // Othervise just call it
                cfuncp->جمع_کړه_Stmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        لكل (Ast_غوټه* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                portp->protect(خطأ);  // No additional exposure - already part of shown proto
                إذا (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ثابت portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->جمع_کړه_Stmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, خطأ));
                }
            }
        }

        إذا (v3Global.opt.profExec()) {
            cfuncp->جمع_کړه_Stmtsp(AstCStmt::prof_Exec_Section_راوباسه(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        إذا (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_سر_Scopep->create_لنډمهاله("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        أرجع dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_غوټه_FTask* nodep, منطقي ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_غوټه::user2_Clear_ونه();
        AstVar* rtnvarp = تش_اشاره;
        إذا (nodep->isFunction()) {
            AstVar* ثابت portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            إذا (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ثابت bdtypep = portp->dtypep()->basicp();
                إذا (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(ریښتیا)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع تش_اشاره;
                }
                إذا (!bdtypep->isDpiPrimitive()) {
                    إذا (bdtypep->is_Dpi_بټ_Vec() && portp->پلنوالی() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warn_نور()
                                       + "... Suggest make it an output argument instead?");
                    }
                    إذا (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } وإلا إذا (nodep->task_عامه()) {
                إذا (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warn_نور()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            إذا (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(خطأ);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, خطأ);
            rtnvarp = portp;
            rtnvarp->funcLocal(ریښتیا);
            rtnvarp->noCReset(ریښتیا);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            إذا (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(خطأ);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = تش_اشاره;
        إذا (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpi_Open_پلار() && nodep->dpi_Open_ماشوم()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = check_Dpi_Struct_دلیلونه(nodep) ? getDpiFunc(nodep, rtnvarp) : تش_اشاره;
            إذا (!dpiFuncp || (nodep->dpiImport() && nodep->dpi_Open_پلار())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                إذا (rtnvarp) VL_DO_DANGLING(ټېل_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع تش_اشاره;
            }
        }

        AstVarScope* rtnvscp = تش_اشاره;
        إذا (rtnvarp) {
            rtnvscp = جديد AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->جمع_کړه_Varsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string مختاړی;
        إذا (nodep->dpiImport()) {
            مختاړی = "__Vdpiimwrap_";
        } وإلا إذا (nodep->dpiExport()) {
            مختاړی = "__Vdpiexp_";
        } وإلا إذا (ftaskNoInline) {
            مختاړی = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        إذا (!nodep->task_عامه() && !nodep->class_لاره()) suffix = "_" + m_scopep->nameDotless();
        ثابت string name = ((nodep->name() == "new") ? "new" : مختاړی + nodep->name() + suffix);
        AstCFunc* ثابت cfuncp = جديد AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->task_عامه() && rtnvarp) ? rtnvarp->cPubArgType(ریښتیا, ریښتیا) : "")};
        // It's ok to combine imports because this is just a wrapper;
        // duplicate wrappers can get merged.
        cfuncp->dontCombine(!nodep->dpiImport());
        cfuncp->ثبت_ټکی(!nodep->dpiImport());
        cfuncp->func_عامه(nodep->task_عامه());
        cfuncp->dpiContext(nodep->dpiContext());
        cfuncp->dpiExportImpl(nodep->dpiExport());
        cfuncp->dpiImportWrapper(nodep->dpiImport());
        cfuncp->recursive(nodep->recursive());
        // Hardcoded based on UVM usage; TODO make a verilated_std.vlt control for these
        cfuncp->unlikely(nodep->name() == "uvm_report_error" || nodep->name() == "uvm_report_info"
                         || nodep->name() == "uvm_report_fatal"
                         || nodep->name() == "uvm_report_warning");
        إذا (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(ریښتیا);
            cfuncp->isLoose(ریښتیا);
        } وإلا {
            cfuncp->isStatic(خطأ);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpi_خالص(nodep->dpi_خالص());
        إذا (nodep->name() == "new") cfuncp->isConstructor(ریښتیا);
        إذا (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(ریښتیا);
        إذا (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        إذا (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ثابت منطقي needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->task_عامه()) || v3Global.opt.profExec());
        إذا (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        إذا (!nodep->dpiImport() && !nodep->task_عامه()) {
            // Need symbol table
            إذا (cfuncp->isConstructor()) {
                منطقي isInterfaceClass = خطأ;
                إذا (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                إذا (!isInterfaceClass) {
                    ثابت string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->جمع_کړه_Stmtsp(
                        جديد AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        إذا (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_ننوت_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_ننوت_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_ننوت_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        إذا (nodep->dpiExport()) {
            AstScopeName* ثابت snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(ریښتیا);
            snp->unlink_Fr_شا();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        لكل (Ast_غوټه *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlink_Fr_شا();
                    ټېل_Deletep(portp);
                    portp->user2p(const_Pool_جدول(portp));
                } وإلا {
                    إذا (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, خطأ);
                        portp->funcLocal(ریښتیا);
                        إذا (portp->valuep()) ټېل_Deletep(portp->valuep()->unlink_Fr_شا());
                        cfuncp->جمع_کړه_Argsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        إذا (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_غوټه_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                            إذا (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } وإلا {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(ریښتیا);
                    }
                    AstVarScope* ثابت newvscp
                        = جديد AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->جمع_کړه_Varsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        إذا (rtnvarp) cfuncp->جمع_کړه_Argsp(rtnvarp);

        // Move body
        Ast_غوټه* bodysp = nodep->stmtsp();
        إذا (bodysp) {
            unlinkAndClone(nodep, bodysp, ریښتیا);
            AstBegin* ثابت tempp
                = جديد AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, خطأ};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            لكل (Ast_غوټه *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    إذا (portp->isIO()) VL_DO_DANGLING(ټېل_Deletep(portp->unlink_Fr_شا()), portp);
                }
            }
            إذا (tempp->stmtsp()) cfuncp->جمع_کړه_Stmtsp(tempp->stmtsp()->unlink_Fr_شا_With_بل());
            VL_DO_DANGLING(tempp->delete_ونه(), tempp);
        }
        إذا (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        إذا (rtnvscp && nodep->task_عامه()) {
            cfuncp->جمع_کړه_Stmtsp(جديد AstCReturn{
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
                إذا (varp->isFuncLocal()) أرجع;
                // Check if written
                إذا (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->سېټ_Written_By_Dpi();
                    // Remember we had some
                    writesNonLocals = ریښتیا;
                }
                // Check if read
                إذا (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->سېټ_Read_By_Dpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            إذا (writesNonLocals) {
                AstVarScope* ثابت dpiExportTriggerp = getDpiExporTrigger();
                File_کرښه* ثابت flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                Ast_ټاکل* ثابت assignp
                    = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    جديد AstConst{flp, AstConst::بټ_True{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                إذا (v3Global.opt.systemC()) {
                    assignp->جمع_کړه_بل_دلته(جديد AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        إذا (nodep->needProcess()) cfuncp->سېټ_Need_Process();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        أرجع cfuncp;
    }

    فراغ iterateIntoFTask(Ast_غوټه_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    فراغ ورننباسه_Before_Stmt(Ast_غوټه* nodep, Ast_غوټه* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->جمع_کړه_دلته_This_As_بل(newp);
    }
    فراغ process_دلیلونه(Ast_غوټه_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ثابت arrayp = VN_CAST(argp->exprp(), ConsArray);
            إذا (!arrayp) استمر;

            File_کرښه* ثابت flp = arrayp->fileline();
            AstVarScope* ثابت substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->is_دننی(ریښتیا);

            Ast_ټاکل* ثابت assignp
                = جديد Ast_ټاکل{flp, جديد AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlink_Fr_شا()};

            Ast_بیان_Stmt* ثابت exprstmtp = جديد Ast_بیان_Stmt{
                flp, assignp, جديد AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    منطقي isIfaceLocalImpure(Ast_غوټه_FTask* nodep, Ast_غوټه* impurep) {
        ثابت AstScope* ثابت scopep = m_statep->getScope(nodep);
        إذا (!TaskStateVisitor::isIfaceFTaskScope(scopep)) أرجع خطأ;

        ثابت AstVarRef* ثابت refp = VN_CAST(impurep, VarRef);
        إذا (!refp || !refp->varScopep()) أرجع خطأ;

        أرجع refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    فراغ لیدنه(AstWith* nodep) override {
        إذا (nodep->user1_سېټ_Once()) {
            // Make sure that the return expression is converted only once
            أرجع;
        }
        Ast_غوټه_بیان* ثابت withExprp = VN_AS(nodep->exprp()->unlink_Fr_شا(), غوټه_بیان);
        nodep->جمع_کړه_Exprp(جديد AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    فراغ لیدنه(AstCNew* nodep) override {
        VL_RESTORER(m_دننه_New);
        m_دننه_New = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        إذا (m_دننه_Sensesp && !nodep->is_خالص()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(تش_اشاره);  // So V3Broken doesn't complain
            أرجع;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        إذا (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_شمېر(), nodep, "No non-inline task, but no error issued");
            إذا (VN_IS(nodep->backp(), Stmt_بیان)) {
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            } وإلا {
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}});
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
            أرجع;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = تش_اشاره;
        إذا (nodep->taskp()->isFunction()) {
            AstVar* ثابت fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            إذا (Ast_ټاکل* ثابت assignp = VN_CAST(nodep->backp(), ټاکل)) {
                إذا (AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ثابت vscp = lhsp->varScopep();
                    إذا (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_ونه(
                            fvarp->dtypep()->skipRefp())
                        && !has_Ref_دلیل(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            إذا (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_غوټه* beginp;
        AstCNew* cnewp = تش_اشاره;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ثابت منطقي virtualIfaceCall
            = TaskStateVisitor::is_Virtual_Iface_لاره_Call(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        إذا (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            process_دلیلونه(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } وإلا {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        إذا (VN_IS(nodep, New)) {  // New not legal as while() condition
            ورننباسه_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا إذا (VN_IS(nodep->backp(), غوټه_ټاکل)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            ورننباسه_Before_Stmt(nodep, beginp);
            AstVarRef* ثابت outrefp = جديد AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا إذا (!VN_IS(nodep->backp(), Stmt_بیان)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ثابت outrefp = جديد AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_بیان_Stmt* lambdap = جديد Ast_بیان_Stmt{nodep->fileline(), beginp, outrefp};

            إذا (m_دننه_New) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_ونه(خطأ);
                newvarp->funcLocal(ریښتیا);

                // Create a new VarScope for the new variable
                AstVarScope* ثابت newvscp
                    = جديد AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->جمع_کړه_Varsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreach_And_بل([outvscp, newvscp](AstVarRef* refp) {
                    إذا (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->has_پایله(خطأ);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtype_سېټ_String();
                lambdap->جمع_کړه_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            VIsCached::clear_Cache_ونه();
        } وإلا {  // VN_IS(nodep->backp(), StmtExpr)
            ورننباسه_Before_Stmt(nodep, beginp);
            إذا (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_کوډ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlink_Fr_شا();
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    فراغ لیدنه(Ast_غوټه_FTask* ثابت nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        إذا (!nodep->user1_سېټ_Once()) {  // Just one creation needed per function
            // Expand functions in it
            صحيح modes = 0;
            إذا (nodep->dpiImport()) modes++;
            إذا (nodep->dpiExport()) modes++;
            إذا (nodep->task_عامه()) modes++;
            إذا (nodep->class_لاره()) modes++;
            إذا (v3Global.opt.protectIds() && nodep->task_عامه()) {
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
            إذا (noInline && !nodep->class_لاره() && !nodep->recursive()) {
                إذا (Ast_غوټه* ثابت impurep = m_statep->checkImpure(nodep)) {
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
                || nodep->task_عامه()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_غوټه_FTask* ثابت clonedFuncp = nodep->clone_ونه(خطأ);
                إذا (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ثابت منطقي makeNonInlineFunc = noInline || needsNonInlineCFunc;
                إذا (AstCFunc* ثابت cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->جمع_کړه_بل_دلته(cfuncp);
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
                    AstVarScope* ثابت vscp = m_statep->پیدا_کړه_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ټېل_Deletep(vscp->unlink_Fr_شا()), vscp);
                }
            }
            لكل (Ast_غوټه *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ثابت vscp = m_statep->پیدا_کړه_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ټېل_Deletep(vscp->unlink_Fr_شا()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlink_Fr_شا();
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ لیدنه(Ast_ټاکل_Force* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::ویکټور<Ast_غوټه_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_غوټه_FTask_Ref* refp) { refs.ټېل_شا(refp); });
        لكل (Ast_غوټه_FTask_Ref* ثابت refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ثابت interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_ونه_خالص(خطأ)};
            Ast_Sen_ونه* ثابت senTreep = جديد Ast_Sen_ونه{
                nodep->fileline(), جديد Ast_Sen_توکی{nodep->fileline(), Ast_Sen_توکی::Combo{}}};
            Ast_فعال* ثابت activep
                = جديد Ast_فعال{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_ونه_Storep(activep->sentreep());
            activep->جمع_کړه_Stmtsp(
                جديد AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, تش_اشاره, assignp});
            m_scopep->جمع_کړه_Blocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(جديد AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_ونه(), refp);
        }
    }
    فراغ لیدنه(Ast_غوټه_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Stmt_بیان* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->exprp()) VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_Sen_توکی* nodep) override {
        UASSERT_OBJ(!m_دننه_Sensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_دننه_Sensesp);
        m_دننه_Sensesp = ریښتیا;
        iterateChildren(nodep);
    }
    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    TaskVisitor(AstNetlist* nodep, TaskStateVisitor* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~TaskVisitor() {
        V3Stats::جمع_کړه_Stat("Optimizations, Functions inlined", m_statInlines);
        V3Stats::جمع_کړه_Stat("Optimizations, Hierarchical DPI wrappers with costs",
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
    فراغ markSensesAndIterate(Ast_غوټه* nodep) {
        إذا (!m_underIfaceFunc) أرجع;
        VL_RESTORER(m_underSenses);
        m_underSenses = ریښتیا;
        iterate_And_بل_Const_Null(nodep);
    }
    // VISITORS
    فراغ لیدنه(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_Sen_ونه* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    فراغ لیدنه(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterate_And_بل_Const_Null(nodep->stmtsp());
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (!m_underSenses) أرجع;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        إذا (nodep->varp()->isFuncLocal()) أرجع;
        إذا (AstIface* ثابت ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    څرګند TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

ثابت حرف* ثابت V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_غوټه_FTask_Ref* nodep, Ast_غوټه* taskStmtsp,
                                    V3TaskConnectState* statep, منطقي makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ثابت std::string, صحيح> name_منزل_شاخص;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    صحيح tpinnum = 0;
    AstVar* sformatp = تش_اشاره;
    لكل (Ast_غوټه* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            إذا (portp->isIO()) {
                tconnects.emplace_شا(portp, static_cast<AstArg*>(تش_اشاره));
                name_منزل_شاخص.emplace(portp->name(), tpinnum);  // For name based connections
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
        إذا (!argp->name().تشه()) {
            // By name
            ثابت خپلکاری it = name_منزل_شاخص.پیدا_کړه(argp->name());
            إذا (it == name_منزل_شاخص.پای_ټکی()) {
                إذا (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warn_نور() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_شا()->delete_ونه(), argp);
                }
            } وإلا {
                إذا (tconnects[it->دوهم].دوهم && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->دوهم].دوهم->unlink_Fr_شا()->delete_ونه();
                }
                tconnects[it->دوهم].دوهم = argp;
                reorganize = ریښتیا;
            }
        } وإلا {  // By pin number
            إذا (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } وإلا إذا (ppinnum >= tpinnum) {
                إذا (sformatp) {
                    tconnects.emplace_شا(sformatp, static_cast<AstArg*>(تش_اشاره));
                    tconnects[ppinnum].دوهم = argp;
                    ++tpinnum;
                } وإلا إذا (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warn_نور() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_شا()->delete_ونه(), argp);
                }
            } وإلا {
                tconnects[ppinnum].دوهم = argp;
            }
        }
        ++ppinnum;
    }

    إذا (!makeChanges) أرجع tconnects;

    // Connect missing ones
    VInsertion_سېټ<ثابت AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    لكل (صحيح i = 0; i < tpinnum; ++i) {
        AstVar* ثابت portp = tconnects[i].لومړی;
        إذا (!tconnects[i].دوهم || !tconnects[i].دوهم->exprp()) {
            Ast_غوټه_بیان* newvaluep = تش_اشاره;
            إذا (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = جديد AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } وإلا إذا (AstFuncRef* ثابت funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ثابت Ast_غوټه_FTask* ثابت funcp = funcRefp->taskp();
                إذا (funcp->class_لاره() && funcp->isStatic()) newvaluep = funcRefp;
            } وإلا إذا (AstConst* ثابت constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            إذا (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), غوټه_بیان));
                إذا (!VN_IS(newvaluep, Const)) {
                    إذا (statep) {
                        portp->pin_عدد(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.ورننباسه(portp);
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
            newvaluep = newvaluep->backp() ? newvaluep->clone_ونه(ریښتیا) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ثابت newp = جديد AstArg{nodep->fileline(), portp->name(), newvaluep};
            إذا (tconnects[i].دوهم) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].دوهم->unlink_Fr_شا()->delete_ونه(),
                            tconnects[i].دوهم = تش_اشاره);
            }
            tconnects[i].دوهم = newp;
            reorganize = ریښتیا;
        }
        إذا (tconnects[i].دوهم) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].دوهم);
        } وإلا {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    لكل (ثابت خپلکاری& tconnect : tconnects) {
        AstArg* ثابت argp = tconnect.دوهم;
        argp->name("");  // Can forget name as will add back in pin order
    }

    إذا (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        طالما (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlink_Fr_شا();
        }
        لكل (صحيح i = 0; i < tpinnum; ++i) {
            AstArg* ثابت argp = tconnects[i].دوهم;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->جمع_کړه_Argsp(argp);
        }
    }

    إذا (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_ونه("-  ftref-out: ");
        لكل (صحيح i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvt_منزل_Hex(tconnects[i].لومړی)
                               << "  conn=" << cvt_منزل_Hex(tconnects[i].دوهم));
        }
    }  // LCOV_EXCL_STOP

    إذا (!argWrap.تشه()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        أرجع taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            تش_اشاره);
    }
    أرجع tconnects;
}

فراغ V3Task::taskConnectWrap(Ast_غوټه_FTask_Ref* nodep, ثابت V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ثابت VInsertion_سېټ<ثابت AstVar*>& argWrap) {
    statep->سېټ_Did_Wrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    لكل (ثابت AstVar* varp : argWrap) newname += "_" + cvt_منزل_Str(varp->pin_عدد());
    ثابت خپلکاری pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(تش_اشاره));
    إذا (pair.دوهم) {
        pair.لومړی->دوهم = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_غوټه_FTask* ثابت newTaskp = pair.لومړی->دوهم;

    // Remove the defaulted arguments from original outside call
    لكل (ثابت خپلکاری& tconnect : tconnects) {
        ثابت AstVar* ثابت portp = tconnect.لومړی;
        AstArg* ثابت argp = tconnect.دوهم;
        إذا (argWrap.exists(portp)) {  // Removed arg
            statep->ټېل_Deletep(argp->unlink_Fr_شا());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_غوټه_FTask* V3Task::taskConnectWrapNew(Ast_غوټه_FTask* taskp, ثابت string& newname,
                                         ثابت V3TaskConnects& tconnects,
                                         ثابت VInsertion_سېټ<ثابت AstVar*>& argWrap) {
    std::map<ثابت AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_غوټه_FTask* ثابت newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->جمع_کړه_بل_دلته(newTaskp);

    Ast_غوټه_FTask_Ref* newCallp = تش_اشاره;
    Ast_غوټه* newCallInsertp = تش_اشاره;
    إذا (VN_IS(taskp, Func)) {
        AstVar* ثابت fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ثابت newFVarp = fvarp->clone_ونه(ریښتیا);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = جديد AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = جديد Ast_ټاکل{taskp->fileline(),
                            جديد AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } وإلا إذا (VN_IS(taskp, Task)) {
        newCallp = جديد AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = جديد Ast_Stmt_بیان{taskp->fileline(), newCallp};
    } وإلا {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    لكل (ثابت خپلکاری& tconnect : tconnects) {
        AstVar* ثابت portp = tconnect.لومړی;
        AstVar* newPortp;
        إذا (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = جديد AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(ریښتیا);
            إذا (newPortp->valuep()) newPortp->valuep()->unlink_Fr_شا()->delete_ونه();
            newTaskp->جمع_کړه_Stmtsp(newPortp);
        } وإلا {  // Defaulting arg
            Ast_غوټه_بیان* ثابت valuep = VN_AS(portp->valuep(), غوټه_بیان);
            إذا ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ثابت VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ثابت refp = VN_AS(valuep->clone_ونه(خطأ), VarRef);
                refp->access(refAccess);
                AstArg* ثابت newArgp = جديد AstArg{portp->fileline(), portp->name(), refp};
                newCallp->جمع_کړه_Argsp(newArgp);
                استمر;
            }
            // Create local temporary
            newPortp = جديد AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(ریښتیا);
            newTaskp->جمع_کړه_Stmtsp(newPortp);
            // Runtime-assign it to the default
            إذا (!VN_IS(valuep, تشه_کتار)) {
                Ast_ټاکل* ثابت newAssignp
                    = جديد Ast_ټاکل{valuep->fileline(),
                                    جديد AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_ونه(ریښتیا)};
                newTaskp->جمع_کړه_Stmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ثابت newArgp = جديد AstArg{
            portp->fileline(), portp->name(),
            جديد AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->جمع_کړه_Argsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->جمع_کړه_Stmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ثابت خپلکاری it = oldNewVars.پیدا_کړه(refp->varp());
        إذا (it != oldNewVars.پای_ټکی()) refp->varp(it->دوهم);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    أرجع newTaskp;
}

string V3Task::ټاکل_دننی_منزل_Dpi(AstVar* portp, منطقي isPtr, ثابت string& frSuffix,
                                   ثابت string& منزل_Suffix, ثابت string& fr_مختاړی) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ثابت string frName = fr_مختاړی + portp->name() + frSuffix;
    ثابت string منزل_Name = portp->name() + منزل_Suffix;
    ثابت string idx = portp->name() + "__Vidx";
    اندازه_t unpack_اندازه = 1;  // non-unpacked array is treated as size 1
    صحيح unpackDim = 0;
    إذا (AstUnpackArrayDType* ثابت unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpack_اندازه = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(خطأ).دوهم;
        إذا (unpackDim > 0) UASSERT_OBJ(unpack_اندازه > 0, portp, "size must be greater than 0");
    }
    إذا (portp->basicp()->is_Dpi_بټ_Vec() || portp->basicp()->isDpiLogicVec()) {
        ثابت منطقي is_بټ = portp->basicp()->is_Dpi_بټ_Vec();
        ثابت منطقي needsFor = unpack_اندازه > 1;
        إذا (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvt_منزل_Str(unpack_اندازه) + "; ++"
                   + idx + ") ";
        }

        stmt += (is_بټ ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvt_منزل_Str(portp->پلنوالی()) + ", ";
        stmt += منزل_Name;
        إذا (needsFor) {
            stmt += " + " + cvt_منزل_Str(portp->dtypep()->skipRefp()->پلنوالی_ټکي()) + " * " + idx;
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
        ثابت string unpackAt = unpack_اندازه > 1 ? "[" + idx + "]" : "[0]";
        إذا (unpackDim > 0) {
            ثابت string forStmt = unpack_اندازه > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvt_منزل_Str(unpack_اندازه) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + منزل_Name + unpackAt;
        } وإلا {
            إذا (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += منزل_Name;
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
    أرجع stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::ټاکل_Dpi_منزل_دننی(ثابت std::string& lhsName, AstVar* varp) {
    std::string cvt;
    منطقي useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_منزل_دننی_Cvt_Stmt(varp->dtypep());
    ثابت std::ویکټور<std::pair<صحيح, صحيح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ثابت صحيح ټول = strides.تشه() ? 1 : strides[0].لومړی * strides[0].دوهم;
    ثابت صحيح پلنوالی_ټکي = varp->basicp()->پلنوالی_ټکي();
    string statements;
    لكل (صحيح i = 0; i < ټول; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        لكل (ثابت خپلکاری& stride : strides) {
            lhs += "[" + std::منزل_string((i / stride.دوهم) % stride.لومړی) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        إذا (useSvVec) {
            ثابت std::string دننه_واټن = std::منزل_string(i * پلنوالی_ټکي);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + دننه_واټن + ");\n";
        } وإلا {
            ثابت std::string elem = strides.تشه() ? "" : "[" + std::منزل_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    أرجع statements;
}

فراغ V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ثابت TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_ونه("task", 0, dump_ونه_Either_Level() >= 3);
}
