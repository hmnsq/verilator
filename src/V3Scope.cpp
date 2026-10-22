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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Scope.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"

#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

ଶ୍ରେଣୀ ScopeVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;
    ଅଚଳ VNUser3InUse m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    using VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    Ast_ଗଣ୍ଠି_Procedure* m_procedurep = nullptr;  // Current procedure
    AstScope* m_scopep = nullptr;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = nullptr;  // Cell that instantiates this module
    AstScope* m_aboveScopep = nullptr;  // Scope that instantiates this scope
    ତାର୍କିକ m_last = ମିଥ୍ୟା;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    size_t m_lib_ଶୀର୍ଷ_Name_Len = 0;

    std::unordered_map<Ast_ଗଣ୍ଠି_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::vector<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::vector<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    ସ୍ଥିର ଶୂନ୍ୟ ଗଣନା_Instantiations(Ast_ଗଣ୍ଠି_Module* modp) {
        modp->user3Inc();
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (ଅଚଳ AstCell* ଅଚଳ cellp = VN_CAST(stmtp, Cell)) {
                ଗଣନା_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    template <typename T_ଗଣ୍ଠି>
    T_ଗଣ୍ଠି* cloneOrMove(T_ଗଣ୍ଠି* nodep) {
        T_ଗଣ୍ଠି* clonep;
        ଯଦି (m_last) {
            nodep->unlinkFrBack();
            clonep = nodep;
        } ଅନ୍ୟଥା {
            clonep = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        nodep->user2p(clonep);
        m_scopep->addBlocksp(clonep);
        ଫେରନ୍ତୁ clonep;
    }

    ଶୂନ୍ୟ cleanupVarRefs() {
        ପାଇଁ (ଅଚଳ auto& itr : m_varRefScopes) {
            AstVarRef* ଅଚଳ nodep = itr.first;
            AstScope* scopep = itr.second;
            ଯଦି (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                ଅଚଳ auto it2 = m_classOrPackageScopes.find(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.end(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->second;
            }
            nodep->classOrPackagep(nullptr);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = nullptr;
            AstScope* ସନ୍ଧାନ_Scopep = scopep;
            ଯେତେବେଳେ (ସନ୍ଧାନ_Scopep) {
                ଅଚଳ auto it3 = m_varScopes.find(std::make_pair(nodep->varp(), ସନ୍ଧାନ_Scopep));
                ଯଦି (it3 != m_varScopes.end()) {
                    varscp = it3->second;
                    ଭାଙ୍ଗ;
                }
                ସନ୍ଧାନ_Scopep = ସନ୍ଧାନ_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = nodep->ଶୀର୍ଷ_Modulep();
        ଯଦି (!modp) {
            nodep->v3error("No top level module found");
            ଫେରନ୍ତୁ;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = nullptr;
        m_aboveScopep = nullptr;
        ଗଣନା_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        ପାଇଁ (AstScope* ଅଚଳ scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        cleanupVarRefs();
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_lib_ଶୀର୍ଷ_Name_Len);
        // Create required blocks and add to module
        string scopename;
        ଯଦି (!m_aboveScopep) {
            scopename = "TOP";
        } ଅନ୍ୟଥା {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            ଯଦି (!v3Global.opt.libCreate().empty() && m_aboveScopep->is_ଶୀର୍ଷ()
                && VN_IS(nodep, Module)) {
                m_lib_ଶୀର୍ଷ_Name_Len = scopename.size();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();

        m_scopep = ନୂତନ AstScope{
            (m_aboveCellp ? static_cast<Ast_ଗଣ୍ଠି*>(m_aboveCellp) : static_cast<Ast_ଗଣ୍ଠି*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        ଯଦି (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        ପାଇଁ (Ast_ଗଣ୍ଠି* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            ଯଦି (AstCell* ଅଚଳ cellp = VN_CAST(cellnextp, Cell)) cells.ଠେଲ_back(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        ପାଇଁ (AstCell* ଅଚଳ cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                ଯଦି (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();
        m_modp = nodep;
        ଯଦି (m_modp->is_ଶୀର୍ଷ()) {
            v3Global.rootp()->create_ଶୀର୍ଷ_Scope(m_scopep);
        } ଅନ୍ୟଥା {
            m_scopes.ଠେଲ_back(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_last);
            ଅଚଳ uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_last = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        ଯଦି (!m_aboveScopep) {
            scopename = "TOP";
        } ଅନ୍ୟଥା {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();

        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ abovep
            = (m_aboveCellp ? static_cast<Ast_ଗଣ୍ଠି*>(m_aboveCellp) : static_cast<Ast_ଗଣ୍ଠି*>(nodep));
        m_scopep
            = ନୂତନ AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();
        m_scopes.ଠେଲ_back(m_scopep);

        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCellInline* nodep) override {  //
        ଯଦି (!v3Global.opt.vpi()) ଫେରନ୍ତୁ;
        m_scopep->addInlinesp(
            ନୂତନ AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ଶୂନ୍ୟ visit(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ଶୂନ୍ୟ visit(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ଶୂନ୍ୟ visit(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* ଅଚଳ clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_ଗଣ୍ଠି_FTask* ଅଚଳ clonep = [&]() {
            VL_RESTORER(m_last);
            // Only one scope will be created, so avoid pointless cloning
            ଯଦି (nodep->classMethod()) m_last = ସତ୍ୟ;
            ଫେରନ୍ତୁ cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        // Make new scope variable
        ଯଦି (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                ଯଦି (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* ଅଚଳ varscp = ନୂତନ AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            ଯଦି (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(ମିଥ୍ୟା);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->addVarsp(varscp);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_back(nodep, m_scopep);
    }
    ଶୂନ୍ୟ visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        ଯଦି (m_lib_ଶୀର୍ଷ_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_ଶୀର୍ଷ_Name_Len);
        ଅଚଳ string prefix = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(prefix + nodep->scopeAttr());
        nodep->scopeEntr(prefix + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_ଶୀର୍ଷ(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Scope cleanup -- remove unused activates

ଶ୍ରେଣୀ ScopeCleanupVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope we are building

    // METHODS

    // VISITORS
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ଆଭାସୀ ଶୂନ୍ୟ movedDeleteOrIterate(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } ଅନ୍ୟଥା {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    ଶୂନ୍ୟ visit(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    ଶୂନ୍ୟ visit(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    ଶୂନ୍ୟ visit(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    ଶୂନ୍ୟ visit(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(nullptr);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        ଯଦି (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_ଗଣ୍ଠି_FTask* ଅଚଳ newp = VN_AS(nodep->taskp()->user2p(), ଗଣ୍ଠି_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } ଅନ୍ୟଥା ଯଦି (!VN_IS(nodep, MethodCall)) {
            nodep->taskp(nullptr);
            VIsCached::clear_Cache_ବୃକ୍ଷ();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(nullptr);
        iterateChildren(nodep);
    }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Scope class functions

ଶୂନ୍ୟ V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ଅଚଳ ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::setScoped();
    V3Global::dump_Check_Global_ବୃକ୍ଷ("scope", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
