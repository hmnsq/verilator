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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Scope.h"

#অন্তর্ভাৱ "V3ConstPool.h"

#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

শ্ৰেণী ScopeVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    ধ্রুৱ VNUser1InUse m_inuser1;
    ধ্রুৱ VNUser2InUse m_inuser2;
    ধ্রুৱ VNUser3InUse m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    using VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    Ast_গ্ৰন্থি_Procedure* m_procedurep = nullptr;  // Current procedure
    AstScope* m_scopep = nullptr;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = nullptr;  // Cell that instantiates this module
    AstScope* m_aboveScopep = nullptr;  // Scope that instantiates this scope
    তার্কিক m_last = মিছা;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    size_t m_lib_শীৰ্ষ_Name_Len = 0;

    std::unordered_map<Ast_গ্ৰন্থি_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::vector<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::vector<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    স্থির শূন্য গুনতি_Instantiations(Ast_গ্ৰন্থি_Module* modp) {
        modp->user3Inc();
        বাবে (Ast_গ্ৰন্থি* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (ধ্রুৱ AstCell* ধ্রুৱ cellp = VN_CAST(stmtp, Cell)) {
                গুনতি_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    template <typename T_গ্ৰন্থি>
    T_গ্ৰন্থি* cloneOrMove(T_গ্ৰন্থি* nodep) {
        T_গ্ৰন্থি* clonep;
        যদি (m_last) {
            nodep->unlinkFrBack();
            clonep = nodep;
        } অন্যথা {
            clonep = nodep->clone_বৃক্ষ(মিছা);
        }
        nodep->user2p(clonep);
        m_scopep->addBlocksp(clonep);
        ঘূৰাই clonep;
    }

    শূন্য cleanupVarRefs() {
        বাবে (ধ্রুৱ auto& itr : m_varRefScopes) {
            AstVarRef* ধ্রুৱ nodep = itr.first;
            AstScope* scopep = itr.second;
            যদি (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                ধ্রুৱ auto it2 = m_classOrPackageScopes.find(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.end(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->second;
            }
            nodep->classOrPackagep(nullptr);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = nullptr;
            AstScope* অনুসন্ধান_Scopep = scopep;
            যেতিয়ালৈ (অনুসন্ধান_Scopep) {
                ধ্রুৱ auto it3 = m_varScopes.find(std::make_pair(nodep->varp(), অনুসন্ধান_Scopep));
                যদি (it3 != m_varScopes.end()) {
                    varscp = it3->second;
                    ভঙ্গ;
                }
                অনুসন্ধান_Scopep = অনুসন্ধান_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = nodep->শীৰ্ষ_Modulep();
        যদি (!modp) {
            nodep->v3error("No top level module found");
            ঘূৰাই;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = nullptr;
        m_aboveScopep = nullptr;
        গুনতি_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        বাবে (AstScope* ধ্রুৱ scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        cleanupVarRefs();
    }
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        VL_RESTORER(m_lib_শীৰ্ষ_Name_Len);
        // Create required blocks and add to module
        string scopename;
        যদি (!m_aboveScopep) {
            scopename = "TOP";
        } অন্যথা {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            যদি (!v3Global.opt.libCreate().empty() && m_aboveScopep->is_শীৰ্ষ()
                && VN_IS(nodep, Module)) {
                m_lib_শীৰ্ষ_Name_Len = scopename.size();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_গ্ৰন্থি::user1_Clear_বৃক্ষ();

        m_scopep = নূতন AstScope{
            (m_aboveCellp ? static_cast<Ast_গ্ৰন্থি*>(m_aboveCellp) : static_cast<Ast_গ্ৰন্থি*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        যদি (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        বাবে (Ast_গ্ৰন্থি* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            যদি (AstCell* ধ্রুৱ cellp = VN_CAST(cellnextp, Cell)) cells.ঠেলো_back(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        বাবে (AstCell* ধ্রুৱ cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                যদি (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_গ্ৰন্থি::user1_Clear_বৃক্ষ();
        m_modp = nodep;
        যদি (m_modp->is_শীৰ্ষ()) {
            v3Global.rootp()->create_শীৰ্ষ_Scope(m_scopep);
        } অন্যথা {
            m_scopes.ঠেলো_back(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_last);
            ধ্রুৱ uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_last = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    শূন্য visit(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        যদি (!m_aboveScopep) {
            scopename = "TOP";
        } অন্যথা {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_গ্ৰন্থি::user1_Clear_বৃক্ষ();

        ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ abovep
            = (m_aboveCellp ? static_cast<Ast_গ্ৰন্থি*>(m_aboveCellp) : static_cast<Ast_গ্ৰন্থি*>(nodep));
        m_scopep
            = নূতন AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_গ্ৰন্থি::user1_Clear_বৃক্ষ();
        m_scopes.ঠেলো_back(m_scopep);

        iterateChildren(nodep);
    }
    শূন্য visit(AstCellInline* nodep) override {  //
        যদি (!v3Global.opt.vpi()) ঘূৰাই;
        m_scopep->addInlinesp(
            নূতন AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    শূন্য visit(AstActive* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    শূন্য visit(Ast_গ্ৰন্থি_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    শূন্য visit(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    শূন্য visit(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    শূন্য visit(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    শূন্য visit(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* ধ্রুৱ clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_গ্ৰন্থি_FTask* ধ্রুৱ clonep = [&]() {
            VL_RESTORER(m_last);
            // Only one scope will be created, so avoid pointless cloning
            যদি (nodep->classMethod()) m_last = সঁচা;
            ঘূৰাই cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    শূন্য visit(AstVar* nodep) override {
        // Make new scope variable
        যদি (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            যদি (ধ্রুৱ AstIfaceRefDType* ধ্রুৱ ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                যদি (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* ধ্রুৱ varscp = নূতন AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            যদি (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(মিছা);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->addVarsp(varscp);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_back(nodep, m_scopep);
    }
    শূন্য visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        যদি (m_lib_শীৰ্ষ_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_শীৰ্ষ_Name_Len);
        ধ্রুৱ string prefix = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(prefix + nodep->scopeAttr());
        nodep->scopeEntr(prefix + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    শূন্য visit(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_শীৰ্ষ(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Scope cleanup -- remove unused activates

শ্ৰেণী ScopeCleanupVisitor final : সার্ৱজনিক VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope we are building

    // METHODS

    // VISITORS
    শূন্য visit(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    আভাস শূন্য movedDeleteOrIterate(Ast_গ্ৰন্থি* nodep) {
        যদি (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } অন্যথা {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    শূন্য visit(Ast_গ্ৰন্থি_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    শূন্য visit(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    শূন্য visit(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    শূন্য visit(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    শূন্য visit(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    শূন্য visit(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(nullptr);
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        যদি (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_গ্ৰন্থি_FTask* ধ্রুৱ newp = VN_AS(nodep->taskp()->user2p(), গ্ৰন্থি_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } অন্যথা যদি (!VN_IS(nodep, MethodCall)) {
            nodep->taskp(nullptr);
            VIsCached::clear_Cache_বৃক্ষ();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(nullptr);
        iterateChildren(nodep);
    }

    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Scope class functions

শূন্য V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ধ্রুৱ ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::setScoped();
    V3Global::dump_Check_Global_বৃক্ষ("scope", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
