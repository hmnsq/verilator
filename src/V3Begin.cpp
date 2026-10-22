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
    अचर VNUser1InUse m_inuser1;
    तार्किक m_anyFuncInBegin = असत्य;

सार्वजनिक:
    BeginState() = स्वभाव;
    ~BeginState() = स्वभाव;
    शून्य userMarkChanged(Ast_गाँठो* nodep) {
        nodep->user1(सत्य);
        m_anyFuncInBegin = सत्य;
    }
    तार्किक anyFuncInBegin() अचर { फर्काऊ m_anyFuncInBegin; }
};

//######################################################################

वर्ग BeginVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* अचर m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_गाँठो_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाँठो_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_गाँठो* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    पूर्णाङ्क m_ifDepth = 0;  // Current if depth
    तार्किक m_keepBegins = असत्य;  // True if begins should not be inlined

    // METHODS

    string dot(अचर string& a, अचर string& b) { फर्काऊ VString::dot(a, "__DOT__", b); }

    शून्य dotNames(अचर std::string& name, File_पङ्क्ति* अचर flp, अचर वर्ण* अचर blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        यदि (name == "") फर्काऊ;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        जबसम्म ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            अचर std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::म_माप("__DOT__"));
            यदि (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            यदि (!m_ftaskp) {
                AstCellInline* अचर inlinep = नूतन AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_गाँठो::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    शून्य lift_गाँठो(Ast_गाँठो* nodep) {
        nodep->unlinkFrBack();
        यदि (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            यदि (!m_liftedp) {
                m_liftedp = nodep;
            } अन्यथा {
                m_liftedp->addNext(nodep);
            }
        } अन्यथा {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    शून्य visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        यदि (m_unnamedScope != "") {
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
    शून्य visit(Ast_गाँठो_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        यदि (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* अचर initp) {
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
        यदि (Ast_गाँठो* अचर itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = सत्य;
            iterateAndNextNull(nodep->forksp());
        }
        Ast_गाँठो* addsp = nullptr;
        यदि (Ast_गाँठो* अचर declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_गाँठो::addNext(addsp, declsp);
        }
        यदि (Ast_गाँठो* अचर stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_गाँठो::addNext(addsp, stmtsp);
        }
        यदि (addsp) nodep->addHereThisAsNext(addsp);
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
            m_keepBegins = असत्य;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        यदि (m_keepBegins) {
            nodep->name("");
            फर्काऊ;
        }
        Ast_गाँठो* addsp = nullptr;
        यदि (Ast_गाँठो* अचर declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_गाँठो::addNext(addsp, declsp);
        }
        यदि (Ast_गाँठो* अचर stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_गाँठो::addNext(addsp, stmtsp);
        }
        यदि (addsp) {
            nodep->replaceWith(addsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_गाँठो_Block* nodep) override {
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
        यदि (m_keepBegins) {
            nodep->name("");
            फर्काऊ;
        }
        Ast_गाँठो* addsp = nullptr;
        यदि (Ast_गाँठो* अचर declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_गाँठो::addNext(addsp, declsp);
        }
        यदि (Ast_गाँठो* अचर stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_गाँठो::addNext(addsp, stmtsp);
        }
        यदि (addsp) {
            nodep->replaceWith(addsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstVar* nodep) override {
        यदि (nodep->user1SetOnce()) { फर्काऊ; }
        // If static variable, move it outside a function.
        यदि (nodep->lifetime().isStatic() && m_ftaskp) {
            अचर std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            यदि (nodep->isIO()) {
                यदि (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* अचर portp = nodep->clone_रूख_Pure(असत्य);
                nodep->replaceWith(portp);

                यदि (nodep->is_आगत() || nodep->isInout()) {
                    AstAssign* अचर initAssignp = नूतन AstAssign{
                        nodep->fileline(), नूतन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        नूतन AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                यदि (nodep->isWritable()) {
                    AstAssign* अचर endAssignp = नूतन AstAssign{
                        nodep->fileline(), नूतन AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        नूतन AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } अन्यथा {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(असत्य);
        } अन्यथा यदि (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_गाँठो(nodep);
        }
    }
    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Don't double-add text's
        Ast_गाँठो* wasUnderp = m_ftaskp;
        यदि (!m_ftaskp) wasUnderp = m_procedurep;
        यदि (wasUnderp) {
            यदि (nodep->stmtsp()) {
                Ast_गाँठो* अचर newp = नूतन AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    शून्य visit(AstTypedef* nodep) override {
        यदि (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_गाँठो(nodep);
        }
    }
    शून्य visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        यदि (m_namedScope != "") {
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
        यदि (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        यदि (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        अचर std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        यदि (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    शून्य visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    शून्य visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = असत्य;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        यदि (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } अन्यथा यदि (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } अन्यथा यदि (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, सत्य);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो* nodep) override {
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
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        यदि (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstVarRef* nodep) override {
        यदि (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        यदि (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

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
        यदि (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("begin", 0, dump_रूख_Either_Level() >= 3);
}

स्थिर Ast_गाँठो* createForeachLoop(Ast_गाँठो_Foreach* /*nodep*/, Ast_गाँठो* bodysp, तार्किक arrayMayResize,
                                  Ast_गाँठो_अभिव्यक्ति* subfromp, AstVar* varp, Ast_गाँठो_अभिव्यक्ति* leftp,
                                  Ast_गाँठो_अभिव्यक्ति* rightp, VNType गाँठो_Type) {
    File_पङ्क्ति* अचर fl = varp->fileline();
    Ast_गाँठो_अभिव्यक्ति* varRefp = नूतन AstVarRef{fl, varp, VAccess::READ};
    Ast_गाँठो_अभिव्यक्ति* condp;
    तार्किक inc = सत्य;
    विकल्प (गाँठो_Type) {
    पक्ष VNType::LteS: condp = नूतन AstLteS{fl, varRefp, rightp}; रोक;
    पक्ष VNType::Lt: condp = नूतन AstLt{fl, varRefp, rightp}; रोक;
    पक्ष VNType::GteS:
        condp = नूतन AstGteS{fl, varRefp, rightp};
        inc = असत्य;
        रोक;
    स्वभाव: varp->v3fatalSrc("Missing comparison handling"); रोक;
    }
    Ast_गाँठो_अभिव्यक्ति* incp;
    यदि (inc)
        incp = नूतन AstAdd{fl, varRefp->clone_रूख(असत्य), नूतन AstConst{fl, 1}};
    अन्यथा
        incp = नूतन AstSub{fl, varRefp->clone_रूख(असत्य), नूतन AstConst{fl, 1}};

    AstLoop* अचर loopp = नूतन AstLoop{fl};
    loopp->addStmtsp(नूतन AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    Ast_गाँठो_अभिव्यक्ति* sizeGetp = nullptr;
    यदि (arrayMayResize) {
        sizeVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(सत्य);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = नूतन AstCMethodHard{fl, subfromp->clone_रूख_Pure(असत्य), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    Ast_गाँठो_Stmt* incStmtp = नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, incp};
    यदि (arrayMayResize) {
        incStmtp = नूतन AstIf{fl,
                             नूतन AstLte{fl, नूतन AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->clone_रूख_Pure(असत्य)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_गाँठो* अचर stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    फर्काऊ stmtsp;
}
स्थिर Ast_गाँठो* createForeachLoopRanged(Ast_गाँठो_Foreach* nodep, Ast_गाँठो* bodysp,
                                        Ast_गाँठो_अभिव्यक्ति* subfromp, AstVar* varp,
                                        अचर VNumRange& declRange) {
    File_पङ्क्ति* अचर fl = varp->fileline();
    V3_सङ्ख्या बायाँ{nodep, 32};
    V3_सङ्ख्या दायाँ{nodep, 32};
    बायाँ.isSigned(सत्य);
    दायाँ.isSigned(सत्य);
    बायाँ.setLongS(declRange.बायाँ());
    दायाँ.setLongS(declRange.दायाँ());
    Ast_गाँठो_अभिव्यक्ति* अचर leftp = नूतन AstConst{fl, बायाँ};
    Ast_गाँठो_अभिव्यक्ति* अचर rightp = नूतन AstConst{fl, दायाँ};
    फर्काऊ createForeachLoop(nodep, bodysp, असत्य /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.बायाँ() <= declRange.दायाँ() ? VNType::LteS : VNType::GteS);
}
स्थिर Ast_गाँठो* createForeachAssoc(File_पङ्क्ति* fl, AstVar* varp, Ast_गाँठो_अभिव्यक्ति* subfromp,
                                   Ast_गाँठो_DType* fromDtp, Ast_गाँठो* bodyPointp) {
    Ast_गाँठो* loopp = varp;
    AstVar* अचर next_varp  // Iterator containing next element (to handle mid-array delete)
        = नूतन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(सत्य);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* अचर more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = नूतन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(सत्य);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    Ast_गाँठो_अभिव्यक्ति* अचर firstp
        = नूतन AstCMethodHard{fl, subfromp->clone_रूख_Pure(असत्य), VCMethod::ASSOC_FIRST,
                             नूतन AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_गाँठो_अभिव्यक्ति* अचर nextp
        = नूतन AstCMethodHard{fl, subfromp->clone_रूख_Pure(असत्य), VCMethod::ASSOC_NEXT,
                             नूतन AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(नूतन AstAssign{fl, नूतन AstVarRef{fl, more_varp, VAccess::WRITE},
                                 नूतन AstNeq{fl, नूतन AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* अचर lp = नूतन AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(नूतन AstLoopTest{fl, lp, नूतन AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                नूतन AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, more_varp, VAccess::WRITE},
                                नूतन AstNeq{fl, नूतन AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    फर्काऊ loopp;
}

स्थिर तार्किक arrayMayResizeCheck(Ast_गाँठो* nodesp, AstVar* fromVarp) {
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
    यदि (!nodesp) फर्काऊ असत्य;
    फर्काऊ nodesp->existsAndNext([&](अचर Ast_गाँठो* nodep) -> तार्किक {
        यदि (अचर Ast_गाँठो_Var_Ref* varrefp = VN_CAST(nodep, गाँठो_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            यदि (varrefp->access().isWriteOrRW()) {
                यदि (!fromVarp) फर्काऊ सत्य;
                यदि (varrefp->varp() == fromVarp) फर्काऊ सत्य;
            }
        }
        // No idea what a function might be doing
        यदि (VN_IS(nodep, MethodCall) || VN_IS(nodep, गाँठो_FTask_Ref)) फर्काऊ सत्य;
        फर्काऊ असत्य;
    });
}

Ast_गाँठो* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    अचर AstForeachHeader* अचर headerp = nodep->headerp();
    Ast_गाँठो_अभिव्यक्ति* अचर fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* अचर fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_गाँठो_DType* fromDtp = fromp->dtypep()->skipRefp();
    अचर तार्किक arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_गाँठो* bodyPointp = नूतन AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, असत्य};
    Ast_गाँठो* newp = nullptr;
    Ast_गाँठो* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_गाँठो_अभिव्यक्ति* subfromp = fromp->clone_रूख_Pure(असत्य);
    // Major dimension first
    लागि (Ast_गाँठो *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        अचर तार्किक empty = VN_IS(argsp, Empty);
        AstVar* अचर varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        यदि (varp) varp->unlinkFrBack()->usedLoopIdx(सत्य);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_पङ्क्ति* अचर fl = argsp->fileline();
        यदि (varp) {
            Ast_गाँठो* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            यदि (अचर Ast_गाँठो_Array_DType* अचर adtypep = VN_CAST(fromDtp, गाँठो_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } अन्यथा यदि (अचर AstBasicDType* अचर adtypep = VN_CAST(fromDtp, BasicDType)) {
                यदि (adtypep->isString()) {
                    AstConst* अचर leftp = नूतन AstConst{fl, 0};
                    Ast_गाँठो_अभिव्यक्ति* अचर rightp = नूतन AstLenN{fl, fromp->clone_रूख_Pure(असत्य)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } अन्यथा {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } अन्यथा यदि (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* अचर leftp = नूतन AstConst{fl, 0};
                Ast_गाँठो_अभिव्यक्ति* अचर rightp = नूतन AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), गाँठो_Array_DType)
                        ? नूतन AstArraySel{fl, subfromp->clone_रूख_Pure(असत्य),
                                          नूतन AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_रूख_Pure(असत्य),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = नूतन AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
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
    VL_DO_DANGLING(subfromp->delete_रूख(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_गाँठो* अचर bodyp = nodep->bodyp();
    यदि (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
        यदि (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_रूख(), bodyPointp);
        फर्काऊ nullptr;
    }
    यदि (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } अन्यथा {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_रूख(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    फर्काऊ newp;
}
