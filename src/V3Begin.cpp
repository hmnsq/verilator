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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Begin.h"

#समावेश "V3Stats.h"
#समावेश "V3String.h"
#समावेश "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    स्थिर VNUser1InUse m_inuser1;
    bool m_anyFuncInBegin = false;

खुला:
    BeginState() = शेष;
    ~BeginState() = शेष;
    व्योम userMarkChanged(Ast_गाँठ* nodep) {
        nodep->user1(true);
        m_anyFuncInBegin = true;
    }
    bool anyFuncInBegin() स्थिर { वापस m_anyFuncInBegin; }
};

//######################################################################

श्रेणी BeginVisitor final : खुला VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* स्थिर m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाँठ_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_गाँठ* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    पूर्णांक m_ifDepth = 0;  // Current if depth
    bool m_keepBegins = false;  // True if begins should not be inlined

    // METHODS

    string dot(स्थिर string& a, स्थिर string& b) { वापस VString::dot(a, "__DOT__", b); }

    व्योम dotNames(स्थिर std::string& name, File_पंक्ति* स्थिर flp, स्थिर अक्षर* स्थिर blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        अगर (name == "") वापस;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::आकार_type pos;
        जबतक ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            स्थिर std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::म_माप("__DOT__"));
            अगर (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            अगर (!m_ftaskp) {
                AstCellInline* स्थिर inlinep = नया AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_गाँठ::add_अग्र_Null(m_inlinesp, inlinep);
            }
        }
    }

    व्योम lift_गाँठ(Ast_गाँठ* nodep) {
        nodep->unlinkFrBack();
        अगर (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            अगर (!m_liftedp) {
                m_liftedp = nodep;
            } अन्यथा {
                m_liftedp->add_अग्र(nodep);
            }
        } अन्यथा {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    व्योम visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = true;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        अगर (m_unnamedScope != "") {
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
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        अगर (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* स्थिर initp) {
            initp->unlinkFrBack();
            m_ftaskp->add_Here_This_As_अग्र(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    व्योम visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_अग्र_Null(nodep->itemsp());
        // Repalce node with body then delete
        अगर (Ast_गाँठ* स्थिर itemsp = nodep->itemsp()) {
            nodep->add_अग्र_Here(itemsp->unlink_Fr_Back_With_अग्र());
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_अग्र_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = true;
            iterate_And_अग्र_Null(nodep->forksp());
        }
        Ast_गाँठ* addsp = nullptr;
        अगर (Ast_गाँठ* स्थिर declsp = nodep->declsp()) {
            declsp->unlink_Fr_Back_With_अग्र();
            addsp = Ast_गाँठ::add_अग्र(addsp, declsp);
        }
        अगर (Ast_गाँठ* स्थिर stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_Back_With_अग्र();
            addsp = Ast_गाँठ::add_अग्र(addsp, stmtsp);
        }
        अगर (addsp) nodep->add_Here_This_As_अग्र(addsp);
        nodep->name("");
    }
    व्योम visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = false;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        अगर (m_keepBegins) {
            nodep->name("");
            वापस;
        }
        Ast_गाँठ* addsp = nullptr;
        अगर (Ast_गाँठ* स्थिर declsp = nodep->declsp()) {
            declsp->unlink_Fr_Back_With_अग्र();
            addsp = Ast_गाँठ::add_अग्र(addsp, declsp);
        }
        अगर (Ast_गाँठ* स्थिर stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_Back_With_अग्र();
            addsp = Ast_गाँठ::add_अग्र(addsp, stmtsp);
        }
        अगर (addsp) {
            nodep->replaceWith(addsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(Ast_गाँठ_Block* nodep) override {
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
        अगर (m_keepBegins) {
            nodep->name("");
            वापस;
        }
        Ast_गाँठ* addsp = nullptr;
        अगर (Ast_गाँठ* स्थिर declsp = nodep->declsp()) {
            declsp->unlink_Fr_Back_With_अग्र();
            addsp = Ast_गाँठ::add_अग्र(addsp, declsp);
        }
        अगर (Ast_गाँठ* स्थिर stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_Back_With_अग्र();
            addsp = Ast_गाँठ::add_अग्र(addsp, stmtsp);
        }
        अगर (addsp) {
            nodep->replaceWith(addsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstVar* nodep) override {
        अगर (nodep->user1SetOnce()) { वापस; }
        // If static variable, move it outside a function.
        अगर (nodep->lifetime().isStatic() && m_ftaskp) {
            स्थिर std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            अगर (nodep->isIO()) {
                अगर (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* स्थिर portp = nodep->clone_वृक्ष_Pure(false);
                nodep->replaceWith(portp);

                अगर (nodep->is_निवेश() || nodep->isInout()) {
                    AstAssign* स्थिर initAssignp = नया AstAssign{
                        nodep->fileline(), नया AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        नया AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->add_अग्र_Here(initAssignp);
                }

                अगर (nodep->isWritable()) {
                    AstAssign* स्थिर endAssignp = नया AstAssign{
                        nodep->fileline(), नया AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        नया AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } अन्यथा {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->add_Here_This_As_अग्र(nodep);
            nodep->funcLocal(false);
        } अन्यथा अगर (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_गाँठ(nodep);
        }
    }
    व्योम visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_Back_With_अग्र());
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        अगर (nodep->user1SetOnce()) वापस;  // Don't double-add text's
        Ast_गाँठ* wasUnderp = m_ftaskp;
        अगर (!m_ftaskp) wasUnderp = m_procedurep;
        अगर (wasUnderp) {
            अगर (nodep->stmtsp()) {
                Ast_गाँठ* स्थिर newp = नया AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_Back_With_अग्र()};
                wasUnderp->add_Here_This_As_अग्र(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    व्योम visit(AstTypedef* nodep) override {
        अगर (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_गाँठ(nodep);
        }
    }
    व्योम visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        अगर (m_namedScope != "") {
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
    व्योम visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        अगर (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        अगर (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        अगर (nodep->user1SetOnce()) वापस;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        स्थिर std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        अगर (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    व्योम visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    व्योम visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = false;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        अगर (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } अन्यथा अगर (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } अन्यथा अगर (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, true);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = false;
        iterateChildren(nodep);
    }

खुला:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = शेष;
};

//######################################################################

श्रेणी BeginRelinkVisitor final : खुला VNVisitorConst {
    // Replace tasks with new pointer
निजी:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        अगर (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstVarRef* nodep) override {
        अगर (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        अगर (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = शेष;
};

//######################################################################
// Task class functions

व्योम V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        अगर (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("begin", 0, dump_वृक्ष_Either_Level() >= 3);
}

अटल Ast_गाँठ* createForeachLoop(Ast_गाँठ_Foreach* /*nodep*/, Ast_गाँठ* bodysp, bool arrayMayResize,
                                  Ast_गाँठ_व्यंजक* subfromp, AstVar* varp, Ast_गाँठ_व्यंजक* leftp,
                                  Ast_गाँठ_व्यंजक* rightp, VNType गाँठ_Type) {
    File_पंक्ति* स्थिर fl = varp->fileline();
    Ast_गाँठ_व्यंजक* varRefp = नया AstVarRef{fl, varp, VAccess::READ};
    Ast_गाँठ_व्यंजक* condp;
    bool inc = true;
    चयन (गाँठ_Type) {
    हाल VNType::LteS: condp = नया AstLteS{fl, varRefp, rightp}; अवरोध;
    हाल VNType::Lt: condp = नया AstLt{fl, varRefp, rightp}; अवरोध;
    हाल VNType::GteS:
        condp = नया AstGteS{fl, varRefp, rightp};
        inc = false;
        अवरोध;
    शेष: varp->v3fatalSrc("Missing comparison handling"); अवरोध;
    }
    Ast_गाँठ_व्यंजक* incp;
    अगर (inc)
        incp = नया AstAdd{fl, varRefp->clone_वृक्ष(false), नया AstConst{fl, 1}};
    अन्यथा
        incp = नया AstSub{fl, varRefp->clone_वृक्ष(false), नया AstConst{fl, 1}};

    AstLoop* स्थिर loopp = नया AstLoop{fl};
    loopp->addStmtsp(नया AstLoopTest{fl, loopp, condp});
    AstVar* आकार_Varp = nullptr;
    Ast_गाँठ_व्यंजक* आकार_Getp = nullptr;
    अगर (arrayMayResize) {
        आकार_Varp = नया AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        आकार_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        आकार_Varp->usedLoopIdx(true);  // Not technically an index, but used only inside loop
        varp->add_अग्र(आकार_Varp);
        आकार_Getp
            = नया AstCMethodHard{fl, subfromp->clone_वृक्ष_Pure(false), VCMethod::DYN_आकार, nullptr};
        आकार_Getp->dtypeSetUInt32();
        loopp->addStmtsp(नया AstAssign{fl, नया AstVarRef{fl, आकार_Varp, VAccess::WRITE}, आकार_Getp});
    }
    loopp->addStmtsp(bodysp);
    Ast_गाँठ_Stmt* incStmtp = नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE}, incp};
    अगर (arrayMayResize) {
        incStmtp = नया AstIf{fl,
                             नया AstLte{fl, नया AstVarRef{fl, आकार_Varp, VAccess::READ},
                                        आकार_Getp->clone_वृक्ष_Pure(false)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_गाँठ* स्थिर stmtsp = varp;  // New statements for outer loop
    stmtsp->add_अग्र(नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->add_अग्र(loopp);
    वापस stmtsp;
}
अटल Ast_गाँठ* createForeachLoopRanged(Ast_गाँठ_Foreach* nodep, Ast_गाँठ* bodysp,
                                        Ast_गाँठ_व्यंजक* subfromp, AstVar* varp,
                                        स्थिर VNumRange& declRange) {
    File_पंक्ति* स्थिर fl = varp->fileline();
    V3_संख्या बाँए{nodep, 32};
    V3_संख्या दाँए{nodep, 32};
    बाँए.isSigned(true);
    दाँए.isSigned(true);
    बाँए.setLongS(declRange.बाँए());
    दाँए.setLongS(declRange.दाँए());
    Ast_गाँठ_व्यंजक* स्थिर leftp = नया AstConst{fl, बाँए};
    Ast_गाँठ_व्यंजक* स्थिर rightp = नया AstConst{fl, दाँए};
    वापस createForeachLoop(nodep, bodysp, false /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.बाँए() <= declRange.दाँए() ? VNType::LteS : VNType::GteS);
}
अटल Ast_गाँठ* createForeachAssoc(File_पंक्ति* fl, AstVar* varp, Ast_गाँठ_व्यंजक* subfromp,
                                   Ast_गाँठ_DType* fromDtp, Ast_गाँठ* bodyPointp) {
    Ast_गाँठ* loopp = varp;
    AstVar* स्थिर अग्र_varp  // Iterator containing next element (to handle mid-array delete)
        = नया AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    अग्र_varp->usedLoopIdx(true);
    अग्र_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->add_अग्र(अग्र_varp);

    AstVar* स्थिर more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = नया AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(true);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->add_अग्र(more_varp);

    Ast_गाँठ_व्यंजक* स्थिर firstp
        = नया AstCMethodHard{fl, subfromp->clone_वृक्ष_Pure(false), VCMethod::ASSOC_FIRST,
                             नया AstVarRef{fl, अग्र_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_गाँठ_व्यंजक* स्थिर nextp
        = नया AstCMethodHard{fl, subfromp->clone_वृक्ष_Pure(false), VCMethod::ASSOC_अग्र,
                             नया AstVarRef{fl, अग्र_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->add_अग्र(नया AstAssign{fl, नया AstVarRef{fl, more_varp, VAccess::WRITE},
                                 नया AstNeq{fl, नया AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* स्थिर lp = नया AstLoop{fl};
    loopp->add_अग्र(lp);
    lp->addStmtsp(नया AstLoopTest{fl, lp, नया AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE},
                                नया AstVarRef{fl, अग्र_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(नया AstAssign{fl, नया AstVarRef{fl, more_varp, VAccess::WRITE},
                                नया AstNeq{fl, नया AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    वापस loopp;
}

अटल bool arrayMayResizeCheck(Ast_गाँठ* nodesp, AstVar* fromVarp) {
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
    अगर (!nodesp) वापस false;
    वापस nodesp->exists_And_अग्र([&](स्थिर Ast_गाँठ* nodep) -> bool {
        अगर (स्थिर Ast_गाँठ_Var_Ref* varrefp = VN_CAST(nodep, गाँठ_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            अगर (varrefp->access().isWriteOrRW()) {
                अगर (!fromVarp) वापस true;
                अगर (varrefp->varp() == fromVarp) वापस true;
            }
        }
        // No idea what a function might be doing
        अगर (VN_IS(nodep, MethodCall) || VN_IS(nodep, गाँठ_FTask_Ref)) वापस true;
        वापस false;
    });
}

Ast_गाँठ* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    स्थिर AstForeachHeader* स्थिर headerp = nodep->headerp();
    Ast_गाँठ_व्यंजक* स्थिर fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* स्थिर fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_गाँठ_DType* fromDtp = fromp->dtypep()->skipRefp();
    स्थिर bool arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_गाँठ* bodyPointp = नया AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, false};
    Ast_गाँठ* newp = nullptr;
    Ast_गाँठ* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_गाँठ_व्यंजक* subfromp = fromp->clone_वृक्ष_Pure(false);
    // Major dimension first
    क्रम (Ast_गाँठ *argsp = headerp->elementsp(), *अग्र_argsp; argsp; argsp = अग्र_argsp) {
        अग्र_argsp = argsp->nextp();
        स्थिर bool empty = VN_IS(argsp, Empty);
        AstVar* स्थिर varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        अगर (varp) varp->unlinkFrBack()->usedLoopIdx(true);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_पंक्ति* स्थिर fl = argsp->fileline();
        अगर (varp) {
            Ast_गाँठ* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            अगर (स्थिर Ast_गाँठ_Array_DType* स्थिर adtypep = VN_CAST(fromDtp, गाँठ_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } अन्यथा अगर (स्थिर AstBasicDType* स्थिर adtypep = VN_CAST(fromDtp, BasicDType)) {
                अगर (adtypep->isString()) {
                    AstConst* स्थिर leftp = नया AstConst{fl, 0};
                    Ast_गाँठ_व्यंजक* स्थिर rightp = नया AstLenN{fl, fromp->clone_वृक्ष_Pure(false)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } अन्यथा {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } अन्यथा अगर (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* स्थिर leftp = नया AstConst{fl, 0};
                Ast_गाँठ_व्यंजक* स्थिर rightp = नया AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), गाँठ_Array_DType)
                        ? नया AstArraySel{fl, subfromp->clone_वृक्ष_Pure(false),
                                          नया AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_वृक्ष_Pure(false),
                    VCMethod::DYN_आकार};
                AstVarRef* varRefp = नया AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(false);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = नया AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } अन्यथा अगर (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = नया AstVarRef{fl, varp, VAccess::READ};
                subfromp = नया AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            अगर (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_वृक्ष(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_गाँठ* स्थिर bodyp = nodep->bodyp();
    अगर (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        अगर (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_वृक्ष(), bodyPointp);
        वापस nullptr;
    }
    अगर (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_Back_With_अग्र());
    } अन्यथा {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_वृक्ष(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    वापस newp;
}
