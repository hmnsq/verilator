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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Fork.h"

#अन्तर्भाव "V3AstNodeExpr.h"
#अन्तर्भाव "V3MemberMap.h"

#अन्तर्भाव <limits>
#अन्तर्भाव <set>

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग ForkDynScopeInstance final {
सार्वजनिक:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    तार्किक initialized() अचर { फर्काऊ m_classp != nullptr; }
};

वर्ग ForkDynScopeFrame final {
    // MEMBERS
    Ast_गाँठो_Module* अचर m_modp;  // Module to insert the scope into
    Ast_गाँठो* अचर m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    अचर माप_प्रकार m_class_id;  // Dynscope class ID
    अचर माप_प्रकार m_id;  // Dynscope ID

सार्वजनिक:
    ForkDynScopeFrame(Ast_गाँठो_Module* modp, Ast_गाँठो* procp, माप_प्रकार class_id, माप_प्रकार id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = नूतन AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = नूतन AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = नूतन AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(असत्य);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        फर्काऊ m_instance;
    }

    अचर ForkDynScopeInstance& instance() अचर { फर्काऊ m_instance; }
    शून्य capture_Var_घुसाउ(AstVar* varp) {
        स्वतः r = m_captures.emplace(varp);
        यदि (r.second) m_captureOrder.थप_back(varp);
    }
    तार्किक captured(AstVar* varp) { फर्काऊ m_captures.गन्ती(varp) != 0; }
    Ast_गाँठो* procp() अचर { फर्काऊ m_procp; }

    शून्य populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        लागि (AstVar* varp : m_captureOrder) {
            यदि (varp->direction().isAny()) {
                varp = varp->clone_रूख(असत्य);
                varp->direction(VDirection::NONE);
            } अन्यथा {
                varp->unlinkFrBack();
            }
            varp->funcLocal(असत्य);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(असत्य);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* अचर newp
            = नूतन AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(सत्य);
        newp->classMethod(सत्य);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    शून्य linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        यदि (AstFork* अचर forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            फर्काऊ;
        }

        Ast_गाँठो* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_गाँठो* initp = stmtp;
        जबसम्म (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* अचर newp = नूतन AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), गाँठो_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_गाँठो* अचर asgnp = नूतन AstAssign{
            m_procp->fileline(),
            नूतन AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_गाँठो* initsp = nullptr;  // Arguments need to be copied
        लागि (AstVar* varp : m_captureOrder) {
            यदि (!varp->direction().isAny()) जारी;

            यदि (varp->direction().isNonOutput()) {
                AstMemberSel* अचर memberselp = नूतन AstMemberSel{
                    varp->fileline(),
                    नूतन AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_गाँठो* initAsgnp
                    = नूतन AstAssign{varp->fileline(), memberselp,
                                    नूतन AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_गाँठो::addNext(initsp, initAsgnp);
            }

            यदि (AstBasicDType* अचर dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                यदि (dtypep->isEvent()) जारी;
            }

            यदि (varp->direction().isWritable()) {
                AstMemberSel* अचर memberselp = नूतन AstMemberSel{
                    varp->fileline(),
                    नूतन AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_गाँठो* writebackAsgnp = नूतन AstAssign{
                    varp->fileline(), नूतन AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_गाँठो::addNext(stmtp, writebackAsgnp);
            }
        }
        यदि (initsp) Ast_गाँठो::addNext(asgnp, initsp);

        यदि (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } अन्यथा {
            Ast_गाँठो::addNext(asgnp, static_cast<Ast_गाँठो*>(initp));
            stmtp = asgnp;
        }

        Ast_गाँठो::addNext(static_cast<Ast_गाँठो*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    तार्किक linked() अचर { फर्काऊ m_instance.initialized() && m_instance.m_handlep->backp(); }

गुप्त:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* अचर newp = नूतन AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), गाँठो_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        फर्काऊ नूतन AstAssign{
            m_procp->fileline(),
            नूतन AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    शून्य linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        अचर std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* अचर beginp = नूतन AstBegin{forkp->fileline(), name, m_instance.m_handlep, सत्य};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        यदि (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { फर्काऊ "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(अचर Ast_गाँठो* fromp) {
        फर्काऊ "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_गाँठो* getProcStmts() {
        Ast_गाँठो* stmtsp = nullptr;
        यदि (!m_procp) फर्काऊ nullptr;
        यदि (AstBegin* अचर beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } अन्यथा यदि (Ast_गाँठो_FTask* अचर taskp = VN_CAST(m_procp, गाँठो_FTask)) {
            stmtsp = taskp->stmtsp();
        } अन्यथा {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        फर्काऊ stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

वर्ग DynScopeVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    अचर VNUser1InUse m_inuser1;
    अचर VNUser2InUse m_inuser2;

    // STATE
    तार्किक m_inFunc = असत्य;  // True if in a function
    Ast_गाँठो_Module* m_modp = nullptr;  // Module we are currently under
    Ast_गाँठो* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_गाँठो*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_गाँठो*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    तार्किक m_afterTimingControl = असत्य;  // A timing control might've be executed in the current
                                        // process
    माप_प्रकार m_id = 0;  // Unique ID for a frame
    माप_प्रकार m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_गाँठो* nodep) {
        स्वतः frameIt = m_frames.find(nodep);
        यदि (frameIt == m_frames.end()) फर्काऊ nullptr;
        फर्काऊ frameIt->second;
    }

    अचर ForkDynScopeFrame* frameOf(Ast_गाँठो* nodep) अचर {
        स्वतः frameIt = m_frames.find(nodep);
        यदि (frameIt == m_frames.end()) फर्काऊ nullptr;
        फर्काऊ frameIt->second;
    }

    ForkDynScopeFrame* थप_Dyn_Scope_Frame(Ast_गाँठो* procp) {
        ForkDynScopeFrame* अचर framep
            = नूतन ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        स्वतः r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.थप_back(procp);
        फर्काऊ framep;
    }

    शून्य replaceWithMemberSel(AstVarRef* refp, अचर ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* अचर membersel = नूतन AstMemberSel{
            refp->fileline(), नूतन AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        यदि (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } अन्यथा {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(थप_Deletep(refp), refp);
    }

    स्थिर तार्किक hasAsyncFork(Ast_गाँठो* nodep) {
        फर्काऊ nodep->exists([](AstFork* forkp) { फर्काऊ !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { फर्काऊ सत्य; });
    }

    शून्य bind_गाँठो_To_Dyn_Scope(Ast_गाँठो* nodep, ForkDynScopeFrame* framep) {
        स्वतः r = m_frames.emplace(nodep, framep);
        यदि (r.second) m_frameOrder.थप_back(nodep);
    }
    शून्य bindInitIterate(Ast_गाँठो* stmtsp, ForkDynScopeFrame* framep) {
        लागि (Ast_गाँठो* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            यदि (AstAssign* अचर asgnp = VN_CAST(stmtp, Assign)) {
                bind_गाँठो_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } अन्यथा यदि (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } अन्यथा यदि (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } अन्यथा {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    तार्किक needsDynScope(अचर AstVarRef* refp) अचर {
        अचर AstVar* अचर varp = refp->varp();
        फर्काऊ
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_मान() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_modp);
        यदि (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        यदि (hasAsyncFork(nodep)) थप_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    शून्य visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        यदि (hasAsyncFork(nodep)) थप_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    शून्य visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        यदि (!nodep->joinType().join()) ++m_forkDepth;

        अचर तार्किक oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        यदि (nodep->declsp() || nodep->stmtsp()) framep = थप_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        लागि (Ast_गाँठो* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* अचर varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            यदि (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                यदि (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_घुसाउ(varp);
                bind_गाँठो_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        लागि (Ast_गाँठो* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = असत्य;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        यदि (nodep->isTimingControl()) m_afterTimingControl = सत्य;
    }
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        visit(static_cast<Ast_गाँठो_अभिव्यक्ति*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = सत्य;
    }
    शून्य visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* अचर framep = frameOf(m_procp);
        यदि (!framep) फर्काऊ;  // Cannot be legally referenced from a fork
        bind_गाँठो_To_Dyn_Scope(nodep, framep);
    }
    शून्य visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* अचर framep = frameOf(nodep->varp());
        यदि (!framep) फर्काऊ;
        यदि (needsDynScope(nodep)) {
            तार्किक isEvent = असत्य;
            यदि (AstBasicDType* अचर dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            यदि (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            यदि (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_घुसाउ(nodep->varp());
        }
        bind_गाँठो_To_Dyn_Scope(nodep, framep);
    }
    शून्य visit(AstAssign* nodep) override {
        यदि (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_मान()) {
            nodep->lhsp()->user2(सत्य);
        }
        visit(static_cast<Ast_गाँठो_Stmt*>(nodep));
    }
    शून्य visit(AstAssignDly* nodep) override {
        यदि (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    फर्काऊ refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(सत्य);
            // Put it in a fork to prevent lifetime issues with the local
            File_पङ्क्ति* अचर flp = nodep->fileline();
            AstFork* अचर forkp = नूतन AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(नूतन AstBegin{flp, "", nodep, असत्य});
            UINFO(9, "assign new fork " << forkp);
        } अन्यथा {
            visit(static_cast<Ast_गाँठो_Stmt*>(nodep));
        }
    }
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (nodep->isTimingControl()) m_afterTimingControl = सत्य;
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        तार्किक typesAdded = असत्य;
        लागि (स्वतः orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            स्वतः frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            यदि (!framep->instance().initialized()) जारी;
            यदि (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = सत्य;
            }
            यदि (AstVarRef* अचर refp = VN_CAST(frameIt->first, VarRef)) {
                यदि (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        यदि (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        लागि (स्वतः गाँठो_frame : m_frames) frames.घुसाउ(गाँठो_frame.second);
        लागि (स्वतः* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

वर्ग ForkVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    अचर VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_गाँठो_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    माप_प्रकार m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    तार्किक m_inFork = असत्य;  // Traversal in an async fork
    तार्किक m_hasNestedFork = असत्य;  // Branch being taskified contains a nested fork
    तार्किक m_inInitStmt = असत्य;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_गाँठो_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        लागि (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            यदि (varp->name() == refp->name()) रोक;
        }
        यदि (varp) फर्काऊ varp;

        // Create a local copy to capture
        File_पङ्क्ति* अचर flp = refp->fileline();
        varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::आगत);
        varp->funcLocal(सत्य);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_गाँठो::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* अचर argp = नूतन AstArg{flp, refp->name(), refp->clone_रूख(असत्य)};
        m_capturedArgsp = Ast_गाँठो::addNext(m_capturedArgsp, argp);
        फर्काऊ varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    तार्किक taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = असत्य;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        यदि (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) फर्काऊ असत्य;

        // Create task holding the statement and repalce statement with call to that task
        File_पङ्क्ति* अचर flp = beginp->fileline();
        अचर std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* अचर taskp = नूतन AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        यदि (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_गाँठो::addNext(m_tasksp, taskp);
        यदि (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        यदि (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* अचर callp = नूतन AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        लागि (AstVar* अचर localp : m_forkLocalsp) localp->funcLocal(सत्य);

        // We did wrap the body
        फर्काऊ सत्य;
    }
    स्थिर तार्किक isForkJoinNoneSentinelDelay(अचर Ast_गाँठो* अचर nodep) {
        अचर AstDelay* अचर delayp = VN_CAST(nodep, Delay);
        यदि (!delayp) फर्काऊ असत्य;
        अचर AstConst* अचर constp = VN_CAST(delayp->lhsp(), Const);
        फर्काऊ constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    स्थिर तार्किक is_Disable_Queue_थप_Self_Prefix(Ast_गाँठो* nodep) {
        जबसम्म (AstJumpBlock* अचर jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        फर्काऊ nodep && nodep->is_Disable_Queue_थप_Self_Stmt();
    }
    ढाँचा <typename T_Owner>
    स्थिर तार्किक घुसाउ_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* अचर ownerp,
                                                          Ast_गाँठो* अचर firstStmtp,
                                                          Ast_गाँठो* अचर delayp) {
        Ast_गाँठो* घुसाउ_Beforep = firstStmtp;
        जबसम्म (घुसाउ_Beforep && घुसाउ_Beforep->is_Disable_Queue_थप_Self_Stmt()) {
            घुसाउ_Beforep = घुसाउ_Beforep->nextp();
        }
        यदि (AstJumpBlock* अचर jumpBlockp = VN_CAST(घुसाउ_Beforep, JumpBlock)) {
            यदि (घुसाउ_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                फर्काऊ सत्य;
            }
        }
        यदि (घुसाउ_Beforep == firstStmtp) फर्काऊ असत्य;
        यदि (घुसाउ_Beforep) {
            घुसाउ_Beforep->addHereThisAsNext(delayp);
        } अन्यथा {
            ownerp->addStmtsp(delayp);
        }
        फर्काऊ सत्य;
    }
    स्थिर शून्य moveForkSentinelAfterDisableQueuePushes(AstBegin* अचर beginp) {
        Ast_गाँठो* अचर firstStmtp = beginp->stmtsp();
        यदि (!isForkJoinNoneSentinelDelay(firstStmtp)) फर्काऊ;
        Ast_गाँठो* अचर afterSentinelp = firstStmtp->nextp();
        यदि (!is_Disable_Queue_थप_Self_Prefix(afterSentinelp)) फर्काऊ;

        Ast_गाँठो* अचर delayp = firstStmtp->unlinkFrBack();
        अचर तार्किक moved
            = घुसाउ_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    स्थिर तार्किक forkIsDisableable(AstFork* अचर nodep) {
        लागि (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            यदि (is_Disable_Queue_थप_Self_Prefix(itemp->stmtsp())) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        यदि (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    शून्य visit(AstFork* nodep) override {
        m_hasNestedFork = सत्य;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        यदि ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_पङ्क्ति* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            लागि (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                यदि (!itemp->stmtsp()) जारी;
                AstDelay* अचर delayp = नूतन AstDelay{
                    fl,
                    नूतन AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    असत्य};
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
        यदि (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            फर्काऊ;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = सत्य;
            लागि (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                यदि (taskify(itemp)) wrappedp.थप_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        लागि (AstBegin* अचर beginp : wrappedp) iterateAndNextNull(beginp);
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        यदि (m_inFork) m_forkLocalsp.घुसाउ(nodep);
    }
    शून्य visit(AstVarRef* nodep) override {
        यदि (!m_inFork) फर्काऊ;
        AstVar* अचर varp = nodep->varp();
        // Not sure why this is OK ...
        यदि (!varp->isFuncLocal() && varp->isClassMember()) फर्काऊ;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        यदि (varp->lifetime().isStatic()) फर्काऊ;
        // If this ref is to a variable that will move into the task, then nothing to do
        यदि (m_forkLocalsp.गन्ती(varp)) फर्काऊ;

        यदि (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_मान() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            फर्काऊ;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    शून्य visit(AstAssign* nodep) override {
        यदि (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_मान()) {
            nodep->lhsp()->user2(सत्य);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = सत्य;
        iterateChildren(nodep);
    }

    शून्य visit(AstThisRef* nodep) override {}
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = स्वभाव;
};

//######################################################################
// Fork class functions

शून्य V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_रूख("fork_dynscope", 0, dump_रूख_Either_Level() >= 3);
}

शून्य V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_रूख("fork_tasks", 0, dump_रूख_Either_Level() >= 3);
}
