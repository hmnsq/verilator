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

صنف Task_پایه_رأس VL_NOT_FINAL : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Task_پایه_رأس, V3_گراف_رأس);
    Ast_گره* m_impurep = اشاره_تهی;  // Node causing impure function w/ outside references
    منطقي m_noInline = خطأ;  // Marked with pragma
عام:
    صریح Task_پایه_رأس(V3_گراف* graphp)
        : V3_گراف_رأس{graphp} {}
    ~Task_پایه_رأس() override = افتراضي;
    منطقي خالص_3() ثابت { أرجع m_impurep == اشاره_تهی; }
    Ast_گره* impure_گره() ثابت { أرجع m_impurep; }
    فراغ impure(Ast_گره* nodep) { m_impurep = nodep; }
    منطقي noInline() ثابت { أرجع m_noInline; }
    فراغ noInline(منطقي flag) { m_noInline = flag; }
};

صنف Task_FTask_رأس final : عام Task_پایه_رأس {
    VL_RTTI_IMPL(Task_FTask_رأس, Task_پایه_رأس);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_گره_FTask* ثابت m_nodep;
    AstCFunc* m_cFuncp = اشاره_تهی;
    منطقي m_needsNonInlineCFunc = خطأ;  // Needs CFunc for selected non-inlined call sites

عام:
    Task_FTask_رأس(V3_گراف* graphp, Ast_گره_FTask* nodep)
        : Task_پایه_رأس{graphp}
        , m_nodep{nodep} {}
    ~Task_FTask_رأس() override = افتراضي;
    Ast_گره_FTask* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
    string name() ثابت override VL_MT_STABLE { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع خالص_3() ? "black" : "red"; }
    AstCFunc* cFuncp() ثابت { أرجع m_cFuncp; }
    فراغ cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    منطقي needsNonInlineCFunc() ثابت { أرجع m_needsNonInlineCFunc; }
    فراغ needsNonInlineCFunc(منطقي flag) { m_needsNonInlineCFunc = flag; }
};

صنف Task_کد_رأس final : عام Task_پایه_رأس {
    VL_RTTI_IMPL(Task_کد_رأس, Task_پایه_رأس);
    // Top vertex for all calls not under another task
عام:
    صریح Task_کد_رأس(V3_گراف* graphp)
        : Task_پایه_رأس{graphp} {}
    ~Task_کد_رأس() override = افتراضي;
    string name() ثابت override VL_MT_STABLE { أرجع "*CODE*"; }
    string dotColor() ثابت override { أرجع "green"; }
};

صنف Task_یال final : عام V3_گراف_یال {
    VL_RTTI_IMPL(Task_یال, V3_گراف_یال)
عام:
    Task_یال(V3_گراف* graphp, Task_پایه_رأس* fromp, Task_پایه_رأس* بالاترین)
        : V3_گراف_یال{graphp, fromp, بالاترین, 1, خطأ} {}
    ~Task_یال() override = افتراضي;
    string dotLabel() ثابت override { أرجع "w" + cvt_مقصد_Str(وزن_2()); }
};

//######################################################################

صنف TaskStateVisitor final : عام VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ثابت VNUser3_درون_Use m_inuser3;
    ثابت VNUser4_درون_Use m_inuser4;

    // TYPES
    استفاده Var_مقصد_Scope_Map = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    استفاده Func_مقصد_Class_Map = std::unordered_map<ثابت Ast_گره_FTask*, AstClass*>;
    // MEMBERS
    Var_مقصد_Scope_Map m_var_مقصد_Scope_Map;  // Map for Var -> VarScope mappings
    Func_مقصد_Class_Map m_func_مقصد_Class_Map;  // Map for ctor func -> class
    Ast_گره_FTask* m_ctorp = اشاره_تهی;  // Class constructor
    AstClass* m_classp = اشاره_تهی;  // Current class
    V3_گراف m_call_گراف;  // Task call graph
    Task_پایه_رأس* m_curVxp;  // Current vertex we're adding to
    std::وکتور<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    منطقي m_under_پورت_Var = خطأ;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

