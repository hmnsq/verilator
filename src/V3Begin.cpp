// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Removal of named begin blocks
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
// V3Begin's Transformations:
//
// Each module:
//      Look for BEGINs
//          BEGIN(VAR...) -> VAR ... {renamed}
//      FOR -> WHILEs
//      Move static function variables and their AstInitialStatic blocks before a function
//
// There are two scopes; named BEGINs change %m and variable scopes.
// Unnamed BEGINs change only variable, not $display("%m") scope.
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Begin.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਤਾਰਕਿਕ m_any_Func_ਵਿੱਚ_Begin = ਝੂਠ;

ਜਨਤਕ:
    BeginState() = ਮੂਲ;
    ~BeginState() = ਮੂਲ;
    ਖਾਲੀ user_Mark_ਬਦਲਿਆ(Ast_ਗੰਢ* nodep) {
        nodep->user1(ਸੱਚ);
        m_any_Func_ਵਿੱਚ_Begin = ਸੱਚ;
    }
    ਤਾਰਕਿਕ any_Func_ਵਿੱਚ_Begin() ਅਚਲ { ਵਾਪਸ m_any_Func_ਵਿੱਚ_Begin; }
};

//######################################################################

ਵਰਗ BeginVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ਅਚਲ m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    AstCellInline* m_inlinesp = ਖਾਲੀ_ਸੂਚਕ;  // AstCellInlines to add to m_modp
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    Ast_ਗੰਢ_Procedure* m_procedurep = ਖਾਲੀ_ਸੂਚਕ;  // Current procedure
    Ast_ਗੰਢ* m_liftedp = ਖਾਲੀ_ਸੂਚਕ;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    ਪੂਰਨ m_if_ਡੂੰਘਾਈ = 0;  // Current if depth
    ਤਾਰਕਿਕ m_keepBegins = ਝੂਠ;  // True if begins should not be inlined

    // METHODS

    string dot(ਅਚਲ string& a, ਅਚਲ string& b) { ਵਾਪਸ VString::dot(a, "__DOT__", b); }

    ਖਾਲੀ dot_ਨਾਂ(ਅਚਲ std::string& name, File_ਸਤਰ* ਅਚਲ flp, ਅਚਲ ਅੱਖਰ* ਅਚਲ ਖੰਡ_Name) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        ਜੇ (name == "") ਵਾਪਸ;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::ਮਾਪ_type pos;
        ਜਦੋਂ ((pos = dottedname.ਲੱਭੋ("__DOT__")) != std::string::npos) {
            ਅਚਲ std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::ਲੜੀ_ਲੰਬਾਈ("__DOT__"));
            ਜੇ (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            ਜੇ (!m_ftaskp) {
                AstCellInline* ਅਚਲ inlinep = ਨਵਾਂ AstCellInline{flp, m_unnamedScope, ਖੰਡ_Name};
                m_inlinesp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(m_inlinesp, inlinep);
            }
        }
    }

    ਖਾਲੀ lift_ਗੰਢ(Ast_ਗੰਢ* nodep) {
        nodep->unlink_Fr_ਪਿੱਠ();
        ਜੇ (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            ਜੇ (!m_liftedp) {
                m_liftedp = nodep;
            } ਨਹੀਂਤਾਂ {
                m_liftedp->ਜੋੜੋ_ਅਗਲਾ(nodep);
            }
        } ਨਹੀਂਤਾਂ {
            // Move to module
            m_modp->ਜੋੜੋ_Stmtsp(nodep);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convert_ਮੰਜ਼ਿਲ_While(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        ਜੇ (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_ਬਦਲਿਆ(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = ਖਾਲੀ_ਸੂਚਕ;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        ਜੇ (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_ਬਦਲਿਆ(nodep);
        }
        // BEGIN wrapping a function rename that function, but don't affect
        // the inside function's variables.  We then restart with empty
        // naming; so that any begin's inside the function will rename
        // inside the function.
        // Process children
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_liftedp);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        m_displayScope = dot(m_displayScope, nodep->name());

        m_ftaskp = nodep;
        m_liftedp = ਖਾਲੀ_ਸੂਚਕ;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* ਅਚਲ initp) {
            initp->unlink_Fr_ਪਿੱਠ();
            m_ftaskp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dot_ਨਾਂ(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_ਅਗਲਾ_Null(nodep->itemsp());
        // Repalce node with body then delete
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ itemsp = nodep->itemsp()) {
            nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(itemsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        dot_ਨਾਂ(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ਸੱਚ;
            iterate_And_ਅਗਲਾ_Null(nodep->forksp());
        }
        Ast_ਗੰਢ* addsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ declsp = nodep->declsp()) {
            declsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            addsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(addsp, declsp);
        }
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            addsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(addsp, stmtsp);
        }
        ਜੇ (addsp) nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(addsp);
        nodep->name("");
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ਝੂਠ;
            dot_ਨਾਂ(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        ਜੇ (m_keepBegins) {
            nodep->name("");
            ਵਾਪਸ;
        }
        Ast_ਗੰਢ* addsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ declsp = nodep->declsp()) {
            declsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            addsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(addsp, declsp);
        }
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            addsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(addsp, stmtsp);
        }
        ਜੇ (addsp) {
            nodep->replaceWith(addsp);
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਖੰਡ* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dot_ਨਾਂ(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        ਜੇ (m_keepBegins) {
            nodep->name("");
            ਵਾਪਸ;
        }
        Ast_ਗੰਢ* addsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ declsp = nodep->declsp()) {
            declsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            addsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(addsp, declsp);
        }
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            addsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(addsp, stmtsp);
        }
        ਜੇ (addsp) {
            nodep->replaceWith(addsp);
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) { ਵਾਪਸ; }
        // If static variable, move it outside a function.
        ਜੇ (nodep->lifetime().isStatic() && m_ftaskp) {
            ਅਚਲ std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            ਜੇ (nodep->isIO()) {
                ਜੇ (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ਅਚਲ portp = nodep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                nodep->replaceWith(portp);

                ਜੇ (nodep->is_ਨਿਵੇਸ਼() || nodep->isInout()) {
                    Ast_ਸੌਂਪੋ* ਅਚਲ initAssignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                        nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        ਨਵਾਂ AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(initAssignp);
                }

                ਜੇ (nodep->isWritable()) {
                    Ast_ਸੌਂਪੋ* ਅਚਲ ਸਿਰਾ_Assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                        nodep->fileline(), ਨਵਾਂ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        ਨਵਾਂ AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->ਜੋੜੋ_Stmtsp(ਸਿਰਾ_Assignp);
                }
            } ਨਹੀਂਤਾਂ {
                nodep->unlink_Fr_ਪਿੱਠ();
            }
            nodep->name(newName);
            m_ftaskp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(nodep);
            nodep->funcLocal(ਝੂਠ);
        } ਨਹੀਂਤਾਂ ਜੇ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_ਬਦਲਿਆ(nodep);
            // Move it under enclosing tree
            lift_ਗੰਢ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Don't double-add text's
        Ast_ਗੰਢ* wasUnderp = m_ftaskp;
        ਜੇ (!m_ftaskp) wasUnderp = m_procedurep;
        ਜੇ (wasUnderp) {
            ਜੇ (nodep->stmtsp()) {
                Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()};
                wasUnderp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedef* nodep) override {
        ਜੇ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_ਬਦਲਿਆ(nodep);
            // Move it under enclosing tree
            lift_ਗੰਢ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        ਜੇ (m_namedScope != "") {
            m_statep->user_Mark_ਬਦਲਿਆ(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlink_Fr_ਪਿੱਠ();
            m_modp->ਜੋੜੋ_Stmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        ਜੇ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        ਜੇ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ਅਚਲ std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        ਜੇ (!scname.ਖਾਲੀ_ਥਾਵਾਂ()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    ਖਾਲੀ ਦੇਖੋ(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ਝੂਠ;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_if_ਡੂੰਘਾਈ);
        ਜੇ (m_if_ਡੂੰਘਾਈ == -1 || v3Global.opt.if_ਡੂੰਘਾਈ() < 1) {  // Turned off
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->ਤਰਜੀਹ_Pragma()) {
            m_if_ਡੂੰਘਾਈ = -1;
        } ਨਹੀਂਤਾਂ ਜੇ (++m_if_ਡੂੰਘਾਈ > v3Global.opt.if_ਡੂੰਘਾਈ()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::IFDEPTH, ਸੱਚ);  // Warn only once
            m_if_ਡੂੰਘਾਈ = -1;
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ਝੂਠ;
        iterateChildren(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = ਮੂਲ;
};

//######################################################################

ਵਰਗ BeginRelinkVisitor final : ਜਨਤਕ VNVisitorConst {
    // Replace tasks with new pointer
ਨਿਜੀ:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        ਜੇ (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        ਜੇ (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = ਮੂਲ;
};

//######################################################################
// Task class functions

ਖਾਲੀ V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        ਜੇ (state.any_Func_ਵਿੱਚ_Begin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("begin", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਸਥਿਰ Ast_ਗੰਢ* createForeachLoop(Ast_ਗੰਢ_Foreach* /*nodep*/, Ast_ਗੰਢ* bodysp, ਤਾਰਕਿਕ arrayMayResize,
                                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* subfromp, AstVar* varp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* leftp,
                                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* rightp, VNType ਗੰਢ_Type) {
    File_ਸਤਰ* ਅਚਲ fl = varp->fileline();
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* varRefp = ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp;
    ਤਾਰਕਿਕ inc = ਸੱਚ;
    ਚੋਣ (ਗੰਢ_Type) {
    ਹਾਲਤ VNType::LteS: condp = ਨਵਾਂ AstLteS{fl, varRefp, rightp}; ਰੋਕੋ;
    ਹਾਲਤ VNType::Lt: condp = ਨਵਾਂ AstLt{fl, varRefp, rightp}; ਰੋਕੋ;
    ਹਾਲਤ VNType::GteS:
        condp = ਨਵਾਂ AstGteS{fl, varRefp, rightp};
        inc = ਝੂਠ;
        ਰੋਕੋ;
    ਮੂਲ: varp->v3fatalSrc("Missing comparison handling"); ਰੋਕੋ;
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* incp;
    ਜੇ (inc)
        incp = ਨਵਾਂ Ast_ਜੋੜੋ{fl, varRefp->clone_ਰੁੱਖ(ਝੂਠ), ਨਵਾਂ AstConst{fl, 1}};
    ਨਹੀਂਤਾਂ
        incp = ਨਵਾਂ AstSub{fl, varRefp->clone_ਰੁੱਖ(ਝੂਠ), ਨਵਾਂ AstConst{fl, 1}};

    AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{fl};
    loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{fl, loopp, condp});
    AstVar* ਮਾਪ_Varp = ਖਾਲੀ_ਸੂਚਕ;
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮਾਪ_Getp = ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (arrayMayResize) {
        ਮਾਪ_Varp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->ਲੱਭੋ_UInt32_DType()};
        ਮਾਪ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ਮਾਪ_Varp->ਵਰਤਿਆ_Loop_Idx(ਸੱਚ);  // Not technically an index, but used only inside loop
        varp->ਜੋੜੋ_ਅਗਲਾ(ਮਾਪ_Varp);
        ਮਾਪ_Getp
            = ਨਵਾਂ AstCMethodHard{fl, subfromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), VCMethod::DYN_ਮਾਪ, ਖਾਲੀ_ਸੂਚਕ};
        ਮਾਪ_Getp->dtype_ਸੈੱਟ_UInt32();
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, ਮਾਪ_Varp, VAccess::WRITE}, ਮਾਪ_Getp});
    }
    loopp->ਜੋੜੋ_Stmtsp(bodysp);
    Ast_ਗੰਢ_Stmt* incStmtp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, incp};
    ਜੇ (arrayMayResize) {
        incStmtp = ਨਵਾਂ AstIf{fl,
                             ਨਵਾਂ AstLte{fl, ਨਵਾਂ AstVarRef{fl, ਮਾਪ_Varp, VAccess::READ},
                                        ਮਾਪ_Getp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)},
                             incStmtp};
    }
    loopp->ਜੋੜੋ_Stmtsp(incStmtp);
    Ast_ਗੰਢ* ਅਚਲ stmtsp = varp;  // New statements for outer loop
    stmtsp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->ਜੋੜੋ_ਅਗਲਾ(loopp);
    ਵਾਪਸ stmtsp;
}
ਸਥਿਰ Ast_ਗੰਢ* createForeachLoopRanged(Ast_ਗੰਢ_Foreach* nodep, Ast_ਗੰਢ* bodysp,
                                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* subfromp, AstVar* varp,
                                        ਅਚਲ VNumRange& declRange) {
    File_ਸਤਰ* ਅਚਲ fl = varp->fileline();
    V3_ਸੰਖਿਆ ਖੱਬੇ{nodep, 32};
    V3_ਸੰਖਿਆ ਸੱਜੇ{nodep, 32};
    ਖੱਬੇ.isSigned(ਸੱਚ);
    ਸੱਜੇ.isSigned(ਸੱਚ);
    ਖੱਬੇ.ਸੈੱਟ_Long_S(declRange.ਖੱਬੇ());
    ਸੱਜੇ.ਸੈੱਟ_Long_S(declRange.ਸੱਜੇ());
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ leftp = ਨਵਾਂ AstConst{fl, ਖੱਬੇ};
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rightp = ਨਵਾਂ AstConst{fl, ਸੱਜੇ};
    ਵਾਪਸ createForeachLoop(nodep, bodysp, ਝੂਠ /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.ਖੱਬੇ() <= declRange.ਸੱਜੇ() ? VNType::LteS : VNType::GteS);
}
ਸਥਿਰ Ast_ਗੰਢ* createForeachAssoc(File_ਸਤਰ* fl, AstVar* varp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* subfromp,
                                   Ast_ਗੰਢ_DType* fromDtp, Ast_ਗੰਢ* bodyPointp) {
    Ast_ਗੰਢ* loopp = varp;
    AstVar* ਅਚਲ ਅਗਲਾ_varp  // Iterator containing next element (to handle mid-array delete)
        = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    ਅਗਲਾ_varp->ਵਰਤਿਆ_Loop_Idx(ਸੱਚ);
    ਅਗਲਾ_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->ਜੋੜੋ_ਅਗਲਾ(ਅਗਲਾ_varp);

    AstVar* ਅਚਲ ਹੋਰ_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed{}, 1};
    ਹੋਰ_varp->ਵਰਤਿਆ_Loop_Idx(ਸੱਚ);  // Not technically an index, but used only inside loop
    ਹੋਰ_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->ਜੋੜੋ_ਅਗਲਾ(ਹੋਰ_varp);

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ firstp
        = ਨਵਾਂ AstCMethodHard{fl, subfromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), VCMethod::ASSOC_ਪਹਿਲਾ,
                             ਨਵਾਂ AstVarRef{fl, ਅਗਲਾ_varp, VAccess::READWRITE}};
    firstp->dtype_ਸੈੱਟ_Int();
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nextp
        = ਨਵਾਂ AstCMethodHard{fl, subfromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), VCMethod::ASSOC_ਅਗਲਾ,
                             ਨਵਾਂ AstVarRef{fl, ਅਗਲਾ_varp, VAccess::READWRITE}};
    nextp->dtype_ਸੈੱਟ_Int();

    // _Vmore = array.first(__Vnext)
    loopp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, ਹੋਰ_varp, VAccess::WRITE},
                                 ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ਅਚਲ lp = ਨਵਾਂ AstLoop{fl};
    loopp->ਜੋੜੋ_ਅਗਲਾ(lp);
    lp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{fl, lp, ਨਵਾਂ AstVarRef{fl, ਹੋਰ_varp, VAccess::READ}});

    // index = __Vnext
    lp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                                ਨਵਾਂ AstVarRef{fl, ਅਗਲਾ_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, ਹੋਰ_varp, VAccess::WRITE},
                                ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstConst{fl, 0}, nextp}});
    lp->ਜੋੜੋ_Stmtsp(bodyPointp);
    ਵਾਪਸ loopp;
}

