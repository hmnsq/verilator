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

جماعت Task_بنیاد_راس VL_NOT_FINAL : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Task_بنیاد_راس, V3_گراف_راس);
    Ast_گرہ* m_impurep = خالی_اشاریہ;  // Node causing impure function w/ outside references
    منطقی m_noInline = جھوٹ;  // Marked with pragma
عوامی:
    صریح Task_بنیاد_راس(V3_گراف* graphp)
        : V3_گراف_راس{graphp} {}
    ~Task_بنیاد_راس() override = طےشدہ;
    منطقی خالص_3() ثابت { واپس m_impurep == خالی_اشاریہ; }
    Ast_گرہ* impure_گرہ() ثابت { واپس m_impurep; }
    خالی impure(Ast_گرہ* nodep) { m_impurep = nodep; }
    منطقی noInline() ثابت { واپس m_noInline; }
    خالی noInline(منطقی flag) { m_noInline = flag; }
};

جماعت Task_FTask_راس final : عوامی Task_بنیاد_راس {
    VL_RTTI_IMPL(Task_FTask_راس, Task_بنیاد_راس);
    // Every task gets a vertex, and we link tasks together based on funcrefs.
    Ast_گرہ_FTask* ثابت m_nodep;
    AstCFunc* m_cFuncp = خالی_اشاریہ;
    منطقی m_needsNonInlineCFunc = جھوٹ;  // Needs CFunc for selected non-inlined call sites

عوامی:
    Task_FTask_راس(V3_گراف* graphp, Ast_گرہ_FTask* nodep)
        : Task_بنیاد_راس{graphp}
        , m_nodep{nodep} {}
    ~Task_FTask_راس() override = طےشدہ;
    Ast_گرہ_FTask* nodep() ثابت VL_MT_STABLE { واپس m_nodep; }
    string name() ثابت override VL_MT_STABLE { واپس nodep()->name(); }
    string dotColor() ثابت override { واپس خالص_3() ? "black" : "red"; }
    AstCFunc* cFuncp() ثابت { واپس m_cFuncp; }
    خالی cFuncp(AstCFunc* nodep) { m_cFuncp = nodep; }
    منطقی needsNonInlineCFunc() ثابت { واپس m_needsNonInlineCFunc; }
    خالی needsNonInlineCFunc(منطقی flag) { m_needsNonInlineCFunc = flag; }
};

جماعت Task_رمز_راس final : عوامی Task_بنیاد_راس {
    VL_RTTI_IMPL(Task_رمز_راس, Task_بنیاد_راس);
    // Top vertex for all calls not under another task
عوامی:
    صریح Task_رمز_راس(V3_گراف* graphp)
        : Task_بنیاد_راس{graphp} {}
    ~Task_رمز_راس() override = طےشدہ;
    string name() ثابت override VL_MT_STABLE { واپس "*CODE*"; }
    string dotColor() ثابت override { واپس "green"; }
};

جماعت Task_کنارہ final : عوامی V3_گراف_کنارہ {
    VL_RTTI_IMPL(Task_کنارہ, V3_گراف_کنارہ)
عوامی:
    Task_کنارہ(V3_گراف* graphp, Task_بنیاد_راس* fromp, Task_بنیاد_راس* چوٹی)
        : V3_گراف_کنارہ{graphp, fromp, چوٹی, 1, جھوٹ} {}
    ~Task_کنارہ() override = طےشدہ;
    string dotLabel() ثابت override { واپس "w" + cvt_مقصد_Str(وزن_2()); }
};

//######################################################################

جماعت TaskStateVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  Output:
    //   AstNodeFTask::user3p   // AstScope* this FTask is under
    //   AstNodeFTask::user4p   // GraphFTaskVertex* this FTask is under
    //   AstVar::user4p         // GraphFTaskVertex* this variable is declared in
    ثابت VNUser3_اندر_Use m_inuser3;
    ثابت VNUser4_اندر_Use m_inuser4;

    // TYPES
    استعمال Var_مقصد_Scope_Map = std::map<std::pair<AstScope*, AstVar*>, AstVarScope*>;
    استعمال Func_مقصد_Class_Map = std::unordered_map<ثابت Ast_گرہ_FTask*, AstClass*>;
    // MEMBERS
    Var_مقصد_Scope_Map m_var_مقصد_Scope_Map;  // Map for Var -> VarScope mappings
    Func_مقصد_Class_Map m_func_مقصد_Class_Map;  // Map for ctor func -> class
    Ast_گرہ_FTask* m_ctorp = خالی_اشاریہ;  // Class constructor
    AstClass* m_classp = خالی_اشاریہ;  // Current class
    V3_گراف m_call_گراف;  // Task call graph
    Task_بنیاد_راس* m_curVxp;  // Current vertex we're adding to
    std::سمتیہ<AstInitialAutomatic*> m_initialps;  // Initial blocks to move
    منطقی m_under_پورٹ_Var = جھوٹ;  // Visiting under a port AstVar; any expression there
                                  // is a default value, evaluated at call sites only

عوامی:
    // METHODS
    ساکن منطقی is_Virtual_Iface_طریقہ_Call(ثابت Ast_گرہ_FTask_Ref* refp) {
        ثابت Ast_طریقہ_Call* ثابت mrefp = VN_CAST(refp, طریقہ_Call);
        اگر (!mrefp) واپس جھوٹ;
        ثابت Ast_گرہ_اظہاریہ* ثابت fromp = mrefp->fromp();
        اگر (!fromp || !fromp->dtypep()) واپس جھوٹ;
        ثابت AstIfaceRefDType* ثابت ifrefp = VN_CAST(fromp->dtypep()->skipRefp(), IfaceRefDType);
        واپس ifrefp && ifrefp->isVirtual();
    }
    ساکن منطقی isIfaceFTaskScope(ثابت AstScope* scopep) {
        واپس scopep && VN_IS(scopep->modp(), Iface);
    }
    AstScope* getScope(Ast_گرہ_FTask* nodep) {
        AstScope* ثابت scopep = VN_AS(nodep->user3p(), Scope);
        UASSERT_OBJ(scopep, nodep, "No scope for function");
        واپس scopep;
    }
    AstVarScope* ڈھونڈو_Var_Scope(AstScope* scopep, AstVar* nodep) {
        ثابت خودکار iter = m_var_مقصد_Scope_Map.ڈھونڈو(std::make_pair(scopep, nodep));
        UASSERT_OBJ(iter != m_var_مقصد_Scope_Map.انتہا(), nodep, "No scope for var");
        واپس iter->دوسرا;
    }
    AstClass* getClassp(Ast_گرہ_FTask* nodep) {
        AstClass* ثابت classp = m_func_مقصد_Class_Map[nodep];
        UASSERT_OBJ(classp, nodep, "No class for ctor func");
        واپس classp;
    }
    خالی remapFuncClassp(Ast_گرہ_FTask* nodep, Ast_گرہ_FTask* newp) {
        m_func_مقصد_Class_Map[newp] = getClassp(nodep);
    }
    منطقی ftaskNoInline(Ast_گرہ_FTask* nodep) { واپس get_FTask_راس(nodep)->noInline(); }
    منطقی ftaskNeedsNonInlineCFunc(Ast_گرہ_FTask* nodep) {
        واپس get_FTask_راس(nodep)->needsNonInlineCFunc();
    }
    AstCFunc* ftaskCFuncp(Ast_گرہ_FTask* nodep) { واپس get_FTask_راس(nodep)->cFuncp(); }
    خالی ftaskCFuncp(Ast_گرہ_FTask* nodep, AstCFunc* cfuncp) {
        get_FTask_راس(nodep)->cFuncp(cfuncp);
    }
    // Returns pointer to node that makes the given FTask impure
    Ast_گرہ* checkImpure(Ast_گرہ_FTask* nodep) { واپس get_FTask_راس(nodep)->impure_گرہ(); }

