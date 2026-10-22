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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Scope.h"

#અન્તર્ભાવ "V3ConstPool.h"

#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

વર્ગ ScopeVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;
    અચલ VNUser3InUse m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    using VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    Ast_ગાંઠ_Procedure* m_procedurep = nullptr;  // Current procedure
    AstScope* m_scopep = nullptr;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = nullptr;  // Cell that instantiates this module
    AstScope* m_aboveScopep = nullptr;  // Scope that instantiates this scope
    તાર્કિક m_last = ખોટું;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    size_t m_lib_ટોચ_Name_Len = 0;

    std::unordered_map<Ast_ગાંઠ_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::vector<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::vector<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    સ્થિર શૂન્ય ગણતરી_Instantiations(Ast_ગાંઠ_Module* modp) {
        modp->user3Inc();
        માટે (Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (અચલ AstCell* અચલ cellp = VN_CAST(stmtp, Cell)) {
                ગણતરી_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    template <typename T_ગાંઠ>
    T_ગાંઠ* cloneOrMove(T_ગાંઠ* nodep) {
        T_ગાંઠ* clonep;
        જો (m_last) {
            nodep->unlinkFrBack();
            clonep = nodep;
        } નહીંતર {
            clonep = nodep->clone_વૃક્ષ(ખોટું);
        }
        nodep->user2p(clonep);
        m_scopep->addBlocksp(clonep);
        પાછા clonep;
    }

    શૂન્ય cleanupVarRefs() {
        માટે (અચલ auto& itr : m_varRefScopes) {
            AstVarRef* અચલ nodep = itr.first;
            AstScope* scopep = itr.second;
            જો (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                અચલ auto it2 = m_classOrPackageScopes.find(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.end(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->second;
            }
            nodep->classOrPackagep(nullptr);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = nullptr;
            AstScope* શોધ_Scopep = scopep;
            જ્યારે (શોધ_Scopep) {
                અચલ auto it3 = m_varScopes.find(std::make_pair(nodep->varp(), શોધ_Scopep));
                જો (it3 != m_varScopes.end()) {
                    varscp = it3->second;
                    તોડો;
                }
                શોધ_Scopep = શોધ_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        Ast_ગાંઠ_Module* અચલ modp = nodep->ટોચ_Modulep();
        જો (!modp) {
            nodep->v3error("No top level module found");
            પાછા;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = nullptr;
        m_aboveScopep = nullptr;
        ગણતરી_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        માટે (AstScope* અચલ scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        cleanupVarRefs();
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_lib_ટોચ_Name_Len);
        // Create required blocks and add to module
        string scopename;
        જો (!m_aboveScopep) {
            scopename = "TOP";
        } નહીંતર {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            જો (!v3Global.opt.libCreate().empty() && m_aboveScopep->is_ટોચ()
                && VN_IS(nodep, Module)) {
                m_lib_ટોચ_Name_Len = scopename.size();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_ગાંઠ::user1_Clear_વૃક્ષ();

        m_scopep = નવું AstScope{
            (m_aboveCellp ? static_cast<Ast_ગાંઠ*>(m_aboveCellp) : static_cast<Ast_ગાંઠ*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        જો (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        માટે (Ast_ગાંઠ* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            જો (AstCell* અચલ cellp = VN_CAST(cellnextp, Cell)) cells.ધકેલો_back(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        માટે (AstCell* અચલ cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_ગાંઠ_Module* અચલ modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                જો (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_ગાંઠ::user1_Clear_વૃક્ષ();
        m_modp = nodep;
        જો (m_modp->is_ટોચ()) {
            v3Global.rootp()->create_ટોચ_Scope(m_scopep);
        } નહીંતર {
            m_scopes.ધકેલો_back(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_last);
            અચલ uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_last = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    શૂન્ય visit(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        જો (!m_aboveScopep) {
            scopename = "TOP";
        } નહીંતર {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_ગાંઠ::user1_Clear_વૃક્ષ();

        અચલ Ast_ગાંઠ* અચલ abovep
            = (m_aboveCellp ? static_cast<Ast_ગાંઠ*>(m_aboveCellp) : static_cast<Ast_ગાંઠ*>(nodep));
        m_scopep
            = નવું AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_ગાંઠ::user1_Clear_વૃક્ષ();
        m_scopes.ધકેલો_back(m_scopep);

        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCellInline* nodep) override {  //
        જો (!v3Global.opt.vpi()) પાછા;
        m_scopep->addInlinesp(
            નવું AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    શૂન્ય visit(AstActive* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    શૂન્ય visit(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    શૂન્ય visit(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    શૂન્ય visit(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* અચલ clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_ગાંઠ_FTask* અચલ clonep = [&]() {
            VL_RESTORER(m_last);
            // Only one scope will be created, so avoid pointless cloning
            જો (nodep->classMethod()) m_last = સાચું;
            પાછા cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    શૂન્ય visit(AstVar* nodep) override {
        // Make new scope variable
        જો (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            જો (અચલ AstIfaceRefDType* અચલ ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                જો (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* અચલ varscp = નવું AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            જો (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(ખોટું);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->addVarsp(varscp);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_back(nodep, m_scopep);
    }
    શૂન્ય visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        જો (m_lib_ટોચ_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_ટોચ_Name_Len);
        અચલ string prefix = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(prefix + nodep->scopeAttr());
        nodep->scopeEntr(prefix + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_ટોચ(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = મૂળભૂત;
};

//######################################################################
// Scope cleanup -- remove unused activates

વર્ગ ScopeCleanupVisitor final : જાહેર VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope we are building

    // METHODS

    // VISITORS
    શૂન્ય visit(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    આભાસી શૂન્ય movedDeleteOrIterate(Ast_ગાંઠ* nodep) {
        જો (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } નહીંતર {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    શૂન્ય visit(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    શૂન્ય visit(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    શૂન્ય visit(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    શૂન્ય visit(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    શૂન્ય visit(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(nullptr);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        જો (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_ગાંઠ_FTask* અચલ newp = VN_AS(nodep->taskp()->user2p(), ગાંઠ_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } નહીંતર જો (!VN_IS(nodep, MethodCall)) {
            nodep->taskp(nullptr);
            VIsCached::clear_Cache_વૃક્ષ();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(nullptr);
        iterateChildren(nodep);
    }

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = મૂળભૂત;
};

//######################################################################
// Scope class functions

શૂન્ય V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        અચલ ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::setScoped();
    V3Global::dump_Check_Global_વૃક્ષ("scope", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
