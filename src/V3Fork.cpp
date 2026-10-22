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

رده ForkDynScopeInstance final {
عمومی:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    منطقی initialized() ثابت { بازگشت m_classp != nullptr; }
};

رده ForkDynScopeFrame final {
    // MEMBERS
    Ast_گره_Module* ثابت m_modp;  // Module to insert the scope into
    Ast_گره* ثابت m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ثابت size_t m_class_id;  // Dynscope class ID
    ثابت size_t m_id;  // Dynscope ID

عمومی:
    ForkDynScopeFrame(Ast_گره_Module* modp, Ast_گره* procp, size_t class_id, size_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = جدید AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = جدید AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = جدید AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(نادرست);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        بازگشت m_instance;
    }

    ثابت ForkDynScopeInstance& instance() ثابت { بازگشت m_instance; }
    تهی capture_Var_درج(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        اگر (r.second) m_captureOrder.افزودن_back(varp);
    }
    منطقی captured(AstVar* varp) { بازگشت m_captures.شمار(varp) != 0; }
    Ast_گره* procp() ثابت { بازگشت m_procp; }

    تهی populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        برای (AstVar* varp : m_captureOrder) {
            اگر (varp->direction().isAny()) {
                varp = varp->clone_درخت(نادرست);
                varp->direction(VDirection::NONE);
            } وگرنه {
                varp->unlinkFrBack();
            }
            varp->funcLocal(نادرست);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(نادرست);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ثابت newp
            = جدید AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(درست);
        newp->classMethod(درست);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    تهی linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        اگر (AstFork* ثابت forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            بازگشت;
        }

        Ast_گره* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_گره* initp = stmtp;
        وقتی (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ثابت newp = جدید AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), گره_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_گره* ثابت asgnp = جدید AstAssign{
            m_procp->fileline(),
            جدید AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_گره* initsp = nullptr;  // Arguments need to be copied
        برای (AstVar* varp : m_captureOrder) {
            اگر (!varp->direction().isAny()) ادامه;

            اگر (varp->direction().isNonOutput()) {
                AstMemberSel* ثابت memberselp = جدید AstMemberSel{
                    varp->fileline(),
                    جدید AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_گره* initAsgnp
                    = جدید AstAssign{varp->fileline(), memberselp,
                                    جدید AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_گره::addNext(initsp, initAsgnp);
            }

            اگر (AstBasicDType* ثابت dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                اگر (dtypep->isEvent()) ادامه;
            }

            اگر (varp->direction().isWritable()) {
                AstMemberSel* ثابت memberselp = جدید AstMemberSel{
                    varp->fileline(),
                    جدید AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_گره* writebackAsgnp = جدید AstAssign{
                    varp->fileline(), جدید AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_گره::addNext(stmtp, writebackAsgnp);
            }
        }
        اگر (initsp) Ast_گره::addNext(asgnp, initsp);

        اگر (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } وگرنه {
            Ast_گره::addNext(asgnp, static_cast<Ast_گره*>(initp));
            stmtp = asgnp;
        }

        Ast_گره::addNext(static_cast<Ast_گره*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    منطقی linked() ثابت { بازگشت m_instance.initialized() && m_instance.m_handlep->backp(); }

خصوصی:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ثابت newp = جدید AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), گره_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        بازگشت جدید AstAssign{
            m_procp->fileline(),
            جدید AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    تهی linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ثابت std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* ثابت beginp = جدید AstBegin{forkp->fileline(), name, m_instance.m_handlep, درست};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        اگر (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { بازگشت "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(ثابت Ast_گره* fromp) {
        بازگشت "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_گره* getProcStmts() {
        Ast_گره* stmtsp = nullptr;
        اگر (!m_procp) بازگشت nullptr;
        اگر (AstBegin* ثابت beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } وگرنه اگر (Ast_گره_FTask* ثابت taskp = VN_CAST(m_procp, گره_FTask)) {
            stmtsp = taskp->stmtsp();
        } وگرنه {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        بازگشت stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

رده DynScopeVisitor final : عمومی VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    // STATE
    منطقی m_inFunc = نادرست;  // True if in a function
    Ast_گره_Module* m_modp = nullptr;  // Module we are currently under
    Ast_گره* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_گره*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_گره*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    منطقی m_afterTimingControl = نادرست;  // A timing control might've be executed in the current
                                        // process
    size_t m_id = 0;  // Unique ID for a frame
    size_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_گره* nodep) {
        auto frameIt = m_frames.find(nodep);
        اگر (frameIt == m_frames.end()) بازگشت nullptr;
        بازگشت frameIt->second;
    }

    ثابت ForkDynScopeFrame* frameOf(Ast_گره* nodep) ثابت {
        auto frameIt = m_frames.find(nodep);
        اگر (frameIt == m_frames.end()) بازگشت nullptr;
        بازگشت frameIt->second;
    }

    ForkDynScopeFrame* افزودن_Dyn_Scope_Frame(Ast_گره* procp) {
        ForkDynScopeFrame* ثابت framep
            = جدید ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.افزودن_back(procp);
        بازگشت framep;
    }

    تهی replaceWithMemberSel(AstVarRef* refp, ثابت ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* ثابت membersel = جدید AstMemberSel{
            refp->fileline(), جدید AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        اگر (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } وگرنه {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
    }

    ایستا منطقی hasAsyncFork(Ast_گره* nodep) {
        بازگشت nodep->exists([](AstFork* forkp) { بازگشت !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { بازگشت درست; });
    }

    تهی bind_گره_To_Dyn_Scope(Ast_گره* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        اگر (r.second) m_frameOrder.افزودن_back(nodep);
    }
    تهی bindInitIterate(Ast_گره* stmtsp, ForkDynScopeFrame* framep) {
        برای (Ast_گره* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            اگر (AstAssign* ثابت asgnp = VN_CAST(stmtp, Assign)) {
                bind_گره_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } وگرنه اگر (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } وگرنه اگر (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } وگرنه {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    منطقی needsDynScope(ثابت AstVarRef* refp) ثابت {
        ثابت AstVar* ثابت varp = refp->varp();
        بازگشت
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_مقدار() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        اگر (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        اگر (hasAsyncFork(nodep)) افزودن_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    تهی visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        اگر (hasAsyncFork(nodep)) افزودن_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    تهی visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        اگر (!nodep->joinType().join()) ++m_forkDepth;

        ثابت منطقی oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        اگر (nodep->declsp() || nodep->stmtsp()) framep = افزودن_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        برای (Ast_گره* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ثابت varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            اگر (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                اگر (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_درج(varp);
                bind_گره_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        برای (Ast_گره* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = نادرست;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        اگر (nodep->isTimingControl()) m_afterTimingControl = درست;
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        visit(static_cast<Ast_گره_عبارت*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = درست;
    }
    تهی visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* ثابت framep = frameOf(m_procp);
        اگر (!framep) بازگشت;  // Cannot be legally referenced from a fork
        bind_گره_To_Dyn_Scope(nodep, framep);
    }
    تهی visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* ثابت framep = frameOf(nodep->varp());
        اگر (!framep) بازگشت;
        اگر (needsDynScope(nodep)) {
            منطقی isEvent = نادرست;
            اگر (AstBasicDType* ثابت dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            اگر (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            اگر (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_درج(nodep->varp());
        }
        bind_گره_To_Dyn_Scope(nodep, framep);
    }
    تهی visit(AstAssign* nodep) override {
        اگر (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_مقدار()) {
            nodep->lhsp()->user2(درست);
        }
        visit(static_cast<Ast_گره_Stmt*>(nodep));
    }
    تهی visit(AstAssignDly* nodep) override {
        اگر (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    بازگشت refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(درست);
            // Put it in a fork to prevent lifetime issues with the local
            File_سطر* ثابت flp = nodep->fileline();
            AstFork* ثابت forkp = جدید AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(جدید AstBegin{flp, "", nodep, نادرست});
            UINFO(9, "assign new fork " << forkp);
        } وگرنه {
            visit(static_cast<Ast_گره_Stmt*>(nodep));
        }
    }
    تهی visit(Ast_گره* nodep) override {
        اگر (nodep->isTimingControl()) m_afterTimingControl = درست;
        iterateChildren(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        منطقی typesAdded = نادرست;
        برای (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            اگر (!framep->instance().initialized()) ادامه;
            اگر (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = درست;
            }
            اگر (AstVarRef* ثابت refp = VN_CAST(frameIt->first, VarRef)) {
                اگر (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        اگر (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        برای (auto گره_frame : m_frames) frames.درج(گره_frame.second);
        برای (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

رده ForkVisitor final : عمومی VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ثابت VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_گره_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    size_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    منطقی m_inFork = نادرست;  // Traversal in an async fork
    منطقی m_hasNestedFork = نادرست;  // Branch being taskified contains a nested fork
    منطقی m_inInitStmt = نادرست;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_گره_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        برای (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            اگر (varp->name() == refp->name()) بشکن;
        }
        اگر (varp) بازگشت varp;

        // Create a local copy to capture
        File_سطر* ثابت flp = refp->fileline();
        varp = جدید AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::درون_داد);
        varp->funcLocal(درست);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_گره::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ثابت argp = جدید AstArg{flp, refp->name(), refp->clone_درخت(نادرست)};
        m_capturedArgsp = Ast_گره::addNext(m_capturedArgsp, argp);
        بازگشت varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    منطقی taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = نادرست;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        اگر (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) بازگشت نادرست;

        // Create task holding the statement and repalce statement with call to that task
        File_سطر* ثابت flp = beginp->fileline();
        ثابت std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* ثابت taskp = جدید AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        اگر (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_گره::addNext(m_tasksp, taskp);
        اگر (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        اگر (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* ثابت callp = جدید AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        برای (AstVar* ثابت localp : m_forkLocalsp) localp->funcLocal(درست);

        // We did wrap the body
        بازگشت درست;
    }
    ایستا منطقی isForkJoinNoneSentinelDelay(ثابت Ast_گره* ثابت nodep) {
        ثابت AstDelay* ثابت delayp = VN_CAST(nodep, Delay);
        اگر (!delayp) بازگشت نادرست;
        ثابت AstConst* ثابت constp = VN_CAST(delayp->lhsp(), Const);
        بازگشت constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    ایستا منطقی is_Disable_Queue_افزودن_Self_Prefix(Ast_گره* nodep) {
        وقتی (AstJumpBlock* ثابت jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        بازگشت nodep && nodep->is_Disable_Queue_افزودن_Self_Stmt();
    }
    template <typename T_Owner>
    ایستا منطقی درج_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* ثابت ownerp,
                                                          Ast_گره* ثابت firstStmtp,
                                                          Ast_گره* ثابت delayp) {
        Ast_گره* درج_Beforep = firstStmtp;
        وقتی (درج_Beforep && درج_Beforep->is_Disable_Queue_افزودن_Self_Stmt()) {
            درج_Beforep = درج_Beforep->nextp();
        }
        اگر (AstJumpBlock* ثابت jumpBlockp = VN_CAST(درج_Beforep, JumpBlock)) {
            اگر (درج_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                بازگشت درست;
            }
        }
        اگر (درج_Beforep == firstStmtp) بازگشت نادرست;
        اگر (درج_Beforep) {
            درج_Beforep->addHereThisAsNext(delayp);
        } وگرنه {
            ownerp->addStmtsp(delayp);
        }
        بازگشت درست;
    }
    ایستا تهی moveForkSentinelAfterDisableQueuePushes(AstBegin* ثابت beginp) {
        Ast_گره* ثابت firstStmtp = beginp->stmtsp();
        اگر (!isForkJoinNoneSentinelDelay(firstStmtp)) بازگشت;
        Ast_گره* ثابت afterSentinelp = firstStmtp->nextp();
        اگر (!is_Disable_Queue_افزودن_Self_Prefix(afterSentinelp)) بازگشت;

        Ast_گره* ثابت delayp = firstStmtp->unlinkFrBack();
        ثابت منطقی moved
            = درج_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ایستا منطقی forkIsDisableable(AstFork* ثابت nodep) {
        برای (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            اگر (is_Disable_Queue_افزودن_Self_Prefix(itemp->stmtsp())) بازگشت درست;
        }
        بازگشت نادرست;
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        اگر (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    تهی visit(AstFork* nodep) override {
        m_hasNestedFork = درست;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        اگر ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_سطر* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            برای (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                اگر (!itemp->stmtsp()) ادامه;
                AstDelay* ثابت delayp = جدید AstDelay{
                    fl,
                    جدید AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    نادرست};
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
        اگر (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            بازگشت;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = درست;
            برای (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                اگر (taskify(itemp)) wrappedp.افزودن_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        برای (AstBegin* ثابت beginp : wrappedp) iterateAndNextNull(beginp);
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstVar* nodep) override {
        اگر (m_inFork) m_forkLocalsp.درج(nodep);
    }
    تهی visit(AstVarRef* nodep) override {
        اگر (!m_inFork) بازگشت;
        AstVar* ثابت varp = nodep->varp();
        // Not sure why this is OK ...
        اگر (!varp->isFuncLocal() && varp->isClassMember()) بازگشت;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        اگر (varp->lifetime().isStatic()) بازگشت;
        // If this ref is to a variable that will move into the task, then nothing to do
        اگر (m_forkLocalsp.شمار(varp)) بازگشت;

        اگر (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_مقدار() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            بازگشت;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    تهی visit(AstAssign* nodep) override {
        اگر (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_مقدار()) {
            nodep->lhsp()->user2(درست);
        }
        iterateChildren(nodep);
    }
    تهی visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = درست;
        iterateChildren(nodep);
    }
    تهی visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = درست;
        iterateChildren(nodep);
    }

    تهی visit(AstThisRef* nodep) override {}
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = پیشفرض;
};

//######################################################################
// Fork class functions

تهی V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("fork_dynscope", 0, dump_درخت_Either_Level() >= 3);
}

تهی V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("fork_tasks", 0, dump_درخت_Either_Level() >= 3);
}
