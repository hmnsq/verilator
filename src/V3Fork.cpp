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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Fork.h"

#شامل "V3AstNodeExpr.h"
#شامل "V3MemberMap.h"

#شامل <limits>
#شامل <set>

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت ForkDynScopeInstance final {
عوامی:
    AstClass* m_classp = خالی_اشاریہ;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = خالی_اشاریہ;  // RefDType for the above
    AstVar* m_handlep = خالی_اشاریہ;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    منطقی initialized() ثابت { واپس m_classp != خالی_اشاریہ; }
};

جماعت Fork_Dyn_Scope_چوکھٹا final {
    // MEMBERS
    Ast_گرہ_Module* ثابت m_modp;  // Module to insert the scope into
    Ast_گرہ* ثابت m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_capture_ترتیب;  // Variables to be moved into the dynscope
    std::سیٹ<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ثابت سائز_t m_class_id;  // Dynscope class ID
    ثابت سائز_t m_id;  // Dynscope ID

عوامی:
    Fork_Dyn_Scope_چوکھٹا(Ast_گرہ_Module* modp, Ast_گرہ* procp, سائز_t class_id, سائز_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = نیا AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = نیا AstClassRefDType{m_procp->fileline(), m_instance.m_classp, خالی_اشاریہ};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = نیا AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(جھوٹ);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        واپس m_instance;
    }

    ثابت ForkDynScopeInstance& instance() ثابت { واپس m_instance; }
    خالی capture_Var_داخل_کرو(AstVar* varp) {
        خودکار r = m_captures.emplace(varp);
        اگر (r.دوسرا) m_capture_ترتیب.ڈالو_عقب(varp);
    }
    منطقی captured(AstVar* varp) { واپس m_captures.گنتی(varp) != 0; }
    Ast_گرہ* procp() ثابت { واپس m_procp; }

    خالی populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        برائے (AstVar* varp : m_capture_ترتیب) {
            اگر (varp->direction().isAny()) {
                varp = varp->clone_درخت(جھوٹ);
                varp->direction(VDirection::NONE);
            } ورنہ {
                varp->unlink_Fr_عقب();
            }
            varp->funcLocal(جھوٹ);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->مستعمل_Loop_Idx(جھوٹ);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->جوڑو_Stmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ثابت newp
            = نیا AstFunc{m_instance.m_classp->fileline(), "new", خالی_اشاریہ, خالی_اشاریہ};
        newp->isConstructor(سچ);
        newp->class_طریقہ(سچ);
        newp->dtypep(newp->ڈھونڈو_Void_DType());
        m_instance.m_classp->جوڑو_Stmtsp(newp);
    }

    خالی linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        اگر (AstFork* ثابت forkp = VN_CAST(m_procp, Fork)) {
            link_Nodes_کا_Fork(memberMap, forkp);
            واپس;
        }

        Ast_گرہ* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_عقب_With_اگلا(&stmtpHandle);

        // Find node after last variable declaration
        Ast_گرہ* initp = stmtp;
        جب (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ثابت newp = نیا AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.ڈھونڈو_Member(m_instance.m_classp, "new"), گرہ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_گرہ* ثابت asgnp = نیا Ast_تفویض{
            m_procp->fileline(),
            نیا AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_گرہ* initsp = خالی_اشاریہ;  // Arguments need to be copied
        برائے (AstVar* varp : m_capture_ترتیب) {
            اگر (!varp->direction().isAny()) جاری;

            اگر (varp->direction().isNonOutput()) {
                AstMemberSel* ثابت memberselp = نیا AstMemberSel{
                    varp->fileline(),
                    نیا AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.ڈھونڈو_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_گرہ* initAsgnp
                    = نیا Ast_تفویض{varp->fileline(), memberselp,
                                    نیا AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_گرہ::جوڑو_اگلا(initsp, initAsgnp);
            }

            اگر (AstBasicDType* ثابت dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.سیٹ_Assigns_Events();
                اگر (dtypep->isEvent()) جاری;
            }

            اگر (varp->direction().isWritable()) {
                AstMemberSel* ثابت memberselp = نیا AstMemberSel{
                    varp->fileline(),
                    نیا AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.ڈھونڈو_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_گرہ* writebackAsgnp = نیا Ast_تفویض{
                    varp->fileline(), نیا AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_گرہ::جوڑو_اگلا(stmtp, writebackAsgnp);
            }
        }
        اگر (initsp) Ast_گرہ::جوڑو_اگلا(asgnp, initsp);

        اگر (initp != stmtp) {
            initp->جوڑو_یہاں_This_As_اگلا(asgnp);
        } ورنہ {
            Ast_گرہ::جوڑو_اگلا(asgnp, static_cast<Ast_گرہ*>(initp));
            stmtp = asgnp;
        }

        Ast_گرہ::جوڑو_اگلا(static_cast<Ast_گرہ*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->جوڑو_Stmtsp(m_instance.m_classp);
    }

    منطقی linked() ثابت { واپس m_instance.initialized() && m_instance.m_handlep->backp(); }

نجی:
    Ast_تفویض* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ثابت newp = نیا AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.ڈھونڈو_Member(m_instance.m_classp, "new"), گرہ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        واپس نیا Ast_تفویض{
            m_procp->fileline(),
            نیا AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    خالی link_Nodes_کا_Fork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ثابت std::string name = "_Vwrapped_"  //
                                 + (forkp->name().خالی_خانے() ? "" : forkp->name() + "_")  //
                                 + std::مقصد_string(m_id);
        AstBegin* ثابت beginp = نیا AstBegin{forkp->fileline(), name, m_instance.m_handlep, سچ};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->جوڑو_Stmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        اگر (forkp->stmtsp()) beginp->جوڑو_Stmtsp(forkp->stmtsp()->unlink_Fr_عقب_With_اگلا());
        // Put the Fork back at the end of the Begin
        beginp->جوڑو_Stmtsp(forkp);
        m_modp->جوڑو_Stmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { واپس "__VDynScope_" + cvt_مقصد_Str(m_class_id); }

    string generateDynScopeHandleName(ثابت Ast_گرہ* fromp) {
        واپس "__VDynScope_" + (!fromp->name().خالی_خانے() ? (fromp->name() + "_") : "ANON_")
               + cvt_مقصد_Str(m_id);
    }

    Ast_گرہ* getProcStmts() {
        Ast_گرہ* stmtsp = خالی_اشاریہ;
        اگر (!m_procp) واپس خالی_اشاریہ;
        اگر (AstBegin* ثابت beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } ورنہ اگر (Ast_گرہ_FTask* ثابت taskp = VN_CAST(m_procp, گرہ_FTask)) {
            stmtsp = taskp->stmtsp();
        } ورنہ {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        واپس stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

جماعت DynScopeVisitor final : عوامی VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;

    // STATE
    منطقی m_اندر_Func = جھوٹ;  // True if in a function
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Module we are currently under
    Ast_گرہ* m_procp = خالی_اشاریہ;  // Function/task/block we are currently under
    std::deque<Ast_گرہ*> m_چوکھٹا_ترتیب;  // Ordered list of frames (for determinism)
    std::map<Ast_گرہ*, Fork_Dyn_Scope_چوکھٹا*> m_چوکھٹے;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_fork_گہرائی = 0;  // Number of asynchronous forks we are currently under
    منطقی m_afterTimingControl = جھوٹ;  // A timing control might've be executed in the current
                                        // process
    سائز_t m_id = 0;  // Unique ID for a frame
    سائز_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    Fork_Dyn_Scope_چوکھٹا* چوکھٹا_کا(Ast_گرہ* nodep) {
        خودکار چوکھٹا_It = m_چوکھٹے.ڈھونڈو(nodep);
        اگر (چوکھٹا_It == m_چوکھٹے.انتہا()) واپس خالی_اشاریہ;
        واپس چوکھٹا_It->دوسرا;
    }

    ثابت Fork_Dyn_Scope_چوکھٹا* چوکھٹا_کا(Ast_گرہ* nodep) ثابت {
        خودکار چوکھٹا_It = m_چوکھٹے.ڈھونڈو(nodep);
        اگر (چوکھٹا_It == m_چوکھٹے.انتہا()) واپس خالی_اشاریہ;
        واپس چوکھٹا_It->دوسرا;
    }

    Fork_Dyn_Scope_چوکھٹا* ڈالو_Dyn_Scope_چوکھٹا(Ast_گرہ* procp) {
        Fork_Dyn_Scope_چوکھٹا* ثابت framep
            = نیا Fork_Dyn_Scope_چوکھٹا{m_modp, procp, m_class_id++, m_id++};
        خودکار r = m_چوکھٹے.emplace(procp, framep);
        UASSERT_OBJ(r.دوسرا, m_modp, "Procedure already contains a frame");
        m_چوکھٹا_ترتیب.ڈالو_عقب(procp);
        واپس framep;
    }

    خالی replaceWithMemberSel(AstVarRef* refp, ثابت ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlink_Fr_عقب(&handle);
        AstMemberSel* ثابت membersel = نیا AstMemberSel{
            refp->fileline(), نیا AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        اگر (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.ڈھونڈو_Member(dynScope.m_classp, refp->varp()->name()), Var));
        } ورنہ {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
    }

    ساکن منطقی hasAsyncFork(Ast_گرہ* nodep) {
        واپس nodep->exists([](AstFork* forkp) { واپس !forkp->joinType().join(); })
               || nodep->exists([](Ast_تفویض_Dly*) { واپس سچ; });
    }

    خالی bind_گرہ_مقصد_Dyn_Scope(Ast_گرہ* nodep, Fork_Dyn_Scope_چوکھٹا* framep) {
        خودکار r = m_چوکھٹے.emplace(nodep, framep);
        اگر (r.دوسرا) m_چوکھٹا_ترتیب.ڈالو_عقب(nodep);
    }
    خالی bindInitIterate(Ast_گرہ* stmtsp, Fork_Dyn_Scope_چوکھٹا* framep) {
        برائے (Ast_گرہ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            اگر (Ast_تفویض* ثابت asgnp = VN_CAST(stmtp, تفویض)) {
                bind_گرہ_مقصد_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } ورنہ اگر (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } ورنہ اگر (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } ورنہ {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    منطقی needsDynScope(ثابت AstVarRef* refp) ثابت {
        ثابت AstVar* ثابت varp = refp->varp();
        واپس
            // Can this variable escape the scope
            ((m_fork_گہرائی > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_قدر() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        اگر (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_اندر_Func);
        m_اندر_Func = VN_IS(nodep, Func);
        اگر (hasAsyncFork(nodep)) ڈالو_Dyn_Scope_چوکھٹا(m_procp);
        iterateChildren(nodep);
    }
    خالی دورہ(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        اگر (hasAsyncFork(nodep)) ڈالو_Dyn_Scope_چوکھٹا(m_procp);
        iterateChildren(nodep);
    }
    خالی دورہ(AstFork* nodep) override {
        VL_RESTORER(m_fork_گہرائی);
        اگر (!nodep->joinType().join()) ++m_fork_گہرائی;

        ثابت منطقی oldAfterTimingControl = m_afterTimingControl;

        Fork_Dyn_Scope_چوکھٹا* framep = خالی_اشاریہ;
        اگر (nodep->declsp() || nodep->stmtsp()) framep = ڈالو_Dyn_Scope_چوکھٹا(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        برائے (Ast_گرہ* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ثابت varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            اگر (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                اگر (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_داخل_کرو(varp);
                bind_گرہ_مقصد_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        برائے (Ast_گرہ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = جھوٹ;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        اگر (nodep->isTimingControl()) m_afterTimingControl = سچ;
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        دورہ(static_cast<Ast_گرہ_اظہاریہ*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = سچ;
    }
    خالی دورہ(AstVar* nodep) override {
        nodep->user1(m_fork_گہرائی);
        Fork_Dyn_Scope_چوکھٹا* ثابت framep = چوکھٹا_کا(m_procp);
        اگر (!framep) واپس;  // Cannot be legally referenced from a fork
        bind_گرہ_مقصد_Dyn_Scope(nodep, framep);
    }
    خالی دورہ(AstVarRef* nodep) override {
        Fork_Dyn_Scope_چوکھٹا* ثابت framep = چوکھٹا_کا(nodep->varp());
        اگر (!framep) واپس;
        اگر (needsDynScope(nodep)) {
            منطقی isEvent = جھوٹ;
            اگر (AstBasicDType* ثابت dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.سیٹ_Assigns_Events();
                isEvent = dtypep->isEvent();
            }
            اگر (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_اندر_Func ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            اگر (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_داخل_کرو(nodep->varp());
        }
        bind_گرہ_مقصد_Dyn_Scope(nodep, framep);
    }
    خالی دورہ(Ast_تفویض* nodep) override {
        اگر (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_قدر()) {
            nodep->lhsp()->user2(سچ);
        }
        دورہ(static_cast<Ast_گرہ_Stmt*>(nodep));
    }
    خالی دورہ(Ast_تفویض_Dly* nodep) override {
        اگر (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    واپس refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(سچ);
            // Put it in a fork to prevent lifetime issues with the local
            File_سطر* ثابت flp = nodep->fileline();
            AstFork* ثابت forkp = نیا AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->جوڑو_Forksp(نیا AstBegin{flp, "", nodep, جھوٹ});
            UINFO(9, "assign new fork " << forkp);
        } ورنہ {
            دورہ(static_cast<Ast_گرہ_Stmt*>(nodep));
        }
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (nodep->isTimingControl()) m_afterTimingControl = سچ;
        iterateChildren(nodep);
    }

عوامی:
    // CONSTRUCTORS
    صریح DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        دورہ(nodep);

        // Commit changes to AST
        منطقی typesAdded = جھوٹ;
        برائے (خودکار orderp : m_چوکھٹا_ترتیب) {
            UINFO(9, "Frame commit " << orderp);
            خودکار چوکھٹا_It = m_چوکھٹے.ڈھونڈو(orderp);
            UASSERT_OBJ(چوکھٹا_It != m_چوکھٹے.انتہا(), orderp, "m_frames didn't contain m_frameOrder");
            Fork_Dyn_Scope_چوکھٹا* framep = چوکھٹا_It->دوسرا;
            اگر (!framep->instance().initialized()) جاری;
            اگر (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = سچ;
            }
            اگر (AstVarRef* ثابت refp = VN_CAST(چوکھٹا_It->پہلا, VarRef)) {
                اگر (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        اگر (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::سیٹ<Fork_Dyn_Scope_چوکھٹا*> چوکھٹے;
        برائے (خودکار گرہ_چوکھٹا : m_چوکھٹے) چوکھٹے.داخل_کرو(گرہ_چوکھٹا.دوسرا);
        برائے (خودکار* framep : چوکھٹے) حذف framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

جماعت ForkVisitor final : عوامی VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ثابت VNUser2_اندر_Use m_inuser2;

    // STATE - for current AstNodeModule
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Class/module we are currently under
    AstTask* m_tasksp = خالی_اشاریہ;  // Tasks exrtracted under current module
    سائز_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    منطقی m_اندر_Fork = جھوٹ;  // Traversal in an async fork
    منطقی m_hasNestedFork = جھوٹ;  // Branch being taskified contains a nested fork
    منطقی m_اندر_Init_Stmt = جھوٹ;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Enclosing function/task
    std::سیٹ<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = خالی_اشاریہ;  // Local copies of captured variables
    AstArg* m_capturedArgsp = خالی_اشاریہ;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = خالی_اشاریہ;
        برائے (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            اگر (varp->name() == refp->name()) توڑو;
        }
        اگر (varp) واپس varp;

        // Create a local copy to capture
        File_سطر* ثابت flp = refp->fileline();
        varp = نیا AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::داخلہ);
        varp->funcLocal(سچ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_گرہ::جوڑو_اگلا(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ثابت argp = نیا AstArg{flp, refp->name(), refp->clone_درخت(جھوٹ)};
        m_capturedArgsp = Ast_گرہ::جوڑو_اگلا(m_capturedArgsp, argp);
        واپس varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    منطقی taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = جھوٹ;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = خالی_اشاریہ;
        m_capturedArgsp = خالی_اشاریہ;
        iterate(beginp);

        // No need to do it if no variabels are captured
        اگر (m_forkLocalsp.خالی_خانے() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) واپس جھوٹ;

        // Create task holding the statement and repalce statement with call to that task
        File_سطر* ثابت flp = beginp->fileline();
        ثابت std::string name = "__VforkTask_" + std::مقصد_string(m_nForkTasks++);
        AstTask* ثابت taskp = نیا AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        اگر (m_hasNestedFork) {
            taskp->class_طریقہ(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_گرہ::جوڑو_اگلا(m_tasksp, taskp);
        اگر (beginp->declsp()) taskp->جوڑو_Stmtsp(beginp->declsp()->unlink_Fr_عقب_With_اگلا());
        اگر (beginp->stmtsp()) taskp->جوڑو_Stmtsp(beginp->stmtsp()->unlink_Fr_عقب_With_اگلا());
        AstTaskRef* ثابت callp = نیا AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->جوڑو_Stmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        برائے (AstVar* ثابت localp : m_forkLocalsp) localp->funcLocal(سچ);

        // We did wrap the body
        واپس سچ;
    }
    ساکن منطقی isForkJoinNoneSentinelDelay(ثابت Ast_گرہ* ثابت nodep) {
        ثابت AstDelay* ثابت delayp = VN_CAST(nodep, Delay);
        اگر (!delayp) واپس جھوٹ;
        ثابت AstConst* ثابت constp = VN_CAST(delayp->lhsp(), Const);
        واپس constp && (constp->مقصد_UQuad() == std::numeric_limits<uint64_t>::max());
    }
    ساکن منطقی is_Disable_قطار_ڈالو_Self_سابقہ(Ast_گرہ* nodep) {
        جب (Ast_Jump_بلاک* ثابت jumpBlockp = VN_CAST(nodep, Jump_بلاک)) {
            nodep = jumpBlockp->stmtsp();
        }
        واپس nodep && nodep->is_Disable_قطار_ڈالو_Self_Stmt();
    }
    سانچہ <قسم_نام T_Owner>
    ساکن منطقی داخل_کرو_Fork_Sentinel_After_Disable_قطار_Pushes(T_Owner* ثابت ownerp,
                                                          Ast_گرہ* ثابت پہلا_Stmtp,
                                                          Ast_گرہ* ثابت delayp) {
        Ast_گرہ* داخل_کرو_Beforep = پہلا_Stmtp;
        جب (داخل_کرو_Beforep && داخل_کرو_Beforep->is_Disable_قطار_ڈالو_Self_Stmt()) {
            داخل_کرو_Beforep = داخل_کرو_Beforep->nextp();
        }
        اگر (Ast_Jump_بلاک* ثابت jumpBlockp = VN_CAST(داخل_کرو_Beforep, Jump_بلاک)) {
            اگر (داخل_کرو_Fork_Sentinel_After_Disable_قطار_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                واپس سچ;
            }
        }
        اگر (داخل_کرو_Beforep == پہلا_Stmtp) واپس جھوٹ;
        اگر (داخل_کرو_Beforep) {
            داخل_کرو_Beforep->جوڑو_یہاں_This_As_اگلا(delayp);
        } ورنہ {
            ownerp->جوڑو_Stmtsp(delayp);
        }
        واپس سچ;
    }
    ساکن خالی move_Fork_Sentinel_After_Disable_قطار_Pushes(AstBegin* ثابت beginp) {
        Ast_گرہ* ثابت پہلا_Stmtp = beginp->stmtsp();
        اگر (!isForkJoinNoneSentinelDelay(پہلا_Stmtp)) واپس;
        Ast_گرہ* ثابت afterSentinelp = پہلا_Stmtp->nextp();
        اگر (!is_Disable_قطار_ڈالو_Self_سابقہ(afterSentinelp)) واپس;

        Ast_گرہ* ثابت delayp = پہلا_Stmtp->unlink_Fr_عقب();
        ثابت منطقی moved
            = داخل_کرو_Fork_Sentinel_After_Disable_قطار_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ساکن منطقی forkIsDisableable(AstFork* ثابت nodep) {
        برائے (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            اگر (is_Disable_قطار_ڈالو_Self_سابقہ(itemp->stmtsp())) واپس سچ;
        }
        واپس جھوٹ;
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = خالی_اشاریہ;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        اگر (m_tasksp) nodep->جوڑو_Stmtsp(m_tasksp);
    }

    خالی دورہ(AstFork* nodep) override {
        m_hasNestedFork = سچ;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        اگر ((nodep->joinType().joinNone() && !nodep->فوری_آغاز())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_سطر* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            برائے (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                اگر (!itemp->stmtsp()) جاری;
                AstDelay* ثابت delayp = نیا AstDelay{
                    fl,
                    نیا AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    جھوٹ};
                itemp->prependStmtsp(delayp);
                move_Fork_Sentinel_After_Disable_قطار_Pushes(itemp);
            }
        }

        iterate_And_اگلا_Null(nodep->declsp());
        iterate_And_اگلا_Null(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        اگر (nodep->joinType().join()) {
            iterate_And_اگلا_Null(nodep->forksp());
            واپس;
        }

        std::سمتیہ<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_اندر_Fork);
            m_اندر_Fork = سچ;
            برائے (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                اگر (taskify(itemp)) wrappedp.ڈالو_عقب(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        برائے (AstBegin* ثابت beginp : wrappedp) iterate_And_اگلا_Null(beginp);
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstVar* nodep) override {
        اگر (m_اندر_Fork) m_forkLocalsp.داخل_کرو(nodep);
    }
    خالی دورہ(AstVarRef* nodep) override {
        اگر (!m_اندر_Fork) واپس;
        AstVar* ثابت varp = nodep->varp();
        // Not sure why this is OK ...
        اگر (!varp->isFuncLocal() && varp->isClassMember()) واپس;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        اگر (varp->lifetime().isStatic()) واپس;
        // If this ref is to a variable that will move into the task, then nothing to do
        اگر (m_forkLocalsp.گنتی(varp)) واپس;

        اگر (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_قدر() || nodep->user2())
            && !m_اندر_Init_Stmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warn_مزید()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            واپس;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    خالی دورہ(Ast_تفویض* nodep) override {
        اگر (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_قدر()) {
            nodep->lhsp()->user2(سچ);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_اندر_Init_Stmt);
        m_اندر_Init_Stmt = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_اندر_Init_Stmt);
        m_اندر_Init_Stmt = سچ;
        iterateChildren(nodep);
    }

    خالی دورہ(AstThisRef* nodep) override {}
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح ForkVisitor(AstNetlist* nodep) { دورہ(nodep); }
    ~ForkVisitor() override = طےشدہ;
};

//######################################################################
// Fork class functions

خالی V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("fork_dynscope", 0, dump_درخت_Either_Level() >= 3);
}

خالی V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("fork_tasks", 0, dump_درخت_Either_Level() >= 3);
}
