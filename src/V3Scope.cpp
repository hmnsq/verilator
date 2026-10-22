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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Scope.h"

#समावेश "V3ConstPool.h"

#समावेश <unordered_map>
#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

श्रेणी ScopeVisitor final : खुला VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;
    स्थिर VNUser3InUse m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    using VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    Ast_गाँठ_Procedure* m_procedurep = nullptr;  // Current procedure
    AstScope* m_scopep = nullptr;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = nullptr;  // Cell that instantiates this module
    AstScope* m_aboveScopep = nullptr;  // Scope that instantiates this scope
    bool m_last = false;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    माप_प्रकार m_lib_शीर्ष_Name_Len = 0;

    std::unordered_map<Ast_गाँठ_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::vector<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::vector<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    अटल व्योम गिनती_Instantiations(Ast_गाँठ_Module* modp) {
        modp->user3Inc();
        क्रम (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (स्थिर AstCell* स्थिर cellp = VN_CAST(stmtp, Cell)) {
                गिनती_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    ढाँचा <typename T_गाँठ>
    T_गाँठ* cloneOrMove(T_गाँठ* nodep) {
        T_गाँठ* clonep;
        अगर (m_last) {
            nodep->unlinkFrBack();
            clonep = nodep;
        } अन्यथा {
            clonep = nodep->clone_वृक्ष(false);
        }
        nodep->user2p(clonep);
        m_scopep->addBlocksp(clonep);
        वापस clonep;
    }

    व्योम cleanupVarRefs() {
        क्रम (स्थिर स्वतः& itr : m_varRefScopes) {
            AstVarRef* स्थिर nodep = itr.first;
            AstScope* scopep = itr.second;
            अगर (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                स्थिर स्वतः it2 = m_classOrPackageScopes.find(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.end(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->second;
            }
            nodep->classOrPackagep(nullptr);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = nullptr;
            AstScope* खोज_Scopep = scopep;
            जबतक (खोज_Scopep) {
                स्थिर स्वतः it3 = m_varScopes.find(std::make_pair(nodep->varp(), खोज_Scopep));
                अगर (it3 != m_varScopes.end()) {
                    varscp = it3->second;
                    अवरोध;
                }
                खोज_Scopep = खोज_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {
        Ast_गाँठ_Module* स्थिर modp = nodep->शीर्ष_Modulep();
        अगर (!modp) {
            nodep->v3error("No top level module found");
            वापस;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = nullptr;
        m_aboveScopep = nullptr;
        गिनती_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        क्रम (AstScope* स्थिर scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        cleanupVarRefs();
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_lib_शीर्ष_Name_Len);
        // Create required blocks and add to module
        string scopename;
        अगर (!m_aboveScopep) {
            scopename = "TOP";
        } अन्यथा {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            अगर (!v3Global.opt.libCreate().empty() && m_aboveScopep->is_शीर्ष()
                && VN_IS(nodep, Module)) {
                m_lib_शीर्ष_Name_Len = scopename.आकार();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_गाँठ::user1_Clear_वृक्ष();

        m_scopep = नया AstScope{
            (m_aboveCellp ? static_cast<Ast_गाँठ*>(m_aboveCellp) : static_cast<Ast_गाँठ*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        अगर (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        क्रम (Ast_गाँठ* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            अगर (AstCell* स्थिर cellp = VN_CAST(cellnextp, Cell)) cells.धकेलो_back(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        क्रम (AstCell* स्थिर cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_गाँठ_Module* स्थिर modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                अगर (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_गाँठ::user1_Clear_वृक्ष();
        m_modp = nodep;
        अगर (m_modp->is_शीर्ष()) {
            v3Global.rootp()->create_शीर्ष_Scope(m_scopep);
        } अन्यथा {
            m_scopes.धकेलो_back(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_last);
            स्थिर uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_last = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    व्योम visit(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        अगर (!m_aboveScopep) {
            scopename = "TOP";
        } अन्यथा {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_गाँठ::user1_Clear_वृक्ष();

        स्थिर Ast_गाँठ* स्थिर abovep
            = (m_aboveCellp ? static_cast<Ast_गाँठ*>(m_aboveCellp) : static_cast<Ast_गाँठ*>(nodep));
        m_scopep
            = नया AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_गाँठ::user1_Clear_वृक्ष();
        m_scopes.धकेलो_back(m_scopep);

        iterateChildren(nodep);
    }
    व्योम visit(AstCellInline* nodep) override {  //
        अगर (!v3Global.opt.vpi()) वापस;
        m_scopep->addInlinesp(
            नया AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    व्योम visit(AstActive* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    व्योम visit(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    व्योम visit(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    व्योम visit(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    व्योम visit(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* स्थिर clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_गाँठ_FTask* स्थिर clonep = [&]() {
            VL_RESTORER(m_last);
            // Only one scope will be created, so avoid pointless cloning
            अगर (nodep->classMethod()) m_last = true;
            वापस cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    व्योम visit(AstVar* nodep) override {
        // Make new scope variable
        अगर (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            अगर (स्थिर AstIfaceRefDType* स्थिर ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                अगर (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* स्थिर varscp = नया AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            अगर (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(false);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->addVarsp(varscp);
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_back(nodep, m_scopep);
    }
    व्योम visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        अगर (m_lib_शीर्ष_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_शीर्ष_Name_Len);
        स्थिर string prefix = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(prefix + nodep->scopeAttr());
        nodep->scopeEntr(prefix + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    व्योम visit(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_शीर्ष(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = शेष;
};

//######################################################################
// Scope cleanup -- remove unused activates

श्रेणी ScopeCleanupVisitor final : खुला VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope we are building

    // METHODS

    // VISITORS
    व्योम visit(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    भव व्योम movedDeleteOrIterate(Ast_गाँठ* nodep) {
        अगर (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } अन्यथा {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    व्योम visit(Ast_गाँठ_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    व्योम visit(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    व्योम visit(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    व्योम visit(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    व्योम visit(Ast_गाँठ_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    व्योम visit(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    व्योम visit(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(nullptr);
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        अगर (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_गाँठ_FTask* स्थिर newp = VN_AS(nodep->taskp()->user2p(), गाँठ_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } अन्यथा अगर (!VN_IS(nodep, MethodCall)) {
            nodep->taskp(nullptr);
            VIsCached::clear_Cache_वृक्ष();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(nullptr);
        iterateChildren(nodep);
    }

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = शेष;
};

//######################################################################
// Scope class functions

व्योम V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        स्थिर ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::setScoped();
    V3Global::dump_Check_Global_वृक्ष("scope", 0, dump_वृक्ष_Either_Level() >= 3);
}
