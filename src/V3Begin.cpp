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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Begin.h"

#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3String.h"
#અન્તર્ભાવ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    અચલ VNUser1InUse m_inuser1;
    તાર્કિક m_anyFuncInBegin = ખોટું;

જાહેર:
    BeginState() = મૂળભૂત;
    ~BeginState() = મૂળભૂત;
    શૂન્ય userMarkChanged(Ast_ગાંઠ* nodep) {
        nodep->user1(સાચું);
        m_anyFuncInBegin = સાચું;
    }
    તાર્કિક anyFuncInBegin() અચલ { પાછા m_anyFuncInBegin; }
};

//######################################################################

વર્ગ BeginVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* અચલ m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ગાંઠ_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_ગાંઠ* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    પૂર્ણાંક m_ifDepth = 0;  // Current if depth
    તાર્કિક m_keepBegins = ખોટું;  // True if begins should not be inlined

    // METHODS

    string dot(અચલ string& a, અચલ string& b) { પાછા VString::dot(a, "__DOT__", b); }

    શૂન્ય dotNames(અચલ std::string& name, File_લીટી* અચલ flp, અચલ અક્ષર* અચલ blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        જો (name == "") પાછા;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        જ્યારે ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            અચલ std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            જો (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            જો (!m_ftaskp) {
                AstCellInline* અચલ inlinep = નવું AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_ગાંઠ::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    શૂન્ય lift_ગાંઠ(Ast_ગાંઠ* nodep) {
        nodep->unlinkFrBack();
        જો (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            જો (!m_liftedp) {
                m_liftedp = nodep;
            } નહીંતર {
                m_liftedp->addNext(nodep);
            }
        } નહીંતર {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    શૂન્ય visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        જો (m_unnamedScope != "") {
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
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        જો (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* અચલ initp) {
            initp->unlinkFrBack();
            m_ftaskp->addHereThisAsNext(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    શૂન્ય visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterateAndNextNull(nodep->itemsp());
        // Repalce node with body then delete
        જો (Ast_ગાંઠ* અચલ itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = સાચું;
            iterateAndNextNull(nodep->forksp());
        }
        Ast_ગાંઠ* addsp = nullptr;
        જો (Ast_ગાંઠ* અચલ declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ગાંઠ::addNext(addsp, declsp);
        }
        જો (Ast_ગાંઠ* અચલ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ગાંઠ::addNext(addsp, stmtsp);
        }
        જો (addsp) nodep->addHereThisAsNext(addsp);
        nodep->name("");
    }
    શૂન્ય visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ખોટું;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        જો (m_keepBegins) {
            nodep->name("");
            પાછા;
        }
        Ast_ગાંઠ* addsp = nullptr;
        જો (Ast_ગાંઠ* અચલ declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ગાંઠ::addNext(addsp, declsp);
        }
        જો (Ast_ગાંઠ* અચલ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ગાંઠ::addNext(addsp, stmtsp);
        }
        જો (addsp) {
            nodep->replaceWith(addsp);
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Block* nodep) override {
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
        જો (m_keepBegins) {
            nodep->name("");
            પાછા;
        }
        Ast_ગાંઠ* addsp = nullptr;
        જો (Ast_ગાંઠ* અચલ declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_ગાંઠ::addNext(addsp, declsp);
        }
        જો (Ast_ગાંઠ* અચલ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_ગાંઠ::addNext(addsp, stmtsp);
        }
        જો (addsp) {
            nodep->replaceWith(addsp);
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstVar* nodep) override {
        જો (nodep->user1SetOnce()) { પાછા; }
        // If static variable, move it outside a function.
        જો (nodep->lifetime().isStatic() && m_ftaskp) {
            અચલ std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            જો (nodep->isIO()) {
                જો (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* અચલ portp = nodep->clone_વૃક્ષ_Pure(ખોટું);
                nodep->replaceWith(portp);

                જો (nodep->is_આગત() || nodep->isInout()) {
                    AstAssign* અચલ initAssignp = નવું AstAssign{
                        nodep->fileline(), નવું AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        નવું AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                જો (nodep->isWritable()) {
                    AstAssign* અચલ endAssignp = નવું AstAssign{
                        nodep->fileline(), નવું AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        નવું AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } નહીંતર {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(ખોટું);
        } નહીંતર જો (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_ગાંઠ(nodep);
        }
    }
    શૂન્ય visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        જો (nodep->user1SetOnce()) પાછા;  // Don't double-add text's
        Ast_ગાંઠ* wasUnderp = m_ftaskp;
        જો (!m_ftaskp) wasUnderp = m_procedurep;
        જો (wasUnderp) {
            જો (nodep->stmtsp()) {
                Ast_ગાંઠ* અચલ newp = નવું AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        } નહીંતર {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    શૂન્ય visit(AstTypedef* nodep) override {
        જો (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_ગાંઠ(nodep);
        }
    }
    શૂન્ય visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        જો (m_namedScope != "") {
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
    શૂન્ય visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        જો (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        જો (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        જો (nodep->user1SetOnce()) પાછા;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        અચલ std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        જો (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    શૂન્ય visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ખોટું;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        જો (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } નહીંતર જો (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } નહીંતર જો (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, સાચું);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ખોટું;
        iterateChildren(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = મૂળભૂત;
};

//######################################################################

વર્ગ BeginRelinkVisitor final : જાહેર VNVisitorConst {
    // Replace tasks with new pointer
ખાનગી:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        જો (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        જો (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        જો (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = મૂળભૂત;
};

//######################################################################
// Task class functions

શૂન્ય V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        જો (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("begin", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

સ્થિર Ast_ગાંઠ* createForeachLoop(Ast_ગાંઠ_Foreach* /*nodep*/, Ast_ગાંઠ* bodysp, તાર્કિક arrayMayResize,
                                  Ast_ગાંઠ_પદાવલિ* subfromp, AstVar* varp, Ast_ગાંઠ_પદાવલિ* leftp,
                                  Ast_ગાંઠ_પદાવલિ* rightp, VNType ગાંઠ_Type) {
    File_લીટી* અચલ fl = varp->fileline();
    Ast_ગાંઠ_પદાવલિ* varRefp = નવું AstVarRef{fl, varp, VAccess::READ};
    Ast_ગાંઠ_પદાવલિ* condp;
    તાર્કિક inc = સાચું;
    પસંદગી (ગાંઠ_Type) {
    સ્થિતિ VNType::LteS: condp = નવું AstLteS{fl, varRefp, rightp}; તોડો;
    સ્થિતિ VNType::Lt: condp = નવું AstLt{fl, varRefp, rightp}; તોડો;
    સ્થિતિ VNType::GteS:
        condp = નવું AstGteS{fl, varRefp, rightp};
        inc = ખોટું;
        તોડો;
    મૂળભૂત: varp->v3fatalSrc("Missing comparison handling"); તોડો;
    }
    Ast_ગાંઠ_પદાવલિ* incp;
    જો (inc)
        incp = નવું AstAdd{fl, varRefp->clone_વૃક્ષ(ખોટું), નવું AstConst{fl, 1}};
    નહીંતર
        incp = નવું AstSub{fl, varRefp->clone_વૃક્ષ(ખોટું), નવું AstConst{fl, 1}};

    AstLoop* અચલ loopp = નવું AstLoop{fl};
    loopp->addStmtsp(નવું AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    Ast_ગાંઠ_પદાવલિ* sizeGetp = nullptr;
    જો (arrayMayResize) {
        sizeVarp = નવું AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(સાચું);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = નવું AstCMethodHard{fl, subfromp->clone_વૃક્ષ_Pure(ખોટું), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(નવું AstAssign{fl, નવું AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    Ast_ગાંઠ_Stmt* incStmtp = નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE}, incp};
    જો (arrayMayResize) {
        incStmtp = નવું AstIf{fl,
                             નવું AstLte{fl, નવું AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->clone_વૃક્ષ_Pure(ખોટું)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_ગાંઠ* અચલ stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    પાછા stmtsp;
}
સ્થિર Ast_ગાંઠ* createForeachLoopRanged(Ast_ગાંઠ_Foreach* nodep, Ast_ગાંઠ* bodysp,
                                        Ast_ગાંઠ_પદાવલિ* subfromp, AstVar* varp,
                                        અચલ VNumRange& declRange) {
    File_લીટી* અચલ fl = varp->fileline();
    V3_સંખ્યા ડાબું{nodep, 32};
    V3_સંખ્યા જમણું{nodep, 32};
    ડાબું.isSigned(સાચું);
    જમણું.isSigned(સાચું);
    ડાબું.setLongS(declRange.ડાબું());
    જમણું.setLongS(declRange.જમણું());
    Ast_ગાંઠ_પદાવલિ* અચલ leftp = નવું AstConst{fl, ડાબું};
    Ast_ગાંઠ_પદાવલિ* અચલ rightp = નવું AstConst{fl, જમણું};
    પાછા createForeachLoop(nodep, bodysp, ખોટું /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.ડાબું() <= declRange.જમણું() ? VNType::LteS : VNType::GteS);
}
સ્થિર Ast_ગાંઠ* createForeachAssoc(File_લીટી* fl, AstVar* varp, Ast_ગાંઠ_પદાવલિ* subfromp,
                                   Ast_ગાંઠ_DType* fromDtp, Ast_ગાંઠ* bodyPointp) {
    Ast_ગાંઠ* loopp = varp;
    AstVar* અચલ next_varp  // Iterator containing next element (to handle mid-array delete)
        = નવું AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(સાચું);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* અચલ more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = નવું AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(સાચું);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    Ast_ગાંઠ_પદાવલિ* અચલ firstp
        = નવું AstCMethodHard{fl, subfromp->clone_વૃક્ષ_Pure(ખોટું), VCMethod::ASSOC_FIRST,
                             નવું AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_ગાંઠ_પદાવલિ* અચલ nextp
        = નવું AstCMethodHard{fl, subfromp->clone_વૃક્ષ_Pure(ખોટું), VCMethod::ASSOC_NEXT,
                             નવું AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(નવું AstAssign{fl, નવું AstVarRef{fl, more_varp, VAccess::WRITE},
                                 નવું AstNeq{fl, નવું AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* અચલ lp = નવું AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(નવું AstLoopTest{fl, lp, નવું AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                                નવું AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(નવું AstAssign{fl, નવું AstVarRef{fl, more_varp, VAccess::WRITE},
                                નવું AstNeq{fl, નવું AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    પાછા loopp;
}

સ્થિર તાર્કિક arrayMayResizeCheck(Ast_ગાંઠ* nodesp, AstVar* fromVarp) {
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
    જો (!nodesp) પાછા ખોટું;
    પાછા nodesp->existsAndNext([&](અચલ Ast_ગાંઠ* nodep) -> તાર્કિક {
        જો (અચલ Ast_ગાંઠ_Var_Ref* varrefp = VN_CAST(nodep, ગાંઠ_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            જો (varrefp->access().isWriteOrRW()) {
                જો (!fromVarp) પાછા સાચું;
                જો (varrefp->varp() == fromVarp) પાછા સાચું;
            }
        }
        // No idea what a function might be doing
        જો (VN_IS(nodep, MethodCall) || VN_IS(nodep, ગાંઠ_FTask_Ref)) પાછા સાચું;
        પાછા ખોટું;
    });
}

Ast_ગાંઠ* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    અચલ AstForeachHeader* અચલ headerp = nodep->headerp();
    Ast_ગાંઠ_પદાવલિ* અચલ fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* અચલ fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_ગાંઠ_DType* fromDtp = fromp->dtypep()->skipRefp();
    અચલ તાર્કિક arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_ગાંઠ* bodyPointp = નવું AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, ખોટું};
    Ast_ગાંઠ* newp = nullptr;
    Ast_ગાંઠ* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_ગાંઠ_પદાવલિ* subfromp = fromp->clone_વૃક્ષ_Pure(ખોટું);
    // Major dimension first
    માટે (Ast_ગાંઠ *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        અચલ તાર્કિક empty = VN_IS(argsp, Empty);
        AstVar* અચલ varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        જો (varp) varp->unlinkFrBack()->usedLoopIdx(સાચું);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_લીટી* અચલ fl = argsp->fileline();
        જો (varp) {
            Ast_ગાંઠ* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            જો (અચલ Ast_ગાંઠ_Array_DType* અચલ adtypep = VN_CAST(fromDtp, ગાંઠ_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } નહીંતર જો (અચલ AstBasicDType* અચલ adtypep = VN_CAST(fromDtp, BasicDType)) {
                જો (adtypep->isString()) {
                    AstConst* અચલ leftp = નવું AstConst{fl, 0};
                    Ast_ગાંઠ_પદાવલિ* અચલ rightp = નવું AstLenN{fl, fromp->clone_વૃક્ષ_Pure(ખોટું)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } નહીંતર {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } નહીંતર જો (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* અચલ leftp = નવું AstConst{fl, 0};
                Ast_ગાંઠ_પદાવલિ* અચલ rightp = નવું AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), ગાંઠ_Array_DType)
                        ? નવું AstArraySel{fl, subfromp->clone_વૃક્ષ_Pure(ખોટું),
                                          નવું AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_વૃક્ષ_Pure(ખોટું),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = નવું AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(ખોટું);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = નવું AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } નહીંતર જો (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = નવું AstVarRef{fl, varp, VAccess::READ};
                subfromp = નવું AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            જો (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_વૃક્ષ(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_ગાંઠ* અચલ bodyp = nodep->bodyp();
    જો (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        જો (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_વૃક્ષ(), bodyPointp);
        પાછા nullptr;
    }
    જો (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } નહીંતર {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_વૃક્ષ(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    પાછા newp;
}
