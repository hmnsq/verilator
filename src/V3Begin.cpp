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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Begin.h"

#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3String.h"
#अन्तर्भाव "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    तार्किक m_any_Func_अन्तर्_Begin = असत्य;

सार्वजनिक:
    BeginState() = स्वभाव;
    ~BeginState() = स्वभाव;
    शून्य user_Mark_परिवर्तितम्(Ast_ग्रन्थिः* nodep) {
        nodep->user1(सत्य);
        m_any_Func_अन्तर्_Begin = सत्य;
    }
    तार्किक any_Func_अन्तर्_Begin() ध्रुव { प्रत्यागम m_any_Func_अन्तर्_Begin; }
};

//######################################################################

वर्ग BeginVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ध्रुव m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    AstCellInline* m_inlinesp = शून्यसूचकः;  // AstCellInlines to add to m_modp
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Current function/task
    Ast_ग्रन्थिः_Procedure* m_procedurep = शून्यसूचकः;  // Current procedure
    Ast_ग्रन्थिः* m_liftedp = शून्यसूचकः;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    पूर्णाङ्क m_if_गभीरता = 0;  // Current if depth
    तार्किक m_keepBegins = असत्य;  // True if begins should not be inlined

    // METHODS

    string dot(ध्रुव string& a, ध्रुव string& b) { प्रत्यागम VString::dot(a, "__DOT__", b); }

    शून्य dot_नामानि(ध्रुव std::string& name, File_पङ्क्तिः* ध्रुव flp, ध्रुव वर्ण* ध्रुव खण्डम्_Name) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        यदि (name == "") प्रत्यागम;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::आकार_type pos;
        यावत् ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            ध्रुव std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::वाक्यदैर्घ्यम्("__DOT__"));
            यदि (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            यदि (!m_ftaskp) {
                AstCellInline* ध्रुव inlinep = नूतन AstCellInline{flp, m_unnamedScope, खण्डम्_Name};
                m_inlinesp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(m_inlinesp, inlinep);
            }
        }
    }

    शून्य lift_ग्रन्थिः(Ast_ग्रन्थिः* nodep) {
        nodep->unlink_Fr_पृष्ठभागः();
        यदि (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            यदि (!m_liftedp) {
                m_liftedp = nodep;
            } अन्यथा {
                m_liftedp->सङ्कलय_अग्र(nodep);
            }
        } अन्यथा {
            // Move to module
            m_modp->सङ्कलय_Stmtsp(nodep);
        }
    }

    // VISITORS
    शून्य दर्शय(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convert_गन्तव्यम्_While(nodep), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        यदि (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_परिवर्तितम्(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = शून्यसूचकः;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        यदि (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_परिवर्तितम्(nodep);
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
        m_liftedp = शून्यसूचकः;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* ध्रुव initp) {
            initp->unlink_Fr_पृष्ठभागः();
            m_ftaskp->सङ्कलय_अत्र_This_As_अग्र(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dot_नामानि(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_अग्र_Null(nodep->itemsp());
        // Repalce node with body then delete
        यदि (Ast_ग्रन्थिः* ध्रुव itemsp = nodep->itemsp()) {
            nodep->सङ्कलय_अग्र_अत्र(itemsp->unlink_Fr_पृष्ठभागः_With_अग्र());
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstFork* nodep) override {
        dot_नामानि(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_अग्र_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = सत्य;
            iterate_And_अग्र_Null(nodep->forksp());
        }
        Ast_ग्रन्थिः* addsp = शून्यसूचकः;
        यदि (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            declsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            addsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(addsp, declsp);
        }
        यदि (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            addsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(addsp, stmtsp);
        }
        यदि (addsp) nodep->सङ्कलय_अत्र_This_As_अग्र(addsp);
        nodep->name("");
    }
    शून्य दर्शय(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = असत्य;
            dot_नामानि(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        यदि (m_keepBegins) {
            nodep->name("");
            प्रत्यागम;
        }
        Ast_ग्रन्थिः* addsp = शून्यसूचकः;
        यदि (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            declsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            addsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(addsp, declsp);
        }
        यदि (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            addsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(addsp, stmtsp);
        }
        यदि (addsp) {
            nodep->replaceWith(addsp);
        } अन्यथा {
            nodep->unlink_Fr_पृष्ठभागः();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_खण्डम्* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dot_नामानि(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        यदि (m_keepBegins) {
            nodep->name("");
            प्रत्यागम;
        }
        Ast_ग्रन्थिः* addsp = शून्यसूचकः;
        यदि (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            declsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            addsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(addsp, declsp);
        }
        यदि (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            addsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(addsp, stmtsp);
        }
        यदि (addsp) {
            nodep->replaceWith(addsp);
        } अन्यथा {
            nodep->unlink_Fr_पृष्ठभागः();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstVar* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) { प्रत्यागम; }
        // If static variable, move it outside a function.
        यदि (nodep->lifetime().isStatic() && m_ftaskp) {
            ध्रुव std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            यदि (nodep->isIO()) {
                यदि (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ध्रुव portp = nodep->clone_वृक्षः_विशुद्धम्(असत्य);
                nodep->replaceWith(portp);

                यदि (nodep->is_निवेशः() || nodep->isInout()) {
                    Ast_नियोजय* ध्रुव initAssignp = नूतन Ast_नियोजय{
                        nodep->fileline(), नूतन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        नूतन AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->सङ्कलय_अग्र_अत्र(initAssignp);
                }

                यदि (nodep->isWritable()) {
                    Ast_नियोजय* ध्रुव अन्तः_Assignp = नूतन Ast_नियोजय{
                        nodep->fileline(), नूतन AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        नूतन AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->सङ्कलय_Stmtsp(अन्तः_Assignp);
                }
            } अन्यथा {
                nodep->unlink_Fr_पृष्ठभागः();
            }
            nodep->name(newName);
            m_ftaskp->सङ्कलय_अत्र_This_As_अग्र(nodep);
            nodep->funcLocal(असत्य);
        } अन्यथा यदि (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_परिवर्तितम्(nodep);
            // Move it under enclosing tree
            lift_ग्रन्थिः(nodep);
        }
    }
    शून्य दर्शय(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Don't double-add text's
        Ast_ग्रन्थिः* wasUnderp = m_ftaskp;
        यदि (!m_ftaskp) wasUnderp = m_procedurep;
        यदि (wasUnderp) {
            यदि (nodep->stmtsp()) {
                Ast_ग्रन्थिः* ध्रुव newp = नूतन AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र()};
                wasUnderp->सङ्कलय_अत्र_This_As_अग्र(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        } अन्यथा {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    शून्य दर्शय(AstTypedef* nodep) override {
        यदि (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_परिवर्तितम्(nodep);
            // Move it under enclosing tree
            lift_ग्रन्थिः(nodep);
        }
    }
    शून्य दर्शय(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        यदि (m_namedScope != "") {
            m_statep->user_Mark_परिवर्तितम्(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlink_Fr_पृष्ठभागः();
            m_modp->सङ्कलय_Stmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        यदि (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        यदि (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ध्रुव std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        यदि (!scname.रिक्तानि()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    शून्य दर्शय(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = असत्य;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_if_गभीरता);
        यदि (m_if_गभीरता == -1 || v3Global.opt.if_गभीरता() < 1) {  // Turned off
        } अन्यथा यदि (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->प्राथमिकता_Pragma()) {
            m_if_गभीरता = -1;
        } अन्यथा यदि (++m_if_गभीरता > v3Global.opt.if_गभीरता()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3_Error_संहिता::IFDEPTH, सत्य);  // Warn only once
            m_if_गभीरता = -1;
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = असत्य;
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = स्वभाव;
};

//######################################################################

वर्ग BeginRelinkVisitor final : सार्वजनिक VNVisitorConst {
    // Replace tasks with new pointer
गुप्त:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        यदि (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        यदि (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = स्वभाव;
};

//######################################################################
// Task class functions

शून्य V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        यदि (state.any_Func_अन्तर्_Begin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("begin", 0, dump_वृक्षः_Either_Level() >= 3);
}

स्थिर Ast_ग्रन्थिः* createForeachLoop(Ast_ग्रन्थिः_Foreach* /*nodep*/, Ast_ग्रन्थिः* bodysp, तार्किक arrayMayResize,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* subfromp, AstVar* varp, Ast_ग्रन्थिः_व्यञ्जकम्* leftp,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* rightp, VNType ग्रन्थिः_Type) {
    File_पङ्क्तिः* ध्रुव fl = varp->fileline();
    Ast_ग्रन्थिः_व्यञ्जकम्* varRefp = नूतन AstVarRef{fl, varp, VAccess::READ};
    Ast_ग्रन्थिः_व्यञ्जकम्* condp;
    तार्किक inc = सत्य;
    विकल्प (ग्रन्थिः_Type) {
    पक्ष VNType::LteS: condp = नूतन AstLteS{fl, varRefp, rightp}; भङ्ग;
    पक्ष VNType::Lt: condp = नूतन AstLt{fl, varRefp, rightp}; भङ्ग;
    पक्ष VNType::GteS:
        condp = नूतन AstGteS{fl, varRefp, rightp};
        inc = असत्य;
        भङ्ग;
    स्वभाव: varp->v3fatalSrc("Missing comparison handling"); भङ्ग;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* incp;
    यदि (inc)
        incp = नूतन Ast_सङ्कलय{fl, varRefp->clone_वृक्षः(असत्य), नूतन AstConst{fl, 1}};
    अन्यथा
        incp = नूतन AstSub{fl, varRefp->clone_वृक्षः(असत्य), नूतन AstConst{fl, 1}};

    AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
    loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{fl, loopp, condp});
    AstVar* आकार_Varp = शून्यसूचकः;
    Ast_ग्रन्थिः_व्यञ्जकम्* आकार_Getp = शून्यसूचकः;
    यदि (arrayMayResize) {
        आकार_Varp = नूतन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        आकार_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        आकार_Varp->प्रयुक्तम्_Loop_Idx(सत्य);  // Not technically an index, but used only inside loop
        varp->सङ्कलय_अग्र(आकार_Varp);
        आकार_Getp
            = नूतन AstCMethodHard{fl, subfromp->clone_वृक्षः_विशुद्धम्(असत्य), VCMethod::DYN_आकार, शून्यसूचकः};
        आकार_Getp->dtype_समुच्चयः_UInt32();
        loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, आकार_Varp, VAccess::WRITE}, आकार_Getp});
    }
    loopp->सङ्कलय_Stmtsp(bodysp);
    Ast_ग्रन्थिः_Stmt* incStmtp = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, incp};
    यदि (arrayMayResize) {
        incStmtp = नूतन AstIf{fl,
                             नूतन AstLte{fl, नूतन AstVarRef{fl, आकार_Varp, VAccess::READ},
                                        आकार_Getp->clone_वृक्षः_विशुद्धम्(असत्य)},
                             incStmtp};
    }
    loopp->सङ्कलय_Stmtsp(incStmtp);
    Ast_ग्रन्थिः* ध्रुव stmtsp = varp;  // New statements for outer loop
    stmtsp->सङ्कलय_अग्र(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->सङ्कलय_अग्र(loopp);
    प्रत्यागम stmtsp;
}
स्थिर Ast_ग्रन्थिः* createForeachLoopRanged(Ast_ग्रन्थिः_Foreach* nodep, Ast_ग्रन्थिः* bodysp,
                                        Ast_ग्रन्थिः_व्यञ्जकम्* subfromp, AstVar* varp,
                                        ध्रुव VNumRange& declRange) {
    File_पङ्क्तिः* ध्रुव fl = varp->fileline();
    V3_सङ्ख्या वामतः{nodep, 32};
    V3_सङ्ख्या दक्षिणतः{nodep, 32};
    वामतः.isSigned(सत्य);
    दक्षिणतः.isSigned(सत्य);
    वामतः.समुच्चयः_Long_S(declRange.वामतः());
    दक्षिणतः.समुच्चयः_Long_S(declRange.दक्षिणतः());
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव leftp = नूतन AstConst{fl, वामतः};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rightp = नूतन AstConst{fl, दक्षिणतः};
    प्रत्यागम createForeachLoop(nodep, bodysp, असत्य /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.वामतः() <= declRange.दक्षिणतः() ? VNType::LteS : VNType::GteS);
}
स्थिर Ast_ग्रन्थिः* createForeachAssoc(File_पङ्क्तिः* fl, AstVar* varp, Ast_ग्रन्थिः_व्यञ्जकम्* subfromp,
                                   Ast_ग्रन्थिः_DType* fromDtp, Ast_ग्रन्थिः* bodyPointp) {
    Ast_ग्रन्थिः* loopp = varp;
    AstVar* ध्रुव अग्र_varp  // Iterator containing next element (to handle mid-array delete)
        = नूतन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    अग्र_varp->प्रयुक्तम्_Loop_Idx(सत्य);
    अग्र_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->सङ्कलय_अग्र(अग्र_varp);

    AstVar* ध्रुव अधिकम्_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = नूतन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    अधिकम्_varp->प्रयुक्तम्_Loop_Idx(सत्य);  // Not technically an index, but used only inside loop
    अधिकम्_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->सङ्कलय_अग्र(अधिकम्_varp);

    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firstp
        = नूतन AstCMethodHard{fl, subfromp->clone_वृक्षः_विशुद्धम्(असत्य), VCMethod::ASSOC_FIRST,
                             नूतन AstVarRef{fl, अग्र_varp, VAccess::READWRITE}};
    firstp->dtype_समुच्चयः_Int();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nextp
        = नूतन AstCMethodHard{fl, subfromp->clone_वृक्षः_विशुद्धम्(असत्य), VCMethod::ASSOC_अग्र,
                             नूतन AstVarRef{fl, अग्र_varp, VAccess::READWRITE}};
    nextp->dtype_समुच्चयः_Int();

    // _Vmore = array.first(__Vnext)
    loopp->सङ्कलय_अग्र(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, अधिकम्_varp, VAccess::WRITE},
                                 नूतन AstNeq{fl, नूतन AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ध्रुव lp = नूतन AstLoop{fl};
    loopp->सङ्कलय_अग्र(lp);
    lp->सङ्कलय_Stmtsp(नूतन AstLoopTest{fl, lp, नूतन AstVarRef{fl, अधिकम्_varp, VAccess::READ}});

    // index = __Vnext
    lp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                नूतन AstVarRef{fl, अग्र_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, अधिकम्_varp, VAccess::WRITE},
                                नूतन AstNeq{fl, नूतन AstConst{fl, 0}, nextp}});
    lp->सङ्कलय_Stmtsp(bodyPointp);
    प्रत्यागम loopp;
}

स्थिर तार्किक arrayMayResizeCheck(Ast_ग्रन्थिः* nodesp, AstVar* fromVarp) {
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
    यदि (!nodesp) प्रत्यागम असत्य;
    प्रत्यागम nodesp->exists_And_अग्र([&](ध्रुव Ast_ग्रन्थिः* nodep) -> तार्किक {
        यदि (ध्रुव Ast_ग्रन्थिः_Var_Ref* varrefp = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            यदि (varrefp->access().isWriteOrRW()) {
                यदि (!fromVarp) प्रत्यागम सत्य;
                यदि (varrefp->varp() == fromVarp) प्रत्यागम सत्य;
            }
        }
        // No idea what a function might be doing
        यदि (VN_IS(nodep, विधानम्_Call) || VN_IS(nodep, ग्रन्थिः_FTask_Ref)) प्रत्यागम सत्य;
        प्रत्यागम असत्य;
    });
}

Ast_ग्रन्थिः* V3Begin::convert_गन्तव्यम्_While(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ध्रुव Ast_Foreach_शीर्षकम्* ध्रुव headerp = nodep->headerp();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ध्रुव fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : शून्यसूचकः;
    Ast_ग्रन्थिः_DType* fromDtp = fromp->dtypep()->skipRefp();
    ध्रुव तार्किक arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_ग्रन्थिः* bodyPointp = नूतन AstBegin{nodep->fileline(), "[EditWrapper]", शून्यसूचकः, असत्य};
    Ast_ग्रन्थिः* newp = शून्यसूचकः;
    Ast_ग्रन्थिः* lastp = nodep;
    AstVar* nestedIndexp = शून्यसूचकः;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_ग्रन्थिः_व्यञ्जकम्* subfromp = fromp->clone_वृक्षः_विशुद्धम्(असत्य);
    // Major dimension first
    प्रति (Ast_ग्रन्थिः *argsp = headerp->elementsp(), *अग्र_argsp; argsp; argsp = अग्र_argsp) {
        अग्र_argsp = argsp->nextp();
        ध्रुव तार्किक रिक्तानि = VN_IS(argsp, रिक्तानि_2);
        AstVar* ध्रुव varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || रिक्तानि, argsp, "Missing foreach loop variable");
        यदि (varp) varp->unlink_Fr_पृष्ठभागः()->प्रयुक्तम्_Loop_Idx(सत्य);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_पङ्क्तिः* ध्रुव fl = argsp->fileline();
        यदि (varp) {
            Ast_ग्रन्थिः* loopp = शून्यसूचकः;
            VNRelinker handle;
            lastp->unlink_Fr_पृष्ठभागः(&handle);
            यदि (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव adtypep = VN_CAST(fromDtp, ग्रन्थिः_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } अन्यथा यदि (ध्रुव AstBasicDType* ध्रुव adtypep = VN_CAST(fromDtp, BasicDType)) {
                यदि (adtypep->isString()) {
                    AstConst* ध्रुव leftp = नूतन AstConst{fl, 0};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rightp = नूतन AstLenN{fl, fromp->clone_वृक्षः_विशुद्धम्(असत्य)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } अन्यथा {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } अन्यथा यदि (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* ध्रुव leftp = नूतन AstConst{fl, 0};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rightp = नूतन AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), ग्रन्थिः_Array_DType)
                        ? नूतन AstArraySel{fl, subfromp->clone_वृक्षः_विशुद्धम्(असत्य),
                                          नूतन AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_वृक्षः_विशुद्धम्(असत्य),
                    VCMethod::DYN_आकार};
                AstVarRef* varRefp = नूतन AstVarRef{fl, varp, VAccess::READ};
                rightp->dtype_समुच्चयः_Int();
                rightp->protect(असत्य);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = नूतन AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } अन्यथा यदि (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = नूतन AstVarRef{fl, varp, VAccess::READ};
                subfromp = नूतन AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            यदि (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_वृक्षः(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_ग्रन्थिः* ध्रुव bodyp = nodep->bodyp();
    यदि (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        यदि (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_वृक्षः(), bodyPointp);
        प्रत्यागम शून्यसूचकः;
    }
    यदि (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_पृष्ठभागः_With_अग्र());
    } अन्यथा {
        bodyPointp->unlink_Fr_पृष्ठभागः();
    }
    VL_DO_DANGLING(bodyPointp->delete_वृक्षः(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    प्रत्यागम newp;
}
