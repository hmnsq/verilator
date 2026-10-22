// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Create separate tasks for forked processes that
//              can outlive their parents
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
// V3Fork's Transformations:
//
// Each module:
//      Look for FORKs [JOIN_NONE]/[JOIN_ANY]
//          VARREF(var) -> MEMBERSEL(var->name, VARREF(dynscope)) (for write/RW refs)
//          FORK(stmts) -> TASK(stmts), FORK(TASKREF(inits))
//
// FORKs that spawn tasks which might outlive their parents require those
// tasks to carry their own frames and as such they require their own
// variable scopes.
//
// There are two mechanisms that work together to achieve that. ForkVisitor
// moves bodies of forked processes into new tasks, which results in them getting their
// own scopes. The original statements get replaced with a call to the task which
// passes the required variables by value.
//
// The second mechanism, DynScopeVisitor, is designed to handle variables which can't be
// captured by value and instead require a reference. Those variables get moved into an
// "anonymous" object, ie. a class with appropriate fields gets generated and an object
// of this class gets instantiated in place of the original variable declarations.
// Any references to those variables are replaced with references to the object's field.
// Since objects are reference-counted this ensures that the variables are accessible
// as long as both the parent and the forked processes require them to be.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Fork.h"

#ضمن "V3AstNodeExpr.h"
#ضمن "V3MemberMap.h"

#ضمن <limits>
#ضمن <set>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف ForkDynScopeInstance final {
عام:
    AstClass* m_classp = اشاره_تهی;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = اشاره_تهی;  // RefDType for the above
    AstVar* m_handlep = اشاره_تهی;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    منطقي initialized() ثابت { أرجع m_classp != اشاره_تهی; }
};