ਸਥਿਰ ਤਾਰਕਿਕ arrayMayResizeCheck(Ast_ਗੰਢ* nodesp, AstVar* fromVarp) {
    // IEEE 1800-2023 12.7.3 "If the dimensions of a dynamically sized
    // array are changed while iterating over a foreach-loop construct, th
    // results are undefined".  However UVM calls delete within a foreach
    // in the uvm_reg code and in the tests, and expects the foreach to see
    // all elements post-delete.
    //
    // Cannot just check body for delete() because the body may call a
    // function that does a delete.  Cannot easily have the verilated_types
    // code set a flag on delete, because might be multiple foreach in
    // flight. Could use generation counter and each loop checks, but adds
    // time to every array modification. So, we assume if the array
    // shrinks, we should repeat the loop.
    ਜੇ (!nodesp) ਵਾਪਸ ਝੂਠ;
    ਵਾਪਸ nodesp->exists_And_ਅਗਲਾ([&](ਅਚਲ Ast_ਗੰਢ* nodep) -> ਤਾਰਕਿਕ {
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* varrefp = VN_CAST(nodep, ਗੰਢ_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            ਜੇ (varrefp->access().isWriteOrRW()) {
                ਜੇ (!fromVarp) ਵਾਪਸ ਸੱਚ;
                ਜੇ (varrefp->varp() == fromVarp) ਵਾਪਸ ਸੱਚ;
            }
        }
        // No idea what a function might be doing
        ਜੇ (VN_IS(nodep, ਵਿਧੀ_Call) || VN_IS(nodep, ਗੰਢ_FTask_Ref)) ਵਾਪਸ ਸੱਚ;
        ਵਾਪਸ ਝੂਠ;
    });
}

