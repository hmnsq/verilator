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

वर्ग ScopeVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    स्थिरांक VNUser1InUse m_inuser1;
    स्थिरांक VNUser2InUse m_inuser2;
    स्थिरांक VNUser3InUse m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    using VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    Ast_गाठ_Procedure* m_procedurep = nullptr;  // Current procedure
    AstScope* m_scopep = nullptr;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = nullptr;  // Cell that instantiates this module
    AstScope* m_aboveScopep = nullptr;  // Scope that instantiates this scope
    तार्किक m_last = असत्य;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    माप_प्रकार m_lib_शीर्ष_Name_Len = 0;

    std::unordered_map<Ast_गाठ_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::vector<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::vector<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    स्थिर शून्य मोजणी_Instantiations(Ast_गाठ_Module* modp) {
        modp->user3Inc();
        साठी (Ast_गाठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (स्थिरांक AstCell* स्थिरांक cellp = VN_CAST(stmtp, Cell)) {
                मोजणी_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    ढाँचा <typename T_गाठ>
    T_गाठ* cloneOrMove(T_गाठ* nodep) {
        T_गाठ* clonep;
        जर (m_last) {
            nodep->unlinkFrBack();
            clonep = nodep;
        } अन्यथा {
            clonep = nodep->clone_वृक्ष(असत्य);
        }
        nodep->user2p(clonep);
        m_scopep->addBlocksp(clonep);
        परत clonep;
    }

    शून्य cleanupVarRefs() {
        साठी (स्थिरांक स्वतः& itr : m_varRefScopes) {
            AstVarRef* स्थिरांक nodep = itr.first;
            AstScope* scopep = itr.second;
            जर (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                स्थिरांक स्वतः it2 = m_classOrPackageScopes.find(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.end(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->second;
            }
            nodep->classOrPackagep(nullptr);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = nullptr;
            AstScope* शोध_Scopep = scopep;
            पर्यंत (शोध_Scopep) {
                स्थिरांक स्वतः it3 = m_varScopes.find(std::make_pair(nodep->varp(), शोध_Scopep));
                जर (it3 != m_varScopes.end()) {
                    varscp = it3->second;
                    थांबा;
                }
                शोध_Scopep = शोध_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        Ast_गाठ_Module* स्थिरांक modp = nodep->शीर्ष_Modulep();
        जर (!modp) {
            nodep->v3error("No top level module found");
            परत;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = nullptr;
        m_aboveScopep = nullptr;
        मोजणी_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        साठी (AstScope* स्थिरांक scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        cleanupVarRefs();
    }
    शून्य visit(Ast_गाठ_Module* nodep) override {
        VL_RESTORER(m_lib_शीर्ष_Name_Len);
        // Create required blocks and add to module
        string scopename;
        जर (!m_aboveScopep) {
            scopename = "TOP";
        } अन्यथा {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            जर (!v3Global.opt.libCreate().empty() && m_aboveScopep->is_शीर्ष()
                && VN_IS(nodep, Module)) {
                m_lib_शीर्ष_Name_Len = scopename.size();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_गाठ::user1_Clear_वृक्ष();

        m_scopep = नवीन AstScope{
            (m_aboveCellp ? static_cast<Ast_गाठ*>(m_aboveCellp) : static_cast<Ast_गाठ*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        जर (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        साठी (Ast_गाठ* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            जर (AstCell* स्थिरांक cellp = VN_CAST(cellnextp, Cell)) cells.ढकला_back(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        साठी (AstCell* स्थिरांक cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_गाठ_Module* स्थिरांक modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                जर (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_गाठ::user1_Clear_वृक्ष();
        m_modp = nodep;
        जर (m_modp->is_शीर्ष()) {
            v3Global.rootp()->create_शीर्ष_Scope(m_scopep);
        } अन्यथा {
            m_scopes.ढकला_back(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_last);
            स्थिरांक uint64_t nInsts = nodep->user3();
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
        जर (!m_aboveScopep) {
            scopename = "TOP";
        } अन्यथा {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_गाठ::user1_Clear_वृक्ष();

        स्थिरांक Ast_गाठ* स्थिरांक abovep
            = (m_aboveCellp ? static_cast<Ast_गाठ*>(m_aboveCellp) : static_cast<Ast_गाठ*>(nodep));
        m_scopep
            = नवीन AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_गाठ::user1_Clear_वृक्ष();
        m_scopes.ढकला_back(m_scopep);

        iterateChildren(nodep);
    }
    शून्य visit(AstCellInline* nodep) override {  //
        जर (!v3Global.opt.vpi()) परत;
        m_scopep->addInlinesp(
            नवीन AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    शून्य visit(AstActive* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    शून्य visit(Ast_गाठ_Procedure* nodep) override {
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
        AstCFunc* स्थिरांक clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_गाठ_FTask* स्थिरांक clonep = [&]() {
            VL_RESTORER(m_last);
            // Only one scope will be created, so avoid pointless cloning
            जर (nodep->classMethod()) m_last = सत्य;
            परत cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    शून्य visit(AstVar* nodep) override {
        // Make new scope variable
        जर (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            जर (स्थिरांक AstIfaceRefDType* स्थिरांक ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                जर (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* स्थिरांक varscp = नवीन AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            जर (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(असत्य);
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
        जर (m_lib_शीर्ष_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_शीर्ष_Name_Len);
        स्थिरांक string prefix = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(prefix + nodep->scopeAttr());
        nodep->scopeEntr(prefix + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    शून्य visit(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_शीर्ष(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Scope cleanup -- remove unused activates

वर्ग ScopeCleanupVisitor final : सार्वजनिक VNVisitor {
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

    आभासी शून्य movedDeleteOrIterate(Ast_गाठ* nodep) {
        जर (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } अन्यथा {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    शून्य visit(Ast_गाठ_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(Ast_गाठ_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    शून्य visit(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    शून्य visit(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(nullptr);
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        जर (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_गाठ_FTask* स्थिरांक newp = VN_AS(nodep->taskp()->user2p(), गाठ_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } अन्यथा जर (!VN_IS(nodep, MethodCall)) {
            nodep->taskp(nullptr);
            VIsCached::clear_Cache_वृक्ष();
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
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Scope class functions

शून्य V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        स्थिरांक ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::setScoped();
    V3Global::dump_Check_Global_वृक्ष("scope", 0, dump_वृक्ष_Either_Level() >= 3);
}
