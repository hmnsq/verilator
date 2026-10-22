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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Begin.h"

#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ತಾರ್ಕಿಕ m_any_Func_ಒಳ_Begin = ಸುಳ್ಳು;

ಸಾರ್ವಜನಿಕ:
    BeginState() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~BeginState() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ user_Mark_ಬದಲಾಯಿತು(Ast_ಗಂಟು* nodep) {
        nodep->user1(ನಿಜ);
        m_any_Func_ಒಳ_Begin = ನಿಜ;
    }
    ತಾರ್ಕಿಕ any_Func_ಒಳ_Begin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_any_Func_ಒಳ_Begin; }
};

//######################################################################

ವರ್ಗ BeginVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ಸ್ಥಿರಾಂಕ m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    AstCellInline* m_inlinesp = ಶೂನ್ಯಸೂಚಕ;  // AstCellInlines to add to m_modp
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    Ast_ಗಂಟು_Procedure* m_procedurep = ಶೂನ್ಯಸೂಚಕ;  // Current procedure
    Ast_ಗಂಟು* m_liftedp = ಶೂನ್ಯಸೂಚಕ;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    ಪೂರ್ಣಾಂಕ m_if_ಆಳ = 0;  // Current if depth
    ತಾರ್ಕಿಕ m_keepBegins = ಸುಳ್ಳು;  // True if begins should not be inlined

    // METHODS

    string dot(ಸ್ಥಿರಾಂಕ string& a, ಸ್ಥಿರಾಂಕ string& b) { ಹಿಂತಿರುಗಿ VString::dot(a, "__DOT__", b); }

    ಶೂನ್ಯ dot_ಹೆಸರುಗಳು(ಸ್ಥಿರಾಂಕ std::string& name, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಖಂಡ_Name) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        ಆದರೆ (name == "") ಹಿಂತಿರುಗಿ;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::ಗಾತ್ರ_type pos;
        ಆಗ ((pos = dottedname.ಕಂಡುಹಿಡಿ("__DOT__")) != std::string::npos) {
            ಸ್ಥಿರಾಂಕ std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("__DOT__"));
            ಆದರೆ (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            ಆದರೆ (!m_ftaskp) {
                AstCellInline* ಸ್ಥಿರಾಂಕ inlinep = ಹೊಸ AstCellInline{flp, m_unnamedScope, ಖಂಡ_Name};
                m_inlinesp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(m_inlinesp, inlinep);
            }
        }
    }

    ಶೂನ್ಯ lift_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            ಆದರೆ (!m_liftedp) {
                m_liftedp = nodep;
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_liftedp->ಕೂಡಿಸು_ಮುಂದಿನ(nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Move to module
            m_modp->ಕೂಡಿಸು_Stmtsp(nodep);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convert_ಗಮ್ಯ_While(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        ಆದರೆ (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_ಬದಲಾಯಿತು(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = ಶೂನ್ಯಸೂಚಕ;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        ಆದರೆ (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_ಬದಲಾಯಿತು(nodep);
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
        m_liftedp = ಶೂನ್ಯಸೂಚಕ;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* ಸ್ಥಿರಾಂಕ initp) {
            initp->unlink_Fr_ಹಿಂಭಾಗ();
            m_ftaskp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dot_ಹೆಸರುಗಳು(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_ಮುಂದಿನ_Null(nodep->itemsp());
        // Repalce node with body then delete
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ itemsp = nodep->itemsp()) {
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(itemsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        dot_ಹೆಸರುಗಳು(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ನಿಜ;
            iterate_And_ಮುಂದಿನ_Null(nodep->forksp());
        }
        Ast_ಗಂಟು* addsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declsp = nodep->declsp()) {
            declsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            addsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(addsp, declsp);
        }
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            addsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(addsp, stmtsp);
        }
        ಆದರೆ (addsp) nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(addsp);
        nodep->name("");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ಸುಳ್ಳು;
            dot_ಹೆಸರುಗಳು(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        ಆದರೆ (m_keepBegins) {
            nodep->name("");
            ಹಿಂತಿರುಗಿ;
        }
        Ast_ಗಂಟು* addsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declsp = nodep->declsp()) {
            declsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            addsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(addsp, declsp);
        }
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            addsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(addsp, stmtsp);
        }
        ಆದರೆ (addsp) {
            nodep->replaceWith(addsp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಖಂಡ* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dot_ಹೆಸರುಗಳು(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        ಆದರೆ (m_keepBegins) {
            nodep->name("");
            ಹಿಂತಿರುಗಿ;
        }
        Ast_ಗಂಟು* addsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declsp = nodep->declsp()) {
            declsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            addsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(addsp, declsp);
        }
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            addsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(addsp, stmtsp);
        }
        ಆದರೆ (addsp) {
            nodep->replaceWith(addsp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) { ಹಿಂತಿರುಗಿ; }
        // If static variable, move it outside a function.
        ಆದರೆ (nodep->lifetime().isStatic() && m_ftaskp) {
            ಸ್ಥಿರಾಂಕ std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            ಆದರೆ (nodep->isIO()) {
                ಆದರೆ (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ಸ್ಥಿರಾಂಕ portp = nodep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                nodep->replaceWith(portp);

                ಆದರೆ (nodep->is_ಒಳಹರಿವು() || nodep->isInout()) {
                    Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ initAssignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                        nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        ಹೊಸ AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(initAssignp);
                }

                ಆದರೆ (nodep->isWritable()) {
                    Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ ಕೊನೆ_Assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                        nodep->fileline(), ಹೊಸ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        ಹೊಸ AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->ಕೂಡಿಸು_Stmtsp(ಕೊನೆ_Assignp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->unlink_Fr_ಹಿಂಭಾಗ();
            }
            nodep->name(newName);
            m_ftaskp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(nodep);
            nodep->funcLocal(ಸುಳ್ಳು);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_ಬದಲಾಯಿತು(nodep);
            // Move it under enclosing tree
            lift_ಗಂಟು(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Don't double-add text's
        Ast_ಗಂಟು* wasUnderp = m_ftaskp;
        ಆದರೆ (!m_ftaskp) wasUnderp = m_procedurep;
        ಆದರೆ (wasUnderp) {
            ಆದರೆ (nodep->stmtsp()) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()};
                wasUnderp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedef* nodep) override {
        ಆದರೆ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_ಬದಲಾಯಿತು(nodep);
            // Move it under enclosing tree
            lift_ಗಂಟು(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        ಆದರೆ (m_namedScope != "") {
            m_statep->user_Mark_ಬದಲಾಯಿತು(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            m_modp->ಕೂಡಿಸು_Stmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        ಆದರೆ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        ಆದರೆ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ಸ್ಥಿರಾಂಕ std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        ಆದರೆ (!scname.ಖಾಲಿಗಳು()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    ಶೂನ್ಯ ಭೇಟಿ(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ಸುಳ್ಳು;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_if_ಆಳ);
        ಆದರೆ (m_if_ಆಳ == -1 || v3Global.opt.if_ಆಳ() < 1) {  // Turned off
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->ಆದ್ಯತೆ_Pragma()) {
            m_if_ಆಳ = -1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (++m_if_ಆಳ > v3Global.opt.if_ಆಳ()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::IFDEPTH, ನಿಜ);  // Warn only once
            m_if_ಆಳ = -1;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ಸುಳ್ಳು;
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################

ವರ್ಗ BeginRelinkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // Replace tasks with new pointer
ಖಾಸಗಿ:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        ಆದರೆ (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        ಆದರೆ (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Task class functions

ಶೂನ್ಯ V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        ಆದರೆ (state.any_Func_ಒಳ_Begin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("begin", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಸ್ಥಿರ Ast_ಗಂಟು* createForeachLoop(Ast_ಗಂಟು_Foreach* /*nodep*/, Ast_ಗಂಟು* bodysp, ತಾರ್ಕಿಕ arrayMayResize,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subfromp, AstVar* varp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* leftp,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rightp, VNType ಗಂಟು_Type) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = varp->fileline();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* varRefp = ಹೊಸ AstVarRef{fl, varp, VAccess::READ};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp;
    ತಾರ್ಕಿಕ inc = ನಿಜ;
    ಆಯ್ಕೆ (ಗಂಟು_Type) {
    ಸ್ಥಿತಿ VNType::LteS: condp = ಹೊಸ AstLteS{fl, varRefp, rightp}; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VNType::Lt: condp = ಹೊಸ AstLt{fl, varRefp, rightp}; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VNType::GteS:
        condp = ಹೊಸ AstGteS{fl, varRefp, rightp};
        inc = ಸುಳ್ಳು;
        ನಿಲ್ಲಿಸು;
    ಪೂರ್ವನಿಯೋಜಿತ: varp->v3fatalSrc("Missing comparison handling"); ನಿಲ್ಲಿಸು;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* incp;
    ಆದರೆ (inc)
        incp = ಹೊಸ Ast_ಕೂಡಿಸು{fl, varRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, 1}};
    ಇಲ್ಲದಿದ್ದರೆ
        incp = ಹೊಸ AstSub{fl, varRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, 1}};

    AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{fl};
    loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{fl, loopp, condp});
    AstVar* ಗಾತ್ರ_Varp = ಶೂನ್ಯಸೂಚಕ;
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಗಾತ್ರ_Getp = ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (arrayMayResize) {
        ಗಾತ್ರ_Varp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->ಕಂಡುಹಿಡಿ_UInt32_DType()};
        ಗಾತ್ರ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ಗಾತ್ರ_Varp->ಬಳಸಿದ_Loop_Idx(ನಿಜ);  // Not technically an index, but used only inside loop
        varp->ಕೂಡಿಸು_ಮುಂದಿನ(ಗಾತ್ರ_Varp);
        ಗಾತ್ರ_Getp
            = ಹೊಸ AstCMethodHard{fl, subfromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), VCMethod::DYN_ಗಾತ್ರ, ಶೂನ್ಯಸೂಚಕ};
        ಗಾತ್ರ_Getp->dtype_ಗಣ_UInt32();
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, ಗಾತ್ರ_Varp, VAccess::WRITE}, ಗಾತ್ರ_Getp});
    }
    loopp->ಕೂಡಿಸು_Stmtsp(bodysp);
    Ast_ಗಂಟು_Stmt* incStmtp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, incp};
    ಆದರೆ (arrayMayResize) {
        incStmtp = ಹೊಸ AstIf{fl,
                             ಹೊಸ AstLte{fl, ಹೊಸ AstVarRef{fl, ಗಾತ್ರ_Varp, VAccess::READ},
                                        ಗಾತ್ರ_Getp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)},
                             incStmtp};
    }
    loopp->ಕೂಡಿಸು_Stmtsp(incStmtp);
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = varp;  // New statements for outer loop
    stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(loopp);
    ಹಿಂತಿರುಗಿ stmtsp;
}
ಸ್ಥಿರ Ast_ಗಂಟು* createForeachLoopRanged(Ast_ಗಂಟು_Foreach* nodep, Ast_ಗಂಟು* bodysp,
                                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subfromp, AstVar* varp,
                                        ಸ್ಥಿರಾಂಕ VNumRange& declRange) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = varp->fileline();
    V3_ಸಂಖ್ಯೆ ಎಡ{nodep, 32};
    V3_ಸಂಖ್ಯೆ ಬಲ{nodep, 32};
    ಎಡ.isSigned(ನಿಜ);
    ಬಲ.isSigned(ನಿಜ);
    ಎಡ.ಗಣ_Long_S(declRange.ಎಡ());
    ಬಲ.ಗಣ_Long_S(declRange.ಬಲ());
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ leftp = ಹೊಸ AstConst{fl, ಎಡ};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rightp = ಹೊಸ AstConst{fl, ಬಲ};
    ಹಿಂತಿರುಗಿ createForeachLoop(nodep, bodysp, ಸುಳ್ಳು /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.ಎಡ() <= declRange.ಬಲ() ? VNType::LteS : VNType::GteS);
}
ಸ್ಥಿರ Ast_ಗಂಟು* createForeachAssoc(File_ಸಾಲು* fl, AstVar* varp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subfromp,
                                   Ast_ಗಂಟು_DType* fromDtp, Ast_ಗಂಟು* bodyPointp) {
    Ast_ಗಂಟು* loopp = varp;
    AstVar* ಸ್ಥಿರಾಂಕ ಮುಂದಿನ_varp  // Iterator containing next element (to handle mid-array delete)
        = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    ಮುಂದಿನ_varp->ಬಳಸಿದ_Loop_Idx(ನಿಜ);
    ಮುಂದಿನ_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->ಕೂಡಿಸು_ಮುಂದಿನ(ಮುಂದಿನ_varp);

    AstVar* ಸ್ಥಿರಾಂಕ ಇನ್ನಷ್ಟು_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlag_ಬಿಟ್_Packed{}, 1};
    ಇನ್ನಷ್ಟು_varp->ಬಳಸಿದ_Loop_Idx(ನಿಜ);  // Not technically an index, but used only inside loop
    ಇನ್ನಷ್ಟು_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->ಕೂಡಿಸು_ಮುಂದಿನ(ಇನ್ನಷ್ಟು_varp);

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ firstp
        = ಹೊಸ AstCMethodHard{fl, subfromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), VCMethod::ASSOC_ಮೊದಲ,
                             ಹೊಸ AstVarRef{fl, ಮುಂದಿನ_varp, VAccess::READWRITE}};
    firstp->dtype_ಗಣ_Int();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nextp
        = ಹೊಸ AstCMethodHard{fl, subfromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), VCMethod::ASSOC_ಮುಂದಿನ,
                             ಹೊಸ AstVarRef{fl, ಮುಂದಿನ_varp, VAccess::READWRITE}};
    nextp->dtype_ಗಣ_Int();

    // _Vmore = array.first(__Vnext)
    loopp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, ಇನ್ನಷ್ಟು_varp, VAccess::WRITE},
                                 ಹೊಸ AstNeq{fl, ಹೊಸ AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ಸ್ಥಿರಾಂಕ lp = ಹೊಸ AstLoop{fl};
    loopp->ಕೂಡಿಸು_ಮುಂದಿನ(lp);
    lp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{fl, lp, ಹೊಸ AstVarRef{fl, ಇನ್ನಷ್ಟು_varp, VAccess::READ}});

    // index = __Vnext
    lp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                                ಹೊಸ AstVarRef{fl, ಮುಂದಿನ_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, ಇನ್ನಷ್ಟು_varp, VAccess::WRITE},
                                ಹೊಸ AstNeq{fl, ಹೊಸ AstConst{fl, 0}, nextp}});
    lp->ಕೂಡಿಸು_Stmtsp(bodyPointp);
    ಹಿಂತಿರುಗಿ loopp;
}

ಸ್ಥಿರ ತಾರ್ಕಿಕ arrayMayResizeCheck(Ast_ಗಂಟು* nodesp, AstVar* fromVarp) {
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
    ಆದರೆ (!nodesp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಹಿಂತಿರುಗಿ nodesp->exists_And_ಮುಂದಿನ([&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) -> ತಾರ್ಕಿಕ {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* varrefp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            ಆದರೆ (varrefp->access().isWriteOrRW()) {
                ಆದರೆ (!fromVarp) ಹಿಂತಿರುಗಿ ನಿಜ;
                ಆದರೆ (varrefp->varp() == fromVarp) ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        // No idea what a function might be doing
        ಆದರೆ (VN_IS(nodep, ವಿಧಾನ_Call) || VN_IS(nodep, ಗಂಟು_FTask_Ref)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    });
}

Ast_ಗಂಟು* V3Begin::convert_ಗಮ್ಯ_While(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ಸ್ಥಿರಾಂಕ Ast_Foreach_ಶೀರ್ಷಿಕೆ* ಸ್ಥಿರಾಂಕ headerp = nodep->headerp();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ಸ್ಥಿರಾಂಕ fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : ಶೂನ್ಯಸೂಚಕ;
    Ast_ಗಂಟು_DType* fromDtp = fromp->dtypep()->skipRefp();
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_ಗಂಟು* bodyPointp = ಹೊಸ AstBegin{nodep->fileline(), "[EditWrapper]", ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
    Ast_ಗಂಟು* newp = ಶೂನ್ಯಸೂಚಕ;
    Ast_ಗಂಟು* lastp = nodep;
    AstVar* nestedIndexp = ಶೂನ್ಯಸೂಚಕ;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subfromp = fromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
    // Major dimension first
    ಗಾಗಿ (Ast_ಗಂಟು *argsp = headerp->elementsp(), *ಮುಂದಿನ_argsp; argsp; argsp = ಮುಂದಿನ_argsp) {
        ಮುಂದಿನ_argsp = argsp->nextp();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಖಾಲಿಗಳು = VN_IS(argsp, ಖಾಲಿಗಳು_2);
        AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || ಖಾಲಿಗಳು, argsp, "Missing foreach loop variable");
        ಆದರೆ (varp) varp->unlink_Fr_ಹಿಂಭಾಗ()->ಬಳಸಿದ_Loop_Idx(ನಿಜ);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = argsp->fileline();
        ಆದರೆ (varp) {
            Ast_ಗಂಟು* loopp = ಶೂನ್ಯಸೂಚಕ;
            VNRelinker handle;
            lastp->unlink_Fr_ಹಿಂಭಾಗ(&handle);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, ಗಂಟು_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, BasicDType)) {
                ಆದರೆ (adtypep->isString()) {
                    AstConst* ಸ್ಥಿರಾಂಕ leftp = ಹೊಸ AstConst{fl, 0};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rightp = ಹೊಸ AstLenN{fl, fromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, ಸರತಿ_DType)) {
                AstConst* ಸ್ಥಿರಾಂಕ leftp = ಹೊಸ AstConst{fl, 0};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rightp = ಹೊಸ AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), ಗಂಟು_Array_DType)
                        ? ಹೊಸ AstArraySel{fl, subfromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                          ಹೊಸ AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                    VCMethod::DYN_ಗಾತ್ರ};
                AstVarRef* varRefp = ಹೊಸ AstVarRef{fl, varp, VAccess::READ};
                rightp->dtype_ಗಣ_Int();
                rightp->protect(ಸುಳ್ಳು);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = ಹೊಸ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = ಹೊಸ AstVarRef{fl, varp, VAccess::READ};
                subfromp = ಹೊಸ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            ಆದರೆ (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_ವೃಕ್ಷ(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodyp = nodep->bodyp();
    ಆದರೆ (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        ಆದರೆ (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_ವೃಕ್ಷ(), bodyPointp);
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    ಆದರೆ (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
    } ಇಲ್ಲದಿದ್ದರೆ {
        bodyPointp->unlink_Fr_ಹಿಂಭಾಗ();
    }
    VL_DO_DANGLING(bodyPointp->delete_ವೃಕ್ಷ(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    ಹಿಂತಿರುಗಿ newp;
}
