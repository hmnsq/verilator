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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Scope.h"

#अन्तर्भाव "V3ConstPool.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

वग्ग ScopeVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;
    ध्रुव VNUser3InUse m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    using VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
    Ast_ग्रन्थिः_Procedure* m_procedurep = nullptr;  // Current procedure
    AstScope* m_scopep = nullptr;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = nullptr;  // Cell that instantiates this module
    AstScope* m_aboveScopep = nullptr;  // Scope that instantiates this scope
    तार्किक m_last = मिच्छा;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    size_t m_lib_शिखरम्_Name_Len = 0;

    std::unordered_map<Ast_ग्रन्थिः_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::vector<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::vector<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    स्थिर शून्य गणनम्_Instantiations(Ast_ग्रन्थिः_Module* modp) {
        modp->user3Inc();
        अत्थाय (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (ध्रुव AstCell* ध्रुव cellp = VN_CAST(stmtp, Cell)) {
                गणनम्_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    template <typename T_ग्रन्थिः>
    T_ग्रन्थिः* cloneOrMove(T_ग्रन्थिः* nodep) {
        T_ग्रन्थिः* clonep;
        सचे (m_last) {
            nodep->unlinkFrBack();
            clonep = nodep;
        } अञ्ञथा {
            clonep = nodep->clone_वृक्षः(मिच्छा);
        }
        nodep->user2p(clonep);
        m_scopep->addBlocksp(clonep);
        पटिनिवत्त clonep;
    }

    शून्य cleanupVarRefs() {
        अत्थाय (ध्रुव auto& itr : m_varRefScopes) {
            AstVarRef* ध्रुव nodep = itr.first;
            AstScope* scopep = itr.second;
            सचे (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                ध्रुव auto it2 = m_classOrPackageScopes.find(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.end(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->second;
            }
            nodep->classOrPackagep(nullptr);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = nullptr;
            AstScope* अन्वेषणम्_Scopep = scopep;
            याव (अन्वेषणम्_Scopep) {
                ध्रुव auto it3 = m_varScopes.find(std::make_pair(nodep->varp(), अन्वेषणम्_Scopep));
                सचे (it3 != m_varScopes.end()) {
                    varscp = it3->second;
                    भङ्ग;
                }
                अन्वेषणम्_Scopep = अन्वेषणम्_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        Ast_ग्रन्थिः_Module* ध्रुव modp = nodep->शिखरम्_Modulep();
        सचे (!modp) {
            nodep->v3error("No top level module found");
            पटिनिवत्त;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = nullptr;
        m_aboveScopep = nullptr;
        गणनम्_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        अत्थाय (AstScope* ध्रुव scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        cleanupVarRefs();
    }
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_lib_शिखरम्_Name_Len);
        // Create required blocks and add to module
        string scopename;
        सचे (!m_aboveScopep) {
            scopename = "TOP";
        } अञ्ञथा {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            सचे (!v3Global.opt.libCreate().empty() && m_aboveScopep->is_शिखरम्()
                && VN_IS(nodep, Module)) {
                m_lib_शिखरम्_Name_Len = scopename.size();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_ग्रन्थिः::user1_Clear_वृक्षः();

        m_scopep = नूतन AstScope{
            (m_aboveCellp ? static_cast<Ast_ग्रन्थिः*>(m_aboveCellp) : static_cast<Ast_ग्रन्थिः*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        सचे (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        अत्थाय (Ast_ग्रन्थिः* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            सचे (AstCell* ध्रुव cellp = VN_CAST(cellnextp, Cell)) cells.प्रक्षिप_back(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        अत्थाय (AstCell* ध्रुव cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_ग्रन्थिः_Module* ध्रुव modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                सचे (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_ग्रन्थिः::user1_Clear_वृक्षः();
        m_modp = nodep;
        सचे (m_modp->is_शिखरम्()) {
            v3Global.rootp()->create_शिखरम्_Scope(m_scopep);
        } अञ्ञथा {
            m_scopes.प्रक्षिप_back(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_last);
            ध्रुव uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_last = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    शून्य visit(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        सचे (!m_aboveScopep) {
            scopename = "TOP";
        } अञ्ञथा {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_ग्रन्थिः::user1_Clear_वृक्षः();

        ध्रुव Ast_ग्रन्थिः* ध्रुव abovep
            = (m_aboveCellp ? static_cast<Ast_ग्रन्थिः*>(m_aboveCellp) : static_cast<Ast_ग्रन्थिः*>(nodep));
        m_scopep
            = नूतन AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_ग्रन्थिः::user1_Clear_वृक्षः();
        m_scopes.प्रक्षिप_back(m_scopep);

        iterateChildren(nodep);
    }
    शून्य visit(AstCellInline* nodep) override {  //
        सचे (!v3Global.opt.vpi()) पटिनिवत्त;
        m_scopep->addInlinesp(
            नूतन AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    शून्य visit(AstActive* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    शून्य visit(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    शून्य visit(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    शून्य visit(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    शून्य visit(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* ध्रुव clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_ग्रन्थिः_FTask* ध्रुव clonep = [&]() {
            VL_RESTORER(m_last);
            // Only one scope will be created, so avoid pointless cloning
            सचे (nodep->classMethod()) m_last = सच्च;
            पटिनिवत्त cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    शून्य visit(AstVar* nodep) override {
        // Make new scope variable
        सचे (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            सचे (ध्रुव AstIfaceRefDType* ध्रुव ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                सचे (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* ध्रुव varscp = नूतन AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            सचे (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(मिच्छा);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->addVarsp(varscp);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_back(nodep, m_scopep);
    }
    शून्य visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        सचे (m_lib_शिखरम्_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_शिखरम्_Name_Len);
        ध्रुव string prefix = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(prefix + nodep->scopeAttr());
        nodep->scopeEntr(prefix + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    शून्य visit(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_शिखरम्(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = स्वभाव;
};

//######################################################################
// Scope cleanup -- remove unused activates

वग्ग ScopeCleanupVisitor final : सार्वजनिक VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope we are building

    // METHODS

    // VISITORS
    शून्य visit(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    आभास शून्य movedDeleteOrIterate(Ast_ग्रन्थिः* nodep) {
        सचे (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } अञ्ञथा {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    शून्य visit(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(nullptr);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        सचे (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_ग्रन्थिः_FTask* ध्रुव newp = VN_AS(nodep->taskp()->user2p(), ग्रन्थिः_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } अञ्ञथा सचे (!VN_IS(nodep, MethodCall)) {
            nodep->taskp(nullptr);
            VIsCached::clear_Cache_वृक्षः();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(nullptr);
        iterateChildren(nodep);
    }

    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = स्वभाव;
};

//######################################################################
// Scope class functions

शून्य V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ध्रुव ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::setScoped();
    V3Global::dump_Check_Global_वृक्षः("scope", 0, dump_वृक्षः_Either_Level() >= 3);
}
