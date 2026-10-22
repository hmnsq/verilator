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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Fork.h"

#ଅନ୍ତର୍ଭାଵ "V3AstNodeExpr.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"

#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ ForkDynScopeInstance final {
ସର୍ବସାଧାରଣ:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    ତାର୍କିକ initialized() ଅଚଳ { ଫେରନ୍ତୁ m_classp != nullptr; }
};

ଶ୍ରେଣୀ ForkDynScopeFrame final {
    // MEMBERS
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // Module to insert the scope into
    Ast_ଗଣ୍ଠି* ଅଚଳ m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ଅଚଳ size_t m_class_id;  // Dynscope class ID
    ଅଚଳ size_t m_id;  // Dynscope ID

ସର୍ବସାଧାରଣ:
    ForkDynScopeFrame(Ast_ଗଣ୍ଠି_Module* modp, Ast_ଗଣ୍ଠି* procp, size_t class_id, size_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = ନୂତନ AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = ନୂତନ AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = ନୂତନ AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(ମିଥ୍ୟା);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        ଫେରନ୍ତୁ m_instance;
    }

    ଅଚଳ ForkDynScopeInstance& instance() ଅଚଳ { ଫେରନ୍ତୁ m_instance; }
    ଶୂନ୍ୟ capture_Var_ଭର୍ତ୍ତି(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        ଯଦି (r.second) m_captureOrder.ଠେଲ_back(varp);
    }
    ତାର୍କିକ captured(AstVar* varp) { ଫେରନ୍ତୁ m_captures.ଗଣନା_2(varp) != 0; }
    Ast_ଗଣ୍ଠି* procp() ଅଚଳ { ଫେରନ୍ତୁ m_procp; }

    ଶୂନ୍ୟ populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        ପାଇଁ (AstVar* varp : m_captureOrder) {
            ଯଦି (varp->direction().isAny()) {
                varp = varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                varp->direction(VDirection::NONE);
            } ଅନ୍ୟଥା {
                varp->unlinkFrBack();
            }
            varp->funcLocal(ମିଥ୍ୟା);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(ମିଥ୍ୟା);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ଅଚଳ newp
            = ନୂତନ AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(ସତ୍ୟ);
        newp->classMethod(ସତ୍ୟ);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    ଶୂନ୍ୟ linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        ଯଦି (AstFork* ଅଚଳ forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            ଫେରନ୍ତୁ;
        }

        Ast_ଗଣ୍ଠି* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_ଗଣ୍ଠି* initp = stmtp;
        ଯେତେବେଳେ (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ଅଚଳ newp = ନୂତନ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ଗଣ୍ଠି_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_ଗଣ୍ଠି* ଅଚଳ asgnp = ନୂତନ AstAssign{
            m_procp->fileline(),
            ନୂତନ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_ଗଣ୍ଠି* initsp = nullptr;  // Arguments need to be copied
        ପାଇଁ (AstVar* varp : m_captureOrder) {
            ଯଦି (!varp->direction().isAny()) ଜାରି;

            ଯଦି (varp->direction().isNonOutput()) {
                AstMemberSel* ଅଚଳ memberselp = ନୂତନ AstMemberSel{
                    varp->fileline(),
                    ନୂତନ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ଗଣ୍ଠି* initAsgnp
                    = ନୂତନ AstAssign{varp->fileline(), memberselp,
                                    ନୂତନ AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_ଗଣ୍ଠି::addNext(initsp, initAsgnp);
            }

            ଯଦି (AstBasicDType* ଅଚଳ dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                ଯଦି (dtypep->isEvent()) ଜାରି;
            }

            ଯଦି (varp->direction().isWritable()) {
                AstMemberSel* ଅଚଳ memberselp = ନୂତନ AstMemberSel{
                    varp->fileline(),
                    ନୂତନ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ଗଣ୍ଠି* writebackAsgnp = ନୂତନ AstAssign{
                    varp->fileline(), ନୂତନ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_ଗଣ୍ଠି::addNext(stmtp, writebackAsgnp);
            }
        }
        ଯଦି (initsp) Ast_ଗଣ୍ଠି::addNext(asgnp, initsp);

        ଯଦି (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି::addNext(asgnp, static_cast<Ast_ଗଣ୍ଠି*>(initp));
            stmtp = asgnp;
        }

        Ast_ଗଣ୍ଠି::addNext(static_cast<Ast_ଗଣ୍ଠି*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    ତାର୍କିକ linked() ଅଚଳ { ଫେରନ୍ତୁ m_instance.initialized() && m_instance.m_handlep->backp(); }

ବ୍ୟକ୍ତିଗତ:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ଅଚଳ newp = ନୂତନ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ଗଣ୍ଠି_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        ଫେରନ୍ତୁ ନୂତନ AstAssign{
            m_procp->fileline(),
            ନୂତନ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    ଶୂନ୍ୟ linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ଅଚଳ std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* ଅଚଳ beginp = ନୂତନ AstBegin{forkp->fileline(), name, m_instance.m_handlep, ସତ୍ୟ};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        ଯଦି (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { ଫେରନ୍ତୁ "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(ଅଚଳ Ast_ଗଣ୍ଠି* fromp) {
        ଫେରନ୍ତୁ "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_ଗଣ୍ଠି* getProcStmts() {
        Ast_ଗଣ୍ଠି* stmtsp = nullptr;
        ଯଦି (!m_procp) ଫେରନ୍ତୁ nullptr;
        ଯଦି (AstBegin* ଅଚଳ beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ taskp = VN_CAST(m_procp, ଗଣ୍ଠି_FTask)) {
            stmtsp = taskp->stmtsp();
        } ଅନ୍ୟଥା {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        ଫେରନ୍ତୁ stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

ଶ୍ରେଣୀ DynScopeVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;

    // STATE
    ତାର୍କିକ m_inFunc = ମିଥ୍ୟା;  // True if in a function
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Module we are currently under
    Ast_ଗଣ୍ଠି* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_ଗଣ୍ଠି*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_ଗଣ୍ଠି*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    ତାର୍କିକ m_afterTimingControl = ମିଥ୍ୟା;  // A timing control might've be executed in the current
                                        // process
    size_t m_id = 0;  // Unique ID for a frame
    size_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_ଗଣ୍ଠି* nodep) {
        auto frameIt = m_frames.find(nodep);
        ଯଦି (frameIt == m_frames.end()) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ frameIt->second;
    }

    ଅଚଳ ForkDynScopeFrame* frameOf(Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {
        auto frameIt = m_frames.find(nodep);
        ଯଦି (frameIt == m_frames.end()) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ frameIt->second;
    }

    ForkDynScopeFrame* ଠେଲ_Dyn_Scope_Frame(Ast_ଗଣ୍ଠି* procp) {
        ForkDynScopeFrame* ଅଚଳ framep
            = ନୂତନ ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.ଠେଲ_back(procp);
        ଫେରନ୍ତୁ framep;
    }

    ଶୂନ୍ୟ replaceWithMemberSel(AstVarRef* refp, ଅଚଳ ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* ଅଚଳ membersel = ନୂତନ AstMemberSel{
            refp->fileline(), ନୂତନ AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        ଯଦି (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } ଅନ୍ୟଥା {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
    }

    ସ୍ଥିର ତାର୍କିକ hasAsyncFork(Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ nodep->exists([](AstFork* forkp) { ଫେରନ୍ତୁ !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { ଫେରନ୍ତୁ ସତ୍ୟ; });
    }

    ଶୂନ୍ୟ bind_ଗଣ୍ଠି_To_Dyn_Scope(Ast_ଗଣ୍ଠି* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        ଯଦି (r.second) m_frameOrder.ଠେଲ_back(nodep);
    }
    ଶୂନ୍ୟ bindInitIterate(Ast_ଗଣ୍ଠି* stmtsp, ForkDynScopeFrame* framep) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstAssign* ଅଚଳ asgnp = VN_CAST(stmtp, Assign)) {
                bind_ଗଣ୍ଠି_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } ଅନ୍ୟଥା ଯଦି (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } ଅନ୍ୟଥା ଯଦି (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } ଅନ୍ୟଥା {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    ତାର୍କିକ needsDynScope(ଅଚଳ AstVarRef* refp) ଅଚଳ {
        ଅଚଳ AstVar* ଅଚଳ varp = refp->varp();
        ଫେରନ୍ତୁ
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_ମୂଲ୍ୟ() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        ଯଦି (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        ଯଦି (hasAsyncFork(nodep)) ଠେଲ_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        ଯଦି (hasAsyncFork(nodep)) ଠେଲ_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        ଯଦି (!nodep->joinType().join()) ++m_forkDepth;

        ଅଚଳ ତାର୍କିକ oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        ଯଦି (nodep->declsp() || nodep->stmtsp()) framep = ଠେଲ_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        ପାଇଁ (Ast_ଗଣ୍ଠି* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ଅଚଳ varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            ଯଦି (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                ଯଦି (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_ଭର୍ତ୍ତି(varp);
                bind_ଗଣ୍ଠି_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = ମିଥ୍ୟା;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        ଯଦି (nodep->isTimingControl()) m_afterTimingControl = ସତ୍ୟ;
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        visit(static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = ସତ୍ୟ;
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* ଅଚଳ framep = frameOf(m_procp);
        ଯଦି (!framep) ଫେରନ୍ତୁ;  // Cannot be legally referenced from a fork
        bind_ଗଣ୍ଠି_To_Dyn_Scope(nodep, framep);
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* ଅଚଳ framep = frameOf(nodep->varp());
        ଯଦି (!framep) ଫେରନ୍ତୁ;
        ଯଦି (needsDynScope(nodep)) {
            ତାର୍କିକ isEvent = ମିଥ୍ୟା;
            ଯଦି (AstBasicDType* ଅଚଳ dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            ଯଦି (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            ଯଦି (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_ଭର୍ତ୍ତି(nodep->varp());
        }
        bind_ଗଣ୍ଠି_To_Dyn_Scope(nodep, framep);
    }
    ଶୂନ୍ୟ visit(AstAssign* nodep) override {
        ଯଦି (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_ମୂଲ୍ୟ()) {
            nodep->lhsp()->user2(ସତ୍ୟ);
        }
        visit(static_cast<Ast_ଗଣ୍ଠି_Stmt*>(nodep));
    }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        ଯଦି (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    ଫେରନ୍ତୁ refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(ସତ୍ୟ);
            // Put it in a fork to prevent lifetime issues with the local
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            AstFork* ଅଚଳ forkp = ନୂତନ AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(ନୂତନ AstBegin{flp, "", nodep, ମିଥ୍ୟା});
            UINFO(9, "assign new fork " << forkp);
        } ଅନ୍ୟଥା {
            visit(static_cast<Ast_ଗଣ୍ଠି_Stmt*>(nodep));
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (nodep->isTimingControl()) m_afterTimingControl = ସତ୍ୟ;
        iterateChildren(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        ତାର୍କିକ typesAdded = ମିଥ୍ୟା;
        ପାଇଁ (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            ଯଦି (!framep->instance().initialized()) ଜାରି;
            ଯଦି (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = ସତ୍ୟ;
            }
            ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(frameIt->first, VarRef)) {
                ଯଦି (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        ଯଦି (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        ପାଇଁ (auto ଗଣ୍ଠି_frame : m_frames) frames.ଭର୍ତ୍ତି(ଗଣ୍ଠି_frame.second);
        ପାଇଁ (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

ଶ୍ରେଣୀ ForkVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ଅଚଳ VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    size_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    ତାର୍କିକ m_inFork = ମିଥ୍ୟା;  // Traversal in an async fork
    ତାର୍କିକ m_hasNestedFork = ମିଥ୍ୟା;  // Branch being taskified contains a nested fork
    ତାର୍କିକ m_inInitStmt = ମିଥ୍ୟା;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        ପାଇଁ (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ଯଦି (varp->name() == refp->name()) ଭାଙ୍ଗ;
        }
        ଯଦି (varp) ଫେରନ୍ତୁ varp;

        // Create a local copy to capture
        File_ଧାଡ଼ି* ଅଚଳ flp = refp->fileline();
        varp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::ନିବେଶ);
        varp->funcLocal(ସତ୍ୟ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_ଗଣ୍ଠି::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ଅଚଳ argp = ନୂତନ AstArg{flp, refp->name(), refp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
        m_capturedArgsp = Ast_ଗଣ୍ଠି::addNext(m_capturedArgsp, argp);
        ଫେରନ୍ତୁ varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    ତାର୍କିକ taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = ମିଥ୍ୟା;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        ଯଦି (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Create task holding the statement and repalce statement with call to that task
        File_ଧାଡ଼ି* ଅଚଳ flp = beginp->fileline();
        ଅଚଳ std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* ଅଚଳ taskp = ନୂତନ AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        ଯଦି (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_ଗଣ୍ଠି::addNext(m_tasksp, taskp);
        ଯଦି (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        ଯଦି (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* ଅଚଳ callp = ନୂତନ AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        ପାଇଁ (AstVar* ଅଚଳ localp : m_forkLocalsp) localp->funcLocal(ସତ୍ୟ);

        // We did wrap the body
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ସ୍ଥିର ତାର୍କିକ isForkJoinNoneSentinelDelay(ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
        ଅଚଳ AstDelay* ଅଚଳ delayp = VN_CAST(nodep, Delay);
        ଯଦି (!delayp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(delayp->lhsp(), Const);
        ଫେରନ୍ତୁ constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    ସ୍ଥିର ତାର୍କିକ is_Disable_Queue_ଠେଲ_Self_Prefix(Ast_ଗଣ୍ଠି* nodep) {
        ଯେତେବେଳେ (AstJumpBlock* ଅଚଳ jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        ଫେରନ୍ତୁ nodep && nodep->is_Disable_Queue_ଠେଲ_Self_Stmt();
    }
    template <typename T_Owner>
    ସ୍ଥିର ତାର୍କିକ ଭର୍ତ୍ତି_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* ଅଚଳ ownerp,
                                                          Ast_ଗଣ୍ଠି* ଅଚଳ firstStmtp,
                                                          Ast_ଗଣ୍ଠି* ଅଚଳ delayp) {
        Ast_ଗଣ୍ଠି* ଭର୍ତ୍ତି_Beforep = firstStmtp;
        ଯେତେବେଳେ (ଭର୍ତ୍ତି_Beforep && ଭର୍ତ୍ତି_Beforep->is_Disable_Queue_ଠେଲ_Self_Stmt()) {
            ଭର୍ତ୍ତି_Beforep = ଭର୍ତ୍ତି_Beforep->nextp();
        }
        ଯଦି (AstJumpBlock* ଅଚଳ jumpBlockp = VN_CAST(ଭର୍ତ୍ତି_Beforep, JumpBlock)) {
            ଯଦି (ଭର୍ତ୍ତି_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଯଦି (ଭର୍ତ୍ତି_Beforep == firstStmtp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (ଭର୍ତ୍ତି_Beforep) {
            ଭର୍ତ୍ତି_Beforep->addHereThisAsNext(delayp);
        } ଅନ୍ୟଥା {
            ownerp->addStmtsp(delayp);
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ସ୍ଥିର ଶୂନ୍ୟ moveForkSentinelAfterDisableQueuePushes(AstBegin* ଅଚଳ beginp) {
        Ast_ଗଣ୍ଠି* ଅଚଳ firstStmtp = beginp->stmtsp();
        ଯଦି (!isForkJoinNoneSentinelDelay(firstStmtp)) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି* ଅଚଳ afterSentinelp = firstStmtp->nextp();
        ଯଦି (!is_Disable_Queue_ଠେଲ_Self_Prefix(afterSentinelp)) ଫେରନ୍ତୁ;

        Ast_ଗଣ୍ଠି* ଅଚଳ delayp = firstStmtp->unlinkFrBack();
        ଅଚଳ ତାର୍କିକ moved
            = ଭର୍ତ୍ତି_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ସ୍ଥିର ତାର୍କିକ forkIsDisableable(AstFork* ଅଚଳ nodep) {
        ପାଇଁ (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            ଯଦି (is_Disable_Queue_ଠେଲ_Self_Prefix(itemp->stmtsp())) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        ଯଦି (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        m_hasNestedFork = ସତ୍ୟ;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        ଯଦି ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_ଧାଡ଼ି* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            ପାଇଁ (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                ଯଦି (!itemp->stmtsp()) ଜାରି;
                AstDelay* ଅଚଳ delayp = ନୂତନ AstDelay{
                    fl,
                    ନୂତନ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    ମିଥ୍ୟା};
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
        ଯଦି (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            ଫେରନ୍ତୁ;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = ସତ୍ୟ;
            ପାଇଁ (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                ଯଦି (taskify(itemp)) wrappedp.ଠେଲ_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        ପାଇଁ (AstBegin* ଅଚଳ beginp : wrappedp) iterateAndNextNull(beginp);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (m_inFork) m_forkLocalsp.ଭର୍ତ୍ତି(nodep);
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (!m_inFork) ଫେରନ୍ତୁ;
        AstVar* ଅଚଳ varp = nodep->varp();
        // Not sure why this is OK ...
        ଯଦି (!varp->isFuncLocal() && varp->isClassMember()) ଫେରନ୍ତୁ;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        ଯଦି (varp->lifetime().isStatic()) ଫେରନ୍ତୁ;
        // If this ref is to a variable that will move into the task, then nothing to do
        ଯଦି (m_forkLocalsp.ଗଣନା_2(varp)) ଫେରନ୍ତୁ;

        ଯଦି (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_ମୂଲ୍ୟ() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            ଫେରନ୍ତୁ;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    ଶୂନ୍ୟ visit(AstAssign* nodep) override {
        ଯଦି (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_ମୂଲ୍ୟ()) {
            nodep->lhsp()->user2(ସତ୍ୟ);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = ସତ୍ୟ;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstThisRef* nodep) override {}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Fork class functions

ଶୂନ୍ୟ V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("fork_dynscope", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("fork_tasks", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
