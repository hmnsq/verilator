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
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ತಾರ್ಕಿಕ m_anyFuncInBegin = ಅಸತ್ಯ;

ಸಾರ್ವಜನಿಕ:
    BeginState() = ಸ್ವಭಾವ;
    ~BeginState() = ಸ್ವಭಾವ;
    ಶೂನ್ಯ userMarkChanged(Ast_ಗಂಟು* nodep) {
        nodep->user1(ಸತ್ಯ);
        m_anyFuncInBegin = ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ anyFuncInBegin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_anyFuncInBegin; }
};

//######################################################################

ವರ್ಗ BeginVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ಧ್ರುವ m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ಗಂಟು_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_ಗಂಟು* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    ಪೂರ್ಣಾಙ್ಕ m_ifDepth = 0;  // Current if depth
    ತಾರ್ಕಿಕ m_keepBegins = ಅಸತ್ಯ;  // True if begins should not be inlined

    // METHODS

    string dot(ಧ್ರುವ string& a, ಧ್ರುವ string& b) { ಪ್ರತ್ಯಾಗಮ VString::dot(a, "__DOT__", b); }

    ಶೂನ್ಯ dotNames(ಧ್ರುವ std::string& name, File_ಸಾಲು* ಧ್ರುವ flp, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        ಯದಿ (name == "") ಪ್ರತ್ಯಾಗಮ;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        ಯಾವತ್ ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            ಧ್ರುವ std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            ಯದಿ (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            ಯದಿ (!m_ftaskp) {
                AstCellInline* ಧ್ರುವ inlinep = ನೂತನ AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_ಗಂಟು::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    ಶೂನ್ಯ lift_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        nodep->unlinkFrBack();
        ಯದಿ (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            ಯದಿ (!m_liftedp) {
                m_liftedp = nodep;
            } ಅನ್ಯಥಾ {
                m_liftedp->addNext(nodep);
            }
        } ಅನ್ಯಥಾ {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        ಯದಿ (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->userMarkChanged(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = nullptr;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        ಯದಿ (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->userMarkChanged(nodep);
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
        m_liftedp = nullptr;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* ಧ್ರುವ initp) {
            initp->unlinkFrBack();
            m_ftaskp->addHereThisAsNext(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterateAndNextNull(nodep->itemsp());
        // Repalce node with body then delete
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ಸತ್ಯ;
            iterateAndNextNull(nodep->forksp());
        }
        Ast_ಗಂಟು* addsp = nullptr;
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ಗಂಟು::addNext(addsp, declsp);
        }
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ಗಂಟು::addNext(addsp, stmtsp);
        }
        ಯದಿ (addsp) nodep->addHereThisAsNext(addsp);
        nodep->name("");
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ಅಸತ್ಯ;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        ಯದಿ (m_keepBegins) {
            nodep->name("");
            ಪ್ರತ್ಯಾಗಮ;
        }
        Ast_ಗಂಟು* addsp = nullptr;
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ಗಂಟು::addNext(addsp, declsp);
        }
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ಗಂಟು::addNext(addsp, stmtsp);
        }
        ಯದಿ (addsp) {
            nodep->replaceWith(addsp);
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Block* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dotNames(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        ಯದಿ (m_keepBegins) {
            nodep->name("");
            ಪ್ರತ್ಯಾಗಮ;
        }
        Ast_ಗಂಟು* addsp = nullptr;
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ಗಂಟು::addNext(addsp, declsp);
        }
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ಗಂಟು::addNext(addsp, stmtsp);
        }
        ಯದಿ (addsp) {
            nodep->replaceWith(addsp);
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) { ಪ್ರತ್ಯಾಗಮ; }
        // If static variable, move it outside a function.
        ಯದಿ (nodep->lifetime().isStatic() && m_ftaskp) {
            ಧ್ರುವ std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            ಯದಿ (nodep->isIO()) {
                ಯದಿ (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ಧ್ರುವ portp = nodep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                nodep->replaceWith(portp);

                ಯದಿ (nodep->is_ಒಳಹರಿವು() || nodep->isInout()) {
                    AstAssign* ಧ್ರುವ initAssignp = ನೂತನ AstAssign{
                        nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        ನೂತನ AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                ಯದಿ (nodep->isWritable()) {
                    AstAssign* ಧ್ರುವ endAssignp = ನೂತನ AstAssign{
                        nodep->fileline(), ನೂತನ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        ನೂತನ AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } ಅನ್ಯಥಾ {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(ಅಸತ್ಯ);
        } ಅನ್ಯಥಾ ಯದಿ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_ಗಂಟು(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Don't double-add text's
        Ast_ಗಂಟು* wasUnderp = m_ftaskp;
        ಯದಿ (!m_ftaskp) wasUnderp = m_procedurep;
        ಯದಿ (wasUnderp) {
            ಯದಿ (nodep->stmtsp()) {
                Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    ಶೂನ್ಯ visit(AstTypedef* nodep) override {
        ಯದಿ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_ಗಂಟು(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        ಯದಿ (m_namedScope != "") {
            m_statep->userMarkChanged(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlinkFrBack();
            m_modp->addStmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        ಯದಿ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        ಯದಿ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ಧ್ರುವ std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        ಯದಿ (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    ಶೂನ್ಯ visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ಅಸತ್ಯ;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        ಯದಿ (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } ಅನ್ಯಥಾ ಯದಿ (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } ಅನ್ಯಥಾ ಯದಿ (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, ಸತ್ಯ);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ಅಸತ್ಯ;
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################

ವರ್ಗ BeginRelinkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // Replace tasks with new pointer
ಗುಪ್ತ:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        ಯದಿ (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        ಯದಿ (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Task class functions

ಶೂನ್ಯ V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        ಯದಿ (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("begin", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಸ್ಥಿರ Ast_ಗಂಟು* createForeachLoop(Ast_ಗಂಟು_Foreach* /*nodep*/, Ast_ಗಂಟು* bodysp, ತಾರ್ಕಿಕ arrayMayResize,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subfromp, AstVar* varp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* leftp,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rightp, VNType ಗಂಟು_Type) {
    File_ಸಾಲು* ಧ್ರುವ fl = varp->fileline();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* varRefp = ನೂತನ AstVarRef{fl, varp, VAccess::READ};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp;
    ತಾರ್ಕಿಕ inc = ಸತ್ಯ;
    ವಿಕಲ್ಪ (ಗಂಟು_Type) {
    ಪಕ್ಷ VNType::LteS: condp = ನೂತನ AstLteS{fl, varRefp, rightp}; ಭಙ್ಗ;
    ಪಕ್ಷ VNType::Lt: condp = ನೂತನ AstLt{fl, varRefp, rightp}; ಭಙ್ಗ;
    ಪಕ್ಷ VNType::GteS:
        condp = ನೂತನ AstGteS{fl, varRefp, rightp};
        inc = ಅಸತ್ಯ;
        ಭಙ್ಗ;
    ಸ್ವಭಾವ: varp->v3fatalSrc("Missing comparison handling"); ಭಙ್ಗ;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* incp;
    ಯದಿ (inc)
        incp = ನೂತನ AstAdd{fl, varRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ನೂತನ AstConst{fl, 1}};
    ಅನ್ಯಥಾ
        incp = ನೂತನ AstSub{fl, varRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ನೂತನ AstConst{fl, 1}};

    AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{fl};
    loopp->addStmtsp(ನೂತನ AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sizeGetp = nullptr;
    ಯದಿ (arrayMayResize) {
        sizeVarp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(ಸತ್ಯ);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = ನೂತನ AstCMethodHard{fl, subfromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    Ast_ಗಂಟು_Stmt* incStmtp = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, incp};
    ಯದಿ (arrayMayResize) {
        incStmtp = ನೂತನ AstIf{fl,
                             ನೂತನ AstLte{fl, ನೂತನ AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_ಗಂಟು* ಧ್ರುವ stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    ಪ್ರತ್ಯಾಗಮ stmtsp;
}
ಸ್ಥಿರ Ast_ಗಂಟು* createForeachLoopRanged(Ast_ಗಂಟು_Foreach* nodep, Ast_ಗಂಟು* bodysp,
                                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subfromp, AstVar* varp,
                                        ಧ್ರುವ VNumRange& declRange) {
    File_ಸಾಲು* ಧ್ರುವ fl = varp->fileline();
    V3_ಸಂಖ್ಯೆ ಎಡ{nodep, 32};
    V3_ಸಂಖ್ಯೆ ಬಲ{nodep, 32};
    ಎಡ.isSigned(ಸತ್ಯ);
    ಬಲ.isSigned(ಸತ್ಯ);
    ಎಡ.setLongS(declRange.ಎಡ());
    ಬಲ.setLongS(declRange.ಬಲ());
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ leftp = ನೂತನ AstConst{fl, ಎಡ};
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rightp = ನೂತನ AstConst{fl, ಬಲ};
    ಪ್ರತ್ಯಾಗಮ createForeachLoop(nodep, bodysp, ಅಸತ್ಯ /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.ಎಡ() <= declRange.ಬಲ() ? VNType::LteS : VNType::GteS);
}
ಸ್ಥಿರ Ast_ಗಂಟು* createForeachAssoc(File_ಸಾಲು* fl, AstVar* varp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subfromp,
                                   Ast_ಗಂಟು_DType* fromDtp, Ast_ಗಂಟು* bodyPointp) {
    Ast_ಗಂಟು* loopp = varp;
    AstVar* ಧ್ರುವ next_varp  // Iterator containing next element (to handle mid-array delete)
        = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(ಸತ್ಯ);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* ಧ್ರುವ more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(ಸತ್ಯ);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ firstp
        = ನೂತನ AstCMethodHard{fl, subfromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), VCMethod::ASSOC_FIRST,
                             ನೂತನ AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nextp
        = ನೂತನ AstCMethodHard{fl, subfromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), VCMethod::ASSOC_NEXT,
                             ನೂತನ AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, more_varp, VAccess::WRITE},
                                 ನೂತನ AstNeq{fl, ನೂತನ AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ಧ್ರುವ lp = ನೂತನ AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(ನೂತನ AstLoopTest{fl, lp, ನೂತನ AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                                ನೂತನ AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, more_varp, VAccess::WRITE},
                                ನೂತನ AstNeq{fl, ನೂತನ AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    ಪ್ರತ್ಯಾಗಮ loopp;
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
    ಯದಿ (!nodesp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ nodesp->existsAndNext([&](ಧ್ರುವ Ast_ಗಂಟು* nodep) -> ತಾರ್ಕಿಕ {
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* varrefp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            ಯದಿ (varrefp->access().isWriteOrRW()) {
                ಯದಿ (!fromVarp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                ಯದಿ (varrefp->varp() == fromVarp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        // No idea what a function might be doing
        ಯದಿ (VN_IS(nodep, MethodCall) || VN_IS(nodep, ಗಂಟು_FTask_Ref)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    });
}

Ast_ಗಂಟು* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ಧ್ರುವ AstForeachHeader* ಧ್ರುವ headerp = nodep->headerp();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ಧ್ರುವ fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_ಗಂಟು_DType* fromDtp = fromp->dtypep()->skipRefp();
    ಧ್ರುವ ತಾರ್ಕಿಕ arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_ಗಂಟು* bodyPointp = ನೂತನ AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, ಅಸತ್ಯ};
    Ast_ಗಂಟು* newp = nullptr;
    Ast_ಗಂಟು* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subfromp = fromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
    // Major dimension first
    ಪ್ರತಿ (Ast_ಗಂಟು *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        ಧ್ರುವ ತಾರ್ಕಿಕ empty = VN_IS(argsp, Empty);
        AstVar* ಧ್ರುವ varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        ಯದಿ (varp) varp->unlinkFrBack()->usedLoopIdx(ಸತ್ಯ);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_ಸಾಲು* ಧ್ರುವ fl = argsp->fileline();
        ಯದಿ (varp) {
            Ast_ಗಂಟು* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, ಗಂಟು_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, BasicDType)) {
                ಯದಿ (adtypep->isString()) {
                    AstConst* ಧ್ರುವ leftp = ನೂತನ AstConst{fl, 0};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rightp = ನೂತನ AstLenN{fl, fromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } ಅನ್ಯಥಾ {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* ಧ್ರುವ leftp = ನೂತನ AstConst{fl, 0};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rightp = ನೂತನ AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), ಗಂಟು_Array_DType)
                        ? ನೂತನ AstArraySel{fl, subfromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                          ನೂತನ AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = ನೂತನ AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(ಅಸತ್ಯ);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = ನೂತನ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = ನೂತನ AstVarRef{fl, varp, VAccess::READ};
                subfromp = ನೂತನ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            ಯದಿ (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_ವೃಕ್ಷ(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_ಗಂಟು* ಧ್ರುವ bodyp = nodep->bodyp();
    ಯದಿ (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        ಯದಿ (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_ವೃಕ್ಷ(), bodyPointp);
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಯದಿ (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } ಅನ್ಯಥಾ {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_ವೃಕ್ಷ(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    ಪ್ರತ್ಯಾಗಮ newp;
}
