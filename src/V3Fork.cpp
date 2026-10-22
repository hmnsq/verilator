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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Fork.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstNodeExpr.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ ForkDynScopeInstance final {
ਜਨਤਕ:
    AstClass* m_classp = ਖਾਲੀ_ਸੂਚਕ;  // Class for holding variables of dynamic scope
    AstClassRefDType* m_refDTypep = ਖਾਲੀ_ਸੂਚਕ;  // RefDType for the above
    AstVar* m_handlep = ਖਾਲੀ_ਸੂਚਕ;  // Class handle for holding variables of dynamic scope

    // True if the instance exists
    ਤਾਰਕਿਕ initialized() ਅਚਲ { ਵਾਪਸ m_classp != ਖਾਲੀ_ਸੂਚਕ; }
};

ਵਰਗ Fork_Dyn_Scope_ਚੌਖਟਾ final {
    // MEMBERS
    Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // Module to insert the scope into
    Ast_ਗੰਢ* ਅਚਲ m_procp;  // Procedure/block associated with that dynscope
    std::deque<AstVar*> m_capture_ਕ੍ਰਮ;  // Variables to be moved into the dynscope
    std::ਸੈੱਟ<AstVar*> m_captures;  // Variables to be moved into the dynscope
    ForkDynScopeInstance m_instance;  // Nodes to be injected into the AST to create the dynscope
    ਅਚਲ ਮਾਪ_t m_class_id;  // Dynscope class ID
    ਅਚਲ ਮਾਪ_t m_id;  // Dynscope ID

ਜਨਤਕ:
    Fork_Dyn_Scope_ਚੌਖਟਾ(Ast_ਗੰਢ_Module* modp, Ast_ਗੰਢ* procp, ਮਾਪ_t class_id, ਮਾਪ_t id)
        : m_modp{modp}
        , m_procp{procp}
        , m_class_id{class_id}
        , m_id{id} {}

    ForkDynScopeInstance& createInstancePrototype() {
        UASSERT_OBJ(!m_instance.initialized(), m_procp, "Dynamic scope already instantiated.");

        m_instance.m_classp
            = ਨਵਾਂ AstClass{m_procp->fileline(), generateDynScopeClassName(), m_modp->libname()};
        UINFO(9, "new dynscope class " << m_instance.m_classp);
        m_instance.m_refDTypep
            = ਨਵਾਂ AstClassRefDType{m_procp->fileline(), m_instance.m_classp, ਖਾਲੀ_ਸੂਚਕ};
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(m_instance.m_refDTypep);
        m_instance.m_handlep
            = ਨਵਾਂ AstVar{m_procp->fileline(), VVarType::BLOCKTEMP,
                         generateDynScopeHandleName(m_procp), m_instance.m_refDTypep};
        m_instance.m_handlep->funcLocal(ਝੂਠ);
        m_instance.m_handlep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        UINFO(9, "new dynscope var " << m_instance.m_handlep);

        ਵਾਪਸ m_instance;
    }

    ਅਚਲ ForkDynScopeInstance& instance() ਅਚਲ { ਵਾਪਸ m_instance; }
    ਖਾਲੀ capture_Var_ਪਾਓ(AstVar* varp) {
        ਸਵੈਚਾਲਿਤ r = m_captures.emplace(varp);
        ਜੇ (r.ਦੂਜਾ) m_capture_ਕ੍ਰਮ.ਧੱਕੋ_ਪਿੱਠ(varp);
    }
    ਤਾਰਕਿਕ captured(AstVar* varp) { ਵਾਪਸ m_captures.ਗਿਣਤੀ_2(varp) != 0; }
    Ast_ਗੰਢ* procp() ਅਚਲ { ਵਾਪਸ m_procp; }

    ਖਾਲੀ populateClass() {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No DynScope prototype");

        // Move variables into the class
        ਲਈ (AstVar* varp : m_capture_ਕ੍ਰਮ) {
            ਜੇ (varp->direction().isAny()) {
                varp = varp->clone_ਰੁੱਖ(ਝੂਠ);
                varp->direction(VDirection::NONE);
            } ਨਹੀਂਤਾਂ {
                varp->unlink_Fr_ਪਿੱਠ();
            }
            varp->funcLocal(ਝੂਠ);
            varp->varType(VVarType::MEMBER);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->ਵਰਤਿਆ_Loop_Idx(ਝੂਠ);  // No longer unrollable
            UINFO(9, "insert DynScope member " << varp);
            m_instance.m_classp->ਜੋੜੋ_Stmtsp(varp);
        }

        // Create class's constructor
        AstFunc* ਅਚਲ newp
            = ਨਵਾਂ AstFunc{m_instance.m_classp->fileline(), "new", ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        newp->isConstructor(ਸੱਚ);
        newp->class_ਵਿਧੀ(ਸੱਚ);
        newp->dtypep(newp->ਲੱਭੋ_Void_DType());
        m_instance.m_classp->ਜੋੜੋ_Stmtsp(newp);
    }

    ਖਾਲੀ linkNodes(VMemberMap& memberMap) {
        UASSERT_OBJ(m_instance.initialized(), m_procp, "No dynamic scope prototype");
        UASSERT_OBJ(!linked(), m_instance.m_handlep, "Handle already linked");

        ਜੇ (AstFork* ਅਚਲ forkp = VN_CAST(m_procp, Fork)) {
            link_Nodes_ਦਾ_Fork(memberMap, forkp);
            ਵਾਪਸ;
        }

        Ast_ਗੰਢ* stmtp = getProcStmts();
        UASSERT(stmtp, "trying to instantiate dynamic scope while not under proc");
        VNRelinker stmtpHandle;
        stmtp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(&stmtpHandle);

        // Find node after last variable declaration
        Ast_ਗੰਢ* initp = stmtp;
        ਜਦੋਂ (initp && VN_IS(initp, Var)) initp = initp->nextp();
        UASSERT(stmtp, "Procedure lacks body");
        UASSERT(initp, "Procedure lacks statements besides declarations");

        AstNew* ਅਚਲ newp = ਨਵਾਂ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.ਲੱਭੋ_Member(m_instance.m_classp, "new"), ਗੰਢ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        Ast_ਗੰਢ* ਅਚਲ asgnp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            m_procp->fileline(),
            ਨਵਾਂ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};

        Ast_ਗੰਢ* initsp = ਖਾਲੀ_ਸੂਚਕ;  // Arguments need to be copied
        ਲਈ (AstVar* varp : m_capture_ਕ੍ਰਮ) {
            ਜੇ (!varp->direction().isAny()) ਜਾਰੀ;

            ਜੇ (varp->direction().isNonOutput()) {
                AstMemberSel* ਅਚਲ memberselp = ਨਵਾਂ AstMemberSel{
                    varp->fileline(),
                    ਨਵਾਂ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::WRITE},
                    VN_AS(memberMap.ਲੱਭੋ_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_ਗੰਢ* initAsgnp
                    = ਨਵਾਂ Ast_ਸੌਂਪੋ{varp->fileline(), memberselp,
                                    ਨਵਾਂ AstVarRef{varp->fileline(), varp, VAccess::READ}};
                initsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(initsp, initAsgnp);
            }

            ਜੇ (AstBasicDType* ਅਚਲ dtypep = VN_CAST(varp->dtypep()->skipRefp(), BasicDType)) {
                v3Global.ਸੈੱਟ_Assigns_Events();
                ਜੇ (dtypep->isEvent()) ਜਾਰੀ;
            }

            ਜੇ (varp->direction().isWritable()) {
                AstMemberSel* ਅਚਲ memberselp = ਨਵਾਂ AstMemberSel{
                    varp->fileline(),
                    ਨਵਾਂ AstVarRef{varp->fileline(), m_instance.m_handlep, VAccess::READ},
                    VN_AS(memberMap.ਲੱਭੋ_Member(m_instance.m_classp, varp->name()), Var)};
                Ast_ਗੰਢ* writebackAsgnp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    varp->fileline(), ਨਵਾਂ AstVarRef{varp->fileline(), varp, VAccess::WRITE},
                    memberselp};
                stmtp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtp, writebackAsgnp);
            }
        }
        ਜੇ (initsp) Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(asgnp, initsp);

        ਜੇ (initp != stmtp) {
            initp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(asgnp);
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(asgnp, static_cast<Ast_ਗੰਢ*>(initp));
            stmtp = asgnp;
        }

        Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(static_cast<Ast_ਗੰਢ*>(m_instance.m_handlep), stmtp);
        stmtpHandle.relink(m_instance.m_handlep);
        m_modp->ਜੋੜੋ_Stmtsp(m_instance.m_classp);
    }

    ਤਾਰਕਿਕ linked() ਅਚਲ { ਵਾਪਸ m_instance.initialized() && m_instance.m_handlep->backp(); }

