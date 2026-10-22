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
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;
    ثابت VNUser3_دننه_Use m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    کارول VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    Ast_غوټه_Procedure* m_procedurep = تش_اشاره;  // Current procedure
    AstScope* m_scopep = تش_اشاره;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = تش_اشاره;  // Cell that instantiates this module
    AstScope* m_aboveScopep = تش_اشاره;  // Scope that instantiates this scope
    منطقي m_وروستی = خطأ;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    اندازه_t m_lib_سر_Name_Len = 0;

    std::unordered_map<Ast_غوټه_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::ویکټور<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::ویکټور<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    ساكن فراغ شمېر_Instantiations(Ast_غوټه_Module* modp) {
        modp->user3Inc();
        لكل (Ast_غوټه* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) {
                شمېر_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    کينډۍ <ډول_نوم T_غوټه>
    T_غوټه* cloneOrMove(T_غوټه* nodep) {
        T_غوټه* clonep;
        إذا (m_وروستی) {
            nodep->unlink_Fr_شا();
            clonep = nodep;
        } وإلا {
            clonep = nodep->clone_ونه(خطأ);
        }
        nodep->user2p(clonep);
        m_scopep->جمع_کړه_Blocksp(clonep);
        أرجع clonep;
    }

    فراغ cleanupVarRefs() {
        لكل (ثابت خپلکاری& itr : m_varRefScopes) {
            AstVarRef* ثابت nodep = itr.لومړی;
            AstScope* scopep = itr.دوهم;
            إذا (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                ثابت خپلکاری it2 = m_classOrPackageScopes.پیدا_کړه(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.پای_ټکی(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->دوهم;
            }
            nodep->classOrPackagep(تش_اشاره);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = تش_اشاره;
            AstScope* پلټنه_Scopep = scopep;
            طالما (پلټنه_Scopep) {
                ثابت خپلکاری it3 = m_varScopes.پیدا_کړه(std::make_pair(nodep->varp(), پلټنه_Scopep));
                إذا (it3 != m_varScopes.پای_ټکی()) {
                    varscp = it3->دوهم;
                    اكسر;
                }
                پلټنه_Scopep = پلټنه_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {
        Ast_غوټه_Module* ثابت modp = nodep->سر_Modulep();
        إذا (!modp) {
            nodep->v3error("No top level module found");
            أرجع;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = تش_اشاره;
        m_aboveScopep = تش_اشاره;
        شمېر_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        لكل (AstScope* ثابت scopep : m_scopes) scopep->modp()->جمع_کړه_Stmtsp(scopep);
        cleanupVarRefs();
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_lib_سر_Name_Len);
        // Create required blocks and add to module
        string scopename;
        إذا (!m_aboveScopep) {
            scopename = "TOP";
        } وإلا {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            إذا (!v3Global.opt.libCreate().تشه() && m_aboveScopep->is_سر()
                && VN_IS(nodep, Module)) {
                m_lib_سر_Name_Len = scopename.اندازه();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_غوټه::user1_Clear_ونه();

        m_scopep = جديد AstScope{
            (m_aboveCellp ? static_cast<Ast_غوټه*>(m_aboveCellp) : static_cast<Ast_غوټه*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        إذا (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        لكل (Ast_غوټه* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            إذا (AstCell* ثابت cellp = VN_CAST(cellnextp, Cell)) cells.ټېل_شا(cellp);
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
                Ast_غوټه_Module* ثابت modp = cellp->modp();
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
        Ast_غوټه::user1_Clear_ونه();
        m_modp = nodep;
        إذا (m_modp->is_سر()) {
            v3Global.rootp()->create_سر_Scope(m_scopep);
        } وإلا {
            m_scopes.ټېل_شا(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_وروستی);
            ثابت uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_وروستی = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    فراغ لیدنه(AstClass* nodep) override {
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
        Ast_غوټه::user1_Clear_ونه();

        ثابت Ast_غوټه* ثابت abovep
            = (m_aboveCellp ? static_cast<Ast_غوټه*>(m_aboveCellp) : static_cast<Ast_غوټه*>(nodep));
        m_scopep
            = جديد AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_غوټه::user1_Clear_ونه();
        m_scopes.ټېل_شا(m_scopep);

        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCellInline* nodep) override {  //
        إذا (!v3Global.opt.vpi()) أرجع;
        m_scopep->جمع_کړه_Inlinesp(
            جديد AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    فراغ لیدنه(Ast_فعال* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ لیدنه(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ لیدنه(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ لیدنه(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* ثابت clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_غوټه_FTask* ثابت clonep = [&]() {
            VL_RESTORER(m_وروستی);
            // Only one scope will be created, so avoid pointless cloning
            إذا (nodep->class_لاره()) m_وروستی = ریښتیا;
            أرجع cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    فراغ لیدنه(AstVar* nodep) override {
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
            m_scopep->جمع_کړه_Varsp(varscp);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_شا(nodep, m_scopep);
    }
    فراغ لیدنه(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        إذا (m_lib_سر_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_سر_Name_Len);
        ثابت string مختاړی = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(مختاړی + nodep->scopeAttr());
        nodep->scopeEntr(مختاړی + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_سر(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = افتراضي;
};

//######################################################################
// Scope cleanup -- remove unused activates

صنف ScopeCleanupVisitor final : عام VNVisitor {
    // STATE
    AstScope* m_scopep = تش_اشاره;  // Current scope we are building

    // METHODS

    // VISITORS
    فراغ لیدنه(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    مجازي فراغ movedDeleteOrIterate(Ast_غوټه* nodep) {
        إذا (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } وإلا {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        }
    }

    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ لیدنه(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ لیدنه(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ لیدنه(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ لیدنه(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    فراغ لیدنه(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(تش_اشاره);
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        إذا (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_غوټه_FTask* ثابت newp = VN_AS(nodep->taskp()->user2p(), غوټه_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } وإلا إذا (!VN_IS(nodep, لاره_Call)) {
            nodep->taskp(تش_اشاره);
            VIsCached::clear_Cache_ونه();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(تش_اشاره);
        iterateChildren(nodep);
    }

    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
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
    V3ConstPool::سېټ_Scoped();
    V3Global::dump_Check_Global_ونه("scope", 0, dump_ونه_Either_Level() >= 3);
}
