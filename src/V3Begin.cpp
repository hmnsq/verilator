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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Begin.h"

#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3String.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    സ്ഥിരം VNUser1InUse m_inuser1;
    തർക്കം m_anyFuncInBegin = അസത്യം;

പൊതു:
    BeginState() = സ്ഥിരസ്ഥിതി;
    ~BeginState() = സ്ഥിരസ്ഥിതി;
    ശൂന്യം userMarkChanged(Ast_കണ്ണി* nodep) {
        nodep->user1(സത്യം);
        m_anyFuncInBegin = സത്യം;
    }
    തർക്കം anyFuncInBegin() സ്ഥിരം { തിരികെ m_anyFuncInBegin; }
};

//######################################################################

വർഗ്ഗം BeginVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* സ്ഥിരം m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_കണ്ണി_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_കണ്ണി* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    പൂർണ്ണസംഖ്യ m_ifDepth = 0;  // Current if depth
    തർക്കം m_keepBegins = അസത്യം;  // True if begins should not be inlined

    // METHODS

    string dot(സ്ഥിരം string& a, സ്ഥിരം string& b) { തിരികെ VString::dot(a, "__DOT__", b); }

    ശൂന്യം dotNames(സ്ഥിരം std::string& name, File_വരി* സ്ഥിരം flp, സ്ഥിരം അക്ഷരം* സ്ഥിരം blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        എങ്കിൽ (name == "") തിരികെ;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        ആകുമ്പോൾ ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            സ്ഥിരം std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            എങ്കിൽ (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            എങ്കിൽ (!m_ftaskp) {
                AstCellInline* സ്ഥിരം inlinep = പുതിയ AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_കണ്ണി::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    ശൂന്യം lift_കണ്ണി(Ast_കണ്ണി* nodep) {
        nodep->unlinkFrBack();
        എങ്കിൽ (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            എങ്കിൽ (!m_liftedp) {
                m_liftedp = nodep;
            } അല്ലെങ്കിൽ {
                m_liftedp->addNext(nodep);
            }
        } അല്ലെങ്കിൽ {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    ശൂന്യം visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        എങ്കിൽ (m_unnamedScope != "") {
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
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        എങ്കിൽ (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* സ്ഥിരം initp) {
            initp->unlinkFrBack();
            m_ftaskp->addHereThisAsNext(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterateAndNextNull(nodep->itemsp());
        // Repalce node with body then delete
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = സത്യം;
            iterateAndNextNull(nodep->forksp());
        }
        Ast_കണ്ണി* addsp = nullptr;
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_കണ്ണി::addNext(addsp, declsp);
        }
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_കണ്ണി::addNext(addsp, stmtsp);
        }
        എങ്കിൽ (addsp) nodep->addHereThisAsNext(addsp);
        nodep->name("");
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = അസത്യം;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        എങ്കിൽ (m_keepBegins) {
            nodep->name("");
            തിരികെ;
        }
        Ast_കണ്ണി* addsp = nullptr;
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_കണ്ണി::addNext(addsp, declsp);
        }
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_കണ്ണി::addNext(addsp, stmtsp);
        }
        എങ്കിൽ (addsp) {
            nodep->replaceWith(addsp);
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Block* nodep) override {
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
        എങ്കിൽ (m_keepBegins) {
            nodep->name("");
            തിരികെ;
        }
        Ast_കണ്ണി* addsp = nullptr;
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_കണ്ണി::addNext(addsp, declsp);
        }
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_കണ്ണി::addNext(addsp, stmtsp);
        }
        എങ്കിൽ (addsp) {
            nodep->replaceWith(addsp);
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) { തിരികെ; }
        // If static variable, move it outside a function.
        എങ്കിൽ (nodep->lifetime().isStatic() && m_ftaskp) {
            സ്ഥിരം std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            എങ്കിൽ (nodep->isIO()) {
                എങ്കിൽ (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* സ്ഥിരം portp = nodep->clone_വൃക്ഷം_Pure(അസത്യം);
                nodep->replaceWith(portp);

                എങ്കിൽ (nodep->is_നിവേശം() || nodep->isInout()) {
                    AstAssign* സ്ഥിരം initAssignp = പുതിയ AstAssign{
                        nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        പുതിയ AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                എങ്കിൽ (nodep->isWritable()) {
                    AstAssign* സ്ഥിരം endAssignp = പുതിയ AstAssign{
                        nodep->fileline(), പുതിയ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        പുതിയ AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } അല്ലെങ്കിൽ {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(അസത്യം);
        } അല്ലെങ്കിൽ എങ്കിൽ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_കണ്ണി(nodep);
        }
    }
    ശൂന്യം visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Don't double-add text's
        Ast_കണ്ണി* wasUnderp = m_ftaskp;
        എങ്കിൽ (!m_ftaskp) wasUnderp = m_procedurep;
        എങ്കിൽ (wasUnderp) {
            എങ്കിൽ (nodep->stmtsp()) {
                Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    ശൂന്യം visit(AstTypedef* nodep) override {
        എങ്കിൽ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_കണ്ണി(nodep);
        }
    }
    ശൂന്യം visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        എങ്കിൽ (m_namedScope != "") {
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
    ശൂന്യം visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        എങ്കിൽ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        എങ്കിൽ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        സ്ഥിരം std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        എങ്കിൽ (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    ശൂന്യം visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = അസത്യം;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        എങ്കിൽ (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } അല്ലെങ്കിൽ എങ്കിൽ (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, സത്യം);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = അസത്യം;
        iterateChildren(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################

വർഗ്ഗം BeginRelinkVisitor final : പൊതു VNVisitorConst {
    // Replace tasks with new pointer
സ്വകാര്യ:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        എങ്കിൽ (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        എങ്കിൽ (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Task class functions

ശൂന്യം V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        എങ്കിൽ (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("begin", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

സ്ഥിതം Ast_കണ്ണി* createForeachLoop(Ast_കണ്ണി_Foreach* /*nodep*/, Ast_കണ്ണി* bodysp, തർക്കം arrayMayResize,
                                  Ast_കണ്ണി_വ്യഞ്ജകം* subfromp, AstVar* varp, Ast_കണ്ണി_വ്യഞ്ജകം* leftp,
                                  Ast_കണ്ണി_വ്യഞ്ജകം* rightp, VNType കണ്ണി_Type) {
    File_വരി* സ്ഥിരം fl = varp->fileline();
    Ast_കണ്ണി_വ്യഞ്ജകം* varRefp = പുതിയ AstVarRef{fl, varp, VAccess::READ};
    Ast_കണ്ണി_വ്യഞ്ജകം* condp;
    തർക്കം inc = സത്യം;
    തിരഞ്ഞെടുപ്പ് (കണ്ണി_Type) {
    അവസ്ഥ VNType::LteS: condp = പുതിയ AstLteS{fl, varRefp, rightp}; നിർത്തുക;
    അവസ്ഥ VNType::Lt: condp = പുതിയ AstLt{fl, varRefp, rightp}; നിർത്തുക;
    അവസ്ഥ VNType::GteS:
        condp = പുതിയ AstGteS{fl, varRefp, rightp};
        inc = അസത്യം;
        നിർത്തുക;
    സ്ഥിരസ്ഥിതി: varp->v3fatalSrc("Missing comparison handling"); നിർത്തുക;
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* incp;
    എങ്കിൽ (inc)
        incp = പുതിയ AstAdd{fl, varRefp->clone_വൃക്ഷം(അസത്യം), പുതിയ AstConst{fl, 1}};
    അല്ലെങ്കിൽ
        incp = പുതിയ AstSub{fl, varRefp->clone_വൃക്ഷം(അസത്യം), പുതിയ AstConst{fl, 1}};

    AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{fl};
    loopp->addStmtsp(പുതിയ AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    Ast_കണ്ണി_വ്യഞ്ജകം* sizeGetp = nullptr;
    എങ്കിൽ (arrayMayResize) {
        sizeVarp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(സത്യം);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = പുതിയ AstCMethodHard{fl, subfromp->clone_വൃക്ഷം_Pure(അസത്യം), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    Ast_കണ്ണി_Stmt* incStmtp = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, incp};
    എങ്കിൽ (arrayMayResize) {
        incStmtp = പുതിയ AstIf{fl,
                             പുതിയ AstLte{fl, പുതിയ AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->clone_വൃക്ഷം_Pure(അസത്യം)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_കണ്ണി* സ്ഥിരം stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    തിരികെ stmtsp;
}
സ്ഥിതം Ast_കണ്ണി* createForeachLoopRanged(Ast_കണ്ണി_Foreach* nodep, Ast_കണ്ണി* bodysp,
                                        Ast_കണ്ണി_വ്യഞ്ജകം* subfromp, AstVar* varp,
                                        സ്ഥിരം VNumRange& declRange) {
    File_വരി* സ്ഥിരം fl = varp->fileline();
    V3_സംഖ്യ ഇടത്{nodep, 32};
    V3_സംഖ്യ വലത്{nodep, 32};
    ഇടത്.isSigned(സത്യം);
    വലത്.isSigned(സത്യം);
    ഇടത്.setLongS(declRange.ഇടത്());
    വലത്.setLongS(declRange.വലത്());
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം leftp = പുതിയ AstConst{fl, ഇടത്};
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rightp = പുതിയ AstConst{fl, വലത്};
    തിരികെ createForeachLoop(nodep, bodysp, അസത്യം /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.ഇടത്() <= declRange.വലത്() ? VNType::LteS : VNType::GteS);
}
സ്ഥിതം Ast_കണ്ണി* createForeachAssoc(File_വരി* fl, AstVar* varp, Ast_കണ്ണി_വ്യഞ്ജകം* subfromp,
                                   Ast_കണ്ണി_DType* fromDtp, Ast_കണ്ണി* bodyPointp) {
    Ast_കണ്ണി* loopp = varp;
    AstVar* സ്ഥിരം next_varp  // Iterator containing next element (to handle mid-array delete)
        = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(സത്യം);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* സ്ഥിരം more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(സത്യം);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം firstp
        = പുതിയ AstCMethodHard{fl, subfromp->clone_വൃക്ഷം_Pure(അസത്യം), VCMethod::ASSOC_FIRST,
                             പുതിയ AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nextp
        = പുതിയ AstCMethodHard{fl, subfromp->clone_വൃക്ഷം_Pure(അസത്യം), VCMethod::ASSOC_NEXT,
                             പുതിയ AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, more_varp, VAccess::WRITE},
                                 പുതിയ AstNeq{fl, പുതിയ AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* സ്ഥിരം lp = പുതിയ AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(പുതിയ AstLoopTest{fl, lp, പുതിയ AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                                പുതിയ AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, more_varp, VAccess::WRITE},
                                പുതിയ AstNeq{fl, പുതിയ AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    തിരികെ loopp;
}

സ്ഥിതം തർക്കം arrayMayResizeCheck(Ast_കണ്ണി* nodesp, AstVar* fromVarp) {
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
    എങ്കിൽ (!nodesp) തിരികെ അസത്യം;
    തിരികെ nodesp->existsAndNext([&](സ്ഥിരം Ast_കണ്ണി* nodep) -> തർക്കം {
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* varrefp = VN_CAST(nodep, കണ്ണി_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            എങ്കിൽ (varrefp->access().isWriteOrRW()) {
                എങ്കിൽ (!fromVarp) തിരികെ സത്യം;
                എങ്കിൽ (varrefp->varp() == fromVarp) തിരികെ സത്യം;
            }
        }
        // No idea what a function might be doing
        എങ്കിൽ (VN_IS(nodep, MethodCall) || VN_IS(nodep, കണ്ണി_FTask_Ref)) തിരികെ സത്യം;
        തിരികെ അസത്യം;
    });
}

Ast_കണ്ണി* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    സ്ഥിരം AstForeachHeader* സ്ഥിരം headerp = nodep->headerp();
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* സ്ഥിരം fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_കണ്ണി_DType* fromDtp = fromp->dtypep()->skipRefp();
    സ്ഥിരം തർക്കം arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_കണ്ണി* bodyPointp = പുതിയ AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, അസത്യം};
    Ast_കണ്ണി* newp = nullptr;
    Ast_കണ്ണി* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_കണ്ണി_വ്യഞ്ജകം* subfromp = fromp->clone_വൃക്ഷം_Pure(അസത്യം);
    // Major dimension first
    വേണ്ടി (Ast_കണ്ണി *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        സ്ഥിരം തർക്കം empty = VN_IS(argsp, Empty);
        AstVar* സ്ഥിരം varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        എങ്കിൽ (varp) varp->unlinkFrBack()->usedLoopIdx(സത്യം);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_വരി* സ്ഥിരം fl = argsp->fileline();
        എങ്കിൽ (varp) {
            Ast_കണ്ണി* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം adtypep = VN_CAST(fromDtp, കണ്ണി_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, BasicDType)) {
                എങ്കിൽ (adtypep->isString()) {
                    AstConst* സ്ഥിരം leftp = പുതിയ AstConst{fl, 0};
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rightp = പുതിയ AstLenN{fl, fromp->clone_വൃക്ഷം_Pure(അസത്യം)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } അല്ലെങ്കിൽ {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* സ്ഥിരം leftp = പുതിയ AstConst{fl, 0};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rightp = പുതിയ AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), കണ്ണി_Array_DType)
                        ? പുതിയ AstArraySel{fl, subfromp->clone_വൃക്ഷം_Pure(അസത്യം),
                                          പുതിയ AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_വൃക്ഷം_Pure(അസത്യം),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = പുതിയ AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(അസത്യം);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = പുതിയ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = പുതിയ AstVarRef{fl, varp, VAccess::READ};
                subfromp = പുതിയ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            എങ്കിൽ (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_വൃക്ഷം(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_കണ്ണി* സ്ഥിരം bodyp = nodep->bodyp();
    എങ്കിൽ (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        എങ്കിൽ (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_വൃക്ഷം(), bodyPointp);
        തിരികെ nullptr;
    }
    എങ്കിൽ (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } അല്ലെങ്കിൽ {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_വൃക്ഷം(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    തിരികെ newp;
}
