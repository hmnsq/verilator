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

বর্গ ForkDynScopeInstance final {
সার্বজনিক:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    তার্কিক initialized() ধ্রুব { প্রত্যাগম m_classp != nullptr; }
};

বর্গ ForkDynScopeFrame final {
    // MEMBERS
    Ast_গ্রন্থি_Module* ধ্রুব m_modp;  // Module to insert the scope into
    Ast_গ্রন্থি* ধ্রুব m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ধ্রুব size_t m_class_id;  // Dynscope class ID
    ধ্রুব size_t m_id;  // Dynscope ID

সার্বজনিক:
    ForkDynScopeFrame(Ast_গ্রন্থি_Module* modp, Ast_গ্রন্থি* procp, size_t class_id, size_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = নূতন AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = নূতন AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = নূতন AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(অসত্য);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        প্রত্যাগম m_instance;
    }

    ধ্রুব ForkDynScopeInstance& instance() ধ্রুব { প্রত্যাগম m_instance; }
    শূন্য capture_Var_ঢোকাও(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        যদি (r.second) m_captureOrder.ঠেলো_back(varp);
    }
    তার্কিক captured(AstVar* varp) { প্রত্যাগম m_captures.গুনতি(varp) != 0; }
    Ast_গ্রন্থি* procp() ধ্রুব { প্রত্যাগম m_procp; }

    শূন্য populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        প্রতি (AstVar* varp : m_captureOrder) {
            যদি (varp->direction().isAny()) {
                varp = varp->clone_বৃক্ষ(অসত্য);
                varp->direction(VDirection::NONE);
            } অন্যথা {
                varp->unlinkFrBack();
            }
            varp->funcLocal(অসত্য);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(অসত্য);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ধ্রুব newp
            = নূতন AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(সত্য);
        newp->classMethod(সত্য);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    শূন্য linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        যদি (AstFork* ধ্রুব forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            প্রত্যাগম;
        }

        Ast_গ্রন্থি* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_গ্রন্থি* initp = stmtp;
        যাবত্ (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ধ্রুব newp = নূতন AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), গ্রন্থি_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_গ্রন্থি* ধ্রুব asgnp = নূতন AstAssign{
            m_procp->fileline(),
            নূতন AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_গ্রন্থি* initsp = nullptr;  // Arguments need to be copied
        প্রতি (AstVar* varp : m_captureOrder) {
            যদি (!varp->direction().isAny()) অনুবর্ত;

            যদি (varp->direction().isNonOutput()) {
                AstMemberSel* ধ্রুব memberselp = নূতন AstMemberSel{
                    varp->fileline(),
                    নূতন AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_গ্রন্থি* initAsgnp
                    = নূতন AstAssign{varp->fileline(), memberselp,
                                    নূতন AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_গ্রন্থি::addNext(initsp, initAsgnp);
            }

            যদি (AstBasicDType* ধ্রুব dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                যদি (dtypep->isEvent()) অনুবর্ত;
            }

            যদি (varp->direction().isWritable()) {
                AstMemberSel* ধ্রুব memberselp = নূতন AstMemberSel{
                    varp->fileline(),
                    নূতন AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_গ্রন্থি* writebackAsgnp = নূতন AstAssign{
                    varp->fileline(), নূতন AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_গ্রন্থি::addNext(stmtp, writebackAsgnp);
            }
        }
        যদি (initsp) Ast_গ্রন্থি::addNext(asgnp, initsp);

        যদি (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } অন্যথা {
            Ast_গ্রন্থি::addNext(asgnp, static_cast<Ast_গ্রন্থি*>(initp));
            stmtp = asgnp;
        }

        Ast_গ্রন্থি::addNext(static_cast<Ast_গ্রন্থি*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    তার্কিক linked() ধ্রুব { প্রত্যাগম m_instance.initialized() && m_instance.m_handlep->backp(); }

গুপ্ত:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ধ্রুব newp = নূতন AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), গ্রন্থি_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        প্রত্যাগম নূতন AstAssign{
            m_procp->fileline(),
            নূতন AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    শূন্য linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ধ্রুব std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* ধ্রুব beginp = নূতন AstBegin{forkp->fileline(), name, m_instance.m_handlep, সত্য};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        যদি (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { প্রত্যাগম "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(ধ্রুব Ast_গ্রন্থি* fromp) {
        প্রত্যাগম "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_গ্রন্থি* getProcStmts() {
        Ast_গ্রন্থি* stmtsp = nullptr;
        যদি (!m_procp) প্রত্যাগম nullptr;
        যদি (AstBegin* ধ্রুব beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } অন্যথা যদি (Ast_গ্রন্থি_FTask* ধ্রুব taskp = VN_CAST(m_procp, গ্রন্থি_FTask)) {
            stmtsp = taskp->stmtsp();
        } অন্যথা {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        প্রত্যাগম stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

বর্গ DynScopeVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser2InUse m_inuser2;

    // STATE
    তার্কিক m_inFunc = অসত্য;  // True if in a function
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Module we are currently under
    Ast_গ্রন্থি* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_গ্রন্থি*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_গ্রন্থি*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    তার্কিক m_afterTimingControl = অসত্য;  // A timing control might've be executed in the current
                                        // process
    size_t m_id = 0;  // Unique ID for a frame
    size_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_গ্রন্থি* nodep) {
        auto frameIt = m_frames.find(nodep);
        যদি (frameIt == m_frames.end()) প্রত্যাগম nullptr;
        প্রত্যাগম frameIt->second;
    }

    ধ্রুব ForkDynScopeFrame* frameOf(Ast_গ্রন্থি* nodep) ধ্রুব {
        auto frameIt = m_frames.find(nodep);
        যদি (frameIt == m_frames.end()) প্রত্যাগম nullptr;
        প্রত্যাগম frameIt->second;
    }

    ForkDynScopeFrame* ঠেলো_Dyn_Scope_Frame(Ast_গ্রন্থি* procp) {
        ForkDynScopeFrame* ধ্রুব framep
            = নূতন ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.ঠেলো_back(procp);
        প্রত্যাগম framep;
    }

    শূন্য replaceWithMemberSel(AstVarRef* refp, ধ্রুব ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* ধ্রুব membersel = নূতন AstMemberSel{
            refp->fileline(), নূতন AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
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

    স্থির তার্কিক hasAsyncFork(Ast_গ্রন্থি* nodep) {
        প্রত্যাগম nodep->exists([](AstFork* forkp) { প্রত্যাগম !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { প্রত্যাগম সত্য; });
    }

    শূন্য bind_গ্রন্থি_To_Dyn_Scope(Ast_গ্রন্থি* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        যদি (r.second) m_frameOrder.ঠেলো_back(nodep);
    }
    শূন্য bindInitIterate(Ast_গ্রন্থি* stmtsp, ForkDynScopeFrame* framep) {
        প্রতি (Ast_গ্রন্থি* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            যদি (AstAssign* ধ্রুব asgnp = VN_CAST(stmtp, Assign)) {
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

    তার্কিক needsDynScope(ধ্রুব AstVarRef* refp) ধ্রুব {
        ধ্রুব AstVar* ধ্রুব varp = refp->varp();
        প্রত্যাগম
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

        ধ্রুব তার্কিক oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        যদি (nodep->declsp() || nodep->stmtsp()) framep = ঠেলো_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        প্রতি (Ast_গ্রন্থি* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ধ্রুব varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            যদি (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                যদি (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_ঢোকাও(varp);
                bind_গ্রন্থি_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        প্রতি (Ast_গ্রন্থি* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = অসত্য;
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
        ForkDynScopeFrame* ধ্রুব framep = frameOf(m_procp);
        যদি (!framep) প্রত্যাগম;  // Cannot be legally referenced from a fork
        bind_গ্রন্থি_To_Dyn_Scope(nodep, framep);
    }
    শূন্য visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* ধ্রুব framep = frameOf(nodep->varp());
        যদি (!framep) প্রত্যাগম;
        যদি (needsDynScope(nodep)) {
            তার্কিক isEvent = অসত্য;
            যদি (AstBasicDType* ধ্রুব dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
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
                    প্রত্যাগম refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(সত্য);
            // Put it in a fork to prevent lifetime issues with the local
            File_সারি* ধ্রুব flp = nodep->fileline();
            AstFork* ধ্রুব forkp = নূতন AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(নূতন AstBegin{flp, "", nodep, অসত্য});
            UINFO(9, "assign new fork " << forkp);
        } অন্যথা {
            visit(static_cast<Ast_গ্রন্থি_Stmt*>(nodep));
        }
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (nodep->isTimingControl()) m_afterTimingControl = সত্য;
        iterateChildren(nodep);
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        তার্কিক typesAdded = অসত্য;
        প্রতি (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            যদি (!framep->instance().initialized()) অনুবর্ত;
            যদি (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = সত্য;
            }
            যদি (AstVarRef* ধ্রুব refp = VN_CAST(frameIt->first, VarRef)) {
                যদি (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        যদি (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        প্রতি (auto গ্রন্থি_frame : m_frames) frames.ঢোকাও(গ্রন্থি_frame.second);
        প্রতি (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

বর্গ ForkVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ধ্রুব VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    size_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    তার্কিক m_inFork = অসত্য;  // Traversal in an async fork
    তার্কিক m_hasNestedFork = অসত্য;  // Branch being taskified contains a nested fork
    তার্কিক m_inInitStmt = অসত্য;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        প্রতি (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            যদি (varp->name() == refp->name()) ভঙ্গ;
        }
        যদি (varp) প্রত্যাগম varp;

        // Create a local copy to capture
        File_সারি* ধ্রুব flp = refp->fileline();
        varp = নূতন AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::প্রবেশ);
        varp->funcLocal(সত্য);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_গ্রন্থি::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ধ্রুব argp = নূতন AstArg{flp, refp->name(), refp->clone_বৃক্ষ(অসত্য)};
        m_capturedArgsp = Ast_গ্রন্থি::addNext(m_capturedArgsp, argp);
        প্রত্যাগম varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    তার্কিক taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = অসত্য;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        যদি (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) প্রত্যাগম অসত্য;

        // Create task holding the statement and repalce statement with call to that task
        File_সারি* ধ্রুব flp = beginp->fileline();
        ধ্রুব std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* ধ্রুব taskp = নূতন AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        যদি (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_গ্রন্থি::addNext(m_tasksp, taskp);
        যদি (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        যদি (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* ধ্রুব callp = নূতন AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        প্রতি (AstVar* ধ্রুব localp : m_forkLocalsp) localp->funcLocal(সত্য);

        // We did wrap the body
        প্রত্যাগম সত্য;
    }
    স্থির তার্কিক isForkJoinNoneSentinelDelay(ধ্রুব Ast_গ্রন্থি* ধ্রুব nodep) {
        ধ্রুব AstDelay* ধ্রুব delayp = VN_CAST(nodep, Delay);
        যদি (!delayp) প্রত্যাগম অসত্য;
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(delayp->lhsp(), Const);
        প্রত্যাগম constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    স্থির তার্কিক is_Disable_Queue_ঠেলো_Self_Prefix(Ast_গ্রন্থি* nodep) {
        যাবত্ (AstJumpBlock* ধ্রুব jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        প্রত্যাগম nodep && nodep->is_Disable_Queue_ঠেলো_Self_Stmt();
    }
    template <typename T_Owner>
    স্থির তার্কিক ঢোকাও_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* ধ্রুব ownerp,
                                                          Ast_গ্রন্থি* ধ্রুব firstStmtp,
                                                          Ast_গ্রন্থি* ধ্রুব delayp) {
        Ast_গ্রন্থি* ঢোকাও_Beforep = firstStmtp;
        যাবত্ (ঢোকাও_Beforep && ঢোকাও_Beforep->is_Disable_Queue_ঠেলো_Self_Stmt()) {
            ঢোকাও_Beforep = ঢোকাও_Beforep->nextp();
        }
        যদি (AstJumpBlock* ধ্রুব jumpBlockp = VN_CAST(ঢোকাও_Beforep, JumpBlock)) {
            যদি (ঢোকাও_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                প্রত্যাগম সত্য;
            }
        }
        যদি (ঢোকাও_Beforep == firstStmtp) প্রত্যাগম অসত্য;
        যদি (ঢোকাও_Beforep) {
            ঢোকাও_Beforep->addHereThisAsNext(delayp);
        } অন্যথা {
            ownerp->addStmtsp(delayp);
        }
        প্রত্যাগম সত্য;
    }
    স্থির শূন্য moveForkSentinelAfterDisableQueuePushes(AstBegin* ধ্রুব beginp) {
        Ast_গ্রন্থি* ধ্রুব firstStmtp = beginp->stmtsp();
        যদি (!isForkJoinNoneSentinelDelay(firstStmtp)) প্রত্যাগম;
        Ast_গ্রন্থি* ধ্রুব afterSentinelp = firstStmtp->nextp();
        যদি (!is_Disable_Queue_ঠেলো_Self_Prefix(afterSentinelp)) প্রত্যাগম;

        Ast_গ্রন্থি* ধ্রুব delayp = firstStmtp->unlinkFrBack();
        ধ্রুব তার্কিক moved
            = ঢোকাও_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    স্থির তার্কিক forkIsDisableable(AstFork* ধ্রুব nodep) {
        প্রতি (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            যদি (is_Disable_Queue_ঠেলো_Self_Prefix(itemp->stmtsp())) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
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
            প্রতি (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                যদি (!itemp->stmtsp()) অনুবর্ত;
                AstDelay* ধ্রুব delayp = নূতন AstDelay{
                    fl,
                    নূতন AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    অসত্য};
                itemp->prependStmtsp(delayp);
                moveForkSentinelAfterDisableQueuePushes(itemp);
            }
        }

        iterateAndNextNull(nodep->declsp());
        iterateAndNextNull(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        যদি (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            প্রত্যাগম;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = সত্য;
            প্রতি (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                যদি (taskify(itemp)) wrappedp.ঠেলো_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        প্রতি (AstBegin* ধ্রুব beginp : wrappedp) iterateAndNextNull(beginp);
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
        যদি (!m_inFork) প্রত্যাগম;
        AstVar* ধ্রুব varp = nodep->varp();
        // Not sure why this is OK ...
        যদি (!varp->isFuncLocal() && varp->isClassMember()) প্রত্যাগম;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        যদি (varp->lifetime().isStatic()) প্রত্যাগম;
        // If this ref is to a variable that will move into the task, then nothing to do
        যদি (m_forkLocalsp.গুনতি(varp)) প্রত্যাগম;

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
            প্রত্যাগম;
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

সার্বজনিক:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = স্বভাব;
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
