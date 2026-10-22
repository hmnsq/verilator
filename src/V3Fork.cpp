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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Fork.h"

#അന്തര്ഭാവ "V3AstNodeExpr.h"
#അന്തര്ഭാവ "V3MemberMap.h"

#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം ForkDynScopeInstance final {
പൊതു:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    തർക്കം initialized() സ്ഥിരം { തിരികെ m_classp != nullptr; }
};

വർഗ്ഗം ForkDynScopeFrame final {
    // MEMBERS
    Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // Module to insert the scope into
    Ast_കണ്ണി* സ്ഥിരം m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    സ്ഥിരം size_t m_class_id;  // Dynscope class ID
    സ്ഥിരം size_t m_id;  // Dynscope ID

പൊതു:
    ForkDynScopeFrame(Ast_കണ്ണി_Module* modp, Ast_കണ്ണി* procp, size_t class_id, size_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = പുതിയ AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = പുതിയ AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = പുതിയ AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(അസത്യം);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        തിരികെ m_instance;
    }

    സ്ഥിരം ForkDynScopeInstance& instance() സ്ഥിരം { തിരികെ m_instance; }
    ശൂന്യം capture_Var_തിരുകുക(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        എങ്കിൽ (r.second) m_captureOrder.തള്ളുക_back(varp);
    }
    തർക്കം captured(AstVar* varp) { തിരികെ m_captures.എണ്ണം(varp) != 0; }
    Ast_കണ്ണി* procp() സ്ഥിരം { തിരികെ m_procp; }

    ശൂന്യം populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        വേണ്ടി (AstVar* varp : m_captureOrder) {
            എങ്കിൽ (varp->direction().isAny()) {
                varp = varp->clone_വൃക്ഷം(അസത്യം);
                varp->direction(VDirection::NONE);
            } അല്ലെങ്കിൽ {
                varp->unlinkFrBack();
            }
            varp->funcLocal(അസത്യം);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(അസത്യം);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* സ്ഥിരം newp
            = പുതിയ AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(സത്യം);
        newp->classMethod(സത്യം);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    ശൂന്യം linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        എങ്കിൽ (AstFork* സ്ഥിരം forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            തിരികെ;
        }

        Ast_കണ്ണി* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_കണ്ണി* initp = stmtp;
        ആകുമ്പോൾ (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* സ്ഥിരം newp = പുതിയ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), കണ്ണി_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_കണ്ണി* സ്ഥിരം asgnp = പുതിയ AstAssign{
            m_procp->fileline(),
            പുതിയ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_കണ്ണി* initsp = nullptr;  // Arguments need to be copied
        വേണ്ടി (AstVar* varp : m_captureOrder) {
            എങ്കിൽ (!varp->direction().isAny()) തുടരുക;

            എങ്കിൽ (varp->direction().isNonOutput()) {
                AstMemberSel* സ്ഥിരം memberselp = പുതിയ AstMemberSel{
                    varp->fileline(),
                    പുതിയ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_കണ്ണി* initAsgnp
                    = പുതിയ AstAssign{varp->fileline(), memberselp,
                                    പുതിയ AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_കണ്ണി::addNext(initsp, initAsgnp);
            }

            എങ്കിൽ (AstBasicDType* സ്ഥിരം dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                എങ്കിൽ (dtypep->isEvent()) തുടരുക;
            }

            എങ്കിൽ (varp->direction().isWritable()) {
                AstMemberSel* സ്ഥിരം memberselp = പുതിയ AstMemberSel{
                    varp->fileline(),
                    പുതിയ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_കണ്ണി* writebackAsgnp = പുതിയ AstAssign{
                    varp->fileline(), പുതിയ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_കണ്ണി::addNext(stmtp, writebackAsgnp);
            }
        }
        എങ്കിൽ (initsp) Ast_കണ്ണി::addNext(asgnp, initsp);

        എങ്കിൽ (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി::addNext(asgnp, static_cast<Ast_കണ്ണി*>(initp));
            stmtp = asgnp;
        }

        Ast_കണ്ണി::addNext(static_cast<Ast_കണ്ണി*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    തർക്കം linked() സ്ഥിരം { തിരികെ m_instance.initialized() && m_instance.m_handlep->backp(); }

സ്വകാര്യ:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* സ്ഥിരം newp = പുതിയ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), കണ്ണി_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        തിരികെ പുതിയ AstAssign{
            m_procp->fileline(),
            പുതിയ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    ശൂന്യം linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        സ്ഥിരം std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* സ്ഥിരം beginp = പുതിയ AstBegin{forkp->fileline(), name, m_instance.m_handlep, സത്യം};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        എങ്കിൽ (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { തിരികെ "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(സ്ഥിരം Ast_കണ്ണി* fromp) {
        തിരികെ "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_കണ്ണി* getProcStmts() {
        Ast_കണ്ണി* stmtsp = nullptr;
        എങ്കിൽ (!m_procp) തിരികെ nullptr;
        എങ്കിൽ (AstBegin* സ്ഥിരം beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം taskp = VN_CAST(m_procp, കണ്ണി_FTask)) {
            stmtsp = taskp->stmtsp();
        } അല്ലെങ്കിൽ {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        തിരികെ stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

വർഗ്ഗം DynScopeVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;

    // STATE
    തർക്കം m_inFunc = അസത്യം;  // True if in a function
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Module we are currently under
    Ast_കണ്ണി* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_കണ്ണി*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_കണ്ണി*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    തർക്കം m_afterTimingControl = അസത്യം;  // A timing control might've be executed in the current
                                        // process
    size_t m_id = 0;  // Unique ID for a frame
    size_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_കണ്ണി* nodep) {
        auto frameIt = m_frames.find(nodep);
        എങ്കിൽ (frameIt == m_frames.end()) തിരികെ nullptr;
        തിരികെ frameIt->second;
    }

    സ്ഥിരം ForkDynScopeFrame* frameOf(Ast_കണ്ണി* nodep) സ്ഥിരം {
        auto frameIt = m_frames.find(nodep);
        എങ്കിൽ (frameIt == m_frames.end()) തിരികെ nullptr;
        തിരികെ frameIt->second;
    }

    ForkDynScopeFrame* തള്ളുക_Dyn_Scope_Frame(Ast_കണ്ണി* procp) {
        ForkDynScopeFrame* സ്ഥിരം framep
            = പുതിയ ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.തള്ളുക_back(procp);
        തിരികെ framep;
    }

    ശൂന്യം replaceWithMemberSel(AstVarRef* refp, സ്ഥിരം ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* സ്ഥിരം membersel = പുതിയ AstMemberSel{
            refp->fileline(), പുതിയ AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        എങ്കിൽ (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } അല്ലെങ്കിൽ {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
    }

    സ്ഥിതം തർക്കം hasAsyncFork(Ast_കണ്ണി* nodep) {
        തിരികെ nodep->exists([](AstFork* forkp) { തിരികെ !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { തിരികെ സത്യം; });
    }

    ശൂന്യം bind_കണ്ണി_To_Dyn_Scope(Ast_കണ്ണി* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        എങ്കിൽ (r.second) m_frameOrder.തള്ളുക_back(nodep);
    }
    ശൂന്യം bindInitIterate(Ast_കണ്ണി* stmtsp, ForkDynScopeFrame* framep) {
        വേണ്ടി (Ast_കണ്ണി* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstAssign* സ്ഥിരം asgnp = VN_CAST(stmtp, Assign)) {
                bind_കണ്ണി_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } അല്ലെങ്കിൽ എങ്കിൽ (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } അല്ലെങ്കിൽ {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    തർക്കം needsDynScope(സ്ഥിരം AstVarRef* refp) സ്ഥിരം {
        സ്ഥിരം AstVar* സ്ഥിരം varp = refp->varp();
        തിരികെ
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_മൂല്യം() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        എങ്കിൽ (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        എങ്കിൽ (hasAsyncFork(nodep)) തള്ളുക_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        എങ്കിൽ (hasAsyncFork(nodep)) തള്ളുക_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        എങ്കിൽ (!nodep->joinType().join()) ++m_forkDepth;

        സ്ഥിരം തർക്കം oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        എങ്കിൽ (nodep->declsp() || nodep->stmtsp()) framep = തള്ളുക_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        വേണ്ടി (Ast_കണ്ണി* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* സ്ഥിരം varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            എങ്കിൽ (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                എങ്കിൽ (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_തിരുകുക(varp);
                bind_കണ്ണി_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = അസത്യം;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        എങ്കിൽ (nodep->isTimingControl()) m_afterTimingControl = സത്യം;
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        visit(static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = സത്യം;
    }
    ശൂന്യം visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* സ്ഥിരം framep = frameOf(m_procp);
        എങ്കിൽ (!framep) തിരികെ;  // Cannot be legally referenced from a fork
        bind_കണ്ണി_To_Dyn_Scope(nodep, framep);
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* സ്ഥിരം framep = frameOf(nodep->varp());
        എങ്കിൽ (!framep) തിരികെ;
        എങ്കിൽ (needsDynScope(nodep)) {
            തർക്കം isEvent = അസത്യം;
            എങ്കിൽ (AstBasicDType* സ്ഥിരം dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            എങ്കിൽ (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            എങ്കിൽ (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_തിരുകുക(nodep->varp());
        }
        bind_കണ്ണി_To_Dyn_Scope(nodep, framep);
    }
    ശൂന്യം visit(AstAssign* nodep) override {
        എങ്കിൽ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_മൂല്യം()) {
            nodep->lhsp()->user2(സത്യം);
        }
        visit(static_cast<Ast_കണ്ണി_Stmt*>(nodep));
    }
    ശൂന്യം visit(AstAssignDly* nodep) override {
        എങ്കിൽ (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    തിരികെ refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(സത്യം);
            // Put it in a fork to prevent lifetime issues with the local
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            AstFork* സ്ഥിരം forkp = പുതിയ AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(പുതിയ AstBegin{flp, "", nodep, അസത്യം});
            UINFO(9, "assign new fork " << forkp);
        } അല്ലെങ്കിൽ {
            visit(static_cast<Ast_കണ്ണി_Stmt*>(nodep));
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (nodep->isTimingControl()) m_afterTimingControl = സത്യം;
        iterateChildren(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        തർക്കം typesAdded = അസത്യം;
        വേണ്ടി (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            എങ്കിൽ (!framep->instance().initialized()) തുടരുക;
            എങ്കിൽ (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = സത്യം;
            }
            എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(frameIt->first, VarRef)) {
                എങ്കിൽ (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        എങ്കിൽ (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        വേണ്ടി (auto കണ്ണി_frame : m_frames) frames.തിരുകുക(കണ്ണി_frame.second);
        വേണ്ടി (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

വർഗ്ഗം ForkVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    സ്ഥിരം VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    size_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    തർക്കം m_inFork = അസത്യം;  // Traversal in an async fork
    തർക്കം m_hasNestedFork = അസത്യം;  // Branch being taskified contains a nested fork
    തർക്കം m_inInitStmt = അസത്യം;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        വേണ്ടി (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            എങ്കിൽ (varp->name() == refp->name()) നിർത്തുക;
        }
        എങ്കിൽ (varp) തിരികെ varp;

        // Create a local copy to capture
        File_വരി* സ്ഥിരം flp = refp->fileline();
        varp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::നിവേശം);
        varp->funcLocal(സത്യം);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_കണ്ണി::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* സ്ഥിരം argp = പുതിയ AstArg{flp, refp->name(), refp->clone_വൃക്ഷം(അസത്യം)};
        m_capturedArgsp = Ast_കണ്ണി::addNext(m_capturedArgsp, argp);
        തിരികെ varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    തർക്കം taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = അസത്യം;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        എങ്കിൽ (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) തിരികെ അസത്യം;

        // Create task holding the statement and repalce statement with call to that task
        File_വരി* സ്ഥിരം flp = beginp->fileline();
        സ്ഥിരം std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* സ്ഥിരം taskp = പുതിയ AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        എങ്കിൽ (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_കണ്ണി::addNext(m_tasksp, taskp);
        എങ്കിൽ (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        എങ്കിൽ (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* സ്ഥിരം callp = പുതിയ AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        വേണ്ടി (AstVar* സ്ഥിരം localp : m_forkLocalsp) localp->funcLocal(സത്യം);

        // We did wrap the body
        തിരികെ സത്യം;
    }
    സ്ഥിതം തർക്കം isForkJoinNoneSentinelDelay(സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep) {
        സ്ഥിരം AstDelay* സ്ഥിരം delayp = VN_CAST(nodep, Delay);
        എങ്കിൽ (!delayp) തിരികെ അസത്യം;
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(delayp->lhsp(), Const);
        തിരികെ constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    സ്ഥിതം തർക്കം is_Disable_Queue_തള്ളുക_Self_Prefix(Ast_കണ്ണി* nodep) {
        ആകുമ്പോൾ (AstJumpBlock* സ്ഥിരം jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        തിരികെ nodep && nodep->is_Disable_Queue_തള്ളുക_Self_Stmt();
    }
    template <typename T_Owner>
    സ്ഥിതം തർക്കം തിരുകുക_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* സ്ഥിരം ownerp,
                                                          Ast_കണ്ണി* സ്ഥിരം firstStmtp,
                                                          Ast_കണ്ണി* സ്ഥിരം delayp) {
        Ast_കണ്ണി* തിരുകുക_Beforep = firstStmtp;
        ആകുമ്പോൾ (തിരുകുക_Beforep && തിരുകുക_Beforep->is_Disable_Queue_തള്ളുക_Self_Stmt()) {
            തിരുകുക_Beforep = തിരുകുക_Beforep->nextp();
        }
        എങ്കിൽ (AstJumpBlock* സ്ഥിരം jumpBlockp = VN_CAST(തിരുകുക_Beforep, JumpBlock)) {
            എങ്കിൽ (തിരുകുക_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                തിരികെ സത്യം;
            }
        }
        എങ്കിൽ (തിരുകുക_Beforep == firstStmtp) തിരികെ അസത്യം;
        എങ്കിൽ (തിരുകുക_Beforep) {
            തിരുകുക_Beforep->addHereThisAsNext(delayp);
        } അല്ലെങ്കിൽ {
            ownerp->addStmtsp(delayp);
        }
        തിരികെ സത്യം;
    }
    സ്ഥിതം ശൂന്യം moveForkSentinelAfterDisableQueuePushes(AstBegin* സ്ഥിരം beginp) {
        Ast_കണ്ണി* സ്ഥിരം firstStmtp = beginp->stmtsp();
        എങ്കിൽ (!isForkJoinNoneSentinelDelay(firstStmtp)) തിരികെ;
        Ast_കണ്ണി* സ്ഥിരം afterSentinelp = firstStmtp->nextp();
        എങ്കിൽ (!is_Disable_Queue_തള്ളുക_Self_Prefix(afterSentinelp)) തിരികെ;

        Ast_കണ്ണി* സ്ഥിരം delayp = firstStmtp->unlinkFrBack();
        സ്ഥിരം തർക്കം moved
            = തിരുകുക_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    സ്ഥിതം തർക്കം forkIsDisableable(AstFork* സ്ഥിരം nodep) {
        വേണ്ടി (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            എങ്കിൽ (is_Disable_Queue_തള്ളുക_Self_Prefix(itemp->stmtsp())) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        എങ്കിൽ (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    ശൂന്യം visit(AstFork* nodep) override {
        m_hasNestedFork = സത്യം;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        എങ്കിൽ ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_വരി* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            വേണ്ടി (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                എങ്കിൽ (!itemp->stmtsp()) തുടരുക;
                AstDelay* സ്ഥിരം delayp = പുതിയ AstDelay{
                    fl,
                    പുതിയ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    അസത്യം};
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
        എങ്കിൽ (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            തിരികെ;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = സത്യം;
            വേണ്ടി (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                എങ്കിൽ (taskify(itemp)) wrappedp.തള്ളുക_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        വേണ്ടി (AstBegin* സ്ഥിരം beginp : wrappedp) iterateAndNextNull(beginp);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (m_inFork) m_forkLocalsp.തിരുകുക(nodep);
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (!m_inFork) തിരികെ;
        AstVar* സ്ഥിരം varp = nodep->varp();
        // Not sure why this is OK ...
        എങ്കിൽ (!varp->isFuncLocal() && varp->isClassMember()) തിരികെ;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        എങ്കിൽ (varp->lifetime().isStatic()) തിരികെ;
        // If this ref is to a variable that will move into the task, then nothing to do
        എങ്കിൽ (m_forkLocalsp.എണ്ണം(varp)) തിരികെ;

        എങ്കിൽ (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_മൂല്യം() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            തിരികെ;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    ശൂന്യം visit(AstAssign* nodep) override {
        എങ്കിൽ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_മൂല്യം()) {
            nodep->lhsp()->user2(സത്യം);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = സത്യം;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstThisRef* nodep) override {}
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Fork class functions

ശൂന്യം V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_വൃക്ഷം("fork_dynscope", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_വൃക്ഷം("fork_tasks", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
