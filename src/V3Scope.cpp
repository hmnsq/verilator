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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Scope.h"

#அந்தர்பாவ "V3ConstPool.h"

#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

வகுப்பு ScopeVisitor final : பொது VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;
    மாறிலி VNUser3InUse m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    using VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    Ast_முனை_Procedure* m_procedurep = nullptr;  // Current procedure
    AstScope* m_scopep = nullptr;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = nullptr;  // Cell that instantiates this module
    AstScope* m_aboveScopep = nullptr;  // Scope that instantiates this scope
    தருக்க m_last = பொய்;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    size_t m_lib_உச்சி_Name_Len = 0;

    std::unordered_map<Ast_முனை_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::vector<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::vector<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    நிலையான வெறுமை மொத்தம்_Instantiations(Ast_முனை_Module* modp) {
        modp->user3Inc();
        க்கு (Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (மாறிலி AstCell* மாறிலி cellp = VN_CAST(stmtp, Cell)) {
                மொத்தம்_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    template <typename T_முனை>
    T_முனை* cloneOrMove(T_முனை* nodep) {
        T_முனை* clonep;
        எனில் (m_last) {
            nodep->unlinkFrBack();
            clonep = nodep;
        } இல்லையெனில் {
            clonep = nodep->clone_மரம்(பொய்);
        }
        nodep->user2p(clonep);
        m_scopep->addBlocksp(clonep);
        திரும்பு clonep;
    }

    வெறுமை cleanupVarRefs() {
        க்கு (மாறிலி auto& itr : m_varRefScopes) {
            AstVarRef* மாறிலி nodep = itr.first;
            AstScope* scopep = itr.second;
            எனில் (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                மாறிலி auto it2 = m_classOrPackageScopes.find(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.end(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->second;
            }
            nodep->classOrPackagep(nullptr);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = nullptr;
            AstScope* தேடல்_Scopep = scopep;
            போது (தேடல்_Scopep) {
                மாறிலி auto it3 = m_varScopes.find(std::make_pair(nodep->varp(), தேடல்_Scopep));
                எனில் (it3 != m_varScopes.end()) {
                    varscp = it3->second;
                    நிறுத்து;
                }
                தேடல்_Scopep = தேடல்_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        Ast_முனை_Module* மாறிலி modp = nodep->உச்சி_Modulep();
        எனில் (!modp) {
            nodep->v3error("No top level module found");
            திரும்பு;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = nullptr;
        m_aboveScopep = nullptr;
        மொத்தம்_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        க்கு (AstScope* மாறிலி scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        cleanupVarRefs();
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_lib_உச்சி_Name_Len);
        // Create required blocks and add to module
        string scopename;
        எனில் (!m_aboveScopep) {
            scopename = "TOP";
        } இல்லையெனில் {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            எனில் (!v3Global.opt.libCreate().empty() && m_aboveScopep->is_உச்சி()
                && VN_IS(nodep, Module)) {
                m_lib_உச்சி_Name_Len = scopename.size();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_முனை::user1_Clear_மரம்();

        m_scopep = புதிய AstScope{
            (m_aboveCellp ? static_cast<Ast_முனை*>(m_aboveCellp) : static_cast<Ast_முனை*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        எனில் (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        க்கு (Ast_முனை* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            எனில் (AstCell* மாறிலி cellp = VN_CAST(cellnextp, Cell)) cells.தள்ளு_back(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        க்கு (AstCell* மாறிலி cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_முனை_Module* மாறிலி modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                எனில் (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_முனை::user1_Clear_மரம்();
        m_modp = nodep;
        எனில் (m_modp->is_உச்சி()) {
            v3Global.rootp()->create_உச்சி_Scope(m_scopep);
        } இல்லையெனில் {
            m_scopes.தள்ளு_back(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_last);
            மாறிலி uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_last = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    வெறுமை visit(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        எனில் (!m_aboveScopep) {
            scopename = "TOP";
        } இல்லையெனில் {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_முனை::user1_Clear_மரம்();

        மாறிலி Ast_முனை* மாறிலி abovep
            = (m_aboveCellp ? static_cast<Ast_முனை*>(m_aboveCellp) : static_cast<Ast_முனை*>(nodep));
        m_scopep
            = புதிய AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_முனை::user1_Clear_மரம்();
        m_scopes.தள்ளு_back(m_scopep);

        iterateChildren(nodep);
    }
    வெறுமை visit(AstCellInline* nodep) override {  //
        எனில் (!v3Global.opt.vpi()) திரும்பு;
        m_scopep->addInlinesp(
            புதிய AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    வெறுமை visit(AstActive* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    வெறுமை visit(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    வெறுமை visit(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    வெறுமை visit(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    வெறுமை visit(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* மாறிலி clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_முனை_FTask* மாறிலி clonep = [&]() {
            VL_RESTORER(m_last);
            // Only one scope will be created, so avoid pointless cloning
            எனில் (nodep->classMethod()) m_last = உண்மை;
            திரும்பு cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    வெறுமை visit(AstVar* nodep) override {
        // Make new scope variable
        எனில் (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            எனில் (மாறிலி AstIfaceRefDType* மாறிலி ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                எனில் (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* மாறிலி varscp = புதிய AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            எனில் (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(பொய்);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->addVarsp(varscp);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_back(nodep, m_scopep);
    }
    வெறுமை visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        எனில் (m_lib_உச்சி_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_உச்சி_Name_Len);
        மாறிலி string prefix = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(prefix + nodep->scopeAttr());
        nodep->scopeEntr(prefix + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    வெறுமை visit(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_உச்சி(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = இயல்பு;
};

//######################################################################
// Scope cleanup -- remove unused activates

வகுப்பு ScopeCleanupVisitor final : பொது VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope we are building

    // METHODS

    // VISITORS
    வெறுமை visit(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    மெய்நிகர் வெறுமை movedDeleteOrIterate(Ast_முனை* nodep) {
        எனில் (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } இல்லையெனில் {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    வெறுமை visit(Ast_முனை_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    வெறுமை visit(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    வெறுமை visit(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    வெறுமை visit(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    வெறுமை visit(Ast_முனை_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    வெறுமை visit(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    வெறுமை visit(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(nullptr);
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        எனில் (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_முனை_FTask* மாறிலி newp = VN_AS(nodep->taskp()->user2p(), முனை_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } இல்லையெனில் எனில் (!VN_IS(nodep, MethodCall)) {
            nodep->taskp(nullptr);
            VIsCached::clear_Cache_மரம்();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(nullptr);
        iterateChildren(nodep);
    }

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = இயல்பு;
};

//######################################################################
// Scope class functions

வெறுமை V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        மாறிலி ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::setScoped();
    V3Global::dump_Check_Global_மரம்("scope", 0, dump_மரம்_Either_Level() >= 3);
}
