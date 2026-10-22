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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Scope.h"

#ضمن "V3ConstPool.h"

#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

صنف ScopeVisitor final : عام VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser2_درون_Use m_inuser2;
    ثابت VNUser3_درون_Use m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    استفاده VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    Ast_گره_Procedure* m_procedurep = اشاره_تهی;  // Current procedure
    AstScope* m_scopep = اشاره_تهی;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = اشاره_تهی;  // Cell that instantiates this module
    AstScope* m_aboveScopep = اشاره_تهی;  // Scope that instantiates this scope
    منطقي m_آخرین = خطأ;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    اندازه_t m_lib_بالاترین_Name_Len = 0;

    std::unordered_map<Ast_گره_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::وکتور<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::وکتور<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    ساكن فراغ شمار_Instantiations(Ast_گره_Module* modp) {
        modp->user3Inc();
        لكل (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) {
                شمار_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    قالب <نام_نوع T_گره>
    T_گره* cloneOrMove(T_گره* nodep) {
        T_گره* clonep;
        إذا (m_آخرین) {
            nodep->unlink_Fr_پشت();
            clonep = nodep;
        } وإلا {
            clonep = nodep->clone_درخت(خطأ);
        }
        nodep->user2p(clonep);
        m_scopep->جمع_کن_Blocksp(clonep);
        أرجع clonep;
    }

    فراغ cleanupVarRefs() {
        لكل (ثابت خودکار& itr : m_varRefScopes) {
            AstVarRef* ثابت nodep = itr.اولی;
            AstScope* scopep = itr.دومی;
            إذا (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                ثابت خودکار it2 = m_classOrPackageScopes.پیدا_کن(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.انتها(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->دومی;
            }
            nodep->classOrPackagep(اشاره_تهی);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = اشاره_تهی;
            AstScope* جستجو_Scopep = scopep;
            طالما (جستجو_Scopep) {
                ثابت خودکار it3 = m_varScopes.پیدا_کن(std::make_pair(nodep->varp(), جستجو_Scopep));
                إذا (it3 != m_varScopes.انتها()) {
                    varscp = it3->دومی;
                    اكسر;
                }
                جستجو_Scopep = جستجو_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    فراغ بازدید(AstNetlist* nodep) override {
        Ast_گره_Module* ثابت modp = nodep->بالاترین_Modulep();
        إذا (!modp) {
            nodep->v3error("No top level module found");
            أرجع;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = اشاره_تهی;
        m_aboveScopep = اشاره_تهی;
        شمار_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        لكل (AstScope* ثابت scopep : m_scopes) scopep->modp()->جمع_کن_Stmtsp(scopep);
        cleanupVarRefs();
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_lib_بالاترین_Name_Len);
        // Create required blocks and add to module
        string scopename;
        إذا (!m_aboveScopep) {
            scopename = "TOP";
        } وإلا {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            إذا (!v3Global.opt.libCreate().خالی() && m_aboveScopep->is_بالاترین()
                && VN_IS(nodep, Module)) {
                m_lib_بالاترین_Name_Len = scopename.اندازه();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_گره::user1_Clear_درخت();

        m_scopep = جديد AstScope{
            (m_aboveCellp ? static_cast<Ast_گره*>(m_aboveCellp) : static_cast<Ast_گره*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        إذا (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        لكل (Ast_گره* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            إذا (AstCell* ثابت cellp = VN_CAST(cellnextp, Cell)) cells.افزودن_پشت(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        لكل (AstCell* ثابت cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_گره_Module* ثابت modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                إذا (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_گره::user1_Clear_درخت();
        m_modp = nodep;
        إذا (m_modp->is_بالاترین()) {
            v3Global.rootp()->create_بالاترین_Scope(m_scopep);
        } وإلا {
            m_scopes.افزودن_پشت(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_آخرین);
            ثابت uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_آخرین = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    فراغ بازدید(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        إذا (!m_aboveScopep) {
            scopename = "TOP";
        } وإلا {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_گره::user1_Clear_درخت();

        ثابت Ast_گره* ثابت abovep
            = (m_aboveCellp ? static_cast<Ast_گره*>(m_aboveCellp) : static_cast<Ast_گره*>(nodep));
        m_scopep
            = جديد AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_گره::user1_Clear_درخت();
        m_scopes.افزودن_پشت(m_scopep);

        iterateChildren(nodep);
    }
    فراغ بازدید(AstCellInline* nodep) override {  //
        إذا (!v3Global.opt.vpi()) أرجع;
        m_scopep->جمع_کن_Inlinesp(
            جديد AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    فراغ بازدید(Ast_فعال* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    فراغ بازدید(Ast_گره_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ بازدید(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ بازدید(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ بازدید(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ بازدید(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* ثابت clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_گره_FTask* ثابت clonep = [&]() {
            VL_RESTORER(m_آخرین);
            // Only one scope will be created, so avoid pointless cloning
            إذا (nodep->class_روش()) m_آخرین = درست;
            أرجع cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    فراغ بازدید(AstVar* nodep) override {
        // Make new scope variable
        إذا (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            إذا (ثابت AstIfaceRefDType* ثابت ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                إذا (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* ثابت varscp = جديد AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            إذا (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(خطأ);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->جمع_کن_Varsp(varscp);
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_پشت(nodep, m_scopep);
    }
    فراغ بازدید(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        إذا (m_lib_بالاترین_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_بالاترین_Name_Len);
        ثابت string پیشوند = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(پیشوند + nodep->scopeAttr());
        nodep->scopeEntr(پیشوند + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    فراغ بازدید(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_بالاترین(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = افتراضي;
};

//######################################################################
// Scope cleanup -- remove unused activates

صنف ScopeCleanupVisitor final : عام VNVisitor {
    // STATE
    AstScope* m_scopep = اشاره_تهی;  // Current scope we are building

    // METHODS

    // VISITORS
    فراغ بازدید(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    مجازی فراغ movedDeleteOrIterate(Ast_گره* nodep) {
        إذا (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } وإلا {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
        }
    }

    فراغ بازدید(Ast_گره_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ بازدید(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ بازدید(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ بازدید(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ بازدید(Ast_گره_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ بازدید(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    فراغ بازدید(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(اشاره_تهی);
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        إذا (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_گره_FTask* ثابت newp = VN_AS(nodep->taskp()->user2p(), گره_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } وإلا إذا (!VN_IS(nodep, روش_Call)) {
            nodep->taskp(اشاره_تهی);
            VIsCached::clear_Cache_درخت();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(اشاره_تهی);
        iterateChildren(nodep);
    }

    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = افتراضي;
};

//######################################################################
// Scope class functions

فراغ V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::مجموعه_Scoped();
    V3Global::dump_Check_Global_درخت("scope", 0, dump_درخت_Either_Level() >= 3);
}
