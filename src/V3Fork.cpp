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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Fork.h"

#समावेश "V3AstNodeExpr.h"
#समावेश "V3MemberMap.h"

#समावेश <limits>
#समावेश <set>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी ForkDynScopeInstance final {
खुला:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    bool initialized() स्थिर { वापस m_classp != nullptr; }
};

श्रेणी ForkDynScopeFrame final {
    // MEMBERS
    Ast_गाँठ_Module* स्थिर m_modp;  // Module to insert the scope into
    Ast_गाँठ* स्थिर m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    स्थिर माप_प्रकार m_class_id;  // Dynscope class ID
    स्थिर माप_प्रकार m_id;  // Dynscope ID

खुला:
    ForkDynScopeFrame(Ast_गाँठ_Module* modp, Ast_गाँठ* procp, माप_प्रकार class_id, माप_प्रकार id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = नया AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = नया AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = नया AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(false);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        वापस m_instance;
    }

    स्थिर ForkDynScopeInstance& instance() स्थिर { वापस m_instance; }
    व्योम capture_Var_जोड़ो(AstVar* varp) {
        स्वतः r = m_captures.emplace(varp);
        अगर (r.second) m_captureOrder.धकेलो_back(varp);
    }
    bool captured(AstVar* varp) { वापस m_captures.गिनती(varp) != 0; }
    Ast_गाँठ* procp() स्थिर { वापस m_procp; }

    व्योम populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        क्रम (AstVar* varp : m_captureOrder) {
            अगर (varp->direction().isAny()) {
                varp = varp->clone_वृक्ष(false);
                varp->direction(VDirection::NONE);
            } अन्यथा {
                varp->unlinkFrBack();
            }
            varp->funcLocal(false);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(false);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* स्थिर newp
            = नया AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(true);
        newp->classMethod(true);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    व्योम linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        अगर (AstFork* स्थिर forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            वापस;
        }

        Ast_गाँठ* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_Back_With_अग्र(&stmtpHandle);

        // Find node after last variable declaration
        Ast_गाँठ* initp = stmtp;
        जबतक (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* स्थिर newp = नया AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), गाँठ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_गाँठ* स्थिर asgnp = नया AstAssign{
            m_procp->fileline(),
            नया AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_गाँठ* initsp = nullptr;  // Arguments need to be copied
        क्रम (AstVar* varp : m_captureOrder) {
            अगर (!varp->direction().isAny()) जारी;

            अगर (varp->direction().isNonOutput()) {
                AstMemberSel* स्थिर memberselp = नया AstMemberSel{
                    varp->fileline(),
                    नया AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_गाँठ* initAsgnp
                    = नया AstAssign{varp->fileline(), memberselp,
                                    नया AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_गाँठ::add_अग्र(initsp, initAsgnp);
            }

            अगर (AstBasicDType* स्थिर dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                अगर (dtypep->isEvent()) जारी;
            }

            अगर (varp->direction().isWritable()) {
                AstMemberSel* स्थिर memberselp = नया AstMemberSel{
                    varp->fileline(),
                    नया AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_गाँठ* writebackAsgnp = नया AstAssign{
                    varp->fileline(), नया AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_गाँठ::add_अग्र(stmtp, writebackAsgnp);
            }
        }
        अगर (initsp) Ast_गाँठ::add_अग्र(asgnp, initsp);

        अगर (initp != stmtp) {
            initp->add_Here_This_As_अग्र(asgnp);
        } अन्यथा {
            Ast_गाँठ::add_अग्र(asgnp, static_cast<Ast_गाँठ*>(initp));
            stmtp = asgnp;
        }

        Ast_गाँठ::add_अग्र(static_cast<Ast_गाँठ*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    bool linked() स्थिर { वापस m_instance.initialized() && m_instance.m_handlep->backp(); }

निजी:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* स्थिर newp = नया AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), गाँठ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        वापस नया AstAssign{
            m_procp->fileline(),
            नया AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    व्योम linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        स्थिर std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* स्थिर beginp = नया AstBegin{forkp->fileline(), name, m_instance.m_handlep, true};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        अगर (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlink_Fr_Back_With_अग्र());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { वापस "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(स्थिर Ast_गाँठ* fromp) {
        वापस "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_गाँठ* getProcStmts() {
        Ast_गाँठ* stmtsp = nullptr;
        अगर (!m_procp) वापस nullptr;
        अगर (AstBegin* स्थिर beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } अन्यथा अगर (Ast_गाँठ_FTask* स्थिर taskp = VN_CAST(m_procp, गाँठ_FTask)) {
            stmtsp = taskp->stmtsp();
        } अन्यथा {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        वापस stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

श्रेणी DynScopeVisitor final : खुला VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;

    // STATE
    bool m_inFunc = false;  // True if in a function
    Ast_गाँठ_Module* m_modp = nullptr;  // Module we are currently under
    Ast_गाँठ* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_गाँठ*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_गाँठ*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    bool m_afterTimingControl = false;  // A timing control might've be executed in the current
                                        // process
    माप_प्रकार m_id = 0;  // Unique ID for a frame
    माप_प्रकार m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_गाँठ* nodep) {
        स्वतः frameIt = m_frames.find(nodep);
        अगर (frameIt == m_frames.end()) वापस nullptr;
        वापस frameIt->second;
    }

    स्थिर ForkDynScopeFrame* frameOf(Ast_गाँठ* nodep) स्थिर {
        स्वतः frameIt = m_frames.find(nodep);
        अगर (frameIt == m_frames.end()) वापस nullptr;
        वापस frameIt->second;
    }

    ForkDynScopeFrame* धकेलो_Dyn_Scope_Frame(Ast_गाँठ* procp) {
        ForkDynScopeFrame* स्थिर framep
            = नया ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        स्वतः r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.धकेलो_back(procp);
        वापस framep;
    }

    व्योम replaceWithMemberSel(AstVarRef* refp, स्थिर ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* स्थिर membersel = नया AstMemberSel{
            refp->fileline(), नया AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        अगर (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } अन्यथा {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
    }

    अटल bool hasAsyncFork(Ast_गाँठ* nodep) {
        वापस nodep->exists([](AstFork* forkp) { वापस !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { वापस true; });
    }

    व्योम bind_गाँठ_To_Dyn_Scope(Ast_गाँठ* nodep, ForkDynScopeFrame* framep) {
        स्वतः r = m_frames.emplace(nodep, framep);
        अगर (r.second) m_frameOrder.धकेलो_back(nodep);
    }
    व्योम bindInitIterate(Ast_गाँठ* stmtsp, ForkDynScopeFrame* framep) {
        क्रम (Ast_गाँठ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            अगर (AstAssign* स्थिर asgnp = VN_CAST(stmtp, Assign)) {
                bind_गाँठ_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } अन्यथा अगर (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } अन्यथा अगर (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } अन्यथा {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    bool needsDynScope(स्थिर AstVarRef* refp) स्थिर {
        स्थिर AstVar* स्थिर varp = refp->varp();
        वापस
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_मूल्य() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        अगर (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        अगर (hasAsyncFork(nodep)) धकेलो_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    व्योम visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        अगर (hasAsyncFork(nodep)) धकेलो_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    व्योम visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        अगर (!nodep->joinType().join()) ++m_forkDepth;

        स्थिर bool oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        अगर (nodep->declsp() || nodep->stmtsp()) framep = धकेलो_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        क्रम (Ast_गाँठ* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* स्थिर varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            अगर (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                अगर (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_जोड़ो(varp);
                bind_गाँठ_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        क्रम (Ast_गाँठ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = false;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        अगर (nodep->isTimingControl()) m_afterTimingControl = true;
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        visit(static_cast<Ast_गाँठ_व्यंजक*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = true;
    }
    व्योम visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* स्थिर framep = frameOf(m_procp);
        अगर (!framep) वापस;  // Cannot be legally referenced from a fork
        bind_गाँठ_To_Dyn_Scope(nodep, framep);
    }
    व्योम visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* स्थिर framep = frameOf(nodep->varp());
        अगर (!framep) वापस;
        अगर (needsDynScope(nodep)) {
            bool isEvent = false;
            अगर (AstBasicDType* स्थिर dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            अगर (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            अगर (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_जोड़ो(nodep->varp());
        }
        bind_गाँठ_To_Dyn_Scope(nodep, framep);
    }
    व्योम visit(AstAssign* nodep) override {
        अगर (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_मूल्य()) {
            nodep->lhsp()->user2(true);
        }
        visit(static_cast<Ast_गाँठ_Stmt*>(nodep));
    }
    व्योम visit(AstAssignDly* nodep) override {
        अगर (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    वापस refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(true);
            // Put it in a fork to prevent lifetime issues with the local
            File_पंक्ति* स्थिर flp = nodep->fileline();
            AstFork* स्थिर forkp = नया AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(नया AstBegin{flp, "", nodep, false});
            UINFO(9, "assign new fork " << forkp);
        } अन्यथा {
            visit(static_cast<Ast_गाँठ_Stmt*>(nodep));
        }
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (nodep->isTimingControl()) m_afterTimingControl = true;
        iterateChildren(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        bool typesAdded = false;
        क्रम (स्वतः orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            स्वतः frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            अगर (!framep->instance().initialized()) जारी;
            अगर (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = true;
            }
            अगर (AstVarRef* स्थिर refp = VN_CAST(frameIt->first, VarRef)) {
                अगर (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        अगर (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        क्रम (स्वतः गाँठ_frame : m_frames) frames.जोड़ो(गाँठ_frame.second);
        क्रम (स्वतः* framep : frames) मिटाओ framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

श्रेणी ForkVisitor final : खुला VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    स्थिर VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_गाँठ_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    माप_प्रकार m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    bool m_inFork = false;  // Traversal in an async fork
    bool m_hasNestedFork = false;  // Branch being taskified contains a nested fork
    bool m_inInitStmt = false;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        क्रम (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            अगर (varp->name() == refp->name()) अवरोध;
        }
        अगर (varp) वापस varp;

        // Create a local copy to capture
        File_पंक्ति* स्थिर flp = refp->fileline();
        varp = नया AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::निवेश);
        varp->funcLocal(true);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_गाँठ::add_अग्र(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* स्थिर argp = नया AstArg{flp, refp->name(), refp->clone_वृक्ष(false)};
        m_capturedArgsp = Ast_गाँठ::add_अग्र(m_capturedArgsp, argp);
        वापस varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    bool taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = false;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        अगर (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) वापस false;

        // Create task holding the statement and repalce statement with call to that task
        File_पंक्ति* स्थिर flp = beginp->fileline();
        स्थिर std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* स्थिर taskp = नया AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        अगर (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_गाँठ::add_अग्र(m_tasksp, taskp);
        अगर (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlink_Fr_Back_With_अग्र());
        अगर (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlink_Fr_Back_With_अग्र());
        AstTaskRef* स्थिर callp = नया AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        क्रम (AstVar* स्थिर localp : m_forkLocalsp) localp->funcLocal(true);

        // We did wrap the body
        वापस true;
    }
    अटल bool isForkJoinNoneSentinelDelay(स्थिर Ast_गाँठ* स्थिर nodep) {
        स्थिर AstDelay* स्थिर delayp = VN_CAST(nodep, Delay);
        अगर (!delayp) वापस false;
        स्थिर AstConst* स्थिर constp = VN_CAST(delayp->lhsp(), Const);
        वापस constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    अटल bool is_Disable_Queue_धकेलो_Self_Prefix(Ast_गाँठ* nodep) {
        जबतक (AstJumpBlock* स्थिर jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        वापस nodep && nodep->is_Disable_Queue_धकेलो_Self_Stmt();
    }
    ढाँचा <typename T_Owner>
    अटल bool जोड़ो_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* स्थिर ownerp,
                                                          Ast_गाँठ* स्थिर firstStmtp,
                                                          Ast_गाँठ* स्थिर delayp) {
        Ast_गाँठ* जोड़ो_Beforep = firstStmtp;
        जबतक (जोड़ो_Beforep && जोड़ो_Beforep->is_Disable_Queue_धकेलो_Self_Stmt()) {
            जोड़ो_Beforep = जोड़ो_Beforep->nextp();
        }
        अगर (AstJumpBlock* स्थिर jumpBlockp = VN_CAST(जोड़ो_Beforep, JumpBlock)) {
            अगर (जोड़ो_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                वापस true;
            }
        }
        अगर (जोड़ो_Beforep == firstStmtp) वापस false;
        अगर (जोड़ो_Beforep) {
            जोड़ो_Beforep->add_Here_This_As_अग्र(delayp);
        } अन्यथा {
            ownerp->addStmtsp(delayp);
        }
        वापस true;
    }
    अटल व्योम moveForkSentinelAfterDisableQueuePushes(AstBegin* स्थिर beginp) {
        Ast_गाँठ* स्थिर firstStmtp = beginp->stmtsp();
        अगर (!isForkJoinNoneSentinelDelay(firstStmtp)) वापस;
        Ast_गाँठ* स्थिर afterSentinelp = firstStmtp->nextp();
        अगर (!is_Disable_Queue_धकेलो_Self_Prefix(afterSentinelp)) वापस;

        Ast_गाँठ* स्थिर delayp = firstStmtp->unlinkFrBack();
        स्थिर bool moved
            = जोड़ो_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    अटल bool forkIsDisableable(AstFork* स्थिर nodep) {
        क्रम (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            अगर (is_Disable_Queue_धकेलो_Self_Prefix(itemp->stmtsp())) वापस true;
        }
        वापस false;
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        अगर (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    व्योम visit(AstFork* nodep) override {
        m_hasNestedFork = true;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        अगर ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_पंक्ति* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            क्रम (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                अगर (!itemp->stmtsp()) जारी;
                AstDelay* स्थिर delayp = नया AstDelay{
                    fl,
                    नया AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    false};
                itemp->prependStmtsp(delayp);
                moveForkSentinelAfterDisableQueuePushes(itemp);
            }
        }

        iterate_And_अग्र_Null(nodep->declsp());
        iterate_And_अग्र_Null(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        अगर (nodep->joinType().join()) {
            iterate_And_अग्र_Null(nodep->forksp());
            वापस;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = true;
            क्रम (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                अगर (taskify(itemp)) wrappedp.धकेलो_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        क्रम (AstBegin* स्थिर beginp : wrappedp) iterate_And_अग्र_Null(beginp);
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstVar* nodep) override {
        अगर (m_inFork) m_forkLocalsp.जोड़ो(nodep);
    }
    व्योम visit(AstVarRef* nodep) override {
        अगर (!m_inFork) वापस;
        AstVar* स्थिर varp = nodep->varp();
        // Not sure why this is OK ...
        अगर (!varp->isFuncLocal() && varp->isClassMember()) वापस;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        अगर (varp->lifetime().isStatic()) वापस;
        // If this ref is to a variable that will move into the task, then nothing to do
        अगर (m_forkLocalsp.गिनती(varp)) वापस;

        अगर (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_मूल्य() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            वापस;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    व्योम visit(AstAssign* nodep) override {
        अगर (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_मूल्य()) {
            nodep->lhsp()->user2(true);
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = true;
        iterateChildren(nodep);
    }
    व्योम visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = true;
        iterateChildren(nodep);
    }

    व्योम visit(AstThisRef* nodep) override {}
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = शेष;
};

//######################################################################
// Fork class functions

व्योम V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("fork_dynscope", 0, dump_वृक्ष_Either_Level() >= 3);
}

व्योम V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("fork_tasks", 0, dump_वृक्ष_Either_Level() >= 3);
}
