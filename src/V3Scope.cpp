// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity block domains
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
// V3Scope's Transformations:
//
//      For every CELL that references this module, create a
//              SCOPE
//                      {all blocked statements}
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Scope.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

ਵਰਗ ScopeVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    ਵਰਤੋਂ VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    Ast_ਗੰਢ_Procedure* m_procedurep = ਖਾਲੀ_ਸੂਚਕ;  // Current procedure
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = ਖਾਲੀ_ਸੂਚਕ;  // Cell that instantiates this module
    AstScope* m_aboveScopep = ਖਾਲੀ_ਸੂਚਕ;  // Scope that instantiates this scope
    ਤਾਰਕਿਕ m_ਆਖਰੀ = ਝੂਠ;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    ਮਾਪ_t m_lib_ਸਿਖਰ_Name_Len = 0;

    std::unordered_map<Ast_ਗੰਢ_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::ਸਦਿਸ਼<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::ਸਦਿਸ਼<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    ਸਥਿਰ ਖਾਲੀ ਗਿਣਤੀ_Instantiations(Ast_ਗੰਢ_Module* modp) {
        modp->user3Inc();
        ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (ਅਚਲ AstCell* ਅਚਲ cellp = VN_CAST(stmtp, Cell)) {
                ਗਿਣਤੀ_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਗੰਢ>
    T_ਗੰਢ* cloneOrMove(T_ਗੰਢ* nodep) {
        T_ਗੰਢ* clonep;
        ਜੇ (m_ਆਖਰੀ) {
            nodep->unlink_Fr_ਪਿੱਠ();
            clonep = nodep;
        } ਨਹੀਂਤਾਂ {
            clonep = nodep->clone_ਰੁੱਖ(ਝੂਠ);
        }
        nodep->user2p(clonep);
        m_scopep->ਜੋੜੋ_Blocksp(clonep);
        ਵਾਪਸ clonep;
    }

    ਖਾਲੀ cleanupVarRefs() {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_varRefScopes) {
            AstVarRef* ਅਚਲ nodep = itr.ਪਹਿਲਾ;
            AstScope* scopep = itr.ਦੂਜਾ;
            ਜੇ (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                ਅਚਲ ਸਵੈਚਾਲਿਤ it2 = m_classOrPackageScopes.ਲੱਭੋ(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.ਸਿਰਾ(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->ਦੂਜਾ;
            }
            nodep->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = ਖਾਲੀ_ਸੂਚਕ;
            AstScope* ਖੋਜ_Scopep = scopep;
            ਜਦੋਂ (ਖੋਜ_Scopep) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ it3 = m_varScopes.ਲੱਭੋ(std::make_pair(nodep->varp(), ਖੋਜ_Scopep));
                ਜੇ (it3 != m_varScopes.ਸਿਰਾ()) {
                    varscp = it3->ਦੂਜਾ;
                    ਰੋਕੋ;
                }
                ਖੋਜ_Scopep = ਖੋਜ_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        Ast_ਗੰਢ_Module* ਅਚਲ modp = nodep->ਸਿਖਰ_Modulep();
        ਜੇ (!modp) {
            nodep->v3error("No top level module found");
            ਵਾਪਸ;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = ਖਾਲੀ_ਸੂਚਕ;
        m_aboveScopep = ਖਾਲੀ_ਸੂਚਕ;
        ਗਿਣਤੀ_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        ਲਈ (AstScope* ਅਚਲ scopep : m_scopes) scopep->modp()->ਜੋੜੋ_Stmtsp(scopep);
        cleanupVarRefs();
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_lib_ਸਿਖਰ_Name_Len);
        // Create required blocks and add to module
        string scopename;
        ਜੇ (!m_aboveScopep) {
            scopename = "TOP";
        } ਨਹੀਂਤਾਂ {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ() && m_aboveScopep->is_ਸਿਖਰ()
                && VN_IS(nodep, Module)) {
                m_lib_ਸਿਖਰ_Name_Len = scopename.ਮਾਪ();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();

        m_scopep = ਨਵਾਂ AstScope{
            (m_aboveCellp ? static_cast<Ast_ਗੰਢ*>(m_aboveCellp) : static_cast<Ast_ਗੰਢ*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        ਜੇ (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        ਲਈ (Ast_ਗੰਢ* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            ਜੇ (AstCell* ਅਚਲ cellp = VN_CAST(cellnextp, Cell)) cells.ਧੱਕੋ_ਪਿੱਠ(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        ਲਈ (AstCell* ਅਚਲ cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_ਗੰਢ_Module* ਅਚਲ modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                ਜੇ (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();
        m_modp = nodep;
        ਜੇ (m_modp->is_ਸਿਖਰ()) {
            v3Global.rootp()->create_ਸਿਖਰ_Scope(m_scopep);
        } ਨਹੀਂਤਾਂ {
            m_scopes.ਧੱਕੋ_ਪਿੱਠ(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_ਆਖਰੀ);
            ਅਚਲ uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_ਆਖਰੀ = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        ਜੇ (!m_aboveScopep) {
            scopename = "TOP";
        } ਨਹੀਂਤਾਂ {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();

        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ abovep
            = (m_aboveCellp ? static_cast<Ast_ਗੰਢ*>(m_aboveCellp) : static_cast<Ast_ਗੰਢ*>(nodep));
        m_scopep
            = ਨਵਾਂ AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();
        m_scopes.ਧੱਕੋ_ਪਿੱਠ(m_scopep);

        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCellInline* nodep) override {  //
        ਜੇ (!v3Global.opt.vpi()) ਵਾਪਸ;
        m_scopep->ਜੋੜੋ_Inlinesp(
            ਨਵਾਂ AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* ਅਚਲ clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_ਗੰਢ_FTask* ਅਚਲ clonep = [&]() {
            VL_RESTORER(m_ਆਖਰੀ);
            // Only one scope will be created, so avoid pointless cloning
            ਜੇ (nodep->class_ਵਿਧੀ()) m_ਆਖਰੀ = ਸੱਚ;
            ਵਾਪਸ cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        // Make new scope variable
        ਜੇ (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                ਜੇ (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* ਅਚਲ varscp = ਨਵਾਂ AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            ਜੇ (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(ਝੂਠ);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->ਜੋੜੋ_Varsp(varscp);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_ਪਿੱਠ(nodep, m_scopep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        ਜੇ (m_lib_ਸਿਖਰ_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_ਸਿਖਰ_Name_Len);
        ਅਚਲ string ਅਗੇਤਰ = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(ਅਗੇਤਰ + nodep->scopeAttr());
        nodep->scopeEntr(ਅਗੇਤਰ + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_ਸਿਖਰ(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = ਮੂਲ;
};

//######################################################################
// Scope cleanup -- remove unused activates

ਵਰਗ ScopeCleanupVisitor final : ਜਨਤਕ VNVisitor {
    // STATE
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope we are building

    // METHODS

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ਆਭਾਸੀ ਖਾਲੀ movedDeleteOrIterate(Ast_ਗੰਢ* nodep) {
        ਜੇ (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    ਖਾਲੀ ਦੇਖੋ(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        ਜੇ (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_ਗੰਢ_FTask* ਅਚਲ newp = VN_AS(nodep->taskp()->user2p(), ਗੰਢ_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(nodep, ਵਿਧੀ_Call)) {
            nodep->taskp(ਖਾਲੀ_ਸੂਚਕ);
            VIsCached::clear_Cache_ਰੁੱਖ();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(ਖਾਲੀ_ਸੂਚਕ);
        iterateChildren(nodep);
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = ਮੂਲ;
};

//######################################################################
// Scope class functions

ਖਾਲੀ V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ਅਚਲ ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::ਸੈੱਟ_Scoped();
    V3Global::dump_Check_Global_ਰੁੱਖ("scope", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
