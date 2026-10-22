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
    AstClass* m_classp = शून्यसूचकः;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = शून्यसूचकः;  // RefDType for the above
    AstVar* m_handlep = शून्यसूचकः;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    तार्किक initialized() ध्रुव { प्रत्यागम m_classp != शून्यसूचकः; }
};

वर्ग ForkDynScopeFrame final {
    // MEMBERS
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module to insert the scope into
    Ast_ग्रन्थिः* ध्रुव m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_capture_क्रमः;  // Variables to be moved into the dynscope
    std::समुच्चयः<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ध्रुव आकार_t m_class_id;  // Dynscope class ID
    ध्रुव आकार_t m_id;  // Dynscope ID

सार्वजनिक:
    ForkDynScopeFrame(Ast_ग्रन्थिः_Module* modp, Ast_ग्रन्थिः* procp, आकार_t class_id, आकार_t id)
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
            = नूतन AstClassRefDType{m_procp->fileline(), m_instance.m_classp, शून्यसूचकः};
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = नूतन AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(असत्य);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        प्रत्यागम m_instance;
    }

    ध्रुव ForkDynScopeInstance& instance() ध्रुव { प्रत्यागम m_instance; }
    शून्य capture_Var_निवेशय(AstVar* varp) {
        स्वचालित r = m_captures.emplace(varp);
        यदि (r.second) m_capture_क्रमः.प्रक्षिप_पृष्ठभागः(varp);
    }
    तार्किक captured(AstVar* varp) { प्रत्यागम m_captures.गणनम्(varp) != 0; }
    Ast_ग्रन्थिः* procp() ध्रुव { प्रत्यागम m_procp; }

    शून्य populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        प्रति (AstVar* varp : m_capture_क्रमः) {
            यदि (varp->direction().isAny()) {
                varp = varp->clone_वृक्षः(असत्य);
                varp->direction(VDirection::NONE);
            } अन्यथा {
                varp->unlink_Fr_पृष्ठभागः();
            }
            varp->funcLocal(असत्य);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->प्रयुक्तम्_Loop_Idx(असत्य);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->सङ्कलय_Stmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ध्रुव newp
            = नूतन AstFunc{m_instance.m_classp->fileline(), "new", शून्यसूचकः, शून्यसूचकः};
        newp->isConstructor(सत्य);
        newp->class_विधानम्(सत्य);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->सङ्कलय_Stmtsp(newp);
    }

    शून्य linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        यदि (AstFork* ध्रुव forkp = VN_CAST(m_procp, Fork)) {
            link_Nodes_सम्बन्धि_Fork(memberMap, forkp);
            प्रत्यागम;
        }

        Ast_ग्रन्थिः* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_पृष्ठभागः_With_अग्र(&stmtpHandle);

        // Find node after last variable declaration
        Ast_ग्रन्थिः* initp = stmtp;
        यावत् (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ध्रुव newp = नूतन AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ग्रन्थिः_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_ग्रन्थिः* ध्रुव asgnp = नूतन Ast_नियोजय{
            m_procp->fileline(),
            नूतन AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_ग्रन्थिः* initsp = शून्यसूचकः;  // Arguments need to be copied
        प्रति (AstVar* varp : m_capture_क्रमः) {
            यदि (!varp->direction().isAny()) अनुवर्त;

            यदि (varp->direction().isNonOutput()) {
                AstMemberSel* ध्रुव memberselp = नूतन AstMemberSel{
                    varp->fileline(),
                    नूतन AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ग्रन्थिः* initAsgnp
                    = नूतन Ast_नियोजय{varp->fileline(), memberselp,
                                    नूतन AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(initsp, initAsgnp);
            }

            यदि (AstBasicDType* ध्रुव dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.समुच्चयः_Assigns_Events();
                यदि (dtypep->isEvent()) अनुवर्त;
            }

            यदि (varp->direction().isWritable()) {
                AstMemberSel* ध्रुव memberselp = नूतन AstMemberSel{
                    varp->fileline(),
                    नूतन AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ग्रन्थिः* writebackAsgnp = नूतन Ast_नियोजय{
                    varp->fileline(), नूतन AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_ग्रन्थिः::सङ्कलय_अग्र(stmtp, writebackAsgnp);
            }
        }
        यदि (initsp) Ast_ग्रन्थिः::सङ्कलय_अग्र(asgnp, initsp);

        यदि (initp != stmtp) {
            initp->सङ्कलय_अत्र_This_As_अग्र(asgnp);
        } अन्यथा {
            Ast_ग्रन्थिः::सङ्कलय_अग्र(asgnp, static_cast<Ast_ग्रन्थिः*>(initp));
            stmtp = asgnp;
        }

        Ast_ग्रन्थिः::सङ्कलय_अग्र(static_cast<Ast_ग्रन्थिः*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->सङ्कलय_Stmtsp(m_instance.m_classp);
    }

    तार्किक linked() ध्रुव { प्रत्यागम m_instance.initialized() && m_instance.m_handlep->backp(); }

गुप्त:
    Ast_नियोजय* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ध्रुव newp = नूतन AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ग्रन्थिः_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        प्रत्यागम नूतन Ast_नियोजय{
            m_procp->fileline(),
            नूतन AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    शून्य link_Nodes_सम्बन्धि_Fork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ध्रुव std::string name = "_Vwrapped_"  //
                                 + (forkp->name().रिक्तानि() ? "" : forkp->name() + "_")  //
                                 + std::गन्तव्यम्_string(m_id);
        AstBegin* ध्रुव beginp = नूतन AstBegin{forkp->fileline(), name, m_instance.m_handlep, सत्य};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->सङ्कलय_Stmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        यदि (forkp->stmtsp()) beginp->सङ्कलय_Stmtsp(forkp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
        // Put the Fork back at the end of the Begin
        beginp->सङ्कलय_Stmtsp(forkp);
        m_modp->सङ्कलय_Stmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { प्रत्यागम "__VDynScope_" + cvt_गन्तव्यम्_Str(m_class_id); }

    string generateDynScopeHandleName(ध्रुव Ast_ग्रन्थिः* fromp) {
        प्रत्यागम "__VDynScope_" + (!fromp->name().रिक्तानि() ? (fromp->name() + "_") : "ANON_")
               + cvt_गन्तव्यम्_Str(m_id);
    }

    Ast_ग्रन्थिः* getProcStmts() {
        Ast_ग्रन्थिः* stmtsp = शून्यसूचकः;
        यदि (!m_procp) प्रत्यागम शून्यसूचकः;
        यदि (AstBegin* ध्रुव beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } अन्यथा यदि (Ast_ग्रन्थिः_FTask* ध्रुव taskp = VN_CAST(m_procp, ग्रन्थिः_FTask)) {
            stmtsp = taskp->stmtsp();
        } अन्यथा {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        प्रत्यागम stmtsp;
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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // STATE
    तार्किक m_अन्तर्_Func = असत्य;  // True if in a function
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Module we are currently under
    Ast_ग्रन्थिः* m_procp = शून्यसूचकः;  // Function/task/block we are currently under
    std::deque<Ast_ग्रन्थिः*> m_frame_क्रमः;  // Ordered list of frames (for determinism)
    std::map<Ast_ग्रन्थिः*, ForkDynScopeFrame*> m_चौकटाः;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_fork_गभीरता = 0;  // Number of asynchronous forks we are currently under
    तार्किक m_afterTimingControl = असत्य;  // A timing control might've be executed in the current
                                        // process
    आकार_t m_id = 0;  // Unique ID for a frame
    आकार_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frame_सम्बन्धि(Ast_ग्रन्थिः* nodep) {
        स्वचालित frameIt = m_चौकटाः.find(nodep);
        यदि (frameIt == m_चौकटाः.अन्तः_2()) प्रत्यागम शून्यसूचकः;
        प्रत्यागम frameIt->second;
    }

    ध्रुव ForkDynScopeFrame* frame_सम्बन्धि(Ast_ग्रन्थिः* nodep) ध्रुव {
        स्वचालित frameIt = m_चौकटाः.find(nodep);
        यदि (frameIt == m_चौकटाः.अन्तः_2()) प्रत्यागम शून्यसूचकः;
        प्रत्यागम frameIt->second;
    }

    ForkDynScopeFrame* प्रक्षिप_Dyn_Scope_Frame(Ast_ग्रन्थिः* procp) {
        ForkDynScopeFrame* ध्रुव framep
            = नूतन ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        स्वचालित r = m_चौकटाः.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frame_क्रमः.प्रक्षिप_पृष्ठभागः(procp);
        प्रत्यागम framep;
    }

    शून्य replaceWithMemberSel(AstVarRef* refp, ध्रुव ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlink_Fr_पृष्ठभागः(&handle);
        AstMemberSel* ध्रुव membersel = नूतन AstMemberSel{
            refp->fileline(), नूतन AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        यदि (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } अन्यथा {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
    }

    स्थिर तार्किक hasAsyncFork(Ast_ग्रन्थिः* nodep) {
        प्रत्यागम nodep->exists([](AstFork* forkp) { प्रत्यागम !forkp->joinType().join(); })
               || nodep->exists([](Ast_नियोजय_Dly*) { प्रत्यागम सत्य; });
    }

    शून्य bind_ग्रन्थिः_गन्तव्यम्_Dyn_Scope(Ast_ग्रन्थिः* nodep, ForkDynScopeFrame* framep) {
        स्वचालित r = m_चौकटाः.emplace(nodep, framep);
        यदि (r.second) m_frame_क्रमः.प्रक्षिप_पृष्ठभागः(nodep);
    }
    शून्य bindInitIterate(Ast_ग्रन्थिः* stmtsp, ForkDynScopeFrame* framep) {
        प्रति (Ast_ग्रन्थिः* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            यदि (Ast_नियोजय* ध्रुव asgnp = VN_CAST(stmtp, नियोजय)) {
                bind_ग्रन्थिः_गन्तव्यम्_Dyn_Scope(asgnp->lhsp(), framep);
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

    तार्किक needsDynScope(ध्रुव AstVarRef* refp) ध्रुव {
        ध्रुव AstVar* ध्रुव varp = refp->varp();
        प्रत्यागम
            // Can this variable escape the scope
            ((m_fork_गभीरता > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_मानम्() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        यदि (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_अन्तर्_Func);
        m_अन्तर्_Func = VN_IS(nodep, Func);
        यदि (hasAsyncFork(nodep)) प्रक्षिप_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        यदि (hasAsyncFork(nodep)) प्रक्षिप_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstFork* nodep) override {
        VL_RESTORER(m_fork_गभीरता);
        यदि (!nodep->joinType().join()) ++m_fork_गभीरता;

        ध्रुव तार्किक oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = शून्यसूचकः;
        यदि (nodep->declsp() || nodep->stmtsp()) framep = प्रक्षिप_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        प्रति (Ast_ग्रन्थिः* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ध्रुव varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            यदि (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                यदि (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_निवेशय(varp);
                bind_ग्रन्थिः_गन्तव्यम्_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        प्रति (Ast_ग्रन्थिः* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = असत्य;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        यदि (nodep->isTimingControl()) m_afterTimingControl = सत्य;
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        दर्शय(static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = सत्य;
    }
    शून्य दर्शय(AstVar* nodep) override {
        nodep->user1(m_fork_गभीरता);
        ForkDynScopeFrame* ध्रुव framep = frame_सम्बन्धि(m_procp);
        यदि (!framep) प्रत्यागम;  // Cannot be legally referenced from a fork
        bind_ग्रन्थिः_गन्तव्यम्_Dyn_Scope(nodep, framep);
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        ForkDynScopeFrame* ध्रुव framep = frame_सम्बन्धि(nodep->varp());
        यदि (!framep) प्रत्यागम;
        यदि (needsDynScope(nodep)) {
            तार्किक isEvent = असत्य;
            यदि (AstBasicDType* ध्रुव dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.समुच्चयः_Assigns_Events();
                isEvent = dtypep->isEvent();
            }
            यदि (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_अन्तर्_Func ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            यदि (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_निवेशय(nodep->varp());
        }
        bind_ग्रन्थिः_गन्तव्यम्_Dyn_Scope(nodep, framep);
    }
    शून्य दर्शय(Ast_नियोजय* nodep) override {
        यदि (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_मानम्()) {
            nodep->lhsp()->user2(सत्य);
        }
        दर्शय(static_cast<Ast_ग्रन्थिः_Stmt*>(nodep));
    }
    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override {
        यदि (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    प्रत्यागम refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(सत्य);
            // Put it in a fork to prevent lifetime issues with the local
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            AstFork* ध्रुव forkp = नूतन AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->सङ्कलय_Forksp(नूतन AstBegin{flp, "", nodep, असत्य});
            UINFO(9, "assign new fork " << forkp);
        } अन्यथा {
            दर्शय(static_cast<Ast_ग्रन्थिः_Stmt*>(nodep));
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (nodep->isTimingControl()) m_afterTimingControl = सत्य;
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        दर्शय(nodep);

        // Commit changes to AST
        तार्किक typesAdded = असत्य;
        प्रति (स्वचालित orderp : m_frame_क्रमः) {
            UINFO(9, "Frame commit " << orderp);
            स्वचालित frameIt = m_चौकटाः.find(orderp);
            UASSERT_OBJ(frameIt != m_चौकटाः.अन्तः_2(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            यदि (!framep->instance().initialized()) अनुवर्त;
            यदि (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = सत्य;
            }
            यदि (AstVarRef* ध्रुव refp = VN_CAST(frameIt->first, VarRef)) {
                यदि (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        यदि (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::समुच्चयः<ForkDynScopeFrame*> चौकटाः;
        प्रति (स्वचालित ग्रन्थिः_frame : m_चौकटाः) चौकटाः.निवेशय(ग्रन्थिः_frame.second);
        प्रति (स्वचालित* framep : चौकटाः) विलोपय framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

वर्ग ForkVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // STATE - for current AstNodeModule
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Class/module we are currently under
    AstTask* m_tasksp = शून्यसूचकः;  // Tasks exrtracted under current module
    आकार_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    तार्किक m_अन्तर्_Fork = असत्य;  // Traversal in an async fork
    तार्किक m_hasNestedFork = असत्य;  // Branch being taskified contains a nested fork
    तार्किक m_अन्तर्_Init_Stmt = असत्य;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Enclosing function/task
    std::समुच्चयः<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = शून्यसूचकः;  // Local copies of captured variables
    AstArg* m_capturedArgsp = शून्यसूचकः;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = शून्यसूचकः;
        प्रति (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            यदि (varp->name() == refp->name()) भङ्ग;
        }
        यदि (varp) प्रत्यागम varp;

        // Create a local copy to capture
        File_पङ्क्तिः* ध्रुव flp = refp->fileline();
        varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::निवेशः);
        varp->funcLocal(सत्य);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ध्रुव argp = नूतन AstArg{flp, refp->name(), refp->clone_वृक्षः(असत्य)};
        m_capturedArgsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_capturedArgsp, argp);
        प्रत्यागम varp;
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
        m_capturedVarsp = शून्यसूचकः;
        m_capturedArgsp = शून्यसूचकः;
        iterate(beginp);

        // No need to do it if no variabels are captured
        यदि (m_forkLocalsp.रिक्तानि() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) प्रत्यागम असत्य;

        // Create task holding the statement and repalce statement with call to that task
        File_पङ्क्तिः* ध्रुव flp = beginp->fileline();
        ध्रुव std::string name = "__VforkTask_" + std::गन्तव्यम्_string(m_nForkTasks++);
        AstTask* ध्रुव taskp = नूतन AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        यदि (m_hasNestedFork) {
            taskp->class_विधानम्(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_tasksp, taskp);
        यदि (beginp->declsp()) taskp->सङ्कलय_Stmtsp(beginp->declsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
        यदि (beginp->stmtsp()) taskp->सङ्कलय_Stmtsp(beginp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
        AstTaskRef* ध्रुव callp = नूतन AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->सङ्कलय_Stmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        प्रति (AstVar* ध्रुव localp : m_forkLocalsp) localp->funcLocal(सत्य);

        // We did wrap the body
        प्रत्यागम सत्य;
    }
    स्थिर तार्किक isForkJoinNoneSentinelDelay(ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
        ध्रुव AstDelay* ध्रुव delayp = VN_CAST(nodep, Delay);
        यदि (!delayp) प्रत्यागम असत्य;
        ध्रुव AstConst* ध्रुव constp = VN_CAST(delayp->lhsp(), Const);
        प्रत्यागम constp && (constp->गन्तव्यम्_UQuad() == std::numeric_limits<uint64_t>::max());
    }
    स्थिर तार्किक is_Disable_Queue_प्रक्षिप_Self_उपसर्गः(Ast_ग्रन्थिः* nodep) {
        यावत् (Ast_Jump_खण्डम्* ध्रुव jumpBlockp = VN_CAST(nodep, Jump_खण्डम्)) {
            nodep = jumpBlockp->stmtsp();
        }
        प्रत्यागम nodep && nodep->is_Disable_Queue_प्रक्षिप_Self_Stmt();
    }
    प्रतिरूपम् <प्रकारनाम T_Owner>
    स्थिर तार्किक निवेशय_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* ध्रुव ownerp,
                                                          Ast_ग्रन्थिः* ध्रुव firstStmtp,
                                                          Ast_ग्रन्थिः* ध्रुव delayp) {
        Ast_ग्रन्थिः* निवेशय_Beforep = firstStmtp;
        यावत् (निवेशय_Beforep && निवेशय_Beforep->is_Disable_Queue_प्रक्षिप_Self_Stmt()) {
            निवेशय_Beforep = निवेशय_Beforep->nextp();
        }
        यदि (Ast_Jump_खण्डम्* ध्रुव jumpBlockp = VN_CAST(निवेशय_Beforep, Jump_खण्डम्)) {
            यदि (निवेशय_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                प्रत्यागम सत्य;
            }
        }
        यदि (निवेशय_Beforep == firstStmtp) प्रत्यागम असत्य;
        यदि (निवेशय_Beforep) {
            निवेशय_Beforep->सङ्कलय_अत्र_This_As_अग्र(delayp);
        } अन्यथा {
            ownerp->सङ्कलय_Stmtsp(delayp);
        }
        प्रत्यागम सत्य;
    }
    स्थिर शून्य moveForkSentinelAfterDisableQueuePushes(AstBegin* ध्रुव beginp) {
        Ast_ग्रन्थिः* ध्रुव firstStmtp = beginp->stmtsp();
        यदि (!isForkJoinNoneSentinelDelay(firstStmtp)) प्रत्यागम;
        Ast_ग्रन्थिः* ध्रुव afterSentinelp = firstStmtp->nextp();
        यदि (!is_Disable_Queue_प्रक्षिप_Self_उपसर्गः(afterSentinelp)) प्रत्यागम;

        Ast_ग्रन्थिः* ध्रुव delayp = firstStmtp->unlink_Fr_पृष्ठभागः();
        ध्रुव तार्किक moved
            = निवेशय_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    स्थिर तार्किक forkIsDisableable(AstFork* ध्रुव nodep) {
        प्रति (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            यदि (is_Disable_Queue_प्रक्षिप_Self_उपसर्गः(itemp->stmtsp())) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = शून्यसूचकः;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        यदि (m_tasksp) nodep->सङ्कलय_Stmtsp(m_tasksp);
    }

    शून्य दर्शय(AstFork* nodep) override {
        m_hasNestedFork = सत्य;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        यदि ((nodep->joinType().joinNone() && !nodep->तात्कालिकम्_Start())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_पङ्क्तिः* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            प्रति (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                यदि (!itemp->stmtsp()) अनुवर्त;
                AstDelay* ध्रुव delayp = नूतन AstDelay{
                    fl,
                    नूतन AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    असत्य};
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
        यदि (nodep->joinType().join()) {
            iterate_And_अग्र_Null(nodep->forksp());
            प्रत्यागम;
        }

        std::सदिशम्<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_अन्तर्_Fork);
            m_अन्तर्_Fork = सत्य;
            प्रति (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                यदि (taskify(itemp)) wrappedp.प्रक्षिप_पृष्ठभागः(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        प्रति (AstBegin* ध्रुव beginp : wrappedp) iterate_And_अग्र_Null(beginp);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstVar* nodep) override {
        यदि (m_अन्तर्_Fork) m_forkLocalsp.निवेशय(nodep);
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (!m_अन्तर्_Fork) प्रत्यागम;
        AstVar* ध्रुव varp = nodep->varp();
        // Not sure why this is OK ...
        यदि (!varp->isFuncLocal() && varp->isClassMember()) प्रत्यागम;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        यदि (varp->lifetime().isStatic()) प्रत्यागम;
        // If this ref is to a variable that will move into the task, then nothing to do
        यदि (m_forkLocalsp.गणनम्(varp)) प्रत्यागम;

        यदि (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_मानम्() || nodep->user2())
            && !m_अन्तर्_Init_Stmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warn_अधिकम्()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            प्रत्यागम;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    शून्य दर्शय(Ast_नियोजय* nodep) override {
        यदि (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_मानम्()) {
            nodep->lhsp()->user2(सत्य);
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_अन्तर्_Init_Stmt);
        m_अन्तर्_Init_Stmt = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_अन्तर्_Init_Stmt);
        m_अन्तर्_Init_Stmt = सत्य;
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstThisRef* nodep) override {}
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट ForkVisitor(AstNetlist* nodep) { दर्शय(nodep); }
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
