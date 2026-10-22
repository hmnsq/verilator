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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Begin.h"

#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    ଅଚଳ VNUser1InUse m_inuser1;
    ତାର୍କିକ m_anyFuncInBegin = ମିଥ୍ୟା;

ସର୍ବସାଧାରଣ:
    BeginState() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~BeginState() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ userMarkChanged(Ast_ଗଣ୍ଠି* nodep) {
        nodep->user1(ସତ୍ୟ);
        m_anyFuncInBegin = ସତ୍ୟ;
    }
    ତାର୍କିକ anyFuncInBegin() ଅଚଳ { ଫେରନ୍ତୁ m_anyFuncInBegin; }
};

//######################################################################

ଶ୍ରେଣୀ BeginVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ଅଚଳ m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ଗଣ୍ଠି_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_ଗଣ୍ଠି* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ifDepth = 0;  // Current if depth
    ତାର୍କିକ m_keepBegins = ମିଥ୍ୟା;  // True if begins should not be inlined

    // METHODS

    string dot(ଅଚଳ string& a, ଅଚଳ string& b) { ଫେରନ୍ତୁ VString::dot(a, "__DOT__", b); }

    ଶୂନ୍ୟ dotNames(ଅଚଳ std::string& name, File_ଧାଡ଼ି* ଅଚଳ flp, ଅଚଳ ଅକ୍ଷର* ଅଚଳ blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        ଯଦି (name == "") ଫେରନ୍ତୁ;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        ଯେତେବେଳେ ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            ଅଚଳ std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            ଯଦି (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            ଯଦି (!m_ftaskp) {
                AstCellInline* ଅଚଳ inlinep = ନୂତନ AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_ଗଣ୍ଠି::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    ଶୂନ୍ୟ lift_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି* nodep) {
        nodep->unlinkFrBack();
        ଯଦି (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            ଯଦି (!m_liftedp) {
                m_liftedp = nodep;
            } ଅନ୍ୟଥା {
                m_liftedp->addNext(nodep);
            }
        } ଅନ୍ୟଥା {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        ଯଦି (m_unnamedScope != "") {
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
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        ଯଦି (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* ଅଚଳ initp) {
            initp->unlinkFrBack();
            m_ftaskp->addHereThisAsNext(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterateAndNextNull(nodep->itemsp());
        // Repalce node with body then delete
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ସତ୍ୟ;
            iterateAndNextNull(nodep->forksp());
        }
        Ast_ଗଣ୍ଠି* addsp = nullptr;
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ଗଣ୍ଠି::addNext(addsp, declsp);
        }
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ଗଣ୍ଠି::addNext(addsp, stmtsp);
        }
        ଯଦି (addsp) nodep->addHereThisAsNext(addsp);
        nodep->name("");
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ମିଥ୍ୟା;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        ଯଦି (m_keepBegins) {
            nodep->name("");
            ଫେରନ୍ତୁ;
        }
        Ast_ଗଣ୍ଠି* addsp = nullptr;
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ଗଣ୍ଠି::addNext(addsp, declsp);
        }
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ଗଣ୍ଠି::addNext(addsp, stmtsp);
        }
        ଯଦି (addsp) {
            nodep->replaceWith(addsp);
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Block* nodep) override {
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
        ଯଦି (m_keepBegins) {
            nodep->name("");
            ଫେରନ୍ତୁ;
        }
        Ast_ଗଣ୍ଠି* addsp = nullptr;
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ଗଣ୍ଠି::addNext(addsp, declsp);
        }
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ଗଣ୍ଠି::addNext(addsp, stmtsp);
        }
        ଯଦି (addsp) {
            nodep->replaceWith(addsp);
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (nodep->user1SetOnce()) { ଫେରନ୍ତୁ; }
        // If static variable, move it outside a function.
        ଯଦି (nodep->lifetime().isStatic() && m_ftaskp) {
            ଅଚଳ std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            ଯଦି (nodep->isIO()) {
                ଯଦି (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ଅଚଳ portp = nodep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                nodep->replaceWith(portp);

                ଯଦି (nodep->is_ନିବେଶ() || nodep->isInout()) {
                    AstAssign* ଅଚଳ initAssignp = ନୂତନ AstAssign{
                        nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        ନୂତନ AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                ଯଦି (nodep->isWritable()) {
                    AstAssign* ଅଚଳ endAssignp = ନୂତନ AstAssign{
                        nodep->fileline(), ନୂତନ AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        ନୂତନ AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } ଅନ୍ୟଥା {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(ମିଥ୍ୟା);
        } ଅନ୍ୟଥା ଯଦି (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_ଗଣ୍ଠି(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Don't double-add text's
        Ast_ଗଣ୍ଠି* wasUnderp = m_ftaskp;
        ଯଦି (!m_ftaskp) wasUnderp = m_procedurep;
        ଯଦି (wasUnderp) {
            ଯଦି (nodep->stmtsp()) {
                Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    ଶୂନ୍ୟ visit(AstTypedef* nodep) override {
        ଯଦି (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_ଗଣ୍ଠି(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        ଯଦି (m_namedScope != "") {
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
    ଶୂନ୍ୟ visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        ଯଦି (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        ଯଦି (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ଅଚଳ std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        ଯଦି (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    ଶୂନ୍ୟ visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ମିଥ୍ୟା;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        ଯଦି (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } ଅନ୍ୟଥା ଯଦି (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } ଅନ୍ୟଥା ଯଦି (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, ସତ୍ୟ);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ମିଥ୍ୟା;
        iterateChildren(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################

ଶ୍ରେଣୀ BeginRelinkVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // Replace tasks with new pointer
ବ୍ୟକ୍ତିଗତ:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        ଯଦି (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        ଯଦି (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Task class functions

ଶୂନ୍ୟ V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        ଯଦି (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("begin", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ସ୍ଥିର Ast_ଗଣ୍ଠି* createForeachLoop(Ast_ଗଣ୍ଠି_Foreach* /*nodep*/, Ast_ଗଣ୍ଠି* bodysp, ତାର୍କିକ arrayMayResize,
                                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subfromp, AstVar* varp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* leftp,
                                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rightp, VNType ଗଣ୍ଠି_Type) {
    File_ଧାଡ଼ି* ଅଚଳ fl = varp->fileline();
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* varRefp = ନୂତନ AstVarRef{fl, varp, VAccess::READ};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp;
    ତାର୍କିକ inc = ସତ୍ୟ;
    ବାଛ (ଗଣ୍ଠି_Type) {
    ସ୍ଥିତି VNType::LteS: condp = ନୂତନ AstLteS{fl, varRefp, rightp}; ଭାଙ୍ଗ;
    ସ୍ଥିତି VNType::Lt: condp = ନୂତନ AstLt{fl, varRefp, rightp}; ଭାଙ୍ଗ;
    ସ୍ଥିତି VNType::GteS:
        condp = ନୂତନ AstGteS{fl, varRefp, rightp};
        inc = ମିଥ୍ୟା;
        ଭାଙ୍ଗ;
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: varp->v3fatalSrc("Missing comparison handling"); ଭାଙ୍ଗ;
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* incp;
    ଯଦି (inc)
        incp = ନୂତନ AstAdd{fl, varRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ନୂତନ AstConst{fl, 1}};
    ଅନ୍ୟଥା
        incp = ନୂତନ AstSub{fl, varRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ନୂତନ AstConst{fl, 1}};

    AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{fl};
    loopp->addStmtsp(ନୂତନ AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sizeGetp = nullptr;
    ଯଦି (arrayMayResize) {
        sizeVarp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(ସତ୍ୟ);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = ନୂତନ AstCMethodHard{fl, subfromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    Ast_ଗଣ୍ଠି_Stmt* incStmtp = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, incp};
    ଯଦି (arrayMayResize) {
        incStmtp = ନୂତନ AstIf{fl,
                             ନୂତନ AstLte{fl, ନୂତନ AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    ଫେରନ୍ତୁ stmtsp;
}
ସ୍ଥିର Ast_ଗଣ୍ଠି* createForeachLoopRanged(Ast_ଗଣ୍ଠି_Foreach* nodep, Ast_ଗଣ୍ଠି* bodysp,
                                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subfromp, AstVar* varp,
                                        ଅଚଳ VNumRange& declRange) {
    File_ଧାଡ଼ି* ଅଚଳ fl = varp->fileline();
    V3_ସଂଖ୍ୟା ବାମ{nodep, 32};
    V3_ସଂଖ୍ୟା ଡାହାଣ{nodep, 32};
    ବାମ.isSigned(ସତ୍ୟ);
    ଡାହାଣ.isSigned(ସତ୍ୟ);
    ବାମ.setLongS(declRange.ବାମ());
    ଡାହାଣ.setLongS(declRange.ଡାହାଣ());
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ leftp = ନୂତନ AstConst{fl, ବାମ};
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rightp = ନୂତନ AstConst{fl, ଡାହାଣ};
    ଫେରନ୍ତୁ createForeachLoop(nodep, bodysp, ମିଥ୍ୟା /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.ବାମ() <= declRange.ଡାହାଣ() ? VNType::LteS : VNType::GteS);
}
ସ୍ଥିର Ast_ଗଣ୍ଠି* createForeachAssoc(File_ଧାଡ଼ି* fl, AstVar* varp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subfromp,
                                   Ast_ଗଣ୍ଠି_DType* fromDtp, Ast_ଗଣ୍ଠି* bodyPointp) {
    Ast_ଗଣ୍ଠି* loopp = varp;
    AstVar* ଅଚଳ next_varp  // Iterator containing next element (to handle mid-array delete)
        = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(ସତ୍ୟ);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* ଅଚଳ more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(ସତ୍ୟ);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ firstp
        = ନୂତନ AstCMethodHard{fl, subfromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), VCMethod::ASSOC_FIRST,
                             ନୂତନ AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nextp
        = ନୂତନ AstCMethodHard{fl, subfromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), VCMethod::ASSOC_NEXT,
                             ନୂତନ AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, more_varp, VAccess::WRITE},
                                 ନୂତନ AstNeq{fl, ନୂତନ AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ଅଚଳ lp = ନୂତନ AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(ନୂତନ AstLoopTest{fl, lp, ନୂତନ AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                                ନୂତନ AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, more_varp, VAccess::WRITE},
                                ନୂତନ AstNeq{fl, ନୂତନ AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    ଫେରନ୍ତୁ loopp;
}

ସ୍ଥିର ତାର୍କିକ arrayMayResizeCheck(Ast_ଗଣ୍ଠି* nodesp, AstVar* fromVarp) {
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
    ଯଦି (!nodesp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଫେରନ୍ତୁ nodesp->existsAndNext([&](ଅଚଳ Ast_ଗଣ୍ଠି* nodep) -> ତାର୍କିକ {
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* varrefp = VN_CAST(nodep, ଗଣ୍ଠି_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            ଯଦି (varrefp->access().isWriteOrRW()) {
                ଯଦି (!fromVarp) ଫେରନ୍ତୁ ସତ୍ୟ;
                ଯଦି (varrefp->varp() == fromVarp) ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        // No idea what a function might be doing
        ଯଦି (VN_IS(nodep, MethodCall) || VN_IS(nodep, ଗଣ୍ଠି_FTask_Ref)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    });
}

Ast_ଗଣ୍ଠି* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ଅଚଳ AstForeachHeader* ଅଚଳ headerp = nodep->headerp();
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ଅଚଳ fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_ଗଣ୍ଠି_DType* fromDtp = fromp->dtypep()->skipRefp();
    ଅଚଳ ତାର୍କିକ arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_ଗଣ୍ଠି* bodyPointp = ନୂତନ AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, ମିଥ୍ୟା};
    Ast_ଗଣ୍ଠି* newp = nullptr;
    Ast_ଗଣ୍ଠି* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subfromp = fromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
    // Major dimension first
    ପାଇଁ (Ast_ଗଣ୍ଠି *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        ଅଚଳ ତାର୍କିକ empty = VN_IS(argsp, Empty);
        AstVar* ଅଚଳ varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        ଯଦି (varp) varp->unlinkFrBack()->usedLoopIdx(ସତ୍ୟ);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_ଧାଡ଼ି* ଅଚଳ fl = argsp->fileline();
        ଯଦି (varp) {
            Ast_ଗଣ୍ଠି* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ adtypep = VN_CAST(fromDtp, ଗଣ୍ଠି_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ adtypep = VN_CAST(fromDtp, BasicDType)) {
                ଯଦି (adtypep->isString()) {
                    AstConst* ଅଚଳ leftp = ନୂତନ AstConst{fl, 0};
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rightp = ନୂତନ AstLenN{fl, fromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } ଅନ୍ୟଥା {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } ଅନ୍ୟଥା ଯଦି (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* ଅଚଳ leftp = ନୂତନ AstConst{fl, 0};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rightp = ନୂତନ AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), ଗଣ୍ଠି_Array_DType)
                        ? ନୂତନ AstArraySel{fl, subfromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                          ନୂତନ AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = ନୂତନ AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(ମିଥ୍ୟା);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = ନୂତନ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = ନୂତନ AstVarRef{fl, varp, VAccess::READ};
                subfromp = ନୂତନ AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            ଯଦି (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_ବୃକ୍ଷ(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_ଗଣ୍ଠି* ଅଚଳ bodyp = nodep->bodyp();
    ଯଦି (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        ଯଦି (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_ବୃକ୍ଷ(), bodyPointp);
        ଫେରନ୍ତୁ nullptr;
    }
    ଯଦି (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } ଅନ୍ୟଥା {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_ବୃକ୍ଷ(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    ଫେରନ୍ତୁ newp;
}
