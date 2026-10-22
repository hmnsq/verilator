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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Scope.h"

#അന്തര്ഭാവ "V3ConstPool.h"

#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

വർഗ്ഗം ScopeVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;
    സ്ഥിരം VNUser3InUse m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    using VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    Ast_കണ്ണി_Procedure* m_procedurep = nullptr;  // Current procedure
    AstScope* m_scopep = nullptr;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = nullptr;  // Cell that instantiates this module
    AstScope* m_aboveScopep = nullptr;  // Scope that instantiates this scope
    തർക്കം m_last = അസത്യം;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    size_t m_lib_മുകൾ_Name_Len = 0;

    std::unordered_map<Ast_കണ്ണി_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::vector<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::vector<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    സ്ഥിതം ശൂന്യം എണ്ണം_Instantiations(Ast_കണ്ണി_Module* modp) {
        modp->user3Inc();
        വേണ്ടി (Ast_കണ്ണി* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstCell* സ്ഥിരം cellp = VN_CAST(stmtp, Cell)) {
                എണ്ണം_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    template <typename T_കണ്ണി>
    T_കണ്ണി* cloneOrMove(T_കണ്ണി* nodep) {
        T_കണ്ണി* clonep;
        എങ്കിൽ (m_last) {
            nodep->unlinkFrBack();
            clonep = nodep;
        } അല്ലെങ്കിൽ {
            clonep = nodep->clone_വൃക്ഷം(അസത്യം);
        }
        nodep->user2p(clonep);
        m_scopep->addBlocksp(clonep);
        തിരികെ clonep;
    }

    ശൂന്യം cleanupVarRefs() {
        വേണ്ടി (സ്ഥിരം auto& itr : m_varRefScopes) {
            AstVarRef* സ്ഥിരം nodep = itr.first;
            AstScope* scopep = itr.second;
            എങ്കിൽ (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                സ്ഥിരം auto it2 = m_classOrPackageScopes.find(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.end(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->second;
            }
            nodep->classOrPackagep(nullptr);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = nullptr;
            AstScope* തിരയൽ_Scopep = scopep;
            ആകുമ്പോൾ (തിരയൽ_Scopep) {
                സ്ഥിരം auto it3 = m_varScopes.find(std::make_pair(nodep->varp(), തിരയൽ_Scopep));
                എങ്കിൽ (it3 != m_varScopes.end()) {
                    varscp = it3->second;
                    നിർത്തുക;
                }
                തിരയൽ_Scopep = തിരയൽ_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        Ast_കണ്ണി_Module* സ്ഥിരം modp = nodep->മുകൾ_Modulep();
        എങ്കിൽ (!modp) {
            nodep->v3error("No top level module found");
            തിരികെ;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = nullptr;
        m_aboveScopep = nullptr;
        എണ്ണം_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        വേണ്ടി (AstScope* സ്ഥിരം scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        cleanupVarRefs();
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_lib_മുകൾ_Name_Len);
        // Create required blocks and add to module
        string scopename;
        എങ്കിൽ (!m_aboveScopep) {
            scopename = "TOP";
        } അല്ലെങ്കിൽ {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            എങ്കിൽ (!v3Global.opt.libCreate().empty() && m_aboveScopep->is_മുകൾ()
                && VN_IS(nodep, Module)) {
                m_lib_മുകൾ_Name_Len = scopename.size();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_കണ്ണി::user1_Clear_വൃക്ഷം();

        m_scopep = പുതിയ AstScope{
            (m_aboveCellp ? static_cast<Ast_കണ്ണി*>(m_aboveCellp) : static_cast<Ast_കണ്ണി*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        എങ്കിൽ (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        വേണ്ടി (Ast_കണ്ണി* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            എങ്കിൽ (AstCell* സ്ഥിരം cellp = VN_CAST(cellnextp, Cell)) cells.തള്ളുക_back(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        വേണ്ടി (AstCell* സ്ഥിരം cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_കണ്ണി_Module* സ്ഥിരം modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                എങ്കിൽ (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_കണ്ണി::user1_Clear_വൃക്ഷം();
        m_modp = nodep;
        എങ്കിൽ (m_modp->is_മുകൾ()) {
            v3Global.rootp()->create_മുകൾ_Scope(m_scopep);
        } അല്ലെങ്കിൽ {
            m_scopes.തള്ളുക_back(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_last);
            സ്ഥിരം uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_last = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    ശൂന്യം visit(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        എങ്കിൽ (!m_aboveScopep) {
            scopename = "TOP";
        } അല്ലെങ്കിൽ {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_കണ്ണി::user1_Clear_വൃക്ഷം();

        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം abovep
            = (m_aboveCellp ? static_cast<Ast_കണ്ണി*>(m_aboveCellp) : static_cast<Ast_കണ്ണി*>(nodep));
        m_scopep
            = പുതിയ AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_കണ്ണി::user1_Clear_വൃക്ഷം();
        m_scopes.തള്ളുക_back(m_scopep);

        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCellInline* nodep) override {  //
        എങ്കിൽ (!v3Global.opt.vpi()) തിരികെ;
        m_scopep->addInlinesp(
            പുതിയ AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    ശൂന്യം visit(AstActive* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ശൂന്യം visit(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ശൂന്യം visit(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ശൂന്യം visit(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* സ്ഥിരം clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_കണ്ണി_FTask* സ്ഥിരം clonep = [&]() {
            VL_RESTORER(m_last);
            // Only one scope will be created, so avoid pointless cloning
            എങ്കിൽ (nodep->classMethod()) m_last = സത്യം;
            തിരികെ cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    ശൂന്യം visit(AstVar* nodep) override {
        // Make new scope variable
        എങ്കിൽ (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                എങ്കിൽ (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* സ്ഥിരം varscp = പുതിയ AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            എങ്കിൽ (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(അസത്യം);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->addVarsp(varscp);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_back(nodep, m_scopep);
    }
    ശൂന്യം visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        എങ്കിൽ (m_lib_മുകൾ_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_മുകൾ_Name_Len);
        സ്ഥിരം string prefix = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(prefix + nodep->scopeAttr());
        nodep->scopeEntr(prefix + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_മുകൾ(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Scope cleanup -- remove unused activates

വർഗ്ഗം ScopeCleanupVisitor final : പൊതു VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope we are building

    // METHODS

    // VISITORS
    ശൂന്യം visit(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    മായാ ശൂന്യം movedDeleteOrIterate(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    ശൂന്യം visit(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    ശൂന്യം visit(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    ശൂന്യം visit(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    ശൂന്യം visit(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    ശൂന്യം visit(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(nullptr);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        എങ്കിൽ (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_കണ്ണി_FTask* സ്ഥിരം newp = VN_AS(nodep->taskp()->user2p(), കണ്ണി_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(nodep, MethodCall)) {
            nodep->taskp(nullptr);
            VIsCached::clear_Cache_വൃക്ഷം();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(nullptr);
        iterateChildren(nodep);
    }

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Scope class functions

ശൂന്യം V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        സ്ഥിരം ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::setScoped();
    V3Global::dump_Check_Global_വൃക്ഷം("scope", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
