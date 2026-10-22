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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Fork.h"

#অন্তর্ভাব "V3AstNodeExpr.h"
#অন্তর্ভাব "V3MemberMap.h"

#অন্তর্ভাব <limits>
#অন্তর্ভাব <set>

VL_DEFINE_DEBUG_FUNCTIONS;

শ্রেণী ForkDynScopeInstance final {
সর্বজনীন:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    যৌক্তিক initialized() ধ্রুবক { ফেরত m_classp != nullptr; }
};

শ্রেণী ForkDynScopeFrame final {
    // MEMBERS
    Ast_গ্রন্থি_Module* ধ্রুবক m_modp;  // Module to insert the scope into
    Ast_গ্রন্থি* ধ্রুবক m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ধ্রুবক আকার_t m_class_id;  // Dynscope class ID
    ধ্রুবক আকার_t m_id;  // Dynscope ID

সর্বজনীন:
    ForkDynScopeFrame(Ast_গ্রন্থি_Module* modp, Ast_গ্রন্থি* procp, আকার_t class_id, আকার_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = নতুন AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = নতুন AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = নতুন AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(মিথ্যা);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        ফেরত m_instance;
    }

    ধ্রুবক ForkDynScopeInstance& instance() ধ্রুবক { ফেরত m_instance; }
    শূন্য capture_Var_ঢোকাও(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        যদি (r.second) m_captureOrder.ঠেলো_back(varp);
    }
    যৌক্তিক captured(AstVar* varp) { ফেরত m_captures.গুনতি(varp) != 0; }
    Ast_গ্রন্থি* procp() ধ্রুবক { ফেরত m_procp; }

    শূন্য populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        জন্য (AstVar* varp : m_captureOrder) {
            যদি (varp->direction().isAny()) {
                varp = varp->clone_বৃক্ষ(মিথ্যা);
                varp->direction(VDirection::NONE);
            } অন্যথা {
                varp->unlinkFrBack();
            }
            varp->funcLocal(মিথ্যা);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(মিথ্যা);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ধ্রুবক newp
            = নতুন AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(সত্য);
        newp->classMethod(সত্য);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    শূন্য linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        যদি (AstFork* ধ্রুবক forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            ফেরত;
        }

        Ast_গ্রন্থি* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_Back_With_পরবর্তী(&stmtpHandle);

        // Find node after last variable declaration
        Ast_গ্রন্থি* initp = stmtp;
        যখন (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ধ্রুবক newp = নতুন AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), গ্রন্থি_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_গ্রন্থি* ধ্রুবক asgnp = নতুন AstAssign{
            m_procp->fileline(),
            নতুন AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_গ্রন্থি* initsp = nullptr;  // Arguments need to be copied
        জন্য (AstVar* varp : m_captureOrder) {
            যদি (!varp->direction().isAny()) চালু;

            যদি (varp->direction().isNonOutput()) {
                AstMemberSel* ধ্রুবক memberselp = নতুন AstMemberSel{
                    varp->fileline(),
                    নতুন AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_গ্রন্থি* initAsgnp
                    = নতুন AstAssign{varp->fileline(), memberselp,
                                    নতুন AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_গ্রন্থি::add_পরবর্তী(initsp, initAsgnp);
            }

            যদি (AstBasicDType* ধ্রুবক dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                যদি (dtypep->isEvent()) চালু;
            }

            যদি (varp->direction().isWritable()) {
                AstMemberSel* ধ্রুবক memberselp = নতুন AstMemberSel{
                    varp->fileline(),
                    নতুন AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_গ্রন্থি* writebackAsgnp = নতুন AstAssign{
                    varp->fileline(), নতুন AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_গ্রন্থি::add_পরবর্তী(stmtp, writebackAsgnp);
            }
        }
        যদি (initsp) Ast_গ্রন্থি::add_পরবর্তী(asgnp, initsp);

        যদি (initp != stmtp) {
            initp->add_Here_This_As_পরবর্তী(asgnp);
        } অন্যথা {
            Ast_গ্রন্থি::add_পরবর্তী(asgnp, static_cast<Ast_গ্রন্থি*>(initp));
            stmtp = asgnp;
        }

        Ast_গ্রন্থি::add_পরবর্তী(static_cast<Ast_গ্রন্থি*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    যৌক্তিক linked() ধ্রুবক { ফেরত m_instance.initialized() && m_instance.m_handlep->backp(); }

ব্যক্তিগত:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ধ্রুবক newp = নতুন AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), গ্রন্থি_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        ফেরত নতুন AstAssign{
            m_procp->fileline(),
            নতুন AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    শূন্য linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ধ্রুবক std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* ধ্রুবক beginp = নতুন AstBegin{forkp->fileline(), name, m_instance.m_handlep, সত্য};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        যদি (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { ফেরত "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(ধ্রুবক Ast_গ্রন্থি* fromp) {
        ফেরত "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_গ্রন্থি* getProcStmts() {
        Ast_গ্রন্থি* stmtsp = nullptr;
        যদি (!m_procp) ফেরত nullptr;
        যদি (AstBegin* ধ্রুবক beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } অন্যথা যদি (Ast_গ্রন্থি_FTask* ধ্রুবক taskp = VN_CAST(m_procp, গ্রন্থি_FTask)) {
            stmtsp = taskp->stmtsp();
        } অন্যথা {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        ফেরত stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

শ্রেণী DynScopeVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    ধ্রুবক VNUser1InUse m_inuser1;
    ধ্রুবক VNUser2InUse m_inuser2;

    // STATE
    যৌক্তিক m_inFunc = মিথ্যা;  // True if in a function
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Module we are currently under
    Ast_গ্রন্থি* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_গ্রন্থি*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_গ্রন্থি*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    যৌক্তিক m_afterTimingControl = মিথ্যা;  // A timing control might've be executed in the current
                                        // process
    আকার_t m_id = 0;  // Unique ID for a frame
    আকার_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_গ্রন্থি* nodep) {
        auto frameIt = m_frames.find(nodep);
        যদি (frameIt == m_frames.end()) ফেরত nullptr;
        ফেরত frameIt->second;
    }

    ধ্রুবক ForkDynScopeFrame* frameOf(Ast_গ্রন্থি* nodep) ধ্রুবক {
        auto frameIt = m_frames.find(nodep);
        যদি (frameIt == m_frames.end()) ফেরত nullptr;
        ফেরত frameIt->second;
    }

    ForkDynScopeFrame* ঠেলো_Dyn_Scope_Frame(Ast_গ্রন্থি* procp) {
        ForkDynScopeFrame* ধ্রুবক framep
            = নতুন ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.ঠেলো_back(procp);
        ফেরত framep;
    }

    শূন্য replaceWithMemberSel(AstVarRef* refp, ধ্রুবক ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* ধ্রুবক membersel = নতুন AstMemberSel{
            refp->fileline(), নতুন AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        যদি (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } অন্যথা {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
    }

    স্থির যৌক্তিক hasAsyncFork(Ast_গ্রন্থি* nodep) {
        ফেরত nodep->exists([](AstFork* forkp) { ফেরত !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { ফেরত সত্য; });
    }

    শূন্য bind_গ্রন্থি_To_Dyn_Scope(Ast_গ্রন্থি* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        যদি (r.second) m_frameOrder.ঠেলো_back(nodep);
    }
    শূন্য bindInitIterate(Ast_গ্রন্থি* stmtsp, ForkDynScopeFrame* framep) {
        জন্য (Ast_গ্রন্থি* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            যদি (AstAssign* ধ্রুবক asgnp = VN_CAST(stmtp, Assign)) {
                bind_গ্রন্থি_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } অন্যথা যদি (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } অন্যথা যদি (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } অন্যথা {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    যৌক্তিক needsDynScope(ধ্রুবক AstVarRef* refp) ধ্রুবক {
        ধ্রুবক AstVar* ধ্রুবক varp = refp->varp();
        ফেরত
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_মান() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        যদি (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        যদি (hasAsyncFork(nodep)) ঠেলো_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    শূন্য visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        যদি (hasAsyncFork(nodep)) ঠেলো_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    শূন্য visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        যদি (!nodep->joinType().join()) ++m_forkDepth;

        ধ্রুবক যৌক্তিক oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        যদি (nodep->declsp() || nodep->stmtsp()) framep = ঠেলো_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        জন্য (Ast_গ্রন্থি* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ধ্রুবক varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            যদি (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                যদি (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_ঢোকাও(varp);
                bind_গ্রন্থি_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        জন্য (Ast_গ্রন্থি* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = মিথ্যা;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        যদি (nodep->isTimingControl()) m_afterTimingControl = সত্য;
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        visit(static_cast<Ast_গ্রন্থি_রাশিমালা*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = সত্য;
    }
    শূন্য visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* ধ্রুবক framep = frameOf(m_procp);
        যদি (!framep) ফেরত;  // Cannot be legally referenced from a fork
        bind_গ্রন্থি_To_Dyn_Scope(nodep, framep);
    }
    শূন্য visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* ধ্রুবক framep = frameOf(nodep->varp());
        যদি (!framep) ফেরত;
        যদি (needsDynScope(nodep)) {
            যৌক্তিক isEvent = মিথ্যা;
            যদি (AstBasicDType* ধ্রুবক dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            যদি (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            যদি (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_ঢোকাও(nodep->varp());
        }
        bind_গ্রন্থি_To_Dyn_Scope(nodep, framep);
    }
    শূন্য visit(AstAssign* nodep) override {
        যদি (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_মান()) {
            nodep->lhsp()->user2(সত্য);
        }
        visit(static_cast<Ast_গ্রন্থি_Stmt*>(nodep));
    }
    শূন্য visit(AstAssignDly* nodep) override {
        যদি (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    ফেরত refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(সত্য);
            // Put it in a fork to prevent lifetime issues with the local
            File_সারি* ধ্রুবক flp = nodep->fileline();
            AstFork* ধ্রুবক forkp = নতুন AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(নতুন AstBegin{flp, "", nodep, মিথ্যা});
            UINFO(9, "assign new fork " << forkp);
        } অন্যথা {
            visit(static_cast<Ast_গ্রন্থি_Stmt*>(nodep));
        }
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (nodep->isTimingControl()) m_afterTimingControl = সত্য;
        iterateChildren(nodep);
    }

সর্বজনীন:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        যৌক্তিক typesAdded = মিথ্যা;
        জন্য (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            যদি (!framep->instance().initialized()) চালু;
            যদি (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = সত্য;
            }
            যদি (AstVarRef* ধ্রুবক refp = VN_CAST(frameIt->first, VarRef)) {
                যদি (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        যদি (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        জন্য (auto গ্রন্থি_frame : m_frames) frames.ঢোকাও(গ্রন্থি_frame.second);
        জন্য (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

শ্রেণী ForkVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ধ্রুবক VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    আকার_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    যৌক্তিক m_inFork = মিথ্যা;  // Traversal in an async fork
    যৌক্তিক m_hasNestedFork = মিথ্যা;  // Branch being taskified contains a nested fork
    যৌক্তিক m_inInitStmt = মিথ্যা;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        জন্য (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            যদি (varp->name() == refp->name()) বিরতি;
        }
        যদি (varp) ফেরত varp;

        // Create a local copy to capture
        File_সারি* ধ্রুবক flp = refp->fileline();
        varp = নতুন AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::প্রবেশ);
        varp->funcLocal(সত্য);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_গ্রন্থি::add_পরবর্তী(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ধ্রুবক argp = নতুন AstArg{flp, refp->name(), refp->clone_বৃক্ষ(মিথ্যা)};
        m_capturedArgsp = Ast_গ্রন্থি::add_পরবর্তী(m_capturedArgsp, argp);
        ফেরত varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    যৌক্তিক taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = মিথ্যা;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        যদি (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) ফেরত মিথ্যা;

        // Create task holding the statement and repalce statement with call to that task
        File_সারি* ধ্রুবক flp = beginp->fileline();
        ধ্রুবক std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* ধ্রুবক taskp = নতুন AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        যদি (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_গ্রন্থি::add_পরবর্তী(m_tasksp, taskp);
        যদি (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlink_Fr_Back_With_পরবর্তী());
        যদি (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
        AstTaskRef* ধ্রুবক callp = নতুন AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        জন্য (AstVar* ধ্রুবক localp : m_forkLocalsp) localp->funcLocal(সত্য);

        // We did wrap the body
        ফেরত সত্য;
    }
    স্থির যৌক্তিক isForkJoinNoneSentinelDelay(ধ্রুবক Ast_গ্রন্থি* ধ্রুবক nodep) {
        ধ্রুবক AstDelay* ধ্রুবক delayp = VN_CAST(nodep, Delay);
        যদি (!delayp) ফেরত মিথ্যা;
        ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(delayp->lhsp(), Const);
        ফেরত constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    স্থির যৌক্তিক is_Disable_Queue_ঠেলো_Self_Prefix(Ast_গ্রন্থি* nodep) {
        যখন (AstJumpBlock* ধ্রুবক jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        ফেরত nodep && nodep->is_Disable_Queue_ঠেলো_Self_Stmt();
    }
    template <typename T_Owner>
    স্থির যৌক্তিক ঢোকাও_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* ধ্রুবক ownerp,
                                                          Ast_গ্রন্থি* ধ্রুবক firstStmtp,
                                                          Ast_গ্রন্থি* ধ্রুবক delayp) {
        Ast_গ্রন্থি* ঢোকাও_Beforep = firstStmtp;
        যখন (ঢোকাও_Beforep && ঢোকাও_Beforep->is_Disable_Queue_ঠেলো_Self_Stmt()) {
            ঢোকাও_Beforep = ঢোকাও_Beforep->nextp();
        }
        যদি (AstJumpBlock* ধ্রুবক jumpBlockp = VN_CAST(ঢোকাও_Beforep, JumpBlock)) {
            যদি (ঢোকাও_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                ফেরত সত্য;
            }
        }
        যদি (ঢোকাও_Beforep == firstStmtp) ফেরত মিথ্যা;
        যদি (ঢোকাও_Beforep) {
            ঢোকাও_Beforep->add_Here_This_As_পরবর্তী(delayp);
        } অন্যথা {
            ownerp->addStmtsp(delayp);
        }
        ফেরত সত্য;
    }
    স্থির শূন্য moveForkSentinelAfterDisableQueuePushes(AstBegin* ধ্রুবক beginp) {
        Ast_গ্রন্থি* ধ্রুবক firstStmtp = beginp->stmtsp();
        যদি (!isForkJoinNoneSentinelDelay(firstStmtp)) ফেরত;
        Ast_গ্রন্থি* ধ্রুবক afterSentinelp = firstStmtp->nextp();
        যদি (!is_Disable_Queue_ঠেলো_Self_Prefix(afterSentinelp)) ফেরত;

        Ast_গ্রন্থি* ধ্রুবক delayp = firstStmtp->unlinkFrBack();
        ধ্রুবক যৌক্তিক moved
            = ঢোকাও_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    স্থির যৌক্তিক forkIsDisableable(AstFork* ধ্রুবক nodep) {
        জন্য (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            যদি (is_Disable_Queue_ঠেলো_Self_Prefix(itemp->stmtsp())) ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        যদি (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    শূন্য visit(AstFork* nodep) override {
        m_hasNestedFork = সত্য;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        যদি ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_সারি* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            জন্য (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                যদি (!itemp->stmtsp()) চালু;
                AstDelay* ধ্রুবক delayp = নতুন AstDelay{
                    fl,
                    নতুন AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    মিথ্যা};
                itemp->prependStmtsp(delayp);
                moveForkSentinelAfterDisableQueuePushes(itemp);
            }
        }

        iterate_And_পরবর্তী_Null(nodep->declsp());
        iterate_And_পরবর্তী_Null(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        যদি (nodep->joinType().join()) {
            iterate_And_পরবর্তী_Null(nodep->forksp());
            ফেরত;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = সত্য;
            জন্য (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                যদি (taskify(itemp)) wrappedp.ঠেলো_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        জন্য (AstBegin* ধ্রুবক beginp : wrappedp) iterate_And_পরবর্তী_Null(beginp);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstVar* nodep) override {
        যদি (m_inFork) m_forkLocalsp.ঢোকাও(nodep);
    }
    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_inFork) ফেরত;
        AstVar* ধ্রুবক varp = nodep->varp();
        // Not sure why this is OK ...
        যদি (!varp->isFuncLocal() && varp->isClassMember()) ফেরত;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        যদি (varp->lifetime().isStatic()) ফেরত;
        // If this ref is to a variable that will move into the task, then nothing to do
        যদি (m_forkLocalsp.গুনতি(varp)) ফেরত;

        যদি (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_মান() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            ফেরত;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    শূন্য visit(AstAssign* nodep) override {
        যদি (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_মান()) {
            nodep->lhsp()->user2(সত্য);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = সত্য;
        iterateChildren(nodep);
    }

    শূন্য visit(AstThisRef* nodep) override {}
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Fork class functions

শূন্য V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_বৃক্ষ("fork_dynscope", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

শূন্য V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_বৃক্ষ("fork_tasks", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
