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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Scope.h"

#شامل "V3ConstPool.h"

#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

جماعت ScopeVisitor final : عوامی VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;
    ثابت VNUser3_اندر_Use m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    استعمال VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    Ast_گرہ_Procedure* m_procedurep = خالی_اشاریہ;  // Current procedure
    AstScope* m_scopep = خالی_اشاریہ;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = خالی_اشاریہ;  // Cell that instantiates this module
    AstScope* m_aboveScopep = خالی_اشاریہ;  // Scope that instantiates this scope
    منطقی m_آخری = جھوٹ;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    سائز_t m_lib_چوٹی_Name_Len = 0;

    std::unordered_map<Ast_گرہ_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::سمتیہ<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::سمتیہ<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    ساکن خالی گنتی_Instantiations(Ast_گرہ_Module* modp) {
        modp->user3Inc();
        برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (ثابت AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) {
                گنتی_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    سانچہ <قسم_نام T_گرہ>
    T_گرہ* cloneOrMove(T_گرہ* nodep) {
        T_گرہ* clonep;
        اگر (m_آخری) {
            nodep->unlink_Fr_عقب();
            clonep = nodep;
        } ورنہ {
            clonep = nodep->clone_درخت(جھوٹ);
        }
        nodep->user2p(clonep);
        m_scopep->جوڑو_Blocksp(clonep);
        واپس clonep;
    }

    خالی cleanupVarRefs() {
        برائے (ثابت خودکار& itr : m_varRefScopes) {
            AstVarRef* ثابت nodep = itr.پہلا;
            AstScope* scopep = itr.دوسرا;
            اگر (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                ثابت خودکار it2 = m_classOrPackageScopes.ڈھونڈو(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.انتہا(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->دوسرا;
            }
            nodep->classOrPackagep(خالی_اشاریہ);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = خالی_اشاریہ;
            AstScope* تلاش_Scopep = scopep;
            جب (تلاش_Scopep) {
                ثابت خودکار it3 = m_varScopes.ڈھونڈو(std::make_pair(nodep->varp(), تلاش_Scopep));
                اگر (it3 != m_varScopes.انتہا()) {
                    varscp = it3->دوسرا;
                    توڑو;
                }
                تلاش_Scopep = تلاش_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {
        Ast_گرہ_Module* ثابت modp = nodep->چوٹی_Modulep();
        اگر (!modp) {
            nodep->v3error("No top level module found");
            واپس;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = خالی_اشاریہ;
        m_aboveScopep = خالی_اشاریہ;
        گنتی_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        برائے (AstScope* ثابت scopep : m_scopes) scopep->modp()->جوڑو_Stmtsp(scopep);
        cleanupVarRefs();
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_lib_چوٹی_Name_Len);
        // Create required blocks and add to module
        string scopename;
        اگر (!m_aboveScopep) {
            scopename = "TOP";
        } ورنہ {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            اگر (!v3Global.opt.libCreate().خالی_خانے() && m_aboveScopep->is_چوٹی()
                && VN_IS(nodep, Module)) {
                m_lib_چوٹی_Name_Len = scopename.سائز();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_گرہ::user1_Clear_درخت();

        m_scopep = نیا AstScope{
            (m_aboveCellp ? static_cast<Ast_گرہ*>(m_aboveCellp) : static_cast<Ast_گرہ*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        اگر (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        برائے (Ast_گرہ* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            اگر (AstCell* ثابت cellp = VN_CAST(cellnextp, Cell)) cells.ڈالو_عقب(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        برائے (AstCell* ثابت cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_گرہ_Module* ثابت modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                اگر (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_گرہ::user1_Clear_درخت();
        m_modp = nodep;
        اگر (m_modp->is_چوٹی()) {
            v3Global.rootp()->create_چوٹی_Scope(m_scopep);
        } ورنہ {
            m_scopes.ڈالو_عقب(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_آخری);
            ثابت uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_آخری = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    خالی دورہ(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        اگر (!m_aboveScopep) {
            scopename = "TOP";
        } ورنہ {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_گرہ::user1_Clear_درخت();

        ثابت Ast_گرہ* ثابت abovep
            = (m_aboveCellp ? static_cast<Ast_گرہ*>(m_aboveCellp) : static_cast<Ast_گرہ*>(nodep));
        m_scopep
            = نیا AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_گرہ::user1_Clear_درخت();
        m_scopes.ڈالو_عقب(m_scopep);

        iterateChildren(nodep);
    }
    خالی دورہ(AstCellInline* nodep) override {  //
        اگر (!v3Global.opt.vpi()) واپس;
        m_scopep->جوڑو_Inlinesp(
            نیا AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    خالی دورہ(Ast_فعال* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    خالی دورہ(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    خالی دورہ(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    خالی دورہ(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    خالی دورہ(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* ثابت clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_گرہ_FTask* ثابت clonep = [&]() {
            VL_RESTORER(m_آخری);
            // Only one scope will be created, so avoid pointless cloning
            اگر (nodep->class_طریقہ()) m_آخری = سچ;
            واپس cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    خالی دورہ(AstVar* nodep) override {
        // Make new scope variable
        اگر (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            اگر (ثابت AstIfaceRefDType* ثابت ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                اگر (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* ثابت varscp = نیا AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            اگر (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(جھوٹ);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->جوڑو_Varsp(varscp);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_عقب(nodep, m_scopep);
    }
    خالی دورہ(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        اگر (m_lib_چوٹی_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_چوٹی_Name_Len);
        ثابت string سابقہ = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(سابقہ + nodep->scopeAttr());
        nodep->scopeEntr(سابقہ + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    خالی دورہ(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_چوٹی(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = طےشدہ;
};

//######################################################################
// Scope cleanup -- remove unused activates

جماعت ScopeCleanupVisitor final : عوامی VNVisitor {
    // STATE
    AstScope* m_scopep = خالی_اشاریہ;  // Current scope we are building

    // METHODS

    // VISITORS
    خالی دورہ(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    مجازی خالی movedDeleteOrIterate(Ast_گرہ* nodep) {
        اگر (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } ورنہ {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        }
    }

    خالی دورہ(Ast_گرہ_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    خالی دورہ(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    خالی دورہ(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    خالی دورہ(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    خالی دورہ(Ast_گرہ_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    خالی دورہ(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    خالی دورہ(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(خالی_اشاریہ);
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        اگر (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_گرہ_FTask* ثابت newp = VN_AS(nodep->taskp()->user2p(), گرہ_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } ورنہ اگر (!VN_IS(nodep, طریقہ_Call)) {
            nodep->taskp(خالی_اشاریہ);
            VIsCached::clear_Cache_درخت();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(خالی_اشاریہ);
        iterateChildren(nodep);
    }

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = طےشدہ;
};

//######################################################################
// Scope class functions

خالی V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::سیٹ_Scoped();
    V3Global::dump_Check_Global_درخت("scope", 0, dump_درخت_Either_Level() >= 3);
}
