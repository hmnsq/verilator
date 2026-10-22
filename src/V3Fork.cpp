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
    AstClass* m_classp = ಶೂನ್ಯಸೂಚಕ;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = ಶೂನ್ಯಸೂಚಕ;  // RefDType for the above
    AstVar* m_handlep = ಶೂನ್ಯಸೂಚಕ;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    ತಾರ್ಕಿಕ initialized() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_classp != ಶೂನ್ಯಸೂಚಕ; }
};

ವರ್ಗ Fork_Dyn_Scope_ಚೌಕಟ್ಟು final {
    // MEMBERS
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // Module to insert the scope into
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_capture_ಕ್ರಮ;  // Variables to be moved into the dynscope
    std::ಗಣ<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_class_id;  // Dynscope class ID
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_id;  // Dynscope ID

ಸಾರ್ವಜನಿಕ:
    Fork_Dyn_Scope_ಚೌಕಟ್ಟು(Ast_ಗಂಟು_Module* modp, Ast_ಗಂಟು* procp, ಗಾತ್ರ_t class_id, ಗಾತ್ರ_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = ಹೊಸ AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = ಹೊಸ AstClassRefDType{m_procp->fileline(), m_instance.m_classp, ಶೂನ್ಯಸೂಚಕ};
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = ಹೊಸ AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(ಸುಳ್ಳು);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        ಹಿಂತಿರುಗಿ m_instance;
    }

    ಸ್ಥಿರಾಂಕ ForkDynScopeInstance& instance() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_instance; }
    ಶೂನ್ಯ capture_Var_ಸೇರಿಸು(AstVar* varp) {
        ಸ್ವಯಂಚಾಲಿತ r = m_captures.emplace(varp);
        ಆದರೆ (r.ಎರಡನೇ) m_capture_ಕ್ರಮ.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
    }
    ತಾರ್ಕಿಕ captured(AstVar* varp) { ಹಿಂತಿರುಗಿ m_captures.ಎಣಿಕೆ_2(varp) != 0; }
    Ast_ಗಂಟು* procp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_procp; }

    ಶೂನ್ಯ populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        ಗಾಗಿ (AstVar* varp : m_capture_ಕ್ರಮ) {
            ಆದರೆ (varp->direction().isAny()) {
                varp = varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                varp->direction(VDirection::NONE);
            } ಇಲ್ಲದಿದ್ದರೆ {
                varp->unlink_Fr_ಹಿಂಭಾಗ();
            }
            varp->funcLocal(ಸುಳ್ಳು);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->ಬಳಸಿದ_Loop_Idx(ಸುಳ್ಳು);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->ಕೂಡಿಸು_Stmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstFunc{m_instance.m_classp->fileline(), "new", ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        newp->isConstructor(ನಿಜ);
        newp->class_ವಿಧಾನ(ನಿಜ);
        newp->dtypep(newp->ಕಂಡುಹಿಡಿ_Void_DType());
        m_instance.m_classp->ಕೂಡಿಸು_Stmtsp(newp);
    }

    ಶೂನ್ಯ linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        ಆದರೆ (AstFork* ಸ್ಥಿರಾಂಕ forkp = VN_CAST(m_procp, Fork)) {
            link_Nodes_ದ_Fork(memberMap, forkp);
            ಹಿಂತಿರುಗಿ;
        }

        Ast_ಗಂಟು* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ(&stmtpHandle);

        // Find node after last variable declaration
        Ast_ಗಂಟು* initp = stmtp;
        ಆಗ (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.ಕಂಡುಹಿಡಿ_Member(m_instance.m_classp, "new"), ಗಂಟು_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ asgnp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            m_procp->fileline(),
            ಹೊಸ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_ಗಂಟು* initsp = ಶೂನ್ಯಸೂಚಕ;  // Arguments need to be copied
        ಗಾಗಿ (AstVar* varp : m_capture_ಕ್ರಮ) {
            ಆದರೆ (!varp->direction().isAny()) ಮುಂದುವರಿ;

            ಆದರೆ (varp->direction().isNonOutput()) {
                AstMemberSel* ಸ್ಥಿರಾಂಕ memberselp = ಹೊಸ AstMemberSel{
                    varp->fileline(),
                    ಹೊಸ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.ಕಂಡುಹಿಡಿ_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_ಗಂಟು* initAsgnp
                    = ಹೊಸ Ast_ನಿಯೋಜಿಸು{varp->fileline(), memberselp,
                                    ಹೊಸ AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(initsp, initAsgnp);
            }

            ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.ಗಣ_Assigns_Events();
                ಆದರೆ (dtypep->isEvent()) ಮುಂದುವರಿ;
            }

            ಆದರೆ (varp->direction().isWritable()) {
                AstMemberSel* ಸ್ಥಿರಾಂಕ memberselp = ಹೊಸ AstMemberSel{
                    varp->fileline(),
                    ಹೊಸ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.ಕಂಡುಹಿಡಿ_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_ಗಂಟು* writebackAsgnp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                    varp->fileline(), ಹೊಸ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtp, writebackAsgnp);
            }
        }
        ಆದರೆ (initsp) Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(asgnp, initsp);

        ಆದರೆ (initp != stmtp) {
            initp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(asgnp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(asgnp, static_cast<Ast_ಗಂಟು*>(initp));
            stmtp = asgnp;
        }

        Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(static_cast<Ast_ಗಂಟು*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->ಕೂಡಿಸು_Stmtsp(m_instance.m_classp);
    }

    ತಾರ್ಕಿಕ linked() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_instance.initialized() && m_instance.m_handlep->backp(); }

ಖಾಸಗಿ:
    Ast_ನಿಯೋಜಿಸು* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.ಕಂಡುಹಿಡಿ_Member(m_instance.m_classp, "new"), ಗಂಟು_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            m_procp->fileline(),
            ಹೊಸ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    ಶೂನ್ಯ link_Nodes_ದ_Fork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ಸ್ಥಿರಾಂಕ std::string name = "_Vwrapped_"  //
                                 + (forkp->name().ಖಾಲಿಗಳು() ? "" : forkp->name() + "_")  //
                                 + std::ಗಮ್ಯ_string(m_id);
        AstBegin* ಸ್ಥಿರಾಂಕ beginp = ಹೊಸ AstBegin{forkp->fileline(), name, m_instance.m_handlep, ನಿಜ};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->ಕೂಡಿಸು_Stmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        ಆದರೆ (forkp->stmtsp()) beginp->ಕೂಡಿಸು_Stmtsp(forkp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        // Put the Fork back at the end of the Begin
        beginp->ಕೂಡಿಸು_Stmtsp(forkp);
        m_modp->ಕೂಡಿಸು_Stmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { ಹಿಂತಿರುಗಿ "__VDynScope_" + cvt_ಗಮ್ಯ_Str(m_class_id); }

    string generateDynScopeHandleName(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* fromp) {
        ಹಿಂತಿರುಗಿ "__VDynScope_" + (!fromp->name().ಖಾಲಿಗಳು() ? (fromp->name() + "_") : "ANON_")
               + cvt_ಗಮ್ಯ_Str(m_id);
    }

    Ast_ಗಂಟು* getProcStmts() {
        Ast_ಗಂಟು* stmtsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!m_procp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ taskp = VN_CAST(m_procp, ಗಂಟು_FTask)) {
            stmtsp = taskp->stmtsp();
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        ಹಿಂತಿರುಗಿ stmtsp;
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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // STATE
    ತಾರ್ಕಿಕ m_ಒಳ_Func = ಸುಳ್ಳು;  // True if in a function
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Module we are currently under
    Ast_ಗಂಟು* m_procp = ಶೂನ್ಯಸೂಚಕ;  // Function/task/block we are currently under
    std::deque<Ast_ಗಂಟು*> m_ಚೌಕಟ್ಟು_ಕ್ರಮ;  // Ordered list of frames (for determinism)
    std::map<Ast_ಗಂಟು*, Fork_Dyn_Scope_ಚೌಕಟ್ಟು*> m_ಚೌಕಟ್ಟುಗಳು;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_fork_ಆಳ = 0;  // Number of asynchronous forks we are currently under
    ತಾರ್ಕಿಕ m_afterTimingControl = ಸುಳ್ಳು;  // A timing control might've be executed in the current
                                        // process
    ಗಾತ್ರ_t m_id = 0;  // Unique ID for a frame
    ಗಾತ್ರ_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    Fork_Dyn_Scope_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟು_ದ(Ast_ಗಂಟು* nodep) {
        ಸ್ವಯಂಚಾಲಿತ ಚೌಕಟ್ಟು_It = m_ಚೌಕಟ್ಟುಗಳು.ಕಂಡುಹಿಡಿ(nodep);
        ಆದರೆ (ಚೌಕಟ್ಟು_It == m_ಚೌಕಟ್ಟುಗಳು.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ ಚೌಕಟ್ಟು_It->ಎರಡನೇ;
    }

    ಸ್ಥಿರಾಂಕ Fork_Dyn_Scope_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟು_ದ(Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
        ಸ್ವಯಂಚಾಲಿತ ಚೌಕಟ್ಟು_It = m_ಚೌಕಟ್ಟುಗಳು.ಕಂಡುಹಿಡಿ(nodep);
        ಆದರೆ (ಚೌಕಟ್ಟು_It == m_ಚೌಕಟ್ಟುಗಳು.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ ಚೌಕಟ್ಟು_It->ಎರಡನೇ;
    }

    Fork_Dyn_Scope_ಚೌಕಟ್ಟು* ತಳ್ಳು_Dyn_Scope_ಚೌಕಟ್ಟು(Ast_ಗಂಟು* procp) {
        Fork_Dyn_Scope_ಚೌಕಟ್ಟು* ಸ್ಥಿರಾಂಕ framep
            = ಹೊಸ Fork_Dyn_Scope_ಚೌಕಟ್ಟು{m_modp, procp, m_class_id++, m_id++};
        ಸ್ವಯಂಚಾಲಿತ r = m_ಚೌಕಟ್ಟುಗಳು.emplace(procp, framep);
        UASSERT_OBJ(r.ಎರಡನೇ, m_modp, "Procedure already contains a frame");
        m_ಚೌಕಟ್ಟು_ಕ್ರಮ.ತಳ್ಳು_ಹಿಂಭಾಗ(procp);
        ಹಿಂತಿರುಗಿ framep;
    }

    ಶೂನ್ಯ replaceWithMemberSel(AstVarRef* refp, ಸ್ಥಿರಾಂಕ ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlink_Fr_ಹಿಂಭಾಗ(&handle);
        AstMemberSel* ಸ್ಥಿರಾಂಕ membersel = ಹೊಸ AstMemberSel{
            refp->fileline(), ಹೊಸ AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        ಆದರೆ (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(dynScope.m_classp, refp->varp()->name()), Var));
        } ಇಲ್ಲದಿದ್ದರೆ {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasAsyncFork(Ast_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ nodep->exists([](AstFork* forkp) { ಹಿಂತಿರುಗಿ !forkp->joinType().join(); })
               || nodep->exists([](Ast_ನಿಯೋಜಿಸು_Dly*) { ಹಿಂತಿರುಗಿ ನಿಜ; });
    }

    ಶೂನ್ಯ bind_ಗಂಟು_ಗಮ್ಯ_Dyn_Scope(Ast_ಗಂಟು* nodep, Fork_Dyn_Scope_ಚೌಕಟ್ಟು* framep) {
        ಸ್ವಯಂಚಾಲಿತ r = m_ಚೌಕಟ್ಟುಗಳು.emplace(nodep, framep);
        ಆದರೆ (r.ಎರಡನೇ) m_ಚೌಕಟ್ಟು_ಕ್ರಮ.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
    }
    ಶೂನ್ಯ bindInitIterate(Ast_ಗಂಟು* stmtsp, Fork_Dyn_Scope_ಚೌಕಟ್ಟು* framep) {
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ asgnp = VN_CAST(stmtp, ನಿಯೋಜಿಸು)) {
                bind_ಗಂಟು_ಗಮ್ಯ_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    ತಾರ್ಕಿಕ needsDynScope(ಸ್ಥಿರಾಂಕ AstVarRef* refp) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
        ಹಿಂತಿರುಗಿ
            // Can this variable escape the scope
            ((m_fork_ಆಳ > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_ಮೌಲ್ಯ() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        ಆದರೆ (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_ಒಳ_Func);
        m_ಒಳ_Func = VN_IS(nodep, Func);
        ಆದರೆ (hasAsyncFork(nodep)) ತಳ್ಳು_Dyn_Scope_ಚೌಕಟ್ಟು(m_procp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        ಆದರೆ (hasAsyncFork(nodep)) ತಳ್ಳು_Dyn_Scope_ಚೌಕಟ್ಟು(m_procp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        VL_RESTORER(m_fork_ಆಳ);
        ಆದರೆ (!nodep->joinType().join()) ++m_fork_ಆಳ;

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ oldAfterTimingControl = m_afterTimingControl;

        Fork_Dyn_Scope_ಚೌಕಟ್ಟು* framep = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->declsp() || nodep->stmtsp()) framep = ತಳ್ಳು_Dyn_Scope_ಚೌಕಟ್ಟು(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        ಗಾಗಿ (Ast_ಗಂಟು* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            ಆದರೆ (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                ಆದರೆ (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_ಸೇರಿಸು(varp);
                bind_ಗಂಟು_ಗಮ್ಯ_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = ಸುಳ್ಳು;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        ಆದರೆ (nodep->isTimingControl()) m_afterTimingControl = ನಿಜ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = ನಿಜ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        nodep->user1(m_fork_ಆಳ);
        Fork_Dyn_Scope_ಚೌಕಟ್ಟು* ಸ್ಥಿರಾಂಕ framep = ಚೌಕಟ್ಟು_ದ(m_procp);
        ಆದರೆ (!framep) ಹಿಂತಿರುಗಿ;  // Cannot be legally referenced from a fork
        bind_ಗಂಟು_ಗಮ್ಯ_Dyn_Scope(nodep, framep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        Fork_Dyn_Scope_ಚೌಕಟ್ಟು* ಸ್ಥಿರಾಂಕ framep = ಚೌಕಟ್ಟು_ದ(nodep->varp());
        ಆದರೆ (!framep) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (needsDynScope(nodep)) {
            ತಾರ್ಕಿಕ isEvent = ಸುಳ್ಳು;
            ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.ಗಣ_Assigns_Events();
                isEvent = dtypep->isEvent();
            }
            ಆದರೆ (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_ಒಳ_Func ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            ಆದರೆ (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_ಸೇರಿಸು(nodep->varp());
        }
        bind_ಗಂಟು_ಗಮ್ಯ_Dyn_Scope(nodep, framep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_ಮೌಲ್ಯ()) {
            nodep->lhsp()->user2(ನಿಜ);
        }
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        ಆದರೆ (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    ಹಿಂತಿರುಗಿ refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(ನಿಜ);
            // Put it in a fork to prevent lifetime issues with the local
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            AstFork* ಸ್ಥಿರಾಂಕ forkp = ಹೊಸ AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->ಕೂಡಿಸು_Forksp(ಹೊಸ AstBegin{flp, "", nodep, ಸುಳ್ಳು});
            UINFO(9, "assign new fork " << forkp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (nodep->isTimingControl()) m_afterTimingControl = ನಿಜ;
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        ಭೇಟಿ(nodep);

        // Commit changes to AST
        ತಾರ್ಕಿಕ typesAdded = ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ orderp : m_ಚೌಕಟ್ಟು_ಕ್ರಮ) {
            UINFO(9, "Frame commit " << orderp);
            ಸ್ವಯಂಚಾಲಿತ ಚೌಕಟ್ಟು_It = m_ಚೌಕಟ್ಟುಗಳು.ಕಂಡುಹಿಡಿ(orderp);
            UASSERT_OBJ(ಚೌಕಟ್ಟು_It != m_ಚೌಕಟ್ಟುಗಳು.ಕೊನೆ(), orderp, "m_frames didn't contain m_frameOrder");
            Fork_Dyn_Scope_ಚೌಕಟ್ಟು* framep = ಚೌಕಟ್ಟು_It->ಎರಡನೇ;
            ಆದರೆ (!framep->instance().initialized()) ಮುಂದುವರಿ;
            ಆದರೆ (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = ನಿಜ;
            }
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(ಚೌಕಟ್ಟು_It->ಮೊದಲ, VarRef)) {
                ಆದರೆ (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        ಆದರೆ (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::ಗಣ<Fork_Dyn_Scope_ಚೌಕಟ್ಟು*> ಚೌಕಟ್ಟುಗಳು;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ ಗಂಟು_ಚೌಕಟ್ಟು : m_ಚೌಕಟ್ಟುಗಳು) ಚೌಕಟ್ಟುಗಳು.ಸೇರಿಸು(ಗಂಟು_ಚೌಕಟ್ಟು.ಎರಡನೇ);
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ* framep : ಚೌಕಟ್ಟುಗಳು) ಅಳಿಸು framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

ವರ್ಗ ForkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // STATE - for current AstNodeModule
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Class/module we are currently under
    AstTask* m_tasksp = ಶೂನ್ಯಸೂಚಕ;  // Tasks exrtracted under current module
    ಗಾತ್ರ_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    ತಾರ್ಕಿಕ m_ಒಳ_Fork = ಸುಳ್ಳು;  // Traversal in an async fork
    ತಾರ್ಕಿಕ m_hasNestedFork = ಸುಳ್ಳು;  // Branch being taskified contains a nested fork
    ತಾರ್ಕಿಕ m_ಒಳ_Init_Stmt = ಸುಳ್ಳು;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Enclosing function/task
    std::ಗಣ<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = ಶೂನ್ಯಸೂಚಕ;  // Local copies of captured variables
    AstArg* m_capturedArgsp = ಶೂನ್ಯಸೂಚಕ;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ಆದರೆ (varp->name() == refp->name()) ನಿಲ್ಲಿಸು;
        }
        ಆದರೆ (varp) ಹಿಂತಿರುಗಿ varp;

        // Create a local copy to capture
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = refp->fileline();
        varp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::ಒಳಹರಿವು);
        varp->funcLocal(ನಿಜ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ಸ್ಥಿರಾಂಕ argp = ಹೊಸ AstArg{flp, refp->name(), refp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
        m_capturedArgsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_capturedArgsp, argp);
        ಹಿಂತಿರುಗಿ varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    ತಾರ್ಕಿಕ taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = ಸುಳ್ಳು;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = ಶೂನ್ಯಸೂಚಕ;
        m_capturedArgsp = ಶೂನ್ಯಸೂಚಕ;
        iterate(beginp);

        // No need to do it if no variabels are captured
        ಆದರೆ (m_forkLocalsp.ಖಾಲಿಗಳು() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Create task holding the statement and repalce statement with call to that task
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = beginp->fileline();
        ಸ್ಥಿರಾಂಕ std::string name = "__VforkTask_" + std::ಗಮ್ಯ_string(m_nForkTasks++);
        AstTask* ಸ್ಥಿರಾಂಕ taskp = ಹೊಸ AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        ಆದರೆ (m_hasNestedFork) {
            taskp->class_ವಿಧಾನ(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_tasksp, taskp);
        ಆದರೆ (beginp->declsp()) taskp->ಕೂಡಿಸು_Stmtsp(beginp->declsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        ಆದರೆ (beginp->stmtsp()) taskp->ಕೂಡಿಸು_Stmtsp(beginp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        AstTaskRef* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ localp : m_forkLocalsp) localp->funcLocal(ನಿಜ);

        // We did wrap the body
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isForkJoinNoneSentinelDelay(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        ಸ್ಥಿರಾಂಕ AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_CAST(nodep, Delay);
        ಆದರೆ (!delayp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(delayp->lhsp(), Const);
        ಹಿಂತಿರುಗಿ constp && (constp->ಗಮ್ಯ_UQuad() == std::numeric_limits<uint64_t>::max());
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Disable_ಸರತಿ_ತಳ್ಳು_Self_ಪೂರ್ವಪ್ರತ್ಯಯ(Ast_ಗಂಟು* nodep) {
        ಆಗ (Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ jumpBlockp = VN_CAST(nodep, Jump_ಖಂಡ)) {
            nodep = jumpBlockp->stmtsp();
        }
        ಹಿಂತಿರುಗಿ nodep && nodep->is_Disable_ಸರತಿ_ತಳ್ಳು_Self_Stmt();
    }
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Owner>
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಸೇರಿಸು_Fork_Sentinel_After_Disable_ಸರತಿ_Pushes(T_Owner* ಸ್ಥಿರಾಂಕ ownerp,
                                                          Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Stmtp,
                                                          Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ delayp) {
        Ast_ಗಂಟು* ಸೇರಿಸು_Beforep = ಮೊದಲ_Stmtp;
        ಆಗ (ಸೇರಿಸು_Beforep && ಸೇರಿಸು_Beforep->is_Disable_ಸರತಿ_ತಳ್ಳು_Self_Stmt()) {
            ಸೇರಿಸು_Beforep = ಸೇರಿಸು_Beforep->nextp();
        }
        ಆದರೆ (Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ jumpBlockp = VN_CAST(ಸೇರಿಸು_Beforep, Jump_ಖಂಡ)) {
            ಆದರೆ (ಸೇರಿಸು_Fork_Sentinel_After_Disable_ಸರತಿ_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಆದರೆ (ಸೇರಿಸು_Beforep == ಮೊದಲ_Stmtp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (ಸೇರಿಸು_Beforep) {
            ಸೇರಿಸು_Beforep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(delayp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ownerp->ಕೂಡಿಸು_Stmtsp(delayp);
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ move_Fork_Sentinel_After_Disable_ಸರತಿ_Pushes(AstBegin* ಸ್ಥಿರಾಂಕ beginp) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Stmtp = beginp->stmtsp();
        ಆದರೆ (!isForkJoinNoneSentinelDelay(ಮೊದಲ_Stmtp)) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ afterSentinelp = ಮೊದಲ_Stmtp->nextp();
        ಆದರೆ (!is_Disable_ಸರತಿ_ತಳ್ಳು_Self_ಪೂರ್ವಪ್ರತ್ಯಯ(afterSentinelp)) ಹಿಂತಿರುಗಿ;

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ delayp = ಮೊದಲ_Stmtp->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ moved
            = ಸೇರಿಸು_Fork_Sentinel_After_Disable_ಸರತಿ_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ forkIsDisableable(AstFork* ಸ್ಥಿರಾಂಕ nodep) {
        ಗಾಗಿ (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            ಆದರೆ (is_Disable_ಸರತಿ_ತಳ್ಳು_Self_ಪೂರ್ವಪ್ರತ್ಯಯ(itemp->stmtsp())) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = ಶೂನ್ಯಸೂಚಕ;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        ಆದರೆ (m_tasksp) nodep->ಕೂಡಿಸು_Stmtsp(m_tasksp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        m_hasNestedFork = ನಿಜ;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        ಆದರೆ ((nodep->joinType().joinNone() && !nodep->ತಕ್ಷಣ_ಆರಂಭ())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_ಸಾಲು* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            ಗಾಗಿ (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                ಆದರೆ (!itemp->stmtsp()) ಮುಂದುವರಿ;
                AstDelay* ಸ್ಥಿರಾಂಕ delayp = ಹೊಸ AstDelay{
                    fl,
                    ಹೊಸ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    ಸುಳ್ಳು};
                itemp->prependStmtsp(delayp);
                move_Fork_Sentinel_After_Disable_ಸರತಿ_Pushes(itemp);
            }
        }

        iterate_And_ಮುಂದಿನ_Null(nodep->declsp());
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        ಆದರೆ (nodep->joinType().join()) {
            iterate_And_ಮುಂದಿನ_Null(nodep->forksp());
            ಹಿಂತಿರುಗಿ;
        }

        std::ಸದಿಶ<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_ಒಳ_Fork);
            m_ಒಳ_Fork = ನಿಜ;
            ಗಾಗಿ (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                ಆದರೆ (taskify(itemp)) wrappedp.ತಳ್ಳು_ಹಿಂಭಾಗ(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        ಗಾಗಿ (AstBegin* ಸ್ಥಿರಾಂಕ beginp : wrappedp) iterate_And_ಮುಂದಿನ_Null(beginp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (m_ಒಳ_Fork) m_forkLocalsp.ಸೇರಿಸು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (!m_ಒಳ_Fork) ಹಿಂತಿರುಗಿ;
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        // Not sure why this is OK ...
        ಆದರೆ (!varp->isFuncLocal() && varp->isClassMember()) ಹಿಂತಿರುಗಿ;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        ಆದರೆ (varp->lifetime().isStatic()) ಹಿಂತಿರುಗಿ;
        // If this ref is to a variable that will move into the task, then nothing to do
        ಆದರೆ (m_forkLocalsp.ಎಣಿಕೆ_2(varp)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_ಮೌಲ್ಯ() || nodep->user2())
            && !m_ಒಳ_Init_Stmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warn_ಇನ್ನಷ್ಟು()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            ಹಿಂತಿರುಗಿ;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_ಮೌಲ್ಯ()) {
            nodep->lhsp()->user2(ನಿಜ);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_ಒಳ_Init_Stmt);
        m_ಒಳ_Init_Stmt = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_ಒಳ_Init_Stmt);
        m_ಒಳ_Init_Stmt = ನಿಜ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstThisRef* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ForkVisitor(AstNetlist* nodep) { ಭೇಟಿ(nodep); }
    ~ForkVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
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
