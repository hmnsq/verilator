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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Scope.h"

#అన్తర్భావ "V3ConstPool.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Scope class functions

తరగతి ScopeVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstVar::user1p           -> AstVarScope*.  Replacement for this variable
    // AstCell::user2p          -> AstScope*.  The scope created inside the cell
    // AstTask::user2p          -> AstTask*.  Replacement task
    // AstNodeModule::user3()   -> uint64_t.  Pending instantiations of this module
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;

    // TYPES
    // These cannot be unordered unless make a specialized hashing pair (gcc-8)
    ఉపయోగించి VarScopeMap = std::map<std::pair<AstVar*, AstScope*>, AstVarScope*>;

    // STATE, inside processing a single module
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    Ast_కణుపు_Procedure* m_procedurep = శూన్యసూచిక;  // Current procedure
    AstScope* m_scopep = శూన్యసూచిక;  // Current scope we are building
    // STATE, for passing down one level of hierarchy (may need save/restore)
    AstCell* m_aboveCellp = శూన్యసూచిక;  // Cell that instantiates this module
    AstScope* m_aboveScopep = శూన్యసూచిక;  // Scope that instantiates this scope
    తార్కిక m_చివరి = అబద్ధం;  // Scoping the last instantiation of the current module
    // The name length of the scope of the library top module instance, while within it
    కొలత_t m_lib_శిఖరం_Name_Len = 0;

    std::unordered_map<Ast_కణుపు_Module*, AstScope*>
        m_classOrPackageScopes;  // Scopes for each class or package
    VarScopeMap m_varScopes;  // Varscopes created for each scope and var
    // Varrefs-in-scopes needing fixup when done
    std::సదిశ<std::pair<AstVarRef*, AstScope*>> m_varRefScopes;
    std::సదిశ<AstScope*> m_scopes;  // Scopes awaiting attachment to their modules

    // METHODS

    // Count module instantiations, visiting cells the same way the traversal below does
    స్థిర శూన్య లెక్క_Instantiations(Ast_కణుపు_Module* modp) {
        modp->user3Inc();
        కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstCell* స్థిరరాశి cellp = VN_CAST(stmtp, Cell)) {
                లెక్క_Instantiations(cellp->modp());
            }
        }
    }

    // Copy, or move (on the last instantiation), the given node under
    // the current scope, and return the version now under the scope
    మూస <రకం_పేరు T_కణుపు>
    T_కణుపు* cloneOrMove(T_కణుపు* nodep) {
        T_కణుపు* clonep;
        ఒకవేళ (m_చివరి) {
            nodep->unlink_Fr_వెనుక();
            clonep = nodep;
        } లేకపోతే {
            clonep = nodep->clone_వృక్షం(అబద్ధం);
        }
        nodep->user2p(clonep);
        m_scopep->కూడు_Blocksp(clonep);
        తిరిగి clonep;
    }

    శూన్య cleanupVarRefs() {
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_varRefScopes) {
            AstVarRef* స్థిరరాశి nodep = itr.మొదటి;
            AstScope* scopep = itr.రెండవ;
            ఒకవేళ (nodep->classOrPackagep()
                && !VN_IS(nodep->classOrPackagep(),
                          Module)) {  // Module scopes are not in m_classOrPackageScopes
                స్థిరరాశి స్వయంచాలక it2 = m_classOrPackageScopes.కనుగొను(nodep->classOrPackagep());
                UASSERT_OBJ(it2 != m_classOrPackageScopes.చివర(), nodep,
                            "Can't locate class or package scope");
                scopep = it2->రెండవ;
            }
            nodep->classOrPackagep(శూన్యసూచిక);  // No longer needed after V3Scope
            // Search up the scope hierarchy for the variable
            AstVarScope* varscp = శూన్యసూచిక;
            AstScope* శోధన_Scopep = scopep;
            అయితే (శోధన_Scopep) {
                స్థిరరాశి స్వయంచాలక it3 = m_varScopes.కనుగొను(std::make_pair(nodep->varp(), శోధన_Scopep));
                ఒకవేళ (it3 != m_varScopes.చివర()) {
                    varscp = it3->రెండవ;
                    ఆపు;
                }
                శోధన_Scopep = శోధన_Scopep->aboveScopep();
            }
            UASSERT_OBJ(varscp, nodep, "Can't locate varref scope");
            nodep->varScopep(varscp);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        Ast_కణుపు_Module* స్థిరరాశి modp = nodep->శిఖరం_Modulep();
        ఒకవేళ (!modp) {
            nodep->v3error("No top level module found");
            తిరిగి;
        }
        // Operate starting at the top of the hierarchy
        m_aboveCellp = శూన్యసూచిక;
        m_aboveScopep = శూన్యసూచిక;
        లెక్క_Instantiations(modp);
        iterate(modp);
        // Defer attachment until all instances are visited so cell collection and child
        // iteration do not rescan a growing list of completed scopes.
        కొరకు (AstScope* స్థిరరాశి scopep : m_scopes) scopep->modp()->కూడు_Stmtsp(scopep);
        cleanupVarRefs();
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_lib_శిఖరం_Name_Len);
        // Create required blocks and add to module
        string scopename;
        ఒకవేళ (!m_aboveScopep) {
            scopename = "TOP";
        } లేకపోతే {
            scopename = m_aboveScopep->name() + "." + m_aboveCellp->name();
            // The top module is the only module directly under 'TOP'
            ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు() && m_aboveScopep->is_శిఖరం()
                && VN_IS(nodep, Module)) {
                m_lib_శిఖరం_Name_Len = scopename.కొలత();
            }
        }

        UINFO(4, " MOD AT " << scopename << "  " << nodep);
        Ast_కణుపు::user1_Clear_వృక్షం();

        m_scopep = కొత్త AstScope{
            (m_aboveCellp ? static_cast<Ast_కణుపు*>(m_aboveCellp) : static_cast<Ast_కణుపు*>(nodep))
                ->fileline(),
            nodep, scopename, m_aboveScopep, m_aboveCellp};
        ఒకవేళ (VN_IS(nodep, Package)) m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Get list of cells before we edit, to avoid excess visits (issue #6059)
        std::deque<AstCell*> cells;
        కొరకు (Ast_కణుపు* cellnextp = nodep->stmtsp(); cellnextp; cellnextp = cellnextp->nextp()) {
            ఒకవేళ (AstCell* స్థిరరాశి cellp = VN_CAST(cellnextp, Cell)) cells.నెట్టు_వెనుక(cellp);
        }

        // Now for each child cell, iterate the module this cell points to
        కొరకు (AstCell* స్థిరరాశి cellp : cells) {
            VL_RESTORER(m_scopep);  // Protects m_scopep set in called module
            // which is "above" in this code, but later in code execution order
            VL_RESTORER(m_aboveCellp);
            VL_RESTORER(m_aboveScopep);
            {
                m_aboveCellp = cellp;
                m_aboveScopep = m_scopep;
                Ast_కణుపు_Module* స్థిరరాశి modp = cellp->modp();
                UASSERT_OBJ(modp, cellp, "Unlinked mod");
                iterate(modp);  // Recursive call to visit(AstNodeModule)
                ఒకవేళ (VN_IS(modp, Iface)) {
                    // Remember newly created scope
                    cellp->user2p(m_scopep);
                }
            }
        }

        // Create scope for the current usage of this module
        UINFO(4, " back AT " << scopename << "  " << nodep);
        Ast_కణుపు::user1_Clear_వృక్షం();
        m_modp = nodep;
        ఒకవేళ (m_modp->is_శిఖరం()) {
            v3Global.rootp()->create_శిఖరం_Scope(m_scopep);
        } లేకపోతే {
            m_scopes.నెట్టు_వెనుక(m_scopep);
        }

        // Copy blocks into this scope
        {
            VL_RESTORER(m_చివరి);
            స్థిరరాశి uint64_t nInsts = nodep->user3();
            UASSERT_OBJ(nInsts, nodep, "Module scoped more times than instantiated");
            nodep->user3(nInsts - 1);
            m_చివరి = nInsts == 1;
            iterateChildren(nodep);
        }

        // ***Note m_scopep is passed back to the caller of the routine (above)
    }
    శూన్య సందర్శించు(AstClass* nodep) override {
        // Create required blocks and add to module
        VL_RESTORER(m_scopep);
        VL_RESTORER(m_aboveCellp);
        VL_RESTORER(m_aboveScopep);
        VL_RESTORER(m_modp);
        m_aboveScopep = m_scopep;
        m_modp = nodep;

        string scopename;
        ఒకవేళ (!m_aboveScopep) {
            scopename = "TOP";
        } లేకపోతే {
            scopename = m_aboveScopep->name() + "." + nodep->name();
        }

        UINFO(4, " CLASS AT " << scopename << "  " << nodep);
        Ast_కణుపు::user1_Clear_వృక్షం();

        స్థిరరాశి Ast_కణుపు* స్థిరరాశి abovep
            = (m_aboveCellp ? static_cast<Ast_కణుపు*>(m_aboveCellp) : static_cast<Ast_కణుపు*>(nodep));
        m_scopep
            = కొత్త AstScope{abovep->fileline(), m_modp, scopename, m_aboveScopep, m_aboveCellp};
        m_classOrPackageScopes.emplace(nodep, m_scopep);

        // Create scope for the current usage of this cell
        Ast_కణుపు::user1_Clear_వృక్షం();
        m_scopes.నెట్టు_వెనుక(m_scopep);

        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCellInline* nodep) override {  //
        ఒకవేళ (!v3Global.opt.vpi()) తిరిగి;
        m_scopep->కూడు_Inlinesp(
            కొత్త AstCellInlineScope{nodep->fileline(), nodep->name(), nodep->origModName()});
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Actives now made after scoping");
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        // Add to list of blocks under this scope
        // Check don't miss varref scope assignments
        UASSERT_OBJ(!m_procedurep, nodep, "prodedure in procedure");
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    శూన్య సందర్శించు(AstAlias* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    శూన్య సందర్శించు(AstAliasScope* nodep) override {
        // Copy under the scope but don't recurse
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    శూన్య సందర్శించు(AstCoverToggle* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    Move " << nodep);
        // We iterate under the *clone*
        iterateChildren(cloneOrMove(nodep));
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    CFUNC " << nodep);
        AstCFunc* స్థిరరాశి clonep = cloneOrMove(nodep);
        clonep->scopep(m_scopep);
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        // Add to list of blocks under this scope
        UINFO(4, "    FTASK " << nodep);
        Ast_కణుపు_FTask* స్థిరరాశి clonep = [&]() {
            VL_RESTORER(m_చివరి);
            // Only one scope will be created, so avoid pointless cloning
            ఒకవేళ (nodep->class_పద్ధతి()) m_చివరి = నిజం;
            తిరిగి cloneOrMove(nodep);
        }();
        clonep->user2p(clonep);  // For recursive self-references after cloneTree
        // We iterate under the *clone*
        iterateChildren(clonep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        // Make new scope variable
        ఒకవేళ (!nodep->user1p()) {
            AstScope* scopep = m_scopep;
            ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifrefp = VN_CAST(nodep->dtypep(), IfaceRefDType)) {
                // Attach every non-virtual interface variable its inner scope
                ఒకవేళ (ifrefp->cellp()) scopep = VN_AS(ifrefp->cellp()->user2p(), Scope);
            }
            AstVarScope* స్థిరరాశి varscp = కొత్త AstVarScope{nodep->fileline(), scopep, nodep};
            UINFO(6, "   New scope " << varscp);
            ఒకవేళ (m_aboveCellp && !m_aboveCellp->isTrace()) varscp->trace(అబద్ధం);
            nodep->user1p(varscp);
            UASSERT_OBJ(m_scopep, nodep, "No scope for var");
            m_varScopes.emplace(std::make_pair(nodep, m_scopep), varscp);
            m_scopep->కూడు_Varsp(varscp);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // VarRef needs to point to VarScope
        // Make sure variable has made user1p.
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        // We may have not made the variable yet, and we can't make it now as
        // the var's referenced package etc might not be created yet.
        // So push to a list and post-correct.
        // No check here for nodep->classOrPackagep(), will check when walk list.
        m_varRefScopes.emplace_వెనుక(nodep, m_scopep);
    }
    శూన్య సందర్శించు(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        std::string scopeName = m_scopep->name();
        // Drop the "l2-name" (name of the top module) for library builds, otherwise
        // would get "top.u_lib.lib.foo" instead of correct hierarchical path "top.u_lib.foo"
        ఒకవేళ (m_lib_శిఖరం_Name_Len) scopeName = "TOP" + scopeName.substr(m_lib_శిఖరం_Name_Len);
        స్థిరరాశి string ఉపసర్గ = "__DOT__"s + scopeName;
        // TOP and above will be the user's name().
        // Note 'TOP.' is stripped by scopePrettyName
        // To keep correct visual order, must add before existing
        nodep->scopeAttr(ఉపసర్గ + nodep->scopeAttr());
        nodep->scopeEntr(ఉపసర్గ + nodep->scopeEntr());
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        UASSERT_OBJ(nodep->is_శిఖరం(), nodep,
                    "Non-top scopes must remain detached until all instances are scoped");
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ScopeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeVisitor() override = అప్రమేయ;
};

//######################################################################
// Scope cleanup -- remove unused activates

తరగతి ScopeCleanupVisitor final : బహిరంగ VNVisitor {
    // STATE
    AstScope* m_scopep = శూన్యసూచిక;  // Current scope we are building

    // METHODS

    // VISITORS
    శూన్య సందర్శించు(AstScope* nodep) override {
        // Want to ignore blocks under it
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    మాయా శూన్య movedDeleteOrIterate(Ast_కణుపు* nodep) {
        ఒకవేళ (m_scopep) {
            // The new block; repair varrefs
            iterateChildren(nodep);
        } లేకపోతే {
            // A block that was just moved under a scope, Kill it.
            // Certain nodes can be referenced later in this pass, notably
            // an FTaskRef needs to access the FTask to find the cloned task
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override { movedDeleteOrIterate(nodep); }
    శూన్య సందర్శించు(AstAlias* nodep) override { movedDeleteOrIterate(nodep); }
    శూన్య సందర్శించు(AstAliasScope* nodep) override { movedDeleteOrIterate(nodep); }
    శూన్య సందర్శించు(AstCoverToggle* nodep) override { movedDeleteOrIterate(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override { movedDeleteOrIterate(nodep); }
    శూన్య సందర్శించు(AstCFunc* nodep) override { movedDeleteOrIterate(nodep); }

    శూన్య సందర్శించు(AstVarXRef* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        nodep->varp(శూన్యసూచిక);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        // The crossrefs are dealt with in V3LinkDot
        UINFO(9, "   Old pkg-taskref " << nodep);
        ఒకవేళ (nodep->classOrPackagep()) {
            // Point to the clone
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
            Ast_కణుపు_FTask* స్థిరరాశి newp = VN_AS(nodep->taskp()->user2p(), కణుపు_FTask);
            UASSERT_OBJ(newp, nodep, "No clone for package function");
            nodep->taskp(newp);
            UINFO(9, "   New pkg-taskref " << nodep);
        } లేకపోతే ఒకవేళ (!VN_IS(nodep, పద్ధతి_Call)) {
            nodep->taskp(శూన్యసూచిక);
            VIsCached::clear_Cache_వృక్షం();
            UINFO(9, "   New pkg-taskref " << nodep);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstModportFTaskRef* nodep) override {
        // The modport persists only for JSON dump
        // The crossrefs are dealt with in V3LinkDot
        nodep->ftaskp(శూన్యసూచిక);
        iterateChildren(nodep);
    }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ScopeCleanupVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ScopeCleanupVisitor() override = అప్రమేయ;
};

//######################################################################
// Scope class functions

శూన్య V3Scope::scopeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        స్థిరరాశి ScopeVisitor visitor{nodep};
        ScopeCleanupVisitor{nodep};
    }  // Destruct before checking
    V3ConstPool::సమితి_Scoped();
    V3Global::dump_Check_Global_వృక్షం("scope", 0, dump_వృక్షం_Either_Level() >= 3);
}
