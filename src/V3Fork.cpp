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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Fork.h"

#ಅನ್ತರ್ಭಾವ "V3AstNodeExpr.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"

#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ ForkDynScopeInstance final {
ಸಾರ್ವಜನಿಕ:
    AstClass* m_classp = nullptr;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = nullptr;  // RefDType for the above
    AstVar* m_handlep = nullptr;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    ತಾರ್ಕಿಕ initialized() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classp != nullptr; }
};

ವರ್ಗ ForkDynScopeFrame final {
    // MEMBERS
    Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // Module to insert the scope into
    Ast_ಗಂಟು* ಧ್ರುವ m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_captureOrder;  // Variables to be moved into the dynscope
    std::set<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ಧ್ರುವ size_t m_class_id;  // Dynscope class ID
    ಧ್ರುವ size_t m_id;  // Dynscope ID

ಸಾರ್ವಜನಿಕ:
    ForkDynScopeFrame(Ast_ಗಂಟು_Module* modp, Ast_ಗಂಟು* procp, size_t class_id, size_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = ನೂತನ AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = ನೂತನ AstClassRefDType{m_procp->fileline(), m_instance.m_classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = ನೂತನ AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(ಅಸತ್ಯ);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        ಪ್ರತ್ಯಾಗಮ m_instance;
    }

    ಧ್ರುವ ForkDynScopeInstance& instance() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_instance; }
    ಶೂನ್ಯ capture_Var_ಸೇರಿಸು(AstVar* varp) {
        auto r = m_captures.emplace(varp);
        ಯದಿ (r.second) m_captureOrder.ತಳ್ಳು_back(varp);
    }
    ತಾರ್ಕಿಕ captured(AstVar* varp) { ಪ್ರತ್ಯಾಗಮ m_captures.ಎಣಿಕೆ(varp) != 0; }
    Ast_ಗಂಟು* procp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_procp; }

    ಶೂನ್ಯ populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        ಪ್ರತಿ (AstVar* varp : m_captureOrder) {
            ಯದಿ (varp->direction().isAny()) {
                varp = varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                varp->direction(VDirection::NONE);
            } ಅನ್ಯಥಾ {
                varp->unlinkFrBack();
            }
            varp->funcLocal(ಅಸತ್ಯ);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->usedLoopIdx(ಅಸತ್ಯ);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->addStmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ಧ್ರುವ newp
            = ನೂತನ AstFunc{m_instance.m_classp->fileline(), "new", nullptr, nullptr};
        newp->isConstructor(ಸತ್ಯ);
        newp->classMethod(ಸತ್ಯ);
        newp->dtypep(newp->findVoidDType());
        m_instance.m_classp->addStmtsp(newp);
    }

    ಶೂನ್ಯ linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        ಯದಿ (AstFork* ಧ್ರುವ forkp = VN_CAST(m_procp, Fork)) {
            linkNodesOfFork(memberMap, forkp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        Ast_ಗಂಟು* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlinkFrBackWithNext(&stmtpHandle);

        // Find node after last variable declaration
        Ast_ಗಂಟು* initp = stmtp;
        ಯಾವತ್ (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ಧ್ರುವ newp = ನೂತನ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ಗಂಟು_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_ಗಂಟು* ಧ್ರುವ asgnp = ನೂತನ AstAssign{
            m_procp->fileline(),
            ನೂತನ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_ಗಂಟು* initsp = nullptr;  // Arguments need to be copied
        ಪ್ರತಿ (AstVar* varp : m_captureOrder) {
            ಯದಿ (!varp->direction().isAny()) ಅನುವರ್ತ;

            ಯದಿ (varp->direction().isNonOutput()) {
                AstMemberSel* ಧ್ರುವ memberselp = ನೂತನ AstMemberSel{
                    varp->fileline(),
                    ನೂತನ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ಗಂಟು* initAsgnp
                    = ನೂತನ AstAssign{varp->fileline(), memberselp,
                                    ನೂತನ AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_ಗಂಟು::addNext(initsp, initAsgnp);
            }

            ಯದಿ (AstBasicDType* ಧ್ರುವ dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                ಯದಿ (dtypep->isEvent()) ಅನುವರ್ತ;
            }

            ಯದಿ (varp->direction().isWritable()) {
                AstMemberSel* ಧ್ರುವ memberselp = ನೂತನ AstMemberSel{
                    varp->fileline(),
                    ನೂತನ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.findMember(m_instance.m_classp, varp->name()), Var)};
                Ast_ಗಂಟು* writebackAsgnp = ನೂತನ AstAssign{
                    varp->fileline(), ನೂತನ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_ಗಂಟು::addNext(stmtp, writebackAsgnp);
            }
        }
        ಯದಿ (initsp) Ast_ಗಂಟು::addNext(asgnp, initsp);

        ಯದಿ (initp != stmtp) {
            initp->addHereThisAsNext(asgnp);
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು::addNext(asgnp, static_cast<Ast_ಗಂಟು*>(initp));
            stmtp = asgnp;
        }

        Ast_ಗಂಟು::addNext(static_cast<Ast_ಗಂಟು*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    ತಾರ್ಕಿಕ linked() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_instance.initialized() && m_instance.m_handlep->backp(); }

ಗುಪ್ತ:
    AstAssign* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ಧ್ರುವ newp = ನೂತನ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.findMember(m_instance.m_classp, "new"), ಗಂಟು_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{
            m_procp->fileline(),
            ನೂತನ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    ಶೂನ್ಯ linkNodesOfFork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ಧ್ರುವ std::string name = "_Vwrapped_"  //
                                 + (forkp->name().empty() ? "" : forkp->name() + "_")  //
                                 + std::to_string(m_id);
        AstBegin* ಧ್ರುವ beginp = ನೂತನ AstBegin{forkp->fileline(), name, m_instance.m_handlep, ಸತ್ಯ};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->addStmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        ಯದಿ (forkp->stmtsp()) beginp->addStmtsp(forkp->stmtsp()->unlinkFrBackWithNext());
        // Put the Fork back at the end of the Begin
        beginp->addStmtsp(forkp);
        m_modp->addStmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { ಪ್ರತ್ಯಾಗಮ "__VDynScope_" + cvtToStr(m_class_id); }

    string generateDynScopeHandleName(ಧ್ರುವ Ast_ಗಂಟು* fromp) {
        ಪ್ರತ್ಯಾಗಮ "__VDynScope_" + (!fromp->name().empty() ? (fromp->name() + "_") : "ANON_")
               + cvtToStr(m_id);
    }

    Ast_ಗಂಟು* getProcStmts() {
        Ast_ಗಂಟು* stmtsp = nullptr;
        ಯದಿ (!m_procp) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (AstBegin* ಧ್ರುವ beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ taskp = VN_CAST(m_procp, ಗಂಟು_FTask)) {
            stmtsp = taskp->stmtsp();
        } ಅನ್ಯಥಾ {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        ಪ್ರತ್ಯಾಗಮ stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

ವರ್ಗ DynScopeVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // STATE
    ತಾರ್ಕಿಕ m_inFunc = ಅಸತ್ಯ;  // True if in a function
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Module we are currently under
    Ast_ಗಂಟು* m_procp = nullptr;  // Function/task/block we are currently under
    std::deque<Ast_ಗಂಟು*> m_frameOrder;  // Ordered list of frames (for determinism)
    std::map<Ast_ಗಂಟು*, ForkDynScopeFrame*> m_frames;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_forkDepth = 0;  // Number of asynchronous forks we are currently under
    ತಾರ್ಕಿಕ m_afterTimingControl = ಅಸತ್ಯ;  // A timing control might've be executed in the current
                                        // process
    size_t m_id = 0;  // Unique ID for a frame
    size_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    ForkDynScopeFrame* frameOf(Ast_ಗಂಟು* nodep) {
        auto frameIt = m_frames.find(nodep);
        ಯದಿ (frameIt == m_frames.end()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ frameIt->second;
    }

    ಧ್ರುವ ForkDynScopeFrame* frameOf(Ast_ಗಂಟು* nodep) ಧ್ರುವ {
        auto frameIt = m_frames.find(nodep);
        ಯದಿ (frameIt == m_frames.end()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ frameIt->second;
    }

    ForkDynScopeFrame* ತಳ್ಳು_Dyn_Scope_Frame(Ast_ಗಂಟು* procp) {
        ForkDynScopeFrame* ಧ್ರುವ framep
            = ನೂತನ ForkDynScopeFrame{m_modp, procp, m_class_id++, m_id++};
        auto r = m_frames.emplace(procp, framep);
        UASSERT_OBJ(r.second, m_modp, "Procedure already contains a frame");
        m_frameOrder.ತಳ್ಳು_back(procp);
        ಪ್ರತ್ಯಾಗಮ framep;
    }

    ಶೂನ್ಯ replaceWithMemberSel(AstVarRef* refp, ಧ್ರುವ ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlinkFrBack(&handle);
        AstMemberSel* ಧ್ರುವ membersel = ನೂತನ AstMemberSel{
            refp->fileline(), ನೂತನ AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        ಯದಿ (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.findMember(dynScope.m_classp, refp->varp()->name()), Var));
        } ಅನ್ಯಥಾ {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasAsyncFork(Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ nodep->exists([](AstFork* forkp) { ಪ್ರತ್ಯಾಗಮ !forkp->joinType().join(); })
               || nodep->exists([](AstAssignDly*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; });
    }

    ಶೂನ್ಯ bind_ಗಂಟು_To_Dyn_Scope(Ast_ಗಂಟು* nodep, ForkDynScopeFrame* framep) {
        auto r = m_frames.emplace(nodep, framep);
        ಯದಿ (r.second) m_frameOrder.ತಳ್ಳು_back(nodep);
    }
    ಶೂನ್ಯ bindInitIterate(Ast_ಗಂಟು* stmtsp, ForkDynScopeFrame* framep) {
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstAssign* ಧ್ರುವ asgnp = VN_CAST(stmtp, Assign)) {
                bind_ಗಂಟು_To_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } ಅನ್ಯಥಾ ಯದಿ (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } ಅನ್ಯಥಾ ಯದಿ (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } ಅನ್ಯಥಾ {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    ತಾರ್ಕಿಕ needsDynScope(ಧ್ರುವ AstVarRef* refp) ಧ್ರುವ {
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = refp->varp();
        ಪ್ರತ್ಯಾಗಮ
            // Can this variable escape the scope
            ((m_forkDepth > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_ಮೌಲ್ಯ() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        ಯದಿ (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_inFunc);
        m_inFunc = VN_IS(nodep, Func);
        ಯದಿ (hasAsyncFork(nodep)) ತಳ್ಳು_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        ಯದಿ (hasAsyncFork(nodep)) ತಳ್ಳು_Dyn_Scope_Frame(m_procp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        VL_RESTORER(m_forkDepth);
        ಯದಿ (!nodep->joinType().join()) ++m_forkDepth;

        ಧ್ರುವ ತಾರ್ಕಿಕ oldAfterTimingControl = m_afterTimingControl;

        ForkDynScopeFrame* framep = nullptr;
        ಯದಿ (nodep->declsp() || nodep->stmtsp()) framep = ತಳ್ಳು_Dyn_Scope_Frame(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        ಪ್ರತಿ (Ast_ಗಂಟು* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ಧ್ರುವ varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            ಯದಿ (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                ಯದಿ (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_ಸೇರಿಸು(varp);
                bind_ಗಂಟು_To_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = ಅಸತ್ಯ;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        ಯದಿ (nodep->isTimingControl()) m_afterTimingControl = ಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        visit(static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = ಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        nodep->user1(m_forkDepth);
        ForkDynScopeFrame* ಧ್ರುವ framep = frameOf(m_procp);
        ಯದಿ (!framep) ಪ್ರತ್ಯಾಗಮ;  // Cannot be legally referenced from a fork
        bind_ಗಂಟು_To_Dyn_Scope(nodep, framep);
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ForkDynScopeFrame* ಧ್ರುವ framep = frameOf(nodep->varp());
        ಯದಿ (!framep) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (needsDynScope(nodep)) {
            ತಾರ್ಕಿಕ isEvent = ಅಸತ್ಯ;
            ಯದಿ (AstBasicDType* ಧ್ರುವ dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.setAssignsEvents();
                isEvent = dtypep->isEvent();
            }
            ಯದಿ (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_inFunc ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            ಯದಿ (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_ಸೇರಿಸು(nodep->varp());
        }
        bind_ಗಂಟು_To_Dyn_Scope(nodep, framep);
    }
    ಶೂನ್ಯ visit(AstAssign* nodep) override {
        ಯದಿ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_ಮೌಲ್ಯ()) {
            nodep->lhsp()->user2(ಸತ್ಯ);
        }
        visit(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
    }
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override {
        ಯದಿ (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    ಪ್ರತ್ಯಾಗಮ refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(ಸತ್ಯ);
            // Put it in a fork to prevent lifetime issues with the local
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            AstFork* ಧ್ರುವ forkp = ನೂತನ AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->addForksp(ನೂತನ AstBegin{flp, "", nodep, ಅಸತ್ಯ});
            UINFO(9, "assign new fork " << forkp);
        } ಅನ್ಯಥಾ {
            visit(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (nodep->isTimingControl()) m_afterTimingControl = ಸತ್ಯ;
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        visit(nodep);

        // Commit changes to AST
        ತಾರ್ಕಿಕ typesAdded = ಅಸತ್ಯ;
        ಪ್ರತಿ (auto orderp : m_frameOrder) {
            UINFO(9, "Frame commit " << orderp);
            auto frameIt = m_frames.find(orderp);
            UASSERT_OBJ(frameIt != m_frames.end(), orderp, "m_frames didn't contain m_frameOrder");
            ForkDynScopeFrame* framep = frameIt->second;
            ಯದಿ (!framep->instance().initialized()) ಅನುವರ್ತ;
            ಯದಿ (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = ಸತ್ಯ;
            }
            ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(frameIt->first, VarRef)) {
                ಯದಿ (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        ಯದಿ (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::set<ForkDynScopeFrame*> frames;
        ಪ್ರತಿ (auto ಗಂಟು_frame : m_frames) frames.ಸೇರಿಸು(ಗಂಟು_frame.second);
        ಪ್ರತಿ (auto* framep : frames) delete framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

ವರ್ಗ ForkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // STATE - for current AstNodeModule
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Class/module we are currently under
    AstTask* m_tasksp = nullptr;  // Tasks exrtracted under current module
    size_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    ತಾರ್ಕಿಕ m_inFork = ಅಸತ್ಯ;  // Traversal in an async fork
    ತಾರ್ಕಿಕ m_hasNestedFork = ಅಸತ್ಯ;  // Branch being taskified contains a nested fork
    ತಾರ್ಕಿಕ m_inInitStmt = ಅಸತ್ಯ;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Enclosing function/task
    std::set<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = nullptr;  // Local copies of captured variables
    AstArg* m_capturedArgsp = nullptr;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = nullptr;
        ಪ್ರತಿ (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ಯದಿ (varp->name() == refp->name()) ಭಙ್ಗ;
        }
        ಯದಿ (varp) ಪ್ರತ್ಯಾಗಮ varp;

        // Create a local copy to capture
        File_ಸಾಲು* ಧ್ರುವ flp = refp->fileline();
        varp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::ಒಳಹರಿವು);
        varp->funcLocal(ಸತ್ಯ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_ಗಂಟು::addNext(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ಧ್ರುವ argp = ನೂತನ AstArg{flp, refp->name(), refp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
        m_capturedArgsp = Ast_ಗಂಟು::addNext(m_capturedArgsp, argp);
        ಪ್ರತ್ಯಾಗಮ varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    ತಾರ್ಕಿಕ taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = ಅಸತ್ಯ;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = nullptr;
        m_capturedArgsp = nullptr;
        iterate(beginp);

        // No need to do it if no variabels are captured
        ಯದಿ (m_forkLocalsp.empty() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Create task holding the statement and repalce statement with call to that task
        File_ಸಾಲು* ಧ್ರುವ flp = beginp->fileline();
        ಧ್ರುವ std::string name = "__VforkTask_" + std::to_string(m_nForkTasks++);
        AstTask* ಧ್ರುವ taskp = ನೂತನ AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        ಯದಿ (m_hasNestedFork) {
            taskp->classMethod(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_ಗಂಟು::addNext(m_tasksp, taskp);
        ಯದಿ (beginp->declsp()) taskp->addStmtsp(beginp->declsp()->unlinkFrBackWithNext());
        ಯದಿ (beginp->stmtsp()) taskp->addStmtsp(beginp->stmtsp()->unlinkFrBackWithNext());
        AstTaskRef* ಧ್ರುವ callp = ನೂತನ AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->addStmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        ಪ್ರತಿ (AstVar* ಧ್ರುವ localp : m_forkLocalsp) localp->funcLocal(ಸತ್ಯ);

        // We did wrap the body
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isForkJoinNoneSentinelDelay(ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep) {
        ಧ್ರುವ AstDelay* ಧ್ರುವ delayp = VN_CAST(nodep, Delay);
        ಯದಿ (!delayp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(delayp->lhsp(), Const);
        ಪ್ರತ್ಯಾಗಮ constp && (constp->toUQuad() == std::numeric_limits<uint64_t>::max());
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Disable_Queue_ತಳ್ಳು_Self_Prefix(Ast_ಗಂಟು* nodep) {
        ಯಾವತ್ (AstJumpBlock* ಧ್ರುವ jumpBlockp = VN_CAST(nodep, JumpBlock)) {
            nodep = jumpBlockp->stmtsp();
        }
        ಪ್ರತ್ಯಾಗಮ nodep && nodep->is_Disable_Queue_ತಳ್ಳು_Self_Stmt();
    }
    template <typename T_Owner>
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಸೇರಿಸು_Fork_Sentinel_After_Disable_Queue_Pushes(T_Owner* ಧ್ರುವ ownerp,
                                                          Ast_ಗಂಟು* ಧ್ರುವ firstStmtp,
                                                          Ast_ಗಂಟು* ಧ್ರುವ delayp) {
        Ast_ಗಂಟು* ಸೇರಿಸು_Beforep = firstStmtp;
        ಯಾವತ್ (ಸೇರಿಸು_Beforep && ಸೇರಿಸು_Beforep->is_Disable_Queue_ತಳ್ಳು_Self_Stmt()) {
            ಸೇರಿಸು_Beforep = ಸೇರಿಸು_Beforep->nextp();
        }
        ಯದಿ (AstJumpBlock* ಧ್ರುವ jumpBlockp = VN_CAST(ಸೇರಿಸು_Beforep, JumpBlock)) {
            ಯದಿ (ಸೇರಿಸು_Fork_Sentinel_After_Disable_Queue_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಯದಿ (ಸೇರಿಸು_Beforep == firstStmtp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (ಸೇರಿಸು_Beforep) {
            ಸೇರಿಸು_Beforep->addHereThisAsNext(delayp);
        } ಅನ್ಯಥಾ {
            ownerp->addStmtsp(delayp);
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ moveForkSentinelAfterDisableQueuePushes(AstBegin* ಧ್ರುವ beginp) {
        Ast_ಗಂಟು* ಧ್ರುವ firstStmtp = beginp->stmtsp();
        ಯದಿ (!isForkJoinNoneSentinelDelay(firstStmtp)) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು* ಧ್ರುವ afterSentinelp = firstStmtp->nextp();
        ಯದಿ (!is_Disable_Queue_ತಳ್ಳು_Self_Prefix(afterSentinelp)) ಪ್ರತ್ಯಾಗಮ;

        Ast_ಗಂಟು* ಧ್ರುವ delayp = firstStmtp->unlinkFrBack();
        ಧ್ರುವ ತಾರ್ಕಿಕ moved
            = ಸೇರಿಸು_Fork_Sentinel_After_Disable_Queue_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ forkIsDisableable(AstFork* ಧ್ರುವ nodep) {
        ಪ್ರತಿ (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            ಯದಿ (is_Disable_Queue_ತಳ್ಳು_Self_Prefix(itemp->stmtsp())) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = nullptr;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        ಯದಿ (m_tasksp) nodep->addStmtsp(m_tasksp);
    }

    ಶೂನ್ಯ visit(AstFork* nodep) override {
        m_hasNestedFork = ಸತ್ಯ;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        ಯದಿ ((nodep->joinType().joinNone() && !nodep->immediateStart())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_ಸಾಲು* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            ಪ್ರತಿ (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                ಯದಿ (!itemp->stmtsp()) ಅನುವರ್ತ;
                AstDelay* ಧ್ರುವ delayp = ನೂತನ AstDelay{
                    fl,
                    ನೂತನ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    ಅಸತ್ಯ};
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
        ಯದಿ (nodep->joinType().join()) {
            iterateAndNextNull(nodep->forksp());
            ಪ್ರತ್ಯಾಗಮ;
        }

        std::vector<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_inFork);
            m_inFork = ಸತ್ಯ;
            ಪ್ರತಿ (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                ಯದಿ (taskify(itemp)) wrappedp.ತಳ್ಳು_back(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        ಪ್ರತಿ (AstBegin* ಧ್ರುವ beginp : wrappedp) iterateAndNextNull(beginp);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (m_inFork) m_forkLocalsp.ಸೇರಿಸು(nodep);
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (!m_inFork) ಪ್ರತ್ಯಾಗಮ;
        AstVar* ಧ್ರುವ varp = nodep->varp();
        // Not sure why this is OK ...
        ಯದಿ (!varp->isFuncLocal() && varp->isClassMember()) ಪ್ರತ್ಯಾಗಮ;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        ಯದಿ (varp->lifetime().isStatic()) ಪ್ರತ್ಯಾಗಮ;
        // If this ref is to a variable that will move into the task, then nothing to do
        ಯದಿ (m_forkLocalsp.ಎಣಿಕೆ(varp)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_ಮೌಲ್ಯ() || nodep->user2())
            && !m_inInitStmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warnMore()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    ಶೂನ್ಯ visit(AstAssign* nodep) override {
        ಯದಿ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_ಮೌಲ್ಯ()) {
            nodep->lhsp()->user2(ಸತ್ಯ);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitStmt);
        m_inInitStmt = ಸತ್ಯ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstThisRef* nodep) override {}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ForkVisitor(AstNetlist* nodep) { visit(nodep); }
    ~ForkVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Fork class functions

ಶೂನ್ಯ V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("fork_dynscope", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("fork_tasks", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
