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
    ध्रुव VNUser1InUse m_inuser1;
    तार्किक m_anyFuncInBegin = मिच्छ;

सार्वजनिक:
    BeginState() = स्वभाव;
    ~BeginState() = स्वभाव;
    शून्य userMarkChanged(Ast_ग्रन्थिः* nodep) {
        nodep->user1(सच्च);
        m_anyFuncInBegin = सच्च;
    }
    तार्किक anyFuncInBegin() ध्रुव { प्रत्यागम m_anyFuncInBegin; }
};

//######################################################################

वर्ग BeginVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ध्रुव m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_ग्रन्थिः_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ग्रन्थिः_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_ग्रन्थिः* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    पूर्णाङ्क m_ifDepth = 0;  // Current if depth
    तार्किक m_keepBegins = मिच्छ;  // True if begins should not be inlined

    // METHODS

    string dot(ध्रुव string& a, ध्रुव string& b) { प्रत्यागम VString::dot(a, "__DOT__", b); }

    शून्य dotNames(ध्रुव std::string& name, File_पङ्क्तिः* ध्रुव flp, ध्रुव वर्ण* ध्रुव blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        जइ (name == "") प्रत्यागम;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        जाव ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            ध्रुव std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            जइ (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            जइ (!m_ftaskp) {
                AstCellInline* ध्रुव inlinep = नूतन AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_ग्रन्थिः::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    शून्य lift_ग्रन्थिः(Ast_ग्रन्थिः* nodep) {
        nodep->unlinkFrBack();
        जइ (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            जइ (!m_liftedp) {
                m_liftedp = nodep;
            } अण्णहा {
                m_liftedp->addNext(nodep);
            }
        } अण्णहा {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    शून्य visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = सच्च;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        जइ (m_unnamedScope != "") {
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
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        जइ (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* ध्रुव initp) {
            initp->unlinkFrBack();
            m_ftaskp->addHereThisAsNext(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    शून्य visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterateAndNextNull(nodep->itemsp());
        // Repalce node with body then delete
        जइ (Ast_ग्रन्थिः* ध्रुव itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = सच्च;
            iterateAndNextNull(nodep->forksp());
        }
        Ast_ग्रन्थिः* addsp = nullptr;
        जइ (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ग्रन्थिः::addNext(addsp, declsp);
        }
        जइ (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ग्रन्थिः::addNext(addsp, stmtsp);
        }
        जइ (addsp) nodep->addHereThisAsNext(addsp);
        nodep->name("");
    }
    शून्य visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = मिच्छ;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        जइ (m_keepBegins) {
            nodep->name("");
            प्रत्यागम;
        }
        Ast_ग्रन्थिः* addsp = nullptr;
        जइ (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ग्रन्थिः::addNext(addsp, declsp);
        }
        जइ (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ग्रन्थिः::addNext(addsp, stmtsp);
        }
        जइ (addsp) {
            nodep->replaceWith(addsp);
        } अण्णहा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Block* nodep) override {
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
        जइ (m_keepBegins) {
            nodep->name("");
            प्रत्यागम;
        }
        Ast_ग्रन्थिः* addsp = nullptr;
        जइ (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ग्रन्थिः::addNext(addsp, declsp);
        }
        जइ (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ग्रन्थिः::addNext(addsp, stmtsp);
        }
        जइ (addsp) {
            nodep->replaceWith(addsp);
        } अण्णहा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstVar* nodep) override {
        जइ (nodep->user1SetOnce()) { प्रत्यागम; }
        // If static variable, move it outside a function.
        जइ (nodep->lifetime().isStatic() && m_ftaskp) {
            ध्रुव std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            जइ (nodep->isIO()) {
                जइ (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ध्रुव portp = nodep->clone_वृक्षः_Pure(मिच्छ);
                nodep->replaceWith(portp);

                जइ (nodep->is_निवेशः() || nodep->isInout()) {
                    AstAssign* ध्रुव initAssignp = नूतन AstAssign{
                        nodep->fileline(), नूतन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        नूतन AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                जइ (nodep->isWritable()) {
                    AstAssign* ध्रुव endAssignp = नूतन AstAssign{
                        nodep->fileline(), नूतन AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        नूतन AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } अण्णहा {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(मिच्छ);
        } अण्णहा जइ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_ग्रन्थिः(nodep);
        }
    }
    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        जइ (nodep->user1SetOnce()) प्रत्यागम;  // Don't double-add text's
        Ast_ग्रन्थिः* wasUnderp = m_ftaskp;
        जइ (!m_ftaskp) wasUnderp = m_procedurep;
        जइ (wasUnderp) {
            जइ (nodep->stmtsp()) {
                Ast_ग्रन्थिः* ध्रुव newp = नूतन AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
        } अण्णहा {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    शून्य visit(AstTypedef* nodep) override {
        जइ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_ग्रन्थिः(nodep);
        }
    }
    शून्य visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        जइ (m_namedScope != "") {
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
    शून्य visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        जइ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        जइ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        जइ (nodep->user1SetOnce()) प्रत्यागम;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ध्रुव std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        जइ (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    शून्य visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    शून्य visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = मिच्छ;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        जइ (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } अण्णहा जइ (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } अण्णहा जइ (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, सच्च);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = मिच्छ;
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
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        जइ (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstVarRef* nodep) override {
        जइ (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        जइ (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

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
        जइ (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("begin", 0, dump_वृक्षः_Either_Level() >= 3);
}

स्थिर Ast_ग्रन्थिः* createForeachLoop(Ast_ग्रन्थिः_Foreach* /*nodep*/, Ast_ग्रन्थिः* bodysp, तार्किक arrayMayResize,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* subfromp, AstVar* varp, Ast_ग्रन्थिः_व्यञ्जकम्* leftp,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* rightp, VNType ग्रन्थिः_Type) {
    File_पङ्क्तिः* ध्रुव fl = varp->fileline();
    Ast_ग्रन्थिः_व्यञ्जकम्* varRefp = नूतन AstVarRef{fl, varp, VAccess::READ};
    Ast_ग्रन्थिः_व्यञ्जकम्* condp;
    तार्किक inc = सच्च;
    विकल्प (ग्रन्थिः_Type) {
    पक्ष VNType::LteS: condp = नूतन AstLteS{fl, varRefp, rightp}; भङ्ग;
    पक्ष VNType::Lt: condp = नूतन AstLt{fl, varRefp, rightp}; भङ्ग;
    पक्ष VNType::GteS:
        condp = नूतन AstGteS{fl, varRefp, rightp};
        inc = मिच्छ;
        भङ्ग;
    स्वभाव: varp->v3fatalSrc("Missing comparison handling"); भङ्ग;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* incp;
    जइ (inc)
        incp = नूतन AstAdd{fl, varRefp->clone_वृक्षः(मिच्छ), नूतन AstConst{fl, 1}};
    अण्णहा
        incp = नूतन AstSub{fl, varRefp->clone_वृक्षः(मिच्छ), नूतन AstConst{fl, 1}};

    AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
    loopp->addStmtsp(नूतन AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    Ast_ग्रन्थिः_व्यञ्जकम्* sizeGetp = nullptr;
    जइ (arrayMayResize) {
        sizeVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(सच्च);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = नूतन AstCMethodHard{fl, subfromp->clone_वृक्षः_Pure(मिच्छ), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    Ast_ग्रन्थिः_Stmt* incStmtp = नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, incp};
    जइ (arrayMayResize) {
        incStmtp = नूतन AstIf{fl,
                             नूतन AstLte{fl, नूतन AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->clone_वृक्षः_Pure(मिच्छ)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_ग्रन्थिः* ध्रुव stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    प्रत्यागम stmtsp;
}
स्थिर Ast_ग्रन्थिः* createForeachLoopRanged(Ast_ग्रन्थिः_Foreach* nodep, Ast_ग्रन्थिः* bodysp,
                                        Ast_ग्रन्थिः_व्यञ्जकम्* subfromp, AstVar* varp,
                                        ध्रुव VNumRange& declRange) {
    File_पङ्क्तिः* ध्रुव fl = varp->fileline();
    V3_सङ्ख्या वामः{nodep, 32};
    V3_सङ्ख्या दक्षिणः{nodep, 32};
    वामः.isSigned(सच्च);
    दक्षिणः.isSigned(सच्च);
    वामः.setLongS(declRange.वामः());
    दक्षिणः.setLongS(declRange.दक्षिणः());
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव leftp = नूतन AstConst{fl, वामः};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rightp = नूतन AstConst{fl, दक्षिणः};
    प्रत्यागम createForeachLoop(nodep, bodysp, मिच्छ /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.वामः() <= declRange.दक्षिणः() ? VNType::LteS : VNType::GteS);
}
स्थिर Ast_ग्रन्थिः* createForeachAssoc(File_पङ्क्तिः* fl, AstVar* varp, Ast_ग्रन्थिः_व्यञ्जकम्* subfromp,
                                   Ast_ग्रन्थिः_DType* fromDtp, Ast_ग्रन्थिः* bodyPointp) {
    Ast_ग्रन्थिः* loopp = varp;
    AstVar* ध्रुव next_varp  // Iterator containing next element (to handle mid-array delete)
        = नूतन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(सच्च);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* ध्रुव more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = नूतन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(सच्च);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firstp
        = नूतन AstCMethodHard{fl, subfromp->clone_वृक्षः_Pure(मिच्छ), VCMethod::ASSOC_FIRST,
                             नूतन AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nextp
        = नूतन AstCMethodHard{fl, subfromp->clone_वृक्षः_Pure(मिच्छ), VCMethod::ASSOC_NEXT,
                             नूतन AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(नूतन AstAssign{fl, नूतन AstVarRef{fl, more_varp, VAccess::WRITE},
                                 नूतन AstNeq{fl, नूतन AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ध्रुव lp = नूतन AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(नूतन AstLoopTest{fl, lp, नूतन AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                नूतन AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, more_varp, VAccess::WRITE},
                                नूतन AstNeq{fl, नूतन AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
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
    जइ (!nodesp) प्रत्यागम मिच्छ;
    प्रत्यागम nodesp->existsAndNext([&](ध्रुव Ast_ग्रन्थिः* nodep) -> तार्किक {
        जइ (ध्रुव Ast_ग्रन्थिः_Var_Ref* varrefp = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            जइ (varrefp->access().isWriteOrRW()) {
                जइ (!fromVarp) प्रत्यागम सच्च;
                जइ (varrefp->varp() == fromVarp) प्रत्यागम सच्च;
            }
        }
        // No idea what a function might be doing
        जइ (VN_IS(nodep, MethodCall) || VN_IS(nodep, ग्रन्थिः_FTask_Ref)) प्रत्यागम सच्च;
        प्रत्यागम मिच्छ;
    });
}

Ast_ग्रन्थिः* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ध्रुव AstForeachHeader* ध्रुव headerp = nodep->headerp();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ध्रुव fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_ग्रन्थिः_DType* fromDtp = fromp->dtypep()->skipRefp();
    ध्रुव तार्किक arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_ग्रन्थिः* bodyPointp = नूतन AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, मिच्छ};
    Ast_ग्रन्थिः* newp = nullptr;
    Ast_ग्रन्थिः* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_ग्रन्थिः_व्यञ्जकम्* subfromp = fromp->clone_वृक्षः_Pure(मिच्छ);
    // Major dimension first
    प्रति (Ast_ग्रन्थिः *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        ध्रुव तार्किक empty = VN_IS(argsp, Empty);
        AstVar* ध्रुव varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        जइ (varp) varp->unlinkFrBack()->usedLoopIdx(सच्च);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_पङ्क्तिः* ध्रुव fl = argsp->fileline();
        जइ (varp) {
            Ast_ग्रन्थिः* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            जइ (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव adtypep = VN_CAST(fromDtp, ग्रन्थिः_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } अण्णहा जइ (ध्रुव AstBasicDType* ध्रुव adtypep = VN_CAST(fromDtp, BasicDType)) {
                जइ (adtypep->isString()) {
                    AstConst* ध्रुव leftp = नूतन AstConst{fl, 0};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rightp = नूतन AstLenN{fl, fromp->clone_वृक्षः_Pure(मिच्छ)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } अण्णहा {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } अण्णहा जइ (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* ध्रुव leftp = नूतन AstConst{fl, 0};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rightp = नूतन AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), ग्रन्थिः_Array_DType)
                        ? नूतन AstArraySel{fl, subfromp->clone_वृक्षः_Pure(मिच्छ),
                                          नूतन AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_वृक्षः_Pure(मिच्छ),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = नूतन AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(मिच्छ);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = नूतन AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } अण्णहा जइ (VN_IS(fromDtp, AssocArrayDType)) {
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
            जइ (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_वृक्षः(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_ग्रन्थिः* ध्रुव bodyp = nodep->bodyp();
    जइ (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
        जइ (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_वृक्षः(), bodyPointp);
        प्रत्यागम nullptr;
    }
    जइ (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } अण्णहा {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_वृक्षः(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    प्रत्यागम newp;
}
