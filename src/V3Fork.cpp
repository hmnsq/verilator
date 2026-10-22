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
    AstClass* m_classp = خالی_سوچک;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = خالی_سوچک;  // RefDType for the above
    AstVar* m_handlep = خالی_سوچک;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    منطقي initialized() ثابت { أرجع m_classp != خالی_سوچک; }
};

صنف Fork_Dyn_Scope_چوکھٹا final {
    // MEMBERS
    Ast_گرہ_Module* ثابت m_modp;  // Module to insert the scope into
    Ast_گرہ* ثابت m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_capture_ترتیب;  // Variables to be moved into the dynscope
    std::سیٹ<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ثابت ناپ_t m_class_id;  // Dynscope class ID
    ثابت ناپ_t m_id;  // Dynscope ID

عام:
    Fork_Dyn_Scope_چوکھٹا(Ast_گرہ_Module* modp, Ast_گرہ* procp, ناپ_t class_id, ناپ_t id)
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
            = جديد AstClassRefDType{m_procp->fileline(), m_instance.m_classp, خالی_سوچک};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = جديد AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(خطأ);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        أرجع m_instance;
    }

    ثابت ForkDynScopeInstance& instance() ثابت { أرجع m_instance; }
    فراغ capture_Var_داخل_کرو(AstVar* varp) {
        خودکار r = m_captures.emplace(varp);
        إذا (r.دوجا) m_capture_ترتیب.ڈالو_پٹھ(varp);
    }
    منطقي captured(AstVar* varp) { أرجع m_captures.گنتی(varp) != 0; }
    Ast_گرہ* procp() ثابت { أرجع m_procp; }

    فراغ populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        لكل (AstVar* varp : m_capture_ترتیب) {
            إذا (varp->direction().isAny()) {
                varp = varp->clone_درخت(خطأ);
                varp->direction(VDirection::NONE);
            } وإلا {
                varp->unlink_Fr_پٹھ();
            }
            varp->funcLocal(خطأ);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->ورتیا_Loop_Idx(خطأ);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->جوڑو_Stmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ثابت newp
            = جديد AstFunc{m_instance.m_classp->fileline(), "new", خالی_سوچک, خالی_سوچک};
        newp->isConstructor(سچ);
        newp->class_طریقہ(سچ);
        newp->dtypep(newp->لبھو_Void_DType());
        m_instance.m_classp->جوڑو_Stmtsp(newp);
    }

    فراغ linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        إذا (AstFork* ثابت forkp = VN_CAST(m_procp, Fork)) {
            link_Nodes_دا_Fork(memberMap, forkp);
            أرجع;
        }

        Ast_گرہ* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_پٹھ_With_اگلا(&stmtpHandle);

        // Find node after last variable declaration
        Ast_گرہ* initp = stmtp;
        طالما (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ثابت newp = جديد AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.لبھو_Member(m_instance.m_classp, "new"), گرہ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_گرہ* ثابت asgnp = جديد Ast_سونپو{
            m_procp->fileline(),
            جديد AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_گرہ* initsp = خالی_سوچک;  // Arguments need to be copied
        لكل (AstVar* varp : m_capture_ترتیب) {
            إذا (!varp->direction().isAny()) استمر;

            إذا (varp->direction().isNonOutput()) {
                AstMemberSel* ثابت memberselp = جديد AstMemberSel{
                    varp->fileline(),
                    جديد AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.لبھو_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_گرہ* initAsgnp
                    = جديد Ast_سونپو{varp->fileline(), memberselp,
                                    جديد AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_گرہ::جوڑو_اگلا(initsp, initAsgnp);
            }

            إذا (AstBasicDType* ثابت dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.سیٹ_Assigns_Events();
                إذا (dtypep->isEvent()) استمر;
            }

            إذا (varp->direction().isWritable()) {
                AstMemberSel* ثابت memberselp = جديد AstMemberSel{
                    varp->fileline(),
                    جديد AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.لبھو_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_گرہ* writebackAsgnp = جديد Ast_سونپو{
                    varp->fileline(), جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_گرہ::جوڑو_اگلا(stmtp, writebackAsgnp);
            }
        }
        إذا (initsp) Ast_گرہ::جوڑو_اگلا(asgnp, initsp);

        إذا (initp != stmtp) {
            initp->جوڑو_ایتھے_This_As_اگلا(asgnp);
        } وإلا {
            Ast_گرہ::جوڑو_اگلا(asgnp, static_cast<Ast_گرہ*>(initp));
            stmtp = asgnp;
        }

        Ast_گرہ::جوڑو_اگلا(static_cast<Ast_گرہ*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->جوڑو_Stmtsp(m_instance.m_classp);
    }

    منطقي linked() ثابت { أرجع m_instance.initialized() && m_instance.m_handlep->backp(); }

خاص:
    Ast_سونپو* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ثابت newp = جديد AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.لبھو_Member(m_instance.m_classp, "new"), گرہ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        أرجع جديد Ast_سونپو{
            m_procp->fileline(),
            جديد AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    فراغ link_Nodes_دا_Fork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ثابت std::string name = "_Vwrapped_"  //
                                 + (forkp->name().خالی_تھاواں() ? "" : forkp->name() + "_")  //
                                 + std::منزل_string(m_id);
        AstBegin* ثابت beginp = جديد AstBegin{forkp->fileline(), name, m_instance.m_handlep, سچ};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->جوڑو_Stmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        إذا (forkp->stmtsp()) beginp->جوڑو_Stmtsp(forkp->stmtsp()->unlink_Fr_پٹھ_With_اگلا());
        // Put the Fork back at the end of the Begin
        beginp->جوڑو_Stmtsp(forkp);
        m_modp->جوڑو_Stmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { أرجع "__VDynScope_" + cvt_منزل_Str(m_class_id); }

    string generateDynScopeHandleName(ثابت Ast_گرہ* fromp) {
        أرجع "__VDynScope_" + (!fromp->name().خالی_تھاواں() ? (fromp->name() + "_") : "ANON_")
               + cvt_منزل_Str(m_id);
    }

    Ast_گرہ* getProcStmts() {
        Ast_گرہ* stmtsp = خالی_سوچک;
        إذا (!m_procp) أرجع خالی_سوچک;
        إذا (AstBegin* ثابت beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } وإلا إذا (Ast_گرہ_FTask* ثابت taskp = VN_CAST(m_procp, گرہ_FTask)) {
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
    ثابت VNUser1_وچ_Use m_inuser1;
    ثابت VNUser2_وچ_Use m_inuser2;

    // STATE
    منطقي m_وچ_Func = خطأ;  // True if in a function
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Module we are currently under
    Ast_گرہ* m_procp = خالی_سوچک;  // Function/task/block we are currently under
    std::deque<Ast_گرہ*> m_چوکھٹا_ترتیب;  // Ordered list of frames (for determinism)
    std::map<Ast_گرہ*, Fork_Dyn_Scope_چوکھٹا*> m_چوکھٹے;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_fork_ڈونگھائی = 0;  // Number of asynchronous forks we are currently under
    منطقي m_afterTimingControl = خطأ;  // A timing control might've be executed in the current
                                        // process
    ناپ_t m_id = 0;  // Unique ID for a frame
    ناپ_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    Fork_Dyn_Scope_چوکھٹا* چوکھٹا_دا(Ast_گرہ* nodep) {
        خودکار چوکھٹا_It = m_چوکھٹے.لبھو(nodep);
        إذا (چوکھٹا_It == m_چوکھٹے.سرا()) أرجع خالی_سوچک;
        أرجع چوکھٹا_It->دوجا;
    }

    ثابت Fork_Dyn_Scope_چوکھٹا* چوکھٹا_دا(Ast_گرہ* nodep) ثابت {
        خودکار چوکھٹا_It = m_چوکھٹے.لبھو(nodep);
        إذا (چوکھٹا_It == m_چوکھٹے.سرا()) أرجع خالی_سوچک;
        أرجع چوکھٹا_It->دوجا;
    }

    Fork_Dyn_Scope_چوکھٹا* ڈالو_Dyn_Scope_چوکھٹا(Ast_گرہ* procp) {
        Fork_Dyn_Scope_چوکھٹا* ثابت framep
            = جديد Fork_Dyn_Scope_چوکھٹا{m_modp, procp, m_class_id++, m_id++};
        خودکار r = m_چوکھٹے.emplace(procp, framep);
        UASSERT_OBJ(r.دوجا, m_modp, "Procedure already contains a frame");
        m_چوکھٹا_ترتیب.ڈالو_پٹھ(procp);
        أرجع framep;
    }

    فراغ replaceWithMemberSel(AstVarRef* refp, ثابت ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlink_Fr_پٹھ(&handle);
        AstMemberSel* ثابت membersel = جديد AstMemberSel{
            refp->fileline(), جديد AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        إذا (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.لبھو_Member(dynScope.m_classp, refp->varp()->name()), Var));
        } وإلا {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
    }

    ساكن منطقي hasAsyncFork(Ast_گرہ* nodep) {
        أرجع nodep->exists([](AstFork* forkp) { أرجع !forkp->joinType().join(); })
               || nodep->exists([](Ast_سونپو_Dly*) { أرجع سچ; });
    }

    فراغ bind_گرہ_منزل_Dyn_Scope(Ast_گرہ* nodep, Fork_Dyn_Scope_چوکھٹا* framep) {
        خودکار r = m_چوکھٹے.emplace(nodep, framep);
        إذا (r.دوجا) m_چوکھٹا_ترتیب.ڈالو_پٹھ(nodep);
    }
    فراغ bindInitIterate(Ast_گرہ* stmtsp, Fork_Dyn_Scope_چوکھٹا* framep) {
        لكل (Ast_گرہ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_سونپو* ثابت asgnp = VN_CAST(stmtp, سونپو)) {
                bind_گرہ_منزل_Dyn_Scope(asgnp->lhsp(), framep);
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
            ((m_fork_ڈونگھائی > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_قدر() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        إذا (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_وچ_Func);
        m_وچ_Func = VN_IS(nodep, Func);
        إذا (hasAsyncFork(nodep)) ڈالو_Dyn_Scope_چوکھٹا(m_procp);
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        إذا (hasAsyncFork(nodep)) ڈالو_Dyn_Scope_چوکھٹا(m_procp);
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstFork* nodep) override {
        VL_RESTORER(m_fork_ڈونگھائی);
        إذا (!nodep->joinType().join()) ++m_fork_ڈونگھائی;

        ثابت منطقي oldAfterTimingControl = m_afterTimingControl;

        Fork_Dyn_Scope_چوکھٹا* framep = خالی_سوچک;
        إذا (nodep->declsp() || nodep->stmtsp()) framep = ڈالو_Dyn_Scope_چوکھٹا(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        لكل (Ast_گرہ* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ثابت varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            إذا (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                إذا (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_داخل_کرو(varp);
                bind_گرہ_منزل_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        لكل (Ast_گرہ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = خطأ;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        إذا (nodep->isTimingControl()) m_afterTimingControl = سچ;
    }
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        ویکھو(static_cast<Ast_گرہ_اظہاریہ*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = سچ;
    }
    فراغ ویکھو(AstVar* nodep) override {
        nodep->user1(m_fork_ڈونگھائی);
        Fork_Dyn_Scope_چوکھٹا* ثابت framep = چوکھٹا_دا(m_procp);
        إذا (!framep) أرجع;  // Cannot be legally referenced from a fork
        bind_گرہ_منزل_Dyn_Scope(nodep, framep);
    }
    فراغ ویکھو(AstVarRef* nodep) override {
        Fork_Dyn_Scope_چوکھٹا* ثابت framep = چوکھٹا_دا(nodep->varp());
        إذا (!framep) أرجع;
        إذا (needsDynScope(nodep)) {
            منطقي isEvent = خطأ;
            إذا (AstBasicDType* ثابت dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.سیٹ_Assigns_Events();
                isEvent = dtypep->isEvent();
            }
            إذا (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_وچ_Func ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            إذا (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_داخل_کرو(nodep->varp());
        }
        bind_گرہ_منزل_Dyn_Scope(nodep, framep);
    }
    فراغ ویکھو(Ast_سونپو* nodep) override {
        إذا (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_قدر()) {
            nodep->lhsp()->user2(سچ);
        }
        ویکھو(static_cast<Ast_گرہ_Stmt*>(nodep));
    }
    فراغ ویکھو(Ast_سونپو_Dly* nodep) override {
        إذا (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    أرجع refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(سچ);
            // Put it in a fork to prevent lifetime issues with the local
            File_سطر* ثابت flp = nodep->fileline();
            AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->جوڑو_Forksp(جديد AstBegin{flp, "", nodep, خطأ});
            UINFO(9, "assign new fork " << forkp);
        } وإلا {
            ویکھو(static_cast<Ast_گرہ_Stmt*>(nodep));
        }
    }
    فراغ ویکھو(Ast_گرہ* nodep) override {
        إذا (nodep->isTimingControl()) m_afterTimingControl = سچ;
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        ویکھو(nodep);

        // Commit changes to AST
        منطقي typesAdded = خطأ;
        لكل (خودکار orderp : m_چوکھٹا_ترتیب) {
            UINFO(9, "Frame commit " << orderp);
            خودکار چوکھٹا_It = m_چوکھٹے.لبھو(orderp);
            UASSERT_OBJ(چوکھٹا_It != m_چوکھٹے.سرا(), orderp, "m_frames didn't contain m_frameOrder");
            Fork_Dyn_Scope_چوکھٹا* framep = چوکھٹا_It->دوجا;
            إذا (!framep->instance().initialized()) استمر;
            إذا (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = سچ;
            }
            إذا (AstVarRef* ثابت refp = VN_CAST(چوکھٹا_It->پہلا, VarRef)) {
                إذا (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        إذا (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::سیٹ<Fork_Dyn_Scope_چوکھٹا*> چوکھٹے;
        لكل (خودکار گرہ_چوکھٹا : m_چوکھٹے) چوکھٹے.داخل_کرو(گرہ_چوکھٹا.دوجا);
        لكل (خودکار* framep : چوکھٹے) مٹاؤ framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

صنف ForkVisitor final : عام VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ثابت VNUser2_وچ_Use m_inuser2;

    // STATE - for current AstNodeModule
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Class/module we are currently under
    AstTask* m_tasksp = خالی_سوچک;  // Tasks exrtracted under current module
    ناپ_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    منطقي m_وچ_Fork = خطأ;  // Traversal in an async fork
    منطقي m_hasNestedFork = خطأ;  // Branch being taskified contains a nested fork
    منطقي m_وچ_Init_Stmt = خطأ;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_گرہ_FTask* m_ftaskp = خالی_سوچک;  // Enclosing function/task
    std::سیٹ<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = خالی_سوچک;  // Local copies of captured variables
    AstArg* m_capturedArgsp = خالی_سوچک;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = خالی_سوچک;
        لكل (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (varp->name() == refp->name()) اكسر;
        }
        إذا (varp) أرجع varp;

        // Create a local copy to capture
        File_سطر* ثابت flp = refp->fileline();
        varp = جديد AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::داخلہ);
        varp->funcLocal(سچ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_گرہ::جوڑو_اگلا(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ثابت argp = جديد AstArg{flp, refp->name(), refp->clone_درخت(خطأ)};
        m_capturedArgsp = Ast_گرہ::جوڑو_اگلا(m_capturedArgsp, argp);
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
        m_capturedVarsp = خالی_سوچک;
        m_capturedArgsp = خالی_سوچک;
        iterate(beginp);

        // No need to do it if no variabels are captured
        إذا (m_forkLocalsp.خالی_تھاواں() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) أرجع خطأ;

        // Create task holding the statement and repalce statement with call to that task
        File_سطر* ثابت flp = beginp->fileline();
        ثابت std::string name = "__VforkTask_" + std::منزل_string(m_nForkTasks++);
        AstTask* ثابت taskp = جديد AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        إذا (m_hasNestedFork) {
            taskp->class_طریقہ(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_گرہ::جوڑو_اگلا(m_tasksp, taskp);
        إذا (beginp->declsp()) taskp->جوڑو_Stmtsp(beginp->declsp()->unlink_Fr_پٹھ_With_اگلا());
        إذا (beginp->stmtsp()) taskp->جوڑو_Stmtsp(beginp->stmtsp()->unlink_Fr_پٹھ_With_اگلا());
        AstTaskRef* ثابت callp = جديد AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->جوڑو_Stmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        لكل (AstVar* ثابت localp : m_forkLocalsp) localp->funcLocal(سچ);

        // We did wrap the body
        أرجع سچ;
    }
    ساكن منطقي isForkJoinNoneSentinelDelay(ثابت Ast_گرہ* ثابت nodep) {
        ثابت AstDelay* ثابت delayp = VN_CAST(nodep, Delay);
        إذا (!delayp) أرجع خطأ;
        ثابت AstConst* ثابت constp = VN_CAST(delayp->lhsp(), Const);
        أرجع constp && (constp->منزل_UQuad() == std::numeric_limits<uint64_t>::max());
    }
    ساكن منطقي is_Disable_قطار_ڈالو_Self_اگیتر(Ast_گرہ* nodep) {
        طالما (Ast_Jump_بلاک* ثابت jumpBlockp = VN_CAST(nodep, Jump_بلاک)) {
            nodep = jumpBlockp->stmtsp();
        }
        أرجع nodep && nodep->is_Disable_قطار_ڈالو_Self_Stmt();
    }
    سانچا <قسم_ناں T_Owner>
    ساكن منطقي داخل_کرو_Fork_Sentinel_After_Disable_قطار_Pushes(T_Owner* ثابت ownerp,
                                                          Ast_گرہ* ثابت پہلا_Stmtp,
                                                          Ast_گرہ* ثابت delayp) {
        Ast_گرہ* داخل_کرو_Beforep = پہلا_Stmtp;
        طالما (داخل_کرو_Beforep && داخل_کرو_Beforep->is_Disable_قطار_ڈالو_Self_Stmt()) {
            داخل_کرو_Beforep = داخل_کرو_Beforep->nextp();
        }
        إذا (Ast_Jump_بلاک* ثابت jumpBlockp = VN_CAST(داخل_کرو_Beforep, Jump_بلاک)) {
            إذا (داخل_کرو_Fork_Sentinel_After_Disable_قطار_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                أرجع سچ;
            }
        }
        إذا (داخل_کرو_Beforep == پہلا_Stmtp) أرجع خطأ;
        إذا (داخل_کرو_Beforep) {
            داخل_کرو_Beforep->جوڑو_ایتھے_This_As_اگلا(delayp);
        } وإلا {
            ownerp->جوڑو_Stmtsp(delayp);
        }
        أرجع سچ;
    }
    ساكن فراغ move_Fork_Sentinel_After_Disable_قطار_Pushes(AstBegin* ثابت beginp) {
        Ast_گرہ* ثابت پہلا_Stmtp = beginp->stmtsp();
        إذا (!isForkJoinNoneSentinelDelay(پہلا_Stmtp)) أرجع;
        Ast_گرہ* ثابت afterSentinelp = پہلا_Stmtp->nextp();
        إذا (!is_Disable_قطار_ڈالو_Self_اگیتر(afterSentinelp)) أرجع;

        Ast_گرہ* ثابت delayp = پہلا_Stmtp->unlink_Fr_پٹھ();
        ثابت منطقي moved
            = داخل_کرو_Fork_Sentinel_After_Disable_قطار_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ساكن منطقي forkIsDisableable(AstFork* ثابت nodep) {
        لكل (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            إذا (is_Disable_قطار_ڈالو_Self_اگیتر(itemp->stmtsp())) أرجع سچ;
        }
        أرجع خطأ;
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = خالی_سوچک;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        إذا (m_tasksp) nodep->جوڑو_Stmtsp(m_tasksp);
    }

    فراغ ویکھو(AstFork* nodep) override {
        m_hasNestedFork = سچ;
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
        إذا (nodep->joinType().join()) {
            iterate_And_اگلا_Null(nodep->forksp());
            أرجع;
        }

        std::سمتیہ<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_وچ_Fork);
            m_وچ_Fork = سچ;
            لكل (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                إذا (taskify(itemp)) wrappedp.ڈالو_پٹھ(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        لكل (AstBegin* ثابت beginp : wrappedp) iterate_And_اگلا_Null(beginp);
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstVar* nodep) override {
        إذا (m_وچ_Fork) m_forkLocalsp.داخل_کرو(nodep);
    }
    فراغ ویکھو(AstVarRef* nodep) override {
        إذا (!m_وچ_Fork) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Not sure why this is OK ...
        إذا (!varp->isFuncLocal() && varp->isClassMember()) أرجع;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        إذا (varp->lifetime().isStatic()) أرجع;
        // If this ref is to a variable that will move into the task, then nothing to do
        إذا (m_forkLocalsp.گنتی(varp)) أرجع;

        إذا (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_قدر() || nodep->user2())
            && !m_وچ_Init_Stmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warn_ہور()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            أرجع;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    فراغ ویکھو(Ast_سونپو* nodep) override {
        إذا (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_قدر()) {
            nodep->lhsp()->user2(سچ);
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_وچ_Init_Stmt);
        m_وچ_Init_Stmt = سچ;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_وچ_Init_Stmt);
        m_وچ_Init_Stmt = سچ;
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstThisRef* nodep) override {}
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ForkVisitor(AstNetlist* nodep) { ویکھو(nodep); }
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
