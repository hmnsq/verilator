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

वग्ग ForkDynScopeInstance final {
सार्वजनिक:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    तार्किक initialized() ध्रुव { पटिनिवत्त m_classp != nullptr; }
};

वग्ग ForkDynScopeFrame final {
    // MEMBERS
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module to insert the scope into
    Ast_ग्रन्थिः* ध्रुव m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ध्रुव size_t m_class_id;  // Dynscope class ID
    ध्रुव size_t m_id;  // Dynscope ID

सार्वजनिक:
    ForkDynScopeFrame(Ast_ग्रन्थिः_Module* modp, Ast_ग्रन्थिः* procp, size_t class_id, size_t id)
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
        m_instance.m_handlep->funcLocal(मिच्छा);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        पटिनिवत्त m_instance;
    }

    ध्रुव ForkDynScopeInstance& instance() ध्रुव { पटिनिवत्त m_instance; }
    शून्य capture_Var_निवेशय(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        सचे (r.second) m_captureOrder.प्रक्षिप_back(varp);
    }
    तार्किक captured(AstVar* varp) { पटिनिवत्त m_captures.गणनम्(varp) != 0; }
    Ast_ग्रन्थिः* procp() ध्रुव { पटिनिवत्त m_procp; }

    शून्य populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        अत्थाय (AstVar* varp : m_captureOrder) {
            सचे (varp->direction().isAny()) {
                varp = varp->clone_वृक्षः(मिच्छा);
                varp->direction(VDirection::NONE);
            } अञ्ञथा {
                varp->unlinkFrBack();
            }
            varp->funcLocal(मिच्छा);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(मिच्छा);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ध्रुव newp
            = नूतन AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(सच्च);
        newp->classMethod(सच्च);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    शून्य linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        सचे (AstFork* ध्रुव forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            पटिनिवत्त;
        }

        Ast_ग्रन्थिः* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_ग्रन्थिः* initp = stmtp;
        याव (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ध्रुव newp = नूतन AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ग्रन्थिः_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_ग्रन्थिः* ध्रुव asgnp = नूतन AstAssign{
            m_procp->fileline(),
            नूतन AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_ग्रन्थिः* initsp = nullptr;  // Arguments need to be copied
        अत्थाय (AstVar* varp : m_captureOrder) {
            सचे (!varp->direction().isAny()) अनुवर्त;

            सचे (varp->direction().isNonOutput()) {
                AstMemberSel* ध्रुव memberselp = नूतन AstMemberSel{
                    varp->fileline(),
                    नूतन AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ग्रन्थिः* initAsgnp
                    = नूतन AstAssign{varp->fileline(), memberselp,
                                    नूतन AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_ग्रन्थिः::addNext(initsp, initAsgnp);
            }

            सचे (AstBasicDType* ध्रुव dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                सचे (dtypep->isEvent()) अनुवर्त;
            }

            सचे (varp->direction().isWritable()) {
                AstMemberSel* ध्रुव memberselp = नूतन AstMemberSel{
                    varp->fileline(),
                    नूतन AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ग्रन्थिः* writebackAsgnp = नूतन AstAssign{
                    varp->fileline(), नूतन AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_ग्रन्थिः::addNext(stmtp, writebackAsgnp);
            }
        }
        सचे (initsp) Ast_ग्रन्थिः::addNext(asgnp, initsp);

        सचे (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } अञ्ञथा {
            Ast_ग्रन्थिः::addNext(asgnp, static_cast<Ast_ग्रन्थिः*>(initp));
            stmtp = asgnp;
        }

        Ast_ग्रन्थिः::addNext(static_cast<Ast_ग्रन्थिः*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    तार्किक linked() ध्रुव { पटिनिवत्त m_instance.initialized() && m_instance.m_handlep->backp(); }

गुप्त:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ध्रुव newp = नूतन AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ग्रन्थिः_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        पटिनिवत्त नूतन AstAssign{
            m_procp->fileline(),
            नूतन AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    शून्य linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ध्रुव std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* ध्रुव beginp = नूतन AstBegin{forkp->fileline(), name, m_instance.m_handlep, सच्च};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        सचे (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { पटिनिवत्त "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(ध्रुव Ast_ग्रन्थिः* fromp) {
        पटिनिवत्त "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_ग्रन्थिः* getProcStmts() {
        Ast_ग्रन्थिः* stmtsp = nullptr;
        सचे (!m_procp) पटिनिवत्त nullptr;
        सचे (AstBegin* ध्रुव beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } अञ्ञथा सचे (Ast_ग्रन्थिः_FTask* ध्रुव taskp = VN_CAST(m_procp, ग्रन्थिः_FTask)) {
            stmtsp = taskp->stmtsp();
        } अञ्ञथा {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        पटिनिवत्त stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

वग्ग DynScopeVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;

    // STATE
    तार्किक m_inFunc = मिच्छा;  // True if in a function
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Module we are currently under
    Ast_ग्रन्थिः* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_ग्रन्थिः*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_ग्रन्थिः*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    तार्किक m_afterTimingControl = मिच्छा;  // A timing control might've be executed in the current
                                        // process
    size_t m_id = 0;  // Unique ID for a frame
    size_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_ग्रन्थिः* nodep) {
        auto frameIt = m_frames.find(nodep);
        सचे (frameIt == m_frames.end()) पटिनिवत्त nullptr;
        पटिनिवत्त frameIt->second;
    }

    ध्रुव ForkDynScopeFrame* frameOf(Ast_ग्रन्थिः* nodep) ध्रुव {
        auto frameIt = m_frames.find(nodep);
        सचे (frameIt == m_frames.end()) पटिनिवत्त nullptr;
        पटिनिवत्त frameIt->second;
    }

    ForkDynScopeFrame* प्रक्षिप_Dyn_Scope_Frame(Ast_ग्रन्थिः* procp) {
        ForkDynScopeFrame* ध्रुव framep
            = नूतन ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.प्रक्षिप_back(procp);
        पटिनिवत्त framep;
    }

    शून्य replaceWithMemberSel(AstVarRef* refp, ध्रुव ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* ध्रुव membersel = नूतन AstMemberSel{
            refp->fileline(), नूतन AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        सचे (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } अञ्ञथा {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
    }

    स्थिर तार्किक hasAsyncFork(Ast_ग्रन्थिः* nodep) {
        पटिनिवत्त nodep->exists([](AstFork* forkp) { पटिनिवत्त !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { पटिनिवत्त सच्च; });
    }

    शून्य bind_ग्रन्थिः_To_Dyn_Scope(Ast_ग्रन्थिः* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        सचे (r.second) m_frameOrder.प्रक्षिप_back(nodep);
    }
    शून्य bindInitIterate(Ast_ग्रन्थिः* stmtsp, ForkDynScopeFrame* framep) {
        अत्थाय (Ast_ग्रन्थिः* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            सचे (AstAssign* ध्रुव asgnp = VN_CAST(stmtp, Assign)) {
                bind_ग्रन्थिः_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } अञ्ञथा सचे (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } अञ्ञथा सचे (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } अञ्ञथा {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    तार्किक needsDynScope(ध्रुव AstVarRef* refp) ध्रुव {
        ध्रुव AstVar* ध्रुव varp = refp->varp();
        पटिनिवत्त
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_मानम्() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        सचे (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        सचे (hasAsyncFork(nodep)) प्रक्षिप_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    शून्य visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        सचे (hasAsyncFork(nodep)) प्रक्षिप_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    शून्य visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        सचे (!nodep->joinType().join()) ++m_forkDepth;

        ध्रुव तार्किक oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        सचे (nodep->declsp() || nodep->stmtsp()) framep = प्रक्षिप_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        अत्थाय (Ast_ग्रन्थिः* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ध्रुव varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            सचे (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                सचे (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_निवेशय(varp);
                bind_ग्रन्थिः_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        अत्थाय (Ast_ग्रन्थिः* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = मिच्छा;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        सचे (nodep->isTimingControl()) m_afterTimingControl = सच्च;
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        visit(static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = सच्च;
    }
    शून्य visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* ध्रुव framep = frameOf(m_procp);
        सचे (!framep) पटिनिवत्त;  // Cannot be legally referenced from a fork
        bind_ग्रन्थिः_To_Dyn_Scope(nodep, framep);
    }
    शून्य visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* ध्रुव framep = frameOf(nodep->varp());
        सचे (!framep) पटिनिवत्त;
        सचे (needsDynScope(nodep)) {
            तार्किक isEvent = मिच्छा;
            सचे (AstBasicDType* ध्रुव dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            सचे (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            सचे (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_निवेशय(nodep->varp());
        }
        bind_ग्रन्थिः_To_Dyn_Scope(nodep, framep);
    }
    शून्य visit(AstAssign* nodep) override {
        सचे (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_मानम्()) {
            nodep->lhsp()->user2(सच्च);
        }
        visit(static_cast<Ast_ग्रन्थिः_Stmt*>(nodep));
    }
    शून्य visit(AstAssignDly* nodep) override {
        सचे (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    पटिनिवत्त refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(सच्च);
            // Put it in a fork to prevent lifetime issues with the local
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            AstFork* ध्रुव forkp = नूतन AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(नूतन AstBegin{flp, "", nodep, मिच्छा});
            UINFO(9, "assign new fork " << forkp);
        } अञ्ञथा {
            visit(static_cast<Ast_ग्रन्थिः_Stmt*>(nodep));
        }
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        सचे (nodep->isTimingControl()) m_afterTimingControl = सच्च;
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        तार्किक typesAdded = मिच्छा;
        अत्थाय (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            सचे (!framep->instance().initialized()) अनुवर्त;
            सचे (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = सच्च;
            }
            सचे (AstVarRef* ध्रुव refp = VN_CAST(frameIt->first, VarRef)) {
                सचे (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        सचे (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        अत्थाय (auto ग्रन्थिः_frame : m_frames) frames.निवेशय(ग्रन्थिः_frame.second);
        अत्थाय (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

वग्ग ForkVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ध्रुव VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    size_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    तार्किक m_inFork = मिच्छा;  // Traversal in an async fork
    तार्किक m_hasNestedFork = मिच्छा;  // Branch being taskified contains a nested fork
    तार्किक m_inInitStmt = मिच्छा;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_ग्रन्थिः_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        अत्थाय (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            सचे (varp->name() == refp->name()) भङ्ग;
        }
        सचे (varp) पटिनिवत्त varp;

        // Create a local copy to capture
        File_पङ्क्तिः* ध्रुव flp = refp->fileline();
        varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::निवेशः);
        varp->funcLocal(सच्च);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_ग्रन्थिः::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ध्रुव argp = नूतन AstArg{flp, refp->name(), refp->clone_वृक्षः(मिच्छा)};
        m_capturedArgsp = Ast_ग्रन्थिः::addNext(m_capturedArgsp, argp);
        पटिनिवत्त varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    तार्किक taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = मिच्छा;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        सचे (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) पटिनिवत्त मिच्छा;

        // Create task holding the statement and repalce statement with call to that task
        File_पङ्क्तिः* ध्रुव flp = beginp->fileline();
        ध्रुव std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* ध्रुव taskp = नूतन AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        सचे (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_ग्रन्थिः::addNext(m_tasksp, taskp);
        सचे (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        सचे (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* ध्रुव callp = नूतन AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        अत्थाय (AstVar* ध्रुव localp : m_forkLocalsp) localp->funcLocal(सच्च);

        // We did wrap the body
        पटिनिवत्त सच्च;
    }
    स्थिर तार्किक isForkJoinNoneSentinelDelay(ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
        ध्रुव AstDelay* ध्रुव delayp = VN_CAST(nodep, Delay);
        सचे (!delayp) पटिनिवत्त मिच्छा;
        ध्रुव AstConst* ध्रुव constp = VN_CAST(delayp->lhsp(), Const);
        पटिनिवत्त constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    स्थिर तार्किक is_Disable_Queue_प्रक्षिप_Self_Prefix(Ast_ग्रन्थिः* nodep) {
        याव (AstJumpBlock* ध्रुव jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        पटिनिवत्त nodep && nodep->is_Disable_Queue_प्रक्षिप_Self_Stmt();
    }
    template <typename T_Owner>
    स्थिर तार्किक निवेशय_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* ध्रुव ownerp,
                                                          Ast_ग्रन्थिः* ध्रुव firstStmtp,
                                                          Ast_ग्रन्थिः* ध्रुव delayp) {
        Ast_ग्रन्थिः* निवेशय_Beforep = firstStmtp;
        याव (निवेशय_Beforep && निवेशय_Beforep->is_Disable_Queue_प्रक्षिप_Self_Stmt()) {
            निवेशय_Beforep = निवेशय_Beforep->nextp();
        }
        सचे (AstJumpBlock* ध्रुव jumpBlockp = VN_CAST(निवेशय_Beforep, JumpBlock)) {
            सचे (निवेशय_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                पटिनिवत्त सच्च;
            }
        }
        सचे (निवेशय_Beforep == firstStmtp) पटिनिवत्त मिच्छा;
        सचे (निवेशय_Beforep) {
            निवेशय_Beforep->addHereThisAsNext(delayp);
        } अञ्ञथा {
            ownerp->addStmtsp(delayp);
        }
        पटिनिवत्त सच्च;
    }
    स्थिर शून्य moveForkSentinelAfterDisableQueuePushes(AstBegin* ध्रुव beginp) {
        Ast_ग्रन्थिः* ध्रुव firstStmtp = beginp->stmtsp();
        सचे (!isForkJoinNoneSentinelDelay(firstStmtp)) पटिनिवत्त;
        Ast_ग्रन्थिः* ध्रुव afterSentinelp = firstStmtp->nextp();
        सचे (!is_Disable_Queue_प्रक्षिप_Self_Prefix(afterSentinelp)) पटिनिवत्त;

        Ast_ग्रन्थिः* ध्रुव delayp = firstStmtp->unlinkFrBack();
        ध्रुव तार्किक moved
            = निवेशय_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    स्थिर तार्किक forkIsDisableable(AstFork* ध्रुव nodep) {
        अत्थाय (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            सचे (is_Disable_Queue_प्रक्षिप_Self_Prefix(itemp->stmtsp())) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        सचे (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    शून्य visit(AstFork* nodep) override {
        m_hasNestedFork = सच्च;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        सचे ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_पङ्क्तिः* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            अत्थाय (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                सचे (!itemp->stmtsp()) अनुवर्त;
                AstDelay* ध्रुव delayp = नूतन AstDelay{
                    fl,
                    नूतन AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    मिच्छा};
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
        सचे (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            पटिनिवत्त;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = सच्च;
            अत्थाय (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                सचे (taskify(itemp)) wrappedp.प्रक्षिप_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        अत्थाय (AstBegin* ध्रुव beginp : wrappedp) iterateAndNextNull(beginp);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        सचे (m_inFork) m_forkLocalsp.निवेशय(nodep);
    }
    शून्य visit(AstVarRef* nodep) override {
        सचे (!m_inFork) पटिनिवत्त;
        AstVar* ध्रुव varp = nodep->varp();
        // Not sure why this is OK ...
        सचे (!varp->isFuncLocal() && varp->isClassMember()) पटिनिवत्त;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        सचे (varp->lifetime().isStatic()) पटिनिवत्त;
        // If this ref is to a variable that will move into the task, then nothing to do
        सचे (m_forkLocalsp.गणनम्(varp)) पटिनिवत्त;

        सचे (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_मानम्() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            पटिनिवत्त;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    शून्य visit(AstAssign* nodep) override {
        सचे (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_मानम्()) {
            nodep->lhsp()->user2(सच्च);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = सच्च;
        iterateChildren(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = सच्च;
        iterateChildren(nodep);
    }

    शून्य visit(AstThisRef* nodep) override {}
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_वृक्षः("fork_dynscope", 0, dump_वृक्षः_Either_Level() >= 3);
}

शून्य V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("fork_tasks", 0, dump_वृक्षः_Either_Level() >= 3);
}