ਨਿਜੀ:
    Ast_ਸੌਂਪੋ* instantiateDynScope(VMemberMap& memberMap) {
        AstNew* ਅਚਲ newp = ਨਵਾਂ AstNew{m_procp->fileline()};
        newp->taskp(VN_AS(memberMap.ਲੱਭੋ_Member(m_instance.m_classp, "new"), ਗੰਢ_FTask));
        newp->dtypep(m_instance.m_refDTypep);
        newp->classOrPackagep(m_instance.m_classp);

        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{
            m_procp->fileline(),
            ਨਵਾਂ AstVarRef{m_procp->fileline(), m_instance.m_handlep, VAccess::WRITE}, newp};
    }

    // Wrap Fork in Begin
    ਖਾਲੀ link_Nodes_ਦਾ_Fork(VMemberMap& memberMap, AstFork* forkp) {
        // Replace the Fork with a Begin
        ਅਚਲ std::string name = "_Vwrapped_"  //
                                 + (forkp->name().ਖਾਲੀ_ਥਾਵਾਂ() ? "" : forkp->name() + "_")  //
                                 + std::ਮੰਜ਼ਿਲ_string(m_id);
        AstBegin* ਅਚਲ beginp = ਨਵਾਂ AstBegin{forkp->fileline(), name, m_instance.m_handlep, ਸੱਚ};
        forkp->replaceWith(beginp);
        // Create the dynamic scope in the Begin
        beginp->ਜੋੜੋ_Stmtsp(instantiateDynScope(memberMap));
        // Move all sequential statements there
        ਜੇ (forkp->stmtsp()) beginp->ਜੋੜੋ_Stmtsp(forkp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        // Put the Fork back at the end of the Begin
        beginp->ਜੋੜੋ_Stmtsp(forkp);
        m_modp->ਜੋੜੋ_Stmtsp(m_instance.m_classp);
    }

    string generateDynScopeClassName() { ਵਾਪਸ "__VDynScope_" + cvt_ਮੰਜ਼ਿਲ_Str(m_class_id); }

    string generateDynScopeHandleName(ਅਚਲ Ast_ਗੰਢ* fromp) {
        ਵਾਪਸ "__VDynScope_" + (!fromp->name().ਖਾਲੀ_ਥਾਵਾਂ() ? (fromp->name() + "_") : "ANON_")
               + cvt_ਮੰਜ਼ਿਲ_Str(m_id);
    }

    Ast_ਗੰਢ* getProcStmts() {
        Ast_ਗੰਢ* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!m_procp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (AstBegin* ਅਚਲ beginp = VN_CAST(m_procp, Begin)) {
            stmtsp = beginp->stmtsp();
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ taskp = VN_CAST(m_procp, ਗੰਢ_FTask)) {
            stmtsp = taskp->stmtsp();
        } ਨਹੀਂਤਾਂ {
            m_procp->v3fatalSrc("m_procp is not a begin block or a procedure");
        }
        ਵਾਪਸ stmtsp;
    }
};

