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
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ثابت VNUser3InUse m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    using VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    AstNodeModule* m_modp = nullptr;  // Current module
    AstNodeProcedure* m_procedurep = nullptr;  // Current procedure
    AstScope* m_scopep = nullptr;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = nullptr;  // Cell that instantiates this module
    AstScope* m_aboveScopep = nullptr;  // Scope that instantiates this scope
    منطقي m_last = خطأ;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    size_t m_libTopNameLen = 0;

    std::unordered_map<AstNodeModule*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::vector<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::vector<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    ساكن فراغ countInstantiations(AstNodeModule* modp) {
        modp->user3Inc();
        لكل (AstNode* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            𐡄𐡍 (ثابت AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) {
                countInstantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    template <typename T_Node>
    T_Node* cloneOrMove(T_Node* nodep) {
        T_Node* clonep;
        𐡄𐡍 (m_last) {
            nodep->unlinkFrBack();
            clonep = nodep;
        } وإلا {
            clonep = nodep->cloneTree(خطأ);
        }
        nodep->user2p(clonep);
        m_scopep->addBlocksp(clonep);
        أرجع clonep;
    }

    فراغ cleanupVarRefs() {
        لكل (ثابت auto& itr : m_varRefScopes) {
            AstVarRef* ثابت nodep = itr.first;
            AstScope* scopep = itr.second;
            𐡄𐡍 (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                ثابت auto it2 = m_classOrPackageScopes.find(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.end(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->second;
            }
            nodep->classOrPackagep(nullptr);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = nullptr;
            AstScope* searchScopep = scopep;
            طالما (searchScopep) {
                ثابت auto it3 = m_varScopes.find(std::make_pair(nodep->varp(), searchScopep));
                𐡄𐡍 (it3 != m_varScopes.end()) {
                    varscp = it3->second;
                    اكسر;
                }
                searchScopep = searchScopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        AstNodeModule* ثابت modp = nodep->topModulep();
        𐡄𐡍 (!modp) {
            nodep->v3error("No top level module found");
            أرجع;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = nullptr;
        m_aboveScopep = nullptr;
        countInstantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        لكل (AstScope* ثابت scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        cleanupVarRefs();
    }
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_libTopNameLen);
        // Create required blocks and add to module
        string scopename;
        𐡄𐡍 (!m_aboveScopep) {
            scopename = "TOP";
        } وإلا {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            𐡄𐡍 (!v3Global.opt.libCreate().empty() && m_aboveScopep->isTop()
                && VN_IS(nodep, Module)) {
                m_libTopNameLen = scopename.size();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        AstNode::user1ClearTree();

        m_scopep = جديد AstScope{
            (m_aboveCellp ? static_cast<AstNode*>(m_aboveCellp) : static_cast<AstNode*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        𐡄𐡍 (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        لكل (AstNode* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            𐡄𐡍 (AstCell* ثابت cellp = VN_CAST(cellnextp, Cell)) cells.push_back(cellp);
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
                AstNodeModule* ثابت modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                𐡄𐡍 (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        AstNode::user1ClearTree();
        m_modp = nodep;
        𐡄𐡍 (m_modp->isTop()) {
            v3Global.rootp()->createTopScope(m_scopep);
        } وإلا {
            m_scopes.push_back(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_last);
            ثابت uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_last = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    فراغ visit(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        𐡄𐡍 (!m_aboveScopep) {
            scopename = "TOP";
        } وإلا {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        AstNode::user1ClearTree();

        ثابت AstNode* ثابت abovep
            = (m_aboveCellp ? static_cast<AstNode*>(m_aboveCellp) : static_cast<AstNode*>(nodep));
        m_scopep
            = جديد AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        AstNode::user1ClearTree();
        m_scopes.push_back(m_scopep);

        iterateChildren(nodep);
    }
    فراغ visit(AstCellInline* nodep) override {  //
        𐡄𐡍 (!v3Global.opt.vpi()) أرجع;
        m_scopep->addInlinesp(
            جديد AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    فراغ visit(AstActive* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    فراغ visit(AstNodeProcedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ visit(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ visit(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ visit(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    فراغ visit(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* ثابت clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    فراغ visit(AstNodeFTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        AstNodeFTask* ثابت clonep = [&]() {
            VL_RESTORER(m_last);
            // Only one scope will be created, so avoid pointless cloning
            𐡄𐡍 (nodep->classMethod()) m_last = 𐡒𐡔𐡈;
            أرجع cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    فراغ visit(AstVar* nodep) override {
        // Make new scope variable
        𐡄𐡍 (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            𐡄𐡍 (ثابت AstIfaceRefDType* ثابت ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                𐡄𐡍 (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* ثابت varscp = جديد AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            𐡄𐡍 (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(خطأ);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->addVarsp(varscp);
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_back(nodep, m_scopep);
    }
    فراغ visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        𐡄𐡍 (m_libTopNameLen) scopeName = "TOP" + scopeName.substr(m_libTopNameLen);
        ثابت string prefix = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(prefix + nodep->scopeAttr());
        nodep->scopeEntr(prefix + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    فراغ visit(AstScope* nodep) override {
        UASSERT_OBJ(nodep->isTop(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = افتراضي;
};

//######################################################################
// Scope cleanup -- remove unused activates

صنف ScopeCleanupVisitor final : عام VNVisitor {
    // STATE
    AstScope* m_scopep = nullptr;  // Current scope we are building

    // METHODS

    // VISITORS
    فراغ visit(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    virtual فراغ movedDeleteOrIterate(AstNode* nodep) {
        𐡄𐡍 (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } وإلا {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        }
    }

    فراغ visit(AstNodeProcedure* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ visit(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ visit(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ visit(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ visit(AstNodeFTask* nodep) override { movedDeleteOrIterate(nodep); }
    فراغ visit(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    فراغ visit(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(nullptr);
    }
    فراغ visit(AstNodeFTaskRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        𐡄𐡍 (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            AstNodeFTask* ثابت newp = VN_AS(nodep->taskp()->user2p(), NodeFTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } وإلا 𐡄𐡍 (!VN_IS(nodep, MethodCall)) {
            nodep->taskp(nullptr);
            VIsCached::clearCacheTree();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(nullptr);
        iterateChildren(nodep);
    }

    //--------------------
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
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
    V3ConstPool::setScoped();
    V3Global::dumpCheckGlobalTree("scope", 0, dumpTreeEitherLevel() >= 3);
}
