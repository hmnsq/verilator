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
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    منطقي initialized() ثابت { أرجع m_classp != nullptr; }
};

صنف ForkDynScopeFrame final {
    // MEMBERS
    Ast_عقدة_Module* ثابت m_modp;  // Module to insert the scope into
    Ast_عقدة* ثابت m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ثابت size_t m_class_id;  // Dynscope class ID
    ثابت size_t m_id;  // Dynscope ID

عام:
    ForkDynScopeFrame(Ast_عقدة_Module* modp, Ast_عقدة* procp, size_t class_id, size_t id)
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
            = جديد AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = جديد AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(خطأ);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        أرجع m_instance;
    }

    ثابت ForkDynScopeInstance& instance() ثابت { أرجع m_instance; }
    فراغ capture_Var_أدرج(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        إذا (r.second) m_captureOrder.ادفع_back(varp);
    }
    منطقي captured(AstVar* varp) { أرجع m_captures.عدد(varp) != 0; }
    Ast_عقدة* procp() ثابت { أرجع m_procp; }

    فراغ populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        لكل (AstVar* varp : m_captureOrder) {
            إذا (varp->direction().isAny()) {
                varp = varp->clone_شجرة(خطأ);
                varp->direction(VDirection::NONE);
            } وإلا {
                varp->unlinkFrBack();
            }
            varp->funcLocal(خطأ);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(خطأ);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ثابت newp
            = جديد AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(true);
        newp->classMethod(true);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    فراغ linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        إذا (AstFork* ثابت forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            أرجع;
        }

        Ast_عقدة* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_عقدة* initp = stmtp;
        طالما (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ثابت newp = جديد AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), عقدة_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_عقدة* ثابت asgnp = جديد AstAssign{
            m_procp->fileline(),
            جديد AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_عقدة* initsp = nullptr;  // Arguments need to be copied
        لكل (AstVar* varp : m_captureOrder) {
            إذا (!varp->direction().isAny()) استمر;

            إذا (varp->direction().isNonOutput()) {
                AstMemberSel* ثابت memberselp = جديد AstMemberSel{
                    varp->fileline(),
                    جديد AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_عقدة* initAsgnp
                    = جديد AstAssign{varp->fileline(), memberselp,
                                    جديد AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_عقدة::addNext(initsp, initAsgnp);
            }

            إذا (AstBasicDType* ثابت dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                إذا (dtypep->isEvent()) استمر;
            }

            إذا (varp->direction().isWritable()) {
                AstMemberSel* ثابت memberselp = جديد AstMemberSel{
                    varp->fileline(),
                    جديد AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_عقدة* writebackAsgnp = جديد AstAssign{
                    varp->fileline(), جديد AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_عقدة::addNext(stmtp, writebackAsgnp);
            }
        }
        إذا (initsp) Ast_عقدة::addNext(asgnp, initsp);

        إذا (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } وإلا {
            Ast_عقدة::addNext(asgnp, static_cast<Ast_عقدة*>(initp));
            stmtp = asgnp;
        }

        Ast_عقدة::addNext(static_cast<Ast_عقدة*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    منطقي linked() ثابت { أرجع m_instance.initialized() && m_instance.m_handlep->backp(); }

خاص:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ثابت newp = جديد AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), عقدة_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        أرجع جديد AstAssign{
            m_procp->fileline(),
            جديد AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    فراغ linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ثابت std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* ثابت beginp = جديد AstBegin{forkp->fileline(), name, m_instance.m_handlep, true};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        إذا (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { أرجع "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(ثابت Ast_عقدة* fromp) {
        أرجع "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_عقدة* getProcStmts() {
        Ast_عقدة* stmtsp = nullptr;
        إذا (!m_procp) أرجع nullptr;
        إذا (AstBegin* ثابت beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } وإلا إذا (Ast_عقدة_FTask* ثابت taskp = VN_CAST(m_procp, عقدة_FTask)) {
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
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    // STATE
    منطقي m_inFunc = خطأ;  // True if in a function
    Ast_عقدة_Module* m_modp = nullptr;  // Module we are currently under
    Ast_عقدة* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_عقدة*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_عقدة*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    منطقي m_afterTimingControl = خطأ;  // A timing control might've be executed in the current
                                        // process
    size_t m_id = 0;  // Unique ID for a frame
    size_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_عقدة* nodep) {
        auto frameIt = m_frames.find(nodep);
        إذا (frameIt == m_frames.end()) أرجع nullptr;
        أرجع frameIt->second;
    }

    ثابت ForkDynScopeFrame* frameOf(Ast_عقدة* nodep) ثابت {
        auto frameIt = m_frames.find(nodep);
        إذا (frameIt == m_frames.end()) أرجع nullptr;
        أرجع frameIt->second;
    }

    ForkDynScopeFrame* ادفع_Dyn_Scope_Frame(Ast_عقدة* procp) {
        ForkDynScopeFrame* ثابت framep
            = جديد ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.ادفع_back(procp);
        أرجع framep;
    }

    فراغ replaceWithMemberSel(AstVarRef* refp, ثابت ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* ثابت membersel = جديد AstMemberSel{
            refp->fileline(), جديد AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        إذا (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } وإلا {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ادفع_Deletep(refp), refp);
    }

    ساكن منطقي hasAsyncFork(Ast_عقدة* nodep) {
        أرجع nodep->exists([](AstFork* forkp) { أرجع !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { أرجع true; });
    }

    فراغ bind_عقدة_To_Dyn_Scope(Ast_عقدة* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        إذا (r.second) m_frameOrder.ادفع_back(nodep);
    }
    فراغ bindInitIterate(Ast_عقدة* stmtsp, ForkDynScopeFrame* framep) {
        لكل (Ast_عقدة* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            إذا (AstAssign* ثابت asgnp = VN_CAST(stmtp, Assign)) {
                bind_عقدة_To_Dyn_Scope(asgnp->lhsp(), framep);
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
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_قيمة() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_modp);
        إذا (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_عقدة_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        إذا (hasAsyncFork(nodep)) ادفع_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    فراغ visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        إذا (hasAsyncFork(nodep)) ادفع_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    فراغ visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        إذا (!nodep->joinType().join()) ++m_forkDepth;

        ثابت منطقي oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        إذا (nodep->declsp() || nodep->stmtsp()) framep = ادفع_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        لكل (Ast_عقدة* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ثابت varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            إذا (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                إذا (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_أدرج(varp);
                bind_عقدة_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        لكل (Ast_عقدة* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = خطأ;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        إذا (nodep->isTimingControl()) m_afterTimingControl = true;
    }
    فراغ visit(Ast_عقدة_FTask_Ref* nodep) override {
        visit(static_cast<Ast_عقدة_تعبير*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = true;
    }
    فراغ visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* ثابت framep = frameOf(m_procp);
        إذا (!framep) أرجع;  // Cannot be legally referenced from a fork
        bind_عقدة_To_Dyn_Scope(nodep, framep);
    }
    فراغ visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* ثابت framep = frameOf(nodep->varp());
        إذا (!framep) أرجع;
        إذا (needsDynScope(nodep)) {
            منطقي isEvent = خطأ;
            إذا (AstBasicDType* ثابت dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            إذا (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            إذا (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_أدرج(nodep->varp());
        }
        bind_عقدة_To_Dyn_Scope(nodep, framep);
    }
    فراغ visit(AstAssign* nodep) override {
        إذا (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_قيمة()) {
            nodep->lhsp()->user2(true);
        }
        visit(static_cast<Ast_عقدة_Stmt*>(nodep));
    }
    فراغ visit(AstAssignDly* nodep) override {
        إذا (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    أرجع refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(true);
            // Put it in a fork to prevent lifetime issues with the local
            File_سطر* ثابت flp = nodep->fileline();
            AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(جديد AstBegin{flp, "", nodep, خطأ});
            UINFO(9, "assign new fork " << forkp);
        } وإلا {
            visit(static_cast<Ast_عقدة_Stmt*>(nodep));
        }
    }
    فراغ visit(Ast_عقدة* nodep) override {
        إذا (nodep->isTimingControl()) m_afterTimingControl = true;
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        منطقي typesAdded = خطأ;
        لكل (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            إذا (!framep->instance().initialized()) استمر;
            إذا (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = true;
            }
            إذا (AstVarRef* ثابت refp = VN_CAST(frameIt->first, VarRef)) {
                إذا (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        إذا (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        لكل (auto عقدة_frame : m_frames) frames.أدرج(عقدة_frame.second);
        لكل (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

صنف ForkVisitor final : عام VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ثابت VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_عقدة_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    size_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    منطقي m_inFork = خطأ;  // Traversal in an async fork
    منطقي m_hasNestedFork = خطأ;  // Branch being taskified contains a nested fork
    منطقي m_inInitStmt = خطأ;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_عقدة_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        لكل (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (varp->name() == refp->name()) اكسر;
        }
        إذا (varp) أرجع varp;

        // Create a local copy to capture
        File_سطر* ثابت flp = refp->fileline();
        varp = جديد AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::مدخل);
        varp->funcLocal(true);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_عقدة::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ثابت argp = جديد AstArg{flp, refp->name(), refp->clone_شجرة(خطأ)};
        m_capturedArgsp = Ast_عقدة::addNext(m_capturedArgsp, argp);
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
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        إذا (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) أرجع خطأ;

        // Create task holding the statement and repalce statement with call to that task
        File_سطر* ثابت flp = beginp->fileline();
        ثابت std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* ثابت taskp = جديد AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        إذا (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_عقدة::addNext(m_tasksp, taskp);
        إذا (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        إذا (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* ثابت callp = جديد AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        لكل (AstVar* ثابت localp : m_forkLocalsp) localp->funcLocal(true);

        // We did wrap the body
        أرجع true;
    }
    ساكن منطقي isForkJoinNoneSentinelDelay(ثابت Ast_عقدة* ثابت nodep) {
        ثابت AstDelay* ثابت delayp = VN_CAST(nodep, Delay);
        إذا (!delayp) أرجع خطأ;
        ثابت AstConst* ثابت constp = VN_CAST(delayp->lhsp(), Const);
        أرجع constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    ساكن منطقي is_Disable_Queue_ادفع_Self_Prefix(Ast_عقدة* nodep) {
        طالما (AstJumpBlock* ثابت jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        أرجع nodep && nodep->is_Disable_Queue_ادفع_Self_Stmt();
    }
    template <typename T_Owner>
    ساكن منطقي أدرج_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* ثابت ownerp,
                                                          Ast_عقدة* ثابت firstStmtp,
                                                          Ast_عقدة* ثابت delayp) {
        Ast_عقدة* أدرج_Beforep = firstStmtp;
        طالما (أدرج_Beforep && أدرج_Beforep->is_Disable_Queue_ادفع_Self_Stmt()) {
            أدرج_Beforep = أدرج_Beforep->nextp();
        }
        إذا (AstJumpBlock* ثابت jumpBlockp = VN_CAST(أدرج_Beforep, JumpBlock)) {
            إذا (أدرج_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                أرجع true;
            }
        }
        إذا (أدرج_Beforep == firstStmtp) أرجع خطأ;
        إذا (أدرج_Beforep) {
            أدرج_Beforep->addHereThisAsNext(delayp);
        } وإلا {
            ownerp->addStmtsp(delayp);
        }
        أرجع true;
    }
    ساكن فراغ moveForkSentinelAfterDisableQueuePushes(AstBegin* ثابت beginp) {
        Ast_عقدة* ثابت firstStmtp = beginp->stmtsp();
        إذا (!isForkJoinNoneSentinelDelay(firstStmtp)) أرجع;
        Ast_عقدة* ثابت afterSentinelp = firstStmtp->nextp();
        إذا (!is_Disable_Queue_ادفع_Self_Prefix(afterSentinelp)) أرجع;

        Ast_عقدة* ثابت delayp = firstStmtp->unlinkFrBack();
        ثابت منطقي moved
            = أدرج_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ساكن منطقي forkIsDisableable(AstFork* ثابت nodep) {
        لكل (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            إذا (is_Disable_Queue_ادفع_Self_Prefix(itemp->stmtsp())) أرجع true;
        }
        أرجع خطأ;
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        إذا (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    فراغ visit(AstFork* nodep) override {
        m_hasNestedFork = true;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        إذا ((nodep->joinType().joinNone() && !nodep->immediateStart())
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
        إذا (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            أرجع;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = true;
            لكل (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                إذا (taskify(itemp)) wrappedp.ادفع_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        لكل (AstBegin* ثابت beginp : wrappedp) iterateAndNextNull(beginp);
    }
    فراغ visit(Ast_عقدة_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        إذا (m_inFork) m_forkLocalsp.أدرج(nodep);
    }
    فراغ visit(AstVarRef* nodep) override {
        إذا (!m_inFork) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Not sure why this is OK ...
        إذا (!varp->isFuncLocal() && varp->isClassMember()) أرجع;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        إذا (varp->lifetime().isStatic()) أرجع;
        // If this ref is to a variable that will move into the task, then nothing to do
        إذا (m_forkLocalsp.عدد(varp)) أرجع;

        إذا (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_قيمة() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            أرجع;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    فراغ visit(AstAssign* nodep) override {
        إذا (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_قيمة()) {
            nodep->lhsp()->user2(true);
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = true;
        iterateChildren(nodep);
    }
    فراغ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = true;
        iterateChildren(nodep);
    }

    فراغ visit(AstThisRef* nodep) override {}
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = افتراضي;
};

//######################################################################
// Fork class functions

فراغ V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_شجرة("fork_dynscope", 0, dump_شجرة_Either_Level() >= 3);
}

فراغ V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_شجرة("fork_tasks", 0, dump_شجرة_Either_Level() >= 3);
}
