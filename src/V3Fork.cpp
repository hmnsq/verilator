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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Fork.h"

#અન્તર્ભાવ "V3AstNodeExpr.h"
#અન્તર્ભાવ "V3MemberMap.h"

#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ ForkDynScopeInstance final {
જાહેર:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    તાર્કિક initialized() અચલ { પાછા m_classp != nullptr; }
};

વર્ગ ForkDynScopeFrame final {
    // MEMBERS
    Ast_ગાંઠ_Module* અચલ m_modp;  // Module to insert the scope into
    Ast_ગાંઠ* અચલ m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    અચલ size_t m_class_id;  // Dynscope class ID
    અચલ size_t m_id;  // Dynscope ID

જાહેર:
    ForkDynScopeFrame(Ast_ગાંઠ_Module* modp, Ast_ગાંઠ* procp, size_t class_id, size_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = નવું AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = નવું AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = નવું AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(ખોટું);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        પાછા m_instance;
    }

    અચલ ForkDynScopeInstance& instance() અચલ { પાછા m_instance; }
    શૂન્ય capture_Var_ઉમેરો(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        જો (r.second) m_captureOrder.ધકેલો_back(varp);
    }
    તાર્કિક captured(AstVar* varp) { પાછા m_captures.ગણતરી(varp) != 0; }
    Ast_ગાંઠ* procp() અચલ { પાછા m_procp; }

    શૂન્ય populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        માટે (AstVar* varp : m_captureOrder) {
            જો (varp->direction().isAny()) {
                varp = varp->clone_વૃક્ષ(ખોટું);
                varp->direction(VDirection::NONE);
            } નહીંતર {
                varp->unlinkFrBack();
            }
            varp->funcLocal(ખોટું);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(ખોટું);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* અચલ newp
            = નવું AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(સાચું);
        newp->classMethod(સાચું);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    શૂન્ય linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        જો (AstFork* અચલ forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            પાછા;
        }

        Ast_ગાંઠ* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_ગાંઠ* initp = stmtp;
        જ્યારે (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* અચલ newp = નવું AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ગાંઠ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_ગાંઠ* અચલ asgnp = નવું AstAssign{
            m_procp->fileline(),
            નવું AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_ગાંઠ* initsp = nullptr;  // Arguments need to be copied
        માટે (AstVar* varp : m_captureOrder) {
            જો (!varp->direction().isAny()) ચાલુ;

            જો (varp->direction().isNonOutput()) {
                AstMemberSel* અચલ memberselp = નવું AstMemberSel{
                    varp->fileline(),
                    નવું AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ગાંઠ* initAsgnp
                    = નવું AstAssign{varp->fileline(), memberselp,
                                    નવું AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_ગાંઠ::addNext(initsp, initAsgnp);
            }

            જો (AstBasicDType* અચલ dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                જો (dtypep->isEvent()) ચાલુ;
            }

            જો (varp->direction().isWritable()) {
                AstMemberSel* અચલ memberselp = નવું AstMemberSel{
                    varp->fileline(),
                    નવું AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ગાંઠ* writebackAsgnp = નવું AstAssign{
                    varp->fileline(), નવું AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_ગાંઠ::addNext(stmtp, writebackAsgnp);
            }
        }
        જો (initsp) Ast_ગાંઠ::addNext(asgnp, initsp);

        જો (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } નહીંતર {
            Ast_ગાંઠ::addNext(asgnp, static_cast<Ast_ગાંઠ*>(initp));
            stmtp = asgnp;
        }

        Ast_ગાંઠ::addNext(static_cast<Ast_ગાંઠ*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    તાર્કિક linked() અચલ { પાછા m_instance.initialized() && m_instance.m_handlep->backp(); }

ખાનગી:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* અચલ newp = નવું AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ગાંઠ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        પાછા નવું AstAssign{
            m_procp->fileline(),
            નવું AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    શૂન્ય linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        અચલ std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* અચલ beginp = નવું AstBegin{forkp->fileline(), name, m_instance.m_handlep, સાચું};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        જો (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { પાછા "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(અચલ Ast_ગાંઠ* fromp) {
        પાછા "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_ગાંઠ* getProcStmts() {
        Ast_ગાંઠ* stmtsp = nullptr;
        જો (!m_procp) પાછા nullptr;
        જો (AstBegin* અચલ beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } નહીંતર જો (Ast_ગાંઠ_FTask* અચલ taskp = VN_CAST(m_procp, ગાંઠ_FTask)) {
            stmtsp = taskp->stmtsp();
        } નહીંતર {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        પાછા stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

વર્ગ DynScopeVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;

    // STATE
    તાર્કિક m_inFunc = ખોટું;  // True if in a function
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Module we are currently under
    Ast_ગાંઠ* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_ગાંઠ*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_ગાંઠ*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    તાર્કિક m_afterTimingControl = ખોટું;  // A timing control might've be executed in the current
                                        // process
    size_t m_id = 0;  // Unique ID for a frame
    size_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_ગાંઠ* nodep) {
        auto frameIt = m_frames.find(nodep);
        જો (frameIt == m_frames.end()) પાછા nullptr;
        પાછા frameIt->second;
    }

    અચલ ForkDynScopeFrame* frameOf(Ast_ગાંઠ* nodep) અચલ {
        auto frameIt = m_frames.find(nodep);
        જો (frameIt == m_frames.end()) પાછા nullptr;
        પાછા frameIt->second;
    }

    ForkDynScopeFrame* ધકેલો_Dyn_Scope_Frame(Ast_ગાંઠ* procp) {
        ForkDynScopeFrame* અચલ framep
            = નવું ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.ધકેલો_back(procp);
        પાછા framep;
    }

    શૂન્ય replaceWithMemberSel(AstVarRef* refp, અચલ ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* અચલ membersel = નવું AstMemberSel{
            refp->fileline(), નવું AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        જો (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } નહીંતર {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
    }

    સ્થિર તાર્કિક hasAsyncFork(Ast_ગાંઠ* nodep) {
        પાછા nodep->exists([](AstFork* forkp) { પાછા !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { પાછા સાચું; });
    }

    શૂન્ય bind_ગાંઠ_To_Dyn_Scope(Ast_ગાંઠ* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        જો (r.second) m_frameOrder.ધકેલો_back(nodep);
    }
    શૂન્ય bindInitIterate(Ast_ગાંઠ* stmtsp, ForkDynScopeFrame* framep) {
        માટે (Ast_ગાંઠ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            જો (AstAssign* અચલ asgnp = VN_CAST(stmtp, Assign)) {
                bind_ગાંઠ_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } નહીંતર જો (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } નહીંતર જો (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } નહીંતર {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    તાર્કિક needsDynScope(અચલ AstVarRef* refp) અચલ {
        અચલ AstVar* અચલ varp = refp->varp();
        પાછા
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_કિંમત() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        જો (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        જો (hasAsyncFork(nodep)) ધકેલો_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        જો (hasAsyncFork(nodep)) ધકેલો_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        જો (!nodep->joinType().join()) ++m_forkDepth;

        અચલ તાર્કિક oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        જો (nodep->declsp() || nodep->stmtsp()) framep = ધકેલો_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        માટે (Ast_ગાંઠ* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* અચલ varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            જો (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                જો (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_ઉમેરો(varp);
                bind_ગાંઠ_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        માટે (Ast_ગાંઠ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = ખોટું;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        જો (nodep->isTimingControl()) m_afterTimingControl = સાચું;
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        visit(static_cast<Ast_ગાંઠ_પદાવલિ*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = સાચું;
    }
    શૂન્ય visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* અચલ framep = frameOf(m_procp);
        જો (!framep) પાછા;  // Cannot be legally referenced from a fork
        bind_ગાંઠ_To_Dyn_Scope(nodep, framep);
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* અચલ framep = frameOf(nodep->varp());
        જો (!framep) પાછા;
        જો (needsDynScope(nodep)) {
            તાર્કિક isEvent = ખોટું;
            જો (AstBasicDType* અચલ dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            જો (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            જો (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_ઉમેરો(nodep->varp());
        }
        bind_ગાંઠ_To_Dyn_Scope(nodep, framep);
    }
    શૂન્ય visit(AstAssign* nodep) override {
        જો (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_કિંમત()) {
            nodep->lhsp()->user2(સાચું);
        }
        visit(static_cast<Ast_ગાંઠ_Stmt*>(nodep));
    }
    શૂન્ય visit(AstAssignDly* nodep) override {
        જો (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    પાછા refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(સાચું);
            // Put it in a fork to prevent lifetime issues with the local
            File_લીટી* અચલ flp = nodep->fileline();
            AstFork* અચલ forkp = નવું AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(નવું AstBegin{flp, "", nodep, ખોટું});
            UINFO(9, "assign new fork " << forkp);
        } નહીંતર {
            visit(static_cast<Ast_ગાંઠ_Stmt*>(nodep));
        }
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (nodep->isTimingControl()) m_afterTimingControl = સાચું;
        iterateChildren(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        તાર્કિક typesAdded = ખોટું;
        માટે (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            જો (!framep->instance().initialized()) ચાલુ;
            જો (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = સાચું;
            }
            જો (AstVarRef* અચલ refp = VN_CAST(frameIt->first, VarRef)) {
                જો (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        જો (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        માટે (auto ગાંઠ_frame : m_frames) frames.ઉમેરો(ગાંઠ_frame.second);
        માટે (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

વર્ગ ForkVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    અચલ VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    size_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    તાર્કિક m_inFork = ખોટું;  // Traversal in an async fork
    તાર્કિક m_hasNestedFork = ખોટું;  // Branch being taskified contains a nested fork
    તાર્કિક m_inInitStmt = ખોટું;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        માટે (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            જો (varp->name() == refp->name()) તોડો;
        }
        જો (varp) પાછા varp;

        // Create a local copy to capture
        File_લીટી* અચલ flp = refp->fileline();
        varp = નવું AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::આગત);
        varp->funcLocal(સાચું);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_ગાંઠ::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* અચલ argp = નવું AstArg{flp, refp->name(), refp->clone_વૃક્ષ(ખોટું)};
        m_capturedArgsp = Ast_ગાંઠ::addNext(m_capturedArgsp, argp);
        પાછા varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    તાર્કિક taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = ખોટું;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        જો (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) પાછા ખોટું;

        // Create task holding the statement and repalce statement with call to that task
        File_લીટી* અચલ flp = beginp->fileline();
        અચલ std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* અચલ taskp = નવું AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        જો (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_ગાંઠ::addNext(m_tasksp, taskp);
        જો (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        જો (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* અચલ callp = નવું AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        માટે (AstVar* અચલ localp : m_forkLocalsp) localp->funcLocal(સાચું);

        // We did wrap the body
        પાછા સાચું;
    }
    સ્થિર તાર્કિક isForkJoinNoneSentinelDelay(અચલ Ast_ગાંઠ* અચલ nodep) {
        અચલ AstDelay* અચલ delayp = VN_CAST(nodep, Delay);
        જો (!delayp) પાછા ખોટું;
        અચલ AstConst* અચલ constp = VN_CAST(delayp->lhsp(), Const);
        પાછા constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    સ્થિર તાર્કિક is_Disable_Queue_ધકેલો_Self_Prefix(Ast_ગાંઠ* nodep) {
        જ્યારે (AstJumpBlock* અચલ jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        પાછા nodep && nodep->is_Disable_Queue_ધકેલો_Self_Stmt();
    }
    template <typename T_Owner>
    સ્થિર તાર્કિક ઉમેરો_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* અચલ ownerp,
                                                          Ast_ગાંઠ* અચલ firstStmtp,
                                                          Ast_ગાંઠ* અચલ delayp) {
        Ast_ગાંઠ* ઉમેરો_Beforep = firstStmtp;
        જ્યારે (ઉમેરો_Beforep && ઉમેરો_Beforep->is_Disable_Queue_ધકેલો_Self_Stmt()) {
            ઉમેરો_Beforep = ઉમેરો_Beforep->nextp();
        }
        જો (AstJumpBlock* અચલ jumpBlockp = VN_CAST(ઉમેરો_Beforep, JumpBlock)) {
            જો (ઉમેરો_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                પાછા સાચું;
            }
        }
        જો (ઉમેરો_Beforep == firstStmtp) પાછા ખોટું;
        જો (ઉમેરો_Beforep) {
            ઉમેરો_Beforep->addHereThisAsNext(delayp);
        } નહીંતર {
            ownerp->addStmtsp(delayp);
        }
        પાછા સાચું;
    }
    સ્થિર શૂન્ય moveForkSentinelAfterDisableQueuePushes(AstBegin* અચલ beginp) {
        Ast_ગાંઠ* અચલ firstStmtp = beginp->stmtsp();
        જો (!isForkJoinNoneSentinelDelay(firstStmtp)) પાછા;
        Ast_ગાંઠ* અચલ afterSentinelp = firstStmtp->nextp();
        જો (!is_Disable_Queue_ધકેલો_Self_Prefix(afterSentinelp)) પાછા;

        Ast_ગાંઠ* અચલ delayp = firstStmtp->unlinkFrBack();
        અચલ તાર્કિક moved
            = ઉમેરો_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    સ્થિર તાર્કિક forkIsDisableable(AstFork* અચલ nodep) {
        માટે (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            જો (is_Disable_Queue_ધકેલો_Self_Prefix(itemp->stmtsp())) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        જો (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    શૂન્ય visit(AstFork* nodep) override {
        m_hasNestedFork = સાચું;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        જો ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_લીટી* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            માટે (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                જો (!itemp->stmtsp()) ચાલુ;
                AstDelay* અચલ delayp = નવું AstDelay{
                    fl,
                    નવું AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    ખોટું};
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
        જો (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            પાછા;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = સાચું;
            માટે (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                જો (taskify(itemp)) wrappedp.ધકેલો_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        માટે (AstBegin* અચલ beginp : wrappedp) iterateAndNextNull(beginp);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVar* nodep) override {
        જો (m_inFork) m_forkLocalsp.ઉમેરો(nodep);
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        જો (!m_inFork) પાછા;
        AstVar* અચલ varp = nodep->varp();
        // Not sure why this is OK ...
        જો (!varp->isFuncLocal() && varp->isClassMember()) પાછા;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        જો (varp->lifetime().isStatic()) પાછા;
        // If this ref is to a variable that will move into the task, then nothing to do
        જો (m_forkLocalsp.ગણતરી(varp)) પાછા;

        જો (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_કિંમત() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            પાછા;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    શૂન્ય visit(AstAssign* nodep) override {
        જો (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_કિંમત()) {
            nodep->lhsp()->user2(સાચું);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = સાચું;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstThisRef* nodep) override {}
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = મૂળભૂત;
};

//######################################################################
// Fork class functions

શૂન્ય V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_વૃક્ષ("fork_dynscope", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_વૃક્ષ("fork_tasks", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