//######################################################################
// Dynamic scope visitor, creates classes and objects for dynamic scoping of variables and
// replaces references to variables that need a dynamic scope with references to object's
// members

ਵਰਗ DynScopeVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstVar::user1()          -> uint64_t.  timing-control fork nesting level of that variable
    // AstVarRef::user2()       -> bool. Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    // AstAssignDly::user2()    -> bool.  Already visited
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // STATE
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Func = ਝੂਠ;  // True if in a function
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Module we are currently under
    Ast_ਗੰਢ* m_procp = ਖਾਲੀ_ਸੂਚਕ;  // Function/task/block we are currently under
    std::deque<Ast_ਗੰਢ*> m_ਚੌਖਟਾ_ਕ੍ਰਮ;  // Ordered list of frames (for determinism)
    std::map<Ast_ਗੰਢ*, Fork_Dyn_Scope_ਚੌਖਟਾ*> m_ਚੌਖਟੇ;  // Map nodes to related DynScopeFrames
    VMemberMap m_memberMap;  // Class member look-up
    uint64_t m_fork_ਡੂੰਘਾਈ = 0;  // Number of asynchronous forks we are currently under
    ਤਾਰਕਿਕ m_afterTimingControl = ਝੂਠ;  // A timing control might've be executed in the current
                                        // process
    ਮਾਪ_t m_id = 0;  // Unique ID for a frame
    ਮਾਪ_t m_class_id = 0;  // Unique ID for a frame class

    // METHODS

    Fork_Dyn_Scope_ਚੌਖਟਾ* ਚੌਖਟਾ_ਦਾ(Ast_ਗੰਢ* nodep) {
        ਸਵੈਚਾਲਿਤ ਚੌਖਟਾ_It = m_ਚੌਖਟੇ.ਲੱਭੋ(nodep);
        ਜੇ (ਚੌਖਟਾ_It == m_ਚੌਖਟੇ.ਸਿਰਾ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ ਚੌਖਟਾ_It->ਦੂਜਾ;
    }

    ਅਚਲ Fork_Dyn_Scope_ਚੌਖਟਾ* ਚੌਖਟਾ_ਦਾ(Ast_ਗੰਢ* nodep) ਅਚਲ {
        ਸਵੈਚਾਲਿਤ ਚੌਖਟਾ_It = m_ਚੌਖਟੇ.ਲੱਭੋ(nodep);
        ਜੇ (ਚੌਖਟਾ_It == m_ਚੌਖਟੇ.ਸਿਰਾ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ ਚੌਖਟਾ_It->ਦੂਜਾ;
    }

    Fork_Dyn_Scope_ਚੌਖਟਾ* ਧੱਕੋ_Dyn_Scope_ਚੌਖਟਾ(Ast_ਗੰਢ* procp) {
        Fork_Dyn_Scope_ਚੌਖਟਾ* ਅਚਲ framep
            = ਨਵਾਂ Fork_Dyn_Scope_ਚੌਖਟਾ{m_modp, procp, m_class_id++, m_id++};
        ਸਵੈਚਾਲਿਤ r = m_ਚੌਖਟੇ.emplace(procp, framep);
        UASSERT_OBJ(r.ਦੂਜਾ, m_modp, "Procedure already contains a frame");
        m_ਚੌਖਟਾ_ਕ੍ਰਮ.ਧੱਕੋ_ਪਿੱਠ(procp);
        ਵਾਪਸ framep;
    }

    ਖਾਲੀ replaceWithMemberSel(AstVarRef* refp, ਅਚਲ ForkDynScopeInstance& dynScope) {
        VNRelinker handle;
        refp->unlink_Fr_ਪਿੱਠ(&handle);
        AstMemberSel* ਅਚਲ membersel = ਨਵਾਂ AstMemberSel{
            refp->fileline(), ਨਵਾਂ AstVarRef{refp->fileline(), dynScope.m_handlep, refp->access()},
            refp->varp()};
        ਜੇ (refp->varp()->direction().isAny()) {
            membersel->varp(
                VN_AS(m_memberMap.ਲੱਭੋ_Member(dynScope.m_classp, refp->varp()->name()), Var));
        } ਨਹੀਂਤਾਂ {
            membersel->varp(refp->varp());
        }
        handle.relink(membersel);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
    }

    ਸਥਿਰ ਤਾਰਕਿਕ hasAsyncFork(Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ nodep->exists([](AstFork* forkp) { ਵਾਪਸ !forkp->joinType().join(); })
               || nodep->exists([](Ast_ਸੌਂਪੋ_Dly*) { ਵਾਪਸ ਸੱਚ; });
    }

    ਖਾਲੀ bind_ਗੰਢ_ਮੰਜ਼ਿਲ_Dyn_Scope(Ast_ਗੰਢ* nodep, Fork_Dyn_Scope_ਚੌਖਟਾ* framep) {
        ਸਵੈਚਾਲਿਤ r = m_ਚੌਖਟੇ.emplace(nodep, framep);
        ਜੇ (r.ਦੂਜਾ) m_ਚੌਖਟਾ_ਕ੍ਰਮ.ਧੱਕੋ_ਪਿੱਠ(nodep);
    }
    ਖਾਲੀ bindInitIterate(Ast_ਗੰਢ* stmtsp, Fork_Dyn_Scope_ਚੌਖਟਾ* framep) {
        ਲਈ (Ast_ਗੰਢ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (Ast_ਸੌਂਪੋ* ਅਚਲ asgnp = VN_CAST(stmtp, ਸੌਂਪੋ)) {
                bind_ਗੰਢ_ਮੰਜ਼ਿਲ_Dyn_Scope(asgnp->lhsp(), framep);
                iterate(asgnp->rhsp());
            } ਨਹੀਂਤਾਂ ਜੇ (AstInitialAutomaticStmt* astmtp
                       = VN_CAST(stmtp, InitialAutomaticStmt)) {  // Moves in V3Begin
                // Underlying assign RHS might use function argument, so can't just
                // move whole thing into the new class's constructor/statements
                bindInitIterate(astmtp->stmtsp(), framep);
            } ਨਹੀਂਤਾਂ ਜੇ (AstInitialStaticStmt* astmtp
                       = VN_CAST(stmtp, InitialStaticStmt)) {  // Moves in V3Begin
                bindInitIterate(astmtp->stmtsp(), framep);
            } ਨਹੀਂਤਾਂ {
                stmtp->v3fatalSrc("Invalid node under block item initialization part of fork");
            }
        }
    }

    ਤਾਰਕਿਕ needsDynScope(ਅਚਲ AstVarRef* refp) ਅਚਲ {
        ਅਚਲ AstVar* ਅਚਲ varp = refp->varp();
        ਵਾਪਸ
            // Can this variable escape the scope
            ((m_fork_ਡੂੰਘਾਈ > varp->user1()) && varp->isFuncLocal())
            && varp->lifetime().isAutomatic()
            && (
                // Is it mutated
                (varp->is_Class_Handle_ਮੁੱਲ() ? refp->user2() : refp->access().isWriteOrRW())
                // Or is it after a timing-control event
                || m_afterTimingControl);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        ਜੇ (!VN_IS(nodep, Class)) m_modp = nodep;
        VL_RESTORER(m_id);
        m_id = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        VL_RESTORER(m_ਵਿੱਚ_Func);
        m_ਵਿੱਚ_Func = VN_IS(nodep, Func);
        ਜੇ (hasAsyncFork(nodep)) ਧੱਕੋ_Dyn_Scope_ਚੌਖਟਾ(m_procp);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        ਜੇ (hasAsyncFork(nodep)) ਧੱਕੋ_Dyn_Scope_ਚੌਖਟਾ(m_procp);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        VL_RESTORER(m_fork_ਡੂੰਘਾਈ);
        ਜੇ (!nodep->joinType().join()) ++m_fork_ਡੂੰਘਾਈ;

        ਅਚਲ ਤਾਰਕਿਕ oldAfterTimingControl = m_afterTimingControl;

        Fork_Dyn_Scope_ਚੌਖਟਾ* framep = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->declsp() || nodep->stmtsp()) framep = ਧੱਕੋ_Dyn_Scope_ਚੌਖਟਾ(nodep);

        // This can be probably optimized to detect cases in which dynscopes could be avoided
        ਲਈ (Ast_ਗੰਢ* declp = nodep->declsp(); declp; declp = declp->nextp()) {
            AstVar* ਅਚਲ varp = VN_CAST(declp, Var);
            UASSERT_OBJ(varp, declp, "Invalid node under block item initialization part of fork");
            UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
            ਜੇ (varp->lifetime().isAutomatic()) {  // else V3Begin will move later
                ਜੇ (!framep->instance().initialized()) framep->createInstancePrototype();
                framep->capture_Var_ਪਾਓ(varp);
                bind_ਗੰਢ_ਮੰਜ਼ਿਲ_Dyn_Scope(varp, framep);
            }
        }
        bindInitIterate(nodep->stmtsp(), framep);

        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            m_afterTimingControl = ਝੂਠ;
            iterate(stmtp);
        }
        m_afterTimingControl = oldAfterTimingControl;
        ਜੇ (nodep->isTimingControl()) m_afterTimingControl = ਸੱਚ;
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(nodep));
        // We are before V3Timing, so unfortunately we need to treat any calls as suspending,
        // just to be safe. This might be improved if we could propagate suspendability
        // before doing all the other timing-related stuff.
        m_afterTimingControl = ਸੱਚ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        nodep->user1(m_fork_ਡੂੰਘਾਈ);
        Fork_Dyn_Scope_ਚੌਖਟਾ* ਅਚਲ framep = ਚੌਖਟਾ_ਦਾ(m_procp);
        ਜੇ (!framep) ਵਾਪਸ;  // Cannot be legally referenced from a fork
        bind_ਗੰਢ_ਮੰਜ਼ਿਲ_Dyn_Scope(nodep, framep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        Fork_Dyn_Scope_ਚੌਖਟਾ* ਅਚਲ framep = ਚੌਖਟਾ_ਦਾ(nodep->varp());
        ਜੇ (!framep) ਵਾਪਸ;
        ਜੇ (needsDynScope(nodep)) {
            ਤਾਰਕਿਕ isEvent = ਝੂਠ;
            ਜੇ (AstBasicDType* ਅਚਲ dtypep = VN_CAST(nodep->dtypep()->skipRefp(), BasicDType)) {
                v3Global.ਸੈੱਟ_Assigns_Events();
                isEvent = dtypep->isEvent();
            }
            ਜੇ (!isEvent && m_afterTimingControl && nodep->varp()->isWritable()
                && nodep->access().isWriteOrRW()) {
                // The output variable may not exist after a delay, so we can't just write to it
                nodep->v3error(
                    "Writing to an "
                    << nodep->varp()->verilogKwd() << " automatic variable of a "
                    << (m_ਵਿੱਚ_Func ? "function" : "task")
                    << " after a timing control is not allowed (IEEE 1800-2023 13.2.2)");
            }
            ਜੇ (!framep->instance().initialized()) framep->createInstancePrototype();
            framep->capture_Var_ਪਾਓ(nodep->varp());
        }
        bind_ਗੰਢ_ਮੰਜ਼ਿਲ_Dyn_Scope(nodep, framep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override {
        ਜੇ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_ਮੁੱਲ()) {
            nodep->lhsp()->user2(ਸੱਚ);
        }
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Stmt*>(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        ਜੇ (m_procp && !nodep->user2()  // Unhandled AssignDly in function/task
            && nodep->lhsp()->exists(  // And writes to a local variable
                [](AstVarRef* refp) {
                    ਵਾਪਸ refp->access().isWriteOrRW() && refp->varp()->isFuncLocal();
                })) {
            nodep->user2(ਸੱਚ);
            // Put it in a fork to prevent lifetime issues with the local
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            AstFork* ਅਚਲ forkp = ਨਵਾਂ AstFork{flp, VJoinType::JOIN_NONE};
            nodep->replaceWith(forkp);
            forkp->ਜੋੜੋ_Forksp(ਨਵਾਂ AstBegin{flp, "", nodep, ਝੂਠ});
            UINFO(9, "assign new fork " << forkp);
        } ਨਹੀਂਤਾਂ {
            ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Stmt*>(nodep));
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (nodep->isTimingControl()) m_afterTimingControl = ਸੱਚ;
        iterateChildren(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ DynScopeVisitor(AstNetlist* nodep) {
        // Create Dynamic scope class prototypes and objects
        ਦੇਖੋ(nodep);

        // Commit changes to AST
        ਤਾਰਕਿਕ typesAdded = ਝੂਠ;
        ਲਈ (ਸਵੈਚਾਲਿਤ orderp : m_ਚੌਖਟਾ_ਕ੍ਰਮ) {
            UINFO(9, "Frame commit " << orderp);
            ਸਵੈਚਾਲਿਤ ਚੌਖਟਾ_It = m_ਚੌਖਟੇ.ਲੱਭੋ(orderp);
            UASSERT_OBJ(ਚੌਖਟਾ_It != m_ਚੌਖਟੇ.ਸਿਰਾ(), orderp, "m_frames didn't contain m_frameOrder");
            Fork_Dyn_Scope_ਚੌਖਟਾ* framep = ਚੌਖਟਾ_It->ਦੂਜਾ;
            ਜੇ (!framep->instance().initialized()) ਜਾਰੀ;
            ਜੇ (!framep->linked()) {
                framep->populateClass();
                framep->linkNodes(m_memberMap);
                typesAdded = ਸੱਚ;
            }
            ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(ਚੌਖਟਾ_It->ਪਹਿਲਾ, VarRef)) {
                ਜੇ (framep->captured(refp->varp())) replaceWithMemberSel(refp, framep->instance());
            }
        }

        ਜੇ (typesAdded) v3Global.rootp()->typeTablep()->repairCache();
    }
    ~DynScopeVisitor() override {
        std::ਸੈੱਟ<Fork_Dyn_Scope_ਚੌਖਟਾ*> ਚੌਖਟੇ;
        ਲਈ (ਸਵੈਚਾਲਿਤ ਗੰਢ_ਚੌਖਟਾ : m_ਚੌਖਟੇ) ਚੌਖਟੇ.ਪਾਓ(ਗੰਢ_ਚੌਖਟਾ.ਦੂਜਾ);
        ਲਈ (ਸਵੈਚਾਲਿਤ* framep : ਚੌਖਟੇ) ਮਿਟਾਓ framep;
    }
};

//######################################################################
// Fork visitor, transforms asynchronous blocks into separate tasks

ਵਰਗ ForkVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // STATE - for current AstNodeModule
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Class/module we are currently under
    AstTask* m_tasksp = ਖਾਲੀ_ਸੂਚਕ;  // Tasks exrtracted under current module
    ਮਾਪ_t m_nForkTasks = 0;  // Sequence numbers for task names

    // STATE - for current AstFork item
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Fork = ਝੂਠ;  // Traversal in an async fork
    ਤਾਰਕਿਕ m_hasNestedFork = ਝੂਠ;  // Branch being taskified contains a nested fork
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Init_Stmt = ਝੂਠ;  // Traversal in InitialStaticStmt/InitialAutomaticStmt
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Enclosing function/task
    std::ਸੈੱਟ<AstVar*> m_forkLocalsp;  // Variables local to a given fork
    AstVar* m_capturedVarsp = ਖਾਲੀ_ਸੂਚਕ;  // Local copies of captured variables
    AstArg* m_capturedArgsp = ਖਾਲੀ_ਸੂਚਕ;  // References to captured variables (as args)

    // METHODS
    AstVar* capture(AstVarRef* refp) {
        AstVar* varp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (varp = m_capturedVarsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ਜੇ (varp->name() == refp->name()) ਰੋਕੋ;
        }
        ਜੇ (varp) ਵਾਪਸ varp;

        // Create a local copy to capture
        File_ਸਤਰ* ਅਚਲ flp = refp->fileline();
        varp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, refp->name(), refp->dtypep()};
        varp->direction(VDirection::ਨਿਵੇਸ਼);
        varp->funcLocal(ਸੱਚ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_capturedVarsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_capturedVarsp, varp);
        // Pass variable as argument
        AstArg* ਅਚਲ argp = ਨਵਾਂ AstArg{flp, refp->name(), refp->clone_ਰੁੱਖ(ਝੂਠ)};
        m_capturedArgsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_capturedArgsp, argp);
        ਵਾਪਸ varp;
    }

    // Wrap body of the given Begin (an AstFork branch), in an AstTask, and
    // replace body with a call to that task. Returns true iff wrapped.
    ਤਾਰਕਿਕ taskify(AstBegin* beginp) {
        // Visit statement to gather variables (And recursively process)
        VL_RESTORER_CLEAR(m_forkLocalsp);
        VL_RESTORER(m_hasNestedFork);
        m_hasNestedFork = ਝੂਠ;
        VL_RESTORER(m_capturedVarsp);
        VL_RESTORER(m_capturedArgsp);
        m_capturedVarsp = ਖਾਲੀ_ਸੂਚਕ;
        m_capturedArgsp = ਖਾਲੀ_ਸੂਚਕ;
        iterate(beginp);

        // No need to do it if no variabels are captured
        ਜੇ (m_forkLocalsp.ਖਾਲੀ_ਥਾਵਾਂ() && !m_capturedVarsp && !v3Global.opt.fTaskifyAll()) ਵਾਪਸ ਝੂਠ;

        // Create task holding the statement and repalce statement with call to that task
        File_ਸਤਰ* ਅਚਲ flp = beginp->fileline();
        ਅਚਲ std::string name = "__VforkTask_" + std::ਮੰਜ਼ਿਲ_string(m_nForkTasks++);
        AstTask* ਅਚਲ taskp = ਨਵਾਂ AstTask{flp, name, m_capturedVarsp};
        // A branch that spawns another process must hand its captured variables over to it
        ਜੇ (m_hasNestedFork) {
            taskp->class_ਵਿਧੀ(VN_IS(m_modp, Class));
            taskp->isStatic(m_ftaskp && m_ftaskp->isStatic());
        }
        m_tasksp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_tasksp, taskp);
        ਜੇ (beginp->declsp()) taskp->ਜੋੜੋ_Stmtsp(beginp->declsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        ਜੇ (beginp->stmtsp()) taskp->ਜੋੜੋ_Stmtsp(beginp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        AstTaskRef* ਅਚਲ callp = ਨਵਾਂ AstTaskRef{flp, taskp, m_capturedArgsp};
        beginp->ਜੋੜੋ_Stmtsp(callp->makeStmt());

        // Variables were moved under the task, so make sure they are marked as funcLocal
        ਲਈ (AstVar* ਅਚਲ localp : m_forkLocalsp) localp->funcLocal(ਸੱਚ);

        // We did wrap the body
        ਵਾਪਸ ਸੱਚ;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ isForkJoinNoneSentinelDelay(ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep) {
        ਅਚਲ AstDelay* ਅਚਲ delayp = VN_CAST(nodep, Delay);
        ਜੇ (!delayp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(delayp->lhsp(), Const);
        ਵਾਪਸ constp && (constp->ਮੰਜ਼ਿਲ_UQuad() == std::numeric_limits<uint64_t>::max());
    }
    ਸਥਿਰ ਤਾਰਕਿਕ is_Disable_ਕਤਾਰ_ਧੱਕੋ_Self_ਅਗੇਤਰ(Ast_ਗੰਢ* nodep) {
        ਜਦੋਂ (Ast_Jump_ਖੰਡ* ਅਚਲ jumpBlockp = VN_CAST(nodep, Jump_ਖੰਡ)) {
            nodep = jumpBlockp->stmtsp();
        }
        ਵਾਪਸ nodep && nodep->is_Disable_ਕਤਾਰ_ਧੱਕੋ_Self_Stmt();
    }
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Owner>
    ਸਥਿਰ ਤਾਰਕਿਕ ਪਾਓ_Fork_Sentinel_After_Disable_ਕਤਾਰ_Pushes(T_Owner* ਅਚਲ ownerp,
                                                          Ast_ਗੰਢ* ਅਚਲ ਪਹਿਲਾ_Stmtp,
                                                          Ast_ਗੰਢ* ਅਚਲ delayp) {
        Ast_ਗੰਢ* ਪਾਓ_Beforep = ਪਹਿਲਾ_Stmtp;
        ਜਦੋਂ (ਪਾਓ_Beforep && ਪਾਓ_Beforep->is_Disable_ਕਤਾਰ_ਧੱਕੋ_Self_Stmt()) {
            ਪਾਓ_Beforep = ਪਾਓ_Beforep->nextp();
        }
        ਜੇ (Ast_Jump_ਖੰਡ* ਅਚਲ jumpBlockp = VN_CAST(ਪਾਓ_Beforep, Jump_ਖੰਡ)) {
            ਜੇ (ਪਾਓ_Fork_Sentinel_After_Disable_ਕਤਾਰ_Pushes(jumpBlockp, jumpBlockp->stmtsp(),
                                                          delayp)) {
                ਵਾਪਸ ਸੱਚ;
            }
        }
        ਜੇ (ਪਾਓ_Beforep == ਪਹਿਲਾ_Stmtp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (ਪਾਓ_Beforep) {
            ਪਾਓ_Beforep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(delayp);
        } ਨਹੀਂਤਾਂ {
            ownerp->ਜੋੜੋ_Stmtsp(delayp);
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਸਥਿਰ ਖਾਲੀ move_Fork_Sentinel_After_Disable_ਕਤਾਰ_Pushes(AstBegin* ਅਚਲ beginp) {
        Ast_ਗੰਢ* ਅਚਲ ਪਹਿਲਾ_Stmtp = beginp->stmtsp();
        ਜੇ (!isForkJoinNoneSentinelDelay(ਪਹਿਲਾ_Stmtp)) ਵਾਪਸ;
        Ast_ਗੰਢ* ਅਚਲ afterSentinelp = ਪਹਿਲਾ_Stmtp->nextp();
        ਜੇ (!is_Disable_ਕਤਾਰ_ਧੱਕੋ_Self_ਅਗੇਤਰ(afterSentinelp)) ਵਾਪਸ;

        Ast_ਗੰਢ* ਅਚਲ delayp = ਪਹਿਲਾ_Stmtp->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ ਤਾਰਕਿਕ moved
            = ਪਾਓ_Fork_Sentinel_After_Disable_ਕਤਾਰ_Pushes(beginp, afterSentinelp, delayp);
        UASSERT_OBJ(moved, beginp, "Failed to move fork sentinel after disable queue pushes");
    }
    ਸਥਿਰ ਤਾਰਕਿਕ forkIsDisableable(AstFork* ਅਚਲ nodep) {
        ਲਈ (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
            ਜੇ (is_Disable_ਕਤਾਰ_ਧੱਕੋ_Self_ਅਗੇਤਰ(itemp->stmtsp())) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_nForkTasks);
        VL_RESTORER(m_tasksp);
        m_modp = nodep;
        m_nForkTasks = 0;
        m_tasksp = ਖਾਲੀ_ਸੂਚਕ;
        iterateChildren(nodep);
        // Add extracted tasks, they don't need to be visited again
        ਜੇ (m_tasksp) nodep->ਜੋੜੋ_Stmtsp(m_tasksp);
    }

    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        m_hasNestedFork = ਸੱਚ;
        // IEEE 1800-2023 9.3.2: In all cases, processes spawned by a fork-join block shall not
        // start executing until the parent process is blocked or terminates. Because join and
        // join_any block the parent process, deferring branch start with a synthetic #0 delay is
        // normally only needed for join_none. A fork that can be disabled by name needs the same
        // deferral for every join type so all branches register their processes before any branch
        // body can disable the block. Compiler-generated immediate-start forks already have the
        // required ordering and must arm their event controls before the parent continues.
        ਜੇ ((nodep->joinType().joinNone() && !nodep->ਤੁਰੰਤ_ਸ਼ੁਰੂ())
            || forkIsDisableable(nodep)) {
            UINFO(9, "Adding fork branch start sentinels " << nodep);
            File_ਸਤਰ* fl = nodep->fileline();
            // We use a sentinel value of UINT64_MAX to mark this delay so that it goes to the
            // ACTIVE region with a delay value of 0.
            ਲਈ (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), Begin);
                ਜੇ (!itemp->stmtsp()) ਜਾਰੀ;
                AstDelay* ਅਚਲ delayp = ਨਵਾਂ AstDelay{
                    fl,
                    ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                    ਝੂਠ};
                itemp->prependStmtsp(delayp);
                move_Fork_Sentinel_After_Disable_ਕਤਾਰ_Pushes(itemp);
            }
        }

        iterate_And_ਅਗਲਾ_Null(nodep->declsp());
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());

        // A plain 'join' blocks the parent until every branch finishes, so a branch cannot
        // outlive the variables it references and need not be extracted into a task; just
        // recurse to process any forks nested inside the branches. join_any and join_none
        // branches may outlive the fork, so each branch body is wrapped in a task that
        // captures the variables it references.
        ਜੇ (nodep->joinType().join()) {
            iterate_And_ਅਗਲਾ_Null(nodep->forksp());
            ਵਾਪਸ;
        }

        std::ਸਦਿਸ਼<AstBegin*> wrappedp;
        {
            VL_RESTORER(m_ਵਿੱਚ_Fork);
            m_ਵਿੱਚ_Fork = ਸੱਚ;
            ਲਈ (AstBegin* itemp = nodep->forksp(); itemp; itemp = VN_AS(itemp->nextp(), Begin)) {
                ਜੇ (taskify(itemp)) wrappedp.ਧੱਕੋ_ਪਿੱਠ(itemp);
            }
        }
        // Analyze replacements in context of enclosing fork
        ਲਈ (AstBegin* ਅਚਲ beginp : wrappedp) iterate_And_ਅਗਲਾ_Null(beginp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (m_ਵਿੱਚ_Fork) m_forkLocalsp.ਪਾਓ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (!m_ਵਿੱਚ_Fork) ਵਾਪਸ;
        AstVar* ਅਚਲ varp = nodep->varp();
        // Not sure why this is OK ...
        ਜੇ (!varp->isFuncLocal() && varp->isClassMember()) ਵਾਪਸ;
        // We must know the lifetime at this point, otherwise we can't decide if need to capture
        UASSERT_OBJ(!varp->lifetime().isNone(), nodep, "Variable's lifetime is unknown");
        // Static variables are fine, they are always availabel
        ਜੇ (varp->lifetime().isStatic()) ਵਾਪਸ;
        // If this ref is to a variable that will move into the task, then nothing to do
        ਜੇ (m_forkLocalsp.ਗਿਣਤੀ_2(varp)) ਵਾਪਸ;

        ਜੇ (nodep->access().isWriteOrRW() && (!nodep->is_Class_Handle_ਮੁੱਲ() || nodep->user2())
            && !m_ਵਿੱਚ_Init_Stmt) {
            nodep->v3warn(
                E_LIFETIME,
                "Invalid reference: Process might outlive variable "
                    << varp->prettyNameQ() << ".\n"
                    << varp->warn_ਹੋਰ()
                    << "... Suggest use it as read-only to initialize a local copy at the "
                       "beginning of the process, or declare it as static. It is also "
                       "possible to refer by reference to objects and their members.");
            ਵਾਪਸ;
        }

        // Capture variable and redirect reference to the capturede copy
        nodep->varp(capture(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override {
        ਜੇ (VN_IS(nodep->lhsp(), VarRef) && nodep->lhsp()->is_Class_Handle_ਮੁੱਲ()) {
            nodep->lhsp()->user2(ਸੱਚ);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Init_Stmt);
        m_ਵਿੱਚ_Init_Stmt = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Init_Stmt);
        m_ਵਿੱਚ_Init_Stmt = ਸੱਚ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstThisRef* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ForkVisitor(AstNetlist* nodep) { ਦੇਖੋ(nodep); }
    ~ForkVisitor() override = ਮੂਲ;
};

//######################################################################
// Fork class functions

ਖਾਲੀ V3Fork::makeDynamicScopes(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DynScopeVisitor{nodep}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("fork_dynscope", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3Fork::makeTasks(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ForkVisitor{nodep}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("fork_tasks", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