عام:
    // METHODS
    ساكن منطقي is_Virtual_Iface_روش_Call(ثابت Ast_گره_FTask_Ref* refp) {
        ثابت Ast_روش_Call* ثابت mrefp = VN_CAST(refp, روش_Call);
        إذا (!mrefp) أرجع خطأ;
        ثابت Ast_گره_عبارت* ثابت fromp = mrefp->fromp();
        إذا (!fromp || !fromp->dtypep()) أرجع خطأ;
        ثابت AstIfaceRefDType* ثابت ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        أرجع ifrefp && ifrefp->isVirtual();
    }
    ساكن منطقي isIfaceFTaskScope(ثابت AstScope* scopep) {
        أرجع scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_گره_FTask* nodep) {
        AstScope* ثابت scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        أرجع scopep;
    }
    AstVarScope* پیدا_کن_Var_Scope(AstScope* scopep, AstVar* nodep) {
        ثابت خودکار iter = m_var_مقصد_Scope_Map.پیدا_کن(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_var_مقصد_Scope_Map.انتها(), nodep, "No scope for var");
        أرجع iter->دومی;
    }
    AstClass* getClassp(Ast_گره_FTask* nodep) {
        AstClass* ثابت classp = m_func_مقصد_Class_Map[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        أرجع classp;
    }
    فراغ remapFuncClassp(Ast_گره_FTask* nodep, Ast_گره_FTask* newp) {
        m_func_مقصد_Class_Map[newp] = getClassp(nodep);
    }
    منطقي ftaskNoInline(Ast_گره_FTask* nodep) { أرجع get_FTask_رأس(nodep)->noInline(); }
    منطقي ftaskNeedsNonInlineCFunc(Ast_گره_FTask* nodep) {
        أرجع get_FTask_رأس(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_گره_FTask* nodep) { أرجع get_FTask_رأس(nodep)->cFuncp(); }
    فراغ ftaskCFuncp(Ast_گره_FTask* nodep, AstCFunc* cfuncp) {
        get_FTask_رأس(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_گره* checkImpure(Ast_گره_FTask* nodep) { أرجع get_FTask_رأس(nodep)->impure_گره(); }

خاص:
    Task_FTask_رأس* get_FTask_رأس(Ast_گره_FTask* nodep) {
        إذا (!nodep->user4p()) nodep->user4p(جديد Task_FTask_رأس{&m_call_گراف, nodep});
        أرجع static_cast<Task_FTask_رأس*>(nodep->user4u().مقصد_گراف_رأس());
    }

    فراغ propagateImpureReason() {
        std::وکتور<Task_FTask_رأس*> workList;
        // Gather all impure FTask vertices
        لكل (V3_گراف_رأس& vtx : m_call_گراف.vertices()) {
            إذا (Task_FTask_رأس* ثابت ftVtxp = vtx.cast<Task_FTask_رأس>()) {
                إذا (!ftVtxp->خالص_3()) workList.emplace_پشت(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        طالما (!workList.خالی()) {
            Task_FTask_رأس* ثابت calleep = workList.پشت();
            workList.برداشت_پشت();
            UASSERT_OBJ(!calleep->خالص_3(), calleep->nodep(), "Should not enqueue pure vertex");
            لكل (V3_گراف_یال& یال : calleep->درون_یالها()) {
                إذا (Task_FTask_رأس* ثابت callerp = یال.fromp()->cast<Task_FTask_رأس>()) {
                    إذا (!callerp->خالص_3()) استمر;  // Already processed or on workList
                    callerp->impure(calleep->impure_گره());
                    workList.emplace_پشت(callerp);
                }
            }
        }
    }

    فراغ decideInlining() {
        لكل (V3_گراف_رأس& vtx : m_call_گراف.vertices()) {
            Task_FTask_رأس* ثابت ftVtxp = vtx.cast<Task_FTask_رأس>();
            إذا (!ftVtxp) استمر;
            // Move on if already decided not to inline
            إذا (ftVtxp->noInline()) استمر;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            إذا (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(درست);
                استمر;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            إذا (v3Global.opt.fInlineFuncsEager()) استمر;

            Ast_گره_FTask* ثابت ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            إذا (!ftaskp->verilogFunction()) استمر;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            إذا (ftaskp->task_عمومی()) استمر;
            // If the function is pure, it's OK not to inline, so don't do it
            إذا (ftVtxp->خالص_3()) ftVtxp->noInline(درست);
        }
    }

    // VISITORS
    فراغ بازدید(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        لكل (Ast_گره* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVarScope* ثابت vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ثابت varp = vscp->varp();
                إذا (varp->isFuncLocal() || varp->is_استفاده_شده_Loop_Idx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_var_مقصد_Scope_Map.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        لكل (Ast_گره* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_گره_FTask* ثابت taskp = VN_CAST(stmtp, گره_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        Task_FTask_رأس* ثابت taskVtxp = get_FTask_رأس(nodep->taskp());
        جديد Task_یال{&m_call_گراف, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        إذا (is_Virtual_Iface_روش_Call(nodep)) taskVtxp->needsNonInlineCFunc(درست);
        // Do we have to disable inlining the function?
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        إذا (!taskVtxp->noInline()) {  // Else short-circuit below
            لكل (ثابت خودکار& itr : tconnects) {
                ثابت AstVar* ثابت portp = itr.اولی;
                ثابت AstArg* ثابت argp = itr.دومی;
                إذا (ثابت Ast_گره_عبارت* ثابت pinp = argp->exprp()) {
                    إذا ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(درست);
                    }
                }
            }
        }
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = get_FTask_رأس(nodep);
        إذا (nodep->dpiImport()) m_curVxp->noInline(درست);
        إذا (nodep->class_روش()) m_curVxp->noInline(درست);  // Until V3Task supports it
        إذا (nodep->recursive()) m_curVxp->noInline(درست);
        إذا (nodep->isConstructor()) {
            m_curVxp->noInline(درست);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_func_مقصد_Class_Map[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstPragma* nodep) override {
        إذا (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(درست);
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ بازدید(AstVar* nodep) override {
        VL_RESTORER(m_under_پورت_Var);
        إذا (nodep->isIO()) m_under_پورت_Var = درست;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    فراغ بازدید(AstVarRef* nodep) override {
        iterateChildren(nodep);
        إذا (m_under_پورت_Var) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        إذا (nodep->access().isReadOnly() && varp->const_Pool_مدخل()) أرجع;
        إذا (varp->user4u().مقصد_گراف_رأس() != m_curVxp) {
            إذا (m_curVxp->خالص_3() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    فراغ بازدید(AstClass* nodep) override {
        // Move initial statements into the constructor
        VL_RESTORER_CLEAR(m_initialps);
        VL_RESTORER(m_ctorp);
        VL_RESTORER(m_classp);
        m_ctorp = اشاره_تهی;
        m_classp = nodep;
        {  // Find m_initialps, m_ctor
            iterateChildren(nodep);
        }
        UASSERT_OBJ(m_ctorp, nodep, "class constructor missing");  // LinkDot always makes it
        Ast_گره* insertp = اشاره_تهی;
        لكل (AstInitialAutomatic* initialp : m_initialps) {
            إذا (Ast_گره* ثابت movep = initialp->stmtsp()) {
                movep->unlink_Fr_پشت_With_بعدی();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_گره::جمع_کن_بعدی_Null(insertp, movep);
            }
            VL_DO_DANGLING(افزودن_Deletep(initialp->unlink_Fr_پشت()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    فراغ بازدید(AstInitialAutomatic* nodep) override {
        m_initialps.افزودن_پشت(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = جديد Task_کد_رأس{&m_call_گراف};
        //
        iterate(nodep);
        //
        m_call_گراف.remove_Redundant_یالها_جمع(&Task_یال::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        إذا (dump_گراف_Level()) m_call_گراف.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = افتراضي;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

بنية TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    ساكن std::وکتور<std::pair<صحيح, صحيح>> unpackDimsAndStrides(AstVar* varp) {
        Ast_گره_DType* ثابت dtypep = varp->dtypep()->skipRefp();
        std::وکتور<std::pair<صحيح, صحيح>> dimStrides;
        AstUnpackArrayDType* ثابت unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        إذا (!unpackp) أرجع dimStrides;

        ثابت std::وکتور<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ثابت اندازه_t nDims = dims.اندازه();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].اولی = dims.پشت()->elementsConst();
        dimStrides[nDims - 1].دومی = 1;
        // Rest are densly packed
        لكل (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].اولی = dims[i]->elementsConst();
            dimStrides[i].دومی = dimStrides[i + 1].اولی * dimStrides[i + 1].دومی;
        }
        أرجع dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    ساكن std::pair<std::string, منطقي> dpi_مقصد_داخلی_Cvt_Stmt(ثابت Ast_گره_DType* dtypep) {
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();

        // DPI types using Primitive C types
        إذا (basicp->keyword() == VBasicDTypeKwd::CHANDLE) أرجع {"VL_CVT_VP_Q(", خطأ};
        إذا (basicp->keyword() == VBasicDTypeKwd::STRING) أرجع {"VL_CVT_N_CSTR(", خطأ};
        إذا (basicp->isDpiPrimitive()) أرجع {"(", خطأ};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->is_Dpi_بیت_Vec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ثابت std::string vecType = basicp->is_Dpi_بیت_Vec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ثابت حرف اندازه_Char = dtypep->عرض() <= 8    ? 'C'
                              : dtypep->عرض() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ثابت std::string& اندازه = std::مقصد_string(dtypep->عرض());
        // cppcheck-suppress strPlusChar
        أرجع {"VL_SET_"s + اندازه_Char + "_" + vecType + "(" + اندازه + ", ", درست};
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
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser2_درون_Use m_inuser2;

    // TYPES
    استفاده DpiCFuncs = std::map<ثابت string, std::tuple<Ast_گره_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ثابت m_statep;  // Common state between visitors
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    Ast_بالاترین_Scope* ثابت m_بالاترین_Scopep = v3Global.rootp()->بالاترین_Scopep();  // The AstTopScope
    AstScope* m_scopep = اشاره_تهی;  // Current scope
    Ast_گره* m_insStmtp = اشاره_تهی;  // Where to insert statement
    منطقي m_درون_Sensesp = خطأ;  // Are we under a senitem?
    منطقي m_درون_New = خطأ;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    صحيح m_uncon_Var_رقم = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpi_نامها;  // Map of all created DPI functions
    std::map<std::string, ثابت AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ثابت string& name, AstVar* examplep) {
        AstVar* ثابت newvarp = جديد AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(درست);
        funcp->جمع_کن_Varsp(newvarp);
        AstVarScope* ثابت newvscp = جديد AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->جمع_کن_Varsp(newvscp);
        أرجع newvscp;
    }
    AstVarScope* create_درون_داد_Var(AstCFunc* funcp, ثابت string& name, VBasicDTypeKwd kwd) {
        AstVar* ثابت newvarp
            = جديد AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->پیدا_کن_Basic_DType(kwd)};
        newvarp->funcLocal(درست);
        newvarp->direction(VDirection::درون_داد);
        funcp->جمع_کن_Argsp(newvarp);
        AstVarScope* ثابت newvscp = جديد AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->جمع_کن_Varsp(newvscp);
        أرجع newvscp;
    }
    ساكن AstVarScope* const_Pool_جدول(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ثابت refp = V3ConstPool::پیدا_کن(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ثابت vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_درخت(), refp);
        أرجع vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ثابت Ast_گره_FTask_Ref* refp, AstVar* invarp,
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
            newvarp->is_داخلی(درست);
            أرجع newvscp;
        }
    }

    // Replace varrefs with new var pointer
    فراغ relink(Ast_گره* nodep) {
        nodep->foreach_And_بعدی([](AstVarRef* refp) {
            إذا (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ثابت newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    Ast_تخصیص_بده* connect_پورت_Make_درون_تخصیص_بده(Ast_گره_عبارت* pinp, AstVarScope* newvscp, منطقي خالص_Check) {
        // Create input assignment to go in FRONT of function call
        Ast_گره_عبارت* درون_Pinp = pinp;
        إذا (AstResizeLValue* sinPinp = VN_CAST(درون_Pinp, ResizeLValue)) درون_Pinp = sinPinp->lhsp();
        Ast_گره_عبارت* درون_Pin_Clonep
            = خالص_Check ? درون_Pinp->clone_درخت_خالص(درست) : درون_Pinp->clone_درخت(درست);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (arrayp && arrayp->is_معکوس_از(درون_Pin_Clonep->dtypep())) {
            درون_Pin_Clonep
                = جديد Ast_Cvt_Unpack_مقصد_Unpack{pinp->fileline(), درون_Pin_Clonep, newvscp->dtypep()};
        }
        Ast_تخصیص_بده* ثابت assp = جديد Ast_تخصیص_بده{
            pinp->fileline(), جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            درون_Pin_Clonep};
        assp->fileline()->modifyWarnOff(V3_Error_کد::BLKSEQ, درست);  // Ok if in <= block
        أرجع assp;
    }
    Ast_تخصیص_بده* connect_پورت_Make_بیرون_تخصیص_بده(AstVar* portp, Ast_گره_عبارت* pinp, AstVarScope* newvscp,
                                        منطقي خالص_Check) {
        // If needed, remap size of function to caller's output size
        Ast_گره_عبارت* بیرون_Pinp = pinp;
        Ast_گره_عبارت* postRhsp = جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        إذا (AstResizeLValue* soutPinp = VN_CAST(بیرون_Pinp, ResizeLValue)) {
            بیرون_Pinp = soutPinp->lhsp();
            إذا (VN_IS(بیرون_Pinp, RToIRoundS) || VN_IS(بیرون_Pinp, RToIS)) {
                بیرون_Pinp = VN_AS(بیرون_Pinp, گره_Uniop)->lhsp();
                إذا (postRhsp->isSigned()) {
                    postRhsp = جديد AstISToRD{pinp->fileline(), postRhsp};
                } وإلا {
                    postRhsp = جديد AstIToRD{pinp->fileline(), postRhsp};
                }
            } وإلا {
                إذا (Ast_گره_Uniop* aoutPinp = VN_CAST(بیرون_Pinp, Extend)) {
                    بیرون_Pinp = aoutPinp->lhsp();
                } وإلا إذا (Ast_گره_Uniop* aoutPinp = VN_CAST(بیرون_Pinp, ExtendS)) {
                    بیرون_Pinp = aoutPinp->lhsp();
                } وإلا إذا (AstSel* aoutPinp = VN_CAST(بیرون_Pinp, Sel)) {
                    بیرون_Pinp = aoutPinp->fromp();
                } وإلا {
                    بیرون_Pinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                إذا (بیرون_Pinp->عرض() < portp->عرض()) {
                    postRhsp = جديد AstSel{pinp->fileline(), postRhsp, 0, pinp->عرض()};
                } وإلا {  // pin width > port width
                    إذا (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = جديد AstExtendS{pinp->fileline(), postRhsp};
                    } وإلا {
                        postRhsp = جديد AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(بیرون_Pinp);
        }
        إذا (VN_IS(بیرون_Pinp, IToRD) || VN_IS(بیرون_Pinp, ISToRD)) {
            بیرون_Pinp = VN_AS(بیرون_Pinp, گره_Uniop)->lhsp();
            postRhsp = جديد AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(بیرون_Pinp);
        }
        // Put output assignment AFTER function call
        Ast_گره_عبارت* ثابت بیرون_Pin_Clonep
            = خالص_Check ? بیرون_Pinp->clone_درخت_خالص(درست) : بیرون_Pinp->clone_درخت(درست);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(بیرون_Pinp->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (arrayp && arrayp->is_معکوس_از(postRhsp->dtypep())) {
            postRhsp = جديد Ast_Cvt_Unpack_مقصد_Unpack{pinp->fileline(), postRhsp, بیرون_Pinp->dtypep()};
        }
        Ast_تخصیص_بده* ثابت assp = جديد Ast_تخصیص_بده{pinp->fileline(), بیرون_Pin_Clonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3_Error_کد::BLKSEQ, درست);  // Ok if in <= block
        أرجع assp;
    }

    فراغ changeAtWriteRecurse(Ast_گره_عبارت* ثابت exprp) {
        // Change nested at methods to writable variant
        إذا (AstCMethodHard* ثابت cMethodp = VN_CAST(exprp, CMethodHard)) {
            إذا (cMethodp->روش() == VCMethod::ARRAY_AT) {
                cMethodp->روش(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    فراغ connect_پورت(ثابت Ast_گره_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_گره* beginp,
                     منطقي inlineTask) {
        Ast_گره_عبارت* pinp = argp->exprp();
        إذا (inlineTask) {
            portp->unlink_Fr_پشت();
            افزودن_Deletep(portp);  // Remove it from the clone (not original)
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
                                                    + std::مقصد_string(m_uncon_Var_رقم++),
                                                portp->dtypep()};
                m_modp->جمع_کن_Stmtsp(varp);
                AstVarScope* ثابت newvscp = جديد AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->جمع_کن_Varsp(newvscp);
                AstVarRef* ثابت repp = جديد AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(افزودن_Deletep(pinp), pinp);
                pinp = repp;
            }
            إذا (inlineTask) {
                افزودن_Deletep(pinp->unlink_Fr_پشت());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlink_Fr_پشت()->delete_درخت(), argp);  // Args no longer needed
            }
            إذا (portp->isRef() || portp->isConstRef()) {
                منطقي refArgOk = خطأ;
                إذا (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = درست;
                } وإلا إذا (AstCMethodHard* ثابت cMethodp = VN_CAST(pinp, CMethodHard)) {
                    إذا (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), صف_DType)) {
                        refArgOk = cMethodp->روش() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->روش() == VCMethod::DYN_AT_WRITE_APPEND_پشت;
                    } وإلا {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->روش() == VCMethod::ARRAY_AT_WRITE;
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
                    افزودن_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                Ast_تخصیص_بده* ثابت preassp = connect_پورت_Make_درون_تخصیص_بده(pinp, newvscp, درست);
                beginp->جمع_کن_بعدی_اینجا(preassp);

                Ast_تخصیص_بده* ثابت postassp = connect_پورت_Make_بیرون_تخصیص_بده(portp, pinp, newvscp, درست);
                beginp->جمع_کن_بعدی(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } وإلا إذا (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                إذا (!inlineTask) {
                    pinp->replaceWith(جديد AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    افزودن_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                Ast_تخصیص_بده* ثابت postassp = connect_پورت_Make_بیرون_تخصیص_بده(portp, pinp, newvscp, خطأ);
                // Put assignment BEHIND of all other statements
                beginp->جمع_کن_بعدی(postassp);
            } وإلا إذا (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                Ast_تخصیص_بده* ثابت preassp = connect_پورت_Make_درون_تخصیص_بده(pinp, newvscp, خطأ);
                // Put assignment in FRONT of all other statements
                beginp->جمع_کن_بعدی_اینجا(preassp);
            }
        }
    }

    منطقي has_Ref_آرگومان(Ast_گره_FTask* nodep) {
        لكل (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isRef() || varp->isConstRef()) أرجع درست;
            }
        }
        أرجع خطأ;
    }

    Ast_گره* createInlinedFTask(Ast_گره_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_گره* ثابت newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_درخت(درست) : اشاره_تهی;
        Ast_گره* ثابت beginp
            = جديد AstComment{refp->fileline(), "Function: "s + refp->name(), درست};
        إذا (newbodysp) beginp->جمع_کن_بعدی(newbodysp);
        إذا (debug() >= 9) beginp->dump_درخت_And_بعدی(خروجی, "-  newbegi: ");
        //
        // Create input variables
        Ast_گره::user2_Clear_درخت();
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            لكل (ثابت خودکار& itr : tconnects) {
                AstVar* ثابت portp = itr.اولی;
                AstArg* ثابت argp = itr.دومی;
                connect_پورت(refp, portp, argp, beginp, درست);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_گره* nextstmtp;
            لكل (Ast_گره* stmtp = beginp; stmtp; stmtp = nextstmtp) {
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
                            Ast_گره* ثابت crstp = جديد Ast_تخصیص_بده{
                                portp->fileline(),
                                جديد AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                جديد AstCReset{portp->fileline(), portp, خطأ}};
                            portp->replaceWith(crstp);
                        } وإلا {
                            portp->unlink_Fr_پشت();
                        }
                        افزودن_Deletep(portp);  // Remove it from the clone (not original)
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
        إذا (debug() >= 9) beginp->dump_درخت_And_بعدی(خروجی, "-  iotask: ");
        أرجع beginp;
    }

    Ast_گره* createNonInlinedFTask(Ast_گره_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ثابت cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_گره* ثابت beginp
            = جديد AstComment{refp->fileline(), "Function: "s + refp->name(), درست};
        Ast_گره_CCall* ccallp;
        إذا (VN_IS(refp, New)) {
            AstCNew* ثابت cnewp = جديد AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } وإلا إذا (ثابت Ast_روش_Call* ثابت mrefp = VN_CAST(refp, روش_Call)) {
            ccallp = جديد AstCMethodCall{refp->fileline(), mrefp->fromp()->unlink_Fr_پشت(), cfuncp};
            ccallp->dtype_مجموعه_Void();
            beginp->جمع_کن_بعدی(ccallp->makeStmt());
        } وإلا {
            ccallp = جديد AstCCall{refp->fileline(), cfuncp};
            ccallp->dtype_مجموعه_Void();
            beginp->جمع_کن_بعدی(ccallp->makeStmt());
        }

        إذا (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } وإلا إذا (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } وإلا إذا (ثابت Ast_روش_Call* ثابت روش_Callp = VN_CAST(refp, روش_Call)) {
            ccallp->superReference(روش_Callp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            لكل (ثابت خودکار& itr : tconnects) {
                AstVar* ثابت portp = itr.اولی;
                AstArg* ثابت argp = itr.دومی;
                connect_پورت(refp, portp, argp, beginp, خطأ);
            }
        }
        // First argument is symbol table, then output if a function
        ثابت منطقي needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        إذا (needSyms) ccallp->argTypes("vlSymsp");

        إذا (refp->taskp()->dpiContext()) {
            AstScopeName* ثابت snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlink_Fr_پشت();
            File_سطر* ثابت flp = refp->fileline();
            // __Vscopep
            ccallp->جمع_کن_Argsp(snp);
            // __Vfilenamep
            ccallp->جمع_کن_Argsp(
                جديد AstCExpr{flp, AstCExpr::خالص{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->جمع_کن_Argsp(جديد AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        لكل (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->جمع_کن_Argsp(argp->exprp()->unlink_Fr_پشت());
        }

        إذا (outvscp) ccallp->جمع_کن_Argsp(جديد AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        إذا (debug() >= 9) beginp->dump_درخت_And_بعدی(خروجی, "-  nitask: ");
        أرجع beginp;
    }

    string dpiSignature(Ast_گره_FTask* nodep, AstVar* rtnvarp) ثابت {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        إذا (nodep->dpi_خالص()) dpiproto += "pure ";
        إذا (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(درست, درست) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string آرگومانها;
        لكل (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    إذا (آرگومانها != "") {
                        آرگومانها += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ثابت std::string dpiType = portp->dpiArgType(خطأ, خطأ);
                    dpiproto += dpiType;
                    ثابت std::string vType = portp->dtypep()->شامل_است_Unpacked_Struct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(خطأ);
                    إذا (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    آرگومانها += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        أرجع dpiproto;
    }

    ساكن فراغ checkLegalCIdentifier(Ast_گره* nodep, ثابت string& name) {
        إذا (name.انتها() != std::پیدا_کن_if(name.begin(), name.انتها(), [](حرف c) {
                أرجع !std::آیا_حرف_یا_رقم(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    ساكن Ast_گره* create_Dpi_موقت(AstVar* portp, ثابت string& suffix) {
        ثابت string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        أرجع جديد AstCStmt{portp->fileline(), stmt};
    }

    فراغ unlinkAndClone(Ast_گره_FTask* funcp, Ast_گره* nodep, منطقي with_بعدی) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        إذا (funcp->recursive()) {
            VNRelinker relinkHandle;
            إذا (with_بعدی) {
                nodep->unlink_Fr_پشت_With_بعدی(&relinkHandle);
            } وإلا {
                nodep->unlink_Fr_پشت(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_گره* ثابت newp = nodep->clone_درخت(with_بعدی);
            relinkHandle.relink(newp);
        } وإلا {
            إذا (with_بعدی) {
                nodep->unlink_Fr_پشت_With_بعدی();
            } وإلا {
                nodep->unlink_Fr_پشت();
            }
        }
    }

    ساكن Ast_گره* create_تخصیص_بده_داخلی_مقصد_Dpi(AstVar* portp, منطقي isPtr, ثابت string& frSuffix,
                                              ثابت string& مقصد_Suffix) {
        ثابت string stmt = V3Task::تخصیص_بده_داخلی_مقصد_Dpi(portp, isPtr, frSuffix, مقصد_Suffix);
        أرجع جديد AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    ساكن Ast_گره_Stmt* createDpiScalarCvt(Ast_گره_عبارت* internalp, ثابت std::string& dpi,
                                           ثابت Ast_گره_DType* dtypep, منطقي مقصد_Dpi) {
        File_سطر* ثابت flp = internalp->fileline();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        إذا (مقصد_Dpi) {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            إذا (basicp->isDpiPrimitive()) {
                cstmtp->جمع_کن(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->جمع_کن(internalp);
                cstmtp->جمع_کن(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } وإلا {
                cstmtp->جمع_کن((basicp->is_Dpi_بیت_Vec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::مقصد_string(dtypep->عرض()) + ", " + dpi + ", ");
                cstmtp->جمع_کن(internalp);
                cstmtp->جمع_کن(");");
            }
            أرجع cstmtp;
        }
        std::string cvt;
        منطقي useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_مقصد_داخلی_Cvt_Stmt(dtypep);
        إذا (useSvVec) {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            cstmtp->جمع_کن(cvt);
            cstmtp->جمع_کن(internalp);
            cstmtp->جمع_کن(", " + dpi + ");");
            أرجع cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ثابت صحيح cwidth = basicp->عرض_واژگان() * VL_EDATASIZE;
        Ast_گره_عبارت* ثابت rhsp = جديد AstCExpr{flp, cvt + dpi + ")", cwidth};
        أرجع جديد Ast_تخصیص_بده{flp, internalp, جديد AstSel{flp, rhsp, 0, dtypep->عرض()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    ساكن Ast_گره_Stmt* createDpiCvt(Ast_گره_عبارت* internalp, ثابت std::string& dpi,
                                     ثابت Ast_گره_DType* dtypep, منطقي مقصد_Dpi) {
        File_سطر* ثابت flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_گره_Stmt* stmtsp = اشاره_تهی;
        إذا (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            لكل (صحيح i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_گره_عبارت* ثابت elemp = جديد AstArraySel{flp, internalp->clone_درخت(خطأ), i};
                stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::مقصد_string(i) + "]",
                                                       arrayp->subDTypep(), مقصد_Dpi));
            }
        } وإلا إذا (dtypep->شامل_است_Unpacked_Struct()) {
            لكل (ثابت AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ثابت selp
                    = جديد AstStructSel{flp, internalp->clone_درخت(خطأ), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_گره::جمع_کن_بعدی(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), مقصد_Dpi));
            }
        } وإلا {
            أرجع createDpiScalarCvt(internalp, dpi, dtypep, مقصد_Dpi);
        }
        VL_DO_DANGLING(internalp->delete_درخت(), internalp);
        أرجع stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    ساكن Ast_گره_Stmt* createDpiArgCvt(AstVarScope* vscp, ثابت std::string& dpiName,
                                        منطقي مقصد_Dpi) {
        AstVar* ثابت varp = vscp->varp();
        File_سطر* ثابت flp = vscp->fileline();
        ثابت std::وکتور<std::pair<صحيح, صحيح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ثابت صحيح مجموع = strides.خالی() ? 1 : strides[0].اولی * strides[0].دومی;
        ثابت منطقي isStruct = varp->dtypep()->شامل_است_Unpacked_Struct();
        ثابت Ast_گره_DType* ثابت elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_گره_Stmt* stmtsp = اشاره_تهی;
        لكل (صحيح i = 0; i < مجموع; ++i) {
            Ast_گره_عبارت* internalp
                = جديد AstVarRef{flp, vscp, مقصد_Dpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            لكل (ثابت خودکار& stride : strides) {
                internalp = جديد AstArraySel(flp, internalp, (i / stride.دومی) % stride.اولی);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            إذا (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::مقصد_string(i * varp->basicp()->عرض_واژگان());
            } وإلا إذا (isStruct || !strides.خالی()) {
                dpi += "[" + std::مقصد_string(i) + "]";
            }
            stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, مقصد_Dpi));
        }
        أرجع stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_گره_FTask* ثابت nodep, AstVar* ثابت rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_سطر* ثابت flp = nodep->fileline();
        ثابت std::string cname = Ast_گره::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ثابت حرف* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ثابت std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(درست, درست) : "";
        // The function we are building
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(درست);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(درست);
        funcp->مدخل_نقطه(درست);
        funcp->isStatic(درست);
        funcp->protect(خطأ);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_بالاترین_Scopep->scopep()->جمع_کن_Blocksp(funcp);
        // Check the arguments before converting them
        إذا (!make_پورت_List(nodep, funcp)) أرجع اشاره_تهی;

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
        funcp->جمع_کن_Stmtsp(prep);
        // Static doesn't need save-restore as if below will re-fill proper value
        prep->جمع_کن("static int __Vfuncnum = -1;\n");
        // First time init (faster than what the compiler does if we did a singleton
        prep->جمع_کن("if (VL_UNLIKELY(__Vfuncnum == -1)) {\n");
        prep->جمع_کن("__Vfuncnum = Verilated::exportFuncNum(\"" + nodep->cname() + "\");\n");
        prep->جمع_کن("}\n");
        // If the find fails, it will throw an error
        prep->جمع_کن("const VerilatedScope* const __Vscopep = Verilated::dpiScope();\n");
        // If 'dpiScope()' fails and '__Vscopep' is null; the exportFind function throws an error
        // If __Vcb is null the exportFind function throws and error
        ثابت std::string cbtype
            = VIdProtect::protect(v3Global.opt.پیشوند() + "__Vcb_" + nodep->cname() + "_t");
        prep->جمع_کن(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ثابت callp = جديد AstCStmt{flp};
        ثابت خودکار جمع_کن_Func_Arg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ثابت vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(خطأ);
            portp->protect(خطأ);
            // Add argument to call
            ثابت VAccess access = portp->direction().pinAccess();
            callp->جمع_کن(", ");
            callp->جمع_کن(جديد AstVarRef{portp->fileline(), vscp, access});
            أرجع vscp;
        };
        // Call callback
        callp->جمع_کن("(*__Vcb)(");
        // First argument is the Syms
        callp->جمع_کن("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_گره* بیرون_Cvtsp = اشاره_تهی;  // Conversions of output/inout arguments, after the call
        لكل (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت portp = VN_CAST(stmtp, Var);
            إذا (!portp) استمر;
            إذا (!portp->isIO()) استمر;
            إذا (portp->isFuncReturn()) استمر;
            إذا (portp == rtnvarp) استمر;  // Handled below
            // Add argument to call
            AstVarScope* ثابت outvscp = جمع_کن_Func_Arg(portp);
            ثابت منطقي isStruct = portp->dtypep()->شامل_است_Unpacked_Struct();
            إذا (portp->isWritable()) {
                بیرون_Cvtsp = Ast_گره::جمع_کن_بعدی(
                    بیرون_Cvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), درست)
                                       : create_تخصیص_بده_داخلی_مقصد_Dpi(portp, درست, tmpSuffixp, ""));
            }
            إذا (!portp->isNonOutput()) استمر;
            // Convert input/inout arguments to dpi type
            ثابت std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(خطأ).دومی == 0
                      ? "*"
                      : "";
            funcp->جمع_کن_Stmtsp(createDpiArgCvt(outvscp, deref + portp->name(), خطأ));
        }
        // Return value argument goes last
        إذا (rtnvarp) جمع_کن_Func_Arg(rtnvarp);
        // Close statement
        callp->جمع_کن(");");
        // Call the user function
        funcp->جمع_کن_Stmtsp(callp);
        // Convert output/inout arguments back to internal type
        إذا (بیرون_Cvtsp) funcp->جمع_کن_Stmtsp(بیرون_Cvtsp);
        // Convert return value
        إذا (rtnvarp) {
            funcp->جمع_کن_Stmtsp(create_Dpi_موقت(rtnvarp, ""));
            funcp->جمع_کن_Stmtsp(create_تخصیص_بده_داخلی_مقصد_Dpi(rtnvarp, خطأ, tmpSuffixp, ""));
            ثابت std::string شاخص = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->جمع_کن_Stmtsp(جديد AstCStmt{flp, "return " + rtnvarp->name() + شاخص + ";"});
        }
        أرجع funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_گره_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ثابت string name = Ast_گره::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ثابت string rtnType = rtnvarp            ? rtnvarp->dpiArgType(درست, درست)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ثابت funcp = جديد AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(درست);
        funcp->dontCombine(درست);
        funcp->مدخل_نقطه(خطأ);
        funcp->is_روش(خطأ);
        funcp->protect(خطأ);
        funcp->dpi_خالص(nodep->dpi_خالص());

        ثابت صحيح هزینه = static_cast<صحيح>(V3Control::get_Profile_داده(funcp->name()));
        m_statHierDpisWithCosts += (هزینه != 0);
        funcp->هزینه(هزینه);

        // Add DPI Import to top, since it's a global function
        m_بالاترین_Scopep->scopep()->جمع_کن_Blocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        إذا (!make_پورت_List(nodep, funcp)) أرجع اشاره_تهی;
        أرجع funcp;
    }

    AstCFunc* getDpiFunc(Ast_گره_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ثابت string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ثابت خودکار pair = m_dpi_نامها.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, اشاره_تهی));
        إذا (pair.دومی) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ثابت funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.اولی->دومی) = funcp;
            أرجع funcp;
        } وإلا {
            // Seen this cname import before. Check if it's the same prototype.
            ثابت Ast_گره_FTask* اولی_Nodep;
            string اولی_Signature;
            AstCFunc* اولی_Funcp;
            std::tie(اولی_Nodep, اولی_Signature, اولی_Funcp) = pair.اولی->دومی;
            إذا (signature != اولی_Signature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warn_بیشتر()  //
                               << "... New signature:      " << signature << '\n'  //
                               << اولی_Nodep->warnOther()
                               << "... Original signature: " << اولی_Signature << '\n'  //
                               << اولی_Nodep->warnContextSecondary());
                أرجع اشاره_تهی;
            } وإلا {
                // Same signature, return the previously created CFunc
                أرجع اولی_Funcp;
            }
        }
    }

    ساكن منطقي make_پورت_List(Ast_گره_FTask* nodep, AstCFunc* dpip) {
        منطقي allOk = درست;
        // Copy nodep's list of function I/O to the new dpip c function
        لكل (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO()) {
                    // Move it to new function
                    AstVar* ثابت newPortp = portp->clone_درخت(خطأ);
                    newPortp->funcLocal(درست);
                    dpip->جمع_کن_Argsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    إذا (!portp->basicp() && !portp->dtypep()->شامل_است_Unpacked_Struct()) {
                        allOk = خطأ;
                        portp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: DPI argument of type "
                                << portp->dtypep()->prettyTypeName() << '\n'
                                << portp->warn_بیشتر()
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
        أرجع !name.خالی() && VString::isIdentifier(name)
               && !std::آیا_رقم(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_واژگان::isKeyword(name).خالی();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    منطقي checkDpiStruct(ثابت AstVar* portp, ثابت AstStructDType* sdtypep, منطقي nested) {
        ثابت std::string name = sdtypep->dpiName();
        إذا (!(nested && name.خالی()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(درست) << "\n"
                              << portp->warn_بیشتر()
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
            ثابت Ast_گره_DType* dtypep = itemp->subDTypep()->skipRefp();
            طالما (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ثابت Ast_گره_UOr_Struct_DType* ثابت subp = VN_CAST(dtypep, گره_UOr_Struct_DType);
            ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ثابت منطقي isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            إذا (dtypep->شامل_است_Unpacked_Struct()) {
                إذا (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), درست)) أرجع خطأ;
            } وإلا إذا (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(درست)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                أرجع خطأ;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        إذا (name.خالی()) أرجع درست;
        ثابت AstStructDType* ثابت firstp = m_dpiStructs.emplace(name, sdtypep).اولی->دومی;
        إذا (firstp->dpiTypedef() == sdtypep->dpiTypedef()) أرجع درست;
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
    منطقي check_Dpi_Struct_آرگومانها(ثابت Ast_گره_FTask* nodep) {
        منطقي allOk = درست;
        لكل (ثابت Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var);
            إذا (portp && portp->isIO() && portp->dtypep()->شامل_است_Unpacked_Struct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       خطأ)
                        && allOk;
            }
        }
        أرجع allOk;
    }

    فراغ bodyDpiImportFunc(Ast_گره_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ثابت حرف* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        إذا (v3Global.opt.profExec()) {
            cfuncp->جمع_کن_Stmtsp(AstCStmt::prof_Exec_Section_افزودن(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string آرگومانها;
        لكل (Ast_گره* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ثابت portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                إذا (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    إذا (آرگومانها != "") آرگومانها += ", ";

                    إذا (portp->isDpiOpenArray()) {
                        Ast_گره_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, صف_DType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ثابت string propName = portp->name() + "__Vopenprops";
                        ثابت string prop_کد = portp->vlPropDecl(propName);
                        cfuncp->جمع_کن_Stmtsp(جديد AstCStmt{portp->fileline(), prop_کد});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ثابت string name = portp->name() + "__Vopenarray";
                        ثابت string var_کد
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->جمع_کن_Stmtsp(جديد AstCStmt{portp->fileline(), var_کد});
                        آرگومانها += "&" + name;
                    } وإلا {
                        // Unpacked structures are passed by reference from a C array
                        ثابت منطقي isStruct = portp->dtypep()->شامل_است_Unpacked_Struct();
                        إذا (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            إذا (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) آرگومانها += "&";
                        }

                        آرگومانها += portp->name() + tmpSuffixp;

                        cfuncp->جمع_کن_Stmtsp(create_Dpi_موقت(portp, tmpSuffixp));
                        إذا (portp->isNonOutput()) {
                            cfuncp->جمع_کن_Stmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, درست)
                                    : create_تخصیص_بده_داخلی_مقصد_Dpi(portp, خطأ, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        إذا (nodep->dpiContext()) {
            ثابت string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->جمع_کن_Stmtsp(جديد AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ثابت callp = جديد AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtype_مجموعه_Void();
            callp->argTypes(آرگومانها);
            إذا (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->جمع_کن_Stmtsp(create_Dpi_موقت(rtnvscp->varp(), tmpSuffixp));
                ثابت std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ثابت cstmtp = جديد AstCStmt{nodep->fileline()};
                cstmtp->جمع_کن(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->جمع_کن(" = ");
                cstmtp->جمع_کن(callp);  // RHS
                cstmtp->جمع_کن(";");
                cfuncp->جمع_کن_Stmtsp(cstmtp);
            } وإلا {
                // Othervise just call it
                cfuncp->جمع_کن_Stmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        لكل (Ast_گره* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                portp->protect(خطأ);  // No additional exposure - already part of shown proto
                إذا (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ثابت portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->جمع_کن_Stmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, خطأ));
                }
            }
        }

        إذا (v3Global.opt.profExec()) {
            cfuncp->جمع_کن_Stmtsp(AstCStmt::prof_Exec_Section_برداشت(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        إذا (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_بالاترین_Scopep->create_موقت("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        أرجع dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_گره_FTask* nodep, منطقي ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_گره::user2_Clear_درخت();
        AstVar* rtnvarp = اشاره_تهی;
        إذا (nodep->isFunction()) {
            AstVar* ثابت portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            إذا (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ثابت bdtypep = portp->dtypep()->basicp();
                إذا (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(درست)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع اشاره_تهی;
                }
                إذا (!bdtypep->isDpiPrimitive()) {
                    إذا (bdtypep->is_Dpi_بیت_Vec() && portp->عرض() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warn_بیشتر()
                                       + "... Suggest make it an output argument instead?");
                    }
                    إذا (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } وإلا إذا (nodep->task_عمومی()) {
                إذا (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warn_بیشتر()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            إذا (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(خطأ);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, خطأ);
            rtnvarp = portp;
            rtnvarp->funcLocal(درست);
            rtnvarp->noCReset(درست);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            إذا (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(خطأ);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = اشاره_تهی;
        إذا (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpi_Open_والد() && nodep->dpi_Open_فرزند()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = check_Dpi_Struct_آرگومانها(nodep) ? getDpiFunc(nodep, rtnvarp) : اشاره_تهی;
            إذا (!dpiFuncp || (nodep->dpiImport() && nodep->dpi_Open_والد())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                إذا (rtnvarp) VL_DO_DANGLING(افزودن_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع اشاره_تهی;
            }
        }

        AstVarScope* rtnvscp = اشاره_تهی;
        إذا (rtnvarp) {
            rtnvscp = جديد AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->جمع_کن_Varsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string پیشوند;
        إذا (nodep->dpiImport()) {
            پیشوند = "__Vdpiimwrap_";
        } وإلا إذا (nodep->dpiExport()) {
            پیشوند = "__Vdpiexp_";
        } وإلا إذا (ftaskNoInline) {
            پیشوند = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        إذا (!nodep->task_عمومی() && !nodep->class_روش()) suffix = "_" + m_scopep->nameDotless();
        ثابت string name = ((nodep->name() == "new") ? "new" : پیشوند + nodep->name() + suffix);
        AstCFunc* ثابت cfuncp = جديد AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->task_عمومی() && rtnvarp) ? rtnvarp->cPubArgType(درست, درست) : "")};
        // It's ok to combine imports because this is just a wrapper;
        // duplicate wrappers can get merged.
        cfuncp->dontCombine(!nodep->dpiImport());
        cfuncp->مدخل_نقطه(!nodep->dpiImport());
        cfuncp->func_عمومی(nodep->task_عمومی());
        cfuncp->dpiContext(nodep->dpiContext());
        cfuncp->dpiExportImpl(nodep->dpiExport());
        cfuncp->dpiImportWrapper(nodep->dpiImport());
        cfuncp->recursive(nodep->recursive());
        // Hardcoded based on UVM usage; TODO make a verilated_std.vlt control for these
        cfuncp->unlikely(nodep->name() == "uvm_report_error" || nodep->name() == "uvm_report_info"
                         || nodep->name() == "uvm_report_fatal"
                         || nodep->name() == "uvm_report_warning");
        إذا (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(درست);
            cfuncp->isLoose(درست);
        } وإلا {
            cfuncp->isStatic(خطأ);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpi_خالص(nodep->dpi_خالص());
        إذا (nodep->name() == "new") cfuncp->isConstructor(درست);
        إذا (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(درست);
        إذا (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        إذا (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ثابت منطقي needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->task_عمومی()) || v3Global.opt.profExec());
        إذا (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        إذا (!nodep->dpiImport() && !nodep->task_عمومی()) {
            // Need symbol table
            إذا (cfuncp->isConstructor()) {
                منطقي isInterfaceClass = خطأ;
                إذا (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                إذا (!isInterfaceClass) {
                    ثابت string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->جمع_کن_Stmtsp(
                        جديد AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        إذا (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_درون_داد_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_درون_داد_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_درون_داد_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        إذا (nodep->dpiExport()) {
            AstScopeName* ثابت snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(درست);
            snp->unlink_Fr_پشت();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        لكل (Ast_گره *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlink_Fr_پشت();
                    افزودن_Deletep(portp);
                    portp->user2p(const_Pool_جدول(portp));
                } وإلا {
                    إذا (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, خطأ);
                        portp->funcLocal(درست);
                        إذا (portp->valuep()) افزودن_Deletep(portp->valuep()->unlink_Fr_پشت());
                        cfuncp->جمع_کن_Argsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        إذا (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_گره_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                            إذا (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } وإلا {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(درست);
                    }
                    AstVarScope* ثابت newvscp
                        = جديد AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->جمع_کن_Varsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        إذا (rtnvarp) cfuncp->جمع_کن_Argsp(rtnvarp);

        // Move body
        Ast_گره* bodysp = nodep->stmtsp();
        إذا (bodysp) {
            unlinkAndClone(nodep, bodysp, درست);
            AstBegin* ثابت tempp
                = جديد AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, خطأ};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            لكل (Ast_گره *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    إذا (portp->isIO()) VL_DO_DANGLING(افزودن_Deletep(portp->unlink_Fr_پشت()), portp);
                }
            }
            إذا (tempp->stmtsp()) cfuncp->جمع_کن_Stmtsp(tempp->stmtsp()->unlink_Fr_پشت_With_بعدی());
            VL_DO_DANGLING(tempp->delete_درخت(), tempp);
        }
        إذا (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        إذا (rtnvscp && nodep->task_عمومی()) {
            cfuncp->جمع_کن_Stmtsp(جديد AstCReturn{
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
                    varp->مجموعه_Written_By_Dpi();
                    // Remember we had some
                    writesNonLocals = درست;
                }
                // Check if read
                إذا (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->مجموعه_Read_By_Dpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            إذا (writesNonLocals) {
                AstVarScope* ثابت dpiExportTriggerp = getDpiExporTrigger();
                File_سطر* ثابت flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                Ast_تخصیص_بده* ثابت assignp
                    = جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    جديد AstConst{flp, AstConst::بیت_True{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                إذا (v3Global.opt.systemC()) {
                    assignp->جمع_کن_بعدی_اینجا(جديد AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        إذا (nodep->needProcess()) cfuncp->مجموعه_Need_Process();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        أرجع cfuncp;
    }

    فراغ iterateIntoFTask(Ast_گره_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    فراغ درج_Before_Stmt(Ast_گره* nodep, Ast_گره* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->جمع_کن_اینجا_This_As_بعدی(newp);
    }
    فراغ process_آرگومانها(Ast_گره_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ثابت arrayp = VN_CAST(argp->exprp(), ConsArray);
            إذا (!arrayp) استمر;

            File_سطر* ثابت flp = arrayp->fileline();
            AstVarScope* ثابت substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->is_داخلی(درست);

            Ast_تخصیص_بده* ثابت assignp
                = جديد Ast_تخصیص_بده{flp, جديد AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlink_Fr_پشت()};

            Ast_عبارت_Stmt* ثابت exprstmtp = جديد Ast_عبارت_Stmt{
                flp, assignp, جديد AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    منطقي isIfaceLocalImpure(Ast_گره_FTask* nodep, Ast_گره* impurep) {
        ثابت AstScope* ثابت scopep = m_statep->getScope(nodep);
        إذا (!TaskStateVisitor::isIfaceFTaskScope(scopep)) أرجع خطأ;

        ثابت AstVarRef* ثابت refp = VN_CAST(impurep, VarRef);
        إذا (!refp || !refp->varScopep()) أرجع خطأ;

        أرجع refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    فراغ بازدید(AstWith* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) {
            // Make sure that the return expression is converted only once
            أرجع;
        }
        Ast_گره_عبارت* ثابت withExprp = VN_AS(nodep->exprp()->unlink_Fr_پشت(), گره_عبارت);
        nodep->جمع_کن_Exprp(جديد AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    فراغ بازدید(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    فراغ بازدید(AstCNew* nodep) override {
        VL_RESTORER(m_درون_New);
        m_درون_New = درست;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        إذا (m_درون_Sensesp && !nodep->is_خالص()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(اشاره_تهی);  // So V3Broken doesn't complain
            أرجع;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        إذا (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_شمار(), nodep, "No non-inline task, but no error issued");
            إذا (VN_IS(nodep->backp(), Stmt_عبارت)) {
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
            } وإلا {
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
            أرجع;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = اشاره_تهی;
        إذا (nodep->taskp()->isFunction()) {
            AstVar* ثابت fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            إذا (Ast_تخصیص_بده* ثابت assignp = VN_CAST(nodep->backp(), تخصیص_بده)) {
                إذا (AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ثابت vscp = lhsp->varScopep();
                    إذا (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_درخت(
                            fvarp->dtypep()->skipRefp())
                        && !has_Ref_آرگومان(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            إذا (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_گره* beginp;
        AstCNew* cnewp = اشاره_تهی;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ثابت منطقي virtualIfaceCall
            = TaskStateVisitor::is_Virtual_Iface_روش_Call(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        إذا (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            process_آرگومانها(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } وإلا {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        إذا (VN_IS(nodep, New)) {  // New not legal as while() condition
            درج_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (VN_IS(nodep->backp(), گره_تخصیص_بده)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            درج_Before_Stmt(nodep, beginp);
            AstVarRef* ثابت outrefp = جديد AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (!VN_IS(nodep->backp(), Stmt_عبارت)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ثابت outrefp = جديد AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_عبارت_Stmt* lambdap = جديد Ast_عبارت_Stmt{nodep->fileline(), beginp, outrefp};

            إذا (m_درون_New) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_درخت(خطأ);
                newvarp->funcLocal(درست);

                // Create a new VarScope for the new variable
                AstVarScope* ثابت newvscp
                    = جديد AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->جمع_کن_Varsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreach_And_بعدی([outvscp, newvscp](AstVarRef* refp) {
                    إذا (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->has_نتیجه(خطأ);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ثابت exprp = جديد AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtype_مجموعه_String();
                lambdap->جمع_کن_Stmtsp(جديد AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            VIsCached::clear_Cache_درخت();
        } وإلا {  // VN_IS(nodep->backp(), StmtExpr)
            درج_Before_Stmt(nodep, beginp);
            إذا (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_کد::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlink_Fr_پشت();
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    فراغ بازدید(Ast_گره_FTask* ثابت nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        إذا (!nodep->user1_مجموعه_Once()) {  // Just one creation needed per function
            // Expand functions in it
            صحيح modes = 0;
            إذا (nodep->dpiImport()) modes++;
            إذا (nodep->dpiExport()) modes++;
            إذا (nodep->task_عمومی()) modes++;
            إذا (nodep->class_روش()) modes++;
            إذا (v3Global.opt.protectIds() && nodep->task_عمومی()) {
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
            إذا (noInline && !nodep->class_روش() && !nodep->recursive()) {
                إذا (Ast_گره* ثابت impurep = m_statep->checkImpure(nodep)) {
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
                || nodep->task_عمومی()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_گره_FTask* ثابت clonedFuncp = nodep->clone_درخت(خطأ);
                إذا (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ثابت منطقي makeNonInlineFunc = noInline || needsNonInlineCFunc;
                إذا (AstCFunc* ثابت cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->جمع_کن_بعدی_اینجا(cfuncp);
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
                    AstVarScope* ثابت vscp = m_statep->پیدا_کن_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(افزودن_Deletep(vscp->unlink_Fr_پشت()), vscp);
                }
            }
            لكل (Ast_گره *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ثابت vscp = m_statep->پیدا_کن_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(افزودن_Deletep(vscp->unlink_Fr_پشت()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlink_Fr_پشت();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    فراغ بازدید(Ast_تخصیص_بده_Force* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::وکتور<Ast_گره_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_گره_FTask_Ref* refp) { refs.افزودن_پشت(refp); });
        لكل (Ast_گره_FTask_Ref* ثابت refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ثابت interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            Ast_تخصیص_بده* ثابت assignp = جديد Ast_تخصیص_بده{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_درخت_خالص(خطأ)};
            Ast_Sen_درخت* ثابت senTreep = جديد Ast_Sen_درخت{
                nodep->fileline(), جديد Ast_Sen_قلم{nodep->fileline(), Ast_Sen_قلم::Combo{}}};
            Ast_فعال* ثابت activep
                = جديد Ast_فعال{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_درخت_Storep(activep->sentreep());
            activep->جمع_کن_Stmtsp(
                جديد AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, اشاره_تهی, assignp});
            m_scopep->جمع_کن_Blocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(جديد AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_درخت(), refp);
        }
    }
    فراغ بازدید(Ast_گره_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_Stmt_عبارت* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->exprp()) VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
    }
    فراغ بازدید(Ast_Sen_قلم* nodep) override {
        UASSERT_OBJ(!m_درون_Sensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_درون_Sensesp);
        m_درون_Sensesp = درست;
        iterateChildren(nodep);
    }
    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    TaskVisitor(AstNetlist* nodep, TaskStateVisitor* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~TaskVisitor() {
        V3Stats::جمع_کن_Stat("Optimizations, Functions inlined", m_statInlines);
        V3Stats::جمع_کن_Stat("Optimizations, Hierarchical DPI wrappers with costs",
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
    فراغ markSensesAndIterate(Ast_گره* nodep) {
        إذا (!m_underIfaceFunc) أرجع;
        VL_RESTORER(m_underSenses);
        m_underSenses = درست;
        iterate_And_بعدی_Const_Null(nodep);
    }
    // VISITORS
    فراغ بازدید(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_Sen_درخت* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    فراغ بازدید(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
    }
    فراغ بازدید(AstVarRef* nodep) override {
        إذا (!m_underSenses) أرجع;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        إذا (nodep->varp()->isFuncLocal()) أرجع;
        إذا (AstIface* ثابت ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    صریح TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

ثابت حرف* ثابت V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_گره_FTask_Ref* nodep, Ast_گره* taskStmtsp,
                                    V3TaskConnectState* statep, منطقي makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ثابت std::string, صحيح> name_مقصد_شاخص;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    صحيح tpinnum = 0;
    AstVar* sformatp = اشاره_تهی;
    لكل (Ast_گره* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            إذا (portp->isIO()) {
                tconnects.emplace_پشت(portp, static_cast<AstArg*>(اشاره_تهی));
                name_مقصد_شاخص.emplace(portp->name(), tpinnum);  // For name based connections
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
        إذا (!argp->name().خالی()) {
            // By name
            ثابت خودکار it = name_مقصد_شاخص.پیدا_کن(argp->name());
            إذا (it == name_مقصد_شاخص.انتها()) {
                إذا (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warn_بیشتر() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_پشت()->delete_درخت(), argp);
                }
            } وإلا {
                إذا (tconnects[it->دومی].دومی && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->دومی].دومی->unlink_Fr_پشت()->delete_درخت();
                }
                tconnects[it->دومی].دومی = argp;
                reorganize = درست;
            }
        } وإلا {  // By pin number
            إذا (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } وإلا إذا (ppinnum >= tpinnum) {
                إذا (sformatp) {
                    tconnects.emplace_پشت(sformatp, static_cast<AstArg*>(اشاره_تهی));
                    tconnects[ppinnum].دومی = argp;
                    ++tpinnum;
                } وإلا إذا (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warn_بیشتر() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_پشت()->delete_درخت(), argp);
                }
            } وإلا {
                tconnects[ppinnum].دومی = argp;
            }
        }
        ++ppinnum;
    }

    إذا (!makeChanges) أرجع tconnects;

    // Connect missing ones
    VInsertion_مجموعه<ثابت AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    لكل (صحيح i = 0; i < tpinnum; ++i) {
        AstVar* ثابت portp = tconnects[i].اولی;
        إذا (!tconnects[i].دومی || !tconnects[i].دومی->exprp()) {
            Ast_گره_عبارت* newvaluep = اشاره_تهی;
            إذا (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = جديد AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } وإلا إذا (AstFuncRef* ثابت funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ثابت Ast_گره_FTask* ثابت funcp = funcRefp->taskp();
                إذا (funcp->class_روش() && funcp->isStatic()) newvaluep = funcRefp;
            } وإلا إذا (AstConst* ثابت constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            إذا (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), گره_عبارت));
                إذا (!VN_IS(newvaluep, Const)) {
                    إذا (statep) {
                        portp->pin_رقم(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.درج(portp);
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
            newvaluep = newvaluep->backp() ? newvaluep->clone_درخت(درست) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ثابت newp = جديد AstArg{nodep->fileline(), portp->name(), newvaluep};
            إذا (tconnects[i].دومی) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].دومی->unlink_Fr_پشت()->delete_درخت(),
                            tconnects[i].دومی = اشاره_تهی);
            }
            tconnects[i].دومی = newp;
            reorganize = درست;
        }
        إذا (tconnects[i].دومی) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].دومی);
        } وإلا {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    لكل (ثابت خودکار& tconnect : tconnects) {
        AstArg* ثابت argp = tconnect.دومی;
        argp->name("");  // Can forget name as will add back in pin order
    }

    إذا (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        طالما (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlink_Fr_پشت();
        }
        لكل (صحيح i = 0; i < tpinnum; ++i) {
            AstArg* ثابت argp = tconnects[i].دومی;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->جمع_کن_Argsp(argp);
        }
    }

    إذا (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_درخت("-  ftref-out: ");
        لكل (صحيح i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvt_مقصد_Hex(tconnects[i].اولی)
                               << "  conn=" << cvt_مقصد_Hex(tconnects[i].دومی));
        }
    }  // LCOV_EXCL_STOP

    إذا (!argWrap.خالی()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        أرجع taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            اشاره_تهی);
    }
    أرجع tconnects;
}

فراغ V3Task::taskConnectWrap(Ast_گره_FTask_Ref* nodep, ثابت V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ثابت VInsertion_مجموعه<ثابت AstVar*>& argWrap) {
    statep->مجموعه_Did_Wrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    لكل (ثابت AstVar* varp : argWrap) newname += "_" + cvt_مقصد_Str(varp->pin_رقم());
    ثابت خودکار pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(اشاره_تهی));
    إذا (pair.دومی) {
        pair.اولی->دومی = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_گره_FTask* ثابت newTaskp = pair.اولی->دومی;

    // Remove the defaulted arguments from original outside call
    لكل (ثابت خودکار& tconnect : tconnects) {
        ثابت AstVar* ثابت portp = tconnect.اولی;
        AstArg* ثابت argp = tconnect.دومی;
        إذا (argWrap.exists(portp)) {  // Removed arg
            statep->افزودن_Deletep(argp->unlink_Fr_پشت());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_گره_FTask* V3Task::taskConnectWrapNew(Ast_گره_FTask* taskp, ثابت string& newname,
                                         ثابت V3TaskConnects& tconnects,
                                         ثابت VInsertion_مجموعه<ثابت AstVar*>& argWrap) {
    std::map<ثابت AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_گره_FTask* ثابت newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->جمع_کن_بعدی_اینجا(newTaskp);

    Ast_گره_FTask_Ref* newCallp = اشاره_تهی;
    Ast_گره* newCallInsertp = اشاره_تهی;
    إذا (VN_IS(taskp, Func)) {
        AstVar* ثابت fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ثابت newFVarp = fvarp->clone_درخت(درست);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = جديد AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = جديد Ast_تخصیص_بده{taskp->fileline(),
                            جديد AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } وإلا إذا (VN_IS(taskp, Task)) {
        newCallp = جديد AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = جديد Ast_Stmt_عبارت{taskp->fileline(), newCallp};
    } وإلا {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    لكل (ثابت خودکار& tconnect : tconnects) {
        AstVar* ثابت portp = tconnect.اولی;
        AstVar* newPortp;
        إذا (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = جديد AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(درست);
            إذا (newPortp->valuep()) newPortp->valuep()->unlink_Fr_پشت()->delete_درخت();
            newTaskp->جمع_کن_Stmtsp(newPortp);
        } وإلا {  // Defaulting arg
            Ast_گره_عبارت* ثابت valuep = VN_AS(portp->valuep(), گره_عبارت);
            إذا ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ثابت VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ثابت refp = VN_AS(valuep->clone_درخت(خطأ), VarRef);
                refp->access(refAccess);
                AstArg* ثابت newArgp = جديد AstArg{portp->fileline(), portp->name(), refp};
                newCallp->جمع_کن_Argsp(newArgp);
                استمر;
            }
            // Create local temporary
            newPortp = جديد AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(درست);
            newTaskp->جمع_کن_Stmtsp(newPortp);
            // Runtime-assign it to the default
            إذا (!VN_IS(valuep, خالی_صف)) {
                Ast_تخصیص_بده* ثابت newAssignp
                    = جديد Ast_تخصیص_بده{valuep->fileline(),
                                    جديد AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_درخت(درست)};
                newTaskp->جمع_کن_Stmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ثابت newArgp = جديد AstArg{
            portp->fileline(), portp->name(),
            جديد AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->جمع_کن_Argsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->جمع_کن_Stmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ثابت خودکار it = oldNewVars.پیدا_کن(refp->varp());
        إذا (it != oldNewVars.انتها()) refp->varp(it->دومی);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    أرجع newTaskp;
}

string V3Task::تخصیص_بده_داخلی_مقصد_Dpi(AstVar* portp, منطقي isPtr, ثابت string& frSuffix,
                                   ثابت string& مقصد_Suffix, ثابت string& fr_پیشوند) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ثابت string frName = fr_پیشوند + portp->name() + frSuffix;
    ثابت string مقصد_Name = portp->name() + مقصد_Suffix;
    ثابت string idx = portp->name() + "__Vidx";
    اندازه_t unpack_اندازه = 1;  // non-unpacked array is treated as size 1
    صحيح unpackDim = 0;
    إذا (AstUnpackArrayDType* ثابت unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpack_اندازه = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(خطأ).دومی;
        إذا (unpackDim > 0) UASSERT_OBJ(unpack_اندازه > 0, portp, "size must be greater than 0");
    }
    إذا (portp->basicp()->is_Dpi_بیت_Vec() || portp->basicp()->isDpiLogicVec()) {
        ثابت منطقي is_بیت = portp->basicp()->is_Dpi_بیت_Vec();
        ثابت منطقي needsFor = unpack_اندازه > 1;
        إذا (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvt_مقصد_Str(unpack_اندازه) + "; ++"
                   + idx + ") ";
        }

        stmt += (is_بیت ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvt_مقصد_Str(portp->عرض()) + ", ";
        stmt += مقصد_Name;
        إذا (needsFor) {
            stmt += " + " + cvt_مقصد_Str(portp->dtypep()->skipRefp()->عرض_واژگان()) + " * " + idx;
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
                                             + cvt_مقصد_Str(unpack_اندازه) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + مقصد_Name + unpackAt;
        } وإلا {
            إذا (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += مقصد_Name;
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
std::string V3Task::تخصیص_بده_Dpi_مقصد_داخلی(ثابت std::string& lhsName, AstVar* varp) {
    std::string cvt;
    منطقي useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_مقصد_داخلی_Cvt_Stmt(varp->dtypep());
    ثابت std::وکتور<std::pair<صحيح, صحيح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ثابت صحيح مجموع = strides.خالی() ? 1 : strides[0].اولی * strides[0].دومی;
    ثابت صحيح عرض_واژگان = varp->basicp()->عرض_واژگان();
    string statements;
    لكل (صحيح i = 0; i < مجموع; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        لكل (ثابت خودکار& stride : strides) {
            lhs += "[" + std::مقصد_string((i / stride.دومی) % stride.اولی) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        إذا (useSvVec) {
            ثابت std::string جابه_جایی = std::مقصد_string(i * عرض_واژگان);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + جابه_جایی + ");\n";
        } وإلا {
            ثابت std::string elem = strides.خالی() ? "" : "[" + std::مقصد_string(i) + "]";
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
    V3Global::dump_Check_Global_درخت("task", 0, dump_درخت_Either_Level() >= 3);
}
