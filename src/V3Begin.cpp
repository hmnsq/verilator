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
    स्थिरांक VNUser1InUse m_inuser1;
    तार्किक m_anyFuncInBegin = असत्य;

सार्वजनिक:
    BeginState() = पूर्वनिर्धारित;
    ~BeginState() = पूर्वनिर्धारित;
    शून्य userMarkChanged(Ast_गाठ* nodep) {
        nodep->user1(सत्य);
        m_anyFuncInBegin = सत्य;
    }
    तार्किक anyFuncInBegin() स्थिरांक { परत m_anyFuncInBegin; }
};

//######################################################################

वर्ग BeginVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* स्थिरांक m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_गाठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाठ_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_गाठ* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    पूर्णांक m_ifDepth = 0;  // Current if depth
    तार्किक m_keepBegins = असत्य;  // True if begins should not be inlined

    // METHODS

    string dot(स्थिरांक string& a, स्थिरांक string& b) { परत VString::dot(a, "__DOT__", b); }

    शून्य dotNames(स्थिरांक std::string& name, File_ओळ* स्थिरांक flp, स्थिरांक अक्षर* स्थिरांक blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        जर (name == "") परत;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        पर्यंत ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            स्थिरांक std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::म_माप("__DOT__"));
            जर (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            जर (!m_ftaskp) {
                AstCellInline* स्थिरांक inlinep = नवीन AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_गाठ::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    शून्य lift_गाठ(Ast_गाठ* nodep) {
        nodep->unlinkFrBack();
        जर (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            जर (!m_liftedp) {
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
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        जर (m_unnamedScope != "") {
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
    शून्य visit(Ast_गाठ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        जर (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* स्थिरांक initp) {
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
        जर (Ast_गाठ* स्थिरांक itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
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
        Ast_गाठ* addsp = nullptr;
        जर (Ast_गाठ* स्थिरांक declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_गाठ::addNext(addsp, declsp);
        }
        जर (Ast_गाठ* स्थिरांक stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_गाठ::addNext(addsp, stmtsp);
        }
        जर (addsp) nodep->addHereThisAsNext(addsp);
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
        जर (m_keepBegins) {
            nodep->name("");
            परत;
        }
        Ast_गाठ* addsp = nullptr;
        जर (Ast_गाठ* स्थिरांक declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_गाठ::addNext(addsp, declsp);
        }
        जर (Ast_गाठ* स्थिरांक stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_गाठ::addNext(addsp, stmtsp);
        }
        जर (addsp) {
            nodep->replaceWith(addsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_गाठ_Block* nodep) override {
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
        जर (m_keepBegins) {
            nodep->name("");
            परत;
        }
        Ast_गाठ* addsp = nullptr;
        जर (Ast_गाठ* स्थिरांक declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_गाठ::addNext(addsp, declsp);
        }
        जर (Ast_गाठ* स्थिरांक stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_गाठ::addNext(addsp, stmtsp);
        }
        जर (addsp) {
            nodep->replaceWith(addsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstVar* nodep) override {
        जर (nodep->user1SetOnce()) { परत; }
        // If static variable, move it outside a function.
        जर (nodep->lifetime().isStatic() && m_ftaskp) {
            स्थिरांक std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            जर (nodep->isIO()) {
                जर (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* स्थिरांक portp = nodep->clone_वृक्ष_Pure(असत्य);
                nodep->replaceWith(portp);

                जर (nodep->is_आदान() || nodep->isInout()) {
                    AstAssign* स्थिरांक initAssignp = नवीन AstAssign{
                        nodep->fileline(), नवीन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        नवीन AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                जर (nodep->isWritable()) {
                    AstAssign* स्थिरांक endAssignp = नवीन AstAssign{
                        nodep->fileline(), नवीन AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        नवीन AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } अन्यथा {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(असत्य);
        } अन्यथा जर (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_गाठ(nodep);
        }
    }
    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        जर (nodep->user1SetOnce()) परत;  // Don't double-add text's
        Ast_गाठ* wasUnderp = m_ftaskp;
        जर (!m_ftaskp) wasUnderp = m_procedurep;
        जर (wasUnderp) {
            जर (nodep->stmtsp()) {
                Ast_गाठ* स्थिरांक newp = नवीन AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    शून्य visit(AstTypedef* nodep) override {
        जर (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_गाठ(nodep);
        }
    }
    शून्य visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        जर (m_namedScope != "") {
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
        जर (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        जर (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        जर (nodep->user1SetOnce()) परत;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        स्थिरांक std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        जर (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Cover_Decl* nodep) override {
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
        जर (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } अन्यथा जर (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } अन्यथा जर (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, सत्य);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ* nodep) override {
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
    ~BeginVisitor() override = पूर्वनिर्धारित;
};

//######################################################################

वर्ग BeginRelinkVisitor final : सार्वजनिक VNVisitorConst {
    // Replace tasks with new pointer
खाजगी:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        जर (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstVarRef* nodep) override {
        जर (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        जर (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Task class functions

शून्य V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        जर (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("begin", 0, dump_वृक्ष_Either_Level() >= 3);
}

स्थिर Ast_गाठ* createForeachLoop(Ast_गाठ_Foreach* /*nodep*/, Ast_गाठ* bodysp, तार्किक arrayMayResize,
                                  Ast_गाठ_अभिव्यक्ती* subfromp, AstVar* varp, Ast_गाठ_अभिव्यक्ती* leftp,
                                  Ast_गाठ_अभिव्यक्ती* rightp, VNType गाठ_Type) {
    File_ओळ* स्थिरांक fl = varp->fileline();
    Ast_गाठ_अभिव्यक्ती* varRefp = नवीन AstVarRef{fl, varp, VAccess::READ};
    Ast_गाठ_अभिव्यक्ती* condp;
    तार्किक inc = सत्य;
    निवड (गाठ_Type) {
    स्थिती VNType::LteS: condp = नवीन AstLteS{fl, varRefp, rightp}; थांबा;
    स्थिती VNType::Lt: condp = नवीन AstLt{fl, varRefp, rightp}; थांबा;
    स्थिती VNType::GteS:
        condp = नवीन AstGteS{fl, varRefp, rightp};
        inc = असत्य;
        थांबा;
    पूर्वनिर्धारित: varp->v3fatalSrc("Missing comparison handling"); थांबा;
    }
    Ast_गाठ_अभिव्यक्ती* incp;
    जर (inc)
        incp = नवीन AstAdd{fl, varRefp->clone_वृक्ष(असत्य), नवीन AstConst{fl, 1}};
    अन्यथा
        incp = नवीन AstSub{fl, varRefp->clone_वृक्ष(असत्य), नवीन AstConst{fl, 1}};

    AstLoop* स्थिरांक loopp = नवीन AstLoop{fl};
    loopp->addStmtsp(नवीन AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    Ast_गाठ_अभिव्यक्ती* sizeGetp = nullptr;
    जर (arrayMayResize) {
        sizeVarp = नवीन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(सत्य);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = नवीन AstCMethodHard{fl, subfromp->clone_वृक्ष_Pure(असत्य), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(नवीन AstAssign{fl, नवीन AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    Ast_गाठ_Stmt* incStmtp = नवीन AstAssign{fl, नवीन AstVarRef{fl, varp, VAccess::WRITE}, incp};
    जर (arrayMayResize) {
        incStmtp = नवीन AstIf{fl,
                             नवीन AstLte{fl, नवीन AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->clone_वृक्ष_Pure(असत्य)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_गाठ* स्थिरांक stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(नवीन AstAssign{fl, नवीन AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    परत stmtsp;
}
स्थिर Ast_गाठ* createForeachLoopRanged(Ast_गाठ_Foreach* nodep, Ast_गाठ* bodysp,
                                        Ast_गाठ_अभिव्यक्ती* subfromp, AstVar* varp,
                                        स्थिरांक VNumRange& declRange) {
    File_ओळ* स्थिरांक fl = varp->fileline();
    V3_संख्या डावा{nodep, 32};
    V3_संख्या उजवा{nodep, 32};
    डावा.isSigned(सत्य);
    उजवा.isSigned(सत्य);
    डावा.setLongS(declRange.डावा());
    उजवा.setLongS(declRange.उजवा());
    Ast_गाठ_अभिव्यक्ती* स्थिरांक leftp = नवीन AstConst{fl, डावा};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक rightp = नवीन AstConst{fl, उजवा};
    परत createForeachLoop(nodep, bodysp, असत्य /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.डावा() <= declRange.उजवा() ? VNType::LteS : VNType::GteS);
}
स्थिर Ast_गाठ* createForeachAssoc(File_ओळ* fl, AstVar* varp, Ast_गाठ_अभिव्यक्ती* subfromp,
                                   Ast_गाठ_DType* fromDtp, Ast_गाठ* bodyPointp) {
    Ast_गाठ* loopp = varp;
    AstVar* स्थिरांक next_varp  // Iterator containing next element (to handle mid-array delete)
        = नवीन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(सत्य);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* स्थिरांक more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = नवीन AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(सत्य);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    Ast_गाठ_अभिव्यक्ती* स्थिरांक firstp
        = नवीन AstCMethodHard{fl, subfromp->clone_वृक्ष_Pure(असत्य), VCMethod::ASSOC_FIRST,
                             नवीन AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_गाठ_अभिव्यक्ती* स्थिरांक nextp
        = नवीन AstCMethodHard{fl, subfromp->clone_वृक्ष_Pure(असत्य), VCMethod::ASSOC_NEXT,
                             नवीन AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(नवीन AstAssign{fl, नवीन AstVarRef{fl, more_varp, VAccess::WRITE},
                                 नवीन AstNeq{fl, नवीन AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* स्थिरांक lp = नवीन AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(नवीन AstLoopTest{fl, lp, नवीन AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(नवीन AstAssign{fl, नवीन AstVarRef{fl, varp, VAccess::WRITE},
                                नवीन AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(नवीन AstAssign{fl, नवीन AstVarRef{fl, more_varp, VAccess::WRITE},
                                नवीन AstNeq{fl, नवीन AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    परत loopp;
}

स्थिर तार्किक arrayMayResizeCheck(Ast_गाठ* nodesp, AstVar* fromVarp) {
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
    जर (!nodesp) परत असत्य;
    परत nodesp->existsAndNext([&](स्थिरांक Ast_गाठ* nodep) -> तार्किक {
        जर (स्थिरांक Ast_गाठ_Var_Ref* varrefp = VN_CAST(nodep, गाठ_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            जर (varrefp->access().isWriteOrRW()) {
                जर (!fromVarp) परत सत्य;
                जर (varrefp->varp() == fromVarp) परत सत्य;
            }
        }
        // No idea what a function might be doing
        जर (VN_IS(nodep, MethodCall) || VN_IS(nodep, गाठ_FTask_Ref)) परत सत्य;
        परत असत्य;
    });
}

Ast_गाठ* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    स्थिरांक AstForeachHeader* स्थिरांक headerp = nodep->headerp();
    Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* स्थिरांक fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_गाठ_DType* fromDtp = fromp->dtypep()->skipRefp();
    स्थिरांक तार्किक arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_गाठ* bodyPointp = नवीन AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, असत्य};
    Ast_गाठ* newp = nullptr;
    Ast_गाठ* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_गाठ_अभिव्यक्ती* subfromp = fromp->clone_वृक्ष_Pure(असत्य);
    // Major dimension first
    साठी (Ast_गाठ *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        स्थिरांक तार्किक empty = VN_IS(argsp, Empty);
        AstVar* स्थिरांक varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        जर (varp) varp->unlinkFrBack()->usedLoopIdx(सत्य);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_ओळ* स्थिरांक fl = argsp->fileline();
        जर (varp) {
            Ast_गाठ* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            जर (स्थिरांक Ast_गाठ_Array_DType* स्थिरांक adtypep = VN_CAST(fromDtp, गाठ_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } अन्यथा जर (स्थिरांक AstBasicDType* स्थिरांक adtypep = VN_CAST(fromDtp, BasicDType)) {
                जर (adtypep->isString()) {
                    AstConst* स्थिरांक leftp = नवीन AstConst{fl, 0};
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक rightp = नवीन AstLenN{fl, fromp->clone_वृक्ष_Pure(असत्य)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } अन्यथा {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } अन्यथा जर (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* स्थिरांक leftp = नवीन AstConst{fl, 0};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक rightp = नवीन AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), गाठ_Array_DType)
                        ? नवीन AstArraySel{fl, subfromp->clone_वृक्ष_Pure(असत्य),
                                          नवीन AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_वृक्ष_Pure(असत्य),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = नवीन AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(असत्य);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = नवीन AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } अन्यथा जर (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = नवीन AstVarRef{fl, varp, VAccess::READ};
                subfromp = नवीन AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            जर (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_वृक्ष(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_गाठ* स्थिरांक bodyp = nodep->bodyp();
    जर (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        जर (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_वृक्ष(), bodyPointp);
        परत nullptr;
    }
    जर (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } अन्यथा {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_वृक्ष(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    परत newp;
}