صنف Fork_Dyn_Scope_قاب final {
    // MEMBERS
    Ast_گره_Module* ثابت m_modp;  // Module to insert the scope into
    Ast_گره* ثابت m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_capture_ترتیب;  // Variables to be moved into the dynscope
    std::مجموعه<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ثابت اندازه_t m_class_id;  // Dynscope class ID
    ثابت اندازه_t m_id;  // Dynscope ID

عام:
    Fork_Dyn_Scope_قاب(Ast_گره_Module* modp, Ast_گره* procp, اندازه_t class_id, اندازه_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = جديد AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = جديد AstClassRefDType{m_procp->fileline(), m_instance.m_classp, اشاره_تهی};
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = جديد AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(خطأ);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        أرجع m_instance;
    }

    ثابت ForkDynScopeInstance& instance() ثابت { أرجع m_instance; }
    فراغ capture_Var_درج(AstVar* varp) {
        خودکار r = m_captures.emplace(varp);
        إذا (r.دومی) m_capture_ترتیب.افزودن_پشت(varp);
    }
    منطقي captured(AstVar* varp) { أرجع m_captures.شمار(varp) != 0; }
    Ast_گره* procp() ثابت { أرجع m_procp; }

    فراغ populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        لكل (AstVar* varp : m_capture_ترتیب) {
            إذا (varp->direction().isAny()) {
                varp = varp->clone_درخت(خطأ);
                varp->direction(VDirection::NONE);
            } وإلا {
                varp->unlink_Fr_پشت();
            }
            varp->funcLocal(خطأ);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->استفاده_شده_Loop_Idx(خطأ);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->جمع_کن_Stmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ثابت newp
            = جديد AstFunc{m_instance.m_classp->fileline(), "new", اشاره_تهی, اشاره_تهی};
        newp->isConstructor(درست);
        newp->class_روش(درست);
        newp->dtypep(newp->پیدا_کن_Void_DType());
        m_instance.m_classp->جمع_کن_Stmtsp(newp);
    }

    فراغ linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        إذا (AstFork* ثابت forkp = VN_CAST(m_procp, Fork)) {
            link_Nodes_از_Fork(memberMap, forkp);
            أرجع;
        }

        Ast_گره* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_پشت_With_بعدی(&stmtpHandle);

        // Find node after last variable declaration
        Ast_گره* initp = stmtp;
        طالما (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ثابت newp = جديد AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.پیدا_کن_Member(m_instance.m_classp, "new"), گره_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_گره* ثابت asgnp = جديد Ast_تخصیص_بده{
            m_procp->fileline(),
            جديد AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_گره* initsp = اشاره_تهی;  // Arguments need to be copied
        لكل (AstVar* varp : m_capture_ترتیب) {
            إذا (!varp->direction().isAny()) استمر;

            إذا (varp->direction().isNonOutput()) {
                AstMemberSel* ثابت memberselp = جديد AstMemberSel{
                    varp->fileline(),
                    جديد AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.پیدا_کن_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_گره* initAsgnp
                    = جديد Ast_تخصیص_بده{varp->fileline(), memberselp,
                                    جديد AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_گره::جمع_کن_بعدی(initsp, initAsgnp);
            }

            إذا (AstBasicDType* ثابت dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.مجموعه_Assigns_Events();
                إذا (dtypep->isEvent()) استمر;
            }

            إذا (varp->direction().isWritable()) {
                AstMemberSel* ثابت memberselp = جديد AstMemberSel{
                    varp->fileline(),
                    جديد AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.پیدا_کن_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_گره* writebackAsgnp = جديد Ast_تخصیص_بده{
                    varp->fileline(), جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_گره::جمع_کن_بعدی(stmtp, writebackAsgnp);
            }
        }
        إذا (initsp) Ast_گره::جمع_کن_بعدی(asgnp, initsp);

        إذا (initp != stmtp) {
            initp->جمع_کن_اینجا_This_As_بعدی(asgnp);
        } وإلا {
            Ast_گره::جمع_کن_بعدی(asgnp, static_cast<Ast_گره*>(initp));
            stmtp = asgnp;
        }

        Ast_گره::جمع_کن_بعدی(static_cast<Ast_گره*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->جمع_کن_Stmtsp(m_instance.m_classp);
    }

    منطقي linked() ثابت { أرجع m_instance.initialized() && m_instance.m_handlep->backp(); }

خاص:
    Ast_تخصیص_بده* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ثابت newp = جديد AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.پیدا_کن_Member(m_instance.m_classp, "new"), گره_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        أرجع جديد Ast_تخصیص_بده{
            m_procp->fileline(),
            جديد AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    فراغ link_Nodes_از_Fork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ثابت std::string name = "_Vwrapped_"  //
                                 + (forkp->name().خالی() ? "" : forkp->name() + "_")  //
                                 + std::مقصد_string(m_id);
        AstBegin* ثابت beginp = جديد AstBegin{forkp->fileline(), name, m_instance.m_handlep, درست};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->جمع_کن_Stmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        إذا (forkp->stmtsp()) beginp->جمع_کن_Stmtsp(forkp->stmtsp()->unlink_Fr_پشت_With_بعدی());
        // Put the Fork back at the end of the Begin
        beginp->جمع_کن_Stmtsp(forkp);
        m_modp->جمع_کن_Stmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { أرجع "__VDynScope_" + cvt_مقصد_Str(m_class_id); }

    string generateDynScopeHandleName(ثابت Ast_گره* fromp) {
        أرجع "__VDynScope_" + (!fromp->name().خالی() ? (fromp->name() + "_") : "ANON_")
               + cvt_مقصد_Str(m_id);
    }

    Ast_گره* getProcStmts() {
        Ast_گره* stmtsp = اشاره_تهی;
        إذا (!m_procp) أرجع اشاره_تهی;
        إذا (AstBegin* ثابت beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } وإلا إذا (Ast_گره_FTask* ثابت taskp = VN_CAST(m_procp, گره_FTask)) {
            stmtsp = taskp->stmtsp();
        } وإلا {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        أرجع stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

صنف DynScopeVisitor final : عام VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser2_درون_Use m_inuser2;

    // STATE
    منطقي m_درون_Func = خطأ;  // True if in a function
    Ast_گره_Module* m_modp = اشاره_تهی;  // Module we are currently under
    Ast_گره* m_procp = اشاره_تهی;  // Function/task/block we are currently under
    std::deque<Ast_گره*> m_قاب_ترتیب;  // Ordered list of frames (for determinism)
    std::map<Ast_گره*, Fork_Dyn_Scope_قاب*> m_قابها;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_fork_عمق = 0;  // Number of asynchronous forks we are currently under
    منطقي m_afterTimingControl = خطأ;  // A timing control might've be executed in the current
                                        // process
    اندازه_t m_id = 0;  // Unique ID for a frame
    اندازه_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    Fork_Dyn_Scope_قاب* قاب_از(Ast_گره* nodep) {
        خودکار قاب_It = m_قابها.پیدا_کن(nodep);
        إذا (قاب_It == m_قابها.انتها()) أرجع اشاره_تهی;
        أرجع قاب_It->دومی;
    }

    ثابت Fork_Dyn_Scope_قاب* قاب_از(Ast_گره* nodep) ثابت {
        خودکار قاب_It = m_قابها.پیدا_کن(nodep);
        إذا (قاب_It == m_قابها.انتها()) أرجع اشاره_تهی;
        أرجع قاب_It->دومی;
    }

    Fork_Dyn_Scope_قاب* افزودن_Dyn_Scope_قاب(Ast_گره* procp) {
        Fork_Dyn_Scope_قاب* ثابت framep
            = جديد Fork_Dyn_Scope_قاب{m_modp, procp, m_class_id++, m_id++};
        خودکار r = m_قابها.emplace(procp, framep);
        UASSERT_OBJ(r.دومی, m_modp, "Procedure already contains a frame");
        m_قاب_ترتیب.افزودن_پشت(procp);
        أرجع framep;
    }

    فراغ replaceWithMemberSel(AstVarRef* refp, ثابت ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlink_Fr_پشت(&handle);
        AstMemberSel* ثابت membersel = جديد AstMemberSel{
            refp->fileline(), جديد AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        إذا (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.پیدا_کن_Member(dynScope.m_classp, refp->varp()->name()), Var));
        } وإلا {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
    }

    ساكن منطقي hasAsyncFork(Ast_گره* nodep) {
        أرجع nodep->exists([](AstFork* forkp) { أرجع !forkp->joinType().join(); })
               || nodep->exists([](Ast_تخصیص_بده_Dly*) { أرجع درست; });
    }

    فراغ bind_گره_مقصد_Dyn_Scope(Ast_گره* nodep, Fork_Dyn_Scope_قاب* framep) {
        خودکار r = m_قابها.emplace(nodep, framep);
        إذا (r.دومی) m_قاب_ترتیب.افزودن_پشت(nodep);
    }
    فراغ bindInitIterate(Ast_گره* stmtsp, Fork_Dyn_Scope_قاب* framep) {
        لكل (Ast_گره* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_تخصیص_بده* ثابت asgnp = VN_CAST(stmtp, تخصیص_بده)) {
                bind_گره_مقصد_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } وإلا إذا (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } وإلا إذا (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } وإلا {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    منطقي needsDynScope(ثابت AstVarRef* refp) ثابت {
        ثابت AstVar* ثابت varp = refp->varp();
        أرجع
            // Can this variable escape the scope
            ((m_fork_عمق > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_مقدار() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        إذا (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_درون_Func);
        m_درون_Func = VN_IS(nodep, Func);
        إذا (hasAsyncFork(nodep)) افزودن_Dyn_Scope_قاب(m_procp);
        iterateChildren(nodep);
    }
    فراغ بازدید(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        إذا (hasAsyncFork(nodep)) افزودن_Dyn_Scope_قاب(m_procp);
        iterateChildren(nodep);
    }
    فراغ بازدید(AstFork* nodep) override {
        VL_RESTORER(m_fork_عمق);
        إذا (!nodep->joinType().join()) ++m_fork_عمق;

        ثابت منطقي oldAfterTimingControl = m_afterTimingControl;

        Fork_Dyn_Scope_قاب* framep = اشاره_تهی;
        إذا (nodep->declsp() || nodep->stmtsp()) framep = افزودن_Dyn_Scope_قاب(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        لكل (Ast_گره* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ثابت varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            إذا (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                إذا (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_درج(varp);
                bind_گره_مقصد_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        لكل (Ast_گره* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = خطأ;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        إذا (nodep->isTimingControl()) m_afterTimingControl = درست;
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        بازدید(static_cast<Ast_گره_عبارت*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = درست;
    }
    فراغ بازدید(AstVar* nodep) override {
        nodep->user1(m_fork_عمق);
        Fork_Dyn_Scope_قاب* ثابت framep = قاب_از(m_procp);
        إذا (!framep) أرجع;  // Cannot be legally referenced from a fork
        bind_گره_مقصد_Dyn_Scope(nodep, framep);
    }
    فراغ بازدید(AstVarRef* nodep) override {
        Fork_Dyn_Scope_قاب* ثابت framep = قاب_از(nodep->varp());
        إذا (!framep) أرجع;
        إذا (needsDynScope(nodep)) {
            منطقي isEvent = خطأ;
            إذا (AstBasicDType* ثابت dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.مجموعه_Assigns_Events();
                isEvent = dtypep->isEvent();
            }
            إذا (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_درون_Func ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            إذا (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_درج(nodep->varp());
        }
        bind_گره_مقصد_Dyn_Scope(nodep, framep);
    }
    فراغ بازدید(Ast_تخصیص_بده* nodep) override {
        إذا (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_مقدار()) {
            nodep->lhsp()->user2(درست);
        }
        بازدید(static_cast<Ast_گره_Stmt*>(nodep));
    }
    فراغ بازدید(Ast_تخصیص_بده_Dly* nodep) override {
        إذا (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    أرجع refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(درست);
            // Put it in a fork to prevent lifetime issues with the local
            File_سطر* ثابت flp = nodep->fileline();
            AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->جمع_کن_Forksp(جديد AstBegin{flp, "", nodep, خطأ});
            UINFO(9, "assign new fork " << forkp);
        } وإلا {
            بازدید(static_cast<Ast_گره_Stmt*>(nodep));
        }
    }
    فراغ بازدید(Ast_گره* nodep) override {
        إذا (nodep->isTimingControl()) m_afterTimingControl = درست;
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    صریح DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        بازدید(nodep);

        // Commit changes to AST
        منطقي typesAdded = خطأ;
        لكل (خودکار orderp : m_قاب_ترتیب) {
            UINFO(9, "Frame commit " << orderp);
            خودکار قاب_It = m_قابها.پیدا_کن(orderp);
            UASSERT_OBJ(قاب_It != m_قابها.انتها(), orderp, "m_frames didn't contain m_frameOrder");
            Fork_Dyn_Scope_قاب* framep = قاب_It->دومی;
            إذا (!framep->instance().initialized()) استمر;
            إذا (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = درست;
            }
            إذا (AstVarRef* ثابت refp = VN_CAST(قاب_It->اولی, VarRef)) {
                إذا (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        إذا (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::مجموعه<Fork_Dyn_Scope_قاب*> قابها;
        لكل (خودکار گره_قاب : m_قابها) قابها.درج(گره_قاب.دومی);
        لكل (خودکار* framep : قابها) حذف framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

صنف ForkVisitor final : عام VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ثابت VNUser2_درون_Use m_inuser2;

    // STATE - for current AstNodeModule
    Ast_گره_Module* m_modp = اشاره_تهی;  // Class/module we are currently under
    AstTask* m_tasksp = اشاره_تهی;  // Tasks exrtracted under current module
    اندازه_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    منطقي m_درون_Fork = خطأ;  // Traversal in an async fork
    منطقي m_hasNestedFork = خطأ;  // Branch being taskified contains a nested fork
    منطقي m_درون_Init_Stmt = خطأ;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_گره_FTask* m_ftaskp = اشاره_تهی;  // Enclosing function/task
    std::مجموعه<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = اشاره_تهی;  // Local copies of captured variables
    AstArg* m_capturedArgsp = اشاره_تهی;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = اشاره_تهی;
        لكل (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (varp->name() == refp->name()) اكسر;
        }
        إذا (varp) أرجع varp;

        // Create a local copy to capture
        File_سطر* ثابت flp = refp->fileline();
        varp = جديد AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::درون_داد);
        varp->funcLocal(درست);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_گره::جمع_کن_بعدی(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ثابت argp = جديد AstArg{flp, refp->name(), refp->clone_درخت(خطأ)};
        m_capturedArgsp = Ast_گره::جمع_کن_بعدی(m_capturedArgsp, argp);
        أرجع varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    منطقي taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = خطأ;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = اشاره_تهی;
        m_capturedArgsp = اشاره_تهی;
        iterate(beginp);

        // No need to do it if no variabels are captured
        إذا (m_forkLocalsp.خالی() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) أرجع خطأ;

        // Create task holding the statement and repalce statement with call to that task
        File_سطر* ثابت flp = beginp->fileline();
        ثابت std::string name = "__VforkTask_" + std::مقصد_string(m_nForkTasks++);
        AstTask* ثابت taskp = جديد AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        إذا (m_hasNestedFork) {
            taskp->class_روش(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_گره::جمع_کن_بعدی(m_tasksp, taskp);
        إذا (beginp->declsp()) taskp->جمع_کن_Stmtsp(beginp->declsp()->unlink_Fr_پشت_With_بعدی());
        إذا (beginp->stmtsp()) taskp->جمع_کن_Stmtsp(beginp->stmtsp()->unlink_Fr_پشت_With_بعدی());
        AstTaskRef* ثابت callp = جديد AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->جمع_کن_Stmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        لكل (AstVar* ثابت localp : m_forkLocalsp) localp->funcLocal(درست);

        // We did wrap the body
        أرجع درست;
    }
    ساكن منطقي isForkJoinNoneSentinelDelay(ثابت Ast_گره* ثابت nodep) {
        ثابت AstDelay* ثابت delayp = VN_CAST(nodep, Delay);
        إذا (!delayp) أرجع خطأ;
        ثابت AstConst* ثابت constp = VN_CAST(delayp->lhsp(), Const);
        أرجع constp && (constp->مقصد_UQuad() == std::numeric_limits<uint64_t>::max());
    }
    ساكن منطقي is_Disable_صف_افزودن_Self_پیشوند(Ast_گره* nodep) {
        طالما (Ast_Jump_بلوک* ثابت jumpBlockp = VN_CAST(nodep, Jump_بلوک)) {
            nodep = jumpBlockp->stmtsp();
        }
        أرجع nodep && nodep->is_Disable_صف_افزودن_Self_Stmt();
    }
    قالب <نام_نوع T_Owner>
    ساكن منطقي درج_Fork_Sentinel_After_Disable_صف_Pushes(T_Owner* ثابت ownerp,
                                                          Ast_گره* ثابت اولی_Stmtp,
                                                          Ast_گره* ثابت delayp) {
        Ast_گره* درج_Beforep = اولی_Stmtp;
        طالما (درج_Beforep && درج_Beforep->is_Disable_صف_افزودن_Self_Stmt()) {
            درج_Beforep = درج_Beforep->nextp();
        }
        إذا (Ast_Jump_بلوک* ثابت jumpBlockp = VN_CAST(درج_Beforep, Jump_بلوک)) {
            إذا (درج_Fork_Sentinel_After_Disable_صف_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                أرجع درست;
            }
        }
        إذا (درج_Beforep == اولی_Stmtp) أرجع خطأ;
        إذا (درج_Beforep) {
            درج_Beforep->جمع_کن_اینجا_This_As_بعدی(delayp);
        } وإلا {
            ownerp->جمع_کن_Stmtsp(delayp);
        }
        أرجع درست;
    }
    ساكن فراغ move_Fork_Sentinel_After_Disable_صف_Pushes(AstBegin* ثابت beginp) {
        Ast_گره* ثابت اولی_Stmtp = beginp->stmtsp();
        إذا (!isForkJoinNoneSentinelDelay(اولی_Stmtp)) أرجع;
        Ast_گره* ثابت afterSentinelp = اولی_Stmtp->nextp();
        إذا (!is_Disable_صف_افزودن_Self_پیشوند(afterSentinelp)) أرجع;

        Ast_گره* ثابت delayp = اولی_Stmtp->unlink_Fr_پشت();
        ثابت منطقي moved
            = درج_Fork_Sentinel_After_Disable_صف_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ساكن منطقي forkIsDisableable(AstFork* ثابت nodep) {
        لكل (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            إذا (is_Disable_صف_افزودن_Self_پیشوند(itemp->stmtsp())) أرجع درست;
        }
        أرجع خطأ;
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = اشاره_تهی;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        إذا (m_tasksp) nodep->جمع_کن_Stmtsp(m_tasksp);
    }

    فراغ بازدید(AstFork* nodep) override {
        m_hasNestedFork = درست;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        إذا ((nodep->joinType().joinNone() && !nodep->فوری_شروع())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_سطر* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            لكل (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                إذا (!itemp->stmtsp()) استمر;
                AstDelay* ثابت delayp = جديد AstDelay{
                    fl,
                    جديد AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    خطأ};
                itemp->prependStmtsp(delayp);
                move_Fork_Sentinel_After_Disable_صف_Pushes(itemp);
            }
        }

        iterate_And_بعدی_Null(nodep->declsp());
        iterate_And_بعدی_Null(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        إذا (nodep->joinType().join()) {
            iterate_And_بعدی_Null(nodep->forksp());
            أرجع;
        }

        std::وکتور<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_درون_Fork);
            m_درون_Fork = درست;
            لكل (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                إذا (taskify(itemp)) wrappedp.افزودن_پشت(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        لكل (AstBegin* ثابت beginp : wrappedp) iterate_And_بعدی_Null(beginp);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstVar* nodep) override {
        إذا (m_درون_Fork) m_forkLocalsp.درج(nodep);
    }
    فراغ بازدید(AstVarRef* nodep) override {
        إذا (!m_درون_Fork) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Not sure why this is OK ...
        إذا (!varp->isFuncLocal() && varp->isClassMember()) أرجع;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        إذا (varp->lifetime().isStatic()) أرجع;
        // If this ref is to a variable that will move into the task, then nothing to do
        إذا (m_forkLocalsp.شمار(varp)) أرجع;

        إذا (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_مقدار() || nodep->user2())
            && !m_درون_Init_Stmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warn_بیشتر()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            أرجع;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    فراغ بازدید(Ast_تخصیص_بده* nodep) override {
        إذا (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_مقدار()) {
            nodep->lhsp()->user2(درست);
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_درون_Init_Stmt);
        m_درون_Init_Stmt = درست;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_درون_Init_Stmt);
        m_درون_Init_Stmt = درست;
        iterateChildren(nodep);
    }

    فراغ بازدید(AstThisRef* nodep) override {}
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح ForkVisitor(AstNetlist* nodep) { بازدید(nodep); }
    ~ForkVisitor() override = افتراضي;
};

//######################################################################
// Fork class functions

فراغ V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("fork_dynscope", 0, dump_درخت_Either_Level() >= 3);
}

فراغ V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("fork_tasks", 0, dump_درخت_Either_Level() >= 3);
}
