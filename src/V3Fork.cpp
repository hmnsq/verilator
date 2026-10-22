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
    AstClass* m_classp = خالي_اشارو;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = خالي_اشارو;  // RefDType for the above
    AstVar* m_handlep = خالي_اشارو;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    منطقي initialized() ثابت { أرجع m_classp != خالي_اشارو; }
};

صنف Fork_Dyn_Scope_چوکٺو final {
    // MEMBERS
    Ast_ڳنڍ_Module* ثابت m_modp;  // Module to insert the scope into
    Ast_ڳنڍ* ثابت m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_capture_ترتيب;  // Variables to be moved into the dynscope
    std::سيٽ<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ثابت سائيز_t m_class_id;  // Dynscope class ID
    ثابت سائيز_t m_id;  // Dynscope ID

عام:
    Fork_Dyn_Scope_چوکٺو(Ast_ڳنڍ_Module* modp, Ast_ڳنڍ* procp, سائيز_t class_id, سائيز_t id)
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
            = جديد AstClassRefDType{m_procp->fileline(), m_instance.m_classp, خالي_اشارو};
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = جديد AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(خطأ);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        أرجع m_instance;
    }

    ثابت ForkDynScopeInstance& instance() ثابت { أرجع m_instance; }
    فراغ capture_Var_وجهو(AstVar* varp) {
        خودڪار r = m_captures.emplace(varp);
        إذا (r.ٻيو) m_capture_ترتيب.ڌڪو_پٺ(varp);
    }
    منطقي captured(AstVar* varp) { أرجع m_captures.ڳڻپ(varp) != 0; }
    Ast_ڳنڍ* procp() ثابت { أرجع m_procp; }

    فراغ populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        لكل (AstVar* varp : m_capture_ترتيب) {
            إذا (varp->direction().isAny()) {
                varp = varp->clone_وڻ(خطأ);
                varp->direction(VDirection::NONE);
            } وإلا {
                varp->unlink_Fr_پٺ();
            }
            varp->funcLocal(خطأ);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->استعمال_ٿيل_Loop_Idx(خطأ);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->جوڙيو_Stmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ثابت newp
            = جديد AstFunc{m_instance.m_classp->fileline(), "new", خالي_اشارو, خالي_اشارو};
        newp->isConstructor(سچ);
        newp->class_طريقو(سچ);
        newp->dtypep(newp->ڳوليو_Void_DType());
        m_instance.m_classp->جوڙيو_Stmtsp(newp);
    }

    فراغ linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        إذا (AstFork* ثابت forkp = VN_CAST(m_procp, Fork)) {
            link_Nodes_جو_Fork(memberMap, forkp);
            أرجع;
        }

        Ast_ڳنڍ* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_پٺ_With_ايندڙ(&stmtpHandle);

        // Find node after last variable declaration
        Ast_ڳنڍ* initp = stmtp;
        طالما (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ثابت newp = جديد AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.ڳوليو_Member(m_instance.m_classp, "new"), ڳنڍ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_ڳنڍ* ثابت asgnp = جديد Ast_سونپيو{
            m_procp->fileline(),
            جديد AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_ڳنڍ* initsp = خالي_اشارو;  // Arguments need to be copied
        لكل (AstVar* varp : m_capture_ترتيب) {
            إذا (!varp->direction().isAny()) استمر;

            إذا (varp->direction().isNonOutput()) {
                AstMemberSel* ثابت memberselp = جديد AstMemberSel{
                    varp->fileline(),
                    جديد AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.ڳوليو_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_ڳنڍ* initAsgnp
                    = جديد Ast_سونپيو{varp->fileline(), memberselp,
                                    جديد AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_ڳنڍ::جوڙيو_ايندڙ(initsp, initAsgnp);
            }

            إذا (AstBasicDType* ثابت dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.سيٽ_Assigns_Events();
                إذا (dtypep->isEvent()) استمر;
            }

            إذا (varp->direction().isWritable()) {
                AstMemberSel* ثابت memberselp = جديد AstMemberSel{
                    varp->fileline(),
                    جديد AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.ڳوليو_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_ڳنڍ* writebackAsgnp = جديد Ast_سونپيو{
                    varp->fileline(), جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtp, writebackAsgnp);
            }
        }
        إذا (initsp) Ast_ڳنڍ::جوڙيو_ايندڙ(asgnp, initsp);

        إذا (initp != stmtp) {
            initp->جوڙيو_هتي_This_As_ايندڙ(asgnp);
        } وإلا {
            Ast_ڳنڍ::جوڙيو_ايندڙ(asgnp, static_cast<Ast_ڳنڍ*>(initp));
            stmtp = asgnp;
        }

        Ast_ڳنڍ::جوڙيو_ايندڙ(static_cast<Ast_ڳنڍ*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->جوڙيو_Stmtsp(m_instance.m_classp);
    }

    منطقي linked() ثابت { أرجع m_instance.initialized() && m_instance.m_handlep->backp(); }

خاص:
    Ast_سونپيو* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ثابت newp = جديد AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.ڳوليو_Member(m_instance.m_classp, "new"), ڳنڍ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        أرجع جديد Ast_سونپيو{
            m_procp->fileline(),
            جديد AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    فراغ link_Nodes_جو_Fork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ثابت std::string name = "_Vwrapped_"  //
                                 + (forkp->name().خالي_جايون() ? "" : forkp->name() + "_")  //
                                 + std::منزل_string(m_id);
        AstBegin* ثابت beginp = جديد AstBegin{forkp->fileline(), name, m_instance.m_handlep, سچ};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->جوڙيو_Stmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        إذا (forkp->stmtsp()) beginp->جوڙيو_Stmtsp(forkp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
        // Put the Fork back at the end of the Begin
        beginp->جوڙيو_Stmtsp(forkp);
        m_modp->جوڙيو_Stmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { أرجع "__VDynScope_" + cvt_منزل_Str(m_class_id); }

    string generateDynScopeHandleName(ثابت Ast_ڳنڍ* fromp) {
        أرجع "__VDynScope_" + (!fromp->name().خالي_جايون() ? (fromp->name() + "_") : "ANON_")
               + cvt_منزل_Str(m_id);
    }

    Ast_ڳنڍ* getProcStmts() {
        Ast_ڳنڍ* stmtsp = خالي_اشارو;
        إذا (!m_procp) أرجع خالي_اشارو;
        إذا (AstBegin* ثابت beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } وإلا إذا (Ast_ڳنڍ_FTask* ثابت taskp = VN_CAST(m_procp, ڳنڍ_FTask)) {
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
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;

    // STATE
    منطقي m_اندريون_Func = خطأ;  // True if in a function
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Module we are currently under
    Ast_ڳنڍ* m_procp = خالي_اشارو;  // Function/task/block we are currently under
    std::deque<Ast_ڳنڍ*> m_چوکٺو_ترتيب;  // Ordered list of frames (for determinism)
    std::map<Ast_ڳنڍ*, Fork_Dyn_Scope_چوکٺو*> m_چوکٺا;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_fork_اونهائي = 0;  // Number of asynchronous forks we are currently under
    منطقي m_afterTimingControl = خطأ;  // A timing control might've be executed in the current
                                        // process
    سائيز_t m_id = 0;  // Unique ID for a frame
    سائيز_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    Fork_Dyn_Scope_چوکٺو* چوکٺو_جو(Ast_ڳنڍ* nodep) {
        خودڪار چوکٺو_It = m_چوکٺا.ڳوليو(nodep);
        إذا (چوکٺو_It == m_چوکٺا.پڇاڙي()) أرجع خالي_اشارو;
        أرجع چوکٺو_It->ٻيو;
    }

    ثابت Fork_Dyn_Scope_چوکٺو* چوکٺو_جو(Ast_ڳنڍ* nodep) ثابت {
        خودڪار چوکٺو_It = m_چوکٺا.ڳوليو(nodep);
        إذا (چوکٺو_It == m_چوکٺا.پڇاڙي()) أرجع خالي_اشارو;
        أرجع چوکٺو_It->ٻيو;
    }

    Fork_Dyn_Scope_چوکٺو* ڌڪو_Dyn_Scope_چوکٺو(Ast_ڳنڍ* procp) {
        Fork_Dyn_Scope_چوکٺو* ثابت framep
            = جديد Fork_Dyn_Scope_چوکٺو{m_modp, procp, m_class_id++, m_id++};
        خودڪار r = m_چوکٺا.emplace(procp, framep);
        UASSERT_OBJ(r.ٻيو, m_modp, "Procedure already contains a frame");
        m_چوکٺو_ترتيب.ڌڪو_پٺ(procp);
        أرجع framep;
    }

    فراغ replaceWithMemberSel(AstVarRef* refp, ثابت ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlink_Fr_پٺ(&handle);
        AstMemberSel* ثابت membersel = جديد AstMemberSel{
            refp->fileline(), جديد AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        إذا (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.ڳوليو_Member(dynScope.m_classp, refp->varp()->name()), Var));
        } وإلا {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
    }

    ساكن منطقي hasAsyncFork(Ast_ڳنڍ* nodep) {
        أرجع nodep->exists([](AstFork* forkp) { أرجع !forkp->joinType().join(); })
               || nodep->exists([](Ast_سونپيو_Dly*) { أرجع سچ; });
    }

    فراغ bind_ڳنڍ_منزل_Dyn_Scope(Ast_ڳنڍ* nodep, Fork_Dyn_Scope_چوکٺو* framep) {
        خودڪار r = m_چوکٺا.emplace(nodep, framep);
        إذا (r.ٻيو) m_چوکٺو_ترتيب.ڌڪو_پٺ(nodep);
    }
    فراغ bindInitIterate(Ast_ڳنڍ* stmtsp, Fork_Dyn_Scope_چوکٺو* framep) {
        لكل (Ast_ڳنڍ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_سونپيو* ثابت asgnp = VN_CAST(stmtp, سونپيو)) {
                bind_ڳنڍ_منزل_Dyn_Scope(asgnp->lhsp(), framep);
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
            ((m_fork_اونهائي > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_قدر() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        إذا (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_اندريون_Func);
        m_اندريون_Func = VN_IS(nodep, Func);
        إذا (hasAsyncFork(nodep)) ڌڪو_Dyn_Scope_چوکٺو(m_procp);
        iterateChildren(nodep);
    }
    فراغ گهمو(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        إذا (hasAsyncFork(nodep)) ڌڪو_Dyn_Scope_چوکٺو(m_procp);
        iterateChildren(nodep);
    }
    فراغ گهمو(AstFork* nodep) override {
        VL_RESTORER(m_fork_اونهائي);
        إذا (!nodep->joinType().join()) ++m_fork_اونهائي;

        ثابت منطقي oldAfterTimingControl = m_afterTimingControl;

        Fork_Dyn_Scope_چوکٺو* framep = خالي_اشارو;
        إذا (nodep->declsp() || nodep->stmtsp()) framep = ڌڪو_Dyn_Scope_چوکٺو(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        لكل (Ast_ڳنڍ* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ثابت varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            إذا (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                إذا (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_وجهو(varp);
                bind_ڳنڍ_منزل_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        لكل (Ast_ڳنڍ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = خطأ;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        إذا (nodep->isTimingControl()) m_afterTimingControl = سچ;
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        گهمو(static_cast<Ast_ڳنڍ_اظهار*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = سچ;
    }
    فراغ گهمو(AstVar* nodep) override {
        nodep->user1(m_fork_اونهائي);
        Fork_Dyn_Scope_چوکٺو* ثابت framep = چوکٺو_جو(m_procp);
        إذا (!framep) أرجع;  // Cannot be legally referenced from a fork
        bind_ڳنڍ_منزل_Dyn_Scope(nodep, framep);
    }
    فراغ گهمو(AstVarRef* nodep) override {
        Fork_Dyn_Scope_چوکٺو* ثابت framep = چوکٺو_جو(nodep->varp());
        إذا (!framep) أرجع;
        إذا (needsDynScope(nodep)) {
            منطقي isEvent = خطأ;
            إذا (AstBasicDType* ثابت dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.سيٽ_Assigns_Events();
                isEvent = dtypep->isEvent();
            }
            إذا (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_اندريون_Func ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            إذا (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_وجهو(nodep->varp());
        }
        bind_ڳنڍ_منزل_Dyn_Scope(nodep, framep);
    }
    فراغ گهمو(Ast_سونپيو* nodep) override {
        إذا (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_قدر()) {
            nodep->lhsp()->user2(سچ);
        }
        گهمو(static_cast<Ast_ڳنڍ_Stmt*>(nodep));
    }
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override {
        إذا (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    أرجع refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(سچ);
            // Put it in a fork to prevent lifetime issues with the local
            File_سٽ* ثابت flp = nodep->fileline();
            AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->جوڙيو_Forksp(جديد AstBegin{flp, "", nodep, خطأ});
            UINFO(9, "assign new fork " << forkp);
        } وإلا {
            گهمو(static_cast<Ast_ڳنڍ_Stmt*>(nodep));
        }
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (nodep->isTimingControl()) m_afterTimingControl = سچ;
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        گهمو(nodep);

        // Commit changes to AST
        منطقي typesAdded = خطأ;
        لكل (خودڪار orderp : m_چوکٺو_ترتيب) {
            UINFO(9, "Frame commit " << orderp);
            خودڪار چوکٺو_It = m_چوکٺا.ڳوليو(orderp);
            UASSERT_OBJ(چوکٺو_It != m_چوکٺا.پڇاڙي(), orderp, "m_frames didn't contain m_frameOrder");
            Fork_Dyn_Scope_چوکٺو* framep = چوکٺو_It->ٻيو;
            إذا (!framep->instance().initialized()) استمر;
            إذا (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = سچ;
            }
            إذا (AstVarRef* ثابت refp = VN_CAST(چوکٺو_It->پهريون, VarRef)) {
                إذا (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        إذا (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::سيٽ<Fork_Dyn_Scope_چوکٺو*> چوکٺا;
        لكل (خودڪار ڳنڍ_چوکٺو : m_چوکٺا) چوکٺا.وجهو(ڳنڍ_چوکٺو.ٻيو);
        لكل (خودڪار* framep : چوکٺا) ميٽيو framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

صنف ForkVisitor final : عام VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ثابت VNUser2_اندريون_Use m_inuser2;

    // STATE - for current AstNodeModule
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Class/module we are currently under
    AstTask* m_tasksp = خالي_اشارو;  // Tasks exrtracted under current module
    سائيز_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    منطقي m_اندريون_Fork = خطأ;  // Traversal in an async fork
    منطقي m_hasNestedFork = خطأ;  // Branch being taskified contains a nested fork
    منطقي m_اندريون_Init_Stmt = خطأ;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Enclosing function/task
    std::سيٽ<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = خالي_اشارو;  // Local copies of captured variables
    AstArg* m_capturedArgsp = خالي_اشارو;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = خالي_اشارو;
        لكل (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (varp->name() == refp->name()) اكسر;
        }
        إذا (varp) أرجع varp;

        // Create a local copy to capture
        File_سٽ* ثابت flp = refp->fileline();
        varp = جديد AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::داخلا);
        varp->funcLocal(سچ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_ڳنڍ::جوڙيو_ايندڙ(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ثابت argp = جديد AstArg{flp, refp->name(), refp->clone_وڻ(خطأ)};
        m_capturedArgsp = Ast_ڳنڍ::جوڙيو_ايندڙ(m_capturedArgsp, argp);
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
        m_capturedVarsp = خالي_اشارو;
        m_capturedArgsp = خالي_اشارو;
        iterate(beginp);

        // No need to do it if no variabels are captured
        إذا (m_forkLocalsp.خالي_جايون() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) أرجع خطأ;

        // Create task holding the statement and repalce statement with call to that task
        File_سٽ* ثابت flp = beginp->fileline();
        ثابت std::string name = "__VforkTask_" + std::منزل_string(m_nForkTasks++);
        AstTask* ثابت taskp = جديد AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        إذا (m_hasNestedFork) {
            taskp->class_طريقو(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_ڳنڍ::جوڙيو_ايندڙ(m_tasksp, taskp);
        إذا (beginp->declsp()) taskp->جوڙيو_Stmtsp(beginp->declsp()->unlink_Fr_پٺ_With_ايندڙ());
        إذا (beginp->stmtsp()) taskp->جوڙيو_Stmtsp(beginp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
        AstTaskRef* ثابت callp = جديد AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->جوڙيو_Stmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        لكل (AstVar* ثابت localp : m_forkLocalsp) localp->funcLocal(سچ);

        // We did wrap the body
        أرجع سچ;
    }
    ساكن منطقي isForkJoinNoneSentinelDelay(ثابت Ast_ڳنڍ* ثابت nodep) {
        ثابت AstDelay* ثابت delayp = VN_CAST(nodep, Delay);
        إذا (!delayp) أرجع خطأ;
        ثابت AstConst* ثابت constp = VN_CAST(delayp->lhsp(), Const);
        أرجع constp && (constp->منزل_UQuad() == std::numeric_limits<uint64_t>::max());
    }
    ساكن منطقي is_Disable_قطار_ڌڪو_Self_اڳياڙي(Ast_ڳنڍ* nodep) {
        طالما (Ast_Jump_بلاڪ* ثابت jumpBlockp = VN_CAST(nodep, Jump_بلاڪ)) {
            nodep = jumpBlockp->stmtsp();
        }
        أرجع nodep && nodep->is_Disable_قطار_ڌڪو_Self_Stmt();
    }
    سانچو <قسم_نالو T_Owner>
    ساكن منطقي وجهو_Fork_Sentinel_After_Disable_قطار_Pushes(T_Owner* ثابت ownerp,
                                                          Ast_ڳنڍ* ثابت پهريون_Stmtp,
                                                          Ast_ڳنڍ* ثابت delayp) {
        Ast_ڳنڍ* وجهو_Beforep = پهريون_Stmtp;
        طالما (وجهو_Beforep && وجهو_Beforep->is_Disable_قطار_ڌڪو_Self_Stmt()) {
            وجهو_Beforep = وجهو_Beforep->nextp();
        }
        إذا (Ast_Jump_بلاڪ* ثابت jumpBlockp = VN_CAST(وجهو_Beforep, Jump_بلاڪ)) {
            إذا (وجهو_Fork_Sentinel_After_Disable_قطار_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                أرجع سچ;
            }
        }
        إذا (وجهو_Beforep == پهريون_Stmtp) أرجع خطأ;
        إذا (وجهو_Beforep) {
            وجهو_Beforep->جوڙيو_هتي_This_As_ايندڙ(delayp);
        } وإلا {
            ownerp->جوڙيو_Stmtsp(delayp);
        }
        أرجع سچ;
    }
    ساكن فراغ move_Fork_Sentinel_After_Disable_قطار_Pushes(AstBegin* ثابت beginp) {
        Ast_ڳنڍ* ثابت پهريون_Stmtp = beginp->stmtsp();
        إذا (!isForkJoinNoneSentinelDelay(پهريون_Stmtp)) أرجع;
        Ast_ڳنڍ* ثابت afterSentinelp = پهريون_Stmtp->nextp();
        إذا (!is_Disable_قطار_ڌڪو_Self_اڳياڙي(afterSentinelp)) أرجع;

        Ast_ڳنڍ* ثابت delayp = پهريون_Stmtp->unlink_Fr_پٺ();
        ثابت منطقي moved
            = وجهو_Fork_Sentinel_After_Disable_قطار_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ساكن منطقي forkIsDisableable(AstFork* ثابت nodep) {
        لكل (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            إذا (is_Disable_قطار_ڌڪو_Self_اڳياڙي(itemp->stmtsp())) أرجع سچ;
        }
        أرجع خطأ;
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = خالي_اشارو;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        إذا (m_tasksp) nodep->جوڙيو_Stmtsp(m_tasksp);
    }

    فراغ گهمو(AstFork* nodep) override {
        m_hasNestedFork = سچ;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        إذا ((nodep->joinType().joinNone() && !nodep->فوري_شروعات())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_سٽ* fl = nodep->fileline();
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

        iterate_And_ايندڙ_Null(nodep->declsp());
        iterate_And_ايندڙ_Null(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        إذا (nodep->joinType().join()) {
            iterate_And_ايندڙ_Null(nodep->forksp());
            أرجع;
        }

        std::ويڪٽر<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_اندريون_Fork);
            m_اندريون_Fork = سچ;
            لكل (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                إذا (taskify(itemp)) wrappedp.ڌڪو_پٺ(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        لكل (AstBegin* ثابت beginp : wrappedp) iterate_And_ايندڙ_Null(beginp);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstVar* nodep) override {
        إذا (m_اندريون_Fork) m_forkLocalsp.وجهو(nodep);
    }
    فراغ گهمو(AstVarRef* nodep) override {
        إذا (!m_اندريون_Fork) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Not sure why this is OK ...
        إذا (!varp->isFuncLocal() && varp->isClassMember()) أرجع;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        إذا (varp->lifetime().isStatic()) أرجع;
        // If this ref is to a variable that will move into the task, then nothing to do
        إذا (m_forkLocalsp.ڳڻپ(varp)) أرجع;

        إذا (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_قدر() || nodep->user2())
            && !m_اندريون_Init_Stmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warn_وڌيڪ()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            أرجع;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    فراغ گهمو(Ast_سونپيو* nodep) override {
        إذا (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_قدر()) {
            nodep->lhsp()->user2(سچ);
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_اندريون_Init_Stmt);
        m_اندريون_Init_Stmt = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_اندريون_Init_Stmt);
        m_اندريون_Init_Stmt = سچ;
        iterateChildren(nodep);
    }

    فراغ گهمو(AstThisRef* nodep) override {}
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ForkVisitor(AstNetlist* nodep) { گهمو(nodep); }
    ~ForkVisitor() override = افتراضي;
};

//######################################################################
// Fork class functions

فراغ V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_وڻ("fork_dynscope", 0, dump_وڻ_Either_Level() >= 3);
}

فراغ V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_وڻ("fork_tasks", 0, dump_وڻ_Either_Level() >= 3);
}