نجی:
    Task_FTask_راس* get_FTask_راس(Ast_گرہ_FTask* nodep) {
        اگر (!nodep->user4p()) nodep->user4p(نیا Task_FTask_راس{&m_call_گراف, nodep});
        واپس static_cast<Task_FTask_راس*>(nodep->user4u().مقصد_گراف_راس());
    }

    خالی propagateImpureReason() {
        std::سمتیہ<Task_FTask_راس*> workList;
        // Gather all impure FTask vertices
        برائے (V3_گراف_راس& vtx : m_call_گراف.vertices()) {
            اگر (Task_FTask_راس* ثابت ftVtxp = vtx.cast<Task_FTask_راس>()) {
                اگر (!ftVtxp->خالص_3()) workList.emplace_عقب(ftVtxp);
            }
        }
        // Mark every pure caller as impure and enqueue them - this propagates impureness
        جب (!workList.خالی_خانے()) {
            Task_FTask_راس* ثابت calleep = workList.عقب();
            workList.نکالو_عقب();
            UASSERT_OBJ(!calleep->خالص_3(), calleep->nodep(), "Should not enqueue pure vertex");
            برائے (V3_گراف_کنارہ& کنارہ : calleep->اندر_کنارے()) {
                اگر (Task_FTask_راس* ثابت callerp = کنارہ.fromp()->cast<Task_FTask_راس>()) {
                    اگر (!callerp->خالص_3()) جاری;  // Already processed or on workList
                    callerp->impure(calleep->impure_گرہ());
                    workList.emplace_عقب(callerp);
                }
            }
        }
    }

    خالی decideInlining() {
        برائے (V3_گراف_راس& vtx : m_call_گراف.vertices()) {
            Task_FTask_راس* ثابت ftVtxp = vtx.cast<Task_FTask_راس>();
            اگر (!ftVtxp) جاری;
            // Move on if already decided not to inline
            اگر (ftVtxp->noInline()) جاری;
            // If inlining blanked disabled, don't do it. Note this usually
            // results in large amounts of breakage so is mostly for testing.
            اگر (!v3Global.opt.fInlineFuncs()) {
                ftVtxp->noInline(سچ);
                جاری;
            }

            // If eagerly inlining (historic - and still default - behaviour) then nothing to do
            اگر (v3Global.opt.fInlineFuncsEager()) جاری;

            Ast_گرہ_FTask* ثابت ftaskp = ftVtxp->nodep();
            // We are relying on inlining of some internal functions for
            // functional corretness. Probably that's not a good thing?
            اگر (!ftaskp->verilogFunction()) جاری;
            // Non-inlned public functions are a bit broken right now, so keep
            // inlining them (internal calls are inlined + keep public version)
            اگر (ftaskp->task_عوامی()) جاری;
            // If the function is pure, it's OK not to inline, so don't do it
            اگر (ftVtxp->خالص_3()) ftVtxp->noInline(سچ);
        }
    }

    // VISITORS
    خالی دورہ(AstScope* nodep) override {
        // Each FTask is unique per-scope, so AstNodeFTaskRefs do not need
        // pointers to what scope the FTask is to be invoked under.
        // However, to create variables, we need to track the scopes involved.
        // Find all var->varscope mappings, for later cleanup
        برائے (Ast_گرہ* stmtp = nodep->varsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVarScope* ثابت vscp = VN_CAST(stmtp, VarScope)) {
                AstVar* ثابت varp = vscp->varp();
                اگر (varp->isFuncLocal() || varp->is_مستعمل_Loop_Idx()
                    || varp->lifetime().isAutomatic()) {
                    UINFO(9, "   funcvsc " << vscp);
                    m_var_مقصد_Scope_Map.emplace(std::make_pair(nodep, varp), vscp);
                }
            }
        }
        // Likewise, all FTask->scope mappings
        برائے (Ast_گرہ* stmtp = nodep->blocksp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (Ast_گرہ_FTask* ثابت taskp = VN_CAST(stmtp, گرہ_FTask)) taskp->user3p(nodep);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        // We make multiple edges if a task is called multiple times from another task.
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked task");
        Task_FTask_راس* ثابت taskVtxp = get_FTask_راس(nodep->taskp());
        نیا Task_کنارہ{&m_call_گراف, m_curVxp, taskVtxp};
        // Virtual-interface method calls dispatch through a runtime handle and
        // must not be inlined.
        اگر (is_Virtual_Iface_طریقہ_Call(nodep)) taskVtxp->needsNonInlineCFunc(سچ);
        // Do we have to disable inlining the function?
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        اگر (!taskVtxp->noInline()) {  // Else short-circuit below
            برائے (ثابت خودکار& itr : tconnects) {
                ثابت AstVar* ثابت portp = itr.پہلا;
                ثابت AstArg* ثابت argp = itr.دوسرا;
                اگر (ثابت Ast_گرہ_اظہاریہ* ثابت pinp = argp->exprp()) {
                    اگر ((portp->isRef() || portp->isConstRef()) && !VN_IS(pinp, VarRef)) {
                        UINFO(9, "No function inline due to ref " << pinp);
                        taskVtxp->noInline(سچ);
                    }
                }
            }
        }
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        UINFO(9, "  TASK " << nodep);
        VL_RESTORER(m_curVxp);
        m_curVxp = get_FTask_راس(nodep);
        اگر (nodep->dpiImport()) m_curVxp->noInline(سچ);
        اگر (nodep->class_طریقہ()) m_curVxp->noInline(سچ);  // Until V3Task supports it
        اگر (nodep->recursive()) m_curVxp->noInline(سچ);
        اگر (nodep->isConstructor()) {
            m_curVxp->noInline(سچ);
            m_ctorp = nodep;
            UASSERT_OBJ(m_classp, nodep, "Ctor not under class");
            m_func_مقصد_Class_Map[nodep] = m_classp;
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstPragma* nodep) override {
        اگر (nodep->pragType() == VPragmaType::NO_INLINE_TASK) {
            // Just mark for the next steps, and we're done with it.
            m_curVxp->noInline(سچ);
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        } ورنہ {
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstVar* nodep) override {
        VL_RESTORER(m_under_پورٹ_Var);
        اگر (nodep->isIO()) m_under_پورٹ_Var = سچ;
        iterateChildren(nodep);
        nodep->user4p(m_curVxp);  // Remember what task it's under
    }
    خالی دورہ(AstVarRef* nodep) override {
        iterateChildren(nodep);
        اگر (m_under_پورٹ_Var) واپس;
        AstVar* ثابت varp = nodep->varp();
        // Reading a constant pool entry does not depend on external runtime state.
        اگر (nodep->access().isReadOnly() && varp->const_Pool_اندراج()) واپس;
        اگر (varp->user4u().مقصد_گراف_راس() != m_curVxp) {
            اگر (m_curVxp->خالص_3() && !varp->isXTemp() && !varp->isParam()) m_curVxp->impure(nodep);
        }
    }
    خالی دورہ(AstClass* nodep) override {
        // Move initial statements into the constructor
        VL_RESTORER_CLEAR(m_initialps);
        VL_RESTORER(m_ctorp);
        VL_RESTORER(m_classp);
        m_ctorp = خالی_اشاریہ;
        m_classp = nodep;
        {  // Find m_initialps, m_ctor
            iterateChildren(nodep);
        }
        UASSERT_OBJ(m_ctorp, nodep, "class constructor missing");  // LinkDot always makes it
        Ast_گرہ* insertp = خالی_اشاریہ;
        برائے (AstInitialAutomatic* initialp : m_initialps) {
            اگر (Ast_گرہ* ثابت movep = initialp->stmtsp()) {
                movep->unlink_Fr_عقب_With_اگلا();
                // Next InitialAutomatic must go in order after what we inserted
                insertp = Ast_گرہ::جوڑو_اگلا_Null(insertp, movep);
            }
            VL_DO_DANGLING(ڈالو_Deletep(initialp->unlink_Fr_عقب()), initialp);
        }
        m_initialps.clear();
        m_ctorp->prependStmtsp(insertp);
    }
    خالی دورہ(AstInitialAutomatic* nodep) override {
        m_initialps.ڈالو_عقب(nodep);
        iterateChildren(nodep);
    }
    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح TaskStateVisitor(AstNetlist* nodep) {
        m_curVxp = نیا Task_رمز_راس{&m_call_گراف};
        //
        iterate(nodep);
        //
        m_call_گراف.remove_Redundant_کنارے_مجموع(&Task_کنارہ::followAlwaysTrue);
        propagateImpureReason();
        decideInlining();
        اگر (dump_گراف_Level()) m_call_گراف.dumpDotFilePrefixed("task_call");
    }
    ~TaskStateVisitor() override = طےشدہ;
    VL_UNCOPYABLE(TaskStateVisitor);
};

//######################################################################
// DPI related utility functions

ساخت TaskDpiUtils final {
    // Returns a vector of ('elements', 'stride') pairs for each unpacked dimension
    ساکن std::سمتیہ<std::pair<صحیح, صحیح>> unpackDimsAndStrides(AstVar* varp) {
        Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
        std::سمتیہ<std::pair<صحیح, صحیح>> dimStrides;
        AstUnpackArrayDType* ثابت unpackp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        اگر (!unpackp) واپس dimStrides;

        ثابت std::سمتیہ<AstUnpackArrayDType*> dims = unpackp->unpackDimensions();
        ثابت سائز_t nDims = dims.سائز();
        dimStrides.resize(nDims);
        // Stride of fastest varying dimension is 1.
        dimStrides[nDims - 1].پہلا = dims.عقب()->elementsConst();
        dimStrides[nDims - 1].دوسرا = 1;
        // Rest are densly packed
        برائے (ssize_t i = nDims - 2; i >= 0; --i) {
            dimStrides[i].پہلا = dims[i]->elementsConst();
            dimStrides[i].دوسرا = dimStrides[i + 1].پہلا * dimStrides[i + 1].دوسرا;
        }
        واپس dimStrides;
    }
    // Returns the prefix of a function-call like statement used to convert
    // from IEEE DPI data types to internal types, and a bool that is true
    // if type uses svBitVecVal/svLogicVecVal and ther result is returned via
    // an output parameter, o false if uses C primitive type and result is the
    // return value.
    ساکن std::pair<std::string, منطقی> dpi_مقصد_اندرونی_Cvt_Stmt(ثابت Ast_گرہ_DType* dtypep) {
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();

        // DPI types using Primitive C types
        اگر (basicp->keyword() == VBasicDTypeKwd::CHANDLE) واپس {"VL_CVT_VP_Q(", جھوٹ};
        اگر (basicp->keyword() == VBasicDTypeKwd::STRING) واپس {"VL_CVT_N_CSTR(", جھوٹ};
        اگر (basicp->isDpiPrimitive()) واپس {"(", جھوٹ};

        // DPI types using svBitVecVal/svLogicVecVal
        UASSERT_OBJ(basicp->is_Dpi_ثنائی_ہندسہ_Vec() || basicp->isDpiLogicVec(), dtypep,
                    "Should use svBitVecVal/svLogicVecVal");

        ثابت std::string vecType = basicp->is_Dpi_ثنائی_ہندسہ_Vec() ? "SVBV" : "SVLV";
        dtypep = dtypep->skipRefp();
        ثابت حرف سائز_Char = dtypep->چوڑائی() <= 8    ? 'C'
                              : dtypep->چوڑائی() <= 16 ? 'S'
                                                      : *dtypep->charIQWN();
        ثابت std::string& سائز = std::مقصد_string(dtypep->چوڑائی());
        // cppcheck-suppress strPlusChar
        واپس {"VL_SET_"s + سائز_Char + "_" + vecType + "(" + سائز + ", ", سچ};
    }
};

//######################################################################
// Task state, as a visitor of each AstNode

جماعت TaskVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Each module:
    //    AstNodeFTask::user1   // True if its been expanded
    // Each funccall
    //  to 'relink' function:
    //    AstVar::user2p        // AstVarScope* to replace varref with
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;

    // TYPES
    استعمال DpiCFuncs = std::map<ثابت string, std::tuple<Ast_گرہ_FTask*, std::string, AstCFunc*>>;

    // STATE
    TaskStateVisitor* ثابت m_statep;  // Common state between visitors
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    Ast_چوٹی_Scope* ثابت m_چوٹی_Scopep = v3Global.rootp()->چوٹی_Scopep();  // The AstTopScope
    AstScope* m_scopep = خالی_اشاریہ;  // Current scope
    Ast_گرہ* m_insStmtp = خالی_اشاریہ;  // Where to insert statement
    منطقی m_اندر_Sensesp = جھوٹ;  // Are we under a senitem?
    منطقی m_اندر_New = جھوٹ;  // Are we under a constructor?
    // Variables created by createVarScope, shared by instances
    V3SharedTmps m_funcTmps{"__Vfunc", VVarType::BLOCKTEMP};  // For function calls
    V3SharedTmps m_taskTmps{"__Vtask", VVarType::BLOCKTEMP};  // For task calls
    V3SharedTmps m_consArrayTmps{"__VconsArrayTemp", VVarType::BLOCKTEMP};  // For ConsArray args
    V3SharedTmps m_forceTmps{"__Vforcefuncout", VVarType::VAR};  // For calls in force statements
    صحیح m_uncon_Var_عدد = 0;  // Unique bad connection variable

    // STATE - across all visitors
    DpiCFuncs m_dpi_نام;  // Map of all created DPI functions
    std::map<std::string, ثابت AstStructDType*> m_dpiStructs;  // C names of DPI structures
    VDouble0 m_statInlines;  // Statistic tracking
    VDouble0 m_statHierDpisWithCosts;  // Statistic tracking

    // METHODS

    AstVarScope* createFuncVar(AstCFunc* funcp, ثابت string& name, AstVar* examplep) {
        AstVar* ثابت newvarp = نیا AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, examplep};
        newvarp->funcLocal(سچ);
        funcp->جوڑو_Varsp(newvarp);
        AstVarScope* ثابت newvscp = نیا AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->جوڑو_Varsp(newvscp);
        واپس newvscp;
    }
    AstVarScope* create_داخلہ_Var(AstCFunc* funcp, ثابت string& name, VBasicDTypeKwd kwd) {
        AstVar* ثابت newvarp
            = نیا AstVar{funcp->fileline(), VVarType::BLOCKTEMP, name, funcp->ڈھونڈو_Basic_DType(kwd)};
        newvarp->funcLocal(سچ);
        newvarp->direction(VDirection::داخلہ);
        funcp->جوڑو_Argsp(newvarp);
        AstVarScope* ثابت newvscp = نیا AstVarScope{funcp->fileline(), m_scopep, newvarp};
        m_scopep->جوڑو_Varsp(newvscp);
        واپس newvscp;
    }
    ساکن AstVarScope* const_Pool_جدول(AstVar* paramp) {
        // Move array params in functions into constant pool, return the VarScope of the entry
        AstVarRef* ثابت refp = V3ConstPool::ڈھونڈو(VN_AS(paramp->valuep(), ConsArray));
        AstVarScope* ثابت vscp = refp->varScopep();
        VL_DO_DANGLING(refp->delete_درخت(), refp);
        واپس vscp;
    }
    // Create variable for 'invarp' of the call 'refp', named after the called task and 'suffix'
    AstVarScope* createVarScope(ثابت Ast_گرہ_FTask_Ref* refp, AstVar* invarp,
                                ثابت std::string& suffix) {
        اگر (invarp->isParam() && VN_IS(invarp->valuep(), ConsArray)) {
            واپس const_Pool_جدول(invarp);
        } ورنہ {
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
            newvarp->funcLocal(جھوٹ);
            newvarp->propagateAttrFrom(invarp);
            newvarp->is_اندرونی(سچ);
            واپس newvscp;
        }
    }

    // Replace varrefs with new var pointer
    خالی relink(Ast_گرہ* nodep) {
        nodep->foreach_And_اگلا([](AstVarRef* refp) {
            اگر (refp->varp()->user2p()) {  // It's being converted to an alias.
                AstVarScope* ثابت newvscp = VN_AS(refp->varp()->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(refp->varScopep()->varp());
            }
        });
    }

    Ast_تفویض* connect_پورٹ_Make_اندر_تفویض(Ast_گرہ_اظہاریہ* pinp, AstVarScope* newvscp, منطقی خالص_Check) {
        // Create input assignment to go in FRONT of function call
        Ast_گرہ_اظہاریہ* اندر_Pinp = pinp;
        اگر (AstResizeLValue* sinPinp = VN_CAST(اندر_Pinp, ResizeLValue)) اندر_Pinp = sinPinp->lhsp();
        Ast_گرہ_اظہاریہ* اندر_Pin_Clonep
            = خالص_Check ? اندر_Pinp->clone_درخت_خالص(سچ) : اندر_Pinp->clone_درخت(سچ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(newvscp->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (arrayp && arrayp->is_الٹ_کا(اندر_Pin_Clonep->dtypep())) {
            اندر_Pin_Clonep
                = نیا Ast_Cvt_Unpack_مقصد_Unpack{pinp->fileline(), اندر_Pin_Clonep, newvscp->dtypep()};
        }
        Ast_تفویض* ثابت assp = نیا Ast_تفویض{
            pinp->fileline(), نیا AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE},
            اندر_Pin_Clonep};
        assp->fileline()->modifyWarnOff(V3_Error_رمز::BLKSEQ, سچ);  // Ok if in <= block
        واپس assp;
    }
    Ast_تفویض* connect_پورٹ_Make_برآمد_تفویض(AstVar* portp, Ast_گرہ_اظہاریہ* pinp, AstVarScope* newvscp,
                                        منطقی خالص_Check) {
        // If needed, remap size of function to caller's output size
        Ast_گرہ_اظہاریہ* برآمد_Pinp = pinp;
        Ast_گرہ_اظہاریہ* postRhsp = نیا AstVarRef{newvscp->fileline(), newvscp, VAccess::READ};
        اگر (AstResizeLValue* soutPinp = VN_CAST(برآمد_Pinp, ResizeLValue)) {
            برآمد_Pinp = soutPinp->lhsp();
            اگر (VN_IS(برآمد_Pinp, RToIRoundS) || VN_IS(برآمد_Pinp, RToIS)) {
                برآمد_Pinp = VN_AS(برآمد_Pinp, گرہ_Uniop)->lhsp();
                اگر (postRhsp->isSigned()) {
                    postRhsp = نیا AstISToRD{pinp->fileline(), postRhsp};
                } ورنہ {
                    postRhsp = نیا AstIToRD{pinp->fileline(), postRhsp};
                }
            } ورنہ {
                اگر (Ast_گرہ_Uniop* aoutPinp = VN_CAST(برآمد_Pinp, Extend)) {
                    برآمد_Pinp = aoutPinp->lhsp();
                } ورنہ اگر (Ast_گرہ_Uniop* aoutPinp = VN_CAST(برآمد_Pinp, ExtendS)) {
                    برآمد_Pinp = aoutPinp->lhsp();
                } ورنہ اگر (AstSel* aoutPinp = VN_CAST(برآمد_Pinp, Sel)) {
                    برآمد_Pinp = aoutPinp->fromp();
                } ورنہ {
                    برآمد_Pinp->v3fatalSrc("Inout pin resizing should have had extend or select");
                }
                اگر (برآمد_Pinp->چوڑائی() < portp->چوڑائی()) {
                    postRhsp = نیا AstSel{pinp->fileline(), postRhsp, 0, pinp->چوڑائی()};
                } ورنہ {  // pin width > port width
                    اگر (pinp->isSigned() && postRhsp->isSigned()) {
                        postRhsp = نیا AstExtendS{pinp->fileline(), postRhsp};
                    } ورنہ {
                        postRhsp = نیا AstExtend{pinp->fileline(), postRhsp};
                    }
                }
            }
            postRhsp->dtypeFrom(برآمد_Pinp);
        }
        اگر (VN_IS(برآمد_Pinp, IToRD) || VN_IS(برآمد_Pinp, ISToRD)) {
            برآمد_Pinp = VN_AS(برآمد_Pinp, گرہ_Uniop)->lhsp();
            postRhsp = نیا AstRToIRoundS{pinp->fileline(), postRhsp};
            postRhsp->dtypeFrom(برآمد_Pinp);
        }
        // Put output assignment AFTER function call
        Ast_گرہ_اظہاریہ* ثابت برآمد_Pin_Clonep
            = خالص_Check ? برآمد_Pinp->clone_درخت_خالص(سچ) : برآمد_Pinp->clone_درخت(سچ);
        // A reversed array is assigned by the left-to-right order of elements (IEEE 1800-2023 7.6)
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(برآمد_Pinp->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (arrayp && arrayp->is_الٹ_کا(postRhsp->dtypep())) {
            postRhsp = نیا Ast_Cvt_Unpack_مقصد_Unpack{pinp->fileline(), postRhsp, برآمد_Pinp->dtypep()};
        }
        Ast_تفویض* ثابت assp = نیا Ast_تفویض{pinp->fileline(), برآمد_Pin_Clonep, postRhsp};
        assp->fileline()->modifyWarnOff(V3_Error_رمز::BLKSEQ, سچ);  // Ok if in <= block
        واپس assp;
    }

    خالی changeAtWriteRecurse(Ast_گرہ_اظہاریہ* ثابت exprp) {
        // Change nested at methods to writable variant
        اگر (AstCMethodHard* ثابت cMethodp = VN_CAST(exprp, CMethodHard)) {
            اگر (cMethodp->طریقہ() == VCMethod::ARRAY_AT) {
                cMethodp->طریقہ(VCMethod::ARRAY_AT_WRITE);
            }
            changeAtWriteRecurse(cMethodp->fromp());
        }
    }

    خالی connect_پورٹ(ثابت Ast_گرہ_FTask_Ref* refp, AstVar* portp, AstArg* argp, Ast_گرہ* beginp,
                     منطقی inlineTask) {
        Ast_گرہ_اظہاریہ* pinp = argp->exprp();
        اگر (inlineTask) {
            portp->unlink_Fr_عقب();
            ڈالو_Deletep(portp);  // Remove it from the clone (not original)
        }
        اگر (!pinp) {
            // Too few arguments in function call
        } ورنہ {
            UINFO(9, "     Port " << portp);
            UINFO(9, "      pin " << pinp);
            اگر (portp->isWritable() && VN_IS(pinp, Const)) {
                pinp->v3error("Function/task " + portp->direction().prettyName()  // e.g. "output"
                              + " connected to constant instead of variable: "
                              + portp->prettyNameQ());
                // Make temp pin to tie it off
                AstVar* ثابت varp = نیا AstVar{pinp->fileline(), VVarType::STMTTEMP,
                                                "__VfuncUnconn_" + portp->name() + "__"
                                                    + std::مقصد_string(m_uncon_Var_عدد++),
                                                portp->dtypep()};
                m_modp->جوڑو_Stmtsp(varp);
                AstVarScope* ثابت newvscp = نیا AstVarScope{pinp->fileline(), m_scopep, varp};
                m_scopep->جوڑو_Varsp(newvscp);
                AstVarRef* ثابت repp = نیا AstVarRef{pinp->fileline(), newvscp, VAccess::WRITE};
                pinp->replaceWith(repp);
                VL_DO_DANGLING(ڈالو_Deletep(pinp), pinp);
                pinp = repp;
            }
            اگر (inlineTask) {
                ڈالو_Deletep(pinp->unlink_Fr_عقب());  // Cloned in assignment below
                VL_DO_DANGLING(argp->unlink_Fr_عقب()->delete_درخت(), argp);  // Args no longer needed
            }
            اگر (portp->isRef() || portp->isConstRef()) {
                منطقی refArgOk = جھوٹ;
                اگر (VN_IS(pinp, VarRef) || VN_IS(pinp, MemberSel) || VN_IS(pinp, StructSel)
                    || VN_IS(pinp, ArraySel)) {
                    refArgOk = سچ;
                } ورنہ اگر (AstCMethodHard* ثابت cMethodp = VN_CAST(pinp, CMethodHard)) {
                    اگر (VN_IS(cMethodp->fromp()->dtypep()->skipRefp(), قطار_DType)) {
                        refArgOk = cMethodp->طریقہ() == VCMethod::DYN_AT_WRITE_APPEND
                                   || cMethodp->طریقہ() == VCMethod::DYN_AT_WRITE_APPEND_عقب;
                    } ورنہ {
                        changeAtWriteRecurse(cMethodp);
                        refArgOk = cMethodp->طریقہ() == VCMethod::ARRAY_AT_WRITE;
                    }
                }
                اگر (refArgOk) {
                    اگر (AstVarRef* ثابت varrefp = VN_CAST(pinp, VarRef)) {
                        varrefp->access(VAccess::READWRITE);
                    }
                } ورنہ {
                    pinp->v3error("Function/task ref argument is not of allowed type");
                }
                اگر (inlineTask) {
                    اگر (AstVarRef* ثابت varrefp = VN_CAST(pinp, VarRef)) {
                        // Connect to this exact variable
                        AstVarScope* ثابت localVscp = varrefp->varScopep();
                        UASSERT_OBJ(localVscp, varrefp, "Null var scope");
                        portp->user2p(localVscp);
                    } ورنہ {
                        pinp->v3fatalSrc("ref argument should have caused non-inline of function");
                    }
                }
            } ورنہ اگر (portp->isInout()) {
                // UINFOTREE(9, pinp, "", "pinrsize-");

                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                اگر (!inlineTask) {
                    pinp->replaceWith(
                        نیا AstVarRef{newvscp->fileline(), newvscp, VAccess::READWRITE});
                    ڈالو_Deletep(pinp);  // Cloned by connectPortMakeInAssign
                }
                // Put input assignment in FRONT of all other statements
                Ast_تفویض* ثابت preassp = connect_پورٹ_Make_اندر_تفویض(pinp, newvscp, سچ);
                beginp->جوڑو_اگلا_یہاں(preassp);

                Ast_تفویض* ثابت postassp = connect_پورٹ_Make_برآمد_تفویض(portp, pinp, newvscp, سچ);
                beginp->جوڑو_اگلا(postassp);
                // if (debug() >= 9) beginp->dumpTreeAndNext(cout, "-pinrsize-out- ");
            } ورنہ اگر (portp->isWritable()) {
                // Even if it's referencing a varref, we still make a temporary
                // Else task(x,x,x) might produce incorrect results
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                اگر (!inlineTask) {
                    pinp->replaceWith(نیا AstVarRef{newvscp->fileline(), newvscp, VAccess::WRITE});
                    ڈالو_Deletep(pinp);  // Cloned by connectPortMakeOutAssign
                }
                Ast_تفویض* ثابت postassp = connect_پورٹ_Make_برآمد_تفویض(portp, pinp, newvscp, جھوٹ);
                // Put assignment BEHIND of all other statements
                beginp->جوڑو_اگلا(postassp);
            } ورنہ اگر (inlineTask && portp->isNonOutput()) {
                // Make input variable
                AstVarScope* ثابت newvscp
                    = createVarScope(refp, portp, "__" + portp->shortName());
                portp->user2p(newvscp);
                Ast_تفویض* ثابت preassp = connect_پورٹ_Make_اندر_تفویض(pinp, newvscp, جھوٹ);
                // Put assignment in FRONT of all other statements
                beginp->جوڑو_اگلا_یہاں(preassp);
            }
        }
    }

    منطقی has_Ref_دلیل(Ast_گرہ_FTask* nodep) {
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isRef() || varp->isConstRef()) واپس سچ;
            }
        }
        واپس جھوٹ;
    }

    Ast_گرہ* createInlinedFTask(Ast_گرہ_FTask_Ref* refp, AstVarScope* outvscp) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        Ast_گرہ* ثابت newbodysp
            = refp->taskp()->stmtsp() ? refp->taskp()->stmtsp()->clone_درخت(سچ) : خالی_اشاریہ;
        Ast_گرہ* ثابت beginp
            = نیا AstComment{refp->fileline(), "Function: "s + refp->name(), سچ};
        اگر (newbodysp) beginp->جوڑو_اگلا(newbodysp);
        اگر (debug() >= 9) beginp->dump_درخت_And_اگلا(خارج, "-  newbegi: ");
        //
        // Create input variables
        Ast_گرہ::user2_Clear_درخت();
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, beginp);
            برائے (ثابت خودکار& itr : tconnects) {
                AstVar* ثابت portp = itr.پہلا;
                AstArg* ثابت argp = itr.دوسرا;
                connect_پورٹ(refp, portp, argp, beginp, سچ);
            }
        }
        UASSERT_OBJ(!refp->argsp(), refp, "Arg wasn't removed by above loop");
        {
            Ast_گرہ* nextstmtp;
            برائے (Ast_گرہ* stmtp = beginp; stmtp; stmtp = nextstmtp) {
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
                            Ast_گرہ* ثابت crstp = نیا Ast_تفویض{
                                portp->fileline(),
                                نیا AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                                نیا AstCReset{portp->fileline(), portp, جھوٹ}};
                            portp->replaceWith(crstp);
                        } ورنہ {
                            portp->unlink_Fr_عقب();
                        }
                        ڈالو_Deletep(portp);  // Remove it from the clone (not original)
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
        اگر (debug() >= 9) beginp->dump_درخت_And_اگلا(خارج, "-  iotask: ");
        واپس beginp;
    }

    Ast_گرہ* createNonInlinedFTask(Ast_گرہ_FTask_Ref* refp, AstVarScope* outvscp, AstCNew*& cnewpr) {
        // outvscp is the variable for functions only, if nullptr, it's a task
        UASSERT_OBJ(refp->taskp(), refp, "Unlinked?");
        AstCFunc* ثابت cfuncp = m_statep->ftaskCFuncp(refp->taskp());
        UASSERT_OBJ(cfuncp, refp, "No non-inline task associated with this task call?");
        //
        Ast_گرہ* ثابت beginp
            = نیا AstComment{refp->fileline(), "Function: "s + refp->name(), سچ};
        Ast_گرہ_CCall* ccallp;
        اگر (VN_IS(refp, New)) {
            AstCNew* ثابت cnewp = نیا AstCNew{refp->fileline(), cfuncp};
            cnewp->dtypep(refp->dtypep());
            ccallp = cnewp;
            // Parent AstNew will replace with this CNew
            cnewpr = cnewp;
        } ورنہ اگر (ثابت Ast_طریقہ_Call* ثابت mrefp = VN_CAST(refp, طریقہ_Call)) {
            ccallp = نیا AstCMethodCall{refp->fileline(), mrefp->fromp()->unlink_Fr_عقب(), cfuncp};
            ccallp->dtype_سیٹ_Void();
            beginp->جوڑو_اگلا(ccallp->makeStmt());
        } ورنہ {
            ccallp = نیا AstCCall{refp->fileline(), cfuncp};
            ccallp->dtype_سیٹ_Void();
            beginp->جوڑو_اگلا(ccallp->makeStmt());
        }

        اگر (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(refp, FuncRef)) {
            ccallp->superReference(funcRefp->superReference());
        } ورنہ اگر (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(refp, TaskRef)) {
            ccallp->superReference(taskRefp->superReference());
        } ورنہ اگر (ثابت Ast_طریقہ_Call* ثابت طریقہ_Callp = VN_CAST(refp, طریقہ_Call)) {
            ccallp->superReference(طریقہ_Callp->superReference());
        }

        // Convert complicated outputs to temp signals
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(refp, refp->taskp()->stmtsp());
            برائے (ثابت خودکار& itr : tconnects) {
                AstVar* ثابت portp = itr.پہلا;
                AstArg* ثابت argp = itr.دوسرا;
                connect_پورٹ(refp, portp, argp, beginp, جھوٹ);
            }
        }
        // First argument is symbol table, then output if a function
        ثابت منطقی needSyms = !refp->taskp()->dpiImport() || v3Global.opt.profExec();
        اگر (needSyms) ccallp->argTypes("vlSymsp");

        اگر (refp->taskp()->dpiContext()) {
            AstScopeName* ثابت snp = refp->scopeNamep();
            UASSERT_OBJ(snp, refp, "Missing scoping context");
            snp->unlink_Fr_عقب();
            File_سطر* ثابت flp = refp->fileline();
            // __Vscopep
            ccallp->جوڑو_Argsp(snp);
            // __Vfilenamep
            ccallp->جوڑو_Argsp(
                نیا AstCExpr{flp, AstCExpr::خالص{}, "\"" + flp->filenameEsc() + "\"", 64});
            // __Vlineno
            ccallp->جوڑو_Argsp(نیا AstConst(flp, flp->lineno()));
        }

        // Move artument expression to the CCall, without AstArg
        برائے (AstArg *argp = refp->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ccallp->جوڑو_Argsp(argp->exprp()->unlink_Fr_عقب());
        }

        اگر (outvscp) ccallp->جوڑو_Argsp(نیا AstVarRef{refp->fileline(), outvscp, VAccess::WRITE});

        اگر (debug() >= 9) beginp->dump_درخت_And_اگلا(خارج, "-  nitask: ");
        واپس beginp;
    }

    string dpiSignature(Ast_گرہ_FTask* nodep, AstVar* rtnvarp) ثابت {
        // Return fancy signature for DPI function. Variable names are not included so differences
        // in only argument names will not matter (as required by the standard).
        string dpiproto;
        اگر (nodep->dpi_خالص()) dpiproto += "pure ";
        اگر (nodep->dpiContext()) dpiproto += "context ";
        dpiproto += rtnvarp ? rtnvarp->dpiArgType(سچ, سچ) : "void";
        dpiproto += " " + nodep->cname() + " (";
        string دلائل;
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isIO() && !portp->isFuncReturn() && portp != rtnvarp) {
                    اگر (دلائل != "") {
                        دلائل += ", ";
                        dpiproto += ", ";
                    }
                    // Include both the Verilator and C type names, as if either
                    // differ we may get C compilation problems later. An unpacked structure
                    // has one layout per C name (checkDpiStructArgs), so its C name and
                    // elements suffice, as e.g. specializations differ only in Verilator names.
                    ثابت std::string dpiType = portp->dpiArgType(جھوٹ, جھوٹ);
                    dpiproto += dpiType;
                    ثابت std::string vType = portp->dtypep()->شامل_ہے_Unpacked_Struct()
                                                  ? portp->dpiTmpVarType("")
                                                  : portp->dtypep()->prettyDTypeName(جھوٹ);
                    اگر (!portp->isDpiOpenArray() && dpiType != vType) {
                        dpiproto += " /* " + vType + " */ ";
                    }
                    دلائل += portp->name();  // Leftover so ,'s look nice
                }
            }
        }
        dpiproto += ")";
        واپس dpiproto;
    }

    ساکن خالی checkLegalCIdentifier(Ast_گرہ* nodep, ثابت string& name) {
        اگر (name.انتہا() != std::ڈھونڈو_if(name.begin(), name.انتہا(), [](حرف c) {
                واپس !std::حرف_یا_ہندسہ(c) && c != '_';
            })) {
            nodep->v3error("DPI function has illegal characters in C identifier name '" << name
                                                                                        << '\'');
        }
    }

    ساکن Ast_گرہ* create_Dpi_عارضی(AstVar* portp, ثابت string& suffix) {
        ثابت string stmt = portp->dpiTmpVarType(portp->name() + suffix) + ";";
        واپس نیا AstCStmt{portp->fileline(), stmt};
    }

    خالی unlinkAndClone(Ast_گرہ_FTask* funcp, Ast_گرہ* nodep, منطقی with_اگلا) {
        UASSERT_OBJ(nodep, funcp, "null in function object clone");
        اگر (funcp->recursive()) {
            VNRelinker relinkHandle;
            اگر (with_اگلا) {
                nodep->unlink_Fr_عقب_With_اگلا(&relinkHandle);
            } ورنہ {
                nodep->unlink_Fr_عقب(&relinkHandle);
            }
            // Recursive functions require the original argument list to
            // still be live for linking purposes.
            // The old function gets clone, so that node pointers are mostly
            // retained through the V3Task transformations
            Ast_گرہ* ثابت newp = nodep->clone_درخت(with_اگلا);
            relinkHandle.relink(newp);
        } ورنہ {
            اگر (with_اگلا) {
                nodep->unlink_Fr_عقب_With_اگلا();
            } ورنہ {
                nodep->unlink_Fr_عقب();
            }
        }
    }

    ساکن Ast_گرہ* create_تفویض_اندرونی_مقصد_Dpi(AstVar* portp, منطقی isPtr, ثابت string& frSuffix,
                                              ثابت string& مقصد_Suffix) {
        ثابت string stmt = V3Task::تفویض_اندرونی_مقصد_Dpi(portp, isPtr, frSuffix, مقصد_Suffix);
        واپس نیا AstCStmt{portp->fileline(), stmt};
    }

    // Create statement converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep', neither an unpacked array nor an unpacked structure, to DPI if 'toDpi', else from
    // DPI. If the DPI type is svBitVecVal/svLogicVecVal, 'dpi' is a pointer to the value.
    ساکن Ast_گرہ_Stmt* createDpiScalarCvt(Ast_گرہ_اظہاریہ* internalp, ثابت std::string& dpi,
                                           ثابت Ast_گرہ_DType* dtypep, منطقی مقصد_Dpi) {
        File_سطر* ثابت flp = internalp->fileline();
        ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
        اگر (مقصد_Dpi) {
            AstCStmt* ثابت cstmtp = نیا AstCStmt{flp};
            اگر (basicp->isDpiPrimitive()) {
                cstmtp->جوڑو(dpi + (basicp->isCHandle() ? " = VL_CVT_Q_VP(" : " = "));
                cstmtp->جوڑو(internalp);
                cstmtp->جوڑو(basicp->isCHandle() ? ");" : basicp->isString() ? ".c_str();" : ";");
            } ورنہ {
                cstmtp->جوڑو((basicp->is_Dpi_ثنائی_ہندسہ_Vec() ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                            + dtypep->skipRefp()->charIQWN() + "("
                            + std::مقصد_string(dtypep->چوڑائی()) + ", " + dpi + ", ");
                cstmtp->جوڑو(internalp);
                cstmtp->جوڑو(");");
            }
            واپس cstmtp;
        }
        std::string cvt;
        منطقی useSvVec;
        std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_مقصد_اندرونی_Cvt_Stmt(dtypep);
        اگر (useSvVec) {
            AstCStmt* ثابت cstmtp = نیا AstCStmt{flp};
            cstmtp->جوڑو(cvt);
            cstmtp->جوڑو(internalp);
            cstmtp->جوڑو(", " + dpi + ");");
            واپس cstmtp;
        }
        // Number of bits in the C expression result, for masking.
        ثابت صحیح cwidth = basicp->چوڑائی_الفاظ() * VL_EDATASIZE;
        Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstCExpr{flp, cvt + dpi + ")", cwidth};
        واپس نیا Ast_تفویض{flp, internalp, نیا AstSel{flp, rhsp, 0, dtypep->چوڑائی()}};
    }

    // Create statements converting between 'internalp' and the DPI C expression 'dpi' of type
    // 'dtypep'. Unpacked arrays and unpacked structures are converted by their elements and
    // members, as DPI passes them as C arrays and C structures (IEEE 1800-2023 H.7.3).
    ساکن Ast_گرہ_Stmt* createDpiCvt(Ast_گرہ_اظہاریہ* internalp, ثابت std::string& dpi,
                                     ثابت Ast_گرہ_DType* dtypep, منطقی مقصد_Dpi) {
        File_سطر* ثابت flp = internalp->fileline();
        dtypep = dtypep->skipRefp();
        Ast_گرہ_Stmt* stmtsp = خالی_اشاریہ;
        اگر (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            // Internal index 0 is of the lowest SystemVerilog index, as C index 0 is
            برائے (صحیح i = 0; i < arrayp->elementsConst(); ++i) {
                Ast_گرہ_اظہاریہ* ثابت elemp = نیا AstArraySel{flp, internalp->clone_درخت(جھوٹ), i};
                stmtsp = Ast_گرہ::جوڑو_اگلا(stmtsp,
                                          createDpiCvt(elemp, dpi + "[" + std::مقصد_string(i) + "]",
                                                       arrayp->subDTypep(), مقصد_Dpi));
            }
        } ورنہ اگر (dtypep->شامل_ہے_Unpacked_Struct()) {
            برائے (ثابت AstMemberDType* itemp = VN_AS(dtypep, StructDType)->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstStructSel* ثابت selp
                    = نیا AstStructSel{flp, internalp->clone_درخت(جھوٹ), itemp->name()};
                selp->dtypeFrom(itemp->dtypep());
                stmtsp
                    = Ast_گرہ::جوڑو_اگلا(stmtsp, createDpiCvt(selp, dpi + "." + itemp->prettyName(),
                                                            itemp->subDTypep(), مقصد_Dpi));
            }
        } ورنہ {
            واپس createDpiScalarCvt(internalp, dpi, dtypep, مقصد_Dpi);
        }
        VL_DO_DANGLING(internalp->delete_درخت(), internalp);
        واپس stmtsp;
    }

    // Create statements converting between the internal variable 'vscp' and the DPI temporary or
    // argument 'dpiName'. Converts to DPI if 'toDpi', else from DPI. Internal representation is
    // scalar, 1D, or multi-dimensional array (similar to SV). DPI value is scalar or 1D array (if
    // unpacked array), always an array if of unpacked structures, as passed by reference.
    ساکن Ast_گرہ_Stmt* createDpiArgCvt(AstVarScope* vscp, ثابت std::string& dpiName,
                                        منطقی مقصد_Dpi) {
        AstVar* ثابت varp = vscp->varp();
        File_سطر* ثابت flp = vscp->fileline();
        ثابت std::سمتیہ<std::pair<صحیح, صحیح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
        // Total number of elements in unpacked array
        ثابت صحیح کل = strides.خالی_خانے() ? 1 : strides[0].پہلا * strides[0].دوسرا;
        ثابت منطقی isStruct = varp->dtypep()->شامل_ہے_Unpacked_Struct();
        ثابت Ast_گرہ_DType* ثابت elemDTypep = varp->dtypep()->elemDTypep();
        // The resulting list of statements
        Ast_گرہ_Stmt* stmtsp = خالی_اشاریہ;
        برائے (صحیح i = 0; i < کل; ++i) {
            Ast_گرہ_اظہاریہ* internalp
                = نیا AstVarRef{flp, vscp, مقصد_Dpi ? VAccess::READ : VAccess::WRITE};

            // Extract a scalar from multi-dimensional array (internal format)
            برائے (ثابت خودکار& stride : strides) {
                internalp = نیا AstArraySel(flp, internalp, (i / stride.دوسرا) % stride.پہلا);
            }

            // Extract a scalar from DPI temporary var that is scalar or 1D array
            std::string dpi = dpiName;
            اگر (!isStruct && !varp->basicp()->isDpiPrimitive()) {
                dpi += " + " + std::مقصد_string(i * varp->basicp()->چوڑائی_الفاظ());
            } ورنہ اگر (isStruct || !strides.خالی_خانے()) {
                dpi += "[" + std::مقصد_string(i) + "]";
            }
            stmtsp = Ast_گرہ::جوڑو_اگلا(stmtsp, createDpiCvt(internalp, dpi, elemDTypep, مقصد_Dpi));
        }
        واپس stmtsp;
    }

    // Create dispatch wrapper
    AstCFunc* makeDpiExportDispatcher(Ast_گرہ_FTask* ثابت nodep, AstVar* ثابت rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        File_سطر* ثابت flp = nodep->fileline();
        ثابت std::string cname = Ast_گرہ::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, cname);
        ثابت حرف* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();
        ثابت std::string rtnType = rtnvarp ? rtnvarp->dpiArgType(سچ, سچ) : "";
        // The function we are building
        AstCFunc* ثابت funcp = نیا AstCFunc{flp, cname, m_scopep, rtnType};
        funcp->dpiExportDispatcher(سچ);
        funcp->dpiContext(nodep->dpiContext());
        funcp->dontCombine(سچ);
        funcp->اندراج_نقطہ(سچ);
        funcp->isStatic(سچ);
        funcp->protect(جھوٹ);
        funcp->cname(cname);
        // Add DPI Export to top, since it's a global function
        m_چوٹی_Scopep->scopep()->جوڑو_Blocksp(funcp);
        // Check the arguments before converting them
        اگر (!make_پورٹ_List(nodep, funcp)) واپس خالی_اشاریہ;

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
        AstCStmt* ثابت prep = نیا AstCStmt{flp};
        funcp->جوڑو_Stmtsp(prep);
        // Static doesn't need save-restore as if below will re-fill proper value
        prep->جوڑو("static int __Vfuncnum = -1;\n");
        // First time init (faster than what the compiler does if we did a singleton
        prep->جوڑو("if (VL_UNLIKELY(__Vfuncnum == -1)) {\n");
        prep->جوڑو("__Vfuncnum = Verilated::exportFuncNum(\"" + nodep->cname() + "\");\n");
        prep->جوڑو("}\n");
        // If the find fails, it will throw an error
        prep->جوڑو("const VerilatedScope* const __Vscopep = Verilated::dpiScope();\n");
        // If 'dpiScope()' fails and '__Vscopep' is null; the exportFind function throws an error
        // If __Vcb is null the exportFind function throws and error
        ثابت std::string cbtype
            = VIdProtect::protect(v3Global.opt.سابقہ() + "__Vcb_" + nodep->cname() + "_t");
        prep->جوڑو(cbtype + " __Vcb = reinterpret_cast<" + cbtype
                  + ">(VerilatedScope::exportFind(__Vscopep, __Vfuncnum));");

        // Convert input/inout DPI arguments to Internal types, and construct the call
        AstCStmt* ثابت callp = نیا AstCStmt{flp};
        ثابت خودکار جوڑو_Func_Arg = [&](AstVar* portp) -> AstVarScope* {
            // No createDpiTemp; we make a real internal variable instead
            AstVarScope* ثابت vscp = createFuncVar(funcp, portp->name() + tmpSuffixp, portp);
            // No information exposure; is already visible in import/export func template
            vscp->varp()->protect(جھوٹ);
            portp->protect(جھوٹ);
            // Add argument to call
            ثابت VAccess access = portp->direction().pinAccess();
            callp->جوڑو(", ");
            callp->جوڑو(نیا AstVarRef{portp->fileline(), vscp, access});
            واپس vscp;
        };
        // Call callback
        callp->جوڑو("(*__Vcb)(");
        // First argument is the Syms
        callp->جوڑو("(" + EmitCUtil::symClassName() + "*)(__Vscopep->symsp())");
        // Add function arguments
        Ast_گرہ* برآمد_Cvtsp = خالی_اشاریہ;  // Conversions of output/inout arguments, after the call
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت portp = VN_CAST(stmtp, Var);
            اگر (!portp) جاری;
            اگر (!portp->isIO()) جاری;
            اگر (portp->isFuncReturn()) جاری;
            اگر (portp == rtnvarp) جاری;  // Handled below
            // Add argument to call
            AstVarScope* ثابت outvscp = جوڑو_Func_Arg(portp);
            ثابت منطقی isStruct = portp->dtypep()->شامل_ہے_Unpacked_Struct();
            اگر (portp->isWritable()) {
                برآمد_Cvtsp = Ast_گرہ::جوڑو_اگلا(
                    برآمد_Cvtsp, isStruct ? createDpiArgCvt(outvscp, portp->name(), سچ)
                                       : create_تفویض_اندرونی_مقصد_Dpi(portp, سچ, tmpSuffixp, ""));
            }
            اگر (!portp->isNonOutput()) جاری;
            // Convert input/inout arguments to dpi type
            ثابت std::string deref
                = portp->isInout() && !isStruct  //
                          && portp->basicp()->isDpiPrimitive()  //
                          && portp->dtypep()->skipRefp()->dimensions(جھوٹ).دوسرا == 0
                      ? "*"
                      : "";
            funcp->جوڑو_Stmtsp(createDpiArgCvt(outvscp, deref + portp->name(), جھوٹ));
        }
        // Return value argument goes last
        اگر (rtnvarp) جوڑو_Func_Arg(rtnvarp);
        // Close statement
        callp->جوڑو(");");
        // Call the user function
        funcp->جوڑو_Stmtsp(callp);
        // Convert output/inout arguments back to internal type
        اگر (برآمد_Cvtsp) funcp->جوڑو_Stmtsp(برآمد_Cvtsp);
        // Convert return value
        اگر (rtnvarp) {
            funcp->جوڑو_Stmtsp(create_Dpi_عارضی(rtnvarp, ""));
            funcp->جوڑو_Stmtsp(create_تفویض_اندرونی_مقصد_Dpi(rtnvarp, جھوٹ, tmpSuffixp, ""));
            ثابت std::string اشاریہ = rtnvarp->basicp()->isDpiPrimitive() ? "" : "[0]";
            funcp->جوڑو_Stmtsp(نیا AstCStmt{flp, "return " + rtnvarp->name() + اشاریہ + ";"});
        }
        واپس funcp;
    }

    AstCFunc* makeDpiImportPrototype(Ast_گرہ_FTask* nodep, AstVar* rtnvarp) {
        // Verilog name has __ conversion and other tricks, to match DPI C code, back that out
        ثابت string name = Ast_گرہ::prettyName(nodep->cname());
        checkLegalCIdentifier(nodep, name);
        // Tasks (but not void functions) return a boolean 'int' indicating disabled
        ثابت string rtnType = rtnvarp            ? rtnvarp->dpiArgType(سچ, سچ)
                               : nodep->dpiTask() ? "int"
                                                  : "";
        AstCFunc* ثابت funcp = نیا AstCFunc{nodep->fileline(), name, m_scopep, rtnType};
        funcp->dpiContext(nodep->dpiContext());
        funcp->dpiImportPrototype(سچ);
        funcp->dontCombine(سچ);
        funcp->اندراج_نقطہ(جھوٹ);
        funcp->is_طریقہ(جھوٹ);
        funcp->protect(جھوٹ);
        funcp->dpi_خالص(nodep->dpi_خالص());

        ثابت صحیح لاگت = static_cast<صحیح>(V3Control::get_Profile_ڈیٹا(funcp->name()));
        m_statHierDpisWithCosts += (لاگت != 0);
        funcp->لاگت(لاگت);

        // Add DPI Import to top, since it's a global function
        m_چوٹی_Scopep->scopep()->جوڑو_Blocksp(funcp);
        funcp->dpiCDecl(nodep->dpiCDecl());
        اگر (!make_پورٹ_List(nodep, funcp)) واپس خالی_اشاریہ;
        واپس funcp;
    }

    AstCFunc* getDpiFunc(Ast_گرہ_FTask* nodep, AstVar* rtnvarp) {
        UASSERT_OBJ(nodep->dpiImport() || nodep->dpiExport(), nodep, "Not a DPI function");
        // Compute unique signature of this DPI function
        ثابت string signature = dpiSignature(nodep, rtnvarp);
        // Only create one DPI Import prototype or DPI Export entry point for each unique cname as
        // it is illegal for the user to attach multiple tasks with different signatures to one DPI
        // cname.
        ثابت خودکار pair = m_dpi_نام.emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(nodep->cname()),
                                             std::forward_as_tuple(nodep, signature, خالی_اشاریہ));
        اگر (pair.دوسرا) {
            // First time encountering this cname. Create Import prototype / Export entry point
            AstCFunc* ثابت funcp = nodep->dpiExport() ? makeDpiExportDispatcher(nodep, rtnvarp)
                                                       : makeDpiImportPrototype(nodep, rtnvarp);
            std::get<2>(pair.پہلا->دوسرا) = funcp;
            واپس funcp;
        } ورنہ {
            // Seen this cname import before. Check if it's the same prototype.
            ثابت Ast_گرہ_FTask* پہلا_Nodep;
            string پہلا_Signature;
            AstCFunc* پہلا_Funcp;
            std::tie(پہلا_Nodep, پہلا_Signature, پہلا_Funcp) = pair.پہلا->دوسرا;
            اگر (signature != پہلا_Signature) {
                // Different signature, so error.
                nodep->v3error("Duplicate declaration of DPI function with different signature: '"
                               << nodep->cname() << "'\n"
                               << nodep->warnContextPrimary() << '\n'
                               << nodep->warn_مزید()  //
                               << "... New signature:      " << signature << '\n'  //
                               << پہلا_Nodep->warnOther()
                               << "... Original signature: " << پہلا_Signature << '\n'  //
                               << پہلا_Nodep->warnContextSecondary());
                واپس خالی_اشاریہ;
            } ورنہ {
                // Same signature, return the previously created CFunc
                واپس پہلا_Funcp;
            }
        }
    }

    ساکن منطقی make_پورٹ_List(Ast_گرہ_FTask* nodep, AstCFunc* dpip) {
        منطقی allOk = سچ;
        // Copy nodep's list of function I/O to the new dpip c function
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isIO()) {
                    // Move it to new function
                    AstVar* ثابت newPortp = portp->clone_درخت(جھوٹ);
                    newPortp->funcLocal(سچ);
                    dpip->جوڑو_Argsp(newPortp);
                    // Unpacked structures were checked by checkDpiStruct
                    اگر (!portp->basicp() && !portp->dtypep()->شامل_ہے_Unpacked_Struct()) {
                        allOk = جھوٹ;
                        portp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: DPI argument of type "
                                << portp->dtypep()->prettyTypeName() << '\n'
                                << portp->warn_مزید()
                                << "... For best portability, use bit, byte, int, or longint");
                        // We don't warn on logic either, although the 4-stateness is lost.
                        // That's what other simulators do.
                    }
                }
            }
        }
        واپس allOk;
    }

    // Whether C code, and the C++ Verilator generates, can use 'name' as an identifier
    ساکن منطقی isDpiCIdentifier(ثابت string& name) {
        واپس !name.خالی_خانے() && VString::isIdentifier(name)
               && !std::ہندسہ_ہے(name[0])
               // C reserves names beginning with '__', or '_' and an uppercase letter
               && !(name[0] == '_' && (name[1] == '_' || std::isupper(name[1])))
               && V3_Language_الفاظ::isKeyword(name).خالی_خانے();
    }

    // Check DPI can pass the unpacked structure 'sdtypep' of argument 'portp', else report.
    // If 'nested' in another structure, it may be anonymous, as declared where used.
    منطقی checkDpiStruct(ثابت AstVar* portp, ثابت AstStructDType* sdtypep, منطقی nested) {
        ثابت std::string name = sdtypep->dpiName();
        اگر (!(nested && name.خالی_خانے()) && !isDpiCIdentifier(name)) {
            portp->v3warn(E_UNSUPPORTED,
                          "Unsupported: DPI argument of unpacked structure type "
                          "without a typedef name usable in C: "
                              << sdtypep->prettyDTypeNameQ(سچ) << "\n"
                              << portp->warn_مزید()
                              << "... Suggest use a typedef with a C identifier name");
            واپس جھوٹ;
        }
        برائے (ثابت AstMemberDType* itemp = sdtypep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            اگر (!isDpiCIdentifier(itemp->prettyName())) {
                portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI argument of unpacked structure "
                                             "type with member name not usable in C: "
                                                 << itemp->prettyNameQ());
                واپس جھوٹ;
            }
            // Of the types of arguments, or unpacked arrays of them (IEEE 1800-2023 35.5.6)
            ثابت Ast_گرہ_DType* dtypep = itemp->subDTypep()->skipRefp();
            جب (ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ثابت Ast_گرہ_UOr_Struct_DType* ثابت subp = VN_CAST(dtypep, گرہ_UOr_Struct_DType);
            ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType);
            // Neither an unpacked array nor an unpacked structure, as createDpiScalarCvt converts
            ثابت منطقی isScalar = (subp && subp->packed()) || VN_IS(dtypep, PackArrayDType)
                                  || (basicp && !basicp->isEvent());
            اگر (dtypep->شامل_ہے_Unpacked_Struct()) {
                اگر (!checkDpiStruct(portp, VN_AS(dtypep, StructDType), سچ)) واپس جھوٹ;
            } ورنہ اگر (!isScalar) {
                portp->v3error("DPI argument " << portp->prettyNameQ()
                                               << " has unpacked structure member "
                                               << itemp->prettyNameQ() << " of type "
                                               << itemp->subDTypep()->prettyDTypeNameQ(سچ)
                                               << ", which DPI does not allow"
                                               << " (IEEE 1800-2023 35.5.6)");
                واپس جھوٹ;
            }
        }
        // C has no scopes, so cannot have different structures of the same name
        اگر (name.خالی_خانے()) واپس سچ;
        ثابت AstStructDType* ثابت firstp = m_dpiStructs.emplace(name, sdtypep).پہلا->دوسرا;
        اگر (firstp->dpiTypedef() == sdtypep->dpiTypedef()) واپس سچ;
        portp->v3warn(E_UNSUPPORTED, "Unsupported: DPI arguments of different unpacked "
                                     "structure types with the same name '"
                                         << name << "'\n"
                                         << portp->warnContextPrimary() << '\n'
                                         << firstp->warnOther()
                                         << "... Location of the other structure type\n"
                                         << firstp->warnContextSecondary());
        واپس جھوٹ;
    }

    // Check DPI can pass the unpacked structure arguments of 'nodep', else report
    منطقی check_Dpi_Struct_دلائل(ثابت Ast_گرہ_FTask* nodep) {
        منطقی allOk = سچ;
        برائے (ثابت Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت portp = VN_CAST(stmtp, Var);
            اگر (portp && portp->isIO() && portp->dtypep()->شامل_ہے_Unpacked_Struct()) {
                allOk = checkDpiStruct(portp, VN_AS(portp->dtypep()->elemDTypep(), StructDType),
                                       جھوٹ)
                        && allOk;
            }
        }
        واپس allOk;
    }

    خالی bodyDpiImportFunc(Ast_گرہ_FTask* nodep, AstVarScope* rtnvscp, AstCFunc* cfuncp,
                           AstCFunc* dpiFuncp) {
        ثابت حرف* ثابت tmpSuffixp = V3Task::dpiTemporaryVarSuffix();

        اگر (v3Global.opt.profExec()) {
            cfuncp->جوڑو_Stmtsp(AstCStmt::prof_Exec_Section_ڈالو(nodep->fileline(), "dpiimports"));
        }

        // Convert input/inout arguments to DPI types
        string دلائل;
        برائے (Ast_گرہ* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                AstVarScope* ثابت portvscp
                    = VN_AS(portp->user2p(), VarScope);  // Remembered when we created it earlier
                اگر (portp->isIO() && !portp->isFuncReturn() && portvscp != rtnvscp
                    && portp->name() != "__Vscopep"  // Passed to dpiContext, not callee
                    && portp->name() != "__Vfilenamep" && portp->name() != "__Vlineno") {

                    اگر (دلائل != "") دلائل += ", ";

                    اگر (portp->isDpiOpenArray()) {
                        Ast_گرہ_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                        UASSERT_OBJ(!VN_IS(dtypep, DynArrayDType) && !VN_IS(dtypep, قطار_DType),
                                    portp,
                                    "Passing dynamic array or queue as actual argument to DPI "
                                    "open array is not yet supported");
                        // Ideally we'd make a table of variable
                        // characteristics, and reuse it wherever we can
                        // At least put them into the module's CTOR as static?
                        ثابت string propName = portp->name() + "__Vopenprops";
                        ثابت string prop_رمز = portp->vlPropDecl(propName);
                        cfuncp->جوڑو_Stmtsp(نیا AstCStmt{portp->fileline(), prop_رمز});
                        //
                        // At runtime we need the svOpenArrayHandle to
                        // point to this task & thread's data, in addition
                        // to static info about the variable
                        ثابت string name = portp->name() + "__Vopenarray";
                        ثابت string var_رمز
                            = ("VerilatedDpiOpenVar "
                               // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                               + name + " (&" + propName + ", &" + portp->name() + ");\n");
                        cfuncp->جوڑو_Stmtsp(نیا AstCStmt{portp->fileline(), var_رمز});
                        دلائل += "&" + name;
                    } ورنہ {
                        // Unpacked structures are passed by reference from a C array
                        ثابت منطقی isStruct = portp->dtypep()->شامل_ہے_Unpacked_Struct();
                        اگر (portp->isWritable() && !isStruct
                            && portp->basicp()->isDpiPrimitive()) {
                            اگر (!VN_IS(portp->dtypep()->skipRefp(), UnpackArrayDType)) دلائل += "&";
                        }

                        دلائل += portp->name() + tmpSuffixp;

                        cfuncp->جوڑو_Stmtsp(create_Dpi_عارضی(portp, tmpSuffixp));
                        اگر (portp->isNonOutput()) {
                            cfuncp->جوڑو_Stmtsp(
                                isStruct
                                    ? createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, سچ)
                                    : create_تفویض_اندرونی_مقصد_Dpi(portp, جھوٹ, "", tmpSuffixp));
                        }
                    }
                }
            }
        }

        // Store context, if needed
        اگر (nodep->dpiContext()) {
            ثابت string stmt = "Verilated::dpiContext(__Vscopep, __Vfilenamep, __Vlineno);";
            cfuncp->جوڑو_Stmtsp(نیا AstCStmt{nodep->fileline(), stmt});
        }

        {  // Call the imported function
            AstCCall* ثابت callp = نیا AstCCall{nodep->fileline(), dpiFuncp};
            callp->dtype_سیٹ_Void();
            callp->argTypes(دلائل);
            اگر (rtnvscp) {
                // If it has a return value, capture it
                cfuncp->جوڑو_Stmtsp(create_Dpi_عارضی(rtnvscp->varp(), tmpSuffixp));
                ثابت std::string sel = rtnvscp->varp()->basicp()->isDpiPrimitive() ? "" : "[0]";
                AstCStmt* ثابت cstmtp = نیا AstCStmt{nodep->fileline()};
                cstmtp->جوڑو(rtnvscp->varp()->name() + tmpSuffixp + sel);  // LHS
                cstmtp->جوڑو(" = ");
                cstmtp->جوڑو(callp);  // RHS
                cstmtp->جوڑو(";");
                cfuncp->جوڑو_Stmtsp(cstmtp);
            } ورنہ {
                // Othervise just call it
                cfuncp->جوڑو_Stmtsp(callp->makeStmt());
            }
        }

        // Convert output/inout arguments back to internal type
        برائے (Ast_گرہ* stmtp = cfuncp->argsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                portp->protect(جھوٹ);  // No additional exposure - already part of shown proto
                اگر (portp->isIO() && (portp->isWritable() || portp->isFuncReturn())
                    && !portp->isDpiOpenArray()) {
                    AstVarScope* ثابت portvscp = VN_AS(
                        portp->user2p(), VarScope);  // Remembered when we created it earlier
                    cfuncp->جوڑو_Stmtsp(
                        createDpiArgCvt(portvscp, portp->name() + tmpSuffixp, جھوٹ));
                }
            }
        }

        اگر (v3Global.opt.profExec()) {
            cfuncp->جوڑو_Stmtsp(AstCStmt::prof_Exec_Section_نکالو(nodep->fileline(), "dpiimports"));
        }
    }

    AstVarScope* getDpiExporTrigger() {
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstVarScope* dpiExportTriggerp = netlistp->dpiExportTriggerp();
        اگر (!dpiExportTriggerp) {
            // Create the global DPI export trigger flag the first time we encounter a DPI export.
            // This flag is set any time a DPI export is invoked, and cleared at the end of eval.
            dpiExportTriggerp = m_چوٹی_Scopep->create_عارضی("__Vdpi_export_trigger", 1);
            netlistp->dpiExportTriggerp(dpiExportTriggerp);
        }
        واپس dpiExportTriggerp;
    }

    AstCFunc* makeUserFunc(Ast_گرہ_FTask* nodep, منطقی ftaskNoInline) {
        // Given a already cloned node, make a public C function, or a non-inline C function
        // Probably some of this work should be done later, but...
        // should the type of the function be bool/uint32/64 etc (based on lookup) or IData?
        Ast_گرہ::user2_Clear_درخت();
        AstVar* rtnvarp = خالی_اشاریہ;
        اگر (nodep->isFunction()) {
            AstVar* ثابت portp = VN_AS(nodep->fvarp(), Var);
            UASSERT_OBJ(portp, nodep, "function without function output variable");
            UASSERT_OBJ(portp->isFuncReturn(), nodep, "Not marked as function return var");
            اگر (nodep->dpiImport() || nodep->dpiExport()) {
                AstBasicDType* ثابت bdtypep = portp->dtypep()->basicp();
                اگر (!bdtypep) {
                    portp->v3error("DPI function may not return type "
                                   << portp->dtypep()->prettyDTypeNameQ(سچ)
                                   << " (IEEE 1800-2023 35.5.5)");
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس خالی_اشاریہ;
                }
                اگر (!bdtypep->isDpiPrimitive()) {
                    اگر (bdtypep->is_Dpi_ثنائی_ہندسہ_Vec() && portp->چوڑائی() > 32) {
                        portp->v3error("DPI function may not return a > 32 bits wide type "
                                       "other than basic types.\n"
                                       + portp->warn_مزید()
                                       + "... Suggest make it an output argument instead?");
                    }
                    اگر (bdtypep->isDpiLogicVec()) {
                        portp->v3error("DPI function may not return a 4-state type "
                                       "other than a single 'logic' (IEEE 1800-2023 35.5.5)");
                    }
                }
            } ورنہ اگر (nodep->task_عوامی()) {
                اگر (portp->isWide()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Public functions with return > 64 bits wide.\n"
                                      + nodep->warn_مزید()
                                      + "... Suggest make it an output argument instead?");
                }
            }

            اگر (ftaskNoInline || nodep->dpiExport()) {
                portp->funcReturn(جھوٹ);  // Converting return to 'outputs'
            }
            unlinkAndClone(nodep, portp, جھوٹ);
            rtnvarp = portp;
            rtnvarp->funcLocal(سچ);
            rtnvarp->noCReset(سچ);  // As made for port in V3LinkResolve
            rtnvarp->name(rtnvarp->name()
                          + "__Vfuncrtn");  // Avoid conflict with DPI function name
            اگر (nodep->dpiImport() || nodep->dpiExport()) rtnvarp->protect(جھوٹ);
        }

        // Create/pick up the DPI CFunc for DPI Import/ DPI Export.
        AstCFunc* dpiFuncp = خالی_اشاریہ;
        اگر (nodep->dpiImport() || nodep->dpiExport()) {
            UASSERT_OBJ(!(nodep->dpi_Open_والد() && nodep->dpi_Open_بچہ()), nodep,
                        "DPI task should not be both parent and child");
            dpiFuncp = check_Dpi_Struct_دلائل(nodep) ? getDpiFunc(nodep, rtnvarp) : خالی_اشاریہ;
            اگر (!dpiFuncp || (nodep->dpiImport() && nodep->dpi_Open_والد())) {
                // No need to make more than just the DPI Import prototype, the children will
                // create the wrapper implementations.
                اگر (rtnvarp) VL_DO_DANGLING(ڈالو_Deletep(rtnvarp), rtnvarp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                واپس خالی_اشاریہ;
            }
        }

        AstVarScope* rtnvscp = خالی_اشاریہ;
        اگر (rtnvarp) {
            rtnvscp = نیا AstVarScope{rtnvarp->fileline(), m_scopep, rtnvarp};
            m_scopep->جوڑو_Varsp(rtnvscp);
            rtnvarp->user2p(rtnvscp);
        }

        string سابقہ;
        اگر (nodep->dpiImport()) {
            سابقہ = "__Vdpiimwrap_";
        } ورنہ اگر (nodep->dpiExport()) {
            سابقہ = "__Vdpiexp_";
        } ورنہ اگر (ftaskNoInline) {
            سابقہ = "__VnoInFunc_";
        }
        // Unless public, v3Descope will not uniquify function names even if duplicate per-scope,
        // so make it unique now.
        string suffix;  // So, make them unique
        اگر (!nodep->task_عوامی() && !nodep->class_طریقہ()) suffix = "_" + m_scopep->nameDotless();
        ثابت string name = ((nodep->name() == "new") ? "new" : سابقہ + nodep->name() + suffix);
        AstCFunc* ثابت cfuncp = نیا AstCFunc{
            nodep->fileline(), name, m_scopep,
            ((nodep->task_عوامی() && rtnvarp) ? rtnvarp->cPubArgType(سچ, سچ) : "")};
        // It's ok to combine imports because this is just a wrapper;
        // duplicate wrappers can get merged.
        cfuncp->dontCombine(!nodep->dpiImport());
        cfuncp->اندراج_نقطہ(!nodep->dpiImport());
        cfuncp->func_عوامی(nodep->task_عوامی());
        cfuncp->dpiContext(nodep->dpiContext());
        cfuncp->dpiExportImpl(nodep->dpiExport());
        cfuncp->dpiImportWrapper(nodep->dpiImport());
        cfuncp->recursive(nodep->recursive());
        // Hardcoded based on UVM usage; TODO make a verilated_std.vlt control for these
        cfuncp->unlikely(nodep->name() == "uvm_report_error" || nodep->name() == "uvm_report_info"
                         || nodep->name() == "uvm_report_fatal"
                         || nodep->name() == "uvm_report_warning");
        اگر (nodep->dpiImport() || nodep->dpiExport()) {
            cfuncp->isStatic(سچ);
            cfuncp->isLoose(سچ);
        } ورنہ {
            cfuncp->isStatic(جھوٹ);
        }
        cfuncp->isVirtual(nodep->isVirtual());
        cfuncp->dpi_خالص(nodep->dpi_خالص());
        اگر (nodep->name() == "new") cfuncp->isConstructor(سچ);
        اگر (nodep->isCovergroupSample()) cfuncp->isCovergroupSample(سچ);
        اگر (cfuncp->dpiExportImpl()) cfuncp->cname(nodep->cname());

        اگر (cfuncp->dpiImportWrapper()) cfuncp->cname(nodep->cname());

        ثابت منطقی needSyms
            = nodep->needsSyms()
              && ((!nodep->dpiImport() && !nodep->task_عوامی()) || v3Global.opt.profExec());
        اگر (needSyms) cfuncp->argTypes(EmitCUtil::symClassVar());

        اگر (!nodep->dpiImport() && !nodep->task_عوامی()) {
            // Need symbol table
            اگر (cfuncp->isConstructor()) {
                منطقی isInterfaceClass = جھوٹ;
                اگر (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
                    isInterfaceClass = classp->isInterfaceClass();
                }
                اگر (!isInterfaceClass) {
                    ثابت string stmt = VIdProtect::protect("_ctor_var_reset") + "(vlSymsp);";
                    cfuncp->جوڑو_Stmtsp(
                        نیا AstCStmt{nodep->fileline(), stmt, VCStmtType::CTOR_VAR_RESET_CALL});
                }
            }
        }
        اگر (nodep->dpiContext()) {
            // First three args go to dpiContext call
            create_داخلہ_Var(cfuncp, "__Vscopep", VBasicDTypeKwd::SCOPEPTR);
            create_داخلہ_Var(cfuncp, "__Vfilenamep", VBasicDTypeKwd::CHARPTR);
            create_داخلہ_Var(cfuncp, "__Vlineno", VBasicDTypeKwd::INT);
        }

        اگر (nodep->dpiExport()) {
            AstScopeName* ثابت snp = nodep->scopeNamep();
            UASSERT_OBJ(snp, nodep, "Missing scoping context");
            snp->dpiExport(سچ);
            snp->unlink_Fr_عقب();
            cfuncp->scopeNamep(snp);
        }

        // Create list of arguments and move to function
        برائے (Ast_گرہ *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                اگر (portp->isParam() && VN_IS(portp->valuep(), ConsArray)) {
                    // Move array parameters in functions into constant pool
                    portp->unlink_Fr_عقب();
                    ڈالو_Deletep(portp);
                    portp->user2p(const_Pool_جدول(portp));
                } ورنہ {
                    اگر (portp->isIO()) {
                        // Move it to new function
                        unlinkAndClone(nodep, portp, جھوٹ);
                        portp->funcLocal(سچ);
                        اگر (portp->valuep()) ڈالو_Deletep(portp->valuep()->unlink_Fr_عقب());
                        cfuncp->جوڑو_Argsp(portp);
                        // Pass inputs to DPI import wrappers by reference, unless fits in register
                        اگر (cfuncp->dpiImportWrapper() && portp->isReadOnly()) {
                            Ast_گرہ_DType* ثابت dtypep = portp->dtypep()->skipRefp();
                            اگر (dtypep->isCompound() || dtypep->isWide()) {
                                portp->direction(VDirection::CONSTREF);
                            }
                        }
                    } ورنہ {
                        // "Normal" variable, mark inside function
                        portp->funcLocal(سچ);
                    }
                    AstVarScope* ثابت newvscp
                        = نیا AstVarScope{portp->fileline(), m_scopep, portp};
                    m_scopep->جوڑو_Varsp(newvscp);
                    portp->user2p(newvscp);
                }
            }
        }

        // Fake output variable if was a function.  It's more efficient to
        // have it last, rather than first, as the C compiler can sometimes
        // avoid copying variables when calling shells if argument 1
        // remains argument 1 (which it wouldn't if a return got added).
        اگر (rtnvarp) cfuncp->جوڑو_Argsp(rtnvarp);

        // Move body
        Ast_گرہ* bodysp = nodep->stmtsp();
        اگر (bodysp) {
            unlinkAndClone(nodep, bodysp, سچ);
            AstBegin* ثابت tempp
                = نیا AstBegin{nodep->fileline(), "[EditWrapper]", bodysp, جھوٹ};
            VL_DANGLING(bodysp);
            // If we cloned due to recursion, now need to rip out the ports
            // (that remained in place) then got cloned
            برائے (Ast_گرہ *nextp, *stmtp = tempp->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    اگر (portp->isIO()) VL_DO_DANGLING(ڈالو_Deletep(portp->unlink_Fr_عقب()), portp);
                }
            }
            اگر (tempp->stmtsp()) cfuncp->جوڑو_Stmtsp(tempp->stmtsp()->unlink_Fr_عقب_With_اگلا());
            VL_DO_DANGLING(tempp->delete_درخت(), tempp);
        }
        اگر (nodep->dpiImport()) bodyDpiImportFunc(nodep, rtnvscp, cfuncp, dpiFuncp);

        // Return statement
        اگر (rtnvscp && nodep->task_عوامی()) {
            cfuncp->جوڑو_Stmtsp(نیا AstCReturn{
                rtnvscp->fileline(), نیا AstVarRef{rtnvscp->fileline(), rtnvscp, VAccess::READ}});
        }
        // Replace variable refs
        relink(cfuncp);

        اگر (cfuncp->dpiExportImpl()) {
            // Mark all non-local variables written by the DPI exported function as being updated
            // by DPI exports. This ensures correct ordering and change detection later.

            // Mark non-local variables written by the exported function
            منطقی writesNonLocals = جھوٹ;
            cfuncp->foreach([&writesNonLocals](AstVarRef* refp) {
                AstVar* ثابت varp = refp->varScopep()->varp();
                // We are ignoring function locals as they should not be referenced anywhere
                // outside the enclosing AstCFunc, hence they are irrelevant for code ordering.
                اگر (varp->isFuncLocal()) واپس;
                // Check if written
                اگر (refp->access().isWriteOrRW()) {
                    // Mark it as written by DPI export
                    varp->سیٹ_Written_By_Dpi();
                    // Remember we had some
                    writesNonLocals = سچ;
                }
                // Check if read
                اگر (refp->access().isReadOrRW()) {
                    // Mark it as read by DPI export
                    varp->سیٹ_Read_By_Dpi();
                }
            });

            // If this DPI export writes some non-local variables, set the DPI Export Trigger flag
            // in the function.
            اگر (writesNonLocals) {
                AstVarScope* ثابت dpiExportTriggerp = getDpiExporTrigger();
                File_سطر* ثابت flp = cfuncp->fileline();

                // Set DPI export trigger flag every time the DPI export is called.
                Ast_تفویض* ثابت assignp
                    = نیا Ast_تفویض{flp, نیا AstVarRef{flp, dpiExportTriggerp, VAccess::WRITE},
                                    نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};

                // Add as first statement (to avoid issues with early returns) to exported function
                cfuncp->prependStmtsp(assignp);

                // With --sc, also wake the SystemC wrapper: it cannot see the flag, and may
                // otherwise sleep until its next timed slot. (usesTiming() is not known yet.)
                اگر (v3Global.opt.systemC()) {
                    assignp->جوڑو_اگلا_یہاں(نیا AstCStmt{
                        flp, "vlSymsp->__Vm_wakeEvent.notify(sc_core::SC_ZERO_TIME);\n"});
                }
            }
        }

        // Mark the fact that this function allocates std::process
        اگر (nodep->needProcess()) cfuncp->سیٹ_Need_Process();

        // Delete rest of cloned task and return new func
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        UINFOTREE(9, cfuncp, "", "userFunc");
        واپس cfuncp;
    }

    خالی iterateIntoFTask(Ast_گرہ_FTask* nodep) {
        // Iterate into the FTask we are calling.  Note it may be under a different
        // scope then the caller, so we need to restore state.
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_modp);
        VL_RESTORER(m_insStmtp);
        m_scopep = m_statep->getScope(nodep);
        m_modp = m_scopep->modp();
        iterate(nodep);
    }
    خالی داخل_کرو_Before_Stmt(Ast_گرہ* nodep, Ast_گرہ* newp) {
        UINFOTREE(9, nodep, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Function call not underneath a statement");
        UINFOTREE(9, newp, "", "newfunc");
        m_insStmtp->جوڑو_یہاں_This_As_اگلا(newp);
    }
    خالی process_دلائل(Ast_گرہ_FTask_Ref* nodep) {
        // Create a fresh variable for each concat array present in pins list
        برائے (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            AstConsArray* ثابت arrayp = VN_CAST(argp->exprp(), ConsArray);
            اگر (!arrayp) جاری;

            File_سطر* ثابت flp = arrayp->fileline();
            AstVarScope* ثابت substvscp = m_consArrayTmps.make(flp, m_scopep, arrayp->dtypep());
            substvscp->varp()->is_اندرونی(سچ);

            Ast_تفویض* ثابت assignp
                = نیا Ast_تفویض{flp, نیا AstVarRef{arrayp->fileline(), substvscp, VAccess::WRITE},
                                arrayp->unlink_Fr_عقب()};

            Ast_اظہاریہ_Stmt* ثابت exprstmtp = نیا Ast_اظہاریہ_Stmt{
                flp, assignp, نیا AstVarRef{arrayp->fileline(), substvscp, VAccess::READ}};
            argp->exprp(exprstmtp);
        }
    }

    منطقی isIfaceLocalImpure(Ast_گرہ_FTask* nodep, Ast_گرہ* impurep) {
        ثابت AstScope* ثابت scopep = m_statep->getScope(nodep);
        اگر (!TaskStateVisitor::isIfaceFTaskScope(scopep)) واپس جھوٹ;

        ثابت AstVarRef* ثابت refp = VN_CAST(impurep, VarRef);
        اگر (!refp || !refp->varScopep()) واپس جھوٹ;

        واپس refp->varScopep()->scopep() == scopep;
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    خالی دورہ(AstWith* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) {
            // Make sure that the return expression is converted only once
            واپس;
        }
        Ast_گرہ_اظہاریہ* ثابت withExprp = VN_AS(nodep->exprp()->unlink_Fr_عقب(), گرہ_اظہاریہ);
        nodep->جوڑو_Exprp(نیا AstCReturn{withExprp->fileline(), withExprp});
        iterateChildren(nodep);
    }
    خالی دورہ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
        UASSERT_OBJ(!m_insStmtp, nodep, "Didn't finish out last statement");
    }
    خالی دورہ(AstCNew* nodep) override {
        VL_RESTORER(m_اندر_New);
        m_اندر_New = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        اگر (m_اندر_Sensesp && !nodep->is_خالص()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Impure function calls in sensitivity lists");
            nodep->taskp(خالی_اشاریہ);  // So V3Broken doesn't complain
            واپس;
        }
        // Includes handling AstMethodCall, AstNew
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked?");
        iterateIntoFTask(nodep->taskp());  // First, do hierarchical funcs
        اگر (m_statep->ftaskNoInline(nodep->taskp()) && !m_statep->ftaskCFuncp(nodep->taskp())) {
            // An earlier error (e.g. a DPI import declared twice with conflicting
            // signatures) prevented creating the function, so remove the call
            UASSERT_OBJ(V3Error::error_گنتی(), nodep, "No non-inline task, but no error issued");
            اگر (VN_IS(nodep->backp(), Stmt_اظہاریہ)) {
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            } ورنہ {
                nodep->replaceWith(نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False_Erroring{}});
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            }
            واپس;
        }
        UINFO(4, " FTask REF   " << nodep);
        UINFOTREE(9, nodep, "", "inlfunc");
        UASSERT_OBJ(m_scopep, nodep, "func ref not under scope");
        // Create output variable
        AstVarScope* outvscp = خالی_اشاریہ;
        اگر (nodep->taskp()->isFunction()) {
            AstVar* ثابت fvarp = VN_AS(nodep->taskp()->fvarp(), Var);
            // If the call is on the RHS of a simple assignment 'lhs = call()',
            // the LHS variable can be reused as the output variable iff it has
            // the same type, and the function does not read the output variable itself.
            // This can be proven cheaply if the LHS is an automatic variable and the
            // function does not have any ref arguments. This arises a lot after V3LiftExpr.
            اگر (Ast_تفویض* ثابت assignp = VN_CAST(nodep->backp(), تفویض)) {
                اگر (AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef)) {
                    AstVarScope* ثابت vscp = lhsp->varScopep();
                    اگر (vscp->varp()->lifetime().isAutomatic()
                        && vscp->varp()->dtypep()->skipRefp()->same_درخت(
                            fvarp->dtypep()->skipRefp())
                        && !has_Ref_دلیل(nodep->taskp())) {
                        outvscp = vscp;
                    }
                }
            }
            // Otherwise create a new variable for the result
            اگر (!outvscp) outvscp = createVarScope(nodep, fvarp, "__Vfuncout");
        }
        // Create cloned statements
        Ast_گرہ* beginp;
        AstCNew* cnewp = خالی_اشاریہ;
        // getScope() is safe here: TaskStateVisitor stamped all FTask scopes before this pass.
        ثابت منطقی virtualIfaceCall
            = TaskStateVisitor::is_Virtual_Iface_طریقہ_Call(nodep)
              && TaskStateVisitor::isIfaceFTaskScope(m_statep->getScope(nodep->taskp()));
        اگر (m_statep->ftaskNoInline(nodep->taskp()) || virtualIfaceCall) {
            process_دلائل(nodep);
            // This may share VarScope's with a public task, if any.  Yuk.
            beginp = createNonInlinedFTask(nodep, outvscp, cnewp /*ref*/);
        } ورنہ {
            beginp = createInlinedFTask(nodep, outvscp);
            ++m_statInlines;
        }

        اگر (VN_IS(nodep, New)) {  // New not legal as while() condition
            داخل_کرو_Before_Stmt(nodep, beginp);
            UASSERT_OBJ(cnewp, nodep, "didn't create cnew for new");
            nodep->replaceWith(cnewp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ اگر (VN_IS(nodep->backp(), گرہ_تفویض)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like assign to non-function");
            داخل_کرو_Before_Stmt(nodep, beginp);
            AstVarRef* ثابت outrefp = نیا AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            nodep->replaceWith(outrefp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } ورنہ اگر (!VN_IS(nodep->backp(), Stmt_اظہاریہ)) {
            UASSERT_OBJ(nodep->taskp()->isFunction(), nodep,
                        "funcref-like expression to non-function");
            AstVarRef* ثابت outrefp = نیا AstVarRef{nodep->fileline(), outvscp, VAccess::READ};
            Ast_اظہاریہ_Stmt* lambdap = نیا Ast_اظہاریہ_Stmt{nodep->fileline(), beginp, outrefp};

            اگر (m_اندر_New) {
                AstVar* varp = outvscp->varp();

                // Create a new var that will be inside the lambda
                AstVar* newvarp = varp->clone_درخت(جھوٹ);
                newvarp->funcLocal(سچ);

                // Create a new VarScope for the new variable
                AstVarScope* ثابت newvscp
                    = نیا AstVarScope{newvarp->fileline(), m_scopep, newvarp};
                m_scopep->جوڑو_Varsp(newvscp);

                // Replace all references so they point to the new var and varscope
                lambdap->stmtsp()->foreach_And_اگلا([outvscp, newvscp](AstVarRef* refp) {
                    اگر (refp->varScopep() == outvscp) {
                        refp->varScopep(newvscp);
                        refp->varp(newvscp->varp());
                    }
                });

                // Add variable initialization
                lambdap->prependStmtsp(newvarp);
                lambdap->has_نتیجہ(جھوٹ);

                // Add return statement - TODO: Why not VarRef(outvscp)?
                AstCExpr* ثابت exprp = نیا AstCExpr{nodep->fileline(), varp->name()};
                exprp->dtype_سیٹ_String();
                lambdap->جوڑو_Stmtsp(نیا AstCReturn{nodep->fileline(), exprp});
            }

            // AstExprStmt is currently treated as impure, so clear the cached purity of its
            // parents
            nodep->replaceWith(lambdap);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            VIsCached::clear_Cache_درخت();
        } ورنہ {  // VN_IS(nodep->backp(), StmtExpr)
            داخل_کرو_Before_Stmt(nodep, beginp);
            اگر (nodep->taskp()->isFunction()
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_رمز::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
            }
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
        UINFO(4, "  FTask REF Done.");
    }
    خالی دورہ(Ast_گرہ_FTask* ثابت nodep) override {
        UINFO(4, " visitFTask   " << nodep);
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep->stmtsp();  // Might be null if no statements, but we won't use it
        اگر (!nodep->user1_سیٹ_Once()) {  // Just one creation needed per function
            // Expand functions in it
            صحیح modes = 0;
            اگر (nodep->dpiImport()) modes++;
            اگر (nodep->dpiExport()) modes++;
            اگر (nodep->task_عوامی()) modes++;
            اگر (nodep->class_طریقہ()) modes++;
            اگر (v3Global.opt.protectIds() && nodep->task_عوامی()) {
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
            اگر (noInline && !nodep->class_طریقہ() && !nodep->recursive()) {
                اگر (Ast_گرہ* ثابت impurep = m_statep->checkImpure(nodep)) {
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
                || nodep->task_عوامی()) {
                // Clone it first, because we may have later FTaskRef's that still need
                // the original version.
                Ast_گرہ_FTask* ثابت clonedFuncp = nodep->clone_درخت(جھوٹ);
                اگر (nodep->isConstructor()) m_statep->remapFuncClassp(nodep, clonedFuncp);
                ثابت منطقی makeNonInlineFunc = noInline || needsNonInlineCFunc;
                اگر (AstCFunc* ثابت cfuncp = makeUserFunc(clonedFuncp, makeNonInlineFunc)) {
                    nodep->جوڑو_اگلا_یہاں(cfuncp);
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
                    AstVarScope* ثابت vscp = m_statep->ڈھونڈو_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ڈالو_Deletep(vscp->unlink_Fr_عقب()), vscp);
                }
            }
            برائے (Ast_گرہ *nextp, *stmtp = nodep->stmtsp(); stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                    AstVarScope* ثابت vscp = m_statep->ڈھونڈو_Var_Scope(m_scopep, portp);
                    UINFO(9, "   funcremovevsc " << vscp);
                    VL_DO_DANGLING(ڈالو_Deletep(vscp->unlink_Fr_عقب()), vscp);
                }
            }
            // Just push for deletion, as other references to func may
            // remain until visitor exits
            nodep->unlink_Fr_عقب();
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }
    خالی دورہ(Ast_تفویض_Force* nodep) override {
        // Force statements cannot be converted to always blocks outside of a logic block
        // This causes function calls on RHS of force assignments to be improperly inlined and
        // called just once. To prevent this, we create a temporary variable for each function
        // reference on RHS. This variable is declared in the nearest scope and gets a continuous
        // assignment of the function, so it can be converted to always and properly inlined This
        // temporary variable becomes the RHS of the force assignment
        std::سمتیہ<Ast_گرہ_FTask_Ref*> refs;
        nodep->rhsp()->foreach([&refs](Ast_گرہ_FTask_Ref* refp) { refs.ڈالو_عقب(refp); });
        برائے (Ast_گرہ_FTask_Ref* ثابت refp : refs) {

            // Create the temporary variable and its scope. Every function call gets its own
            // variable in each scope, which is shared with the same call in other instances.
            AstVarScope* ثابت interVscp
                = m_forceTmps.make(refp->fileline(), m_scopep, refp->dtypep(), refp->name());

            // Recompute the helper in a combo block so any inlined function body stays
            // inside schedulable logic rather than spilling statements at module scope.
            Ast_تفویض* ثابت assignp = نیا Ast_تفویض{
                nodep->fileline(), نیا AstVarRef{nodep->fileline(), interVscp, VAccess::WRITE},
                nodep->rhsp()->clone_درخت_خالص(جھوٹ)};
            Ast_Sen_درخت* ثابت senTreep = نیا Ast_Sen_درخت{
                nodep->fileline(), نیا Ast_Sen_شے{nodep->fileline(), Ast_Sen_شے::Combo{}}};
            Ast_فعال* ثابت activep
                = نیا Ast_فعال{nodep->fileline(), "force-func-update", senTreep};
            activep->sen_درخت_Storep(activep->sentreep());
            activep->جوڑو_Stmtsp(
                نیا AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, خالی_اشاریہ, assignp});
            m_scopep->جوڑو_Blocksp(activep);

            // Replace RHS of force assignment with the temporary variable
            refp->replaceWith(نیا AstVarRef{nodep->fileline(), interVscp, VAccess::READ});
            VL_DO_DANGLING(refp->delete_درخت(), refp);
        }
    }
    خالی دورہ(Ast_گرہ_Foreach* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc(
            "Foreach statements should have been converted to while statements in V3Begin.cpp");
    }
    خالی دورہ(Ast_گرہ_Stmt* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateChildren(nodep);
        اگر (!nodep->exprp()) VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
    }
    خالی دورہ(Ast_Sen_شے* nodep) override {
        UASSERT_OBJ(!m_اندر_Sensesp, nodep, "Senitem under senitem?");
        VL_RESTORER(m_اندر_Sensesp);
        m_اندر_Sensesp = سچ;
        iterateChildren(nodep);
    }
    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    TaskVisitor(AstNetlist* nodep, TaskStateVisitor* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~TaskVisitor() {
        V3Stats::جوڑو_Stat("Optimizations, Functions inlined", m_statInlines);
        V3Stats::جوڑو_Stat("Optimizations, Hierarchical DPI wrappers with costs",
                         m_statHierDpisWithCosts);
    }
};

//######################################################################
// Mark interface members under timing controls of interface CFuncs as interface-sensed

جماعت TaskIfaceSensVisitor final : عوامی VNVisitorConst {
    // STATE
    منطقی m_underIfaceFunc = جھوٹ;  // Under a CFunc owned by an interface scope
    منطقی m_underSenses = جھوٹ;  // Under a sensitivity expression of such a CFunc

    // METHODS
    خالی markSensesAndIterate(Ast_گرہ* nodep) {
        اگر (!m_underIfaceFunc) واپس;
        VL_RESTORER(m_underSenses);
        m_underSenses = سچ;
        iterate_And_اگلا_Const_Null(nodep);
    }
    // VISITORS
    خالی دورہ(AstCFunc* nodep) override {
        VL_RESTORER(m_underIfaceFunc);
        m_underIfaceFunc = VN_IS(nodep->scopep()->modp(), Iface);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_Sen_درخت* nodep) override { markSensesAndIterate(nodep->sensesp()); }
    خالی دورہ(AstWait* nodep) override {
        markSensesAndIterate(nodep->condp());
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
    }
    خالی دورہ(AstVarRef* nodep) override {
        اگر (!m_underSenses) واپس;
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope");
        // Keep temps: the clocking event var is a MODULETEMP
        اگر (nodep->varp()->isFuncLocal()) واپس;
        اگر (AstIface* ثابت ifacep = VN_CAST(nodep->varScopep()->scopep()->modp(), Iface)) {
            nodep->varp()->sensIfacep(ifacep);
        }
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح TaskIfaceSensVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~TaskIfaceSensVisitor() override = طےشدہ;
};

//######################################################################
// Task class functions

ثابت حرف* ثابت V3Task::s_dpiTemporaryVarSuffix = "__Vcvt";

V3TaskConnects V3Task::taskConnects(Ast_گرہ_FTask_Ref* nodep, Ast_گرہ* taskStmtsp,
                                    V3TaskConnectState* statep, منطقی makeChanges) {
    // Output list will be in order of the port declaration variables (so
    // func calls are made right in C)
    // Missing pin/expr?  We return (pinvar, nullptr)
    // Extra   pin/expr?  We clean it up
    UINFO(9, "taskConnects " << nodep);
    std::map<ثابت std::string, صحیح> name_مقصد_اشاریہ;
    V3TaskConnects tconnects;
    UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");

    // Find ports
    صحیح tpinnum = 0;
    AstVar* sformatp = خالی_اشاریہ;
    برائے (Ast_گرہ* stmtp = taskStmtsp; stmtp; stmtp = stmtp->nextp()) {
        اگر (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
            اگر (portp->isIO()) {
                tconnects.emplace_عقب(portp, static_cast<AstArg*>(خالی_اشاریہ));
                name_مقصد_اشاریہ.emplace(portp->name(), tpinnum);  // For name based connections
                ++tpinnum;
                اگر (portp->attrSFormat()) {
                    sformatp = portp;
                } ورنہ اگر (sformatp) {
                    portp->v3error("/*verilator sformat*/ can only be applied to last argument of "
                                   "a function");
                }
            }
        }
    }

    // Find pins
    صحیح ppinnum = 0;
    منطقی reorganize = جھوٹ;
    برائے (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
        nextp = VN_AS(argp->nextp(), Arg);
        اگر (!argp->name().خالی_خانے()) {
            // By name
            ثابت خودکار it = name_مقصد_اشاریہ.ڈھونڈو(argp->name());
            اگر (it == name_مقصد_اشاریہ.انتہا()) {
                اگر (makeChanges) {
                    argp->v3error(
                        "No such argument "
                        << argp->prettyNameQ() << " in call to " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << '\n'
                        << nodep->warnContextPrimary() << '\n'
                        << nodep->warn_مزید() << "... Location of " << nodep->taskp()->verilogKwd()
                        << " " << nodep->taskp()->prettyNameQ() << " declaration\n"
                        << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_عقب()->delete_درخت(), argp);
                }
            } ورنہ {
                اگر (tconnects[it->دوسرا].دوسرا && makeChanges) {
                    argp->v3error("Duplicate argument " << argp->prettyNameQ()
                                                        << " in function call to "
                                                        << nodep->taskp()->prettyTypeName());
                    tconnects[it->دوسرا].دوسرا->unlink_Fr_عقب()->delete_درخت();
                }
                tconnects[it->دوسرا].دوسرا = argp;
                reorganize = سچ;
            }
        } ورنہ {  // By pin number
            اگر (nodep->taskp()->prettyName() == "randomize") {
                // Arguments to randomize() are special, will be handled in V3Randomize
            } ورنہ اگر (ppinnum >= tpinnum) {
                اگر (sformatp) {
                    tconnects.emplace_عقب(sformatp, static_cast<AstArg*>(خالی_اشاریہ));
                    tconnects[ppinnum].دوسرا = argp;
                    ++tpinnum;
                } ورنہ اگر (makeChanges) {
                    argp->v3error("Too many arguments in call to "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << '\n'
                                  << nodep->warnContextPrimary() << '\n'
                                  << nodep->warn_مزید() << "... Location of "
                                  << nodep->taskp()->verilogKwd() << " "
                                  << nodep->taskp()->prettyNameQ() << " declaration:\n"
                                  << nodep->taskp()->warnContextSecondary());
                    // We'll just delete it; seems less error prone than making a false argument
                    VL_DO_DANGLING(argp->unlink_Fr_عقب()->delete_درخت(), argp);
                }
            } ورنہ {
                tconnects[ppinnum].دوسرا = argp;
            }
        }
        ++ppinnum;
    }

    اگر (!makeChanges) واپس tconnects;

    // Connect missing ones
    VInsertion_سیٹ<ثابت AstVar*> argWrap;  // Which ports are defaulted; need arg wrapper creation
    برائے (صحیح i = 0; i < tpinnum; ++i) {
        AstVar* ثابت portp = tconnects[i].پہلا;
        اگر (!tconnects[i].دوسرا || !tconnects[i].دوسرا->exprp()) {
            Ast_گرہ_اظہاریہ* newvaluep = خالی_اشاریہ;
            اگر (!portp->valuep()) {
                nodep->v3error("Missing argument on non-defaulted argument "
                               << portp->prettyNameQ() << " in function call to "
                               << nodep->taskp()->prettyTypeName());
                newvaluep = نیا AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
            } ورنہ اگر (AstFuncRef* ثابت funcRefp = VN_CAST(portp->valuep(), FuncRef)) {
                ثابت Ast_گرہ_FTask* ثابت funcp = funcRefp->taskp();
                اگر (funcp->class_طریقہ() && funcp->isStatic()) newvaluep = funcRefp;
            } ورنہ اگر (AstConst* ثابت constp = VN_CAST(portp->valuep(), Const)) {
                newvaluep = constp;
            }

            اگر (!newvaluep) {
                // The default value for this port might be a constant
                // expression that hasn't been folded yet. Try folding it
                // now; we don't have much to lose if it fails.
                newvaluep = V3Const::constifyEdit(VN_AS(portp->valuep(), گرہ_اظہاریہ));
                اگر (!VN_IS(newvaluep, Const)) {
                    اگر (statep) {
                        portp->pin_عدد(i + 1);  // Make sure correct, will use to build name
                        UINFO(9, "taskConnects arg wrapper needed " << portp->valuep());
                        argWrap.داخل_کرو(portp);
                    } ورنہ {  // statep = nullptr, called too late or otherwise to handle args
                        // Problem otherwise is we might have a varref, task
                        // call, or something else that only makes sense in the
                        // domain of the function (or class containing the method),
                        // versus that of the callee.
                        nodep->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-constant default value in missing argument "
                                << portp->prettyNameQ() << " in function call to "
                                << nodep->taskp()->prettyTypeName());
                        newvaluep = نیا AstConst{nodep->fileline(), AstConst::Unsized32{}, 0};
                    }
                }
            }
            newvaluep = newvaluep->backp() ? newvaluep->clone_درخت(سچ) : newvaluep;
            // To avoid problems with callee needing to know to deleteTree
            // or not, we make this into a pin
            UINFO(9, "Default pin for " << portp);
            AstArg* ثابت newp = نیا AstArg{nodep->fileline(), portp->name(), newvaluep};
            اگر (tconnects[i].دوسرا) {  // Have a "nullptr" pin already defined for it
                VL_DO_CLEAR(tconnects[i].دوسرا->unlink_Fr_عقب()->delete_درخت(),
                            tconnects[i].دوسرا = خالی_اشاریہ);
            }
            tconnects[i].دوسرا = newp;
            reorganize = سچ;
        }
        اگر (tconnects[i].دوسرا) {
            UINFO(9, "Connect " << portp << "  ->  " << tconnects[i].دوسرا);
        } ورنہ {
            UINFO(9, "Connect " << portp << "  ->  NONE");
        }
    }

    برائے (ثابت خودکار& tconnect : tconnects) {
        AstArg* ثابت argp = tconnect.دوسرا;
        argp->name("");  // Can forget name as will add back in pin order
    }

    اگر (reorganize) {
        // To simplify downstream, put argument list back into pure pinnumber ordering
        جب (nodep->argsp()) {
            // Must unlink each pin, not all pins linked together as one list
            nodep->argsp()->unlink_Fr_عقب();
        }
        برائے (صحیح i = 0; i < tpinnum; ++i) {
            AstArg* ثابت argp = tconnects[i].دوسرا;
            UASSERT_OBJ(argp, nodep, "Lost argument in func conversion");
            nodep->جوڑو_Argsp(argp);
        }
    }

    اگر (debug() >= 9) {  // LCOV_EXCL_START
        nodep->dump_درخت("-  ftref-out: ");
        برائے (صحیح i = 0; i < tpinnum; ++i) {
            UINFO(0, "   pin " << i << "  pin=" << cvt_مقصد_Hex(tconnects[i].پہلا)
                               << "  conn=" << cvt_مقصد_Hex(tconnects[i].دوسرا));
        }
    }  // LCOV_EXCL_STOP

    اگر (!argWrap.خالی_خانے()) {
        UINFO(9, "Arg wrapper generation " << nodep);
        // Create wrapper function with default argument settings.
        // Needed because the default needs symbol table of the called function.
        taskConnectWrap(nodep, tconnects, statep, argWrap);
        // Regenerate all connections, this time connecting to the wrapper
        واپس taskConnects(nodep, nodep->taskp()->stmtsp(),
                            // statep null, so can't recurse forever
                            خالی_اشاریہ);
    }
    واپس tconnects;
}

خالی V3Task::taskConnectWrap(Ast_گرہ_FTask_Ref* nodep, ثابت V3TaskConnects& tconnects,
                             V3TaskConnectState* statep,
                             ثابت VInsertion_سیٹ<ثابت AstVar*>& argWrap) {
    statep->سیٹ_Did_Wrap();
    // Make wrapper name such that is same iff same args are defaulted
    std::string newname = nodep->name() + "__Vtcwrap";
    برائے (ثابت AstVar* varp : argWrap) newname += "_" + cvt_مقصد_Str(varp->pin_عدد());
    ثابت خودکار pair = statep->wrapMap().emplace(std::piecewise_construct,
                                                std::forward_as_tuple(nodep->taskp(), newname),
                                                std::forward_as_tuple(خالی_اشاریہ));
    اگر (pair.دوسرا) {
        pair.پہلا->دوسرا = taskConnectWrapNew(nodep->taskp(), newname, tconnects, argWrap);
    }
    Ast_گرہ_FTask* ثابت newTaskp = pair.پہلا->دوسرا;

    // Remove the defaulted arguments from original outside call
    برائے (ثابت خودکار& tconnect : tconnects) {
        ثابت AstVar* ثابت portp = tconnect.پہلا;
        AstArg* ثابت argp = tconnect.دوسرا;
        اگر (argWrap.exists(portp)) {  // Removed arg
            statep->ڈالو_Deletep(argp->unlink_Fr_عقب());
        }
    }
    // Change outside call to connect to new function
    nodep->taskp(newTaskp);
    nodep->name(newTaskp->name());
    // UINFOTREE(9, nodep, "", "taskConnectWrap-call");
}

Ast_گرہ_FTask* V3Task::taskConnectWrapNew(Ast_گرہ_FTask* taskp, ثابت string& newname,
                                         ثابت V3TaskConnects& tconnects,
                                         ثابت VInsertion_سیٹ<ثابت AstVar*>& argWrap) {
    std::map<ثابت AstVar*, AstVar*> oldNewVars;  // Old -> new var mappings

    Ast_گرہ_FTask* ثابت newTaskp = taskp->cloneType(newname);
    newTaskp->propagateAttrFrom(taskp);
    taskp->جوڑو_اگلا_یہاں(newTaskp);

    Ast_گرہ_FTask_Ref* newCallp = خالی_اشاریہ;
    Ast_گرہ* newCallInsertp = خالی_اشاریہ;
    اگر (VN_IS(taskp, Func)) {
        AstVar* ثابت fvarp = VN_AS(taskp->fvarp(), Var);
        UASSERT_OBJ(fvarp, taskp, "FuncRef without fvar");
        AstVar* ثابت newFVarp = fvarp->clone_درخت(سچ);
        oldNewVars.emplace(fvarp, newFVarp);
        newFVarp->name(newTaskp->name());
        newTaskp->fvarp(newFVarp);
        newTaskp->dtypeFrom(newFVarp);
        newCallp = نیا AstFuncRef{taskp->fileline(), VN_AS(taskp, Func)};
        newCallInsertp
            = نیا Ast_تفویض{taskp->fileline(),
                            نیا AstVarRef{fvarp->fileline(), newFVarp, VAccess::WRITE}, newCallp};
        newCallInsertp->dtypeFrom(newFVarp);
    } ورنہ اگر (VN_IS(taskp, Task)) {
        newCallp = نیا AstTaskRef{taskp->fileline(), VN_AS(taskp, Task)};
        newCallInsertp = نیا Ast_Stmt_اظہاریہ{taskp->fileline(), newCallp};
    } ورنہ {
        taskp->v3fatalSrc("Unsupported: Non-constant default value in missing argument in a "
                          << taskp->prettyTypeName());
    }

    // Create wrapper's ports matching original's
    برائے (ثابت خودکار& tconnect : tconnects) {
        AstVar* ثابت portp = tconnect.پہلا;
        AstVar* newPortp;
        اگر (!argWrap.exists(portp)) {  // Not removed arg
            newPortp = نیا AstVar{portp->fileline(), portp->varType(), portp->name(), portp};
            newPortp->propagateWrapAttrFrom(portp);
            newPortp->funcLocal(سچ);
            اگر (newPortp->valuep()) newPortp->valuep()->unlink_Fr_عقب()->delete_درخت();
            newTaskp->جوڑو_Stmtsp(newPortp);
        } ورنہ {  // Defaulting arg
            Ast_گرہ_اظہاریہ* ثابت valuep = VN_AS(portp->valuep(), گرہ_اظہاریہ);
            اگر ((portp->isRef() || portp->isConstRef()) && VN_IS(valuep, VarRef)) {
                ثابت VAccess refAccess = portp->direction().pinAccess();
                AstVarRef* ثابت refp = VN_AS(valuep->clone_درخت(جھوٹ), VarRef);
                refp->access(refAccess);
                AstArg* ثابت newArgp = نیا AstArg{portp->fileline(), portp->name(), refp};
                newCallp->جوڑو_Argsp(newArgp);
                جاری;
            }
            // Create local temporary
            newPortp = نیا AstVar{portp->fileline(), VVarType::BLOCKTEMP, portp->name(),
                                  portp->dtypep()};
            newPortp->propagateAttrFrom(portp);
            newPortp->funcLocal(سچ);
            newTaskp->جوڑو_Stmtsp(newPortp);
            // Runtime-assign it to the default
            اگر (!VN_IS(valuep, خالی_خانے_قطار)) {
                Ast_تفویض* ثابت newAssignp
                    = نیا Ast_تفویض{valuep->fileline(),
                                    نیا AstVarRef{valuep->fileline(), newPortp, VAccess::WRITE},
                                    valuep->clone_درخت(سچ)};
                newTaskp->جوڑو_Stmtsp(newAssignp);
            }
        }
        oldNewVars.emplace(portp, newPortp);
        AstArg* ثابت newArgp = نیا AstArg{
            portp->fileline(), portp->name(),
            نیا AstVarRef{portp->fileline(), newPortp, portp->direction().pinAccess()}};
        newCallp->جوڑو_Argsp(newArgp);
    }
    // Create wrapper call to original, passing arguments, adding setting of return value
    newTaskp->جوڑو_Stmtsp(newCallInsertp);
    // Replace any varref's to original to new ports (e.g. in argument equations)
    newTaskp->foreach([=](AstVarRef* refp) {
        ثابت خودکار it = oldNewVars.ڈھونڈو(refp->varp());
        اگر (it != oldNewVars.انتہا()) refp->varp(it->دوسرا);
    });
    // UINFOTREE(9, newTaskp, "", "taskConnectWrap-new");
    واپس newTaskp;
}

string V3Task::تفویض_اندرونی_مقصد_Dpi(AstVar* portp, منطقی isPtr, ثابت string& frSuffix,
                                   ثابت string& مقصد_Suffix, ثابت string& fr_سابقہ) {
    // Create assignment from internal format into DPI temporary
    // Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
    // DPI temporary is scalar or 1D array (if unpacked array)
    string stmt;
    string ket;
    // Someday we'll have better type support, and this can make variables and casts.
    // But for now, we'll just text-bash it.
    ثابت string frName = fr_سابقہ + portp->name() + frSuffix;
    ثابت string مقصد_Name = portp->name() + مقصد_Suffix;
    ثابت string idx = portp->name() + "__Vidx";
    سائز_t unpack_سائز = 1;  // non-unpacked array is treated as size 1
    صحیح unpackDim = 0;
    اگر (AstUnpackArrayDType* ثابت unpackp
        = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
        unpack_سائز = unpackp->arrayUnpackedElements();
        unpackDim = unpackp->dimensions(جھوٹ).دوسرا;
        اگر (unpackDim > 0) UASSERT_OBJ(unpack_سائز > 0, portp, "size must be greater than 0");
    }
    اگر (portp->basicp()->is_Dpi_ثنائی_ہندسہ_Vec() || portp->basicp()->isDpiLogicVec()) {
        ثابت منطقی is_ثنائی_ہندسہ = portp->basicp()->is_Dpi_ثنائی_ہندسہ_Vec();
        ثابت منطقی needsFor = unpack_سائز > 1;
        اگر (needsFor) {
            stmt = "for (size_t " + idx + " = 0; " + idx + " < " + cvt_مقصد_Str(unpack_سائز) + "; ++"
                   + idx + ") ";
        }

        stmt += (is_ثنائی_ہندسہ ? "VL_SET_SVBV_"s : "VL_SET_SVLV_"s)
                + portp->dtypep()->skipRefp()->charIQWN() + "(" + cvt_مقصد_Str(portp->چوڑائی()) + ", ";
        stmt += مقصد_Name;
        اگر (needsFor) {
            stmt += " + " + cvt_مقصد_Str(portp->dtypep()->skipRefp()->چوڑائی_الفاظ()) + " * " + idx;
        }
        stmt += ", ";
        اگر (unpackDim > 0) {  // Access multi-dimensional array as a 1D array
            stmt += "(&" + frName;
            برائے (صحیح i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")[" + (needsFor ? idx : "0") + "])";
        } ورنہ {
            stmt += frName + ")";
        }
    } ورنہ {
        ثابت منطقی isChandle
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::CHANDLE;
        ثابت منطقی isString
            = portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING;
        ثابت string unpackAt = unpack_سائز > 1 ? "[" + idx + "]" : "[0]";
        اگر (unpackDim > 0) {
            ثابت string forStmt = unpack_سائز > 1
                                       ? "for (size_t " + idx + " = 0; " + idx + " < "
                                             + cvt_مقصد_Str(unpack_سائز) + "; ++" + idx + ") "
                                       : "";
            stmt += forStmt + مقصد_Name + unpackAt;
        } ورنہ {
            اگر (isPtr) stmt += "*";  // DPI outputs are pointers
            stmt += مقصد_Name;
        }
        stmt += " = ";
        اگر (isChandle) {
            stmt += "VL_CVT_Q_VP(";
            ket += ")";
        }
        اگر (unpackDim > 0) {
            stmt += "(&" + frName;
            برائے (صحیح i = 0; i < unpackDim; ++i) stmt += "[0]";
            stmt += ")" + unpackAt;
        } ورنہ {
            stmt += frName;
        }
        اگر (isString) stmt += ".c_str()";
    }
    stmt += ket + ";";
    واپس stmt;
}

// Create assignment from DPI temporary into internal format
// DPI temporary is scalar or 1D array (if unpacked array)
// Internal representation is scalar, 1D, or multi-dimensional array (similar to SV)
std::string V3Task::تفویض_Dpi_مقصد_اندرونی(ثابت std::string& lhsName, AstVar* varp) {
    std::string cvt;
    منطقی useSvVec;
    std::tie(cvt, useSvVec) = TaskDpiUtils::dpi_مقصد_اندرونی_Cvt_Stmt(varp->dtypep());
    ثابت std::سمتیہ<std::pair<صحیح, صحیح>> strides = TaskDpiUtils::unpackDimsAndStrides(varp);
    ثابت صحیح کل = strides.خالی_خانے() ? 1 : strides[0].پہلا * strides[0].دوسرا;
    ثابت صحیح چوڑائی_الفاظ = varp->basicp()->چوڑائی_الفاظ();
    string statements;
    برائے (صحیح i = 0; i < کل; ++i) {
        std::string lhs = lhsName;
        // extract a scalar from multi-dimensional array (internal format)
        برائے (ثابت خودکار& stride : strides) {
            lhs += "[" + std::مقصد_string((i / stride.دوسرا) % stride.پہلا) + "]";
        }
        // extract a scalar from DPI temporary var that is scalar or 1D array
        اگر (useSvVec) {
            ثابت std::string انحراف = std::مقصد_string(i * چوڑائی_الفاظ);
            statements += cvt + " " + lhs + ", " + varp->name() + " + " + انحراف + ");\n";
        } ورنہ {
            ثابت std::string elem = strides.خالی_خانے() ? "" : "[" + std::مقصد_string(i) + "]";
            statements += lhs + " = " + cvt + varp->name() + elem + ")" + ";\n";
        }
    }
    واپس statements;
}

خالی V3Task::taskAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        TaskStateVisitor visitors{nodep};
        ثابت TaskVisitor visitor{nodep, &visitors};
    }  // Destruct before checking
    { TaskIfaceSensVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("task", 0, dump_درخت_Either_Level() >= 3);
}