Ast_ਗੰਢ* V3Begin::convert_ਮੰਜ਼ਿਲ_While(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ਅਚਲ Ast_Foreach_ਸਿਰਲੇਖ* ਅਚਲ headerp = nodep->headerp();
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ਅਚਲ fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : ਖਾਲੀ_ਸੂਚਕ;
    Ast_ਗੰਢ_DType* fromDtp = fromp->dtypep()->skipRefp();
    ਅਚਲ ਤਾਰਕਿਕ arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_ਗੰਢ* bodyPointp = ਨਵਾਂ AstBegin{nodep->fileline(), "[EditWrapper]", ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
    Ast_ਗੰਢ* newp = ਖਾਲੀ_ਸੂਚਕ;
    Ast_ਗੰਢ* lastp = nodep;
    AstVar* nestedIndexp = ਖਾਲੀ_ਸੂਚਕ;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* subfromp = fromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
    // Major dimension first
    ਲਈ (Ast_ਗੰਢ *argsp = headerp->elementsp(), *ਅਗਲਾ_argsp; argsp; argsp = ਅਗਲਾ_argsp) {
        ਅਗਲਾ_argsp = argsp->nextp();
        ਅਚਲ ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ = VN_IS(argsp, ਖਾਲੀ_ਥਾਵਾਂ_2);
        AstVar* ਅਚਲ varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || ਖਾਲੀ_ਥਾਵਾਂ, argsp, "Missing foreach loop variable");
        ਜੇ (varp) varp->unlink_Fr_ਪਿੱਠ()->ਵਰਤਿਆ_Loop_Idx(ਸੱਚ);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_ਸਤਰ* ਅਚਲ fl = argsp->fileline();
        ਜੇ (varp) {
            Ast_ਗੰਢ* loopp = ਖਾਲੀ_ਸੂਚਕ;
            VNRelinker handle;
            lastp->unlink_Fr_ਪਿੱਠ(&handle);
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ adtypep = VN_CAST(fromDtp, ਗੰਢ_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ adtypep = VN_CAST(fromDtp, BasicDType)) {
                ਜੇ (adtypep->isString()) {
                    AstConst* ਅਚਲ leftp = ਨਵਾਂ AstConst{fl, 0};
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rightp = ਨਵਾਂ AstLenN{fl, fromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } ਨਹੀਂਤਾਂ {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, ਕਤਾਰ_DType)) {
                AstConst* ਅਚਲ leftp = ਨਵਾਂ AstConst{fl, 0};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rightp = ਨਵਾਂ AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), ਗੰਢ_Array_DType)
                        ? ਨਵਾਂ AstArraySel{fl, subfromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                          ਨਵਾਂ AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                    VCMethod::DYN_ਮਾਪ};
                AstVarRef* varRefp = ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ};
                rightp->dtype_ਸੈੱਟ_Int();
                rightp->protect(ਝੂਠ);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = ਨਵਾਂ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ};
                subfromp = ਨਵਾਂ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            ਜੇ (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_ਰੁੱਖ(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_ਗੰਢ* ਅਚਲ bodyp = nodep->bodyp();
    ਜੇ (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        ਜੇ (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_ਰੁੱਖ(), bodyPointp);
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    ਜੇ (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
    } ਨਹੀਂਤਾਂ {
        bodyPointp->unlink_Fr_ਪਿੱਠ();
    }
    VL_DO_DANGLING(bodyPointp->delete_ਰੁੱਖ(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    ਵਾਪਸ newp;
}
